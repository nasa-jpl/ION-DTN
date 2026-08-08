/*
 * tests/bp_receive_concurrent_timer/dotest.c: two threads, two SAPs, each in a
 * deadline-bearing bp_receive() with no traffic.  Every reception must report
 * BpReceptionTimedOut; a spurious BpReceptionInterrupted, or a receive that
 * never returns, means the two calls are sharing timer state.
 */

#include <pthread.h>
#include <string.h>
#include "bp.h"
#include "check.h"
#include "platform.h"
#include "testutil.h"

#define ITERATIONS	40 /* Per thread. */
#define TIMEOUT_SECONDS 1  /* Positive => a timer thread. */

static char eidA[] = "ipn:1.1";
static char eidB[] = "ipn:1.2";

typedef struct
{
	char *eid;
	int   badResults; /* Anything other than a timeout. */
	int   iterations; /* Receptions actually completed. */
	int   fatal;	  /* bp_open()/bp_receive() error. */
} ReceiverState;

static void *timeoutReceiver(void *arg)
{
	ReceiverState *state = (ReceiverState *) arg;
	BpSAP	       sap = NULL;
	BpDelivery     dlv;
	int	       i;

	if (bp_open(state->eid, &sap) < 0 || sap == NULL)
	{
		state->fatal = 1;
		return NULL;
	}

	for (i = 0; i < ITERATIONS; i++)
	{
		if (bp_receive(sap, &dlv, TIMEOUT_SECONDS) < 0)
		{
			state->fatal = 1;
			break;
		}

		/* Nothing is ever sent here; a timeout is the only outcome. */
		if (dlv.result != BpReceptionTimedOut)
		{
			state->badResults++;
		}

		state->iterations++;
		bp_release_delivery(&dlv, 1);
	}

	bp_close(sap);
	return NULL;
}

int main(int argc, char **argv)
{
	pthread_t     threadA;
	pthread_t     threadB;
	ReceiverState stateA;
	ReceiverState stateB;

	(void) argc;

	synch_killm(60);

	ionstart_default_config("loopback-ltp/loopback.ionrc",
			"loopback-ltp/loopback.ionsecrc",
			"loopback-ltp/loopback.ltprc",
			"loopback-ltp/loopback.bprc",
			"loopback-ltp/loopback.ipnrc", NULL);

	fail_unless(bp_attach() >= 0);

	memset(&stateA, 0, sizeof stateA);
	memset(&stateB, 0, sizeof stateB);
	stateA.eid = eidA;
	stateB.eid = eidB;

	fail_unless(pthread_create(&threadA, NULL, timeoutReceiver, &stateA) == 0);
	fail_unless(pthread_create(&threadB, NULL, timeoutReceiver, &stateB) == 0);

	fail_unless(pthread_join(threadA, NULL) == 0);
	fail_unless(pthread_join(threadB, NULL) == 0);

	fail_unless(stateA.fatal == 0,
			"receiver A hit a fatal bp_open()/bp_receive() error");
	fail_unless(stateB.fatal == 0,
			"receiver B hit a fatal bp_open()/bp_receive() error");

	/* A shortfall means a receive wedged on mis-handled timer state. */

	fail_unless(stateA.iterations == ITERATIONS,
			"receiver A completed fewer receptions than expected");
	fail_unless(stateB.iterations == ITERATIONS,
			"receiver B completed fewer receptions than expected");

	fail_unless(stateA.badResults == 0,
			"receiver A saw spurious non-timeout results");
	fail_unless(stateB.badResults == 0,
			"receiver B saw spurious non-timeout results");

	writeErrmsgMemos();
	bp_detach();

	ionstop();

	return check_summary(argv[0]);
}
