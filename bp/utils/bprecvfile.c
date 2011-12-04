/*
	bprecvfile.c:	Test program for receiving a files sent as
			a bundle.
									*/
/*									*/
/*	Copyright (c) 2008, California Institute of Technology.		*/
/*	All rights reserved.						*/
/*	Author: Scott Burleigh, Jet Propulsion Laboratory		*/
/*									*/

#include <bp.h>

#define	BPRECVBUFSZ	(65536)

static BpSAP	_bpsap(BpSAP *newSAP)
{
	static BpSAP	sap = NULL;

	if (newSAP)
	{
		sap = *newSAP;
		sm_TaskVarAdd((int *) &sap);
	}

	return sap;
}

static int	_running(int *newState)
{
	static int	state = 1;

	if (newState)
	{
		state = *newState;
	}

	return state;
}

static void	handleQuit()
{
	int	stop = 0;

	writeMemo("[i] bprecvfile interrupted.");
	oK(_running(&stop));
	bp_interrupt(_bpsap(NULL));
}

static int	receiveFile(Sdr sdr, BpDelivery *dlv)
{
	static char	buffer[BPRECVBUFSZ];
	static int	fileCount = 0;
	int		contentLength;
	int		remainingLength;
	char		fileName[64];
	int		testFile = -1;
	ZcoReader	reader;
	int		recvLength;
	char		completionText[80];

	fileCount++;
	isprintf(fileName, sizeof fileName, "testfile%d", fileCount);
	contentLength = zco_source_data_length(sdr, dlv->adu);
	testFile = iopen(fileName, O_WRONLY | O_CREAT, 0666);
	if (testFile < 0)
	{
		putSysErrmsg("bprecvfile: can't open test file", fileName);
		return -1;
	}

	sdr_begin_xn(sdr);
	zco_start_receiving(sdr, dlv->adu, &reader);
	remainingLength = contentLength;
	while (remainingLength > 0)
	{
		recvLength = BPRECVBUFSZ;
		if (remainingLength < recvLength)
		{
			recvLength = remainingLength;
		}

		if (zco_receive_source(sdr, &reader, recvLength, buffer) < 0)
		{
			zco_stop_receiving(sdr, &reader);
			close(testFile);
			sdr_cancel_xn(sdr);
			putErrmsg("bprecvfile: can't receive bundle content.",
					fileName);
			return -1;
		}

		if (write(testFile, buffer, recvLength) < 1)
		{
			zco_stop_receiving(sdr, &reader);
			close(testFile);
			sdr_cancel_xn(sdr);
			putSysErrmsg("bprecvfile: can't write to test file",
					fileName);
			return -1;
		}

		remainingLength -= recvLength;
	}

	zco_stop_receiving(sdr, &reader);
	close(testFile);
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("bprecvfile: can't handle bundle delivery.",
				fileName);
		return -1;
	}

	isprintf(completionText, sizeof completionText, "bprecvfile has \
created '%s', size %d.", fileName, contentLength);
	writeMemo(completionText);
	return 0;
}

#ifdef VXWORKS
int	bprecvfile(int a1, int a2, int a3, int a4, int a5,
		int a6, int a7, int a8, int a9, int a10)
{
	char		*ownEid = (char *) a1;
#else
int	main(int argc, char **argv)
{
	char		*ownEid = (argc > 1 ? argv[1] : NULL);
#endif
	BpSAP		sap;
	Sdr		sdr;
	BpDelivery	dlv;
	int		stop = 0;

	if (ownEid == NULL)
	{
		PUTS("Usage: bprecvfile <own endpoint ID>");
		return 0;
	}

	if (bp_attach() < 0)
	{
		putErrmsg("Can't attach to BP.", NULL);
		return -1;
	}

	if (bp_open(ownEid, &sap) < 0)
	{
		putErrmsg("Can't open own endpoint.", ownEid);
		return -1;
	}

	oK(_bpsap(&sap));
	sdr = bp_get_sdr();
	isignal(SIGINT, handleQuit);
	writeMemo("[i] bprecvfile is running.");
	while (_running(NULL))
	{
		if (bp_receive(sap, &dlv, BP_BLOCKING) < 0)
		{
			putErrmsg("bprecvfile bundle reception failed.", NULL);
			oK(_running(&stop));
			continue;
		}

		switch (dlv.result)
		{
		case BpEndpointStopped:
			oK(_running(&stop));
			break;		/*	Out of switch.		*/

		case BpPayloadPresent:
			if (receiveFile(sdr, &dlv) < 0)
			{
				putErrmsg("bprecvfile cannot continue.", NULL);
				oK(_running(&stop));
			}

			/*	Intentional fall-through to default.	*/

		default:
			break;		/*	Out of switch.		*/
		}

		bp_release_delivery(&dlv, 1);
	}

	bp_close(sap);
	writeErrmsgMemos();
	writeMemo("[i] Stopping bprecvfile.");
	bp_detach();
	return 0;
}
