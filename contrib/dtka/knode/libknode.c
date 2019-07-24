/*
	libknode.c:	common functions for DTKA key management at
			ION nodes.

	Nodeor: Scott Burleigh, JPL

	Copyright (c) 2013, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
	
									*/
#include "knode.h"

static Object	_knodedbObject(Object *newDbObj)
{
	static Object	obj = 0;

	if (newDbObj)
	{
		obj = *newDbObj;
	}

	return obj;
}

static DtkaNodeDB	*_knodeConstants()
{
	static DtkaNodeDB	buf;
	static DtkaNodeDB	*db = NULL;
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
		dbObject = _knodedbObject(NULL);
		if (dbObject)
		{
			if (sdr_heap_is_halted(sdr))
			{
				sdr_read(sdr, (char *) &buf, dbObject,
						sizeof(DtkaNodeDB));
			}
			else
			{
				CHKNULL(sdr_begin_xn(sdr));
				sdr_read(sdr, (char *) &buf, dbObject,
						sizeof(DtkaNodeDB));
				sdr_exit_xn(sdr);
			}

			db = &buf;
		}
	}

	return db;
}

static char	*_knodevdbName()
{
	return "knodevdb";
}

static DtkaNodeVdb	*_knodevdb(char **name)
{
	static DtkaNodeVdb	*vdb = NULL;
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
			vdb = (DtkaNodeVdb *) psp(wm, vdbAddress);
			return vdb;
		}

		/*	DTKA volatile database doesn't exist yet.	*/

		sdr = getIonsdr();
		CHKNULL(sdr_begin_xn(sdr));	/*	To lock memory.	*/
		vdbAddress = psm_zalloc(wm, sizeof(DtkaNodeVdb));
		if (vdbAddress == 0)
		{
			sdr_exit_xn(sdr);
			putErrmsg("No space for volatile database.", NULL);
			return NULL;
		}

		vdb = (DtkaNodeVdb *) psp(wm, vdbAddress);
		memset((char *) vdb, 0, sizeof(DtkaNodeVdb));
		vdb->clockPid = ERROR;		/*	None yet.	*/
		vdb->mgrPid = ERROR;		/*	None yet.	*/
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

static char	*_knodedbName()
{
	return "knodedb";
}

int	knodeInit()
{
	Sdr		sdr;
	Object		knodedbObject;
	DtkaNodeDB	knodedbBuf;
	char		*knodevdbName = _knodevdbName();
	int		i;
	DtkaAuthority	*auth;
	int		seqLength = DTKA_FEC_Q / 2;
	int		sharenum;

	if (bp_attach() < 0)
	{
		putErrmsg("DTKA can't attach to ION.", NULL);
		return -1;
	}

	sdr = getIonsdr();

	/*	Recover the DTKA database, creating it if necessary.	*/

	CHKERR(sdr_begin_xn(sdr));
	knodedbObject = sdr_find(sdr, _knodedbName(), NULL);
	switch (knodedbObject)
	{
	case -1:		/*	SDR error.			*/
		sdr_cancel_xn(sdr);
		putErrmsg("Can't search for DTKA database in SDR.", NULL);
		return -1;

	case 0:			/*	Not found; must create new DB.	*/
		knodedbObject = sdr_malloc(sdr, sizeof(DtkaNodeDB));
		if (knodedbObject == 0)
		{
			sdr_cancel_xn(sdr);
			putErrmsg("No space for database.", NULL);
			return -1;
		}

		/*	Initialize the non-volatile database.		*/

		memset((char *) &knodedbBuf, 0, sizeof(DtkaNodeDB));

		/*	Default values.					*/

		knodedbBuf.nextKeyGenTime = getCtime() + 4;
		knodedbBuf.keyGenInterval = 604800;	/*	Weekly	*/
		knodedbBuf.effectiveLeadTime = 345600;	/*	4 days	*/
		for (i = 0, auth = knodedbBuf.authorities; i < DTKA_NUM_AUTHS;
				i++, auth++)
		{
			sharenum = seqLength * i;
			auth->firstPrimaryShare = sharenum;
			auth->lastPrimaryShare = (sharenum + seqLength) - 1;
			sharenum += seqLength;
			if (sharenum >= DTKA_FEC_M)
			{
				sharenum = 0;
			}

			auth->firstBackupShare = sharenum;
			auth->lastBackupShare = (sharenum + seqLength) - 1;
		}

		/*	Management information.				*/

		knodedbBuf.bulletins = sdr_list_create(sdr);
		sdr_write(sdr, knodedbObject, (char *) &knodedbBuf,
				sizeof(DtkaNodeDB));
		sdr_catlg(sdr, _knodedbName(), 0, knodedbObject);
		if (sdr_end_xn(sdr))
		{
			putErrmsg("Can't create DTKA database.", NULL);
			return -1;
		}

		break;

	default:			/*	Found DB in the SDR.	*/
		sdr_exit_xn(sdr);
	}

	oK(_knodedbObject(&knodedbObject));/*	Save database location.	*/
	oK(_knodeConstants());

	/*	Locate volatile database, initializing as necessary.	*/

	if (_knodevdb(&knodevdbName) == NULL)
	{
		putErrmsg("DTKA can't initialize vdb.", NULL);
		return -1;
	}

	return 0;		/*	DTKA service is available.	*/
}

int	knodeStart()
{
	Sdr		sdr = getIonsdr();
	DtkaNodeVdb	*knodevdb = _knodevdb(NULL);

	CHKERR(sdr_begin_xn(sdr));	/*	Just to lock memory.	*/

	/*	Start the DTKA manager daemon if necessary.		*/

	if (knodevdb->mgrPid == ERROR
	|| sm_TaskExists(knodevdb->mgrPid) == 0)
	{
		knodevdb->mgrPid = pseudoshell("knmgr");
	}

	/*	Start the DTKA clock daemon if necessary.		*/

	if (knodevdb->clockPid == ERROR
	|| sm_TaskExists(knodevdb->clockPid) == 0)
	{
		knodevdb->clockPid = pseudoshell("knclock");
	}

	sdr_exit_xn(sdr);	/*	Unlock memory.			*/
	return 0;
}

int	knodeIsStarted()
{
	DtkaNodeVdb	*vdb = _knodevdb(NULL);

	return (vdb && vdb->mgrPid != ERROR && vdb->clockPid != ERROR);
}

void	knodeStop()		/*	Reverses knodeStart.		*/
{
	Sdr		sdr = getIonsdr();
	DtkaNodeVdb	*knodevdb = _knodevdb(NULL);

	/*	Tell all DTKA authority processes to stop.		*/

	CHKVOID(sdr_begin_xn(sdr));	/*	Just to lock memory.	*/

	/*	Stop manager task.					*/

	if (knodevdb->mgrPid != ERROR)
	{
		sm_TaskKill(knodevdb->mgrPid, SIGTERM);
	}

	/*	Stop clock task.					*/

	if (knodevdb->clockPid != ERROR)
	{
		sm_TaskKill(knodevdb->clockPid, SIGTERM);
	}

	sdr_exit_xn(sdr);	/*	Unlock memory.			*/

	/*	Wait until all DTKA processes have stopped.		*/

	if (knodevdb->mgrPid != ERROR)
	{
		while (sm_TaskExists(knodevdb->mgrPid))
		{
			microsnooze(100000);
		}
	}

	if (knodevdb->clockPid != ERROR)
	{
		while (sm_TaskExists(knodevdb->clockPid))
		{
			microsnooze(100000);
		}
	}

	/*	Now erase all the tasks.				*/

	CHKVOID(sdr_begin_xn(sdr));	/*	Just to lock memory.	*/
	knodevdb->mgrPid = ERROR;
	knodevdb->clockPid = ERROR;
	sdr_exit_xn(sdr);		/*	Unlock memory.		*/
}

int	knodeAttach()
{
	Object		knodedbObject = _knodedbObject(NULL);
	DtkaNodeVdb	*knodevdb = _knodevdb(NULL);
	Sdr		sdr;
	char		*knodevdbName = _knodevdbName();

	if (knodedbObject && knodevdb)
	{
		return 0;		/*	Already attached.	*/
	}

	if (bp_attach() < 0)
	{
		putErrmsg("DTKA can't attach to ION.", NULL);
		return -1;
	}

	sdr = getIonsdr();

	/*	Locate the DTKA database.				*/

	if (knodedbObject == 0)
	{
		CHKERR(sdr_begin_xn(sdr));	/*	Lock database.	*/
		knodedbObject = sdr_find(sdr, _knodedbName(), NULL);
		sdr_exit_xn(sdr);	/*	Unlock database.	*/
		if (knodedbObject == 0)
		{
			putErrmsg("Can't find DTKA database.", NULL);
			return -1;
		}

		oK(_knodedbObject(&knodedbObject));
	}

	oK(_knodeConstants());

	/*	Locate the DTKA volatile database.			*/

	if (knodevdb == NULL)
	{
		if (_knodevdb(&knodevdbName) == NULL)
		{
			putErrmsg("DTKA volatile database not found.", NULL);
			return -1;
		}
	}

	return 0;		/*	DTKA service is available.	*/
}

Object	getKnodeDbObject()
{
	return _knodedbObject(NULL);
}

DtkaNodeDB	*getKnodeConstants()
{
	return _knodeConstants();
}
