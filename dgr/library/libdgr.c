/*
	libdgr.c:	functions enabling the implementation of DGR
			applications.

	Author: Scott Burleigh, JPL

	Copyright (c) 2003, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
	
									*/
#include "dgr.h"
#include "memmgr.h"
#include <pthread.h>

#define	DGR_DB_ORDER	(8)
#define	DGR_BUCKETS	(1 << DGR_DB_ORDER)
#define	DGR_BIN_COUNT	(257)		/*	Must be prime for hash.	*/
#define	DGR_MAX_DESTS	(256)
#define	DGR_SEQNBR_MASK	(DGR_BUCKETS - 1)
#define	DGR_MIN_XMIT	(2)
#define	NBR_OF_OCC_LVLS	(3)
#define DGR_MAX_XMIT	(DGR_MIN_XMIT + NBR_OF_OCC_LVLS)

#define	DGR_BUF_SIZE	(65535)

#define	MTAKE(size)	mtake(__FILE__, __LINE__, size)
#define MRELEASE(ptr)	mrelease(__FILE__, __LINE__, ptr)

#ifndef SYS_CLOCK_RES
#define SYS_CLOCK_RES	10000		/*	Linux (usec per tick)	*/
#endif

#ifndef EPISODE_PERIOD
#define EPISODE_PERIOD	100000		/*	.1 sec (microseconds)	*/
#endif

#ifndef MAX_BACKLOG
#define MAX_BACKLOG	524288		/*	.5 MB (bytes)		*/
#endif

#ifndef MIN_TIMEOUT
#define MIN_TIMEOUT	2		/*	per Stevens (seconds)	*/
#endif

#ifndef MAX_TIMEOUT
#define MAX_TIMEOUT	60		/*	per Stevens (seconds)	*/
#endif

/*	For initial rate control it's usually best to start slow, to
 *	avoid thrashing.  So the default retard rate is 10 usec/byte.	*/

#ifndef INITIAL_RETARD
#define INITIAL_RETARD	10		/*	microseconds per byte	*/
#endif

/*		Initial RTT estimation parameters per Stevens.		*/
#define	INIT_SMOOTHED	(0)
#define	INIT_MRD	(750000)	/*	.75 seconds		*/
#define	INIT_RTT	(INIT_SMOOTHED + (4 * INIT_MRD))

static int		SystemClockResolution = SYS_CLOCK_RES;
static int		MinMicrosnooze = 0;
static int		EpisodeUsec = EPISODE_PERIOD;
static int		MinBytesToTransmit = 0;
static int		MaxBacklog = MAX_BACKLOG;
static int		MinTimeout = MIN_TIMEOUT * 1000000;
static int		MaxTimeout = MAX_TIMEOUT * 1000000;
static int		InitialRetard = INITIAL_RETARD;
static int		occupancy[NBR_OF_OCC_LVLS];
static int		memmgrId = -1;
static MemAllocator	mtake = NULL;
static MemDeallocator	mrelease = NULL;

typedef enum
{
	SendMessage = 1,
	HandleTimeout,
	HandleAck
} RecordOperation;

typedef struct
{
	unsigned int	time;
	unsigned int	seqNbr;
} CapsuleId;

typedef struct
{
	CapsuleId	id;
	char		content[1];
} Capsule;

typedef enum
{
	DgrMsgOut = 1,
	DgrMsgIn, 
	DgrDeliverySuccess,
	DgrDeliveryFailure
} DgrRecordType;

typedef struct
{
	Lyst		msgs;		/*	DgrRecord		*/
	pthread_mutex_t	mutex;
} DgrArqBucket;

typedef struct dgr_rec
{
	DgrRecordType	type;

	/*	Refer to destination if outbound, source if inbound.	*/
	unsigned short	portNbr;
	unsigned int	ipAddress;

	/*	Relevant only for outbound messages.			*/
	int		notificationFlags;
	int		transmissionCount;
	DgrArqBucket	*bucket;
	LystElt		outboundMsgsElt;
	struct timeval	transmitTime;
	LystElt		pendingResendsElt;

	/*	Relevant only for delivery failures.			*/
	int		errnbr;

	/*	Common to all types of record.				*/
	int		contentLength;
	Capsule		capsule;
} *DgrRecord;

typedef struct
{
	CapsuleId	id;
} SendReq;

typedef struct
{
	CapsuleId	id;
	struct timeval	resendTime;
} ResendReq;

typedef enum
{
	DgrSapClosed = 0,
	DgrSapOpen,
	DgrSapDamaged
} DgrSapState;

typedef struct
{
	int		capacity;
	int		serviceLoad;
	int		bytesTransmitted;
	int		bytesAcknowledged;
	int		unusedCapacity;
} EpisodeHistory;

typedef struct
{
	unsigned short	portNbr;
	unsigned int	ipAddress;
	int		msgsSent;
	int		backlog;	/*	bytes			*/
	int		msgsInBacklog;
	int		lessActiveDest;
	int		moreActiveDest;
	LystElt		ownElt;
	struct timeval	cursorXmitTime;
	int		rttSmoothed;	/*	microseconds		*/
	int		meanRttDeviation;
	int		rttPredicted;	/*	microseconds		*/
	int		predictedResends;

	/*	For congestion avoidance by flow control and
	 *	dynamic estimation of sustainable data rate.		*/

	int		episodeCount;
	int		bytesOriginated;
	int		meanBytesResent;
	int		bytesTransmitted;
	int		bytesAcknowledged;
	int		serviceLoad;	/*	bytes sendable		*/
	EpisodeHistory	episodes[8];
	int		currentEpisode;
	int		totalCapacity;
	int		totalServiceLoad;
	int		totalBytesTransmitted;
	int		totalBytesAcknowledged;
	int		totalUnusedCapacity;
	int		retard;		/*	microseconds per byte	*/
	int		bytesToTransmit;
	int		pendingDelay;	/*	microseconds		*/
} DgrDest;

typedef struct dgrsapst
{
	DgrSapState	state;
	int		udpSocket;

	pthread_mutex_t	sapMutex;
	pthread_cond_t	sapCV;
	time_t		capsuleTime;
	unsigned int	capsuleSeqNbr;
	int		backlog;	/*	Total, for all dests.	*/

	Lyst		outboundMsgs;	/*	(SendReq *)		*/
	struct llcv_str	outboundCV_str;
	Llcv		outboundCV;

	Lyst		pendingResends;	/*	(ResendReq *)		*/
	pthread_mutex_t	pendingResendsMutex;

	Lyst		inboundEvents;	/*	DgrRecord		*/
	struct llcv_str	inboundCV_str;
	Llcv		inboundCV;

	DgrArqBucket	buckets[DGR_BUCKETS];
	pthread_t	sender;
	pthread_t	resender;
	pthread_t	receiver;

	pthread_mutex_t	destsMutex;
	int		destsCount;
	DgrDest		dests[DGR_MAX_DESTS];
	Lyst		destLysts[DGR_BIN_COUNT];
	int		leastActiveDest;
	int		mostActiveDest;
	DgrDest		defaultDest;
} DgrSAP;

#if 0
/*	*	*	Test instrumentation	*	*	*	*/

static int	originalMsgs;
static int	resends[DGR_MAX_XMIT - 1];
static int	timeouts[DGR_MAX_XMIT];
static int	appliedAcks;
static int	traceMeasuredRtt;
static int	traceRttDeviation;
static int	traceRttSmoothed;
static int	traceMeanRttDeviation;
static int	traceRttPredicted;
static int	tracePredictedResends;
static int	computedRtt;
static int	traceBytesOriginated;
static int	traceBytesTransmitted;
static int	traceBytesAcknowledged;
static int	traceBytesResent;
static int	traceUnusedCapacity;
static int	traceBytesToTransmit;
static int	traceRetard;
static int	aggregateDelay;
static int	rcSnoozes;

static void	dgrtrace()
{
	char	tracebuf[128];

	iprintf(tracebuf, sizeof tracebuf,
		"%7d %7d %7d %7d %7d %7d %7d %7d %7d %3d\n", originalMsgs,
		resends[0], traceBytesOriginated, traceBytesResent,
		traceBytesTransmitted, traceUnusedCapacity,
		traceBytesAcknowledged, timeouts[0],
		traceBytesToTransmit, traceRetard);
	PUTS(tracebuf);
}
#endif
/*	*	*	Common utility functions	*	*	*/

static void	crashThread(DgrSAP *sap, char *msg)
{
	if (sap->state == DgrSapOpen)
	{
		sap->state = DgrSapDamaged;
	}

	putSysErrmsg(msg, NULL);
}

static int	time_to_stop(Llcv llcv)
{
	return 1;
}

	/*	Hash function adapted from Dr. Dobbs, April 1996.	*/

static int	hashDestId(unsigned short portNbr, unsigned int ipAddress)
{
	struct x
	{
		unsigned int	ipAddress;
		unsigned short	portNbr;
	} w;

	char		*name = (char *) &w;
	int		length = sizeof(unsigned int) + sizeof(unsigned short);
	int		i = 0;
	unsigned int	h = 0;
	unsigned int	g = 0;

	w.ipAddress = ipAddress;
	w.portNbr = portNbr;
	for (i = 0; i < length; i++, name++)
	{
		h = (h << 4) + *name;
		g = h & 0xf0000000;
		if (g)
		{
			h ^= g >> 24;
		}

		h &= ~g;
	}
	
	return h % DGR_BIN_COUNT;
}

static DgrDest	*findDest(DgrSAP *sap, unsigned short portNbr,
			unsigned int ipAddress, int *idx)
{
	int	bin;
	LystElt	elt;
	long	i;
	DgrDest	*dest;

	bin = hashDestId(portNbr, ipAddress);
	for (elt = lyst_first(sap->destLysts[bin]); elt; elt = lyst_next(elt))
	{
		dest = sap->dests + (i = (long) lyst_data(elt));
		if (dest->ipAddress == ipAddress && dest->portNbr == portNbr)
		{
			*idx = i;
			return dest;
		}
	}

	*idx = -1;
	return &sap->defaultDest;
}

static int	insertEvent(DgrSAP *sap, DgrRecord rec)
{
	LystElt	elt;

	llcv_lock(sap->inboundCV);
	if (rec == NULL)		/*	Interruption.		*/
	{
		elt = lyst_insert_first(sap->inboundEvents, rec);
	}
	else				/*	Normal event.		*/
	{
		elt = lyst_insert_last(sap->inboundEvents, rec);
	}

	llcv_unlock(sap->inboundCV);
	if (elt == NULL)
	{
		crashThread(sap, "Can't insert event");
		return -1;
	}

	/*	Tell the application that there's an event to handle.	*/

	llcv_signal(sap->inboundCV, llcv_lyst_not_empty);
	return 0;
}

/*	*	*	Common operational functions	*	*	*/

static void	initializeDest(DgrDest *dest, unsigned short portNbr,
			unsigned int ipAddress)
{
	dest->portNbr = portNbr;
	dest->ipAddress = ipAddress;
	dest->rttSmoothed = INIT_SMOOTHED;
	dest->meanRttDeviation = INIT_MRD;
	dest->rttPredicted = INIT_RTT;
	dest->retard = InitialRetard;
	dest->bytesToTransmit = EpisodeUsec / dest->retard;
#if 0
computedRtt = 0;
traceMeasuredRtt = 0;
traceRttDeviation = 0;
traceRttSmoothed = dest->rttSmoothed;
traceMeanRttDeviation = dest->meanRttDeviation;
traceRttPredicted = dest->rttPredicted;
tracePredictedResends = dest->predictedResends;
traceRetard = dest->retard;
traceBytesToTransmit = dest->bytesToTransmit;
#endif
}

static DgrDest	*addNewDest(DgrSAP *sap, unsigned short portNbr,
			unsigned int ipAddress, int *destIdx)
{
	long	newDest;
	DgrDest	*dest;
	int	nextDest;
	int	bin;

	if (sap->destsCount < DGR_MAX_DESTS)	/*	Empty slot.	*/
	{
		nextDest = DGR_MAX_DESTS - sap->destsCount;
		newDest = nextDest - 1;
		dest = sap->dests + newDest;
		sap->leastActiveDest = newDest;
		if (sap->destsCount == 0)	/*	The first one.	*/
		{
			sap->mostActiveDest = newDest;
		}

		sap->destsCount++;
	}
	else	/*	Replace the dest that's currently least active.	*/
	{
		newDest = sap->leastActiveDest;
		dest = sap->dests + newDest;
		lyst_delete(dest->ownElt);
		nextDest = dest->moreActiveDest;
		memset((char *) dest, 0, sizeof(DgrDest));
	}

	/*	Insert subscript of this location as new entry in
	 *	the dests list that this IP address hashes to.		*/

	bin = hashDestId(portNbr, ipAddress);
	dest->ownElt = lyst_insert_last(sap->destLysts[bin], (void *) newDest);
	if (dest->ownElt == NULL)
	{
		crashThread(sap, "Can't add new active destination");
		return NULL;
	}

	dest->lessActiveDest = -1;	/*	New one's least active.	*/
	dest->moreActiveDest = nextDest;
	initializeDest(dest, portNbr, ipAddress);
	*destIdx = newDest;
	return dest;
}

static void	loseOutboundMsg(DgrSAP *sap, DgrRecord rec)
{
	SendReq		*req;

	llcv_lock(sap->outboundCV);
	req = (SendReq *) lyst_data(rec->outboundMsgsElt);
	MRELEASE(req);
	lyst_delete(rec->outboundMsgsElt);
	llcv_unlock(sap->outboundCV);
	rec->outboundMsgsElt = NULL;
}

static void	losePendingResend(DgrSAP *sap, DgrRecord rec)
{
	ResendReq	*req;

	pthread_mutex_lock(&sap->pendingResendsMutex);
	req = (ResendReq *) lyst_data(rec->pendingResendsElt);
	MRELEASE(req);
	lyst_delete(rec->pendingResendsElt);
	rec->pendingResendsElt = NULL;
	pthread_mutex_unlock(&sap->pendingResendsMutex);
}

static void	removeRecord(DgrSAP *sap, DgrRecord rec, LystElt arqElt)
{
	/*	Any pendingResendsElt for this record has already
	 *	been deleted.						*/

	if (rec->outboundMsgsElt)
	{
		loseOutboundMsg(sap, rec);
	}

	lyst_delete(arqElt);	/*	Remove from database bucket.	*/

	/*	Enable more messages to be sent.			*/

	pthread_mutex_lock(&sap->sapMutex);
	sap->backlog -= (rec->contentLength + sizeof(CapsuleId));
	if (sap->backlog <= MaxBacklog)
	{
		pthread_cond_signal(&sap->sapCV);
	}

	pthread_mutex_unlock(&sap->sapMutex);
}

static void	adjustActiveDestChain(DgrSAP *sap, int destIdx)
{
	int	ceiling;
	DgrDest	*dest;
	DgrDest	*other = NULL;
	DgrDest	*next;
	DgrDest	*prev;

	dest = sap->dests + destIdx;
	ceiling = dest->moreActiveDest;
	while (ceiling < DGR_MAX_DESTS)
	{
		other = sap->dests + ceiling;
		if (dest->msgsSent <= other->msgsSent)	/*	ceiling	*/
		{
			break;
		}

		/*	Have become more active than this one.		*/

		ceiling = other->moreActiveDest;
	}

	if (ceiling != dest->moreActiveDest)	/*	more active now	*/
	{
		/*	Detach from current neighbors in list.		*/

		if (dest->lessActiveDest == -1)	/*	currently least	*/
		{
			sap->leastActiveDest = dest->moreActiveDest;
		}
		else	/*	Not currently the least active dest.	*/
		{
			prev = sap->dests + dest->lessActiveDest;
			prev->moreActiveDest = dest->moreActiveDest;
		}

		/*	(Cannot already be the most active dest.)	*/

		next = sap->dests + dest->moreActiveDest;
		next->lessActiveDest = dest->lessActiveDest;

		/*	Insert before new ceiling, overtaking another.	*/

		dest->moreActiveDest = ceiling;
		if (ceiling == DGR_MAX_DESTS)	/*	now most active	*/
		{
			dest->lessActiveDest = sap->mostActiveDest;
			sap->mostActiveDest = destIdx;
		}
		else	/*	Insert before the more active dest.	*/
		{
			dest->lessActiveDest = other->lessActiveDest;
			other->lessActiveDest = destIdx;
		}

		prev = sap->dests + dest->lessActiveDest;
		prev->moreActiveDest = destIdx;
	}
}

static int	insertResendReq(DgrSAP *sap, DgrRecord rec, DgrDest *dest,
			int destIdx)
{
	ResendReq	*req;
	int		rtt;
	LystElt		elt;
	ResendReq	*pending;

	req = MTAKE(sizeof(ResendReq));
	if (req == NULL)
	{
		crashThread(sap, "Can't create resend request");
		return -1;
	}

	req->id.time = rec->capsule.id.time;
	req->id.seqNbr = rec->capsule.id.seqNbr;
	req->resendTime.tv_sec = rec->transmitTime.tv_sec;
	req->resendTime.tv_usec = rec->transmitTime.tv_usec;

	/*	Locate and apply RTT computation parameters.		*/

	if (rec->transmissionCount > 1)	/*	Retransmission.		*/
	{
		if (destIdx < 0)	/*	Destination not active.	*/
		{
			rtt = INIT_RTT;		/*	Default RTT.	*/
		}
		else	/*	Dest is active, we predict RTT for it.	*/
		{
			dest = sap->dests + destIdx;

			/*	Apply exponential backoff to predicted
			 *	round-trip time as necessary.		*/

			rtt = dest->rttPredicted << dest->predictedResends;
		}

//resends[rec->transmissionCount - 2]++;
	}
	else	/*	Original transmission, note dest activity.	*/
	{
		if (destIdx < 0)	/*	Newly active dest.	*/
		{
			dest = addNewDest(sap, rec->portNbr, rec->ipAddress,
					&destIdx);
			if (dest == NULL)
			{
				return -1;
			}
		}

		/*	Apply exponential backoff to predicted round-
		 *	trip time as necessary.				*/

		rtt = dest->rttPredicted << dest->predictedResends;

		/*	Record activity and reorder dests by level of
		 *	activity as necessary.				*/

		dest->backlog += (rec->contentLength + sizeof(CapsuleId));
		dest->msgsInBacklog++;
		dest->msgsSent++;
		adjustActiveDestChain(sap, destIdx);
		dest->bytesOriginated += rec->contentLength;
//traceBytesOriginated = dest->bytesOriginated;
//originalMsgs++;
	}

	dest->bytesTransmitted += rec->contentLength;
//traceBytesTransmitted = dest->bytesTransmitted;
	if (rtt < MinTimeout)
	{
		rtt = MinTimeout;
	}

	if (rtt > MaxTimeout)
	{
		rtt = MaxTimeout;
	}

//computedRtt = rtt;
	/*	Compute acknowledgment deadline (resendTime).		*/

	req->resendTime.tv_usec += rtt;
	while (req->resendTime.tv_usec > 1000000)
	{
		req->resendTime.tv_sec += 1;
		req->resendTime.tv_usec -= 1000000;
	}

	/*	Now insert the retransmission request into the
	 *	pendingResends list, preserving resendTime order.	*/

	pthread_mutex_lock(&sap->pendingResendsMutex);
	for (elt = lyst_last(sap->pendingResends); elt; elt = lyst_prev(elt))
	{
		pending = (ResendReq *) lyst_data(elt);
		if (pending->resendTime.tv_sec > req->resendTime.tv_sec)
		{
			continue;
		}

		if (pending->resendTime.tv_sec < req->resendTime.tv_sec
		|| pending->resendTime.tv_usec <= req->resendTime.tv_usec)
		{
			break;	/*	Found "floor" item.		*/
		}
	}

	if (elt)
	{
		elt = rec->pendingResendsElt = lyst_insert_after(elt, req);
	}
	else
	{
		elt = rec->pendingResendsElt =
				lyst_insert_first(sap->pendingResends, req);
	}

	pthread_mutex_unlock(&sap->pendingResendsMutex);
	if (elt == NULL)
	{
		crashThread(sap, "Can't append resend request");
		return -1;
	}

	return 0;
}

static int	sendMessage(DgrSAP *sap, DgrRecord rec, LystElt arqElt,
			DgrDest *dest, int destIdx)
{
	struct sockaddr_in	socketAddress;
	struct sockaddr		*sockName = (struct sockaddr *) &socketAddress;

	/*	Temporarily invert the capsule ID fields for message
	 *	transmission purposes.					*/

	rec->capsule.id.time = htonl(rec->capsule.id.time);
	rec->capsule.id.seqNbr = htonl(rec->capsule.id.seqNbr);

	/*	Plug current time into record's transmit time.		*/

	getCurrentTime(&(rec->transmitTime));

	/*	Send the message.  Error results returned by sendto
	 *	do not crash the thread; eventually the application
	 *	will be notified of the delivery failure and take
	 *	some action such as closing the SAP.			*/

	socketAddress.sin_family = AF_INET;
	socketAddress.sin_port = htons(rec->portNbr);
	socketAddress.sin_addr.s_addr = htonl(rec->ipAddress);
	if (sendto(sap->udpSocket, (char *) &(rec->capsule), sizeof(CapsuleId)
			+ rec->contentLength, 0, sockName,
			sizeof(struct sockaddr_in)) < 0)
	{
		rec->errnbr = errno;
	}

	/*	Get the capsule ID fields back into local byte order
	 *	for searching purposes.					*/

	rec->capsule.id.time = ntohl(rec->capsule.id.time);
	rec->capsule.id.seqNbr = ntohl(rec->capsule.id.seqNbr);

	/*	Update record's transmission counter for ARQ control.	*/

	rec->transmissionCount++;

	/*	Delete outboundMsgs object; it's no longer needed.	*/

	loseOutboundMsg(sap, rec);

	/*	We may need to retransmit this record, so tell resender
	 *	to schedule retransmission in case acknowledgment is
	 *	not received.						*/

	return insertResendReq(sap, rec, dest, destIdx);
}

static void	noteCompletion(DgrRecord rec, DgrDest *dest, int destIdx)
{
	struct timeval	arrivalTime;
	int		measuredRtt;
	int		deviation;
	int		negative;	/*	Boolean			*/
	int		adj;
	int		change;

	/*	Update predicted RTT, but only if this is not a
	 *	delayed acknowledgement, and only if we can be sure
	 *	that the original transmission is the one that has
	 *	been acknowledged (else we have the retransmission
	 *	ambiguity problem: we can't have any confidence in
	 *	computed round-trip time for this capsule, so we
	 *	can't use that RTT to predict appropriate retransmit
	 *	times for future capsules).				*/

	getCurrentTime(&arrivalTime);

	/*	At the least, update backlog count to mitigate
	 *	maximum transmissions limit.				*/

	dest->backlog -= (rec->contentLength + sizeof(CapsuleId));
	dest->msgsInBacklog--;
	if (rec->transmissionCount > 1)
	{
		return;	/*	Might not be ack of most recent xmit.	*/
	}

	if (rec->transmitTime.tv_sec < dest->cursorXmitTime.tv_sec
	|| (rec->transmitTime.tv_sec == dest->cursorXmitTime.tv_sec
		&& rec->transmitTime.tv_usec < dest->cursorXmitTime.tv_usec))
	{
		return;	/*	A delayed acknowledgment, somehow.	*/
	}

	/*	Timely acknowledgment of an original transmission to
	 *	an active dest.  Update retransmission parameters with
	 *	acknowledgement information.				*/

	dest->cursorXmitTime.tv_sec = rec->transmitTime.tv_sec;
	dest->cursorXmitTime.tv_usec = rec->transmitTime.tv_usec;
	if (arrivalTime.tv_usec < rec->transmitTime.tv_usec)
	{
		arrivalTime.tv_usec += 1000000;
		arrivalTime.tv_sec--;
	}

	/*	Lifting all of this calculation from Jacobson (1988)
	 *	via Stevens (1997).					*/

	measuredRtt =
		((arrivalTime.tv_sec - rec->transmitTime.tv_sec) * 1000000)
			+ (arrivalTime.tv_usec - rec->transmitTime.tv_usec);
//traceMeasuredRtt = measuredRtt;

	/*	Add 1/8 of deviation to smoothed RTT.			*/

	deviation = measuredRtt - dest->rttSmoothed;
//traceRttDeviation = deviation;
	negative = deviation < 0 ? 1 : 0;
	deviation = abs(deviation);
	adj = (deviation >> 3) & 0x1fffffff;
	if (negative) adj = 0 - adj;
	dest->rttSmoothed += adj;
//traceRttSmoothed = dest->rttSmoothed;

	/*	Add 1/4 of change in deviation to mean deviation.	*/

	change = deviation - dest->meanRttDeviation;
	negative = change < 0 ? 1 : 0;
	change = abs(change);
	adj = (change >> 2) & 0x3fffffff;
	if (negative) adj = 0 - adj;
	dest->meanRttDeviation += adj;
//traceMeanRttDeviation = dest->meanRttDeviation;

	/*	For new prediction of RTT use smoothed RTT plus four
	 *	times the mean deviation.				*/

	dest->rttPredicted = dest->rttSmoothed + (dest->meanRttDeviation << 2);
//traceRttPredicted = dest->rttPredicted;
	dest->predictedResends = 0;
//tracePredictedResends = dest->predictedResends;
}

static int	handleAck(DgrSAP *sap, DgrRecord rec, LystElt arqElt,
			DgrDest *dest, int destIdx)
{
//appliedAcks++;
	dest->bytesAcknowledged += rec->contentLength;

	/*	May want to update predicted RTT if the acknowledging
	 *	dest is active enough for us to be trying to predict
	 *	accurate RTT.						*/

	if (destIdx >= 0)	/*	An active destination.		*/
	{
		noteCompletion(rec, dest, destIdx);
	}

	/*	Delete pendingResends object; it's now moot.		*/

	losePendingResend(sap, rec);

	/*	Remove record from database; it won't be sent again.	*/

	removeRecord(sap, rec, arqElt);

	/*	If application doesn't want to be notified of delivery
	 *	success for this record, just erase it and return.	*/

	if ((rec->notificationFlags & DGR_NOTE_ACKED) == 0)
	{
		MRELEASE(rec);
		return 0;
	}

	/*	Pass the record back to the application as a delivery
	 *	success.						*/

	rec->type = DgrDeliverySuccess;
	return insertEvent(sap, rec);
}

static int	insertSendReq(DgrSAP *sap, DgrRecord rec)
{
	SendReq		*req;
	LystElt		elt;

	req = MTAKE(sizeof(SendReq));
	if (req == NULL)
	{
		crashThread(sap, "Can't create send request");
		return -1;
	}

	req->id.time = rec->capsule.id.time;
	req->id.seqNbr = rec->capsule.id.seqNbr;
	llcv_lock(sap->outboundCV);
	elt = rec->outboundMsgsElt = lyst_insert_last(sap->outboundMsgs, req);
	llcv_unlock(sap->outboundCV);
	if (elt == NULL)
	{
		crashThread(sap, "Can't append send request");
		return -1;
	}

	/*	Tell the sender thread that there's something to send.	*/

	llcv_signal(sap->outboundCV, llcv_lyst_not_empty);
	return 0;
}

static int	handleTimeout(DgrSAP *sap, DgrRecord rec, LystElt arqElt,
			DgrDest *dest, int destIdx)
{
	int	maxTransmissions = 0;
	int	i;

//timeouts[rec->transmissionCount - 1]++;
	if (destIdx >= 0)	/*	An active destination.		*/
	{
		/*	Update retransmission control if this is
		 *	the most up-to-date information on this dest
		 *	relationship.
		 *
		 *	Because in DGR we do concurrent rather than
		 *	sequential transmission to each destination
		 *	dest, we can't base our RTT estimation
		 *	adjustments on the transmission time of the
		 *	most recently transmitted message -- it may
		 *	not be useful.  In its place, we track the
		 *	"cursor" transmission time for the dest:
		 *	the transmission time of the message for
		 *	which we've got the most recent information,
		 *	be it an acknowledgment or a timeout.		*/

		if (rec->transmitTime.tv_sec > dest->cursorXmitTime.tv_sec
		|| (rec->transmitTime.tv_sec == dest->cursorXmitTime.tv_sec
			&& rec->transmitTime.tv_usec
				>= dest->cursorXmitTime.tv_usec))
		{
			dest->cursorXmitTime.tv_sec
					= rec->transmitTime.tv_sec;
			dest->cursorXmitTime.tv_usec
					= rec->transmitTime.tv_usec;
			if (rec->transmissionCount > dest->predictedResends)
			{
				dest->predictedResends = rec->transmissionCount;
//tracePredictedResends = dest->predictedResends;
			}
		}

		/*	Adjust transmission limit: diminishes as an
		 *	exponential function of increase in backlog
		 *	of unacknowledged messages:
		 *	
		 *	If dest's backlog < this	...then transmission
		 *	fraction of max backlog...	limit is ...
		 *		1/(2**6 = 64)			5
		 *		1/(2**4 = 16)			4
		 *		1/(2**2 = 4)			3
		 *
		 *	If dest's backlog is not less than 1/4 of
		 *	the maximum backlog, the maximum number of
		 *	transmissions is 2 (that is, original
		 *	transmission and one retransmission).		*/

		maxTransmissions = DGR_MAX_XMIT;
		for (i = 0; i < NBR_OF_OCC_LVLS; i++)
		{
			if (dest->backlog < occupancy[i])
			{
				break;
			}

			maxTransmissions--;
		}

		if (rec->transmissionCount == maxTransmissions)
		{
			/*	Record will not be retransmitted, so
			 *	remove it from backlog.			*/

			dest->backlog -= (rec->contentLength
					+ sizeof(CapsuleId));
			dest->msgsInBacklog--;
		}
	}

	/*	Delete pendingResends object; sender will insert
	 *	a new one if necessary.					*/

	losePendingResend(sap, rec);

	/*	If record has reached the retransmit limit, send it
	 *	to the application as a delivery failure.		*/

	if (rec->transmissionCount == maxTransmissions)
	{
		/*	Remove record from database; it won't be
		 *	sent again.					*/

		removeRecord(sap, rec, arqElt);

		/*	If application doesn't want to be notified
		 *	of delivery failure for this record, just
		 *	erase it and return.				*/

		if ((rec->notificationFlags & DGR_NOTE_FAILED) == 0)
		{
			MRELEASE(rec);
			return 0;
		}

		/*	Pass the record back to the application as a
		 *	delivery failure.				*/

		rec->type = DgrDeliveryFailure;
		if (rec->errnbr == 0)
		{
			rec->errnbr = ETIMEDOUT;
		}

		return insertEvent(sap, rec);
	}

	/*	We're going to transmit this record one more time,
	 *	so leave it in its current location in the ARQ bucket
	 *	and tell sender to re-send it.				*/

	dest->serviceLoad += rec->contentLength;
	return insertSendReq(sap, rec);
}

static int	arq(DgrSAP *sap, time_t capsuleTime,
			unsigned int capsuleSeqNbr, RecordOperation op)
{
	DgrArqBucket	*bucket;
	LystElt		elt;
	DgrRecord	rec;
	DgrDest		*dest;
	int		destIdx;
	int		result;

	bucket = sap->buckets + (capsuleSeqNbr & DGR_SEQNBR_MASK);
	pthread_mutex_lock(&bucket->mutex);
	pthread_mutex_lock(&sap->destsMutex);
	if (op == SendMessage)
	{
		for (elt = lyst_last(bucket->msgs); elt; elt = lyst_prev(elt))
		{
			rec = (DgrRecord) lyst_data(elt);
			if (rec->capsule.id.time > capsuleTime)
			{
				continue;
			}

			if (rec->capsule.id.time < capsuleTime)
			{
				pthread_mutex_unlock(&sap->destsMutex);
				pthread_mutex_unlock(&bucket->mutex);
				return 0;	/*	What happened?	*/
			}

			/*	Found a match on time.			*/

			if (rec->capsule.id.seqNbr > capsuleSeqNbr)
			{
				continue;
			}

			if (rec->capsule.id.seqNbr < capsuleSeqNbr)
			{
				pthread_mutex_unlock(&sap->destsMutex);
				pthread_mutex_unlock(&bucket->mutex);
				return 0;	/*	What happened?	*/
			}

			/*	Found the matching record.		*/

			break;
		}

		if (elt == NULL)
		{
			pthread_mutex_unlock(&sap->destsMutex);
			pthread_mutex_unlock(&bucket->mutex);
			return 0;		/*	What happened?	*/
		}

		dest = findDest(sap, rec->portNbr, rec->ipAddress, &destIdx);
		result = sendMessage(sap, rec, elt, dest, destIdx);
		pthread_mutex_unlock(&sap->destsMutex);
		pthread_mutex_unlock(&bucket->mutex);
		return result;
	}

	/*	Timeout or ACK for previously sent message, so search
	 *	from the front of the list rather than the back.	*/

	for (elt = lyst_first(bucket->msgs); elt; elt = lyst_next(elt))
	{
		rec = (DgrRecord) lyst_data(elt);
		if (rec->capsule.id.time < capsuleTime)
		{
			continue;
		}

		if (rec->capsule.id.time > capsuleTime)
		{
			pthread_mutex_unlock(&sap->destsMutex);
			pthread_mutex_unlock(&bucket->mutex);
			return 0;	/*	Record is already gone.	*/
		}

		/*	Found a match on time.				*/

		if (rec->capsule.id.seqNbr < capsuleSeqNbr)
		{
			continue;
		}

		if (rec->capsule.id.seqNbr > capsuleSeqNbr)
		{
			pthread_mutex_unlock(&sap->destsMutex);
			pthread_mutex_unlock(&bucket->mutex);
			return 0;	/*	Record is already gone.	*/
		}

		/*	Found the matching record.			*/

		break;
	}

	if (elt == NULL)
	{
		pthread_mutex_unlock(&sap->destsMutex);
		pthread_mutex_unlock(&bucket->mutex);
		return 0;		/*	Record is already gone.	*/
	}

	dest = findDest(sap, rec->portNbr, rec->ipAddress, &destIdx);
	if (op == HandleAck)
	{
		result = handleAck(sap, rec, elt, dest, destIdx);
	}
	else
	{
		result = handleTimeout(sap, rec, elt, dest, destIdx);
	}

	pthread_mutex_unlock(&sap->destsMutex);
	pthread_mutex_unlock(&bucket->mutex);
	return result;
}

static void	resetDestActivity(DgrSAP *sap)
{
	int	i;
	DgrDest	*dest;
	int	maxActivity;
	int	mostActive;
	int	n;
	int	ceiling;
	DgrDest	*ceilingDest;

	pthread_mutex_lock(&sap->destsMutex);
	if (sap->destsCount < DGR_MAX_DESTS)
	{
		/*	Table is not full, no need to clear out dests
		 *	that used to be active but no longer are.	*/

		pthread_mutex_unlock(&sap->destsMutex);
		return;
	}

	/*	Time to clean house.  First, find the most active dest.	*/

	maxActivity = -1;
	for (i = 0; i < DGR_MAX_DESTS; i++)
	{
		dest = sap->dests + i;

		/*	Reduce activity count to current backlog for
		 *	all dests, and flag all as not yet inserted
		 *	into activity chain.				*/

		dest->msgsSent = dest->msgsInBacklog;
		dest->moreActiveDest = -1;
		dest->lessActiveDest = -1;
		if (dest->msgsSent > maxActivity)
		{
			maxActivity = dest->msgsSent;
			mostActive = i;
		}
	}

	sap->mostActiveDest = mostActive;
	sap->leastActiveDest = mostActive;
	dest = sap->dests + mostActive;
	dest->moreActiveDest = DGR_MAX_DESTS;

	/*	Now insert all other dests into the chain in descending
	 *	activity order.  Obviously there are faster ways to do
	 *	this sort, but we don't do it very often, so optimizing
	 *	it seems premature; something to come back to later.	*/

	for (n = 0; n < (DGR_MAX_DESTS - 1); n++)
	{
		ceiling = mostActive;
		ceilingDest = dest;
		maxActivity = -1;

		/*	Find most active dest other than those that
		 *	have already been sorted into the list.		*/

		for (i = 0; i < DGR_MAX_DESTS; i++)
		{
			dest = sap->dests + i;
			if (dest->moreActiveDest > 0)
			{
				continue;	/*	Already sorted.	*/
			}

			/*	This dest hasn't been reinserted into
			 *	the chain yet.				*/

			if (dest->msgsSent > maxActivity)
			{
				maxActivity = dest->msgsSent;
				mostActive = i;
			}
		}

		/*	Insert this one in front of the one found
		 *	on the prior pass.				*/

		ceilingDest->lessActiveDest = mostActive;
		sap->leastActiveDest = mostActive;
		dest = sap->dests + mostActive;
		dest->moreActiveDest = ceiling;
	}

	pthread_mutex_unlock(&sap->destsMutex);
}

static void	adjustRetard(DgrDest *dest)
{
	int	i;
	int	maxIncrease;
	int	maxDecrease;
	int	unusedCapacity;
	int	capacity;
	int	newDataRate;
	int	booster;

	if (dest->episodeCount < 8)
	{
		dest->episodeCount++;
	}

	/*	The basic rate control principle: base planned data
	 *	transmission rate on the mean rate of acknowledgement
	 *	over the recent past, inflated by a 50% aggressiveness
	 *	factor to increase the rate rapidly when recovering
	 *	from a period of lost connectivity.  Then compute
	 *	retard as a function of the planned transmission rate.	*/

//traceBytesAcknowledged = dest->bytesAcknowledged;
//traceBytesResent = dest->bytesTransmitted - dest->bytesOriginated;
#if 0
	maxChange = (dest->bytesToTransmit >> 3) & 0x1fffffff;
	maxChange = dest->bytesToTransmit - ((dest->bytesToTransmit >> 2) & 0x3fffffff);
#endif
	/*	Start by computing upper and lower limits on the
	 *	amount by which planned data transmission rate may
	 *	change in any one episode.				*/

	maxIncrease = dest->bytesToTransmit;	/*	Double.		*/
	maxDecrease = (dest->bytesToTransmit >> 3) & 0x1fffffff;

	/*	Next, finish compiling statistics for the current
	 *	episode.  Begin by computing the current episode's
	 *	unused capacity (number of bytes that could have been
	 *	sent but were not) as the episode's estimated total
	 *	capacity minus actual number of bytes transmitted.	*/

	i = dest->currentEpisode;
	unusedCapacity = dest->episodes[i].capacity - dest->bytesTransmitted;
	if (unusedCapacity < 0)
	{
		unusedCapacity = 0;
	}
//traceUnusedCapacity = unusedCapacity;

	dest->totalServiceLoad -= dest->episodes[i].serviceLoad;
	dest->totalBytesTransmitted -= dest->episodes[i].bytesTransmitted;
	dest->totalBytesAcknowledged -= dest->episodes[i].bytesAcknowledged;
	dest->totalUnusedCapacity -= dest->episodes[i].unusedCapacity;
	dest->episodes[i].serviceLoad = dest->serviceLoad;
	dest->episodes[i].bytesTransmitted = dest->bytesTransmitted;
	dest->episodes[i].bytesAcknowledged = dest->bytesAcknowledged;
	dest->episodes[i].unusedCapacity = unusedCapacity;
	dest->totalServiceLoad += dest->episodes[i].serviceLoad;
	dest->totalBytesTransmitted += dest->episodes[i].bytesTransmitted;
	dest->totalBytesAcknowledged += dest->episodes[i].bytesAcknowledged;
	dest->totalUnusedCapacity += dest->episodes[i].unusedCapacity;

	/*	Then advance to the next history slot.			*/

	if (++i > 7) i = 0;
	dest->currentEpisode = i;

	/*	Now start compiling statistics for the next episode:
	 *	insert the projected capacity for that episode, which
	 *	is estimated as the mean sum of bytes acknowledged and
	 *	unused capacity over the past eight episodes.		*/

	capacity = (dest->totalBytesAcknowledged + dest->totalUnusedCapacity)
			/ dest->episodeCount;
	dest->totalCapacity -= dest->episodes[i].capacity;
	dest->episodes[i].capacity = capacity;
	dest->totalCapacity += dest->episodes[i].capacity;

	/*	To compute the next episode's planned data transmission
	 *	rate, start with the episode's estimated capacity and
	 *	then inflate that rate by 50%.				*/

	booster = (capacity >> 1) & 0x7fffffff;
	newDataRate = capacity + booster;

	/*	Now limit change in data rate as determined earlier.	*/

	if (newDataRate < dest->bytesToTransmit)	/*	Slower.	*/
	{
		if ((dest->bytesToTransmit - newDataRate) > maxDecrease)
		{
			newDataRate = dest->bytesToTransmit - maxDecrease;
		}
	}
	else						/*	Faster.	*/
	{
		if ((newDataRate - dest->bytesToTransmit) > maxIncrease)
		{
			newDataRate = dest->bytesToTransmit + maxIncrease;
		}
	}

	/*	But never let data rate drop below minimum pulse rate.	*/

	if (newDataRate < MinBytesToTransmit)
	{
		newDataRate = MinBytesToTransmit;
	}

	dest->bytesToTransmit = newDataRate;
//traceBytesToTransmit = newDataRate;

	/*	To calculate retard (the number of microseconds of
	 *	delay to impose per byte of transmitted content),
	 *	divide the control episode duration in microseconds
	 *	by the number of bytes to transmit in the next
	 *	episode.						*/

	dest->retard = EpisodeUsec / newDataRate;
//traceRetard = dest->retard;

	/*	Note that retard may be zero, in the event that the
	 *	network is transmitting way faster than we need it to.	*/

//dgrtrace();
//traceBytesAcknowledged = 0;
//dest->serviceLoad = 0;
//traceBytesTransmitted = 0;
//traceBytesOriginated = 0;
//aggregateDelay = 0;
	dest->bytesAcknowledged = 0;
	dest->bytesTransmitted = 0;
	dest->bytesOriginated = 0;
}

static void	adjustRateControl(DgrSAP *sap)
{
	int	i;
	DgrDest	*dest;

//PUTS("---------------------------------------------------------------");

	pthread_mutex_lock(&sap->destsMutex);
	for (i = 0, dest = sap->dests; i < DGR_MAX_DESTS; i++, dest++)
	{
		if (dest->bytesToTransmit > 0)
		{
			adjustRetard(dest);
		}
	}

//appliedAcks = 0;
	pthread_mutex_unlock(&sap->destsMutex);
}

/*	*	*	DGR background thread functions		*	*/

static void	*sender(void *parm)
{
	DgrSAP		*sap = (DgrSAP *) parm;
	sigset_t	signals;
	LystElt		elt;
	SendReq		*req;
	time_t		capsuleTime;
	unsigned int	capsuleSeqNbr;

	sigfillset(&signals);
	pthread_sigmask(SIG_BLOCK, &signals, NULL);
	while (sap->state == DgrSapOpen)
	{
		if (llcv_wait(sap->outboundCV, llcv_lyst_not_empty,
					LLCV_BLOCKING))
		{
			crashThread(sap, "Sender thread failed on wait");
			return NULL;
		}

		if (sap->state != DgrSapOpen)
		{
			return NULL;
		}

		while (1)
		{
			llcv_lock(sap->outboundCV);
			elt = lyst_first(sap->outboundMsgs);
			if (elt == NULL)
			{
				llcv_unlock(sap->outboundCV);
				break;
			}

			req = (SendReq *) lyst_data(elt);
			capsuleTime = req->id.time;
			capsuleSeqNbr = req->id.seqNbr;
			llcv_unlock(sap->outboundCV);
			if (arq(sap, capsuleTime, capsuleSeqNbr, SendMessage))
			{
				putErrmsg("Sender thread terminated.", NULL);
				return NULL;
			}
		}
	}

	return NULL;
}

static void	*resender(void *parm)
{
	DgrSAP		*sap = (DgrSAP *) parm;
	sigset_t	signals;
	int		cycleNbr = 1;
	struct timeval	currentTime;
	LystElt		elt;
	ResendReq	*req;
	time_t		capsuleTime;
	unsigned int	capsuleSeqNbr;

	sigfillset(&signals);
	pthread_sigmask(SIG_BLOCK, &signals, NULL);
	while (1)
	{
		microsnooze(EpisodeUsec);
		if (sap->state != DgrSapOpen)
		{
			return NULL;
		}

		adjustRateControl(sap);
		if ((cycleNbr & 0x7f) == 0)	/*	Every 128th.	*/
		{
			resetDestActivity(sap);
		}

		/*	Deal with all retransmissions that are now due.	*/

		getCurrentTime(&currentTime);
		while (1)
		{
			pthread_mutex_lock(&sap->pendingResendsMutex);
			elt = lyst_first(sap->pendingResends);
			if (elt == NULL)
			{
				pthread_mutex_unlock(&sap->pendingResendsMutex);
				break;	/*	No more for now.	*/
			}

			req = (ResendReq *) lyst_data(elt);
			if (req->resendTime.tv_sec > currentTime.tv_sec)
			{
				pthread_mutex_unlock(&sap->pendingResendsMutex);
				break;	/*	No more for now.	*/
			}

			if (req->resendTime.tv_sec < currentTime.tv_sec
			|| req->resendTime.tv_usec <= currentTime.tv_usec)
			{
				/*	Capsule's resend time is here.	*/

				capsuleTime = req->id.time;
				capsuleSeqNbr = req->id.seqNbr;
				pthread_mutex_unlock(&sap->pendingResendsMutex);
				if (arq(sap, capsuleTime, capsuleSeqNbr,
						HandleTimeout))
				{
					putErrmsg("Resender thread terminated.",
							NULL);
					return NULL;
				}

				continue;
			}

			pthread_mutex_unlock(&sap->pendingResendsMutex);
			break;		/*	No more for now.	*/
		}
	}
}

static void	*receiver(void *parm)
{
	DgrSAP			*sap = (DgrSAP *) parm;
	char			*buffer;
	sigset_t		signals;
	struct sockaddr_in	socketAddress;
	struct sockaddr		*sockName = (struct sockaddr *) &socketAddress;
	socklen_t		sockaddrlen;
	unsigned short		portNbr;
	Capsule			*capsule;
	int			length;
	int			reclength;
	DgrRecord		rec;

	buffer = MTAKE(DGR_BUF_SIZE);
	if (buffer == NULL)
	{
		putSysErrmsg("Receiver thread can't allocate buffer", NULL);
		return NULL;
	}

	sigfillset(&signals);
	pthread_sigmask(SIG_BLOCK, &signals, NULL);
	capsule = (Capsule *) buffer;
	while (1)
	{
		sockaddrlen = sizeof(struct sockaddr_in);
		length = recvfrom(sap->udpSocket, buffer, DGR_BUF_SIZE, 0,
				sockName, &sockaddrlen);
		if (length < 0)
		{
			switch (errno)
			{
			case EBADF:	/*	Socket has been closed.	*/
				break;	/*	Out of switch.		*/

			case EAGAIN:	/*	Bad checksum?  Ignore.	*/
				continue;

			default:
				crashThread(sap, "Receiver thread failed on \
recvfrom");
				break;	/*	Out of switch.		*/
			}

			break;		/*	Out of main loop.	*/
		}

		if (sap->state != DgrSapOpen)
		{
			break;		/*	Out of main loop.	*/
		}

		if (length < sizeof(CapsuleId))
		{
			continue;	/*	Ignore random guck.	*/
		}

		if (length == sizeof(CapsuleId))	/*	ACK	*/
		{
			if (arq(sap, ntohl(capsule->id.time),
				ntohl(capsule->id.seqNbr), HandleAck))
			{
				putErrmsg("Receiver thread has terminated.",
						NULL);
				break;	/*	Out of main loop.	*/
			}

			continue;
		}

		/*	Inbound message.  Extract sender's port nbr.	*/

		portNbr = ntohs(socketAddress.sin_port);

		/*	Now send acknowledgment.			*/

		if (sendto(sap->udpSocket, (char *) capsule, sizeof(CapsuleId),
					0, sockName, sockaddrlen) < 0)
		{
			if (errno == EBADF)	/*	Socket closed.	*/
			{
				break;	/*	Out of main loop.	*/
			}

			crashThread(sap, "Receiver thread failed sending \
acknowledgement");
			break;		/*	Out of main loop.	*/
		}

		/*	Create content arrival event.			*/

		length -= sizeof(CapsuleId);	/*	content length	*/
		reclength = sizeof(struct dgr_rec) + (length - 1);
		rec = (DgrRecord) MTAKE(reclength);
		if (rec == NULL)
		{
			crashThread(sap, "Receiver thread failed creating \
content arrival event");
			break;		/*	Out of main loop.	*/
		}

		memset((char *) rec, 0, reclength);
		rec->type = DgrMsgIn;
		rec->portNbr = portNbr;
		rec->ipAddress = ntohl(socketAddress.sin_addr.s_addr);
		rec->contentLength = length;
		memcpy(rec->capsule.content, capsule->content, length);
		if (insertEvent(sap, rec))
		{
			putErrmsg("Receiver thread has terminated.", NULL);
			break;		/*	Out of main loop.	*/
		}
	}

	/*	Main loop terminated for some reason; wrap up.		*/

	MRELEASE(buffer);
	return NULL;
}

/*	*	*	DGR API functions	*	*	*	*/

static void	forgetObject(LystElt elt, void *userData)
{
	MRELEASE(lyst_data(elt));
}

static void	cleanUpSAP(DgrSAP *sap)
{
	int		i;
	DgrArqBucket	*bucket;

	if (sap->udpSocket >= 0)
	{
		close(sap->udpSocket);
	}

	if (sap->outboundMsgs)
	{
		lyst_delete_set(sap->outboundMsgs, forgetObject, NULL);
		lyst_destroy(sap->outboundMsgs);
	}

	if (sap->outboundCV)
	{
		llcv_close(sap->outboundCV);
	}

	if (sap->inboundEvents)
	{
		lyst_delete_set(sap->inboundEvents, forgetObject, NULL);
		lyst_destroy(sap->inboundEvents);
	}

	if (sap->inboundCV)
	{
		llcv_close(sap->inboundCV);
	}

	for (i = 0, bucket = sap->buckets; i < DGR_BUCKETS; i++, bucket++)
	{
		if (bucket->msgs)
		{
			lyst_delete_set(bucket->msgs, forgetObject, NULL);
			lyst_destroy(bucket->msgs);
		}

		pthread_mutex_destroy(&bucket->mutex);
	}

	for (i = 0; i < DGR_BIN_COUNT; i++)
	{
		lyst_destroy(sap->destLysts[i]);
	}

	if (sap->pendingResends)
	{
		lyst_delete_set(sap->pendingResends, forgetObject, NULL);
		lyst_destroy(sap->pendingResends);
	}

	pthread_mutex_lock(&sap->sapMutex);
	pthread_cond_destroy(&sap->sapCV);
	pthread_mutex_destroy(&sap->sapMutex);
	pthread_mutex_destroy(&sap->pendingResendsMutex);
	pthread_mutex_destroy(&sap->destsMutex);
	MRELEASE(sap);
}

DgrRC	dgr_open(unsigned short ownPortNbr, unsigned int ownIpAddress,
		char *memmgrName, DgrSAP **sapp)
{
	char			hostname[MAXHOSTNAMELEN + 1];
	struct sockaddr_in	socketAddress;
	struct sockaddr		*sockName = (struct sockaddr *) &socketAddress;
	int			mmid;
	DgrSAP			*sap;
	int			i;
	int			factor;
	DgrArqBucket		*bucket;

	if (sapp == NULL)
	{
		errno = EINVAL;
		putSysErrmsg("'Dgr' pointer is NULL", NULL);
		return DgrFailed;
	}

	/*	Validate endpoint address.				*/

	if (ownIpAddress == 0)
	{
		if (getNameOfHost(hostname, sizeof hostname) < 0)
		{
			putErrmsg("Can't get name of local host.", NULL);
			return DgrFailed;
		}
		
		ownIpAddress = getInternetAddress(hostname);
		if (ownIpAddress == 0)
		{
			putErrmsg("Can't get IP address of local host.",
					NULL);
			return DgrFailed;
		}
	}

	memset((char *) sockName, 0, sizeof(struct sockaddr_in));
	socketAddress.sin_family = AF_INET;
	ownIpAddress = htonl(ownIpAddress);
	socketAddress.sin_addr.s_addr = ownIpAddress;
	ownPortNbr = htons(ownPortNbr);
	socketAddress.sin_port = ownPortNbr;

	/*	Determine memory management procedures.			*/

	if (memmgrName)
	{
		mmid = memmgr_find(memmgrName);
		if (mmid < 0)
		{
			putErrmsg("Can't find memory manager.", memmgrName);
			return DgrFailed;
		}
	}
	else
	{
		mmid = 0;	/*	default is malloc/free		*/
	}

	/*	Initialize program constants.				*/

	MinMicrosnooze = SystemClockResolution;
	for (i = 0, factor = 64; i < NBR_OF_OCC_LVLS; i++, factor /= 4)
	{
		occupancy[i] = MaxBacklog / factor;
	}

	/*	(MinBytesToTransmit in each episode is based on
	 *	minimum transmission rate of 1200 bps = 150 bytes
	 *	per second; must scale 150 by length of rate control
	 *	episode.)						*/

	MinBytesToTransmit = (150 * EpisodeUsec) / 1000000;
	if (memmgrId < 0)
	{
		memmgrId = mmid;
		mtake = memmgr_take(memmgrId);
		mrelease = memmgr_release(memmgrId);
	}
	else
	{
		if (mmid != memmgrId)
		{
			errno = EINVAL;
			putSysErrmsg("Memory manager selections conflict",
					memmgrName);
			return DgrFailed;
		}
	}

	/*	Initialize service access point constants.		*/

	sap = (DgrSAP *) MTAKE(sizeof(DgrSAP));
	if (sap == NULL)
	{
		putErrmsg("Can't create DGR service access point.", NULL);
		return DgrFailed;
	}

	memset((char *) sap, 0, sizeof(DgrSAP));
	sap->state = DgrSapOpen;
	sap->leastActiveDest = -1;
	sap->mostActiveDest = DGR_MAX_DESTS;
	initializeDest(&sap->defaultDest, 0, 0);

	/*	Initialize UDP socket for DGR service.			*/

	sap->udpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sap->udpSocket < 0)
	{
		putSysErrmsg("Can't open DGR UDP socket", NULL);
		cleanUpSAP(sap);
		return DgrFailed;
	}

	if (reUseAddress(sap->udpSocket) < 0)
	{
		cleanUpSAP(sap);
		putSysErrmsg("Can't reuse address on DGR UDP socket",
				utoa(ntohs(ownPortNbr)));
		return DgrFailed;
	}

	if (bind(sap->udpSocket, sockName, sizeof(struct sockaddr)) < 0)
	{
		cleanUpSAP(sap);
		putSysErrmsg("Can't bind DGR UDP socket",
				utoa(ntohs(ownPortNbr)));
		return DgrFailed;
	}

	closeOnExec(sap->udpSocket);

	/*	Create lists and management structures.			*/

	if ((sap->outboundMsgs = lyst_create_using(memmgrId)) == NULL)
	{
		putErrmsg("Can't create list of outbound messages.", NULL);
		cleanUpSAP(sap);
		return DgrFailed;
	}

	if ((sap->outboundCV = llcv_open(sap->outboundMsgs,
			&(sap->outboundCV_str))) == NULL)
	{
		putErrmsg("Can't outbound messages CV.", NULL);
		cleanUpSAP(sap);
		return DgrFailed;
	}

	if ((sap->inboundEvents = lyst_create_using(memmgrId)) == NULL)
	{
		putErrmsg("Can't create list of inbound events.", NULL);
		cleanUpSAP(sap);
		return DgrFailed;
	}

	if ((sap->inboundCV = llcv_open(sap->inboundEvents,
			&(sap->inboundCV_str))) == NULL)
	{
		putErrmsg("Can't create inbound events CV.", NULL);
		cleanUpSAP(sap);
		return DgrFailed;
	}

	for (i = 0; i < DGR_BIN_COUNT; i++)
	{
		if ((sap->destLysts[i] = lyst_create_using(memmgrId)) == NULL)
		{
			putErrmsg("Can't create list of destinations.", NULL);
			cleanUpSAP(sap);
			return DgrFailed;
		}
	}

	for (i = 0, bucket = sap->buckets; i < DGR_BUCKETS; i++, bucket++)
	{
		if ((bucket->msgs = lyst_create_using(memmgrId)) == NULL
		|| pthread_mutex_init(&bucket->mutex, NULL))
		{
			putSysErrmsg("Can't create message bucket", NULL);
			cleanUpSAP(sap);
			return DgrFailed;
		}
	}

	if ((sap->pendingResends = lyst_create_using(memmgrId)) == NULL)
	{
		putErrmsg("Can't create list of resend requests.", NULL);
		cleanUpSAP(sap);
		return DgrFailed;
	}

	if (pthread_mutex_init(&sap->sapMutex, NULL)
	|| pthread_cond_init(&sap->sapCV, NULL)
	|| pthread_mutex_init(&sap->pendingResendsMutex, NULL)
	|| pthread_mutex_init(&sap->destsMutex, NULL))
	{
		putSysErrmsg("Can't initialize mutex(es)", NULL);
		cleanUpSAP(sap);
		return DgrFailed;
	}

	/*	Spawn all threads.					*/

	if (pthread_create(&(sap->sender), NULL, sender, sap)
	|| pthread_create(&(sap->resender), NULL, resender, sap)
	|| pthread_create(&(sap->receiver), NULL, receiver, sap))
	{
		putSysErrmsg("Can't spawn thread(s)", NULL);
		cleanUpSAP(sap);
		return DgrFailed;
	}

	*sapp = sap;
	return DgrOpened;
}

void	dgr_getsockname(DgrSAP *sap, unsigned short *portNbr,
		unsigned int *ipAddress)
{
	struct sockaddr		buf;
	socklen_t		buflen = sizeof buf;
	struct sockaddr_in	*nm = (struct sockaddr_in *) &buf;

	*portNbr = *ipAddress = 0;	/*	Default.		*/
	if (sap == NULL)
	{
		return;
	}

	if (getsockname(sap->udpSocket, &buf, &buflen) < 0)
	{
		return;
	}

	memcpy((char *) ipAddress, (char *) &(nm->sin_addr.s_addr), 4);
	*ipAddress = ntohl(*ipAddress);
	*portNbr = nm->sin_port;
	*portNbr = ntohs(*portNbr);
}

void	dgr_close(DgrSAP *sap)
{
	struct sockaddr	sockName;
       	socklen_t	sockNameLen = sizeof(sockName);
	char		shutdown = 1;
	int		result;

	if (sap == NULL || sap->state == DgrSapClosed)
	{
		return;
	}

	sap->state = DgrSapClosed;

	/*	Terminate any dgr_receive that is currently in
	 *	progress.						*/

	llcv_signal(sap->inboundCV, time_to_stop);

	/*	Tell the sender thread to shut itself down.		*/

	llcv_signal(sap->outboundCV, time_to_stop);
	pthread_join(sap->sender, NULL);

	/*	The resender thread will shut down on its next timeout.	*/

	pthread_join(sap->resender, NULL);

	/*	Prompt the receiver thread to shut itself down.		*/

	if (getsockname(sap->udpSocket, &sockName, &sockNameLen) == 0)
	{
		result = sendto(sap->udpSocket, &shutdown, 1, 0, &sockName,
				sizeof(struct sockaddr_in));
	}

	pthread_join(sap->receiver, NULL);

	/*	Now destroy all remaining management and communication
	 *	structures.						*/

	cleanUpSAP(sap);
}

DgrRC	dgr_send(DgrSAP *sap, unsigned short toPortNbr,
		unsigned int toIpAddress, int notificationFlags, char *content,
		int length)
{
	int		reclength;
	DgrRecord	rec;
	DgrDest		*dest;
	int		destIdx;
	int		delay;			/*	microseconds	*/
	int		usecToSnooze;
	int		usecSnoozed;
	time_t		currentTime;
	LystElt		elt;

	if (sap == NULL || toPortNbr == 0 || toIpAddress == 0
	|| notificationFlags < 0 || notificationFlags > 3
	|| content == NULL || length < 1 || length > DGR_BUF_SIZE)
	{
		errno = EINVAL;
		putSysErrmsg("Invalid transmission parameter(s)", NULL);
		return DgrFailed;
	}

	if (sap->state == DgrSapDamaged)
	{
		errno = EINVAL;
		putSysErrmsg("Access point damaged; close and reopen", NULL);
		return DgrFailed;
	}

	if (sap->state == DgrSapClosed)
	{
		errno = EINVAL;
		putSysErrmsg("Access point has not been opened", NULL);
		return DgrFailed;
	}

	/*	Construct the outbound DGR record.			*/

	reclength = sizeof(struct dgr_rec) + (length - 1);
	rec = (DgrRecord) MTAKE(reclength);
	if (rec == NULL)
	{
		putSysErrmsg("Can't create outbound message record", NULL);
		return DgrFailed;
	}

	memset((char *) rec, 0, reclength);
	rec->type = DgrMsgOut;
	rec->portNbr = toPortNbr;
	rec->ipAddress = toIpAddress;
	rec->notificationFlags = notificationFlags;
	rec->contentLength = length;
	memcpy(rec->capsule.content, content, length);

	/*	Apply rate control.  First compute delay: multiply
	 *	content length by computed "retard" rate for this
	 *	destination, the number of microseconds of delay
	 *	per byte to impose whenever sending a datagram to
	 *	the destination, and update statistics for future
	 *	rate control adjustment.				*/

	pthread_mutex_lock(&sap->destsMutex);
	dest = findDest(sap, toPortNbr, toIpAddress, &destIdx);
	dest->serviceLoad += length;
	delay = length * dest->retard;
//aggregateDelay += delay;

	/*	Now apply the computed rate control delay.		*/

	dest->pendingDelay += delay;
	usecToSnooze = dest->pendingDelay;
	pthread_mutex_unlock(&sap->destsMutex);
	usecSnoozed = 0;
	while (usecToSnooze > SystemClockResolution)
	{
//rcSnoozes++;
		microsnooze(SystemClockResolution);
		usecToSnooze -= SystemClockResolution;
		usecSnoozed += SystemClockResolution;
	}

	if (usecSnoozed > 0)
	{
		pthread_mutex_lock(&sap->destsMutex);
		dest->pendingDelay -= usecSnoozed;
		pthread_mutex_unlock(&sap->destsMutex);
	}

	/*	Safety net: prevent volume of in-process messages
	 *	from getting out of hand.				*/

	pthread_mutex_lock(&sap->sapMutex);
	while (sap->backlog > MaxBacklog)
	{
		pthread_cond_wait(&sap->sapCV, &sap->sapMutex);
	}

	/*	Now proceed with transmission.				*/

	currentTime = time(NULL);
	sap->backlog += (rec->contentLength + sizeof(CapsuleId));
	if (currentTime == sap->capsuleTime)
	{
		sap->capsuleSeqNbr++;
	}
	else
	{
		sap->capsuleTime = currentTime;
		sap->capsuleSeqNbr = 0;
	}

	rec->capsule.id.time = sap->capsuleTime;
	rec->capsule.id.seqNbr = sap->capsuleSeqNbr;
	pthread_mutex_unlock(&sap->sapMutex);

	/*	Store in a bucket of the DGR ARQ (record) database.
	 *	Select bucket by computing capsuleSeqNbr modulo the
	 *	number of buckets - 1.					*/

	rec->bucket = sap->buckets + (rec->capsule.id.seqNbr & DGR_SEQNBR_MASK);

	/*	Insert the new record in the selected database bucket.	*/

	pthread_mutex_lock(&rec->bucket->mutex);
	elt = lyst_insert_last(rec->bucket->msgs, rec);
	pthread_mutex_unlock(&rec->bucket->mutex);
	if (elt == NULL)				/*	Bail.	*/
	{
		putErrmsg("Can't append outbound record.", NULL);
		MRELEASE(rec);
		pthread_mutex_lock(&sap->sapMutex);
		sap->backlog -= (length + sizeof(CapsuleId));
		pthread_cond_signal(&sap->sapCV);
		pthread_mutex_unlock(&sap->sapMutex);
		pthread_mutex_lock(&sap->destsMutex);
		dest = findDest(sap, toPortNbr, toIpAddress, &destIdx);
		dest->serviceLoad -= length;
		pthread_mutex_unlock(&sap->destsMutex);
		return DgrFailed;
	}

	/*	Insert transmission request for the sender thread to
	 *	act on.							*/

	if (insertSendReq(sap, rec) < 0)
	{
		putErrmsg("Can't append tranmission request.", NULL);
		lyst_delete(elt);
		MRELEASE(rec);
		pthread_mutex_lock(&sap->sapMutex);
		sap->backlog -= (length + sizeof(CapsuleId));
		pthread_cond_signal(&sap->sapCV);
		pthread_mutex_unlock(&sap->sapMutex);
		pthread_mutex_lock(&sap->destsMutex);
		dest = findDest(sap, toPortNbr, toIpAddress, &destIdx);
		dest->serviceLoad -= length;
		pthread_mutex_unlock(&sap->destsMutex);
		return DgrFailed;
	}

	return DgrDatagramSent;
}

DgrRC	dgr_receive(DgrSAP *sap, unsigned short *fromPortNbr,
		unsigned int *fromIpAddress, char *content, int *length,
		int *errnbr, int timeoutSeconds)
{
	time_t		currentTime;
	time_t		wakeupTime;
	int		timeoutUsec;
	LystElt		elt;
	DgrRecord	rec;
	DgrRC		result = DgrFailed;

	if (sap == NULL || fromPortNbr == NULL || fromIpAddress == NULL
	|| content == NULL || length == NULL)
	{
		errno = EINVAL;
		putSysErrmsg("Invalid reception parameter(s)", NULL);
		return result;
	}

	if (sap->state == DgrSapDamaged)
	{
		errno = EINVAL;
		putSysErrmsg("Access point damaged; close and reopen", NULL);
		return DgrFailed;
	}

	if (sap->state == DgrSapClosed)
	{
		errno = EINVAL;
		putSysErrmsg("Access point has not been opened", NULL);
		return DgrFailed;
	}

	if (timeoutSeconds == DGR_BLOCKING)
	{
		timeoutUsec = LLCV_BLOCKING;
	}
	else
	{
		currentTime = time(NULL);
		wakeupTime = currentTime + timeoutSeconds;
		timeoutUsec = timeoutSeconds * 1000000;
	}

	while (1)
	{
		if (llcv_wait(sap->inboundCV, llcv_lyst_not_empty, timeoutUsec))
		{
			if (errno == ETIMEDOUT)
			{
				result = DgrTimedOut;
			}
			else
			{
				putErrmsg("Reception failed in wait.", NULL);
				result = DgrFailed;
			}

			/*	Lyst is still empty, or some error.	*/

			return result;
		}

		/*	Lyst is no longer empty, or SAP is now closed.	*/

		break;				/*	Out of loop.	*/
	}

	/*	SAP might have been closed while we were sleeping.	*/

	if (sap->state == DgrSapDamaged)
	{
		putErrmsg("Access point no longer usable.", NULL);
		return DgrFailed;
	}

	if (sap->state == DgrSapClosed)
	{
		writeMemo("[i] DGR access point has been closed.");
		return DgrFailed;
	}

	llcv_lock(sap->inboundCV);
	elt = lyst_first(sap->inboundEvents);
	rec = (DgrRecord) lyst_data(elt);
	lyst_delete(elt);
	llcv_unlock(sap->inboundCV);
	if (rec == NULL)		/*	Interrupted.		*/
	{
		return DgrInterrupted;
	}

	*length = rec->contentLength;
	memcpy(content, rec->capsule.content, rec->contentLength);
	switch (rec->type)
	{
	case DgrMsgIn:
		*fromPortNbr = rec->portNbr;
		*fromIpAddress = rec->ipAddress;
		result = DgrDatagramReceived;
		break;

	case DgrDeliverySuccess:
		result = DgrDatagramAcknowledged;
		break;

	default:
		*errnbr = rec->errnbr;
		result = DgrDatagramNotAcknowledged;
	}

	/*	Reclaim memory occupied by the inbound message
	 *	or delivery failure.					*/

	MRELEASE(rec);
	return result;
}

void	dgr_interrupt(DgrSAP *sap)
{
	int	result;

	result = insertEvent(sap, NULL);
}
