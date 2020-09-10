/*
	kacompile.c:	periodic bulletin compilation daemon for
			DTKA.

	Author: Scott Burleigh, JPL

	Copyright (c) 2013, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
	
									*/
#include "kauth.h"

static long	_running(long *newValue)
{
	void	*value;
	long	state;
	
	if (newValue)			/*	Changing state.		*/
	{
		value = (void *) (*newValue);
		state = (long) sm_TaskVar(&value);
	}
	else				/*	Just check.		*/
	{
		state = (long) sm_TaskVar(NULL);
	}

	return state;
}

static void	shutDown()	/*	Commands kacompile termination.	*/
{
	long	stop = 0;

	oK(_running(&stop));	/*	Terminates kacompile.		*/
}

static int	publishBulletin(Sdr sdr, DtkaAuthDB *db, BpSAP sap)
{
	int		buflen;
	unsigned char	*buffer;
	unsigned char	*cursor;
	unsigned int	bytesRemaining;
	Object		elt;
			OBJ_POINTER(DtkaRecord, rec);
	int		recLen;
	int		bulletinLen = 0;
	char		fileName[32];
	int		fd;
	time_t		bulletinId;
	char		destEid[32];
	Object		fileRef;
	Object		zco;
	int		result;
	Object		newBundle;

	/*	Allocate temporary buffer of the maximum size that
	 *	might be needed to hold the entire bulletin.		*/

	buflen = sdr_list_length(sdr, db->pendingRecords) * DTKA_MAX_REC;
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
		GET_OBJ_POINTER(sdr, DtkaRecord, rec, sdr_list_data(sdr, elt));
		recLen = dtka_serialize(cursor, bytesRemaining, rec->nodeNbr,
				&(rec->effectiveTime), rec->assertionTime,
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

	isprintf(fileName, 32, "dtka_proposed.%d", db->currentCompilationTime);
	fd = iopen(fileName, O_WRONLY | O_CREAT | O_TRUNC, 0777);
	if (fd < 0)
	{
		putSysErrmsg("Can't create bulletin file", fileName);
		if (buffer) MRELEASE(buffer);
		return -1;
	}

	bulletinId = htonl(db->currentCompilationTime);
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
	isprintf(destEid, 32, "imc:%d.0", DTKA_CONFER);
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

	return result;
}

#if defined (VXWORKS) || defined (RTEMS) || defined (bionic)
int	kacompile(int a1, int a2, int a3, int a4, int a5,
		int a6, int a7, int a8, int a9, int a10)
{
#else
int	main(int argc, char *argv[])
{
#endif
	char		ownEid[32];
	BpSAP		sap;
	Sdr		sdr;
	Object		dbobj;
	DtkaAuthDB	db;
	long		state = 1;
	time_t		currentTime;
	int		interval;

	if (kauthAttach() < 0)
	{
		putErrmsg("kacompile can't attach to dtka.", NULL);
		return 1;
	}

	isprintf(ownEid, sizeof ownEid, "ipn:" UVAST_FIELDSPEC ".%d",
			getOwnNodeNbr(), DTKA_CONFER);
	if (bp_open(ownEid, &sap) < 0)
	{
		putErrmsg("Can't open own endpoint.", ownEid);
		ionDetach();
		return 1;
	}

	sdr = getIonsdr();
	dbobj = getKauthDbObject();
	if (dbobj == 0)
	{
		putErrmsg("No DTKA authority database.", NULL);
		ionDetach();
		return 1;
	}

	sdr_read(sdr, (char *) &db, dbobj, sizeof(DtkaAuthDB));
	isignal(SIGTERM, shutDown);

	/*	Main loop: wait for next compilation opportunity,
	 *	then compile and publish proposed bulletin and
	 *	schedule next compilation.				*/

	oK(_running(&state));
	writeMemo("[i] kacompile is running.");
	while (_running(NULL))
	{
		/*	Sleep until 5 seconds before the next bulletin
		 *	compilation opportunity.			*/

		currentTime = getCtime();
		interval = db.nextCompilationTime - currentTime;
		if (interval > 5)
		{
			snooze(interval - 5);
			continue;	/*	In case interrupted.	*/
		}

		/*	Time to start compiling a bulletin.		*/

		if (sdr_begin_xn(sdr) < 0)
		{
			putErrmsg("Can't update DTKA next compile time.", NULL);
			state = 0;
			oK(_running(&state));
			continue;
		}

		sdr_stage(sdr, (char *) &db, dbobj, sizeof(DtkaAuthDB));
		db.currentCompilationTime = db.nextCompilationTime;
		db.nextCompilationTime += db.compilationInterval;
		sdr_write(sdr, dbobj, (char *) &db, sizeof(DtkaAuthDB));
		if (sdr_end_xn(sdr) < 0)
		{
			putErrmsg("Can't schedule next compilation.", NULL);
			state = 0;	/*	Terminate loop.		*/
			oK(_running(&state));
		}

		/*	Start bulletin publisher to handle the current
		 *	compilation.					*/

		if (pseudoshell("kapublish") == ERROR)
		{
			putErrmsg("Can't spawn bulletin publisher.", NULL);
			state = 0;	/*	Terminate loop.		*/
			oK(_running(&state));
		}

		/*	Wait until scheduled compilation time, about
		 *	another 5 seconds.  At minimum, give kapublish
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
	writeMemo("[i] kacompile has ended.");
	ionDetach();
	return 0;
}
