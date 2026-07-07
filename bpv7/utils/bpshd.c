/*
 * bpshd.c: SSH-like remote shell daemon over Bundle Protocol.
 *
 * This file is the session manager: it attaches to BP, opens the
 * listen endpoint, owns the table of live client sessions, routes
 * decoded frames to them (INIT opens a session, REQ runs a command,
 * EXIT_SESSION closes one), and owns the queue of bundles deferred
 * while a command is running.  A single session's internals -- the
 * /bin/sh child, command execution and output streaming -- live in
 * the bpshd_session object (bpshd_session.c / .h).
 *
 * Still missing: bpsec gate, ionadmin configuration, idle-session
 * reaper.
 */

#include <bp.h>
#include "bpsh_proto.h"
#include "bpshd_session.h"

/*	Bundles received while a command was running that weren't that
 *	session's stdin; replayed through the dispatcher once it returns.*/
typedef struct DeferredBundle
{
	unsigned char	      *bytes;
	size_t		       bytesLen;
	char		      *sourceEid;
	struct DeferredBundle *next;
} DeferredBundle;

static BpSAP	      sap;
static int	      running = 1;
static BpshSession **sessions;
static int	      sessionCount = 0;
static int	      sessionCap = 0;
static DeferredBundle *deferredHead = NULL;
static DeferredBundle *deferredTail = NULL;
static ReqAttendant   attendant;	/* blocking transmission of output */
static int	      attendantStarted = 0;

static void handleQuit(int signum)
{
	(void) signum;
	running = 0;
	if (attendantStarted)
	{
		/*	Unblock a send that's waiting for ZCO space.	*/
		ionPauseAttendant(&attendant);
	}

	if (sap)
	{
		bp_interrupt(sap);
	}
}

/*	Deferred-bundle FIFO.  Append a copy of bytes / sourceEid; return
 *	0 on success, -1 on alloc failure.				*/
static int enqueueDeferred(const unsigned char *bytes, size_t bytesLen,
		const char *sourceEid)
{
	DeferredBundle *d;
	size_t		eidLen;

	d = MTAKE(sizeof(DeferredBundle));
	if (d == NULL)
	{
		return -1;
	}

	memset(d, 0, sizeof(*d));

	d->bytes = MTAKE(bytesLen);
	if (d->bytes == NULL)
	{
		MRELEASE(d);
		return -1;
	}

	eidLen = strlen(sourceEid);
	d->sourceEid = MTAKE(eidLen + 1);
	if (d->sourceEid == NULL)
	{
		MRELEASE(d->bytes);
		MRELEASE(d);
		return -1;
	}

	memcpy(d->bytes, bytes, bytesLen);
	memcpy(d->sourceEid, sourceEid, eidLen + 1);
	d->bytesLen = bytesLen;

	if (deferredTail == NULL)
	{
		deferredHead = d;
	}
	else
	{
		deferredTail->next = d;
	}

	deferredTail = d;
	return 0;
}

static DeferredBundle *dequeueDeferred(void)
{
	DeferredBundle *d = deferredHead;

	if (d == NULL)
	{
		return NULL;
	}

	deferredHead = d->next;
	if (deferredHead == NULL)
	{
		deferredTail = NULL;
	}

	d->next = NULL;
	return d;
}

static void freeDeferred(DeferredBundle *d)
{
	if (d == NULL)
	{
		return;
	}

	if (d->bytes)
	{
		MRELEASE(d->bytes);
	}

	if (d->sourceEid)
	{
		MRELEASE(d->sourceEid);
	}

	MRELEASE(d);
}

/*	The defer callback handed to bpshSessionRun: a session passes us a
 *	bundle that arrived mid-command but wasn't its own stdin.	*/
static void deferBundle(void *ctx, unsigned char *bytes, size_t len,
		const char *srcEid)
{
	(void) ctx;
	if (enqueueDeferred(bytes, len, srcEid) < 0)
	{
		writeMemo("[?] bpshd: can't defer bundle; dropping.");
	}
}

/*	Session table: linear scan keyed by source EID.  Expects tens of
 *	sessions max; replace with a hash if scaling demands it.	*/
static BpshSession *findSession(const char *sourceEid)
{
	int i;

	for (i = 0; i < sessionCount; i++)
	{
		if (strcmp(bpshSessionEid(sessions[i]), sourceEid) == 0)
		{
			return sessions[i];
		}
	}

	return NULL;
}

static int addSession(BpshSession *s)
{
	if (sessionCount == sessionCap)
	{
		int	      newCap = sessionCap == 0 ? 8 : sessionCap * 2;
		BpshSession **bigger = MTAKE(newCap * sizeof(BpshSession *));

		if (bigger == NULL)
		{
			return -1;
		}

		if (sessions != NULL)
		{
			memcpy(bigger, sessions,
					sessionCount * sizeof(BpshSession *));
			MRELEASE(sessions);
		}

		sessions = bigger;
		sessionCap = newCap;
	}

	sessions[sessionCount++] = s;
	return 0;
}

/*	Drop s from the table, then close it.				*/
static void removeSession(BpshSession *s)
{
	int i;

	for (i = 0; i < sessionCount; i++)
	{
		if (sessions[i] == s)
		{
			sessions[i] = sessions[sessionCount - 1];
			sessionCount--;
			break;
		}
	}

	bpshSessionClose(s);
}

static void teardownAllSessions(void)
{
	int i;

	for (i = 0; i < sessionCount; i++)
	{
		bpshSessionClose(sessions[i]);
	}

	sessionCount = 0;
	if (sessions != NULL)
	{
		MRELEASE(sessions);
		sessions = NULL;
		sessionCap = 0;
	}
}

static int handleFrame(BpshFrame *frame, char *sourceEid)
{
	BpshSession *s;
	char	    *cmd;

	switch (frame->msgType)
	{
	case BpshMsgInit:
	{
		int wantStdin = 0;
		if (frame->payloadLen >= 1 && frame->payload[0] == 0x01)
		{
			wantStdin = 1;
		}

		s = findSession(sourceEid);
		if (s != NULL)
		{
			writeMemoNote("[i] bpshd INIT replaces prior session for",
					sourceEid);
			removeSession(s);
		}

		s = bpshSessionOpen(sap, sourceEid, frame->sessionId, wantStdin);
		if (s == NULL)
		{
			putErrmsg("bpshd: can't open session.", sourceEid);
			return bpsh_send_error(sap, sourceEid, frame->sessionId,
					0, "unable to start shell");
		}

		if (addSession(s) < 0)
		{
			putErrmsg("bpshd: can't register session.", sourceEid);
			bpshSessionClose(s);
			return bpsh_send_error(sap, sourceEid, frame->sessionId,
					0, "unable to register session");
		}

		writeMemoNote(wantStdin ? "[i] bpshd INIT (stdin)" :
					  "[i] bpshd INIT",
				sourceEid);

		/*	Send the initial cwd before INIT_ACK (lower seq) so
		 *	the client's first prompt already shows it.	*/
		bpshSessionSendCwd(s);
		return bpsh_send_ctl(sap, sourceEid, BpshMsgInitAck,
				bpshSessionId(s), bpshSessionNextSeq(s));
	}

	case BpshMsgReq:
		s = findSession(sourceEid);
		if (s == NULL)
		{
			return bpsh_send_error(sap, sourceEid, frame->sessionId,
					0,
					"session not initialized; send INIT first");
		}

		if (bpshSessionId(s) != frame->sessionId)
		{
			return bpsh_send_error(sap, sourceEid, frame->sessionId,
					0, "session id mismatch; reinit");
		}

		if (!bpshSessionAlive(s))
		{
			return bpsh_send_error(sap, sourceEid, frame->sessionId,
					bpshSessionNextSeq(s),
					"shell exited; reinitialize session");
		}

		cmd = MTAKE(frame->payloadLen + 1);
		if (cmd == NULL)
		{
			return bpsh_send_exit(sap, sourceEid, frame->sessionId,
					bpshSessionNextSeq(s), -1,
					BpshCauseKilled);
		}

		memcpy(cmd, frame->payload, frame->payloadLen);
		cmd[frame->payloadLen] = '\0';
		writeMemoNote("[i] bpshd REQ", cmd);
		bpshSessionRun(s, cmd, &running, deferBundle, NULL);
		MRELEASE(cmd);

		if (!bpshSessionAlive(s))
		{
			removeSession(s);
		}

		return 0;

	case BpshMsgExitSession:
		s = findSession(sourceEid);
		if (s != NULL)
		{
			writeMemoNote("[i] bpshd EXIT_SESSION", sourceEid);
			removeSession(s);
		}

		return 0;

	default:
		writeMemoNote("[?] bpshd unexpected msgType",
				bpsh_msgtype_name(frame->msgType));
		return 0;
	}
}

/*	Replay one bundle deferred while a command ran.  Returns 1 if a
 *	deferred bundle was handled (call again), 0 if the queue is empty.*/
static int drainDeferred(void)
{
	DeferredBundle *d;
	BpshFrame	frame;

	if (deferredHead == NULL)
	{
		return 0;
	}

	d = dequeueDeferred();
	if (bpsh_decode(d->bytes, d->bytesLen, &frame) >= 0)
	{
		handleFrame(&frame, d->sourceEid);
	}

	freeDeferred(d);
	return 1;
}

/*	Receive decoded frames and dispatch them, first replaying any
 *	bundles deferred while a command was running.			*/
static int receiveLoop(void)
{
	BpshFrame      frame;
	unsigned char *bytes;
	size_t	       bytesLen;
	char	       srcEid[BPSH_MAX_EID];
	int	       rc;

	while (running)
	{
		if (drainDeferred())
		{
			continue;
		}

		rc = bpsh_recv_frame(sap, BP_BLOCKING, BPSHD_RECV_BUFSIZE, &frame,
				&bytes, &bytesLen, srcEid, sizeof srcEid);
		if (rc == BPSH_RECV_FATAL)
		{
			writeMemo("[i] bpshd receive loop ending.");
			break;
		}

		if (rc == BPSH_RECV_NONE || rc == BPSH_RECV_SKIP)
		{
			continue;
		}

		handleFrame(&frame, srcEid);
		MRELEASE(bytes);
	}

	return 0;
}

static void usage(void)
{
	fprintf(stderr,
			"Usage: bpshd <listen EID>\n"
			"\n"
			"Listens on <listen EID> for bpsh client requests.  Each\n"
			"client session gets a persistent /bin/sh; commands are\n"
			"run inside it (so cd, env-vars persist), and stdout /\n"
			"stderr / exit-code are returned in separate bundles.\n");
}

int main(int argc, char **argv)
{
	char *listenEid;

	if (argc < 2 || argv[1][0] == '-')
	{
		usage();
		return 1;
	}

	listenEid = argv[1];

	if (bp_attach() < 0)
	{
		putErrmsg("bpshd: bp_attach failed.", NULL);
		return 1;
	}

	if (bp_open(listenEid, &sap) < 0)
	{
		putErrmsg("bpshd: can't open listen endpoint.", listenEid);
		bp_detach();
		return 1;
	}

	/*	Send command output with flow control: ionCreateZco then
	 *	blocks for ZCO space instead of failing on large output.	*/
	if (ionStartAttendant(&attendant) < 0)
	{
		putErrmsg("bpshd: can't start transmission attendant.", NULL);
		bp_close(sap);
		bp_detach();
		return 1;
	}

	attendantStarted = 1;
	bpsh_set_send_attendant(&attendant);

	signal(SIGINT, handleQuit);
	signal(SIGTERM, handleQuit);
	/*	Don't let a closed shell pipe abort the daemon.		*/
	signal(SIGPIPE, SIG_IGN);

	writeMemoNote("[i] bpshd listening on", listenEid);
	receiveLoop();

	teardownAllSessions();
	bpsh_set_send_attendant(NULL);
	ionStopAttendant(&attendant);
	bp_close(sap);
	bp_detach();
	return 0;
}
