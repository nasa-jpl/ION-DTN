/*
	lgsend.c:	driver for BP-based load-and-go system.
									*/
/*									*/
/*	Copyright (c) 2008, California Institute of Technology.		*/
/*	All rights reserved.						*/
/*	Author: Scott Burleigh, Jet Propulsion Laboratory		*/
/*									*/

#include <bp.h>

#ifdef VXWORKS
int	lgsend(int a1, int a2, int a3, int a4, int a5,
		int a6, int a7, int a8, int a9, int a10)
{
	char	*cmdFileName = (char *) a1;
	char	*ownEid = (char *) a2;
	char	*destEid = (char *) a3;
#else
int	main(int argc, char **argv)
{
	char	*cmdFileName = (argc > 1 ? argv[1] : NULL);
	char	*ownEid = (argc > 2 ? argv[2] : NULL);
	char	*destEid = (argc > 3 ? argv[3] : NULL);
#endif
	BpSAP	sap;
	Sdr	sdr;
	FILE	*cmdFile;
	long	fileSize;
	Object	adu;
	int	offset;
	char	line[256];
	int	lineLength;
	Object	bundleZco;
	Object	newBundle;

	if (cmdFileName == NULL || ownEid == NULL ||destEid == NULL)
	{
		PUTS("Usage: lgsend <LG cmd file name> <own endpoint ID> \
<destination endpoint ID>");
		return 0;
	}

	if (bp_attach() < 0)
	{
		putErrmsg("lgsend: can't attach to BP.", NULL);
		return 1;
	}

	if (bp_open(ownEid, &sap) < 0)
	{
		putErrmsg("lgsend: can't open own endpoint.", NULL);
		return 1;
	}

	sdr = bp_get_sdr();
	cmdFile = fopen(cmdFileName, "r");
	if (cmdFile == NULL)
	{
		bp_close(sap);
		putSysErrmsg("lgsend: can't open file of LG commands",
				cmdFileName);
		return 1;
	}

	if (fseek(cmdFile, 0, SEEK_END) < 0
	|| (fileSize = ftell(cmdFile)) < 0
	|| fseek(cmdFile, 0, SEEK_SET) < 0)
	{
		fclose(cmdFile);
		bp_close(sap);
		putSysErrmsg("lgsend: can't get size of LG command file",
				cmdFileName);
		return 1;
	}

	if (fileSize > 64000)
	{
		fclose(cmdFile);
		bp_close(sap);
		putSysErrmsg("lgsend: LG cmd file size > 64000.",
				itoa(fileSize));
		return 1;
	}

	sdr_begin_xn(sdr);
	adu = sdr_malloc(sdr, fileSize);
	if (adu == 0)
	{
		sdr_cancel_xn(sdr);
		fclose(cmdFile);
		bp_close(sap);
		putErrmsg("lgsend: no space for application data unit.", NULL);
		return 1;
	}

	offset = 0;
	while (offset < fileSize)
	{
		if (fgets(line, sizeof line, cmdFile) == NULL)
		{
			sdr_cancel_xn(sdr);
			fclose(cmdFile);
			bp_close(sap);
			putSysErrmsg("lgsend: fgets failed", NULL);
			return 1;
		}

		/*	Write command file line into ADU.		*/

		lineLength = strlen(line);	/*	Includes \n.	*/
		sdr_write(sdr, adu + offset, line, lineLength);
		offset += lineLength;
	}

	fclose(cmdFile);
	bundleZco = zco_create(sdr, ZcoSdrSource, adu, 0, fileSize);
	if (sdr_end_xn(sdr) < 0 || bundleZco == 0)
	{
		bp_close(sap);
		putErrmsg("lgsend: can't create application data unit.", NULL);
		return 1;
	}

	/*	Now send the bundle to the destination DTN node.	*/

	if (bp_send(sap, BP_BLOCKING, destEid, NULL, 86400,
			BP_EXPEDITED_PRIORITY, SourceCustodyRequired,
			0, 0, NULL, bundleZco, &newBundle) < 1)
	{
		bp_close(sap);
		putErrmsg("lgsend: can't send bundle.", NULL);
		return 1;
	}

	bp_close(sap);
	writeErrmsgMemos();
	PUTS("lgsend: completed.");
	bp_detach();
	return 0;
}
