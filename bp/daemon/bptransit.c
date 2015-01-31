/*
	bptransit.c:	pseudo-application for inserting received
			bundles into the Outbound ZCO account for
			forwarding under admission control.

	Author: Scott Burleigh, JPL

	Copyright (c) 2015, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
	
									*/
#include "bpP.h"
#include "bei.h"
#include "sdrhash.h"
#include "smlist.h"

/*	The following functions are nominally private to libbpP.c but
 *	we do need access to them in this one additional source file.	*/

extern void	noteBundleInserted(Bundle *bundle);
extern void	noteBundleRemoved(Bundle *bundle);

/*	Daemon termination control.					*/

static ReqAttendant	*_attendant(ReqAttendant *newAttendant)
{
	static ReqAttendant	*attendant = NULL;

	if (newAttendant)
	{
		attendant = newAttendant;
	}

	return attendant;
}

static void	shutDown()	/*	Commands bptransit termination.	*/
{
	ionPauseAttendant(_attendant(NULL));
}

#if defined (ION_LWT)
int	bptransit(int a1, int a2, int a3, int a4, int a5,
		int a6, int a7, int a8, int a9, int a10)
{
#else
int	main(int argc, char *argv[])
{
#endif
	Sdr		sdr;
	BpDB		*db;
	BpVdb		*vdb;
	ReqAttendant	attendant;
	int		running = 1;
	Object		elt;
	Object		bundleAddr;
	Bundle		bundle;
	int		priority;
	char		*dictionary;
	Object		newPayload;
	char		*eidString;
	int		result;

	if (bpAttach() < 0)
	{
		putErrmsg("bptransit can't attach to BP.", NULL);
		return 1;
	}

	sdr = getIonsdr();
	db = getBpConstants();
	vdb = getBpVdb();
	if (ionStartAttendant(&attendant))
	{
		putErrmsg("Can't initialize blocking transit.", NULL);
		return 1;
	}

	oK(_attendant(&attendant));
	isignal(SIGTERM, shutDown);

	/*	Main loop: get in-transit bundle, copy it into Outbound
	 *	ZCO space, delete it from Inbound ZCO space.		*/

	writeMemo("[i] bptransit is running.");
	while (running && !(sm_SemEnded(vdb->transitSemaphore)))
	{
		CHKZERO(sdr_begin_xn(sdr));
		elt = sdr_list_first(sdr, db->transitQueue);
		if (elt == 0)	/*	Wait for in-transit notice.	*/
		{
			sdr_exit_xn(sdr);
			if (sm_SemTake(vdb->transitSemaphore) < 0)
			{
				putErrmsg("Can't take transit semaphore.",
						NULL);
				running = 0;
			}

			continue;
		}

		bundleAddr = (Object) sdr_list_data(sdr, elt);
		sdr_stage(sdr, (char *) &bundle, bundleAddr, sizeof(Bundle));
		if ((dictionary = retrieveDictionary(&bundle))
				== (char *) &bundle)
		{
			sdr_exit_xn(sdr);
			putErrmsg("Can't retrieve dictionary.", NULL);
			running = 0;
			continue;
		}

		priority = COS_FLAGS(bundle.bundleProcFlags) & 0x03;
		newPayload = ionCreateZco(ZcoZcoSource, bundle.payload.content,
				0, bundle.payload.length, priority,
				bundle.extendedCOS.ordinal, ZcoOutbound,
				&attendant);
		switch (newPayload)
		{
		case (Object) ERROR:
			releaseDictionary(dictionary);
			sdr_cancel_xn(sdr);
			putErrmsg("Can't create payload ZCO.", NULL);
			running = 0;
			continue;

		case 0:		/*	Not enough ZCO space.		*/
			releaseDictionary(dictionary);
			sdr_cancel_xn(sdr);
			running = 0;
			continue;

		default:
			break;			/*	Out of switch.	*/
		}

		/*	New payload ZCO has been created; still in xn.	*/

		sdr_list_delete(sdr, elt, NULL, NULL);
		bundle.transitElt = 0;
		noteBundleRemoved(&bundle);	/*	Out of Inbound.	*/
		zco_destroy(sdr, bundle.payload.content);
		bundle.payload.content = newPayload;
		bundle.acct = ZcoOutbound;
		if (patchExtensionBlocks(&bundle) < 0)
		{
			releaseDictionary(dictionary);
			sdr_cancel_xn(sdr);
			putErrmsg("Can't insert missing extensions.", NULL);
			running = 0;
			continue;
		}

		sdr_write(sdr, bundleAddr, (char *) &bundle, sizeof(Bundle));
		noteBundleInserted(&bundle);	/*	Into Outbound.	*/

		/*	Bundle is now ready to be forwarded.		*/

		if (printEid(&bundle.destination, dictionary, &eidString) < 0)
		{
			releaseDictionary(dictionary);
			sdr_cancel_xn(sdr);
			putErrmsg("Can't print destination EID.", NULL);
			running = 0;
			continue;
		}

		result = forwardBundle(bundleAddr, &bundle, eidString);
		MRELEASE(eidString);
		releaseDictionary(dictionary);
		if (result < 0)
		{
			sdr_cancel_xn(sdr);
			putErrmsg("Can't enqueue bundle for forwarding.", NULL);
			running = 0;
			continue;
		}

		if (sdr_end_xn(sdr) < 0)
		{
			putErrmsg("Failed migrating in-transit bundle.", NULL);
			running = 0;
		}
	}

	writeErrmsgMemos();
	writeMemo("[i] bptransit has ended.");
	ionDetach();
	return 0;
}
