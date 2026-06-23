/*
	ionrestart.c:	Stops ION, re-initializes all volatile
			databases, then restarts ION.  The purpose
			of this utility is to wipe out potentially
			corrupt volatile database information in the
			event that an ION transaction is reversed:
			transaction reversal protects non-volatile
			database integrity but does not realign the
			volatile database with the repaired heap.
									*/
/*									*/
/*	Copyright (c) 2012, California Institute of Technology.		*/
/*	All rights reserved.						*/
/*	Author: Scott Burleigh, Jet Propulsion Laboratory		*/
/*									*/

#include "platform.h"
#include "sdrP.h"
#include "rfx.h"
#include "ltpP.h"
#include "bpP.h"
#include "cgr.h"
#include "cfdpP.h"

#ifndef RESTART_GRACE_PERIOD
#define RESTART_GRACE_PERIOD	3
#endif

#define	RESTART_LOOP_INTERVAL	(RESTART_GRACE_PERIOD * 5)

extern void	ionDropVdb(void);
extern void	ionRaiseVdb(void);

static void	restartION(Sdr sdrv)
{
	int	i;
	int	restart_bp = 1;
	int	restart_ltp = 1;
	int	restart_cfdp = 1;
	time_t	prevRestartTime;

	/*	Stop all tasks.						*/
	if (cfdpAttach() < 0)
	{
		restart_cfdp = 0;
		writeMemo("[!] ionrestart can't attach to CFDP.");
	}
	else
	{
		cfdpStop();
		for (i = 0; i < 5; i++)
		{
			if (cfdp_entity_is_started())
			{
				snooze(1);
				continue;	/*	Not stopped.	*/
			}

			break;
		}

		if (i == 5)
		{
			writeMemo("[!] ionrestart: CFDP not stopped.");
		}
		else
		{
			writeMemo("[i] ionrestart: CFDP stopped.");
		}
	}

	if (bpAttach() < 0)
	{
		restart_bp = 0;
		writeMemo("[!] ionrestart can't attach to BP.");
	}
	else
	{
		BpVdb		*bpvdb = getBpVdb();
		PsmPartition	bpwm = getIonwm();
		PsmAddress	elt;
		VPlan		*vplan;
		int		oldClockPid = ERROR;

		/*	Maximum of 64 bpclm processes tracked.		*/

		int		oldClmPids[64];
		int		oldClmCount = 0;
		int		j;

		/*	Save the old bpclock PID before stopping.	*/

		if (bpvdb != NULL)
		{
			oldClockPid = bpvdb->clockPid;

			/*	Save old bpclm PIDs before stopping.	*/

			for (elt = sm_list_first(bpwm, bpvdb->plans); elt;
					elt = sm_list_next(bpwm, elt))
			{
				vplan = (VPlan *) psp(bpwm,
						sm_list_data(bpwm, elt));
				if (vplan->clmPid != ERROR &&
						oldClmCount < 64)
				{
					oldClmPids[oldClmCount++] =
							vplan->clmPid;
				}
			}
		}

		cgr_stop();
		bpStop();
		for (i = 0; i < 5; i++)
		{
			if (bp_agent_is_started())
			{
				snooze(1);
				continue;	/*	Not stopped.	*/
			}

			break;
		}

		if (i == 5)
		{
			writeMemo("[!] ionrestart: BP not stopped.");
		}
		else
		{
			writeMemo("[i] ionrestart: BP stopped.");
		}

		/*	Ensure old bpclock process is actually dead.
		 *	It may still be unwinding after transaction
		 *	crash even though bpStop() completed.		*/

		if (oldClockPid != ERROR && sm_TaskExists(oldClockPid))
		{
			writeMemo("[i] ionrestart: Waiting for old bpclock \
to terminate...");
			for (i = 0; i < 50 && sm_TaskExists(oldClockPid); i++)
			{
				microsnooze(100000);
			}

			if (sm_TaskExists(oldClockPid))
			{
				writeMemo("[!] ionrestart: Old bpclock not \
responding, sending SIGKILL.");
				sm_TaskKill(oldClockPid, SIGKILL);
				for (i = 0; i < 10 && sm_TaskExists(oldClockPid);
						i++)
				{
					microsnooze(100000);
				}
			}

			if (sm_TaskExists(oldClockPid))
			{
				writeMemo("[!] ionrestart: Old bpclock still \
running after SIGKILL.");
			}
			else
			{
				writeMemo("[i] ionrestart: Old bpclock \
terminated.");
			}
		}

		/*	Ensure old bpclm processes are actually dead.
		 *	They may still be unwinding after transaction
		 *	crash even though bpStop() completed.		*/

		for (j = 0; j < oldClmCount; j++)
		{
			if (sm_TaskExists(oldClmPids[j]))
			{
				writeMemo("[i] ionrestart: Waiting for old \
bpclm to terminate...");
				for (i = 0; i < 50 &&
						sm_TaskExists(oldClmPids[j]);
						i++)
				{
					microsnooze(100000);
				}

				if (sm_TaskExists(oldClmPids[j]))
				{
					writeMemo("[!] ionrestart: Old bpclm \
not responding, sending SIGKILL.");
					sm_TaskKill(oldClmPids[j], SIGKILL);
					for (i = 0; i < 10 &&
						sm_TaskExists(oldClmPids[j]);
							i++)
					{
						microsnooze(100000);
					}
				}

				if (sm_TaskExists(oldClmPids[j]))
				{
					writeMemo("[!] ionrestart: Old bpclm \
still running after SIGKILL.");
				}
				else
				{
					writeMemo("[i] ionrestart: Old bpclm \
terminated.");
				}
			}
		}
	}

	if (ltpAttach() < 0)
	{
		restart_ltp = 0;
		writeMemo("[!] ionrestart can't attach to LTP.");
	}
	else
	{
		ltpStop();
		for (i = 0; i < 5; i++)
		{
			if (ltp_engine_is_started())
			{
				snooze(1);
				continue;	/*	Not stopped.	*/
			}

			break;
		}

		if (i == 5)
		{
			writeMemo("[!] ionrestart: LTP not stopped.");
		}
		else
		{
			writeMemo("[i] ionrestart: LTP stopped.");
		}
	}

	rfx_stop();
	for (i = 0; i < 5; i++)
	{
		if (rfx_system_is_started())
		{
			snooze(1);
			continue;		/*	Not stopped.	*/
		}

		break;
	}

	if (i == 5)
	{
		writeMemo("[!] ionrestart: rfxclock not stopped.");
	}
	else
	{
		writeMemo("[i] ionrestart: rfxclock stopped.");
	}

	/*	Terminate all remaining tasks by ending the
	 *	transaction lock.					*/

#ifdef ION_HAVE_ROBUST_MUTEX
	ion_ipc_atomic_set(&sdrv->sdr->sdrXnEnded, 1);
#else
	sm_SemEnd(sdrv->sdr->sdrSemaphore);
#endif

	/*	Drop all volatile databases.				*/

	if (restart_cfdp)
	{
		cfdpDropVdb();
		writeMemo("[i] ionrestart: CFDP volatile database dropped.");
	}
	if (restart_bp)
	{
		bpDropVdb();
		writeMemo("[i] ionrestart: BP volatile database dropped.");
	}

	if (restart_ltp)
	{
		ltpDropVdb();
		writeMemo("[i] ionrestart: LTP volatile database dropped.");
	}

	ionDropVdb();
	writeMemo("[i] ionrestart: ION volatile database dropped.");

	/*	Re-enable transactions.					*/

#ifdef ION_HAVE_ROBUST_MUTEX
	ion_ipc_atomic_set(&sdrv->sdr->sdrXnEnded, 0);
#else
	sm_SemUnend(sdrv->sdr->sdrSemaphore);
	sm_SemGive(sdrv->sdr->sdrSemaphore);
#endif

	/*	Now re-create all of the volatile databases.		*/

	ionRaiseVdb();
	writeMemo("[i] ionrestart: ION volatile database raised.");
	if (restart_ltp)
	{
		ltpRaiseVdb();
		writeMemo("[i] ionrestart: LTP volatile database raised.");
	}

	if (restart_bp)
	{
		bpRaiseVdb();
		writeMemo("[i] ionrestart: BP volatile database raised.");
	}

	cgr_start();
	if (restart_cfdp)
	{
		cfdpRaiseVdb();
		writeMemo("[i] ionrestart: CFDP volatile database raised.");
	}

	/*	If it's safe, restart all ION tasks.			*/

	prevRestartTime = sdrv->sdr->restartTime;
	sdrv->sdr->restartTime = getCtime();
	if ((sdrv->sdr->restartTime - prevRestartTime) < RESTART_LOOP_INTERVAL)
	{
		writeMemo("[!] Inferred restart loop.  Tasks not restarted.");
		return;
	}

	/*	Exit the hijacked transaction before starting daemons.
	 *	This ensures each daemon can begin its own transaction
	 *	cleanly without racing with ionrestart's transaction.	*/

	sdrv->sdr->xnDepth = 1;
	sdrv->sdr->modified = 0;
	sdr_exit_xn(sdrv);

	/*	Disable fail-fast mode during daemon restart to prevent
	 *	assertion failures from aborting newly restarted daemons.  */
	{
		int	off = 0;

		oK(_coreFileNeeded(&off));
	}

	/*	Set halted flag during daemon startup.  This allows
	 *	spawned daemons to pass sdrFetchSafe() checks while
	 *	they initialize, preventing race conditions where
	 *	daemons try to access SDR before fully attached.	*/

	sdrv->sdr->halted = 1;

	rfx_start();
	for (i = 0; i < 5; i++)
	{
		if (!rfx_system_is_started())
		{
			snooze(1);
			continue;		/*	Not started.	*/
		}

		break;
	}

	if (i == 5)
	{
		writeMemo("[!] ionrestart: rfxclock not restarted.");
	}
	else
	{
		writeMemo("[i] ionrestart: rfxclock restarted.");
	}

	if (restart_ltp)
	{
		ltpStart();
		for (i = 0; i < 5; i++)
		{
			if (!ltp_engine_is_started())
			{
				snooze(1);
				continue;	/*	Not started.	*/
			}

			break;
		}

		if (i == 5)
		{
			writeMemo("[!] ionrestart: LTP not restarted.");
		}
		else
		{
			writeMemo("[i] ionrestart: LTP restarted.");
		}
	}

	if (restart_bp)
	{
		bpStart();
		for (i = 0; i < 5; i++)
		{
			if (!bp_agent_is_started())
			{
				snooze(1);
				continue;	/*	Not started.	*/
			}

			break;
		}

		if (i == 5)
		{
			writeMemo("[!] ionrestart: BP not restarted.");
		}
		else
		{
			writeMemo("[i] ionrestart: BP restarted.");
		}
	}

	if (restart_cfdp)
	{
		cfdpStart(NULL);
		for (i = 0; i < 5; i++)
		{
			if (!cfdp_entity_is_started())
			{
				snooze(1);
				continue;	/*	Not started.	*/
			}

			break;
		}

		if (i == 5)
		{
			writeMemo("[!] ionrestart: CFDP not restarted.");
		}
		else
		{
			writeMemo("[i] ionrestart: CFDP restarted.");
		}
	}

	/*	Clear halted flag now that all daemons have started.	*/

	sdrv->sdr->halted = 0;

	/*	Restore fail-fast mode after successful restart.	*/
	{
		int	on = 1;

		oK(_coreFileNeeded(&on));
	}
}

#if defined (ION_LWT)
int	ionrestart(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
#else
int	main(void)
{
#endif
	Sdr		sdrv;
#ifndef ION_HAVE_ROBUST_MUTEX
	sm_SemId	sdrSemaphore;
#endif

	if (ionAttach() < 0)
	{
		putErrmsg("ionrestart can't attach to ION.", NULL);
		return 1;
	}

	/*	Prevent other tasks from beginning new transactions while
	 *	the failing task terminates: such transactions would
	 *	interfere with recovery from the failed transaction.	*/

	sdrv = getIonsdr();
#ifdef ION_HAVE_ROBUST_MUTEX
	/*	On a robust-mutex build the lock needs no owner
	 *	impersonation: terminateXn has already released it, so
	 *	there is no open transaction to take over.  Raising
	 *	sdrXnEnded makes takeSdr refuse new transactions for the
	 *	duration of the grace period.				*/

	ion_ipc_atomic_set(&sdrv->sdr->sdrXnEnded, 1);
	snooze(RESTART_GRACE_PERIOD);
	ion_ipc_atomic_set(&sdrv->sdr->sdrXnEnded, 0);
#else
	/*	Hijack the current transaction, i.e., impersonate the
	 *	current owner of the ION mutex.  ionAttach() entails
	 *	calling sdr_start_using, which gives the SDR semaphore;
	 *	temporarily setting the semaphore to -1 keeps the failing
	 *	task from beginning new transactions before it exits.	*/

	sdrv->sdr->sdrOwnerTask = sm_TaskIdSelf();
	sdrv->sdr->sdrOwnerThread = pthread_self();
	sdrSemaphore = sdrv->sdr->sdrSemaphore;
	sdrv->sdr->sdrSemaphore = -1;

	/*	Wait for the failing task to terminate, then re-enable
	 *	transactions and perform the restart.			*/

	snooze(RESTART_GRACE_PERIOD);
	sdrv->sdr->sdrSemaphore = sdrSemaphore;
#endif

	/* Given transaction reversal was successful, set the
	 * modified flag to 0 from this point on ward */
	sdrv->sdr->modified = 0;
	restartION(sdrv);

	/*	Terminate.						*/

	writeMemo("[i] ionrestart: finished restarting ION.");
	ionDetach();
	return 0;
}
