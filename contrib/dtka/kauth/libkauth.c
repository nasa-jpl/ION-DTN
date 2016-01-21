/*
	libkauth.c:	common functions for DTKA key authority
       			programs.

	Author: Scott Burleigh, JPL

	Copyright (c) 2013, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
	
									*/
#include "kauth.h"

static Object	_kauthdbObject(Object *newDbObj)
{
	static Object	obj = 0;

	if (newDbObj)
	{
		obj = *newDbObj;
	}

	return obj;
}

static DtkaAuthDB	*_kauthConstants()
{
	static DtkaAuthDB	buf;
	static DtkaAuthDB	*db = NULL;
	Sdr			sdr;
	Object			dbObject;

	if (db == NULL)
	{
		/*	Load constants into a conveniently accessed
		 *	structure.  Note that this CANNOT be treated
		 *	as a current database image in later
		 *	processing.					*/

		sdr = getIonsdr();
		CHKNULL(sdr);
		dbObject = _kauthdbObject(NULL);
		if (dbObject)
		{
			if (sdr_heap_is_halted(sdr))
			{
				sdr_read(sdr, (char *) &buf, dbObject,
						sizeof(DtkaAuthDB));
			}
			else
			{
				CHKNULL(sdr_begin_xn(sdr));
				sdr_read(sdr, (char *) &buf, dbObject,
						sizeof(DtkaAuthDB));
				sdr_exit_xn(sdr);
			}

			db = &buf;
		}
	}

	return db;
}

static char	*_kauthvdbName()
{
	return "kauthvdb";
}

static DtkaAuthVdb	*_kauthvdb(char **name)
{
	static DtkaAuthVdb	*vdb = NULL;
	PsmPartition		wm;
	PsmAddress		vdbAddress;
	PsmAddress		elt;
	Sdr			sdr;

	if (name)
	{
		if (*name == NULL)	/*	Terminating.		*/
		{
			vdb = NULL;
			return vdb;
		}

		/*	Attaching to volatile database.			*/

		wm = getIonwm();
		if (psm_locate(wm, *name, &vdbAddress, &elt) < 0)
		{
			putErrmsg("Failed searching for vdb.", NULL);
			return vdb;
		}

		if (elt)
		{
			vdb = (DtkaAuthVdb *) psp(wm, vdbAddress);
			return vdb;
		}

		/*	DTKA volatile database doesn't exist yet.	*/

		sdr = getIonsdr();
		CHKNULL(sdr_begin_xn(sdr));	/*	To lock memory.	*/
		vdbAddress = psm_zalloc(wm, sizeof(DtkaAuthVdb));
		if (vdbAddress == 0)
		{
			sdr_exit_xn(sdr);
			putErrmsg("No space for volatile database.", NULL);
			return NULL;
		}

		vdb = (DtkaAuthVdb *) psp(wm, vdbAddress);
		memset((char *) vdb, 0, sizeof(DtkaAuthVdb));
		vdb->recvPid = ERROR;		/*	None yet.	*/
		vdb->compilePid = ERROR;	/*	None yet.	*/
		if (psm_catlg(wm, *name, vdbAddress) < 0)
		{
			sdr_exit_xn(sdr);
			putErrmsg("Can't initialize volatile database.", NULL);
			return NULL;
		}

		sdr_exit_xn(sdr);	/*	Unlock wm.		*/
	}

	return vdb;
}

static char	*_kauthdbName()
{
	return "kauthdb";
}

int	kauthInit()
{
	Sdr		sdr;
	Object		kauthdbObject;
	DtkaAuthDB	kauthdbBuf;
	char		*kauthvdbName = _kauthvdbName();

	if (bp_attach() < 0)
	{
		putErrmsg("DTKA can't attach to ION.", NULL);
		return -1;
	}

	sdr = getIonsdr();

	/*	Recover the DTKA database, creating it if necessary.	*/

	CHKERR(sdr_begin_xn(sdr));
	kauthdbObject = sdr_find(sdr, _kauthdbName(), NULL);
	switch (kauthdbObject)
	{
	case -1:		/*	SDR error.			*/
		sdr_cancel_xn(sdr);
		putErrmsg("Can't search for DTKA database in SDR.", NULL);
		return -1;

	case 0:			/*	Not found; must create new DB.	*/
		kauthdbObject = sdr_malloc(sdr, sizeof(DtkaAuthDB));
		if (kauthdbObject == 0)
		{
			sdr_cancel_xn(sdr);
			putErrmsg("No space for database.", NULL);
			return -1;
		}

		/*	Initialize the non-volatile database.		*/

		memset((char *) &kauthdbBuf, 0, sizeof(DtkaAuthDB));

		/*	Default values.					*/

		kauthdbBuf.nextCompilationTime = 0;
		kauthdbBuf.compilationInterval = 3600;	/*	Hourly.	*/
		kauthdbBuf.consensusInterval = 60;	/*	60 sec.	*/

		/*	Management information.				*/

		kauthdbBuf.currentRecords = sdr_list_create(sdr);
		kauthdbBuf.pendingRecords = sdr_list_create(sdr);
		sdr_write(sdr, kauthdbObject, (char *) &kauthdbBuf,
				sizeof(DtkaAuthDB));
		sdr_catlg(sdr, _kauthdbName(), 0, kauthdbObject);
		if (sdr_end_xn(sdr))
		{
			putErrmsg("Can't create DTKA database.", NULL);
			return -1;
		}

		break;

	default:			/*	Found DB in the SDR.	*/
		sdr_exit_xn(sdr);
	}

	oK(_kauthdbObject(&kauthdbObject));/*	Save database location.	*/
	oK(_kauthConstants());

	/*	Locate volatile database, initializing as necessary.	*/

	if (_kauthvdb(&kauthvdbName) == NULL)
	{
		putErrmsg("DTKA can't initialize vdb.", NULL);
		return -1;
	}

	return 0;		/*	DTKA service is available.	*/
}

int	kauthStart()
{
	Sdr		sdr = getIonsdr();
	DtkaAuthVdb	*kauthvdb = _kauthvdb(NULL);

	CHKERR(sdr_begin_xn(sdr));	/*	Just to lock memory.	*/

	/*	Start the DTKA receiver daemon if necessary.		*/

	if (kauthvdb->recvPid == ERROR
	|| sm_TaskExists(kauthvdb->recvPid) == 0)
	{
		kauthvdb->recvPid = pseudoshell("karecv");
	}

	/*	Start the DTKA compiler daemon if necessary.		*/

	if (kauthvdb->compilePid == ERROR
	|| sm_TaskExists(kauthvdb->compilePid) == 0)
	{
		kauthvdb->compilePid = pseudoshell("kacompile");
	}

	sdr_exit_xn(sdr);	/*	Unlock memory.			*/
	return 0;
}

int	kauthIsStarted()
{
	DtkaAuthVdb	*vdb = _kauthvdb(NULL);

	return (vdb && vdb->recvPid != ERROR && vdb->compilePid != ERROR);
}

void	kauthStop()		/*	Reverses kauthStart.		*/
{
	Sdr		sdr = getIonsdr();
	DtkaAuthVdb	*kauthvdb = _kauthvdb(NULL);

	/*	Tell all DTKA authority processes to stop.		*/

	CHKVOID(sdr_begin_xn(sdr));	/*	Just to lock memory.	*/

	/*	Stop receiver task.					*/

	if (kauthvdb->recvPid != ERROR)
	{
		sm_TaskKill(kauthvdb->recvPid, SIGTERM);
	}

	/*	Stop compiler task.					*/

	if (kauthvdb->compilePid != ERROR)
	{
		sm_TaskKill(kauthvdb->compilePid, SIGTERM);
	}

	sdr_exit_xn(sdr);	/*	Unlock memory.			*/

	/*	Wait until all DTKA processes have stopped.		*/

	if (kauthvdb->recvPid != ERROR)
	{
		while (sm_TaskExists(kauthvdb->recvPid))
		{
			microsnooze(100000);
		}
	}

	if (kauthvdb->compilePid != ERROR)
	{
		while (sm_TaskExists(kauthvdb->compilePid))
		{
			microsnooze(100000);
		}
	}

	/*	Now erase all the tasks.				*/

	CHKVOID(sdr_begin_xn(sdr));	/*	Just to lock memory.	*/
	kauthvdb->recvPid = ERROR;
	kauthvdb->compilePid = ERROR;
	sdr_exit_xn(sdr);		/*	Unlock memory.		*/
}

int	kauthAttach()
{
	Object		kauthdbObject = _kauthdbObject(NULL);
	DtkaAuthVdb	*kauthvdb = _kauthvdb(NULL);
	Sdr		sdr;
	char		*kauthvdbName = _kauthvdbName();

	if (kauthdbObject && kauthvdb)
	{
		return 0;		/*	Already attached.	*/
	}

	if (bp_attach() < 0)
	{
		putErrmsg("DTKA can't attach to BP.", NULL);
		return -1;
	}

	sdr = getIonsdr();

	/*	Locate the DTKA database.				*/

	if (kauthdbObject == 0)
	{
		CHKERR(sdr_begin_xn(sdr));	/*	Lock database.	*/
		kauthdbObject = sdr_find(sdr, _kauthdbName(), NULL);
		sdr_exit_xn(sdr);	/*	Unlock database.	*/
		if (kauthdbObject == 0)
		{
			putErrmsg("Can't find DTKA database.", NULL);
			return -1;
		}

		oK(_kauthdbObject(&kauthdbObject));
	}

	oK(_kauthConstants());

	/*	Locate the DTKA volatile database.			*/

	if (kauthvdb == NULL)
	{
		if (_kauthvdb(&kauthvdbName) == NULL)
		{
			putErrmsg("DTKA volatile database not found.", NULL);
			return -1;
		}
	}

	return 0;		/*	DTKA service is available.	*/
}

Object	getKauthDbObject()
{
	return _kauthdbObject(NULL);
}

DtkaAuthDB	*getKauthConstants()
{
	return _kauthConstants();
}
