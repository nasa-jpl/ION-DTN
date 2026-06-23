/*

	ionexit.c:	Cleanly shut down ION.


	Written 5/2011 by Greg Menke, Columbus, under contract with NASA GSFC

*/



#include "bp.h"
#include "bpP.h"

#ifdef ENABLE_BSSP
#include "bssp.h"
#include "bsspP.h"
#endif

#include "cfdp.h"
#include "cfdpP.h"

#ifdef ENABLE_DTPC
#include "dtpc.h"
/* Forward declarations to avoid dtpcP.h conflicts with bpP.h */
extern int	dtpcAttach(void);
extern void	_dtpcStop(void);
#define dtpcStop()	_dtpcStop()
#endif

#ifdef ENABLE_TC
#include "tcaP.h"
#include "tccP.h"
#endif

#include "ltp.h"
#include "ltpP.h"

#include "rfx.h"



static void	printText(char *text)
{
   PUTS(text);
}



#if defined (ION_LWT)
int	ionexit(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
	char	*p1 = (char *) a1;
	char	*p2 = (char *) a2;
#else
int	main(int argc, char **argv)
{
#endif
	int loopcount, errcount= 0, deletesdr = -1, stopipc = -1;
#ifdef ENABLE_TC
	char msgbuf[80];
	int groupNbr;
#endif

	/*	Options:
	 *	  k  - keep SDR (do not delete)
	 *	  n  - node-only: skip sm_ipc_stop(), safe for
	 *	       multi-ION-per-host when other instances
	 *	       are still running
	 *	Flags can be combined: "ionexit k n"		*/

#if defined (ION_LWT)
	if (p1 != NULL)
	{
		if (strcmp(p1, "k") == 0) deletesdr = 0;
		if (strcmp(p1, "n") == 0) stopipc = 0;
	}
	if (p2 != NULL)
	{
		if (strcmp(p2, "k") == 0) deletesdr = 0;
		if (strcmp(p2, "n") == 0) stopipc = 0;
	}
#else
	{
		int argn;
		for (argn = 1; argn < argc; argn++)
		{
			if (strcmp(argv[argn], "k") == 0)
			{
				deletesdr = 0;
			}
			else if (strcmp(argv[argn], "n") == 0)
			{
				stopipc = 0;
			}
		}
	}
#endif

	printText("Running ionexit" );
	printText( ((deletesdr) ? "will delete SDR" : "keeping SDR") );
	printText( ((stopipc) ? "will stop IPC" : "keeping IPC (node-only mode)") );

	if (ionAttach() == 0)
	{
#ifdef ENABLE_DTPC
		if (dtpcAttach() == 0)
		{
			printText("Issuing DTPC stop.");
			dtpcStop();

			for (loopcount = 5; dtpc_entity_is_started() && loopcount; loopcount--)
			{
				snooze(1);
			}
			if (!loopcount)
			{
				errcount++;
				printText("***** DTPC did not shut down");
			}
		}
		else
			printText("Unable to attach to DTPC");
#endif

#ifdef ENABLE_TC
		/* Stop all TCA instances (try group numbers 1-10) */
		for (groupNbr = 1; groupNbr <= 10; groupNbr++)
		{
			if (tcaAttach(groupNbr) == 0 && tcaIsStarted(groupNbr))
			{
				isprintf(msgbuf, sizeof(msgbuf), "Issuing TCA stop for group %d.", groupNbr);
				printText(msgbuf);
				tcaStop(groupNbr);

				for (loopcount = 5; tcaIsStarted(groupNbr) && loopcount; loopcount--)
				{
					snooze(1);
				}
				if (!loopcount)
				{
					errcount++;
					isprintf(msgbuf, sizeof(msgbuf), "***** TCA group %d did not shut down", groupNbr);
					printText(msgbuf);
				}
			}
		}

		/* Stop all TCC instances (try group numbers 1-10) */
		for (groupNbr = 1; groupNbr <= 10; groupNbr++)
		{
			if (tccAttach(groupNbr) == 0 && tccIsStarted(groupNbr))
			{
				isprintf(msgbuf, sizeof(msgbuf), "Issuing TCC stop for group %d.", groupNbr);
				printText(msgbuf);
				tccStop(groupNbr);

				for (loopcount = 5; tccIsStarted(groupNbr) && loopcount; loopcount--)
				{
					snooze(1);
				}
				if (!loopcount)
				{
					errcount++;
					isprintf(msgbuf, sizeof(msgbuf), "***** TCC group %d did not shut down", groupNbr);
					printText(msgbuf);
				}
			}
		}
#endif

		if (bpAttach() == 0)
		{
			printText("Issuing BP stop.");
			bpStop();

			for( loopcount= 5; bp_agent_is_started() && loopcount; loopcount--)
			{
				snooze(1);
			}
			if( !loopcount )
			{
				errcount++;
				printText("***** BP did not shut down");
			}
		}
		else
			printText("Unable to attach to BP");



		if (ltpAttach() == 0)
		{
			printText("Issuing LTP stop.");
			ltpStop();

			for( loopcount = 5; ltp_engine_is_started() && loopcount; loopcount-- )
			{
				snooze(1);
			}
			if( !loopcount )
			{
				errcount++;
				printText("***** LTP did not shut down");
			}
		}
		else
			printText("Unable to attach to LTP");

#ifdef ENABLE_BSSP
		if (bsspAttach() == 0)
		{
			printText("Issuing BSSP stop.");
			bsspStop();

			for( loopcount = 5; bssp_engine_is_started() && loopcount; loopcount-- )
			{
				snooze(1);
			}
			if( !loopcount )
			{
				errcount++;
				printText("***** BSSP did not shut down");
			}
		}
		else
			printText("Unable to attach to BSSP");
#endif
		if (cfdpAttach() == 0)
		{
			printText("Issuing CFDP stop.");
			cfdpStop();

			for( loopcount = 5; cfdp_entity_is_started() && loopcount; loopcount-- )
			{
				snooze(1);
			}
			if( !loopcount )
			{
				errcount++;
				printText("***** CFDP did not shut down");
			}
		}
		else
			printText("Unable to attach to CFDP");




		{
			printText("Issuing RFX stop.");
			rfx_stop();

			for( loopcount= 5; rfx_system_is_started() && loopcount; loopcount-- )
			{
				snooze(1);
			}
			if( !loopcount )
			{
				errcount++;
				printText("***** RFX did not shut down");
			}
		}


		/*	Give non-clock processes time to detect the stop
		 *	flag and exit.  The *Stop() calls above send
		 *	SIGTERM only to specific daemon PIDs (clocks,
		 *	cpsd, transit); other processes (CLAs, forwarders,
		 *	admin endpoints) rely on polling the stop flag
		 *	via semaphore-gated checks, typically on a
		 *	1-second timeout cycle.  Without this grace
		 *	period, ionTerminate/sm_ipc_stop would destroy
		 *	shared resources while those processes still
		 *	need them to detect the shutdown.		*/

		printText("Waiting for processes to finish shutting down...");
		snooze(3);

		{
			if( deletesdr )
			{
				printText("Deleting SDR");
				ionTerminate(1);
			}

			if( stopipc )
			{
				printText("Shutting down the IPC system");
				sm_ipc_stop();
			}
		}

	}
	else
	{
		printText("Unable to attach to ION");
	}


	printText("Stopping ionexit.");

	return (errcount != 0)? -1 : 0;
}
