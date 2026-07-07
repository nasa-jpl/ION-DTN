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
	Sdr		sdr;
	char		sourceEid[SDRSTRING_BUFSZ];
	uvast		threshold;
	char		reportToBuffer[SDRSTRING_BUFSZ];
	char		*reportToEid;
	int		ttl;
	BpAncillaryData	bibeAncillaryData;
	BpSAP		sap;
	unsigned char	*buffer;
	Object		bundleZco;
	BpAncillaryData	ancillaryData;
	vast		bundleZcoLength;
	uvast		remainingSourceLength;
	uvast		offset;
	uvast		segmentLength;
	Object		bpduZco;
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

	bibeFind(peerEid, &bclaAddr, &bclaElt);
	if (bclaElt == 0)
	{
		writeMemoNote("[?] No such bcla", peerEid);
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

	/*	Command-line argument is now validated.			*/

	sdr = getIonsdr();
	CHKZERO(sdr_begin_xn(sdr));
	sdr_read(sdr, (char *) &bcla, bclaAddr, sizeof(Bcla));
	sdr_string_read(sdr, sourceEid, bcla.source);

	/*	The properties of the encapsulating bundle are taken
	 *	from the bcla as configured by bibeadmin.		*/

	if (bcla.threshold == 0)
	{
		threshold = (uvast) -1;		/*	Max integer.	*/
	}
	else
	{
		threshold = bcla.threshold;
	}

	reportToEid = NULL;
	if (bcla.reportTo)
	{
		sdr_string_read(sdr, reportToBuffer, bcla.reportTo);
		if (strlen(reportToBuffer) > 0)
		{
			reportToEid = reportToBuffer;
		}
	}

	ttl = bcla.lifespan;
	memcpy((char *) &bibeAncillaryData, (char *) &bcla.ancillaryData,
			sizeof(BpAncillaryData));
	bibeAncillaryData.flags &= (BP_MINIMUM_LATENCY | BP_PROTOCOL_ANY);
	sdr_read(sdr, (char *) &outduct, sdr_list_data(sdr, vduct->outductElt),
			sizeof(Outduct));
	sdr_exit_xn(sdr);

	/*	Open BP service access point for sending bundles.	*/

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

	/*	Allocate buffer for BPDU header.			*/

	buffer = (unsigned char *) MTAKE(1	/*	Array () open	*/
			+ sizeof(uvast)		/*	Transfer ID	*/
			+ sizeof(uvast)		/*	Total length	*/
			+ sizeof(uvast)		/*	Offset		*/
					);
	if (buffer == NULL)
	{
		bp_close(sap);
		putErrmsg("Can't create buffer for CLO; stopping.", NULL);
		return -1;
	}

	/*	Can now begin transmitting to remote bibecli duct.	*/

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

		bundleZcoLength = zco_length(sdr, bundleZco);

		/*	Each BPDU (a message comprising a header
		 *	followed by a serialized outbound bundle
		 *	segment; the payload of the encapsulating
		 *	bundle) will be formed by prepending a
		 *	BPDU message header to a clone of part or
		 *	all of the outbound bundle.			*/

		remainingSourceLength = bundleZcoLength;
		offset = 0;

		/*	Get transfer ID for segmentation.		*/

		CHKZERO(sdr_begin_xn(sdr));
		sdr_stage(sdr, (char *) &bcla, bclaAddr, sizeof(Bcla));
		bcla.count++;
		sdr_write(sdr, bclaAddr, (char *) &bcla, sizeof(Bcla));

		/*	Send bundles encapsulating segments of bundle.	*/

		zco_bond(sdr, bundleZco);
		while (remainingSourceLength > 0)
		{
			if (remainingSourceLength > threshold)
			{
				segmentLength = threshold;
			}
			else
			{
				segmentLength = remainingSourceLength;
			}

			bpduZco = zco_clone(sdr, bundleZco, offset,
					segmentLength);
			if (sdr_end_xn(sdr))
			{
				putErrmsg("Can't clone from source bundle; \
CLO stopping.", NULL);
				remainingSourceLength = 0;
				continue;
			}

			/*	Serialize the BPDU record, an array
			 *	of 1 or 4 elements.			*/

			cursor = buffer;
			if (segmentLength == bundleZcoLength)
			{
				/*	Simple BPDU, no segmentation.	*/

				uvtemp = 1;
				oK(cbor_encode_array_open(uvtemp, &cursor));
			}
			else
			{
				/*	Segment BPDU.			*/

				uvtemp = 4;
				oK(cbor_encode_array_open(uvtemp, &cursor));
				uvtemp = bcla.count;
				oK(cbor_encode_integer(uvtemp, &cursor));
				uvtemp = bundleZcoLength;
				oK(cbor_encode_integer(uvtemp, &cursor));
				uvtemp = offset;
				oK(cbor_encode_integer(uvtemp, &cursor));
			}

			/*	Last element of content array is the
		 	*	encapsulated bundle segment, a CBOR
		 	*	byte string.				*/

			uvtemp = segmentLength;
			oK(cbor_encode_byte_string(NULL, uvtemp, &cursor));

			/*	Complete construction of BPDU.		*/

			hdrlen = cursor - buffer;
			CHKZERO(sdr_begin_xn(sdr));
			zco_prepend_header(sdr, bpduZco, (char *) buffer,
					hdrlen);
			if (sdr_end_xn(sdr))
			{
				putErrmsg("Can't prepend header; CLO stopping.",
						NULL);
				shutDownClo();
				remainingSourceLength = 0;
				continue;
			}

			/*	Send bundle whose payload is the BPDU,
			 *	i.e., a ZCO comprising the BPDU message
			 *	header and the encapsulated bundle
			 *	segment.
			 *
			 *	Note that ttl must be converted from
			 *	seconds to milliseconds for BP
			 *	processing.				*/

			switch (bpSend(&(sap->endpointMetaEid),
					peerEid, reportToEid, ttl * 1000,
					bcla.classOfService, NoCustodyRequested,
					bcla.bsrFlags, 0, &bibeAncillaryData,
					bpduZco, NULL, 0))
			{
			case -1:	/*	System error.		*/
				putErrmsg("Can't send encapsulating bundle.",
						NULL);
				remainingSourceLength = 0;
				continue;

			case 0:		/*	Malformed request.	*/
				writeMemo("[?] Encapsulating bundle not sent.");
				remainingSourceLength = 0;
				continue;
			}

			/*	Successful transmission of BIBE bundle.	*/

			remainingSourceLength -= segmentLength;
			offset += segmentLength;
		}

		/*	Done with segmentation and transmission.	*/

		if (offset < bundleZcoLength)	/*	Didn't finish.	*/
		{
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
		}
		else	/*	Successful BIBE encapsulation.		*/
		{
			if (bpHandleXmitSuccess(bundleZco) < 0)
			{
				putErrmsg("Can't handle xmit success.", NULL);
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
