/*
	tcpcli.c:	ION TCP convergence-layer adapter daemon.
			Handles both transmission and reception.

	NOTE: this source file implements TCPCL as defined by
	RFC 7242.  There are some flaws in RFC 7242, which this
	implementation attempts to accommodate.  One important
	note: the expectation in RFC 7242 is that a single socket
	pair can be used for bidirectional bundle transmission
	between two nodes, but in ION's implementation of TCPCL
	this is currently not possible.	 Here's why.

	TCP connections are unidirectional.  The node accepting
	the connection does not know the socket name of the
	connecting node until the connection has been made, and
	in fact it doesn't know anything about the connecting
	node until after the exchange of contact headers.  This
	makes it impossible to manage forwarding (as ION does)
	by associating egress plans, linked to Outducts that
	cite convergence-layer endpoints, with neighboring
	nodes.  The convergence-layer endpoint (socket name)
	for a TCPCL Outduct may be unknown at the time the
	Outduct is established (because it only is known at
	connection time).  This is a problem because a socket
	name (IP address and port number) must be noted in
	some pre-established Outduct specifications in order
	for the local node to be the connecting node rather
	than the node that is connected to.  Overriding plan
	rules have the same problem.

	In the future it should be possible to design some sort
	of abstract TCPCL Outduct specification for "whatever
	TCP connection is made to node X, which you can try to
	connect to at IP address Y and port Z".  Such Outducts
	could be created in the course of forwarding management,
	and in the event of an unexpected connection from a
	previously unknown neighbor we could create such an
	Outduct dynamically at connection time.  Dynamically
	created Outducts would be useless without corresponding
	dynamically created egress plans, but neighbor discovery
	will create such plans (though without any overriding
	plan rules).  All of this dynamic creation of plans and
	Outducts constitutes wholly unmanaged forwarding, but
	that will work fine with opportunistic routing.

	For now, though, implementation of TCPCL to suit the
	needs of Space Station operations does not require
	this conceptual leap.  tcpcli will execute TCPCL in
	a compliant manner, just not in the manner anticipated
	by the designers of TCPCL.

	Author: Scott Burleigh, JPL

	Copyright (c) 2015, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
	
									*/
#include "bpP.h"
#include "ipnfw.h"
#include "dtn2fw.h"
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

#define	TCPCL_SEGMENT_ACKS	(1)
#define	TCPCL_REACTIVE		(0)
#define	TCPCL_REFUSALS		(0)
#define	TCPCL_LENGTH_MSGS	(0)

#define	TCPCL_PLANNED		(0)
#define	TCPCL_CHANCE		(1)

#ifdef	QUERY
#undef	QUERY
#endif
#define	QUERY			(-1)

/*	Each TCPCL-speaking node must be prepared to accept TCPCL
 *	connections from other nodes but must also initiate those
 *	connections according to egress plans as discussed above.
 *	Note that these two distinct connections - one asserted,
 *	the other accepted - cannot be considered two distinct
 *	nodes: in order to comply with RFC 7242 we must apply
 *	rules on reconnection and shutdown on a node basis rather
 *	than a connection basis.
 *
 *	Accordingly, the ION TCPCL adapter manages a list of
 *	TCPCL Neighbors (nodes), each of which may contain up to
 *	two distinct Connections, one Planned (asserted) and
 *	one Chance (accepted).  Each Connection has a single
 *	socket (TCP connection) and has a single reception thread
 *	that executes most of the TCPCL protocol.  The Planned
 *	connection additionally has a transmission thread that
 *	dequeues bundles from the Outduct to which the associated
 *	egress plan maps and transmits data segments.  When
 *	TCPCL data segments are acknowledged, the acknowledgments
 *	are applied to transmitted bundles in FIFO fashion;
 *	bundles awaiting acknowledgment are retained in a
 *	pipeline list, from which they can be relocated into
 *	the Limbo list in the event that link disruption is
 *	detected.  Insertion into the pipeline list is throttled,
 *	an additional flow control measure.				*/

typedef struct
{
	int			sock;
	pthread_mutex_t		mutex;
	int			hasMutex;	/*	Boolean.	*/
	pthread_t		receiver;
	int			hasReceiver;	/*	Boolean.	*/
	vast			lengthReceived;	/*	Current in.	*/
	int			secUntilShutdown;	/*	(idle)	*/
	int			keepaliveInterval;
	int			secUntilKeepalive;
	int			secSinceReception;
	int			timeoutCount;
	int			segmentAcks;	/*	Boolean		*/
	int			reactiveFrags;	/*	Boolean		*/
	int			bundleRefusals;	/*	Boolean		*/
	int			lengthMessages;	/*	Boolean		*/
	int			shutDown;	/*	Boolean		*/
	int			hasEnded;	/*	Boolean		*/
	struct tcpcl_neighbor	*neighbor;	/*	Back reference	*/

	/*	For planned connection only.				*/

	VOutduct		*outduct;
	pthread_t		sender;
	int			hasSender;	/*	Boolean.	*/
	Lyst			pipeline;	/*	All outbound.	*/
	struct llcv_str		throttleLlcv;
	Llcv			throttle;	/*	On pipeline.	*/
	int			hasThrottle;	/*	Boolean.	*/
	uvast			lengthSent;	/*	Oldest out.	*/
	uvast			lengthAcked;	/*	Oldest out.	*/
	int			reconnectInterval;
	int			secUntilReconnect;
	int			newlyAdded;	/*	Boolean.	*/
} TcpclConnection;

typedef enum
{
	ThreadStopped = 0,
	ThreadRunning = 1
} ThreadState;

typedef struct tcpcl_neighbor
{
	char			*eid;		/*	Remote node ID.	*/
	char			*destDuctName;	/*	From directive.	*/
	VInduct			*induct;	/*	(Shared.)	*/
	int			connecting;	/*	Boolean		*/
	pthread_t		connectThread;
	ThreadState		connectThreadState;
	int			disconnecting;	/*	Boolean		*/
	pthread_t		cleanupThread;
	ThreadState		cleanupThreadState;
	int			mustDelete;	/*	Boolean		*/
	TcpclConnection		connections[2];
	TcpclConnection		*pc;		/*	Planned.	*/
	TcpclConnection		*cc;		/*	Chance.		*/
} TcpclNeighbor;

static void	deleteTcpclNeighbor(TcpclNeighbor *neighbor);
static void	stopSenderThread(TcpclConnection *connection);
static void	*handleContacts(void *parm);

static char	*procName()
{
	return "tcpcli";
}

#ifndef mingw
static void	handleStopThread()
{
	isignal(SIGINT, handleStopThread);
}
#endif
static void	handleStopTcpcli()
{
	isignal(SIGTERM, handleStopTcpcli);
	ionKillMainThread(procName());
}

/*	*	*	Utility functions	*	*	*	*/

static int	receiveSdnv(TcpclConnection *p, uvast *val)
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
			 *	of connection.				*/

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
						p->neighbor->eid);
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

/*	*	Connection and neighbor management functions	*	*/

typedef struct
{
	Lyst		neighbors;
	TcpclConnection	*connection;
	char		*buffer;
	AcqWorkArea	*work;
	ReqAttendant	attendant;
} ReceiverThreadParms;

typedef struct
{
	TcpclConnection	*connection;
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
		if (strcmp(neighbor->eid, eid) == 0)
		{
			return elt;
		}
	}

	return NULL;
}

static LystElt	addTcpclNeighbor(char *eid, VInduct *induct, Lyst neighbors)
{
	TcpclNeighbor	*neighbor;
	size_t		len;
	TcpclConnection	*connection;
	LystElt		elt;
	int		i;

	neighbor = (TcpclNeighbor *) MTAKE(sizeof(TcpclNeighbor));
	if (neighbor == NULL)
	{
		putErrmsg("tcpcli can't allocate new neighbor.", eid);
		return NULL;
	}

	len = istrlen(eid, MAX_EID_LEN) + 1;
	if (len == 0 || (neighbor->eid = MTAKE(len)) == NULL)
	{
		MRELEASE(neighbor);
		putErrmsg("tcpcli can't allocate new neighbor EID.", eid);
		return NULL;
	}

	oK(istrcpy(neighbor->eid, eid, len));
	neighbor->induct = induct;
	neighbor->pc = &(neighbor->connections[0]);
	neighbor->cc = &(neighbor->connections[1]);
	for (i = 0; i < 2; i++)
	{
		connection = &(neighbor->connections[i]);
		connection->sock = -1;
		connection->neighbor = neighbor;
		connection->secUntilShutdown = -1;
		connection->secUntilKeepalive = -1;
	}

	elt = lyst_insert_last(neighbors, (void *) neighbor);
	if (elt == NULL)
	{
		MRELEASE(neighbor->eid);
		MRELEASE(neighbor);
		putErrmsg("tcpcli can't insert new neighbor into list.", eid);
		return NULL;
	}

	return elt;
}

static void	cancelXmit(LystElt elt, void *userdata)
{
	Sdr	sdr = getIonsdr();
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
	else
	{
		oK(sdr_begin_xn(sdr));
		zco_destroy(sdr, bundleZco);
		if (sdr_end_xn(sdr) < 0)
		{
			putErrmsg("tcpcli can't destroy ZCO.", NULL);
			ionKillMainThread(procName());
		}
	}
}

static int	beginConnection(LystElt neighborElt, int newSocket,
			int fromAccept)
{
	TcpclNeighbor		*neighbor = NULL;
	TcpclConnection		*connection;
	int			newNeighbor = 0;
	ReceiverThreadParms	*rtp;

	neighbor = (TcpclNeighbor *) lyst_data(neighborElt);
	if (fromAccept)			/*	accept() succeeded.	*/
	{
		connection = neighbor->cc;
		newNeighbor = 1;
	}
	else				/*	connect() succeeded.	*/
	{
		connection = neighbor->pc;

		/*	Neighbor may have already been created upon
		 *	acceptance of a connection.			*/

		if (neighbor->cc->sock == -1)
		{
			newNeighbor = 1;
		}
	}

	connection->sock = newSocket;
	connection->keepaliveInterval = 0;
	connection->secUntilKeepalive = -1;
	connection->secUntilShutdown = -1;
	rtp = (ReceiverThreadParms *) MTAKE(sizeof(ReceiverThreadParms));
	if (rtp == NULL)
	{
		if (newNeighbor)
		{
			deleteTcpclNeighbor(neighbor);
			lyst_delete(neighborElt);
		}

		putErrmsg("tcpcli can't allocate new receiver parms.", NULL);
		closesocket(newSocket);
		return -1;
	}

	rtp->neighbors = lyst_lyst(neighborElt);
	rtp->connection = connection;
	pthread_mutex_init(&(connection->mutex), NULL);
	connection->hasMutex = 1;
	pthread_mutex_lock(&(connection->mutex));
	if (pthread_begin(&(connection->receiver), NULL, handleContacts, rtp))
	{
		MRELEASE(rtp);
		pthread_mutex_unlock(&(connection->mutex));
		pthread_mutex_destroy(&(connection->mutex));
		connection->hasMutex = 0;
		if (newNeighbor)
		{
			deleteTcpclNeighbor(neighbor);
			lyst_delete(neighborElt);
		}

		putSysErrmsg("tcpcli can't create new receiver thread", NULL);
		closesocket(newSocket);
		return -1;
	}

	connection->hasReceiver = 1;
	if (fromAccept)			/*	Connection by accept().	*/
	{
		/*	Let receiver thread start running.		*/

		pthread_mutex_unlock(&(connection->mutex));
		return 0;		/*	Nothing more to do.	*/
	}

	/*	This is a planned connection, so bundle transmission
	 *	and reconnection are possible.				*/

	connection->newlyAdded = 1;
	connection->reconnectInterval = 1;
	connection->secUntilReconnect = -1;
	connection->throttle = &(connection->throttleLlcv);
	connection->pipeline = lyst_create_using(getIonMemoryMgr());
	if (connection->pipeline == NULL)
	{
		connection->shutDown = 1;
		pthread_mutex_unlock(&(connection->mutex));
		pthread_join(connection->receiver, NULL);
		MRELEASE(rtp);
		pthread_mutex_destroy(&(connection->mutex));
		connection->hasMutex = 0;
		if (newNeighbor)
		{
			deleteTcpclNeighbor(neighbor);
			lyst_delete(neighborElt);
		}

		putErrmsg("tcpcli can't create pipeline list.", NULL);
		closesocket(newSocket);
		return -1;
	}

	lyst_delete_set(connection->pipeline, cancelXmit, NULL);
	if (llcv_open(connection->pipeline, connection->throttle) == NULL)
	{
		lyst_destroy(connection->pipeline);
		connection->shutDown = 1;
		pthread_mutex_unlock(&(connection->mutex));
		pthread_join(connection->receiver, NULL);
		MRELEASE(rtp);
		pthread_mutex_destroy(&(connection->mutex));
		connection->hasMutex = 0;
		if (newNeighbor)
		{
			deleteTcpclNeighbor(neighbor);
			lyst_delete(neighborElt);
		}

		putErrmsg("tcpcli can't open pipeline.", NULL);
		closesocket(newSocket);
		return -1;
	}

	connection->hasThrottle = 1;

	/*	Let receiver thread start running.			*/

	pthread_mutex_unlock(&(connection->mutex));
	return 0;
}

static int	reopenConnection(TcpclConnection *connection)
{
	TcpclNeighbor	*neighbor = connection->neighbor;

	if (connection->secUntilReconnect != 0)
	{
		return 0;	/*	Must not reconnect yet.		*/
	}

	/*	Okay to make next connection attempt.			*/

	switch (itcp_connect(neighbor->destDuctName, BpTcpDefaultPortNbr,
			&(connection->sock)))
	{
	case -1:		/*	System failure.			*/
		putErrmsg("tcpcli failed on TCP reconnect.", neighbor->eid);
		return -1;

	case 0:			/*	Neighbor still refuses.		*/
		writeMemoNote("[i] tcpcli unable to reconnect",
				neighbor->eid);
		connection->reconnectInterval <<= 1;
		if (connection->reconnectInterval > MAX_RECONNECT_INTERVAL)
		{
			connection->reconnectInterval = MAX_RECONNECT_INTERVAL;
		}

		connection->secUntilReconnect = connection->reconnectInterval;
		return 0;
	}

	/*	Reconnection succeeded.					*/

	if (bpUnblockOutduct("tcp", neighbor->destDuctName) < 0)
	{
		putErrmsg("tcpcli connected but didn't unblock outduct.",
				neighbor->destDuctName);
		return -1;
	}

	connection->lengthSent = 0;
	connection->lengthAcked = 0;
	return 1;		/*	Reconnected.			*/
}

static void	closeConnection(TcpclConnection *connection)
{
	Sdr		sdr = getIonsdr();
	TcpclNeighbor	*neighbor;
	VOutduct	*vduct;
	PsmAddress	vductElt;

	if (connection->sock != -1)
	{
		closesocket(connection->sock);
		connection->sock = -1;
	}

	connection->secUntilKeepalive = -1;
	if (connection->hasThrottle)	/*	Planned connection.	*/
	{
		neighbor = connection->neighbor;
		findOutduct("tcp", neighbor->destDuctName, &vduct, &vductElt);
		if (vductElt)
		{
			if (bpBlockOutduct("tcp", neighbor->destDuctName) < 0)
			{
				ionKillMainThread(procName());
				return;
			}
		}

		oK(sdr_begin_xn(sdr));
		lyst_clear(connection->pipeline);
		oK(sdr_end_xn(sdr));
		if (connection->reconnectInterval == 0)
		{
			/*	Never reconnect.			*/

			connection->secUntilReconnect = -1;
		}
		else
		{
			connection->secUntilReconnect =
					connection->reconnectInterval;
		}
	}
}

static int	sendShutdown(TcpclConnection *connection, char reason,
			int doNotReconnect)
{
	char	shutdown[3];
	int	len = 1;
	int	result;

	if (connection->sock == -1)	/*	Nothing to send.	*/
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

	pthread_mutex_lock(&(connection->mutex));
	result = itcp_send(&connection->sock, shutdown, len);
	pthread_mutex_unlock(&(connection->mutex));
	return result;
}

static void	eraseConnection(TcpclConnection *connection)
{
	TcpclNeighbor	*neighbor = connection->neighbor;

	stopSenderThread(connection);
	if (connection->hasReceiver)
	{
#ifdef mingw
		shutdown(connection->sock, SD_BOTH);
#else
		pthread_kill(connection->receiver, SIGINT);
#endif
		pthread_join(connection->receiver, NULL);
	}

	closeConnection(connection);
	if (connection->pipeline)
	{
		lyst_destroy(connection->pipeline);
	}

	if (connection->hasThrottle)
	{
		llcv_close(connection->throttle);
	}

	if (connection->hasMutex)
	{
		pthread_mutex_destroy(&(connection->mutex));
	}

	memset(connection, 0, sizeof(TcpclConnection));
	connection->sock = -1;
	connection->neighbor = neighbor;
	connection->secUntilShutdown = -1;
	connection->secUntilKeepalive = -1;
}

static void	endConnection(TcpclConnection *connection, char reason)
{
	oK(sendShutdown(connection, reason, 0));
	connection->shutDown = 1;
	eraseConnection(connection);
}

static void	deleteTcpclNeighbor(TcpclNeighbor *neighbor)
{
	endConnection(neighbor->pc, -1);
	endConnection(neighbor->cc, -1);
	if (neighbor->eid)
	{
		MRELEASE(neighbor->eid);
	}

	if (neighbor->destDuctName)
	{
		MRELEASE(neighbor->destDuctName);
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

	oK(sdr_end_xn(sdr));
}

/*	*	*	Sender thread functions		*	*	*/

static int pipeline_not_full(Llcv llcv)
{
	return (lyst_length(llcv->list) < MAX_PIPELINE_LENGTH ? 1 : 0);
}

static int	sendBundleByTcpcl(SenderThreadParms *stp, Object bundleZco)
{
	Sdr		sdr = getIonsdr();
	TcpclConnection	*connection = stp->connection;
	TcpclNeighbor	*neighbor = connection->neighbor;
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

	if (connection->sock == -1)
	{
		return 0;	/*	Connection has been lost.	*/
	}

	if (llcv_wait(connection->throttle, pipeline_not_full, LLCV_BLOCKING))
	{
		putErrmsg("Wait on TCPCL pipeline throttle condition failed.",
				neighbor->eid);
		return -1;
	}

	pthread_mutex_lock(&(connection->mutex));
	elt = lyst_insert_last(connection->pipeline, (void *) bundleZco);
	pthread_mutex_unlock(&(connection->mutex));
	if (elt == NULL)
	{
		putErrmsg("Can't append transmitted ZCO to tcpcli pipeline.",
				neighbor->eid);
		return -1;
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
			putErrmsg("Incomplete zco_transmit.", neighbor->eid);
			return -1;
		}

		firstByte = 0x10 | flags;
		segHeader[0] = firstByte;
		encodeSdnv(&segLengthSdnv, bytesToLoad);
		memcpy(segHeader + 1, segLengthSdnv.text, segLengthSdnv.length);
		segHeaderLen = 1 + segLengthSdnv.length;
		pthread_mutex_lock(&(connection->mutex));
		if (itcp_send(&connection->sock, segHeader, segHeaderLen) < 1)
		{
			pthread_mutex_unlock(&(connection->mutex));
			writeMemoNote("[?] tcpcl connection lost (seg header)",
					neighbor->eid);
			return 0;
		}

		if (itcp_send(&connection->sock, stp->buffer, bytesToSend) < 1)
		{
			pthread_mutex_unlock(&(connection->mutex));
			writeMemoNote("[?] tcpcl connection lost (seg content)",
					neighbor->eid);
			return 0;
		}

		pthread_mutex_unlock(&(connection->mutex));
		flags = 0x00;			/*	No longer 1st.	*/
		bytesRemaining -= bytesToSend;
	}

	return 1;	/*	Bundle was successfully sent.		*/
}

static int	sendOneBundle(SenderThreadParms *stp)
{
	unsigned int	maxPayloadLength;
	TcpclConnection	*connection = stp->connection;
	TcpclNeighbor	*neighbor = connection->neighbor;
	Object		bundleZco;
	BpExtendedCOS	extendedCOS;
	char		destDuctName[SDRSTRING_BUFSZ];

	while (1)
	{
		if (sm_SemEnded(connection->outduct->semaphore))
		{
			writeMemoNote("[i] tcpcli outduct stopped",
					neighbor->destDuctName);
			return 0;	/*	Time to give up.	*/
		}

		/*	Loop until max payload length is known.		*/

		if (!(maxPayloadLengthKnown(connection->outduct,
				&maxPayloadLength)))
		{
			snooze(1);
			continue;
		}

		/*	Get the next bundle to send.			*/

		if (bpDequeue(connection->outduct, stp->outflows, &bundleZco,
			&extendedCOS, destDuctName, maxPayloadLength, -1) < 0)
		{
			putErrmsg("Can't dequeue bundle.", NULL);
			return -1;
		}

		if (bundleZco == 0)	/*	Outduct stopped.	*/
		{
			writeMemoNote("[i] tcpcli outduct stopped",
					neighbor->destDuctName);
			return 0;
		}

		/*	Send that bundle.				*/

		return sendBundleByTcpcl(stp, bundleZco);
	}
}

static void	*sendBundles(void *parm)
{
	SenderThreadParms	*stp = (SenderThreadParms *) parm;
	TcpclConnection		*connection = stp->connection;
	TcpclNeighbor		*neighbor = connection->neighbor;
	Sdr			sdr = getIonsdr();
	Outduct			duct;
	int			i;

	/*	Load other required sender thread parms.		*/

	connection->newlyAdded = 0;
	stp->buffer = MTAKE(TCPCL_BUFSZ);
	if (stp->buffer == NULL)
	{
		putErrmsg("No memory for TCP buffer.", neighbor->eid);
		ionKillMainThread(procName());
		return NULL;
	}

	sdr_read(sdr, (char *) &duct, sdr_list_data(sdr,
			connection->outduct->outductElt), sizeof(Outduct));
	memset((char *) (stp->outflows), 0, sizeof stp->outflows);
	stp->outflows[0].outboundBundles = duct.bulkQueue;
	stp->outflows[1].outboundBundles = duct.stdQueue;
	stp->outflows[2].outboundBundles = duct.urgentQueue;
	for (i = 0; i < 3; i++)
	{
		stp->outflows[i].svcFactor = 1 << i;
	}

	/*	Ready to start sending bundles.				*/

	while (connection->sock != -1)
	{
		switch (sendOneBundle(stp))
		{
		case -1:		/*	System failure.		*/
			putErrmsg("tcpcli failed sending bundle.",
					neighbor->eid);
			ionKillMainThread(procName());

			/*	Intentional fall-through to next case.	*/

		case 0:			/*	Protocol failure.	*/
			closeConnection(connection);
			continue;

		case 1:	/*	Successful transmission.		*/
			/*	Reset keepalive countdown.		*/

			if (connection->keepaliveInterval > 0)
			{
				connection->secUntilKeepalive =
						connection->keepaliveInterval;
			}
		}
	}

	writeErrmsgMemos();
	writeMemoNote("[i] tcpcli sender thread has ended", neighbor->eid);
	MRELEASE(stp->buffer);
	MRELEASE(stp);
	return NULL;
}

static void	stopSenderThread(TcpclConnection *connection)
{
	if (!(connection->hasSender))
	{
		return;
	}

	/*	Enable sendOneBundle to exit.				*/

	sm_SemEnd(connection->outduct->semaphore);

	/*	Enable sendBundleByTcpcl to exit.			*/

	llcv_signal(connection->throttle, pipeline_not_full);

	/*	Signal thread in case it's not already stopping.	*/

#ifdef mingw
	shutdown(connection->sock, SD_BOTH);
#else
	pthread_kill(connection->sender, SIGINT);
#endif

	/*	Forget connection's sender.				*/

	pthread_join(connection->sender, NULL);
	connection->hasSender = 0;
}

/*	*	*	Receiver thread functions	*	*	*/

static int	sendContactHeader(TcpclConnection *connection)
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

	/*	connection->sock is known to be a connected socket.	*/

	pthread_mutex_lock(&(connection->mutex));
	result = itcp_send(&connection->sock, contactHeader, len);
	pthread_mutex_unlock(&(connection->mutex));
	return result;
}

static int	receiveContactHeader(ReceiverThreadParms *rtp)
{
	Sdr			sdr = getIonsdr();
	TcpclConnection		*connection = rtp->connection;
	TcpclNeighbor		*neighbor = connection->neighbor;
	unsigned char		header[8];
	unsigned short		keepaliveInterval;
	uvast			eidLength;
	char			*eidbuf;
	LystElt			elt;
	TcpclNeighbor		*knownNeighbor;
	int			result;
	VOutduct		*outduct;
	PsmAddress		vductElt;
	SenderThreadParms	*stp;

	if (itcp_recv(&connection->sock, (char *) header, sizeof header) < 1)
	{
		putErrmsg("Can't get TCPCL contact header.", neighbor->eid);
		return 0;
	}

	if (memcmp(header, "dtn!", 4) != 0)
	{
		writeMemoNote("[?] Invalid TCPCL contact header",
				neighbor->eid);
		return 0;
	}

	if (header[4] < 3)		/*	Version mismatch.	*/
	{
		oK(sendShutdown(connection, 0x01, 0));
		writeMemoNote("[?] Bad version number in TCPCL contact header",
				neighbor->eid);
		neighbor->mustDelete = 1;
		return 0;
	}

	/*	6th byte of header is flags.				*/

	connection->segmentAcks = TCPCL_SEGMENT_ACKS && (header[5] & 0x01);
	connection->reactiveFrags = TCPCL_REACTIVE && (header[5] & 0x02);
	connection->bundleRefusals = (TCPCL_REFUSALS && (header[5] & 0x04))
			&& connection->segmentAcks;
	connection->lengthMessages = TCPCL_LENGTH_MSGS && (header[5] & 0x08);

	/*	7th-8th bytes of header are keepalive interval.		*/

	memcpy((char *) &keepaliveInterval, (char *) (header + 6), 2);
	keepaliveInterval = ntohs(keepaliveInterval);
	if (keepaliveInterval < KEEPALIVE_INTERVAL)
	{
		connection->keepaliveInterval = keepaliveInterval;
	}
	else
	{
		connection->keepaliveInterval = KEEPALIVE_INTERVAL;
	}

	if (connection->keepaliveInterval == 0)
	{
		connection->secUntilKeepalive = -1;	/*	None.	*/
	}
	else
	{
		connection->secUntilKeepalive = connection->keepaliveInterval;
	}

	/*	Next is the neighboring node's ID, an endpoint ID.	*/

	if (receiveSdnv(connection, &eidLength) < 1)
	{
		putErrmsg("Can't get EID length in TCPCL contact header",
				neighbor->eid);
		return 0;
	}

	eidbuf = MTAKE(eidLength + 1);
	if (eidbuf == NULL)
	{
		putErrmsg("Not enough memory for EID.", neighbor->eid);
		return -1;
	}

	if (itcp_recv(&connection->sock, eidbuf, eidLength) < 1)
	{
		MRELEASE(eidbuf);
		putErrmsg("Can't get TCPCL contact header EID.", neighbor->eid);
		return 0;
	}

	eidbuf[eidLength] = '\0';
	if (connection->hasThrottle == 0)	/*	From accept().	*/
	{
		/*	If a TcpclNeighbor already exists whose eid
		 *	is equal to eidbuf:
		 *	-	If that neighbor already has a
		 *		connection from accept(), then
		 *		shutdown this connection.
		 *	-	Else, copy this connection into
		 *		that neighbor.
		 *	-	In either case, delete the tentative
		 *		TcpclNeighbor.
		 *	Otherwise, update the eid from eidbuf.		*/

		CHKERR(sdr_begin_xn(sdr));	/*	Lock list.	*/
		elt = findNeighborForEid(rtp->neighbors, eidbuf);
		if (elt)
		{
			knownNeighbor = (TcpclNeighbor *) lyst_data(elt);
			sdr_exit_xn(sdr);	/*	Unlock list.	*/
			MRELEASE(eidbuf);	/*	Not needed.	*/
			if (knownNeighbor->cc->sock != -1)
			{
				result = 0;	/*	Rejected.	*/
				oK(sendShutdown(connection, 0x02, 0));
			}
			else	/*	Complementary connection.	*/
			{
				/*	Copy current connection into
				 *	known neighbor.			*/

				result = 1;	/*	Accepted.	*/
				memcpy((char *) (knownNeighbor->cc),
						(char *) connection,
						sizeof(TcpclConnection));
				knownNeighbor->cc->neighbor = knownNeighbor;
				rtp->connection = knownNeighbor->cc;

				/*	Make sure deletion of the
				 *	tentative neighbor doesn't
				 *	affect the connection.		*/

				connection->sock = -1;
				connection->hasReceiver = 0;
				connection->hasMutex = 0;
				connection->hasSender = 0;
				connection->hasThrottle = 0;
			}

			/*	In any case, tentative neighbor is
			 *	no longer needed.			*/

			neighbor->mustDelete = 1;
			return result;
		}

		/*	This is a new neighbor; just note its EID.	*/

		MRELEASE(neighbor->eid);
		neighbor->eid = eidbuf;
		sdr_exit_xn(sdr);	/*	Unlock list.		*/
		return 1;		/*	Nothing more to do.	*/
	}

	/*	Connection is the result of a successful connect().	*/

	if (strcmp(eidbuf, neighbor->eid) != 0)
	{
		/*	The node that we have connect()ed to is not
		 *	the one that we thought it was.			*/

		writeMemoNote("[?] expected tcpcl EID", neighbor->eid);
		writeMemoNote("[?] received tcpcl EID", eidbuf);
		MRELEASE(eidbuf);
		oK(sendShutdown(connection, 0x02, 0));
		neighbor->mustDelete = 1;
		return 0;
	}

	MRELEASE(eidbuf);			/*	Not needed.	*/

	/*	This is a connection over which bundles may be
	 *	transmitted, so we must start a sender thread for
	 *	the outduct that we use for those bundles.		*/

	findOutduct("tcp", neighbor->destDuctName, &outduct, &vductElt);
	if (vductElt == 0)
	{
		putErrmsg("tcpli can't find outduct.", neighbor->destDuctName);
		return -1;
	}

	connection->outduct = outduct;
	sm_SemUnend(connection->outduct->semaphore);
	sm_SemGive(connection->outduct->semaphore);
	stp = (SenderThreadParms *) MTAKE(sizeof(SenderThreadParms));
	if (stp == NULL)
	{
		putErrmsg("tcpcli can't allocate space for sender parms.", 
				neighbor->eid);
		return -1;
	}

	stp->connection = connection;
	if (pthread_begin(&(connection->sender), NULL, sendBundles, stp))
	{
		MRELEASE(stp);
		putSysErrmsg("tcpcli can't create new sender thread", 
				neighbor->eid);
		return -1;
	}

	connection->hasSender = 1;
	return 1;
}

static int	sendAck(TcpclConnection *connection)
{
	char	ack[11];
	Sdnv	ackLengthSdnv;
	int	len;
	int	result;

	ack[0] = 0x20;
	encodeSdnv(&ackLengthSdnv, connection->lengthReceived);
	memcpy(ack + 1, ackLengthSdnv.text, ackLengthSdnv.length);
	len = 1 + ackLengthSdnv.length;
	pthread_mutex_lock(&(connection->mutex));
	result = itcp_send(&connection->sock, ack, len);
	pthread_mutex_unlock(&(connection->mutex));
	return result;
}

static int	handleDataSegment(ReceiverThreadParms *rtp,
			unsigned char msgtypeByte)
{
	TcpclConnection	*connection = rtp->connection;
	TcpclNeighbor	*neighbor = connection->neighbor;
	int		result;
	uvast		dataLength;
	uvast		bytesRemaining;
	int		bytesToRead;
	int		extentSize;

	result = receiveSdnv(connection, &dataLength);
	if (result < 1)
	{
		return result;
	}

	if (dataLength == 0)		/*	Nuisance data segment.	*/
	{
		connection->secSinceReception = 0;
		connection->timeoutCount = 0;
		return 1;		/*	Ignore.			*/
	}

	if (msgtypeByte & 0x02)		/*	Start of bundle.	*/
	{
		if (connection->lengthReceived > 0)
		{
			/*	Discard partially received bundle.	*/

			bpCancelAcq(rtp->work);
			connection->lengthReceived = 0;
		}

		if (bpBeginAcq(rtp->work, 0, NULL) < 0)
		{
			return -1;
		}
	}

	bytesRemaining = dataLength;
	while (bytesRemaining > 0)
	{
		bytesToRead = bytesRemaining;
		if (bytesToRead > TCPCL_BUFSZ)
		{
			bytesToRead = TCPCL_BUFSZ;
		}
 
		extentSize = itcp_recv(&connection->sock, rtp->buffer,
				bytesToRead);
		if (extentSize < 1)
		{
			writeMemoNote("[?] Lost TCPCL neighbor", neighbor->eid);
			return 0;
		}

		if (bpContinueAcq(rtp->work, rtp->buffer, extentSize,
				&(rtp->attendant)) < 0)
		{
			return -1;
		}

		bytesRemaining -= extentSize;
		connection->lengthReceived += extentSize;
		if (connection->segmentAcks)
		{
			result = sendAck(connection);
			if (result < 1)
			{
				return result;
			}
		}
	}

	if (msgtypeByte & 0x01)		/*	End of bundle.		*/
	{
		if (bpEndAcq(rtp->work) < 0)
		{
			return -1;
		}

		connection->lengthReceived = 0;
	}

	if (connection->secUntilShutdown != -1)
	{
		/*	Will end the connection when incoming bundles
		 *	stop, but they are still arriving.		*/

		connection->secUntilShutdown = IDLE_SHUTDOWN_INTERVAL;
	}

	connection->secSinceReception = 0;
	connection->timeoutCount = 0;
	return 1;
}

static int	handleAck(ReceiverThreadParms *rtp, unsigned char msgtypeByte)
{
	TcpclConnection	*connection = rtp->connection;
	Sdr		sdr = getIonsdr();
	int		result;
	uvast		lengthAcked;
	LystElt		elt;
	Object		bundleZco = 0;

	result = receiveSdnv(connection, &lengthAcked);
	if (result < 1)
	{
		return result;
	}

	if (lengthAcked == 0)		/*	Nuisance ack.		*/
	{
		return 1;		/*	Ignore it.		*/
	}

	if (connection->hasThrottle == 0)
	{
		/*	This is a chance connection, created by
		 *	accepting connect() from the neighbor node.
		 *	Because it's not associated with any Plan,
		 *	it can't have had bundles enqueued to it
		 *	for transmission; therefore this Ack segment
		 *	must be bogus.					*/

		return 1;		/*	Ignore it.		*/
	}

	oK(sdr_begin_xn(sdr));
	if (connection->lengthSent == 0)
	{
		/*	Must get the oldest bundle, to which this ack
		 *	pertains.					*/

		pthread_mutex_lock(&(connection->mutex));
		elt = lyst_first(connection->pipeline);
		if (elt)
		{
			bundleZco = (Object) lyst_data(elt);
		}

		pthread_mutex_unlock(&(connection->mutex));
		if (bundleZco == 0)
		{
			/*	Nothing to acknowledge.			*/

			sdr_exit_xn(sdr);
			return 1;	/*	Ignore acknowledgment.	*/
		}

		/*	Initialize for ack reception.			*/

		connection->lengthSent = zco_length(sdr, bundleZco);
		connection->lengthAcked = 0;
	}

	if (lengthAcked <= connection->lengthAcked
	|| lengthAcked > connection->lengthSent)
	{
		/*	Acknowledgment sequence is violated, so 
		 *	didn't ack the end of the oldest bundle.	*/

		pthread_mutex_lock(&(connection->mutex));
		elt = lyst_first(connection->pipeline);
		if (elt)
		{
			bundleZco = (Object) lyst_data_set(elt, NULL);
			lyst_delete(elt);
		}

		pthread_mutex_unlock(&(connection->mutex));
		llcv_signal(connection->throttle, pipeline_not_full);
		if (bundleZco == 0 || bpHandleXmitFailure(bundleZco) < 0)
		{
			if (sdr_end_xn(sdr) < 0)
			{
				putErrmsg("Can't clear oldest bundle.", NULL);
			}

			return -1;
		}
	}
	else	/*	Acknowledgments are ascending.			*/
	{
		connection->lengthAcked = lengthAcked;
		if (connection->lengthAcked < connection->lengthSent)
		{
			sdr_exit_xn(sdr);
			return 1;	/*	Not fully acked yet.	*/
		}

		/*	Entire bundle has been received.		*/

		pthread_mutex_lock(&(connection->mutex));
		elt = lyst_first(connection->pipeline);
		if (elt)
		{
			bundleZco = (Object) lyst_data_set(elt, NULL);
			lyst_delete(elt);
		}

		pthread_mutex_unlock(&(connection->mutex));
		llcv_signal(connection->throttle, pipeline_not_full);
		if (bundleZco == 0 || bpHandleXmitSuccess(bundleZco, 0) < 0)
		{
			if (sdr_end_xn(sdr) < 0)
			{
				putErrmsg("Can't clear oldest bundle.", NULL);
			}

			return -1;
		}
	}

	/*	Destroy bundle, unless there's stewardship or custody.	*/

	zco_destroy(sdr, bundleZco);
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't destroy bundle ZCO.", NULL);
		return -1;
	}

	connection->lengthSent = 0;
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
	TcpclConnection	*connection = rtp->connection;

	connection->secSinceReception = 0;
	connection->timeoutCount = 0;
	return 1;
}

static int	handleShutdown(ReceiverThreadParms *rtp,
			unsigned char msgtypeByte)
{
	TcpclConnection	*connection = rtp->connection;
	TcpclNeighbor	*neighbor = connection->neighbor;
	int		result;
	unsigned char	reasonCode;
	uvast		reconnectInterval;

	if (msgtypeByte & 0x02)
	{
		result = irecv(connection->sock, (char *) &reasonCode, 1, 0);
		switch (result)
		{
		case -1:
			if (errno != EINTR)	/*	(Shutdown)	*/
			{
				putSysErrmsg("irecv() error on TCP socket",
						neighbor->eid);
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
		result = receiveSdnv(connection, &reconnectInterval);
		if (result < 1)
		{
			return result;
		}

		connection->reconnectInterval = reconnectInterval;
		if (connection->reconnectInterval == 0)
		{
			neighbor->mustDelete = 1;
		}
	}

	return 0;			/*	Abandon connection.	*/
}

static int	handleLength(ReceiverThreadParms *rtp,
			unsigned char msgtypeByte)
{
	int	result;
	uvast	bundleLength;

	result = receiveSdnv(rtp->connection, &bundleLength);
	if (result < 1)
	{
		return result;
	}

	return 1;			/*	LENGTH is ignored.	*/
}

static int	handleMessages(ReceiverThreadParms *rtp)
{
	TcpclConnection		*connection = rtp->connection;
	TcpclNeighbor		*neighbor = connection->neighbor;
	unsigned char		msgtypeByte;
	int			msgType;

	while (1)
	{
		switch (irecv(connection->sock, (char *) &msgtypeByte, 1, 0))
		{
		case -1:
			if (errno != EINTR)	/*	Not shutdown.	*/
			{
				putSysErrmsg("irecv() error on TCP socket",
						neighbor->eid);
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
						neighbor->eid);
				return -1;

			case 0:		/*	Connection closed.	*/
				return 0;
			}

			continue;

		case 0x02:
			switch (handleAck(rtp, msgtypeByte))
			{
			case -1:
				putErrmsg("tcpcli segment handling error.",
						neighbor->eid);
				return -1;

			case 0:		/*	Connection closed.	*/
				return 0;
			}

			continue;

		case 0x03:
			switch (handleRefusal(rtp, msgtypeByte))
			{
			case -1:
				putErrmsg("tcpcli segment handling error.",
						neighbor->eid);
				return -1;

			case 0:		/*	Connection closed.	*/
				return 0;
			}

			continue;

		case 0x04:
			switch (handleKeepalive(rtp, msgtypeByte))
			{
			case -1:
				putErrmsg("tcpcli segment handling error.",
						neighbor->eid);
				return -1;

			case 0:		/*	Connection closed.	*/
				return 0;
			}

			continue;

		case 0x05:
			switch (handleShutdown(rtp, msgtypeByte))
			{
			case -1:
				putErrmsg("tcpcli segment handling error.",
						neighbor->eid);
				return -1;

			case 0:		/*	Connection closed.	*/
				return 0;
			}

			continue;

		case 0x06:
			switch (handleLength(rtp, msgtypeByte))
			{
			case -1:
				putErrmsg("tcpcli segment handling error.",
						neighbor->eid);
				return -1;

			case 0:		/*	Connection closed.	*/
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
	TcpclConnection		*connection = rtp->connection;
	TcpclNeighbor		*neighbor = connection->neighbor;
	char			ownEid[MAX_EID_LEN];
	int			result;

	isprintf(ownEid, sizeof ownEid, "ipn:" UVAST_FIELDSPEC ".0",
			getOwnNodeNbr());

	/*	Wait for okay from beginConnection.			*/

	pthread_mutex_lock(&(connection->mutex));
	result = connection->shutDown;
	pthread_mutex_unlock(&(connection->mutex));
	if (result)
	{
		return NULL;
	}

	/*	Load other required receiver thread parms.		*/

	rtp->work = bpGetAcqArea(neighbor->induct);
	if (rtp->work == NULL)
	{
		putErrmsg("tcpcli can't get acquisition work area.",
				neighbor->eid);
		ionKillMainThread(procName());
		return NULL;
	}

	rtp->buffer = MTAKE(TCPCL_BUFSZ);
	if (rtp->buffer == NULL)
	{
		putErrmsg("No memory for TCP buffer.", neighbor->eid);
		ionKillMainThread(procName());
		return NULL;
	}

	if (ionStartAttendant(&(rtp->attendant)) < 0)
	{
		putErrmsg("Can't initialize blocking TCP reception.",
				neighbor->eid);
		ionKillMainThread(procName());
		return NULL;
	}

	/*	Now loop through possibly multiple contact episodes.	*/

	while (!(connection->shutDown))
	{
		if (connection->sock == -1)
		{
			if (connection->hasThrottle == 0)
			{
				break;	/*	Only peer can reopen.	*/
			}

			/*	Connection can be re-established.	*/

			switch (reopenConnection(connection))
			{
			case -1:
				ionKillMainThread(procName());
				connection->shutDown = 1;
				continue;

			case 0:			/*	No reconnect.	*/
				snooze(1);
				continue;

			default:		/*	Reconnected.	*/
				connection->reconnectInterval = 1;
				connection->secUntilReconnect = -1;
			}
		}

		/*	Connection is known to be established, so
		 *	now exchange contact headers.			*/

		if (sendContactHeader(connection) < 1)
		{
			writeMemoNote("[i] tcpcli did not send contact header",
					neighbor->eid);
			closeConnection(connection);
			continue;	/*	Try again.		*/
		}

		result = receiveContactHeader(rtp);

		/*	Contact header reception may have attached
		 *	this receiver thread to a previously
		 *	established neighbor.  Also may have started
		 *	a sender thread.				*/

		connection = rtp->connection;
		neighbor = connection->neighbor;
		switch (result)
		{
		case -1:		/*	System failure.		*/
			putErrmsg("Failure receiving contact header", NULL);
			stopSenderThread(connection);
			ionKillMainThread(procName());
			connection->shutDown = 1;
			closeConnection(connection);
			continue;	/*	Terminate the loop.	*/

		case 0:			/*	Protocol faiure.	*/
			writeMemoNote("[i] tcpcli got no valid contact header",
					neighbor->eid);
			stopSenderThread(connection);
			closeConnection(connection);
			continue;	/*	Try again.		*/
		}

		/*	Contact episode has begun.			*/

		connection->lengthReceived = 0;
		if (connection->hasThrottle == 0
		&& strcmp(neighbor->eid, ownEid) != 0)
		{
			/*	From accept(), and not loopback, so
			 *	end the connection when the incoming
			 *	bundles end.				*/

			connection->secUntilShutdown = IDLE_SHUTDOWN_INTERVAL;
		}

		connection->secSinceReception = 0;
		connection->timeoutCount = 0;
		if (handleMessages(rtp) < 0)
		{
			ionKillMainThread(procName());
			connection->shutDown = 1;
		}

		stopSenderThread(connection);
		closeConnection(connection);
	}

	connection->hasEnded = 1;
	ionPauseAttendant(&(rtp->attendant));
	ionStopAttendant(&(rtp->attendant));
	writeErrmsgMemos();
	writeMemoNote("[i] tcpcli receiver thread has ended", neighbor->eid);
	bpReleaseAcqArea(rtp->work);
	MRELEASE(rtp->buffer);
	MRELEASE(rtp);
	return NULL;
}

/*	*	*	Server thread functions		*	*	*/

typedef struct
{
	int		serverSocket;
	VInduct		*induct;
	int		running;
	Lyst		backlog;	/*	Pending connections.	*/
	pthread_mutex_t	*backlogMutex;
} ServerThreadParms;

static void	*spawnReceivers(void *parm)
{
	/*	Main loop for acceptance of connections and creation
	 *	of threads to service those connections.		*/

	ServerThreadParms	*stp = (ServerThreadParms *) parm;
	saddr			newSocket;
	struct sockaddr		socketName;
	socklen_t		socknamelen;
	LystElt			elt;

	snooze(1);	/*	Let main thread become interruptable.	*/

	/*	Can now begin accepting connections from neighboring
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
	return NULL;
}

/*	*	*	Clock thread functions		*	*	*/

typedef struct
{
	VInduct		*induct;
	int		running;
	Lyst		backlog;	/*	Pending connections.	*/
	pthread_mutex_t	*backlogMutex;
	Lyst		neighbors;
} ClockThreadParms;

static void	*makePlannedConnection(void *parm)
{
	LystElt		neighborElt = (LystElt) parm;
	TcpclNeighbor	*neighbor;
	int		sock;

	neighbor = (TcpclNeighbor *) lyst_data(neighborElt);
	switch (itcp_connect(neighbor->destDuctName, BpTcpDefaultPortNbr,
				&sock))
	{
	case -1:	/*	System failure.				*/
		putErrmsg("tcpcli can't connect to remote node.",
				neighbor->destDuctName);

			/*	Intentional fall-through to next case.	*/

	case 0:		/*	Protocol failure.			*/
		neighbor->connectThreadState = ThreadStopped;
		return NULL;
	}

	if (bpUnblockOutduct("tcp", neighbor->destDuctName) < 0)
	{
		putErrmsg("tcpcli connected but didn't unblock outduct.",
				neighbor->destDuctName);
		closesocket(sock);
		neighbor->connectThreadState = ThreadStopped;
		return NULL;
	}

	/*	TCP connection succeeded, so establish the TCPCL
	 *	connection.						*/

	if (beginConnection(neighborElt, sock, 0) < 0)
	{
		putErrmsg("tcpcli can't add new connection.", NULL);
		neighbor->connectThreadState = ThreadStopped;
		return NULL;
	}

	neighbor->connectThreadState = ThreadStopped;
	return NULL;
}

static int	beginConnectionForPlan(ClockThreadParms *ctp, char *eid,
			char *socketSpec)
{
	LystElt		neighborElt;
	TcpclNeighbor	*neighbor;
	size_t		len = 0;

	/*	If a Neighbor with an open planned connection already
	 *	exists for this plan, nothing to do.			*/

	neighborElt = findNeighborForEid(ctp->neighbors, eid);
	if (neighborElt)
	{
		neighbor = (TcpclNeighbor *) lyst_data(neighborElt);
		if (neighbor->pc->hasSender)
		{
			return 0;	/*	Connection is active.	*/
		}

		if (neighbor->pc->sock != -1)
		{
			/*	Connection is currently not active,
			 *	but it's open and will be active when
			 *	the receiver thread completes exchange
			 *	of contact headers.			*/

			return 0;
		}

		if (neighbor->pc->hasReceiver)
		{
			/*	Connection is not open, but receiver
	 		 *	thread is responsible for reopening it.	*/

			return 0;
		}

		/*	Otherwise, must try to begin new connection.	*/
	}
	else	/*	This is a newly added plan, no neighbor yet.	*/
	{
		neighborElt = addTcpclNeighbor(eid, ctp->induct,
				ctp->neighbors);
		if (neighborElt == NULL)
		{
			putErrmsg("Can't add neighbor.", eid);
			return -1;
		}

		neighbor = (TcpclNeighbor *) lyst_data(neighborElt);
	}

	/*	Neighbor exists, but there is no connection.  Need to
	 *	connect to the indicated socket.			*/

	if (neighbor->connecting	/*	Already on it.		*/
	|| neighbor->disconnecting)	/*	Nothing to do.		*/
	{
		return 0;
	}

	/*	Socket spec for connecting to neighbor may be new.	*/

	if (neighbor->destDuctName == NULL)
	{
		len = istrlen(socketSpec, MAX_CL_DUCT_NAME_LEN + 1) + 1;
		if (len == 0 || (neighbor->destDuctName = MTAKE(len)) == NULL)
		{
			putErrmsg("tcpcli can't copy socket spec.", eid);
			return -1;
		}

		oK(istrcpy(neighbor->destDuctName, socketSpec, len));
	}

	neighbor->connecting = 1;
	neighbor->connectThreadState = ThreadRunning;
	if (pthread_begin(&(neighbor->connectThread), NULL,
			makePlannedConnection, neighborElt))
	{
		putSysErrmsg("tcpcli can't create thread", NULL);
		return -1;
	}

	return 0;
}

static int	referencedInIpn(TcpclConnection *connection, IpnDB *ipndb)
{
	Sdr		sdr = getIonsdr();
	TcpclNeighbor	*neighbor;
	Object		planElt;
	Object		planObj;
			OBJ_POINTER(IpnPlan, ipnPlan);
	Outduct		outduct;
	ClProtocol	protocol;
	Object		ruleElt;
	Object		ruleObj;
			OBJ_POINTER(IpnRule, ipnRule);

	neighbor = connection->neighbor;
	for (planElt = sdr_list_first(sdr, ipndb->plans); planElt;
			planElt = sdr_list_next(sdr, planElt))
	{
		planObj = sdr_list_data(sdr, planElt);
		GET_OBJ_POINTER(sdr, IpnPlan, ipnPlan, planObj);
		if (ipnPlan->defaultDirective.action == xmit)
		{
			sdr_read(sdr, (char *) &outduct, sdr_list_data(sdr,
					ipnPlan->defaultDirective.outductElt),
					sizeof(Outduct));
			sdr_read(sdr, (char *) &protocol, outduct.protocol,
					sizeof(ClProtocol));
			if (strcmp(protocol.name, "tcp") == 0
			&& strcmp(outduct.name, neighbor->destDuctName) == 0)
			{
				return 1;	/*	Referenced.	*/
			}
		}

		for (ruleElt = sdr_list_first(sdr, ipnPlan->rules); ruleElt;
				ruleElt = sdr_list_next(sdr, ruleElt))
		{
			ruleObj = sdr_list_data(sdr, ruleElt);
			GET_OBJ_POINTER(sdr, IpnRule, ipnRule, ruleObj);
			if (ipnRule->directive.action == xmit)
			{
				sdr_read(sdr, (char *) &outduct,
					sdr_list_data(sdr,
					ipnRule->directive.outductElt),
					sizeof(Outduct));
				sdr_read(sdr, (char *) &protocol,
					outduct.protocol, sizeof(ClProtocol));
				if (strcmp(protocol.name, "tcp") == 0
				&& strcmp(outduct.name,
					neighbor->destDuctName) == 0)
				{
					return 1;	/*	Ref.	*/
				}
			}
		}
	}

	return 0;
}

static int	referencedInDtn2(TcpclConnection *connection, Dtn2DB *dtn2db)
{
	Sdr		sdr = getIonsdr();
	TcpclNeighbor	*neighbor;
	Object		planElt;
	Object		planObj;
			OBJ_POINTER(Dtn2Plan, dtn2Plan);
	Outduct		outduct;
	ClProtocol	protocol;
	Object		ruleElt;
	Object		ruleObj;
			OBJ_POINTER(Dtn2Rule, dtn2Rule);

	neighbor = connection->neighbor;
	for (planElt = sdr_list_first(sdr, dtn2db->plans); planElt;
			planElt = sdr_list_next(sdr, planElt))
	{
		planObj = sdr_list_data(sdr, planElt);
		GET_OBJ_POINTER(sdr, Dtn2Plan, dtn2Plan, planObj);
		if (dtn2Plan->defaultDirective.action == xmit)
		{
			sdr_read(sdr, (char *) &outduct, sdr_list_data(sdr,
					dtn2Plan->defaultDirective.outductElt),
					sizeof(Outduct));
			sdr_read(sdr, (char *) &protocol, outduct.protocol,
					sizeof(ClProtocol));
			if (strcmp(protocol.name, "tcp") == 0
			&& strcmp(outduct.name, neighbor->destDuctName) == 0)
			{
				return 1;	/*	Referenced.	*/
			}
		}

		for (ruleElt = sdr_list_first(sdr, dtn2Plan->rules); ruleElt;
				ruleElt = sdr_list_next(sdr, ruleElt))
		{
			ruleObj = sdr_list_data(sdr, ruleElt);
			GET_OBJ_POINTER(sdr, Dtn2Rule, dtn2Rule, ruleObj);
			if (dtn2Rule->directive.action == xmit)
			{
				sdr_read(sdr, (char *) &outduct,
					sdr_list_data(sdr,
					dtn2Rule->directive.outductElt),
					sizeof(Outduct));
				sdr_read(sdr, (char *) &protocol,
					outduct.protocol, sizeof(ClProtocol));
				if (strcmp(protocol.name, "tcp") == 0
				&& strcmp(outduct.name,
					neighbor->destDuctName) == 0)
				{
					return 1;	/*	Ref.	*/
				}
			}
		}
	}

	return 0;
}

static int	noLongerReferenced(TcpclConnection *connection, IpnDB *ipndb,
			Dtn2DB *dtn2db)
{
	Sdr	sdr = getIonsdr();

	CHKERR(sdr_begin_xn(sdr));
	if (ipndb)
	{
		if (referencedInIpn(connection, ipndb))
		{
			sdr_exit_xn(sdr);
			return 0;
		}
	}

	if (dtn2db)
	{
		if (referencedInDtn2(connection, dtn2db))
		{
			sdr_exit_xn(sdr);
			return 0;
		}
	}

	sdr_exit_xn(sdr);
	return 1;			/*	Plan not found.		*/
}

static int	rescanIpn(ClockThreadParms *ctp, IpnDB *ipndb)
{
	Sdr		sdr = getIonsdr();
	Object		planElt;
	Object		planObj;
			OBJ_POINTER(IpnPlan, ipnPlan);
	Outduct		outduct;
	ClProtocol	protocol;
	char		eid[MAX_EID_LEN];
	Object		ruleElt;
	Object		ruleObj;
			OBJ_POINTER(IpnRule, ipnRule);

	CHKERR(sdr_begin_xn(sdr));
	for (planElt = sdr_list_first(sdr, ipndb->plans); planElt;
			planElt = sdr_list_next(sdr, planElt))
	{
		planObj = sdr_list_data(sdr, planElt);
		GET_OBJ_POINTER(sdr, IpnPlan, ipnPlan, planObj);
		if (ipnPlan->defaultDirective.action == xmit)
		{
			sdr_read(sdr, (char *) &outduct, sdr_list_data(sdr,
					ipnPlan->defaultDirective.outductElt),
					sizeof(Outduct));
			sdr_read(sdr, (char *) &protocol, outduct.protocol,
					sizeof(ClProtocol));
			if (strcmp(protocol.name, "tcp") != 0)
			{
				continue;
			}
			
			/*	This is an ipn plan for xmit via a
			 *	TCP outduct for which we may not have
			 *	a connection.				*/

			isprintf(eid, sizeof eid, "ipn:" UVAST_FIELDSPEC ".0",
					ipnPlan->nodeNbr);
			if (beginConnectionForPlan(ctp, eid, outduct.name) < 0)
			{
				sdr_cancel_xn(sdr);
				return -1;
			}

			continue;	/*	Move on to next plan.	*/
		}

		/*	Plan's default directive doesn't need a
		 *	connection, but one of its rules might.		*/

		for (ruleElt = sdr_list_first(sdr, ipnPlan->rules); ruleElt;
				ruleElt = sdr_list_next(sdr, ruleElt))
		{
			ruleObj = sdr_list_data(sdr, ruleElt);
			GET_OBJ_POINTER(sdr, IpnRule, ipnRule, ruleObj);
			if (ipnRule->directive.action == xmit)
			{
				sdr_read(sdr, (char *) &outduct,
					sdr_list_data(sdr,
					ipnRule->directive.outductElt),
					sizeof(Outduct));
				sdr_read(sdr, (char *) &protocol,
					outduct.protocol, sizeof(ClProtocol));
				if (strcmp(protocol.name, "tcp") != 0)
				{
					continue;
				}
			
				/*	This is an ipn rule for xmit
				 *	via a TCP outduct for which
				 *	we may not have a connection.	*/

				isprintf(eid, sizeof eid,
						"ipn:" UVAST_FIELDSPEC ".0",
						ipnPlan->nodeNbr);
				if (beginConnectionForPlan(ctp, eid,
						outduct.name) < 0)
				{
					sdr_cancel_xn(sdr);
					return -1;
				}

				break;	/*	Out of rules loop.	*/
			}
		}
	}

	return sdr_end_xn(sdr);
}

static int	rescanDtn2(ClockThreadParms *ctp, Dtn2DB *dtn2db)
{
	Sdr		sdr = getIonsdr();
	Object		planElt;
	Object		planObj;
			OBJ_POINTER(Dtn2Plan, dtn2Plan);
	Outduct		outduct;
	ClProtocol	protocol;
	char		nodeName[SDRSTRING_BUFSZ];
	char		eid[MAX_EID_LEN];
	Object		ruleElt;
	Object		ruleObj;
			OBJ_POINTER(Dtn2Rule, dtn2Rule);

	CHKERR(sdr_begin_xn(sdr));
	for (planElt = sdr_list_first(sdr, dtn2db->plans); planElt;
			planElt = sdr_list_next(sdr, planElt))
	{
		/*	If plan's default directive isn't xmit via
		 *	an outduct of the TCPCL protocol that does
		 *	not yet exist, skip it.				*/

		planObj = sdr_list_data(sdr, planElt);
		GET_OBJ_POINTER(sdr, Dtn2Plan, dtn2Plan, planObj);
		if (dtn2Plan->defaultDirective.action == xmit)
		{
			sdr_read(sdr, (char *) &outduct, sdr_list_data(sdr,
					dtn2Plan->defaultDirective.outductElt),
					sizeof(Outduct));
			sdr_read(sdr, (char *) &protocol, outduct.protocol,
					sizeof(ClProtocol));
			if (strcmp(protocol.name, "tcp") != 0)
			{
				continue;
			}
			
			/*	This is a dtn2 plan for xmit via a
			 *	TCP outduct for which we may not have
			 *	a connection.				*/

			oK(sdr_string_read(sdr, nodeName, dtn2Plan->nodeName)); 
			isprintf(eid, sizeof eid, "dtn:%s", nodeName);
			if (beginConnectionForPlan(ctp, eid, outduct.name) < 0)
			{
				sdr_cancel_xn(sdr);
				return -1;
			}

			continue;	/*	Move on to next plan.	*/
		}

		/*	Plan's default directive doesn't need a
		 *	connection, but one of its rules might.		*/

		for (ruleElt = sdr_list_first(sdr, dtn2Plan->rules); ruleElt;
				ruleElt = sdr_list_next(sdr, ruleElt))
		{
			ruleObj = sdr_list_data(sdr, ruleElt);
			GET_OBJ_POINTER(sdr, Dtn2Rule, dtn2Rule, ruleObj);
			if (dtn2Rule->directive.action == xmit)
			{
				sdr_read(sdr, (char *) &outduct,
					sdr_list_data(sdr,
					dtn2Rule->directive.outductElt),
					sizeof(Outduct));
				sdr_read(sdr, (char *) &protocol,
					outduct.protocol, sizeof(ClProtocol));
				if (strcmp(protocol.name, "tcp") != 0)
				{
					continue;
				}

				/*	This is a dtn2 rule for xmit
				 *	via TCP outduct for which we
				 *	may not have a connection.	*/

				oK(sdr_string_read(sdr, eid,
						dtn2Plan->nodeName)); 
				if (beginConnectionForPlan(ctp, eid,
						outduct.name) < 0)
				{
					sdr_cancel_xn(sdr);
					return -1;
				}

				break;	/*	Out of rules loop.	*/
			}
		}
	}

	return sdr_end_xn(sdr);
}

static void	*cleanUpPlannedConnection(void *parm)
{
	LystElt		neighborElt = (LystElt) parm;
	TcpclNeighbor	*neighbor;
	TcpclConnection	*connection;

	neighbor = (TcpclNeighbor *) lyst_data(neighborElt);
	connection = neighbor->pc;
	endConnection(connection, -1);
	MRELEASE(neighbor->destDuctName);
	neighbor->destDuctName = NULL;
	neighbor->cleanupThreadState = ThreadStopped;
	return NULL;
}

static int	rescan(ClockThreadParms *ctp, IpnDB *ipndb, Dtn2DB *dtn2db)
{
	LystElt		elt;
	TcpclNeighbor	*neighbor;
	TcpclConnection	*connection;
	VOutduct	*vduct;
	PsmAddress	vductElt;

	/*	First look for newly added ipnfw plans or rules and
	 *	try to create connections for them.			*/

	if (ipndb)
	{
		if (rescanIpn(ctp, ipndb) < 0)
		{
			return -1;
		}
	}

	/*	Next look for newly added dtn2fw plans or rules and
	 *	try to create connections for them.			*/

	if (dtn2db)
	{
		if (rescanDtn2(ctp, dtn2db) < 0)
		{
			return -1;
		}
	}

	/*	Now look for connections that must be ended because
	 *	the plans/rules referencing them have been removed.	*/

	for (elt = lyst_first(ctp->neighbors); elt; elt = lyst_next(elt))
	{
		neighbor = (TcpclNeighbor *) lyst_data(elt);
		connection = neighbor->pc;
		if (connection->hasThrottle == 0)
		{
			/*	No connection to this neighbor per
			 *	plan, nothing to clean up.		*/
			
			continue;
		}

		/*	This connection was originally cited in an
		 *	egress plan or rule.				*/

		if (connection->newlyAdded)
		{
			continue;	/*	Too soon to clean up.	*/
		}

		/*	Connection is well established.			*/

		if (neighbor->disconnecting	/*	Cleaning up.	*/
		|| neighbor->connecting)	/*	Nothing to do.	*/
		{
			continue;
		}

		/*	Clean up as necessary.				*/

		findOutduct("tcp", neighbor->destDuctName, &vduct, &vductElt);
		if (vductElt == 0	/*	Outduct is gone.	*/
		|| noLongerReferenced(connection, ipndb, dtn2db))
		{
			neighbor->disconnecting = 1;
			neighbor->cleanupThreadState = ThreadRunning;
			if (pthread_begin(&(neighbor->cleanupThread), NULL,
					cleanUpPlannedConnection, elt))
			{
				putSysErrmsg("tcpcli can't create thread",
						NULL);
				return -1;
			}
		}
	}

	return 0;
}

static int	sendKeepalive(TcpclConnection *connection)
{
	static char	keepalive = 0x40;
	TcpclNeighbor	*neighbor = connection->neighbor;
	int		result;

	if (connection->sock == -1)
	{
		return 0;
	}

	pthread_mutex_lock(&(connection->mutex));
	result = itcp_send(&connection->sock, &keepalive, 1);
	pthread_mutex_unlock(&(connection->mutex));
	if (result < 1)
	{
		writeMemoNote("[?] tcpcl connection lost (keepalive)",
				neighbor->eid);
		return 0;
	}

	return 1;
}

static int	clearBacklog(ClockThreadParms *ctp)
{
	static char	*noEid = "<unknown node>";
	Sdr		sdr = getIonsdr();
	LystElt		elt;
	int		sock;
	LystElt		neighborElt;

	oK(sdr_begin_xn(sdr));
	pthread_mutex_lock(ctp->backlogMutex);
	while ((elt = lyst_first(ctp->backlog)))
	{
		sock = (saddr) lyst_data(elt);

		/*	This new neighbor may be temporary.  After
		 *	header exchange we may be loading this
		 *	connection onto another existing neighbor.	*/

		neighborElt = addTcpclNeighbor(noEid, ctp->induct,
				ctp->neighbors);
		if (neighborElt == NULL)
		{
			closesocket(sock);
			pthread_mutex_unlock(ctp->backlogMutex);
			sdr_cancel_xn(sdr);
			putErrmsg("tcpcli can't add tcpcl neighbor.", NULL);
			return -1;
		}

		if (beginConnection(neighborElt, sock, 1) < 0)
		{
			pthread_mutex_unlock(ctp->backlogMutex);
			sdr_cancel_xn(sdr);
			putErrmsg("tcpcli can't add responsive connection.",
					NULL);
			return -1;
		}

		/*	lyst_delete function for backlog lyst closes
		 *	sock unless it's -1.				*/

		oK(lyst_data_set(elt, (void *) -1));
		lyst_delete(elt);
	}

	pthread_mutex_unlock(ctp->backlogMutex);
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("tcpcli failed adding responsive connections.", NULL);
		return -1;
	}

	return 0;
}

static void	checkConnection(TcpclConnection *connection)
{
	if (connection->hasEnded)
	{
		eraseConnection(connection);
		return;
	}

	/*	Track idleness in bundle traffic.			*/

	if (connection->secUntilShutdown > 0)
	{
		connection->secUntilShutdown--;
	}

	if (connection->secUntilShutdown == 0)
	{
		endConnection(connection, 0);
		return;
	}

	/*	Track idleness in all reception.			*/

	if (connection->keepaliveInterval != 0)
	{
		connection->secSinceReception++;
		if (connection->secSinceReception == 
				connection->keepaliveInterval)
		{
			connection->timeoutCount++;
			if (connection->timeoutCount > 1)
			{
				endConnection(connection, 0);
				return;
			}
		}
	}

	/*	Track sending of keepalives.				*/

	if (connection->secUntilKeepalive > 0)
	{
		connection->secUntilKeepalive--;
	}

	if (connection->secUntilKeepalive == 0)
	{
		if (sendKeepalive(connection) == 0)
		{
			stopSenderThread(connection);
			closeConnection(connection);
		}
		else
		{
			connection->secUntilKeepalive
					= connection->keepaliveInterval;
		}
	}

	/*	Count down to reconnect.				*/

	if (connection->secUntilReconnect > 0)
	{
		connection->secUntilReconnect--;
	}
}

static void	*handleEvents(void *parm)
{
	/*	Main loop for implementing time-driven operations.	*/

	ClockThreadParms	*ctp = (ClockThreadParms *) parm;
	Sdr			sdr = getIonsdr();
	IpnDB			*ipndb;
	Dtn2DB			*dtn2db;
	time_t			planChangeTime;
	time_t			lastIpnPlanChange = 0;
	time_t			lastDtn2PlanChange = 0;
	int			rescanInterval = 1;
	int			secUntilRescan = 2;
	LystElt			elt;
	LystElt			nextElt;
	TcpclNeighbor		*neighbor;
	int			i;

	while (ctp->running)
	{
		/*	Begin TCPCL connections for all accepted TCP
		 *	connections.					*/

		if (clearBacklog(ctp) < 0)
		{
			ionKillMainThread(procName());
			ctp->running = 0;
			continue;
		}

		/*	Try to begin TCPCL connections for all newly
		 *	added plans and rules that reference those
		 *	connections.					*/

		ipndb = getIpnConstants();
		dtn2db = getDtn2Constants();
		if (ipndb)
		{
			planChangeTime = sdr_list_user_data(sdr, ipndb->plans);
			if (planChangeTime > lastIpnPlanChange)
			{
				secUntilRescan = 1;
				lastIpnPlanChange = planChangeTime;
			}
		}

		if (dtn2db)
		{
			planChangeTime = sdr_list_user_data(sdr, dtn2db->plans);
			if (planChangeTime > lastDtn2PlanChange)
			{
				secUntilRescan = 1;
				lastDtn2PlanChange = planChangeTime;
			}
		}

		secUntilRescan--;
		if (secUntilRescan == 0)
		{
			if (rescan(ctp, ipndb, dtn2db) < 0)
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

		/*	Now look for events whose time has come.	*/

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

			if (neighbor->connecting)
			{
				if (neighbor->connectThreadState
						== ThreadStopped)
				{
					pthread_join(neighbor->connectThread,
							NULL);
					neighbor->connecting = 0;
				}
				else	/*	Can't delete yet.	*/
				{
					continue;
				}
			}

			if (neighbor->disconnecting)
			{
				if (neighbor->cleanupThreadState
						== ThreadStopped)
				{
					pthread_join(neighbor->cleanupThread,
							NULL);
					neighbor->disconnecting = 0;
				}
				else	/*	Can't delete yet.	*/
				{
					continue;
				}
			}

			for (i = 0; i < 2; i++)
			{
				checkConnection(&(neighbor->connections[i]));
			}

			if (neighbor->pc->hasThrottle == 0
			&& neighbor->cc->sock == -1)
			{
				deleteTcpclNeighbor(neighbor);
				lyst_delete(elt);
			}
		}

		snooze(1);
	}

	writeErrmsgMemos();
	writeMemo("[i] tcpcli clock thread has ended.");
	return NULL;
}

/*	*	*	Main thread functions		*	*	*/

static void	dropPendingConnection(LystElt elt, void *userdata)
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
int	tcpcli(int a1, int a2, int a3, int a4, int a5,
		int a6, int a7, int a8, int a9, int a10)
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

	if (bpAttach() < 0 || ipnInit() < 0 || dtn2Init() < 0)
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
	 *	as neighbors are made.  None are created during
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

	lyst_delete_set(backlog, dropPendingConnection, NULL);
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

	/*	Start the clock thread, which does initial load
	 *	of the neighbors lyst.				*/

	ctp.induct = vduct;
	ctp.running = 1;
	ctp.neighbors = neighbors;
	ctp.backlog = backlog;
	ctp.backlogMutex = &backlogMutex;
	if (pthread_begin(&clockThread, NULL, handleEvents, &ctp))
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
	if (pthread_begin(&serverThread, NULL, spawnReceivers, &stp))
	{
		shutDownNeighbors(neighbors);
		snooze(2);	/*	Let clock thread clean up.	*/
		ctp.running = 0;
		pthread_join(clockThread, NULL);
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
	pthread_join(serverThread, NULL);
	shutDownNeighbors(neighbors);
	snooze(2);		/*	Let clock thread clean up.	*/
	ctp.running = 0;
	pthread_join(clockThread, NULL);
	closesocket(stp.serverSocket);
	pthread_mutex_destroy(&backlogMutex);
	lyst_destroy(backlog);
	lyst_destroy(neighbors);
	writeErrmsgMemos();
	writeMemo("[i] tcpcli has ended.");
	bp_detach();
	return 0;
}
