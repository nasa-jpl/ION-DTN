/*

	tests/bp_sap_thread_ownership/dotest.c:	regression test for
		BpSAP ownership conflict handling.

	Verifies that a second bp_open() on an endpoint that is already
	open in the same process -- whether the second caller is the
	same thread or a different thread -- fails gracefully (rc != 0,
	*sapPtr left NULL) instead of returning rc == 0 with a NULL SAP
	that would later null-deref inside bp_receive().

	Process-level conflict (different process opening the same
	endpoint) is already handled by the unchanged "Endpoint is
	already open" path and is not retested here.

										*/

#include <pthread.h>
#include <string.h>
#include <stdio.h>

#include <bp.h>
#include "check.h"
#include "testutil.h"

static char	ownedEid[]      = "ipn:1.1";	/* Held by main thread.    */
static char	independentEid[] = "ipn:1.2";	/* Free, opened by helper. */

typedef struct
{
	char		*eid;
	int		rc;
	BpSAP		sap;
} OpenArgs;

static void	*tryOpen(void *arg)
{
	OpenArgs	*a = (OpenArgs *) arg;

	/*	Poison the out-parameter so we can detect that bp_open()
	 *	properly resets it.  bp_open() begins by setting
	 *	*bpsapPtr = NULL, so on every code path -- success or
	 *	failure -- the poison must be cleared.			*/
	a->sap = (BpSAP) (void *) 0xdeadbeefUL;
	a->rc  = bp_open(a->eid, &a->sap);
	return NULL;
}

int	main(int argc, char **argv)
{
	BpSAP		owner = NULL;
	BpSAP		retry = (BpSAP) (void *) 0xdeadbeefUL;
	int		rc;
	pthread_t	helper;
	OpenArgs	args;

	(void) argc;
	(void) argv;

	synch_killm(60);

	ionstart_default_config("loopback-ltp/loopback.ionrc",
			"loopback-ltp/loopback.ionsecrc",
			"loopback-ltp/loopback.ltprc",
			"loopback-ltp/loopback.bprc",
			"loopback-ltp/loopback.ipnrc",
			NULL);

	fail_unless(bp_attach() >= 0);

	/*	Case 1: the first open by the main thread succeeds.	*/

	fprintf(stderr, "[case 1] main thread bp_open(%s)\n", ownedEid);
	rc = bp_open(ownedEid, &owner);
	fail_unless(rc == 0);
	fail_unless(owner != NULL);

	/*	Case 2: the SAME thread tries to open the SAME endpoint
	 *	again.  Before the fix this returned rc=0 with *sap=NULL,
	 *	leading to a deferred null-pointer crash in bp_receive().
	 *	After the fix it must return non-zero and leave *sap NULL.
	 *	(bp_open() unconditionally sets *bpsapPtr = NULL at entry,
	 *	so the poison value below must be cleared even on the
	 *	error path.)						*/

	fprintf(stderr, "[case 2] same thread bp_open(%s) again\n", ownedEid);
	rc = bp_open(ownedEid, &retry);
	fail_unless(rc != 0);
	fail_unless(retry == NULL);

	/*	Case 3: a DIFFERENT thread in the same process tries to
	 *	open the endpoint that the main thread already owns.
	 *	ION cannot distinguish threads within a process (every
	 *	thread reports the same TGID via sm_TaskIdSelf()), so this
	 *	collapses into the same "already open" error path as
	 *	Case 2 -- which is precisely the point: the helper does
	 *	not get a SAP it could use to race the main thread inside
	 *	bp_receive().						*/

	fprintf(stderr, "[case 3] helper thread bp_open(%s)\n", ownedEid);
	memset(&args, 0, sizeof args);
	args.eid = ownedEid;
	fail_unless(pthread_create(&helper, NULL, tryOpen, &args) == 0);
	fail_unless(pthread_join(helper, NULL) == 0);
	fail_unless(args.rc != 0);
	fail_unless(args.sap == NULL);

	/*	Case 4: a helper thread can still open an INDEPENDENT
	 *	endpoint while the main thread holds another one.  This
	 *	confirms the failure path in Case 3 is endpoint-scoped,
	 *	not a blanket "no opens from other threads" rule.	*/

	fprintf(stderr, "[case 4] helper thread bp_open(%s)\n", independentEid);
	memset(&args, 0, sizeof args);
	args.eid = independentEid;
	fail_unless(pthread_create(&helper, NULL, tryOpen, &args) == 0);
	fail_unless(pthread_join(helper, NULL) == 0);
	fail_unless(args.rc == 0);
	fail_unless(args.sap != NULL);
	bp_close(args.sap);

	/*	Case 5: after bp_close() the endpoint is releasable; the
	 *	main thread (or any thread) can open it again.		*/

	fprintf(stderr, "[case 5] reopen %s after bp_close\n", ownedEid);
	bp_close(owner);
	owner = NULL;
	rc = bp_open(ownedEid, &owner);
	fail_unless(rc == 0);
	fail_unless(owner != NULL);
	bp_close(owner);

	writeErrmsgMemos();
	bp_detach();

	ionstop();

	return check_summary(argv[0]);
}
