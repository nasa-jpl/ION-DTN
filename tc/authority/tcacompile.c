/*
	tcacompile.c:	periodic bulletin compilation daemon for
			a Trusted Collective authority.

	Author: Scott Burleigh, JPL

	Copyright (c) 2020, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
	
									*/
#include "tcaP.h"

static saddr	_running(saddr *newValue)
{
	void	*value;
	saddr	state;
	
	if (newValue)			/*	Changing state.		*/
	{
		value = (void *) (*newValue);
		state = (saddr) sm_TaskVar(&value);
	}
	else				/*	Just check.		*/
	{
		state = (saddr) sm_TaskVar(NULL);
	}

	return state;
}

static void	shutDown()	/*	Commands tcacompile shutdown.	*/
{
	saddr	stop = 0;

	oK(_running(&stop));	/*	Terminates tcacompile.		*/
}

static int	publishBulletin(Sdr sdr, TcaDB *db, BpSAP sap)
{
	int		buflen;
	char		*buffer;
	char		*cursor;
	unsigned int	bytesRemaining;
	Object		elt;
			OBJ_POINTER(TcaRecord, rec);
	int		recLen;
	int		bulletinLen = 0;
	char		fileName[32];
	int		fd;
	uint32_t	bulletinId;
	char		destEid[32];
	Object		fileRef;
	Object		zco;
	int		result;
	Object		newBundle;

	/*	Allocate temporary buffer of the maximum size that
	 *	might be needed to hold the entire bulletin.		*/

	buflen = sdr_list_length(sdr, db->pendingRecords) * TC_MAX_REC;
	if (buflen == 0)
	{
		buffer = NULL;
	}
	else
	{
		buffer = MTAKE(buflen);
		if (buffer == NULL)
		{
			putErrmsg("Not enough memory for proposed bulletin.",
					utoa(buflen));
			return -1;
		}
	}

	cursor = buffer;
	bytesRemaining = buflen;
	for (elt = sdr_list_first(sdr, db->pendingRecords); elt;
			elt = sdr_list_next(sdr, elt))
	{
		GET_OBJ_POINTER(sdr, TcaRecord, rec, sdr_list_data(sdr, elt));
		recLen = tc_serialize(cursor, bytesRemaining, rec->nodeNbr,
				rec->effectiveTime, rec->assertionTime,
				rec->datLength, rec->datValue);
		if (recLen < 0)
		{
			putErrmsg("Can't serialize record.", NULL);
			if (buffer) MRELEASE(buffer);
			return -1;
		}

		cursor += recLen;
		bytesRemaining -= recLen;
		bulletinLen += recLen;
	}

	/*	Now copy that temporary buffer to a file and use the
	 *	file as the application data unit for publishing the
	 *	bulletin.						*/

	isprintf(fileName, 32, "tca_proposed.%d", db->currentCompilationTime);
	fd = iopen(fileName, O_WRONLY | O_CREAT | O_TRUNC, 0777);
	if (fd < 0)
	{
		putSysErrmsg("Can't create bulletin file", fileName);
		if (buffer) MRELEASE(buffer);
		return -1;
	}

	bulletinId = db->currentCompilationTime;
	bulletinId = htonl(bulletinId);
	if (write(fd, (char *) &bulletinId, 4) < 0
	|| (buffer != NULL && write(fd, buffer, bulletinLen) < 0))
	{
		putSysErrmsg("Can't write to bulletin file", fileName);
		close(fd);
		if (buffer) MRELEASE(buffer);
		return -1;
	}

	close(fd);
	if (buffer) MRELEASE(buffer);
	isprintf(destEid, 32, "imc:%d.0", db->bulletinsGroupNbr);
	fileRef = zco_create_file_ref(sdr, fileName, "", ZcoOutbound);
	if (fileRef == 0)
	{
		putErrmsg("Can't create file ref.", NULL);
		unlink(fileName);
		return -1;
	}

	zco = ionCreateZco(ZcoFileSource, fileRef, 0, 4 + bulletinLen,
			BP_STD_PRIORITY, 0, ZcoOutbound, NULL);
	if (zco == 0 || zco == (Object) -1)
	{
		putErrmsg("Can't create ZCO.", NULL);
		zco_destroy_file_ref(sdr, fileRef);
		return -1;
	}

	result = bp_send(sap, destEid, NULL, db->consensusInterval + 5,
			BP_STD_PRIORITY, NoCustodyRequested, 0, 0, NULL,
			zco, &newBundle);
	zco_destroy_file_ref(sdr, fileRef);
	if (result < 0)
	{
		putErrmsg("Failed publishing proposed bulletin.", NULL);
	}

#ifdef TC_DEBUG
	writeMemo("tcacompile: published proposed bulletin");
#endif
	return result;
}

#if defined (ION_LWT)
int	tcacompile(int a1, int a2, int a3, int a4, int a5,
		int a6, int a7, int a8, int a9, int a10)
{
	int	blocksGroupNbr = (a1 ? atoi((char *) a1) : -1);
#else
int	main(int argc, char *argv[])
{
	int	blocksGroupNbr = (argc > 1 ? atoi(argv[1]) : -1);
#endif
	char	ownEid[32];
	BpSAP	sap;
	Sdr	sdr;
	Object	dbobj;
	TcaDB	db;
	saddr	state = 1;
	char	cmdbuf[32];
	time_t	currentTime;
	int	interval;

	if (blocksGroupNbr < 1)
	{
		puts("Usage: tcacompile <IMC group number for TC blocks>");
		return -1;
	}

	if (tcaAttach(blocksGroupNbr) < 0)
	{
		putErrmsg("tcacompile can't attach to tca system.",
				itoa(blocksGroupNbr));
		return 1;
	}

	dbobj = getTcaDBObject(blocksGroupNbr);
	if (dbobj == 0)
	{
		putErrmsg("No TCA authority database.", itoa(blocksGroupNbr));
		ionDetach();
		return 1;
	}

	isprintf(ownEid, sizeof ownEid, "ipn:" UVAST_FIELDSPEC ".0",
			getOwnNodeNbr());
	if (bp_open_source(ownEid, &sap, 0) < 0)
	{
		putErrmsg("Can't open own endpoint.", ownEid);
		ionDetach();
		return 1;
	}

	isignal(SIGTERM, shutDown);

	/*	Main loop: wait for next compilation opportunity,
	 *	then compile and publish proposed bulletin and
	 *	schedule next compilation.				*/

	sdr = getIonsdr();
	oK(_running(&state));
	writeMemo("[i] tcacompile is running.");
	while (_running(NULL))
	{
		/*	Sleep until 5 seconds before the next bulletin
		 *	compilation opportunity.			*/

		currentTime = getCtime();
		if (sdr_begin_xn(sdr) < 0)
		{
			putErrmsg("Can't update TCA next compile time.", NULL);
			state = 0;
			oK(_running(&state));
			continue;
		}

		sdr_stage(sdr, (char *) &db, dbobj, sizeof(TcaDB));
		interval = db.nextCompilationTime - currentTime;
		if (interval > 5)
		{
			sdr_exit_xn(sdr);
			snooze(interval - 5);
			continue;	/*	In case interrupted.	*/
		}

		/*	Time to start compiling a bulletin.		*/

		db.currentCompilationTime = db.nextCompilationTime;
		db.nextCompilationTime += db.compilationInterval;
		sdr_write(sdr, dbobj, (char *) &db, sizeof(TcaDB));
		if (sdr_end_xn(sdr) < 0)
		{
			putErrmsg("Can't schedule next compilation.", NULL);
			state = 0;	/*	Terminate loop.		*/
			oK(_running(&state));
			continue;
		}

		/*	Start bulletin publisher to handle the current
		 *	compilation.					*/

		isprintf(cmdbuf, sizeof cmdbuf, "tcapublish %d",
				blocksGroupNbr);
		if (pseudoshell(cmdbuf) == ERROR)
		{
			putErrmsg("Can't spawn bulletin publisher.", NULL);
			state = 0;	/*	Terminate loop.		*/
			oK(_running(&state));
			continue;
		}

		/*	Wait until scheduled compilation time, about
		 *	another 5 seconds.  At minimum, give tcapublish
		 *	time to get started.				*/

		currentTime = getCtime();
		interval = db.currentCompilationTime - currentTime;
		if (interval < 2)
		{
			interval = 2;
		}

		snooze(interval);

		/*	Now compile and publish proposed bulletin.	*/

		CHKZERO(sdr_begin_xn(sdr));
		if (publishBulletin(sdr, &db, sap) < 0)
		{
			sdr_cancel_xn(sdr);
			putErrmsg("Can't publish proposed bulletin.", NULL);
			state = 0;	/*	Terminate loop.		*/
			oK(_running(&state));
			continue;
		}

		if (sdr_end_xn(sdr) < 0)
		{
			putErrmsg("Can't schedule next compilation.", NULL);
			state = 0;	/*	Terminate loop.		*/
			oK(_running(&state));
		}
	}

	bp_close(sap);
	writeErrmsgMemos();
	writeMemo("[i] tcacompile has ended.");
	ionDetach();
	return 0;
}
