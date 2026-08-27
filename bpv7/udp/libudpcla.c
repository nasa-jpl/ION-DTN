/*
        libudpcla.c:	common functions for BP UDP-based
                        convergence-layer daemons.

        Author: Ted Piotrowski, APL
                Scott Burleigh, JPL

        Modification History:
        August 2025, Dual-stack Extension, ION Dev Team/JPL

        Copyright (c) 2006, California Institute of Technology.
        ALL RIGHTS RESERVED.  U.S. Government Sponsorship
        acknowledged.

                                                                        */
#include "udpcla.h"

#include "ion_atomic.h"

/*	*	*	Sender functions	*	*	*	*/

static int openUdpSocket(int *sock)
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

int sendBytesByUDP(int *bundleSocket, char *from, int length,
		struct sockaddr *socketName)
{
	int bytesWritten;

	CHKERR(socketName && bundleSocket && from);
	while (1) /*	Continue until not interrupted.		*/
	{
		bytesWritten = isendto(*bundleSocket, from, length, 0,
				socketName, sizeof(struct sockaddr));
		if (bytesWritten < 0)
		{
			switch (errno)
			{
			case EINTR: /*	Interrupted; retry.	*/
				continue;

			case EPIPE: /*	Lost connection.	*/
			case EBADF:
			case ETIMEDOUT:
			case ECONNRESET:
				closesocket(*bundleSocket);
				*bundleSocket = -1;
			}

			putSysErrmsg("CLO write() error on socket", NULL);
		}

		return bytesWritten;
	}
}

int sendBundleByUDP(struct sockaddr *socketName, int *bundleSocket,
		unsigned int bundleLength, SdrObject bundleZco,
		unsigned char *buffer)
{
	Sdr       sdr;
	ZcoReader reader;
	int       bytesToSend;
	int       bytesSent;

	CHKERR(socketName && bundleSocket && buffer);
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
	zco_start_transmitting(bundleZco, &reader);
	zco_track_file_offset(&reader);
	CHKERR(sdr_begin_xn(sdr));
	bytesToSend = zco_transmit(sdr, &reader, UDPCLA_BUFSZ, (char *) buffer);
	if (sdr_end_xn(sdr) < 0 || bytesToSend < 0)
	{
		putErrmsg("Can't issue from ZCO.", NULL);
		return -1;
	}

	bytesSent = sendBytesByUDP(bundleSocket, (char *) buffer, bytesToSend,
			socketName);
	if (bytesSent < 0)
	{
		if (bpHandleXmitFailure(bundleZco) < 0)
		{
			putErrmsg("Can't handle xmit failure.", NULL);
			return -1;
		}

		if (*bundleSocket == -1)
		{
			/*	Just lost connection; treat as a
			 *	transient anomaly, note the incomplete
			 *	transmission.				*/

			writeMemo("[i] Lost UDP connection to CLI; restart CLO \
when connectivity is restored.");
			return 0;
		}
		else
		{
			/*	Big problem; shut down.			*/

			putErrmsg("Failed to send by UDP.", NULL);
			return -1;
		}
	}
	else
	{
		if (bpHandleXmitSuccess(bundleZco) < 0)
		{
			putErrmsg("Can't handle xmit success.", NULL);
			return -1;
		}
	}

	return bytesSent;
}

/*
 * DUAL-STACK FUNCTIONS - use getaddrinfo() for automatic IPv4/IPv6 handling
 */

/* Helper function for shutdown signal detection */
static int isUdpClaShutdownSignal(UdpClaSocket *claSock,
		const IonNetworkAddress        *from_addr)
{
	if (from_addr->family != claSock->local_addr.family)
	{
		return 0;
	}

	if (from_addr->family == AF_INET)
	{
		const struct sockaddr_in *from4 =
				(const struct sockaddr_in *) &from_addr->addr;
		struct sockaddr_in *local4 =
				(struct sockaddr_in *) &claSock->local_addr.addr;

		return (from4->sin_addr.s_addr == htonl(INADDR_LOOPBACK)
				&& from4->sin_port == local4->sin_port);
	}
	else if (from_addr->family == AF_INET6)
	{
		const struct sockaddr_in6 *from6 =
				(const struct sockaddr_in6 *) &from_addr->addr;
		struct sockaddr_in6 *local6 =
				(struct sockaddr_in6 *) &claSock->local_addr.addr;

		return (IN6_IS_ADDR_LOOPBACK(&from6->sin6_addr)
				&& from6->sin6_port == local6->sin6_port);
	}

	return 0;
}

int initUdpClaSocket(const char *endpoint, UdpClaSocket *claSock)
{
	IonNetworkAddress bind_addr;
	socklen_t         addr_len;
	int               sockFlags;

	if (!endpoint || !claSock)
	{
		return -1;
	}

	memset(claSock, 0, sizeof(UdpClaSocket));
	ion_atomic_init(&claSock->shutdown_requested, 0);
	pthread_mutex_init(&claSock->shutdown_mutex, NULL);
	claSock->main_socket = -1;
	claSock->shutdown_socket = -1;

	/* getaddrinfo() does all the IPv4/IPv6 work automatically */
	if (resolveNetworkAddressPassive(endpoint, BpUdpDefaultPortNbr,
			    SOCK_DGRAM, IPPROTO_UDP, &bind_addr) < 0)
	{
		putErrmsg("Can't resolve UDP CLA address", endpoint);
		return -1;
	}

	/*
	 * Request dual-stack only for the family-agnostic wildcard, so that
	 * an explicit "[::]" binds IPv6-only rather than quietly serving IPv4.
	 */
	sockFlags = isDualStackWildcard(endpoint) ?
			ION_SOCK_DUALSTACK : ION_SOCK_V6ONLY;
	if (createNetworkSocketEx(SOCK_DGRAM, &bind_addr, sockFlags,
			    &claSock->main_socket) < 0)
	{
		putErrmsg("Can't initialize UDP CLA socket", endpoint);
		return -1;
	}

	/* Get actual bound address using getsockname() */
	addr_len = sizeof(claSock->local_addr.addr);
	if (getsockname(claSock->main_socket,
			(struct sockaddr *) &claSock->local_addr.addr,
			&addr_len)
			< 0)
	{
		closesocket(claSock->main_socket);
		putSysErrmsg("Can't get socket name", NULL);
		return -1;
	}

	claSock->local_addr.addr_len = addr_len;
	claSock->local_addr.family = claSock->local_addr.addr.ss_family;

	/* Prepare shutdown target address */
	claSock->shutdown_target = claSock->local_addr;

	/* Handle "any address" for shutdown signal */
	if (bind_addr.family == AF_INET)
	{
		struct sockaddr_in *sin =
				(struct sockaddr_in *) &claSock->shutdown_target.addr;
		if (sin->sin_addr.s_addr == INADDR_ANY)
		{
			sin->sin_addr.s_addr = htonl(INADDR_LOOPBACK);
		}
	}
	else if (bind_addr.family == AF_INET6)
	{
		struct sockaddr_in6 *sin6 =
				(struct sockaddr_in6 *) &claSock
						->shutdown_target.addr;
		if (IN6_IS_ADDR_UNSPECIFIED(&sin6->sin6_addr))
		{
			sin6->sin6_addr = in6addr_loopback;
		}
	}

	/* Log what we bound to */
	char addr_str[INET6_ADDR_WITH_PORT_STRLEN];
	formatNetworkAddress(&claSock->local_addr, addr_str, sizeof(addr_str));

	char memo[256];
	snprintf(memo, sizeof(memo), "UDP CLA socket bound to %s (%s)", addr_str,
			(claSock->local_addr.family == AF_INET6) ? "IPv6" :
								   "IPv4");
	writeMemo(memo);

	return 0;
}

int receiveUdpClaDatagram(UdpClaSocket *claSock, char *buffer, size_t buffer_size,
		IonNetworkAddress *from_addr, int *is_shutdown)
{
	int bytes_received;

	if (!claSock || !buffer || !is_shutdown)
	{
		return -1;
	}

	*is_shutdown = 0;

	/* Receive datagram - works automatically for IPv4 and IPv6 */
	from_addr->addr_len = sizeof(from_addr->addr);
	bytes_received = recvfrom(claSock->main_socket, buffer, buffer_size, 0,
			(struct sockaddr *) &from_addr->addr,
			&from_addr->addr_len);

	if (bytes_received < 0)
	{
		if (errno == EINTR)
		{
			return 0;
		}
		putSysErrmsg("UDP CLA recvfrom() failed", NULL);
		return -1;
	}

	if (bytes_received == 0)
	{
		return 0;
	}

	from_addr->family = from_addr->addr.ss_family;

	/* Check for shutdown signal: exactly 1 byte with value 1 from loopback */
	if (bytes_received == 1 && buffer[0] == 1)
	{
		if (isUdpClaShutdownSignal(claSock, from_addr))
		{
			*is_shutdown = 1;
			return 1;
		}
	}

	return bytes_received;
}

int sendUdpClaShutdown(UdpClaSocket *claSock)
{
	char shutdown_byte = 1;
	int  bytes_sent;

	if (!claSock)
	{
		return -1;
	}

	pthread_mutex_lock(&claSock->shutdown_mutex);

	if (ion_atomic_get(&claSock->shutdown_requested))
	{
		pthread_mutex_unlock(&claSock->shutdown_mutex);
		return 0; /* Already sent */
	}

	/* Create shutdown socket if needed */
	if (claSock->shutdown_socket < 0)
	{
		claSock->shutdown_socket = socket(claSock->local_addr.family,
				SOCK_DGRAM, IPPROTO_UDP);
		if (claSock->shutdown_socket < 0)
		{
			pthread_mutex_unlock(&claSock->shutdown_mutex);
			putSysErrmsg("Can't create shutdown socket", NULL);
			return -1;
		}
	}

	/* Send 1-byte shutdown signal */
	bytes_sent = sendto(claSock->shutdown_socket, &shutdown_byte, 1, 0,
			(struct sockaddr *) &claSock->shutdown_target.addr,
			claSock->shutdown_target.addr_len);

	if (bytes_sent < 0)
	{
		putSysErrmsg("Can't send UDP CLA shutdown signal", NULL);
		pthread_mutex_unlock(&claSock->shutdown_mutex);
		return -1;
	}

	ion_atomic_set(&claSock->shutdown_requested, 1);

	char target_str[INET6_ADDR_WITH_PORT_STRLEN];
	formatNetworkAddress(&claSock->shutdown_target, target_str,
			sizeof(target_str));

	char memo[256];
	snprintf(memo, sizeof(memo), "Sent UDP CLA shutdown signal to %s",
			target_str);
	writeMemo(memo);

	pthread_mutex_unlock(&claSock->shutdown_mutex);
	return 0;
}

void cleanupUdpClaSocket(UdpClaSocket *claSock)
{
	if (!claSock)
	{
		return;
	}

	if (claSock->main_socket >= 0)
	{
		closesocket(claSock->main_socket);
		claSock->main_socket = -1;
	}

	if (claSock->shutdown_socket >= 0)
	{
		closesocket(claSock->shutdown_socket);
		claSock->shutdown_socket = -1;
	}

	pthread_mutex_destroy(&claSock->shutdown_mutex);
	ion_atomic_mutex_destroy(&claSock->shutdown_requested);
	memset(claSock, 0, sizeof(UdpClaSocket));
}

int sendBundleByUDPDualStack(const IonNetworkAddress *destAddr,
		int *bundleSocket, unsigned int bundleLength,
		SdrObject bundleZco, unsigned char *buffer)
{
	Sdr       sdr;
	ZcoReader reader;
	int       bytesToSend;
	int       bytesSent;

	CHKERR(destAddr && bundleSocket && buffer);
	if (bundleLength > UDPCLA_BUFSZ)
	{
		putErrmsg("Bundle is too big for UDP CLA.", itoa(bundleLength));
		return -1;
	}

	/* Connect to CLI as necessary */
	if (*bundleSocket < 0)
	{
		/* Create socket with correct family from getaddrinfo() */
		*bundleSocket = socket(destAddr->family, SOCK_DGRAM, IPPROTO_UDP);
		if (*bundleSocket < 0)
		{
			putSysErrmsg("CLO can't open UDP socket", NULL);
			return 0; /* Treat as transient error */
		}
	}

	/* Send the bundle in a single UDP datagram */
	sdr = getIonsdr();
	zco_start_transmitting(bundleZco, &reader);
	zco_track_file_offset(&reader);
	CHKERR(sdr_begin_xn(sdr));
	bytesToSend = zco_transmit(sdr, &reader, UDPCLA_BUFSZ, (char *) buffer);
	if (sdr_end_xn(sdr) < 0 || bytesToSend < 0)
	{
		putErrmsg("Can't issue from ZCO.", NULL);
		return -1;
	}

	/* Send using dual-stack address - works for IPv4 and IPv6 */
	bytesSent = sendto(*bundleSocket, (char *) buffer, bytesToSend, 0,
			(const struct sockaddr *) &destAddr->addr, destAddr->addr_len);

	if (bytesSent < 0)
	{
		if (bpHandleXmitFailure(bundleZco) < 0)
		{
			putErrmsg("Can't handle xmit failure.", NULL);
			return -1;
		}

		if (errno == EPIPE || errno == EBADF || errno == ECONNRESET)
		{
			closesocket(*bundleSocket);
			*bundleSocket = -1;
			writeMemo("[i] Lost UDP connection; will retry when connectivity restored.");
			return 0;
		}
		else
		{
			putErrmsg("Failed to send by UDP.", NULL);
			return -1;
		}
	}
	else
	{
		if (bpHandleXmitSuccess(bundleZco) < 0)
		{
			putErrmsg("Can't handle xmit success.", NULL);
			return -1;
		}
	}

	return bytesSent;
}
