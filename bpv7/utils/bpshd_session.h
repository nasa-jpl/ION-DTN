/*
 * bpshd_session.h: one bpshd shell session.
 *
 * A BpshSession is a single client's persistent /bin/sh: a fork()ed
 * shell whose stdin/stdout/stderr the daemon owns, plus a fd-3 pipe
 * for forwarded user stdin and a fd-4 control pipe carrying each
 * command's exit code.  This module is purely the per-session
 * object -- open it, run command lines on it, close it.  The daemon
 * (bpshd.c) owns the collection of sessions, the routing of frames to
 * them, and the queue of bundles deferred while a command runs.
 *
 * While a command runs, the session keeps the shared BP endpoint
 * serviced so its own forwarded-stdin bundles flow; any bundle that
 * isn't this session's stdin is handed to the daemon-supplied defer
 * callback rather than being routed here.
 */

#ifndef BPSHD_SESSION_H
#define BPSHD_SESSION_H

#include "bpsh_proto.h"

#ifdef __cplusplus
extern "C" {
#endif

/*	Largest bundle the daemon will accept (also the cap passed to
 *	bpsh_recv_frame by both the main loop and a running session).	*/
#define BPSHD_RECV_BUFSIZE (64 * 1024)

typedef struct BpshSession BpshSession;

/*	Callback the daemon supplies to bpshSessionRun so a bundle that
 *	arrived mid-command but isn't this session's stdin can be handed
 *	back for the daemon to queue.  The bytes are owned by the caller;
 *	the callback must copy anything it keeps.			*/
typedef void (*BpshDeferFn)(void *ctx, unsigned char *bytes, size_t len,
		const char *srcEid);

/*	bpshSessionOpen: fork a /bin/sh and wire up its pipes.  sap is the
 *	endpoint replies are sent on.  wantStdin keeps the fd-3 stdin pipe
 *	open for forwarding.  Returns NULL on failure.			*/
extern BpshSession *bpshSessionOpen(BpSAP sap, const char *sourceEid,
		uvast sessionId, int wantStdin);

/*	bpshSessionClose: terminate the shell, close fds, free.		*/
extern void bpshSessionClose(BpshSession *s);

/*	bpshSessionSendCwd: read the shell's current working directory and
 *	send it to the client as a CWD frame.  No-op if the cwd can't be
 *	determined (e.g. no /proc).  Sent at INIT and after each command
 *	so the client can show it in the prompt.			*/
extern void bpshSessionSendCwd(BpshSession *s);

/*	bpshSessionRun: run one command line in the session's shell,
 *	streaming STDOUT / STDERR / EXIT back to the client.  While it
 *	runs, the session services its own forwarded-stdin bundles off
 *	sap; any other bundle is passed to defer(ctx, ...).  *running is
 *	polled so a shutdown request can unwind the poll.  Returns 0.	*/
extern int bpshSessionRun(BpshSession *s, const char *cmdline,
		const int *running, BpshDeferFn defer, void *ctx);

/*	Accessors the daemon needs for routing and validation.		*/
extern const char *bpshSessionEid(const BpshSession *s);
extern uvast	   bpshSessionId(const BpshSession *s);
extern int	   bpshSessionAlive(const BpshSession *s);
extern uvast	   bpshSessionNextSeq(BpshSession *s); /* returns, then ++	*/

#ifdef __cplusplus
}
#endif

#endif /* BPSHD_SESSION_H */
