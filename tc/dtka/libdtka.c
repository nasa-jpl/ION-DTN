/*
	libdtka.c:	common functions offered to DTKA client
			software.

	Author: Scott Burleigh, JPL

	Copyright (c) 2020, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
	
									*/
#include "dtka.h"

static Object	_dtkadbObject(Object *newDbObj)
{
	static Object	obj = 0;

	if (newDbObj)
	{
		obj = *newDbObj;
	}

	return obj;
}

static DtkaDB	*_dtkaConstants()
{
	static DtkaDB	buf;
	static DtkaDB	*db = NULL;
	Sdr		sdr;
	Object		dbObject;

	if (db == NULL)
	{
		/*	Load constants into a conveniently accessed
		 *	structure.  Note that this CANNOT be treated
		 *	as a current database image in later
		 *	processing.					*/

		sdr = getIonsdr();
		CHKNULL(sdr);
		dbObject = _dtkadbObject(NULL);
		if (dbObject)
		{
			if (sdr_heap_is_halted(sdr))
			{
				sdr_read(sdr, (char *) &buf, dbObject,
						sizeof(DtkaDB));
			}
			else
			{
				CHKNULL(sdr_begin_xn(sdr));
				sdr_read(sdr, (char *) &buf, dbObject,
						sizeof(DtkaDB));
				sdr_exit_xn(sdr);
			}

			db = &buf;
		}
	}

	return db;
}

Object	getDtkaDbObject()
{
	return _dtkadbObject(NULL);
}

DtkaDB	*getDtkaConstants()
{
	return _dtkaConstants();
}

static char	*_dtkadbName()
{
	return "dtkadb";
}

int	dtkaInit()
{
	Sdr	sdr;
	Object	dtkadbObject;
	DtkaDB	dtkadbBuf;

	if (bp_attach() < 0)
	{
		putErrmsg("DTKA can't attach to ION.", NULL);
		return -1;
	}

	sdr = getIonsdr();

	/*	Recover the dtka database, creating it if necessary.	*/

	CHKERR(sdr_begin_xn(sdr));
	dtkadbObject = sdr_find(sdr, _dtkadbName(), NULL);
	switch (dtkadbObject)
	{
	case -1:		/*	SDR error.			*/
		sdr_cancel_xn(sdr);
		putErrmsg("Can't search for dtka database in SDR.", NULL);
		return -1;

	case 0:			/*	Not found; must create new DB.	*/
		dtkadbObject = sdr_malloc(sdr, sizeof(DtkaDB));
		if (dtkadbObject == 0)
		{
			sdr_cancel_xn(sdr);
			putErrmsg("No space for dtka database.", NULL);
			return -1;
		}

		/*	Initialize the dtka database.			*/

		memset((char *) &dtkadbBuf, 0, sizeof(DtkaDB));

		/*	Default values.					*/

		dtkadbBuf.nextKeyGenTime = 0;
		dtkadbBuf.keyGenInterval = 604800;	/*	Weekly	*/
		dtkadbBuf.effectiveLeadTime = 345600;	/*	4 days	*/
		sdr_write(sdr, dtkadbObject, (char *) &dtkadbBuf,
				sizeof(DtkaDB));
		sdr_catlg(sdr, _dtkadbName(), 0, dtkadbObject);
		if (sdr_end_xn(sdr))
		{
			putErrmsg("Can't create dtka database.", NULL);
			return -1;
		}

		break;

	default:			/*	Found DB in the SDR.	*/
		sdr_exit_xn(sdr);
	}

	oK(_dtkadbObject(&dtkadbObject));/*	Save database location.	*/
	oK(_dtkaConstants());
	return 0;		/*	TCC service is available.	*/
}

int	dtkaAttach()
{
	Object	dtkadbObject = _dtkadbObject(NULL);
	Sdr	sdr;

	if (bp_attach() < 0)
	{
		putErrmsg("DTKA can't attach to ION.", NULL);
		return -1;
	}

	if (dtkadbObject)
	{
		return 0;		/*	Already attached.	*/
	}

	sdr = getIonsdr();
	CHKERR(sdr_begin_xn(sdr));	/*	Lock database.		*/
	dtkadbObject = sdr_find(sdr, _dtkadbName(), NULL);
	sdr_exit_xn(sdr);		/*	Unlock database.	*/
	if (dtkadbObject == 0)
	{
		if (dtkaInit() < 0)
		{
			putErrmsg("Can't find DTKA database.", NULL);
			return -1;
		}

		CHKERR(sdr_begin_xn(sdr));	/*	Lock database.	*/
		dtkadbObject = sdr_find(sdr, _dtkadbName(), NULL);
		sdr_exit_xn(sdr);		/*	Unlock database.*/
	}

	oK(_dtkadbObject(&dtkadbObject));
	oK(_dtkaConstants());
	return 0;		/*	DTKA service is available.	*/
}
