/*
	ltpclo.c:	BP LTP-based convergence-layer output
			daemon, designed to serve as an output duct.

	Author: Scott Burleigh, JPL

	Copyright (c) 2007, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
	
									*/
#include "ltpcla.h"
#include "zco.h"

static sm_SemId		ltpcloSemaphore(sm_SemId *semid)
{
	long		temp;
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

	temp = (long) value;
	semaphore = temp;
	return semaphore;
}

static void	shutDownClo()	/*	Commands CLO termination.	*/
{
	isignal(SIGTERM, shutDownClo);
	sm_SemEnd(ltpcloSemaphore(NULL));
}

/*	*	*	Main thread functions	*	*	*	*/

#if defined (VXWORKS) || defined (RTEMS) || defined (bionic)
int	ltpclo(int a1, int a2, int a3, int a4, int a5,
		int a6, int a7, int a8, int a9, int a10)
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
	int		allGreen = 0;		/*	Boolean		*/
	vast		destEngineNbr;
	Outduct		outduct;
	Outflow		outflows[3];
	int		i;
	int		running = 1;
	Object		bundleZco;
	BpExtendedCOS	extendedCOS;
	char		destDuctName[MAX_CL_DUCT_NAME_LEN + 1];
	unsigned int	redPartLength;
	LtpSessionId	sessionId;

	if (ductName == NULL)
	{
		PUTS("Usage: ltpclo [-]<destination engine number>");
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

	CHKERR(sdr_begin_xn(sdr));
	sdr_read(sdr, (char *) &outduct, sdr_list_data(sdr, vduct->outductElt),
			sizeof(Outduct));
	sdr_exit_xn(sdr);
	destEngineNbr = strtovast(ductName);
	if (destEngineNbr < 0)
	{
		allGreen = 1;
		destEngineNbr = 0 - destEngineNbr;
	}

	memset((char *) outflows, 0, sizeof outflows);
	outflows[0].outboundBundles = outduct.bulkQueue;
	outflows[1].outboundBundles = outduct.stdQueue;
	outflows[2].outboundBundles = outduct.urgentQueue;
	for (i = 0; i < 3; i++)
	{
		outflows[i].svcFactor = 1 << i;
	}

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
		if (bpDequeue(vduct, outflows, &bundleZco, &extendedCOS,
				destDuctName, 0, -1) < 0)
		{
			running = 0;	/*	Terminate CLO.		*/
			continue;
		}

		if (bundleZco == 0)	/*	Interrupted.		*/
		{
			continue;
		}

		if (allGreen)
		{
			redPartLength = 0;
		}
		else
		{
			if (extendedCOS.flags & BP_BEST_EFFORT)
			{
				redPartLength = 0;
			}
			else
			{
				redPartLength = LTP_ALL_RED;
			}
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
