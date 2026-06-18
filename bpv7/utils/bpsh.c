/*
 * bpsh.c: SSH-like remote shell client over Bundle Protocol.
 *
 * Interactive REPL (default) or one-shot mode (-c <cmd>, which exits
 * with the remote command's exit code and forwards stdin when it is
 * not a TTY).  Each command line is sent as one REQ; the per-session
 * reorder buffer delivers the server's STDOUT / STDERR / CWD / EXIT
 * frames in sequence order, while ERROR frames bypass ordering as
 * out-of-band control.
 *
 * The REPL offers raw-mode line editing (cursor movement, command
 * history on the arrow keys and a "history" builtin) and shows the
 * remote shell's working directory in the prompt.
 */

#include <bp.h>
#include <pthread.h>
#include <unistd.h>
#include <poll.h>
#include <termios.h>
#include "bpsh_proto.h"

#define BPSH_LINE_BUFSIZE (8 * 1024)
#define BPSH_MAX_BUNDLE	  (16 * 1024 * 1024)
#define BPSH_STDIN_CHUNK  (32 * 1024)
#define BPSH_CWD_MAX	  4096
#define BPSH_HIST_MAX	  1000

static BpSAP	       sap;
static char	      *ownEid;
static char	      *remoteEid;
static char	       currentCwd[BPSH_CWD_MAX]; /* shell cwd from CWD frames */
static uvast	       sessionId;
static uvast	       sendSeq = 0;
static int	       running = 1;
static pthread_mutex_t sendMutex = PTHREAD_MUTEX_INITIALIZER;

/*	Frame received from the wire, decoded, and held until the
 *	session's next-expected seq matches.  Bytes are owned by the
 *	struct (frame.payload aliases into them).			*/
typedef struct PendingFrame
{
	BpshFrame	     frame;
	unsigned char	    *bytes;
	size_t		     bytesLen;
	struct PendingFrame *next;
} PendingFrame;

static PendingFrame *pendingHead = NULL;
static uvast	     nextRecvSeq = 0;

/*	In-memory command history (oldest first), for up/down recall and
 *	the "history" builtin.						*/
static char	   **history;
static int	     historyCount;
static int	     historyCap;

/*	If the local ION node is torn down (e.g. its shared memory is
 *	removed) while bpsh is running, the next bp_send/SDR call faults
 *	on detached shared memory.  Convert that into a clean exit with a
 *	clear message rather than a bare "Segmentation fault".  Uses only
 *	async-signal-safe calls.					*/
static void handleFault(int signum)
{
	static const char msg[] = "\nbpsh: lost the local ION node "
			"(memory fault); exiting.\n";

	(void) signum;
	if (write(STDERR_FILENO, msg, sizeof msg - 1) < 0)
	{
		/*	Nothing useful to do if even this fails.	*/
	}

	_exit(1);
}

static void handleQuit(int signum)
{
	(void) signum;
	running = 0;
	if (sap)
	{
		bp_interrupt(sap);
	}
}

/*	Build a frame for our session and ship it.  The send mutex makes
 *	seq assignment atomic with the send so the REPL thread and the
 *	one-shot stdin forwarder can't interleave their outbound frames.	*/
static int sendTyped(BpshMsgType type, unsigned char *payload, size_t len)
{
	BpshFrame out;
	int	  rc;

	memset(&out, 0, sizeof(out));
	out.version = BPSH_VERSION;
	out.msgType = type;
	out.sessionId = sessionId;
	out.payload = payload;
	out.payloadLen = len;

	pthread_mutex_lock(&sendMutex);
	out.seqNum = sendSeq++;
	rc = bpsh_send_frame(sap, remoteEid, &out);
	pthread_mutex_unlock(&sendMutex);
	return rc;
}

static int sendInit(int wantStdin)
{
	unsigned char flagByte = 0x01;

	return sendTyped(BpshMsgInit, wantStdin ? &flagByte : NULL,
			wantStdin ? 1 : 0);
}

static int sendStdinChunk(unsigned char *bytes, size_t len)
{
	return sendTyped(BpshMsgStdinChunk, bytes, len);
}

static int sendStdinEof(void)
{
	return sendTyped(BpshMsgStdinEof, NULL, 0);
}

static int sendReq(char *line, size_t len)
{
	return sendTyped(BpshMsgReq, (unsigned char *) line, len);
}

static int sendExitSession(void)
{
	return sendTyped(BpshMsgExitSession, NULL, 0);
}

static void freePending(PendingFrame *p)
{
	if (p == NULL)
	{
		return;
	}

	if (p->bytes)
	{
		MRELEASE(p->bytes);
	}

	MRELEASE(p);
}

/*	Receive one bundle (blocking), decode it, and return a heap
 *	PendingFrame whose .bytes buffer owns the raw bundle bytes.
 *	Returns NULL on interrupt or fatal error.			*/
static PendingFrame *recvOnePending(void)
{
	while (running)
	{
		BpshFrame      frame;
		unsigned char *bytes;
		size_t	       bytesLen;
		PendingFrame  *p;
		int	       rc;

		rc = bpsh_recv_frame(sap, BP_BLOCKING, BPSH_MAX_BUNDLE, &frame,
				&bytes, &bytesLen, NULL, 0);
		if (rc == BPSH_RECV_FATAL)
		{
			return NULL;
		}

		if (rc == BPSH_RECV_NONE)
		{
			if (!running)
			{
				return NULL;
			}

			continue;
		}

		if (rc == BPSH_RECV_SKIP)
		{
			continue;
		}

		p = MTAKE(sizeof(PendingFrame));
		if (p == NULL)
		{
			MRELEASE(bytes);
			continue;
		}

		memset(p, 0, sizeof(*p));
		p->frame = frame;
		p->bytes = bytes;
		p->bytesLen = bytesLen;
		return p;
	}

	return NULL;
}

/*	Pull the next ordered frame for our session.  ERROR frames
 *	bypass ordering and are returned immediately.  Frames for other
 *	sessions are dropped.  Returns NULL on interrupt / shutdown.
 *	Caller must freePending() the result.				*/
static PendingFrame *recvOrderedFrame(void)
{
	while (running)
	{
		PendingFrame  *p;
		PendingFrame **link;

		if (pendingHead != NULL
				&& pendingHead->frame.seqNum == nextRecvSeq)
		{
			p = pendingHead;
			pendingHead = p->next;
			p->next = NULL;
			nextRecvSeq++;
			return p;
		}

		p = recvOnePending();
		if (p == NULL)
		{
			return NULL;
		}

		if (p->frame.sessionId != sessionId)
		{
			freePending(p);
			continue;
		}

		if (p->frame.msgType == BpshMsgError)
		{
			/*	Out-of-band: deliver immediately.	*/
			return p;
		}

		if (p->frame.seqNum < nextRecvSeq)
		{
			/*	Stale duplicate.			*/
			freePending(p);
			continue;
		}

		if (p->frame.seqNum == nextRecvSeq)
		{
			nextRecvSeq++;
			return p;
		}

		/*	Future seq: insert into sorted pending list.	*/
		link = &pendingHead;
		while (*link != NULL && (*link)->frame.seqNum < p->frame.seqNum)
		{
			link = &(*link)->next;
		}

		if (*link != NULL && (*link)->frame.seqNum == p->frame.seqNum)
		{
			/*	Already buffered.			*/
			freePending(p);
			continue;
		}

		p->next = *link;
		*link = p;
	}

	return NULL;
}

/*	Copy a CWD frame's payload into currentCwd, truncating safely.	*/
static void storeCwd(const unsigned char *payload, size_t len)
{
	if (len >= sizeof currentCwd)
	{
		len = sizeof currentCwd - 1;
	}

	memcpy(currentCwd, payload, len);
	currentCwd[len] = '\0';
}

/*	Wait for INIT_ACK matching our sessionId.  Returns 0 on success.	*/
static int awaitInitAck(void)
{
	while (running)
	{
		PendingFrame *p = recvOrderedFrame();

		if (p == NULL)
		{
			return -1;
		}

		if (p->frame.msgType == BpshMsgInitAck)
		{
			freePending(p);
			return 0;
		}

		if (p->frame.msgType == BpshMsgCwd)
		{
			storeCwd(p->frame.payload, p->frame.payloadLen);
			freePending(p);
			continue;
		}

		if (p->frame.msgType == BpshMsgError)
		{
			fprintf(stderr, "bpsh: server error: %.*s\n",
					(int) p->frame.payloadLen,
					p->frame.payload);
			freePending(p);
			return -1;
		}

		freePending(p);
	}

	return -1;
}

/*	Consume frames in seq order, writing STDOUT / STDERR chunks to
 *	their respective streams as they arrive, until EXIT.		*/
static int awaitCommandResult(int *exitCode, BpshExitCause *cause)
{
	while (running)
	{
		PendingFrame *p = recvOrderedFrame();

		if (p == NULL)
		{
			return -1;
		}

		switch (p->frame.msgType)
		{
		case BpshMsgStdout:
			fwrite(p->frame.payload, 1, p->frame.payloadLen, stdout);
			fflush(stdout);
			freePending(p);
			break;

		case BpshMsgStderr:
			fwrite(p->frame.payload, 1, p->frame.payloadLen, stderr);
			fflush(stderr);
			freePending(p);
			break;

		case BpshMsgCwd:
			storeCwd(p->frame.payload, p->frame.payloadLen);
			freePending(p);
			break;

		case BpshMsgExit:
			*exitCode = p->frame.exitCode;
			*cause = p->frame.exitCause;
			freePending(p);
			return 0;

		case BpshMsgError:
			fprintf(stderr, "bpsh: server error: %.*s\n",
					(int) p->frame.payloadLen,
					p->frame.payload);
			freePending(p);
			return -1;

		default:
			freePending(p);
			break;
		}
	}

	return -1;
}

/*	Decide whether to colour the prompt.  Honours NO_COLOR
 *	(https://no-color.org/) and skips when TERM is unset / "dumb".
 *	Stdout is checked because that's where the prompt is written.	*/
static int wantColorPrompt(void)
{
	const char *term;

	if (!isatty(fileno(stdout)))
	{
		return 0;
	}

	if (getenv("NO_COLOR") != NULL)
	{
		return 0;
	}

	term = getenv("TERM");
	if (term == NULL || *term == '\0' || strcmp(term, "dumb") == 0)
	{
		return 0;
	}

	return 1;
}

/*	Append a command to the history, dropping the oldest entry past
 *	BPSH_HIST_MAX and skipping immediate duplicates.		*/
static void historyAdd(const char *line)
{
	char *copy;

	if (historyCount > 0
			&& strcmp(history[historyCount - 1], line) == 0)
	{
		return;
	}

	copy = MTAKE(strlen(line) + 1);
	if (copy == NULL)
	{
		return;
	}

	strcpy(copy, line);

	if (historyCount == BPSH_HIST_MAX)
	{
		MRELEASE(history[0]);
		memmove(history, history + 1,
				(historyCount - 1) * sizeof(char *));
		historyCount--;
	}

	if (historyCount == historyCap)
	{
		int	newCap = historyCap == 0 ? 64 : historyCap * 2;
		char  **bigger = MTAKE(newCap * sizeof(char *));

		if (bigger == NULL)
		{
			MRELEASE(copy);
			return;
		}

		if (history != NULL)
		{
			memcpy(bigger, history,
					historyCount * sizeof(char *));
			MRELEASE(history);
		}

		history = bigger;
		historyCap = newCap;
	}

	history[historyCount++] = copy;
}

/*	Print the command history, numbered, like a shell's "history".	*/
static void historyPrint(void)
{
	int i;

	for (i = 0; i < historyCount; i++)
	{
		printf("%5d  %s\n", i + 1, history[i]);
	}

	fflush(stdout);
}

/*	Replace the edit buffer with src (cursor to end).		*/
static void loadLine(char *buf, size_t size, size_t *len, size_t *pos,
		const char *src)
{
	size_t n = strlen(src);

	if (n > size - 1)
	{
		n = size - 1;
	}

	memcpy(buf, src, n);
	buf[n] = '\0';
	*len = n;
	*pos = n;
}

/*	Repaint the prompt and current line, then park the cursor at the
 *	editing position.  The whole line is reprinted from column 0 so a
 *	coloured prompt and any post-cursor text stay correct; this assumes
 *	the line doesn't wrap, which is fine for typical command lengths.*/
static void redrawLine(const char *prompt, const char *buf, size_t len,
		size_t pos)
{
	printf("\r%s%.*s\033[K", prompt, (int) len, buf);
	if (pos < len)
	{
		printf("\033[%uD", (unsigned) (len - pos));
	}

	fflush(stdout);
}

/*	Read one line from a TTY with minimal in-line editing: printable
 *	bytes insert at the cursor, Backspace deletes, Left/Right (and
 *	Home/End) move within the line, and arrow Up/Down -- plus any other
 *	escape sequence -- are swallowed rather than echoed as text.  The
 *	prompt is drawn here.  On Enter the line (without the newline) is
 *	stored in buf and 0 is returned; -1 is returned on end-of-input
 *	(Ctrl+D on an empty line) or interrupt (Ctrl+C).		*/
static int readLineRaw(char *buf, size_t size, const char *prompt)
{
	struct termios orig;
	struct termios raw;
	size_t	       len = 0;
	size_t	       pos = 0;
	int	       ret = 0;
	int	       hpos = historyCount;	/* == count means the new line	*/
	char	       saved[BPSH_LINE_BUFSIZE];	/* in-progress line stash	*/

	saved[0] = '\0';

	if (tcgetattr(STDIN_FILENO, &orig) < 0)
	{
		/*	Not a real TTY after all: fall back to fgets.	*/
		printf("%s", prompt);
		fflush(stdout);
		return fgets(buf, (int) size, stdin) == NULL ? -1 : 0;
	}

	raw = orig;
	/*	Character-at-a-time, no echo; keep ISIG so Ctrl+C still
	 *	raises SIGINT and unwinds the session as before.	*/
	raw.c_lflag &= ~(ICANON | ECHO);
	raw.c_cc[VMIN] = 1;
	raw.c_cc[VTIME] = 0;
	tcsetattr(STDIN_FILENO, TCSANOW, &raw);

	buf[0] = '\0';
	redrawLine(prompt, buf, len, pos);

	while (running)
	{
		unsigned char c;
		ssize_t	      n = read(STDIN_FILENO, &c, 1);

		if (n < 0)
		{
			ret = -1;	/*	EINTR (Ctrl+C) or error.	*/
			break;
		}

		if (n == 0)		/*	stdin closed.			*/
		{
			ret = (len == 0) ? -1 : 0;
			break;
		}

		if (c == '\r' || c == '\n')
		{
			break;		/*	Line complete.			*/
		}

		if (c == 4)		/*	Ctrl+D.				*/
		{
			if (len == 0)
			{
				ret = -1;
				break;
			}

			continue;	/*	Ignore mid-line.		*/
		}

		if (c == 127 || c == 8)	/*	Backspace.			*/
		{
			if (pos > 0)
			{
				memmove(buf + pos - 1, buf + pos, len - pos);
				pos--;
				len--;
				buf[len] = '\0';
				redrawLine(prompt, buf, len, pos);
			}

			continue;
		}

		if (c == 27)		/*	Escape sequence.		*/
		{
			unsigned char seq0;
			unsigned char seq1;

			if (read(STDIN_FILENO, &seq0, 1) != 1)
			{
				continue;
			}

			if (seq0 != '[' && seq0 != 'O')
			{
				continue;	/*	Bare ESC / Alt-<key>.	*/
			}

			if (read(STDIN_FILENO, &seq1, 1) != 1)
			{
				continue;
			}

			if (seq1 >= '0' && seq1 <= '9')
			{
				/*	Extended seq (Del/Home/End/PgUp...):
				 *	consume through the final '~'.		*/
				unsigned char t;
				while (read(STDIN_FILENO, &t, 1) == 1 && t != '~')
				{
					;
				}

				if (seq1 == '3')			/* Delete */
				{
					if (pos < len)
					{
						memmove(buf + pos, buf + pos + 1,
								len - pos - 1);
						len--;
						buf[len] = '\0';
						redrawLine(prompt, buf, len, pos);
					}
				}
				else if (seq1 == '1' || seq1 == '7')	/* Home	*/
				{
					pos = 0;
					redrawLine(prompt, buf, len, pos);
				}
				else if (seq1 == '4' || seq1 == '8')	/* End	*/
				{
					pos = len;
					redrawLine(prompt, buf, len, pos);
				}

				continue;
			}

			switch (seq1)
			{
			case 'C':		/*	Right arrow.		*/
				if (pos < len)
				{
					pos++;
					redrawLine(prompt, buf, len, pos);
				}

				break;

			case 'D':		/*	Left arrow.		*/
				if (pos > 0)
				{
					pos--;
					redrawLine(prompt, buf, len, pos);
				}

				break;

			case 'H':		/*	Home.			*/
				pos = 0;
				redrawLine(prompt, buf, len, pos);
				break;

			case 'F':		/*	End.			*/
				pos = len;
				redrawLine(prompt, buf, len, pos);
				break;

			case 'A':		/*	Up: older history.	*/
				if (hpos > 0)
				{
					if (hpos == historyCount)
					{
						/*	Stash the in-progress
						 *	line to restore later.	*/
						memcpy(saved, buf, len);
						saved[len] = '\0';
					}

					hpos--;
					loadLine(buf, size, &len, &pos,
							history[hpos]);
					redrawLine(prompt, buf, len, pos);
				}

				break;

			case 'B':		/*	Down: newer history.	*/
				if (hpos < historyCount)
				{
					hpos++;
					loadLine(buf, size, &len, &pos,
							hpos == historyCount ?
								saved :
								history[hpos]);
					redrawLine(prompt, buf, len, pos);
				}

				break;

			default:		/*	Other CSI: ignore.	*/
				break;
			}

			continue;
		}

		if (c < 32)		/*	Other control bytes: ignore.	*/
		{
			continue;
		}

		/*	Printable byte: insert at the cursor.			*/
		if (len + 1 < size)
		{
			memmove(buf + pos + 1, buf + pos, len - pos);
			buf[pos] = (char) c;
			pos++;
			len++;
			buf[len] = '\0';
			redrawLine(prompt, buf, len, pos);
		}
	}

	tcsetattr(STDIN_FILENO, TCSANOW, &orig);
	buf[len] = '\0';
	putchar('\n');
	fflush(stdout);
	return running ? ret : -1;
}

/*	Compose the REPL prompt: bpsh#<local>@<remote>, with the shell's
 *	cwd appended ":<cwd>" once known -- the way a normal shell shows
 *	the working directory.  Coloured when the terminal supports it.	*/
static void buildPrompt(char *out, size_t size, int color)
{
	const char *cwd = currentCwd[0] != '\0' ? currentCwd : NULL;

	if (color)
	{
		/*	bright-cyan "bpsh#", green local EID, default "@",
		 *	bright-blue remote EID, yellow cwd.		*/
		if (cwd)
		{
			isprintf(out, (int) size,
					"\033[1;36mbpsh\033[0m#\033[32m%s\033[0m@"
					"\033[1;34m%s\033[0m:\033[33m%s\033[0m> ",
					ownEid, remoteEid, cwd);
		}
		else
		{
			isprintf(out, (int) size,
					"\033[1;36mbpsh\033[0m#\033[32m%s\033[0m@"
					"\033[1;34m%s\033[0m> ",
					ownEid, remoteEid);
		}
	}
	else if (cwd)
	{
		isprintf(out, (int) size, "bpsh#%s@%s:%s> ", ownEid, remoteEid,
				cwd);
	}
	else
	{
		isprintf(out, (int) size, "bpsh#%s@%s> ", ownEid, remoteEid);
	}
}

static int repl(void)
{
	char	      line[BPSH_LINE_BUFSIZE];
	char	      prompt[BPSH_CWD_MAX + 256];
	size_t	      len;
	int	      exitCode;
	BpshExitCause cause;
	int	      isTty = isatty(fileno(stdin));
	int	      color = isTty && wantColorPrompt();

	while (running)
	{
		buildPrompt(prompt, sizeof prompt, color);

		if (isTty)
		{
			/*	Raw-mode editor: arrow keys navigate / are
			 *	swallowed instead of being typed.		*/
			if (readLineRaw(line, sizeof line, prompt) < 0)
			{
				break;
			}
		}
		else if (fgets(line, sizeof(line), stdin) == NULL)
		{
			break;		/*	End of piped stdin.		*/
		}

		len = strlen(line);
		while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r'))
		{
			len--;
		}

		if (len == 0)
		{
			continue;
		}

		line[len] = '\0';
		historyAdd(line);

		/*	"history" is a client-side builtin: list the locally
		 *	kept command history instead of running it remotely.	*/
		if (strcmp(line, "history") == 0)
		{
			historyPrint();
			continue;
		}

		if (sendReq(line, len) < 0)
		{
			fprintf(stderr, "bpsh: failed to send REQ\n");
			return -1;
		}

		if (awaitCommandResult(&exitCode, &cause) < 0)
		{
			return -1;
		}

		if (cause == BpshCauseKilled)
		{
			if (isTty)
			{
				fprintf(stderr, "[bpsh: session closed]\n");
			}

			break;
		}

		if (cause != BpshCauseNormal)
		{
			fprintf(stderr,
					"[bpsh: command exited (%s)"
					" code=%d]\n",
					bpsh_cause_name(cause), exitCode);
		}
	}

	return 0;
}

/*	Forward bpsh's own stdin to the remote command via STDIN_CHUNK
 *	bundles, then send STDIN_EOF.  Used only in one-shot mode (-c)
 *	when stdin is not a TTY.					*/
static void *stdinForwarder(void *arg)
{
	unsigned char buf[BPSH_STDIN_CHUNK];

	(void) arg;

	while (running)
	{
		struct pollfd pfd;
		ssize_t       n;
		int           pr;

		/*	Poll with a short timeout rather than blocking in
		 *	read(), so that when the remote command exits the
		 *	main thread can clear `running` and we notice it and
		 *	stop, instead of blocking until our stdin is closed.	*/
		pfd.fd = STDIN_FILENO;
		pfd.events = POLLIN;
		pfd.revents = 0;
		pr = poll(&pfd, 1, 200);
		if (pr == 0)		/*	Timeout: re-check running.	*/
		{
			continue;
		}

		if (pr < 0)
		{
			if (errno == EINTR)
			{
				continue;
			}

			break;
		}

		n = read(STDIN_FILENO, buf, sizeof(buf));

		if (n < 0)
		{
			if (errno == EINTR)
			{
				continue;
			}

			break;
		}

		if (n == 0)
		{
			break;
		}

		if (sendStdinChunk(buf, (size_t) n) < 0)
		{
			break;
		}
	}

	sendStdinEof();
	return NULL;
}

static void usage(void)
{
	fprintf(stderr,
			"Usage: bpsh -h <remote EID> [-l <local EID>] [-c <cmd>]\n"
			"       bpsh <local EID> <remote EID>     (positional, REPL)\n"
			"\n"
			"  -h <remote EID>  remote bpshd endpoint (mandatory)\n"
			"  -l <local EID>   local endpoint for replies (mandatory)\n"
			"  -c <cmd>         run <cmd> as one-shot, exit with its rc.\n"
			"                   Stdin is forwarded if not a TTY.\n"
			"\n"
			"Without -c, runs an interactive REPL.\n");
}

int main(int argc, char **argv)
{
	const char   *cmd = NULL;
	int	      opt;
	int	      wantStdin;
	int	      exitCode = 0;
	BpshExitCause cause = BpshCauseNormal;
	pthread_t     forwarder;
	int	      forwarderStarted = 0;

	while ((opt = getopt(argc, argv, "h:l:c:")) != -1)
	{
		switch (opt)
		{
		case 'h':
			remoteEid = optarg;
			break;
		case 'l':
			ownEid = optarg;
			break;
		case 'c':
			cmd = optarg;
			break;
		default:
			usage();
			return 1;
		}
	}

	/*	Backward-compatible positional fallback.		*/
	if (ownEid == NULL && remoteEid == NULL && argc - optind >= 2)
	{
		ownEid = argv[optind];
		remoteEid = argv[optind + 1];
	}

	if (ownEid == NULL || remoteEid == NULL)
	{
		usage();
		return 1;
	}

	if (bp_attach() < 0)
	{
		putErrmsg("bpsh: bp_attach failed.", NULL);
		return 1;
	}

	if (bp_open(ownEid, &sap) < 0)
	{
		putErrmsg("bpsh: can't open local endpoint.", ownEid);
		bp_detach();
		return 1;
	}

	/*	Register SIGINT without SA_RESTART so that fgets in the
	 *	REPL returns EINTR on Ctrl+C and the loop exits cleanly.
	 *	signal() on glibc keeps SA_RESTART set, which would
	 *	silently swallow Ctrl+C at the prompt.			*/
	{
		struct sigaction sa;

		memset(&sa, 0, sizeof(sa));
		sa.sa_handler = handleQuit;
		sigemptyset(&sa.sa_mask);
		sa.sa_flags = 0;
		sigaction(SIGINT, &sa, NULL);

		/*	Now that we're attached to ION, a memory fault is
		 *	most likely the node being torn down underneath us;
		 *	report it cleanly instead of dumping core.	*/
		sa.sa_handler = handleFault;
		sigaction(SIGSEGV, &sa, NULL);
		sigaction(SIGBUS, &sa, NULL);
	}

	srand((unsigned int) (getpid() ^ time(NULL)));
	sessionId = ((uvast) rand() << 32) | (uvast) rand();

	/*	One-shot with non-TTY stdin = stdin forwarding mode.	*/
	wantStdin = (cmd != NULL && !isatty(STDIN_FILENO));

	if (sendInit(wantStdin) < 0)
	{
		fprintf(stderr, "bpsh: failed to send INIT\n");
		bp_close(sap);
		bp_detach();
		return 1;
	}

	if (awaitInitAck() < 0)
	{
		fprintf(stderr, "bpsh: failed to receive INIT_ACK\n");
		bp_close(sap);
		bp_detach();
		return 1;
	}

	if (cmd != NULL)
	{
		/*	One-shot: send REQ, optionally start stdin
		 *	forwarder, await result, exit with rc.		*/
		char *cmdCopy = MTAKE(strlen(cmd) + 1);

		if (cmdCopy == NULL)
		{
			fprintf(stderr, "bpsh: out of memory\n");
			exitCode = 1;
			goto done;
		}

		strcpy(cmdCopy, cmd);
		if (sendReq(cmdCopy, strlen(cmdCopy)) < 0)
		{
			fprintf(stderr, "bpsh: failed to send REQ\n");
			MRELEASE(cmdCopy);
			exitCode = 1;
			goto done;
		}

		MRELEASE(cmdCopy);

		if (wantStdin)
		{
			if (pthread_begin(&forwarder, NULL, stdinForwarder, NULL)
					< 0)
			{
				fprintf(stderr, "bpsh: stdin forwarder thread failed\n");
				exitCode = 1;
				goto done;
			}

			forwarderStarted = 1;
		}

		if (awaitCommandResult(&exitCode, &cause) < 0)
		{
			exitCode = 1;
		}

		if (forwarderStarted)
		{
			/*	The remote command has exited; stop forwarding
			 *	stdin so we don't block waiting for our stdin
			 *	to be closed.				*/
			running = 0;
			pthread_join(forwarder, NULL);
		}
	}
	else
	{
		repl();
	}

done:
	sendExitSession();
	bp_close(sap);
	bp_detach();
	return exitCode;
}
