/*
	libudpcla.c:	common functions for BP UDP-based
			convergence-layer daemons.

	Author: Ted Piotrowski, APL 
		Scott Burleigh, JPL

	Copyright (c) 2006, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
	
									*/
#include "udpcla.h"

/*	*	*	Sender functions	*	*	*	*/

static int	openUdpSocket(int *sock)
{
	*sock = -1;

	*sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (*sock < 0)
	{
		putSysErrmsg("CLO can't open UDP socket", NULL);
		return -1;
	}

	return 0;
}

int	sendBytesByUDP(int *bundleSocket, char *from, int length,
		struct sockaddr* socketName)
{
	int	bytesWritten;

	while (1)	/*	Continue until not interrupted.		*/
	{
		bytesWritten = sendto(*bundleSocket, from, length, 0,
			socketName, sizeof(struct sockaddr));
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
				close(*bundleSocket);
				*bundleSocket = -1;
			}

			putSysErrmsg("CLO write() error on socket", NULL);
		}

		return bytesWritten;
	}
}

int	sendBundleByUDP(struct sockaddr *socketName, int *bundleSocket,
		unsigned int bundleLength, Object bundleZco,
		unsigned char *buffer)
{
	Sdr		sdr;
	ZcoReader	reader;
	int		bytesToSend;
	int		bytesSent;

	if (bundleLength > UDPCLA_BUFSZ)
	{
		putErrmsg("Bundle is too big for UDP CLA.", itoa(bundleLength));
		return -1;
	}

	/*	Connect to CLI as necessary.				*/

	if (*bundleSocket < 0)
	{
		if (openUdpSocket(bundleSocket) < 0)
		{
			/*	Treat I/O error as a transient anomaly,
			 *	note incomplete transmission.		*/

			return 0;
		}
	}

	/*	Send the bundle in a single UDP datagram.		*/

	sdr = getIonsdr();
	sdr_begin_xn(sdr);
	zco_start_transmitting(sdr, bundleZco, &reader);
	bytesToSend = zco_transmit(sdr, &reader, UDPCLA_BUFSZ, (char *) buffer);
	if (bytesToSend < 0)
	{
		sdr_cancel_xn(sdr);
		putSysErrmsg("can't issue from ZCO", NULL);
		return -1;
	}

	zco_stop_transmitting(sdr, &reader);
	zco_destroy_reference(sdr, bundleZco);
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't issue from ZCO.", NULL);
		return -1;
	}

	if (bytesToSend == 0)
	{
		return 0;
	}

	bytesSent = sendBytesByUDP(bundleSocket, (char *) buffer, bytesToSend,
			socketName);
	if (bytesSent < 0)
	{
		if (*bundleSocket == -1)
		{
			/*	Just lost connection; treat as a
			 *	transient anomaly, note the incomplete
			 *	transmission.				*/

			putErrmsg("Lost connection to CLI; restart CLO when \
connectivity is restored.", NULL);
			return 0;
		}

		/*	Big problem; shut down.			*/

		putSysErrmsg("Failed to send by UDP", NULL);
		return -1;
	}

	return bytesSent;
}

/*	*	*	Receiver functions	*	*	*	*/

int	receiveBytesByUDP(int bundleSocket, struct sockaddr_in *fromAddr,
		char *into, int length)
{
	int		bytesRead;
	socklen_t	fromSize;

	fromSize = sizeof(struct sockaddr_in);
	bytesRead = recvfrom(bundleSocket, into, length, 0,
			(struct sockaddr *) fromAddr, &fromSize);
	if (bytesRead < 0)
	{
		if (errno == EBADF)	/*	Shutdown.		*/
		{
			return 0;
		}

		putSysErrmsg("CLI read() error on socket", NULL);
		return -1;
	}

	return bytesRead;
}
