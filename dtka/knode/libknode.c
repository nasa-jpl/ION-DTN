/*
	libknode.c:	common functions offered to DTKA client
			software.

	Nodeor: Scott Burleigh, JPL

	Copyright (c) 2020, California Institute of Technology.
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

static Object	getKnodeDbObject()
{
	return _knodedbObject(NULL);
}

static DtkaNodeDB	*getKnodeConstants()
{
	return _knodeConstants();
}

static char	*_knodedbName()
{
	return "knodedb";
}

static int	knodeInit()
{
	Sdr		sdr;
	Object		knodedbObject;
	DtkaNodeDB	knodedbBuf;
	char		*knodevdbName = _knodevdbName();
	int		i;
	DtkaAuthority	*auth;
	int		seqLength = DTKA_FEC_Q / 2;
	int		sharenum;

	sdr = getIonsdr();

	/*	Recover the knode database, creating it if necessary.	*/

	CHKERR(sdr_begin_xn(sdr));
	knodedbObject = sdr_find(sdr, _knodedbName(), NULL);
	switch (knodedbObject)
	{
	case -1:		/*	SDR error.			*/
		sdr_cancel_xn(sdr);
		putErrmsg("Can't search for knode database in SDR.", NULL);
		return -1;

	case 0:			/*	Not found; must create new DB.	*/
		knodedbObject = sdr_malloc(sdr, sizeof(DtkaNodeDB));
		if (knodedbObject == 0)
		{
			sdr_cancel_xn(sdr);
			putErrmsg("No space for knode database.", NULL);
			return -1;
		}

		/*	Initialize the knode database.			*/

		memset((char *) &knodedbBuf, 0, sizeof(DtkaNodeDB));

		/*	Default values.					*/

		knodedbBuf.nextKeyGenTime = getCtime() + 4;
		knodedbBuf.keyGenInterval = 604800;	/*	Weekly	*/
		knodedbBuf.effectiveLeadTime = 345600;	/*	4 days	*/
		sdr_write(sdr, knodedbObject, (char *) &knodedbBuf,
				sizeof(DtkaNodeDB));
		sdr_catlg(sdr, _knodedbName(), 0, knodedbObject);
		if (sdr_end_xn(sdr))
		{
			putErrmsg("Can't create knode database.", NULL);
			return -1;
		}

		break;

	default:			/*	Found DB in the SDR.	*/
		sdr_exit_xn(sdr);
	}

	oK(_knodedbObject(&knodedbObject));/*	Save database location.	*/
	oK(_knodeConstants());
	return 0;		/*	TCC service is available.	*/
}

static int	knodeAttach()
{
	Object		knodedbObject = _knodedbObject(NULL);
	Sdr		sdr;
	char		*knodevdbName = _knodevdbName();

	if (bp_attach() < 0)
	{
		putErrmsg("DTKA can't attach to ION.", NULL);
		return -1;
	}

	if (knodedbObject)
	{
		return 0;		/*	Already attached.	*/
	}

	sdr = getIonsdr();
	CHKERR(sdr_begin_xn(sdr));	/*	Lock database.		*/
	knodedbObject = sdr_find(sdr, _knodedbName(), NULL);
	sdr_exit_xn(sdr);		/*	Unlock database.	*/
	if (knodedbObject == 0)
	{
		if (knodeInit() < 0)
		{
			putErrmsg("Can't find DTKA database.", NULL);
			return -1;
		}

		knodedbObject = sdr_find(sdr, _knodedbName(), NULL);
	}

	oK(_knodedbObject(&knodedbObject));
	oK(_knodeConstants());
	return 0;		/*	DTKA service is available.	*/
}
