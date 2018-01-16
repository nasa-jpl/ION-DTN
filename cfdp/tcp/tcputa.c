/*
	tcputa.c:	UT-layer adapter using TCP/IP.
									*/
/*									*/
/*	Copyright (c) 2014, California Institute of Technology.		*/
/*	All rights reserved.						*/
/*	Author: Scott Burleigh, Jet Propulsion Laboratory		*/
/*									*/

#include <cfdpP.h>

typedef struct
{
	unsigned int	entityId;
	unsigned int	ipAddress;
	unsigned short	portNbr;
} EntityAddr;

typedef struct
{
	pthread_t	mainThread;
	int		recvSocket;
	int		running;
} RxThreadParms;

/*	*	*	Utility functions	*	*	*	*/

static int	lookUpEntity(unsigned int entityId, unsigned int *ipAddress,
			unsigned short *portNbr)
{
	Sdr	sdr = getIonsdr();
	Entity	entity;
	int	result;

	CHKZERO(sdr_begin_xn(sdr));	/*	Just to lock database.	*/
	if (findEntity(entityId, &entity) != 0 && entity.utLayer == UtTcp)
	{
		*ipAddress = entity.ipAddress;
		*portNbr = entity.portNbr;
		result = 1;
	}
	else
	{
		result = 0;
	}

	sdr_exit_xn(sdr);
	return result;
}

/*	*	*	Receiver thread functions	*	*	*/

static int	openAccessSocket()
{
	unsigned int		ipAddress;
	unsigned short		portNbr;
	struct sockaddr		socketName;
	struct sockaddr_in	*inetName = (struct sockaddr_in *) &socketName;
	int			accessSocket;
	socklen_t		nameLength = sizeof(struct sockaddr);

	if (lookUpEntity(getCfdpConstants()->ownEntityId, &ipAddress,
			&portNbr) == 0)
	{
		putErrmsg("tcputa can't get own socket spec.", NULL);
		return -1;
	}

	ipAddress = htonl(ipAddress);
	portNbr = htons(portNbr);
	memset((char *) &socketName, 0, sizeof(struct sockaddr));
	inetName->sin_family = AF_INET;
	inetName->sin_port = portNbr;
	memcpy((char *) &(inetName->sin_addr.s_addr), (char *) &ipAddress, 4);
	accessSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (accessSocket < 0)
	{
		putSysErrmsg("tcputa can't open TCP socket", NULL);
		return -1;
	}

	if (reUseAddress(accessSocket) < 0)
	{
		putSysErrmsg("tcputa can't make TCP socket reusable", NULL);
		closesocket(accessSocket);
		return -1;
	}

	if (bind(accessSocket, &socketName, nameLength) < 0)
	{
		putSysErrmsg("tcputa can't bind TCP socket", NULL);
		closesocket(accessSocket);
		return -1;
	}

	if (listen(accessSocket, 5) < 0)
	{
		putSysErrmsg("tcputa can't listen on TCP socket", NULL);
		closesocket(accessSocket);
		return -1;
	}

	return accessSocket;
}

static void	*receivePdus(void *parm)
{
	RxThreadParms		*parms = (RxThreadParms *) parm;
	unsigned char		*buffer;
	int			accessSocket;
	struct sockaddr		socketName;
	socklen_t		nameLength;
	int			result;
	int			dataLength;
	int			entityNbrLength;
	int			transactionNbrLength;
	int			remainingPduLength;

	buffer = MTAKE(CFDP_MAX_PDU_SIZE);
	if (buffer == NULL)
	{
		putErrmsg("tcputa receiver thread can't get buffer.", NULL);
		parms->running = 0;
		return NULL;
	}

	accessSocket = openAccessSocket();
	if (accessSocket < 0)
	{
		MRELEASE(buffer);
		putErrmsg("tcputa receiver thread can't open socket.", NULL);
		parms->running = 0;
		return NULL;
	}

	writeMemo("[i] tcputa input has started.");

	/*	We accept and process one connection at a time.  When
	 *	the connection is ended, we can accept a new connection
	 *	from another entity.					*/

	while (parms->running)
	{
		nameLength = sizeof(struct sockaddr);
		parms->recvSocket = accept(accessSocket, &socketName,
				&nameLength);
		if (parms->recvSocket < 0)
		{
			putSysErrmsg("tcputa failed to accept connection",
					NULL);
			parms->running = 0;
			continue;
		}

		if (parms->running == 0)
		{
			break;	/*	Main thread has shut down.	*/
		}

		/*	Receive one CFDP at a time.  PDUs are self-
		 *	delimiting.					*/

		while (parms->running)
		{
			/*	Get fixed-length portion of header.	*/

			result = itcp_recv(&(parms->recvSocket),
					(char *) buffer, 4);
			if (result < 4)		/*	Disconnect.	*/
			{
				if (result < 0)	/*	Must stop.	*/
				{
					parms->running = 0;
				}

				break;	/*	Out of PDU loop.	*/
			}

			dataLength = (buffer[1] << 8) + buffer[2];
			entityNbrLength = ((buffer[3] >> 4) & 0x07) + 1;
			transactionNbrLength = ((buffer[3]) & 0x07) + 1;
			remainingPduLength = entityNbrLength
					+ transactionNbrLength
					+ entityNbrLength
					+ dataLength;

			/*	Get remainder of PDU.			*/

			result = itcp_recv(&(parms->recvSocket),
					((char *) buffer) + 4,
					remainingPduLength);
			if (result < remainingPduLength)
			{
				if (result < 0)	/*	Must stop.	*/
				{
					parms->running = 0;
				}

				break;	/*	Out of PDU loop.	*/
			}

			/*	Pass the received PDU to CFDP.		*/

			if (cfdpHandleInboundPdu(buffer,
					4 + remainingPduLength) < 0)
			{
				putErrmsg("tcputa can't handle inbound PDU.",
						NULL);
				parms->running = 0;
				break;	/*	Out of PDU loop.	*/
			}

			/*	Make sure other tasks have a chance
			 *	to run.					*/

			sm_TaskYield();
		}

		/*	The recv socket is, at minimum, disconnected.	*/

		closesocket(parms->recvSocket);
		parms->recvSocket = -1;

		/*	Ready to accept next connection.		*/
	}

	closesocket(accessSocket);
	MRELEASE(buffer);
	writeMemo("[i] tcputa input thread has stopped.");
	return NULL;
}

/*	*	*	Main thread functions	*	*	*	*/

static int	connectToPeerEntity(uvast destinationEntityNbr,
			uvast *currentPeerEntity, int *xmitSocket)
{
	unsigned int		ipAddress;
	unsigned short		portNbr;
	struct sockaddr		socketName;
	struct sockaddr_in	*inetName = (struct sockaddr_in *) &socketName;

	if (lookUpEntity(destinationEntityNbr, &ipAddress, &portNbr) == 0)
	{
		writeMemoNote("[?] tcputa has no address for this entity",
				itoa(destinationEntityNbr));
		return 0;
	}

	if (*xmitSocket != -1)
	{
		*currentPeerEntity = 0;
		closesocket(*xmitSocket);
	}

	*xmitSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (*xmitSocket < 0)
	{
		putSysErrmsg("tcputa can't open TCP socket", NULL);
		return -1;
	}

	portNbr = htons(portNbr);
	ipAddress = htonl(ipAddress);
	memset((char *) &socketName, 0, sizeof(struct sockaddr));
	inetName->sin_family = AF_INET;
	inetName->sin_port = portNbr;
	memcpy((char *) &(inetName->sin_addr.s_addr), (char *) &ipAddress, 4);
	while (1)
	{
		if (connect(*xmitSocket, &socketName, sizeof(struct sockaddr))
				== 0)
		{
			*currentPeerEntity = destinationEntityNbr;
			return 0;
		}

		/*	Connection failed.				*/

		switch (errno)
		{
		case EINTR:	/*	Interrupted; try again.		*/
			continue;

		case ECONNREFUSED:
		case ENETUNREACH:
		case EBADF:
		case ETIMEDOUT:
			writeMemoNote("[?] tcputa can't connect to this entity",
					itoa(destinationEntityNbr));
			closesocket(*xmitSocket);
			*xmitSocket = -1;
			return 0;

		default:
			putSysErrmsg("tcputa connection failed",
					itoa(destinationEntityNbr));
			closesocket(*xmitSocket);
			*xmitSocket = -1;
			return -1;
		}
	}
}

static int	deletePdu(Object pduZco)
{
	Sdr	sdr = getIonsdr();

	if (sdr_begin_xn(sdr) == 0)
	{
		putErrmsg("tcputa can't delete PDU; terminated.", NULL);
		return -1;
	}

	zco_destroy(sdr, pduZco);
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("tcputa can't delete PDU; terminated.", NULL);
		return -1;
	}

	return 0;
}

#if defined (VXWORKS) || defined (RTEMS) || defined (bionic)
int	tcputa(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
#else
int	main(int argc, char **argv)
	
{
#endif
	RxThreadParms		parms;
	unsigned char		*buffer;
	pthread_t		rxThread;
	int			haveRxThread = 0;
	Sdr			sdr;
	Object			pduZco;
	OutFdu			fduBuffer;
	FinishPdu		fpdu;
	int			direction;
	uvast			currentPeerEntity = 0;
	uvast			destinationEntityNbr = 0;
	int			xmitSocket = -1;
	vast			pduLength;
	ZcoReader		reader;
	int			result;
	vast			bytesToSend;

	if (cfdpAttach() < 0)
	{
		putErrmsg("tcputa can't attach to CFDP.", NULL);
		return 0;
	}

	parms.mainThread = pthread_self();
	buffer = MTAKE(CFDP_MAX_PDU_SIZE);
	if (buffer == NULL)
	{
		putErrmsg("tcputa sender thread can't get buffer.", NULL);
		return -1;
	}

	parms.recvSocket = -1;
	parms.running = 1;
	if (pthread_begin(&rxThread, NULL, receivePdus, &parms))
	{
		putSysErrmsg("tcputa can't create receiver thread", NULL);
		MRELEASE(buffer);
		return -1;
	}

	haveRxThread = 1;
	writeMemo("[i] tcputa is running.");
	sdr = getIonsdr();
	while (parms.running)
	{
		/*	Get an outbound CFDP PDU for transmission.	*/

		if (cfdpDequeueOutboundPdu(&pduZco, &fduBuffer, &fpdu,
					&direction) < 0)
		{
			writeMemo("[?] tcputa can't dequeue outbound CFDP PDU; \
terminating.");
			parms.running = 0;
			continue;
		}

		if (direction == 0)	/*	Forward.		*/
		{
			cfdp_decompress_number(&destinationEntityNbr,
					&fduBuffer.destinationEntityNbr);
		}
		else			/*	Return (Finished).	*/
		{
			cfdp_decompress_number(&destinationEntityNbr,
					&fpdu.transactionId.sourceEntityNbr);
		}

		if (destinationEntityNbr == 0)
		{
			writeMemo("[?] tcputa declining to send to entity 0.");
			if (deletePdu(pduZco) < 0)
			{
				parms.running = 0;
			}

			continue;
		}

		if (destinationEntityNbr != currentPeerEntity)
		{
			if (connectToPeerEntity(destinationEntityNbr,
					&currentPeerEntity, &xmitSocket) < 0)
			{
				putErrmsg("tcputa connection failure.", NULL);
				parms.running = 0;
				continue;
			}

			if (currentPeerEntity != destinationEntityNbr)
			{
				/*	Connection did not succeed.	*/

				if (deletePdu(pduZco) < 0)
				{
					parms.running = 0;
				}

				continue;
			}
		}

		/*	Copy bytes of PDU into buffer for transmission.	*/

		pduLength = zco_length(sdr, pduZco);
		if (pduLength == 0 || pduLength > CFDP_MAX_PDU_SIZE)
		{
			putErrmsg("PDU length is invalid.", itoa(pduLength));
			if (deletePdu(pduZco) < 0)
			{
				parms.running = 0;
			}

			continue;
		}

		zco_start_transmitting(pduZco, &reader);
		zco_track_file_offset(&reader);
		CHKZERO(sdr_begin_xn(sdr));
		bytesToSend = zco_transmit(sdr, &reader, pduLength,
				(char *) buffer);
		zco_destroy(sdr, pduZco);
		if (sdr_end_xn(sdr) < 0 || bytesToSend != pduLength)
		{
			putErrmsg("tcputa can't issue PDU from ZCO.", NULL);
			parms.running = 0;
			continue;
		}

		result = itcp_send(&xmitSocket, (char *) buffer, bytesToSend);
		if (result < bytesToSend)
		{
			if (result < 0)		/*	Must stop.	*/
			{
				parms.running = 0;
			}

			/*	At minimum, connection is lost.		*/

			closesocket(xmitSocket);
			xmitSocket = -1;
			currentPeerEntity = 0;
		}

		sm_TaskYield();
	}

	if (currentPeerEntity)
	{
		closesocket(xmitSocket);
		xmitSocket = -1;
		currentPeerEntity = 0;
	}

	if (haveRxThread)
	{
		if (parms.recvSocket < 0)
		{
			/*	Not currently connected; wake up the
			 *	access thread by connecting to it.	*/

			destinationEntityNbr = getCfdpConstants()->ownEntityId; 
			if (connectToPeerEntity(destinationEntityNbr,
					&currentPeerEntity, &xmitSocket) < 0)
			{
				putErrmsg("tcputa shutdown failure.", NULL);
			}
			else
			{
				/*	Immediately discard the
				 *	connected socket, if any.	*/

				if (currentPeerEntity == destinationEntityNbr)
				{
					closesocket(xmitSocket);
				}
			}
		}
		else	/*	Currently connected.			*/
		{
#ifdef mingw
			shutdown(parms.recvSocket, SD_BOTH);
#else
			pthread_kill(rxThread, SIGTERM);
#endif
		}

		pthread_join(rxThread, NULL);
	}

	MRELEASE(buffer);
	writeErrmsgMemos();
	writeMemo("[i] Stopping tcputa.");
	ionDetach();
	return 0;
}
