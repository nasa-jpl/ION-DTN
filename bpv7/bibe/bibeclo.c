/*
	bibeclo.c:	BP BIBE-based convergence-layer output
			daemon, for use with BPv7.

			Like any CLO, bibeclo takes as input (via
			outduct) an outbound serialized bundle.  It
			encapsulates a copy of this bundle in a
			message (called a bpdu) and then, acting
			as a BP application, uses a BP SAP to request
			that the bundle protocol agent accept the
			bpdu as an application data unit to be used
			as the payload of a new outbound bundle.

	Author:		Scott Burleigh, JPL

	Copyright (c) 2020, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.

									*/
#include "bpP.h"
#include "bibeP.h"
#include "cbr.h"

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

static void	shutDownClo(void)
{
	sm_SemEnd(bibecloSemaphore(NULL));
}

static void	handleQuit(int signum)
{
	/* Tell the compiler that we are not using 'signum' */
	(void)signum;

	isignal(SIGTERM, handleQuit);
	bp_interrupt(_bpduSap(NULL));
	shutDownClo();
}

/*	*	*	Main thread functions	*	*	*	*/

#if defined (VXWORKS) || defined (RTEMS) || defined (bionic)
int	bibeclo(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
	char		*peerEid = (char *) a1;
#else
int	main(int argc, char *argv[])
{
	char		*peerEid = argc > 1 ? argv[1] : NULL;
#endif
	VOutduct	*vduct;
	PsmAddress	vductElt;
	Outduct		outduct;
	Object		bclaAddr;
	Object		bclaElt;
	Bcla		bcla;
	char		sourceEid[SDRSTRING_BUFSZ];
	char		reportToBuffer[SDRSTRING_BUFSZ];
	char		*reportToEid;
	Sdr		sdr;
	int		ttl;
	BpSAP		sap;
	unsigned char	*buffer;
	Object		bundleZco;
	vast		bundleZcoLength;
	Object		bpduZco;
	BpAncillaryData	ancillaryData;
	unsigned char	*cursor;
	uvast		uvtemp;
	int		hdrlen;

	if (peerEid == NULL)
	{
		PUTS("Usage: bibeclo <peer node's endpoint ID>");
		return 0;
	}

	if (bpAttach() < 0)
	{
		putErrmsg("bibeclo can't attach to BP.", NULL);
		return -1;
	}

	findOutduct("bibe", peerEid, &vduct, &vductElt);
	if (vductElt == 0)
	{
		writeMemoNote("[?] No such bibe outduct", peerEid);
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

	/*	Command-line argument is now validated.			*/

	sdr = getIonsdr();
	CHKZERO(sdr_begin_xn(sdr));
	sdr_read(sdr, (char *) &bcla, bclaAddr, sizeof(Bcla));
	sdr_string_read(sdr, sourceEid, bcla.source);
	reportToEid = NULL;
	if (bcla.reportTo)
	{
		sdr_string_read(sdr, reportToBuffer, bcla.reportTo);
		if (strlen(reportToBuffer) > 0)
		{
			reportToEid = reportToBuffer;
		}
	}

	sdr_read(sdr, (char *) &outduct, sdr_list_data(sdr, vduct->outductElt),
			sizeof(Outduct));
	sdr_exit_xn(sdr);

	/*	The properties of the encapsulating bundle are taken
	 *	from the bcla as configured by bibeadmin.		*/

	bcla.ancillaryData.flags &= (BP_MINIMUM_LATENCY | BP_PROTOCOL_ANY);
	ttl = bcla.fwdLatency + bcla.rtnLatency;	/*	seconds	*/
	if (bcla.lifespan > ttl)
	{
		ttl = bcla.lifespan;
	}

	if (bp_open_source(sourceEid, &sap, 0) < 0)
	{
		putErrmsg("Can't open source SAP.", sourceEid);
		shutDownClo();
		return -1;
	}

	_bpduSap(&sap);

	/*	Set up signal handling.  SIGTERM is shutdown signal.	*/

	oK(bibecloSemaphore(&(vduct->semaphore)));
	isignal(SIGTERM, handleQuit);

	/*	Allocate buffer for admin record header.		*/

	buffer = (unsigned char *) MTAKE(1);	/*	Array (1) open	*/
	if (buffer == NULL)
	{
		bp_close(sap);
		putErrmsg("Can't create buffer for CLO; stopping.", NULL);
		return -1;
	}

	/*	Can now begin transmitting to remote duct.		*/

	writeMemoNote("[i] bibeclo is now transmitting to", peerEid);
	while (!(sm_SemEnded(vduct->semaphore)))
	{
		if (bpDequeue(vduct, &bundleZco, &ancillaryData, -1) < 0)
		{
			putErrmsg("Can't dequeue bundle.", NULL);
			shutDownClo();
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

		/*	The BPDU (a message comprising a header
		 *	followed by the serialized outbound bundle;
		 *	the payload of the encapsulating bundle) will
		 *	be formed by prepending a BPDU message header
		 *	to a copy of the outbound bundle.		*/

		bundleZcoLength = zco_length(sdr, bundleZco);
		CHKZERO(sdr_begin_xn(sdr));
		zco_bond(sdr, bundleZco);
		bpduZco = zco_clone(sdr, bundleZco, 0, bundleZcoLength);
		if (sdr_end_xn(sdr))
		{
			putErrmsg("Can't clone source bundle; CLO stopping.",
					NULL);
			shutDownClo();
			continue;
		}

		/*	Serialize the BPDU record, an array of 1
		 *	element.					*/

		cursor = buffer;
		uvtemp = 1;
		oK(cbor_encode_array_open(uvtemp, &cursor));

		/*	Sole element of content array is the
		 *	encapsulated bundle, represented as a
		 *	byte string.					*/

		uvtemp = bundleZcoLength;
		oK(cbor_encode_byte_string(NULL, uvtemp, &cursor));
		hdrlen = cursor - buffer;
		CHKZERO(sdr_begin_xn(sdr));
		zco_prepend_header(sdr, bpduZco, (char *) buffer, hdrlen);
		if (sdr_end_xn(sdr))
		{
			putErrmsg("Can't prepend header; CLO stopping.", NULL);
			shutDownClo();
			continue;
		}

		/*	Send bundle whose payload is the ZCO
		 *	comprising the BPDU message header and the
		 *	encapsulated bundle.
		 *
		 *	Note that ttl must be converted from seconds
		 *	to milliseconds for BP processing.		*/

		switch (bpSend(&(sap->endpointMetaEid),
				peerEid, reportToEid, ttl * 1000,
				bcla.classOfService, NoCustodyRequested,
				bcla.bsrFlags, 0, &bcla.ancillaryData,
				bpduZco, NULL, 0))
		{
		case -1:	/*	System error.			*/
			putErrmsg("Can't send encapsulating bundle.", NULL);
			shutDownClo();
			continue;

		case 0:		/*	Malformed request.		*/
			writeMemo("[?] Encapsulating bundle not sent.");
			CHKZERO(sdr_begin_xn(sdr));
			zco_destroy(sdr, bpduZco);
			if (sdr_end_xn(sdr))
			{
				putErrmsg("Can't recover; CLO stopping.", NULL);
				shutDownClo();
				continue;
			}

			if (bpHandleXmitFailure(bundleZco) < 0)
			{
				putErrmsg("Can't handle xmit failure.", NULL);
				shutDownClo();
				continue;
			}

			break;

		default:
			CHKZERO(sdr_begin_xn(sdr));
			if (bpHandleXmitSuccess(bundleZco) < 0)
			{
				putErrmsg("Can't handle xmit success.", NULL);
				shutDownClo();
				continue;
			}

			if (sdr_end_xn(sdr))
			{
				putErrmsg("Can't release ZCO; CLO stopping.",
						NULL);
				shutDownClo();
				continue;
			}
		}

		/*	Make sure other tasks have a chance to run.	*/

		sm_TaskYield();
	}

	writeErrmsgMemos();
	writeMemo("[i] bibeclo duct has ended.");
	MRELEASE(buffer);
	bp_close(sap);
	ionDetach();
	return 0;
}
