/*
 * bpshd_session.c: one bpshd shell session (see bpshd_session.h).
 *
 * A session is a persistent /bin/sh child whose stdin/stdout/stderr are
 * wired to pipes the daemon owns, plus a pipe at child fd 3 carrying
 * forwarded user stdin and a control pipe at child fd 4.  Each command
 * runs as "{ cmd ; } <&3 4>&-" (or a subshell when forwarding stdin),
 * then echoes its exit status ($?) to fd 4.  The daemon streams the
 * shell's stdout/stderr verbatim (no in-band markers, no holdback) and
 * treats the exit code arriving on fd 4 as the command boundary: once
 * it reads the rc line, it drains any remaining stdout/stderr and
 * reports EXIT.  (All of the command's output is in the pipes before
 * the shell writes the rc, since those writes are sequenced after the
 * command has exited and flushed.)
 */

#include <bp.h>
#include <sys/select.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include "bpsh_proto.h"
#include "bpshd_session.h"

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#define BPSHD_OUTPUT_LIMIT  (16 * 1024 * 1024)
#define BPSHD_READ_CHUNK    (16 * 1024)
/*	Cap on a single output bundle's payload, so one bundle always fits
 *	the outbound ZCO budget (which the attendant blocks against).	*/
#define BPSHD_MAX_PAYLOAD   (16 * 1024)
/*	Default wall-clock limit for a single command before it (and its
 *	shell) is killed; overridable via env BPSHD_CMD_TIMEOUT (seconds,
 *	0 disables).							*/
#define BPSHD_CMD_TIMEOUT_MS (300 * 1000)

struct BpshSession
{
	BpSAP sap;	    /* endpoint replies are sent on		*/
	char *sourceEid;
	uvast sessionId;

	pid_t shellPid;
	int   stdinFd;	    /* parent writes commands here  (child fd 0)	*/
	int   stdoutFd;	    /* parent reads child stdout    (child fd 1)	*/
	int   stderrFd;	    /* parent reads child stderr    (child fd 2)	*/
	int   userStdinFd;  /* parent writes user stdin   -> child fd 3	*/
	int   ctlFd;	    /* parent reads exit codes from  child fd 4	*/
	int   userStdinReq; /* client requested stdin forwarding (INIT)	*/

	uvast replySeq;	    /* per-session monotonic outbound seq		*/
	int   shellAlive;
};

static void closeFd(int *fd)
{
	if (*fd >= 0)
	{
		close(*fd);
		*fd = -1;
	}
}

BpshSession *bpshSessionOpen(BpSAP sap, const char *sourceEid, uvast sessionId,
		int wantStdin)
{
	BpshSession *s;
	int	     inPipe[2] = { -1, -1 };
	int	     outPipe[2] = { -1, -1 };
	int	     errPipe[2] = { -1, -1 };
	int	     dataPipe[2] = { -1, -1 };
	int	     ctlPipe[2] = { -1, -1 };
	pid_t	     pid;
	size_t	     eidLen;

	if (pipe(inPipe) < 0)
	{
		goto fail;
	}

	if (pipe(outPipe) < 0)
	{
		goto fail;
	}

	if (pipe(errPipe) < 0)
	{
		goto fail;
	}

	if (pipe(dataPipe) < 0)
	{
		goto fail;
	}

	if (pipe(ctlPipe) < 0)
	{
		goto fail;
	}

	pid = fork();
	if (pid < 0)
	{
		goto fail;
	}

	if (pid == 0)
	{
		/*	Child: wire pipes to fd 0/1/2, the user-stdin pipe to
		 *	fd 3 (read via "<&3"), and the control pipe to fd 4
		 *	(exit codes written via ">&4").  Not interactive: we
		 *	drive it by writing commands plus an rc printf.	*/
		dup2(inPipe[0], 0);
		dup2(outPipe[1], 1);
		dup2(errPipe[1], 2);
		dup2(dataPipe[0], 3);
		dup2(ctlPipe[1], 4);
		close(inPipe[0]);
		close(inPipe[1]);
		close(outPipe[0]);
		close(outPipe[1]);
		close(errPipe[0]);
		close(errPipe[1]);
		close(dataPipe[0]);
		close(dataPipe[1]);
		close(ctlPipe[0]);
		close(ctlPipe[1]);
		/*	Lead our own process group so a later kill(-pgid)
		 *	tears down the shell and whatever command it's
		 *	running together (used to abort a stalled command
		 *	when the client reconnects).			*/
		setpgid(0, 0);
		execl("/bin/sh", "sh", (char *) NULL);
		_exit(127);
	}

	/*	Parent: keep the ends we'll use, close the rest.	*/
	close(inPipe[0]);
	close(outPipe[1]);
	close(errPipe[1]);
	close(dataPipe[0]);
	close(ctlPipe[1]);

	/*	If client did not request stdin forwarding, close our
	 *	write end so the child sees EOF on fd 3 immediately
	 *	(prevents stdin-reading commands from hanging in REPL).	*/
	if (!wantStdin)
	{
		close(dataPipe[1]);
		dataPipe[1] = -1;
	}

	s = MTAKE(sizeof(BpshSession));
	if (s == NULL)
	{
		kill(pid, SIGKILL);
		waitpid(pid, NULL, 0);
		close(inPipe[1]);
		close(outPipe[0]);
		close(errPipe[0]);
		close(ctlPipe[0]);
		if (dataPipe[1] >= 0)
		{
			close(dataPipe[1]);
		}

		return NULL;
	}

	memset(s, 0, sizeof(*s));
	eidLen = strlen(sourceEid);
	s->sourceEid = MTAKE(eidLen + 1);
	if (s->sourceEid == NULL)
	{
		MRELEASE(s);
		kill(pid, SIGKILL);
		waitpid(pid, NULL, 0);
		close(inPipe[1]);
		close(outPipe[0]);
		close(errPipe[0]);
		close(ctlPipe[0]);
		if (dataPipe[1] >= 0)
		{
			close(dataPipe[1]);
		}

		return NULL;
	}

	/*	Read stdout/stderr non-blocking so we can drain them to
	 *	empty once the command's rc arrives on the control pipe.	*/
	oK(fcntl(outPipe[0], F_SETFL, O_NONBLOCK));
	oK(fcntl(errPipe[0], F_SETFL, O_NONBLOCK));

	memcpy(s->sourceEid, sourceEid, eidLen + 1);
	s->sap = sap;
	s->sessionId = sessionId;
	s->shellPid = pid;
	s->stdinFd = inPipe[1];
	s->stdoutFd = outPipe[0];
	s->stderrFd = errPipe[0];
	s->userStdinFd = dataPipe[1];
	s->ctlFd = ctlPipe[0];
	s->userStdinReq = wantStdin ? 1 : 0;
	s->replySeq = 0;
	s->shellAlive = 1;
	return s;

fail:
	if (inPipe[0] >= 0)
		close(inPipe[0]);
	if (inPipe[1] >= 0)
		close(inPipe[1]);
	if (outPipe[0] >= 0)
		close(outPipe[0]);
	if (outPipe[1] >= 0)
		close(outPipe[1]);
	if (errPipe[0] >= 0)
		close(errPipe[0]);
	if (errPipe[1] >= 0)
		close(errPipe[1]);
	if (dataPipe[0] >= 0)
		close(dataPipe[0]);
	if (dataPipe[1] >= 0)
		close(dataPipe[1]);
	if (ctlPipe[0] >= 0)
		close(ctlPipe[0]);
	if (ctlPipe[1] >= 0)
		close(ctlPipe[1]);
	return NULL;
}

void bpshSessionClose(BpshSession *s)
{
	int i;
	int status;

	if (s == NULL)
	{
		return;
	}

	closeFd(&s->stdinFd);
	closeFd(&s->stdoutFd);
	closeFd(&s->stderrFd);
	closeFd(&s->userStdinFd);
	closeFd(&s->ctlFd);

	if (s->shellPid > 0)
	{
		kill(s->shellPid, SIGTERM);
		/*	Brief wait, then SIGKILL if still alive.	*/
		for (i = 0; i < 50; i++)
		{
			if (waitpid(s->shellPid, &status, WNOHANG) != 0)
			{
				goto reaped;
			}

			usleep(20000);
		}

		kill(s->shellPid, SIGKILL);
		waitpid(s->shellPid, &status, 0);
	}

reaped:
	MRELEASE(s->sourceEid);
	MRELEASE(s);
}

void bpshSessionSendCwd(BpshSession *s)
{
	char	proc[64];
	char	path[PATH_MAX];
	ssize_t n;

	/*	The shell process's cwd reflects any "cd" it has run.  Read
	 *	it via /proc (Linux: .../cwd; Solaris: .../path/cwd).  If
	 *	neither is available, silently skip -- the client just
	 *	won't show a path.					*/
	isprintf(proc, sizeof proc, "/proc/%d/cwd", (int) s->shellPid);
	n = readlink(proc, path, sizeof path - 1);
	if (n < 0)
	{
		isprintf(proc, sizeof proc, "/proc/%d/path/cwd",
				(int) s->shellPid);
		n = readlink(proc, path, sizeof path - 1);
	}

	if (n < 0)
	{
		return;
	}

	path[n] = '\0';
	bpsh_send_bytes(s->sap, s->sourceEid, BpshMsgCwd, s->sessionId,
			s->replySeq++, (unsigned char *) path, (size_t) n);
}

/*	writeAll: write the full buffer or fail.  Returns 0 on success,
 *	-1 on error (including EPIPE if the shell died).		*/
static int writeAll(int fd, const char *buf, size_t len)
{
	while (len > 0)
	{
		ssize_t n = write(fd, buf, len);
		if (n < 0)
		{
			if (errno == EINTR)
			{
				continue;
			}

			return -1;
		}

		buf += n;
		len -= (size_t) n;
	}

	return 0;
}

/*	Monotonic clock, milliseconds, for the command timeout.	*/
static long monoMs(void)
{
	struct timespec ts;

	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (long) (ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

/*	While a command is running we still need to service incoming
 *	bundles -- STDIN_CHUNK / STDIN_EOF for this session are handled
 *	inline (written to / closed on userStdinFd); anything else is
 *	handed to the daemon's defer callback for replay once the command
 *	returns.							*/
static int pollIncoming(BpshSession *s, const int *running, BpshDeferFn defer,
		void *ctx)
{
	while (*running)
	{
		BpshFrame      frame;
		unsigned char *bytes;
		size_t	       bytesLen;
		char	       srcEid[BPSH_MAX_EID];
		int	       rc;

		rc = bpsh_recv_frame(s->sap, BP_POLL, BPSHD_RECV_BUFSIZE, &frame,
				&bytes, &bytesLen, srcEid, sizeof srcEid);
		if (rc == BPSH_RECV_FATAL)
		{
			return -1;
		}

		if (rc == BPSH_RECV_NONE)
		{
			return 0;
		}

		if (rc == BPSH_RECV_SKIP)
		{
			continue;
		}

		/*	Inline handling for stdin frames belonging to this
		 *	session.					*/
		if (frame.sessionId == s->sessionId
				&& strcmp(srcEid, s->sourceEid) == 0)
		{
			if (frame.msgType == BpshMsgStdinChunk)
			{
				if (s->userStdinFd >= 0 && frame.payloadLen > 0)
				{
					if (writeAll(s->userStdinFd,
							    (const char *) frame.payload,
							    frame.payloadLen)
							< 0)
					{
						closeFd(&s->userStdinFd);
					}
				}

				MRELEASE(bytes);
				continue;
			}

			if (frame.msgType == BpshMsgStdinEof)
			{
				closeFd(&s->userStdinFd);
				MRELEASE(bytes);
				continue;
			}
		}

		/*	A fresh INIT from the same client while a command is
		 *	still running here means the client is reconnecting --
		 *	typically to recover from a stalled command.  Kill the
		 *	shell's process group so this runCommand unwinds; the
		 *	INIT, deferred below, then replaces the session.	*/
		if (frame.msgType == BpshMsgInit
				&& strcmp(srcEid, s->sourceEid) == 0
				&& s->shellPid > 0)
		{
			kill(-s->shellPid, SIGKILL);
		}

		/*	Anything else is the daemon's to route.		*/
		if (defer != NULL)
		{
			defer(ctx, bytes, bytesLen, srcEid);
		}

		MRELEASE(bytes);
	}

	return -1;
}

/*	Send buf[0..len) to the client as one or more frames of the given
 *	type, each no larger than BPSHD_MAX_PAYLOAD.  The sequence number
 *	is advanced only after a frame is actually sent, so a failed send
 *	never leaves a gap that would stall the client's in-order reorder
 *	buffer.  Returns 0 if all bytes were sent, -1 otherwise.	*/
static int sendStream(BpshSession *s, BpshMsgType type, unsigned char *buf,
		size_t len)
{
	size_t off = 0;

	while (off < len)
	{
		size_t n = len - off;

		if (n > BPSHD_MAX_PAYLOAD)
		{
			n = BPSHD_MAX_PAYLOAD;
		}

		if (bpsh_send_bytes(s->sap, s->sourceEid, type, s->sessionId,
				    s->replySeq, buf + off, n)
				< 0)
		{
			return -1;
		}

		s->replySeq++;
		off += n;
	}

	return 0;
}

/*	Read all currently-available bytes from a non-blocking fd and
 *	stream them to the client.  Called once a command's rc has arrived
 *	to flush whatever output is still buffered in the pipe -- the
 *	command has exited, so the pipe holds exactly the remaining output
 *	and nothing more.  Returns 0, or -1 on send error.		*/
static int drainStream(BpshSession *s, int fd, BpshMsgType type)
{
	unsigned char chunk[BPSHD_READ_CHUNK];
	ssize_t	      r;

	while ((r = read(fd, chunk, sizeof chunk)) > 0)
	{
		if (sendStream(s, type, chunk, (size_t) r) < 0)
		{
			return -1;
		}
	}

	return 0;
}

/*	Per-command wall-clock timeout in ms (0 = disabled).  Read once
 *	from BPSHD_CMD_TIMEOUT (seconds), else the compiled-in default.	*/
static long cmdTimeoutMs(void)
{
	static int  inited = 0;
	static long ms = BPSHD_CMD_TIMEOUT_MS;

	if (!inited)
	{
		const char *e = getenv("BPSHD_CMD_TIMEOUT");

		if (e != NULL)
		{
			ms = atol(e) * 1000;	/* 0 or negative disables	*/
		}

		inited = 1;
	}

	return ms;
}

/*	Kill the session's process group (command + shell) and mark it
 *	dead -- used for the timeout and output-cap.			*/
static void killSession(BpshSession *s, BpshExitCause *cause, BpshExitCause why)
{
	if (s->shellPid > 0)
	{
		kill(-s->shellPid, SIGKILL);
	}

	s->shellAlive = 0;
	*cause = why;
}

int bpshSessionRun(BpshSession *s, const char *cmdline, const int *running,
		BpshDeferFn defer, void *ctx)
{
	char	      *postscript;
	size_t	       postCap;
	int	       postLen;
	int	       exitCode = 0;
	BpshExitCause  cause = BpshCauseNormal;
	int	       outEof = 0, errEof = 0;
	char	       ctlBuf[32];
	size_t	       ctlLen = 0;
	size_t	       totalOut = 0;
	long	       cmdStartMs;
	const char    *cmdOpen;
	const char    *cmdClose;

	postCap = strlen(cmdline) + 256;
	postscript = MTAKE(postCap);
	if (postscript == NULL)
	{
		return bpsh_send_exit(s->sap, s->sourceEid, s->sessionId,
				s->replySeq++, -1, BpshCauseKilled);
	}

	/*	Run the command with stdin from the user-stdin pipe (fd 3)
	 *	and the control pipe (fd 4) closed; snapshot its exit code,
	 *	echo it to fd 4, then restore $? via "(exit $rc)" so the
	 *	next command's first "$?" still sees the original code (the
	 *	echo would otherwise have reset it to 0).  "{ ...; }" in
	 *	REPL sessions so cd/export persist; "( ... )" (a subshell)
	 *	when forwarding stdin, so a user "exit N" ends only the
	 *	subshell, not the driver shell.				*/
	cmdOpen = s->userStdinReq ? "( " : "{ ";
	cmdClose = s->userStdinReq ? " ) <&3 4>&-" : " ; } <&3 4>&-";
	postLen = _isprintf(postscript, postCap,
			"%s%s%s\n"
			"__bpsh_rc=$?\n"
			"echo \"$__bpsh_rc\" >&4\n"
			"(exit $__bpsh_rc)\n",
			cmdOpen, cmdline, cmdClose);
	if (postLen <= 0 || writeAll(s->stdinFd, postscript, (size_t) postLen) < 0)
	{
		MRELEASE(postscript);
		s->shellAlive = 0;
		return bpsh_send_exit(s->sap, s->sourceEid, s->sessionId,
				s->replySeq++, -1, BpshCauseKilled);
	}

	MRELEASE(postscript);

	cmdStartMs = monoMs();

	while (1)
	{
		fd_set	       rfds;
		int	       maxFd = -1;
		int	       nready;
		long	       now;
		struct timeval tv;
		unsigned char  chunk[BPSHD_READ_CHUNK];
		ssize_t	       r;

		FD_ZERO(&rfds);
		FD_SET(s->stdoutFd, &rfds);
		maxFd = s->stdoutFd;
		FD_SET(s->stderrFd, &rfds);
		if (s->stderrFd > maxFd)
		{
			maxFd = s->stderrFd;
		}

		if (s->ctlFd >= 0)
		{
			FD_SET(s->ctlFd, &rfds);
			if (s->ctlFd > maxFd)
			{
				maxFd = s->ctlFd;
			}
		}

		/*	Wake at least every 100 ms to poll BP and re-check
		 *	the wall-clock timeout.				*/
		tv.tv_sec = 0;
		tv.tv_usec = 100 * 1000;
		nready = select(maxFd + 1, &rfds, NULL, NULL, &tv);
		if (nready < 0)
		{
			if (errno == EINTR)
			{
				continue;
			}

			s->shellAlive = 0;
			cause = BpshCauseKilled;
			break;
		}

		now = monoMs();
		if (cmdTimeoutMs() > 0 && now - cmdStartMs >= cmdTimeoutMs())
		{
			killSession(s, &cause, BpshCauseTimeout);
			break;
		}

		/*	Stream stdout/stderr verbatim as it arrives.	*/
		if (FD_ISSET(s->stdoutFd, &rfds))
		{
			r = read(s->stdoutFd, chunk, sizeof chunk);
			if (r > 0)
			{
				sendStream(s, BpshMsgStdout, chunk, (size_t) r);
				totalOut += (size_t) r;
			}
			else if (r == 0 || (errno != EAGAIN && errno != EWOULDBLOCK))
			{
				outEof = 1;
			}
		}

		if (FD_ISSET(s->stderrFd, &rfds))
		{
			r = read(s->stderrFd, chunk, sizeof chunk);
			if (r > 0)
			{
				sendStream(s, BpshMsgStderr, chunk, (size_t) r);
				totalOut += (size_t) r;
			}
			else if (r == 0 || (errno != EAGAIN && errno != EWOULDBLOCK))
			{
				errEof = 1;
			}
		}

		if (totalOut > BPSHD_OUTPUT_LIMIT)
		{
			killSession(s, &cause, BpshCauseOutputLimit);
			break;
		}

		/*	The command's exit code arriving on fd 4 is the
		 *	command boundary.				*/
		if (s->ctlFd >= 0 && FD_ISSET(s->ctlFd, &rfds))
		{
			r = read(s->ctlFd, ctlBuf + ctlLen,
					sizeof ctlBuf - 1 - ctlLen);
			if (r > 0)
			{
				ctlLen += (size_t) r;
				ctlBuf[ctlLen] = '\0';
				if (memchr(ctlBuf, '\n', ctlLen) != NULL)
				{
					exitCode = (int) strtol(ctlBuf, NULL, 10);
					break;
				}
			}
			else if (r == 0)
			{
				closeFd(&s->ctlFd);
			}
		}

		/*	Both data streams closing without an rc means the
		 *	driver shell itself exited (e.g. the user ran "exit").	*/
		if (outEof && errEof)
		{
			s->shellAlive = 0;
			cause = BpshCauseKilled;
			break;
		}

		/*	Service stdin / interrupt / reconnect bundles.	*/
		pollIncoming(s, running, defer, ctx);
	}

	/*	Drain whatever output is still buffered in the pipes (the
	 *	command's writes all precede its rc, so this captures the
	 *	rest), then report EXIT.				*/
	drainStream(s, s->stdoutFd, BpshMsgStdout);
	drainStream(s, s->stderrFd, BpshMsgStderr);

	/*	Report the (possibly changed) cwd before EXIT so the client
	 *	can refresh its prompt.  Skipped if the shell has died.	*/
	if (s->shellAlive)
	{
		bpshSessionSendCwd(s);
	}

	return bpsh_send_exit(s->sap, s->sourceEid, s->sessionId, s->replySeq++,
			exitCode, cause);
}

const char *bpshSessionEid(const BpshSession *s)
{
	return s->sourceEid;
}

uvast bpshSessionId(const BpshSession *s)
{
	return s->sessionId;
}

int bpshSessionAlive(const BpshSession *s)
{
	return s->shellAlive;
}

uvast bpshSessionNextSeq(BpshSession *s)
{
	return s->replySeq++;
}
