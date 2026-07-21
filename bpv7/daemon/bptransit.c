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

typedef struct
{
	int	running;
} MigrationThreadParms;

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

static void	shutDown(int signum)
{
	/* Tell the compiler that we are not using 'signum' */
	(void)signum;

	sm_SemEnd((getBpVdb())->transitSemaphore);
	ionPauseAttendant(_attendant(NULL));
}

/*	*	Common functions for initiating bundle forwarding	*/

static int	initiateBundleForwarding(Sdr sdr, Bundle *bundle,
			SdrObject bundleAddr, SdrObject newPayload)
{
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

	readEid(&(bundle->destination), &eidString);
	if (eidString == NULL)
	{
		putErrmsg("Can't print destination EID.", NULL);
		return -1;
	}

	result = forwardBundle(bundleAddr, bundle, eidString);
	MRELEASE(eidString);
	if (result < 0)
	{
		putErrmsg("Can't enqueue bundle for forwarding.", NULL);
		return -1;
	}

	return 0;
}

/*	*	*	Main thread functions	*	*	*	*/

#if defined (ION_LWT)
int	bptransit(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
#else
int	main(void)
{
#endif
	Sdr			sdr;
	BpDB			*db;
	BpVdb			*vdb;
	ReqAttendant		attendant;
	MigrationThreadParms	mtp;
	SdrObject		elt;
	SdrObject		bundleAddr;
	Bundle			bundle;
	int			priority;
	vast			fileSpaceNeeded;
	vast			bulkSpaceNeeded;
	vast			heapSpaceNeeded;
	SdrObject		currentElt;
	vast			currentFileSpaceNeeded;
	vast			currentHeapSpaceNeeded;
	ReqTicket		ticket;
	vast			length;
	SdrObject		newPayload;

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

	mtp.running = 1;
	oK(_attendant(&attendant));
	isignal(SIGTERM, shutDown);

	/*	Main loop handles bundles in transit: get bundle,
	 *	copy it into Outbound ZCO space as soon as space
	 *	is available, delete it from Inbound ZCO space.		*/

	writeMemo("[i] bptransit is running.");
	while (mtp.running)
	{
		CHKERR(sdr_begin_xn(sdr));	/*	Lock database.	*/
		elt = sdr_list_first(sdr, db->transit);
		if (elt == 0)	/*	Wait for in-transit notice.	*/
		{
			sdr_exit_xn(sdr);	/*	Unlock.		*/
			if (sm_SemTake(vdb->transitSemaphore) < 0)
			{
				putErrmsg("Can't take transit semaphore.",
						NULL);
				mtp.running = 0;
				continue;
			}

			if (sm_SemEnded(vdb->transitSemaphore))
			{
				mtp.running = 0;
			}

			continue;
		}

		bundleAddr = (SdrObject) sdr_list_data(sdr, elt);
		sdr_read(sdr, (char *) &bundle, bundleAddr, sizeof(Bundle));
		priority = bundle.priority;

		/*	Validate payload before calling zco_get_aggregate_length
		 *	to prevent assertion failure inside the function if
		 *	the ZCO reference is invalid or corrupted.		*/

		if (bundle.payload.content <= 0 || bundle.payload.length < 0)
		{
			putErrmsg("Bundle has invalid payload; discarding.",
					utoa(bundle.payload.length));
			sdr_list_delete(sdr, elt, NULL, NULL);
			bundle.transitElt = 0;
			sdr_write(sdr, bundleAddr, (char *) &bundle,
					sizeof(Bundle));
			/*	This error path modified the SDR (removed the
			 *	bundle from transit), so the transaction must be
			 *	committed.  sdr_exit_xn here would cancel it,
			 *	which corrupts the lock on a non-reversible SDR
			 *	(configFlags=1) and cascades the daemon line.	*/
			if (sdr_end_xn(sdr) < 0)
			{
				putErrmsg("Can't commit discard of bad transit \
bundle.", NULL);
				mtp.running = 0;
			}

			continue;
		}

		zco_get_aggregate_length(sdr, bundle.payload.content, 0,
				bundle.payload.length, &fileSpaceNeeded,
				&bulkSpaceNeeded, &heapSpaceNeeded);
		if (fileSpaceNeeded < 0 || bulkSpaceNeeded < 0
		|| heapSpaceNeeded < 0)
		{
			/*	Payload ZCO source length is shorter than
			 *	bundle.payload.length — the bundle is
			 *	malformed.  Remove it from transit so we
			 *	don't spin; TTL expiry will destroy it.	*/

			putErrmsg("Bundle payload ZCO length mismatch; discarding.",
					utoa(bundle.payload.length));
			sdr_list_delete(sdr, elt, NULL, NULL);
			bundle.transitElt = 0;
			sdr_write(sdr, bundleAddr, (char *) &bundle,
					sizeof(Bundle));
			/*	Commit the removal; sdr_exit_xn would cancel a
			 *	modified transaction and corrupt the lock on a
			 *	non-reversible SDR (#1051/#1052 cascade).	*/
			if (sdr_end_xn(sdr) < 0)
			{
				putErrmsg("Can't commit discard of bad transit \
bundle.", NULL);
				mtp.running = 0;
			}

			continue;
		}

		sdr_exit_xn(sdr);		/*	Unlock.		*/

		/*	Remember this candidate bundle.			*/

		currentElt = elt;
		currentFileSpaceNeeded = fileSpaceNeeded;
		currentHeapSpaceNeeded = heapSpaceNeeded;

		/*	Reserve space for new ZCO.			*/

		if (ionRequestZcoSpace(ZcoOutbound, fileSpaceNeeded,
				bulkSpaceNeeded, heapSpaceNeeded, priority,
				bundle.ordinal, &attendant, &ticket) < 0)
		{
			putErrmsg("Failed trying to reserve ZCO space.", NULL);
			mtp.running = 0;
			continue;
		}

		if (!(ionSpaceAwarded(ticket)))
		{
			/*	Space not currently available.		*/

			if (sm_SemTake(attendant.semaphore) < 0)
			{
				putErrmsg("Failed taking semaphore.", NULL);
				ionShred(ticket);	/*	Cancel.	*/
				mtp.running = 0;
				continue;
			}

			if (sm_SemEnded(attendant.semaphore))
			{
				writeMemo("[i] ZCO request interrupted.");
				ionShred(ticket);	/*	Cancel.	*/
				mtp.running = 0;
				continue;
			}

			/*	ZCO space has now been reserved.	*/
		}

		/*	At this point ZCO space is known to be avbl.	*/

		CHKERR(sdr_begin_xn(sdr));
		elt = sdr_list_first(sdr, db->transit);
		if (elt != currentElt)
		{
			/*	Something happened to this bundle
			 *	while we were waiting for space to
			 *	become available.  Forget about it
			 *	and start over again.			*/

			sdr_exit_xn(sdr);
			ionShred(ticket);		/*	Cancel.	*/
			continue;
		}

		bundleAddr = sdr_list_data(sdr, elt);
		sdr_stage(sdr, (char *) &bundle, bundleAddr, sizeof(Bundle));

		/*	Guard against a race: another daemon may have
		 *	delivered this bundle locally (setting dlvQueueElt)
		 *	while bptransit was blocked waiting for ZCO space.
		 *	forwardBundle() will abort if dlvQueueElt != 0.	*/

		if (bundle.dlvQueueElt != 0)
		{
			putErrmsg("Transit bundle already delivered locally; skipping.",
					NULL);
			sdr_exit_xn(sdr);
			ionShred(ticket);		/*	Cancel.	*/
			continue;
		}

		/*	Re-validate payload after waiting for ZCO space;
		 *	the bundle may have been corrupted or its ZCO
		 *	destroyed by another process in the meantime.	*/

		if (bundle.payload.content <= 0 || bundle.payload.length < 0)
		{
			putErrmsg("Bundle payload became invalid; discarding.",
					utoa(bundle.payload.length));
			sdr_list_delete(sdr, elt, NULL, NULL);
			bundle.transitElt = 0;
			sdr_write(sdr, bundleAddr, (char *) &bundle,
					sizeof(Bundle));
			/*	Commit the removal; sdr_exit_xn would cancel a
			 *	modified transaction and corrupt the lock on a
			 *	non-reversible SDR (#1051/#1052 cascade).	*/
			if (sdr_end_xn(sdr) < 0)
			{
				putErrmsg("Can't commit discard of bad transit \
bundle.", NULL);
				mtp.running = 0;
			}

			ionShred(ticket);		/*	Cancel.	*/
			continue;
		}

		zco_get_aggregate_length(sdr, bundle.payload.content, 0,
				bundle.payload.length, &fileSpaceNeeded,
				&bulkSpaceNeeded, &heapSpaceNeeded);
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
			ionShred(ticket);		/*	Cancel.	*/
			continue;
		}

		/*	Pass additive inverse of length to zco_create
		 *	to note that space has already been awarded.	*/

		length = bundle.payload.length;
		newPayload = zco_create(sdr, ZcoZcoSource,
				bundle.payload.content, 0, 0 - length,
				ZcoOutbound);
		switch (newPayload)
		{
		case (SdrObject) ERROR:
		case 0:
			sdr_cancel_xn(sdr);
			putErrmsg("Can't create payload ZCO.", NULL);
			ionShred(ticket);		/*	Cancel.	*/
			mtp.running = 0;
			continue;

		default:
			break;		/*	Out of switch.		*/
		}

		ionShred(ticket);	/*	Dismiss reservation.	*/
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

		/*	Make sure other tasks have a chance to run.	*/

		sm_TaskYield();
	}

	shutDown(SIGTERM);
	ionStopAttendant(&attendant);
	writeErrmsgMemos();
	writeMemo("[i] bptransit has ended.");
	ionDetach();
	return 0;
}
