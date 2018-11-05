/*
	file2udp.c:	a UDP benchmark sender.
									*/
/*									*/
/*	Copyright (c) 2003, California Institute of Technology.		*/
/*	All rights reserved.						*/

#include <file2udp.h>

static char			*eofLine = EOF_LINE_TEXT;
static int			eofLineLen;
static int			cyclesRequested = 1;
static struct sockaddr		socketName;
static struct sockaddr_in	*inetName;

static struct rtt_struct  	rttinfo;	/* used by rtt_XXX() */
static int		   	rttfirst = 1;
static int		   	tout_flag;	/* used in this file only */

/*
 * Timer routines for round-trip timing of datagrams.
 *
 *	rtt_init()	Called to initialize everything for a given
 *			"connection."
 *	rtt_newpack()	Called before each new packet is transmitted on
 *			a "connection."  Initializes retransmit counter to 0.
 *	rtt_start()	Called before each packet either transmitted or
 *			retransmitted.  Calculates the timeout value for
 *			the packet and starts the timer to calculate the RTT.
 *	rtt_stop()	Called after a packet has been received.
 *	rtt_timeout()	Called after a timeout has occurred.  Tells you
 *			if you should retransmit again, or give up.
 *
 * The difference between rtt_init() and rtt_newpack() is that the former
 * knows nothing about the "connection," while the latter makes use of
 * previous RTT information for a given "connection."
 */

static int	exp_backoff[ RTT_MAXNREXMT + 1 ] = { 1, 2, 4, 8, 16 };
	/* indexed by rtt_nrexmt: 0, 1, 2, ..., RTT_MAXNREXMT.
	   [0] entry (==1) is not used;
	   [1] entry (==2) is used the second time a packet is sent; ... */

/*
 * Initialize an RTT structure.
 * This function is called before the first packet is transmitted.
 */

static void rtt_init(register struct rtt_struct *ptr)
{
	ptr->rtt_rtt    = 0;
	ptr->rtt_srtt   = 0;
	ptr->rtt_rttdev = 1.5;
		/* first timeout at (srtt + (2 * rttdev)) = 3 seconds */
	ptr->rtt_nxtrto = 0;
}

/*
 * Initialize the retransmit counter before a packet is transmitted
 * the first time.
 */

static void rtt_newpack(register struct rtt_struct *ptr)
{
	ptr->rtt_nrexmt = 0;
}

/*
 * Start our RTT timer.
 * This should be called right before the alarm() call before a packet
 * is received.  We calculate the integer alarm() value to use for the
 * timeout (RTO) and return it as the value of the function.
 */

static int rtt_start(register struct rtt_struct *ptr)
{
	register int	rexmt;

	if (ptr->rtt_nrexmt > 0) {
		/*
		 * This is a retransmission.  No need to obtain the
		 * starting time, as we won't use the RTT for anything.
		 * Just apply the exponential back off and return.
		 */

		ptr->rtt_currto *= exp_backoff[ ptr->rtt_nrexmt ];
		return(ptr->rtt_currto);
	}

	if (gettimeofday(&ptr->time_start, (struct timezone *) 0) < 0)
		perror("rtt_start: gettimeofday() error");
	if (ptr->rtt_nxtrto > 0) {
		/*
		 * This is the first transmission of a packet *and* the
		 * last packet had to be retransmitted.  Therefore, we'll
		 * use the final RTO for the previous packet as the
		 * starting RTO for this packet.  If that RTO is OK for
		 * this packet, then we'll start updating the RTT estimators.
		 */

		ptr->rtt_currto = ptr->rtt_nxtrto;
		ptr->rtt_nxtrto = 0;
		return(ptr->rtt_currto);
	}

	/*
	 * Calculate the timeout value based on current estimators:
	 *	smoothed RTT plus twice the deviation.
	 */

	rexmt = (int) (ptr->rtt_srtt + (2.0 * ptr->rtt_rttdev) + 0.5);
	if (rexmt < RTT_RXTMIN)
		rexmt = RTT_RXTMIN;
	else if (rexmt > RTT_RXTMAX)
		rexmt = RTT_RXTMAX;
	return( ptr->rtt_currto = rexmt );
}

/*
 * A response was received.
 * Stop the timer and update the appropriate values in the structure
 * based on this packet's RTT.  We calculate the RTT, then update the
 * smoothed RTT and the RTT variance.
 * This function should be called right after turning off the
 * timer with alarm(0), or right after a timeout occurs.
 */

static void rtt_stop(register struct rtt_struct	*ptr)
{
	double		start, stop, err;

	if (ptr->rtt_nrexmt > 0) {
		/*
		 * The response was for a packet that has been retransmitted.
		 * We don't know which transmission the response corresponds to.
		 * We didn't record the start time in rtt_start(), so there's
		 * no need to record the stop time here.  We also don't
		 * update our estimators.
		 * We do, however, save the RTO corresponding to this
		 * response, and it'll be used for the next packet.
		 */

		ptr->rtt_nxtrto = ptr->rtt_currto;
		return;
	}
	ptr->rtt_nxtrto = 0;		/* for next call to rtt_start() */

	if (gettimeofday(&ptr->time_stop, (struct timezone *) 0) < 0)
		perror("rtt_stop: gettimeofday() error");
	start = ((double) ptr->time_start.tv_sec) * 1000000.0
				+ ptr->time_start.tv_usec;
	stop = ((double) ptr->time_stop.tv_sec) * 1000000.0
				+ ptr->time_stop.tv_usec;
	ptr->rtt_rtt = (stop - start) / 1000000.0;	/* in seconds */

	/*
	 * Update our estimators of RTT and mean deviation of RTT.
	 * See Jacobson's SIGCOMM '88 paper, Appendix A, for the details.
	 * This appendix also contains a fixed-point, integer implementation
	 * (that is actually used in all the post-4.3 TCP code).
	 * We'll use floating point here for simplicity.
	 *
	 * First
	 *	err = (rtt - old_srtt) = difference between this measured value
	 *				 and current estimator.
	 * and
	 *	new_srtt = old_srtt*7/8 + rtt/8.
	 * Then
	 *	new_srtt = old_srtt + err/8.
	 *
	 * Also
	 *	new_rttdev = old_rttdev + (|err| - old_rttdev)/4.
	 */

	err = ptr->rtt_rtt - ptr->rtt_srtt;
	ptr->rtt_srtt += err / 8;

	if (err < 0.0)
		err = -err;	/* |err| */

	ptr->rtt_rttdev += (err - ptr->rtt_rttdev) / 4;
}

/*
 * A timeout has occurred.
 * This function should be called right after the timeout alarm occurs.
 * Return -1 if it's time to give up, else return 0.
 */

static int rtt_timeout(register struct rtt_struct *ptr)
{
	rtt_stop(ptr);

	if (++ptr->rtt_nrexmt > RTT_MAXNREXMT)
		return (-1);		/* time to give up for this packet */

	return (0);
}
#if 0
/*
 * Print debugging information on stderr, if the "rtt_d_flag" is nonzero.
 */

static void rtt_debug(register struct rtt_struct *ptr)
{
	if (rtt_d_flag == 0) return;

	fprintf(stderr, "rtt = %.5f, srtt = %.3f, rttdev = %.3f, currto = %d\n",
			ptr->rtt_rtt, ptr->rtt_srtt, ptr->rtt_rttdev,
			ptr->rtt_currto);
	fflush(stderr);
}
#endif

/*
 * Signal handler for timeouts (SIGALRM).
 * This function is called when the alarm() value that was set counts
 * down to zero.  This indicates that we haven't received a response
 * from the server to the last datagram we sent.
 * All we do is set a flag and return from the signal handler.
 * The occurrence of the signal interrupts the recvfrom() system call
 * (errno = EINTR) above, and we then check the tout_flag flag.
 */

static void	to_alarm()
{
printf("!");
fflush(stdout);
	tout_flag = 1;		/* set flag for function above */
}

/*
 * Send a datagram to a server, and read a response.
 * Establish a timer and resend as necessary.
 * This function is intended for those applications that send a datagram
 * and expect a response.
 * Returns actual size of received datagram, or -1 if error or no response.
 */

static int dgsendrecv(int fd,	/* datagram socket */
		char *outbuff,	/* pointer to buffer to send */
		int outbytes,	/* #bytes to send */
		char *inbuff,	/* pointer to buffer to receive into */
		int inbytes,	/* max #bytes to receive */
		struct sockaddr *destaddr,	/* destination address */
				/* can be 0, if datagram socket is connect'ed */
		int destlen)	/* sizeof(destaddr) */
{
	int	n;
	int	interval;

	if (rttfirst == 1)
	{
		rtt_init(&rttinfo);	/* initialize first time we're called */
		rttfirst = 0;
	}

	rtt_newpack(&rttinfo);		/* initialize for new packet */
	while (1)
	{
		/*	Send the datagram.				*/

		if (sendto(fd, outbuff, outbytes, 0, destaddr, destlen)
				!= outbytes)
		{
			perror("dgsendrecv: sendto error on socket");
			return -1;
		}

		signal(SIGALRM, to_alarm);
		tout_flag = 0;			/* for signal handler */
		interval = rtt_start(&rttinfo);	/* calc timeout value */
printf("%d ", interval);
fflush(stdout);
		alarm(interval);		/* start timer */
		n = recvfrom(fd, inbuff, inbytes, 0, (struct sockaddr *) 0,
				(socklen_t *) 0);
		if (n < 0)
		{
			if (tout_flag)
			{
			/*	The recvfrom() above timed out.  See if
			 *	we've retransmitted enough, and if so
			 *	quit, otherwise try again.		*/

				if (rtt_timeout(&rttinfo) < 0)
				{
					perror("dgsendrecv: no response from \
server");
					rttfirst = 1;	/* reinit for next */
					return -1; /* errno will be EINTR */
				}

				/*	Must send the datagram again.	*/

				errno = 0;	/* clear the error flag */
#ifdef	DEBUG
				perror("dgsendrecv: timeout, retransmitting");
				rtt_d_flag = 1;
				rtt_debug(&rttinfo);
#endif
				continue;
			}

			perror("dgsendrecv: recvfrom error");
			return -1;
		}

		break;	/*	Out of while(1) loop.			*/
	}

	alarm(0);		/* stop signal timer */
	rtt_stop(&rttinfo);	/* stop RTT timer, calc & store new values */
#ifdef	DEBUG
	rtt_debug(&rttinfo);
#endif
	return(n);		/* return size of received datagram */
}

static int	rsend(int sock, char *line, int lineLen)
{
	static long	seqCounter;
	char		buffer[256];
	long		ack;

	seqCounter++;
	memcpy(buffer, (char *) &seqCounter, sizeof seqCounter);
	memcpy(buffer + 4, line, lineLen);
	while (1)
	{
		if (dgsendrecv(sock, buffer, lineLen + 4, (char *) &ack,
				sizeof ack, &socketName, sizeof socketName) < 0)
		{
			perror("dgsendrecv failed");
			return -1;
		}

		if (ack == seqCounter)
		{
			return 0;
		}
	}
}

static void	report(struct timeval *startTime, unsigned long bytesSent)
{
	struct timeval	endTime;
	unsigned long	usec;
	float		rate;

	getCurrentTime(&endTime);
	if (endTime.tv_usec < startTime->tv_usec)
	{
		endTime.tv_sec--;
		endTime.tv_usec += 1000000;
	}

	usec = ((endTime.tv_sec - startTime->tv_sec) * 1000000)
			+ (endTime.tv_usec - startTime->tv_usec);
	printf("Bytes sent = %lu, usec elapsed = %lu.\n", bytesSent, usec);
	rate = (float) (8 * bytesSent) / (float) (usec / 1000000);
	printf("Sending %7.2f bits per second.\n", rate);
}

int	main(int argc, char **argv)
{
	int			cyclesLeft;
	char			*remoteHostName;
	unsigned long		remoteIpAddress;
	unsigned short		portNbr = TEST_PORT_NBR;
	unsigned long		hostNbr;
	int			sock;
	char			*fileName;
	FILE			*inputFile;
	char			line[256];
	int			lineLen;
	struct timeval		startTime;
	unsigned long		bytesSent;

	if (argc < 3)
	{
		puts("Usage:  file2udp <remote host> <name of file to copy> \
[<number of cycles; default = 1>]");
		exit(0);
	}

	if (argc > 3)
	{
		cyclesRequested = atoi(argv[3]);
		if (cyclesRequested < 1)
		{
			cyclesRequested = 1;
		}
	}

	cyclesLeft = cyclesRequested;

	/*	Prepare for UDP transmissions.				*/

	remoteHostName = argv[1];
	remoteIpAddress = getInternetAddress(remoteHostName);
	hostNbr = htonl(remoteIpAddress);
	memset((char *) &socketName, 0, sizeof socketName);
	inetName = (struct sockaddr_in *) &socketName;
	inetName->sin_family = AF_INET;
	inetName->sin_port = htons(portNbr);
	memcpy((char *) &(inetName->sin_addr.s_addr), (char *) &hostNbr, 4);
	sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sock < 0)
	{
		perror("Can't open udp socket");
		exit(0);
	}

	/*	Open file to send.					*/

	fileName = argv[2];
	inputFile = fopen(fileName, "r");
	if (inputFile == NULL)
	{
		perror("Can't open input file");
		exit(0);
	}

	eofLineLen = strlen(eofLine);
	getCurrentTime(&startTime);
	bytesSent = 0;

	/*	Copy text lines from file to SDR.			*/

	while (cyclesLeft > 0)
	{
		if (fgets(line, 256, inputFile) == NULL)
		{
			if (feof(inputFile))
			{
				if (rsend(sock, eofLine, eofLineLen) < 0)
				{
					exit(0);
				}

				bytesSent += eofLineLen;
				fclose(inputFile);
				inputFile = fopen(fileName, "r");
				if (inputFile == NULL)
				{
					perror("Can't reopen input file");
					exit(0);
				}

				cyclesLeft--;
				continue;
			}
			else
			{
				perror("Can't read from input file");
				exit(0);
			}
		}

		lineLen = strlen(line);
		if (rsend(sock, line, lineLen) < 0)
		{
			exit(0);
		}

		bytesSent += lineLen;
	}

	report(&startTime, bytesSent);
	close(sock);
	return 0;
}
