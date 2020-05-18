/*
	bibeclo.c:	BP BIBE-based convergence-layer output
			daemon, for use with BPv6.

	Author:		Scott Burleigh, JPL

	Copyright (c) 2014, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
	
									*/
#include "bpP.h"
#include "bibeP.h"

static sm_SemId		bibecloSemaphore(sm_SemId *semid)
{
	static sm_SemId	semaphore = SM_SEM_NONE;
	
	if (semid)
	{
		semaphore = *semid;
	}

	return semaphore;
}

static BpSAP	_bpduSap(BpSAP *newSap)
{
	void	*value;
	BpSAP	sap;

	if (newSap)			/*	Add task variable.	*/
	{
		value = (void *) (*newSap);
		sap = (BpSAP) sm_TaskVar(&value);
	}
	else				/*	Retrieve task variable.	*/
	{
		sap = (BpSAP) sm_TaskVar(NULL);
	}

	return sap;
}

static void	shutDownClo(int signum)
{
	bp_interrupt(_bpduSap(NULL));
	sm_SemEnd(bibecloSemaphore(NULL));
}

/*	*	*	Main thread functions	*	*	*	*/

#if defined (VXWORKS) || defined (RTEMS) || defined (bionic)
int	bibeclo(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
	char			*peerEid = (char *) a1;
	char			*destEid = (char *) a2;
#else
int	main(int argc, char *argv[])
{
	char			*peerEid = argc > 1 ? argv[1] : NULL;
	char			*destEid = argc > 2 ? argv[2] : NULL;
#endif
	VOutduct		*vduct;
	PsmAddress		vductElt;
	Outduct			outduct;
	Object			bclaAddr;
	Object			bclaElt;
	Bcla			bcla;
	Sdr			sdr;
	char			adminHeader[1];
	char			sourceEid[SDRSTRING_BUFSZ];
	BpSAP			sap;
	Object			bundleZco;
	BpAncillaryData		ancillaryData;

	if (peerEid == NULL || destEid == NULL)
	{
		PUTS("Usage: bibeclo <peer node's ID> <destination node's ID>");
		return 0;
	}

	if (bpAttach() < 0)
	{
		putErrmsg("bibeclo can't attach to BP.", NULL);
		return -1;
	}

	findOutduct("bibe", destEid, &vduct, &vductElt);
	if (vductElt == 0)
	{
		writeMemoNote("[?] No such bibe outduct", destEid);
		return -1;
	}

	if (vduct->cloPid != ERROR && vduct->cloPid != sm_TaskIdSelf())
	{
		writeMemoNote("[?] CLO task is already started for this duct",
				itoa(vduct->cloPid));
		return -1;
	}

	bibeFind(peerEid, &bclaAddr, &bclaElt);
	if (bclaElt == 0)
	{
		writeMemoNote("[?] No such bcla", peerEid);
		return -1;
	}

	/*	All command-line arguments are now validated.		*/

	adminHeader[0] = BP_ENCAPSULATED_BUNDLE << 4;
	sdr = getIonsdr();
	CHKZERO(sdr_begin_xn(sdr));
	sdr_read(sdr, (char *) &bcla, bclaAddr, sizeof(Bcla));
	sdr_string_read(sdr, sourceEid, bcla.source);
	sdr_read(sdr, (char *) &outduct, sdr_list_data(sdr, vduct->outductElt),
			sizeof(Outduct));
	sdr_exit_xn(sdr);
	if (bp_open_source(sourceEid, &sap, 1) < 0)
	{
		putErrmsg("Can't open source SAP.", sourceEid);
		return -1;
	}

	_bpduSap(&sap);

	/*	Set up signal handling.  SIGTERM is shutdown signal.	*/

	oK(bibecloSemaphore(&(vduct->semaphore)));
	isignal(SIGTERM, shutDownClo);

	/*	Can now begin transmitting to remote duct.		*/

	writeMemoNote("[i] bibeclo is running for", destEid);
	writeMemoNote("[i]        transmitting to", peerEid);
	while (!(sm_SemEnded(vduct->semaphore)))
	{
		if (bpDequeue(vduct, &bundleZco, &ancillaryData, 0) < 0)
		{
			putErrmsg("Can't dequeue bundle.", NULL);
			shutDownClo(SIGTERM);
			break;
		}

		if (bundleZco == 0)	 /*	Outduct closed.		*/
		{
			writeMemo("[i] bibeclo outduct closed.");
			sm_SemEnd(bibecloSemaphore(NULL));/*	Stop.	*/
			continue;
		}

		if (bundleZco == 1)	/*	Got a corrupt bundle.	*/
		{
			continue;	/*	Get next bundle.	*/
		}

		memcpy((char *) &ancillaryData, (char *) &bcla.ancillaryData,
				sizeof(BpAncillaryData));

		/*	Embed bundle in admin record.			*/

		CHKZERO(sdr_begin_xn(sdr));
		zco_prepend_header(sdr, bundleZco, adminHeader, 1);
		zco_bond(sdr, bundleZco);
		if (sdr_end_xn(sdr))
		{
			putErrmsg("Can't prepend header; CLO stopping.", NULL);
			shutDownClo(SIGTERM);
			continue;
		}

		/*	Send bundle whose payload is the ZCO
		 *	comprising the admin record header and the
		 *	encapsulated bundle.				*/

		switch (bpSend(&(sap->endpointMetaEid), peerEid, NULL,
				bcla.lifespan, bcla.classOfService,
				NoCustodyRequested, 0, 0, &ancillaryData,
				bundleZco, NULL, BP_ENCAPSULATED_BUNDLE))
		{
		case -1:	/*	System error.			*/
			putErrmsg("Can't send encapsulated bundle.", NULL);
			shutDownClo(SIGTERM);
			continue;

		case 0:		/*	Malformed request.		*/
			writeMemo("[?] Encapsulated bundle not sent.");
		}	

		/*	Make sure other tasks have a chance to run.	*/

		sm_TaskYield();
	}

	writeErrmsgMemos();
	writeMemo("[i] bibeclo duct has ended.");
	bp_close(sap);
	ionDetach();
	return 0;
}
