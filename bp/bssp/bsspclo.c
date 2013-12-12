/*
 *	bsspclo.c:	BP BSSP-based convergence-layer output
 *			daemon, designed to serve as an output duct.
 *
 *	Authors: Sotirios-Angelos Lenas, SPICE
 *		 Scott Burleigh, JPL
 *
 *	Copyright (c) 2013, California Institute of Technology.
 *	Copyright (c) 2013, Space Internetworking Center,
 *	Democritus University of Thrace.
 *	
 *	All rights reserved. U.S. Government and E.U. Sponsorship acknowledged.
 *
 */
#include "bsspcla.h"
#include "zco.h"

static sm_SemId		bsspcloSemaphore(sm_SemId *semid)
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
	sm_SemEnd(bsspcloSemaphore(NULL));
}

/*	*	*	Main thread functions	*	*	*	*/

#if defined (VXWORKS) || defined (RTEMS) || defined (bionic)
int	bsspclo(int a1, int a2, int a3, int a4, int a5,
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
	vast		destEngineNbr;
	Outduct		outduct;
	ClProtocol	protocol;
	Outflow		outflows[3];
	int		i;
	int		running = 1;
	Object		bundleZco;
	BpExtendedCOS	extendedCOS;
	char		destDuctName[MAX_CL_DUCT_NAME_LEN + 1];
	BsspSessionId	sessionId;
	BundleInfo	info;
	Object		bundleAddr;
	Bundle		bundle;

	if (ductName == NULL)
	{
		PUTS("Usage: bsspclo [-]<destination engine number>");
		return 0;
	}

	if (bpAttach() < 0)
	{
		putErrmsg("bsspclo can't attach to BP.", NULL);
		return -1;
	}

	sdr = getIonsdr();
	findOutduct("bssp", ductName, &vduct, &vductElt);
	if (vductElt == 0)
	{
		putErrmsg("No such bssp duct.", ductName);
		return -1;
	}

	if (vduct->cloPid != ERROR && vduct->cloPid != sm_TaskIdSelf())
	{
		putErrmsg("BSSPCLO task is already started for this duct.",
				itoa(vduct->cloPid));
		return -1;
	}

	/*	All command-line arguments are now validated.		*/

	CHKERR(sdr_begin_xn(sdr));
	sdr_read(sdr, (char *) &outduct, sdr_list_data(sdr, vduct->outductElt),
			sizeof(Outduct));
	sdr_read(sdr, (char *) &protocol, outduct.protocol, sizeof(ClProtocol));
	sdr_exit_xn(sdr);
	destEngineNbr = strtovast(ductName);

	if (protocol.nominalRate == 0)
	{
		vduct->xmitThrottle.nominalRate = DEFAULT_BSSP_RATE;
	}
	else
	{
		vduct->xmitThrottle.nominalRate = protocol.nominalRate;
	}

	memset((char *) outflows, 0, sizeof outflows);
	outflows[0].outboundBundles = outduct.bulkQueue;
	outflows[1].outboundBundles = outduct.stdQueue;
	outflows[2].outboundBundles = outduct.urgentQueue;
	for (i = 0; i < 3; i++)
	{
		outflows[i].svcFactor = 1 << i;
	}

	if (bssp_attach() < 0)
	{
		putErrmsg("bsspclo can't initialize BSSP.", NULL);
		return -1;
	}

	/*	Set up signal handling.  SIGTERM is shutdown signal.	*/

	oK(bsspcloSemaphore(&(vduct->semaphore)));
	isignal(SIGTERM, shutDownClo);

	/*	Can now begin transmitting to remote duct.		*/

	writeMemo("[i] bsspclo is running.");
	while (running && !(sm_SemEnded(bsspcloSemaphore(NULL))))
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

		CHKERR(sdr_begin_xn(sdr));
		if (retrieveInTransitBundle(bundleZco, &bundleAddr) < 0)
		{
			putErrmsg("Can't locate in transit bundle.", NULL);
			sdr_cancel_xn(sdr);
			continue;
		}

		if (bundleAddr == 0) /* 	Highly unlikely 	*/
		{
			/*	Bundle not found, so we discard the ADU.   */
			zco_destroy(sdr, bundleZco);
			if (sdr_end_xn(sdr) < 0)
			{
				putErrmsg("Failed destroying ZCO.", NULL);
				break;
			}

			continue;
		}
		else
		{
			sdr_read(sdr, (char *) &bundle, bundleAddr,
					sizeof(Bundle));

			/*  Copy bundle information for later processing by	* 
			 *  Bundle Streaming Service Protocol that implements	*
			 *  the BSS-CL.  					*
			 */
			info.srcNodeNbr = bundle.id.source.c.nodeNbr;
			info.srcServiceNbr = bundle.id.source.c.serviceNbr;
			info.dstNodeNbr = bundle.destination.c.nodeNbr;
			info.dstServiceNbr =  bundle.destination.c.serviceNbr;
			info.creationTime = bundle.id.creationTime;

			sdr_exit_xn(sdr);
		}

		switch (bssp_send(destEngineNbr, BpBsspClientId, bundleZco,
				&sessionId, info))
		{
		case 0:
			putErrmsg("Unable to send this bundle via BSSP.", NULL);
			break;

		case -1:
			putErrmsg("BsspSend failed.", NULL);
			running = 0;	/*	Terminate CLO.		*/
		}

		/*	Make sure other tasks have a chance to run.	*/

		sm_TaskYield();

		/*	Note: bundleZco is destroyed later, when BSSP's
		 *	ExportSession is closed following transmission
		 *	of bundle ZCOs as aggregated into a block.	*/
	}

	writeErrmsgMemos();
	writeMemo("[i] bsspclo duct has ended.");
	ionDetach();
	return 0;
}
