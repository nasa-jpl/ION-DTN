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

int	sendBlockByTCP(struct sockaddr *socketName, int *blockSocket,
		int blockLength, char *block)
{
	int	header = htonl(blockLength);
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

	bytesSent = itcp_send(blockSocket, (char *) &header, 4);
	if (bytesSent < 0)
	{
		/*	Big problem; shut down.				*/

		putErrmsg("Failed to send preamble by TCP.", NULL);
		return -1;
	}

	if (bytesSent == 0)
	{
		/*	Just lost connection; treat as a transient
		 *	anomaly, note incomplete transmission.		*/

		writeMemo("[?] Lost connection to TCP BSI.");
		closesocket(*blockSocket);
		return 0;
	}

	if (blockLength == 0)		/*	Just a keep-alive.	*/
	{
		return 1;	/*	Impossible length; means "OK".	*/
	}

	bytesSent = itcp_send(blockSocket, block, blockLength);
	if (bytesSent < 0)
	{
		/*	Big problem; shut down.			*/

		putErrmsg("Failed to send block by TCP.", NULL);
		return -1;
	}

	if (bytesSent == 0)
	{
		/*	Just lost connection; treat as a transient
		 *	anomaly, note incomplete transmission.		*/

		writeMemo("[?] Lost connection to TCP BSI.");
		return 0;
	}

	return blockLength;
}
