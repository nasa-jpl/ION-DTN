/*
	bibeclo.c:	BP BIBE-based convergence-layer output
			daemon.  Note that this convergence-layer
			output daemon is a "promiscuous" CLO daemon
			which can transmit bundles to any number of
			different peers.

	Author:		Scott Burleigh, JPL

	Copyright (c) 2014, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
	
									*/
#include "bpP.h"

static sm_SemId		bibecloSemaphore(sm_SemId *semid)
{
	static sm_SemId	semaphore = -1;
	
	if (semid)
	{
		semaphore = *semid;
	}

	return semaphore;
}

static void	shutDownClo()	/*	Commands CLO termination.	*/
{
	sm_SemEnd(bibecloSemaphore(NULL));
}

/*	*	*	Main thread functions	*	*	*	*/

#if defined (VXWORKS) || defined (RTEMS) || defined (bionic)
int	bibeclo(int a1, int a2, int a3, int a4, int a5,
		int a6, int a7, int a8, int a9, int a10)
{
#else
int	main(int argc, char *argv[])
{
#endif
	VOutduct		*vduct;
	PsmAddress		vductElt;
	Sdr			sdr;
	Outduct			outduct;
	ClProtocol		protocol;
	Outflow			outflows[3];
	int			i;
	unsigned char		*buffer;
	char			adminHeader[1];
	Object			bundleZco;
	BpExtendedCOS		extendedCOS;
	char			destDuctName[MAX_CL_DUCT_NAME_LEN + 1];
	Bundle			image;
	char			*dictionary = 0;
	unsigned int		bundleLength;
	time_t			currentTime;
	unsigned int		bundleAge;
	unsigned int		ttl;
	Object			newBundle;

	if (bpAttach() < 0)
	{
		putErrmsg("bibeclo can't attach to BP.", NULL);
		return -1;
	}

	findOutduct("bibe", "*", &vduct, &vductElt);
	if (vductElt == 0)
	{
		putErrmsg("No such bibe duct.", "*");
		return -1;
	}

	vduct->xmitThrottle.nominalRate = -1;	/*	No rate control.*/
	if (vduct->cloPid != ERROR && vduct->cloPid != sm_TaskIdSelf())
	{
		putErrmsg("CLO task is already started for this duct.",
				itoa(vduct->cloPid));
		return -1;
	}

	/*	All command-line arguments are now validated.		*/

	buffer = (unsigned char *) MTAKE(BP_MAX_BLOCK_SIZE);
	if (buffer == NULL)
	{
		putErrmsg("Can't create bundle for CLO; stopping.", NULL);
		return -1;
	}

	adminHeader[0] = BP_ENCAPSULATED_BUNDLE << 4;
	sdr = getIonsdr();
	CHKZERO(sdr_begin_xn(sdr));
	sdr_read(sdr, (char *) &outduct, sdr_list_data(sdr, vduct->outductElt),
			sizeof(Outduct));
	sdr_read(sdr, (char *) &protocol, outduct.protocol, sizeof(ClProtocol));
	sdr_exit_xn(sdr);
	memset((char *) outflows, 0, sizeof outflows);
	outflows[0].outboundBundles = outduct.bulkQueue;
	outflows[1].outboundBundles = outduct.stdQueue;
	outflows[2].outboundBundles = outduct.urgentQueue;
	for (i = 0; i < 3; i++)
	{
		outflows[i].svcFactor = 1 << i;
	}

	/*	Set up signal handling.  SIGTERM is shutdown signal.	*/

	oK(bibecloSemaphore(&(vduct->semaphore)));
	isignal(SIGTERM, shutDownClo);

	/*	Can now begin transmitting to remote duct.		*/

	writeMemo("[i] bibeclo is running.");
	while (!(sm_SemEnded(vduct->semaphore)))
	{
		if (bpDequeue(vduct, outflows, &bundleZco, &extendedCOS,
				destDuctName, outduct.maxPayloadLen, 0) < 0)
		{
			sm_SemEnd(bibecloSemaphore(NULL));/*	Stop.	*/
			continue;
		}

		if (bundleZco == 0)	/*	Interrupted.		*/
		{
			continue;
		}

		CHKZERO(sdr_begin_xn(sdr));
		if (decodeBundle(sdr, bundleZco, buffer, &image, &dictionary,
				&bundleLength) < 0)
		{
			putErrmsg("Can't decode bundle; CLO stopping.", NULL);
			shutDownClo();
			continue;
		}

		/*	Embed bundle in admin record.			*/

		zco_prepend_header(sdr, bundleZco, adminHeader, 1);
		if (sdr_end_xn(sdr))
		{
			putErrmsg("Can't prepend header; CLO stopping.", NULL);
			shutDownClo();
			continue;
		}

		currentTime = getUTCTime();
		bundleAge = currentTime -
			(image.id.creationTime.seconds + EPOCH_2000_SEC);
		ttl = image.timeToLive - bundleAge;
		switch (bpSend(NULL, destDuctName, NULL, ttl,
				COS_FLAGS(image.bundleProcFlags),
				NoCustodyRequested, 0, 0, &extendedCOS,
				bundleZco, &newBundle, BP_ENCAPSULATED_BUNDLE))
		{
		case -1:	/*	System error.			*/
			putErrmsg("Can't send encapsulated bundle.", NULL);
			shutDownClo();
			continue;

		case 0:		/*	Malformed request.		*/
			writeMemo("[!] Encapsulated bundle not sent.");
		}	

		/*	Make sure other tasks have a chance to run.	*/

		sm_TaskYield();
	}

	writeErrmsgMemos();
	writeMemo("[i] bibeclo duct has ended.");
	MRELEASE(buffer);
	ionDetach();
	return 0;
}
