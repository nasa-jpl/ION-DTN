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
#include "llcv.h"

#ifndef DGRDEBUG
#define DGRDEBUG	0
#endif

#ifndef DGRWATCHING
#define DGRWATCHING	0
#endif

#define	DGR_DB_ORDER	(8)
#define	DGR_BUCKETS	(1 << DGR_DB_ORDER)
#define	DGR_BIN_COUNT	(257)		/*	Must be prime for hash.	*/
#define	DGR_MAX_DESTS	(256)
#define	DGR_SESNBR_MASK	(DGR_BUCKETS - 1)
#define	DGR_MIN_XMIT	(2)
#define	NBR_OF_OCC_LVLS	(3)
#define DGR_MAX_XMIT	(DGR_MIN_XMIT + NBR_OF_OCC_LVLS)

#define	DGR_BUF_SIZE	(65535)
#define	MAX_DATA_HDR	(58)
#define MAX_DATA_SIZE	(DGR_BUF_SIZE - MAX_DATA_HDR)

#define	MTAKE(size)	sap->mtake(__FILE__, __LINE__, size)
#define MRELEASE(ptr)	sap->mrelease(__FILE__, __LINE__, ptr)

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

static int	_watching()
{
	static int	watching = DGRWATCHING;

	return watching;
}

static int	_occupancy(int j)
{
	static int	occupancy[NBR_OF_OCC_LVLS];
	static int	initialized = 0;
	int		i;
	int		factor;

	if (!initialized)
	{
		for (i = 0, factor = 64; i < NBR_OF_OCC_LVLS; i++, factor /= 4)
		{
			occupancy[i] = MAX_BACKLOG / factor;
		}

		initialized = 1;
	}

	return occupancy[j];
}

/*	DGR is a simplified LTP: each block is transmitted as a
 *	single segment in a single UDP datagram.  So each segment
 *	contains all bytes of all service data units in a single
 *	block, so there is a one-to-one correspondence between
 *	segments and sessions.						*/

typedef struct
{
	/*	Segment ID is an LTP session (block) ID.		*/

	uvast		engineId;
	unsigned int	sessionNbr;
} SegmentId;

typedef struct
{
	SegmentId	id;
	char		content[1];
} Segment;

typedef enum
{
	DgrMsgOut = 1,
	DgrMsgIn, 
	DgrDeliverySuccess,
	DgrDeliveryFailure
} DgrRecordType;

typedef struct
{
	Lyst		msgs;		/*	(DgrRecord *)		*/
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
	Segment		segment;
} *DgrRecord;

typedef struct
{
	SegmentId	id;
} SendReq;

typedef struct
{
	SegmentId	id;
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
	/*	Dests are functionally equivalent to the Spans in
	 *	ION's LTP implementation, but entirely volatile.	*/

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
	/*	The DgrSAP is roughly equivalent to the LTP database
	 *	in ION's LTP implementation, but entirely volatile
	 *	and allocated only to a single client service.		*/

	uvast		engineId;
	unsigned int	clientSvcId;
	DgrSapState	state;
	MemAllocator	mtake;
	MemDeallocator	mrelease;
	int		udpSocket;

	pthread_mutex_t	sapMutex;
	pthread_cond_t	sapCV;
	unsigned int	sessionNbr;
	int		backlog;	/*	Total, for all dests.	*/

	Lyst		outboundMsgs;	/*	(SendReq *)		*/
	struct llcv_str	outboundCV_str;
	Llcv		outboundCV;

	Lyst		pendingResends;	/*	(ResendReq *)		*/
	pthread_mutex_t	pendingResendsMutex;

	Lyst		inboundEvents;	/*	(DgrRecord *)		*/
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

	char		inputBuffer[DGR_BUF_SIZE];
	char		outputBuffer[DGR_BUF_SIZE];
} DgrSAP;

typedef enum
{
	DgrSendMessage = 1,
	DgrHandleTimeout,
	DgrHandleRpt
} RecordOperation;

#if DGRDEBUG
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

	putErrmsg(msg, NULL);
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
	uaddr	i;
	DgrDest	*dest;

	bin = hashDestId(portNbr, ipAddress);
	for (elt = lyst_first(sap->destLysts[bin]); elt; elt = lyst_next(elt))
	{
		dest = sap->dests + (i = (uaddr) lyst_data(elt));
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
	dest->retard = INITIAL_RETARD;
	dest->bytesToTransmit = EPISODE_PERIOD / dest->retard;
#if DGRDEBUG
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
	uaddr	newDest;
	DgrDest	*dest;
	uaddr	nextDest;
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
	sap->backlog -= (rec->contentLength + sizeof(SegmentId));
	if (sap->backlog <= MAX_BACKLOG)
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
	static int	minTimeout = MIN_TIMEOUT * 1000000;
	static int	maxTimeout = MAX_TIMEOUT * 1000000;

	req = MTAKE(sizeof(ResendReq));
	if (req == NULL)
	{
		crashThread(sap, "Can't create resend request");
		return -1;
	}

	req->id.engineId = rec->segment.id.engineId;
	req->id.sessionNbr = rec->segment.id.sessionNbr;
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

#if DGRDEBUG
resends[rec->transmissionCount - 2]++;
#endif
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

		dest->backlog += (rec->contentLength + sizeof(SegmentId));
		dest->msgsInBacklog++;
		dest->msgsSent++;
		adjustActiveDestChain(sap, destIdx);
		dest->bytesOriginated += rec->contentLength;
#if DGRDEBUG
traceBytesOriginated = dest->bytesOriginated;
originalMsgs++;
#endif
	}

	dest->bytesTransmitted += rec->contentLength;
#if DGRDEBUG
traceBytesTransmitted = dest->bytesTransmitted;
#endif
	if (rtt < minTimeout)
	{
		rtt = minTimeout;
	}

	if (rtt > maxTimeout)
	{
		rtt = maxTimeout;
	}

#if DGRDEBUG
computedRtt = rtt;
#endif
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
	unsigned char		*cursor;
	int			length;
	Sdnv			sdnv;
	unsigned int		ckptSerialNbr = rand();
	struct sockaddr_in	socketAddress;
	struct sockaddr		*sockName = (struct sockaddr *) &socketAddress;

	/*	Construct LTP data segment in sap->outputBuffer.	*/

	cursor = (unsigned char *) (sap->outputBuffer);
	length = 0;

	/*	Version number and segment type.			*/

	*cursor = 3;		/*	0000 0011			*/
	length++;
	cursor++;

	/*	Engine ID.						*/

	encodeSdnv(&sdnv, rec->segment.id.engineId);
	memcpy(cursor, sdnv.text, sdnv.length);
	length += sdnv.length;
	cursor += sdnv.length;

	/*	SessionNbr.						*/

	encodeSdnv(&sdnv, rec->segment.id.sessionNbr);
	memcpy(cursor, sdnv.text, sdnv.length);
	length += sdnv.length;
	cursor += sdnv.length;

	/*	Extension lengths.					*/

	*cursor = 0;		/*	0000 0000			*/
	length++;
	cursor++;

	/*	Client service ID.					*/

	encodeSdnv(&sdnv, sap->clientSvcId);
	memcpy(cursor, sdnv.text, sdnv.length);
	length += sdnv.length;
	cursor += sdnv.length;

	/*	Service data offset.					*/

	encodeSdnv(&sdnv, 0);
	memcpy(cursor, sdnv.text, sdnv.length);
	length += sdnv.length;
	cursor += sdnv.length;

	/*	Service data length.					*/

	encodeSdnv(&sdnv, rec->contentLength);
	memcpy(cursor, sdnv.text, sdnv.length);
	length += sdnv.length;
	cursor += sdnv.length;

	/*	Checkpoint serial number.				*/

	encodeSdnv(&sdnv, ckptSerialNbr);
	memcpy(cursor, sdnv.text, sdnv.length);
	length += sdnv.length;
	cursor += sdnv.length;

	/*	Report serial number.					*/

	encodeSdnv(&sdnv, 0);
	memcpy(cursor, sdnv.text, sdnv.length);
	length += sdnv.length;
	cursor += sdnv.length;

	/*	Data content.						*/

	memcpy(cursor, rec->segment.content, rec->contentLength);
	length += rec->contentLength;
	cursor += rec->contentLength;

	/*	Plug current time into record's transmit time.		*/

	getCurrentTime(&(rec->transmitTime));

	/*	Send the message.  Error results returned by sendto
	 *	do not crash the thread; eventually the application
	 *	will be notified of the delivery failure and take
	 *	some action such as closing the SAP.			*/

	socketAddress.sin_family = AF_INET;
	socketAddress.sin_port = htons(rec->portNbr);
	socketAddress.sin_addr.s_addr = htonl(rec->ipAddress);
	if (isendto(sap->udpSocket, sap->outputBuffer, length, 0, sockName,
			sizeof(struct sockaddr_in)) < 0)
	{
		putSysErrmsg("DGR can't send segment", NULL);
		rec->errnbr = errno;
	}

	if (_watching())
	{
		iwatch('g');
	}

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
	 *	computed round-trip time for this segment, so we
	 *	can't use that RTT to predict appropriate retransmit
	 *	times for future segments).				*/

	getCurrentTime(&arrivalTime);

	/*	At the least, update backlog count to mitigate
	 *	maximum transmissions limit.				*/

	dest->backlog -= (rec->contentLength + sizeof(SegmentId));
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
#if DGRDEBUG
traceMeasuredRtt = measuredRtt;
#endif

	/*	Add 1/8 of deviation to smoothed RTT.			*/

	deviation = measuredRtt - dest->rttSmoothed;
#if DGRDEBUG
traceRttDeviation = deviation;
#endif
	negative = deviation < 0 ? 1 : 0;
	deviation = abs(deviation);
	adj = (deviation >> 3) & 0x1fffffff;
	if (negative) adj = 0 - adj;
	dest->rttSmoothed += adj;
#if DGRDEBUG
traceRttSmoothed = dest->rttSmoothed;
#endif

	/*	Add 1/4 of change in deviation to mean deviation.	*/

	change = deviation - dest->meanRttDeviation;
	negative = change < 0 ? 1 : 0;
	change = abs(change);
	adj = (change >> 2) & 0x3fffffff;
	if (negative) adj = 0 - adj;
	dest->meanRttDeviation += adj;
#if DGRDEBUG
traceMeanRttDeviation = dest->meanRttDeviation;
#endif

	/*	For new prediction of RTT use smoothed RTT plus four
	 *	times the mean deviation.				*/

	dest->rttPredicted = dest->rttSmoothed + (dest->meanRttDeviation << 2);
#if DGRDEBUG
traceRttPredicted = dest->rttPredicted;
#endif
	dest->predictedResends = 0;
#if DGRDEBUG
tracePredictedResends = dest->predictedResends;
#endif
}

static int	handleRpt(DgrSAP *sap, DgrRecord rec, LystElt arqElt,
			DgrDest *dest, int destIdx)
{
#if DGRDEBUG
appliedAcks++;
#endif
	if (_watching())
	{
		iwatch('h');
	}

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

	req->id.engineId = rec->segment.id.engineId;
	req->id.sessionNbr = rec->segment.id.sessionNbr;
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

#if DGRDEBUG
timeouts[rec->transmissionCount - 1]++;
#endif
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
#if DGRDEBUG
tracePredictedResends = dest->predictedResends;
#endif
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
			if (dest->backlog < _occupancy(i))
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
					+ sizeof(SegmentId));
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
		if (_watching())
		{
			iwatch('{');
		}

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

	if (_watching())
	{
		iwatch('=');
	}

	dest->serviceLoad += rec->contentLength;
	return insertSendReq(sap, rec);
}

static int	arq(DgrSAP *sap, uvast engineId, unsigned int sessionNbr,
			RecordOperation op)
{
	DgrArqBucket	*bucket;
	LystElt		elt;
	DgrRecord	rec;
	DgrDest		*dest;
	int		destIdx;
	int		result;

	bucket = sap->buckets + (sessionNbr & DGR_SESNBR_MASK);
	pthread_mutex_lock(&bucket->mutex);
	pthread_mutex_lock(&sap->destsMutex);
	if (op == DgrSendMessage)
	{
		for (elt = lyst_last(bucket->msgs); elt; elt = lyst_prev(elt))
		{
			rec = (DgrRecord) lyst_data(elt);
			if (rec->segment.id.engineId > engineId)
			{
				continue;
			}

			if (rec->segment.id.engineId < engineId)
			{
				pthread_mutex_unlock(&sap->destsMutex);
				pthread_mutex_unlock(&bucket->mutex);
				return 0;	/*	What happened?	*/
			}

			/*	Found a match on engine ID.		*/

			if (rec->segment.id.sessionNbr > sessionNbr)
			{
				continue;
			}

			if (rec->segment.id.sessionNbr < sessionNbr)
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
		if (rec->segment.id.engineId < engineId)
		{
			continue;
		}

		if (rec->segment.id.engineId > engineId)
		{
			pthread_mutex_unlock(&sap->destsMutex);
			pthread_mutex_unlock(&bucket->mutex);
			return 0;	/*	Record is already gone.	*/
		}

		/*	Found a match on engine ID.			*/

		if (rec->segment.id.sessionNbr < sessionNbr)
		{
			continue;
		}

		if (rec->segment.id.sessionNbr > sessionNbr)
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
	if (op == DgrHandleRpt)
	{
		result = handleRpt(sap, rec, elt, dest, destIdx);
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
	mostActive = -1;
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
	/*	(minRate, the minimum number of bytes to transmit in
	 *	each episode is based on a minimum transmission rate
	 *	of 1200 bps = 150 bytes per second.  Must scale 150
	 *	by length of rate control episode.)			*/

	static int	minRate = (150 * EPISODE_PERIOD) / 1000000;
	int		i;
	int		maxIncrease;
	int		maxDecrease;
	int		unusedCapacity;
	int		capacity;
	int		newDataRate;
	int		booster;

#if DGRDEBUG
traceBytesAcknowledged = dest->bytesAcknowledged;
traceBytesResent = dest->bytesTransmitted - dest->bytesOriginated;
#endif
	if (dest->episodeCount < 8)
	{
		dest->episodeCount++;
	}

	/*	The basic rate control principle: base planned data
	 *	transmission rate on the mean rate of acknowledgement
	 *	over the recent past, inflated by a 50% aggressiveness
	 *	factor to increase the rate rapidly when recovering
	 *	from a period of lost connectivity.  Then compute
	 *	retard as a function of the planned transmission rate.
	 *
	 *	Start by computing upper and lower limits on the
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

#if DGRDEBUG
traceUnusedCapacity = unusedCapacity;
#endif
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

	if (newDataRate < minRate)
	{
		newDataRate = minRate;
	}

	dest->bytesToTransmit = newDataRate;
#if DGRDEBUG
traceBytesToTransmit = newDataRate;
#endif

	/*	To calculate retard (the number of microseconds of
	 *	delay to impose per byte of transmitted content),
	 *	divide the control episode duration in microseconds
	 *	by the number of bytes to transmit in the next
	 *	episode.						*/

	dest->retard = EPISODE_PERIOD / newDataRate;
#if DGRDEBUG
traceRetard = dest->retard;
#endif

	/*	Note that retard may be zero, in the event that the
	 *	network is transmitting way faster than we need it to.	*/

#if DGRDEBUG
dgrtrace();
traceBytesAcknowledged = 0;
dest->serviceLoad = 0;
traceBytesTransmitted = 0;
traceBytesOriginated = 0;
aggregateDelay = 0;
#endif
	dest->bytesAcknowledged = 0;
	dest->bytesTransmitted = 0;
	dest->bytesOriginated = 0;
}

static void	adjustRateControl(DgrSAP *sap)
{
	int	i;
	DgrDest	*dest;

#if DGRDEBUG
PUTS("---------------------------------------------------------------");
#endif

	pthread_mutex_lock(&sap->destsMutex);
	for (i = 0, dest = sap->dests; i < DGR_MAX_DESTS; i++, dest++)
	{
		if (dest->bytesToTransmit > 0)
		{
			adjustRetard(dest);
		}
	}

#if DGRDEBUG
appliedAcks = 0;
#endif
	pthread_mutex_unlock(&sap->destsMutex);
}

/*	*	*	DGR background thread functions		*	*/

static void	*sender(void *parm)
{
	DgrSAP		*sap = (DgrSAP *) parm;
	LystElt		elt;
	SendReq		*req;
	uvast		engineId;
	unsigned int	sessionNbr;
#ifndef mingw
	sigset_t	signals;

	sigfillset(&signals);
	pthread_sigmask(SIG_BLOCK, &signals, NULL);
#endif
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
			engineId = req->id.engineId;
			sessionNbr = req->id.sessionNbr;
			llcv_unlock(sap->outboundCV);
			if (arq(sap, engineId, sessionNbr, DgrSendMessage))
			{
				writeMemo("[i] DGR sender thread ended.");
				return NULL;
			}
		}
	}

	return NULL;
}

static void	*resender(void *parm)
{
	DgrSAP		*sap = (DgrSAP *) parm;
	int		cycleNbr = 1;
	struct timeval	currentTime;
	LystElt		elt;
	ResendReq	*req;
	uvast		engineId;
	unsigned int	sessionNbr;
#ifndef mingw
	sigset_t	signals;

	sigfillset(&signals);
	pthread_sigmask(SIG_BLOCK, &signals, NULL);
#endif
	while (1)
	{
		microsnooze(EPISODE_PERIOD);
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
				/*	Segment's resend time is here.	*/

				engineId = req->id.engineId;
				sessionNbr = req->id.sessionNbr;
				pthread_mutex_unlock(&sap->pendingResendsMutex);
				if (arq(sap, engineId, sessionNbr,
						DgrHandleTimeout))
				{
					writeMemo("[i] DGR resender ended.");
					return NULL;
				}

				continue;
			}

			pthread_mutex_unlock(&sap->pendingResendsMutex);
			break;		/*	No more for now.	*/
		}

		cycleNbr++;
	}
}

static int	sendAck(DgrSAP *sap, char *reportBuffer, int headerLength,
			unsigned int rptSerialNbr, struct sockaddr *sockName,
			socklen_t sockaddrlen)
{
	char	*cursor;
	int	length;
	Sdnv	sdnv;

	memcpy(reportBuffer, sap->inputBuffer, headerLength);
	*reportBuffer = 9;	/*	00001001; report ACK.		*/
	length = headerLength;
	cursor = reportBuffer + length;

	/*	Report serial number.					*/

	encodeSdnv(&sdnv, rptSerialNbr);
	memcpy(cursor, sdnv.text, sdnv.length);
	length += sdnv.length;
	cursor += sdnv.length;

	/*	ACK is now ready to transmit.				*/

	if (isendto(sap->udpSocket, reportBuffer, length, 0, sockName,
			sockaddrlen) < 0)
	{
		if (errno != EBADF)		/*	Socket closed.	*/
		{
			crashThread(sap, "Receiver thread failed sending \
acknowledgement");
		}

		return -1;
	}

	return 0;
}

static int	sendReport(DgrSAP *sap, char *reportBuffer, int headerLength,
			unsigned int ckptSerialNbr, unsigned int dataLength,
			struct sockaddr *sockName, socklen_t sockaddrlen)
{
	char		*cursor;
	int		length;
	Sdnv		sdnv;
	unsigned int	rptSerialNbr = rand();

	memcpy(reportBuffer, sap->inputBuffer, headerLength);
	*reportBuffer = 8;	/*	00001000; report.		*/
	length = headerLength;
	cursor = reportBuffer + length;

	/*	Report serial number.					*/

	encodeSdnv(&sdnv, rptSerialNbr);
	memcpy(cursor, sdnv.text, sdnv.length);
	length += sdnv.length;
	cursor += sdnv.length;

	/*	Checkpoint serial number.				*/

	encodeSdnv(&sdnv, ckptSerialNbr);
	memcpy(cursor, sdnv.text, sdnv.length);
	length += sdnv.length;
	cursor += sdnv.length;

	/*	Upper bound.						*/

	encodeSdnv(&sdnv, dataLength);
	memcpy(cursor, sdnv.text, sdnv.length);
	length += sdnv.length;
	cursor += sdnv.length;

	/*	Lower bound.						*/

	encodeSdnv(&sdnv, 0);
	memcpy(cursor, sdnv.text, sdnv.length);
	length += sdnv.length;
	cursor += sdnv.length;

	/*	Reception claim count.					*/

	encodeSdnv(&sdnv, 1);
	memcpy(cursor, sdnv.text, sdnv.length);
	length += sdnv.length;
	cursor += sdnv.length;

	/*	Reception claim 1 offset.				*/

	encodeSdnv(&sdnv, 0);
	memcpy(cursor, sdnv.text, sdnv.length);
	length += sdnv.length;
	cursor += sdnv.length;

	/*	Reception claim 1 length.				*/

	encodeSdnv(&sdnv, dataLength);
	memcpy(cursor, sdnv.text, sdnv.length);
	length += sdnv.length;
	cursor += sdnv.length;

	/*	Report is now ready to transmit.			*/

	if (isendto(sap->udpSocket, reportBuffer, length, 0, sockName,
			sockaddrlen) < 0)
	{
		if (errno != EBADF)		/*	Socket closed.	*/
		{
			crashThread(sap, "Receiver thread failed sending \
report");
		}

		return -1;
	}

	return 0;
}

static void	*receiver(void *parm)
{
	DgrSAP			*sap = (DgrSAP *) parm;
	struct sockaddr_in	socketAddress;
	struct sockaddr		*sockName = (struct sockaddr *) &socketAddress;
	socklen_t		sockaddrlen;
	unsigned short		portNbr;
	int			length;
	unsigned char		*cursor;
	int			bytesRemaining;
	unsigned int		versionNbr;
	unsigned int		segmentType;
	int			sdnvLength;
	uvast			engineId;
	uvast			sessionNbr;
	unsigned int		extensionCounts;
	int			headerLength;
	uvast			clientSvcId;
	uvast			svcDataOffset;
	uvast			svcDataLength;
	uvast			ckptSerialNbr;
	uvast			rptSerialNbr;
	char			reportBuffer[64];
	int			reclength;
	DgrRecord		rec;
#ifndef mingw
	sigset_t		signals;

	sigfillset(&signals);
	pthread_sigmask(SIG_BLOCK, &signals, NULL);
#endif
	while (1)
	{
		sockaddrlen = sizeof(struct sockaddr_in);
		length = irecvfrom(sap->udpSocket, sap->inputBuffer,
				DGR_BUF_SIZE, 0, sockName, &sockaddrlen);
		if (length < 0)
		{
			switch (errno)
			{
			case EBADF:	/*	Socket has been closed.	*/
				break;

			case ECONNRESET:/*	Connection reset.	*/
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

		/*	Parse the LTP segment header.			*/

		cursor = (unsigned char *) (sap->inputBuffer);
		bytesRemaining = length;

		/*	Version number.					*/

		if (bytesRemaining < 1)
		{
			continue;	/*	Ignore random guck.	*/
		}

		versionNbr = ((*cursor) >> 4) & 0x0f;
		if (versionNbr != 0)
		{
			continue;	/*	Invalid segment.	*/
		}

		/*	Segment type.					*/

		segmentType = (*cursor) & 0x0f;
		cursor++;
		bytesRemaining--;

		/*	Engine ID.					*/

		if (bytesRemaining < 1)
		{
			continue;	/*	Invalid segment.	*/
		}

		sdnvLength = decodeSdnv(&engineId, cursor);
		if (sdnvLength < 1)
		{
			continue;	/*	Invalid segment.	*/
		}

		cursor += sdnvLength;
		bytesRemaining -= sdnvLength;

		/*	Session Nbr.					*/

		if (bytesRemaining < 1)
		{
			continue;	/*	Invalid segment.	*/
		}

		sdnvLength = decodeSdnv(&sessionNbr, cursor);
		if (sdnvLength < 1)
		{
			continue;	/*	Invalid segment.	*/
		}

		cursor += sdnvLength;
		bytesRemaining -= sdnvLength;

		/*	Extension counts.				*/

		if (bytesRemaining < 1)
		{
			continue;	/*	Invalid segment.	*/
		}

		extensionCounts = *cursor;
		if (extensionCounts != 0)
		{
			continue;	/*	No extension support.	*/
		}

		cursor++;
		bytesRemaining--;

		/*	Segment content.				*/

		if (bytesRemaining < 1)	/*	No content.		*/
		{
			continue;	/*	Invalid segment.	*/
		}

		/*	Process content as indicated by segment type.	*/

		headerLength = length - bytesRemaining;
		if (segmentType == 8)	/*	Report.			*/
		{
			/*	Get report serial number.		*/

			sdnvLength = decodeSdnv(&rptSerialNbr, cursor);
			if (sdnvLength < 1)
			{
				continue;	/*	Invalid segment.*/
			}

			cursor += sdnvLength;
			bytesRemaining -= sdnvLength;
			if (sendAck(sap, reportBuffer, headerLength,
				rptSerialNbr, sockName, sockaddrlen) < 0)
			{
				break;		/*	Main loop.	*/
			}

			if (arq(sap, engineId, sessionNbr, DgrHandleRpt))
			{
				writeMemo("[i] DGR receiver thread ended.");
				break;	/*	Out of main loop.	*/
			}

			continue;
		}

		/*	Note: we always return report ACKs (9s), for
		 *	compliance, but we always ignore all received
		 *	report ACKs.  DGR reports are not retransmitted.
		 *	If the report isn't received, the data segment
		 *	is eventually retransmitted and is acknowledged
		 *	at that time.					*/

		if (segmentType != 3)
		{
			continue;	/*	Not supported.		*/
		}

		/*	Red data, EOB.  Extract sender's port nbr.	*/

		portNbr = ntohs(socketAddress.sin_port);

		/*	Client service ID.				*/

		sdnvLength = decodeSdnv(&clientSvcId, cursor);
		if (sdnvLength < 1)
		{
			continue;	/*	Invalid segment.	*/
		}

		cursor += sdnvLength;
		bytesRemaining -= sdnvLength;

		/*	Service data offset.				*/

		if (bytesRemaining < 1)
		{
			continue;	/*	Invalid segment.	*/
		}

		sdnvLength = decodeSdnv(&svcDataOffset, cursor);
		if (sdnvLength < 1)
		{
			continue;	/*	Invalid segment.	*/
		}

		if (svcDataOffset != 0)
		{
			continue;	/*	Not supported.		*/
		}

		cursor += sdnvLength;
		bytesRemaining -= sdnvLength;

		/*	Service data length.				*/

		if (bytesRemaining < 1)
		{
			continue;	/*	Invalid segment.	*/
		}

		sdnvLength = decodeSdnv(&svcDataLength, cursor);
		if (sdnvLength < 1)
		{
			continue;	/*	Invalid segment.	*/
		}

		cursor += sdnvLength;
		bytesRemaining -= sdnvLength;

		/*	Checkpoint serial number.			*/

		if (bytesRemaining < 1)
		{
			continue;	/*	Invalid segment.	*/
		}
		sdnvLength = decodeSdnv(&ckptSerialNbr, cursor);
		if (sdnvLength < 1)
		{
			continue;	/*	Invalid segment.	*/
		}

		cursor += sdnvLength;
		bytesRemaining -= sdnvLength;

		/*	Report serial number.				*/

		if (bytesRemaining < 1)
		{
			continue;	/*	Invalid segment.	*/
		}

		sdnvLength = decodeSdnv(&rptSerialNbr, cursor);
		if (sdnvLength < 1)
		{
			continue;	/*	Invalid segment.	*/
		}

		cursor += sdnvLength;
		bytesRemaining -= sdnvLength;

		/*	Client service data.				*/

		if (bytesRemaining < 1)
		{
			continue;	/*	Invalid segment.	*/
		}

		if (rptSerialNbr != 0)
		{
			continue;	/*	Not supported.		*/
		}

		if (_watching())
		{
			iwatch('s');
		}

		/*	Now send acknowledgment (report).		*/

		if (sendReport(sap, reportBuffer, headerLength, ckptSerialNbr,
				svcDataLength, sockName, sockaddrlen) < 0)
		{
			break;		/*	Out of main loop.	*/
		}

		/*	Create content arrival event.			*/

		reclength = sizeof(struct dgr_rec) + (svcDataLength - 1);
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
		rec->contentLength = svcDataLength;
		rec->segment.id.engineId = engineId;
		rec->segment.id.sessionNbr = sessionNbr;
		memcpy(rec->segment.content, cursor, svcDataLength);
		if (insertEvent(sap, rec))
		{
			writeMemo("[i] DGR receiver thread ended.");
			break;		/*	Out of main loop.	*/
		}

		if (_watching())
		{
			iwatch('t');
		}
	}

	/*	Main loop terminated for some reason; wrap up.		*/

	return NULL;
}

/*	*	*	DGR API functions	*	*	*	*/

static void	forgetObject(LystElt elt, void *userData)
{
	DgrSAP	*sap = (DgrSAP *) userData;

	MRELEASE(lyst_data(elt));
}

static void	cleanUpSAP(DgrSAP *sap)
{
	int		i;
	DgrArqBucket	*bucket;

	if (sap->udpSocket >= 0)
	{
		closesocket(sap->udpSocket);
	}

	if (sap->outboundMsgs)
	{
		lyst_delete_set(sap->outboundMsgs, forgetObject, sap);
		lyst_destroy(sap->outboundMsgs);
	}

	if (sap->outboundCV)
	{
		llcv_close(sap->outboundCV);
	}

	if (sap->inboundEvents)
	{
		lyst_delete_set(sap->inboundEvents, forgetObject, sap);
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
			lyst_delete_set(bucket->msgs, forgetObject, sap);
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
		lyst_delete_set(sap->pendingResends, forgetObject, sap);
		lyst_destroy(sap->pendingResends);
	}

	pthread_mutex_lock(&sap->sapMutex);
	pthread_cond_destroy(&sap->sapCV);
	pthread_mutex_destroy(&sap->sapMutex);
	pthread_mutex_destroy(&sap->pendingResendsMutex);
	pthread_mutex_destroy(&sap->destsMutex);
	MRELEASE(sap);
}

int	dgr_open(uvast ownEngineId, unsigned int clientSvcId,
		unsigned short ownPortNbr, unsigned int ownIpAddress,
		char *memmgrName, DgrSAP **sapp, DgrRC *rc)
{
	struct sockaddr_in	socketAddress;
	struct sockaddr		*sockName = (struct sockaddr *) &socketAddress;
	int			mmid;
	DgrSAP			*sap;
	int			i;
	DgrArqBucket		*bucket;

	CHKERR(ownEngineId);
	CHKERR(clientSvcId);
	CHKERR(sapp);
	CHKERR(rc);

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
			putErrmsg("DGR can't find memory manager.", memmgrName);
			return -1;
		}
	}
	else
	{
		mmid = 0;	/*	default is malloc/free		*/
	}

	/*	Initialize service access point constants.		*/

	sap = (DgrSAP *)(memmgr_take(mmid))(__FILE__, __LINE__, sizeof(DgrSAP));
	if (sap == NULL)
	{
		putErrmsg("DGR can't create DGR service access point.", NULL);
		return -1;
	}

	memset((char *) sap, 0, sizeof(DgrSAP));
	sap->engineId = ownEngineId;
	sap->clientSvcId = clientSvcId;
	sap->state = DgrSapOpen;
	sap->mtake = memmgr_take(mmid);
	sap->mrelease = memmgr_release(mmid);
	sap->leastActiveDest = -1;
	sap->mostActiveDest = DGR_MAX_DESTS;
	initializeDest(&sap->defaultDest, 0, 0);

	/*	Initialize UDP socket for DGR service.			*/

	sap->udpSocket = socket(AF_INET, SOCK_DGRAM|SOCK_CLOEXEC, IPPROTO_UDP);
	if (sap->udpSocket < 0)
	{
		putSysErrmsg("Can't open DGR UDP socket", NULL);
		cleanUpSAP(sap);
		return -1;
	}

#if (SOCK_CLOEXEC == 0)
	closeOnExec(sap->udpSocket);
#endif
	if (reUseAddress(sap->udpSocket) < 0)
	{
		putSysErrmsg("Can't reuse address on DGR UDP socket",
				utoa(ntohs(ownPortNbr)));
		cleanUpSAP(sap);
		return -1;
	}

	if (bind(sap->udpSocket, sockName, sizeof(struct sockaddr)) < 0)
	{
		putSysErrmsg("Can't bind DGR UDP socket",
				utoa(ntohs(ownPortNbr)));
		cleanUpSAP(sap);
		return -1;
	}

	/*	Create lists and management structures.			*/

	if ((sap->outboundMsgs = lyst_create_using(mmid)) == NULL)
	{
		putErrmsg("DGR can't create list of outbound messages.", NULL);
		cleanUpSAP(sap);
		return -1;
	}

	if ((sap->outboundCV = llcv_open(sap->outboundMsgs,
			&(sap->outboundCV_str))) == NULL)
	{
		putErrmsg("DGR can't outbound messages CV.", NULL);
		cleanUpSAP(sap);
		return -1;
	}

	if ((sap->inboundEvents = lyst_create_using(mmid)) == NULL)
	{
		putErrmsg("DGR can't create list of inbound events.", NULL);
		cleanUpSAP(sap);
		return -1;
	}

	if ((sap->inboundCV = llcv_open(sap->inboundEvents,
			&(sap->inboundCV_str))) == NULL)
	{
		putErrmsg("DGR can't create inbound events CV.", NULL);
		cleanUpSAP(sap);
		return -1;
	}

	for (i = 0; i < DGR_BIN_COUNT; i++)
	{
		if ((sap->destLysts[i] = lyst_create_using(mmid)) == NULL)
		{
			putErrmsg("DGR can't create destinations list.", NULL);
			cleanUpSAP(sap);
			return -1;
		}
	}

	for (i = 0, bucket = sap->buckets; i < DGR_BUCKETS; i++, bucket++)
	{
		if ((bucket->msgs = lyst_create_using(mmid)) == NULL
		|| pthread_mutex_init(&bucket->mutex, NULL))
		{
			putSysErrmsg("DGR can't create message bucket", NULL);
			cleanUpSAP(sap);
			return -1;
		}
	}

	if ((sap->pendingResends = lyst_create_using(mmid)) == NULL)
	{
		putErrmsg("DGR can't create list of resend requests.", NULL);
		cleanUpSAP(sap);
		return -1;
	}

	if (pthread_mutex_init(&sap->sapMutex, NULL)
	|| pthread_cond_init(&sap->sapCV, NULL)
	|| pthread_mutex_init(&sap->pendingResendsMutex, NULL)
	|| pthread_mutex_init(&sap->destsMutex, NULL))
	{
		putSysErrmsg("DGR can't initialize mutex(es)", NULL);
		cleanUpSAP(sap);
		return -1;
	}

	/*	Spawn all threads.					*/

	if (pthread_begin(&(sap->sender), NULL, sender, sap, "libdgr_sender")
	|| pthread_begin(&(sap->resender), NULL, resender, sap, "libdgr_resender")
	|| pthread_begin(&(sap->receiver), NULL, receiver, sap, "libdgr_receiver"))
	{
		putSysErrmsg("DGR can't spawn thread(s)", NULL);
		cleanUpSAP(sap);
		return -1;
	}

	*sapp = sap;
	*rc = DgrOpened;
	return 0;
}

void	dgr_getsockname(DgrSAP *sap, unsigned short *portNbr,
		unsigned int *ipAddress)
{
	struct sockaddr		buf;
	socklen_t		buflen = sizeof buf;
	struct sockaddr_in	*nm = (struct sockaddr_in *) &buf;

	CHKVOID(sap);
	CHKVOID(portNbr);
	CHKVOID(ipAddress);
	*portNbr = *ipAddress = 0;	/*	Default.		*/
	if (getsockname(sap->udpSocket, &buf, &buflen) < 0)
	{
		putSysErrmsg("DGR can't get socket name.", NULL);
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

	CHKVOID(sap);
	if (sap->state == DgrSapClosed)
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

	if (getsockname(sap->udpSocket, &sockName, &sockNameLen) < 0)
	{
		putSysErrmsg("DGR can't get socket name.", NULL);
	}
	else
	{
		if (isendto(sap->udpSocket, &shutdown, 1, 0, &sockName,
				sizeof(struct sockaddr_in)) < 0)
		{
			putSysErrmsg("DGR can't send shutdown packet", NULL);
		}
	}

	pthread_join(sap->receiver, NULL);

	/*	Now destroy all remaining management and communication
	 *	structures.						*/

	cleanUpSAP(sap);
}

int	dgr_send(DgrSAP *sap, unsigned short toPortNbr,
		unsigned int toIpAddress, int notificationFlags, char *content,
		int length, DgrRC *rc)
{
	int		reclength;
	DgrRecord	rec;
	DgrDest		*dest;
	int		destIdx;
	int		delay;			/*	microseconds	*/
	static int	clockResolution = SYS_CLOCK_RES;
	int		usecToSnooze;
	int		usecSnoozed;
	LystElt		elt;

	CHKERR(sap);
	CHKERR(toPortNbr);
	CHKERR(toIpAddress);
	CHKERR(notificationFlags >= 0);
	CHKERR(notificationFlags <= 3);
	CHKERR(content);
	CHKERR(length > 0);
	CHKERR(length <= MAX_DATA_SIZE);
	CHKERR(rc);
	if (sap->state == DgrSapDamaged)
	{
		writeMemo("[?] DGR access point damaged; close and reopen.");
		*rc = DgrFailed;
		return 0;
	}

	if (sap->state == DgrSapClosed)
	{
		writeMemo("[?] DGR access point is not open.");
		*rc = DgrFailed;
		return 0;
	}

	/*	Construct the outbound DGR record.			*/

	reclength = sizeof(struct dgr_rec) + (length - 1);
	rec = (DgrRecord) MTAKE(reclength);
	if (rec == NULL)
	{
		putErrmsg("Can't create outbound message record.", NULL);
		return -1;
	}

	memset((char *) rec, 0, reclength);
	rec->type = DgrMsgOut;
	rec->portNbr = toPortNbr;
	rec->ipAddress = toIpAddress;
	rec->notificationFlags = notificationFlags;
	rec->contentLength = length;
	memcpy(rec->segment.content, content, length);

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
#if DGRDEBUG
aggregateDelay += delay;
#endif

	/*	Now apply the computed rate control delay.		*/

	dest->pendingDelay += delay;
	usecToSnooze = dest->pendingDelay;
	pthread_mutex_unlock(&sap->destsMutex);
	usecSnoozed = 0;
	while (usecToSnooze > clockResolution)
	{
#if DGRDEBUG
rcSnoozes++;
#endif
		microsnooze(clockResolution);
		usecToSnooze -= clockResolution;
		usecSnoozed += clockResolution;
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
	while (sap->backlog > MAX_BACKLOG)
	{
		pthread_cond_wait(&sap->sapCV, &sap->sapMutex);
	}

	/*	Now proceed with transmission.				*/

	sap->backlog += (rec->contentLength + sizeof(SegmentId));
	sap->sessionNbr++;
	rec->segment.id.engineId = sap->engineId;
	rec->segment.id.sessionNbr = sap->sessionNbr;
	pthread_mutex_unlock(&sap->sapMutex);

	/*	Store in a bucket of the DGR ARQ (record) database.
	 *	Select bucket by computing sessionNbr modulo the
	 *	number of buckets - 1.					*/

	rec->bucket = sap->buckets +
			(rec->segment.id.sessionNbr & DGR_SESNBR_MASK);

	/*	Insert the new record in the selected database bucket.	*/

	pthread_mutex_lock(&rec->bucket->mutex);
	elt = lyst_insert_last(rec->bucket->msgs, rec);
	pthread_mutex_unlock(&rec->bucket->mutex);
	if (elt == NULL)				/*	Bail.	*/
	{
		putErrmsg("Can't append outbound record.", NULL);
		MRELEASE(rec);
		pthread_mutex_lock(&sap->sapMutex);
		sap->backlog -= (length + sizeof(SegmentId));
		pthread_cond_signal(&sap->sapCV);
		pthread_mutex_unlock(&sap->sapMutex);
		pthread_mutex_lock(&sap->destsMutex);
		dest = findDest(sap, toPortNbr, toIpAddress, &destIdx);
		dest->serviceLoad -= length;
		pthread_mutex_unlock(&sap->destsMutex);
		return -1;
	}

	/*	Insert transmission request for the sender thread to
	 *	act on.							*/

	if (insertSendReq(sap, rec) < 0)
	{
		putErrmsg("Can't append transmission request.", NULL);
		lyst_delete(elt);
		MRELEASE(rec);
		pthread_mutex_lock(&sap->sapMutex);
		sap->backlog -= (length + sizeof(SegmentId));
		pthread_cond_signal(&sap->sapCV);
		pthread_mutex_unlock(&sap->sapMutex);
		pthread_mutex_lock(&sap->destsMutex);
		dest = findDest(sap, toPortNbr, toIpAddress, &destIdx);
		dest->serviceLoad -= length;
		pthread_mutex_unlock(&sap->destsMutex);
		return -1;
	}

	if (_watching())
	{
		iwatch('e');
		iwatch('f');
	}

	*rc = DgrDatagramSent;
	return 0;
}

int	dgr_receive(DgrSAP *sap, unsigned short *fromPortNbr,
		unsigned int *fromIpAddress, char *content, int *length,
		int *errnbr, int timeoutSeconds, DgrRC *rc)
{
	int		timeoutUsec;
	LystElt		elt;
	DgrRecord	rec;

	CHKERR(sap);
	CHKERR(fromPortNbr);
	CHKERR(fromIpAddress);
	CHKERR(content);
	CHKERR(length);
	CHKERR(errnbr);
	CHKERR(rc);
	if (sap->state == DgrSapDamaged)
	{
		writeMemo("[?] DGR access point damaged; close and reopen.");
		*rc = DgrFailed;
		return 0;
	}

	if (sap->state == DgrSapClosed)
	{
		writeMemo("[?] DGR access point is not open.");
		*rc = DgrFailed;
		return 0;
	}

	if (timeoutSeconds == DGR_BLOCKING)
	{
		timeoutUsec = LLCV_BLOCKING;
	}
	else
	{
		timeoutUsec = timeoutSeconds * 1000000;
	}

	while (1)
	{
		if (llcv_wait(sap->inboundCV, llcv_lyst_not_empty, timeoutUsec))
		{
			if (errno == ETIMEDOUT)
			{
				*rc = DgrTimedOut;
				return 0;
			}

			putErrmsg("DGR reception failed in wait.", NULL);
			return -1;
		}

		/*	Lyst is no longer empty, or SAP is now closed.	*/

		break;				/*	Out of loop.	*/
	}

	/*	SAP might have been closed while we were sleeping.	*/

	if (sap->state == DgrSapDamaged)
	{
		writeMemo("[?] DGR access point no longer usable.");
		*rc = DgrFailed;
		return 0;
	}

	if (sap->state == DgrSapClosed)
	{
		writeMemo("[i] DGR access point has been closed.");
		*rc = DgrFailed;
		return 0;
	}

	llcv_lock(sap->inboundCV);
	elt = lyst_first(sap->inboundEvents);
	rec = (DgrRecord) lyst_data(elt);
	lyst_delete(elt);
	llcv_unlock(sap->inboundCV);
	if (rec == NULL)		/*	Interrupted.		*/
	{
		*rc =  DgrInterrupted;
		return 0;
	}

	*length = rec->contentLength;
	memcpy(content, rec->segment.content, rec->contentLength);
	switch (rec->type)
	{
	case DgrMsgIn:
		*fromPortNbr = rec->portNbr;
		*fromIpAddress = rec->ipAddress;
		*rc = DgrDatagramReceived;
		break;

	case DgrDeliverySuccess:
		*rc = DgrDatagramAcknowledged;
		break;

	default:
		*errnbr = rec->errnbr;
		*rc = DgrDatagramNotAcknowledged;
	}

	/*	Reclaim memory occupied by the inbound message
	 *	or delivery failure.					*/

	MRELEASE(rec);
	return 0;
}

void	dgr_interrupt(DgrSAP *sap)
{
	CHKVOID(sap);
	CHKVOID(insertEvent(sap, NULL) == 0);
}
