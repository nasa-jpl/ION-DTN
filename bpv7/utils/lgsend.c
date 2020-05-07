/*
	lgsend.c:	driver for BP-based load-and-go system.
									*/
/*									*/
/*	Copyright (c) 2008, California Institute of Technology.		*/
/*	All rights reserved.						*/
/*	Author: Scott Burleigh, Jet Propulsion Laboratory		*/
/*									*/

#include <bp.h>

#if defined (ION_LWT)
int	lgsend(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
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
	int	cmdFile;
	int	fileSize;
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
		return -1;
	}

	if (bp_open(ownEid, &sap) < 0)
	{
		putErrmsg("lgsend: can't open own endpoint.", NULL);
		return -1;
	}

	sdr = bp_get_sdr();
	cmdFile = iopen(cmdFileName, O_RDONLY, 0777);
	if (cmdFile < 0)
	{
		bp_close(sap);
		putSysErrmsg("lgsend: can't open file of LG commands",
				cmdFileName);
		return -1;
	}

	if ((fileSize = lseek(cmdFile, 0, SEEK_END)) < 0
	|| lseek(cmdFile, 0, SEEK_SET) < 0)
	{
		close(cmdFile);
		bp_close(sap);
		putSysErrmsg("lgsend: can't get size of LG command file",
				cmdFileName);
		return -1;
	}

	fileSize += 1;		/*	Make room for final newline.	*/
	if (fileSize > 64000)
	{
		close(cmdFile);
		bp_close(sap);
		putErrmsg("lgsend: LG cmd file size > 64000.",
				itoa(fileSize));
		return -1;
	}

	CHKERR(sdr_begin_xn(sdr));
	adu = sdr_malloc(sdr, fileSize);
	if (adu == 0)
	{
		sdr_cancel_xn(sdr);
		close(cmdFile);
		bp_close(sap);
		putErrmsg("lgsend: no space for application data unit.", NULL);
		return -1;
	}

	offset = 0;
	while (1)
	{
		if (igets(cmdFile, line, sizeof line, &lineLength) == NULL)
		{
			if (lineLength == 0)	/*	End of file.	*/
			{
				break;		/*	Out of loop.	*/
			}

			sdr_cancel_xn(sdr);
			close(cmdFile);
			bp_close(sap);
			putErrmsg("igets failed.", NULL);
			return -1;
		}

		/*	Newline (and CR, if any) has been stripped
		 *	from command file line at this point, and
		 *	command file line has been truncated as
		 *	necessary to enable it to be NULL-terminated.
		 *	Replace the NULL with a newline and write to
		 *	SDR object.					*/

		line[lineLength] = '\n';
		lineLength += 1;		/*	For newline.	*/
		if (offset + lineLength > fileSize)
		{
			sdr_cancel_xn(sdr);
			close(cmdFile);
			bp_close(sap);
			putErrmsg("File size error.", NULL);
			return -1;
		}

		sdr_write(sdr, adu + offset, line, lineLength);
		offset += lineLength;
	}

	/*	May have to fill with newlines to replace stripped-
	 *	out CRs and/or truncated text.				*/

	line[0] = '\n';
	while (offset < fileSize)
	{
		sdr_write(sdr, adu + offset, line, 1);
		offset += 1;
	}

	close(cmdFile);
	if (sdr_end_xn(sdr) < 0)
	{
		bp_close(sap);
		putErrmsg("Failed creating command object.", NULL);
		return -1;
	}

	bundleZco = ionCreateZco(ZcoSdrSource, adu, 0, fileSize,
			BP_EXPEDITED_PRIORITY, 0, ZcoOutbound, NULL);
	if (bundleZco == 0 || bundleZco == (Object) ERROR)
	{
		putErrmsg("lgsend: can't create application data unit.", NULL);
	}
	else
	{
		if (bp_send(sap, destEid, NULL, 86400, BP_EXPEDITED_PRIORITY,
				SourceCustodyRequired, 0, 0, NULL, bundleZco,
				&newBundle) < 1)
		{
			putErrmsg("lgsend: can't send bundle.", NULL);
		}
	}

	bp_close(sap);
	writeErrmsgMemos();
	PUTS("lgsend: completed.");
	bp_detach();
	return 0;
}
