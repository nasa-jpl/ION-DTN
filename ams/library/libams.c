/*
	libams.c:	functions enabling the implementation of
			AMS-based applications.

	Author: Scott Burleigh, JPL

	Copyright (c) 2005, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
									*/
#include "amsP.h"

/*	Note that AMS diverges somewhat from exception handling policy
 *	that is implemented in most of the rest of ION: returning a
 *	value of -1 from an AMS function normally does NOT mean that
 *	an unrecoverable system error has been encountered and the
 *	task or process should terminate -- it simply means that the
 *	function encountered a condition that prevented its nominal
 *	and successful completion.
 *
 *	In part this is because AMS is different from other ION
 *	protocol implementations: it manages no shared storage at
 *	all (not even for the MIB) and even DRAM is shared only
 *	among the threads of a single AMS entity (configuration
 *	server, registrar, or application module).  This means that
 *	an error encountered in one function invocation does not
 *	imply a likely consequent failure in any other function of
 *	any other task, so the task only needs to terminate in the
 *	event that it simply cannot continue to function -- not to
 *	protect other tasks from failure.
 *
 *	AMS tasks properly terminate when they cannot initialize
 *	(for one reason or another), when they encounter socket or
 *	file I/O errors, when they find it impossible to allocate
 *	memory from the ION working memory pool, or at the direction
 *	of the user application.  Most other failures are simply
 *	reported and ignored.						*/

static int	ams_invite2(AmsSAP *sap, int roleNbr, int continuumNbr,
			int unitNbr, int subjectNbr, int priority,
			unsigned char flowLabel, AmsSequence sequence,
			AmsDiligence diligence);

/*			Privately defined event types.			*/
#define ACCEPTED_EVT	32
#define SHUTDOWN_EVT	33
#define ENDED_EVT	34
#define BREAK_EVT	35

/*	*	*	AAMS message marshaling functions	*	*/

typedef int		(*MarshalFn)(char **into, void *from, int length);
typedef struct
{
	char		*name;
	MarshalFn	function;
} MarshalRule;

typedef int		(*UnmarshalFn)(void **into, char *from, int length);
typedef struct
{
	char		*name;
	UnmarshalFn	function;
} UnmarshalRule;

#include "marshal.c"

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
	saddr	temp;
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
		CHKERR(elt);
		llcv_signal(eventsQueue, time_to_stop);
		return 0;
	}

	/*	A hack, which works in the absence of a Lyst user
	 *	data variable.  In order to make the current query
	 *	ID number accessible to an llcv condition function,
	 *	we stuff it into the "compare" function pointer in
	 *	the Lyst structure.					*/

	temp = (saddr) lyst_compare_get(eventsQueue->list);
	queryNbr = temp;
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
	CHKERR(elt);
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
	CHKERR(evt);
	evt->type = CRASH_EVT;
	memcpy(evt->value, text, textLength);
	evt->value[textLength] = '\0';
	if (enqueueAmsEvent(sap, evt, NULL, 0, 0, AmsMsgNone) < 0)
	{
		putErrmsg("Can't enqueue AMS crash event.", NULL);
		MRELEASE(evt);
		return -1;
	}

	return 0;
}

static int	enqueueAmsStubEvent(AmsSAP *sap, int eventType, int priority)
{
	AmsEvt	*evt;

	evt = (AmsEvt *) MTAKE(sizeof(AmsEvt));
	CHKERR(evt);
	evt->type = eventType;
	if (enqueueAmsEvent(sap, evt, NULL, 0, priority, AmsMsgNone) < 0)
	{
		putErrmsg("Can't enqueue AMS stub event.", NULL);
		MRELEASE(evt);
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

	if (sap == NULL)
	{
		return;
	}

	sap->state = AmsSapClosed;

	/*	Stop heartbeat, MAMS handler, and MAMS rcvr threads.	*/

	if (sap->haveHeartbeatThread)
	{
		pthread_end(sap->heartbeatThread);
		pthread_join(sap->heartbeatThread, NULL);
	}

	if (sap->haveMamsThread)
	{
		llcv_signal_while_locked(sap->mamsEventsCV, time_to_stop);
		pthread_join(sap->mamsThread, NULL);
	}

	if (sap->mamsTsif.ts)
	{
		sap->mamsTsif.ts->shutdownFn(sap->mamsTsif.sap);
		pthread_join(sap->mamsTsif.receiver, NULL);
		if (sap->mamsTsif.ept)
		{
			MRELEASE(sap->mamsTsif.ept);
		}
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
			RELEASE_CONTENT_SPACE(amsMsg->content);
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
	if (msg->continuumNbr < 1 || msg->continuumNbr > MAX_CONTIN_NBR
	|| sap->venture->msgspaces[msg->continuumNbr] == NULL)
	{
		writeMemoNote("[?] Received message from unknown continuum",
				itoa(msg->continuumNbr));
		return -1;
	}

	msg->unitNbr = (*(header + 4) << 8) + *(header + 5);
	if (msg->unitNbr < 0 || msg->unitNbr > MAX_UNIT_NBR
	|| sap->venture->units[msg->unitNbr] == NULL)
	{
		writeMemoNote("[?] Received message from unknown cell",
				itoa(msg->unitNbr));
		return -1;
	}

	msg->moduleNbr = (unsigned char) *(header + 6);
	if (msg->moduleNbr < 1 || msg->moduleNbr > MAX_MODULE_NBR)
	{
		writeMemoNote("[?] Received message from invalid-nbr module",
				itoa(msg->moduleNbr));
		return -1;
	}

	if (msg->continuumNbr != (_mib(NULL))->localContinuumNbr)
	{
		return 0;	/*	Can't get ultimate sender.	*/
	}

	if (((*sender) =
	sap->venture->units[msg->unitNbr]->cell->modules[msg->moduleNbr])->role
			== NULL)
	{
		writeMemoNote("[?] Received message from unknown module",
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
		writeMemoNote("[?] AMS message header incomplete",
				itoa(length));
		return -1;
	}

	if (*(msgBuffer + 2) & 0x80)	/*	Checksum present.	*/
	{
		deliveredContentLength = length - 18;
		if (deliveredContentLength < 0)
		{
			writeMemo("[?] AMS message truncated.");
			return -1;
		}

		memcpy((char *) &checksum, msgBuffer + (length - 2), 2);
		checksum = ntohs(checksum);
		if (checksum != computeAmsChecksum(msgBuffer, length - 2))
		{
			writeMemo("[?] Checksum failed, AMS msg discarded.");
			return -1;
		}
	}
	else				/*	No checksum.		*/
	{
		deliveredContentLength = length - 16;
	}

	if (getMsgSender(sap, msg, msgBuffer, sender) < 0)
	{
		return -1;
	}

	return deliveredContentLength;
}

static UnmarshalFn	findUnmarshalFn(Subject *subject)
{
	int	i;

	if (subject->nbr < 0			/*	RAMS		*/
	|| subject->unmarshalFnName == NULL)	/*	Not marshaled	*/
	{
		return NULL;
	}

	for (i = 0; i < unmarshalRulesCount; i++)
	{
		if (strcmp(unmarshalRules[i].name, subject->unmarshalFnName)
				== 0)
		{
			return unmarshalRules[i].function;
		}
	}

	return NULL;
}

static int	recoverMsgContent(AmsSAP *sap, AmsMsg *msg, Subject *subject)
{
	char		*newContent;
	int		newContentLength;
	UnmarshalFn	unmarshal;
	char		keyBuffer[512];
	int		keyBufferLength = sizeof keyBuffer;
	int		keyLength;

	/*	Decrypt content as necessary.				*/

	if (sap->role->nbr == 1			/*	RAMS		*/
		/*	If encrypted, leave it so for transmission.	*/
	|| subject->symmetricKeyName == NULL)	/*	not encrypted	*/
	{
		newContentLength = msg->contentLength;
		newContent = TAKE_CONTENT_SPACE(msg->contentLength);
		if (newContent == NULL)
		{
			writeMemoNote("[?] Can't copy AAMS msg content",
					itoa(msg->contentLength));
			msg->content = NULL;
			return -1;
		}

		memcpy(newContent, msg->content, msg->contentLength);
	}
	else
	{
		keyLength = sec_get_key(subject->symmetricKeyName,
				&keyBufferLength, keyBuffer);
		if (keyLength <= 0)
		{
			putErrmsg("Can't fetch symmetric key.",
					subject->symmetricKeyName);
			msg->content = NULL;
			return -1;
		}

		newContentLength = decryptUsingSymmetricKey(&newContent,
				keyBuffer, keyLength, msg->content,
				msg->contentLength);
		if (newContentLength == 0)
		{
			putErrmsg("Can't decrypt AAMS msg content.",
					subject->name);
			msg->content = NULL;
			return -1;
		}
	}

	msg->content = newContent;
	msg->contentLength = newContentLength;

	/*	Unmarshal content as necessary.				*/

	if (sap->role->nbr == 1)		/*	RAMS		*/
	{
		/*	If marshaled, leave it so for transmission.	*/

		unmarshal = NULL;
	}
	else
	{
		unmarshal = findUnmarshalFn(subject);
	}

	if (unmarshal == NULL)
	{
		return 0;	/*	Original content recovered.	*/
	}

	newContentLength = unmarshal((void **) &newContent, msg->content,
			msg->contentLength);
	if (newContentLength == 0)
	{
		putErrmsg("Can't unmarshal AAMS msg content.", subject->name);
		RELEASE_CONTENT_SPACE(msg->content);
		msg->content = NULL;
		return -1;
	}

	msg->content = newContent;
	msg->contentLength = newContentLength;
	return 0;
}

static int	handleMibUpdate(int contentLength, char *content)
{
	char	fileName[MAXPATHLEN];
	int	fd;
	int	result = 0;

	isprintf(fileName, MAXPATHLEN, "amsmib.%d", sm_GetUniqueKey());
	fd = iopen(fileName, O_WRONLY | O_CREAT, 0777);
	if (fd < 0)
	{
		putSysErrmsg("AMS can't open temporary MIB update file",
				fileName);
		return -1;
	}

	if (write(fd, content, contentLength) < contentLength)
	{
		putSysErrmsg("AMS can't write to temporary MIB update file",
				itoa(contentLength));
		result = -1;
	}

	close(fd);
	if (result < 0)
	{
		return result;
	}

	result = updateMib(fileName);
	unlink(fileName);
	return result;
}

int	enqueueAmsMsg(AmsSAP *sap, unsigned char *msgBuffer, int length)
{
	char	*msgContent = ((char *) msgBuffer) + 16;
	int	deliveredContentLength;
	AmsMsg	msg;
	Module	*sender;
	short	subjectNbr;
	Subject	*subject;
	LystElt	elt;
	char	*name;
	int	result;
	AmsEvt	*evt;

	CHKERR(sap);
	CHKERR(msgBuffer);
	CHKERR(length);
	deliveredContentLength = validateAmsMsg(sap, msgBuffer, length, &msg,
			&sender);
	if (deliveredContentLength < 0 || sender == NULL)
	{
		writeMemo("[?] Invalid AMS message.");
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
			writeMemo("[?] Invalid RAMS enclosure.");
			return -1;
		}

		/*	sender is NULL, because it's a foreign module .	*/
	}

	/*	Now working with an originally transmitted message.	*/
#if AMSDEBUG
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
			writeMemo("[?] Message destined for RAMS gateway \
received by non-RAMS-gateway module.");
			return -1;
		}

		/*	Receiving module is a RAMS gateway.		*/

		subjectNbr = 0 - msg.subjectNbr;
		if (subjectNbr > MAX_CONTIN_NBR)
		{
			writeMemoNote("[?] Received msg for invalid continuum",
					itoa(subjectNbr));
			return -1;
		}

		subject = sap->venture->msgspaces[subjectNbr];
	}
	else
	{
		if (msg.subjectNbr == 0 || msg.subjectNbr > MAX_SUBJ_NBR)
		{
			writeMemoNote("[?] Received msg on invalid subject",
					itoa(msg.subjectNbr));
			return -1;
		}

		subject = sap->venture->subjects[msg.subjectNbr];
	}

	if (subject == NULL)
	{
		writeMemoNote("[?] Received message on unknown subject",
				itoa(msg.subjectNbr));
		return -1;
	}

	if (msg.continuumNbr == (_mib(NULL))->localContinuumNbr)
	{
		if (subject->authorizedSenders != NULL)
		{
			/*	Must check sender's bonafides.		*/

			for (elt = lyst_first(subject->authorizedSenders);
					elt; elt = lyst_next(elt))
			{
				name = (char *) lyst_data(elt);
				result = strcmp(name, sender->role->name);
				if (result < 0)
				{
					continue;
				}

				if (result > 0)
				{
					elt = NULL;	/*	End.	*/
				}

				break;
			}

			if (elt == NULL)
			{
				writeMemoNote("[?] Got msg from unauthorized \
sender", sender->role->name);
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
						writeMemoNote("[?] Received \
unsolicited message.", subject->name);
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
		writeMemoNote("[?] AMS message truncated",
				itoa(deliveredContentLength));
		return -1;
	}

	if (msg.contentLength == 0)
	{
		msg.content = NULL;
	}
	else
	{
		msg.content = msgContent;
		if (recoverMsgContent(sap, &msg, subject) < 0)
		{
			return -1;
		}
	}

	/*	If message subject is "amsmib", handle it here.		*/

	if (subject->name && strcmp(subject->name, "amsmib") == 0)
	{
		return handleMibUpdate(msg.contentLength, msg.content);
	}

	/*	Create and enqueue event, signal application thread.	*/

	evt = (AmsEvt *) MTAKE(1 + sizeof(AmsMsg));
	CHKERR(evt);
	evt->type = AMS_MSG_EVT;
	memcpy(evt->value, (char *) &msg, sizeof(AmsMsg));
	return enqueueAmsEvent(sap, evt, msg.content, msg.contextNbr,
			msg.priority, msg.type);
}

static MarshalFn	findMarshalFn(Subject *subject)
{
	int	i;

	if (subject->nbr < 0			/*	RAMS		*/
	|| subject->marshalFnName == NULL)	/*	Not marshaled	*/
	{
		return NULL;
	}

	for (i = 0; i < marshalRulesCount; i++)
	{
		if (strcmp(marshalRules[i].name, subject->marshalFnName) == 0)
		{
			return marshalRules[i].function;
		}
	}

	return NULL;
}

static int	constructMessage(AmsSAP *sap, short subjectNbr, int priority,
			unsigned char flowLabel, int context, char **content,
			int *contentLength, unsigned char *header,
			AmsMsgType msgType)
{
	int		myContinNbr = (_mib(NULL))->localContinuumNbr;
	unsigned long	u8;
	unsigned char	u1;
	short		i2;
	Subject		*subject;
	MarshalFn	marshal;
	char		*newContent;
	int		newContentLength;
	char		keyBuffer[512];
	int		keyBufferLength = sizeof keyBuffer;
	int		keyLength;

	/*	First octet is two bits of version number (which is
	 *	always 00 for now) followed by two bits of message
	 *	type (unary, query, or reply) followed by four bits
	 *	of priority.						*/

	u8 = msgType;
	u1 = ((u8 << 4) & 0x30) + (priority & 0x0f);
	*header = u1;
	*(header + 1) = flowLabel;
	*(header + 2) = 0x80	/*	Checksum always present.	*/
			+ ((myContinNbr >> 8) & 0x0000007f);
	*(header + 3) = myContinNbr & 0x000000ff;
	*(header + 4) = (sap->unit->nbr >> 8) & 0x000000ff;
	*(header + 5) = sap->unit->nbr & 0x000000ff;
	*(header + 6) = sap->moduleNbr;
	*(header + 7) = 0;	/*	Reserved.			*/
	context = htonl(context);
	memcpy(header + 8, (char *) &context, 4);
	i2 = htons(subjectNbr);
	memcpy(header + 12, (char *) &i2, 2);
	if (*contentLength == 0)
	{
		*(header + 14) = 0;
		*(header + 15) = 0;
		return 0;
	}

	if (subjectNbr > 0)
	{
		subject = sap->venture->subjects[subjectNbr];
	}
	else
	{
		subject = sap->venture->msgspaces[0 - subjectNbr];
	}

	/*	Marshal content as necessary.				*/

	if (sap->role->nbr == 1)		/*	RAMS		*/
	{
		/*	If message needs marshaling, it's already done.	*/

		marshal = NULL;
	}
	else
	{
		marshal = findMarshalFn(subject);
	}

	if (marshal)
	{
		newContentLength = marshal(&newContent, (void *) *content,
				*contentLength);
		if (newContentLength == 0)
		{
			putErrmsg("Can't marshal AAMS msg content.",
					subject->name);
			return -1;
		}
	}
	else
	{
		newContentLength = *contentLength;
		newContent = MTAKE(*contentLength);
		if (newContent == NULL)
		{
			putErrmsg("Can't copy AAMS msg content.",
					itoa(*contentLength));
			return -1;
		}

		memcpy(newContent, *content, *contentLength);
	}

	*content = newContent;
	*contentLength = newContentLength;

	/*	Encrypt content as necessary.				*/

	if (sap->role->nbr == 1			/*	RAMS		*/
		/*	If message needs encryption, it's already done.	*/
	|| subject->symmetricKeyName == NULL)	/*	no encryption	*/
	{
		*(header + 14) = ((*contentLength) >> 8) & 0x000000ff;
		*(header + 15) = (*contentLength) & 0x000000ff;
		return 0;	/*	Content is ready to send.	*/
	}

	keyLength = sec_get_key(subject->symmetricKeyName, &keyBufferLength,
			keyBuffer);
	if (keyLength <= 0)
	{
		putErrmsg("Can't fetch symmetric key.",
				subject->symmetricKeyName);
		MRELEASE(*content);
		*content = NULL;
		return -1;
	}

	newContentLength = encryptUsingSymmetricKey(&newContent, keyBuffer,
			keyLength, *content, *contentLength);
	if (newContentLength == 0)
	{
		putErrmsg("Can't encrypt AAMS msg content.", subject->name);
		MRELEASE(*content);
		*content = NULL;
		return -1;
	}

	*content = newContent;
	*contentLength = newContentLength;
	*(header + 14) = ((*contentLength) >> 8) & 0x000000ff;
	*(header + 15) = (*contentLength) & 0x000000ff;
	return 0;
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
	CHKERR(evt);
	memcpy(evt->value, (char *) &msg, sizeof msg);
	evt->type = MSG_TO_SEND_EVT;
	if (enqueueMamsEvent(sap->mamsEventsCV, evt, NULL, 0))
	{
		putErrmsg("Can't enqueue message to registrar.", NULL);
		MRELEASE(evt);
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
	supplementLength += 4;		/*	Unit, module, role nbrs.*/
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
	CHKVOID(supplement);
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
		if (subjectNbr <= MAX_SUBJ_NBR
		&& (*subject = sap->venture->subjects[subjectNbr]) != NULL)
		{
			return 1;
		}
	}

	if (subjectNbr < 0)
	{
		if ((pseudoSubjectNbr = 0 - subjectNbr) <= MAX_CONTIN_NBR
		&& (*subject = sap->venture->msgspaces[pseudoSubjectNbr])
			!= NULL)
		{
			return 1;
		}
	}

	writeMemoNote("[?] Unknown message subject", itoa(subjectNbr));
	return 0;
}

static LystElt	findSubjOfInterest(AmsSAP *sap, Module *module,
			Subject *subject, LystElt *nextSubj)
{
	LystElt		elt;
	SubjOfInterest	*subj;

	/*	This function finds the SubjOfInterest containing
	 *	all XmitRules asserted by this module for the specified
	 *	subject, if any.					*/

#if AMSDEBUG
printf("subjects list length is %u.\n", lyst_length(module->subjects));
#endif
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
			LystElt *nextFan)
{
	LystElt		elt;
	FanModule	*fan;

	/*	This function finds the FanModule containing
	 *	all XmitRule asserted by this module for the specified
	 *	subject, if any.					*/

	if (nextFan) *nextFan = NULL;	/*	Default.		*/
	for (elt = lyst_first(subject->modules); elt; elt = lyst_next(elt))
	{
		fan = (FanModule *) lyst_data(elt);
		if (fan->module->unitNbr < module->unitNbr)
		{
			continue;
		}

		if (fan->module->unitNbr > module->unitNbr)
		{
			if (nextFan) *nextFan = elt;
			break;		/*	Same as end of list.	*/
		}

		/*	Found an FanModule in same unit.		*/

		if (fan->module->nbr < module->nbr)
		{
			continue;
		}

		if (fan->module->nbr > module->nbr)
		{
			if (nextFan) *nextFan = elt;
			break;		/*	Same as end of list.	*/
		}

		/*	Matched unit and module numbers.		*/

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
	CHKERR(evt);
	memcpy(evt->value, (char *) &notice, sizeof notice);
	evt->type = NOTICE_EVT;
	if (enqueueAmsEvent(sap, evt, NULL, 0, 2, AmsMsgNone) < 0)
	{
		putErrmsg("Can't enqueue notice.", NULL);
		MRELEASE(evt);
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
	int		amsMemory = getIonMemoryMgr();
	LystElt		elt;
	LystElt		nextSubj;
	SubjOfInterest	*subj;
	LystElt		nextFan;
	FanModule	*fan = NULL;
	Lyst		rules;
	LystElt		nextRule;
	XmitRule	*rule;
	char		*name;
	int		result;

	/*	Record the message reception relationship.		*/

	elt = findSubjOfInterest(sap, module, subject, &nextSubj);
	if (elt == NULL)	/*	New subject for this module.	*/
	{
		subj = (SubjOfInterest *) MTAKE(sizeof(SubjOfInterest));
		CHKERR(subj);
		subj->subject = subject;
		subj->subscriptions = lyst_create_using(amsMemory);
		CHKERR(subj->subscriptions);
		lyst_delete_set(subj->subscriptions, destroyXmitRule, NULL);
		subj->invitations = lyst_create_using(amsMemory);
		CHKERR(subj->invitations);
		lyst_delete_set(subj->invitations, destroyXmitRule, NULL);
		if (nextSubj)	/*	Insert before this point.	*/
		{
			elt = lyst_insert_before(nextSubj, subj);
		}
		else		/*	Insert at end of list.		*/
		{
			elt = lyst_insert_last(module->subjects, subj);
		}

		CHKERR(elt);

		/*	Also need to insert new FanModule for
		 *	this subject.					*/

		elt = findFanModule(sap, subject, module, &nextFan);
		if (elt)	/*	Should be NULL.			*/
		{
			putErrmsg("FanModules list out of sync!",
					subject->name);
			return -1;
		}

		fan = (FanModule *) MTAKE(sizeof(FanModule));
		CHKERR(fan);
		fan->module = module;
		fan->subj = subj;
		if (nextFan)	/*	Insert before this point.	*/
		{
			subj->fanElt = lyst_insert_before(nextFan, fan);
		}
		else		/*	Insert at end of list.		*/
		{
			subj->fanElt = lyst_insert_last(subject->modules, fan);
		}

		CHKERR(subj->fanElt);
	}
	else	/*	Module already has interest in this subject.	*/
	{
		subj = (SubjOfInterest *) lyst_data(elt);
	}

	rules = (ruleType == SUBSCRIPTION ?
			subj->subscriptions : subj->invitations);
#if AMSDEBUG
printf("ruleType = %d, subj = %lu, subject = %d, rules list is %lu.\n",
ruleType, (unsigned long) subj, subj->subject->nbr, (unsigned long) rules);
#endif
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
	CHKERR(rule);
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

	CHKERR(elt);
#if AMSDEBUG
printf("...inserted rule in rules list %lu...\n", (unsigned long) rules);
#endif

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
			name = (char *) lyst_data(elt);
			result = strcmp(name, module->role->name);
			if (result < 0)
			{
				continue;
			}
			
			if (result == 0)
			{
				rule->flags |= XMIT_IS_OKAY;
			}

			break;
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
	int		myContinNbr = (_mib(NULL))->localContinuumNbr;
	Subject		*subject;
	LystElt		elt;
	AmsEndpoint	*point;

	if (subjectNbr == 0)
	{
		if (ruleType == SUBSCRIPTION)
		{
			if (domainContinuumNbr != myContinNbr)
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
		domainContinuumNbr = myContinNbr;
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
		 *	the asserting module, so skip this assertion.	*/

			return 0;
	}

	/*	Subject & vector are okay; messages are deliverable to
	 *	the asserting module.					*/

	if (noteAssertion(sap, module, subject, domainRoleNbr,
			domainContinuumNbr, domainUnitNbr, priority,
			flowLabel, point, ruleType, flag) < 0)
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
		writeMemo("[?] MAMS assertion was truncated.");
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

static int	processDeclaration(AmsSAP *sap, Module *module,
			int bytesRemaining, char *cursor, int flag)
{
	int		assertionCount;
	unsigned short	u2;

	if (bytesRemaining < 2)
	{
		writeMemo("[?] Declaration lacks subscription count.");
		return -1;
	}

	memcpy((char *) &u2, cursor, 2);
	cursor += 2;
	bytesRemaining -= 2;
	u2 = ntohs(u2);
	assertionCount = u2;
	while (assertionCount > 0)
	{
#if AMSDEBUG
printf("Parsing decl subscription with %d bytes remaining.\n", bytesRemaining);
#endif
		if (parseAssertion(sap, module, &bytesRemaining, &cursor,
				SUBSCRIPTION, flag) < 0)
		{
			writeMemo("[?] Error parsing subscription.");
			return -1;
		}

		assertionCount--;
	}

	if (bytesRemaining < 2)
	{
		writeMemo("[?] Declaration lacks invitation count.");
		return -1;
	}

	memcpy((char *) &u2, cursor, 2);
	cursor += 2;
	bytesRemaining -= 2;
	u2 = ntohs(u2);
	assertionCount = u2;
	while (assertionCount > 0)
	{
#if AMSDEBUG
printf("Parsing decl invitation with %d bytes remaining.\n", bytesRemaining);
#endif
		if (parseAssertion(sap, module, &bytesRemaining, &cursor,
				INVITATION, flag) < 0)
		{
			writeMemo("[?] Error parsing invitation.");
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
			if (domainContinuumNbr ==
					(_mib(NULL))->localContinuumNbr)
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

static int	parseAmsEndpoint(Module *module, int *bytesRemaining,
			char **cursor, char **tsname, int *eptLen, char **ept)
{
	int	gotIt = 0;

	*tsname = *cursor;
	*eptLen = 0;
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
				(*eptLen)++;
			}
		}

		(*cursor)++;
		(*bytesRemaining)--;
		if (gotIt)
		{
			return 0;
		}
	}

	writeMemo("[?] Incomplete AMS endpoint name.");
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
	CHKERR(ep);
	ep->ept = MTAKE(eptLength + 1);
	CHKERR(ep->ept);
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
		writeMemoNote("[?] Can't parse endpoint name", ept);
		MRELEASE(ep->ept);
		MRELEASE(ep);
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

	CHKERR(elt);
	return 0;
}

static int	parseDeliveryVector(Module *module, int *bytesRemaining,
			char **cursor)
{
	AmsMib		*mib = _mib(NULL);
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
		writeMemo("[?] Delivery vector lacks point count.");
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
			writeMemo("[?] Error parsing delivery vector.");
			return -1;
		}

		pointCount--;
		if (endpointInserted)
		{
			continue;	/*	Saved best fit already.	*/
		}

		/*	Look for a tsif that can send to this point.	*/

		lockMib();
		for (i = 0; i < mib->transportServiceCount; i++)
		{
			ts = &(mib->transportServices[i]);
			if (strcmp(ts->name, tsname) == 0)
			{
				/*	We can send to this point.	*/

				result = insertAmsEndpoint(module, vectorNbr,
						ts, eptLength, ept);
				if (result < 0)
				{
					writeMemo("[?] AMS err inserting ept.");
					unlockMib();
					return -1;
				}

				endpointInserted = 1;
				break;
			}
		}

		unlockMib();
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
		writeMemo("[?] Contact summary lacks delivery vector count.");
		return -1;
	}

	vectorCount = **cursor;
	(*cursor)++;
	(*bytesRemaining)--;
	while (vectorCount > 0)
	{
		if (parseDeliveryVector(module, bytesRemaining, cursor))
		{
			writeMemo("[?] Error parsing delivery vector.");
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

	if (roleNbr < 1 || roleNbr > MAX_ROLE_NBR)
	{
		writeMemoNote("[?] role nbr invalid", itoa(roleNbr));
		return -1;
	}

	ept = parseString(cursor, bytesRemaining, &eptLength);
	if (ept == NULL)
	{
		writeMemo("[?] No MAMS endpoint ID string.");
		return -1;
	}

	role = sap->venture->roles[roleNbr];
	cell = sap->venture->units[unitNbr]->cell;
	module = cell->modules[moduleNbr];
	if (module->role == NULL)	/*	Unannounced until now.	*/
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
		putErrmsg("Can't enqueue notice.", NULL);
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

#if AMSDEBUG
printf("Module '%d' got msg of type %d.\n", sap->role->nbr, msg->type);
#endif
	switch (msg->type)
	{
	case heartbeat:
		sap->heartbeatsMissed = 0;
		return;

	case you_are_dead:
		if (enqueueAmsCrash(sap,
			"Killed by registrar; imputed crash.") < 0)
		{
			putErrmsg("Can't enqueue AMS crash.", NULL);
		}

		return;

	case I_am_starting:
		if (msg->supplementLength < 1)
		{
			putErrmsg("I_am_starting lacks MAMS endpoint.", NULL);
			return;
		}

		if (parseModuleId(msg->memo, &roleNbr, &unitNbr, &moduleNbr))
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

		if (parseModuleId(msg->memo, &roleNbr, &unitNbr, &moduleNbr))
		{
			putErrmsg("module_has_started memo field invalid.",
					NULL);
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

			memcpy((char *) &u2, cursor, 2);
			unitNbr = ntohs(u2);
			moduleNbr = (unsigned char) *(cursor + 2);
			roleNbr = (unsigned char) *(cursor + 3);
			bytesRemaining -= 4;
			cursor += 4;
			if (roleNbr < 1 || roleNbr > MAX_ROLE_NBR
					|| unitNbr > MAX_UNIT_NBR
					|| moduleNbr < 1
					|| moduleNbr > MAX_MODULE_NBR)
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

			u4--;
		}

		return;

	case declaration:
		if (parseModuleId(msg->memo, &roleNbr, &unitNbr, &moduleNbr))
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
		if (parseModuleId(msg->memo, &roleNbr, &unitNbr, &moduleNbr))
		{
			putErrmsg("subscribe memo field invalid.", NULL);
			return;
		}

		unit = sap->venture->units[unitNbr];
		module = unit->cell->modules[moduleNbr];
		if (module->role == NULL)
		{
			return;	/*	Registration not yet complete.	*/
		}

		bytesRemaining = msg->supplementLength;
		cursor = msg->supplement;
#if AMSDEBUG
printf("Parsing new subscription with %d bytes remaining.\n", bytesRemaining);
#endif
		if (parseAssertion(sap, module, &bytesRemaining, &cursor,
				SUBSCRIPTION, 0) < 0)
		{
			putErrmsg("Error parsing subscription.", NULL);
		}

		return;

	case unsubscribe:
		if (msg->supplementLength < CANCEL_LEN)
		{
			putErrmsg("unsubscribe lacks cancellation.", NULL);
			return;
		}

		if (parseModuleId(msg->memo, &roleNbr, &unitNbr, &moduleNbr))
		{
			putErrmsg("unsubscribe memo field invalid.", NULL);
			return;
		}

		unit = sap->venture->units[unitNbr];
		module = unit->cell->modules[moduleNbr];
		if (module->role == NULL)
		{
			return;	/*	Registration not yet complete.	*/
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
		if (parseModuleId(msg->memo, &roleNbr, &unitNbr, &moduleNbr))
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
		if (unitNbr > MAX_UNIT_NBR)
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
			if (module->role == NULL)	/*	Unknown.*/
			{
				/*	Nothing to confirm.		*/

				continue;
			}

			module->confirmed = 1;	/*	Still there.	*/
		}

		/*	Now update array of modules in cell.		*/

		moduleCount = 0;
		for (moduleNbr = 1; moduleNbr < MAX_MODULE_NBR; moduleNbr++)
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
				if (cancelUnconfirmedAssertions(sap, module)
						< 0)
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
			putErrmsg("module_status lacks module states count.",
					NULL);
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

			memcpy((char *) &u2, cursor, 2);
			unitNbr = ntohs(u2);
			moduleNbr = (unsigned char) *(cursor + 2);
			roleNbr = (unsigned char) *(cursor + 3);
			bytesRemaining -= 4;
			cursor += 4;
			if (roleNbr < 1 || roleNbr > MAX_ROLE_NBR
					|| unitNbr > MAX_UNIT_NBR
					|| moduleNbr < 1
					|| moduleNbr < MAX_MODULE_NBR)
			{
				putErrmsg("module_status module ID invalid.",
						NULL);
				return;
			}

			if (noteModule(sap, roleNbr, unitNbr, moduleNbr, msg,
					&bytesRemaining, &cursor) < 0)
			{
				putErrmsg("Failed handling module_status.",
						NULL);
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
		if (parseModuleId(msg->memo, &roleNbr, &unitNbr, &moduleNbr))
		{
			putErrmsg("invite memo field invalid.", NULL);
			return;
		}

		unit = sap->venture->units[unitNbr];
		module = unit->cell->modules[moduleNbr];
		if (module->role == NULL)
		{
			return;	/*	Registration not yet complete.	*/
		}

		bytesRemaining = msg->supplementLength;
		cursor = msg->supplement;
#if AMSDEBUG
printf("Parsing new invitation with %d bytes remaining.\n", bytesRemaining);
#endif
		if (parseAssertion(sap, module, &bytesRemaining, &cursor,
				INVITATION, 0) < 0)
		{
			putErrmsg("Error parsing invitation.", NULL);
		}

		return;

	case disinvite:
		if (msg->supplementLength < CANCEL_LEN)
		{
			putErrmsg("disinvite lacks cancellation.", NULL);
			return;
		}

		if (parseModuleId(msg->memo, &roleNbr, &unitNbr, &moduleNbr))
		{
			putErrmsg("disinvite memo field invalid.", NULL);
			return;
		}

		unit = sap->venture->units[unitNbr];
		module = unit->cell->modules[moduleNbr];
		if (module->role == NULL)
		{
			return;	/*	Registration not yet complete.	*/
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
	AmsMib		*mib = _mib(NULL);
	long		queryNbr;
	saddr		temp;
	MamsEndpoint	*ep;
	char		*ept;
	int		eptLen;
	LystElt		elt;
	AmsEvt		*evt;
	MamsMsg		*msg;
	int		result;

	if (sap->csEndpoint == NULL)
	{
		lockMib();
		if (lyst_length(mib->csEndpoints) == 0)
		{
			putErrmsg("Configuration server endpoints list empty.",
					NULL);
			unlockMib();
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

		unlockMib();
		ep = (MamsEndpoint *) lyst_data(sap->csEndpointElt);
	}
	else
	{
		ep = sap->csEndpoint;
	}

	ept = sap->mamsTsif.ept;	/*	Own MAMS endpoint name.	*/
	eptLen = strlen(ept) + 1;
	queryNbr = time(NULL);
	temp = queryNbr;
	lyst_compare_set(sap->mamsEvents, (LystCompareFn) temp);
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
		unlockMib();
		result = llcv_wait(sap->mamsEventsCV, llcv_reply_received,
				N2_INTERVAL * 1000000);
		lockMib();
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
				recycleEvent(evt);
				writeMemo("[?] No registrar for this cell.");
				lyst_compare_set(sap->mamsEvents, NULL);
				return -1;

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
	unsigned char	modules[MAX_MODULE_NBR];
	int		contactSummaryLength;
	int		declarationLength;
	Module		*module;
	char		*supplement;
	int		supplementLength;
	char		*cursor;
	long		queryNbr;
	saddr		temp;
	int		result;
	LystElt		elt;
	AmsEvt		*evt;
	MamsMsg		*msg;

	/*	Build cell status structure to enable reconnect.	*/

	for (i = 1; i <= MAX_MODULE_NBR; i++)
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
		+ 1			/*	Length of modules array.*/
		+ moduleCount;		/*	Array of modules.	*/
	supplement = MTAKE(supplementLength);
	CHKERR(supplement);
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
	temp = queryNbr;
	lyst_compare_set(sap->mamsEvents, (LystCompareFn) temp);
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
		unlockMib();
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

		lockMib();
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
					putErrmsg("Can't enqueue crash.", NULL);
				}

				unlockMib();
				lyst_compare_set(sap->mamsEvents, NULL);
				return 0;

			case reconnected:
				sap->heartbeatsMissed = 0;
				recycleEvent(evt);
				unlockMib();
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
			unlockMib();
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
			reasonString = _rejectionMemos(reasonCode);
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
		writeMemo("[?] Got truncated you_are_in.");
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
	saddr	temp;
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
		writeMemoNote("[?] Contact summary too long",
				itoa(supplementLength));
		return -1;
	}

	supplement = MTAKE(supplementLength);
	CHKERR(supplement);
	loadContactSummary(sap, supplement, supplementLength);
	queryNbr = time(NULL);
	temp = queryNbr;
	lyst_compare_set(sap->mamsEvents, (LystCompareFn) temp);
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
		unlockMib();
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

		lockMib();
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
				unlockMib();
				lyst_compare_set(sap->mamsEvents, NULL);
				return 0;

			case you_are_in:

				/*	Post event to main thread, to
				 *	terminate ams_register2.	*/

				result = process_you_are_in(sap, msg);
				recycleEvent(evt);
				unlockMib();
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
			unlockMib();
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
	LystElt		elt;
	AmsEvt		*evt;
	int		result;
#ifndef mingw
	sigset_t	signals;

	sigfillset(&signals);
	pthread_sigmask(SIG_BLOCK, &signals, NULL);
#endif

	/*	MAMS thread starts off in Unregistered state and
	 *	stays there until registration is complete.  Then
	 *	it enters Registered state, in which it responds to
	 *	MAMS messages from the registrar and other modules.
	 *	Upon unregistration, erasure of the SAP causes the
	 *	MAMS thread to terminate.				*/

	while (1)	/*	Registration event loop.		*/
	{
		lockMib();
		result = getModuleNbr(sap);
		unlockMib();
		if (result < 0)		/*	Unrecoverable failure.	*/
		{
			putErrmsg("Can't register module.", NULL);
			if (enqueueAmsCrash(sap, "Can't register.") < 0)
			{
				putErrmsg("Can't enqueue crash.", NULL);
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
				putErrmsg("Can't enqueue crash.", NULL);
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
				lockMib();
				processMamsMsg(sap, evt);
				unlockMib();
			}

			recycleEvent(evt);
			continue;

		case MSG_TO_SEND_EVT:
			lockMib();
			result = sendMsgToRegistrar(sap, evt);
			unlockMib();
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
				putErrmsg("Can't enqueue stub event.", NULL);
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

#ifndef mingw
	sigset_t	signals;

	sigfillset(&signals);
	pthread_sigmask(SIG_BLOCK, &signals, NULL);
#endif
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

		lockMib();
		if (sap->heartbeatsMissed == 3)
		{
			clearMamsEndpoint(sap->rsEndpoint);
		}

		result = enqueueMsgToRegistrar(sap, heartbeat, sap->moduleNbr,
				0, NULL);
		sap->heartbeatsMissed++;
		unlockMib();
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
	AmsMib		*mib = _mib(NULL);
	int		amsMemory = getIonMemoryMgr();
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
	Subject		*amsmibSubject;
	AppRole		*amsmibRole;

	CHKERR(applicationName);
	CHKERR(authorityName);
	CHKERR(unitName);
	CHKERR(roleName);
	CHKERR(module);
	CHKERR(applicationName);
	CHKERR(applicationName);
	CHKERR(applicationName);
	CHKERR(applicationName);
	CHKERR(applicationName);
	length = strlen(applicationName);
	CHKERR(length > 0);
	CHKERR(length <= MAX_APP_NAME);
	length = strlen(authorityName);
	CHKERR(length > 0);
	CHKERR(length <= MAX_AUTH_NAME);
	length = strlen(unitName);
	CHKERR(length <= MAX_UNIT_NAME);
	length = strlen(roleName);
	CHKERR(length > 0);
	CHKERR(length <= MAX_ROLE_NAME);

	/*	Start building SAP structure.				*/

	sap = (AmsSAP *) MTAKE(sizeof(AmsSAP));
	CHKERR(sap);
	*module = sap;
	memset((char *) sap, 0, sizeof(AmsSAP));
	sap->state = AmsSapClosed;
	sap->primeThread = pthread_self();
	sap->eventMgr = sap->primeThread;
	sap->authorizedEventMgr = sap->primeThread;

	/*	Validate registration parameters.			*/

	for (i = 1; i <= MAX_VENTURE_NBR; i++)
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

	if (i > MAX_VENTURE_NBR)
	{
		isprintf(ventureName, sizeof ventureName, "%s(%s)",
				applicationName, authorityName);
		putErrmsg("Can't register: no such message space.",
				ventureName);
		return -1;
	}

	sap->venture = venture;
	for (i = 0; i <= MAX_UNIT_NBR; i++)
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

	if (i > MAX_UNIT_NBR)
	{
		putErrmsg("Can't register: no such unit in message space.",
				unitName);
		return -1;
	}

	sap->unit = unit;
	sap->rsEndpoint = &(unit->cell->mamsEndpoint);
	for (i = 1; i <= MAX_ROLE_NBR; i++)
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

	if (i > MAX_ROLE_NBR)
	{
		putErrmsg("Can't register: role name invalid for application.",
				roleName);
		return -1;
	}

	sap->role = role;

	/*	Initialize module state data structures.  First, lists.	*/

	sap->mamsEvents = lyst_create_using(amsMemory);
	sap->amsEvents = lyst_create_using(amsMemory);
	sap->delivVectors = lyst_create_using(amsMemory);
	sap->subscriptions = lyst_create_using(amsMemory);
	sap->invitations = lyst_create_using(amsMemory);
	CHKERR(sap->mamsEvents);
	CHKERR(sap->amsEvents);
	CHKERR(sap->delivVectors);
	CHKERR(sap->subscriptions);
	CHKERR(sap->invitations);
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

	if (pthread_begin(&(mtsif->receiver), NULL,
			mib->pts->mamsReceiverFn, mtsif,
			"libams_mams_tsif_receiver"))
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

		if (pthread_begin(&(tsif->receiver), NULL,
				tsif->ts->amsReceiverFn, tsif, "libams_tsif_receiver"))
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
			CHKERR(vector);
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
			CHKERR(vector->interfaces);

			/*	No need for deletion function on
			 *	vector->interfaces: these structures
			 *	are pointed-to by the SAP's
			 *	AmsInterfaces list as well, and are
			 *	deleted when that list is destroyed.	*/

			elt = lyst_insert_last(sap->delivVectors, vector);
			CHKERR(elt);
		}

		/*	Got matching vector; append interface for this
		 *	transport service to it.			*/

		if (lyst_length(vector->interfaces) > 15)
		{
			putErrmsg("Module's vector has > 15 interfaces.",
					itoa(vector->nbr));
			return -1;
		}

		elt = lyst_insert_last(vector->interfaces, tsif);
		CHKERR(elt);
	}

	/*	Create the auxiliary module threads: heartbeat, MAMS.	*/

	if (pthread_begin(&(sap->heartbeatThread), NULL, heartbeatMain,
		sap, "libams_sap_heartbeat"))
	{
		putSysErrmsg("Can't spawn sap heartbeat thread", NULL);
		return -1;
	}

	sap->haveHeartbeatThread = 1;
	if (pthread_begin(&(sap->mamsThread), NULL, mamsMain,
		sap, "libams_sap_mams"))
	{
		putSysErrmsg("Can't spawn sap MAMS thread", NULL);
		return -1;
	}

	sap->haveMamsThread = 1;

	/*	Wait for MAMS thread to complete registration dialogue.	*/

	while (1)
	{
		unlockMib();
		result = llcv_wait(sap->amsEventsCV, llcv_lyst_not_empty,
					LLCV_BLOCKING);
		lockMib();
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

	if (ams_invite2(sap, 1, THIS_CONTINUUM, 0,
			(0 - mib->localContinuumNbr), 8, 0, AmsArrivalOrder,
			AmsBestEffort) < 0)
	{
		putErrmsg("Can't invite RAMS enclosure messages.", NULL);
		return -1;
	}

	/*	Finally, if subject "amsmib" is defined in the MIB,
	 *	then invite messages on that subject.			*/

	amsmibSubject = lookUpSubject(sap->venture, "amsmib");
	amsmibRole = lookUpRole(sap->venture, "amsmib");
	if (amsmibSubject && amsmibRole)
	{
		if (ams_invite2(sap, amsmibRole->nbr, ALL_CONTINUA, 0,
				amsmibSubject->nbr, 8, 0, AmsArrivalOrder,
				AmsBestEffort) < 0)
		{
			putErrmsg("Can't invite MIB update messages.", NULL);
			return -1;
		}
	}

	return 0;
}

int	ams_register(char *mibSource, char *tsorder, char *applicationName,
		char *authorityName, char *unitName, char *roleName,
		AmsModule *module)
{
	AmsMib	*mib;
	int	result;

	/*	Load Management Information Base as necessary.		*/

	oK(_mib(NULL));
	mib = loadMib(mibSource);
	if (mib == NULL)
	{
		putErrmsg("AMS can't load MIB.", mibSource);
		return -1;
	}

	*module = NULL;
	lockMib();
	result = ams_register2(applicationName, authorityName, unitName,
			roleName, tsorder, module);
	unlockMib();
	if (result == 0)		/*	Succeeded.		*/
	{
		(*module)->state = AmsSapOpen;
	}
	else
	{
		eraseSAP(*module);
		*module = NULL;
		unloadMib();
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

	CHKERR(sap);

	/*	Only prime thread can unregister.			*/

	if (!pthread_equal(pthread_self(), sap->primeThread))
	{
		writeMemo("[?] Only prime thread can unregister.");
		return -1;
	}

	unlockMib();
	ams_remove_event_mgr(sap);
	lockMib();
	result = enqueueMsgToRegistrar(sap, I_am_stopping,
		computeModuleId(sap->role->nbr, sap->unit->nbr, sap->moduleNbr),
		0, NULL);

	/*	Wait for MAMS thread to finish dealing with all
	 *	currently enqueued events, then complete shutdown.	*/

	if (enqueueMamsStubEvent(sap->mamsEventsCV, SHUTDOWN_EVT) < 0)
	{
		unlockMib();
		putErrmsg("Crashed AMS service.", NULL);
		return 0;
	}

	while (1)
	{
		unlockMib();
		result = llcv_wait(sap->amsEventsCV, llcv_lyst_not_empty,
					LLCV_BLOCKING);
		if (result < 0)
		{
			putErrmsg("Crashed AMS service.", NULL);
			return 0;
		}

		lockMib();
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
		unlockMib();
		break;
	}

	return 0;
}

int	ams_unregister(AmsSAP *sap)
{
	int	result;

	lockMib();
	result = ams_unregister2(sap);
	unlockMib();
	if (result == 0)
	{
		eraseSAP(sap);
		writeMemo("[i] AMS service terminated.");
		unloadMib();
	}

	return result;
}

static int	validSap(AmsSAP *sap)
{
	CHKZERO(sap);
	if (sap->state != AmsSapOpen)
	{
		return 0;
	}

	return 1;
}

static char	*ams_get_role_name2(AmsSAP *sap, int unitNbr, int moduleNbr)
{
	Unit	*unit;
	Module	*module;

	if (unitNbr >= 0 && unitNbr <= MAX_UNIT_NBR
	&& moduleNbr > 0 && moduleNbr <= MAX_MODULE_NBR)
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
		lockMib();
		result = ams_get_role_name2(sap, unitNbr, moduleNbr);
		unlockMib();
	}

	return result;
}

Lyst	ams_list_msgspaces(AmsSAP *sap)
{
	Lyst	msgspaces = NULL;
	int	i;
	Subject	**msgspace;
	int	msgspaceNbr;
	saddr	temp;

	if (validSap(sap))
	{
		lockMib();
		msgspaces = lyst_create_using(getIonMemoryMgr());
		if (msgspaces)
		{
			for (i = 0, msgspace = sap->venture->msgspaces;
					i <= MAX_CONTIN_NBR; i++, msgspace++)
			{
				if (*msgspace != NULL)
				{
					msgspaceNbr = 0 - (*msgspace)->nbr;
					temp = msgspaceNbr;
					if (lyst_insert_last(msgspaces,
							(void *) temp) == NULL)
					{
						lyst_destroy(msgspaces);
						msgspaces = NULL;
						break;	/*	Loop.	*/
					}
				}
			}
		}

		unlockMib();
	}

	return msgspaces;
}

int	ams_msgspace_is_neighbor(AmsSAP *sap, int continuumNbr)
{
	Subject	*msgspace;

	if (sap == NULL || continuumNbr < 1 || continuumNbr > MAX_CONTIN_NBR)
	{
		return 0;
	}

	msgspace = sap->venture->msgspaces[continuumNbr];
	if (msgspace == NULL)
	{
		return 0;
	}

	return msgspace->isNeighbor;
}

int	ams_get_continuum_nbr()
{
	return (_mib(NULL))->localContinuumNbr;
}

int	ams_rams_net_is_tree(AmsSAP *sap)
{
	CHKERR(sap);
	return sap->venture->ramsNetIsTree;
}

int	ams_subunit_of(AmsSAP *sap, int argUnitNbr, int refUnitNbr)
{
	int	result = 0;

	if (validSap(sap))
	{
		lockMib();
		result = subunitOf(sap, argUnitNbr, refUnitNbr);
		unlockMib();
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
	&& rule->continuumNbr != (_mib(NULL))->localContinuumNbr)
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
	long	longModuleNbr = moduleNbr;
	saddr	temp;

	for (elt = lyst_first(recipients); elt; elt = lyst_next(elt))
	{
		temp = (saddr) lyst_data(elt);
		if (longModuleNbr == temp)
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
	saddr		temp;

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
			continue;	/*	Don't send module copy.	*/
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
			temp = fan->module->nbr;
			lyst_insert_last(recipients, (void *) temp);
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
	int		result;

	CHKERR(sap);
	CHKERR(subjectIsValid(sap, subjectNbr, &subject));
	CHKERR(priority >= 0);
	CHKERR(priority < NBR_OF_PRIORITY_LEVELS);
	CHKERR(contentLength >= 0);
	CHKERR(contentLength <= MAX_AMS_CONTENT);

	/*	Knowing subject, construct message header & encrypt.	*/

	if (constructMessage(sap, subjectNbr, priority, flowLabel, context,
			&content, &contentLength, (unsigned char *) amsHeader,
			AmsMsgUnary) < 0)
	{
		return -1;
	}

	protectedBits = amsHeader[0] & 0xf0;
	recipients = lyst_create_using(getIonMemoryMgr());
	CHKERR(recipients);

	/*	Now send a copy of the message to every subscriber
	 *	that has posted at least one subscription whose domain
	 *	includes the local module.				*/

	if (sendToSubscribers(sap, subject, priority, flowLabel, protectedBits,
			amsHeader, headerLength, content, contentLength,
			recipients) < 0)
	{
		lyst_destroy(recipients);
		MRELEASE(content);
		return -1;
	}

	/*	Finally, send a copy of the message to every subscriber
	 *	that has posted at least one subscription for "all
	 *	subjects".						*/

	subject = sap->venture->subjects[ALL_SUBJECTS];
	result = sendToSubscribers(sap, subject, priority, flowLabel,
			protectedBits, amsHeader, headerLength, content,
			contentLength, recipients);
	lyst_destroy(recipients);
	MRELEASE(content);
	return result;
}

int	ams_publish(AmsSAP *sap, int subjectNbr, int priority,
		unsigned char flowLabel, int contentLength, char *content,
		int context)
{
	int	result = -1;

	if (validSap(sap))
	{
		CHKERR(subjectNbr > 0);
		lockMib();
		result = ams_publish2(sap, subjectNbr, priority, flowLabel,
				contentLength, content, context);
		unlockMib();
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
			int priority, int flowLabel, DeliveryVector *vector,
			LystElt *ruleElt)
{
	Lyst	rules;
	LystElt	elt;
	LystElt	nextRule;
	MsgRule	*rule;

	*ruleElt = NULL;
	rules = (ruleType == SUBSCRIPTION ?
			sap->subscriptions : sap->invitations);
	elt = findMsgRule(sap, rules, subject->nbr, roleNbr, continuumNbr,
			unitNbr, &nextRule);
	if (elt)	/*	Already have a rule for this subject.	*/
	{
		writeMemoNote("[?] Already have this rule", subject->name);
		return 0;
	}

	/*	Must insert new message rule structure.			*/

	rule = (MsgRule *) MTAKE(sizeof(MsgRule));
	CHKERR(rule);
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
		putErrmsg("Can't add rule.", subject->name);
		MRELEASE(rule);
		return -1;
	}

	*ruleElt = elt;
	return 0;
}

static int	ams_invite2(AmsSAP *sap, int roleNbr, int continuumNbr,
			int unitNbr, int subjectNbr, int priority,
			unsigned char flowLabel, AmsSequence sequence,
			AmsDiligence diligence)
{
	int		myContinNbr = (_mib(NULL))->localContinuumNbr;
	Subject		*subject;
	DeliveryVector	*vector;
	LystElt		elt;
	char		*assertion;
	char		*cursor;

	if (priority == 0)
	{
		priority = 8;		/*	Default.		*/
	}
	else
	{
		CHKERR(priority > 0);
		CHKERR(priority < NBR_OF_PRIORITY_LEVELS);
	}

	if (continuumNbr == THIS_CONTINUUM)
	{
		continuumNbr = myContinNbr;
	}

	if (subjectNbr == 0)	/*	Messages on all subjects.	*/
	{
		if (continuumNbr == myContinNbr)
		{
			subject = sap->venture->subjects[ALL_SUBJECTS];
		}
		else
		{
			writeMemoNote("[?] 'All subjects' invitation is \
limited to local continuum", itoa(continuumNbr));
			return -1;
		}
	}
	else
	{
		CHKERR(subjectIsValid(sap, subjectNbr, &subject));
	}

	vector = lookUpDeliveryVector(sap, sequence, diligence);
	if (vector == NULL)
	{
		writeMemo("[?] Have no endpoints for reception at this QOS.");
		return -1;
	}

	if (addMsgRule(sap, INVITATION, subject, roleNbr, continuumNbr,
			unitNbr, priority, flowLabel, vector, &elt) < 0)
	{
		return -1;
	}

	if (elt == NULL)	/*	Pre-existing rule.		*/
	{
		return 0;
	}

	assertion = MTAKE(INVITE_LEN);
	CHKERR(assertion);
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
		lockMib();
		result = ams_invite2(sap, roleNbr, continuumNbr, unitNbr,
			subjectNbr, priority, flowLabel, sequence, diligence);
		unlockMib();
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
		writeMemoNote("[?] Rule to remove not found", subject->name);
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
		continuumNbr = (_mib(NULL))->localContinuumNbr;
	}

	CHKERR(subjectIsValid(sap, subjectNbr, &subject));
	if (removeMsgRule(sap, INVITATION, subject, roleNbr, continuumNbr,
			unitNbr) < 0)
	{
		return 0;		/*	Redundant but okay.	*/
	}

	cancellation = MTAKE(CANCEL_LEN);
	CHKERR(cancellation);
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
			sap->moduleNbr), CANCEL_LEN, (char *) cancellation) < 0)
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
		lockMib();
		result = ams_disinvite2(sap, roleNbr, continuumNbr, unitNbr,
				subjectNbr);
		unlockMib();
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
	int		myContinNbr = (_mib(NULL))->localContinuumNbr;
	Subject		*subject;
	DeliveryVector	*vector;
	LystElt		elt;
	char		*assertion;
	char		*cursor;

	if (priority == 0)
	{
		priority = 8;		/*	Default.		*/
	}
	else
	{
		CHKERR(priority > 0);
		CHKERR(priority < NBR_OF_PRIORITY_LEVELS);
	}

	if (continuumNbr == THIS_CONTINUUM)
	{
		continuumNbr = myContinNbr;
	}

	if (subjectNbr == 0)	/*	Messages on all subjects.	*/
	{
		if (continuumNbr == myContinNbr)
		{
			subject = sap->venture->subjects[ALL_SUBJECTS];
		}
		else
		{
			writeMemoNote("[?] 'All subjects' subscription is \
limited to local continuum", itoa(continuumNbr));
			return -1;
		}
	}
	else
	{
		CHKERR(subjectIsValid(sap, subjectNbr, &subject));
	}

	vector = lookUpDeliveryVector(sap, sequence, diligence);
	if (vector == NULL)
	{
		writeMemo("[?] Have no endpoints for reception at this QOS.");
		return -1;
	}

	if (addMsgRule(sap, SUBSCRIPTION, subject, roleNbr, continuumNbr,
			unitNbr, priority, flowLabel, vector, &elt) < 0)
	{
		return -1;
	}

	if (elt == NULL)	/*	Pre-existing rule.		*/
	{
		return 0;
	}

	assertion = MTAKE(SUBSCRIBE_LEN);
	CHKERR(assertion);
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
		lockMib();
		result = ams_subscribe2(sap, roleNbr, continuumNbr, unitNbr,
			subjectNbr, priority, flowLabel, sequence, diligence);
		unlockMib();
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
		continuumNbr = (_mib(NULL))->localContinuumNbr;
	}

	CHKERR(subjectIsValid(sap, subjectNbr, &subject));
	if (removeMsgRule(sap, SUBSCRIPTION, subject, roleNbr, continuumNbr,
			unitNbr) < 0)
	{
		return 0;		/*	Redundant but okay.	*/
	}

	cancellation = MTAKE(CANCEL_LEN);
	CHKERR(cancellation);
	i2 = subjectNbr;
	i2 = htons(i2);
	memcpy(cancellation, (char *) &i2, 2);
	*(cancellation + 2) = (continuumNbr >> 8) & 0x0000007f;
	*(cancellation + 3) = continuumNbr & 0x000000ff;
	u2 = unitNbr;
	u2 = htons(u2);
	memcpy(cancellation + 4, (char *) &u2, 2);
	*(cancellation + 6) = roleNbr;
	if (enqueueMsgToRegistrar(sap, unsubscribe,
			computeModuleId(sap->role->nbr, sap->unit->nbr,
			sap->moduleNbr), CANCEL_LEN, (char *) cancellation) < 0)
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
		lockMib();
		result = ams_unsubscribe2(sap, roleNbr, continuumNbr, unitNbr,
				subjectNbr);
		unlockMib();
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
	CHKERR(envelope);
	if (continuumNbr == (_mib(NULL))->localContinuumNbr)
	{
		continuumNbr = 0;	/*	To all continua.	*/
	}

	constructEnvelope(envelope, continuumNbr, unitNbr, sourceIdNbr,
			destinationIdNbr, subjectNbr, amsHdrLength, amsHeader,
			contentLength, content, controlCode);
	result = ams_publish2(sap, subject, 1, 0, envelopeLength,
			(char *) envelope, 0);
	MRELEASE(envelope);
	return result;
}

static int	sendMsg(AmsSAP *sap, int continuumNbr, int unitNbr,
			int moduleNbr, int subjectNbr, int priority,
			unsigned char flowLabel, int contentLength,
			char *content, int context, AmsMsgType msgType)
{
	int		myContinNbr = (_mib(NULL))->localContinuumNbr;
	Subject		*subject;
	char		amsHeader[16];
	int		headerLength = sizeof amsHeader;
	int		result;
	Unit		*unit;
	Module		*module;
	LystElt		elt;
	SubjOfInterest	*subj = NULL;
	FanModule	*fan;
	XmitRule	*rule = NULL;

	if (continuumNbr == THIS_CONTINUUM)
	{
		continuumNbr = myContinNbr;
	}

	CHKERR(continuumNbr > 0);
	CHKERR(unitNbr >= 0);
	CHKERR(unitNbr <= MAX_UNIT_NBR);
	CHKERR(moduleNbr > 0);
	CHKERR(moduleNbr <= MAX_MODULE_NBR);
	CHKERR(priority >= 0);
	CHKERR(priority < NBR_OF_PRIORITY_LEVELS);
	CHKERR(contentLength >= 0);
	CHKERR(contentLength <= MAX_AMS_CONTENT);

	/*	Only the RAMS gateway is allowed to send messages
	 *	on the subject number that is the additive inverse
	 *	of the local continuum number, which is reserved for
	 *	local transmission of messages from remote continua.
	 *	All other subject numbers less than 1 are always
	 *	invalid.						*/

	if (subjectNbr < 1)
	{
		CHKERR(sap->role->nbr == 1);	/*	RAMS gateway	*/
		CHKERR(subjectNbr == (0 - myContinNbr));
		subject = sap->venture->msgspaces[myContinNbr];
	}
	else
	{
		CHKERR(subjectIsValid(sap, subjectNbr, &subject));
	}

	/*	Do remote AMS procedures if necessary.			*/

	if (continuumNbr != myContinNbr)
	{
		if (constructMessage(sap, subjectNbr, priority, flowLabel,
				context, &content, &contentLength,
				(unsigned char *) amsHeader, msgType) < 0)
		{
			return -1;
		}

		result = publishInEnvelope(sap, continuumNbr, unitNbr,
			sap->role->nbr, moduleNbr, subjectNbr, amsHeader,
			headerLength, content, contentLength, 5);
		MRELEASE(content);
		return result;
	}

	/*	Find constraints on sending the message.		*/

	unit = sap->venture->units[unitNbr];
	if (unit == NULL)
	{
		writeMemoNote("[?] Unknown destination unit", itoa(unitNbr));
		return -1;
	}

	module = unit->cell->modules[moduleNbr];
	if (module->role == NULL)
	{
		writeMemoNote("[?] Unknown destination module",
				itoa(moduleNbr));
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
		writeMemoNote("[?] Can't send msgs on this subject to this \
module", subject->name);
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

	if (constructMessage(sap, subjectNbr, priority, flowLabel, context,
			&content, &contentLength, (unsigned char *) amsHeader,
			msgType) < 0)
	{
		return -1;
	}

	/*	Send the message.					*/
#if AMSDEBUG
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
	result = rule->amsEndpoint->ts->sendAmsFn(rule->amsEndpoint, sap,
		flowLabel, amsHeader, headerLength, content, contentLength);
	MRELEASE(content);
	return result;
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
		lockMib();
		result = ams_send2(sap, continuumNbr, unitNbr, moduleNbr,
				subjectNbr, priority, flowLabel, contentLength,
				content, context);
		unlockMib();
	}

	return result;
}

static int	deliverTimeout(AmsEvent *event)
{
	AmsEvt	*evt;

	evt = (AmsEvt *) MTAKE(sizeof(AmsEvt));
	CHKERR(evt);
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
		unlockMib();
		result = llcv_wait(sap->amsEventsCV, condition, term);
		lockMib();
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
#ifndef mingw
	sigset_t	signals;

	sigfillset(&signals);
	pthread_sigmask(SIG_BLOCK, &signals, NULL);
#endif
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

	unlockMib();
	pthread_join(sap->eventMgr, NULL);
	lockMib();

	/*	Complete transfer of event management responsibility
	 *	to self.						*/

	sap->eventMgr = sap->authorizedEventMgr;
}

static int	ams_query2(AmsSAP *sap, int continuumNbr, int unitNbr,
			int moduleNbr, int subjectNbr, int priority,
			unsigned char flowLabel, int contentLength,
			char *content, int context, int term, AmsEvent *event)
{
	int		eventMgrNeeded = 0;
	int		result = 0;
	saddr		comparefn;
	pthread_t	mgrThread;

	CHKERR(sap);
	CHKERR(event);
	*event = NULL;
	if (context == 0)
	{
		writeMemo("[?] Non-zero context nbr needed for query.");
		return -1;
	}

	if (term == AMS_POLL	/*	Pseudo-synchronous.		*/
	|| (continuumNbr != THIS_CONTINUUM
		&& continuumNbr != (_mib(NULL))->localContinuumNbr))
	{
		return sendMsg(sap, continuumNbr, unitNbr, moduleNbr,
				subjectNbr, priority, flowLabel, contentLength,
				content, context, AmsMsgQuery);
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

	comparefn = context;
	lyst_compare_set(sap->amsEvents, (LystCompareFn) comparefn);
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
		if (pthread_begin(&mgrThread, NULL, eventMgrMain,
			sap, "libams_event_mgr") < 0)
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
		lockMib();
		result = ams_query2(sap, continuumNbr, unitNbr, moduleNbr,
				subjectNbr, priority, flowLabel, contentLength,
				content, context, term, event);
		unlockMib();
	}

	return result;
}

static int	ams_reply2(AmsSAP *sap, AmsEvt *evt, int subjectNbr,
			int priority, unsigned char flowLabel,
			int contentLength, char *content)
{
	int	result;
	AmsMsg	*msg;

	CHKERR(evt);
	CHKERR(evt->type == AMS_MSG_EVT);
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
		lockMib();
		result = ams_reply2(sap, evt, subjectNbr, priority,
				flowLabel, contentLength, content);
		unlockMib();
	}

	return result;
}

static int	ams_announce2(AmsSAP *sap, int roleNbr, int continuumNbr,
			int unitNbr, int subjectNbr, int priority,
			unsigned char flowLabel, int contentLength,
			char *content, int context)
{
	int		myContinNbr = (_mib(NULL))->localContinuumNbr;
	Subject		*subject;
	char		amsHeader[16];
	int		headerLength = sizeof amsHeader;
	int		result;
	unsigned char	protectedBits;
	Lyst		recipients;
	LystElt		elt;
	FanModule	*fan;
	saddr		temp;
	XmitRule	*rule;

	if (continuumNbr == THIS_CONTINUUM)
	{
		continuumNbr = myContinNbr;
	}

	CHKERR(continuumNbr >= 0);
	CHKERR(unitNbr >= 0);
	CHKERR(unitNbr <= MAX_UNIT_NBR);
	CHKERR(roleNbr >= 0);
	CHKERR(roleNbr <= MAX_ROLE_NBR);
	CHKERR(priority >= 0);
	CHKERR(priority < NBR_OF_PRIORITY_LEVELS);
	CHKERR(contentLength >= 0);
	CHKERR(contentLength <= MAX_AMS_CONTENT);
	CHKERR(subjectNbr > 0);
	CHKERR(subjectIsValid(sap, subjectNbr, &subject));

	/*	Knowing subject, construct message header & encrypt.	*/

	if (constructMessage(sap, subjectNbr, priority, flowLabel, context,
			&content, &contentLength, (unsigned char *) amsHeader,
			AmsMsgUnary) < 0)
	{
		return -1;
	}

	/*	Do remote AMS procedures if necessary.			*/

	if (continuumNbr == ALL_CONTINUA)	/*	All msgspaces.	*/
	{
		result = publishInEnvelope(sap, myContinNbr,
			unitNbr, sap->role->nbr, roleNbr, subjectNbr,
			amsHeader, headerLength, content, contentLength, 6);
		if (result < 0)
		{
			MRELEASE(content);
			return result;
		}
	}
	else		/*	Announcing to just one msgspace.	*/
	{
		if (continuumNbr != myContinNbr)
		{
			result = publishInEnvelope(sap, continuumNbr, unitNbr,
				sap->role->nbr, roleNbr, subjectNbr, amsHeader,
				headerLength, content, contentLength, 6);
			MRELEASE(content);
			return result;
		}
	}

	/*	Destination is either all msgspaces (including the one
	 *	in the local continuum) or just the local continuum's
	 *	message space.  So announce to the local continuum's
	 *	message space now.					*/

	protectedBits = amsHeader[0] & 0xf0;
	recipients = lyst_create_using(getIonMemoryMgr());
	CHKERR(recipients);

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

		/*	Module is in domain of the announce request.	*/

		rule = getXmitRule(sap, fan->subj->invitations);
		if (rule == NULL)
		{
			continue;	/*	Can't send module copy.	*/
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
			lyst_destroy(recipients);
			MRELEASE(content);
			return result;
		}

		temp = fan->module->nbr;
		lyst_insert_last(recipients, (void *) temp);
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
			lyst_destroy(recipients);
			MRELEASE(content);
			return result;
		}
	}

	lyst_destroy(recipients);
	MRELEASE(content);
	return 0;
}

int	ams_announce(AmsSAP *sap, int roleNbr, int continuumNbr, int unitNbr,
		int subjectNbr, int priority, unsigned char flowLabel,
		int contentLength, char *content, int context)
{
	int	result = -1;

	if (validSap(sap))
	{
		lockMib();
		result = ams_announce2(sap, roleNbr, continuumNbr, unitNbr,
				subjectNbr, priority, flowLabel, contentLength,
				content, context);
		unlockMib();
	}

	return result;
}

static int	ams_post_user_event2(AmsSAP *sap, int code, int dataLength,
			char *data, int priority)
{
	AmsEvt	*evt;
	char	*cursor;

	CHKERR(dataLength == 0 || (dataLength > 0 && data != NULL));
	CHKERR(priority >= 0);
	CHKERR(priority < NBR_OF_PRIORITY_LEVELS);
	evt = (AmsEvt *) MTAKE(1 + (2 * sizeof(int)) + dataLength);
	CHKERR(evt);
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
		lockMib();
		result = ams_post_user_event2(sap, code, dataLength, data,
				priority);
		unlockMib();
	}

	return result;
}

static int	ams_get_event2(AmsSAP *sap, int term, AmsEvent *event)
{
	CHKERR(term >= -1);
	CHKERR(event);
	if (!pthread_equal(pthread_self(), sap->eventMgr))
	{
		writeMemo("[?] get_event attempted by non-event-mgr thread.");
		return -1;
	}

	return getEvent(sap, term, event, llcv_lyst_not_empty);
}

int	ams_get_event(AmsSAP *sap, int term, AmsEvent *event)
{
	int	result = -1;

	if (validSap(sap))
	{
		lockMib();
		result = ams_get_event2(sap, term, event);
		unlockMib();
	}

	return result;
}

int	ams_get_event_type(AmsEvent event)
{
	CHKERR(event);
	return event->type;
}

int	ams_parse_msg(AmsEvent event, int *continuumNbr, int *unitNbr,
		int *moduleNbr, int *subjectNbr, int *contentLength,
		char **content, int *context, AmsMsgType *msgType,
		int *priority, unsigned char *flowLabel)
{
	AmsMsg	*msg;

	CHKERR(event);
	CHKERR(event->type == AMS_MSG_EVT);
	CHKERR(continuumNbr);
	CHKERR(unitNbr);
	CHKERR(moduleNbr);
	CHKERR(subjectNbr);
	CHKERR(contentLength);
	CHKERR(content);
	CHKERR(context);
	CHKERR(msgType);
	CHKERR(priority);
	CHKERR(flowLabel);
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
		lockMib();
		result = ams_lookup_unit_nbr2(sap, unitName);
		unlockMib();
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
		lockMib();
		result = ams_lookup_role_nbr2(sap, roleName);
		unlockMib();
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
		lockMib();
		result = ams_lookup_subject_nbr2(sap, subjectName);
		unlockMib();
	}

	return result;
}

int	ams_lookup_continuum_nbr(AmsSAP *sap, char *continuumName)
{
	int	result = -1;

	if (continuumName && validSap(sap))
	{
		lockMib();
		result = lookUpContinuum(continuumName);
		unlockMib();
	}

	return result;
}

static char	*ams_lookup_unit_name2(AmsSAP *sap, int unitNbr)
{
	Unit	*unit;

	if (unitNbr >= 0 && unitNbr <= MAX_UNIT_NBR)
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
		lockMib();
		result = ams_lookup_unit_name2(sap, unitNbr);
		unlockMib();
	}

	return result;
}

static char	*ams_lookup_role_name2(AmsSAP *sap, int roleNbr)
{
	AppRole	*role;

	if (roleNbr > 0 && roleNbr <= MAX_ROLE_NBR)
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
		lockMib();
		result = ams_lookup_role_name2(sap, roleNbr);
		unlockMib();
	}

	return result;
}

static char	*ams_lookup_subject_name2(AmsSAP *sap, int subjectNbr)
{
	Subject	*subject;

	if (subjectNbr > 0 && subjectNbr <= MAX_SUBJ_NBR)
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
		lockMib();
		result = ams_lookup_subject_name2(sap, subjectNbr);
		unlockMib();
	}

	return result;
}

static char	*ams_lookup_continuum_name2(AmsSAP *sap, int continuumNbr)
{
	Continuum	*contin;

	lockMib();
	if (continuumNbr < 0) continuumNbr = 0 - continuumNbr;
       	if (continuumNbr > 0 && continuumNbr <= MAX_CONTIN_NBR)
	{
		contin = (_mib(NULL))->continua[continuumNbr];
		if (contin)	/*	Known continuum.		*/
		{
			unlockMib();
			return contin->name;
		}
	}

	unlockMib();
	return NULL;
}

char	*ams_lookup_continuum_name(AmsSAP *sap, int continuumNbr)
{
	char	*result = NULL;

	if (validSap(sap))
	{
		lockMib();
		result = ams_lookup_continuum_name2(sap, continuumNbr);
		unlockMib();
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

	CHKERR(event);
	CHKERR(event->type == NOTICE_EVT);
	CHKERR(state);
	CHKERR(change);
	CHKERR(unitNbr);
	CHKERR(moduleNbr);
	CHKERR(roleNbr);
	CHKERR(domainContinuumNbr);
	CHKERR(domainUnitNbr);
	CHKERR(subjectNbr);
	CHKERR(priority);
	CHKERR(flowLabel);
	CHKERR(sequence);
	CHKERR(diligence);
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
	CHKERR(event);
	CHKERR(event->type == USER_DEFINED_EVT);
	CHKERR(code);
	CHKERR(dataLength);
	CHKERR(data);
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

	CHKERR(event);
	if (event->type == AMS_MSG_EVT)
	{
		msg = (AmsMsg *) (event->value);
		if (msg->content)
		{
			RELEASE_CONTENT_SPACE(msg->content);
		}
	}

	MRELEASE(event);
	return 0;
}

static int	ams_set_event_mgr2(AmsSAP *sap, AmsEventMgt *rules)
{
	pthread_t	mgrThread;

	CHKERR(rules);

	/*	Only prime thread can set event manager, and it can do
	 *	so only when it is, itself, the current event manager.	*/

	if (!pthread_equal(pthread_self(), sap->primeThread))
	{
		writeMemo("[?] Only prime thread can set event mgr.");
		return -1;
	}

	if (!pthread_equal(sap->eventMgr, sap->primeThread))
	{
		writeMemo("[?] Another event mgr is running.");
		return -1;
	}

	memcpy((char *) &(sap->eventMgtRules), rules, sizeof(AmsEventMgt));
	if (pthread_begin(&mgrThread, NULL, eventMgrMain,
		sap, "libams_event_mgr"))
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
		lockMib();
		result = ams_set_event_mgr2(sap, rules);
		unlockMib();
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
		lockMib();
		ams_remove_event_mgr2(sap);
		unlockMib();
	}
}
