/*
 *	libtcpbsa.c:	common functions for TCP BSSP-based
 *			link service adapter modules.
 *
 *	Authors: Sotirios-Angelos Lenas, SPICE
 *
 *	Copyright (c) 2013, California Institute of Technology.
 *	Copyright (c) 2013, Space Internetworking Center,
 *	Democritus University of Thrace.
 *	
 *	All rights reserved. U.S. Government and E.U. Sponsorship acknowledged.
 */

#include "tcpbsa.h"

int	tcpDelayEnabled = 0;
int	tcpDelayNsecPerByte = 0;

/*	*	*	Sender functions	*	*	*	*/

#ifndef mingw
void	handleConnectionLoss()
{
	isignal(SIGPIPE, handleConnectionLoss);
}
#endif

int	connectToBSI(struct sockaddr *sn, int *sock)
{
	*sock = -1;
	if (sn == NULL)
	{

		return -1;	/*	Silently give up on connection.	*/
	}

	*sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (*sock < 0)
	{
		putSysErrmsg("BSO can't open TCP socket", NULL);
		return -1;
	}

	if (connect(*sock, sn, sizeof(struct sockaddr)) < 0)
	{
		closesocket(*sock);
		*sock = -1;
		putSysErrmsg("BSO can't connect to TCP socket", NULL);
		return -1;
	}

	return 0;
}

int	sendBytesByTCP(int *blockSocket, char *from, int length,
		struct sockaddr *sn)
{
	int	bytesWritten;

	while (1)	/*	Continue until not interrupted.		*/
	{
		bytesWritten = isend(*blockSocket, from, length, 0);
		if (bytesWritten < 0)
		{
			switch (errno)
			{
			case EINTR:	/*	Interrupted; retry.	*/
				continue;

			case EPIPE:	/*	Lost connection.	*/
			case EBADF:
			case ETIMEDOUT:
			case ECONNRESET:
				closesocket(*blockSocket);
				*blockSocket = -1;
				bytesWritten = 0;
			}

			putSysErrmsg("TCP BSO write() error on socket", NULL);
		}

		return bytesWritten;
	}
}

int	sendBlockByTCP(struct sockaddr *socketName, int *blockSocket,
		int blockLength, char *block)
{
	int	totalBytesSent = 0;
	int	header = htonl(blockLength);
	int	bytesRemaining;
	char	*from;
	int	bytesSent;

	/*	Connect to BSI as necessary.				*/

	if (*blockSocket < 0)
	{
		if (connectToBSI(socketName, blockSocket) < 0)
		{
			/*	Treat I/O error as a transient anomaly.	*/

			return 0;
		}
	}

	bytesRemaining = 4;
	from = (char *) &header;
	while (bytesRemaining > 0)
	{
		bytesSent = sendBytesByTCP(blockSocket, from, bytesRemaining,
				socketName);
		if (bytesSent < 0)
		{
			/*	Big problem; shut down.			*/

			putErrmsg("Failed to send preamble by TCP.", NULL);
			return -1;
		}

		if (*blockSocket == -1)
		{
			/*	Just lost connection; treat as a
			 *	transient anomaly, note incomplete
			 *	transmission.				*/

			writeMemo("[?] Lost connection to TCP BSI.");
			return 0;
		}

		bytesRemaining -= bytesSent;
		from += bytesSent;
	}

	if (blockLength == 0)		/*	Just a keep-alive.	*/
	{
		return 1;	/*	Impossible length; means "OK".	*/
	}

	bytesRemaining = blockLength;
	from = block;
	while (bytesRemaining > 0)
	{
		bytesSent = sendBytesByTCP(blockSocket, from, bytesRemaining,
				socketName);
		if (bytesSent < 0)
		{
			/*	Big problem; shut down.			*/

			putErrmsg("Failed to send block by TCP.", NULL);
			return -1;
		}

		if (*blockSocket == -1)
		{
			/*	Just lost connection; treat as a
			 *	transient anomaly, note incomplete
			 *	transmission.				*/

			writeMemo("[?] Lost connection to TCP BSI.");
			return 0;
		}

		totalBytesSent += bytesSent;
		from += bytesSent;
		bytesRemaining -= bytesSent;
	}

	return totalBytesSent;
}
