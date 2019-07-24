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
#ifndef NASA_PROTECTED_FLIGHT_CODE
#include "cfdpP.h"
#endif

#ifndef RESTART_GRACE_PERIOD
#define	RESTART_GRACE_PERIOD	3
#endif

#define	RESTART_LOOP_INTERVAL	(RESTART_GRACE_PERIOD * 5)

extern void	ionDropVdb();
extern void	ionRaiseVdb();

static void	restartION(Sdr sdrv, char *utaCmd)
{
	int	i;
	int	restart_bp = 1;
	int	restart_ltp = 1;
#ifndef NASA_PROTECTED_FLIGHT_CODE
	int	restart_cfdp = 1;
#endif
	time_t	prevRestartTime;

	/*	Stop all tasks.						*/
#ifndef NASA_PROTECTED_FLIGHT_CODE
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
#endif
	if (bpAttach() < 0)
	{
		restart_bp = 0;
		writeMemo("[!] ionrestart can't attach to BP.");
	}
	else
	{
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
	 *	transaction semaphore.					*/

	sm_SemEnd(sdrv->sdr->sdrSemaphore);

	/*	Drop all volatile databases.				*/

#ifndef NASA_PROTECTED_FLIGHT_CODE
	if (restart_cfdp)
	{
		cfdpDropVdb();
		writeMemo("[i] ionrestart: CFDP volatile database dropped.");
	}
#endif
	cgr_stop();
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

	/*	Un-end the transaction semaphore.			*/

	sm_SemUnend(sdrv->sdr->sdrSemaphore);
	sm_SemGive(sdrv->sdr->sdrSemaphore);

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
#ifndef NASA_PROTECTED_FLIGHT_CODE
	if (restart_cfdp)
	{
		cfdpRaiseVdb();
		writeMemo("[i] ionrestart: CFDP volatile database raised.");
	}
#endif
	/*	If it's safe, restart all ION tasks.			*/

	prevRestartTime = sdrv->sdr->restartTime;
	sdrv->sdr->restartTime = getCtime();
	if ((sdrv->sdr->restartTime - prevRestartTime) < RESTART_LOOP_INTERVAL)
	{
		writeMemo("[!] Inferred restart loop.  Tasks not restarted.");
		return;
	}

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
		ltpStart(NULL);
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

#ifndef NASA_PROTECTED_FLIGHT_CODE
	if (restart_cfdp)
	{
		cfdpStart(utaCmd);
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
#endif
}

#if defined (ION_LWT)
int	ionrestart(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
	char		*utaCmd = a1 ? (char *) a1 : "bputa";
#else
int	main(int argc, char **argv)
{
	char		*utaCmd = argc > 1 ? argv[1] : "bputa";
#endif
	Sdr		sdrv;
	sm_SemId	sdrSemaphore;

	if (ionAttach() < 0)
	{
		putErrmsg("ionrestart can't attach to ION.", NULL);
		return 1;
	}

	/*	Hijack the current transaction, i.e., impersonate
	 *	the current owner of the ION mutex.
	 *
	 *	ionAttach() entails calling sdr_start_using, which
	 *	gives the SDR semaphore and thereby would enable the
	 *	failing task to begin new transactions before exiting.
	 *	These new transactions would interfere with recovery
	 *	from the current failed transaction, so they must be
	 *	prevented.  For this purpose, we must temporarily set
	 *	the SDR semaphore to -1, restoring it when we are
	 *	confident that the failing task has terminated.		*/

	sdrv = getIonsdr();
	sdrv->sdr->sdrOwnerTask = sm_TaskIdSelf();
	sdrv->sdr->sdrOwnerThread = pthread_self();
	sdrSemaphore = sdrv->sdr->sdrSemaphore;
	sdrv->sdr->sdrSemaphore = -1;

	/*	Wait for the failing task to terminate, then re-enable
	 *	transactions and perform the restart.			*/

	snooze(RESTART_GRACE_PERIOD);
     	sdrv->sdr->sdrSemaphore = sdrSemaphore;
	restartION(sdrv, utaCmd);

	/*	Close out the hijacked transaction.			*/

	sdrv->sdr->xnDepth = 1;
	sdrv->modified = 0;
	sdr_exit_xn(sdrv);

	/*	Terminate.						*/

	ionDetach();
	writeMemo("[i] ionrestart: finished restarting ION.");
	return 0;
}
