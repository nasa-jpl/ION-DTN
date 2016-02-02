/*
	libocgr.c:	functions that populate the contact plan
			with predicted contacts.

	Author:		Scott Burleigh, JPL

	Michele Rodolfi of University of Bologna participated in
	the conceptual design of the contact prediction procedure.

	Copyright (c) 2016, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
									*/
#include "cgr.h"

#define	LOW_BASE_CONFIDENCE	(.05)
#define	HIGH_BASE_CONFIDENCE	(.20)

typedef struct
{
	uvast		duration;
	uvast		capacity;
	uvast		fromNode;
	uvast		toNode;
	time_t		fromTime;
	time_t		toTime;
	unsigned int	xmitRate;
} PbContact;

static int	removePredictedContacts()
{
	Sdr		sdr = getIonsdr();
	IonDB		iondb;
	Object		obj;
	Object		elt;
	Object		nextElt;
	IonContact	contact;

	CHKERR(sdr_begin_xn(sdr));
	sdr_read(sdr, (char *) &iondb, getIonDbObject(), sizeof(IonDB));
	for (elt = sdr_list_first(sdr, iondb.contacts); elt; elt = nextElt)
	{
		nextElt = sdr_list_next(sdr, elt);
		obj = sdr_list_data(sdr, elt);
		sdr_read(sdr, (char *) &contact, obj, sizeof(IonContact));
		if (contact.confidence == 1.0)
		{
			continue;	/*	Managed or discovered.	*/
		}

		/*	This is a predicted contact.			*/

		if (rfx_remove_contact(contact.fromTime, contact.fromNode,
				contact.toNode) < 0)
		{
			putErrmsg("Failure in rfx_remove_contact.", NULL);
			break;
		}
	}

	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't remove predicted contacts.", NULL);
		return -1;
	}

	return 0;
}

static void	freeContents(LystElt elt, void *userdata)
{
	MRELEASE(lyst_data(elt));
}

static int	insertIntoPredictionBase(Lyst pb, PastContact *logEntry)
{
	vast		duration;
	LystElt		elt;
	PbContact	*contact;

	duration = logEntry->toTime - logEntry->fromTime;
	if (duration <= 0)
	{
		return 0;	/*	Useless contact.		*/
	}

	for (elt = lyst_first(pb); elt; elt = lyst_next(elt))
	{
		contact = (PbContact *) lyst_data(elt);
		if (contact->fromNode < logEntry->fromNode)
		{
			continue;
		}

		if (contact->fromNode > logEntry->fromNode)
		{
			break;
		}

		if (contact->toNode < logEntry->toNode)
		{
			continue;
		}

		if (contact->toNode > logEntry->toNode)
		{
			break;
		}

		if (contact->toTime < logEntry->fromTime)
		{
			/*	Ends before start of log entry.		*/

			continue;
		}

		if (contact->fromTime > logEntry->toTime)
		{
			/*	Starts after end of log entry.		*/

			break;
		}

		/*	This previously inserted contact starts
		 *	before the log entry ends and ends after
		 *	the log entry starts, so the log entry
		 *	overlaps with it and can't be inserted.		*/

		return 0;
	}

	contact = MTAKE(sizeof(PbContact));
	if (contact == NULL)
	{
		putErrmsg("No memory for prediction base contact.", NULL);
		return -1;
	}

	contact->duration = duration;
	contact->capacity = duration * logEntry->xmitRate;
	contact->fromNode = logEntry->fromNode;
	contact->toNode = logEntry->toNode;
	contact->fromTime = logEntry->fromTime;
	contact->toTime = logEntry->toTime;
	contact->xmitRate = logEntry->xmitRate;
	if (elt)
	{
		elt = lyst_insert_before(elt, contact);
	}
	else
	{
		elt = lyst_insert_last(pb, contact);
	}

	if (elt == NULL)
	{
		putErrmsg("No memory for prediction base list element.", NULL);
		return -1;
	}

	return 0;
}

static Lyst	constructPredictionBase()
{
	Sdr		sdr = getIonsdr();
	Lyst		pb;
	IonDB		iondb;
	int		i;
	Object		elt;
	PastContact	logEntry;

	pb = lyst_create_using(getIonMemoryMgr());
	if (pb == NULL)
	{
		putErrmsg("No memory for prediction base.", NULL);
		return NULL;
	}

	lyst_delete_set(pb, freeContents, NULL);
	CHKNULL(sdr_begin_xn(sdr));
	sdr_read(sdr, (char *) &iondb, getIonDbObject(), sizeof(IonDB));
	for (i = 0; i < 2; i++)
	{
		for (elt = sdr_list_first(sdr, iondb.contactLog[i]); elt;
				elt = sdr_list_next(sdr, elt))
		{
			sdr_read(sdr, (char *) &logEntry, sdr_list_data(sdr,
					elt), sizeof(PastContact));
			if (insertIntoPredictionBase(pb, &logEntry) < 0)
			{
				putErrmsg("Can't insert into prediction base.",
						NULL);
				lyst_destroy(pb);
				sdr_exit_xn(sdr);
				return NULL;
			}
		}
	}

	sdr_exit_xn(sdr);
	return pb;
}

static int	processSequence(LystElt start, LystElt end, time_t currentTime)
{
	uvast		fromNode;
	uvast		toNode;
	time_t		horizon;
	LystElt		elt;
	PbContact	*contact;
	uvast		totalCapacity = 0;
	uvast		totalContactDuration = 0;
	unsigned int	contactsCount = 0;
	uvast		totalGapDuration = 0;
	unsigned int	gapsCount = 0;
	LystElt		prevElt;
	PbContact	*prevContact;
	uvast		gapDuration;
	uvast		meanCapacity;
	uvast		meanContactDuration;
	uvast		meanGapDuration;
	vast		deviation;
	uvast		contactDeviationsTotal = 0;
	uvast		gapDeviationsTotal = 0;
	uvast		contactStdDev;
	uvast		gapStdDev;
	float		baseConfidence;
	float		netDoubt;
	float		netConfidence;
	int		i;
	time_t		now;
	time_t		gapEnd;
	time_t		contactEnd;
	unsigned int	xmitRate;

	if (start == NULL)	/*	No sequence found yet.		*/
	{
		return 0;
	}

	contact = (PbContact *) lyst_data(start);
	fromNode = contact->fromNode;
	toNode = contact->toNode;
	horizon = currentTime + (currentTime - contact->fromTime);

	/*	Compute totals and means.				*/

	elt = start;
	prevElt = NULL;
	while (1)
	{
		totalCapacity += contact->capacity;
		totalContactDuration += contact->duration;
		contactsCount++;
		if (prevElt)
		{
			prevContact = (PbContact *) lyst_data(prevElt);
			gapDuration = contact->fromTime - prevContact->toTime;
			totalGapDuration += gapDuration;
			gapsCount++;
		}

		prevElt = elt;
		if (elt == end)
		{
			break;
		}

		elt = lyst_next(elt);
		contact = (PbContact *) lyst_data(elt);
	}

	meanCapacity = totalCapacity / contactsCount;
	meanContactDuration = totalContactDuration / contactsCount;
	meanGapDuration = gapsCount > 0 ? totalGapDuration / gapsCount : 0;

	/*	Compute standard deviations.				*/

	contact = (PbContact *) lyst_data(start);
	elt = start;
	prevElt = NULL;
	while (1)
	{
		deviation = contact->duration - meanContactDuration;
		contactDeviationsTotal += (deviation * deviation);
		if (prevElt)
		{
			prevContact = (PbContact *) lyst_data(prevElt);
			gapDuration = contact->fromTime - prevContact->toTime;
			deviation = gapDuration - meanGapDuration;
			gapDeviationsTotal += (deviation * deviation);
		}

		prevElt = elt;
		if (elt == end)
		{
			break;
		}

		elt = lyst_next(elt);
		contact = (PbContact *) lyst_data(elt);
	}

	contactStdDev = sqrt(contactDeviationsTotal / contactsCount);
	gapStdDev = gapsCount > 0 ? sqrt(gapDeviationsTotal / gapsCount) : 0;

	/*	Select base confidence, compute net confidence.		*/

	if (gapsCount > 1
	&& contactStdDev < meanContactDuration
	&& gapStdDev < meanGapDuration)
	{
		baseConfidence = HIGH_BASE_CONFIDENCE;
	}
	else
	{
		baseConfidence = LOW_BASE_CONFIDENCE;
	}

	netDoubt = 1.0;
	for (i = 0; i < contactsCount; i++)
	{
		netDoubt *= (1.0 - baseConfidence);
	}

	netConfidence = 1.0 - netDoubt;

	/*	Insert predicted contacts.				*/

	contact = (PbContact *) lyst_data(end);
	now = contact->toTime;
	while (now <= horizon)
	{
		if (gapStdDev < meanGapDuration)
		{
			/*	Gap duration may be underestimated.	*/

			gapEnd = now + (meanGapDuration - gapStdDev);
		}
		else
		{
			gapEnd = now;
		}

		/*	Contact duration may be overestimated.		*/

		contactEnd = gapEnd + meanContactDuration + contactStdDev;
		xmitRate = meanCapacity / (contactEnd - gapEnd);
		if (contactEnd > currentTime && xmitRate > 1)
		{
			if (rfx_insert_contact(gapEnd, contactEnd, fromNode,
					toNode, xmitRate, netConfidence) == 0)
			{
				putErrmsg("Can't insert contact.", NULL);
				return -1;
			}
		}

		now = contactEnd;
	}

	return 0;
}

int	cgr_predict_contacts()
{
	time_t		currentTime = getUTCTime();
	Lyst		predictionBase;
	LystElt		elt;
	PbContact	*contact;
	LystElt		startOfSequence = NULL;
	LystElt	 	endOfSequence = NULL;
	uvast		sequenceFromNode = 0;
	uvast		sequenceToNode = 0;
	int		result = 0;

	/*	First, remove all predicted contacts from contact plan.	*/

	if (removePredictedContacts() < 0)
	{
		putErrmsg("Can't predict contacts.", NULL);
		return -1;
	}

	/*	Next, construct a prediction base from the current
	 *	current contact logs (containing all discovered
	 *	contacts that occurred in the past).			*/

	predictionBase = constructPredictionBase();
	if (predictionBase == NULL)
	{
		putErrmsg("Can't predict contacts.", NULL);
		return -1;
	}

	/*	Now generate predicted contacts from the prediction
	 *	base.							*/

	for (elt = lyst_first(predictionBase); elt; elt = lyst_next(elt))
	{
		contact = (PbContact *) lyst_data(elt);
		if (contact->fromNode != sequenceFromNode
		|| contact->toNode != sequenceToNode)
		{
			if (processSequence(startOfSequence, endOfSequence,
					currentTime) < 0)
			{
				putErrmsg("Can't predict contacts.", NULL);
				lyst_destroy(predictionBase);
				return -1;
			}

			/*	Note start of new sequence.		*/

			sequenceFromNode = contact->fromNode;
			sequenceToNode = contact->toNode;
			startOfSequence = elt;
		}

		/*	Continuation of current prediction sequence.	*/

		endOfSequence = elt;
	}

	/*	Process the last sequence in the prediction base.	*/

	if (processSequence(startOfSequence, endOfSequence, currentTime) < 0)
	{
		putErrmsg("Can't predict contacts.", NULL);
		result = -1;
	}

	lyst_destroy(predictionBase);
	return result;
}
