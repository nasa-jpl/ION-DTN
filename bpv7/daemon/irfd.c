/*
	irfd.c:	inter-regional forwarding (IRF) daemon for ION.

	Author: Scott Burleigh, IPNGROUP

	Copyright (c) 2021, IPNGROUP.  ALL RIGHTS RESERVED.  
									*/
#include "ipnfw.h"
#include "bei.h"
#include "irf.h"

static uaddr	_running(uaddr *newValue)
{
	void	*value;
	uaddr	state;

	if (newValue)			/*	Changing state.		*/
	{
		value = (void *) (*newValue);
		state = (uaddr) sm_TaskVar(&value);
	}
	else				/*	Just check.		*/
	{
		state = (uaddr) sm_TaskVar(NULL);
	}

	return state;
}

static void	shutDown(int signum)
{
	uaddr	stop = 0;

	/* Tell the compiler that we are not using 'signum' */
	(void)signum;

	isignal(SIGTERM, SIG_IGN);
	oK(_running(&stop));	/*	Terminates irfd.		*/
}

static IrfCandidate	*findCandidate(IonNode *node, uvast candidateNodeNbr,
				PsmAddress *candidateElt)
{
	PsmPartition	ionwm = getIonwm();
	Sdr		sdr;
	int		result;
	PsmAddress	elt;
	IrfCandidate	*candidate;

	if (node->viaPassageways == 0)
	{
		sdr = getIonsdr();
		CHKNULL(sdr_begin_xn(sdr));
		result = irf_initialize(node);
		sdr_exit_xn(sdr);
		if (result < 0)
		{
			return NULL;
		}
	}

	for (elt = sm_list_first(ionwm, node->viaPassageways); elt;
			elt = sm_list_next(ionwm, elt))
	{
		candidate = (IrfCandidate *) psp(ionwm,
				sm_list_data(ionwm, elt));
		if (candidate->nodeNbr < candidateNodeNbr)
		{
			continue;
		}

		break;
	}

	*candidateElt = elt;
	if (elt && candidate->nodeNbr == candidateNodeNbr)
	{
		return candidate;
	}

	return NULL;
}

/*	*	Notifications handler thread functions	*	*	*/

static int	handleIrfMessage(IonDB *iondb, BpDelivery *dlv)
{
	Sdr		sdr = getIonsdr();
	IonVdb		*ionvdb = getIonVdb();
	unsigned int	buflen;
	unsigned char	*buffer;
	ZcoReader	reader;
	vast		bytesToParse;
	unsigned char	*cursor;
	unsigned int	unparsedBytes;
	uvast		arrayLength;
	uvast		uvtemp;
	uvast		fromNodeNbr;
	uvast		toNodeNbr;
	int		isReachable;
	uvast		passagewaysCount;
	Lyst		passageways;
	IonNode		*terminusNode;
	PsmAddress	elt;
	IrfCandidate	*candidate;
	PsmAddress	nodeElt;
	MetaEid		metaEid;
	VScheme		*vscheme;
	PsmAddress	vschemeElt;
	uvast		candidateNodeNbr;
	LystElt		pwyElt;
	int		result;

	/*	The iondb parameter is currently unused.			*/

	(void)iondb;

	CHKERR(sdr_begin_xn(sdr));
	buflen = zco_source_data_length(sdr, dlv->adu);
	buffer = MTAKE(buflen);
	if (buffer == NULL)
	{
		oK(sdr_end_xn(sdr));
		writeMemoNote("[?] IRF message is too large.", itoa(buflen));
		return 0;
	}

	zco_start_receiving(dlv->adu, &reader);
	bytesToParse = zco_receive_source(sdr, &reader, buflen,
			(char *) buffer);
	oK(sdr_end_xn(sdr));
	if (bytesToParse < 0)
	{
		putErrmsg("Can't receive IRF message.", NULL);
		return -1;
	}

	/*	Parse IRF message.					*/

	cursor = buffer;
	unparsedBytes = bytesToParse;

	/*	Get array length of message.				*/

	arrayLength = 0;
	if (cbor_decode_array_open(&arrayLength, &cursor, &unparsedBytes) < 1)
	{
		writeMemo("[?] Can't decode IRF message array.");
		return 0;
	}

	if (arrayLength != 4)
	{
		writeMemoNote("[?] Bad IRF message array length",
				itoa(arrayLength));
		return 0;
	}

	/*	Get original bundle source node number.			*/

	if (cbor_decode_integer(&uvtemp, CborAny, &cursor,
			&unparsedBytes) < 1)
	{
		writeMemo("[?] Can't decode IRF message 'from' node nbr.");
		return 0;
	}

	fromNodeNbr = uvtemp;

	/*	Get original bundle destination (subject) node number.	*/

	if (cbor_decode_integer(&uvtemp, CborAny, &cursor,
			&unparsedBytes) < 1)
	{
		writeMemo("[?] Can't decode IRF message 'to' node nbr.");
		return 0;
	}

	toNodeNbr = uvtemp;

	/*	Get reachability flag.					*/

	if (cbor_decode_integer(&uvtemp, CborAny, &cursor, &unparsedBytes) < 1)
	{
		writeMemo("[?] Can't decode IRF message isReachable flag.");
		return 0;
	}

	isReachable = uvtemp;

	/*	Get length of passageways array.			*/

	arrayLength = 0;
	if (cbor_decode_array_open(&arrayLength, &cursor, &unparsedBytes) < 1)
	{
		writeMemo("[?] Can't decode IRF message array.");
		return 0;
	}

	passagewaysCount = arrayLength;

	/*	Now get list of passageways to inform.			*/

	passageways = lyst_create_using(getIonMemoryMgr());
	if (passageways == NULL)
	{
		putErrmsg("Can't create lyst of passageway nodes.", NULL);
		return -1;
	}

	while (passagewaysCount > 0)
	{
		if (cbor_decode_integer(&uvtemp, CborAny, &cursor,
				&unparsedBytes) < 1)
		{
			writeMemo("[?] Can't decode IRF message passageway.");
			lyst_destroy(passageways);
			return 0;
		}

		if (lyst_insert_last(passageways, (void *) (uintptr_t) uvtemp)
				== NULL)
		{
			putErrmsg("Can't insert passageway into lyst.", NULL);
			lyst_destroy(passageways);
			return -1;
		}

		passagewaysCount--;
	}

	/*	IRF message has been fully parsed.  Now act on it.	*/

	terminusNode = findNode(ionvdb, toNodeNbr, &nodeElt);
	if (terminusNode == NULL)
	{
		writeMemoNote("[?] Can't apply IRF message.", itoa(toNodeNbr));
		lyst_destroy(passageways);
		return 0;
	}

	if (parseEidString(dlv->bundleSourceEid, &metaEid, &vscheme,
			&vschemeElt) == 0
	|| metaEid.elementNbr == 0)
	{
		writeMemoNote("[?] Can't handle message source EID in IRF.",
				dlv->bundleSourceEid);
		lyst_destroy(passageways);
		return 0;
	}

	candidateNodeNbr = metaEid.elementNbr;
	clearMetaEid(&metaEid);
	candidate = findCandidate(terminusNode, candidateNodeNbr, &elt);
	if (candidate)
	{
		if (isReachable)
		{
			candidate->confirmTime = getCtime();
		}
		else
		{
			candidate->confirmTime = MAX_POSIX_TIME;
		}
	}

	/*	Now propagate the message as necessary: if self is
	 *	a passageway, remove self from the passageway list
	 *	and propagate the message to (a) the next passageway
	 *	in the list, if any, or (b) to the original source
	 *	node of the bundle, otherwise.				*/

	pwyElt = lyst_last(passageways);
	if (pwyElt)
	{
		lyst_delete(pwyElt);
		result = irf_send_msg(fromNodeNbr, toNodeNbr, isReachable,
				passageways);
		if (result < 0)
		{
			putErrmsg("Can't propagate IRF message.", NULL);
		}
	}
	else
	{
		result = 0;
	}

	lyst_destroy(passageways);
	return result;
}

static void	*handleIrfMessages(void *parm)
{
	uaddr		stop = 0;
	Object		iondbObj;
	IonDB		iondb;
	BpSAP		sap;
	BpDelivery	dlv;

	sap = (BpSAP) parm;
	iondbObj = getIonDbObject();
	sdr_read(getIonsdr(), (char *) &iondb, iondbObj, sizeof(IonDB));
	while (_running(NULL) && !(sm_SemEnded(sap->recvSemaphore)))
	{
		if (bp_receive(sap, &dlv, BP_BLOCKING) < 0)
		{
			putErrmsg("IRF message reception failed.", NULL);
			_running(&stop);
			continue;
		}

		switch (dlv.result)
		{
		case BpPayloadPresent:
			break;

		case BpEndpointStopped:
			_running(&stop);
			bp_release_delivery(&dlv, 1);
			continue;

		default:
			bp_release_delivery(&dlv, 1);
			continue;
		}

		/*	Process the notification.			*/

		if (handleIrfMessage(&iondb, &dlv) < 0)
		{
			_running(&stop);
		}

		bp_release_delivery(&dlv, 1);

		/*	Make sure other tasks have a chance to run.	*/

		sm_TaskYield();
	}

	writeMemo("[i] irfd message handler thread ended.");
	writeErrmsgMemos();
	return NULL;
}

/*	*	*	Main thread functions.	*	*	*	*/

static int	noticeIsValid(PwcNotice *notice)
{
	Object		memberObj = 0;
	RegionMember	member;
	Object		elt;

	memberObj = findLocalNode(notice->nodeNbr, &member, &elt);
	if (memberObj == 0)	/*	Node isn't registered.		*/
	{
		return 0;
	}

	return 1;
}

static void	addCandidate(IonNode *node, uvast candidateNodeNbr)
{
	PsmAddress	elt;
	IrfCandidate	*candidate;

	candidate = findCandidate(node, candidateNodeNbr, &elt);
	if (candidate)
	{
		/*	Just mark this candidate as "potential" again.	*/

		candidate->confirmTime = 0;
	}
	else	/*	Insert additional candidate here.		*/
	{
		if (irf_add_candidate(candidateNodeNbr, node, elt) < 0)
		{
			return;
		}
	}
}

static void	removeCandidate(IonNode *node, uvast candidateNodeNbr)
{
	PsmPartition	ionwm = getIonwm();
	PsmAddress	elt;
	PsmAddress	addr;
	IrfCandidate	*candidate;

	candidate = findCandidate(node, candidateNodeNbr, &elt);
	if (candidate)
	{
		addr = sm_list_data(ionwm, elt);
		psm_free(ionwm, addr);
		sm_list_delete(ionwm, elt, NULL, NULL);
	}
}

static void	resetCandidate(IonNode *node, uvast candidateNodeNbr)
{
	PsmAddress	elt;
	IrfCandidate	*candidate;

	candidate = findCandidate(node, candidateNodeNbr, &elt);
	if (candidate)
	{
		candidate->confirmTime = 0;
	}
}

#if defined (ION_LWT)
int	irfd(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
#else
int	main(void)
{
#endif
	Sdr		sdr;
	PsmPartition	ionwm;
	IonVdb		*vdb;
	VScheme		*vscheme;
	PsmAddress	velt;
	MetaEid		meid;
	VEndpoint	*vpoint;
	Object		iondbObj;
	IonDB		iondb;
	uaddr		start = 1;
	char		notificationsEid[32];
	BpSAP		notificationsSap;
	pthread_t	notificationThread;
	Object		elt;
	Object		addr;
	PwcNotice	notice;
	PsmAddress	elt2;
	IonNode		*node;

	if (bpAttach() < 0)
	{
		putErrmsg("irfd can't attach to BP.", NULL);
		return 1;
	}

	sdr = getIonsdr();
	CHKERR(sdr);
	ionwm = getIonwm();
	vdb = getIonVdb();
	isprintf(notificationsEid, sizeof notificationsEid,
			"ipn:" UVAST_FIELDSPEC ".271", getOwnFqnn());
	oK(parseEidString(notificationsEid, &meid, &vscheme, &velt));
	findEndpoint(notificationsEid, &meid, vscheme, &vpoint, &velt);
	clearMetaEid(&meid);
	if (velt == 0)
	{
		writeMemoNote("[i] Not configured for IRF, irfd stopping",
				notificationsEid);
		return 1;
	}

	iondbObj = getIonDbObject();
	CHKERR(iondbObj);
	sdr_read(sdr, (char *) &iondb, iondbObj, sizeof(IonDB));
	isignal(SIGTERM, shutDown);
	oK(_running(&start));

	/*	Start the IRF notification handler thread.		*/

	if (bp_open(notificationsEid, &notificationsSap) < 0)
	{
		putErrmsg("Can't open irfd endpoint.", notificationsEid);
		return 1;
	}

	if (pthread_begin(&notificationThread, NULL, handleIrfMessages,
			notificationsSap, "irfd_notify_handler"))
	{
		putSysErrmsg("irfd can't create notification handler thread.",
				NULL);
		return 1;
	}

	/*	Main loop: snooze 1 second, then drain queue of pending
	 *	passageway change notices.				*/

	writeMemo("[i] irfd is running.");
	while (_running(NULL))
	{
		/*	Sleep for 1 second, then apply all pending
		 *	PWC notices to revise destination nodes'
		 *	viaPassageways lists.				*/

		snooze(1);
		if (!sdr_begin_xn(sdr))
		{
			putErrmsg("irfd failed to begin new transaction.",
					NULL);
			break;
		}

		while (1)
		{
			elt = sdr_list_first(sdr, iondb.pwcNotices);
			if (elt == 0)
			{
				break;	/*	No more to send.	*/
			}

			addr = sdr_list_data(sdr, elt);
			sdr_read(sdr, (char *) &notice, addr,
					sizeof(PwcNotice));
			sdr_free(sdr, addr);
			sdr_list_delete(sdr, elt, NULL, NULL);

			/*	Apply the change to all lists of
			 *	viaPassageways.				*/

			if (!noticeIsValid(&notice))
			{
				continue;
			}

			for (elt2 = sm_rbt_first(ionwm, vdb->nodes); elt2;
					elt2 = sm_rbt_next(ionwm, elt2))
			{
				addr = sm_rbt_data(ionwm, elt2);
				node = (IonNode *) psp(ionwm, addr);
				if (node->viaPassageways == 0)
				{
					continue;
				}

				switch(notice.state)
				{
				case NotPassageway:
					removeCandidate(node, notice.nodeNbr);
					break;

				case Passageway:
					addCandidate(node, notice.nodeNbr);
					break;

				default:
					resetCandidate(node, notice.nodeNbr);
				}
			}
		}

		if (sdr_end_xn(sdr) < 0)
		{
			putErrmsg("Can't send contact plan updates.", NULL);
			break;
		}
	}

	/*	Terminate threads.				*/

	sm_SemEnd(notificationsSap->recvSemaphore);
	pthread_join(notificationThread, NULL);
	bp_close(notificationsSap);

	/*	Wrap up.						*/

	writeErrmsgMemos();
	writeMemo("[i] irfd has ended.");
	ionDetach();
	return 0;
}
