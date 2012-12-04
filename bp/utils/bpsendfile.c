/*
	bpsendfile.c:	test program to send a file as a bundle.
									*/
/*									*/
/*	Copyright (c) 2006, California Institute of Technology.		*/
/*	All rights reserved.						*/
/*	Author: Scott Burleigh, Jet Propulsion Laboratory		*/
/*									*/

#include <bpP.h>

static int	run_bpsendfile(char *ownEid, char *destEid, char *fileName,
			char *svcClass)
{
	int		priority = 0;
	BpExtendedCOS	extendedCOS = { 0, 0, 0 };
	BpCustodySwitch	custodySwitch = NoCustodyRequested;
	BpSAP		sap;
	Sdr		sdr;
	Object		fileRef;
	struct stat	statbuf;
	int		aduLength;
	Object		bundleZco;
	Object		newBundle;

	if (svcClass == NULL)
	{
		priority = BP_STD_PRIORITY;
	}
	else
	{
		if (!bp_parse_class_of_service(svcClass, &extendedCOS,
				&custodySwitch, &priority))
		{
			putErrmsg("Invalid class of service for bpsendfile.",
					svcClass);
			return 0;
		}
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

	if (stat(fileName, &statbuf) < 0)
	{
		bp_close(sap);
		putSysErrmsg("Can't stat the file", fileName);
		return 0;
	}

	aduLength = statbuf.st_size;
	sdr = bp_get_sdr();
	CHKZERO(sdr_begin_xn(sdr));
	if (sdr_heap_depleted(sdr))
	{
		sdr_exit_xn(sdr);
		bp_close(sap);
		putErrmsg("Low on heap space, can't send file.", fileName);
		return 0;
	}

	fileRef = zco_create_file_ref(sdr, fileName, NULL);
	if (fileRef == 0)
	{
		sdr_cancel_xn(sdr);
		bp_close(sap);
		putErrmsg("bpsendfile can't create file ref.", NULL);
		return 0;
	}
	
	bundleZco = zco_create(sdr, ZcoFileSource, fileRef, 0, aduLength);
	if (sdr_end_xn(sdr) < 0 || bundleZco == 0)
	{
		bp_close(sap);
		putErrmsg("bpsendfile can't create ZCO.", NULL);
		return 0;
	}

	while (1)
	{
		switch (bp_send(sap, BP_NONBLOCKING, destEid, NULL, 300,
				priority, custodySwitch, 0, 0,
				&extendedCOS, bundleZco, &newBundle))
		{
		case 0:		/*	No space for bundle.		*/
			if (errno == EWOULDBLOCK)
			{
				microsnooze(250000);
				continue;
			}

			/*	Intentional fall-through to next case.	*/

		case -1:
			putErrmsg("bpsendfile can't send file in bundle.",
					itoa(aduLength));

			/*	Intentional fall-through to next case.	*/

		default:
			break;	/*	Out of switch.			*/
		}

		break;		/*	Out of loop.			*/
	}

	bp_close(sap);
	writeErrmsgMemos();
	PUTS("Stopping bpsendfile.");
	CHKZERO(sdr_begin_xn(sdr));
	zco_destroy_file_ref(sdr, fileRef);
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("bpsendfile can't destroy file reference.", NULL);
	}

	bp_detach();
	return 0;
}

#if defined (VXWORKS) || defined (RTEMS)
int	bpsendfile(int a1, int a2, int a3, int a4, int a5,
		int a6, int a7, int a8, int a9, int a10)
{
	char	*ownEid = (char *) a2;
	char	*destEid = (char *) a3;
	char	*fileName = (char *) a4;
	char	*classOfService = (char *) a5;
#else
int	main(int argc, char **argv)
{
	char	*ownEid = NULL;
	char	*destEid = NULL;
	char	*fileName = NULL;
	char	*classOfService = NULL;

	if (argc > 5) argc = 5;
	switch (argc)
	{
	case 5:
		classOfService = argv[4];
	case 4:
		fileName = argv[3];
	case 3:
		destEid = argv[2];
	case 2:
		ownEid = argv[1];
	default:
		break;
	}
#endif
	if (ownEid == NULL || destEid == NULL || fileName == NULL)
	{
		PUTS("Usage: bpsendfile <own endpoint ID> <destination \
endpoint ID> <file name> [<class of service>]");
		PUTS("\tclass of service: " BP_PARSE_CLASS_OF_SERVICE_USAGE);
		return 0;
	}

	return run_bpsendfile(ownEid, destEid, fileName, classOfService);
}
