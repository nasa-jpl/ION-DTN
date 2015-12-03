/*
	tcpcli.c:	ION TCP convergence-layer adapter daemon.
			Handles both transmission and reception.

	Author: Scott Burleigh, JPL

	Copyright (c) 2015, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
	
									*/
#include "bpP.h"

#define	BpTcpDefaultPortNbr	4556
#define	TCPCLA_BUFSZ		(64 * 1024)

#define TCPCLA_MAGIC 		"dtn!"
#define TCPCLA_MAGIC_SIZE	4
#define TCPCLA_ID_VERSION	0x03
#define TCPCLA_FLAGS		0x00
#define TCPCLA_TYPE_DATA	0x01
#define TCPCLA_TYPE_ACK		0x02
#define TCPCLA_TYPE_REF_BUN	0x03
#define TCPCLA_TYPE_KEEP_AL	0x04
#define TCPCLA_TYPE_SHUT_DN	0x05
#define SHUT_DN_BUFSZ		32
#define SHUT_DN_DELAY_FLAG	0x01
#define SHUT_DN_REASON_FLAG	0x02
#define SHUT_DN_NO	 	0
#define SHUT_DN_IDLE		1
#define SHUT_DN_IDLE_HEX	0x00
#define SHUT_DN_VER		2
#define SHUT_DN_VER_HEX		0x01
#define SHUT_DN_BUSY		3
#define SHUT_DN_BUSY_HEX	0x02

#ifndef TCPCLA_RESCAN_INTERVAL
#define TCPCLA_RESCAN_INTERVAL	(60)
#endif

#ifndef KEEPALIVE_INTERVAL
#define KEEPALIVE_INTERVAL	(15)
#endif
#ifndef IDLE_SHUTDOWN_INTERVAL
#define IDLE_SHUTDOWN_INTERVAL	(60)
#endif
#ifndef MAX_RECONNECT_INTERVAL
#define MAX_RECONNECT_INTERVAL	(3600)
#endif
#define	TCPCLA_SEGMENT_ACKS	(1)
#define	TCPCLA_REACTIVE		(0)
#define	TCPCLA_REFUSALS		(0)
#define	TCPCLA_LENGTH_MSGS	(0)

typedef struct
{
	char			*eid;		/*	Remote node ID.	*/
	int			sock;
	char			*destDuctName;
	pthread_t		sender;
	pthread_t		receiver;
	pthread_mutex_t		mutex;
	sm_SemId		*eobSemaphore;	/*	End of bundle.	*/
	VInduct			*vduct;

	/*	TCPCL exchange state.					*/

	vast			outboundLength;	/*	Curr. outbound.	*/
	vast			lengthAcked;	/*	Curr. outbound.	*/
	vast			inboundLength;	/*	Curr. inbound.	*/
	vast			lengthReceived;	/*	Curr. inbound.	*/
	int			secUntilKeepalive;
	int			secUntilShutdown;	/*	(idle)	*/
	int			secUntilReconnect;
	int			secSinceReception;
	int			timeoutCount;
	int			mustDelete;	/*	Boolean		*/

	/*	TCPCL control parameters.				*/

	int			keepaliveInterval;
	int			reconnectInterval;
	int			segmentAcks;	/*	Boolean		*/
	int			reactiveFrags;	/*	Boolean		*/
	int			bundleRefusals;	/*	Boolean		*/
	int			lengthMessages;	/*	Boolean		*/
} TcpclConnection;

#ifndef mingw
extern void	handleConnectionLoss();
#endif

static void	interruptThread()
{
	isignal(SIGTERM, interruptThread);
	ionKillMainThread("tcpcli");
}

/*	*	*	Contact management functions	*	*	*/

static int	sendContactHeader(TcpclConnection *connection)
{
	int	len = 0;
	char	contactHeader[11 + MAX_EID_LEN];
	int	flagsByte;
	short	keepaliveInterval;
	size_t	eidLength;
	Sdnv	eidLengthSdnv;
	int	result;

	memcpy(contactHeader, "dtn!", 4);
	len += 4;
	contactHeader[len] = 0x03;
	len += 1;
	flagsByte = TCPCLA_SEGMENT_ACKS
			+ (TCPCLA_REACTIVE << 1)
			+ (TCPCLA_REFUSALS << 2)
			+ (TCPCLA_LENGTH_MSGS << 3);
	contactHeader[len] = flagsByte;
	len += 1;
	keepaliveInterval = KEEPALIVE_INTERVAL;
	keepaliveInterval = htons(keepaliveInterval);
	memcpy(contactHeader + len, (char *) &keepaliveInterval, 2);
	len += 2;
	eidLength = istrlen(connection->eid, MAX_EID_LEN);
	encodeSdnv(&eidLengthSdnv, eidLength);
	memcpy(contactHeader + len, eidLengthSdnv.text, eidLengthSdnv.length);
	len += eidLengthSdnv.length;
	memcpy(contactHeader + len, connection->eid, eidLength);
	len += eidLength;

	/*	connection->sock is known to be a connected socket
	 *	at this point.						*/

	pthread_mutex_lock(&(connection->mutex));
	result = itcp_send(connection->sock, contactHeader, len);
	pthread_mutex_unlock(&(connection->mutex));
	switch (result)
	{
	case -1;
		connection->mustDelete = 1;
		return -1;

	case 0:		/*	Lost the TCP connection.		*/
		connection->sock = -1;
		connection->secUntilReconnect = connection->reconnectInterval;
		return 0;

	default:
		return result;
	}
}

static void	sendShutdown(TcpclConnection *connection, char reason)
{
	char	shutdown[2];
	int	len;

	if (connection->sock == -1)	/*	Nothing to send.	*/
	{
		return;
	}

	if (reason < 0)			/*	No reason code.		*/
	{
		shutdown[0] = 0x50;
		len = 1;
	}
	else
	{
		shutdown[0] = 0x52;
		shutdown[1] = reason;
		len = 2;
	}

	pthread_mutex_lock(&(connection->mutex));
	result = itcp_send(connection->sock, shutdown, len);
	pthread_mutex_unlock(&(connection->mutex));
}

static int	receiveSdnv(TcpclConnection *connection, uvast *val)
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

		*val << 7;

		/*	Receive next byte of SDNV.			*/

		switch (irecv(connection->sock, (char *) &byte, 1, 0))
		{
		case -1:
			if (errno == EINTR)	/*	Shutdown.	*/
			{
				return 0;
			}

			putSysErrmsg("irecv() error on TCP socket",
					connection->destDuctName);
			return -1;

		case 0:			/*	Connection closed.	*/
			return 0;

		default:
			break;		/*	Out of switch.		*/
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

static int	receiveContactHeader(TcpclConnection *connection,
			Lyst connections)
{
	int		result;
	unsigned char	header[8];
	unsigned short	keepaliveInterval;
	uvast		eidLength;
	char		*eidbuf;

	result = itcp_recv(connection->sock, (char *) header, sizeof header);
	if (result < sizeof header)
	{
		putErrmsg("Can't get TCPCL contact header.",
				connection->destDuctName);
		connection->mustDelete = 1;
		return result;
	}

	if (memcmp(header, "dtn!", 4) != 0)
	{
		writeMemoNote("[?] Invalid TCPCL contact header",
				connection->destDuctName);
		connection->mustDelete = 1;
		return 0;
	}

	if (header[4] < 3)		/*	Version mismatch.	*/
	{
		oK(sendShutdown(connection, 0x01));
		writeMemoNote("[?] Invalid TCPCL contact header",
				connection->destDuctName);
		connection->mustDelete = 1;
		return 0;
	}

	/*	6th byte of header is flags.				*/

	connection->segmentAcks = TCPCLA_SEGMENT_ACKS && (header[5] & 0x01);
	connection->reactiveFrags = TCPCLA_REACTIVE && (header[5] & 0x02);
	connection->bundleRefusals = (TCPCLA_REFUSALS && (header[5] & 0x04))
			&& connection->segmentAcks;
	connection->lengthMessages = TCPCLA_LENGTH_MSGS && (header[5] & 0x08);

	/*	7th-8th bytes of header are keepalive interval.		*/

	memcpy((char *) &keepaliveInterval, (char *) header + 6, 2);
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

	if (receiveSdnv(connection, &eidLength) < 1)
	{
		putErrmsg("Can't get EID length in TCPCL contact header",
				connection->destDuctName);
		connection->mustDelete = 1;
		return 0;
	}

	eidbuf = MTAKE(eidLength + 1);
	if (eidbuf == NULL)
	{
		putErrmsg("Not enough memory for EID.", utoa(eidLength));
		connection->mustDelete = 1;
		return -1;
	}

	result = itcp_recv(connection->sock, eidbuf, eidLength);
	if (result < eidLength)
	{
		putErrmsg("Can't get TCPCL contact header EID.",
				connection->destDuctName);
		connection->mustDelete = 1;
		return result;
	}

	eidbuf[eidLength] = '\0';
	if (connnection->eid)
	{
		if (strcmp(eidbuf, connection->eid) == 0)
		{
			result = 1;
		}
		else
		{
			writeMemoNote("[?] EIDs don't match",
					connection->destDuctName);
			connection->mustDelete = 1;
			result = 0;
		}

		MRELEASE(eidbuf);
	}
	else
	{
		if (findConnectionForEid(connections, eidbuf) == NULL)
		{
			connection->eid = eidbuf;
			result = 1;
		}
		else
		{
			MRELEASE(eidbuf);
			writeMemoNote("[?] Duplicate connection for EID",
					connection->eid;
			connection->mustDelete = 1;
			result = 0;
		}
	}

	connection->secUntilShutdown = IDLE_SHUTDOWN_INTERVAL;
	connection->secUntilReconnect = -1;
	connection->keepaliveCount = 0;
	return result;
}

static int	reconnect(TcpclConnection *connection, Lyst connections)
{
	int	result;

	if (connection->secUntilReconnect != 0)
	{
		return 0;	/*	Must not reconnect yet.		*/
	}

	/*	Okay to make next reconnection attempt.			*/

	switch (itcp_connect(connection->destDuctName,
			BpStcpDefaultPortNbr, &(connection->sock)))
	{
	case -1:		/*	System failure.			*/
		putErrmsg("tcpcli failed on TCP reconnect.", connection->eid);
		return -1;

	case 0:			/*	Connection still refused.	*/
		writeMemoNote("[i] tcpcli unable to reconnect",
				connection->eid);
		connection->reconnectInterval <<= 1;
		if (connection->reconnectInterval > MAX_RECONNECT_INTERVAL)
		{
			connection->reconnectInterval = MAX_RECONNECT_INTERVAL;
		}

		connection->secUntilReconnect = connection->reconnectInterval;
		return 0;

	default:
		break;		/*	Out of switch.			*/
	}

	/*	TCP connection succeeded.				*/

	result = sendContactHeader(connection);
	if (result < 1)
	{
		putErrmsg("Failed sending contact header.",
				connection->destDuctName);
		return result;
	}

	result = receiveContactHeader(connection, connections);
	if (result < 1)
	{
		putErrmsg("Failed receiving contact header.",
				connection->destDuctName);
		return result;
	}

	connection->reconnectInterval = 1;
	connection->secUntilReconnect = -1;
	if (bpUnblockOutduct("tcp", connection->destDuctName) < 0)
	{
		putErrmsg("tcpcli reconnected but didn't unblock.",
				connection->eid);
	}

	return 1;
}

/*	*	*	Sender thread functions		*	*	*/

typedef struct
{
	int			*running;
	Lyst			connections;
	TcpclConnection		*connection;
	char			*buffer;
	VOutduct		*vduct;
	Outflow			outflows[3];
} SenderThreadParms;

static int	discardBundle(Object *bundleZco)
{
	Sdr	sdr = getIonsdr();
	int	result = 0;

	if (bpHandleXmitFailure(*bundleZco) < 0)
	{
		putErrmsg("tcpcli can't handle bundle xmit failure.", NULL);
		result = -1;
	}
	else
	{
		/*	Destroy bundle, unless there's stewardship
		 *	or custody.					*/

		oK(sdr_begin_xn(sdr));
		zco_destroy(sdr, *bundleZco);
		if (sdr_end_xn(sdr) < 0)
		{
			putErrmsg("tcpcli can't destroy bundle ZCO.", NULL);
			result = -1;
		}
	}

	*bundle = 0;
	sm_SemGive(stp->connection->eobSemaphore);
	return result;
}

static int	sendBundleByTcpcl(SenderThreadParms *stp, Object *bundleZco)
{
	Sdr		sdr = getIonsdr();
	int		result;
	uvast		bytesRemaining;
	int		flags;
	ZcoReader	reader;
	uuvast		bytesToLoad;
	uvast		bytesToSend;
	int		firstByte;
	Sdnv		segLengthSdnv;
	char		segHeader[4];
	int		segHeaderLength;

	if (stp->connection->sock == -1)	/*	Disconnected.	*/
	{
		result = reconnect(stp->connection, stp->connections);
		switch (result)
		{
		case -1:
			return result;

		case 0:
			return discardBundle(bundleZco);

		default:			/*	Reconnected.	*/
			break;			/*	Out of switch.	*/
		}
	}

	zco_start_transmitting(bundleZco, &reader);
	zco_track_file_offset(&reader);
	bytesRemaining = zco_length(sdr, bundleZco);
	stp->connection->outboundLength = bytesRemaining;
	stp->connection->lengthAcked = 0;
	flags = 0x02;				/*	1st segment.	*/
	while (bytesRemaining > 0)
	{
		bytesToLoad = bytesRemaining;
		if (bytesToLoad > TCPCLA_BUFSZ)
		{
			bytesToLoad = TCPCLA_BUFSZ;
		}
		else
		{
			flags &= 0x01;		/*	Last segment.	*/
		}

		firstByte = 0x10 & flags;
		segHeader[0] = firstByte;
		encodeSdnv(&segLengthSdnv, bytesToSend);
		memcpy(segHeader + 1, segLengthSdnv.text, segLengthSdnv.length);
		segHeaderLength = 1 + segLengthSdnv.length;
		pthread_mutex_lock(&(stp->connection->mutex));
		switch (itpc_send(stp->connection->sock, segHeader,
				segHeaderLength))
		{
		case -1:			/*	System failed.	*/
			pthread_mutex_unlock(&(stp->connection->mutex));
			return result;

		case 0:
			writeMemoNote("[?] tcpcl connection lost",
					stp->eid);
			pthread_mutex_unlock(&(stp->connection->mutex));
			return discardBundle(bundleZco);

		default:
			break;			/*	Sent header.	*/
		}

		CHKERR(sdr_begin_xn(sdr));
		bytesToSend = zco_transmit(sdr, &reader, bytesToLoad,
				stp->buffer);
		if (sdr_end_xn(sdr) < 0 || bytesToSend != bytesToLoad)
		{
			putErrmsg("Incomplete zco_transmit.", NULL);
			pthread_mutex_unlock(&(stp->connection->mutex));
			return -1;
		}

		switch (itpc_send(stp->connection->sock, stp->buffer,
				bytesToSend))
		{
		case -1:			/*	System failed.	*/
			pthread_mutex_unlock(&(stp->connection->mutex));
			return result;

		case 0:
			writeMemoNote("[?] tcpcl connection lost",
					stp->eid);
			pthread_mutex_unlock(&(stp->connection->mutex));
			return discardBundle(bundleZco);

		default:
			break;			/*	Sent content.	*/
		}

		pthread_mutex_unlock(&(stp->connection->mutex));
		flags = 0x00;			/*	No longer 1st.	*/
		bytesRemaining -= bytesToSend;
	}

	return 0;
}

static int	sendOneBundle(SenderThreadParms *stp, Object *bundleZco)
{
	unsigned int	maxPayloadLength;
	BpExtendedCOS	extendedCOS;
	int		result;

	while (1)
	{
		/*	Loop until max payload length is known.		*/

		if (!(maxPayloadLengthKnown(stp->vduct, &maxPayloadLength)))
		{
			snooze(1);
			continue;
		}

		/*	If outduct has meanwhile been closed, quit.	*/

		if (sm_SemEnded(stp->vduct->semaphore))
		{
			writeMemoNote("[i] tcpcli outduct closed",
					stp->connection->destDuctName);
			stp->connection->mustDelete = 1;
			stp->running = 0;
			return 0;
		}

		/*	Get the next bundle to send.			*/

		if (bpDequeue(stp->vduct, stp->outflows, bundleZco,
				&extendedCOS, stp->connection->destDuctName,
				maxPayloadLength, -1) < 0)
		{
			putErrmsg("Can't dequeue bundle.", NULL);
			return -1;
		}

		if (*bundleZco == 0)		/*	Interrupted.	*/
		{
			continue;		/*	Try again.	*/
		}

		/*	Send that bundle.				*/

		return sendBundleByTcpcl(stp, bundleZco);
	}
}

static void	*sendBundles(void *parm)
{
	SenderThreadParms	*stp = (SenderThreadParms *) parm;
	Sdr			sdr = getIonsdr();
	char			*procName = "tcpcli";
	PsmAddress		vductElt;
	Outduct			duct;
	int			i;
	Object			bundleZco = 0;
	int			result;

	/*	Load other required sender thread parms.		*/

	stp->buffer = MTAKE(TCPCLA_BUFSZ);
	if (stp->buffer == NULL)
	{
		putErrmsg("No memory for TCP buffer.", stp->connection->eid);
		ionKillMainThread(procName);
		stp->running = 0;
		return NULL;
	}

	findOutduct("tcp", stp->connection->destDuctName, &(stp->vduct),
			&vductElt);
	if (vductElt == 0)
	{
		putErrmsg("No such tcp duct.", stp->connection->destDuctName);
		ionKillMainThread(procName);
		stp->running = 0;
		return NULL;
	}

	CHKNULL(sdr_begin_xn(sdr));	/*	Lock the heap.		*/
	sdr_read(sdr, (char *) &duct, sdr_list_data(sdr,
			stp->vduct->outductElt), sizeof(Outduct));
	sdr_exit_xn(sdr);
	memset((char *) (stp->outflows), 0, sizeof stp->outflows);
	stp->outflows[0].outboundBundles = duct.bulkQueue;
	stp->outflows[1].outboundBundles = duct.stdQueue;
	stp->outflows[2].outboundBundles = duct.urgentQueue;
	for (i = 0; i < 3; i++)
	{
		stp->outflows[i].svcFactor = 1 << i;
	}

	/*	Ready to start sending bundles.				*/

	while (stp->running)
	{
		/*	First, handle result of previous bundle
		 *	transmission, if any.				*/

		if (sm_SemTake(stp->connection->eobSemaphore) < 0)
		{
			putErrmsg("Failed taking eob semaphore.", NULL);
			ionKillMainThread(procName);
			stp->running = 0;
			continue;
		}

		if (sm_SemEnded(stp->connection->eobSemaphore))
		{
			writeMemoNote("[i] tcpcli sender thread stopped",
					stp->connection->eid);
			stp->running = 0;
			continue;
		}

		/*	(Final acknowledgment of previously sent
		 *	bundle, if any, has been received -- or
		 *	the connection has been lost.)			*/

		if (bundleZco)
		{
			if (stp->connection.lengthAcked ==
					stp->connection->outboundLength)
			{
				result = bpHandleXmitSuccess(bundleZco, 0);
			}
			else	/*	Incomplete transmission.	*/
			{
				result = bpHandleXmitFailure(bundleZco);
			}

			/*	Destroy bundle, unless there's
			 *	stewardship or custody.			*/

			oK(sdr_begin_xn(sdr));
			zco_destroy(sdr, bundleZco);
			if (sdr_end_xn(sdr) < 0)
			{
				putErrmsg("Can't destroy bundle ZCO.", NULL);
				ionKillMainThread(procName);
				stp->running = 0;
				continue;
			}

			if (result < 0)
			{
				putErrmsg("Can't handle xmit result.", NULL);
				ionKillMainThread(procName);
				stp->running = 0;
				continue;
			}
		}

		/*	Now send the next bundle in the queue.		*/

		if (sendOneBundle(stp, &bundleZco) < 0)
		{
			putErrmsg("tcpcli failed sending bundle.", NULL);
			ionKillMainThread(procName);
			stp->running = 0;
			continue;
		}

		/*	Reset keepalive countdown.			*/

		if (stp->connection->keepaliveInterval > 0)
		{
			stp->connection->secUntilKeepalive =
					stp->connection->keepaliveInterval;
		}
	}

	writeErrmsgMemos();
	writeMemoNote("[i] tcpcli sender thread has ended.",
			stp->connection->eid);
	MRELEASE(stp->buffer);
	MRELEASE(stp);
	return NULL;
}

/*	*	*	Receiver thread functions	*	*	*/

typedef struct
{
	int			*running;
	Lyst			connections;
	TcpclConnection		*connection;
	char			*buffer;
	AcqWorkArea		*work;
} ReceiverThreadParms;

static int	handleDataSegment(ReceiverThreadParms *rtp,
			unsigned char msgtypeByte)
{
}

static int	handleAck(ReceiverThreadParms *rtp,
			unsigned char msgtypeByte)
{
}

static int	handleRefusal(ReceiverThreadParms *rtp,
			unsigned char msgtypeByte)
{
}

static int	handleKeepalive(ReceiverThreadParms *rtp,
			unsigned char msgtypeByte)
{
}

static int	handleShutdown(ReceiverThreadParms *rtp,
			unsigned char msgtypeByte)
{
}

static int	handleLength(ReceiverThreadParms *rtp,
			unsigned char msgtypeByte)
{
}

static void	*handleMessages(void *parm)
{
	ReceiverThreadParms	*rtp = (ReceiverThreadParms *) parm;
	char			*procName = "tcpcli";
	TcpclConnection		*connection;
	unsigned char		msgtypeByte;
	int			msgType;

	/*	Load other required receiver thread parms.		*/

	rtp->work = bpGetAcqArea(rtp->connection->vduct);
	if (rtp->work == NULL)
	{
		putErrmsg("tcpcli can't get acquisition work area.",
				rtp->connection->destDuctName);
		ionKillMainThread(procName);
		return NULL;
	}

	rtp->buffer = MTAKE(TCPCLA_BUFSZ);
	if (rtp->buffer == NULL)
	{
		putErrmsg("No memory for TCP buffer.",
				rtp->connection->destDuctName);
		ionKillMainThread(procName);
		return NULL;
	}

	connection = rtp->connection;
	if (sendContactHeader(connection) < 1)
	{
		writeErrmsgMemos();
		writeMemoNote("[i] tcpcli failed sending contact header.",
				connection->destDuctName);
		MRELEASE(rtp->buffer);
		MRELEASE(rtp);
		return NULL;
	}

	if (receiveContactHeader(connection, rtp->connections) < 1)
	{
		writeErrmsgMemos();
		writeMemoNote("[i] tcpcli failed receiving contact header.",
				connection->destDuctName);
		MRELEASE(rtp->buffer);
		MRELEASE(rtp);
		return NULL;
	}

	while (rtp->running)
	{
		switch (irecv(connection->sock, (char *) &msgtypeByte, 1, 0))
		{
		case -1:
			if (errno != EINTR)	/*	Not shutdown.	*/
			{
				putSysErrmsg("irecv() error on TCP socket",
						connection->destDuctName);
			}

			/*	Intentional fall-through to next case.	*/

		case 0:			/*	Connection closed.	*/
			connection->mustDelete = 1;
			rtp->running = 0;
			continue;

		default:
			break;		/*	Out of switch.		*/
		}

		msgType = (msgtypeByte >> 4) & 0x0f;
		switch (msgType);
		{
		case 0x01:
			if (handleDataSegment(rtp, msgtypeByte) < 1)
			{
				connection->mustDelete = 1;
				rtp->running = 0;
			}

			continue;

		case 0x01:
			if (handleAck(rtp, msgtypeByte) < 1)
			{
				connection->mustDelete = 1;
				rtp->running = 0;
			}

			continue;

		case 0x01:
			if (handleRefusal(rtp, msgtypeByte) < 1)
			{
				connection->mustDelete = 1;
				rtp->running = 0;
			}

			continue;

		case 0x01:
			if (handleKeepalive(rtp, msgtypeByte) < 1)
			{
				connection->mustDelete = 1;
				rtp->running = 0;
			}

			continue;

		case 0x01:
			if (handleShutdown(rtp, msgtypeByte) < 1)
			{
				connection->mustDelete = 1;
				rtp->running = 0;
			}

			continue;

		case 0x01:
			if (handleLength(rtp, msgtypeByte) < 1)
			{
				connection->mustDelete = 1;
				rtp->running = 0;
			}

			continue;

		default:
			writeMemoNote("[?] TCPCL unknown message type",
					itoa(msgType));
			connection->mustDelete = 1;
			rtp->running = 0;
		}
	}

	//	On receiving last ACK for the current outbound bundle,
	//	give the eobSemaphore.
	//	On reception of segment, reset connection->secUntilShutdown
	//	to IDLE_SHUTDOWN_INTERVAL.
	//	On reception of segment or keepalive, reset
	//	connection->secSinceReception to zero.

	writeErrmsgMemos();
	writeMemoNote("[i] tcpcli receiver thread has ended.",
			stp->connection->eid);
	bpReleaseAcqArea(work);
	MRELEASE(rtp->buffer);
	MRELEASE(rtp);
	return NULL;
}
	
/*	*	*	Connection management functions	*	*	*/

static int	beginConnection(Lyst connections, char *eid, int newSocket,
			char *destDuctName, VInduct *vduct)
{
	TcpclConnection		*connection;
	size_t			len = 0;
	LystElt			connectionElt;
	ReceiverThreadParms	*rtp;
	SenderThreadParms	*stp;

	switch (addOutduct("tcp", destDuctName, NULL, 0))
	{
	case -1:
		putErrmsg("tcpcli can't create outduct.", destDuctName);
		return -1;

	case 0:
		putErrmsg("tcpcli trying to add a duplicate outduct!",
				destDuctName);
		closesocket(newSocket);
		return 0;		/*	Implementation error.	*/

	default:
		break;			/*	Out of switch.		*/
	}

	connection = (TcpclConnection *) MTAKE(sizeof(TcpclConnection));
	if (connection == NULL)
	{
		putErrmsg("tcpcli can't allocate new connection.", NULL);
		removeOutduct("tcp", destDuctName);
		closesocket(newSocket);
		return -1;
	}

	connection->reconnectInterval = 1;
	connection->vduct = vduct;
	pthread_mutex_init(&(connection->mutex), NULL);
	if (eid)
	{
		len = istrlen(eid, MAX_EID_LEN) + 1;
		connection->eid = MTAKE(len);
		if (connection->eid == NULL)
		{
			pthread_mutex_destroy(&(connection->mutex));
			MRELEASE(connection);
			putErrmsg("tcpcli can't copy EID for new connection.",
					NULL);
			removeOutduct("tcp", destDuctName);
			closesocket(newSocket);
			return -1;
		}

		istrcpy(connection->eid, eid, len);
	}
	else
	{
		connection->eid = NULL;
	}

	connection->sock = newSocket;
	len = istrlen(destDuctName, MAX_CL_DUCT_NAME + 1) + 1;
	connection->destDuctName = MTAKE(len);
	if (connection->destDuctName == NULL)
	{
		if (connection->eid) MRELEASE(connection->eid);
		pthread_mutex_destroy(&(connection->mutex));
		MRELEASE(connection);
		putErrmsg("tcpcli can't copy socket spec for new connection.",
				NULL);
		removeOutduct("tcp", destDuctName);
		closesocket(newSocket);
		return -1;
	}

	istrcpy(connection->destDuctName, destDuctName, len);
	connectionElt = lyst_insert_last(connections, connection);
	if (connectionElt == NULL)
	{
		MRELEASE(connection->destDuctName);
		if (connection->eid) MRELEASE(connection->eid);
		pthread_mutex_destroy(&(connection->mutex));
		MRELEASE(connection);
		putErrmsg("tcpcli can't allocate lyst element for new \
connection.", NULL);
		removeOutduct("tcp", destDuctName);
		closesocket(newSocket);
		return -1;
	}

	rtp = (ReceiverThreadParms *) MTAKE(sizeof(ReceiverThreadParms));
	if (rtp == NULL)
	{
		lyst_delete(connectionElt);
		MRELEASE(connection->destDuctName);
		if (connection->eid) MRELEASE(connection->eid);
		pthread_mutex_destroy(&(connection->mutex));
		MRELEASE(connection);
		putErrmsg("tcpcli can't allocate new receiver parms.", NULL);
		removeOutduct("tcp", destDuctName);
		closesocket(newSocket);
		return -1;
	}

	rtp->running = 1;
	rtp->connections = connections;
	rtp->connection = connection;
	stp = (SenderThreadParms *) MTAKE(sizeof(SenderThreadParms));
	if (stp == NULL)
	{
		MRELEASE(rtp);
		lyst_delete(connectionElt);
		MRELEASE(connection->destDuctName);
		if (connection->eid) MRELEASE(connection->eid);
		pthread_mutex_destroy(&(connection->mutex));
		MRELEASE(connection);
		putErrmsg("tcpcli can't allocate new receiver parms.", NULL);
		removeOutduct("tcp", destDuctName);
		closesocket(newSocket);
		return -1;
	}

	stp->running = 1;
	stp->connections = connections;
	stp->connection = connection;
	connection->eobSemaphore = sm_SemCreate(SM_NO_KEY, SM_SEM_FIFO);
	if (connection->eobSemaphore == SM_SEM_NONE)
	{
		MRELEASE(stp);
		MRELEASE(rtp);
		lyst_delete(connectionElt);
		MRELEASE(connection->destDuctName);
		if (connection->eid) MRELEASE(connection->eid);
		pthread_mutex_destroy(&(connection->mutex));
		MRELEASE(connection);
		putErrmsg("tcpcli can't create connection eobSemaphore.", NULL);
		removeOutduct("tcp", destDuctName);
		closesocket(newSocket);
		return -1;
	}

	sm_SemTake(connection->eobSemaphore);		/*	Lock.	*/
	if (pthread_begin(&(connection->receiver), NULL, handleMessages, &rtp))
	{
		sm_SemDelete(connection->eobSemaphore);
		MRELEASE(stp);
		MRELEASE(rtp);
		lyst_delete(connectionElt);
		MRELEASE(connection->destDuctName);
		if (connection->eid) MRELEASE(connection->eid);
		pthread_mutex_destroy(&(connection->mutex));
		MRELEASE(connection);
		putSysErrmsg("tcpcli can't create new receiver thread", NULL);
		removeOutduct("tcp", destDuctName);
		closesocket(newSocket);
		return -1;
	}

	if (pthread_begin(&(connection->sender), NULL, sendBundles, &stp))
	{
		rtp->running = 0;
		pthread_join(connection->receiver, NULL);
		sm_SemDelete(connection->eobSemaphore);
		MRELEASE(stp);
		MRELEASE(rtp);
		lyst_delete(connectionElt);
		MRELEASE(connection->destDuctName);
		if (connection->eid) MRELEASE(connection->eid);
		pthread_mutex_destroy(&(connection->mutex));
		MRELEASE(connection);
		putSysErrmsg("tcpcli can't create new sender thread", NULL);
		removeOutduct("tcp", destDuctName);
		closesocket(newSocket);
		return -1;
	}

	if (bpUnblockOutduct("tcp", connection->destDuctName) < 0)
	{
		putErrmsg("tcpcli connected but didn't unblock.",
				connection->eid);
	}

	return 0;
}

static void	endConnection(TcpclConnection *connection, char reason)
{
	oK(blockOutduct("tcp", connection->destDuctName);
	oK(removeOutduct("tcp", connection->destDuctName);
	sm_SemEnd(connection->eobSemaphore);
	pthread_join(connection->senderThread, NULL);
	sendShutdown(connection, reason);
	closesocket(connection->sock);
	pthread_join(connection->receiverThread, NULL);
	sm_SemDelete(connection->eobSemaphore);
	MRELEASE(connection->destDuctName);
	if (connection->eid) MRELEASE(connection->eid);
	pthread_mutex_destroy(&(connection->mutex));
	MRELEASE(connection);
}

static void	shutDownConnections(Lyst connections)
{
	Sdr	sdr = getIonsdr();
	LystElt	elt;

	oK(sdr_begin_xn(sdr));
	for (elt = lyst_first(connections); elt;)
	{
		endConnection(lyst_data(elt), -1);
		lyst_delete(elt);
	}

	oK(sdr_end_xn(sdr));
}

/*	*	*	Server thread functions		*	*	*/

typedef struct
{
	int		*running;
	Lyst		connections;
	int		serverSocket;
	VInduct		*vduct;
} ServerThreadParms;

static void	*spawnReceivers(void *parm)
{
	/*	Main loop for acceptance of connections and
	 *	creation of threads to service those connections.	*/

	ServerThreadParms	*stp = (ServerThreadParms *) parm;
	Sdr			sdr = getIonsdr();
	char			*procName = "tcpcli";
	socklen_t		nameLength;
	int			newSocket;
	struct sockaddr		socketName;
	struct sockaddr_in	*inetName = (struct sockaddr_in *) &socketName;
	char			socketSpec[32];
	ReceiverThreadParms	*parms;
	LystElt			elt;
	pthread_t		thread;

	
	snooze(1);	/*	Let main thread become interruptable.	*/

	/*	Can now begin accepting connections from remote
	 *	contacts.  On failure, take down the whole CLA.		*/

	while (stp->running)
	{
		nameLength = sizeof(struct sockaddr);
		newSocket = accept(stp->serverSocket, &socketName,
				&nameLength);
		if (newSocket < 0)
		{
			putSysErrmsg("tcpcli accept() failed", NULL);
			ionKillMainThread(procName);
			stp->running = 0;
			continue;
		}

		if (stp->running == 0)
		{
			closesocket(newSocket);
			break;	/*	Main thread has shut down.	*/
		}

		isprintf(socketSpec, sizeof(socketSpec), "%s:%d.", 
				inet_ntoa(inetName->sin_addr),
				ntohs(inetName->sin_port));
		oK(sdr_begin_xn(sdr));
		if (beginConnection(stp->connections, NULL, newSocket,
				socketSpec, vduct) < 0)
		{
			sdr_cancel_xn(sdr);
			putErrmsg("tcpcli server thread can't add connection.",
					NULL);
			ionKillMainThread(procName);
			stp->running = 0;
			continue;
		}

		if (sdr_end_xn(sdr) < 0)
		{
			putErrmsg("tcpcli server thread failed new connection.",
					NULL);
			ionKillMainThread(procName);
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
	int		*running;
	Lyst		connections;
} ClockThreadParms;

static Connection	*findConnectionForEid(Lyst connections, char *eid)
{
	LystElt		elt;
	TcpclConnection	*connection;

	for (elt = lyst_first(connections); elt; elt = lyst_next(elt))
	{
		connection = (TcpConnection *) lyst_data(elt);
		if (strcmp(connection->eid, eid) == 0)
		{
			return connection;
		}
	}

	return NULL;
}

static int	beginConnectionForPlan(ClockThreadParms *ctp, char *eid,
			char *socketSpec)
{
	int	sock;

	/*	If a Connection already exists for this plan, big
	 *	problem.						*/

	if (findConnectionForEid(ctp->connections, eid) != NULL)
	{
		putErrmsg("Connection exists for plan without outduct.", eid);
		continue;
	}

	/*	Try to connect to the indicated socket.			*/

	switch (itcp_connect(socketSpec, BpTcpDefaultPortNbr, &sock))
	{
	case -1:
		putErrmsg("tcpcli can't connect to remote node.", socketSpec);
		return -1;

	case 0:
		return 0;		/*	Connection refused.	*/

	default:
		break;			/*	Out of switch.		*/
	}

	if (beginConnection(ctp->connections, eid, sock, socketSpec) < 0)
	{
		putErrmsg("tcpcli can't add new connection.", NULL);
		return -1;
	}

	return 0;			/*	Successful.		*/
}

static int	noLongerReferenced(TcpclConnection *connection, IpnDB *ipndb,
			Dtn2DB *dtn2db)
{
	Sdr	sdr = getIonsdr();
	Object	planElt;
	Object	planObj;
		OBJ_POINTER(IpnPlan, ipnPlan);
		OBJ_POINTER(Dtn2Plan, dtn2Plan);
	char	destDuctName[SDRSTRING_BUFSZ];
	Object	ruleElt;
	Object	ruleObj;
		OBJ_POINTER(IpnRule, ipnRule);
		OBJ_POINTER(Dtn2Rule, dtn2Rule);

	for (planElt = sdr_list_first(sdr, ipndb->plans); planElt;
			planElt = sdr_list_next(sdr, planElt))
	{
		planObj = sdr_list_data(sdr, planElt);
		GET_OBJ_POINTER(sdr, IpnPlan, ipnPlan, planObj);
		if (ipnPlan->defaultDirective.action == xmit
		&& ipnPlan->defaultDirective.destDuctName != 0)
		{
			sdr_string_read(sdr, destDuctName,
				ipnPlan->defaultDirective.destDuctName);
			if (strcmp(destDuctName, connection->destDuctName) == 0)
			{
				return 0;	/*	Referenced.	*/
			}
		}

		for (ruleElt = sdr_list_first(sdr, ipnPlan->rules); ruleElt;
				ruleElt = sdr_list_next(sdr, ruleElt))
		{
			ruleObj = sdr_list_data(sdr, ruleElt);
			GET_OBJ_POINTER(sdr, IpnRule, ipnRule, ruleObj);
			if (ipnRule->directive.action == xmit
			&& ipnRule->directive.destDuctName != 0)
			{
				sdr_string_read(sdr, destDuctName,
					ipnRule->directive.destDuctName);
				if (strcmp(destDuctName,
					connection->destDuctName) == 0)
				{
					return 0;	/*	Ref.	*/
				}
			}
		}
	}

	for (planElt = sdr_list_first(sdr, dtn2db->plans); planElt;
			planElt = sdr_list_next(sdr, planElt))
	{
		planObj = sdr_list_data(sdr, planElt);
		GET_OBJ_POINTER(sdr, Dtn2Plan, dtn2Plan, planObj);
		if (dtn2Plan->defaultDirective.action == xmit
		&& dtn2Plan->defaultDirective.destDuctName != 0)
		{
			sdr_string_read(sdr, destDuctName,
				dtn2Plan->defaultDirective.destDuctName);
			if (strcmp(destDuctName, connection->destDuctName) == 0)
			{
				return 0;	/*	Referenced.	*/
			}
		}

		for (ruleElt = sdr_list_first(sdr, dtn2Plan->rules); ruleElt;
				ruleElt = sdr_list_next(sdr, ruleElt))
		{
			ruleObj = sdr_list_data(sdr, ruleElt);
			GET_OBJ_POINTER(sdr, Dtn2Rule, dtn2Rule, ruleObj);
			if (dtn2Rule->directive.action == xmit
			&& dtn2Rule->directive.destDuctName != 0)
			{
				sdr_string_read(sdr, destDuctName,
					dtn2Rule->directive.destDuctName);
				if (strcmp(destDuctName,
					connection->destDuctName) == 0)
				{
					return 0;	/*	Ref.	*/
				}
			}
		}
	}

	return 1;			/*	Plan not found.		*/
}

static int	rescan(ClockThreadParms *ctp, IpnDB *ipndb, Dtn2DB *dtn2db)
{
	Sdr		sdr = getIonsdr();
	char		eid[SDRSTRING_BUFSZ];
	Object		planElt;
	Object		planObj;
			OBJ_POINTER(IpnPlan, ipnPlan);
			OBJ_POINTER(Dtn2Plan, dtn2Plan);
	Object		ruleElt;
	Object		ruleObj;
			OBJ_POINTER(IpnRule, ipnRule);
			OBJ_POINTER(Dtn2Rule, dtn2Rule);
	LystElt		elt;
	LystElt		nextElt;
	VInduct		*vduct;
	PsmAddress	vductElt;

	/*	First look for newly added ipnfw plans or rules and try
	 *	to create connections for them.				*/

	for (planElt = sdr_list_first(sdr, ipndb->plans); planElt;
			planElt = sdr_list_next(sdr, planElt))
	{
		planObj = sdr_list_data(sdr, planElt);
		GET_OBJ_POINTER(sdr, IpnPlan, ipnPlan, planObj);
		isprintf(eid, sizeof eid, "ipn:" UVAST_FIELDSPEC, ".0",
				ipnPlan->nodeNbr);
		if (ipnPlan->defaultDirective.action == xmit
		&& ipnPlan->defaultDirective.outductElt == 0)
		{
			/*	This is an ipn plan for xmit via a
			 *	TCP outduct that doesn't exist yet.	*/

			if (beginConnectionForPlan(ctp, eid,
				ipnPlan->defaultDirective.destDuctName) < 0)
			{
				return -1;
			}

			continue;	/*	Move on to next plan.	*/
		}

		/*	Plan's default directive doesn't need a TCP
		 *	outduct, but one of its rules might.		*/

		for (ruleElt = sdr_list_first(sdr, ipnPlan->rules); ruleElt;
				ruleElt = sdr_list_next(sdr, ruleElt))
		{
			ruleObj = sdr_list_data(sdr, ruleElt);
			GET_OBJ_POINTER(sdr, IpnRule, ipnRule, ruleObj);
			if (ipnRule->directive.action == xmit
			&& ipnPlan->directive.outductElt == 0)
			{
				/*	This is an ipn rule for xmit
				 *	via a TCP outduct that doesn't
				 *	exist yet.			*/

				if (beginConnectionForPlan(ctp, eid,
					ipnRule->directive.destDuctName) < 0)
				{
					return -1;
				}

				break;	/*	Out of rules loop.	*/
			}
		}
	}

	/*	Next look for newly added dtn2fw plans or rules and try
	 *	to create connections for them.				*/

	for (planElt = sdr_list_first(sdr, dtn2db->plans); planElt;
			planElt = sdr_list_next(sdr, planElt))
	{
		/*	If plan's default directive isn't xmit via
		 *	an outduct of the TCPCL protocol that does
		 *	not yet exist, skip it.				*/

		planObj = sdr_list_data(sdr, planElt);
		GET_OBJ_POINTER(sdr, Dtn2Plan, dtn2Plan, planObj);
		oK(sdr_string_read(sdr, eid, dtn2Plan->nodeName)); 
		if (dtn2Plan->defaultDirective.action == xmit
		&& dtn2Plan->defaultDirective.outductElt == 0)
		{
			/*	This is a dtn2 plan for xmit via a
			 *	TCP outduct that doesn't exist yet.	*/

			if (beginConnectionForPlan(ctp, eid,
				dtn2Plan->defaultDirective.destDuctName) < 0)
			{
				return -1;
			}

			continue;	/*	Move on to next plan.	*/
		}

		/*	Plan's default directive doesn't need a TCP
		 *	outduct, but one of its rules might.		*/

		for (ruleElt = sdr_list_first(sdr, dtn2Plan->rules); ruleElt;
				ruleElt = sdr_list_next(sdr, ruleElt))
		{
			ruleObj = sdr_list_data(sdr, ruleElt);
			GET_OBJ_POINTER(sdr, Dtn2Rule, dtn2Rule, ruleObj);
			if (dtn2Rule->directive.action == xmit
			&& dtn2Plan->directive.outductElt == 0)
			{
				/*	This is a dtn2 rule for xmit
				 *	via a TCP outduct that doesn't
				 *	exist yet.			*/

				if (beginConnectionForPlan(ctp, eid,
					dtn2Rule->directive.destDuctName) < 0)
				{
					return -1;
				}

				break;	/*	Out of rules loop.	*/
			}
		}
	}

	/*	Now look for connections that must be ended because
	 *	the plans/rules referencing them have been removed.	*/

	for (elt = lyst_first(ctp->connections); elt; elt = nextElt)
	{
		nextElt = lyst_next(elt);
		connection = (TcpclConnection *) lyst_data(elt);
		findOutduct("tcp", connection->destDuctName, &vduct, &vductElt);
		if (vductElt == 0)	/*	Outduct deleted.	*/
		{
			endConnection(connection, -1);
			lyst_delete(elt);
			continue;
		}

		/*	Outduct still exists.  Is there a plan or
		 *	rule that references this connection?		*/

		if (noLongerReferenced(connection, ipndb, dtn2db))
		{
			endConnection(connection, -1);
			lyst_delete(elt);
			continue;
		}
	}

	return 0;
}

static int	sendKeepalive(TcpclConnection *connection)
{
	int		result;
	static char	keepalive = 0x40;

	if (connection->sock == -1)
	{
		result = reconnect(connection);
		if (result != 1)
		{
			return result;
		}
	}

	pthread_mutex_lock(&(connection->mutex));
	result = itcp_send(connection->sock, &keepalive, 1);
	pthread_mutex_unlock(&(connection->mutex));
	if (result != 0)	/*	Success or system failure.	*/
	{
		return result;
	}

	/*	Lost TCP connection.					*/

	connection->sock = -1;
	connection->secUntilReconnect = connection->reconnectInterval;
	return 0;
}

static void	*handleEvents(void *parm)
{
	/*	Main loop for acceptance of connections and
	 *	creation of threads to service those connections.	*/

	ClockThreadParms	*ctp = (ClockThreadParms *) parm;
	Sdr			sdr = getIonsdr();
	IpnDB			*ipndb = getIpnConstants();
	DtnDB			*dtn2db = getDtnConstants();
	time_t			planChangeTime;
	time_t			lastIpnPlanChange = 0;
	time_t			lastDtn2PlanChange = 0;
	int			secUntilRescan = 1;
	LystElt			elt;
	LystElt			nextElt;

	while (ctp->running)
	{
		oK(sdr_begin_xn(sdr));
		planChangeTime = sdr_list_user_data(sdr, ipndb->plans);
		if (planChangeTime > lastIpnPlanChange)
		{
			secUntilRescan = 1;
			lastIpnPlanChange = planChangeTime;
		}

		planChangeTime = sdr_list_user_data(sdr, dtn2db->plans);
		if (planChangeTime > lastDtn2PlanChange)
		{
			secUntilRescan = 1;
			lastDtn2PlanChange = planChangeTime;
		}

		secUntilRescan--;
		if (secUntilRescan == 0)
		{
			if (rescan(ctp, ipndb, dtn2db) < 0)
			{
				sdr_cancel_xn(sdr);
				ctp->running = 0;
				continue;
			}

			secUntilRescan = TCPCL_RESCAN_INTERVAL;
		}

		/*	Now decrement timers for all connections
		 *	and handle resulting events.			*/

		for (elt = lyst_first(ctp->connections); elt; elt = nextElt)
		{
			nextElt = lyst_next(elt);
			connection = (TcpclConnection *) lyst_data(elt);
			if (connection>mustDelete)
			{
				endConnection(connection, -1);
				lyst_delete(elt);
				continue;
			}

			/*	Track idleness in bundle traffic.	*/

			if (connection->secUntilShutdown > 0)
			{
				connection->secUntilShutdown--;
			}

			if (connection->secUntilShutdown == 0)
			{
				endConnection(connection, -1);
				lyst_delete(elt);
				continue;
			}

			/*	Track idleness in reception.		*/

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
						lyst_delete(elt);
						continue;
					}
				}
			}

			/*	Count down to reconnection.		*/

			if (connection->secUntilReconnect > 0)
			{
				connection->secUntilReconnect--;
			}

			/*	Track sending of keepalives.		*/

			if (connection->secUntilKeepalive > 0)
			{
				connection->secUntilKeepalive--;
			}

			if (connection->secUntilKeepalive == 0)
			{
				if (sendKeepalive(connection) < 0)
				{
					ctp->running = 0;
					nextElt = NULL;
					continue;
				}

				connection->secUntilKeepalive =
						connection->keepaliveInterval;
			}
		}

		if (sdr_end_xn(sdr) < 0)
		{
			putErrmsg("tcpcli clock failed.", NULL);
			ctp->running = 0;
			continue;
		}

		snooze(1);
	}

	writeErrmsgMemos();
	writeMemo("[i] tcpcli clock thread has ended.");
	return NULL;
}

/*	*	*	Main thread functions		*	*	*/

static void	stopServerThread(pthread_t serverThread, ServerThreadParms *stp)
{
	int	sock;

	/*	Wake up the server thread by connecting to it.		*/

	stp->running = 0;
	sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock >= 0)
	{
		oK(connect(sock, &(stp->socketName), sizeof(struct sockaddr)));

		/*	Immediately discard the connected socket.	*/

		closesocket(sock);
	}

	pthread_join(serverThread, NULL);
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
	Sdr			sdr;
	Induct			duct;
	unsigned short		portNbr;
	unsigned int		hostNbr;
	struct sockaddr		socketName;
	struct sockaddr_in	*inetName;
	ServerThreadParms	stp;
	socklen_t		nameLength;
	Lyst			connections;
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
		writeMemo("[?] tcpcli can't attach to BP.");
		return 1;
	}

	if (parseSocketSpec(ductName, &portNbr, &hostNbr) != 0)
	{
		writeMemo("[?] tcpcli: can't get induct IP address.", ductName);
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
	 *	as connections are made.  None are created during
	 *	node configuration.					*/

	if (vduct->cliPid != ERROR && vduct->cliPid != sm_TaskIdSelf())
	{
		writeMemoNote("[?] tcpcli task is already started.",
				itoa(vduct->cliPid));
		return 1;
	}

	/*	All command-line arguments are now validated, so
		begin initialization by creating the connections lyst.	*/

	stp.vduct = vduct;
	connections = lyst_create();
	if (connections == NULL)
	{
		putErrmsg("tcpcli can't create lyst of connections", NULL);
		return 1;
	}

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
		lyst_destroy(connections);
		return 1;
	}

	nameLength = sizeof(struct sockaddr);
	if (reUseAddress(stp.serverSocket)
	|| bind(stp.serverSocket, &socketName, nameLength) < 0
	|| listen(stp.serverSocket, 5) < 0
	|| getsockname(stp.serverSocket, &socketName, &nameLength) < 0)
	{
		closesocket(stp.serverSocket);
		lyst_destroy(connections);
		putSysErrmsg("Can't initialize TCP server socket", NULL);
		return 1;
	}

	/*	Set up signal handling: SIGTERM is shutdown signal.	*/

	ionNoteMainThread("tcpcli");
	isignal(SIGTERM, interruptThread);
#ifndef mingw
	isignal(SIGPIPE, SIG_IGN);
#endif

	/*	Start the clock thread, which does initial load
	 *	of the connections lyst.				*/

	ctp.running = 1;
	if (pthread_begin(&clockThread, NULL, handleEvents, &ctp))
	{
		closesocket(stp.serverSocket);
		lyst_destroy(connections);
		putSysErrmsg("tcpcli can't create clock thread", NULL);
		return 1;
	}

	/*	Start the server thread.				*/

	stp.running = 1;
	stp.connections = connections;
	if (pthread_begin(&serverThread, NULL, spawnReceivers, &stp))
	{
		shutDownConnections(connections);
		ctp.running = 0;
		pthread_join(clockThread, NULL);
		closesocket(stp.serverSocket);
		lyst_destroy(connections);
		putSysErrmsg("tcpcli can't create server thread", NULL);
		return 1;
	}

	/*	Now sleep until interrupted by SIGTERM, at which point
	 *	it's time to stop the CLA.				*/

	{
		char	txt[500];

		isprintf(txt, sizeof(txt),
				"[i] tcpcli is running, [%s:%d].", 
				inet_ntoa(inetName->sin_addr),
				ntohs(inetName->sin_port));
		writeMemo(txt);
	}

	ionPauseMainThread(-1);

	/*	Time to shut down.					*/

	stopServerThread(serverThread, &stp);
	shutDownConnections(connections);
	ctp.running = 0;
	pthread_join(clockThread, NULL);
	closesocket(stp.serverSocket);
	lyst_destroy(connections);
	writeErrmsgMemos();
	writeMemo("[i] tcpcli has ended.");
	bp_detach();
	return 0;
}
--------------------------------------------------

/*	*	*	Receiver thread functions	*	*	*/

typedef struct
{
	LystElt		elt;
	struct sockaddr	cloSocketName;
	pthread_mutex_t	*mutex;
	int		bundleSocket;
	pthread_t	thread;
	int		*cliRunning;
        int             receiveRunning;
} ReceiverThreadParms;

static void	terminateReceiverThread(ReceiverThreadParms *parms)
{
	writeErrmsgMemos();
	writeMemo("[i] tcpcli receiver thread stopping.");
	pthread_mutex_lock(parms->mutex);
	if (parms->bundleSocket != -1)
	{
		closesocket(parms->bundleSocket);
		parms->bundleSocket = -1;
	}

	lyst_delete(parms->elt);
	pthread_mutex_unlock(parms->mutex);
}

static void	*receiveBundles(void *parm)
{
	/*	Main loop for bundle reception thread on one
	 *	connection, terminating when connection is lost.	*/

	ReceiverThreadParms	*parms = (ReceiverThreadParms *) parm;
	char			*procName = "tcpcli";
	KeepaliveThreadParms	*kparms;
	AcqWorkArea		*work;
	char			*buffer;
	pthread_t		kthread;
	int			haveKthread = 0;
	int			threadRunning = 1;

	buffer = MTAKE(TCPCLA_BUFSZ);
	if (buffer == NULL)
	{
		putErrmsg("tcpcli can't get TCP buffer", NULL);
		terminateReceiverThread(parms);
		MRELEASE(parms);
		ionKillMainThread(procName);
		return NULL;
	}

	work = bpGetAcqArea(vduct);
	if (work == NULL)
	{
		putErrmsg("tcpcli can't get acquisition work area", NULL);
		MRELEASE(buffer);
		terminateReceiverThread(parms);
		MRELEASE(parms);
		ionKillMainThread(procName);
		return NULL;
	}
	
	/*	Set up parameters for the keepalive thread	*/

	kparms = (KeepaliveThreadParms *)
			MTAKE(sizeof(KeepaliveThreadParms));
	if (kparms == NULL)
	{
		putErrmsg("tcpcli can't allocate for new keepalive thread",
				NULL);
		MRELEASE(buffer);
		bpReleaseAcqArea(work);
		terminateReceiverThread(parms);
		MRELEASE(parms);
		ionKillMainThread(procName);
		return NULL;
	}

	pthread_mutex_lock(parms->mutex);

	/*	Making sure there is no race condition when keep alive
	 *	values are set.						*/

	if (sendContactHeader(&parms->bundleSocket, (unsigned char *) buffer)
			< 0)
	{
		putErrmsg("tcpcli couldn't send contact header", NULL);
		MRELEASE(buffer);
		MRELEASE(kparms);
		closesocket(parms->bundleSocket);
		parms->bundleSocket = -1;
		pthread_mutex_unlock(parms->mutex);
		bpReleaseAcqArea(work);
		terminateReceiverThread(parms);
		MRELEASE(parms);
		return NULL;
	}

	pthread_mutex_unlock(parms->mutex);
	if (receiveContactHeader(&parms->bundleSocket, (unsigned char *) buffer,
			&kparms->keepalivePeriod) < 0)
	{
		putErrmsg("tcpcli couldn't receive contact header", NULL);
		MRELEASE(buffer);
		MRELEASE(kparms);
		pthread_mutex_lock(parms->mutex);
		closesocket(parms->bundleSocket);
		parms->bundleSocket = -1;
		pthread_mutex_unlock(parms->mutex);
		bpReleaseAcqArea(work);
		terminateReceiverThread(parms);
		MRELEASE(parms);
		return NULL;
	}

	/*
	 * The keep alive thread is created only if the negotiated
	 * keep alive is greater than 0.
	 */

	if( kparms->keepalivePeriod > 0 )
	{
		kparms->ductSocket = parms->bundleSocket;
		kparms->mutex = parms->mutex;
		kparms->cliRunning = parms->cliRunning;
                kparms->receiveRunning = &(parms->receiveRunning);
		/*
		 * Creating a thread to send out keep alives, which
		 * makes the TCPCL bi directional
		 */
		if (pthread_begin(&kthread, NULL, sendKeepalives, kparms))
		{
			putSysErrmsg("tcpcli can't create new thread for \
keepalives", NULL);
			ionKillMainThread(procName);
			threadRunning = 0;
		}
		else
		{
			haveKthread = 1;
		}
	}

	/*	Now start receiving bundles.				*/

	while (threadRunning && *(parms->cliRunning) && parms->receiveRunning)
	{
		if (bpBeginAcq(work, 0, NULL) < 0)
		{
			putErrmsg("Can't begin acquisition of bundle.", NULL);
			ionKillMainThread(procName);
			threadRunning = 0;
			continue;
		}

		switch (receiveBundleByTcpCL(parms->bundleSocket, work, buffer))
		{
		case -1:
			putErrmsg("Can't acquire bundle.", NULL);

			/*	Intentional fall-through to next case.	*/

		case 0:				/*	Normal stop.	*/
			threadRunning = 0;
			continue;

		default:
			break;			/*	Out of switch.	*/
		}

		if (bpEndAcq(work) < 0)
		{
			putErrmsg("Can't end acquisition of bundle.", NULL);
			ionKillMainThread(procName);
			threadRunning = 0;
			continue;
		}

		/*	Make sure other tasks have a chance to run.	*/

		sm_TaskYield();
	}

	/*	End of receiver thread; release resources.		*/

	parms->receiveRunning = 0;
	if (haveKthread)
	{
		/*	Wait for keepalive snooze cycle to notice that
		 *	*(parms->cliRunning) or *(parms->receiveRunning)
		 *	is now zero.					*/

		pthread_join(kthread, NULL);
	}

	bpReleaseAcqArea(work);
	MRELEASE(buffer);
	terminateReceiverThread(parms);
	MRELEASE(kparms);
	MRELEASE(parms);
	return NULL;
}
