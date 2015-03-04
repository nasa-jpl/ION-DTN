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

/*	*	*	Daemon termination control.			*/

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
	BpVdb	*vdb;

	vdb = getBpVdb();
	sm_SemEnd(vdb->confirmedTransitSemaphore);
	sm_SemEnd(vdb->provisionalTransitSemaphore);
	ionPauseAttendant(_attendant(NULL));
}

/*	*	Common functions for initiating bundle forwarding	*/

static int	initiateBundleForwarding(Sdr sdr, Bundle *bundle,
			Object bundleAddr, Object newPayload)
{
	char	*dictionary;
	char	*eidString;
	int	result;

	noteBundleRemoved(bundle);		/*	Out of Inbound.	*/
	zco_destroy(sdr, bundle->payload.content);
	bundle->payload.content = newPayload;
	bundle->acct = ZcoOutbound;
	if (patchExtensionBlocks(bundle) < 0)
	{
		putErrmsg("Can't insert missing extensions.", NULL);
		return -1;
	}

	sdr_write(sdr, bundleAddr, (char *) bundle, sizeof(Bundle));
	noteBundleInserted(bundle);		/*	Into Outbound.	*/

	/*	Bundle is now ready to be forwarded.			*/

	if ((dictionary = retrieveDictionary(bundle)) == (char *) bundle)
	{
		putErrmsg("Can't retrieve dictionary.", NULL);
		return -1;
	}

	if (printEid(&(bundle->destination), dictionary, &eidString) < 0)
	{
		putErrmsg("Can't print destination EID.", NULL);
		return -1;
	}

	result = forwardBundle(bundleAddr, bundle, eidString);
	MRELEASE(eidString);
	releaseDictionary(dictionary);
	if (result < 0)
	{
		putErrmsg("Can't enqueue bundle for forwarding.", NULL);
		return -1;
	}

	return 0;
}

/*	*	Functions for provisional transit migration thread	*/

typedef struct
{
	int	running;
} MigrationThreadParms;

static void	*migrateBundles(void *parm)
{
	/*	Main loop for initiating the forwarding of received
	 *	bundles whose payloads are provisional ZCOs.  These
	 *	ZCOs occupy non-Restricted Inbound ZCO space, which
	 *	is a critical resource.  Whenever a provisional ZCO
	 *	cannot be immediately migrated to the Outbound ZCO
	 *	pool, we destroy it (abandoning the bundle) instead
	 *	of waiting for Outbound space to become available.
	 *	This ensures that the Inbound space occupied by the
	 *	ZCO is released, one way or another.			*/

	Sdr			sdr = getIonsdr();
	BpDB			*db = getBpConstants();
	BpVdb			*vdb = getBpVdb();
	MigrationThreadParms	*mtp = (MigrationThreadParms *) parm;
	Object			elt;
	Object			bundleAddr;
	Bundle			bundle;
	int			priority;
	Object			newPayload;

	snooze(1);	/*	Let main thread become interruptible.	*/
	while (mtp->running)
	{
		CHKNULL(sdr_begin_xn(sdr));
		elt = sdr_list_first(sdr, db->provisionalTransit);
		if (elt == 0)	/*	Wait for in-transit notice.	*/
		{
			sdr_exit_xn(sdr);
			if (sm_SemTake(vdb->provisionalTransitSemaphore) < 0)
			{
				putErrmsg("Can't take semaphore.", NULL);
				mtp->running = 0;
				shutDown();
				continue;
			}

			if (sm_SemEnded(vdb->provisionalTransitSemaphore))
			{
				/*	Normal shutdown.		*/

				mtp->running = 0;
			}

			continue;
		}

		/*	Note: still in transaction at this point.	*/

		bundleAddr = (Object) sdr_list_data(sdr, elt);
		sdr_stage(sdr, (char *) &bundle, bundleAddr, sizeof(Bundle));
		priority = COS_FLAGS(bundle.bundleProcFlags) & 0x03;

		/*	Note: it's safe to call ionCreateZco here
		 *	(while we're in a transaction) because we
		 *	don't pass it a pointer to an attendant, so
		 *	it won't block.					*/

		newPayload = ionCreateZco(ZcoZcoSource, bundle.payload.content,
				0, bundle.payload.length, priority,
				bundle.extendedCOS.ordinal, ZcoOutbound, NULL);
		switch (newPayload)
		{
		case (Object) ERROR:
			sdr_cancel_xn(sdr);
			putErrmsg("Can't create payload ZCO.", NULL);
			mtp->running = 0;
			shutDown();
			continue;

		case 0:		/*	Not enough ZCO space.		*/

			/*	Cannot immediately queue this bundle
			 *	for forwarding, so must abandon it to
			 *	free up Inbound ZCO space.		*/

			sdr_list_delete(sdr, elt, NULL, NULL);
			bundle.transitElt = 0;
			sdr_write(sdr, bundleAddr, (char *) &bundle,
					sizeof(Bundle));
			if (bpAbandon(bundleAddr, &bundle, BP_REASON_DEPLETION)
					< 0)
			{
				sdr_cancel_xn(sdr);
				putErrmsg("bpAbandon failed.", NULL);
				mtp->running = 0;
				shutDown();
				continue;
			}

			if (sdr_end_xn(sdr) < 0)
			{
				putErrmsg("Failed migrating bundle.", NULL);
				mtp->running = 0;
				shutDown();
			}

			continue;

		default:
			break;	/*	Out of switch.			*/
		}

		/*	New payload ZCO has been created; still in xn.	*/

		sdr_list_delete(sdr, elt, NULL, NULL);
		bundle.transitElt = 0;
		if (initiateBundleForwarding(sdr, &bundle, bundleAddr,
				newPayload) < 0)
		{
			sdr_cancel_xn(sdr);
			putErrmsg("Can't initiate forwarding of bundle.", NULL);
			mtp->running = 0;
			shutDown();
			continue;
		}

		if (sdr_end_xn(sdr) < 0)
		{
			putErrmsg("Failed migrating in-transit bundle.", NULL);
			mtp->running = 0;
			shutDown();
		}
	}

	return NULL;
}

/*	*	*	Main thread functions	*	*	*	*/

#if defined (ION_LWT)
int	bptransit(int a1, int a2, int a3, int a4, int a5,
		int a6, int a7, int a8, int a9, int a10)
{
#else
int	main(int argc, char *argv[])
{
#endif
	Sdr			sdr;
	BpDB			*db;
	BpVdb			*vdb;
	ReqAttendant		attendant;
	MigrationThreadParms	mtp;
	pthread_t		migrationThread;
	Object			elt;
	Object			bundleAddr;
	Bundle			bundle;
	int			priority;
	vast			fileSpaceNeeded;
	vast			heapSpaceNeeded;
	Object			currentElt;
	vast			currentFileSpaceNeeded;
	vast			currentHeapSpaceNeeded;
	ReqTicket		ticket;
	vast			length;
	Object			newPayload;

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
		return -1;
	}

	oK(_attendant(&attendant));
	isignal(SIGTERM, shutDown);

	/*	Start the provisional transit forwarding thread.	*/

	mtp.running = 1;
	if (pthread_begin(&migrationThread, NULL, migrateBundles, &mtp))
	{
		putErrmsg("bptransit can't create migration thread", NULL);
		ionStopAttendant(&attendant);
		return -1;
	}

	/*	Main loop handles bundles in confirmed transit: get
	 *	bundle, copy it into Outbound ZCO space as soon as
	 *	space is available, delete it from Inbound ZCO space.	*/

	writeMemo("[i] bptransit is running.");
	while (mtp.running)
	{
		CHKERR(sdr_begin_xn(sdr));	/*	Lock database.	*/
		elt = sdr_list_first(sdr, db->confirmedTransit);
		if (elt == 0)	/*	Wait for in-transit notice.	*/
		{
			sdr_exit_xn(sdr);	/*	Unlock.		*/
			if (sm_SemTake(vdb->confirmedTransitSemaphore) < 0)
			{
				putErrmsg("Can't take transit semaphore.",
						NULL);
				mtp.running = 0;
				continue;
			}

			if (sm_SemEnded(vdb->confirmedTransitSemaphore))
			{
				mtp.running = 0;
			}

			continue;
		}

		bundleAddr = (Object) sdr_list_data(sdr, elt);
		sdr_read(sdr, (char *) &bundle, bundleAddr, sizeof(Bundle));
		priority = COS_FLAGS(bundle.bundleProcFlags) & 0x03;
		zco_get_aggregate_length(sdr, bundle.payload.content, 0,
				bundle.payload.length, &fileSpaceNeeded,
				&heapSpaceNeeded);
		sdr_exit_xn(sdr);		/*	Unlock.		*/

		/*	Remember this candidate bundle.			*/

		currentElt = elt;
		currentFileSpaceNeeded = fileSpaceNeeded;
		currentHeapSpaceNeeded = heapSpaceNeeded;

		/*	Reserve space for new ZCO.			*/

		if (ionRequestZcoSpace(ZcoOutbound, fileSpaceNeeded,
				heapSpaceNeeded, priority,
				bundle.extendedCOS.ordinal,
				&attendant, &ticket) < 0)
		{
			putErrmsg("Failed trying to reserve ZCO space.", NULL);
			mtp.running = 0;
			continue;
		}

		if (ticket)	/*	Space not currently available.	*/
		{
			/*	Ticket is req list element for the
			 *	request.  Wait until space available.	*/

			if (sm_SemTake(attendant.semaphore) < 0)
			{
				putErrmsg("Failed taking semaphore.", NULL);
				ionShred(ticket);
				mtp.running = 0;
				continue;
			}

			if (sm_SemEnded(attendant.semaphore))
			{
				writeMemo("[i] ZCO request interrupted.");
				ionShred(ticket);
				continue;
			}

			/*	ZCO space has now been reserved.	*/

			ionShred(ticket);
		}

		/*	At this point ZCO space is known to be avbl.	*/

		CHKERR(sdr_begin_xn(sdr));
		elt = sdr_list_first(sdr, db->confirmedTransit);
		if (elt != currentElt)
		{
			/*	Something happened to this bundle
			 *	while we were waiting for space to
			 *	become available.  Forget about it
			 *	and start over again.			*/

			sdr_exit_xn(sdr);
			continue;
		}

		sdr_stage(sdr, (char *) &bundle, bundleAddr, sizeof(Bundle));
		zco_get_aggregate_length(sdr, bundle.payload.content, 0,
				bundle.payload.length, &fileSpaceNeeded,
				&heapSpaceNeeded);
		if (fileSpaceNeeded != currentFileSpaceNeeded
		|| heapSpaceNeeded != currentHeapSpaceNeeded)
		{
			/*	Very unlikely, but it's possible that
			 *	the bundle we're expecting expired
			 *	and a different bundle got appended
			 *	to the list in a list element that
			 *	is at the same address as the one
			 *	we got last time (deleted and then
			 *	recycled).  If the sizes don't match,
			 *	start over again.			*/

			sdr_exit_xn(sdr);
			continue;
		}

		/*	Pass additive inverse of length to zco_create
		 *	to note that space has already been awarded.	*/

		length = bundle.payload.length;
		newPayload = zco_create(sdr, ZcoZcoSource,
				bundle.payload.content, 0, 0 - length,
				ZcoOutbound, 0);
		switch (newPayload)
		{
		case (Object) ERROR:
		case 0:
			sdr_cancel_xn(sdr);
			putErrmsg("Can't create payload ZCO.", NULL);
			mtp.running = 0;
			continue;

		default:
			break;			/*	Out of switch.	*/
		}

		sdr_list_delete(sdr, elt, NULL, NULL);
		bundle.transitElt = 0;
		if (initiateBundleForwarding(sdr, &bundle, bundleAddr,
				newPayload) < 0)
		{
			sdr_cancel_xn(sdr);
			putErrmsg("Can't initiate forwarding of bundle.", NULL);
			mtp.running = 0;
			continue;
		}

		if (sdr_end_xn(sdr) < 0)
		{
			putErrmsg("Failed migrating bundle.", NULL);
			mtp.running = 0;
		}
	}

	shutDown();
	pthread_join(migrationThread, NULL);
	ionStopAttendant(&attendant);
	writeErrmsgMemos();
	writeMemo("[i] bptransit has ended.");
	ionDetach();
	return 0;
}
