/*
	libams.c:	functions enabling the implementation of
			AMS-based applications.

	Author: Scott Burleigh, JPL

	Copyright (c) 2005, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
									*/
#include "amsP.h"

char		*ModuleCrashedMemo = "Module crashed; must unregister.";

static int	ams_invite2(AmsSAP *sap, int roleNbr, int continuumNbr,
			int unitNbr, int subjectNbr, int priority,
			unsigned char flowLabel, AmsSequence sequence,
			AmsDiligence diligence);

#define	MAX_AMS_CONTENT	(65000)

/*			Privately defined event types.			*/
#define ACCEPTED_EVT	32
#define SHUTDOWN_EVT	33
#define ENDED_EVT	34
#define BREAK_EVT	35

/*	*	*	AMS API support functions	*	*	*/

static LystElt	insertAmsEvent(AmsSAP *sap, AmsEvt *evt, int priority)
{
	Llcv	eventsQueue = sap->amsEventsCV;
	int	i;
	LystElt	lastElt;
	LystElt	elt;

	/*	Enqueue the new event immediately after the last
	 *	currently enqueued event whose priority is equal
	 *	to or greater than that of the new event.		*/
	for (i = priority; i >= 0; i--)
	{
		lastElt = sap->lastForPriority[i];
		if (lastElt)
		{
			break;
		}
	}
	
	if (i < 0)	/*	No event to enqueue after.		*/
	{
		elt = lyst_insert_first(eventsQueue->list, evt);
	}
	else
	{
		elt = lyst_insert_after(lastElt, evt);
	}

	if (elt)
	{
		sap->lastForPriority[priority] = elt;
	}

	return elt;
}

static int	enqueueAmsEvent(AmsSAP *sap, AmsEvt *evt, char *ancillaryBlock,
		int responseNbr, int priority, AmsMsgType msgType)
{
	Llcv	eventsQueue = sap->amsEventsCV;
	long	queryNbr;
	LystElt	elt;

	if (eventsQueue == NULL)
	{
		return 0;	/*	SAP not initialized yet.	*/
	}

	llcv_lock(eventsQueue);

	/*	Events that shut down event loops are inserted at
	 *	the start of the events queue, for immediate handling.
	 *	Priority zero is reserved for these types of events:
	 *	CRASH_EVT, BREAK_EVT, and (optionally) some events
	 *	defined by the application (user events).		*/

	if (priority == 0)
	{
		elt = lyst_insert_first(eventsQueue->list, evt);
		if (elt && sap->lastForPriority[0] == NULL)
		{
			sap->lastForPriority[0] = elt;
		}

		llcv_unlock(eventsQueue);
		if (elt == NULL)
		{
			MRELEASE(evt);
			putErrmsg(NoMemoryMemo, NULL);
			return -1;
		}

		llcv_signal(eventsQueue, time_to_stop);
		return 0;
	}

	/*	A hack, which works in the absence of a Lyst user
	 *	data variable.  In order to make the current query
	 *	ID number accessible to an llcv condition function,
	 *	we stuff it into the "compare" function pointer in
	 *	the Lyst structure.					*/

	queryNbr = (long) lyst_compare_get(eventsQueue->list);
	if (queryNbr != 0)	/*	Need response to specfic msg.	*/
	{
		if (msgType == AmsMsgReply
		&& responseNbr == queryNbr)	/*	This is it.	*/
		{
			elt = lyst_insert_first(eventsQueue->list, evt);
			if (elt)	/*	Must erase query nbr.	*/
			{
				lyst_compare_set(eventsQueue->list, NULL);
				llcv_signal_while_locked(eventsQueue,
						llcv_reply_received);
			}
		}
		else	/*	This isn't it; deal with it later.	*/
		{
			elt = insertAmsEvent(sap, evt, priority);
		}
	}
	else	/*	Any event is worth waking up the thread for.	*/
	{
		elt = insertAmsEvent(sap, evt, priority);
		if (elt)
		{
			llcv_signal_while_locked(eventsQueue,
					llcv_lyst_not_empty);
		}
	}

	llcv_unlock(eventsQueue);
	if (elt == NULL)
	{
		MRELEASE(evt);
		if (ancillaryBlock)
		{
			MRELEASE(ancillaryBlock);
		}

		putErrmsg(NoMemoryMemo, NULL);
		return -1;
	}

	return 0;
}

static int	enqueueAmsCrash(AmsSAP *sap, char *text)
{
	int	textLength;
	char	*silence = "";
	AmsEvt	*evt;

	if (text == NULL)
	{
		textLength = 0;
		text = silence;
	}
	else
	{
		textLength = strlen(text);
	}

	evt = (AmsEvt *) MTAKE(1 + textLength + 1);
	if (evt == NULL)
	{
		putErrmsg(NoMemoryMemo, NULL);
		return -1;
	}

	evt->type = CRASH_EVT;
	memcpy(evt->value, text, textLength);
	evt->value[textLength] = '\0';
	if (enqueueAmsEvent(sap, evt, NULL, 0, 0, AmsMsgNone) < 0)
	{
		MRELEASE(evt);
		putErrmsg(NoMemoryMemo, NULL);
		return -1;
	}

	return 0;
}

static int	enqueueAmsStubEvent(AmsSAP *sap, int eventType, int priority)
{
	AmsEvt	*evt;

	evt = (AmsEvt *) MTAKE(sizeof(AmsEvt));
	if (evt == NULL)
	{
		putErrmsg(NoMemoryMemo, NULL);
		return -1;
	}

	evt->type = eventType;
	if (enqueueAmsEvent(sap, evt, NULL, 0, priority, AmsMsgNone) < 0)
	{
		MRELEASE(evt);
		putErrmsg(NoMemoryMemo, NULL);
		return -1;
	}

	return 0;
}

static void	noteEventDequeued(AmsSAP *sap, LystElt elt)
{
	int	i;

	for (i = 0; i < NBR_OF_PRIORITY_LEVELS; i++)
	{
		if (sap->lastForPriority[i] == elt)
		{
			sap->lastForPriority[i] = NULL;
			break;
		}
	}
}

static void	eraseSAP(AmsSAP *sap)
{
	/*	Note: MIB must *not* be locked at the time eraseSAP
	 *	is entered.  eraseSAP is always (and only) invoked
	 *	from the application's event handling thread (usually
	 *	either the main application thread or an AMS event
	 *	manager thread).					*/

	int		i;
	AmsInterface	*tsif;
//PUTS("In eraseSAP.");

	if (sap == NULL)
	{
		return;
	}

	sap->state = AmsSapClosed;

	/*	Stop heartbeat, MAMS handler, and MAMS rcvr threads.	*/

	if (sap->heartbeatThread)
	{
		pthread_cancel(sap->heartbeatThread);
		pthread_join(sap->heartbeatThread, NULL);
	}

//printf("Module '%d' heartbeat thread stopped.\n", sap->role->nbr);
	if (sap->mamsThread)
	{
		llcv_signal_while_locked(sap->mamsEventsCV, time_to_stop);
		pthread_join(sap->mamsThread, NULL);
	}

//printf("Module '%d' MAMS thread stopped.\n", sap->role->nbr);
	if (sap->mamsTsif.ts)
	{
		sap->mamsTsif.ts->shutdownFn(sap->mamsTsif.sap);
		pthread_join(sap->mamsTsif.receiver, NULL);
//printf("Module '%d' MAMS interface removed.\n", sap->role->nbr);
	}

	/*	Stop all AMS message interfaces.			*/

	for (i = 0; i < sap->transportServiceCount; i++)
	{
		tsif = &(sap->amsTsifs[i]);
		if (tsif->ts)
		{
			tsif->ts->shutdownFn(tsif->sap);
			pthread_join(tsif->receiver, NULL);
			if (tsif->ept)
			{
				MRELEASE(tsif->ept);
			}
//printf("Module '%d' AMS interface removed.\n", sap->role->nbr);
		}
	}

	/*	Clean up the rest of the SAP.				*/

	if (sap->amsEvents)
	{
		lyst_destroy(sap->amsEvents);
	}

	llcv_close(sap->amsEventsCV);
	if (sap->mamsEvents)
	{
		lyst_destroy(sap->mamsEvents);
	}

	llcv_close(sap->mamsEventsCV);
	if (sap->delivVectors)
	{
		lyst_destroy(sap->delivVectors);
	}

	if (sap->subscriptions)
	{
		lyst_destroy(sap->subscriptions);
	}

	if (sap->invitations)
	{
		lyst_destroy(sap->invitations);
	}

	MRELEASE(sap);
}

static void	destroyAmsEvent(LystElt elt, void *userdata)
{
	AmsEvt	*event = (AmsEvt *) lyst_data(elt);
	AmsMsg	*amsMsg;
	MamsMsg	*mamsMsg;

	if (event == NULL) return;
	if (event->type == AMS_MSG_EVT)
	{
		amsMsg = (AmsMsg *) (event->value);
		if (amsMsg->content)
		{
			MRELEASE(amsMsg->content);
		}
	}
	else if (event->type == MAMS_MSG_EVT)
	{
		mamsMsg = (MamsMsg *) (event->value);
		if (mamsMsg->supplement)
		{
			MRELEASE(mamsMsg->supplement);
		}
	}

	MRELEASE(event);
}

static void	destroyDeliveryVector(LystElt elt, void *userdata)
{
	DeliveryVector	*vector = (DeliveryVector *) lyst_data(elt);

	lyst_destroy(vector->interfaces);
	MRELEASE(vector);
}

static void	destroyMsgRule(LystElt elt, void *userdata)
{
	MsgRule	*rule = (MsgRule *) lyst_data(elt);

	MRELEASE(rule);
}

void	destroyXmitRule(LystElt elt, void *userdata)
{
	XmitRule	*rule = (XmitRule *) lyst_data(elt);

	MRELEASE(rule);
}

static int	getMsgSender(AmsSAP *sap, AmsMsg *msg, unsigned char *header,
			Module **sender)
{
	*sender = NULL;		/*	Default.			*/
	msg->continuumNbr = ((*(header + 2) & 0x7f) << 8) + *(header + 3);
	if (msg->continuumNbr < 1 || msg->continuumNbr > MaxContinNbr
	|| sap->venture->msgspaces[msg->continuumNbr] == NULL)
	{
		putErrmsg("Received message from unknown continuum.",
				itoa(msg->continuumNbr));
		return -1;
	}

	msg->unitNbr = (*(header + 4) << 8) + *(header + 5);
	if (msg->unitNbr < 0 || msg->unitNbr > MaxUnitNbr
	|| sap->venture->units[msg->unitNbr] == NULL)
	{
		putErrmsg("Received message from unknown cell.",
				itoa(msg->unitNbr));
		return -1;
	}

	msg->moduleNbr = (unsigned char) *(header + 6);
	if (msg->moduleNbr < 1 || msg->moduleNbr > MaxModuleNbr)
	{
		putErrmsg("Received message from invalid-numbered module.",
				itoa(msg->moduleNbr));
		return -1;
	}

	if (msg->continuumNbr != mib->localContinuumNbr)
	{
		return 0;	/*	Can't get ultimate sender.	*/
	}

	if (((*sender) =
	sap->venture->units[msg->unitNbr]->cell->modules[msg->moduleNbr])->role
			== NULL)
	{
		putErrmsg("Received message from unknown module.",
				itoa(msg->moduleNbr));
		return -1;
	}

	return 0;
}

static int	subunitOf(AmsSAP *sap, int argUnitNbr, int refUnitNbr)
{
	Unit	*argUnit;

	argUnit = sap->venture->units[argUnitNbr];
	while (argUnit)
	{
		if (argUnit->nbr == refUnitNbr)
		{
			return 1;
		}

		argUnit = argUnit->superunit;
	}

	return 0;
}

static LystElt	getMsgRule(AmsSAP *sap, Lyst rules, int subjectNbr, int roleNbr,
			int continuumNbr, int unitNbr)
{
	LystElt	elt;
	MsgRule	*rule;

	/*	This function finds the applicable MsgRule for specified
	 *	subject, role, continuum, unit, if any.  It is NOT used
	 *	for determining the insertion point of a new MsgRule.	*/

	for (elt = lyst_first(rules); elt; elt = lyst_next(elt))
	{
		rule = (MsgRule *) lyst_data(elt);

		/*	MsgRules are ordered by ascending subject
		 *	number.						*/

		if (rule->subject->nbr < subjectNbr)
		{
			continue;
		}

		if (rule->subject->nbr > subjectNbr)
		{
			break;		/*	Same as end of list.	*/
		}

		/*	Found a MsgRule for this subject.		*/

		if (rule->roleNbr != roleNbr && rule->roleNbr != 0)
		{
			continue;	/*	Keep looking.		*/
		}

		if (rule->continuumNbr != continuumNbr
		&& rule->continuumNbr != 0)
		{
			continue;	/*	Keep looking.		*/
		}

		if (subunitOf(sap, unitNbr, rule->unitNbr))
		{
			return elt;	/*	Found matching rule.	*/
		}
	}

	return NULL;
}

static int	validateAmsMsg(AmsSAP *sap, unsigned char *msgBuffer,
				int length, AmsMsg *msg, Module **sender)
{
	int		deliveredContentLength;
	unsigned short	checksum;

	if (length < 16)
	{
		putErrmsg("AMS message header incomplete.", itoa(length));
		return -1;
	}

	if (*(msgBuffer + 2) & 0x80)	/*	Checksum present.	*/
	{
		deliveredContentLength = length - 18;
		if (deliveredContentLength < 0)
		{
			putErrmsg("AMS message truncated.", NULL);
			return -1;
		}

		memcpy((char *) &checksum, msgBuffer + (length - 2), 2);
		checksum = ntohs(checksum);
		if (checksum != computeAmsChecksum(msgBuffer, length - 2))
		{
			putErrmsg("Checksum failed, AMS message discarded.",
					NULL);
			return -1;
		}
	}
	else				/*	No checksum.		*/
	{
		deliveredContentLength = length - 16;
		if (deliveredContentLength < 0)
		{
			putErrmsg("AMS message truncated.", NULL);
			return -1;
		}
	}

	if (getMsgSender(sap, msg, msgBuffer, sender) < 0)
	{
		return -1;
	}

	return deliveredContentLength;
}

int	enqueueAmsMsg(AmsSAP *sap, unsigned char *msgBuffer, int length)
{
	unsigned char	*msgContent = msgBuffer + 16;
	int		deliveredContentLength;
	AmsMsg		msg;
	Module		*sender;
	short		subjectNbr;
	Subject		*subject;
	LystElt		elt;
	AppRole		*role;
	AmsEvt		*evt;

	if (sap == NULL || msgBuffer == NULL || length < 0)
	{
		errno = EINVAL;
		putSysErrmsg(BadParmsMemo, NULL);
		return -1;
	}

	deliveredContentLength = validateAmsMsg(sap, msgBuffer, length, &msg,
			&sender);
	if (deliveredContentLength < 0 || sender == NULL)
	{
		putErrmsg("Invalid AMS message.", NULL);
		return -1;
	}

	if (sender->role->nbr == 1)	/*	Msg from RAMS gateway.	*/
	{
		/*	Skip over header, get to the enclosure.		*/

		msgBuffer += 16;
		msgContent += 16;
		deliveredContentLength = validateAmsMsg(sap, msgBuffer,
				deliveredContentLength, &msg, &sender);
		if (deliveredContentLength < 0)
		{
			putErrmsg("Invalid RAMS enclosure.", NULL);
			return -1;
		}

		/*	sender is NULL, because it's a foreign module .	*/
	}

	/*	Now working with an originally transmitted message.	*/
#if 0
printf("Got  %x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x '%s'\n",
*msgBuffer,
*(msgBuffer + 1),
*(msgBuffer + 2),
*(msgBuffer + 3),
*(msgBuffer + 4),
*(msgBuffer + 5),
*(msgBuffer + 6),
*(msgBuffer + 7),
*(msgBuffer + 8),
*(msgBuffer + 9),
*(msgBuffer + 10),
*(msgBuffer + 11),
*(msgBuffer + 12),
*(msgBuffer + 13),
*(msgBuffer + 14),
*(msgBuffer + 15),
msgBuffer + 16);
fflush(stdout);
#endif
	memcpy((char *) &msg.subjectNbr, msgBuffer + 12, 2);
	msg.subjectNbr = ntohs(msg.subjectNbr);
	if (msg.subjectNbr < 0)	/*	Private msg, to RAMS gateway.	*/
	{
		if (sap->role->nbr != 1)	/*	Misdirected.	*/
		{
			putErrmsg("Message destined for RAMS gateway received \
by non-RAMS-gateway module.", NULL);
			return -1;
		}

		/*	Receiving module is a RAMS gateway.		*/

		subjectNbr = 0 - msg.subjectNbr;
		if (subjectNbr > MaxContinNbr)
		{
			putErrmsg("Received message for invalid continuum.",
					itoa(subjectNbr));
			return -1;
		}

		subject = sap->venture->msgspaces[subjectNbr];
	}
	else
	{
		if (msg.subjectNbr == 0 || msg.subjectNbr > MaxSubjNbr)
		{
			putErrmsg("Received message on invalid subject.",
					itoa(msg.subjectNbr));
			return -1;
		}

		subject = sap->venture->subjects[msg.subjectNbr];
	}

	if (subject == NULL)
	{
		putErrmsg("Received message on unknown subject.",
				itoa(msg.subjectNbr));
		return -1;
	}

	if (msg.continuumNbr == mib->localContinuumNbr)
	{
		if (subject->authorizedSenders != NULL)
		{
			/*	Must check sender's bonafides.		*/

			for (elt = lyst_first(subject->authorizedSenders);
					elt; elt = lyst_next(elt))
			{
				role = (AppRole *) lyst_data(elt);
				if (role == sender->role)
				{
					break;
				}
			}

			if (elt == NULL)
			{
				putErrmsg("Got msg from unauthorized sender.",
						sender->role->name);
				return -1;
			}
		}

		/*	Get rule for processing messages on this subject.*/

		elt = getMsgRule(sap, sap->subscriptions, subject->nbr,
			sender->role->nbr, msg.continuumNbr, msg.unitNbr);
		if (elt == NULL)
		{
			elt = getMsgRule(sap, sap->invitations, subject->nbr,
					sender->role->nbr, msg.continuumNbr,
					msg.unitNbr);
			if (elt == NULL)
			{
				elt = getMsgRule(sap, sap->subscriptions,
						ALL_SUBJECTS, sender->role->nbr,
						msg.continuumNbr, msg.unitNbr);
				if (elt == NULL)
				{
					elt = getMsgRule(sap, sap->invitations,
						ALL_SUBJECTS, sender->role->nbr,
						msg.continuumNbr, msg.unitNbr);
					if (elt == NULL)
					{
						putErrmsg("Received unsolici\
ted message.", subject->name);
						return -1;
					}
				}
			}
		}
	}

	/*	Finish unpacking the message.				*/

	memcpy((char *) &msg.contextNbr, msgBuffer + 8, 4);
	msg.contextNbr = ntohl(msg.contextNbr);
	msg.type = (*msgBuffer >> 4) & 0x03;
	msg.priority = *msgBuffer & 0x0f;
	msg.flowLabel = *(msgBuffer + 1);
	msg.contentLength = ((*(msgBuffer + 14)) << 8) + *(msgBuffer + 15);
	if (msg.contentLength > deliveredContentLength)
	{
		deliveredContentLength -= msg.contentLength;
		putErrmsg("AMS message truncated.",
				itoa(deliveredContentLength));
		return -1;
	}

	if (msg.contentLength == 0)
	{
		msg.content = NULL;
	}
	else
	{
		msg.content = MTAKE(msg.contentLength);
		if (msg.content == NULL)
		{
			putErrmsg(NoMemoryMemo, NULL);
			return -1;
		}

		memcpy(msg.content, msgContent, msg.contentLength);

		/*	Decrypt content as necessary.			*/

		if (subject->symmetricKey)	/*	Key provided.	*/
		{
			decryptUsingSymmetricKey(msg.content, msg.contentLength,
					subject->symmetricKey,
					subject->keyLength, msg.content,
					&msg.contentLength);
		}
	}

	/*	Create and enqueue event, signal application thread.	*/

	evt = (AmsEvt *) MTAKE(1 + sizeof(AmsMsg));
	if (evt == NULL)
	{
		MRELEASE(msg.content);
		putErrmsg(NoMemoryMemo, NULL);
		return -1;
	}

	evt->type = AMS_MSG_EVT;
	memcpy(evt->value, (char *) &msg, sizeof(AmsMsg));
	return enqueueAmsEvent(sap, evt, msg.content, msg.contextNbr,
			msg.priority, msg.type);
}

static void	constructMessage(AmsSAP *sap, short subjectNbr, int priority,
			unsigned char flowLabel, int context, char *content,
			int contentLength, unsigned char *header,
			AmsMsgType msgType)
{
	unsigned long	u8;
	unsigned char	u1;
	short		i2;
	Subject		*subject;

	/*	First octet is two bits of version number (which is
	 *	always 00 for now) followed by two bits of message
	 *	type (unary, query, or reply) followed by four bits
	 *	of priority.						*/

	u8 = msgType;
	u1 = ((u8 << 4) & 0x30) + (priority & 0x0f);
	*header = u1;
	*(header + 1) = flowLabel;
	*(header + 2) = 0x80	/*	Checksum always present.	*/
			+ ((mib->localContinuumNbr >> 8) & 0x0000007f);
	*(header + 3) = mib->localContinuumNbr & 0x000000ff;
	*(header + 4) = (sap->unit->nbr >> 8) & 0x000000ff;
	*(header + 5) = sap->unit->nbr & 0x000000ff;
	*(header + 6) = sap->moduleNbr;
	*(header + 7) = 0;	/*	Reserved.			*/
	context = htonl(context);
	memcpy(header + 8, (char *) &context, 4);
	i2 = htons(subjectNbr);
	memcpy(header + 12, (char *) &i2, 2);
	*(header + 14) = (contentLength >> 8) & 0x000000ff;
	*(header + 15) = contentLength & 0x000000ff;

	/*	Encrypt message content as necessary.			*/

	if (subjectNbr < 0)	/*	RAMS; don't encrypt.		*/
	{
		return;
	}

	subject = sap->venture->subjects[subjectNbr];
	if (subject->symmetricKey)	/*	Key provided.		*/
	{
		encryptUsingSymmetricKey(content, contentLength,
				subject->symmetricKey, subject->keyLength,
				content, &contentLength);
	}
}

static int	enqueueMsgToRegistrar(AmsSAP *sap, MamsPduType pduType,
			signed int memo, unsigned short supplementLength,
			char *supplement)
{
	MamsMsg	msg;
	AmsEvt	*evt;

	memset((char *) &msg, 0, sizeof msg);
	msg.type = pduType;
	msg.memo = memo;
	msg.supplementLength = supplementLength;
	msg.supplement = supplement;
	evt = (AmsEvt *) MTAKE(1 + sizeof(MamsMsg));
	if (evt == NULL)
	{
		putErrmsg(NoMemoryMemo, NULL);
		return -1;
	}

	memcpy(evt->value, (char *) &msg, sizeof msg);
	evt->type = MSG_TO_SEND_EVT;
	if (enqueueMamsEvent(sap->mamsEventsCV, evt, NULL, 0))
	{
		MRELEASE(evt);
		putErrmsg(NoMemoryMemo, NULL);
		return -1;
	}

	return 0;
}

/*	*	*	MAMS message handling functions		*	*/

static int	getDeclarationLength(AmsSAP *sap)
{
	int	declarationLength = 0;

	declarationLength += 2;		/*	Length of the list.	*/
	declarationLength += (SUBSCRIBE_LEN * lyst_length(sap->subscriptions));
	declarationLength += 2;		/*	Length of the list.	*/
	declarationLength += (INVITE_LEN * lyst_length(sap->invitations));
	return declarationLength;
}

static void	loadAssertion(char **cursor, int ruleType, unsigned char
			roleNbr, unsigned int continuumNbr, unsigned short
			unitNbr, short subjectNbr, int vectorNbr, int priority,
			unsigned char flowLabel)
{
	subjectNbr = htons(subjectNbr);
	memcpy(*cursor, (char *) &subjectNbr, 2);
	(*cursor) += 2;
	*(*cursor) = (continuumNbr >> 8) & 0x0000007f;
	*(*cursor + 1) = continuumNbr & 0x000000ff;
	(*cursor) += 2;
	unitNbr = htons(unitNbr);
	memcpy(*cursor, (char *) &unitNbr, 2);
	(*cursor) += 2;
	**cursor = roleNbr;
	(*cursor)++;
	**cursor = ((vectorNbr) << 4) + (priority & 0x0000000f);
	(*cursor)++;
	**cursor = flowLabel;
	(*cursor)++;
}

static void	loadDeclaration(AmsSAP *sap, char *cursor)
{
	LystElt		elt;
	MsgRule		*rule;
	short		i2;

	/*	Load the subscriptions list.				*/

	i2 = lyst_length(sap->subscriptions);
	i2 = htons(i2);
	memcpy(cursor, (char *) &i2, 2);
	cursor += 2;
	for (elt = lyst_first(sap->subscriptions); elt; elt = lyst_next(elt))
	{
		rule = (MsgRule *) lyst_data(elt);
		loadAssertion(&cursor, SUBSCRIPTION, rule->roleNbr,
				rule->continuumNbr, rule->unitNbr,
				rule->subject->nbr, rule->vector->nbr,
				rule->priority, rule->flowLabel);
	}

	/*	Now the invitations list.				*/

	i2 = lyst_length(sap->invitations);
	i2 = htons(i2);
	memcpy(cursor, (char *) &i2, 2);
	cursor += 2;
	for (elt = lyst_first(sap->invitations); elt; elt = lyst_next(elt))
	{
		rule = (MsgRule *) lyst_data(elt);
		loadAssertion(&cursor, INVITATION, rule->roleNbr,
				rule->continuumNbr, rule->unitNbr,
				rule->subject->nbr, rule->vector->nbr,
				rule->priority, rule->flowLabel);
	}
}
#if 0
static void	sendDeclaration(AmsSAP *sap, Module *module)
{
	int	supplementLength;
	char	*supplement;
	char	*cursor;
	int	memo;
	int	result;

	supplementLength = getDeclarationLength(sap);

	/*	Allocate space for the declaration structure.		*/

	if (supplementLength > 65535)
	{
		putErrmsg("Declaration structure too long.",
				itoa(supplementLength));
		return;
	}

	supplement = MTAKE(supplementLength);
	if (supplement == NULL)
	{
		putErrmsg(NoMemoryMemo, NULL);
		return;
	}

	/*	Now load the declaration structure.			*/

	cursor = supplement;
	loadDeclaration(sap, cursor);

	/*	Supplement is now ready; send message.			*/

	memo = computeModuleId(sap->role->nbr, sap->unit->nbr, sap->moduleNbr);
	result = sendMamsMsg(&(module->mamsEndpoint), &(sap->mamsTsif),
			declaration, memo, supplementLength, supplement);
	MRELEASE(supplement);
	if (result < 0)
	{
		putErrmsg("Failed sending declaration", NULL);
	}
}
#endif
static int	getContactSummaryLength(AmsSAP *sap)
{
	int		length = 0;
	LystElt		elt;
	DeliveryVector	*vector;
	LystElt		elt2;
	AmsInterface	*tsif;

	/*	First add length of own NULL-terminated MAMS endpoint
	 *	name.							*/

	length += (strlen(sap->mamsTsif.ept) + 1);

	/*	Then add length of delivery vector list.  Each endpoint
	 *	in each vector is terminated by ',' except the last,
	 *	which is NULL-terminated.				*/

	length += 1;		/*	Length of vectors list.		*/
	for (elt = lyst_first(sap->delivVectors); elt; elt = lyst_next(elt))
	{
		length += 1;	/*	Vector # and ep count.	*/
		vector = (DeliveryVector *) lyst_data(elt);
		for (elt2 = lyst_first(vector->interfaces); elt2;
				elt2 = lyst_next(elt2))
		{
			tsif = (AmsInterface *) lyst_data(elt2);
			length += (strlen(tsif->ts->name)
					+ 1 + strlen(tsif->ept) + 1);
		}
	}

	return length;
}

static void	loadContactSummary(AmsSAP *sap, char *cursor, int bufsize)
{
	LystElt		elt;
	DeliveryVector	*vector;
	unsigned char	u1;
	LystElt		elt2;
	AmsInterface	*tsif;
	LystElt		nextElt;
	int		len;

	/*	First load own NULL-terminated MAMS endpoint name.	*/

	istrcpy(cursor, sap->mamsTsif.ept, bufsize);
	cursor += (strlen(cursor) + 1);

	/*	Then load the delivery vector list.			*/

	*cursor = lyst_length(sap->delivVectors);
	cursor++;
	for (elt = lyst_first(sap->delivVectors); elt; elt = lyst_next(elt))
	{
		vector = (DeliveryVector *) lyst_data(elt);
		u1 = lyst_length(vector->interfaces) & 0x0000000f;
		u1 += (vector->nbr << 4);
		*cursor = u1;
		cursor++;
		for (elt2 = lyst_first(vector->interfaces); elt2;
				elt2 = nextElt)
		{
			nextElt = lyst_next(elt2);
			tsif = (AmsInterface *) lyst_data(elt2);
			len = strlen(tsif->ts->name);
			memcpy(cursor, tsif->ts->name, len);
			cursor += len;
			*cursor = '=';
			cursor++;
			len = strlen(tsif->ept);
			memcpy(cursor, tsif->ept, len);
			cursor += len;
			if (nextElt)
			{
				*cursor = ',';
			}
			else
			{
				*cursor = '\0';
			}

			cursor++;
		}
	}
}

static void	sendModuleStatus(AmsSAP *sap, MamsEndpoint *maap, int pduType)
{
	int	moduleStatesCount = 1;	/*	Dummy count field.	*/
	int	supplementLength = 4;	/*	Length of count.	*/
	int	contactSummaryLength;
	char	*supplement;
	int	memo;
	int	result;

	moduleStatesCount = htonl(moduleStatesCount);
	supplementLength += 4;		/*	Unit, module, role nbrs.	*/
	contactSummaryLength = getContactSummaryLength(sap);
	supplementLength += contactSummaryLength;
       	supplementLength += getDeclarationLength(sap);
	if (supplementLength > 65535)
	{
		putErrmsg("Module status structure too long.",
				itoa(supplementLength));
		return;
	}

	supplement = MTAKE(supplementLength);
	if (supplement == NULL)
	{
		putErrmsg(NoMemoryMemo, NULL);
		return;
	}

	memcpy(supplement, (char *) &moduleStatesCount, 4);
	supplement[4] = (sap->unit->nbr >> 8) & 0xff;
	supplement[5] = sap->unit->nbr & 0xff;
	supplement[6] = sap->moduleNbr & 0xff;
	supplement[7] = sap->role->nbr & 0xff;
	loadContactSummary(sap, supplement + 8, supplementLength - 8);
	loadDeclaration(sap, supplement + 8 + contactSummaryLength);
	if (pduType == module_status)
	{
		memo = computeModuleId(sap->role->nbr, sap->unit->nbr,
				sap->moduleNbr);
	}
	else
	{
		memo = 0;
	}

	/*	Message is now ready; send it.				*/

	result = sendMamsMsg(maap, &(sap->mamsTsif), pduType, memo,
			supplementLength, supplement);
	MRELEASE(supplement);
       	if (result < 0)
	{
		putErrmsg("Failed sending module status.", NULL);
	}
}

static void	eraseAmsEndpoint(AmsEndpoint *ep)
{
	if (ep == NULL) return;
	if (ep->ept)
	{
		MRELEASE(ep->ept);
	}

	ep->ts->clearAmsEndpointFn(ep);
	MRELEASE(ep);
}

void	destroyAmsEndpoint(LystElt elt, void *userdata)
{
	eraseAmsEndpoint((AmsEndpoint *) lyst_data(elt));
}

static int	subjectIsValid(AmsSAP *sap, int subjectNbr, Subject **subject)
{
	int	pseudoSubjectNbr;

	if (subjectNbr > 0)
	{
		if (subjectNbr <= MaxSubjNbr
		&& (*subject = sap->venture->subjects[subjectNbr]) != NULL)
		{
			return 1;
		}
	}

	if (subjectNbr < 0)
	{
		if ((pseudoSubjectNbr = 0 - subjectNbr) <= MaxContinNbr
		&& (*subject = sap->venture->msgspaces[pseudoSubjectNbr])
			!= NULL)
		{
			return 1;
		}
	}

	putErrmsg("Unknown message subject.", itoa(subjectNbr));
	errno = EINVAL;
	return 0;
}

static LystElt	findSubjOfInterest(AmsSAP *sap, Module *module, Subject *subject,
			LystElt *nextSubj)
{
	LystElt		elt;
	SubjOfInterest	*subj;

	/*	This function finds the SubjOfInterest containing
	 *	all XmitRules asserted by this module for the specified
	 *	subject, if any.					*/

//fprintf(stderr, "subjects list length is %d.\n", (int) lyst_length(module->subjects));
	if (nextSubj) *nextSubj = NULL;	/*	Default.		*/
	for (elt = lyst_first(module->subjects); elt; elt = lyst_next(elt))
	{
		subj = (SubjOfInterest *) lyst_data(elt);
		if (subj->subject->nbr < subject->nbr)
		{
			continue;
		}

		if (subj->subject->nbr > subject->nbr)
		{
			if (nextSubj) *nextSubj = elt;
			break;		/*	Same as end of list.	*/
		}

		/*	Matched subject number.				*/

		return elt;
	}

	return NULL;
}

static LystElt	findFanModule(AmsSAP *sap, Subject *subject, Module *module,
			LystElt *nextIntn)
{
	LystElt		elt;
	FanModule	*fan;

	/*	This function finds the FanModule containing
	 *	all XmitRule asserted by this module for the specified
	 *	subject, if any.					*/

	if (nextIntn) *nextIntn = NULL;	/*	Default.		*/
	for (elt = lyst_first(subject->modules); elt; elt = lyst_next(elt))
	{
		fan = (FanModule *) lyst_data(elt);
		if (fan->module->unitNbr < module->unitNbr)
		{
			continue;
		}

		if (fan->module->unitNbr > module->unitNbr)
		{
			if (nextIntn) *nextIntn = elt;
			break;		/*	Same as end of list.	*/
		}

		/*	Found an FanModule in same unit.		*/

		if (fan->module->nbr < module->nbr)
		{
			continue;
		}

		if (fan->module->nbr > module->nbr)
		{
			if (nextIntn) *nextIntn = elt;
			break;		/*	Same as end of list.	*/
		}

		/*	Matched unit and module numbers.			*/

		return elt;
	}

	return NULL;
}

static LystElt	findXmitRule(AmsSAP *sap, Lyst rules, int domainRoleNbr,
			int domainContinuumNbr, int domainUnitNbr,
			LystElt *nextRule)
{
	LystElt		elt;
	XmitRule	*rule;

	/*	This function finds the XmitRule in this list that
	 *	matches the specified role, continuum, and unit.	*/

	if (nextRule) *nextRule = NULL;	/*	Default.		*/
	for (elt = lyst_first(rules); elt; elt = lyst_next(elt))
	{
		rule = (XmitRule *) lyst_data(elt);
		if (rule->roleNbr != domainRoleNbr)
		{
			if (rule->roleNbr == 0)
			{
				/*	Must insert before this one.	*/

				if (nextRule) *nextRule = elt;
				break;
			}

			continue;
		}

		/*	Matched rule's role number.			*/

		if (rule->continuumNbr != domainContinuumNbr)
		{
			if (rule->continuumNbr == 0)
			{
				/*	Must insert before this one.	*/

				if (nextRule) *nextRule = elt;
				break;
			}

			continue;
		}

		/*	Matched rule's continuum number.		*/

		if (rule->unitNbr != domainUnitNbr)
		{
			if (subunitOf(sap, domainUnitNbr, rule->unitNbr))
			{
				/*	Must insert before this one.	*/

				if (nextRule) *nextRule = elt;
				break;
			}

			continue;
		}

		/*	Exact match.					*/

		return elt;
	}

	return NULL;
}

static int	enqueueNotice(AmsSAP *sap, AmsStateType stateType,
			AmsChangeType changeType, int unitNbr,
			int moduleNbr, int roleNbr, int domainContinuumNbr,
			int domainUnitNbr, int subjectNbr, int priority,
			unsigned char flowLabel, AmsSequence sequence,
			AmsDiligence diligence)
{
	AmsNotice	notice;
	AmsEvt		*evt;

	memset((char *) &notice, 0, sizeof notice);
	notice.stateType = stateType;
	notice.changeType = changeType;
	notice.unitNbr = unitNbr;
	notice.moduleNbr = moduleNbr;
	notice.roleNbr = roleNbr;
	notice.domainContinuumNbr = domainContinuumNbr;
	notice.domainUnitNbr = domainUnitNbr;
	notice.subjectNbr = subjectNbr;
	notice.priority = priority;
	notice.flowLabel = flowLabel;
	notice.sequence = sequence;
	notice.diligence = diligence;
	evt = (AmsEvt *) MTAKE(1 + sizeof(AmsNotice));
	if (evt == NULL)
	{
		putErrmsg(NoMemoryMemo, NULL);
		return -1;
	}

	memcpy(evt->value, (char *) &notice, sizeof notice);
	evt->type = NOTICE_EVT;
	if (enqueueAmsEvent(sap, evt, NULL, 0, 2, AmsMsgNone) < 0)
	{
		MRELEASE(evt);
		putErrmsg(NoMemoryMemo, NULL);
		return -1;
	}

	return 0;
}

static int	logCancellation(AmsSAP *sap, Module *module, int domainRoleNbr,
			int domainContinuumNbr, int domainUnitNbr,
			int subjectNbr, int ruleType)
{
	AmsStateType	stateType;

	stateType = (ruleType == SUBSCRIPTION ?
			AmsSubscriptionState : AmsInvitationState);
	if (enqueueNotice(sap, stateType, AmsStateEnds, module->unitNbr,
			module->nbr, domainRoleNbr, domainContinuumNbr,
			domainUnitNbr, subjectNbr, 0, 0, 0, 0) < 0)
	{
		return -1;
	}

	return 0;
}

static int	noteAssertion(AmsSAP *sap, Module *module, Subject *subject,
			int domainRoleNbr, int domainContinuumNbr,
			int domainUnitNbr, int priority, int flowLabel,
			AmsEndpoint *point, int ruleType, int flag)
{
	LystElt		elt;
	LystElt		nextSubj;
	SubjOfInterest	*subj;
	LystElt		nextIntn;
	FanModule	*fan = NULL;
	Lyst		rules;
	LystElt		nextRule;
	XmitRule	*rule;
	AppRole		*role;

	/*	Record the message reception relationship.		*/

	elt = findSubjOfInterest(sap, module, subject, &nextSubj);
	if (elt == NULL)	/*	New subject for this module.	*/
	{
//fprintf(stderr, "...adding new SubjOfInterest %d.\n", subject->nbr);
		subj = (SubjOfInterest *) MTAKE(sizeof(SubjOfInterest));
		if (subj == NULL)
		{
			putErrmsg(NoMemoryMemo, NULL);
			return -1;
		}

		subj->subject = subject;
		subj->subscriptions = lyst_create_using(amsMemory);
		if (subj->subscriptions == NULL)
		{
			MRELEASE(subj);
			putErrmsg(NoMemoryMemo, NULL);
			return -1;
		}

		lyst_delete_set(subj->subscriptions, destroyXmitRule, NULL);
		subj->invitations = lyst_create_using(amsMemory);
		if (subj->invitations == NULL)
		{
			lyst_destroy(subj->subscriptions);
			MRELEASE(subj);
			putErrmsg(NoMemoryMemo, NULL);
			return -1;
		}

		lyst_delete_set(subj->invitations, destroyXmitRule, NULL);
		if (nextSubj)	/*	Insert before this point.	*/
		{
			elt = lyst_insert_before(nextSubj, subj);
		}
		else		/*	Insert at end of list.		*/
		{
			elt = lyst_insert_last(module->subjects, subj);
		}

		if (elt == NULL)
		{
			lyst_destroy(subj->invitations);
			lyst_destroy(subj->subscriptions);
			MRELEASE(subj);
			putErrmsg(NoMemoryMemo, NULL);
			return -1;
		}

		/*	Also need to insert new FanModule for
		 *	this subject.					*/

		elt = findFanModule(sap, subject, module, &nextIntn);
		if (elt)	/*	Should be NULL.			*/
		{
			putErrmsg("AACK!  FanModules list out of sync!",
					subject->name);
			return -1;
		}

		fan = (FanModule *) MTAKE(sizeof(FanModule));
		if (fan == NULL)
		{
			lyst_destroy(subj->invitations);
			lyst_destroy(subj->subscriptions);
			MRELEASE(subj);
			putErrmsg(NoMemoryMemo, NULL);
			return -1;
		}

		fan->module = module;
		fan->subj = subj;
		if (nextIntn)	/*	Insert before this point.	*/
		{
			subj->fanElt = lyst_insert_before(nextIntn, fan);
		}
		else		/*	Insert at end of list.		*/
		{
			subj->fanElt = lyst_insert_last(subject->modules, fan);
		}

		if (subj->fanElt == NULL)
		{
			MRELEASE(fan);
			lyst_destroy(subj->invitations);
			lyst_destroy(subj->subscriptions);
			MRELEASE(subj);
			putErrmsg(NoMemoryMemo, NULL);
			return -1;
		}
	}
	else	/*	Module already has some interest in this subject.	*/
	{
		subj = (SubjOfInterest *) lyst_data(elt);
	}

	rules = (ruleType == SUBSCRIPTION ?
			subj->subscriptions : subj->invitations);
//fprintf(stderr, "ruleType = %d, subj = %d, subject = %d, rules list is %d.\n",ruleType, (int) subj, subj->subject->nbr, (int) rules);
	elt = findXmitRule(sap, rules, domainRoleNbr, domainContinuumNbr,
			domainUnitNbr, &nextRule);
	if (elt)
	{
		/*	Already have a transmission rule for this
		 *	role, continuum, and unit.			*/

		rule = (XmitRule *) lyst_data(elt);
		if (rule->amsEndpoint == point
		&& rule->priority == priority
		&& rule->flowLabel == flowLabel)
		{
			/*	Confirm this rule if necessary.		*/

			rule->flags |= flag;
			return 0;
		}

		/*	Rule has changed.  Delete the old one.		*/

		nextRule = lyst_next(elt);
		lyst_delete(elt);
		if (logCancellation(sap, module, rule->roleNbr,
				rule->continuumNbr, rule->unitNbr,
				subj->subject->nbr, ruleType))
		{
			return -1;
		}
	}

	/*	Need to insert a new transmission rule.  First, create
	 *	the rule (per subscription or invitation).		*/

	rule = (XmitRule *) MTAKE(sizeof(XmitRule));
	if (rule == NULL)
	{
		putErrmsg(NoMemoryMemo, NULL);
		return -1;
	}

	memset((char *) rule, 0, sizeof(XmitRule));

	/*	Insert the rule into module's list of transmission rules
	 *	for this subject.					*/

	if (nextRule)		/*	Insert before this point.	*/
	{
		elt = lyst_insert_before(nextRule, rule);
	}
	else			/*	Insert at end of list.		*/
	{
		elt = lyst_insert_last(rules, rule);
	}

	if (elt == NULL)
	{
		MRELEASE(rule);
		putErrmsg(NoMemoryMemo, NULL);
		return -1;
	}
//fprintf(stderr, "...inserted rule in rules list %d...\n", (int) rules);

	/*	Load XmitRule information, including pre-determination
	 *	of whether or not this module is actually authorized to
	 *	receive these messages at all.				*/

	rule->module = fan;
	rule->subject = subj;
	rule->roleNbr = domainRoleNbr;
	rule->continuumNbr = domainContinuumNbr;
	rule->unitNbr = domainUnitNbr;
	rule->priority = priority;
	rule->flowLabel = flowLabel;
	rule->amsEndpoint = point;
	rule->flags = flag;
	if (subject->authorizedReceivers == NULL)
	{
		rule->flags |= XMIT_IS_OKAY;
	}
	else
	{
		for (elt = lyst_first(subject->authorizedReceivers); elt;
				elt = lyst_next(elt))
		{
			role = (AppRole *) lyst_data(elt);
			if (role == module->role)
			{
				rule->flags |= XMIT_IS_OKAY;
				break;
			}
		}
	}

	return 0;
}

static int	logAssertion(AmsSAP *sap, Module *module, int domainRoleNbr,
			int domainContinuumNbr, int domainUnitNbr,
			int subjectNbr, int priority, unsigned char flowLabel,
			AmsSequence sequence, AmsDiligence diligence,
			int ruleType)
{
	AmsStateType	stateType;

	stateType = (ruleType == SUBSCRIPTION ?
			AmsSubscriptionState : AmsInvitationState);
	if (enqueueNotice(sap, stateType, AmsStateBegins, module->unitNbr,
			module->nbr, domainRoleNbr, domainContinuumNbr,
			domainUnitNbr, subjectNbr, priority, flowLabel,
			sequence, diligence) < 0)
	{
		return -1;
	}

	return 0;
}

static int	processAssertion(AmsSAP *sap, Module *module, int subjectNbr,
			int domainRoleNbr, int domainContinuumNbr,
			int domainUnitNbr, int priority, unsigned char
			flowLabel, int vectorNbr, int ruleType, int flag)
{
	Subject		*subject;
	LystElt		elt;
	AmsEndpoint	*point;

	if (subjectNbr == 0)
	{
		if (ruleType == SUBSCRIPTION)
		{
			if (domainContinuumNbr != mib->localContinuumNbr)
			{
			/*	Can't subscribe to "all subjects" in
			 *	any continuum other than the local
			 *	continuum, so skip this assertion.	*/

				return 0;
			}
		}

		/*	(Invitation for messages on subject zero is
		 *	always okay.  We coerce the domain continuum:
		 *	it can only be the local continuum.)		*/

		subject = sap->venture->subjects[0];
		domainContinuumNbr = mib->localContinuumNbr;
	}
	else
	{
		if (!subjectIsValid(sap, subjectNbr, &subject))
		{
			/*	Subject is unknown, so we skip
			 *	this assertion.				*/

			return 0;
		}
	}

	for (elt = lyst_first(module->amsEndpoints); elt; elt = lyst_next(elt))
	{
		point = (AmsEndpoint *) lyst_data(elt);
		if (point->deliveryVectorNbr < vectorNbr)
		{
			continue;
		}

		if (point->deliveryVectorNbr > vectorNbr)
		{
			elt = NULL;
		}

		break;	/*	In either case, skip rest of list.	*/
	}

	if (elt == NULL)
	{
		/*	No matching delivery vector number; messages
		 *	on this subject are therefore undeliverable to
		 *	the asserting module, so we skip this assertion.	*/

			return 0;
	}

	/*	Subject & vector are okay; messages are deliverable to
	 *	the asserting module.					*/

	if (noteAssertion(sap, module, subject, domainRoleNbr, domainContinuumNbr,
		domainUnitNbr, priority, flowLabel, point, ruleType, flag) < 0)
	{
		return -1;
	}

	return logAssertion(sap, module, domainRoleNbr, domainContinuumNbr,
			domainUnitNbr, subjectNbr, priority, flowLabel,
			point->sequence, point->diligence, ruleType);
}

static int	parseAssertion(AmsSAP *sap, Module *module, int *bytesRemaining,
			char **cursor, int ruleType, int flag)
{
	int		bytesNeeded;
	short		i2;
	unsigned short	u2;
	int		subjectNbr;
	int		domainContinuumNbr;
	int		domainUnitNbr;
	int		domainRoleNbr;
	unsigned char	u1;
	int		vectorNbr;
	int		priority;
	unsigned char	flowLabel;

	bytesNeeded = (ruleType == SUBSCRIPTION ?  SUBSCRIBE_LEN : INVITE_LEN);
	if (*bytesRemaining < bytesNeeded)
	{
		putErrmsg("Assertion truncated.", NULL);
		return -1;
	}

	memcpy((char *) &i2, *cursor, 2);
	i2 = ntohs(i2);
	subjectNbr = i2;
	(*cursor) += 2;
	(*bytesRemaining) -= 2;
	domainContinuumNbr = ((unsigned char ) **cursor) & 0x7f;
	(*cursor)++;
	domainContinuumNbr <<= 8;
	domainContinuumNbr += (unsigned char ) **cursor;
	(*cursor)++;
	(*bytesRemaining) -= 2;
	memcpy((char *) &u2, *cursor, 2);
	u2 = ntohs(u2);
	domainUnitNbr = u2;
	(*cursor) += 2;
	(*bytesRemaining) -= 2;
	domainRoleNbr = (unsigned char) **cursor;
	(*cursor)++;
	(*bytesRemaining)--;
	u1 = (unsigned char) **cursor;
	vectorNbr = (u1 >> 4) & 0x0f;
	priority = u1 & 0x0f;
	(*cursor)++;
	(*bytesRemaining)--;
	flowLabel = (unsigned char) **cursor;
	(*cursor)++;
	(*bytesRemaining)--;
	return processAssertion(sap, module, subjectNbr, domainRoleNbr,
			domainContinuumNbr, domainUnitNbr, priority, flowLabel,
			vectorNbr, ruleType, flag);
}

static int	cancelUnconfirmedAssertions(AmsSAP *sap, Module *module)
{
	LystElt		elt;
	SubjOfInterest	*subj;
	LystElt		elt2;
	XmitRule	*rule;
	LystElt		nextElt;

	for (elt = lyst_first(module->subjects); elt; elt = lyst_next(elt))
	{
		subj = (SubjOfInterest *) lyst_data(elt);
		for (elt2 = lyst_first(subj->subscriptions); elt2;
				elt2 = nextElt)
		{
			nextElt = lyst_next(elt2);
			rule = (XmitRule *) lyst_data(elt2);

			/*	Where flag is confirmed, turn off
			 *	the confirmation bit; where not
			 *	confirmed, turn off the flag itself.	*/

			if (rule->flags & RULE_CONFIRMED)
			{
				rule->flags &= (~RULE_CONFIRMED);
			}
			else
			{
				lyst_delete(elt2);
				if (logCancellation(sap, module, rule->roleNbr,
					rule->continuumNbr, rule->unitNbr,
					subj->subject->nbr, SUBSCRIPTION))
				{
					return -1;
				}
			}
		}

		for (elt2 = lyst_first(subj->invitations); elt2;
				elt2 = nextElt)
		{
			nextElt = lyst_next(elt2);
			rule = (XmitRule *) lyst_data(elt2);

			/*	Where flag is confirmed, turn off
			 *	the confirmation bit; where not
			 *	confirmed, turn off the flag itself.	*/

			if (rule->flags & RULE_CONFIRMED)
			{
				rule->flags &= (~RULE_CONFIRMED);
			}
			else
			{
				lyst_delete(elt2);
				if (logCancellation(sap, module, rule->roleNbr,
					rule->continuumNbr, rule->unitNbr,
					subj->subject->nbr, INVITATION))
				{
					return -1;
				}
			}
		}
	}

	return 0;
}

static int	processDeclaration(AmsSAP *sap, Module *module, int bytesRemaining,
			char *cursor, int flag)
{
	int		assertionCount;
	unsigned short	u2;

	if (bytesRemaining < 2)
	{
		putErrmsg("Declaration lacks subscription count.", NULL);
		return -1;
	}

	memcpy((char *) &u2, cursor, 2);
	cursor += 2;
	bytesRemaining -= 2;
	u2 = ntohs(u2);
	assertionCount = u2;
	while (assertionCount > 0)
	{
//fprintf(stderr, "Parsing decl subscription with %d bytes remaining.\n", bytesRemaining);
		if (parseAssertion(sap, module, &bytesRemaining, &cursor,
				SUBSCRIPTION, flag) < 0)
		{
			putErrmsg("Error parsing subscription.", NULL);
			return -1;
		}

		assertionCount--;
	}

	if (bytesRemaining < 2)
	{
		putErrmsg("Declaration lacks invitation count.", NULL);
		return -1;
	}

	memcpy((char *) &u2, cursor, 2);
	cursor += 2;
	bytesRemaining -= 2;
	u2 = ntohs(u2);
	assertionCount = u2;
	while (assertionCount > 0)
	{
//fprintf(stderr, "Parsing decl invitation with %d bytes remaining.\n", bytesRemaining);
		if (parseAssertion(sap, module, &bytesRemaining, &cursor,
				INVITATION, flag) < 0)
		{
			putErrmsg("Error parsing invitation.", NULL);
			return -1;
		}

		assertionCount--;
	}

	return 0;
}

static int	processCancellation(AmsSAP *sap, Module *module, int subjectNbr,
			int domainRoleNbr, int domainContinuumNbr,
			int domainUnitNbr, int ruleType)
{
	Subject		*subject;
	LystElt		elt;
	SubjOfInterest	*subj;
	Lyst		rules;

	if (subjectNbr == 0)
	{
		if (ruleType == SUBSCRIPTION)
		{
			if (domainContinuumNbr == mib->localContinuumNbr)
			{
				subject = sap->venture->subjects[ALL_SUBJECTS];
			}
			else
			{
			/*	Can't subscribe to "all subjects" in
			 *	any continuum other than the local
			 *	continuum, so skip this cancellation.	*/

				return 0;
			}
		}
		else
		{
			/*	Disinviting messages on subject zero
			 *	is never okay.				*/

			return 0;
		}
	}
	else
	{
		if (!subjectIsValid(sap, subjectNbr, &subject))
		{
			/*	Subject is unknown, so we skip
			 *	this cancellation.			*/

			return 0;
		}
	}

	elt = findSubjOfInterest(sap, module, subject, NULL);
	if (elt == NULL)	/*	No interest in subject.		*/
	{
		return 0;	/*	Nothing to cancel.		*/
	}

	subj = (SubjOfInterest *) lyst_data(elt);
	rules = (ruleType == SUBSCRIPTION ?
			subj->subscriptions : subj->invitations);
	elt = findXmitRule(sap, rules, domainRoleNbr, domainContinuumNbr,
			domainUnitNbr, NULL);
	if (elt == NULL)	/*	No such rule for this subject.	*/
	{
		return 0;	/*	Nothing to cancel.		*/
	}

	lyst_delete(elt);
	if (logCancellation(sap, module, domainRoleNbr, domainContinuumNbr,
			domainUnitNbr, subjectNbr, ruleType))
	{
		return -1;
	}

	return 0;
}

static int	parseAmsEndpoint(Module *module, int *bytesRemaining, char **cursor,
			char **tsname, int *eptLength, char **ept)
{
	int	gotIt = 0;

	*tsname = *cursor;
	*eptLength = 0;
	*ept = NULL;
	while (*bytesRemaining > 0)
	{
		if (*ept == NULL)	/*	Seeking end of ts name.	*/
		{
			if (**cursor == '=')	/*	End of ts name.	*/
			{
				**cursor = '\0';
				*ept = (*cursor) + 1;
			}
		}
		else			/*	Seeking end of ept.	*/
		{
			if (**cursor == ',')	/*	End of ept.	*/
			{
				**cursor = '\0';
			}

			if (**cursor == '\0')	/*	End of ept.	*/
			{
				gotIt = 1;
			}
			else
			{
				(*eptLength)++;
			}
		}

		(*cursor)++;
		(*bytesRemaining)--;
		if (gotIt)
		{
			return 0;
		}
	}

	putErrmsg("Incomplete AMS endpoint name.", NULL);
	return -1;
}

static int	insertAmsEndpoint(Module *module, int vectorNbr, TransSvc *ts,
			int eptLength, char *ept)
{
	AmsEndpoint	*ep;
	LystElt		elt;
	AmsEndpoint	*ep2;
	LystElt		nextElt;

	ep = (AmsEndpoint *) MTAKE(sizeof(AmsEndpoint));
	if (ep == NULL)
	{
		putErrmsg(NoMemoryMemo, NULL);
		return -1;
	}

	ep->ept = MTAKE(eptLength + 1);
	if (ep->ept == NULL)
	{
		putErrmsg(NoMemoryMemo, NULL);
		return -1;
	}

	memcpy(ep->ept, ept, eptLength);
	ep->ept[eptLength] = '\0';
	ep->ts = ts;
	ep->deliveryVectorNbr = vectorNbr;

	/*	The transport service's endpoint parsing function
	 *	examines the endpoint name text and fills in the tsep
	 *	of the endpoint and the diligence and delivery order
	 *	supported by the transport service.			*/

	if ((ts->parseAmsEndpointFn)(ep))
	{
		putErrmsg("Can't parse endpoint name.", ept);
		return -1;
	}

	/*	Insert new endpoint into module's AMS endpoints
	 *	list, in ascending delivery vector number order.	*/

	for (elt = lyst_first(module->amsEndpoints); elt; elt = nextElt)
	{
		nextElt = lyst_next(elt);
		ep2 = (AmsEndpoint *) lyst_data(elt);
		if (ep2->deliveryVectorNbr < vectorNbr)
		{
			continue;
		}

		if (ep2->deliveryVectorNbr == vectorNbr)
		{
			/*	Same as a previously declared delivery
			 *	vector; new one replaces the old one.	*/

			lyst_delete(elt);
			elt = nextElt;
		}

		/*	Insert new endpoint at this location in the
		 *	list.						*/

		break;
	}

	if (elt == NULL)	/*	Insert at the end of the list.	*/
	{
		elt = lyst_insert_last(module->amsEndpoints, ep);
	}
	else
	{
		elt = lyst_insert_before(elt, ep);
	}

	if (elt == NULL)
	{
		eraseAmsEndpoint(ep);
		putErrmsg(NoMemoryMemo, NULL);
		return -1;
	}

	return 0;
}

static int	parseDeliveryVector(Module *module, int *bytesRemaining,
			char **cursor)
{
	unsigned char	u1;
	int		vectorNbr;
	int		pointCount;
	char		*tsname;
	int		eptLength;
	char		*ept;
	int		i;
	TransSvc	*ts;
	int		result;
	int		endpointInserted = 0;

	if (*bytesRemaining < 1)
	{
		putErrmsg("Delivery vector lacks point count.", NULL);
		return -1;
	}

	u1 = **cursor;
	vectorNbr = (u1 >> 4) & 0x0f;
	pointCount = u1 & 0x0f;
	(*cursor)++;
	(*bytesRemaining)--;
	while (pointCount > 0)
	{
		if (parseAmsEndpoint(module, bytesRemaining, cursor, &tsname,
				&eptLength, &ept) < 0)
		{
			putErrmsg("Error parsing delivery vector.", NULL);
			return -1;
		}

		pointCount--;
		if (endpointInserted)
		{
			continue;	/*	Saved best fit already.	*/
		}

		/*	Look for a tsif that can send to this point.	*/

		for (i = 0; i < mib->transportServiceCount; i++)
		{
			ts = &(mib->transportServices[i]);
			if (strcmp(ts->name, tsname) == 0)
			{
				/*	We can send to this point.	*/

				result = insertAmsEndpoint(module, vectorNbr, ts,
						eptLength, ept);
				if (result < 0)
				{
					putErrmsg("Error inserting endpoint.",
							NULL);
					return -1;
				}

				endpointInserted = 1;
				break;
			}
		}
	}

	/*	Note: if endpointInserted is still zero at this point,
	 *	all messages on any subjects on which this module
	 *	requests delivery via this delivery vector will be
	 *	undeliverable.						*/

	return 0;
}

static int	parseDeliveryVectorList(Module *module, int *bytesRemaining,
			char **cursor)
{
	int	vectorCount;

	if (*bytesRemaining < 1)
	{
		putErrmsg("Contact summary lacks delivery vector count.", NULL);
		return -1;
	}

	vectorCount = **cursor;
	(*cursor)++;
	(*bytesRemaining)--;
	while (vectorCount > 0)
	{
		if (parseDeliveryVector(module, bytesRemaining, cursor))
		{
			putErrmsg("Error parsing delivery vector.", NULL);
			return -1;
		}

		vectorCount--;
	}

	return 0;
}

static int	noteModule(AmsSAP *sap, int roleNbr, int unitNbr, int moduleNbr,
			MamsMsg *msg, int *bytesRemaining, char **cursor)
{
	char	*ept;
	int	eptLength;
	AppRole	*role;
	Cell	*cell;
	Module	*module;

	if (roleNbr < 1 || roleNbr > MaxRoleNbr)
	{
		putErrmsg("role nbr invalid.", itoa(roleNbr));
		return -1;
	}

	ept = parseString(cursor, bytesRemaining, &eptLength);
	if (ept == NULL)
	{
		putErrmsg("No MAMS endpoint ID string.", NULL);
		return -1;
	}

	role = sap->venture->roles[roleNbr];
	cell = sap->venture->units[unitNbr]->cell;
	module = cell->modules[moduleNbr];
	if (module->role == NULL)	/*	Not previously announced.	*/
	{
		if (rememberModule(module, role, eptLength, ept) < 0)
		{
			putErrmsg("Can't retain module announcement.", NULL);
			return -1;
		}
	}
	else			/*	Module previously announced.	*/
	{
		if (module->role != role
		|| strcmp(module->mamsEndpoint.ept, ept) != 0)
		{
			putErrmsg("Conflicting module announcements, \
discarding message.", NULL);
			return -1;
		}
	}

	if (parseDeliveryVectorList(module, bytesRemaining, cursor) < 0)
	{
		putErrmsg("Can't retain module announcement.", NULL);
		return -1;
	}

	/*	Notify the application, whether new or not.		*/

	if (enqueueNotice(sap, AmsRegistrationState, AmsStateBegins,
			unitNbr, moduleNbr, roleNbr, 0, 0, 0, 0, 0, 0, 0) < 0)
	{
		putErrmsg(NoMemoryMemo, NULL);
		return -1;
	}

	return 0;
}

static void	processMamsMsg(AmsSAP *sap, AmsEvt *evt)
{
	MamsMsg		*msg = (MamsMsg *) (evt->value);
	int		i;
	int		unitNbr;
	Unit		*unit;
	int		moduleNbr;
	Module		*module;
	int		roleNbr;
	char		*cursor;
	int		bytesRemaining;
	unsigned long	u4;
	short		i2;
	unsigned short	u2;
	int		subjectNbr;
	int		domainContinuumNbr;
	int		domainUnitNbr;
	int		domainRoleNbr;
	int		moduleCount;

//printf("Module '%d' got msg of type %d.\n", sap->role->nbr, msg->type);
	switch (msg->type)
	{
	case heartbeat:
		sap->heartbeatsMissed = 0;
		return;

	case you_are_dead:
		if (enqueueAmsCrash(sap,
			"Killed by registrar; imputed crash.") < 0)
		{
			putErrmsg(NoMemoryMemo, NULL);
		}

		return;

	case I_am_starting:
		if (msg->supplementLength < 1)
		{
			putErrmsg("I_am_starting lacks MAMS endpoint.", NULL);
			return;
		}

		if (parseModuleId(msg->memo, &roleNbr, &unitNbr, &moduleNbr) < 0)
		{
			putErrmsg("I_am_starting memo field invalid.", NULL);
			return;
		}

		if (unitNbr == sap->unit->nbr && moduleNbr == sap->moduleNbr)
		{
			return;	/*	Own self-announcement; ignore.	*/
		}

		bytesRemaining = msg->supplementLength;
		cursor = msg->supplement;
		if (noteModule(sap, roleNbr, unitNbr, moduleNbr, msg,
				&bytesRemaining, &cursor) < 0)
		{
			putErrmsg("Failed handling I_am_starting.", NULL);
			return;
		}

		/*	Tell the new module about self.			*/

		unit = sap->venture->units[unitNbr];
		module = unit->cell->modules[moduleNbr];
		sendModuleStatus(sap, &module->mamsEndpoint, I_am_here);
		return;

	case module_has_started:
		if (msg->supplementLength < 1)
		{
			putErrmsg("module_has_started lacks MAMS endpoint.",
					NULL);
			return;
		}

		if (parseModuleId(msg->memo, &roleNbr, &unitNbr, &moduleNbr) < 0)
		{
			putErrmsg("module_has_started memo field invalid.", NULL);
			return;
		}

		if (unitNbr == sap->unit->nbr && moduleNbr == sap->moduleNbr)
		{
			return;	/*	Own self-announcement; ignore.	*/
		}

		bytesRemaining = msg->supplementLength;
		cursor = msg->supplement;
		if (noteModule(sap, roleNbr, unitNbr, moduleNbr, msg,
				&bytesRemaining, &cursor) < 0)
		{
			putErrmsg("Failed handling module_has_started.", NULL);
		}

		return;

	case I_am_here:
		if (msg->supplementLength < 4)
		{
			putErrmsg("I_am_here lacks module states count.", NULL);
			return;
		}

		memcpy((char *) &u4, msg->supplement, 4);
		u4 = ntohl(u4);		/*	Module states count.	*/
		bytesRemaining = msg->supplementLength - 4;
		cursor = msg->supplement + 4;
		while (u4 > 0)
		{
			if (bytesRemaining < 4)
			{
				putErrmsg("I_am_here module ID incomplete.",
						NULL);
				return;
			}

			memcpy((char *) &unitNbr, cursor, 2);
			unitNbr = ntohs(unitNbr);
			moduleNbr = (unsigned char) *(cursor + 2);
			roleNbr = (unsigned char) *(cursor + 3);
			bytesRemaining -= 4;
			cursor += 4;
			if (roleNbr < 1 || roleNbr > MaxRoleNbr
					|| unitNbr > MaxUnitNbr
					|| moduleNbr < 1 || moduleNbr > MaxModuleNbr)
			{
				putErrmsg("I_am_here module ID invalid.", NULL);
				return;
			}

			if (noteModule(sap, roleNbr, unitNbr, moduleNbr, msg,
					&bytesRemaining, &cursor) < 0)
			{
				putErrmsg("Failed handling I_am_here.", NULL);
				return;
			}

			unit = sap->venture->units[unitNbr];
			module = unit->cell->modules[moduleNbr];
			if (processDeclaration(sap, module, bytesRemaining,
					cursor, 0))
			{
				return;
			}
#if 0
			/*	Tell this module about any subscriptions
			 *	and invitations issued so far.		*/
	
			sendDeclaration(sap, module);
#endif
			u4--;
		}

		return;

	case declaration:
		if (parseModuleId(msg->memo, &roleNbr, &unitNbr, &moduleNbr) < 0)
		{
			putErrmsg("declaration memo field invalid.", NULL);
			return;
		}

		unit = sap->venture->units[unitNbr];
		module = unit->cell->modules[moduleNbr];
		processDeclaration(sap, module, msg->supplementLength,
				msg->supplement, 0);
		return;

	case subscribe:
		if (parseModuleId(msg->memo, &roleNbr, &unitNbr, &moduleNbr) < 0)
		{
			putErrmsg("subscribe memo field invalid.", NULL);
			return;
		}

		unit = sap->venture->units[unitNbr];
		module = unit->cell->modules[moduleNbr];
		if (module->role == NULL)
		{
			putErrmsg("subscribe invalid; unknown module.",
					itoa(moduleNbr));
			return;
		}

		bytesRemaining = msg->supplementLength;
		cursor = msg->supplement;
//fprintf(stderr, "Parsing new subscription with %d bytes remaining.\n", bytesRemaining);
		if (parseAssertion(sap, module, &bytesRemaining, &cursor,
				SUBSCRIPTION, 0) < 0)
		{
			putErrmsg("Error parsing subscription.", NULL);
		}

		return;

	case unsubscribe:
		if (msg->supplementLength < 7)
		{
			putErrmsg("unsubscribe lacks cancellation.", NULL);
			return;
		}

		if (parseModuleId(msg->memo, &roleNbr, &unitNbr, &moduleNbr) < 0)
		{
			putErrmsg("unsubscribe memo field invalid.", NULL);
			return;
		}

		unit = sap->venture->units[unitNbr];
		module = unit->cell->modules[moduleNbr];
		if (module->role == NULL)
		{
			putErrmsg("unsubscribe invalid; unknown module.",
					itoa(moduleNbr));
			return;
		}

		memcpy((char *) &i2, msg->supplement, 2);
		i2 = ntohs(i2);
		subjectNbr = i2;
		domainContinuumNbr = ((*(msg->supplement + 2) & 0x7f) << 8)
				+ *(msg->supplement + 3);
		memcpy((char *) &u2, msg->supplement + 4, 2);
		u2 = ntohs(u2);
		domainUnitNbr = u2;
		domainRoleNbr = *(msg->supplement + 6);
		if (processCancellation(sap, module, subjectNbr, domainRoleNbr,
			domainContinuumNbr, domainUnitNbr, SUBSCRIPTION) < 0)
		{
			putErrmsg("Error handling unsubscription.", NULL);
		}

		return;

	case I_am_stopping:
		if (parseModuleId(msg->memo, &roleNbr, &unitNbr, &moduleNbr) < 0)
		{
			putErrmsg("I_am_stopping memo field invalid.", NULL);
			return;
		}

		unit = sap->venture->units[unitNbr];
		module = unit->cell->modules[moduleNbr];

		/*	Notify the application that all of this module's
		 *	subscriptions and invitations are canceled.	*/

		if (cancelUnconfirmedAssertions(sap, module) < 0)
		{
			putErrmsg("Error canceling assertions.", NULL);
			return;
		}

		/*	Destroy the module's XmitRules (subscriptions
		 *	and invitations) and erase the module itself.	*/

		if (module->role)
		{
			forgetModule(module);
		}

		/*	Notify the application that the module is gone.	*/

		enqueueNotice(sap, AmsRegistrationState, AmsStateEnds,
			unitNbr, moduleNbr, roleNbr, 0, 0, 0, 0, 0, 0, 0);
		return;

	case cell_status:
		unitNbr = (((unsigned int) msg->memo) >> 8) & 0x0000ffff;
		if (unitNbr > MaxUnitNbr)
		{
			putErrmsg("cell_status memo field invalid.", NULL);
			return;
		}

		unit = sap->venture->units[unitNbr];

		/*	First, flag for implied unregistrations.	*/

		moduleCount = (unsigned char) (msg->supplement[0]);
		for (i = 1; i <= moduleCount; i++)
		{
			moduleNbr = (unsigned char) (msg->supplement[i]);
			module = unit->cell->modules[moduleNbr];
			if (module->role == NULL)	/*	Unknown module.	*/
			{
				/*	Nothing to confirm.		*/

				continue;
			}

			module->confirmed = 1;	/*	Still there.	*/
		}

		/*	Now update array of modules in cell.		*/

		moduleCount = 0;
		for (moduleNbr = 1; moduleNbr < MaxModuleNbr; moduleNbr++)
		{
			module = unit->cell->modules[moduleNbr];
			if (module->role == NULL)
			{
				continue;
			}

			if (module->confirmed)	/*	Still here.	*/
			{
				module->confirmed = 0;
			}
			else	/*	Implied unregistration.		*/
			{
				roleNbr = module->role->nbr;
				if (cancelUnconfirmedAssertions(sap, module) < 0)
				{
					putErrmsg("Error canceling assertions.",
							NULL);
					return;
				}

				forgetModule(module);

				/*	Notify application.		*/

				enqueueNotice(sap, AmsRegistrationState,
					AmsStateEnds, unitNbr, moduleNbr,
					roleNbr, 0, 0, 0, 0, 0, 0, 0);
			}
		}

		if (unitNbr == sap->unit->nbr)
		{
			/*	cell_status from own registrar;
			 *	respond with own module_status.		*/

			sendModuleStatus(sap, &sap->unit->cell->mamsEndpoint,
					module_status);
		}

		return;

	case module_status:
		if (msg->supplementLength < 4)
		{
			putErrmsg("module_status lacks module states count.", NULL);
			return;
		}

		memcpy((char *) &u4, msg->supplement, 4);
		u4 = ntohl(u4);		/*	Module states count.	*/
		bytesRemaining = msg->supplementLength - 4;
		cursor = msg->supplement + 4;
		while (u4 > 0)
		{
			if (bytesRemaining < 4)
			{
				putErrmsg("module_status module ID incomplete.",
						NULL);
				return;
			}

			memcpy((char *) &unitNbr, cursor, 2);
			unitNbr = ntohs(unitNbr);
			moduleNbr = (unsigned char) *(cursor + 2);
			roleNbr = (unsigned char) *(cursor + 3);
			bytesRemaining -= 4;
			cursor += 4;
			if (roleNbr < 1 || roleNbr > MaxRoleNbr
					|| unitNbr > MaxUnitNbr
					|| moduleNbr < 1 || moduleNbr < MaxModuleNbr)
			{
				putErrmsg("module_status module ID invalid.", NULL);
				return;
			}

			if (noteModule(sap, roleNbr, unitNbr, moduleNbr, msg,
					&bytesRemaining, &cursor) < 0)
			{
				putErrmsg("Failed handling module_status.", NULL);
				return;
			}

			unit = sap->venture->units[unitNbr];
			module = unit->cell->modules[moduleNbr];
			if (processDeclaration(sap, module, bytesRemaining,
					cursor, RULE_CONFIRMED))
			{
				return;
			}

			if (cancelUnconfirmedAssertions(sap, module) < 0)
			{
				putErrmsg("Error canceling unconfirmed \
assertions.", NULL);
				return;
			}

			u4--;
		}

		return;

	case invite:
		if (parseModuleId(msg->memo, &roleNbr, &unitNbr, &moduleNbr) < 0)
		{
			putErrmsg("invite memo field invalid.", NULL);
			return;
		}

		unit = sap->venture->units[unitNbr];
		module = unit->cell->modules[moduleNbr];
		if (module->role == NULL)
		{
			putErrmsg("invite invalid; unknown module.",
					itoa(moduleNbr));
			return;
		}

		bytesRemaining = msg->supplementLength;
		cursor = msg->supplement;
//fprintf(stderr, "Parsing new invitation with %d bytes remaining.\n", bytesRemaining);
		if (parseAssertion(sap, module, &bytesRemaining, &cursor,
				INVITATION, 0) < 0)
		{
			putErrmsg("Error parsing invitation.", NULL);
		}

		return;

	case disinvite:
		if (msg->supplementLength < 7)
		{
			putErrmsg("disinvite lacks cancellation.", NULL);
			return;
		}

		if (parseModuleId(msg->memo, &roleNbr, &unitNbr, &moduleNbr) < 0)
		{
			putErrmsg("disinvite memo field invalid.", NULL);
			return;
		}

		unit = sap->venture->units[unitNbr];
		module = unit->cell->modules[moduleNbr];
		if (module->role == NULL)
		{
			putErrmsg("disinvite invalid; unknown module.",
					itoa(moduleNbr));
			return;
		}

		memcpy((char *) &i2, msg->supplement, 2);
		i2 = ntohs(i2);
		subjectNbr = i2;
		domainContinuumNbr = ((*(msg->supplement + 2) & 0x7f) << 8)
				+ *(msg->supplement + 3);
		memcpy((char *) &u2, msg->supplement + 4, 2);
		u2 = ntohs(u2);
		domainUnitNbr = u2;
		domainRoleNbr = *(msg->supplement + 6);
		if (processCancellation(sap, module, subjectNbr, domainRoleNbr,
			domainContinuumNbr, domainUnitNbr, INVITATION) < 0)
		{
			putErrmsg("Error handling disinvitation.", NULL);
		}

		return;

	default:		/*	Inapplicable message; ignore.	*/
		return;
	}
}

/*	*	*	MAMS message issuance functions	*	*	*/

static int	process_cell_spec(AmsSAP *sap, MamsMsg *msg)
{
	int	unitNbr;
	char	*cursor;
	int	bytesRemaining;
	char	*ept;
	Unit	*unit;
	Cell	*cell;
	int	eptLength;

	if (msg->supplementLength < 3)
	{
		putErrmsg("Cell spec lacks endpoint name.", NULL);
		return 0;
	}

	unitNbr = (((unsigned char) (msg->supplement[0])) << 8) +
			((unsigned char) (msg->supplement[1]));
	if (unitNbr != sap->unit->nbr)
	{
		putErrmsg("Got spec for foreign cell.", itoa(unitNbr));
		return 0;
	}

	/*	Cell spec for own registrar.			*/

	cursor = msg->supplement + 2;
	bytesRemaining = msg->supplementLength - 2;
	ept = parseString(&cursor, &bytesRemaining, &eptLength);
	if (ept == NULL)
	{
		putErrmsg("Invalid endpoint name in cell spec.", ept);
		return 0;
	}

	if (eptLength == 0)
	{
		writeMemoNote("[?] No registrar for this cell", ept);
		return 0;
	}

	unit = sap->venture->units[unitNbr];
	cell = unit->cell;
	if (constructMamsEndpoint(&(cell->mamsEndpoint), eptLength, ept) < 0)
	{
		clearMamsEndpoint(&(cell->mamsEndpoint));
		putErrmsg("Can't load spec for cell.", NULL);
		return -1;
	}

	sap->heartbeatsMissed = 0;
	return 0;
}

static int	locateRegistrar(AmsSAP *sap)
{
	long		queryNbr;
	MamsEndpoint	*ep;
	char		*ept;
	int		eptLen;
	LystElt		elt;
	AmsEvt		*evt;
	MamsMsg		*msg;
	int		result;

	if (sap->csEndpoint == NULL)
	{
		if (lyst_length(mib->csEndpoints) == 0)
		{
			putErrmsg("Configuration server endpoints list empty.",
					NULL);
			return -1;
		}

		if (sap->csEndpointElt == NULL)
		{
			sap->csEndpointElt = lyst_first(mib->csEndpoints);
		}
		else
		{
			sap->csEndpointElt = lyst_next(sap->csEndpointElt);
			if (sap->csEndpointElt == NULL)
			{
				sap->csEndpointElt =
						lyst_first(mib->csEndpoints);
			}
		}

		ep = (MamsEndpoint *) lyst_data(sap->csEndpointElt);
	}
	else
	{
		ep = sap->csEndpoint;
	}

	ept = sap->mamsTsif.ept;	/*	Own MAMS endpoint name.	*/
	eptLen = strlen(ept) + 1;
	queryNbr = time(NULL);
	lyst_compare_set(sap->mamsEvents, (LystCompareFn) queryNbr);
	result = sendMamsMsg(ep, &(sap->mamsTsif), registrar_query, queryNbr,
			eptLen, ept);
	if (result < 0)
	{
		putErrmsg("Module can't query configuration server.", NULL);
		return -1;
	}

	/*	All AMS events other than response to this message
	 *	(identified by queryNbr) should be ignored for now;
	 *	see enqueueAmsEvent.					*/

	while (1)
	{
		UNLOCK_MIB;
		result = llcv_wait(sap->mamsEventsCV, llcv_reply_received,
				N2_INTERVAL * 1000000);
		LOCK_MIB;
		if (result < 0)
		{
			if (errno == ETIMEDOUT)
			{
				sap->csEndpoint = NULL;
				writeMemo("[?] No config server response.");
				lyst_compare_set(sap->mamsEvents, NULL);
				return 0;
			}

			/*	Unrecoverable failure.			*/

			putErrmsg("Registrar query attempt failed.", NULL);
			return -1;
		}

		/*	A response message has arrived.			*/

		llcv_lock(sap->mamsEventsCV);
		elt = lyst_first(sap->mamsEvents);
		if (elt == NULL)	/*	Interrupted; try again.	*/
		{
			llcv_unlock(sap->mamsEventsCV);
			continue;
		}

		evt = (AmsEvent) lyst_data_set(elt, NULL);
		lyst_delete(elt);
		llcv_unlock(sap->mamsEventsCV);
		switch (evt->type)
		{
		case MAMS_MSG_EVT:
			msg = (MamsMsg *) (evt->value);
			switch (msg->type)
			{
			case registrar_unknown:
				writeMemo("[?] No registrar for this cell.");
				lyst_compare_set(sap->mamsEvents, NULL);
				return 0;

			case cell_spec:
				result = process_cell_spec(sap, msg);
				recycleEvent(evt);
				if (result < 0)
				{
					putErrmsg("Can't handle cell spec.",
							NULL);
					return -1;
				}

				if (sap->rsEndpoint->ept)
				{
				/*	Have located the registrar
				 *	(and, in so doing, a valid
				 *	configuration server).		*/

					sap->csEndpoint = ep;
				}

				lyst_compare_set(sap->mamsEvents, NULL);
				return 0;

			default:
				/*	Stray message; ignore it and
				 *	try again.			*/

				recycleEvent(evt);
				continue;
			}

		case CRASH_EVT:
			putErrmsg("Can't locate registrar.", evt->value);
			recycleEvent(evt);
			return -1;

		default:		/*	Stray event; ignore.	*/
			recycleEvent(evt);
			continue;	/*	Try again.		*/
		}
	}
}

static int	reconnectToRegistrar(AmsSAP *sap)
{
	int		moduleCount = 0;
	int		i;
	unsigned char	modules[MAX_NODE_NBR];
	int		contactSummaryLength;
	int		declarationLength;
	Module		*module;
	char		*supplement;
	int		supplementLength;
	char		*cursor;
	long		queryNbr;
	int		result;
	LystElt		elt;
	AmsEvt		*evt;
	MamsMsg		*msg;

	/*	Build cell status structure to enable reconnect.	*/

	for (i = 1; i <= MaxModuleNbr; i++)
	{
		if ((module = sap->unit->cell->modules[i])->role != NULL)
		{
			modules[moduleCount] = i;
			moduleCount++;
		}
	}

	contactSummaryLength = getContactSummaryLength(sap);
	declarationLength = getDeclarationLength(sap);
	supplementLength = 4		/*	Own unit, module, role.	*/
		+ contactSummaryLength	/*	Own contact summary.	*/
		+ declarationLength	/*	Own declaration.	*/
		+ 1			/*	Length of modules array.	*/
		+ moduleCount;		/*	Array of modules.		*/
	supplement = MTAKE(supplementLength);
	if (supplement == NULL)
	{
		putErrmsg(NoMemoryMemo, NULL);
		return -1;
	}

	cursor = supplement;
	*cursor = (sap->unit->nbr >> 8) & 0xff;
	cursor++;
	*cursor = sap->unit->nbr & 0xff;
	cursor++;
	*cursor = sap->moduleNbr & 0xff;
	cursor++;
	*cursor = sap->role->nbr & 0xff;
	cursor++;
	loadContactSummary(sap, cursor, supplementLength - 4);
	cursor += contactSummaryLength;
	loadDeclaration(sap, cursor);
	cursor += declarationLength;
	*cursor = moduleCount;
	cursor++;
	memcpy(cursor, modules, moduleCount);

	/*	Now send reconnect message.				*/

	queryNbr = time(NULL);
	lyst_compare_set(sap->mamsEvents, (LystCompareFn) queryNbr);
	result = sendMamsMsg(sap->rsEndpoint, &(sap->mamsTsif), reconnect,
			queryNbr, supplementLength, supplement);
	MRELEASE(supplement);
	if (result < 0)
	{
		putErrmsg("Failed sending reconnect msg.", NULL);
		return -1;
	}

	/*	All AMS events other than response to this message
	 *	(identified by queryNbr) should be ignored for now;
	 *	see enqueueAmsEvent.  Wait up to N5_INTERVAL seconds
	 *	for the registrar to get itself reoriented.		*/

	while (1)
	{
		UNLOCK_MIB;
		result = llcv_wait(sap->mamsEventsCV, llcv_reply_received,
				N5_INTERVAL * 1000000);
		if (result < 0)
		{
			if (errno == ETIMEDOUT)
			{
				writeMemo("[?] No registrar response.");
				lyst_compare_set(sap->mamsEvents, NULL);
				return 0;
			}

			/*	Unrecoverable failure.			*/

			putErrmsg("Reconnect attempt failed.", NULL);
			return -1;
		}

		/*	A response message has arrived.			*/

		LOCK_MIB;
		llcv_lock(sap->mamsEventsCV);
		elt = lyst_first(sap->mamsEvents);
		if (elt == NULL)	/*	Interrupted; try again.	*/
		{
			llcv_unlock(sap->mamsEventsCV);
			continue;
		}

		evt = (AmsEvent) lyst_data_set(elt, NULL);
		lyst_delete(elt);
		llcv_unlock(sap->mamsEventsCV);
		switch (evt->type)
		{
		case MAMS_MSG_EVT:
			msg = (MamsMsg *) (evt->value);
			switch (msg->type)
			{
			case you_are_dead:
				recycleEvent(evt);
				if (enqueueAmsCrash(sap,
					"Killed by registrar: reconnect.") < 0)
				{
					putErrmsg(NoMemoryMemo, NULL);
				}

				UNLOCK_MIB;
				lyst_compare_set(sap->mamsEvents, NULL);
				return 0;

			case reconnected:
				sap->heartbeatsMissed = 0;
				recycleEvent(evt);
				UNLOCK_MIB;
				lyst_compare_set(sap->mamsEvents, NULL);
				return 0;

			default:
				/*	Stray message; ignore it and
				 *	try again.			*/

				recycleEvent(evt);
				continue;
			}

		case CRASH_EVT:
			putErrmsg("Can't reconnect to registrar.", evt->value);
			recycleEvent(evt);
			UNLOCK_MIB;
			return -1;

		default:		/*	Stray message; ignore.	*/
			recycleEvent(evt);
			continue;	/*	Try again.		*/
		}
	}
}

static int	sendMsgToRegistrar(AmsSAP *sap, AmsEvt *evt)
{
	int	result;
	MamsMsg	*msg = (MamsMsg *) (evt->value);

	/*	If registrar is unknown:
	 *		If configuration server is known:
	 *			Send registrar query to configuration server.
	 *			If no answer comes back
	 *				Configuration server is unknown.
	 *				Give up for now.
	 *		Else
	 *			Send registrar query to a candidate
	 *					configuration server.
	 *			If an answer comes back
	 *				Configuration server is known.
	 *			Else
	 *				Give up for now.
	 *		If answer from CS identified the registrar
	 *			Registrar is known.
	 *		Else
	 *			Give up for now.
	 *		Send reconnect to registrar.
	 *		If refused or no answer
	 *			Registrar is unknown.
	 *			Give up for now.
	 *	Send message to known registrar.			*/

	if (sap->rsEndpoint->ept == NULL)	/*	Lost registrar.	*/
	{
		if (locateRegistrar(sap) < 0)
		{
			putErrmsg("Can't locate registrar.", NULL);
			return -1;
		}

		/*	Found it?					*/

		if (sap->rsEndpoint->ept == NULL)
		{
			return 0;
		}

		if (reconnectToRegistrar(sap) < 0)
		{
			putErrmsg("Can't reconnect to registrar.", NULL);
			return -1;
		}

		/*	Reconnected?					*/

		if (sap->heartbeatsMissed > 0)
		{
			clearMamsEndpoint(sap->rsEndpoint);
			return 0;
		}
	}

	if (msg->type == I_am_stopping)
	{
		/*	If stopping, we sleep for .5 second to give
	 	*	all message transmissions currently in
	 	*	progress enough time to conclude.		*/

		microsnooze(500000);
	}

	result = sendMamsMsg(sap->rsEndpoint, &(sap->mamsTsif), msg->type,
			msg->memo, msg->supplementLength, msg->supplement);
	if (msg->supplement)
	{
		MRELEASE(msg->supplement);
	}

	return result;
}

/*	*	*	Registration functions	*	*	*	*/

static void	process_rejection(AmsSAP *sap, MamsMsg *msg)
{
	int	reasonCode;
	char	*reasonString;

	if (msg->supplementLength < 1)
	{
		reasonString = "reason omitted";
	}
	else
	{
		reasonCode = (int) *(msg->supplement);
		if (reasonCode < 0 || reasonCode > 4)
		{
			reasonString = "reason unknown";
		}
		else
		{
			reasonString = rejectionMemos[reasonCode];
		}
	}

	writeMemoNote("[?] Registration refused", reasonString);
}

static int	process_you_are_in(AmsSAP *sap, MamsMsg *msg)
{
	Module	*module;
	char	*ept = sap->mamsTsif.ept;
	int	eptLength = strlen(ept) + 1;

	if (msg->supplementLength < 1)
	{
		putErrmsg("Got truncated you_are_in.", NULL);
		return 0;		/*	Not unrecoverable.	*/
	}

	sap->moduleNbr = *(msg->supplement);
	module = sap->unit->cell->modules[sap->moduleNbr];
	if (rememberModule(module, sap->role, eptLength, ept) < 0)
	{
		putErrmsg("Can't retain module announcement.", NULL);
		return -1;
	}

	return (enqueueAmsStubEvent(sap, ACCEPTED_EVT, 1));
}

static int	getModuleNbr(AmsSAP *sap)
{
	int	supplementLength;
	char	*supplement;
	long	queryNbr;
	LystElt	elt;
	AmsEvt	*evt;
	MamsMsg	*msg;
	int	result;

	/*	First get registrar MAMS endpoint if necessary.		*/

	if (sap->rsEndpoint->ept == NULL)
	{
		if (locateRegistrar(sap) < 0)
		{
			putErrmsg("Can't locate registrar.", NULL);
			return -1;
		}

		/*	Was query successful?				*/

		if (sap->rsEndpoint->ept == NULL)
		{
			return 0;
		}
	}

	/*	Now send module_registration message.			*/

	supplementLength = getContactSummaryLength(sap);
	if (supplementLength > 65535)
	{
		putErrmsg("Contact summary too long.", itoa(supplementLength));
		return -1;
	}

	supplement = MTAKE(supplementLength);
	if (supplement == NULL)
	{
		putErrmsg(NoMemoryMemo, NULL);
		return -1;
	}

	loadContactSummary(sap, supplement, supplementLength);
	queryNbr = time(NULL);
	lyst_compare_set(sap->mamsEvents, (LystCompareFn) queryNbr);
	result = sendMamsMsg(sap->rsEndpoint, &(sap->mamsTsif),
		module_registration, queryNbr, supplementLength, supplement);
	MRELEASE(supplement);
       	if (result < 0)
	{
		putErrmsg("Failed sending module_registration.", NULL);
		return -1;
	}

	/*	All AMS events other than response to this message
	 *	(identified by queryNbr) should be ignored for now;
	 *	see enqueueAmsEvent.					*/

	while (1)			/*	Loop past anomalies.	*/
	{
		UNLOCK_MIB;
		result = llcv_wait(sap->mamsEventsCV, llcv_reply_received,
				N2_INTERVAL * 1000000);
		if (result < 0)
		{
			if (errno == ETIMEDOUT)
			{
				/*	Registration may have happened
				 *	and response may simply have
				 *	gotten dropped.  Safest way
				 *	ahead is to abort registration
				 *	and let the application decide
				 *	whether or not to try again
				 *	after getting a new MAMS ept.	*/

				clearMamsEndpoint(sap->rsEndpoint);
				writeMemo("[?] No registrar response.");
				lyst_compare_set(sap->mamsEvents, NULL);
				return 0;
			}

			/*	Unrecoverable failure.			*/

			putErrmsg("Registration attempt failed.", NULL);
			return -1;
		}

		/*	A response message has arrived.			*/

		LOCK_MIB;
		llcv_lock(sap->mamsEventsCV);
		elt = lyst_first(sap->mamsEvents);
		if (elt == NULL)	/*	Interrupted; try again.	*/
		{
			llcv_unlock(sap->mamsEventsCV);
			continue;
		}

		evt = (AmsEvent) lyst_data_set(elt, NULL);
		lyst_delete(elt);
		llcv_unlock(sap->mamsEventsCV);
		switch (evt->type)
		{
		case MAMS_MSG_EVT:
			msg = (MamsMsg *) (evt->value);
			switch (msg->type)
			{
			case rejection:
				process_rejection(sap, msg);
				recycleEvent(evt);
				UNLOCK_MIB;
				lyst_compare_set(sap->mamsEvents, NULL);
				return 0;

			case you_are_in:

				/*	Post event to main thread, to
				 *	terminate ams_register2.	*/

				result = process_you_are_in(sap, msg);
				recycleEvent(evt);
				UNLOCK_MIB;
				if (result < 0)
				{
					putErrmsg("Can't handle acceptance.",
						NULL);
					return -1;
				}

				lyst_compare_set(sap->mamsEvents, NULL);
				return 0;

			default:
				/*	Stray message; ignore it and
				 *	try again.			*/

				recycleEvent(evt);
				continue;
			}

		case CRASH_EVT:
			putErrmsg("Can't register.", evt->value);
			recycleEvent(evt);
			UNLOCK_MIB;
			return -1;	/*	Unrecoverable failure.	*/

		default:		/*	Stray event; ignore.	*/
			recycleEvent(evt);
			continue;	/*	Try again.		*/
		}
	}
}

/*	*	*	Background thread main functions	*	*/

static void	*mamsMain(void *parm)
{
	AmsSAP		*sap = (AmsSAP *) parm;
	sigset_t	signals;
	LystElt		elt;
	AmsEvt		*evt;
	int		result;

	sigfillset(&signals);
	pthread_sigmask(SIG_BLOCK, &signals, NULL);

	/*	MAMS thread starts off in Unregistered state and
	 *	stays there until registration is complete.  Then
	 *	it enters Registered state, in which it responds to
	 *	MAMS messages from the registrar and other modules.
	 *	Upon unregistration, erasure of the SAP causes the
	 *	MAMS thread to terminate.				*/

	while (1)	/*	Registration event loop.		*/
	{
		LOCK_MIB;
		result = getModuleNbr(sap);
		UNLOCK_MIB;
		if (result < 0)		/*	Unrecoverable failure.	*/
		{
			putErrmsg("Can't register module.", NULL);
			if (enqueueAmsCrash(sap, "Can't register.") < 0)
			{
				putErrmsg(NoMemoryMemo, NULL);
			}

			return NULL;
		}

		/*	Was registration attempt successful?		*/

		if (sap->moduleNbr != 0)
		{
			break;
		}

		/*	Not registered yet.  Wait a while, try again.	*/

		microsnooze(N1_INTERVAL * 1000000);
	}

	/*	The MAMS thread has now entered Registered state.	*/

	while (1)	/*	Operation event loop.			*/
	{
		if (llcv_wait(sap->mamsEventsCV, llcv_lyst_not_empty,
				LLCV_BLOCKING) < 0)
		{
			break;		/*	Out of loop.		*/
		}

		llcv_lock(sap->mamsEventsCV);
		elt = lyst_first(sap->mamsEvents);
		if (elt == NULL)	/*	Equivalent to failure.	*/
		{
			llcv_unlock(sap->mamsEventsCV);
			break;		/*	Out of loop.		*/
		}

		evt = (AmsEvent) lyst_data_set(elt, NULL);
		lyst_delete(elt);
		llcv_unlock(sap->mamsEventsCV);
		switch (evt->type)
		{
		case CRASH_EVT:
			/*	Forward the event to the application
			 *	event handling thread.			*/

			if (enqueueAmsCrash(sap, evt->value) < 0)
			{
				putErrmsg(NoMemoryMemo, NULL);
				recycleEvent(evt);
				break;	/*	Out of switch.		*/
			}

			/*	Turn off all AAMS functionality and
			 *	MAMS message handling pending the
			 *	prime thread's unregistration.		*/

			sap->state = AmsSapCrashed;
			recycleEvent(evt);
			continue;

		case MAMS_MSG_EVT:
			if (sap->state == AmsSapOpen)
			{
				LOCK_MIB;
				processMamsMsg(sap, evt);
				UNLOCK_MIB;
			}

			recycleEvent(evt);
			continue;

		case MSG_TO_SEND_EVT:
			LOCK_MIB;
			result = sendMsgToRegistrar(sap, evt);
			UNLOCK_MIB;
			if (result < 0)
			{
				putErrmsg("MAMS service failed.", NULL);
			}

			recycleEvent(evt);
			continue;

		case SHUTDOWN_EVT:
			/*	At this point all application message
			 *	transmission has most likely concluded
			 *	and all enqueued MAMS activity at time
			 *	of unregistration has been handled, so
			 *	we can finish closing.			*/

			if (enqueueAmsStubEvent(sap, ENDED_EVT, 1) < 0)
			{
				putErrmsg(NoMemoryMemo, NULL);
			}

			recycleEvent(evt);
			break;		/*	Out of switch.		*/

		default:		/*	Inapplicable event.	*/
			recycleEvent(evt);
			continue;	/*	Ignore the event.	*/
		}

		break;			/*	Out of loop.		*/
	}

	return NULL;
}

static void	*heartbeatMain(void *parm)
{
	AmsSAP		*sap = (AmsSAP *) parm;
	pthread_mutex_t	mutex;
	pthread_cond_t	cv;
	sigset_t	signals;
	int		result;
	struct timeval	workTime;
	struct timespec	deadline;

	if (pthread_mutex_init(&mutex, NULL))
	{
		putSysErrmsg("can't start heartbeat, mutex init failed", NULL);
		return NULL;
	}

	if (pthread_cond_init(&cv, NULL))
	{
		pthread_mutex_destroy(&mutex);
		putSysErrmsg("can't start heartbeat, cond init failed", NULL);
		return NULL;
	}

	sigfillset(&signals);
	pthread_sigmask(SIG_BLOCK, &signals, NULL);
	while (1)
	{
		getCurrentTime(&workTime);
		deadline.tv_sec = workTime.tv_sec + N4_INTERVAL;
		deadline.tv_nsec = workTime.tv_usec * 1000;
		pthread_mutex_lock(&mutex);
		result = pthread_cond_timedwait(&cv, &mutex, &deadline);
		pthread_mutex_unlock(&mutex);
		if (result)
		{
			errno = result;
			if (errno != ETIMEDOUT)
			{
				putSysErrmsg("heartbeat failure", NULL);
				break;
			}
		}

		if (sap->state != AmsSapOpen)
		{
			continue;
		}

		LOCK_MIB;
		if (sap->heartbeatsMissed == 3)
		{
			clearMamsEndpoint(sap->rsEndpoint);
		}

		result = enqueueMsgToRegistrar(sap, heartbeat, sap->moduleNbr,
				0, NULL);
		sap->heartbeatsMissed++;
		UNLOCK_MIB;
		if (result < 0)
		{
			break;
		}
	}

	writeErrmsgMemos();
	writeMemo("[i] Heartbeat thread ended.");
	return NULL;
}

/*	*	*	AMS API functions	*	*	*	*/

static int	ams_register2(char *applicationName, char *authorityName,
			char *unitName, char *roleName, char *tsorder,
			AmsModule *module)
{
	int		length;
	int		result;
	AmsSAP		*sap;
	int		i;
	Venture		*venture;
	char		ventureName[MAX_APP_NAME + 2 + MAX_AUTH_NAME + 1];
	Unit		*unit = NULL;
	AppRole		*role = NULL;
	AmsInterface	*tsif;
	MamsInterface	*mtsif;
	LystElt		elt;
	AmsEpspec	*amses;
	int		j;
	char		*tspos;
	DeliveryVector	*vector;
	AmsEvt		*evt;

	if (applicationName == NULL || authorityName == NULL
	|| unitName == NULL || roleName == NULL || module == NULL
	|| (length = strlen(applicationName)) == 0 || length > MAX_APP_NAME
	|| (length = strlen(authorityName)) == 0 || length > MAX_AUTH_NAME
	|| (length = strlen(unitName)) > MAX_UNIT_NAME
	|| (length = strlen(roleName)) == 0 || length > MAX_ROLE_NAME)
	{
		putErrmsg(BadParmsMemo, NULL);
		errno = EINVAL;
		return -1;
	}

	/*	Start building SAP structure.				*/

	sap = (AmsSAP *) MTAKE(sizeof(AmsSAP));
	if (sap == NULL)
	{
		putErrmsg(NoMemoryMemo, NULL);
		return -1;
	}

	*module = sap;
	memset((char *) sap, 0, sizeof(AmsSAP));
	sap->state = AmsSapClosed;
	sap->primeThread = pthread_self();
	sap->eventMgr = sap->primeThread;
	sap->authorizedEventMgr = sap->primeThread;

	/*	Validate registration parameters.			*/

	for (i = 1; i <= MaxVentureNbr; i++)
	{
		venture = mib->ventures[i];
		if (venture == NULL)	/*	Number not in use.	*/
		{
			continue;
		}

		if (strcmp(venture->app->name, applicationName) == 0
		&& strcmp(venture->authorityName, authorityName) == 0)
		{
			break;
		}
	}

	if (i > MaxVentureNbr)
	{
		isprintf(ventureName, sizeof ventureName, "%s(%s)",
				applicationName, authorityName);
		putErrmsg("Can't register: no such message space.",
				ventureName);
		errno = EINVAL;
		return -1;
	}

	sap->venture = venture;
	for (i = 0; i <= MaxUnitNbr; i++)
	{
		unit = venture->units[i];
		if (unit == NULL)	/*	Number not in use.	*/
		{
			continue;
		}

		if (strcmp(unit->name, unitName) == 0)
		{
			break;
		}
	}

	if (i > MaxUnitNbr)
	{
		putErrmsg("Can't register: no such unit in message space.",
				unitName);
		errno = EINVAL;
		return -1;
	}

	sap->unit = unit;
	sap->rsEndpoint = &(unit->cell->mamsEndpoint);
	for (i = 1; i <= MaxRoleNbr; i++)
	{
		role = venture->roles[i];
		if (role == NULL)	/*	Number not in use.	*/
		{
			continue;
		}

		if (strcmp(role->name, roleName) == 0)
		{
			break;
		}
	}

	if (i > MaxRoleNbr)
	{
		putErrmsg("Can't register: role name invalid for application.",
				roleName);
		errno = EINVAL;
		return -1;
	}

	sap->role = role;

	/*	Initialize module state data structures.  First, lists.	*/

	sap->mamsEvents = lyst_create_using(amsMemory);
	sap->amsEvents = lyst_create_using(amsMemory);
	sap->delivVectors = lyst_create_using(amsMemory);
	sap->subscriptions = lyst_create_using(amsMemory);
	sap->invitations = lyst_create_using(amsMemory);
	if (sap->mamsEvents == NULL || sap->amsEvents == NULL
	|| sap->delivVectors == NULL || sap->subscriptions == NULL
	|| sap->invitations == NULL)
	{
		putErrmsg(NoMemoryMemo, NULL);
		return -1;
	}

	lyst_delete_set(sap->mamsEvents, destroyEvent, NULL);
	lyst_delete_set(sap->amsEvents, destroyAmsEvent, NULL);
	lyst_delete_set(sap->delivVectors, destroyDeliveryVector, NULL);
	lyst_delete_set(sap->subscriptions, destroyMsgRule, NULL);
	lyst_delete_set(sap->invitations, destroyMsgRule, NULL);

	/*	Now the Llcvs.						*/

	sap->amsEventsCV = llcv_open(sap->amsEvents, &(sap->amsEventsCV_str));
	if (sap->amsEventsCV == NULL)
	{
		putErrmsg("Can't register: can't open AMS events llcv.", NULL);
		return -1;
	}

	sap->mamsEventsCV = llcv_open(sap->mamsEvents,
			&(sap->mamsEventsCV_str));
	if (sap->mamsEventsCV == NULL)
	{
		putErrmsg("Can't register: can't open MAMS events llcv.", NULL);
		return -1;
	}

	/*	Create the MAMS transport service interface.		*/

	mtsif = &(sap->mamsTsif);
	mtsif->ts = mib->pts;
	mtsif->ventureNbr = sap->venture->nbr;
	mtsif->unitNbr = sap->unit->nbr;
	mtsif->roleNbr = sap->role->nbr;
	mtsif->endpointSpec = NULL;
	mtsif->ept = NULL;
	mtsif->eventsQueue = sap->mamsEventsCV;
	if (mtsif->ts->mamsInitFn(mtsif) < 0)
	{
		putErrmsg("Can't initialize MAMS interface.", NULL);
		return -1;
	}

	if (pthread_create(&(mtsif->receiver), NULL,
			mib->pts->mamsReceiverFn, mtsif))
	{
		putSysErrmsg("Can't spawn MAMS tsif thread", NULL);
		return -1;
	}

	/*	Create all AMS transport service interfaces.		*/

	for (i = 0, elt = lyst_first(mib->amsEndpointSpecs); elt;
			i++, elt = lyst_next(elt))
	{
		amses = (AmsEpspec *) lyst_data(elt);
		if (tsorder == NULL)
		{
			j = i;
		}
		else	/*	Overriding the order in the MIB.	*/
		{
			/*	Find requested position (j) in SAP's
			 *	tsifs array for an endpoint generated
			 *	from the i'th AMS endpoint spec in
			 *	the MIB's list of AMS endpoint specs.	*/

			for (j = 0, tspos = tsorder; (*tspos); tspos++, j++)
			{
				if ((((int) *tspos) - 48) /* atoi */ == i)
				{
					break;	/*	It goes here.	*/
				}
			}

			if (*tspos == 0)	/*	Don't want it.	*/
			{
				continue;
			}
		}

		sap->transportServices[j] = amses->ts;
		sap->transportServiceCount++;
		tsif = &(sap->amsTsifs[j]);
		tsif->ts = sap->transportServices[j];
		tsif->amsSap = sap;
		tsif->eventsQueue = sap->amsEventsCV;
		if (tsif->ts->amsInitFn(tsif, amses->epspec) < 0)
		{
			putErrmsg("Can't initialize AMS interface.",
					amses->epspec);
			return -1;
		}

		if (pthread_create(&(tsif->receiver), NULL,
				tsif->ts->amsReceiverFn, tsif))
		{
			putSysErrmsg("Can't spawn tsif thread", NULL);
			return -1;
		}
	}

	/*	Construct all delivery vectors, appending transport
	 *	service endpoint names in descending order of
	 *	preference.						*/

	for (i = 0; i < sap->transportServiceCount; i++)
	{
		tsif = &(sap->amsTsifs[i]);

		/*	Append the endpoint name for this transport
		 *	service to the delivery vector for the
		 *	corresponding service mode: find the vector
		 *	with matching service mode, create new delivery
		 *	vector if necessary.				*/

		for (elt = lyst_first(sap->delivVectors); elt;
				elt = lyst_next(elt))
		{
			vector = (DeliveryVector *) lyst_data(elt);
			if (vector->diligence == tsif->diligence
			&& vector->sequence == tsif->sequence)
			{
				break;
			}
		}

		if (elt == NULL)	/*	New service mode.	*/
		{
			vector = (DeliveryVector *)
					MTAKE(sizeof(DeliveryVector));
			if (vector == NULL)
			{
				putErrmsg(NoMemoryMemo, NULL);
				return -1;
			}

			vector->nbr = lyst_length(sap->delivVectors) + 1;
			if (vector->nbr > 15)
			{
				putErrmsg("Module has > 15 delivery vectors.",
						NULL);
				return -1;
			}

			vector->diligence = tsif->diligence;
			vector->sequence = tsif->sequence;
			vector->interfaces = lyst_create_using(amsMemory);
			if (vector->interfaces == NULL)
			{
				putErrmsg(NoMemoryMemo, NULL);
				return -1;
			}

			/*	No need for deletion function on
			 *	vector->interfaces: these structures
			 *	are pointed-to by the SAP's
			 *	AmsInterfaces list as well, and are
			 *	deleted when that list is destroyed.	*/

			if (lyst_insert_last(sap->delivVectors, vector) == NULL)
			{
				putErrmsg(NoMemoryMemo, NULL);
				return -1;
			}
		}

		/*	Got matching vector; append interface for this
		 *	transport service to it.			*/

		if (lyst_length(vector->interfaces) > 15)
		{
			putErrmsg("Module's vector has > 15 interfaces.",
					itoa(vector->nbr));
			return -1;
		}

		if (lyst_insert_last(vector->interfaces, tsif) == NULL)
		{
			putErrmsg(NoMemoryMemo, NULL);
			return -1;
		}
	}

	/*	Create the auxiliary module threads: heartbeat, MAMS.	*/

	if (pthread_create(&(sap->heartbeatThread), NULL, heartbeatMain, sap)
	|| pthread_create(&(sap->mamsThread), NULL, mamsMain, sap))
	{
		putSysErrmsg("can't spawn sap auxiliary threads", NULL);
		return -1;
	}

	/*	Wait for MAMS thread to complete registration dialogue.	*/

	while (1)
	{
		UNLOCK_MIB;
		result = llcv_wait(sap->amsEventsCV, llcv_lyst_not_empty,
					LLCV_BLOCKING);
		LOCK_MIB;
		if (result < 0)
		{
			putErrmsg("Registration abandoned.", NULL);
			return -1;
		}

		llcv_lock(sap->amsEventsCV);
		elt = lyst_first(sap->amsEvents);
		if (elt == NULL)
		{
			llcv_unlock(sap->amsEventsCV);
			continue;
		}

		noteEventDequeued(sap, elt);
		evt = (AmsEvent) lyst_data_set(elt, NULL);
		lyst_delete(elt);
		llcv_unlock(sap->amsEventsCV);
		switch (evt->type)
		{
		case ACCEPTED_EVT:	/*	Okay, we're done.	*/
			recycleEvent(evt);
			break;		/*	Out of switch.		*/

		case CRASH_EVT:
			putErrmsg("Registration failed.", evt->value);
			recycleEvent(evt);
			return -1;

		default:		/*	Stray event; ignore.	*/
			recycleEvent(evt);
			continue;	/*	Try again.		*/
		}

		break;			/*	Out of loop.		*/
	}

	/*	Note: reliable delivery would be much better, but
	 *	not all implementations of AMS support that QOS.	*/

	result = ams_invite2(sap, 1, THIS_CONTINUUM, 0,
			(0 - mib->localContinuumNbr), 8, 0, AmsArrivalOrder,
			AmsBestEffort);
	if (result < 0)
	{
		putErrmsg("Can't invite RAMS enclosure messages.", NULL);
	}

	return result;
}

int	ams_register(char *mibSource, char *tsorder, char *mName, char *memory,
		unsigned mSize, char *applicationName, char *authorityName,
		char *unitName, char *roleName, AmsModule *module)
{
	int	result;

	if (initMemoryMgt(mName, memory, mSize) < 0)
	{
		return -1;
	}

	/*	Load Management Information Base as necessary.		*/

	if (mib == NULL)
	{
		result = loadMib(mibSource);
		if (result < 0 || mib == NULL)
		{
			putErrmsg("AMS can't load MIB.", mibSource);
			return -1;
		}
	}

	*module = NULL;
	LOCK_MIB;
	result = ams_register2(applicationName, authorityName, unitName,
			roleName, tsorder, module);
	UNLOCK_MIB;
	if (result == 0)		/*	Succeeded.		*/
	{
		(*module)->state = AmsSapOpen;
	}
	else
	{
		eraseSAP(*module);
	}

	return result;
}

int	ams_get_module_nbr(AmsSAP *sap)
{
	if (sap == NULL)
	{
		return -1;
	}

	return sap->moduleNbr;
}

int	ams_get_unit_nbr(AmsSAP *sap)
{
	if (sap == NULL)
	{
		return -1;
	}

	return sap->unit->nbr;
}

static int	ams_unregister2(AmsSAP *sap)
{
	int	result;
	LystElt	elt;
	AmsEvt	*evt;

	if (sap == NULL)
	{
		putErrmsg(BadParmsMemo, NULL);
		errno = EINVAL;
		return -1;
	}

	/*	Only prime thread can unregister.			*/

	if (!pthread_equal(pthread_self(), sap->primeThread))
	{
		putErrmsg("Only prime thread can unregister.", NULL);
		errno = EINVAL;
		return -1;
	}

	UNLOCK_MIB;
	ams_remove_event_mgr(sap);
	LOCK_MIB;
	result = enqueueMsgToRegistrar(sap, I_am_stopping,
		computeModuleId(sap->role->nbr, sap->unit->nbr, sap->moduleNbr),
		0, NULL);

	/*	Wait for MAMS thread to finish dealing with all
	 *	currently enqueued events, then complete shutdown.	*/

	if (enqueueMamsStubEvent(sap->mamsEventsCV, SHUTDOWN_EVT) < 0)
	{
		UNLOCK_MIB;
		putErrmsg("Crashed AMS service.", NULL);
		return 0;
	}

	while (1)
	{
		UNLOCK_MIB;
		result = llcv_wait(sap->amsEventsCV, llcv_lyst_not_empty,
					LLCV_BLOCKING);
		if (result < 0)
		{
			putErrmsg("Crashed AMS service.", NULL);
			return 0;
		}

		LOCK_MIB;
		llcv_lock(sap->amsEventsCV);
		elt = lyst_first(sap->amsEvents);
		if (elt == NULL)
		{
			llcv_unlock(sap->amsEventsCV);
			continue;
		}

		noteEventDequeued(sap, elt);
		evt = (AmsEvent) lyst_data_set(elt, NULL);
		lyst_delete(elt);
		llcv_unlock(sap->amsEventsCV);
		if (evt->type != ENDED_EVT)
		{
			recycleEvent(evt);
			continue;	/*	Ignore everything else.	*/
		}

		/*	MAMS thread has caught up; time to stop.	*/

		recycleEvent(evt);
		UNLOCK_MIB;
		break;
	}

	return 0;
}

int	ams_unregister(AmsSAP *sap)
{
	int	result;

	LOCK_MIB;
	result = ams_unregister2(sap);
	UNLOCK_MIB;
	if (result == 0)
	{
		eraseSAP(sap);
		writeMemo("[i] AMS service terminated.");
	}

	return result;
}

static int	validSap(AmsSAP *sap)
{
	int	result = 0;

	if (sap == NULL)
	{
		putErrmsg(BadParmsMemo, NULL);
		errno = EINVAL;
	}
	else if (sap->state != AmsSapOpen)
	{
		errno = EAGAIN;
	}
	else
	{
		result = 1;
	}

	return result;
}

static char	*ams_get_role_name2(AmsSAP *sap, int unitNbr, int moduleNbr)
{
	Unit	*unit;
	Module	*module;

	if (unitNbr >= 0 && unitNbr <= MaxUnitNbr
	&& moduleNbr > 0 && moduleNbr <= MaxModuleNbr)
	{
		unit = sap->venture->units[unitNbr];
		if (unit)
		{
			module = unit->cell->modules[moduleNbr];
			if (module->role)
			{
				return module->role->name;
			}
		}
	}

	return NULL;
}

char	*ams_get_role_name(AmsSAP *sap, int unitNbr, int moduleNbr)
{
	char	*result = NULL;

	if (validSap(sap))
	{
		LOCK_MIB;
		result = ams_get_role_name2(sap, unitNbr, moduleNbr);
		UNLOCK_MIB;
		if (result == NULL)
		{
			if (errno == 0) errno = EAGAIN;
		}
	}

	return result;
}

Lyst	ams_list_msgspaces(AmsSAP *sap)
{
	Lyst	msgspaces = NULL;
	int	i;
	Subject	**msgspace;
	long	msgspaceNbr;

	if (validSap(sap))
	{
		LOCK_MIB;
		msgspaces = lyst_create_using(amsMemory);
		if (msgspaces)
		{
			for (i = 0, msgspace = sap->venture->msgspaces;
					i <= MAX_CONTIN_NBR; i++, msgspace++)
			{
				if (*msgspace != NULL)
				{
					msgspaceNbr = 0 - (*msgspace)->nbr;
					if (lyst_insert_last(msgspaces,
						(void *) msgspaceNbr) == NULL)
					{
						lyst_destroy(msgspaces);
						msgspaces = NULL;
						break;	/*	Loop.	*/
					}
				}
			}
		}

		UNLOCK_MIB;
	}

	return msgspaces;
}

int	ams_continuum_is_neighbor(int continuumNbr)
{
	Continuum	*contin;

	if (continuumNbr < 1 || continuumNbr > MAX_CONTIN_NBR
	|| (contin = mib->continua[continuumNbr]) == NULL)
	{
		return 0;
	}

	return contin->isNeighbor;
}

int	ams_get_continuum_nbr()
{
	return mib->localContinuumNbr;
}

int	ams_rams_net_is_tree()
{
	return mib->ramsNetIsTree;
}

int	ams_subunit_of(AmsSAP *sap, int argUnitNbr, int refUnitNbr)
{
	int	result = 0;

	if (validSap(sap))
	{
		LOCK_MIB;
		result = subunitOf(sap, argUnitNbr, refUnitNbr);
		UNLOCK_MIB;
	}

	return result;
}

static int	qualifySender(AmsSAP *sap, XmitRule *rule)
{
	if (rule->roleNbr != ANY_ROLE	/*	Role-specific rule.	*/
	&& rule->roleNbr != sap->role->nbr)
	{
		return 0;	/*	Wrong role for this rule.	*/
	}

	if (rule->continuumNbr != ANY_CONTINUUM
	&& rule->continuumNbr != mib->localContinuumNbr)
	{
		return 0;	/*	Wrong continuum for this rule.	*/
	}

	/*	All XmitRules are unit-specific, though the specified
	 *	unit may be the root unit.				*/

	if (subunitOf(sap, sap->unit->nbr, rule->unitNbr))
	{
		return 1;
	}

	return 0;		/*	Wrong unit for this rule.	*/
}

static XmitRule	*getXmitRule(AmsSAP *sap, Lyst rules)
{
	LystElt		elt;
	XmitRule	*rule;

	for (elt = lyst_first(rules); elt; elt = lyst_next(elt))
	{
		rule = (XmitRule *) lyst_data(elt);
		if ((rule->flags & XMIT_IS_OKAY) == 0)
		{
			continue;	/*	Receiver unauthorized.	*/
		}

		if (qualifySender(sap, rule) == 0)
		{
			continue;	/*	Not a qualified sender.	*/
		}

		return rule;
	}

	return NULL;
}	

static int	receivedMsgAlready(Lyst recipients, int moduleNbr)
{
	LystElt	elt;
	long	longModuleNbr = (long) moduleNbr;
		// cast to long to avoid warnings on 64-bit

	for (elt = lyst_first(recipients); elt; elt = lyst_next(elt))
	{
		if (longModuleNbr == (long) lyst_data(elt))
		{
			return 1;
		}
	}

	return 0;
}

static int	sendToSubscribers(AmsSAP *sap, Subject *subject,
			int priority, unsigned char flowLabel, 
			unsigned char protectedBits, char *amsHeader,
			int headerLength, char *content, int contentLength,
	       		Lyst recipients)
{
	LystElt		elt;
	FanModule	*fan;
	XmitRule	*rule;
	int		result;

	for (elt = lyst_first(subject->modules); elt; elt = lyst_next(elt))
	{
		fan = (FanModule *) lyst_data(elt);
		if (subject->nbr == ALL_SUBJECTS)
		{
			if (receivedMsgAlready(recipients, fan->module->nbr))
			{
				continue;/*	Don't send 2nd copy.	*/
			}
		}

		rule = getXmitRule(sap, fan->subj->subscriptions);
		if (rule == NULL)
		{
			continue;	/*	Don't send module a copy.	*/
		}

		/*	Must send this module a copy of this message.
		 *	Supply default priority and/or flow label as
		 *	necessary, and send message to module.		*/

		if (priority)		/*	Override.		*/
		{
			amsHeader[0] = (char) (protectedBits | priority);
		}
		else			/*	Use default per rule.	*/
		{
			amsHeader[0] = (char) (protectedBits | rule->priority);
		}

		if (flowLabel == 0)
		{
			flowLabel = rule->flowLabel;
		}

		result = rule->amsEndpoint->ts->sendAmsFn(rule->amsEndpoint,
				sap, flowLabel, amsHeader, headerLength,
				content, contentLength);
		if (result < 0)
		{
			return result;
		}

		if (subject->nbr != ALL_SUBJECTS)
		{
			lyst_insert_last(recipients,
					(void *) ((long) (fan->module->nbr)));
			// cast to long to avoid warnings on 64-bit
		}
	}

	return 0;
}

static int	ams_publish2(AmsSAP *sap, int subjectNbr, int priority,
			unsigned char flowLabel, int contentLength,
			char *content, int context)
{
	Subject		*subject;
	char		amsHeader[16];
	int		headerLength = sizeof amsHeader;
	unsigned char	protectedBits;
	Lyst		recipients;

	if (subjectNbr == 0 || !subjectIsValid(sap, subjectNbr, &subject)
	|| priority < 0 || priority >= NBR_OF_PRIORITY_LEVELS
	|| contentLength < 0 || contentLength > MAX_AMS_CONTENT)
	{
		putErrmsg(BadParmsMemo, NULL);
		errno = EINVAL;
		return -1;
	}

	/*	Knowing subject, construct message header & encrypt.	*/

	constructMessage(sap, subjectNbr, priority, flowLabel, context, content,
		contentLength, (unsigned char *) amsHeader, AmsMsgUnary);
	protectedBits = amsHeader[0] & 0xf0;
	recipients = lyst_create();

	/*	Now send a copy of the message to every subscriber
	 *	that has posted at least one subscription whose domain
	 *	includes the local module.				*/

	if (sendToSubscribers(sap, subject, priority, flowLabel, protectedBits,
			amsHeader, headerLength, content, contentLength,
			recipients) < 0)
	{
		return -1;
	}

	/*	Finally, send a copy of the message to every subscriber
	 *	that has posted at least one subscription for "all
	 *	subjects".						*/

	subject = sap->venture->subjects[ALL_SUBJECTS];
	if (sendToSubscribers(sap, subject, priority, flowLabel, protectedBits,
			amsHeader, headerLength, content, contentLength,
		       	recipients) < 0)
	{
		return -1;
	}

	lyst_destroy(recipients);
	return 0;
}

int	ams_publish(AmsSAP *sap, int subjectNbr, int priority,
		unsigned char flowLabel, int contentLength, char *content,
		int context)
{
	int	result = -1;

	if (validSap(sap))
	{
		if (subjectNbr < 1)
		{
			putErrmsg(BadParmsMemo, NULL);
			errno = EINVAL;
			return -1;
		}

		LOCK_MIB;
		result = ams_publish2(sap, subjectNbr, priority, flowLabel,
				contentLength, content, context);
		UNLOCK_MIB;
		if (result < 0)
		{
			if (errno == 0) errno = EAGAIN;
		}
	}

	return result;
}

static DeliveryVector	*lookUpDeliveryVector(AmsSAP *sap, AmsSequence sequence,
				AmsDiligence diligence)
{
	LystElt		elt;
	DeliveryVector	*vector;

	for (elt = lyst_first(sap->delivVectors); elt; elt = lyst_next(elt))
	{
		vector = (DeliveryVector *) lyst_data(elt);
		if ((diligence == AmsBestEffort
			|| vector->diligence == AmsAssured)
		&& (sequence == AmsArrivalOrder
			|| vector->sequence == AmsTransmissionOrder))
		{
			return vector;
		}
	}

	return NULL;
}

static LystElt	findMsgRule(AmsSAP *sap, Lyst rules, int subjectNbr,
			int roleNbr, int continuumNbr, int unitNbr,
			LystElt *nextRule)
{
	LystElt	elt;
	MsgRule	*rule;

	/*	This function finds the MsgRule asserted for the
		specified subject, role, continuum, unit, if any.	*/

	if (nextRule) *nextRule = NULL;	/*	Default.		*/
	for (elt = lyst_first(rules); elt; elt = lyst_next(elt))
	{
		rule = (MsgRule *) lyst_data(elt);
		if (rule->subject->nbr < subjectNbr)
		{
			continue;
		}

		if (rule->subject->nbr > subjectNbr)
		{
			if (nextRule) *nextRule = elt;
			break;		/*	Same as end of list.	*/
		}

		/*	Matched rule's subject number.			*/

		if (rule->roleNbr != roleNbr)
		{
			if (rule->roleNbr == 0)
			{
				/*	Must insert before this one.	*/

				if (nextRule) *nextRule = elt;
				break;
			}

			continue;
		}

		/*	Matched rule's role number.			*/

		if (rule->continuumNbr != continuumNbr)
		{
			if (rule->continuumNbr == 0)
			{
				/*	Must insert before this one.	*/

				if (nextRule) *nextRule = elt;
				break;
			}

			continue;
		}

		/*	Matched rule's continuum number.		*/

		if (rule->unitNbr != unitNbr)
		{
			if (subunitOf(sap, unitNbr, rule->unitNbr))
			{
				/*	Must insert before this one.	*/

				if (nextRule) *nextRule = elt;
				break;
			}

			continue;
		}

		/*	Exact match.					*/

		return elt;
	}

	return NULL;
}

static int	addMsgRule(AmsSAP *sap, int ruleType, Subject *subject,
			int roleNbr, int continuumNbr, int unitNbr,
			int priority, int flowLabel, DeliveryVector *vector)
{
	Lyst	rules;
	LystElt	elt;
	LystElt	nextRule;
	MsgRule	*rule;

	rules = (ruleType == SUBSCRIPTION ?
			sap->subscriptions : sap->invitations);
	elt = findMsgRule(sap, rules, subject->nbr, roleNbr, continuumNbr,
			unitNbr, &nextRule);
	if (elt)	/*	Already have a rule for this subject.	*/
	{
		errno = EINVAL;
		return -1;
	}
	else		/*	Must insert new message rule structure.	*/
	{
		rule = (MsgRule *) MTAKE(sizeof(MsgRule));
		if (rule == NULL)
		{
			putErrmsg(NoMemoryMemo, NULL);
			return -1;
		}

		rule->subject = subject;
		rule->roleNbr = roleNbr;
		rule->continuumNbr = continuumNbr;
		rule->unitNbr = unitNbr;
		rule->priority = priority;
		rule->flowLabel = flowLabel;
		rule->vector = vector;
		if (nextRule)
		{
			elt = lyst_insert_before(nextRule, rule);
		}
		else
		{
			elt = lyst_insert_last(rules, rule);
		}

		if (elt == NULL)
		{
			MRELEASE(rule);
			putErrmsg(NoMemoryMemo, NULL);
			return -1;
		}
	}

	return 0;
}

static int	ams_invite2(AmsSAP *sap, int roleNbr, int continuumNbr,
			int unitNbr, int subjectNbr, int priority,
			unsigned char flowLabel, AmsSequence sequence,
			AmsDiligence diligence)
{
	DeliveryVector	*vector;
	Subject		*subject;
	char		*assertion;
	char		*cursor;

	if (priority == 0)
	{
		priority = 8;		/*	Default.		*/
	}
	else
	{
		if (priority < 1 || priority >= NBR_OF_PRIORITY_LEVELS)
		{
			putErrmsg(BadParmsMemo, NULL);
			errno = EINVAL;
			return -1;
		}
	}

	if (continuumNbr == THIS_CONTINUUM)
	{
		continuumNbr = mib->localContinuumNbr;
	}

	if (subjectNbr == 0)	/*	Messages on all subjects.	*/
	{
		if (continuumNbr == mib->localContinuumNbr)
		{
			subject = sap->venture->subjects[ALL_SUBJECTS];
		}
		else
		{
			putErrmsg(BadParmsMemo, NULL);
			errno = EINVAL;
			return -1;
		}
	}
	else
	{
		if (!subjectIsValid(sap, subjectNbr, &subject))
		{
			putErrmsg(BadParmsMemo, NULL);
			errno = EINVAL;
			return -1;
		}
	}

	vector = lookUpDeliveryVector(sap, sequence, diligence);
	if (vector == NULL)
	{
		putErrmsg("Have no endpoints for reception at this QOS.", NULL);
		errno = EINVAL;
		return -1;
	}

	if (addMsgRule(sap, INVITATION, subject, roleNbr, continuumNbr,
			unitNbr, priority, flowLabel, vector) < 0)
	{
		if (errno == EINVAL)
		{
			return 0;	/*	Redundant but okay.	*/
		}

		return -1;
	}

	assertion = MTAKE(INVITE_LEN);
	if (assertion == NULL)
	{
		putErrmsg(NoMemoryMemo, NULL);
		return -1;
	}

	cursor = assertion;
	loadAssertion(&cursor, INVITATION, roleNbr, continuumNbr,
			unitNbr, subjectNbr, vector->nbr, priority, flowLabel);
	if (enqueueMsgToRegistrar(sap, invite,
			computeModuleId(sap->role->nbr, sap->unit->nbr,
			sap->moduleNbr), INVITE_LEN, assertion) < 0)
	{
		return -1;
	}

	/*	Post service indication noting own invitation.		*/

	return enqueueNotice(sap, AmsInvitationState, AmsStateBegins,
			sap->unit->nbr, sap->moduleNbr, roleNbr, continuumNbr,
			unitNbr, subjectNbr, priority, flowLabel, sequence,
			diligence);
}

int	ams_invite(AmsSAP *sap, int roleNbr, int continuumNbr, int unitNbr,
		int subjectNbr, int priority, unsigned char flowLabel,
		AmsSequence sequence, AmsDiligence diligence)
{
	int	result = -1;

	if (validSap(sap))
	{
		LOCK_MIB;
		result = ams_invite2(sap, roleNbr, continuumNbr, unitNbr,
			subjectNbr, priority, flowLabel, sequence, diligence);
		UNLOCK_MIB;
		if (result < 0)
		{
			if (errno == 0) errno = EAGAIN;
		}
	}

	return result;
}

static int	removeMsgRule(AmsSAP *sap, int ruleType, Subject *subject,
			int roleNbr, int continuumNbr, int unitNbr)
{
	Lyst	rules;
	LystElt	elt;

	rules = (ruleType == SUBSCRIPTION ?
			sap->subscriptions : sap->invitations);
	elt = findMsgRule(sap, rules, subject->nbr, roleNbr, continuumNbr,
			unitNbr, NULL);
	if (elt == NULL)
	{
		errno = EINVAL;
		return -1;
	}

	lyst_delete(elt);
	return 0;
}

static int	ams_disinvite2(AmsSAP *sap, int roleNbr, int continuumNbr,
			int unitNbr, int subjectNbr)
{
	Subject		*subject;
	unsigned char	*cancellation;
	short		i2;
	unsigned short	u2;

	if (continuumNbr == THIS_CONTINUUM)
	{
		continuumNbr = mib->localContinuumNbr;
	}

	if (subjectNbr == 0 || !subjectIsValid(sap, subjectNbr, &subject))
	{
		putErrmsg(BadParmsMemo, NULL);
		errno = EINVAL;
		return -1;
	}

	if (removeMsgRule(sap, INVITATION, subject, roleNbr, continuumNbr,
			unitNbr) < 0)
	{
		return 0;		/*	Redundant but okay.	*/
	}

	cancellation = MTAKE(7);
	if (cancellation == NULL)
	{
		putErrmsg(NoMemoryMemo, NULL);
		return -1;
	}

	i2 = subjectNbr;
	i2 = htons(i2);
	memcpy(cancellation, (char *) &i2, 2);
	*(cancellation + 2) = (continuumNbr >> 8) & 0x0000007f;
	*(cancellation + 3) = continuumNbr & 0x000000ff;
	u2 = unitNbr;
	u2 = htons(u2);
	memcpy(cancellation + 4, (char *) &u2, 2);
	*(cancellation + 6) = roleNbr;
	if (enqueueMsgToRegistrar(sap, disinvite,
			computeModuleId(sap->role->nbr, sap->unit->nbr,
			sap->moduleNbr), 7, (char *) cancellation) < 0)
	{
		return -1;
	}

	/*	Post service indication noting own disinvitation.	*/

	return enqueueNotice(sap, AmsInvitationState, AmsStateBegins,
			sap->unit->nbr, sap->moduleNbr, roleNbr, continuumNbr,
			unitNbr, subjectNbr, 0, 0, 0, 0);
}

int	ams_disinvite(AmsSAP *sap, int roleNbr, int continuumNbr, int unitNbr,
		int subjectNbr)
{
	int	result = -1;

	if (validSap(sap))
	{
		LOCK_MIB;
		result = ams_disinvite2(sap, roleNbr, continuumNbr, unitNbr,
				subjectNbr);
		UNLOCK_MIB;
		if (result < 0)
		{
			if (errno == 0) errno = EAGAIN;
		}
	}

	return result;
}

static void	constructEnvelope(unsigned char *envelope, int continuumNbr,
			int unitNbr, int sourceIdNbr, int destinationIdNbr,
			int subjectNbr, int enclosureHdrLength,
			char *enclosureHdr, int enclosureContentLength,
			char *enclosureContent, int controlCode)
{
	short		i2;
	unsigned short	enclosureLength;

	envelope[0] = controlCode;
	envelope[1] = (continuumNbr >> 16) & 0x000000ff;
	envelope[2] = (continuumNbr >> 8) & 0x000000ff;
	envelope[3] = continuumNbr & 0x000000ff;
	envelope[4] = (unitNbr >> 8) & 0x000000ff;
	envelope[5] = unitNbr & 0x000000ff;
	envelope[6] = sourceIdNbr;
	envelope[7] = destinationIdNbr;
	i2 = subjectNbr;
	i2 = htons(i2);
	memcpy(envelope + 8, (unsigned char *) &i2, 2);
	enclosureLength = enclosureHdrLength + enclosureContentLength;
	envelope[10] = (enclosureLength >> 8) & 0x00ff;
	envelope[11] = enclosureLength & 0x00ff;
	if (enclosureLength > 0)
	{
		memcpy(envelope + 12, enclosureHdr, enclosureHdrLength);
		memcpy(envelope + 12 + enclosureHdrLength, enclosureContent,
				enclosureContentLength);
	}
}

static int	ams_subscribe2(AmsSAP *sap, int roleNbr, int continuumNbr,
			int unitNbr, int subjectNbr, int priority,
			unsigned char flowLabel, AmsSequence sequence,
			AmsDiligence diligence)
{
	DeliveryVector	*vector;
	Subject		*subject;
	char		*assertion;
	char		*cursor;

	if (priority == 0)
	{
		priority = 8;		/*	Default.		*/
	}
	else
	{
		if (priority < 1 || priority >= NBR_OF_PRIORITY_LEVELS)
		{
			putErrmsg(BadParmsMemo, NULL);
			errno = EINVAL;
			return -1;
		}
	}

	if (continuumNbr == THIS_CONTINUUM)
	{
		continuumNbr = mib->localContinuumNbr;
	}

	if (subjectNbr == 0)	/*	Messages on all subjects.	*/
	{
		if (continuumNbr == mib->localContinuumNbr)
		{
			subject = sap->venture->subjects[ALL_SUBJECTS];
		}
		else
		{
			putErrmsg(BadParmsMemo, NULL);
			errno = EINVAL;
			return -1;
		}
	}
	else
	{
		if (!subjectIsValid(sap, subjectNbr, &subject))
		{
			putErrmsg(BadParmsMemo, NULL);
			errno = EINVAL;
			return -1;
		}
	}

	vector = lookUpDeliveryVector(sap, sequence, diligence);
	if (vector == NULL)
	{
		putErrmsg("Have no endpoints for reception at this QOS.", NULL);
		errno = EINVAL;
		return -1;
	}

	if (addMsgRule(sap, SUBSCRIPTION, subject, roleNbr, continuumNbr,
			unitNbr, priority, flowLabel, vector) < 0)
	{
		if (errno == EINVAL)
		{
			return 0;	/*	Redundant but okay.	*/
		}

		return -1;
	}

	assertion = MTAKE(SUBSCRIBE_LEN);
	if (assertion == NULL)
	{
		putErrmsg(NoMemoryMemo, NULL);
		return -1;
	}

	cursor = assertion;
	loadAssertion(&cursor, SUBSCRIPTION, roleNbr, continuumNbr,
			unitNbr, subjectNbr, vector->nbr, priority, flowLabel);
	if (enqueueMsgToRegistrar(sap, subscribe,
			computeModuleId(sap->role->nbr, sap->unit->nbr,
			sap->moduleNbr), SUBSCRIBE_LEN, assertion) < 0)
	{
		return -1;
	}

	/*	Post service indication noting own subscription.	*/

	return enqueueNotice(sap, AmsSubscriptionState, AmsStateBegins,
			sap->unit->nbr, sap->moduleNbr, roleNbr, continuumNbr,
			unitNbr, subjectNbr, priority, flowLabel, sequence,
			diligence);
}

int	ams_subscribe(AmsSAP *sap, int roleNbr, int continuumNbr, int unitNbr,
		int subjectNbr, int priority, unsigned char flowLabel,
		AmsSequence sequence, AmsDiligence diligence)
{
	int	result = -1;

	if (validSap(sap))
	{
		LOCK_MIB;
		result = ams_subscribe2(sap, roleNbr, continuumNbr, unitNbr,
			subjectNbr, priority, flowLabel, sequence, diligence);
		UNLOCK_MIB;
		if (result < 0)
		{
			if (errno == 0) errno = EAGAIN;
		}
	}

	return result;
}

static int	ams_unsubscribe2(AmsSAP *sap, int roleNbr, int continuumNbr,
			int unitNbr, int subjectNbr)
{
	Subject		*subject;
	unsigned char	*cancellation;
	short		i2;
	unsigned short	u2;

	if (continuumNbr == THIS_CONTINUUM)
	{
		continuumNbr = mib->localContinuumNbr;
	}

	if (subjectNbr == 0)	/*	Messages on all subjects.	*/
	{
		if (continuumNbr == mib->localContinuumNbr)
		{
			subject = sap->venture->subjects[ALL_SUBJECTS];
		}
		else
		{
			putErrmsg(BadParmsMemo, NULL);
			errno = EINVAL;
			return -1;
		}
	}
	else
	{
		if (!subjectIsValid(sap, subjectNbr, &subject))
		{
			putErrmsg(BadParmsMemo, NULL);
			errno = EINVAL;
			return -1;
		}
	}

	if (removeMsgRule(sap, SUBSCRIPTION, subject, roleNbr, continuumNbr,
			unitNbr) < 0)
	{
		return 0;		/*	Redundant but okay.	*/
	}

	cancellation = MTAKE(7);
	if (cancellation == NULL)
	{
		putErrmsg(NoMemoryMemo, NULL);
		return -1;
	}

	i2 = subjectNbr;
	i2 = htons(i2);
	memcpy(cancellation, (char *) &i2, 2);
*(cancellation + 2) = (continuumNbr >> 16) & 0x000000ff;
	*(cancellation + 2) = (continuumNbr >> 8) & 0x0000007f;
	*(cancellation + 3) = continuumNbr & 0x000000ff;
	u2 = unitNbr;
	u2 = htons(u2);
	memcpy(cancellation + 4, (char *) &u2, 2);
	*(cancellation + 6) = roleNbr;
	if (enqueueMsgToRegistrar(sap, unsubscribe,
			computeModuleId(sap->role->nbr, sap->unit->nbr,
			sap->moduleNbr), 7, (char *) cancellation) < 0)
	{
		return -1;
	}

	/*	Post service indication noting own unsubscription.	*/

	return enqueueNotice(sap, AmsSubscriptionState, AmsStateEnds,
			sap->unit->nbr, sap->moduleNbr, roleNbr, continuumNbr,
			unitNbr, subjectNbr, 0, 0, 0, 0);
}

int	ams_unsubscribe(AmsSAP *sap, int roleNbr, int continuumNbr, int unitNbr,
		int subjectNbr)
{
	int	result = -1;

	if (validSap(sap))
	{
		LOCK_MIB;
		result = ams_unsubscribe2(sap, roleNbr, continuumNbr, unitNbr,
				subjectNbr);
		UNLOCK_MIB;
		if (result < 0)
		{
			if (errno == 0) errno = EAGAIN;
		}
	}

	return result;
}

static int	publishInEnvelope(AmsSAP *sap, int continuumNbr, int unitNbr,
			int sourceIdNbr, int destinationIdNbr, int subjectNbr,
			char *amsHeader, int amsHdrLength, char *content,
			int contentLength, int controlCode)
{
	int		subject = 0 - continuumNbr;
	int		envelopeLength = 12 + amsHdrLength + contentLength;
	unsigned char	*envelope;
	int		result;

	envelope = MTAKE(envelopeLength);
	if (envelope == NULL)
	{
		putErrmsg(NoMemoryMemo, NULL);
		return -1;
	}

	if (continuumNbr == mib->localContinuumNbr)	/*	All.	*/
	{
		continuumNbr = 0;
	}

	constructEnvelope(envelope, continuumNbr, unitNbr, sourceIdNbr,
			destinationIdNbr, subjectNbr, amsHdrLength, amsHeader,
			contentLength, content, controlCode);
	result = ams_publish2(sap, subject, 1, 0, envelopeLength,
			(char *) envelope, 0);
	MRELEASE(envelope);
	return result;
}

static int	sendMsg(AmsSAP *sap, int continuumNbr, int unitNbr, int moduleNbr,
			int subjectNbr, int priority, unsigned char flowLabel,
			int contentLength, char *content, int context,
			AmsMsgType msgType)
{
	Subject		*subject;
	char		amsHeader[16];
	int		headerLength = sizeof amsHeader;
	Unit		*unit;
	Module		*module;
	LystElt		elt;
	SubjOfInterest	*subj = NULL;
	FanModule	*fan;
	XmitRule	*rule = NULL;

	if (continuumNbr == THIS_CONTINUUM)
	{
		continuumNbr = mib->localContinuumNbr;
	}

	if (continuumNbr < 1 || unitNbr < 0 || unitNbr > MaxUnitNbr
	|| moduleNbr < 1 || moduleNbr > MaxModuleNbr
	|| priority < 0 || priority >= NBR_OF_PRIORITY_LEVELS
	|| contentLength < 0 || contentLength > MAX_AMS_CONTENT)
	{
		errno = EINVAL;
		putSysErrmsg(BadParmsMemo, NULL);
		return -1;
	}

	/*	Only the RAMS gateway is allowed to send messages
	 *	on the subject number that is the additive inverse
	 *	of the local continuum number, which is reserved for
	 *	local transmission of messages from remote continua.
	 *	All other subject numbers less than 1 are always
	 *	invalid.						*/

	if (subjectNbr < 1)
	{
		if (sap->role->nbr != 1		/*	RAMS gateway	*/
		|| subjectNbr != (0 - mib->localContinuumNbr))
		{
			errno = EINVAL;
			putSysErrmsg(BadParmsMemo, NULL);
			return -1;
		}

		subject = sap->venture->msgspaces[mib->localContinuumNbr];
	}
	else
	{
		if (!subjectIsValid(sap, subjectNbr, &subject))
		{
			errno = EINVAL;
			putSysErrmsg(BadParmsMemo, NULL);
			return -1;
		}
	}

	/*	Do remote AMS procedures if necessary.			*/

	if (continuumNbr != mib->localContinuumNbr)
	{
		constructMessage(sap, subjectNbr, priority, flowLabel, context,
			content, contentLength, (unsigned char *) amsHeader,
			msgType);
		return publishInEnvelope(sap, continuumNbr, unitNbr,
			sap->role->nbr, moduleNbr, subjectNbr, amsHeader,
			headerLength, content, contentLength, 5);
	}

	/*	Find constraints on sending the message.		*/

	unit = sap->venture->units[unitNbr];
	if (unit == NULL)
	{
		errno = EINVAL;
		putSysErrmsg("Unknown destination unit.", itoa(unitNbr));
		return -1;
	}

	module = unit->cell->modules[moduleNbr];
	if (module->role == NULL)
	{
		errno = EINVAL;
		putSysErrmsg("Unknown destination module.", itoa(moduleNbr));
		return -1;
	}

	if (lyst_length(module->subjects) < lyst_length(subject->modules))
	{
		elt = findSubjOfInterest(sap, module, subject, NULL);
		if (elt)
		{
			subj = (SubjOfInterest *) lyst_data(elt);
		}
		else
		{
			if (subjectNbr > 0)
			{
				subject = sap->venture->subjects[ALL_SUBJECTS];
				elt = findSubjOfInterest(sap, module, subject,
						NULL);
				if (elt)
				{
					subj = (SubjOfInterest *)
							lyst_data(elt);
				}
			}
		}
	}
	else
	{
		elt = findFanModule(sap, subject, module, NULL);
		if (elt)
		{
			fan = (FanModule *) lyst_data(elt);
			subj = fan->subj;
		}
		else
		{
			if (subjectNbr > 0)
			{
				subject = sap->venture->subjects[ALL_SUBJECTS];
				elt = findFanModule(sap, subject, module,
						NULL);
				if (elt)
				{
					fan = (FanModule *)
							lyst_data(elt);
					subj = fan->subj;
				}
			}
		}
	}

	if (subj)
	{
		rule = getXmitRule(sap, subj->invitations);
	}

	if (rule == NULL)
	{
		errno = EINVAL;
		putSysErrmsg("Can't send msgs on this subject to this module",
				NULL);
		return -1;
	}

	if (priority == 0)		/*	No override.		*/
	{
		priority = rule->priority;
	}

	if (flowLabel == 0)		/*	No override.		*/
	{
		flowLabel = rule->flowLabel;
	}

	constructMessage(sap, subjectNbr, priority, flowLabel, context, content,
			contentLength, (unsigned char *) amsHeader, msgType);

	/*	Send the message.					*/
#if 0
printf("Sent %x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x '%s'\n",
amsHeader[0],
amsHeader[1],
amsHeader[2],
amsHeader[3],
amsHeader[4],
amsHeader[5],
amsHeader[6],
amsHeader[7],
amsHeader[8],
amsHeader[9],
amsHeader[10],
amsHeader[11],
amsHeader[12],
amsHeader[13],
amsHeader[14],
amsHeader[15],
content);
fflush(stdout);
#endif
	return rule->amsEndpoint->ts->sendAmsFn(rule->amsEndpoint, sap,
		flowLabel, amsHeader, headerLength, content, contentLength);
}

static int	ams_send2(AmsSAP *sap, int continuumNbr, int unitNbr,
			int moduleNbr, int subjectNbr, int priority,
			unsigned char flowLabel, int contentLength,
			char *content, int context)
{
	return sendMsg(sap, continuumNbr, unitNbr, moduleNbr, subjectNbr,
			priority, flowLabel, contentLength, content,
			context, AmsMsgUnary);
}

int	ams_send(AmsSAP *sap, int continuumNbr, int unitNbr, int moduleNbr,
		int subjectNbr, int priority, unsigned char flowLabel,
		int contentLength, char *content, int context)
{
	int	result = -1;

	if (validSap(sap))
	{
		LOCK_MIB;
		result = ams_send2(sap, continuumNbr, unitNbr, moduleNbr,
				subjectNbr, priority, flowLabel, contentLength,
				content, context);
		UNLOCK_MIB;
		if (result < 0)
		{
			if (errno == 0) errno = EAGAIN;
		}
	}

	return result;
}

static int	deliverTimeout(AmsEvent *event)
{
	AmsEvt	*evt;

	evt = (AmsEvt *) MTAKE(sizeof(AmsEvt));
	if (evt == NULL)
	{
		putErrmsg(NoMemoryMemo, NULL);
		return -1;
	}

	evt->type = TIMEOUT_EVT;
	*event = evt;
	return 0;
}

static int	getEvent(AmsSAP *sap, int term, AmsEvent *event,
			LlcvPredicate condition)
{
	int	result;
	LystElt	elt;
	AmsEvt	*evt;

	*event = NULL;
	while (1)
	{
		UNLOCK_MIB;
		result = llcv_wait(sap->amsEventsCV, condition, term);
		LOCK_MIB;
		if (result < 0)
		{
			lyst_compare_set(sap->amsEvents, NULL);
			if (errno == ETIMEDOUT)
			{
				return deliverTimeout(event);
			}

			return -1;
		}

		/*	A non-timeout event has arrived.		*/

		llcv_lock(sap->amsEventsCV);
		elt = lyst_first(sap->amsEvents);
		if (elt == NULL)	/*	Interrupted; no retry.	*/
		{
			/*	llcv_wait was ended by forced
			 *	signal.  Respond by returning a
			 *	simulated timeout event.		*/

			llcv_unlock(sap->amsEventsCV);
			lyst_compare_set(sap->amsEvents, NULL);
			return deliverTimeout(event);
		}

		noteEventDequeued(sap, elt);
		evt = (AmsEvent) lyst_data_set(elt, NULL);
		lyst_delete(elt);
		llcv_unlock(sap->amsEventsCV);
		switch (evt->type)
		{
		case AMS_MSG_EVT:
		case TIMEOUT_EVT:
		case NOTICE_EVT:
		case USER_DEFINED_EVT:
		case BREAK_EVT:
			*event = evt;
			lyst_compare_set(sap->amsEvents, NULL);
			return 0;

		case CRASH_EVT:
			putErrmsg("AMS module crash.", evt->value);
			recycleEvent(evt);
			return -1;

		default:	/*	Inapplicable event; ignore.	*/
			recycleEvent(evt);
			continue;	/*	Try again.		*/
		}
	}
}

static void	handleNoticeEvent(AmsSAP *sap, AmsEvent *event)
{
	AmsEventMgt	*rules = &(sap->eventMgtRules);
	AmsEvt		*evt = *event;
	AmsStateType	stateType;
	AmsChangeType	changeType;
	int		unitNbr;
	int		moduleNbr;
	int		roleNbr;
	int		domainContinuumNbr;
	int		domainUnitNbr;
	int		subjectNbr;
	int		priority;
	unsigned char	flowLabel;
	AmsSequence	sequence;
	AmsDiligence	diligence;

	ams_parse_notice(evt, &stateType, &changeType, &unitNbr, &moduleNbr,
			&roleNbr, &domainContinuumNbr, &domainUnitNbr,
			&subjectNbr, &priority, &flowLabel, &sequence,
			&diligence);
	switch (stateType)
	{
	case AmsRegistrationState:
		switch (changeType)
		{
		case AmsStateBegins:
			if (rules->registrationHandler)
			{
				rules->registrationHandler(sap,
					rules->registrationHandlerUserData,
					event, unitNbr, moduleNbr, roleNbr);
			}

			return;

		case AmsStateEnds:
			if (rules->unregistrationHandler)
			{
				rules->unregistrationHandler(sap,
					rules->unregistrationHandlerUserData,
					event, unitNbr, moduleNbr);
			}

			return;

		default:
			return;
		}

	case AmsInvitationState:
		switch (changeType)
		{
		case AmsStateBegins:
			if (rules->invitationHandler)
			{
				rules->invitationHandler(sap,
					rules->invitationHandlerUserData,
					event, unitNbr, moduleNbr, roleNbr,
					domainContinuumNbr, domainUnitNbr,
					subjectNbr, priority, flowLabel,
					sequence, diligence);
			}

			return;

		case AmsStateEnds:
			if (rules->disinvitationHandler)
			{
				rules->disinvitationHandler(sap,
					rules->disinvitationHandlerUserData,
					event, unitNbr, moduleNbr, roleNbr,
					domainContinuumNbr, domainUnitNbr,
					subjectNbr);
			}

			return;

		default:
			return;
		}

	case AmsSubscriptionState:
		switch (changeType)
		{
		case AmsStateBegins:
			if (rules->subscriptionHandler)
			{
				rules->subscriptionHandler(sap,
					rules->subscriptionHandlerUserData,
					event, unitNbr, moduleNbr, roleNbr,
					domainContinuumNbr, domainUnitNbr,
					subjectNbr, priority, flowLabel,
					sequence, diligence);
			}

			return;

		case AmsStateEnds:
			if (rules->unsubscriptionHandler)
			{
				rules->unsubscriptionHandler(sap,
					rules->unsubscriptionHandlerUserData,
					event, unitNbr, moduleNbr, roleNbr,
					domainContinuumNbr, domainUnitNbr,
					subjectNbr);
			}

			return;

		default:
			return;
		}

	default:
		return;
	}
}

static void	*eventMgrMain(void *parm)
{
	AmsSAP		*sap = (AmsSAP *) parm;
	AmsEventMgt	*rules = &(sap->eventMgtRules);
	sigset_t	signals;
	AmsEvt		*evt;
	int		continuumNbr;
	int		unitNbr;
	int		moduleNbr;
	int		subjectNbr;
	int		contentLength;
	char		*content;
	int		context;
	AmsMsgType	msgType;
	int		priority;
	unsigned char	flowLabel;
	int		code;
	int		dataLength;
	char		*data;

	sigfillset(&signals);
	pthread_sigmask(SIG_BLOCK, &signals, NULL);
	sap->eventMgr = sap->authorizedEventMgr = pthread_self();
	while (pthread_equal(sap->eventMgr, sap->authorizedEventMgr))
	{
		if (ams_get_event(sap, AMS_BLOCKING, &evt) < 0)
		{
			putErrmsg("Event manager unable to get event.", NULL);
			if (rules->errHandler)
			{
				rules->errHandler(rules->errHandlerUserData,
						&evt);
			}

			break;		/*	Out of event loop.	*/
		}

		switch (evt->type)
		{
		case BREAK_EVT:
			/*	Removal is signaled.  Break simply
			 *	causes event to be recycled and loop
			 *	limit condition to be checked; this
			 *	condition is now false, so the loop
			 *	terminates and the thread exits
			 *	silently.				*/

			break;

		case AMS_MSG_EVT:
			if (rules->msgHandler)
			{
				ams_parse_msg(evt, &continuumNbr, &unitNbr,
					&moduleNbr, &subjectNbr, &contentLength,
					&content, &context, &msgType,
					&priority, &flowLabel);
				rules->msgHandler(sap,
					rules->msgHandlerUserData, &evt,
					continuumNbr, unitNbr, moduleNbr,
					subjectNbr, contentLength, content,
					context, msgType, priority, flowLabel);
			}

			break;

		case NOTICE_EVT:
			handleNoticeEvent(sap, &evt);
			break;

		case USER_DEFINED_EVT:
			if (rules->userEventHandler)
			{
				ams_parse_user_event(evt, &code, &dataLength,
						&data);
				rules->userEventHandler(sap,
					rules->userEventHandlerUserData, &evt,
					code, dataLength, data);
			} 

			break;

		default:
			break;
		}

		/*	Note: user code can retain event for its own
		 *	use by saving the value of evt somewhere and
		 *	setting evt to NULL.  In that case, the user
		 *	code assumes responsibility for recycling the
		 *	event.						*/

		if (evt != NULL)
		{
			ams_recycle_event(evt);
		}
	}

	return NULL;
}

static void	stopEventMgr(AmsSAP *sap)
{
	/*	End event manager's authority to manage events.		*/

	sap->authorizedEventMgr = pthread_self();

	/*	Tell event manager to close down right away.		*/

	if (enqueueAmsStubEvent(sap, BREAK_EVT, 0) < 0)
	{
		putErrmsg("Crashed AMS service.", NULL);
		return;
	}

	/*	Now wait for event manager thread to shut itself down.	*/

	UNLOCK_MIB;
	pthread_join(sap->eventMgr, NULL);
	LOCK_MIB;

	/*	Complete transfer of event management responsibility
	 *	to self.						*/

	sap->eventMgr = sap->authorizedEventMgr;
}

static int	ams_query2(AmsSAP *sap, int continuumNbr, int unitNbr,
			int moduleNbr, int subjectNbr, int priority,
			unsigned char flowLabel, int contentLength,
			char *content, long context, int term, AmsEvent *event)
{
	int		eventMgrNeeded = 0;
	int		result = 0;
	pthread_t	mgrThread;

	if (sap == NULL || event == NULL)
	{
		putErrmsg(BadParmsMemo, NULL);
		errno = EINVAL;
		return -1;
	}

	*event = NULL;
	if (context == 0)
	{
		putErrmsg("Non-zero context nbr needed for query.", NULL);
		errno = EINVAL;
		return -1;
	}

	if (term == AMS_POLL	/*	Pseudo-synchronous.		*/
	|| (continuumNbr != THIS_CONTINUUM
		&& continuumNbr != mib->localContinuumNbr))
	{
		return sendMsg(sap, continuumNbr, unitNbr, moduleNbr, subjectNbr,
				priority, flowLabel, contentLength, content,
				context, AmsMsgQuery);
	}

	/*	True synchronous query.					*/

	if (!pthread_equal(pthread_self(), sap->eventMgr))
	{
		/*	Query by non-event-mgr thread.  Must stop
		 *	event manager for duration of query, then
		 *	restart it.					*/

		eventMgrNeeded = 1;
		stopEventMgr(sap);
	}

	lyst_compare_set(sap->amsEvents, (LystCompareFn) context);
	if (sendMsg(sap, continuumNbr, unitNbr, moduleNbr, subjectNbr,
			priority, flowLabel, contentLength, content,
			context, AmsMsgQuery) < 0)
	{
		putErrmsg("Failed on attempt to send query.", NULL);
		return -1;
	}

	/*	Send succeeded; now wait for reply.  All AMS events
	 *	other than response to this message (identified by
	 *	context number) must be ignored for now; see
	 *	enqueueAmsEvent.					*/

	if (getEvent(sap, term, event, llcv_reply_received) < 0)
	{
		result = -1;
	}

	/*	Restart the event manager thread if necessary.		*/

	if (eventMgrNeeded)
	{
		if (pthread_create(&mgrThread, NULL, eventMgrMain, sap) < 0)
		{
			sap->authorizedEventMgr = sap->primeThread;
			sap->eventMgr = sap->primeThread;
			putSysErrmsg("Can't re-spawn event mgr thread", NULL);
			return -1;
		}
	}

	return result;
}

int	ams_query(AmsSAP *sap, int continuumNbr, int unitNbr, int moduleNbr,
		int subjectNbr, int priority, unsigned char flowLabel,
		int contentLength, char *content, int context, int term,
		AmsEvent *event)
{
	int	result = -1;

	if (validSap(sap))
	{
		LOCK_MIB;
		result = ams_query2(sap, continuumNbr, unitNbr, moduleNbr,
				subjectNbr, priority, flowLabel, contentLength,
				content, context, term, event);
		UNLOCK_MIB;
		if (result < 0)
		{
			if (errno == 0) errno = EAGAIN;
		}
	}

	return result;
}

static int	ams_reply2(AmsSAP *sap, AmsEvt *evt, int subjectNbr,
			int priority, unsigned char flowLabel,
			int contentLength, char *content)
{
	int	result;
	AmsMsg	*msg;

	if (evt == NULL || evt->type != AMS_MSG_EVT)
	{
		putErrmsg("Antecedent msg needed for reply.", NULL);
		errno = EINVAL;
		return -1;
	}

	msg = (AmsMsg *) (evt->value);
	result = sendMsg(sap, msg->continuumNbr, msg->unitNbr, msg->moduleNbr,
			subjectNbr, priority, flowLabel, contentLength,
			content, msg->contextNbr, AmsMsgReply);
	return result;
}

int	ams_reply(AmsSAP *sap, AmsEvt *evt, int subjectNbr, int priority,
		unsigned char flowLabel, int contentLength, char *content)
{
	int	result = -1;

	if (validSap(sap))
	{
		LOCK_MIB;
		result = ams_reply2(sap, evt, subjectNbr, priority,
				flowLabel, contentLength, content);
		UNLOCK_MIB;
		if (result < 0)
		{
			if (errno == 0) errno = EAGAIN;
		}
	}

	return result;
}

static int	ams_announce2(AmsSAP *sap, int roleNbr, int continuumNbr,
			int unitNbr, int subjectNbr, int priority,
			unsigned char flowLabel, int contentLength,
			char *content, int context)
{
	Subject		*subject;
	char		amsHeader[16];
	int		headerLength = sizeof amsHeader;
	int		result;
	unsigned char	protectedBits;
	Lyst		recipients;
	LystElt		elt;
	FanModule	*fan;
	XmitRule	*rule;

	if (continuumNbr == THIS_CONTINUUM)
	{
		continuumNbr = mib->localContinuumNbr;
	}

	if (continuumNbr < 0 || unitNbr < 0 || unitNbr > MaxUnitNbr
	|| roleNbr < 0 || roleNbr > MaxRoleNbr
	|| priority < 0 || priority >= NBR_OF_PRIORITY_LEVELS
	|| contentLength < 0 || contentLength > MAX_AMS_CONTENT)
	{
		putErrmsg(BadParmsMemo, NULL);
		errno = EINVAL;
		return -1;
	}

	if (subjectNbr < 1 || !subjectIsValid(sap, subjectNbr, &subject))
	{
		putErrmsg(BadParmsMemo, NULL);
		errno = EINVAL;
		return -1;
	}

	/*	Knowing subject, construct message header & encrypt.	*/

	constructMessage(sap, subjectNbr, priority, flowLabel, context, content,
		contentLength, (unsigned char *) amsHeader, AmsMsgUnary);

	/*	Do remote AMS procedures if necessary.			*/

	if (continuumNbr == ALL_CONTINUA)	/*	All msgspaces.	*/
	{
		result = publishInEnvelope(sap, mib->localContinuumNbr,
			unitNbr, sap->role->nbr, roleNbr, subjectNbr,
			amsHeader, headerLength, content, contentLength, 6);
		if (result < 0)
		{
			return result;
		}
	}
	else		/*	Announcing to just one msgspace.	*/
	{
		if (continuumNbr != mib->localContinuumNbr)
		{
			return publishInEnvelope(sap, continuumNbr, unitNbr,
				sap->role->nbr, roleNbr, subjectNbr, amsHeader,
				headerLength, content, contentLength, 6);
		}
	}

	/*	Destination is either all msgspaces (including the one
	 *	in the local continuum) or just the local continuum's
	 *	message space.  So announce to the local continuum's
	 *	message space now.					*/

	protectedBits = amsHeader[0] & 0xf0;
	recipients = lyst_create();

	/*	First send a copy of the message to every module in the
	 *	domain of this request that has posted at least one
	 *	expression of interest in this subject whose domain
	 *	includes the local module.  This is not publication,
	 *	but we still use the subject's list of interested modules
	 *	to drive the message distribution: any module that has
	 *	not at least invited messages on this subject cannot
	 *	receive the message we are announcing.			*/

	for (elt = lyst_first(subject->modules); elt; elt = lyst_next(elt))
	{
		fan = (FanModule *) lyst_data(elt);
		if (roleNbr != 0	/*	Role-specific request.	*/
		&& roleNbr != fan->module->role->nbr)
		{
			continue;	/*	Module's role is wrong.	*/
		}

		if (!subunitOf(sap, fan->module->unitNbr, unitNbr))
		{
			continue;	/*	Module in wrong unit.	*/
		}

		/*	Module is in the domain of the announce request.	*/

		rule = getXmitRule(sap, fan->subj->invitations);
		if (rule == NULL)
		{
			continue;	/*	Can't send module a copy.	*/
		}

		/*	Can send this module a copy of this message.
		 *	Supply default priority and/or flow label as
		 *	necessary, and send message to module.		*/

		if (priority)		/*	Override.		*/
		{
			amsHeader[0] = (char) (protectedBits | priority);
		}
		else			/*	Use default per rule.	*/
		{
			amsHeader[0] = (char) (protectedBits | rule->priority);
		}

		if (flowLabel == 0)
		{
			flowLabel = rule->flowLabel;
		}

		result = rule->amsEndpoint->ts->sendAmsFn(rule->amsEndpoint,
				sap, flowLabel, amsHeader, headerLength,
				content, contentLength);
		if (result < 0)
		{
			return result;
		}

		lyst_insert_last(recipients,
				(void *) ((long) (fan->module->nbr)));
		// cast to long to avoid warnings on 64-bit
	}

	/*	Now send a copy of the message to every module in the
	 *	domain of this request that has posted at least one
	 *	expression of interest in ALL SUBJECTS whose domain
	 *	includes the local module.  Again, it's not publication,
	 *	but we still use the subject's list of interested modules
	 *	to drive the message distribution: any module that has
	 *	not at least invited messages on all subjects cannot
	 *	receive the message we are announcing.			*/

	subject = sap->venture->subjects[ALL_SUBJECTS];
	for (elt = lyst_first(subject->modules); elt; elt = lyst_next(elt))
	{
		fan = (FanModule *) lyst_data(elt);
		if (receivedMsgAlready(recipients, fan->module->nbr))
		{
			continue;	/*	Don't send 2nd copy.	*/
		}

		if (roleNbr != 0	/*	Role-specific request.	*/
		&& roleNbr != fan->module->role->nbr)
		{
			continue;	/*	Module's role is wrong.	*/
		}

		if (!subunitOf(sap, fan->module->unitNbr, unitNbr))
		{
			continue;	/*	Module in wrong unit.	*/
		}

		/*	Module is in the domain of the announce request.*/

		rule = getXmitRule(sap, fan->subj->invitations);
		if (rule == NULL)
		{
			continue;	/*	Can't send module a copy.*/
		}

		/*	Can send this module a copy of this message.
		 *	Supply default priority and/or flow label as
		 *	necessary, and send message to module.		*/

		if (priority)		/*	Override.		*/
		{
			amsHeader[0] = (char) (protectedBits | priority);
		}
		else			/*	Use default per rule.	*/
		{
			amsHeader[0] = (char) (protectedBits | rule->priority);
		}

		if (flowLabel == 0)
		{
			flowLabel = rule->flowLabel;
		}

		result = rule->amsEndpoint->ts->sendAmsFn(rule->amsEndpoint,
				sap, flowLabel, amsHeader, headerLength,
				content, contentLength);
		if (result < 0)
		{
			return result;
		}
	}

	lyst_destroy(recipients);
	return 0;
}

int	ams_announce(AmsSAP *sap, int roleNbr, int continuumNbr, int unitNbr,
		int subjectNbr, int priority, unsigned char flowLabel,
		int contentLength, char *content, int context)
{
	int	result = -1;

	if (validSap(sap))
	{
		LOCK_MIB;
		result = ams_announce2(sap, roleNbr, continuumNbr, unitNbr,
				subjectNbr, priority, flowLabel, contentLength,
				content, context);
		UNLOCK_MIB;
		if (result < 0)
		{
			if (errno == 0) errno = EAGAIN;
		}
	}

	return result;
}

static int	ams_post_user_event2(AmsSAP *sap, int code, int dataLength,
			char *data, int priority)
{
	AmsEvt	*evt;
	char	*cursor;

	if (dataLength < 0 || (dataLength > 0 && data == NULL)
	|| priority < 0 || priority >= NBR_OF_PRIORITY_LEVELS)
	{
		putErrmsg(BadParmsMemo, NULL);
		errno = EINVAL;
		return -1;
	}

	evt = (AmsEvt *) MTAKE(1 + (2 * sizeof(int)) + dataLength);
	if (evt == NULL)
	{
		putErrmsg(NoMemoryMemo, NULL);
		return -1;
	}

	evt->type = USER_DEFINED_EVT;
	cursor = evt->value;
	memcpy(cursor, (char *) &code, sizeof(int));
	cursor += sizeof(int);
	memcpy(cursor, (char *) &dataLength, sizeof(int));
	if (dataLength > 0)
	{
		cursor += sizeof(int);
		memcpy(cursor, data, dataLength);
	}

	return enqueueAmsEvent(sap, evt, NULL, 0, priority, AmsMsgNone);
}

int	ams_post_user_event(AmsSAP *sap, int code, int dataLength, char *data,
		int priority)
{
	int	result = -1;

	if (validSap(sap))
	{
		LOCK_MIB;
		result = ams_post_user_event2(sap, code, dataLength, data,
				priority);
		UNLOCK_MIB;
		if (result < 0)
		{
			if (errno == 0) errno = EAGAIN;
		}
	}

	return result;
}

static int	ams_get_event2(AmsSAP *sap, int term, AmsEvent *event)
{
	if (event == NULL || term < -1)
	{
		errno = EINVAL;
		return -1;
	}

	if (!pthread_equal(pthread_self(), sap->eventMgr))
	{
		putErrmsg("get_event attempted by non-event-mgr thread.", NULL);
		errno = EINVAL;
		return -1;
	}

	return getEvent(sap, term, event, llcv_lyst_not_empty);
}

int	ams_get_event(AmsSAP *sap, int term, AmsEvent *event)
{
	int	result = -1;

	if (validSap(sap))
	{
		LOCK_MIB;
		result = ams_get_event2(sap, term, event);
		UNLOCK_MIB;
		if (result < 0)
		{
			if (errno == 0) errno = EAGAIN;
		}
	}

	return result;
}

int	ams_get_event_type(AmsEvent event)
{
	if (event == NULL)
	{
		errno = EINVAL;
		return -1;
	}

	return event->type;
}

int	ams_parse_msg(AmsEvent event, int *continuumNbr, int *unitNbr,
		int *moduleNbr, int *subjectNbr, int *contentLength,
		char **content, int *context, AmsMsgType *msgType,
		int *priority, unsigned char *flowLabel)
{
	AmsMsg	*msg;

	if (event == NULL || event->type != AMS_MSG_EVT
	|| continuumNbr == NULL || unitNbr == NULL || moduleNbr == NULL
	|| subjectNbr == NULL || contentLength == NULL || content == NULL
	|| context == NULL || msgType == NULL || priority == NULL
	|| flowLabel == NULL)
	{
		errno = EINVAL;
		return -1;
	}

	msg = (AmsMsg *) (event->value);
	*continuumNbr = msg->continuumNbr;
	*unitNbr = msg->unitNbr;
	*moduleNbr = msg->moduleNbr;
	*subjectNbr = msg->subjectNbr;
	if (msg->content == NULL)
	{
		*contentLength = 0;
		*content = NULL;
	}
	else
	{
		*contentLength = msg->contentLength;
		*content = msg->content;
	}

	*context = msg->contextNbr;
	*msgType = msg->type;
	*priority = msg->priority;
	*flowLabel = msg->flowLabel;
	return 0;
}

static int	ams_lookup_unit_nbr2(AmsSAP *sap, char *unitName)
{
	Unit	*unit = lookUpUnit(sap->venture, unitName);

	if (unit)
	{
		return unit->nbr;
	}

	return -1;
}

int	ams_lookup_unit_nbr(AmsSAP *sap, char *unitName)
{
	int	result = -1;

	if (unitName && validSap(sap))
	{
		LOCK_MIB;
		result = ams_lookup_unit_nbr2(sap, unitName);
		UNLOCK_MIB;
		if (result < 0)
		{
			if (errno == 0) errno = EAGAIN;
		}
	}

	return result;
}

static int	ams_lookup_role_nbr2(AmsSAP *sap, char *roleName)
{
	AppRole	*role = lookUpRole(sap->venture, roleName);

	if (role)
	{
		return role->nbr;
	}

	return -1;
}

int	ams_lookup_role_nbr(AmsSAP *sap, char *roleName)
{
	int	result = -1;

	if (roleName && validSap(sap))
	{
		LOCK_MIB;
		result = ams_lookup_role_nbr2(sap, roleName);
		UNLOCK_MIB;
		if (result < 0)
		{
			if (errno == 0) errno = EAGAIN;
		}
	}

	return result;
}

static int	ams_lookup_subject_nbr2(AmsSAP *sap, char *subjectName)
{
	Subject	*subject = lookUpSubject(sap->venture, subjectName);

	if (subject)
	{
		return subject->nbr;
	}

	return -1;
}

int	ams_lookup_subject_nbr(AmsSAP *sap, char *subjectName)
{
	int	result = -1;

	if (subjectName && validSap(sap))
	{
		LOCK_MIB;
		result = ams_lookup_subject_nbr2(sap, subjectName);
		UNLOCK_MIB;
		if (result < 0)
		{
			if (errno == 0) errno = EAGAIN;
		}
	}

	return result;
}

int	ams_lookup_continuum_nbr(AmsSAP *sap, char *continuumName)
{
	int	result = -1;

	if (continuumName && validSap(sap))
	{
		LOCK_MIB;
		result = lookUpContinuum(continuumName);
		UNLOCK_MIB;
		if (result < 0)
		{
			if (errno == 0) errno = EAGAIN;
		}
	}

	return result;
}

static char	*ams_lookup_unit_name2(AmsSAP *sap, int unitNbr)
{
	Unit	*unit;

	if (unitNbr >= 0 && unitNbr <= MaxUnitNbr)
	{
		unit = sap->venture->units[unitNbr];
		if (unit)
		{
			return unit->name;
		}
	}

	return NULL;
}

char	*ams_lookup_unit_name(AmsSAP *sap, int unitNbr)
{
	char	*result = NULL;

	if (validSap(sap))
	{
		LOCK_MIB;
		result = ams_lookup_unit_name2(sap, unitNbr);
		UNLOCK_MIB;
		if (result == NULL)
		{
			if (errno == 0) errno = EAGAIN;
		}
	}

	return result;
}

static char	*ams_lookup_role_name2(AmsSAP *sap, int roleNbr)
{
	AppRole	*role;

	if (roleNbr > 0 && roleNbr <= MaxRoleNbr)
	{
		role = sap->venture->roles[roleNbr];
		if (role)
		{
			return role->name;
		}
	}

	return NULL;
}

char	*ams_lookup_role_name(AmsSAP *sap, int roleNbr)
{
	char	*result = NULL;

	if (validSap(sap))
	{
		LOCK_MIB;
		result = ams_lookup_role_name2(sap, roleNbr);
		UNLOCK_MIB;
		if (result == NULL)
		{
			if (errno == 0) errno = EAGAIN;
		}
	}

	return result;
}

static char	*ams_lookup_subject_name2(AmsSAP *sap, int subjectNbr)
{
	Subject	*subject;

	if (subjectNbr > 0 && subjectNbr <= MaxSubjNbr)
	{
		subject = sap->venture->subjects[subjectNbr];
		if (subject)
		{
			return subject->name;
		}
	}

	return NULL;
}

char	*ams_lookup_subject_name(AmsSAP *sap, int subjectNbr)
{
	char	*result = NULL;

	if (validSap(sap))
	{
		LOCK_MIB;
		result = ams_lookup_subject_name2(sap, subjectNbr);
		UNLOCK_MIB;
		if (result == NULL)
		{
			if (errno == 0) errno = EAGAIN;
		}
	}

	return result;
}

static char	*ams_lookup_continuum_name2(AmsSAP *sap, int continuumNbr)
{
	Continuum	*contin;

	if (continuumNbr < 0) continuumNbr = 0 - continuumNbr;
       	if (continuumNbr > 0 && continuumNbr <= MaxContinNbr)
	{
		contin = mib->continua[continuumNbr];
		if (contin)	/*	Known continuum.		*/
		{
			return contin->name;
		}
	}

	return NULL;
}

char	*ams_lookup_continuum_name(AmsSAP *sap, int continuumNbr)
{
	char	*result = NULL;

	if (validSap(sap))
	{
		LOCK_MIB;
		result = ams_lookup_continuum_name2(sap, continuumNbr);
		UNLOCK_MIB;
		if (result == NULL)
		{
			if (errno == 0) errno = EAGAIN;
		}
	}

	return result;
}

int	ams_parse_notice(AmsEvent event, AmsStateType *state,
		AmsChangeType *change, int *unitNbr, int *moduleNbr,
		int *roleNbr, int *domainContinuumNbr, int *domainUnitNbr,
		int *subjectNbr, int *priority, unsigned char *flowLabel,
		AmsSequence *sequence, AmsDiligence *diligence)
{
	AmsNotice	*notice;

	if (event == NULL || event->type != NOTICE_EVT
	|| state == NULL || change == NULL || unitNbr == NULL
	|| moduleNbr == NULL || roleNbr == NULL || domainContinuumNbr == NULL
	|| domainUnitNbr == NULL || subjectNbr == NULL || priority == NULL
	|| flowLabel == NULL || sequence == NULL || diligence == NULL)
	{
		errno = EINVAL;
		return -1;
	}

	notice = (AmsNotice *) (event->value);
	*state = notice->stateType;
	*change = notice->changeType;
	*unitNbr = notice->unitNbr;
	*moduleNbr = notice->moduleNbr;
	*roleNbr = notice->roleNbr;
	*domainContinuumNbr = notice->domainContinuumNbr;
	*domainUnitNbr = notice->domainUnitNbr;
	*subjectNbr = notice->subjectNbr;
	*priority = notice->priority;
	*flowLabel = notice->flowLabel;
	*sequence = notice->sequence;
	*diligence = notice->diligence;
	return 0;
}

int	ams_parse_user_event(AmsEvent event, int *code, int *dataLength,
		char **data)
{
	if (event == NULL || event->type != USER_DEFINED_EVT
	|| code == NULL || dataLength == NULL || data == NULL)
	{
		errno = EINVAL;
		return -1;
	}

	memcpy((char *) code, event->value, sizeof(int));
	memcpy((char *) dataLength, event->value + sizeof(int), sizeof(int));
	if (*dataLength > 0)
	{
		*data = event->value + (2 * sizeof(int));
	}
	else
	{
		*data = NULL;
	}

	return 0;
}

int	ams_recycle_event(AmsEvent event)
{
	AmsMsg	*msg;

	if (event == NULL)
	{
		errno = EINVAL;
		return -1;
	}

	if (event->type == AMS_MSG_EVT)
	{
		msg = (AmsMsg *) (event->value);
		if (msg->content)
		{
			MRELEASE(msg->content);
		}
	}

	MRELEASE(event);
	return 0;
}

static int	ams_set_event_mgr2(AmsSAP *sap, AmsEventMgt *rules)
{
	pthread_t	mgrThread;

	if (rules == NULL)
	{
		putErrmsg(BadParmsMemo, NULL);
		errno = EINVAL;
		return -1;
	}

	/*	Only prime thread can set event manager, and it can do
	 *	so only when it is, itself, the current event manager.	*/

	if (!pthread_equal(pthread_self(), sap->primeThread))
	{
		putErrmsg("Only prime thread can set event mgr.", NULL);
		errno = EINVAL;
		return -1;
	}

	if (!pthread_equal(sap->eventMgr, sap->primeThread))
	{
		putErrmsg("Another event mgr is running.", NULL);
		errno = EINVAL;
		return -1;
	}

	memcpy((char *) &(sap->eventMgtRules), rules, sizeof(AmsEventMgt));
	if (pthread_create(&mgrThread, NULL, eventMgrMain, sap))
	{
		sap->authorizedEventMgr = sap->primeThread;
		sap->eventMgr = sap->primeThread;
		putSysErrmsg("Can't spawn event manager thread", NULL);
		return -1;
	}

	return 0;
}

int	ams_set_event_mgr(AmsSAP *sap, AmsEventMgt *rules)
{
	int	result = -1;

	if (validSap(sap))
	{
		LOCK_MIB;
		result = ams_set_event_mgr2(sap, rules);
		UNLOCK_MIB;
		if (result < 0)
		{
			if (errno == 0) errno = EAGAIN;
		}
	}

	return result;
}

static void	ams_remove_event_mgr2(AmsSAP *sap)
{
	/*	Only prime thread can remove event manager, and it can
	 *	do so only when it is not, itself, the event manager.	*/

	if (sap && pthread_equal(pthread_self(), sap->primeThread)
	&& !pthread_equal(sap->eventMgr, sap->primeThread))
	{
		stopEventMgr(sap);
	}
}

void	ams_remove_event_mgr(AmsSAP *sap)
{
	if (validSap(sap))
	{
		LOCK_MIB;
		ams_remove_event_mgr2(sap);
		UNLOCK_MIB;
	}
}
