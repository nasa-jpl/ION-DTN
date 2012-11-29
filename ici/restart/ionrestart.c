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

extern void	ionDropVdb();
extern void	ionRaiseVdb();

static void	restartION(Sdr sdrv, char *utaCmd)
{
	int	i;
	int restart_bp=1;
	int restart_ltp=1;
	int restart_cfdp=1;

	/*	Stop all tasks.						*/
#ifndef NASA_PROTECTED_FLIGHT_CODE
	if (cfdpAttach() < 0)
	{
		restart_cfdp=0;
		putErrmsg("ionrestart can't attach to CFDP.", NULL);
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
			putErrmsg("CFDP not stopped.", NULL);
		}
	}
#endif

	if (bpAttach() < 0)
	{
		restart_bp=0;
		putErrmsg("ionrestart can't attach to BP.", NULL);
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
			putErrmsg("BP not stopped.", NULL);
		}
	}

	if (ltpAttach() < 0)
	{
		restart_ltp=0;
		putErrmsg("ionrestart can't attach to LTP.", NULL);
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
			putErrmsg("LTP not stopped.", NULL);
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
		putErrmsg("rfxclock not stopped.", NULL);
	}

	/*	Terminate all remaining tasks by ending the
	 *	transaction semaphore.					*/

	sm_SemEnd(sdrv->sdr->sdrSemaphore);

	/*	Drop all volatile databases.				*/

#ifndef NASA_PROTECTED_FLIGHT_CODE
	if(restart_cfdp){
		cfdpDropVdb();
	}
#endif
	cgr_stop();
	if(restart_bp){
		bpDropVdb();
	}
	if(restart_ltp){
		ltpDropVdb();
	}
	ionDropVdb();

	/*	Un-end the transaction semaphore.	*/

	sm_SemUnend(sdrv->sdr->sdrSemaphore);

	/*	Now re-create all of the volatile databases.		*/

	ionRaiseVdb();
	if(restart_ltp){
		ltpRaiseVdb();
	}
	if(restart_bp){
		bpRaiseVdb();
	}
	cgr_start();
#ifndef NASA_PROTECTED_FLIGHT_CODE
	if(restart_cfdp){
		cfdpRaiseVdb();
	}
#endif

	/*	Restart all ION tasks.					*/

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
		putErrmsg("rfxclock not restarted.", NULL);
	}

	if(restart_ltp){
		ltpStart();
		for (i = 0; i < 5; i++)
		{
			if (!ltp_engine_is_started())
			{
				snooze(1);
				continue;		/*	Not started.	*/
			}

			break;
		}

		if (i == 5)
		{
			putErrmsg("LTP not started.", NULL);
		}
	}

	if(restart_bp){
		bpStart();
		for (i = 0; i < 5; i++)
		{
			if (!bp_agent_is_started())
			{
				snooze(1);
				continue;		/*	Not started.	*/
			}

			break;
		}

		if (i == 5)
		{
			putErrmsg("BP not started.", NULL);
		}
	}

#ifndef NASA_PROTECTED_FLIGHT_CODE
	if(restart_cfdp){
		cfdpStart(utaCmd);
		for (i = 0; i < 5; i++)
		{
			if (!cfdp_entity_is_started())
			{
				snooze(1);
				continue;		/*	Not started.	*/
			}

			break;
		}

		if (i == 5)
		{
			putErrmsg("CFDP not started.", NULL);
		}
	}
#endif
}

#if defined (VXWORKS) || defined (RTEMS) || defined (bionic)
int	ionrestart(int a1, int a2, int a3, int a4, int a5,
		int a6, int a7, int a8, int a9, int a10)
{
	char		*utaCmd = a1 ? (char *) a1 : "bputa";
#else
int	main(int argc, char **argv)
{
	char		*utaCmd = argc > 1 ? argv[1] : "bputa";
#endif
	Sdr		sdrv;
	int		sdrOwnerTask;
	pthread_t	sdrOwnerThread;

	if (ionAttach() < 0)
	{
		putErrmsg("ionrestart can't attach to ION.", NULL);
		return 1;
	}

	/*	Impersonate the current owner of the ION mutex.		*/

	sdrv = getIonsdr();
	sdrOwnerTask = sdrv->sdr->sdrOwnerTask;
	sdrOwnerThread = sdrv->sdr->sdrOwnerThread;
	sdrv->sdr->sdrOwnerTask = sm_TaskIdSelf();
	sdrv->sdr->sdrOwnerThread = pthread_self();

	/*	Perform the restart.					*/

	restartION(sdrv, utaCmd);

	/*	Restore current owner of the ION mutex.			*/

	sdrv->sdr->sdrOwnerTask = sdrOwnerTask;
	sdrv->sdr->sdrOwnerThread = sdrOwnerThread;

	/*	Terminate.						*/

	ionDetach();
	writeMemo("[i] ION volatile databases reinitialized.");
	return 0;
}
