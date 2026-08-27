/*
	udplsi.c:	LTP UDP-based link service daemon.

	Author: Scott Burleigh, JPL

	Copyright (c) 2007, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.

									*/
#include "udplsa.h"

#include "ion_atomic.h"

static void	interruptThread(int signum)
{
	/* Tell the compiler that we are not using 'signum' */
	(void)signum;

	isignal(SIGTERM, interruptThread);
	ionKillMainThread("udplsi");
}

/*	*	*	Main thread functions	*	*	*	*/

#if defined (ION_LWT)
int	udplsi(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
	char	*endpointSpec = (char *) a1;
#else
int	main(int argc, char *argv[])
{
	char	*endpointSpec = (argc > 1 ? argv[1] : NULL);
#endif
	Sdr			sdr;
	char			lsiCmd[256];
	LtpVseat		*vseat;
	PsmAddress		vseatElt;
	static udp_ReceiverThreadParms	rtp;
	pthread_t		receiverThread;
	int			fd;
	char			quit = '\0';
	char			localAddrStr[INET6_ADDRSTRLEN + 10];
	socklen_t		addr_len;

	/* usage message with both IPv4 and IPv6 examples */
	if (endpointSpec == NULL)
	{
		PUTS("Usage: udplsi <local host name>[:<port number>]");
		PUTS("  IPv4: udplsi 192.168.1.1:1113");
		PUTS("  IPv6: udplsi [::1]:1113");
		PUTS("  IPv6: udplsi [2001:db8::1]:1113");
		PUTS("  Any:  udplsi 0.0.0.0:1113 or udplsi [::]:1113");
		return 0;
	}

	/*	Note that ltpadmin must be run before the first
	 *	invocation of ltplsi, to initialize the LTP database
	 *	(as necessary) and dynamic database.			*/

	if (ltpInit(0) < 0)
	{
		putErrmsg("udplsi can't initialize LTP.", NULL);
		return 1;
	}

	sdr = getIonsdr();
	isprintf(lsiCmd, sizeof lsiCmd, "udplsi %s", endpointSpec);
	CHKERR(sdr_begin_xn(sdr));
	findSeat(lsiCmd, &vseat, &vseatElt);
	sdr_exit_xn(sdr);
	if (vseatElt == 0)
	{
		writeMemoNote("[?] Undefined LSI", lsiCmd);
		return 1;
	}

	if (vseat->lsiPid != ERROR && vseat->lsiPid != sm_TaskIdSelf())
	{
		writeMemoNote("[?] LSI task is already started", itoa(vseat->lsiPid));
		return 1;
	}

	/*	All command-line arguments are now validated.		*/

	/* Initialize the mutex.  */
	pthread_mutex_init(&rtp.lock, NULL);

	/* Dual-stack address resolution */
	int resolveResult = resolveNetworkAddressPassive(endpointSpec,
			LtpUdpDefaultPortNbr, SOCK_DGRAM, IPPROTO_UDP,
			&rtp.local_addr);
	if (resolveResult < 0)
	{
		putErrmsg("udplsi: Can't resolve dual-stack address",
				endpointSpec);
		return -1;
	}

	/* Set default port if not specified */
	if (rtp.local_addr.family == AF_INET)
	{
		struct sockaddr_in *sin = (struct sockaddr_in*)&rtp.local_addr.addr;
		if (sin->sin_port == 0)
		{
			sin->sin_port = htons(LtpUdpDefaultPortNbr);
		}
	}
	else if (rtp.local_addr.family == AF_INET6)
	{
		struct sockaddr_in6 *sin6 = (struct sockaddr_in6*)&rtp.local_addr.addr;
		if (sin6->sin6_port == 0)
		{
			sin6->sin6_port = htons(LtpUdpDefaultPortNbr);
		}
	}

	/*
	 * Create and bind socket using helper function.  Request dual-stack
	 * only for the family-agnostic wildcard, so that an explicit "[::]"
	 * binds IPv6-only rather than quietly serving IPv4.
	 */
	int sockFlags = isDualStackWildcard(endpointSpec) ?
			ION_SOCK_DUALSTACK : ION_SOCK_V6ONLY;
	if (createNetworkSocketEx(SOCK_DGRAM, &rtp.local_addr, sockFlags,
			    &rtp.linkSocket) < 0)
	{
		putErrmsg("udplsi: Can't create and bind UDP socket", NULL);
		return 1;
	}

	/* Get actual bound address for logging and shutdown */
	addr_len = sizeof(rtp.local_addr.addr);
	if (getsockname(rtp.linkSocket, (struct sockaddr*)&rtp.local_addr.addr, &addr_len) < 0)
	{
		closesocket(rtp.linkSocket);
		putSysErrmsg("LSI can't get socket name", NULL);
		return 1;
	}
	rtp.local_addr.addr_len = addr_len;
	rtp.local_addr.family = rtp.local_addr.addr.ss_family;

	/*	Set up signal handling; SIGTERM is shutdown signal.	*/

	ionNoteMainThread("udplsi");
	isignal(SIGTERM, interruptThread);

	/*	Start the receiver thread.				*/
	ion_atomic_init(&rtp.running, 1);

	if (pthread_begin(&receiverThread, NULL, udplsa_handle_datagrams,
			&rtp, "udplsi_receiver"))
	{
		closesocket(rtp.linkSocket);
		putSysErrmsg("udplsi can't create receiver thread", NULL);
		return 1;
	}

	/*	Now sleep until interrupted by SIGTERM, at which point
	 *	it's time to stop the link service.			*/

	/* Logging */
	formatNetworkAddress(&rtp.local_addr, localAddrStr, sizeof(localAddrStr));
	{
		char	txt[500];

		isprintf(txt, sizeof(txt),
			"[i] udplsi is running, listening on %s (%s).",
			localAddrStr,
			(rtp.local_addr.family == AF_INET6) ? "IPv6" : "IPv4");
		writeMemo(txt);
	}

	ionPauseMainThread(-1);

	/*	Time to shut down.					*/

	writeMemo("[i] udplsi shutting down...");
	ion_atomic_set(&rtp.running, 0);

	/* Shutdown signaling */
	fd = socket(rtp.local_addr.family, SOCK_DGRAM, IPPROTO_UDP);
	if (fd >= 0)
	{
		/* Prepare shutdown target address */
		IonNetworkAddress shutdown_target = rtp.local_addr;

		/* NEW: Handle "any address" cases for shutdown signal */
		if (rtp.local_addr.family == AF_INET)
		{
			struct sockaddr_in *sin = (struct sockaddr_in*)&shutdown_target.addr;
			if (sin->sin_addr.s_addr == INADDR_ANY)
			{
				sin->sin_addr.s_addr = htonl(INADDR_LOOPBACK);
			}
		}
		else if (rtp.local_addr.family == AF_INET6)
		{
			struct sockaddr_in6 *sin6 = (struct sockaddr_in6*)&shutdown_target.addr;
			if (IN6_IS_ADDR_UNSPECIFIED(&sin6->sin6_addr))
			{
				sin6->sin6_addr = in6addr_loopback;
			}
		}

		/* Send shutdown signal */
		if (sendto(fd, &quit, 1, 0,
				(struct sockaddr*)&shutdown_target.addr,
				shutdown_target.addr_len) == 1)
		{
			pthread_join(receiverThread, NULL);
		}
		else
		{
			writeMemo("[!] udplsi: Shutdown signal failed, forcing thread termination");
			pthread_cancel(receiverThread);
			pthread_join(receiverThread, NULL);
		}

		closesocket(fd);
	}
	else
	{
		writeMemo("[!] udplsi: Can't create shutdown socket, forcing thread termination");
		pthread_cancel(receiverThread);
		pthread_join(receiverThread, NULL);
	}

	closesocket(rtp.linkSocket);
	ion_atomic_mutex_destroy(&rtp.running);
	writeErrmsgMemos();
	writeMemo("[i] udplsi has ended.");
	ionDetach();
	return 0;
}
