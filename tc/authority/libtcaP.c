/*
	libtcaP.c:	private functions used by the Trusted
			Collective authority system.

	Author: Scott Burleigh, JPL

	Copyright (c) 2020, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
	
									*/
#include "tcaP.h"

static void	_tcaDBName(char *buffer, int buflen, int blocksGroupNbr)
{
	isprintf(buffer, sizeof buflen, "tcadb.%d", blocksGroupNbr);
}

static void	_tcaVdbName(char *buffer, int buflen, int blocksGroupNbr)
{
	isprintf(buffer, sizeof buflen, "tcavdb.%d", blocksGroupNbr);
}

Object	getTcaDBObject(int blocksGroupNbr)
{
	Sdr	sdr = getIonsdr();
	char	dbname[32];
	Object	obj;

	_tcaDBName(dbname, sizeof dbname, blocksGroupNbr);
	CHKERR(sdr_begin_xn(sdr));
	obj = sdr_find(sdr, dbname, NULL);
	sdr_exit_xn(sdr);
	return obj;
}

TcaVdb	*getTcaVdb(int blocksGroupNbr)
{
	PsmPartition	wm = getIonwm();
	char		vdbname[32];
	PsmAddress	vdbAddress;
	PsmAddress	elt;

	_tcaVdbName(vdbname, sizeof vdbname, blocksGroupNbr);
	if (psm_locate(wm, vdbname, &vdbAddress, &elt) < 0)
	{
		putErrmsg("Failed searching for TCA vdb.", vdbname);
		return NULL;
	}

	if (elt)
	{
		return ((TcaVdb *) psp(wm, vdbAddress));
	}

	return NULL;
}

static int	createTcaVdb(int blocksGroupNbr)
{
	Sdr		sdr = getIonsdr();
	PsmPartition	wm = getIonwm();
	char		vdbname[32];
	PsmAddress	vdbAddress;
	TcaVdb		*vdb;

	if (getTcaVdb(blocksGroupNbr) != NULL)
	{
		return 0;		 /*	Already created.	*/
	}

	/*	TCA volatile database doesn't exist yet.		*/

	_tcaVdbName(vdbname, sizeof vdbname, blocksGroupNbr);
	sdr = getIonsdr();
	CHKERR(sdr_begin_xn(sdr));	/*	To lock memory.		*/
	vdbAddress = psm_zalloc(wm, sizeof(TcaVdb));
	if (vdbAddress == 0)
	{
		sdr_exit_xn(sdr);
		putErrmsg("No space for TCA volatile database.", vdbname);
		return -1;
	}

	vdb = (TcaVdb *) psp(wm, vdbAddress);
	memset((char *) vdb, 0, sizeof(TcaVdb));
	vdb->recvPid = ERROR;		/*	None yet.		*/
	vdb->compilePid = ERROR;	/*	None yet.		*/
	if (psm_catlg(wm, vdbname, vdbAddress) < 0)
	{
		sdr_exit_xn(sdr);
		putErrmsg("Can't initialize TCA volatile database.", vdbname);
		return -1;
	}

	sdr_exit_xn(sdr);		/*	Unlock wm.		*/
	return 0;
}

int	tcaInit(int blocksGroupNbr, int bulletinsGroupNbr, int recordsGroupNbr,
		int auths, int K, double R)
{
	Sdr		sdr;
	char		dbname[32];
	Object		dbobj;
	TcaDB		db;
	Object		authObj;
	TcaAuthority	auth;
	int		i;

	if (bp_attach() < 0)
	{
		putErrmsg("TCA can't attach to ION.", NULL);
		return -1;
	}

	if (blocksGroupNbr < 1)
	{
		putErrmsg("Invalid blocks group number.", itoa(blocksGroupNbr));
		return -1;
	}

	if (createTcaVdb(blocksGroupNbr) < 0)
	{
		putErrmsg("TCA can't initialize volatile database",
				itoa(blocksGroupNbr));
		return -1;
	}

	if (getTcaDBObject(blocksGroupNbr) != 0)
	{
		return 0;		/*	Already created.	*/
	}

	/*	Recover the TCA database, creating it if necessary.	*/

	_tcaDBName(dbname, sizeof dbname, blocksGroupNbr);
	if (blocksGroupNbr < 1 || bulletinsGroupNbr < 1 || recordsGroupNbr < 1
	|| K < 1 || !(R > 0.0 && R < 1.0))
	{
		writeMemoNote("[?] Invalid TCA initialization parameter(s)",
				dbname);
		return 0;
	}

	sdr = getIonsdr();
	CHKERR(sdr_begin_xn(sdr));
	dbobj = sdr_malloc(sdr, sizeof(TcaDB));
	if (dbobj == 0)
	{
		sdr_cancel_xn(sdr);
		putErrmsg("No space for TCA non-volatile database.", dbname);
		return -1;
	}

	/*	Initialize the non-volatile database.			*/

	memset((char *) &db, 0, sizeof(TcaDB));
	db.blocksGroupNbr = blocksGroupNbr;
	db.bulletinsGroupNbr = bulletinsGroupNbr;
	db.recordsGroupNbr = recordsGroupNbr;

	/*	Default values.						*/

	db.nextCompilationTime = 0;
	db.compilationInterval = 3600;		/*	Hourly.		*/
	db.consensusInterval = 60;		/*	60 sec.		*/

	/*	Management information.					*/

	db.fec_K = K;
	db.fec_R = R;
	db.fec_M = K + ((int) (R * K));
	db.fec_N = db.fec_M * 2;
	db.fec_Q = db.fec_N / auths;
	db.authorities = sdr_list_create(sdr);
	db.currentRecords = sdr_list_create(sdr);
	db.pendingRecords = sdr_list_create(sdr);
	sdr_write(sdr, dbobj, (char *) &db,
			sizeof(TcaDB));
	sdr_catlg(sdr, dbname, 0, dbobj);

	/*	Initialize list of authorities.				*/

	memset((char *) &auth, 0, sizeof(TcaAuthority));
	for (i = 0; i < auths; i++)
	{
		authObj = sdr_malloc(sdr, sizeof(TcaAuthority));
		if (authObj == 0)
		{
			sdr_cancel_xn(sdr);
			putErrmsg("No space for TCA authority info.", dbname);
			return -1;
		}

		sdr_write(sdr, authObj, (char *) &auth, sizeof(TcaAuthority));
		if (sdr_list_insert_last(sdr, db.authorities, authObj) == 0)
		{
			sdr_cancel_xn(sdr);
			putErrmsg("Can't create TCA authority info.", dbname);
			return -1;
		}
	}

	if (sdr_end_xn(sdr))
	{
		putErrmsg("Can't create TCA database.", dbname);
		return -1;
	}

	return 0;			/*	TCA service available.	*/
}

int	tcaStart(int blocksGroupNbr)
{
	Sdr	sdr = getIonsdr();
	TcaVdb	*vdb = getTcaVdb(blocksGroupNbr);
	char	cmdBuffer[32];

	if (vdb == NULL)
	{
		return 0;		/*	Nothing to start.	*/
	}

	CHKERR(sdr_begin_xn(sdr));	/*	Just to lock memory.	*/

	/*	Start the TCA receiver daemon if necessary.		*/

	if (vdb->recvPid == ERROR
	|| sm_TaskExists(vdb->recvPid) == 0)
	{
		isprintf(cmdBuffer, sizeof cmdBuffer, "tcarecv %d",
				blocksGroupNbr);
		vdb->recvPid = pseudoshell(cmdBuffer);
	}

	/*	Start the TCA compiler daemon if necessary.		*/

	if (vdb->compilePid == ERROR
	|| sm_TaskExists(vdb->compilePid) == 0)
	{
		isprintf(cmdBuffer, sizeof cmdBuffer, "tcacompile %d",
				blocksGroupNbr);
		vdb->compilePid = pseudoshell(cmdBuffer);
	}

	sdr_exit_xn(sdr);	/*	Unlock memory.			*/
	return 0;
}

int	tcaIsStarted(int blocksGroupNbr)
{
	TcaVdb	*vdb = getTcaVdb(blocksGroupNbr);

	return (vdb && vdb->recvPid != ERROR && vdb->compilePid != ERROR);
}

void	tcaStop(int blocksGroupNbr)	/*	Reverses tcaStart.	*/
{
	Sdr		sdr = getIonsdr();
	TcaVdb	*vdb = getTcaVdb(blocksGroupNbr);

	/*	Tell all TCA authority processes to stop.		*/

	CHKVOID(sdr_begin_xn(sdr));	/*	Just to lock memory.	*/

	/*	Stop tasks.						*/

	if (vdb->recvPid != ERROR)
	{
		sm_TaskKill(vdb->recvPid, SIGTERM);
	}

	if (vdb->compilePid != ERROR)
	{
		sm_TaskKill(vdb->compilePid, SIGTERM);
	}

	sdr_exit_xn(sdr);	/*	Unlock memory.			*/

	/*	Wait until all TCA processes have stopped.		*/

	if (vdb->recvPid != ERROR)
	{
		while (sm_TaskExists(vdb->recvPid))
		{
			microsnooze(100000);
		}
	}

	if (vdb->compilePid != ERROR)
	{
		while (sm_TaskExists(vdb->compilePid))
		{
			microsnooze(100000);
		}
	}

	/*	Now erase all the tasks.				*/

	CHKVOID(sdr_begin_xn(sdr));	/*	Just to lock memory.	*/
	vdb->recvPid = ERROR;
	vdb->compilePid = ERROR;
	sdr_exit_xn(sdr);		/*	Unlock memory.		*/
}

int	tcaAttach(int blocksGroupNbr)
{
	if (bp_attach() < 0)
	{
		putErrmsg("TCA can't attach to BP.", NULL);
		return -1;
	}

	if (blocksGroupNbr < 1)
	{
		putErrmsg("Invalid blocks group number.", itoa(blocksGroupNbr));
		return -1;
	}

	if (getTcaDBObject(blocksGroupNbr) != 0
	&& getTcaVdb(blocksGroupNbr) != NULL)
	{
		return 0;		/*	Already attached.	*/
	}

	return -1;
}
