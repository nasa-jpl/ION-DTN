/*
	tcpcli.c:	ION TCP convergence-layer adapter daemon.
			Handles both transmission and reception.

	Author: Scott Burleigh, JPL

	Copyright (c) 2015, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
	
									*/
#include "bpP.h"
#include "llcv.h"

#define	TCPCL_BUFSZ		(64 * 1024)

#ifndef MAX_RESCAN_INTERVAL
#define MAX_RESCAN_INTERVAL	(20)
#endif

#ifndef KEEPALIVE_INTERVAL
#define KEEPALIVE_INTERVAL	(15)
#endif

#ifndef IDLE_SHUTDOWN_INTERVAL
#define IDLE_SHUTDOWN_INTERVAL	(-1)
#endif

#ifndef MAX_RECONNECT_INTERVAL
#define MAX_RECONNECT_INTERVAL	(3600)
#endif

#ifndef	MAX_PIPELINE_LENGTH
#define	MAX_PIPELINE_LENGTH	(100)
#endif

#ifndef TCPCL_SEGMENT_ACKS
#define	TCPCL_SEGMENT_ACKS	(1)
#endif
#define	TCPCL_REACTIVE		(0)
#define	TCPCL_REFUSALS		(0)
#define	TCPCL_LENGTH_MSGS	(0)

#define	DUCT_BUFLEN		(MAX_CL_DUCT_NAME_LEN + 1)

#define	TCPCL_PLANNED		(0)
#define	TCPCL_CHANCE		(1)

#ifdef	QUERY
#undef	QUERY
#endif
#define	QUERY			(-1)

/*	Each TCPCL-speaking node must be prepared to accept TCPCL
 *	sessions initiated by other nodes (automatically creating
 *	the implied outducts and possibly also the egress plans
 *	that cite them) and must also initiate its own TCPCL
 *	sessions for outducts cited by managed egress plans.
 *
 *	Note that two distinct sessions - one initiated locally,
 *	the other initiated by the peer node - may concurrently
 *	exist between a given pair of nodes.  Since sessions
 *	are not nodes, in order to comply with RFC 7242 we must
 *	apply rules on reconnection and shutdown on a node basis
 *	rather than a session basis.
 *
 *	Accordingly, the ION TCPCL adapter manages a list of
 *	TCPCL Neighbors (nodes), each of which (a) corresponds
 *	to a single egress plan and (b) may conduct up to two
 *	distinct TCPCL sessions, one Planned (locally initiated)
 *	and one Chance (accepted).  Each Session has a single
 *	socket (TCP connection), has a single reception thread
 *	that executes most of the TCPCL protocol, and has a
 *	transmission thread that dequeues bundles from one of
 *	the outducts assigned to the egress plan corresponding
 *	to the Neighbor and transmits data segments.
 *
 *	When TCPCL data segments are acknowledged, the
 *	acknowledgments are applied to transmitted bundles
 *	in FIFO fashion; bundles awaiting acknowledgment are
 *	retained in a pipeline list, from which they can be
 *	relocated into the Limbo list in the event that link
 *	disruption is detected.  Insertion into the pipeline
 *	list is throttled, an additional flow control measure.		*/

typedef struct
{
	int			sock;
	struct tcpcl_neighbor	*neighbor;	/*	Back reference	*/
	pthread_mutex_t		socketMutex;	/*	For socket.	*/
	int			hasSocketMutex;	/*	Boolean.	*/

	/*	Configuration settings and session state.		*/

	int			newlyAdded;	/*	Boolean.	*/
	int			isOpen;		/*	Boolean.	*/
	int			secUntilShutdown;
	int			keepaliveInterval;
	int			secUntilKeepalive;
	int			reconnectInterval;
	int			secUntilReconnect;
	int			secSinceReception;
	int			timeoutCount;
	int			segmentAcks;	/*	Boolean.	*/
	int			reactiveFrags;	/*	Boolean.	*/
	int			bundleRefusals;	/*	Boolean.	*/
	int			lengthMessages;	/*	Boolean.	*/

	/*	Reception function.					*/

	pthread_t		receiver;
	int			hasReceiver;	/*	Boolean.	*/
	vast			lengthReceived;	/*	Current in.	*/

	/*	Administration function.				*/

	pthread_t		admin;
	int			hasAdmin;	/*	Boolean.	*/
	Lyst			signals;
	pthread_mutex_t		sigMutex;	/*	For signals.	*/
	int			hasSigMutex;	/*	Boolean.	*/
	struct llcv_str		triggerLlcv;
	Llcv			trigger;	/*	On signals.	*/

	/*	Transmission function.					*/

	pthread_t		sender;
	int			hasSender;	/*	Boolean.	*/
	char			*outductName;
	VOutduct		*vduct;
	Lyst			pipeline;	/*	All outbound.	*/
	pthread_mutex_t		plMutex;	/*	For pipeline.	*/
	int			hasPlMutex;	/*	Boolean.	*/
	struct llcv_str		throttleLlcv;
	Llcv			throttle;	/*	On pipeline.	*/
	uvast			lengthSent;	/*	Oldest out.	*/
	uvast			lengthAcked;	/*	Oldest out.	*/
} TcpclSession;

typedef struct tcpcl_neighbor
{
	VPlan			*vplan;		/*	Remote node.	*/
	VInduct			*induct;	/*	(Common.)	*/
	int			mustDelete;	/*	Boolean.	*/
	TcpclSession		sessions[2];
	size_t			receptionRate;	/*	Bytes/sec.	*/
} TcpclNeighbor;

static void	*handleContacts(void *parm);

static char	*procName()
{
	return "tcpcli";
}

#ifndef mingw
static void	handleStopThread(int signum)
{
	isignal(SIGINT, handleStopThread);
}
#endif
static void	handleStopTcpcli(int signum)
{
	isignal(SIGTERM, handleStopTcpcli);
	ionKillMainThread(procName());
}

/*	*	*	Utility functions	*	*	*	*/

static int	receiveSdnv(TcpclSession *p, uvast *val)
{
	int		sdnvLength = 0;
	unsigned char	byte;

	*val = 0;
	while (1)
	{
		sdnvLength++;
		if (sdnvLength > 10)
		{
			/*	More than 70 bits, which is invalid.
			 *	There's a serious problem at the
			 *	sender, so we treat this like a loss
			 *	of session.				*/

			return 0;
		}

		/*	Shift numeric value 7 bits to the left (that
		 *	is, multiply by 128) to make room for 7 bits
		 *	of SDNV byte value.				*/

		*val <<= 7;

		/*	Receive next byte of SDNV.			*/

		switch (irecv(p->sock, (char *) &byte, 1, 0))
		{
		case -1:
			if (errno != EINTR)	/*	(Shutdown)	*/
			{
				putSysErrmsg("irecv() error on TCP socket",
						p->outductName);
			}

			/*	Intentional fall-through to next case.	*/

		case 0:			/*	Neighbor closed.	*/
			return 0;
		}

		/*	Insert SDNV byte value (with its high-order
		 *	bit masked off) as low-order 7 bits of the
		 *	numeric value.					*/

		*val |= (byte & 0x7f);
		if ((byte & 0x80) == 0)	/*	Last byte of SDNV.	*/
		{
			break;		/*	Out of loop.		*/
		}
	}

	return sdnvLength;		/*	Succeeded.		*/
}

/*	*	Session and neighbor management functions	*	*/

typedef struct
{
	Lyst		neighbors;
	TcpclSession	*session;
	char		*buffer;
	AcqWorkArea	*work;
	ReqAttendant	attendant;
} ReceiverThreadParms;

typedef struct
{
	TcpclSession	*session;
	char		*buffer;
	Outflow		outflows[3];
} SenderThreadParms;

static LystElt	findNeighborForEid(Lyst neighbors, char *eid)
{
	LystElt		elt;
	TcpclNeighbor	*neighbor;

	for (elt = lyst_first(neighbors); elt; elt = lyst_next(elt))
	{
		neighbor = (TcpclNeighbor *) lyst_data(elt);
		if (neighbor->vplan == NULL)
		{
			continue;
		}

		if (strcmp(neighbor->vplan->neighborEid, eid) == 0)
		{
			return elt;
		}
	}

	return NULL;
}

static LystElt	addTcpclNeighbor(VPlan *vplan, VInduct *induct, Lyst neighbors)
{
	char		*eid;
	TcpclNeighbor	*neighbor;
	TcpclSession	*session;
	LystElt		elt;
	int		i;

	if (vplan)
	{
		eid = vplan->neighborEid;
	}
	else
	{
		eid = "<unknown node>";
	}

	neighbor = (TcpclNeighbor *) MTAKE(sizeof(TcpclNeighbor));
	if (neighbor == NULL)
	{
		putErrmsg("tcpcli can't allocate new neighbor.", eid);
		return NULL;
	}

	neighbor->vplan = vplan;
	neighbor->induct = induct;
	for (i = 0; i < 2; i++)
	{
		session = &(neighbor->sessions[i]);
		memset(session, 0, sizeof(TcpclSession));
		session->sock = -1;
		session->neighbor = neighbor;
		session->secUntilShutdown = -1;
		session->secUntilKeepalive = -1;
	}

	neighbor->receptionRate = 0;
	elt = lyst_insert_last(neighbors, (void *) neighbor);
	if (elt == NULL)
	{
		MRELEASE(neighbor);
		putErrmsg("tcpcli can't insert new neighbor into list.", eid);
		return NULL;
	}

	return elt;
}

static void	cancelXmit(LystElt elt, void *userdata)
{
	Object	bundleZco = (Object) lyst_data(elt);

	if (bundleZco == 0)
	{
		return;
	}

	if (bpHandleXmitFailure(bundleZco) < 0)
	{
		putErrmsg("tcpcli neighbor closure can't handle failed xmit.",
				NULL);
		ionKillMainThread(procName());
	}
}

static int	beginSession(LystElt neighborElt, int newSocket, int sessionIdx)
{
	TcpclNeighbor		*neighbor = NULL;
	TcpclSession		*session;
	ReceiverThreadParms	*rtp;

	neighbor = (TcpclNeighbor *) lyst_data(neighborElt);
	session = &(neighbor->sessions[sessionIdx]);
	session->sock = newSocket;
	session->keepaliveInterval = 0;
	session->secUntilKeepalive = -1;
	session->secUntilShutdown = -1;
	if (sessionIdx == TCPCL_PLANNED)
	{
		session->reconnectInterval = 1;
	}

	session->secUntilReconnect = -1;
	session->newlyAdded = 1;

	/*	Transmission pipeline.					*/

	session->pipeline = lyst_create_using(getIonMemoryMgr());
	if (session->pipeline == NULL)
	{
		putErrmsg("tcpcli can't create pipeline list.", NULL);
		closesocket(newSocket);
		session->sock = -1;
		return -1;
	}

	lyst_delete_set(session->pipeline, cancelXmit, NULL);
	session->throttle = llcv_open(session->pipeline,
			&(session->throttleLlcv));
	if (session->throttle == NULL)
	{
		lyst_destroy(session->pipeline);
		session->pipeline = NULL;
		putErrmsg("tcpcli can't open pipeline.", NULL);
		closesocket(newSocket);
		session->sock = -1;
		return -1;
	}

	/*	Administration signals queue.				*/

	session->signals = lyst_create_using(getIonMemoryMgr());
	if (session->signals == NULL)
	{
		llcv_close(session->throttle);
		session->throttle = NULL;
		lyst_destroy(session->pipeline);
		session->pipeline = NULL;
		putErrmsg("tcpcli can't create signals list.", NULL);
		ionKillMainThread(procName());
		return -1;
	}

	session->trigger = llcv_open(session->signals, &(session->triggerLlcv));
	if (session->trigger == NULL)
	{
		lyst_destroy(session->signals);
		session->signals = NULL;
		llcv_close(session->throttle);
		session->throttle = NULL;
		lyst_destroy(session->pipeline);
		session->pipeline = NULL;
		putErrmsg("tcpcli can't open signals list.", NULL);
		ionKillMainThread(procName());
		return -1;
	}

	/*	Receiver thread.					*/

	rtp = (ReceiverThreadParms *) MTAKE(sizeof(ReceiverThreadParms));
	if (rtp == NULL)
	{
		llcv_close(session->trigger);
		session->trigger = NULL;
		lyst_destroy(session->signals);
		session->signals = NULL;
		llcv_close(session->throttle);
		session->throttle = NULL;
		lyst_destroy(session->pipeline);
		session->pipeline = NULL;
		putErrmsg("tcpcli can't allocate new receiver parms.", NULL);
		closesocket(newSocket);
		session->sock = -1;
		return -1;
	}

	rtp->neighbors = lyst_lyst(neighborElt);
	rtp->session = session;
	pthread_mutex_init(&(session->socketMutex), NULL);
	session->hasSocketMutex = 1;
	pthread_mutex_init(&(session->plMutex), NULL);
	session->hasPlMutex = 1;
	pthread_mutex_init(&(session->sigMutex), NULL);
	session->hasSigMutex = 1;
	if (pthread_begin(&(session->receiver), NULL, handleContacts, rtp, "tcpcli-receiver"))
	{
		MRELEASE(rtp);
		pthread_mutex_unlock(&(session->sigMutex));
		pthread_mutex_destroy(&(session->sigMutex));
		session->hasSigMutex = 0;
		pthread_mutex_unlock(&(session->plMutex));
		pthread_mutex_destroy(&(session->plMutex));
		session->hasPlMutex = 0;
		pthread_mutex_unlock(&(session->socketMutex));
		pthread_mutex_destroy(&(session->socketMutex));
		session->hasSocketMutex = 0;
		llcv_close(session->trigger);
		session->trigger = NULL;
		lyst_destroy(session->signals);
		session->signals = NULL;
		llcv_close(session->throttle);
		session->throttle = NULL;
		lyst_destroy(session->pipeline);
		session->pipeline = NULL;
		putSysErrmsg("tcpcli can't create new receiver thread", NULL);
		closesocket(newSocket);
		session->sock = -1;
		return -1;
	}

	return 0;
}

static int	reopenSession(TcpclSession *session)
{
	TcpclNeighbor	*neighbor = session->neighbor;

	if (session->secUntilReconnect != 0)
	{
		return 0;	/*	Must not reconnect yet.		*/
	}

	/*	Okay to make next session attempt.			*/

	switch (itcp_connect(session->outductName, BpTcpDefaultPortNbr,
			&(session->sock)))
	{
	case -1:		/*	System failure.			*/
		putErrmsg("tcpcli failed on TCP reconnect.",
				neighbor->vplan->neighborEid);
		return -1;

	case 0:			/*	Neighbor still refuses.		*/
		writeMemoNote("[i] tcpcli unable to reconnect",
				neighbor->vplan->neighborEid);
		session->reconnectInterval <<= 1;
		if (session->reconnectInterval > MAX_RECONNECT_INTERVAL)
		{
			session->reconnectInterval = MAX_RECONNECT_INTERVAL;
		}

		session->secUntilReconnect = session->reconnectInterval;
		return 0;
	}

	/*	Reconnection succeeded.					*/

	if (watchSocket(session->sock) < 0)
	{
		closesocket(session->sock);
		putErrmsg("tcpcli can't watch socket.", session->outductName);
		return -1;
	}

	session->lengthSent = 0;
	session->lengthAcked = 0;
	return 1;		/*	Reconnected.			*/
}

static int pipeline_not_full(Llcv llcv)
{
	CHKZERO(llcv);
	return (lyst_length(llcv->list) < MAX_PIPELINE_LENGTH ? 1 : 0);
}

static int	sendSignal(TcpclSession *session, saddr lengthReceived)
{
	LystElt	result;

	pthread_mutex_lock(&session->sigMutex);
	result = lyst_insert_last(session->signals, (void *) lengthReceived);
	pthread_mutex_unlock(&session->sigMutex);
       	if (result== NULL)
	{
		putErrmsg("tcpcli can't enqueue admin signal", NULL);
		return -1;
	}

	llcv_signal(session->trigger, llcv_lyst_not_empty);
	return 0;
}

static void	stopAdminThread(TcpclSession *session)
{
	/*	Signal thread in case it's not already stopping.	*/

	if (session->sock != -1)
	{
		shutdown(session->sock, SD_BOTH);
		closesocket(session->sock);
		session->sock = -1;
	}

	oK(sendSignal(session, -1));		/*	Shutdown.	*/
	pthread_join(session->admin, NULL);
	session->hasAdmin = 0;
}

static void	stopSenderThread(TcpclSession *session)
{
	/*	Signal thread in case it's not already stopping.	*/

	if (session->sock != -1)
	{
		shutdown(session->sock, SD_BOTH);
		closesocket(session->sock);
		session->sock = -1;
	}

	/*	Enable sendBundleByTcpcl to exit.			*/

	if (session->throttle)
	{
		llcv_signal(session->throttle, pipeline_not_full);
	}

	/*	Enable sendOneBundle to exit, detach from Outduct.	*/

	if (session->vduct)
	{
		/*	Here we need to simulate the procedures that
		 *	libbpP.c performs when stopping an outduct.
		 *	First, stopOutduct.				*/

		if (session->vduct->semaphore != SM_SEM_NONE)
		{
			sm_SemEnd(session->vduct->semaphore);
		}

		/*	Then waitForOutduct.				*/

		microsnooze(100000);	/*	Maybe thread stops.	*/
		if (session->vduct->hasThread)
		{
			/*	Note: session->vduct->cloThread
			 *	is the same as session->sender.		*/

			if (pthread_kill(session->vduct->cloThread, SIGCONT)
					== 0)
			{
				pthread_join(session->vduct->cloThread, NULL);
			}

			session->vduct->hasThread = 0;
		}

		/*	Then resetOutduct.				*/

		if (session->vduct->semaphore == SM_SEM_NONE)
		{
			session->vduct->semaphore = sm_SemCreate(SM_NO_KEY,
					SM_SEM_FIFO);
		}
		else
		{
			sm_SemUnend(session->vduct->semaphore);
			sm_SemGive(session->vduct->semaphore);
		}

		sm_SemTake(session->vduct->semaphore);	/*	Lock.	*/

		/*	Finally, detach the session from this outduct.	*/

		session->vduct = NULL;
	}
	else	/*	Just forget session's sender thread.		*/
	{
		if (pthread_kill(session->sender, SIGCONT) == 0)
		{
			pthread_join(session->sender, NULL);
		}
	}

	session->hasSender = 0;
}

static void	closeSession(TcpclSession *session)
{
	Sdr	sdr = getIonsdr();

	if (session->hasPlMutex == 0)
	{
		return;		/*	Session has already been ended.	*/
	}

	/*	Serialize this function in case clock and receiver
	 *	threads try to close the session at the same time.	*/

	pthread_mutex_lock(&(session->plMutex));
	if (session->isOpen == 0)
	{
		pthread_mutex_unlock(&(session->plMutex));
		return;
	}

	session->secUntilKeepalive = -1;
	if (session->hasAdmin)
	{
		stopAdminThread(session);
	}

	if (session->hasSender)
	{
		stopSenderThread(session);
	}

	if (session->sock != -1)
	{
		shutdown(session->sock, SD_BOTH);
		closesocket(session->sock);
		session->sock = -1;
	}

	oK(sdr_begin_xn(sdr));
	lyst_clear(session->pipeline);
	oK(sdr_end_xn(sdr));
	if (session->hasSigMutex)
	{
		pthread_mutex_lock(&session->sigMutex);
		lyst_clear(session->signals);
		pthread_mutex_unlock(&session->sigMutex);
	}

	if (session->reconnectInterval == 0)	/*	Never reconnect.*/
	{
		session->secUntilReconnect = -1;
	}
	else
	{
		session->secUntilReconnect = session->reconnectInterval;
	}

	if (session->outductName)
	{
		if (*(session->outductName) == '#')
		{
			/*	This automatic outduct must be
			 *	destroyed as soon as the accepted
			 *	socket connection ends.			*/

			oK(removeOutduct("tcp", session->outductName));

			/*	Reconnection is not possible, so lose
			 *	the automatic outduct name.		*/

			MRELEASE(session->outductName);
			session->outductName = NULL;
		}
	}

	session->isOpen = 0;
	pthread_mutex_unlock(&(session->plMutex));
}

static int	sendShutdown(TcpclSession *session, char reason,
			int doNotReconnect)
{
	char	shutdown[3];
	int	len = 1;
	int	result;

	if (session->sock == -1)	/*	Nothing to send.	*/
	{
		return 0;
	}

	shutdown[0] = 0x50;
	if (reason >= 0)		/*	Reason code.		*/
	{
		shutdown[0] |= 0x02;
		shutdown[1] = reason;
		len++;
	}

	if (doNotReconnect)
	{
		shutdown[0] |= 0x01;
		shutdown[len] = 0;
		len++;
	}

	pthread_mutex_lock(&(session->socketMutex));
	result = itcp_send(&(session->sock), shutdown, len);
	pthread_mutex_unlock(&(session->socketMutex));
	return result;
}

static void	endSession(TcpclSession *session, char reason)
{
	TcpclNeighbor	*neighbor = session->neighbor;

	oK(sendShutdown(session, reason, 0));
	closeSession(session);
	if (session->outductName)
	{
		/*	outductName is erased in closeSession() for
		 *	any automatic outducts created for accepted
		 *	socket connections. So if not erased yet,
		 *	must be for a managed outduct; that outduct's
		 *	name must be erased from the session here to
		 *	break receiver thread out of contact loop.	*/

		MRELEASE(session->outductName);
		session->outductName = NULL;
	}

	if (session->hasReceiver
	&& pthread_kill(session->receiver, SIGCONT) == 0)
	{
		pthread_join(session->receiver, NULL);
	}

	if (session->hasSigMutex)
	{
		pthread_mutex_unlock(&(session->sigMutex));
		microsnooze(100000);
		pthread_mutex_destroy(&(session->sigMutex));
	}

	if (session->hasPlMutex)
	{
		pthread_mutex_unlock(&(session->plMutex));
		microsnooze(100000);
		pthread_mutex_destroy(&(session->plMutex));
	}

	if (session->hasSocketMutex)
	{
		pthread_mutex_unlock(&(session->socketMutex));
		microsnooze(100000);
		pthread_mutex_destroy(&(session->socketMutex));
	}

	if (session->throttle)
	{
		llcv_signal(session->throttle, pipeline_not_full);
		llcv_close(session->throttle);
		session->throttle = NULL;
	}

	if (session->pipeline)
	{
		lyst_destroy(session->pipeline);
	}

	if (session->trigger)
	{
		llcv_signal(session->trigger, llcv_lyst_not_empty);
		llcv_close(session->trigger);
		session->trigger = NULL;
	}

	if (session->signals)
	{
		lyst_destroy(session->signals);
	}

	memset(session, 0, sizeof(TcpclSession));
	session->sock = -1;
	session->neighbor = neighbor;
	session->secUntilShutdown = -1;
	session->secUntilKeepalive = -1;
}

static void	deleteTcpclNeighbor(TcpclNeighbor *neighbor)
{
	int	i;

	for (i = 0; i < 2; i++)
	{
		endSession(&(neighbor->sessions[i]), -1);
	}

	MRELEASE(neighbor);
}

static void	shutDownNeighbors(Lyst neighbors)
{
	Sdr		sdr = getIonsdr();
	LystElt		elt;
	TcpclNeighbor	*neighbor;

	oK(sdr_begin_xn(sdr));
	for (elt = lyst_first(neighbors); elt; elt = lyst_next(elt))
	{
		neighbor = (TcpclNeighbor *) lyst_data(elt);
		neighbor->mustDelete = 1;
	}

	sdr_exit_xn(sdr);
}

/*	*	*	Sender thread functions		*	*	*/

static int	sendBundleByTcpcl(SenderThreadParms *stp, Object bundleZco)
{
	Sdr		sdr = getIonsdr();
	TcpclSession	*session = stp->session;
	TcpclNeighbor	*neighbor = session->neighbor;
	LystElt		elt;
	uvast		bytesRemaining;
	int		flags;
	ZcoReader	reader;
	uvast		bytesToLoad;
	uvast		bytesToSend;
	int		firstByte;
	Sdnv		segLengthSdnv;
	char		segHeader[4];
	int		segHeaderLen;

	if (session->sock == -1)
	{
		return 0;	/*	Session has been lost.	*/
	}

	if (session->segmentAcks)
	{
		if (llcv_wait(session->throttle, pipeline_not_full,
					LLCV_BLOCKING))
		{
			putErrmsg("Wait on TCPCL pipeline throttle condition \
failed.", session->outductName);
			return -1;
		}

		pthread_mutex_lock(&(session->plMutex));
		elt = lyst_insert_last(session->pipeline, (void *) bundleZco);
		pthread_mutex_unlock(&(session->plMutex));
		if (elt == NULL)
		{
			putErrmsg("Can't append transmitted ZCO to tcpcli \
pipeline.", session->outductName);
			return -1;
		}
	}

	zco_start_transmitting(bundleZco, &reader);
	zco_track_file_offset(&reader);
	bytesRemaining = zco_length(sdr, bundleZco);
	flags = 0x02;				/*	1st segment.	*/
	while (bytesRemaining > 0)
	{
		bytesToLoad = bytesRemaining;
		if (bytesToLoad > TCPCL_BUFSZ)
		{
			bytesToLoad = TCPCL_BUFSZ;
		}
		else
		{
			flags |= 0x01;		/*	Last segment.	*/
		}

		CHKERR(sdr_begin_xn(sdr));
		bytesToSend = zco_transmit(sdr, &reader, bytesToLoad,
				stp->buffer);
		if (sdr_end_xn(sdr) < 0 || bytesToSend != bytesToLoad)
		{
			putErrmsg("Incomplete zco_transmit.",
					session->outductName);
			return -1;
		}

		firstByte = 0x10 | flags;
		segHeader[0] = firstByte;
		encodeSdnv(&segLengthSdnv, bytesToLoad);
		memcpy(segHeader + 1, segLengthSdnv.text, segLengthSdnv.length);
		segHeaderLen = 1 + segLengthSdnv.length;
		pthread_mutex_lock(&(session->socketMutex));
		if (itcp_send(&(session->sock), segHeader, segHeaderLen) < 1)
		{
			pthread_mutex_unlock(&(session->socketMutex));
			writeMemoNote("[?] tcpcl session lost (seg header)",
					neighbor->vplan->neighborEid);
			return 0;
		}

		if (itcp_send(&(session->sock), stp->buffer, bytesToSend) < 1)
		{
			pthread_mutex_unlock(&(session->socketMutex));
			writeMemoNote("[?] tcpcl session lost (seg content)",
					neighbor->vplan->neighborEid);
			return 0;
		}

		pthread_mutex_unlock(&(session->socketMutex));
		flags = 0x00;			/*	No longer 1st.	*/
		bytesRemaining -= bytesToSend;
	}

	if (session->segmentAcks == 0)
	{
		/*	Assume successful transmission.			*/

		if (bpHandleXmitSuccess(bundleZco, 0) < 0)
		{
			putErrmsg("Failed handling TCP transmission success.",
					NULL);
			return -1;
		}
	}

	return 1;	/*	Bundle was successfully sent.		*/
}

static int	sendOneBundle(SenderThreadParms *stp)
{
	TcpclSession	*session = stp->session;
	Object		bundleZco;
	BpAncillaryData	ancillaryData;

	while (1)
	{
		if (sm_SemEnded(session->vduct->semaphore))
		{
			writeMemoNote("[i] tcpcli session output stopped",
					session->outductName);
			return 0;	/*	Time to give up.	*/
		}

		/*	Get the next bundle to send.			*/

		if (bpDequeue(session->vduct, &bundleZco, &ancillaryData, -1)
				< 0)
		{
			putErrmsg("Can't dequeue bundle.", NULL);
			return -1;
		}

		if (bundleZco == 0)	/*	Outduct stopped.	*/
		{
			writeMemoNote("[i] tcpcli session output stopped",
					session->outductName);
			return 0;
		}

		if (bundleZco == 1)	/*	Got a corrupt bundle.	*/
		{
			continue;	/*	Get next bundle.	*/
		}

		/*	Send this bundle.				*/

		return sendBundleByTcpcl(stp, bundleZco);
	}
}

static void	*sendBundles(void *parm)
{
	SenderThreadParms	*stp = (SenderThreadParms *) parm;
	TcpclSession		*session = stp->session;
	TcpclNeighbor		*neighbor = session->neighbor;

	session->hasSender = 1;
	session->newlyAdded = 0;
	writeMemoNote("[i] tcpcli sender thread has started",
			neighbor->vplan->neighborEid);

	/*	Load other required sender thread parms.		*/

	stp->buffer = MTAKE(TCPCL_BUFSZ);
	if (stp->buffer == NULL)
	{
		MRELEASE(stp);
		putErrmsg("No memory for TCP buffer.",
				neighbor->vplan->neighborEid);
		ionKillMainThread(procName());
		return NULL;
	}

	/*	Ready to start sending bundles.				*/

	session->vduct->hasThread = 1;
	session->vduct->cloThread = pthread_self();
	while (session->sock != -1)
	{
		switch (sendOneBundle(stp))
		{
		case -1:		/*	System failure.		*/
			putErrmsg("tcpcli failed sending bundle.",
					neighbor->vplan->neighborEid);
			ionKillMainThread(procName());

			/*	Intentional fall-through to next case.	*/

		case 0:	/*	Protocol failure.			*/
			break;		/*	Out of switch.		*/

		case 1:	/*	Successful transmission.		*/

			/*	Reset keepalive countdown.		*/

			if (session->keepaliveInterval > 0)
			{
				session->secUntilKeepalive =
						session->keepaliveInterval;
			}

			/*	Make sure other tasks get to run.	*/

			sm_TaskYield();
			continue;
		}

		break;			/*	Out of loop.		*/
	}

	writeErrmsgMemos();
	writeMemoNote("[i] tcpcli sender thread has ended",
			neighbor->vplan->neighborEid);
	MRELEASE(stp->buffer);
	MRELEASE(stp);
	session->vduct->hasThread = 0;
#if defined(bionic)
	int task_id = sm_TaskIdSelf();
	sm_TaskForget(task_id);
#endif
	return NULL;
}

/*	*	*	Admin thread functions		*	*	*/

static void	*sendSignals(void *parm)
{
	SenderThreadParms	*stp = (SenderThreadParms *) parm;
	TcpclSession		*session = stp->session;
	char			*tag = session->outductName;
	TcpclNeighbor		*neighbor = session->neighbor;
	int			running = 1;
	LystElt			elt;
	saddr			lengthReceived;
	char			keepalive[1] = { 0x40 };
	int			result;
	char			ack[11];
	Sdnv			ackLengthSdnv;
	int			len;

	session->hasAdmin = 1;
	if (neighbor->vplan)
	{
		tag = neighbor->vplan->neighborEid;
	}

	writeMemoNote("[i] tcpcli admin thread has started", tag);

	/*	ready to  start sending signals.			*/

	while (running)
	{
		if (llcv_wait(session->trigger, llcv_lyst_not_empty,
				LLCV_BLOCKING))
		{
			putErrmsg("Wait on TCPCL signal trigger condition \
failed.", tag);
			ionKillMainThread(procName());
			return NULL;
		}

		/*	Grab the socket, then send all queued-up
			acknowledgments and keepalives.			*/

		if (session->sock == -1)
		{
			running = 0;
			continue;
		}

		pthread_mutex_lock(&(session->socketMutex));
		while (lyst_length(session->signals) > 0)
		{
			pthread_mutex_lock(&session->sigMutex);
			elt = lyst_first(session->signals);
			lengthReceived = (saddr) lyst_data(elt);
			lyst_delete(elt);
			pthread_mutex_unlock(&session->sigMutex);
			if (lengthReceived == -1)/*	Shutdown.	*/
			{
				running = 0;
				break;
			}

			if (lengthReceived == 0)/*	Keepalive.	*/
			{
				result = itcp_send(&(session->sock),
						keepalive, 1);
				if (result < 1)
				{
					writeMemoNote("[?] tcpcl session \
lost (keepalive)", tag);
					ionKillMainThread(procName());
					running = 0;
					break;
				}

				continue;
			}

			/*	Signal is an acknowledgment.		*/

			ack[0] = 0x20;
			encodeSdnv(&ackLengthSdnv, lengthReceived);
			memcpy(ack + 1, ackLengthSdnv.text,
					ackLengthSdnv.length);
			len = 1 + ackLengthSdnv.length;
			result = itcp_send(&(session->sock), ack, len);
			if (result < 1)
			{
				writeMemoNote("[?] tcpcl session lost \
(ack)", tag);
				ionKillMainThread(procName());
				running = 0;
				break;
			}
		}

		pthread_mutex_unlock(&(session->socketMutex));
	}

	writeErrmsgMemos();
	writeMemoNote("[i] tcpcli admin thread has ended", tag);
#if defined(bionic)
	int task_id = sm_TaskIdSelf();
	sm_TaskForget(task_id);
#endif
	return NULL;
}

/*	*	*	Receiver thread functions	*	*	*/

static int	sendContactHeader(TcpclSession *session)
{
	int	len = 0;
	char	contactHeader[11 + MAX_EID_LEN];
	int	flagsByte;
	short	keepaliveInterval;
	char	eid[MAX_EID_LEN];
	size_t	eidLength;
	Sdnv	eidLengthSdnv;
	int	result;

	memcpy(contactHeader, "dtn!", 4);
	len += 4;
	contactHeader[len] = 0x03;		/*	Version.	*/
	len += 1;
	flagsByte = TCPCL_SEGMENT_ACKS
			+ (TCPCL_REACTIVE << 1)
			+ (TCPCL_REFUSALS << 2)
			+ (TCPCL_LENGTH_MSGS << 3);
	contactHeader[len] = flagsByte;
	len += 1;
	keepaliveInterval = KEEPALIVE_INTERVAL;
	keepaliveInterval = htons(keepaliveInterval);
	memcpy(contactHeader + len, (char *) &keepaliveInterval, 2);
	len += 2;
	isprintf(eid, sizeof eid, "ipn:" UVAST_FIELDSPEC ".0", getOwnNodeNbr());
	eidLength = istrlen(eid, MAX_EID_LEN);
	encodeSdnv(&eidLengthSdnv, eidLength);
	memcpy(contactHeader + len, eidLengthSdnv.text, eidLengthSdnv.length);
	len += eidLengthSdnv.length;
	memcpy(contactHeader + len, eid, eidLength);
	len += eidLength;

	/*	session->sock is known to be a connected socket.	*/

	pthread_mutex_lock(&(session->socketMutex));
	result = itcp_send(&(session->sock), contactHeader, len);
	pthread_mutex_unlock(&(session->socketMutex));
	return result;
}

static int	receiveContactHeader(ReceiverThreadParms *rtp)
{
	Sdr			sdr = getIonsdr();
	TcpclSession		*session = rtp->session;
	TcpclNeighbor		*neighbor = session->neighbor;
	VPlan			*vplan;
	PsmAddress		vplanElt;
	unsigned char		header[8];
	unsigned short		keepaliveInterval;
	uvast			eidLength;
	char			*eidbuf;
	LystElt			elt;
	TcpclNeighbor		*knownNeighbor;
	pthread_mutex_t		oldPlMutex;
	TcpclSession		*chanceSession;
	int			result = 1;
	VOutduct		*vduct;
	PsmAddress		vductElt;
	SenderThreadParms	*stp;

	findOutduct("tcp", session->outductName, &vduct, &vductElt);
	if (vductElt == 0)
	{
		putErrmsg("tcpli can't find outduct.", session->outductName);
		return -1;
	}

	if (itcp_recv(&session->sock, (char *) header, sizeof header) < 1)
	{
		putErrmsg("Can't get TCPCL contact header.",
				session->outductName);
		return 0;
	}

	if (memcmp(header, "dtn!", 4) != 0)
	{
		writeMemoNote("[?] Invalid TCPCL contact header",
				session->outductName);
		return 0;
	}

	if (header[4] < 3)		/*	Version mismatch.	*/
	{
		oK(sendShutdown(session, 0x01, 0));
		writeMemoNote("[?] Bad version number in TCPCL contact header",
				session->outductName);
		neighbor->mustDelete = 1;
		return 0;
	}

	/*	6th byte of header is flags.				*/

	session->segmentAcks = TCPCL_SEGMENT_ACKS && (header[5] & 0x01);
	session->reactiveFrags = TCPCL_REACTIVE && (header[5] & 0x02);
	session->bundleRefusals = (TCPCL_REFUSALS && (header[5] & 0x04))
			&& session->segmentAcks;
	session->lengthMessages = TCPCL_LENGTH_MSGS && (header[5] & 0x08);

	/*	7th-8th bytes of header are keepalive interval.		*/

	memcpy((char *) &keepaliveInterval, (char *) (header + 6), 2);
	keepaliveInterval = ntohs(keepaliveInterval);
	if (keepaliveInterval < KEEPALIVE_INTERVAL)
	{
		session->keepaliveInterval = keepaliveInterval;
	}
	else
	{
		session->keepaliveInterval = KEEPALIVE_INTERVAL;
	}

	if (session->keepaliveInterval == 0)
	{
		session->secUntilKeepalive = -1;	/*	None.	*/
	}
	else
	{
		session->secUntilKeepalive = session->keepaliveInterval;
	}

	/*	Next is the neighboring node's ID, an endpoint ID.	*/

	if (receiveSdnv(session, &eidLength) < 1)
	{
		putErrmsg("Can't get EID length in TCPCL contact header",
				session->outductName);
		return 0;
	}

	eidbuf = MTAKE(eidLength + 3);	/*	May need / * at end.	*/
	if (eidbuf == NULL)
	{
		putErrmsg("Not enough memory for EID in TCPCL contact header.",
				session->outductName);
		return -1;
	}

	if (itcp_recv(&session->sock, eidbuf, eidLength) < 1)
	{
		MRELEASE(eidbuf);
		putErrmsg("Can't get TCPCL contact header EID.",
				session->outductName);
		return 0;
	}

	eidbuf[eidLength] = '\0';
	if (*(session->outductName) == '#')	/*	From accept().	*/
	{
		CHKERR(sdr_begin_xn(sdr));	/*	Lock list.	*/
		elt = findNeighborForEid(rtp->neighbors, eidbuf);
		if (elt)
		{
			/*	A TcpclNeighbor already exists whose
			 *	eid is equal to eidbuf.  If that
			 *	neighbor already has a session from
			 *	accept(), then abort this session.
			 *	If not, copy this session into that
			 *	existing neighbor.			*/

			knownNeighbor = (TcpclNeighbor *) lyst_data(elt);
			sdr_exit_xn(sdr);	/*	Unlock list.	*/
			chanceSession = knownNeighbor->sessions + TCPCL_CHANCE;

			/*	Serialize this function: neighbor might
			 *	be trying to reconnect while session is
			 *	still being closed, in which case it
			 *	still has a mutex that can be taken.	*/

			oldPlMutex = chanceSession->plMutex;
			pthread_mutex_lock(&oldPlMutex);
			if (chanceSession->sock != -1)
			{
				result = 0;	/*	Rejected.	*/
				oK(sendShutdown(session, 0x02, 0));
			}
			else	/*	Complementary session.		*/
			{
				/*	Copy this session into
				 *	known neighbor.			*/

				memcpy((char *) chanceSession, (char *) session,
						sizeof(TcpclSession));
				chanceSession->neighbor = knownNeighbor;
				rtp->session = chanceSession;

				/*	Make sure deletion of the
				 *	tentative neighbor doesn't
				 *	affect this session.		*/

				session->sock = -1;
				session->hasSocketMutex = 0;
				session->hasPlMutex = 0;
				session->hasSigMutex = 0;
				session->hasReceiver = 0;
				session->hasSender = 0;
				session->vduct = NULL;
				session->hasAdmin = 0;
				session->pipeline = NULL;
				session->signals = NULL;
				session->outductName = NULL;

				/*	Point to known neighbor session.*/

				session = rtp->session;
				if (session->trigger)
				{
					llcv_close(session->trigger);
					session->trigger = NULL;
				}

				session->trigger = llcv_open(session->signals,
						&(session->triggerLlcv));
				if (session->throttle)
				{
					llcv_close(session->throttle);
					session->throttle = NULL;
				}

				session->throttle = llcv_open(session->pipeline,
						&(session->throttleLlcv));

				/*	An optimization: since this node
				 *	has just connected to us, we can
				 *	guess that it will now accept
				 *	a connection from us (if we know
				 *	how to connect to it).  So our
				 *	reconnect interval can now be
				 *	reset to a small value, enabling
				 *	rapid reconnection.		*/

				if (knownNeighbor->sessions[TCPCL_PLANNED].
						outductName)
				{
					knownNeighbor->sessions[TCPCL_PLANNED].
							reconnectInterval = 2;
					knownNeighbor->sessions[TCPCL_PLANNED].
							secUntilReconnect = 2;
				}
			}

			pthread_mutex_unlock(&oldPlMutex);

			/*	In either case, if this session is
			 *	part of a neighbor that has no planned
			 *	session then delete that neighbor; it
			 *	is temporary and is no longer needed.	*/

			if (neighbor->sessions[TCPCL_PLANNED].outductName
					== NULL)
			{
				neighbor->mustDelete = 1;
			}

			if (result == 0)
			{
				MRELEASE(eidbuf);
				return result;
			}
		}
		else	/*	No existing neighbor for this eid.	*/
		{
			/*	This is a new neighbor, so plug in
			 *	the VPlan for that neighbor (creating
			 *	it if necessary), because that Plan
			 *	association was impossible to know
			 *	until this contact header was received.	*/

			sdr_exit_xn(sdr);	/*	Unlock list.	*/

			/*	dtn-scheme EID identifying node is
			 *	useless for routing if it lacks
			 *	terminating wild-card demux; no
			 *	bundle destination EIDs will match it.	*/

			if (strncmp(eidbuf, "dtn:", 4) == 0
			&& *(eidbuf + (eidLength - 1)) != '*')
			{
				/*	Make DTN plan name usable.	*/

				istrcat(eidbuf, "/*", eidLength + 3);
			}

			findPlan(eidbuf, &vplan, &vplanElt);
			if (vplanElt == 0)
			{
				result = addPlan(eidbuf, ION_DEFAULT_XMIT_RATE);
				if (result == 0)
				{
					writeMemoNote("[?] Can't add egress \
plan for this TCPCL contact header", eidbuf);
					MRELEASE(eidbuf);
					return 0;
				}

				if (result < 0 || bpStartPlan(eidbuf) < 0)
				{
					putErrmsg("Can't add automatic egress \
plan for neighbor.", eidbuf);
					MRELEASE(eidbuf);
					return -1;
				}

				findPlan(eidbuf, &vplan, &vplanElt);
			}

			neighbor->vplan = vplan;
		}

		if (attachPlanDuct(eidbuf, vduct->outductElt) < 0)
		{
			putErrmsg("Can't attach duct to plan.", eidbuf);
			MRELEASE(eidbuf);
			return -1;
		}
	}
	else				/*	From connect().		*/
	{
		if (strcmp(eidbuf, neighbor->vplan->neighborEid) != 0)
		{
			/*	The node that we have connect()ed
			 *	to is not the one that we thought
			 *	it was.					*/

			writeMemoNote("[i] expected tcpcl EID",
					neighbor->vplan->neighborEid);
			writeMemoNote("[i] received tcpcl EID", eidbuf);
		}
	}

	MRELEASE(eidbuf);		/*	No longer needed.	*/

	/*	We must start a sender thread for the outduct that
	 *	we use to send bundles to this neighbor.		*/

	session->vduct = vduct;
	sm_SemUnend(session->vduct->semaphore);
	sm_SemGive(session->vduct->semaphore);
	stp = (SenderThreadParms *) MTAKE(sizeof(SenderThreadParms));
	if (stp == NULL)
	{
		putErrmsg("tcpcli can't allocate space for sender parms.", 
				neighbor->vplan->neighborEid);
		return -1;
	}

	stp->session = session;
	if (pthread_begin(&(session->sender), NULL, sendBundles, stp, "tcpcli-session-sender"))
	{
		MRELEASE(stp);
		putSysErrmsg("tcpcli can't create new sender thread", 
				neighbor->vplan->neighborEid);
		return -1;
	}

	if (pthread_begin(&(session->admin), NULL, sendSignals, stp, "tcpcli-session-admin"))
	{
		stopSenderThread(session);
		putSysErrmsg("tcpcli can't create new admin thread", 
				neighbor->vplan->neighborEid);
		return -1;
	}

	return result;
}

static int	handleDataSegment(ReceiverThreadParms *rtp,
			unsigned char msgtypeByte)
{
	TcpclSession	*session = rtp->session;
	TcpclNeighbor	*neighbor = session->neighbor;
	int		result;
	uvast		dataLength;
	size_t		receptionRate;
	float		snoozeInterval;
	uvast		bytesRemaining;
	int		bytesToRead;
	int		extentSize;

	result = receiveSdnv(session, &dataLength);
	if (result < 1)
	{
		return result;
	}

	if (dataLength == 0)		/*	Nuisance data segment.	*/
	{
		session->secSinceReception = 0;
		session->timeoutCount = 0;
		return 1;		/*	Ignore.			*/
	}

	if (msgtypeByte & 0x02)		/*	Start of bundle.	*/
	{
		if (session->lengthReceived > 0)
		{
			/*	Discard partially received bundle.	*/

			bpCancelAcq(rtp->work);
			session->lengthReceived = 0;
		}

		if (bpBeginAcq(rtp->work, 0, NULL) < 0)
		{
			return -1;
		}
	}

	/*	Enforce rate control: snooze appropriate interval
	 *	as dictated by contact plan reception rate.		*/

	receptionRate = rtp->session->neighbor->receptionRate;
	if (receptionRate > 0)
	{
		snoozeInterval = ((float) dataLength / (float) receptionRate)
		       		* 1000000.0;
		microsnooze((int) snoozeInterval);
	}

	/*	Now finish reading the data segment.			*/

	bytesRemaining = dataLength;
	while (bytesRemaining > 0)
	{
		bytesToRead = bytesRemaining;
		if (bytesToRead > TCPCL_BUFSZ)
		{
			bytesToRead = TCPCL_BUFSZ;
		}
 
		extentSize = itcp_recv(&session->sock, rtp->buffer,
				bytesToRead);
		if (extentSize < 1)
		{
			writeMemoNote("[?] Lost TCPCL neighbor",
					neighbor->vplan->neighborEid);
			return 0;
		}

		if (bpContinueAcq(rtp->work, rtp->buffer, extentSize,
				&(rtp->attendant), 0) < 0)
		{
			return -1;
		}

		bytesRemaining -= extentSize;
		session->lengthReceived += extentSize;
		if (session->segmentAcks)	/*	Send ack.	*/
		{
			result = sendSignal(session, session->lengthReceived);
			if (result < 0)
			{
				return result;
			}
		}

		/*	Make sure other tasks have a chance to run.	*/

		sm_TaskYield();
	}

	if (msgtypeByte & 0x01)		/*	End of bundle.		*/
	{
		if (bpEndAcq(rtp->work) < 0)
		{
			return -1;
		}

		session->lengthReceived = 0;
	}

	if (session->secUntilShutdown != -1)
	{
		/*	Will end the session when incoming bundles
		 *	stop, but they are still arriving.		*/

		session->secUntilShutdown = IDLE_SHUTDOWN_INTERVAL;
	}

	session->secSinceReception = 0;
	session->timeoutCount = 0;
	return 1;
}

static int	handleAck(ReceiverThreadParms *rtp, unsigned char msgtypeByte)
{
	TcpclSession	*session = rtp->session;
	Sdr		sdr = getIonsdr();
	int		result;
	uvast		lengthAcked;
	LystElt		elt;
	Object		bundleZco = 0;

	result = receiveSdnv(session, &lengthAcked);
	if (result < 1)
	{
		return result;
	}

	if (session->segmentAcks == 0)	/*	Inappropriate ack.	*/
	{
		return 1;		/*	Ignore it.		*/
	}

	if (lengthAcked == 0)		/*	Nuisance ack.		*/
	{
		return 1;		/*	Ignore it.		*/
	}

	oK(sdr_begin_xn(sdr));
	if (session->lengthSent == 0)
	{
		/*	Must get the oldest bundle, to which this ack
		 *	pertains.					*/

		pthread_mutex_lock(&(session->plMutex));
		elt = lyst_first(session->pipeline);
		if (elt)
		{
			bundleZco = (Object) lyst_data(elt);
		}

		pthread_mutex_unlock(&(session->plMutex));
		if (bundleZco == 0)
		{
			/*	Nothing to acknowledge.			*/

			sdr_exit_xn(sdr);
			return 1;	/*	Ignore acknowledgment.	*/
		}

		/*	Initialize for ack reception.			*/

		session->lengthSent = zco_length(sdr, bundleZco);
		session->lengthAcked = 0;
	}

	if (lengthAcked <= session->lengthAcked
	|| lengthAcked > session->lengthSent)
	{
		/*	Acknowledgment sequence is violated, so 
		 *	didn't ack the end of the oldest bundle.	*/

		pthread_mutex_lock(&(session->plMutex));
		elt = lyst_first(session->pipeline);
		if (elt)
		{
			bundleZco = (Object) lyst_data_set(elt, NULL);
			lyst_delete(elt);
		}

		pthread_mutex_unlock(&(session->plMutex));
		llcv_signal(session->throttle, pipeline_not_full);
		if (bundleZco == 0)
		{
			if (sdr_end_xn(sdr) < 0)
			{
				putErrmsg("Can't clear oldest bundle.",
					session->neighbor->vplan->neighborEid);
			}

			return -1;
		}

		result = bpHandleXmitFailure(bundleZco);
	}
	else	/*	Acknowledgments are ascending.			*/
	{
		session->lengthAcked = lengthAcked;
		if (session->lengthAcked < session->lengthSent)
		{
			sdr_exit_xn(sdr);
			return 1;	/*	Not fully acked yet.	*/
		}

		/*	Entire bundle has been received.		*/

		pthread_mutex_lock(&(session->plMutex));
		elt = lyst_first(session->pipeline);
		if (elt)
		{
			bundleZco = (Object) lyst_data_set(elt, NULL);
			lyst_delete(elt);
		}

		pthread_mutex_unlock(&(session->plMutex));
		llcv_signal(session->throttle, pipeline_not_full);
		if (bundleZco == 0)
		{
			if (sdr_end_xn(sdr) < 0)
			{
				putErrmsg("Can't clear oldest bundle.",
					session->neighbor->vplan->neighborEid);
			}

			return -1;
		}

		result = bpHandleXmitSuccess(bundleZco, 0);
	}

	if (result < 0)
	{
		oK (sdr_end_xn(sdr));
		putErrmsg("Can't clear oldest bundle.",
				session->neighbor->vplan->neighborEid);
		return -1;		/*	System failure.		*/
	}

	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't handle TCPCL ack.",
				session->neighbor->vplan->neighborEid);
		return -1;
	}

	session->lengthSent = 0;
	return 1;
}

static int	handleRefusal(ReceiverThreadParms *rtp,
			unsigned char msgtypeByte)
{
	return 1;			/*	Refusals are ignored.	*/
}

static int	handleKeepalive(ReceiverThreadParms *rtp,
			unsigned char msgtypeByte)
{
	TcpclSession	*session = rtp->session;

	session->secSinceReception = 0;
	session->timeoutCount = 0;
	return 1;
}

static int	handleShutdown(ReceiverThreadParms *rtp,
			unsigned char msgtypeByte)
{
	TcpclSession	*session = rtp->session;
	TcpclNeighbor	*neighbor = session->neighbor;
	int		result;
	unsigned char	reasonCode;
	uvast		reconnectInterval;

	if (msgtypeByte & 0x02)
	{
		result = irecv(session->sock, (char *) &reasonCode, 1, 0);
		switch (result)
		{
		case -1:
			if (errno != EINTR)	/*	(Shutdown)	*/
			{
				putSysErrmsg("irecv() error on TCP socket",
						neighbor->vplan->neighborEid);
			}

			/*	Intentional fall-through to next case.	*/

		case 0:			/*	Neighbor closed.	*/
			return 0;
		}

		if (reasonCode == 0x01)	/*	Version mismatch.	*/
		{
			neighbor->mustDelete = 1;
		}
	}

	if (msgtypeByte & 0x01)
	{
		result = receiveSdnv(session, &reconnectInterval);
		if (result < 1)
		{
			return result;
		}

		session->reconnectInterval = reconnectInterval;
		if (session->reconnectInterval == 0)
		{
			neighbor->mustDelete = 1;
		}
	}

	return 0;			/*	Abandon session.	*/
}

static int	handleLength(ReceiverThreadParms *rtp,
			unsigned char msgtypeByte)
{
	int	result;
	uvast	bundleLength;

	result = receiveSdnv(rtp->session, &bundleLength);
	if (result < 1)
	{
		return result;
	}

	return 1;			/*	LENGTH is ignored.	*/
}

static int	handleMessages(ReceiverThreadParms *rtp)
{
	TcpclSession	*session = rtp->session;
	unsigned char	msgtypeByte;
	int		msgType;

	while (1)
	{
		switch (irecv(session->sock, (char *) &msgtypeByte, 1, 0))
		{
		case -1:
			if (errno != EINTR)	/*	Not shutdown.	*/
			{
				putSysErrmsg("irecv() error on TCP socket",
						session->outductName);
			}

			/*	Intentional fall-through to next case.	*/

		case 0:			/*	Neighbor closed.	*/
			return 0;
		}

		msgType = (msgtypeByte >> 4) & 0x0f;
		switch (msgType)
		{
		case 0x01:
			switch (handleDataSegment(rtp, msgtypeByte))
			{
			case -1:
				putErrmsg("tcpcli segment handling error.",
						session->outductName);
				return -1;

			case 0:		/*	Session closed.	*/
				return 0;
			}

			continue;

		case 0x02:
			switch (handleAck(rtp, msgtypeByte))
			{
			case -1:
				putErrmsg("tcpcli segment handling error.",
						session->outductName);
				return -1;

			case 0:		/*	Session closed.	*/
				return 0;
			}

			continue;

		case 0x03:
			switch (handleRefusal(rtp, msgtypeByte))
			{
			case -1:
				putErrmsg("tcpcli segment handling error.",
						session->outductName);
				return -1;

			case 0:		/*	Session closed.	*/
				return 0;
			}

			continue;

		case 0x04:
			switch (handleKeepalive(rtp, msgtypeByte))
			{
			case -1:
				putErrmsg("tcpcli segment handling error.",
						session->outductName);
				return -1;

			case 0:		/*	Session closed.	*/
				return 0;
			}

			continue;

		case 0x05:
			switch (handleShutdown(rtp, msgtypeByte))
			{
			case -1:
				putErrmsg("tcpcli segment handling error.",
						session->outductName);
				return -1;

			case 0:		/*	Session closed.	*/
				return 0;
			}

			continue;

		case 0x06:
			switch (handleLength(rtp, msgtypeByte))
			{
			case -1:
				putErrmsg("tcpcli segment handling error.",
						session->outductName);
				return -1;

			case 0:		/*	Session closed.	*/
				return 0;
			}

			continue;

		default:
			writeMemoNote("[?] TCPCL unknown message type",
					itoa(msgType));
			return 0;
		}
	}
}

static void	*handleContacts(void *parm)
{
	ReceiverThreadParms	*rtp = (ReceiverThreadParms *) parm;
	TcpclSession		*session = rtp->session;
	TcpclNeighbor		*neighbor = session->neighbor;
	char			*tag = session->outductName;
	int			running = 1;
	int			result;

	session->hasReceiver = 1;
	if (neighbor->vplan)
	{
		tag = neighbor->vplan->neighborEid;
	}

	/*	Load other required receiver thread parmss.		*/

	rtp->work = bpGetAcqArea(neighbor->induct);
	if (rtp->work == NULL)
	{
		MRELEASE(rtp);
		putErrmsg("tcpcli can't get acquisition work area.", tag);
		ionKillMainThread(procName());
		return NULL;
	}

	rtp->buffer = MTAKE(TCPCL_BUFSZ);
	if (rtp->buffer == NULL)
	{
		bpReleaseAcqArea(rtp->work);
		MRELEASE(rtp);
		putErrmsg("No memory for TCP buffer.", tag);
		ionKillMainThread(procName());
		return NULL;
	}

	if (ionStartAttendant(&(rtp->attendant)) < 0)
	{
		MRELEASE(rtp->buffer);
		bpReleaseAcqArea(rtp->work);
		MRELEASE(rtp);
		putErrmsg("Can't initialize blocking TCP reception.", tag);
		ionKillMainThread(procName());
		return NULL;
	}

	/*	Now loop through possibly multiple contact episodes.	*/

	while (running)
	{
		if (session->sock == -1)	/*	Closed.		*/
		{
			if (session->outductName == NULL)
			{
				running = 0;
				continue;
			}

			if (*(session->outductName) == '#')
			{
				/*	This is a session established
				 *	by accepting a TCP connection
				 *	from a peer node; only that
				 *	peer node can reopen it.	*/

				running = 0;
				continue;
			}

			/*	Session can be re-established.		*/

			switch (reopenSession(session))
			{
			case -1:
				ionKillMainThread(procName());
				running = 0;
				continue;

			case 0:			/*	No reconnect.	*/
				snooze(1);
				continue;

			default:		/*	Reconnected.	*/
				session->reconnectInterval = 1;
				session->secUntilReconnect = -1;
			}
		}

		/*	Session is known to be open, so can now
		 *	exchange contact headers.			*/

		session->isOpen = 1;
		if (sendContactHeader(session) < 1)
		{
			writeMemoNote("[i] tcpcli did not send contact header",
					tag);
			closeSession(session);
			continue;	/*	Try again.		*/
		}

		result = receiveContactHeader(rtp);

		/*	Contact header reception has started a sender
		 *	thread, may have attached this receiver thread
		 *	to a previously established neighbor.		*/

		session = rtp->session;
		neighbor = session->neighbor;
		switch (result)
		{
		case -1:		/*	System failure.		*/
			putErrmsg("Failure receiving contact header", tag);
			ionKillMainThread(procName());
			running = 0;
			closeSession(session);
			continue;	/*	Terminate the loop.	*/

		case 0:			/*	Protocol faiure.	*/
			writeMemoNote("[i] tcpcli got no valid contact header",
					tag);
			closeSession(session);
			continue;	/*	Try again.		*/
		}

		/*	Contact episode has begun.			*/

		tag = neighbor->vplan->neighborEid;
		session->lengthReceived = 0;
		if (*(session->outductName) == '#')
		{
			/*	From accept(), so end the session
			 *	when the incoming bundles end.		*/

			session->secUntilShutdown = IDLE_SHUTDOWN_INTERVAL;
		}

		session->secSinceReception = 0;
		session->timeoutCount = 0;
		if (handleMessages(rtp) < 0)
		{
			ionKillMainThread(procName());
			running = 0;
		}

		if (session->lengthReceived > 0)
		{
			/*	Must discard partially received
			 *	bundle; otherwise, next contact will
			 *	begin out of bundle acquisition sync,
			 *	resulting in bundle parsing failures.	*/

			bpCancelAcq(rtp->work);
			session->lengthReceived = 0;
		}

		closeSession(session);
	}

	ionPauseAttendant(&(rtp->attendant));
	ionStopAttendant(&(rtp->attendant));
	writeErrmsgMemos();
	writeMemoNote("[i] tcpcli receiver thread has ended", tag);
	MRELEASE(rtp->buffer);
	bpReleaseAcqArea(rtp->work);
	MRELEASE(rtp);
#if defined(bionic)
	int task_id = sm_TaskIdSelf();
	sm_TaskForget(task_id);
#endif
	return NULL;
}

/*	*	*	Server thread functions		*	*	*/

typedef struct
{
	int		serverSocket;
	VInduct		*induct;
	int		running;
	Lyst		backlog;	/*	Pending sessions.	*/
	pthread_mutex_t	*backlogMutex;
} ServerThreadParms;

static void	*spawnReceivers(void *parm)
{
	/*	Main loop for acceptance of sessions and creation
	 *	of threads to service those sessions.		*/

	ServerThreadParms	*stp = (ServerThreadParms *) parm;
	saddr			newSocket;
	struct sockaddr		socketName;
	socklen_t		socknamelen;
	LystElt			elt;

	snooze(1);	/*	Let main thread become interruptable.	*/

	/*	Can now begin accepting sessions from neighboring
	 *	nodes.  On failure, take down the whole CLA.		*/

	while (stp->running)
	{
		socknamelen = sizeof socketName;
		newSocket = accept(stp->serverSocket, &socketName,
				&socknamelen);
		if (newSocket < 0)
		{
			putSysErrmsg("tcpcli accept() failed", NULL);
			ionKillMainThread(procName());
			stp->running = 0;
			continue;
		}

		if (stp->running == 0)	/*	Main thread shutdown.	*/
		{
			closesocket(newSocket);
			continue;
		}

		if (watchSocket(newSocket) < 0)
		{
			closesocket(newSocket);
			putErrmsg("tcpcli can't watch socket.", NULL);
			ionKillMainThread(procName());
			stp->running = 0;
			continue;
		}

		pthread_mutex_lock(stp->backlogMutex);
		elt = lyst_insert_last(stp->backlog, (void *) newSocket);
		pthread_mutex_unlock(stp->backlogMutex);
		if (elt == NULL)
		{
			putErrmsg("tcpcli backlog insertion failed.", NULL);
			ionKillMainThread(procName());
			stp->running = 0;
			continue;
		}

		/*	Make sure other tasks have a chance to run.	*/

		sm_TaskYield();
	}

	writeErrmsgMemos();
	writeMemo("[i] tcpcli server thread has ended.");
#if defined(bionic)
	int task_id = sm_TaskIdSelf();
	sm_TaskForget(task_id);
#endif
	return NULL;
}

/*	*	*	Clock thread functions		*	*	*/

typedef struct
{
	VInduct		*induct;
	int		running;
	Lyst		backlog;	/*	Pending sessions.	*/
	pthread_mutex_t	*backlogMutex;
	Lyst		neighbors;
} ClockThreadParms;

static int	beginSessionForDuct(ClockThreadParms *ctp, LystElt neighborElt,
			VPlan *vplan, char *outductName)
{
	size_t		len = 0;
	char		*eid;
	TcpclNeighbor	*neighbor;
	TcpclSession	*session;
	int		sock;

	len = istrlen(outductName, DUCT_BUFLEN);
	CHKERR(len > 2);
	len++;			/*	Add room for terminating NULL.	*/
	if (vplan)
	{
		eid = vplan->neighborEid;
	}
	else
	{
		eid = "<unknown node>";
	}

	/*	If an open session already exists for this duct,
	 *	nothing to do.						*/

	neighbor = (TcpclNeighbor *) lyst_data(neighborElt);
	if (*outductName == '#')	/*	Accepted socket duct.	*/
	{
		sock = atoi(outductName + 2);
		session = &(neighbor->sessions[TCPCL_CHANCE]);
		if (session->outductName)
		{
			closesocket(sock);
			writeMemoNote("[?] Already have an accepted TCPCL \
session with this neighbor", eid); 
			return 0;
		}

		if ((session->outductName = MTAKE(len)) == NULL)
		{
			closesocket(sock);
			putErrmsg("tcpcli can't copy duct name.", outductName);
			return -1;
		}

		oK(istrcpy(session->outductName, outductName, len));
		return beginSession(neighborElt, sock, TCPCL_CHANCE);
	}

	/*	This is a duct for which we need to make a connection.	*/

	session = &(neighbor->sessions[TCPCL_PLANNED]);
	if (session->hasReceiver)	/*	Already connected.	*/
	{
		return 0;
	}

	if (session->outductName == NULL)
	{
		if ((session->outductName = MTAKE(len)) == NULL)
		{
			putErrmsg("tcpcli can't copy socket spec.", eid);
			return -1;
		}

		oK(istrcpy(session->outductName, outductName, len));
	}

	switch (itcp_connect(session->outductName, BpTcpDefaultPortNbr,
				&sock))
	{
	case -1:	/*	System failure.				*/
		putErrmsg("tcpcli can't connect to remote node.",
				session->outductName);
		return -1;

	case 0:		/*	Protocol failure.			*/
		return 0;
	}

	if (watchSocket(sock) < 0)
	{
		closesocket(sock);
		putErrmsg("tcpcli can't watch socket.", session->outductName);
		return -1;
	}

	/*	TCP connection succeeded, so establish the TCPCL
	 *	session.						*/

	return beginSession(neighborElt, sock, TCPCL_PLANNED);
}

static int	rescanPlans(ClockThreadParms *ctp)
{
	Sdr		sdr = getIonsdr();
	PsmPartition	wm = getIonwm();
	BpVdb		*vdb = getBpVdb();
	ClProtocol	clp;
	Object		protocolElt;
	Object		protocolObj;
	PsmAddress	vplanElt;
	VPlan		*vplan;
	Object		planObj;
			OBJ_POINTER(BpPlan, plan);
	Object		ductElt;
	Object		outductElt;
			OBJ_POINTER(Outduct, outduct);
	LystElt		neighborElt;

	CHKERR(sdr_begin_xn(sdr));
	fetchProtocol("tcp", &clp, &protocolElt);
	protocolObj = sdr_list_data(sdr, protocolElt);
	for (vplanElt = sm_list_first(wm, vdb->plans); vplanElt;
			vplanElt = sm_list_next(wm, vplanElt))
	{
		vplan = (VPlan *) psp(wm, sm_list_data(wm, vplanElt));
		planObj = sdr_list_data(sdr, vplan->planElt);
		GET_OBJ_POINTER(sdr, BpPlan, plan, planObj);
		for (ductElt = sdr_list_first(sdr, plan->ducts); ductElt;
				ductElt = sdr_list_next(sdr, ductElt))
		{
			outductElt = sdr_list_data(sdr, ductElt);
			GET_OBJ_POINTER(sdr, Outduct, outduct,
					sdr_list_data(sdr, outductElt));
			if (outduct->protocol != protocolObj)
			{
				continue;	/*	Not a tcp duct.	*/
			}

			if (outduct->name[0] == '#')
			{
				/*	Automatic outduct created for
				 *	an accepted session.		*/

				continue;
			}

			/*	This outduct assignment references a
			 *	TCP outduct for which we may not have
			 *	a session.  Begin session for the TCP
			 *	outduct with the indicated duct name.	*/

			neighborElt = findNeighborForEid(ctp->neighbors,
					vplan->neighborEid);
			if (neighborElt == NULL)
			{
				neighborElt = addTcpclNeighbor(vplan,
						ctp->induct, ctp->neighbors);
				if (neighborElt == NULL)
				{
					sdr_cancel_xn(sdr);
					putErrmsg("Can't add neighbor.",
							vplan->neighborEid);
					return -1;
				}
			}

			if (beginSessionForDuct(ctp, neighborElt, vplan,
					outduct->name) < 0)
			{
				sdr_cancel_xn(sdr);
				putErrmsg("tcpcli can't add planned session.",
						NULL);
				return -1;
			}
		}
	}

	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("tcpcli failed rescanning plans.", NULL);
		return -1;
	}

	return 0;
}

static int	noLongerReferenced(char *outductName)
{
	Sdr		sdr = getIonsdr();
	VOutduct	*vduct;
	PsmAddress	vductElt;
	Object		planElt;
	Object		planObj;
			OBJ_POINTER(BpPlan, plan);
	Object		ductElt;
	Object		outductElt;

	CHKERR(sdr_begin_xn(sdr));
	findOutduct("tcp", outductName, &vduct, &vductElt);
	for (planElt = sdr_list_first(sdr, getBpConstants()->plans); planElt;
			planElt = sdr_list_next(sdr, planElt))
	{
		planObj = sdr_list_data(sdr, planElt);
		GET_OBJ_POINTER(sdr, BpPlan, plan, planObj);
		for (ductElt = sdr_list_first(sdr, plan->ducts); ductElt;
				ductElt = sdr_list_next(sdr, ductElt))
		{
			outductElt = sdr_list_data(sdr, ductElt);
			if (outductElt == vduct->outductElt)
			{
				sdr_exit_xn(sdr);
				return 0;	/*	Outduct in use.	*/
			}
		}
	}

	sdr_exit_xn(sdr);
	return 1;				/*	Outduct unused.	*/
}

static int	rescan(ClockThreadParms *ctp)
{
	LystElt		elt;
	TcpclNeighbor	*neighbor;
	int		i;
	TcpclSession	*session;

	/*	First look for newly added or modified plans and
	 *	try to create sessions for them.			*/

	if (rescanPlans(ctp) < 0)
	{
		return -1;
	}

	/*	Now look for sessions that must be ended because the
	 *	references to their outducts have been removed.		*/

	for (elt = lyst_first(ctp->neighbors); elt; elt = lyst_next(elt))
	{
		neighbor = (TcpclNeighbor *) lyst_data(elt);
		for (i = 0; i < 2; i++)
		{
			session = &(neighbor->sessions[i]);
			if (session->newlyAdded)
			{
				/*	Too soon to clean up.		*/

				continue;
			}

			if (session->outductName == NULL)
			{
				/*	Nothing to clean up.		*/

				continue;
			}

			/*	Session is well established, so clean
			 *	up as necessary.			*/

			if (noLongerReferenced(session->outductName))
			{
				endSession(session, -1);
			}
		}
	}

	return 0;
}

static int	clearBacklog(ClockThreadParms *ctp)
{
	Sdr		sdr = getIonsdr();
	LystElt		elt;
	int		sock;
	LystElt		neighborElt;
	char		outductName[32];
	int		result;

	pthread_mutex_lock(ctp->backlogMutex);
	while ((elt = lyst_first(ctp->backlog)))
	{
		sock = (saddr) lyst_data(elt);

		/*	This new neighbor may be temporary.  After
		 *	header exchange we may be loading this
		 *	session onto another existing neighbor.		*/

		oK(sdr_begin_xn(sdr));		/*	Lock memory.	*/
		neighborElt = addTcpclNeighbor(NULL, ctp->induct,
				ctp->neighbors);
		sdr_exit_xn(sdr);		/*	Unlock.		*/
		if (neighborElt == NULL)
		{
			closesocket(sock);
			pthread_mutex_unlock(ctp->backlogMutex);
			putErrmsg("tcpcli can't add temporary tcpcl neighbor.",
					NULL);
			return -1;
		}

		/*	Create automatic Outduct for this socket.	*/

		isprintf(outductName, sizeof outductName, "#:%d", sock);
		if (addOutduct("tcp", outductName, "", 0) < 0)
		{
			closesocket(sock);
			pthread_mutex_unlock(ctp->backlogMutex);
			putErrmsg("tcpcli can't add automatic outduct.",
					outductName);
			return -1;
		}

		/*	Begin session for the new Outduct.		*/

		oK(sdr_begin_xn(sdr));		/*	Lock memory.	*/
		result = beginSessionForDuct(ctp, neighborElt, NULL,
				outductName);
		sdr_exit_xn(sdr);		/*	Unlock.		*/
		if (result < 0)
		{
			closesocket(sock);
			pthread_mutex_unlock(ctp->backlogMutex);
			putErrmsg("tcpcli can't add responsive session.", NULL);
			return -1;
		}

		/*	lyst_delete function for backlog lyst closes
		 *	sock unless it's -1.				*/

		oK(lyst_data_set(elt, (void *) -1));
		lyst_delete(elt);
	}

	pthread_mutex_unlock(ctp->backlogMutex);
	return 0;
}

static void	checkSession(TcpclSession *session)
{
	/*	Track idleness in bundle traffic.			*/

	if (session->secUntilShutdown > 0)
	{
		session->secUntilShutdown--;
	}

	if (session->secUntilShutdown == 0)
	{
		endSession(session, 0);
		return;
	}

	/*	Track idleness in all reception.			*/

	if (session->keepaliveInterval != 0)
	{
		session->secSinceReception++;
		if (session->secSinceReception == 
				session->keepaliveInterval)
		{
			session->timeoutCount++;
			if (session->timeoutCount > 1)
			{
				endSession(session, 0);
				return;
			}
		}
	}

	/*	Track sending of keepalives.				*/

	if (session->secUntilKeepalive > 0)
	{
		session->secUntilKeepalive--;
	}

	if (session->secUntilKeepalive == 0)
	{
		if (sendSignal(session, 0) < 0)	/*	Keepalive.	*/
		{
			closeSession(session);
		}
		else
		{
			session->secUntilKeepalive
					= session->keepaliveInterval;
		}
	}

	/*	Count down to reconnect.				*/

	if (session->secUntilReconnect > 0)
	{
		session->secUntilReconnect--;
	}
}

static void	*handleEvents(void *parm)
{
	/*	Main loop for implementing time-driven operations.	*/

	ClockThreadParms	*ctp = (ClockThreadParms *) parm;
	Sdr			sdr = getIonsdr();
	BpDB			*bpConstants = getBpConstants();
	time_t			planChangeTime;
	time_t			lastPlanChange = 0;
	int			rescanInterval = 1;
	int			secUntilRescan = 2;
	LystElt			elt;
	LystElt			nextElt;
	TcpclNeighbor		*neighbor;
	IonVdb			*ionvdb = getIonVdb();
	PsmAddress		nextNeighbor;
	IonNeighbor		*ionNeighbor;
	int			i;

	while (ctp->running)
	{
		/*	Begin TCPCL sessions for all accepted TCP
		 *	connections.					*/

		if (clearBacklog(ctp) < 0)
		{
			ionKillMainThread(procName());
			ctp->running = 0;
			continue;
		}

		/*	Now try to begin TCPCL sessions for all egress
		 *	plans that are newly added or removed, or for
		 *	which TCPCL outducts have been newly attached
		 *	or detached.					*/

		planChangeTime = sdr_list_user_data(sdr, bpConstants->plans);
		if (planChangeTime > lastPlanChange)
		{
			secUntilRescan = 1;
			lastPlanChange = planChangeTime;
		}

		secUntilRescan--;
		if (secUntilRescan == 0)
		{
			if (rescan(ctp) < 0)
			{
				ionKillMainThread(procName());
				ctp->running = 0;
				continue;
			}

			secUntilRescan = rescanInterval;
			rescanInterval <<= 1;
			if (rescanInterval > MAX_RESCAN_INTERVAL)
			{
				rescanInterval = MAX_RESCAN_INTERVAL;
			}
		}

		/*	Now look for other events whose time has come.	*/

		for (elt = lyst_first(ctp->neighbors); elt; elt = nextElt)
		{
			nextElt = lyst_next(elt);
			neighbor = (TcpclNeighbor *) lyst_data(elt);
			if (neighbor->mustDelete)
			{
				deleteTcpclNeighbor(neighbor);
				lyst_delete(elt);
				continue;
			}

			if (neighbor->vplan)
			{
				ionNeighbor = findNeighbor(ionvdb,
					neighbor->vplan->neighborNodeNbr,
					&nextNeighbor);
				if (ionNeighbor)
				{
					neighbor->receptionRate
						= ionNeighbor->recvRate;
				}
			}

			for (i = 0; i < 2; i++)
			{
				checkSession(&(neighbor->sessions[i]));
			}
		}

		snooze(1);
	}

	writeErrmsgMemos();
	writeMemo("[i] tcpcli clock thread has ended.");
#if defined(bionic)
	int task_id = sm_TaskIdSelf();
	sm_TaskForget(task_id);
#endif
	return NULL;
}

/*	*	*	Main thread functions		*	*	*/

static void	dropPendingSession(LystElt elt, void *userdata)
{
	saddr	sock = (uaddr) lyst_data(elt);

	if (sock != -1)
	{
		closesocket(sock);
	}
}

static void	wakeUpServerThread(struct sockaddr *socketName)
{
	int	sock;

	/*	Wake up the server thread by connecting to it.		*/

	sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock >= 0)
	{
		oK(connect(sock, socketName, sizeof(struct sockaddr)));

		/*	Immediately discard the connected socket.	*/

		closesocket(sock);
	}
}

#if defined (ION_LWT)
int	tcpcli(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
	char	*ductName = (char *) a1;
#else
int	main(int argc, char *argv[])
{
	char	*ductName = (argc > 1 ? argv[1] : NULL);
#endif
	VInduct			*vduct;
	PsmAddress		vductElt;
	unsigned short		portNbr;
	unsigned int		hostNbr;
	struct sockaddr		socketName;
	struct sockaddr_in	*inetName;
	ServerThreadParms	stp;
	socklen_t		nameLength;
	Lyst			neighbors;
	Lyst			backlog;
	pthread_mutex_t		backlogMutex;
	pthread_t		serverThread;
	ClockThreadParms	ctp;
	pthread_t		clockThread;

	if (ductName == NULL)
	{
		PUTS("Usage: tcpcli <local host name>[:<port number>]");
		return 0;
	}

	if (bpAttach() < 0)
	{
		writeMemo("[?] tcpcli can't attach to bundle protocol.");
		return 1;
	}

	if (parseSocketSpec(ductName, &portNbr, &hostNbr) != 0)
	{
		writeMemoNote("[?] tcpcli: can't get induct IP address",
				ductName);
		return 1;
	}

	if (portNbr == 0)
	{
		portNbr = BpTcpDefaultPortNbr;
	}

	findInduct("tcp", ductName, &vduct, &vductElt);
	if (vductElt == 0)
	{
		writeMemoNote("[?] tcpcli: no induct", ductName);
		return 1;
	}

	/*	NOTE: no outduct lookup here, because all TCPCL
	 *	outducts are created invisibly and dynamically
	 *	as sessions are initiated.  None are created during
	 *	node configuration.					*/

	if (vduct->cliPid != ERROR && vduct->cliPid != sm_TaskIdSelf())
	{
		writeMemoNote("[?] tcpcli task is already started",
				itoa(vduct->cliPid));
		return 1;
	}

	/*	All command-line arguments are now validated, so
		begin initialization by creating the neighbors lyst.	*/

	neighbors = lyst_create_using(getIonMemoryMgr());
	if (neighbors == NULL)
	{
		putErrmsg("tcpcli can't create lyst of neighbors.", NULL);
		return 1;
	}

	backlog = lyst_create_using(getIonMemoryMgr());
	if (backlog == NULL)
	{
		putErrmsg("tcpcli can't create backlog lyst.", NULL);
		lyst_destroy(neighbors);
		return 1;
	}

	lyst_delete_set(backlog, dropPendingSession, NULL);
	pthread_mutex_init(&backlogMutex, NULL);

	/*	Now create the server socket.				*/

	portNbr = htons(portNbr);
	hostNbr = htonl(hostNbr);
	memset((char *) &(socketName), 0, sizeof(struct sockaddr));
	inetName = (struct sockaddr_in *) &(socketName);
	inetName->sin_family = AF_INET;
	inetName->sin_port = portNbr;
	memcpy((char *) &(inetName->sin_addr.s_addr), (char *) &hostNbr, 4);
	stp.serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (stp.serverSocket < 0)
	{
		putSysErrmsg("Can't open TCP server socket", NULL);
		lyst_destroy(backlog);
		lyst_destroy(neighbors);
		return 1;
	}

	nameLength = sizeof(struct sockaddr);
	if (reUseAddress(stp.serverSocket)
	|| bind(stp.serverSocket, &socketName, nameLength) < 0
	|| listen(stp.serverSocket, 5) < 0
	|| getsockname(stp.serverSocket, &socketName, &nameLength) < 0)
	{
		closesocket(stp.serverSocket);
		lyst_destroy(backlog);
		lyst_destroy(neighbors);
		putSysErrmsg("Can't initialize TCP server socket", NULL);
		return 1;
	}

	/*	Set up signal handling: SIGTERM is shutdown signal.	*/

	ionNoteMainThread("tcpcli");
#ifndef mingw
	isignal(SIGPIPE, itcp_handleConnectionLoss);
	isignal(SIGINT, handleStopThread);
#endif
	isignal(SIGTERM, handleStopTcpcli);

	/*	Start the clock thread, which immediately does
	 *	initial load of the neighbors lyst.			*/

	ctp.induct = vduct;
	ctp.running = 1;
	ctp.neighbors = neighbors;
	ctp.backlog = backlog;
	ctp.backlogMutex = &backlogMutex;
	if (pthread_begin(&clockThread, NULL, handleEvents, &ctp, "tcpcli-clock"))
	{
		closesocket(stp.serverSocket);
		lyst_destroy(backlog);
		lyst_destroy(neighbors);
		putSysErrmsg("tcpcli can't create clock thread", NULL);
		return 1;
	}

	/*	Start the server thread.				*/

	stp.induct = vduct;
	stp.running = 1;
	stp.backlog = backlog;
	stp.backlogMutex = &backlogMutex;
	if (pthread_begin(&serverThread, NULL, spawnReceivers, &stp, "tcpcli-server"))
	{
		shutDownNeighbors(neighbors);
		snooze(2);	/*	Let clock thread clean up.	*/
		ctp.running = 0;
		if (pthread_kill(clockThread, SIGCONT) == 0)
		{
			pthread_join(clockThread, NULL);
		}

		closesocket(stp.serverSocket);
		lyst_destroy(backlog);
		lyst_destroy(neighbors);
		putSysErrmsg("tcpcli can't create server thread", NULL);
		return 1;
	}

	/*	Now sleep until interrupted by SIGTERM, at which point
	 *	it's time to stop the CLA.				*/

	{
		char	txt[500];

		isprintf(txt, sizeof(txt),
				"[i] tcpcli is running [%s:%d].", 
				inet_ntoa(inetName->sin_addr),
				ntohs(inetName->sin_port));
		writeMemo(txt);
	}

	ionPauseMainThread(-1);

	/*	Time to shut down.					*/

	stp.running = 0;
	wakeUpServerThread(&socketName);
	if (pthread_kill(serverThread, SIGCONT) == 0)
	{
		pthread_join(serverThread, NULL);
	}

	shutDownNeighbors(neighbors);
	snooze(2);		/*	Let clock thread clean up.	*/
	ctp.running = 0;
	if (pthread_kill(clockThread, SIGCONT) == 0)
	{
		pthread_join(clockThread, NULL);
	}

	closesocket(stp.serverSocket);
	pthread_mutex_destroy(&backlogMutex);
	lyst_destroy(backlog);
	lyst_destroy(neighbors);
	writeErrmsgMemos();
	writeMemo("[i] tcpcli has ended.");
	bp_detach();
	return 0;
}
