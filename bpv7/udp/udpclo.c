/*
	udpclo.c:	BP UDP-based convergence-layer output
			daemon.

	Author: Ted Piotrowski, APL
		Scott Burleigh, JPL

	Copyright (c) 2006, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
	
									*/
#include "udpcla.h"

static sm_SemId		udpcloSemaphore(sm_SemId *semid)
{
	static sm_SemId	semaphore = -1;
	
	if (semid)
	{
		semaphore = *semid;
	}

	return semaphore;
}

static void	shutDownClo(int signum)
{
	sm_SemEnd(udpcloSemaphore(NULL));
}

/*	*	*	Main thread functions	*	*	*	*/

static unsigned long	getUsecTimestamp()
{
	struct timeval	tv;

	getCurrentTime(&tv);
	return ((tv.tv_sec * 1000000) + tv.tv_usec);
}

/* Define the caching interval macro (default: 1 minute) */
#ifndef ADDRESS_CACHE_INTERVAL_MINUTES
#define ADDRESS_CACHE_INTERVAL_MINUTES 1
#endif

/* Convert minutes to microseconds for comparison */
#define CACHE_INTERVAL_USEC (ADDRESS_CACHE_INTERVAL_MINUTES * 60 * 1000000UL)

/* Define the maximum number of failed lookups before stopping (default: 100) */
#ifndef MAX_FAILED_LOOKUPS
#define MAX_FAILED_LOOKUPS 100
#endif

/* Add variables for caching and hostname storage */
static char *remoteHostName = NULL;
static char *endpointSpecCopy = NULL;
static unsigned long lastLookupTime = 0; /* Timestamp of last successful lookup */
static int isAddressValid = 0; /* Boolean: is the cached address valid? */
static unsigned short cachedPortNbr = 0; /* Store port number for reuse */
static unsigned long lastErrorLogTime = 0; /* Timestamp of last error log */
static unsigned int failedLookupCount = 0; /* Count of consecutive failed lookups */
static const unsigned int maxFailedLookups = MAX_FAILED_LOOKUPS; /* Max retries */
static int lastLookupFailed = 0; /* Boolean: was the last lookup attempt a failure? */

#if defined (ION_LWT)
int	udpclo(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
	char			*rttString = (a1 != 0 ? (char *) a1 : NULL);
	char			*endpointSpec = (char *) a2;
#else
int	main(int argc, char *argv[])
{
	char			*rttString = (argc > 1 ? argv[1] : NULL);
	char			*endpointSpec = (argc > 2 ? argv[2] : NULL);
#endif
	unsigned short		portNbr;
	unsigned int		hostNbr;
	struct sockaddr		socketName;
	struct sockaddr_in	*inetName;
	unsigned char		*buffer;
	VOutduct		*vduct;
	PsmAddress		vductElt;
	Sdr			sdr;
	Outduct			outduct;
	Object			planDuctList;
	Object			planObj = 0;
	BpPlan			plan;
	IonNeighbor		*neighbor = NULL;
	PsmAddress		nextElt;
	Object			bundleZco;
	BpAncillaryData		ancillaryData;
	unsigned int		bundleLength;
	int			ductSocket = -1;
	int			bytesSent;

	/*	Rate control calculation is based on treating elapsed
	 *	time as a currency.					*/

	float			timeCostPerByte;/*	In seconds.	*/
	unsigned long		startTimestamp;	/*	Billing cycle.	*/
	unsigned int		totalPaid;	/*	Since last send.*/
	unsigned int		currentPaid;	/*	Sending seg.	*/
	float			totalCostSecs;	/*	For this seg.	*/
	unsigned int		totalCost;	/*	Microseconds.	*/
	unsigned int		balanceDue;	/*	Until next seg.	*/
	unsigned int		prevPaid = 0;	/*	Prior snooze.	*/

	/*	Note: for backward compatibility, we accept and ignore
	 *	a round-trip time value that precedes the endpointSpec.	*/

	if (endpointSpec == NULL)
	{
		if (rttString == NULL)
		{
			PUTS("Usage: udpclo {<remote node's host name> | \
				@} [:<its port number>]");
			return 0;
		}
		else
		{
			endpointSpec = rttString;
		}
	}

	/* Attach to ION */
	if (bpAttach() < 0)
	{
		putErrmsg("udpclo can't attach to BP.", NULL);
		return -1;
	}

	/* Store endpointSpec and extract hostname */
    	if (endpointSpec) 
	{
        	endpointSpecCopy = MTAKE(strlen(endpointSpec) + 1);
        
		if (endpointSpecCopy == NULL) 
		{
            		putErrmsg("udpclo: No memory for endpointSpec copy.", NULL);
            		return -1;
        	}

	        strcpy(endpointSpecCopy, endpointSpec);

	        /* Parse endpointSpec to get port and hostname */
 		parseSocketSpec(endpointSpec, &portNbr, &hostNbr);
        
		if (portNbr == 0)
		{
			portNbr = BpUdpDefaultPortNbr;
		}

        	cachedPortNbr = portNbr; /* Store for reuse in re-resolution */

		/* Extract hostname */
		char *delimiter = strchr(endpointSpec, ':');
		int hostnameLen = delimiter ? (delimiter - endpointSpec) : strlen(endpointSpec);
		remoteHostName = MTAKE(hostnameLen + 1);

		if (remoteHostName == NULL) 
		{
			putErrmsg("udpclo: No memory for remoteHostName.", NULL);
			MRELEASE(endpointSpecCopy);
			return -1;
		}

		strncpy(remoteHostName, endpointSpec, hostnameLen);
		remoteHostName[hostnameLen] = '\0';
        
		if (strcmp(remoteHostName, "@") == 0) 
		{
			/* Replace '@' with local hostname */
			char hostnameBuf[MAXHOSTNAMELEN + 1];
			getNameOfHost(hostnameBuf, sizeof(hostnameBuf));
			MRELEASE(remoteHostName);
			remoteHostName = MTAKE(strlen(hostnameBuf) + 1);

			if (remoteHostName == NULL) 
			{
				putErrmsg("udpclo: No memory for local hostname.", NULL);
				MRELEASE(endpointSpecCopy);
				return -1;
			}
		
			strcpy(remoteHostName, hostnameBuf);
        	}
    	}

	/* Perform initialization */
	portNbr = htons(cachedPortNbr);
	hostNbr = htonl(hostNbr);
	memset((char *) &socketName, 0, sizeof socketName);
	inetName = (struct sockaddr_in *) &socketName;
	inetName->sin_family = AF_INET;
	inetName->sin_port = portNbr;
	memcpy((char *) &(inetName->sin_addr.s_addr), (char *) &hostNbr, 4);

	isAddressValid = (hostNbr != BAD_HOST_NAME); /* Check initial lookup */
	lastLookupTime = getUsecTimestamp(); /* Record initial lookup time */

	if (!isAddressValid) 
	{
		putErrmsg("udpclo: Initial hostname resolution failed.", remoteHostName);
		failedLookupCount = 1;
		lastErrorLogTime = lastLookupTime;
		lastLookupFailed = 1;
	}
	
	buffer = MTAKE(UDPCLA_BUFSZ);
	if (buffer == NULL)
	{
		putErrmsg("udpclo: No memory for UDP buffer in udpclo.", NULL);
		MRELEASE(endpointSpecCopy);
		MRELEASE(remoteHostName);
		return -1;
	}

	findOutduct("udp", endpointSpec, &vduct, &vductElt);
	if (vductElt == 0)
	{
		putErrmsg("udpclo: No such udp duct.", endpointSpec);
		MRELEASE(buffer);
		MRELEASE(endpointSpecCopy);
		MRELEASE(remoteHostName);
		return -1;
	}

	if (vduct->cloPid != ERROR && vduct->cloPid != sm_TaskIdSelf())
	{
		putErrmsg("udpclo: CLO task is already started for this duct.", itoa(vduct->cloPid));
		MRELEASE(buffer);
		MRELEASE(endpointSpecCopy);
		MRELEASE(remoteHostName);
		return -1;
	}

	/*	All command-line arguments are now validated.		*/

	neighbor = NULL;
	sdr = getIonsdr();
	CHKZERO(sdr_begin_xn(sdr));
	sdr_read(sdr, (char *) &outduct, sdr_list_data(sdr, vduct->outductElt),
			sizeof(Outduct));
	if (outduct.planDuctListElt)
	{
		planDuctList = sdr_list_list(sdr, outduct.planDuctListElt);
		planObj = sdr_list_user_data(sdr, planDuctList);
		if (planObj)
		{
			sdr_read(sdr, (char *) &plan, planObj, sizeof(BpPlan));
		}
	}

	sdr_exit_xn(sdr);

	/*	Set up signal handling.  SIGTERM is shutdown signal.	*/

	oK(udpcloSemaphore(&(vduct->semaphore)));
	isignal(SIGTERM, shutDownClo);

	/*	Can now begin transmitting to remote duct.		*/

	{
		char	memoBuf[1024];

		isprintf(memoBuf, sizeof(memoBuf),
				"[i] udpclo is running, spec = '%s'",
				endpointSpec);
		writeMemo(memoBuf);
	}

	startTimestamp = getUsecTimestamp();
	
	/* Main Loop */
	while (!(sm_SemEnded(vduct->semaphore)))
    	{
		unsigned long currentTime = getUsecTimestamp();

		if (bpDequeue(vduct, &bundleZco, &ancillaryData, 0) < 0)
		{
			putErrmsg("udpclo: Can't dequeue bundle.", NULL);
			break;
		}

		if (bundleZco == 0) /* Outduct closed. */
		{
			writeMemo("[i] udpclo outduct closed.");
			sm_SemEnd(udpcloSemaphore(NULL)); /* Stop. */
			continue;
		}

		if (bundleZco == 1) /* Got a corrupt bundle. */
		{
			continue; /* Get next bundle. */
		}

		/* Get bundle length */
		CHKZERO(sdr_begin_xn(sdr));
		bundleLength = zco_length(sdr, bundleZco);
		sdr_exit_xn(sdr);  /* Short transaction, no nested calls expected */

		/* Check if address needs re-resolution */
		if (remoteHostName) 
		{
			if (!isAddressValid || (currentTime - lastLookupTime >= CACHE_INTERVAL_USEC)) 
			{
				unsigned int newHostNbr = getInternetAddress(remoteHostName);
				if (newHostNbr == BAD_HOST_NAME) 
				{
					failedLookupCount++;
					if (failedLookupCount >= maxFailedLookups) 
					{
						putErrmsg("udpclo: Maximum failed lookup attempts reached, stopping daemon.", remoteHostName);
						/* Destroy the ZCO containing the serialized bundle */
						zco_destroy(sdr, bundleZco);
						sm_SemEnd(udpcloSemaphore(NULL)); /* Stop. */
						break;
					}

					isAddressValid = 0;
					lastLookupTime = currentTime;
					lastLookupFailed = 1;
				} 
				else 
				{
					failedLookupCount = 0;
					newHostNbr = htonl(newHostNbr);
					portNbr = htons(cachedPortNbr);
					memset((char *) &socketName, 0, sizeof socketName);
					inetName = (struct sockaddr_in *) &socketName;
					inetName->sin_family = AF_INET;
					inetName->sin_port = portNbr;
					memcpy((char *) &(inetName->sin_addr.s_addr), (char *) &newHostNbr, 4);
					isAddressValid = 1;
					lastLookupTime = currentTime;
					/* log success and reset socket */
					if (lastLookupFailed) 
					{
						writeMemoNote("[i] udpclo: Successfully resolved hostname", remoteHostName);
						if (ductSocket != -1)
						{
							closesocket(ductSocket);
							ductSocket = -1; /* Force reopen */
							writeMemo("[i] udpclo: Closed socket for reinitialization");
						}
                			}
					lastLookupFailed = 0;
				}
			}
		}

		/* abandon transmitting bundle if address is invalid */
		if (!isAddressValid) 
		{
		
			/* Begin SDR transaction to destory the serialized bundle
			 * For UDPCL, the original bundle has already been destroyed
			 * upon dequeueing due to lack of stewardship. */
			if (sdr_begin_xn(sdr) < 0)
			{
				putErrmsg("udpclo: Can't start SDR transaction to destroy serialized bundle.", NULL);
				sm_SemEnd(udpcloSemaphore(NULL));  /* Stop daemon */
				continue;
			}

			/* Destroy the ZCO containing the serialized bundle */
			zco_destroy(sdr, bundleZco);

			/* End the SDR transaction */
			if (sdr_end_xn(sdr) < 0)
			{
				putErrmsg("udpclo: Failed to commit SDR transaction for bundle abandonment.", NULL);
				sm_SemEnd(udpcloSemaphore(NULL));  /* Stop daemon */
				continue;
			}

			continue;  /* Proceed to next bundle */
		}

		/* Let sendBundleByUDP handle invalid IP address */
		bytesSent = sendBundleByUDP(&socketName, &ductSocket, bundleLength, bundleZco, buffer);
		if (bytesSent < bundleLength)
		{
			sm_SemEnd(udpcloSemaphore(NULL)); /* Stop. */
			continue;
		}

		/* Rate control calculation */
		totalPaid = getUsecTimestamp() - startTimestamp;
		startTimestamp = getUsecTimestamp();

		if (totalPaid >= prevPaid)
		{
			currentPaid = totalPaid - prevPaid;
		}
		else
		{
			currentPaid = 0;
		}

		if (neighbor == NULL)
		{
			if (planObj && plan.neighborNodeNbr)
			{
				neighbor = findNeighbor(getIonVdb(),
				plan.neighborNodeNbr, &nextElt);
			}
		}

		if (neighbor && neighbor->xmitRate > 0)
		{
			timeCostPerByte = 1.0 / (neighbor->xmitRate);
		}
		else
		{
			timeCostPerByte = 0.0;
		}

		totalCostSecs = timeCostPerByte * computeECCC(bundleLength);
		totalCost = totalCostSecs * 1000000.0;

		if (totalCost > currentPaid)
		{
			balanceDue = totalCost - currentPaid;
		}
		else
		{
			balanceDue = 0;
		}

		if (balanceDue > 0)
		{
			microsnooze(balanceDue);
		}

		prevPaid = balanceDue;
		sm_TaskYield();
	}

  	/* Clean up */
	if (ductSocket != -1)
	{
        	closesocket(ductSocket);
	}

	writeErrmsgMemos();
	writeMemo("[i] udpclo duct has ended.");
	MRELEASE(buffer);
	MRELEASE(endpointSpecCopy);
	MRELEASE(remoteHostName);
	ionDetach();
	return 0;
}
