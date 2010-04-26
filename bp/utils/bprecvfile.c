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

static int	running;
static BpSAP	sap;

static void	handleQuit()
{
	running = 0;
	bp_interrupt(sap);
}

static int	receiveFile(Sdr sdr, BpDelivery *dlv)
{
	static int	fileCount = 0;
	int		contentLength;
	int		remainingLength;
	char		fileName[64];
	FILE		*testFile = NULL;
	ZcoReader	reader;
	int		recvLength;
	char		buffer[BPRECVBUFSZ];
	char		completionText[80];

	fileCount++;
	isprintf(fileName, sizeof fileName, "testfile%d", fileCount);
	contentLength = zco_source_data_length(sdr, dlv->adu);
	testFile = fopen(fileName, "w");
	if (testFile == NULL)
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
			fclose(testFile);
			sdr_cancel_xn(sdr);
			putErrmsg("bprecvfile: can't receive bundle content.",
					fileName);
			return -1;
		}

		if (fwrite(buffer, recvLength, 1, testFile) < 1)
		{
			zco_stop_receiving(sdr, &reader);
			fclose(testFile);
			sdr_cancel_xn(sdr);
			putSysErrmsg("bprecvfile: can't write to test file",
					fileName);
			return -1;
		}

		remainingLength -= recvLength;
	}

	zco_stop_receiving(sdr, &reader);
	fclose(testFile);
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
	Sdr		sdr;
	BpDelivery	dlv;

	if (ownEid == NULL)
	{
		PUTS("Usage: bprecvfile <own endpoint ID>");
		return 0;
	}

	if (bp_attach() < 0)
	{
		putErrmsg("Can't attach to BP.", NULL);
		return 0;
	}

	if (bp_open(ownEid, &sap) < 0)
	{
		putErrmsg("Can't open own endpoint.", ownEid);
		return 0;
	}

	sdr = bp_get_sdr();
	signal(SIGINT, handleQuit);
	running = 1;
	writeMemo("[i] bprecvfile is running.");
	while (running)
	{
		if (bp_receive(sap, &dlv, BP_BLOCKING) < 0)
		{
			putErrmsg("bprecvfile bundle reception failed.", NULL);
			running = 0;
			continue;
		}

		if (dlv.result == BpPayloadPresent)
		{
			if (receiveFile(sdr, &dlv) < 0)
			{
				putErrmsg("bprecvfile cannot continue.", NULL);
				running = 0;
			}
		}

		bp_release_delivery(&dlv, 1);
	}

	bp_close(sap);
	writeErrmsgMemos();
	writeMemo("[i] Stopping bprecvfile.");
	bp_detach();
	return 0;
}
