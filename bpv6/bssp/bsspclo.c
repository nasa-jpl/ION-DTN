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
#include "ipnfw.h"
#include "zco.h"

static sm_SemId		bsspcloSemaphore(sm_SemId *semid)
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
	sm_SemEnd(bsspcloSemaphore(NULL));
}

/*	*	*	Main thread functions	*	*	*	*/

typedef struct
{
	CbheEid		source;
	CbheEid		dest;
	BpTimestamp	lastBundle;
} BundleStream;

static int	isInOrder(Lyst streams, Bundle *bundle)
{
	LystElt		elt;
	BundleStream	*stream;

	for (elt = lyst_first(streams); elt; elt = lyst_next(elt))
	{
		stream = lyst_data(elt);
		if (stream->source.nodeNbr < bundle->id.source.c.nodeNbr)
		{
			continue;
		}

		if (stream->source.nodeNbr > bundle->id.source.c.nodeNbr)
		{
			break;
		}

		if (stream->source.serviceNbr < bundle->id.source.c.serviceNbr)
		{
			continue;
		}

		if (stream->source.serviceNbr > bundle->id.source.c.serviceNbr)
		{
			break;
		}

		if (stream->dest.nodeNbr < bundle->destination.c.nodeNbr)
		{
			continue;
		}

		if (stream->dest.nodeNbr > bundle->destination.c.nodeNbr)
		{
			break;
		}

		if (stream->dest.serviceNbr < bundle->destination.c.serviceNbr)
		{
			continue;
		}

		if (stream->dest.serviceNbr > bundle->destination.c.serviceNbr)
		{
			break;
		}

		/*	Found matching stream.				*/

		if (bundle->id.creationTime.seconds
				> stream->lastBundle.seconds
		|| (bundle->id.creationTime.seconds
				== stream->lastBundle.seconds
			&& bundle->id.creationTime.count
				> stream->lastBundle.count))
		{
			stream->lastBundle.seconds =
					bundle->id.creationTime.seconds;
			stream->lastBundle.count =
					bundle->id.creationTime.count;
			return 1;
		}

		return 0;
	}

	stream = (BundleStream *) MTAKE(sizeof(BundleStream));
	if (stream == NULL)
	{
		return 0;
	}

	if (elt)
	{
		elt = lyst_insert_before(elt, stream);
	}
	else
	{
		elt = lyst_insert_last(streams, stream);
	}

	if (elt == NULL)
	{
		MRELEASE(stream);
		return 0;
	}

	stream->source.nodeNbr = bundle->id.source.c.nodeNbr;
	stream->source.serviceNbr = bundle->id.source.c.serviceNbr;
	stream->dest.nodeNbr = bundle->destination.c.nodeNbr;
	stream->dest.serviceNbr = bundle->destination.c.serviceNbr;
	stream->lastBundle.seconds = bundle->id.creationTime.seconds;
	stream->lastBundle.count = bundle->id.creationTime.count;
	return 1;
}

static void	eraseStream(LystElt elt, void *userData)
{
	BundleStream	*stream = lyst_data(elt);

	MRELEASE(stream);
}

#if defined (ION_LWT)
int	bsspclo(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
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
	vast		destEngineNbr;
	Outduct		outduct;
	ClProtocol	protocol;
	int		running = 1;
	Object		bundleZco;
	BpAncillaryData	ancillaryData;
	BsspSessionId	sessionId;
	unsigned char	*buffer;
	Lyst		streams;
	Bundle		bundleImage;
	char		*dictionary = 0;
	unsigned int	bundleLength;

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

	buffer = (unsigned char *) MTAKE(BP_MAX_BLOCK_SIZE);
	if (buffer == NULL)
	{
		putErrmsg("Can't get buffer for decoding bundle ZCOs.", NULL);
		return -1;
	}

	streams = lyst_create_using(getIonMemoryMgr());
	if (streams == NULL)
	{
		putErrmsg("Can't create lyst of streams.", NULL);
		MRELEASE(buffer);
		return -1;
	}

	lyst_delete_set(streams, eraseStream, NULL);
	ipnInit();
	CHKERR(sdr_begin_xn(sdr));		/*	Lock the heap.	*/
	sdr_read(sdr, (char *) &outduct, sdr_list_data(sdr, vduct->outductElt),
			sizeof(Outduct));
	sdr_read(sdr, (char *) &protocol, outduct.protocol, sizeof(ClProtocol));
	sdr_exit_xn(sdr);
	destEngineNbr = strtovast(ductName);
	if (bssp_attach() < 0)
	{
		putErrmsg("bsspclo can't initialize BSSP.", NULL);
		lyst_destroy(streams);
		MRELEASE(buffer);
		return -1;
	}

	/*	Set up signal handling.  SIGTERM is shutdown signal.	*/

	oK(bsspcloSemaphore(&(vduct->semaphore)));
	isignal(SIGTERM, shutDownClo);

	/*	Can now begin transmitting to remote duct.		*/

	writeMemo("[i] bsspclo is running.");
	while (running && !(sm_SemEnded(bsspcloSemaphore(NULL))))
	{
		if (bpDequeue(vduct, &bundleZco, &ancillaryData, -1) < 0)
		{
			putErrmsg("Can't dequeue bundle.", NULL);
			break;
		}

		if (bundleZco == 0)	/*	Outduct closed.		*/
		{
			writeMemo("[i] bsspclo outduct closed.");
			running = 0;	/*	Terminate CLO.		*/
			continue;
		}

		if (bundleZco == 1)	/*	Got a corrupt bundle.	*/
		{
			continue;	/*	Get next bundle.	*/
		}

		if (decodeBundle(sdr, bundleZco, buffer, &bundleImage,
				&dictionary, &bundleLength) < 0)
		{
			putErrmsg("Can't decode bundle ZCO.", NULL);
			CHKERR(sdr_begin_xn(sdr));
			zco_destroy(sdr, bundleZco);
			if (sdr_end_xn(sdr) < 0)
			{
				putErrmsg("Failed destroying ZCO.", NULL);
				break;
			}

			continue;
		}

		switch (bssp_send(destEngineNbr, BpBsspClientId, bundleZco,
				isInOrder(streams, &bundleImage), &sessionId))
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
	lyst_destroy(streams);
	MRELEASE(buffer);
	ionDetach();
	return 0;
}
