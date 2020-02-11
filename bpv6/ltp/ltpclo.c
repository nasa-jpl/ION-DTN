/*
	ltpclo.c:	BP LTP-based convergence-layer output
			daemon, designed to serve as an output duct.

	Author: Scott Burleigh, JPL

	Copyright (c) 2007, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
	
									*/
#include "ltpcla.h"
#include "ipnfw.h"
#include "zco.h"

static sm_SemId		ltpcloSemaphore(sm_SemId *semid)
{
	uaddr		temp;
	void		*value;
	sm_SemId	semaphore;
	
	if (semid)			/*	Add task variable.	*/
	{
		temp = *semid;
		value = (void *) temp;
		value = sm_TaskVar(&value);
	}
	else				/*	Retrieve task variable.	*/
	{
		value = sm_TaskVar(NULL);
	}

	temp = (uaddr) value;
	semaphore = temp;
	return semaphore;
}

static void	shutDownClo(int signum)
{
	isignal(SIGTERM, shutDownClo);
	sm_SemEnd(ltpcloSemaphore(NULL));
}

/*	*	*	Main thread functions	*	*	*	*/

#if defined (ION_LWT)
int	ltpclo(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
	char		*ductName = (char *) a1;
#else
int	main(int argc, char *argv[])
{
	char		*ductName = (argc > 1 ? argv[1] : NULL);
#endif
	Sdr		sdr;
	VOutduct	*vduct;
	PsmAddress	vductElt;
	uvast		destEngineNbr;
	Outduct		outduct;
	int		running = 1;
	Object		bundleZco;
	BpAncillaryData	ancillaryData;
	unsigned int	redPartLength;
	LtpSessionId	sessionId;

	if (ductName == NULL)
	{
		PUTS("Usage: ltpclo <destination engine number>");
		return 0;
	}

	if (bpAttach() < 0)
	{
		putErrmsg("ltpclo can't attach to BP.", NULL);
		return -1;
	}

	sdr = getIonsdr();
	findOutduct("ltp", ductName, &vduct, &vductElt);
	if (vductElt == 0)
	{
		putErrmsg("No such ltp duct.", ductName);
		return -1;
	}

	if (vduct->cloPid != ERROR && vduct->cloPid != sm_TaskIdSelf())
	{
		putErrmsg("CLO task is already started for this duct.",
				itoa(vduct->cloPid));
		return -1;
	}

	/*	All command-line arguments are now validated.		*/

	ipnInit();
	CHKERR(sdr_begin_xn(sdr));		/*	Lock the heap.	*/
	sdr_read(sdr, (char *) &outduct, sdr_list_data(sdr, vduct->outductElt),
			sizeof(Outduct));
	sdr_exit_xn(sdr);			/*	Unlock.		*/
	destEngineNbr = strtouvast(ductName);
	if (ltp_attach() < 0)
	{
		putErrmsg("ltpclo can't initialize LTP.", NULL);
		return -1;
	}

	/*	Set up signal handling.  SIGTERM is shutdown signal.	*/

	oK(ltpcloSemaphore(&(vduct->semaphore)));
	isignal(SIGTERM, shutDownClo);

	/*	Can now begin transmitting to remote duct.		*/

	writeMemo("[i] ltpclo is running.");
	while (running && !(sm_SemEnded(ltpcloSemaphore(NULL))))
	{
		if (bpDequeue(vduct, &bundleZco, &ancillaryData, -1) < 0)
		{
			putErrmsg("Can't dequeue bundle.", NULL);
			break;
		}

		if (bundleZco == 0)	/*	Outduct closed.		*/
		{
			writeMemo("[i] ltpclo outduct closed.");
			running = 0;	/*	Terminate CLO.		*/
			continue;
		}

		if (bundleZco == 1)	/*	Got a corrupt bundle.	*/
		{
			continue;	/*	Get next bundle.	*/
		}

		if (ancillaryData.flags & BP_BEST_EFFORT)
		{
			redPartLength = 0;
		}
		else
		{
			redPartLength = LTP_ALL_RED;
		}

		switch (ltp_send(destEngineNbr, BpLtpClientId, bundleZco,
				redPartLength, &sessionId))
		{
		case 0:
			putErrmsg("Unable to send this bundle via LTP.", NULL);
			break;

		case -1:
			putErrmsg("LtpSend failed.", NULL);
			running = 0;	/*	Terminate CLO.		*/
		}

		/*	Make sure other tasks have a chance to run.	*/

		sm_TaskYield();

		/*	Note: bundleZco is destroyed later, when LTP's
		 *	ExportSession is closed following transmission
		 *	of bundle ZCOs as aggregated into a block.	*/
	}

	writeErrmsgMemos();
	writeMemo("[i] ltpclo duct has ended.");
	ionDetach();
	return 0;
}
