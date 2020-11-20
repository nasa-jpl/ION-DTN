/*
	libtccP.c:	private functions used by the Trusted
			Collective client system.

	Nodeor: Scott Burleigh, JPL

	Copyright (c) 2020, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
	
									*/
#include "tccP.h"

static Object	_tccmdbobject(Object *newDbObj)
{
	static Object	obj = 0;

	if (newDbObj)
	{
		obj = *newDbObj;
	}

	return obj;
}

static char	*_tccmdbName()
{
	return "tccdb";
}

static Object	getTccMDB()
{
	Sdr	sdr = getIonsdr();
	Object	mdbobj;
	TccMDB	mdb;

	mdbobj = _tccmdbobject(NULL);
	if (mdbobj)		/*	Found previously.		*/
	{
		return mdbobj;
	}

	/*	Must recover multi-DB for TCC, creating if necessary.	*/

	CHKERR(sdr_begin_xn(sdr));
	mdbobj = sdr_find(sdr, _tccmdbName(), NULL);
	switch (mdbobj)
	{
	case -1:		/*	SDR error.			*/
		sdr_cancel_xn(sdr);
		putErrmsg("Can't search for TCC database in SDR.", NULL);
		return 0;

	case 0:			/*	Not found; must create new DB.	*/
		mdbobj = sdr_malloc(sdr, sizeof(TccMDB));
		if (mdbobj == 0)
		{
			sdr_cancel_xn(sdr);
			putErrmsg("No space for TCC database.", NULL);
			return 0;
		}

		/*	Initialize the non-volatile database.		*/

		mdb.dbs = sdr_list_create(sdr);
		if (mdb.dbs == 0)
		{
			sdr_cancel_xn(sdr);
			putErrmsg("Can't create list of TCC dbs.", NULL);
			return 0;
		}

		sdr_write(sdr, mdbobj, (char *) &mdb, sizeof(TccMDB));
		sdr_catlg(sdr, _tccmdbName(), 0, mdbobj);
		if (sdr_end_xn(sdr))
		{
			putErrmsg("Can't create TCC database.", NULL);
			return 0;
		}

		break;

	default:			/*	Found DB in the SDR.	*/
		sdr_exit_xn(sdr);
	}

	oK(_tccmdbobject(&mdbobj));	/*	Save database location.	*/
	return mdbobj;
}

static PsmAddress	_tccmvdbAddress(PsmAddress *newVdbAddr)
{
	static PsmAddress	addr = 0;

	if (newVdbAddr)
	{
		addr = *newVdbAddr;
	}

	return addr;
}

static char	*_tccvdbName()
{
	return "tccvdb";
}

static PsmAddress	getTccMVdb()
{
	PsmPartition	wm = getIonwm();
	PsmAddress	mvdbAddr;
	PsmAddress	elt;
	Sdr		sdr;
	TccMVdb		*mvdb;

	mvdbAddr = _tccmvdbAddress(NULL);
	if (mvdbAddr)		/*	Found previously.		*/
	{
		return mvdbAddr;
	}

	/*	Must recover multi-VDB for TCC, creating if necessary.	*/

	if (psm_locate(wm, _tccvdbName(), &mvdbAddr, &elt) < 0)
	{
		putErrmsg("Failed searching for TCC vdb.", NULL);
		return 0;
	}

	if (elt)		/*	Found MVDB in working memory.	*/
	{
		return mvdbAddr;
	}

	/*	Not found; must create new multi-VDB.			*/

	sdr = getIonsdr();
	CHKZERO(sdr_begin_xn(sdr));	/*	To lock memory.		*/
	mvdbAddr = psm_zalloc(wm, sizeof(TccMVdb));
	if (mvdbAddr == 0)
	{
		sdr_exit_xn(sdr);
		putErrmsg("No space for TCC volatile database.", NULL);
		return 0;
	}

	/*	Initialize the volatile database.			*/

	mvdb = (TccMVdb *) psp(wm, mvdbAddr);
	mvdb->vdbs = sm_list_create(wm);
	if (mvdb->vdbs == 0)
	{
		sdr_exit_xn(sdr);
		putErrmsg("Can't create list of TCC vdbs.", NULL);
		return 0;
	}

	if (psm_catlg(wm, _tccvdbName(), mvdbAddr) < 0)
	{
		sdr_exit_xn(sdr);
		putErrmsg("Can't create TCC volatile database.", NULL);
		return 0;
	}

	sdr_exit_xn(sdr);
	oK(_tccmvdbAddress(&mvdbAddr));	/*	Save database location.	*/
	return mvdbAddr;
}

static int	createTccVdb(int blocksGroupNbr)
{
	PsmPartition	wm = getIonwm();
	PsmAddress	mvdbAddr;
	TccMVdb		*mvdb;
	PsmAddress	vdbAddr;
	TccVdb		*vdb;
	Sdr		sdr;

	if (getTccVdb(blocksGroupNbr) != NULL)
	{
		return 0;		/*	Already created.	*/
	}

	/*	TCC volatile database doesn't exist yet.		*/

	mvdbAddr = getTccMVdb();
	if (mvdbAddr == 0)
	{
		putErrmsg("Can't get TCC volatile multi-database.", NULL);
		return -1;
	}

	mvdb = (TccMVdb *) psp(wm, mvdbAddr);
	sdr = getIonsdr();
	CHKERR(sdr_begin_xn(sdr));	/*	To lock memory.		*/
	vdbAddr = psm_zalloc(wm, sizeof(TccVdb));
	if (vdbAddr == 0)
	{
		sdr_exit_xn(sdr);
		putErrmsg("No space for TCC volatile database.",
				itoa(blocksGroupNbr));
		return -1;
	}

	vdb = (TccVdb *) psp(wm, vdbAddr);
	memset((char *) vdb, 0, sizeof(TccVdb));
	vdb->blocksGroupNbr = blocksGroupNbr;
	vdb->tccPid = ERROR;		/*	None yet.		*/
	vdb->contentSemaphore = sm_SemCreate(SM_NO_KEY, SM_SEM_FIFO);
	if (sm_list_insert_last(wm, mvdb->vdbs, vdbAddr) == 0)
	{
		sdr_exit_xn(sdr);
		putErrmsg("Can't create volatile database.",
				itoa(blocksGroupNbr));
		return -1;
	}

	sm_SemTake(vdb->contentSemaphore);	/*	Lock.		*/
	sdr_exit_xn(sdr);		/*	Unlock wm.		*/
	return 0;
}

int	tccInit(int blocksGroupNbr, int auths, int K, double R)
{
	Sdr		sdr;
	Object		mdbobj;
	TccMDB		mdb;
	Object		dbobj;
	TccDB		db;
	int		i;
	Object		authObj;
	TccAuthority	auth;
	int		seqLength;
	int		sharenum;

	if (bp_attach() < 0)
	{
		putErrmsg("TCC can't attach to ION.", NULL);
		return -1;
	}

	if (blocksGroupNbr < 1)
	{
		putErrmsg("Invalid blocks group number.", itoa(blocksGroupNbr));
		return -1;
	}

	if (createTccVdb(blocksGroupNbr) < 0)
	{
		putErrmsg("TCC can't initialize volatile database.",
				itoa(blocksGroupNbr));
		return -1;
	}

	if (getTccDBObj(blocksGroupNbr) != 0)
	{
		return 0;		/*	Already created.	*/
	}

	/*	Recover the TCC database, creating it if necessary.	*/

	if (blocksGroupNbr < 1 || auths < 1 || K < 1
	|| !(R > 0.0 && R < 1.0))
	{
		writeMemoNote("[?] Invalid TCC initialization parameter(s)",
				itoa(blocksGroupNbr));
		return 0;
	}

	mdbobj = getTccMDB();
	if (mdbobj == 0)
	{
		putErrmsg("Can't get TCC non-volatile multi-database.",
				itoa(blocksGroupNbr));
		return -1;
	}

	sdr = getIonsdr();
	sdr_read(sdr, (char *) &mdb, mdbobj, sizeof(TccMDB));
	CHKERR(sdr_begin_xn(sdr));
	dbobj = sdr_malloc(sdr, sizeof(TccDB));
	if (dbobj == 0)
	{
		sdr_cancel_xn(sdr);
		putErrmsg("No space for TCC non-volatile database.",
				itoa(blocksGroupNbr));
		return -1;
	}

	/*	Initialize the non-volatile database.			*/

	memset((char *) &db, 0, sizeof(TccDB));
	db.blocksGroupNbr = blocksGroupNbr;
	db.fec_K = K;
	db.fec_R = R;
	db.fec_M = K + ((int) (R * K));
	db.fec_N = db.fec_M * 2;
	db.fec_Q = db.fec_N / auths;
	db.authorities = sdr_list_create(sdr);
	db.bulletins = sdr_list_create(sdr);
	db.contents = sdr_list_create(sdr);
	sdr_write(sdr, dbobj, (char *) &db, sizeof(TccDB));
	if (sdr_list_insert_last(sdr, mdb.dbs, dbobj) == 0)
	{
		sdr_cancel_xn(sdr);
		putErrmsg("Can't create non-volatile database.",
				itoa(blocksGroupNbr));
		return -1;
	}

	/*	Initialize responsibilities of authorities.		*/

	seqLength = db.fec_Q / 2;
	for (i = 0; i < auths; i++)
	{
		memset((char *) &auth, 0, sizeof(TccAuthority));
		sharenum = seqLength * i;
		auth.firstPrimaryShare = sharenum;
		auth.lastPrimaryShare = (sharenum + seqLength) - 1;
		sharenum += seqLength;
		if (sharenum >= db.fec_M)
		{
			sharenum = 0;
		}

		auth.firstBackupShare = sharenum;
		auth.lastBackupShare = (sharenum + seqLength) - 1;
		authObj = sdr_malloc(sdr, sizeof(TccAuthority));
		if (authObj == 0)
		{
			sdr_cancel_xn(sdr);
			putErrmsg("No space for TCC authority info.",
					itoa(blocksGroupNbr));
			return -1;
		}

		sdr_write(sdr, authObj, (char *) &auth, sizeof(TccAuthority));
		if (sdr_list_insert_last(sdr, db.authorities, authObj) == 0)
		{
			sdr_cancel_xn(sdr);
			putErrmsg("Can't create TCC authority info.",
					itoa(blocksGroupNbr));
			return -1;
		}
	}

	if (sdr_end_xn(sdr))
	{
		putErrmsg("Can't create TCC database.", itoa(blocksGroupNbr));
		return -1;
	}

	return 0;			/*	TCC service available.	*/
}

int	tccStart(int blocksGroupNbr)
{
	Sdr	sdr = getIonsdr();
	TccVdb	*vdb = getTccVdb(blocksGroupNbr);
	char	cmdBuffer[32];

	if (vdb == NULL)
	{
		return 0;		/*	Nothing to start.	*/
	}

	CHKERR(sdr_begin_xn(sdr));	/*	Just to lock memory.	*/

	/*	Start the TC client daemon if necessary.		*/

	if (vdb->tccPid == ERROR
	|| sm_TaskExists(vdb->tccPid) == 0)
	{
		isprintf(cmdBuffer, sizeof cmdBuffer, "tcc %d", blocksGroupNbr);
		vdb->tccPid = pseudoshell(cmdBuffer);
	}

	sdr_exit_xn(sdr);	/*	Unlock memory.			*/
	return 0;
}

int	tccIsStarted(int blocksGroupNbr)
{
	TccVdb	*vdb = getTccVdb(blocksGroupNbr);

	return (vdb && vdb->tccPid != ERROR);
}

void	tccStop(int blocksGroupNbr)	/*	Reverses tccStart.	*/
{
	Sdr	sdr = getIonsdr();
	TccVdb	*vdb = getTccVdb(blocksGroupNbr);

	if (vdb == NULL)		/*	Not yet started.	*/
	{
		return;
	}

	/*	Tell TCC client functionality to stop.			*/

	CHKVOID(sdr_begin_xn(sdr));	/*	Just to lock memory.	*/

	/*	Stop TCC daemon task.					*/

	if (vdb->tccPid != ERROR)
	{
		sm_TaskKill(vdb->tccPid, SIGTERM);
	}

	/*	Stop user task.						*/

	if (vdb->contentSemaphore != SM_SEM_NONE)
	{
		sm_SemEnd(vdb->contentSemaphore);
	}

	sdr_exit_xn(sdr);	/*	Unlock memory.			*/

	/*	Wait until all TCC processes have stopped.		*/

	if (vdb->tccPid != ERROR)
	{
		while (sm_TaskExists(vdb->tccPid))
		{
			microsnooze(100000);
		}
	}

	/*	Now erase the daemon task and reset the semaphore.	*/

	CHKVOID(sdr_begin_xn(sdr));	/*	Just to lock memory.	*/
	vdb->tccPid = ERROR;
	if (vdb->contentSemaphore == SM_SEM_NONE)
	{
		vdb->contentSemaphore = sm_SemCreate(SM_NO_KEY, SM_SEM_FIFO);
	}
	else
	{
		sm_SemUnend(vdb->contentSemaphore);
		sm_SemGive(vdb->contentSemaphore);
	}

	sm_SemTake(vdb->contentSemaphore);	/*	Lock.		*/
	sdr_exit_xn(sdr);		/*	Unlock memory.		*/
}

int	tccAttach(int blocksGroupNbr)
{
	if (bp_attach() < 0)
	{
		putErrmsg("TCC can't attach to ION.", itoa(blocksGroupNbr));
		return -1;
	}

	if (blocksGroupNbr < 1)
	{
		putErrmsg("Invalid blocks group number.", itoa(blocksGroupNbr));
		return -1;
	}

	if (getTccDBObj(blocksGroupNbr) != 0
	&& getTccVdb(blocksGroupNbr) != NULL)
	{
		return 0;		/*	Already attached.	*/
	}

	return -1;
}

Object	getTccDBObj(int blocksGroupNbr)
{
	Sdr	sdr = getIonsdr();
	Object	mdbobj;
	TccMDB	mdb;
	Object	elt;
	Object	dbobj;
	TccDB	db;

	mdbobj = getTccMDB();
	if (mdbobj == 0)
	{
		return 0;
	}

	CHKZERO(sdr_begin_xn(sdr));
	sdr_read(sdr, (char *) &mdb, mdbobj, sizeof(TccMDB));
	for (elt = sdr_list_first(sdr, mdb.dbs); elt;
			elt = sdr_list_next(sdr, elt))
	{
		dbobj = sdr_list_data(sdr, elt);
		sdr_read(sdr, (char *) &db, dbobj, sizeof(TccDB));
		if (db.blocksGroupNbr == blocksGroupNbr)
		{
			break;
		}
	}

	sdr_exit_xn(sdr);
	if (elt)
	{
		return dbobj;
	}

	return 0;
}

TccVdb	*getTccVdb(int blocksGroupNbr)
{
	PsmPartition	wm = getIonwm();
	PsmAddress	mvdbAddr;
	TccMVdb		*mvdb;
	PsmAddress	elt;
	PsmAddress	vdbAddr;
	TccVdb		*vdb;

	mvdbAddr = getTccMVdb();
	if (mvdbAddr == 0)
	{
		return NULL;
	}

	mvdb = (TccMVdb *) psp(wm, mvdbAddr);
	for (elt = sm_list_first(wm, mvdb->vdbs); elt;
			elt = sm_list_next(wm, elt))
	{
		vdbAddr = sm_list_data(wm, elt);
		vdb = (TccVdb *) psp(wm, vdbAddr);
		if (vdb->blocksGroupNbr == blocksGroupNbr)
		{
			return vdb;
		}
	}

	return NULL;
}

