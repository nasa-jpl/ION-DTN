/*
	libdtpcP.c:	Functions enabling the implementation of
			DTPC nodes.

	Authors: Giorgos Papastergiou, SPICE
		 Ioannis Alexiadis, SPICE

	Copyright (c) 2011, Space Internetworking Center,
	Democritus University of Thrace. ALL RIGHTS RESERVED.
										*/

#include "dtpcP.h"

/*      *       *       Helpful utility functions       *       *       */

static Object   _dtpcdbObject(Object *newDbObj)
{
	static Object	obj = 0;

	if (newDbObj)
	{
		obj = *newDbObj;
	}

	return obj;
}

static DtpcDB   *_dtpcConstants()
{
	static DtpcDB	buf;
	static DtpcDB	*db = NULL;

	if (db == NULL)
	{
		/*	Load constants into a conveniently accessed
		 *	structure.  Note that this CANNOT be treated
		 *	as a current database image in later
		 *	processing.					*/

		sdr_read(getIonsdr(), (char *) &buf, _dtpcdbObject(NULL),
				sizeof(DtpcDB));
		db = &buf;
	}

	return db;
}

/*      *       *       DTPC service control functions  *       *       */

static char	*_dtpcvdbName()
{
	return "dtpcvdb";
}

int	raiseProfile(Sdr sdr, Object sdrElt, DtpcVdb *vdb)
{
	PsmAddress	elt;
	PsmAddress	addr;
	Profile		*vprofile;
	Object		profileObj;
			OBJ_POINTER(Profile, profile);
	PsmPartition	wm = getIonwm();
	
	profileObj = sdr_list_data(sdr, sdrElt);
	GET_OBJ_POINTER(sdr, Profile, profile, profileObj);

	/*	Copy profile to volatile database			*/

	addr = psm_malloc(wm, sizeof(Profile));
	if (addr == 0)
	{
		return -1;
	}

	elt = sm_list_insert_last(wm, vdb->profiles, addr);
	if (elt == 0)
	{
		psm_free(wm, addr);
		return -1;
	}

	vprofile = (Profile *) psp(wm, addr);
	memset((char *) vprofile, 0, sizeof(Profile));
	memcpy((char *) vprofile, (char *) profile, sizeof(Profile));
	return 0;

}

int	raiseVSap(Sdr sdr, Object elt, DtpcVdb *vdb, unsigned int topicID)
{
	PsmPartition	wm = getIonwm();
	VSap		*vsap;
	PsmAddress	addr;
	PsmAddress	vsapElt;

	addr = psm_malloc(wm, sizeof(VSap));
	if (addr == 0)
	{
		return -1;
	}
	
	vsapElt = sm_list_insert_last(wm, vdb->vsaps, addr);
	if (vsapElt == 0)
	{
		psm_free(wm, addr);
		return -1;
	}

	vsap = (VSap *) psp(wm, addr);
	memset((char *) vsap, 0, sizeof(VSap));
	vsap->topicID = topicID;
	vsap->semaphore = sm_SemCreate(SM_NO_KEY, SM_SEM_FIFO);
	vsap->dlvQueue = elt;
	vsap->appPid = -1;
	sm_SemTake(vsap->semaphore);	/*	Lock.			*/
	return 0;
}

static DtpcVdb  *_dtpcvdb(char **name)
{
	static DtpcVdb	*vdb = NULL;
	PsmPartition	wm;
	PsmAddress	vdbAddress;
	PsmAddress	elt;
	Sdr		sdr;
	Object		sdrElt;
	unsigned int	topicID;

	sdr = getIonsdr();
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
			vdb = (DtpcVdb *) psp(wm, vdbAddress);
			return vdb;
		}

		/*	DTPC volatile database doesn't exist yet.	*/

		CHKNULL(sdr_begin_xn(sdr));	/*	Lock memory.	*/	
		vdbAddress = psm_zalloc(wm, sizeof(DtpcVdb));
		if (vdbAddress == 0)
		{
			sdr_exit_xn(sdr);
			putErrmsg("No space for volatile database.", NULL);
			return NULL;
		}

		vdb = (DtpcVdb *) psp(wm, vdbAddress);
		memset((char *) vdb, 0, sizeof(DtpcVdb));
		vdb->clockPid = -1;
		vdb->dtpcdPid = -1;
		vdb->watching = 0;
		vdb->aduSemaphore = sm_SemCreate(SM_NO_KEY, SM_SEM_FIFO);
		if ((vdb->vsaps = sm_list_create(wm)) == 0)
		{
			sdr_exit_xn(sdr);
			putErrmsg("Can't initialize volatile database.", NULL);
			return NULL;
			
		}

		if ((vdb->profiles = sm_list_create(wm)) == 0)
		{
			sdr_exit_xn(sdr);
			putErrmsg("Can't initialize volatile database.", NULL);
			return NULL;
		}

		if (psm_catlg(wm, *name, vdbAddress) < 0)
		{
			sdr_exit_xn(sdr);
			putErrmsg("Can't initialize volatile database.", NULL);
			return NULL;
		}

		sm_SemTake(vdb->aduSemaphore); /*	Lock.		*/

		/*	Raise vsaps	*/

		for (sdrElt = sdr_list_first(sdr, (_dtpcConstants())->queues);
			sdrElt; sdrElt = sdr_list_next(sdr, sdrElt))
		{
			topicID = (unsigned int) sdr_list_user_data(sdr,
					sdrElt);
			if (topicID == 0)
			{
				sdr_exit_xn(sdr);
				putErrmsg("Can't read sdrlist user data.",
						NULL);
				return NULL;
			}

			if (raiseVSap(sdr, sdrElt, vdb, topicID) < 0)
			{
				sdr_exit_xn(sdr);
				putErrmsg("Can't raise vsaps.", NULL);
				return NULL;
			}
		}
		
		/* 	Raise profiles	*/

		for (sdrElt = sdr_list_first(sdr, (_dtpcConstants())->profiles);
			sdrElt; sdrElt = sdr_list_next(sdr, sdrElt))
		{	
			if (raiseProfile(sdr, sdrElt, vdb) < 0)
			{
				sdr_exit_xn(sdr);
				putErrmsg("Can't raise profiles.", NULL);
				return NULL;
			}
		}

		sdr_exit_xn(sdr);	/*	Unlock memory.	*/
	}

	return vdb;
}

static char	*_dtpcdbName()
{
	return "dtpcdb";
}

int	dtpcInit()
{
	Sdr		sdr;
	Object		dtpcdbObject;
	DtpcDB		dtpcdbBuf;
	char		*dtpcvdbName = _dtpcvdbName();

	if (ionAttach() < 0)
	{
		putErrmsg("DTPC can't attach to ION.", NULL);
		return -1;
	}
	
	sdr = getIonsdr();

	/*	Recover the DTPC database, creating it if necessary.	*/

	CHKERR(sdr_begin_xn(sdr));
	dtpcdbObject = sdr_find(sdr, _dtpcdbName(), NULL);
	switch (dtpcdbObject)
	{
	case -1:		/*	SDR error.			*/
		sdr_cancel_xn(sdr);
		putErrmsg("Can't search for DTPC database in SDR.", NULL);
		return -1;

	case 0:			/*	Not found; must create new DB.	*/
		dtpcdbObject = sdr_malloc(sdr, sizeof(DtpcDB));
		if (dtpcdbObject == 0)
		{
			sdr_cancel_xn(sdr);
			putErrmsg("No space for database.", NULL);
			return -1;
		}

		/*	Initialize the non-volatile database.		*/
		memset((char *) &dtpcdbBuf, 0, sizeof(DtpcDB));

		/*	Management information.				*/

		dtpcdbBuf.outAggregators = sdr_list_create(sdr);
		dtpcdbBuf.inAggregators = sdr_list_create(sdr);
		dtpcdbBuf.events = sdr_list_create(sdr);
		dtpcdbBuf.profiles = sdr_list_create(sdr);
		dtpcdbBuf.queues = sdr_list_create(sdr);
		dtpcdbBuf.outboundAdus = sdr_list_create(sdr);
		
		sdr_write(sdr, dtpcdbObject, (char *) &dtpcdbBuf,
				sizeof(DtpcDB));
		sdr_catlg(sdr, _dtpcdbName(), 0, dtpcdbObject);
		if (sdr_end_xn(sdr))
		{
			putErrmsg("Can't create DTPC database.", NULL);
			return -1;
		}
		break;

	default:			/*	Found DB in the SDR.	*/
		sdr_exit_xn(sdr);
	}

	oK(_dtpcdbObject(&dtpcdbObject));	/*	Save location.	*/
	oK(_dtpcConstants());

	/*	Locate volatile database, initializing as necessary.	*/

	if (_dtpcvdb(&dtpcvdbName) == NULL)
	{
		putErrmsg("DTPC can't initialize vdb.", NULL);
		return -1;
	}
	return 0;	/*	DTPC service is available.	*/
}

Object	getDtpcDbObject()
{
	return _dtpcdbObject(NULL);
}		

DtpcDB	*getDtpcConstants()
{
	return _dtpcConstants();
}

DtpcVdb	*getDtpcVdb()
{
	return	_dtpcvdb(NULL);
}

int	_dtpcStart()
{
	Sdr	sdr = getIonsdr();
	DtpcVdb	*dtpcvdb = _dtpcvdb(NULL);

	CHKERR(sdr_begin_xn(sdr));	/*	Just to lock memory.	*/

	/*	Start the DTPC clock if necessary.			*/
	
	if (dtpcvdb->clockPid < 1 || sm_TaskExists(dtpcvdb->clockPid) == 0)
	{
		dtpcvdb->clockPid = pseudoshell("dtpcclock");
	}

	/*	Start the DTPC daemon service if necessary		*/

	if (dtpcvdb->dtpcdPid < 1 || sm_TaskExists(dtpcvdb->dtpcdPid) == 0)
	{
		dtpcvdb->dtpcdPid = pseudoshell("dtpcd");
	}

	sdr_exit_xn(sdr);
	return 0;
}

void	_dtpcStop()		/*	Reverses dtpcStart.		*/
{
	Sdr		sdr = getIonsdr();
	DtpcVdb		*dtpcvdb = _dtpcvdb(NULL);
	PsmPartition	wm = getIonwm();
	PsmAddress	elt;
	VSap		*vsap;

	/*	Tell all DTPC processes to stop.			*/

	CHKVOID(sdr_begin_xn(sdr));

	/*	Stop user application processes				*/

	for (elt = sm_list_first(wm, dtpcvdb->vsaps); elt; elt =
			sm_list_next(wm, elt))
	{
		vsap = (VSap *) psp(wm, sm_list_data(wm, elt));
		if (vsap->semaphore != SM_SEM_NONE)
		{
			sm_SemEnd(vsap->semaphore);
		}
	}

	/*	Stop dtpcd task						*/

	if (dtpcvdb->aduSemaphore != SM_SEM_NONE)
	{
		sm_SemEnd(dtpcvdb->aduSemaphore);
	}

	/*	Stop clock task						*/

	if (dtpcvdb->clockPid != ERROR)
	{
		sm_TaskKill(dtpcvdb->clockPid, SIGTERM);
	}

	sdr_exit_xn(sdr);

	/*	Wait for all tasks to finish				*/

	while (dtpcvdb->dtpcdPid != ERROR && sm_TaskExists(dtpcvdb->dtpcdPid))
	{
		microsnooze(100000);
	}

	while (dtpcvdb->clockPid != ERROR && sm_TaskExists(dtpcvdb->clockPid))
	{
		microsnooze(100000);
	}

	for (elt = sm_list_first(wm, dtpcvdb->vsaps); elt; elt =
			sm_list_next(wm, elt))
	{
		vsap = (VSap *) psp(wm, sm_list_data(wm, elt));
		if (vsap->semaphore != SM_SEM_NONE)
		{
			while (vsap->appPid != ERROR
			&& sm_TaskExists(vsap->appPid))
			{
				microsnooze(100000);
			}
		}
	}

	/*	Now erase all the tasks and reset the semaphores.	*/

	CHKVOID(sdr_begin_xn(sdr));
	dtpcvdb->dtpcdPid = ERROR;
	dtpcvdb->clockPid = ERROR;
	if (dtpcvdb->aduSemaphore == SM_SEM_NONE)
	{
		dtpcvdb->aduSemaphore = sm_SemCreate(SM_NO_KEY, SM_SEM_FIFO);
	}
	else
	{
		sm_SemUnend(dtpcvdb->aduSemaphore);
		sm_SemGive(dtpcvdb->aduSemaphore);
	}

	sm_SemTake(dtpcvdb->aduSemaphore);		/*	Lock.	*/
	for (elt = sm_list_first(wm, dtpcvdb->vsaps); elt;
			elt = sm_list_next(wm, elt))
	{
		vsap = (VSap *) psp(wm, sm_list_data(wm, elt));
		if (vsap->semaphore == SM_SEM_NONE)
		{
			vsap->semaphore = sm_SemCreate(SM_NO_KEY, SM_SEM_FIFO);
                }
		else
		{
			sm_SemUnend(vsap->semaphore);
			sm_SemGive(vsap->semaphore);
		}

		sm_SemTake(vsap->semaphore);		/*	Lock	*/
		vsap->appPid = ERROR;
        }

	sdr_exit_xn(sdr);
}

int	dtpcAttach()
{
	Object		dtpcdbObject = _dtpcdbObject(NULL);
	DtpcVdb		*dtpcvdb = _dtpcvdb(NULL);
	Sdr		sdr;
	char		*dtpcvdbName = _dtpcvdbName();

	if (dtpcdbObject && dtpcvdb)
	{
		return 0;		/*	Already attached.	*/
	}

	if (ionAttach() < 0)
	{
		putErrmsg("DTPC can't attach to ION.", NULL);
		return -1;
	}

	sdr = getIonsdr();
	if (dtpcdbObject == 0)
	{
		CHKERR(sdr_begin_xn(sdr));
		dtpcdbObject = sdr_find(sdr, _dtpcdbName(), NULL);
		sdr_exit_xn(sdr);
		if (dtpcdbObject == 0)
		{
			putErrmsg("Can't find DTPC database.", NULL);
			return -1;
		}	

		oK(_dtpcdbObject(&dtpcdbObject));
	}

	oK(_dtpcConstants());

	/*	Locate the DTPC volatile database.		*/

	if (dtpcvdb == NULL)
	{
		if(_dtpcvdb(&dtpcvdbName) == NULL)
		{
			putErrmsg("DTPC volatile database not found.", NULL);
			return -1;
		}
	}
	
	return 0;		/*	DTPC service is available.	*/

}

int	initOutAdu(Profile *profile, Object outAggrAddr, Object outAggrElt,
		Object *outAduObj, Object *outAduElt)
{
	Sdr		sdr = getIonsdr();	
	OutAggregator	outAggr;
	OutAdu		outAduBuf;

        sdr_stage(sdr, (char *) &outAggr, outAggrAddr, sizeof(OutAggregator));
	memset((char *) &outAduBuf, 0, sizeof(OutAdu));
	outAduBuf.ageOfAdu = -1;
	outAduBuf.rtxCount = -1;
	if (profile->maxRtx == 0)	/*	No transport service.	*/
	{
		loadScalar(&outAduBuf.seqNum, 0);
	}
	else
	{
		copyScalar(&outAduBuf.seqNum, &outAggr.aduCounter);
		increaseScalar(&outAggr.aduCounter, 1);
	}

	outAduBuf.outAggrElt = outAggrElt;
	outAduBuf.topics = sdr_list_create(sdr);
	*outAduElt = 0;
	*outAduObj = sdr_malloc(sdr, sizeof(OutAdu));
	if (*outAduObj == 0)
	{
		putErrmsg("No space for OutAdu.", NULL);
		return -1;
	}

	sdr_write(sdr, *outAduObj, (char *) &outAduBuf, sizeof(OutAdu)); 
	*outAduElt = sdr_list_insert_last(sdr, outAggr.outAdus, *outAduObj);
	outAggr.inProgressAduElt = *outAduElt;
	sdr_write(sdr, outAggrAddr, (char *) &outAggr, sizeof(OutAggregator));
	if ((_dtpcvdb(NULL))->watching & WATCH_newItem)
	{
		putchar('<');
		fflush(stdout);
	}

	return 0;
}

static Object	insertToTopic(unsigned int topicID, Object outAduObj,
			Object outAduElt, Object recordObj,
			unsigned int lifespan, PayloadRecord *newRecord,
			DtpcSAP sap)
{
	OutAdu		outAdu;
	Topic		topicBuf;
	Object		topicAddr;
	Object		elt;
	Sdr		sdr = getIonsdr();
	time_t		currentTime;

	sdr_stage(sdr, (char *) &outAdu, outAduObj, sizeof(OutAdu));
	for (elt = sdr_list_first(sdr, outAdu.topics); elt;
		elt = sdr_list_next(sdr, elt))
        {
		topicAddr = (Object) sdr_list_data(sdr, elt);
		sdr_stage(sdr, (char *) &topicBuf, topicAddr, sizeof(Topic));
		if (topicBuf.topicID == topicID)
		{
			break;
		}
	}
	
	if (elt == 0)		/*	No Topic found - Create new	*/
	{
		memset((char *) &topicBuf, 0, sizeof(Topic));
		topicBuf.topicID = topicID;
		topicBuf.payloadRecords = sdr_list_create(sdr);
		topicBuf.outAduElt = outAduElt;
		topicAddr = sdr_malloc(sdr, sizeof(Topic));
		if (topicAddr == 0)
		{
			putErrmsg("No space for Topic.", NULL);
			return 0;
		}

		sdr_write(sdr, topicAddr, (char *) &topicBuf, sizeof(Topic)); 
		elt = sdr_list_insert_last(sdr, outAdu.topics, topicAddr);
	}

	if (outAdu.ageOfAdu < 0)
	{
		currentTime = getCtime();
		outAdu.ageOfAdu = 0;
		outAdu.expirationTime = currentTime + lifespan;
	}

	sdr_write(sdr, outAduObj, (char *) &outAdu, sizeof(OutAdu));
	if (sdr_list_insert_last(sdr, topicBuf.payloadRecords, recordObj) == 0)
	{
		putErrmsg("No space for list element for payload record.",
				NULL);
		return 0;
	}

	if (sap->elisionFn != NULL
	&& (sap->elisionFn)(topicBuf.payloadRecords) < 0)
	{
		putErrmsg("Elision function failed.", NULL);
		return 0;
	}

	if ((_dtpcvdb(NULL))->watching & WATCH_r)
	{
		putchar('r');
		fflush(stdout);
	}

	return elt;
}

static int	estimateLength(OutAdu *outAdu)
{
	Sdr	sdr = getIonsdr();
	Object	elt1;
	Object	elt2;
		OBJ_POINTER(Topic, topic);
		OBJ_POINTER(PayloadRecord, record);
	uvast	recordLength;
	int	totalLength = 0;
	
	for (elt1 = sdr_list_first(sdr, outAdu->topics); elt1;
			elt1 = sdr_list_next(sdr, elt1))
	{
		GET_OBJ_POINTER(sdr, Topic, topic, sdr_list_data(sdr, elt1));
		for (elt2 = sdr_list_first(sdr, topic->payloadRecords); elt2;
				elt2 = sdr_list_next(sdr, elt2))
		{
			GET_OBJ_POINTER(sdr, PayloadRecord, record, 
					sdr_list_data(sdr, elt2));
			oK(decodeSdnv(&recordLength, record->length.text));
			totalLength += (int) recordLength;
		}
	}

	return totalLength;
}

static Profile	*findProfileByNumber(unsigned int profNum)
{
	DtpcVdb		*vdb = getDtpcVdb();
	PsmPartition	wm = getIonwm();
	PsmAddress	psmElt;
	Profile		*profile;

	for (psmElt = sm_list_first(wm, vdb->profiles); psmElt; 
			psmElt = sm_list_next(wm, psmElt))
	{
		profile = (Profile *) psp(wm, sm_list_data(wm, psmElt));
		if (profile->profileID == profNum)
		{
			return profile;
		}
	}

	return NULL;
}

int	insertRecord (DtpcSAP sap, char *dstEid, unsigned int profileID,
		unsigned int topicID, Object item, int length)
{
	DtpcVdb		*vdb = getDtpcVdb();
	Sdr		sdr = getIonsdr();
	DtpcDB		*dtpcConstants = _dtpcConstants();
	PayloadRecord	record;
	Object		recordObj;
	OutAdu		outAdu;
	Object		outAduObj;
	Object		outAduElt;
	OutAggregator	outAggr;
	Object		outAggrAddr;
	Profile		*vprofile;	
	Object		sdrElt;
	char		eidBuf[SDRSTRING_BUFSZ];
	Sdnv		lengthSdnv;
	int		totalLength;

	CHKERR(dstEid && item);
	if (*dstEid == 0)
	{
		writeMemo("[?] Zero-length destination EID.");
		return 0;
	}

	if (strlen(dstEid) == 8 && strcmp(dstEid, "dtn:none") == 0)
	{
		writeMemo("[?] Destination endpoint can't be null endpoint.");
		return 0;
	}

	CHKERR(sdr_begin_xn(sdr));	/*	Just to lock memory.	*/

	/*	Load profile parameters		*/

	vprofile = findProfileByNumber(profileID);
	if (vprofile == NULL)
	{
		writeMemo("[?] Can't insert DTPC record; no such profile.");
		return 0;
	}

	/*      Create payload record.					*/

	memset((char *) &record, 0, sizeof(PayloadRecord));
	encodeSdnv(&lengthSdnv, length);
	record.length = lengthSdnv;
	record.payload = item;
	
	/*	Search for an existing outbound payload aggregator.	*/ 

	for (sdrElt = sdr_list_first(sdr, dtpcConstants->outAggregators);
			sdrElt; sdrElt = sdr_list_next(sdr, sdrElt))
	{
		outAggrAddr = (Object) sdr_list_data(sdr, sdrElt);
		sdr_stage(sdr, (char *) &outAggr, outAggrAddr, 
			sizeof(OutAggregator));
		if (sdr_string_read(sdr, eidBuf, outAggr.dstEid) < 0)
		{
			putErrmsg("Failed reading destination EID.", NULL);
			sdr_exit_xn(sdr);
			return -1;
		}

		if (strcmp(dstEid, eidBuf) == 0
		&& outAggr.profileID == profileID)
		{
			break;
		}
	}

	/*	No outbound payload aggregator was found - Create one.	*/

	if (sdrElt == 0)
	{
		memset((char *) &outAggr, 0, sizeof(OutAggregator));
		outAggr.dstEid = sdr_string_create(sdr, dstEid);
		outAggr.profileID = profileID;
		if (vprofile->maxRtx == 0)	/*	No transport.	*/
		{
			loadScalar(&outAggr.aduCounter, 0);
		}
		else
		{
			loadScalar(&outAggr.aduCounter, 1);
		}

		outAggr.outAdus = sdr_list_create(sdr);
		outAggr.inProgressAduElt = 0;
		outAggr.queuedAdus = sdr_list_create(sdr);
		outAggrAddr = sdr_malloc(sdr, sizeof(OutAggregator));
		if (outAggrAddr == 0)
		{
			putErrmsg("No space for the outbound aggregator", NULL);
			sdr_cancel_xn(sdr);
			return -1;
		}

		sdr_write(sdr, outAggrAddr, (char *) &outAggr,
				sizeof(OutAggregator));
		sdrElt = sdr_list_insert_last(sdr, 
				dtpcConstants->outAggregators, outAggrAddr);
		if (vdb->watching & WATCH_o)
		{
			putchar('o');
			fflush(stdout);
		}
	}

	/*	Search for an already opened outAdu 			*/

	if (outAggr.inProgressAduElt == 0)
	{
		if (initOutAdu(vprofile, outAggrAddr, sdrElt, &outAduObj,
				&outAduElt) < 0)
		{
			putErrmsg("Can't create new outAdu", NULL);
			sdr_cancel_xn(sdr);
			return -1;
		}
	}
	else	/*		Load existing one			*/
	{
		outAduObj = sdr_list_data(sdr, outAggr.inProgressAduElt);
		outAduElt = outAggr.inProgressAduElt;
	}

	/*	Append the new application data item to the outAdu.	*/

	recordObj = sdr_malloc(sdr, sizeof(PayloadRecord));
	if (recordObj == 0)
	{
		putErrmsg("No space for payload record.", NULL);
		sdr_cancel_xn(sdr);
		return -1;
	}

	sdr_write(sdr, recordObj, (char *) &record, sizeof(PayloadRecord));
	if (insertToTopic(topicID, outAduObj, outAduElt, recordObj,
			vprofile->lifespan, &record, sap) == 0)
	{
		sdr_cancel_xn(sdr);
		return -1;
	}

	/*	Estimate the resulting total length of the aggregated
	 *	outAdu.							*/

	sdr_stage(sdr, (char *) &outAdu, outAduObj, sizeof(OutAdu));
	totalLength = estimateLength(&outAdu);

	/*	If the estimated length equals or exceeds the
	 *	aggregation size limit (or the aggregation time
	 *	limit is zero, indicating that no aggregation is
	 *	requested for this profile) then finish aggregation
	 *	and create an empty outbound ADU.			*/

	if (totalLength >= vprofile->aggrSizeLimit
	|| vprofile->aggrTimeLimit == 0)
	{
		if (createAdu(vprofile, outAduObj, outAduElt) < 0)
		{
			putErrmsg("Can't send outbound adu.", NULL);
			sdr_cancel_xn(sdr);
			return -1;
		}

		if (initOutAdu(vprofile, outAggrAddr, sdrElt, &outAduObj,
				&outAduElt) < 0)
		{
			putErrmsg("Can't create new outAdu", NULL);
			sdr_cancel_xn(sdr);
			return -1;
		}
	}

	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't insert record", NULL);
		return -1;
	}

	return 1;
}

static Object	insertDtpcTimelineEvent(DtpcEvent *newEvent)
{
	Sdr		sdr = getIonsdr();
	DtpcDB		*dtpcConstants = _dtpcConstants();
	Address		addr;
	Object		elt;
			OBJ_POINTER(DtpcEvent, event);

	CHKZERO(ionLocked());
	addr = sdr_malloc(sdr,sizeof(DtpcEvent));
	if (addr == 0)
	{
		putErrmsg("No space for Dtpc timeline event.", NULL);
		return 0;
	}

	sdr_write(sdr, addr,(char *) newEvent, sizeof(DtpcEvent));
	for (elt = sdr_list_last(sdr, dtpcConstants->events); elt;
			elt = sdr_list_prev(sdr, elt))
	{
		GET_OBJ_POINTER(sdr, DtpcEvent, event,
				sdr_list_data(sdr, elt));
		if (event->scheduledTime <= newEvent->scheduledTime)
		{
			return sdr_list_insert_after(sdr, elt, addr);
		}
	}

	return sdr_list_insert_first(sdr, dtpcConstants->events, addr);
}

int	createAdu(Profile *profile, Object outAduObj, Object outAduElt)
{
	OutAdu			outAdu;
	Sdr			sdr = getIonsdr();
	DtpcVdb			*vdb = getDtpcVdb();
	Sdnv			profNum;
	Sdnv			seqNum;
	Sdnv			topicID;
	Sdnv			recordsCounter;
	char			type;
	int			extentLength;
	uvast			payloadDataLength;
	unsigned char		*buffer;
	unsigned char		*cursor;
	Object			topicElt;
	Object			payloadRecElt;
	Object 			addr;
	Object			zco;
	Object			extent;
				OBJ_POINTER(Topic, topic);
				OBJ_POINTER(PayloadRecord, payloadRec);

	/*	The code to create and give the aggregated
	 *	zco to bp starts here.					*/

	sdr_stage(sdr, (char *) &outAdu, outAduObj, sizeof(OutAdu));

	/*		Construct Header				*/

	type = 0x00;	/*	Top 2 bits are version number 00.	*/
	encodeSdnv(&profNum, profile->profileID);
	scalarToSdnv(&seqNum, &outAdu.seqNum);
	extentLength = 1 + profNum.length + seqNum.length;
	addr = sdr_malloc(sdr, extentLength);
	if (addr == 0)
	{
		putErrmsg("No space in SDR for extent.", NULL);
		return -1;
	}

	buffer = MTAKE(extentLength);
	if (buffer == NULL)
	{
		putErrmsg("Can't construct Dtpc block header.", NULL);
		return -1;
	}

	cursor = buffer;
	*cursor = type;
	cursor++;
	memcpy(cursor, profNum.text, profNum.length);
	cursor += profNum.length;
	memcpy(cursor, seqNum.text, seqNum.length);
	sdr_write(sdr, addr, (char *) buffer, extentLength);
	MRELEASE(buffer);

	/*		Create ZCO and append header			*/

	zco = ionCreateZco(ZcoSdrSource, addr, 0, extentLength,
			profile->classOfService, profile->ancillaryData.ordinal,
			ZcoOutbound, NULL);
	if (zco == 0 || zco == (Object) ERROR)
	{
		putErrmsg("Can't create aggregated ZCO.", NULL);
		return -1;
	}

	/*	Parse all topics and payload records of the
	 *	outbound ADU and append them to the ZCO.		*/

	for (topicElt = sdr_list_first(sdr, outAdu.topics); topicElt;
				topicElt = sdr_list_next(sdr, topicElt))
	{
		GET_OBJ_POINTER(sdr, Topic, topic,
				sdr_list_data(sdr, topicElt));
		encodeSdnv(&topicID, topic->topicID);
		encodeSdnv(&recordsCounter, sdr_list_length(sdr,
				topic->payloadRecords));

		/*	Append Extent containing topic number
		 *	and payload records number.			*/

		extentLength = topicID.length + recordsCounter.length;
		addr = sdr_malloc(sdr, extentLength);
		if (addr == 0)
		{
			putErrmsg("No space in SDR for extent.", NULL);
			return -1;
		}

		buffer = MTAKE(extentLength);
		if (buffer == NULL)
		{
			putErrmsg("Can't allocate memory for extent.", NULL);
			return -1;
		}

		cursor = buffer;
		memcpy(cursor, topicID.text, topicID.length);
		cursor += topicID.length;
		memcpy(cursor, recordsCounter.text, recordsCounter.length);
		sdr_write(sdr, addr, (char *) buffer, extentLength);
		MRELEASE(buffer);
		extent = ionAppendZcoExtent(zco, ZcoSdrSource, addr, 0,
				extentLength, profile->classOfService,
				profile->ancillaryData.ordinal, NULL);
		if (extent == 0 || extent == (Object) ERROR)
		{
			putErrmsg("Can't create ZCO extent.", NULL);
			return -1;
		}

		for (payloadRecElt = sdr_list_first(sdr, topic->payloadRecords);
				payloadRecElt; payloadRecElt =
				sdr_list_next(sdr, payloadRecElt))
		{
			GET_OBJ_POINTER(sdr, PayloadRecord, payloadRec,
					sdr_list_data(sdr, payloadRecElt));
			oK(decodeSdnv(&payloadDataLength,
					payloadRec->length.text));

			/*	Allocate SDR for length sdnv.		*/

			addr = sdr_malloc(sdr, payloadRec->length.length);
			if (addr == 0)
			{
				putErrmsg("No space in SDR for extent.", NULL);
				return -1;
			}

			sdr_write(sdr, addr, (char *) payloadRec->length.text,
						payloadRec->length.length);

			/* Append 2 extents for each payload, one
			 * containing the length sdnv and one
			 * containing the payload itself.		*/

			extent = ionAppendZcoExtent(zco, ZcoSdrSource, addr,
					0, payloadRec->length.length,
					profile->classOfService,
					profile->ancillaryData.ordinal, NULL);
			if (extent == 0 || extent == (Object) ERROR)
			{
				putErrmsg("Can't create ZCO extent.", NULL);
				return -1;
			}

			extent = ionAppendZcoExtent(zco, ZcoSdrSource,
					payloadRec->payload, 0,
					payloadDataLength,
					profile->classOfService,
					profile->ancillaryData.ordinal, NULL);
			if (extent == 0 || extent == (Object) ERROR)
			{
				putErrmsg("Can't create ZCO extent.", NULL);
				return -1;
			}
		}
	}

	outAdu.aggregatedZCO = zco;
	CHKERR(sdr_list_insert_last(sdr, (_dtpcConstants())->outboundAdus,
			outAduObj));
	sdr_write(sdr, outAduObj, (char *) &outAdu, sizeof(OutAdu));
	if (vdb->watching & WATCH_complete)
	{
		putchar('>');
		fflush(stdout);
	}	

	sm_SemGive(vdb->aduSemaphore);
	return 0;
}

int	sendAdu(BpSAP sap)
{
	Sdr		sdr = getIonsdr();
	DtpcDB		*dtpcConstants = _dtpcConstants();
	DtpcVdb		*vdb = getDtpcVdb();
	Object		sdrElt;
	Object		outAduObj;
	OutAdu		outAdu;
			OBJ_POINTER(OutAggregator, outAggr);
	Object		zco;
	Profile		*profile;
	DtpcEvent	event;
	int		secondsConsumed;
	int		nominalRtt;
	int		lifetime;
	time_t		currentTime;
	Object		bundleElt;
	Object		outAduElt;
	char		reportToEid[SDRSTRING_BUFSZ];
	char		dstEid[SDRSTRING_BUFSZ];

	CHKERR(sdr_begin_xn(sdr));
	sdrElt = sdr_list_first(sdr, dtpcConstants->outboundAdus);
	while (sdrElt == 0)
	{
		sdr_exit_xn(sdr);

		/*	Wait until an ADU is created.	*/

		if (sm_SemTake(vdb->aduSemaphore) < 0)
		{
			putErrmsg("dtpcd can't take ADU semaphore.", NULL);
			return -1;
		}	

		if (sm_SemEnded(vdb->aduSemaphore))
		{
			writeMemo("[i] dtpcd stop has been signaled.");
			return -1;
		}

		CHKERR(sdr_begin_xn(sdr));
		sdrElt = sdr_list_first(sdr, dtpcConstants->outboundAdus);
	}

	outAduObj = sdr_list_data(sdr, sdrElt);
	sdr_stage(sdr, (char *) &outAdu, outAduObj, sizeof(OutAdu));
	GET_OBJ_POINTER(sdr, OutAggregator, outAggr,
			sdr_list_data(sdr, outAdu.outAggrElt));

	/*	Create a clone of the aggregated ZCO and give it
	 *	to bp.							*/

	zco = zco_clone(sdr, outAdu.aggregatedZCO, 0,
			zco_length(sdr, outAdu.aggregatedZCO));
	if (zco == 0)
	{
		sdr_cancel_xn(sdr);
		putErrmsg("Can't clone aggregated ZCO.", NULL);
		return -1;
	}

	/* 	Find profile	*/

	profile = findProfileByNumber(outAggr->profileID);
	if (profile == NULL)
	{
		sdr_cancel_xn(sdr);
		putErrmsg("Profile has been removed.", NULL);
		return -1;
	}

	if (sdr_string_read(sdr, reportToEid, profile->reportToEid) < 0
	|| sdr_string_read(sdr, dstEid, outAggr->dstEid) < 0)
	{
		sdr_cancel_xn(sdr);
		putErrmsg("Failed reading endpoint ID.", NULL);
		return -1;
	}

	nominalRtt = profile->lifespan / (profile->maxRtx + 1);
	secondsConsumed = outAdu.rtxCount * nominalRtt;
	lifetime = profile->lifespan - secondsConsumed;
	if (lifetime < 1)
	{
		lifetime = 1;
	}

	if (bp_send(sap, dstEid, reportToEid, lifetime,
			profile->classOfService, profile->custodySwitch,
			profile->srrFlags, 0, &(profile->ancillaryData),
			zco, &outAdu.bundleObj) <= 0)
	{
		sdr_cancel_xn(sdr);
		putErrmsg("DTPC can't send adu.", NULL);
		return -1;
	}

	if (profile->maxRtx == 0)	/*	No transport service.	*/
	{
		deleteAdu(sdr, sdrElt);
		return sdr_end_xn(sdr);
	}

	/*	Transport service is requested for this ADU.  So
	 *	first, track bundle to prevent unnecessary
	 *	retransmissions.					*/

	bundleElt = sdr_list_insert_last(sdr, outAggr->queuedAdus,
			outAdu.bundleObj);
	if (bp_track(outAdu.bundleObj, bundleElt) < 0)
	{
		sdr_cancel_xn(sdr);
		putErrmsg("Can't track bundle.", NULL);
		return -1;
	}

	/*	Bundle has been detained long enough for us to track
	 *	it, so we can now release it for normal processing.	*/

	oK(bp_release(outAdu.bundleObj));

	/*	Get outAduElt from aggregator.				*/

	for (outAduElt = sdr_list_first(sdr, outAggr->outAdus); outAduElt;
			outAduElt = sdr_list_next(sdr, outAduElt))
	{
		if (outAduObj == sdr_list_data(sdr, outAduElt))
		{
			break;	/*	Found element.			*/
		}
	}

	outAdu.rtxCount++;

	/*	Create retransmission event only if there is time
	 *	for one.						*/

	currentTime = getCtime();
	if (outAdu.rtxCount < (int) profile->maxRtx
	&& (currentTime + nominalRtt) < outAdu.expirationTime)
	{
		event.type = ResendAdu;
		event.scheduledTime = currentTime + nominalRtt;
		event.aduElt = outAduElt;

		/*	Insert rtx event into list.			*/

		outAdu.rtxEventElt = insertDtpcTimelineEvent(&event);
		if (outAdu.rtxEventElt == 0)
		{
			sdr_cancel_xn(sdr);
			putErrmsg("Can't schedule Dtpc retransmission event.",
					NULL);
			return -1;
		}
	}

	/*	Create deletion event if none created yet.		*/

	if (outAdu.delEventElt == 0)
	{
		event.type = DeleteAdu;
		event.scheduledTime = outAdu.expirationTime;
		event.aduElt = outAduElt;

		/*	Insert deletion event into list.		*/

		outAdu.delEventElt = insertDtpcTimelineEvent(&event);
		if (outAdu.delEventElt == 0)
		{
			sdr_cancel_xn(sdr);
			putErrmsg("Can't schedule Dtpc deletion event.", NULL);
			return -1;
		}
	}

	sdr_write(sdr, outAduObj, (char *) &outAdu, sizeof(OutAdu));
	sdr_list_delete(sdr, sdrElt, NULL, NULL);
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("DTPC can't send adu.", NULL);
		return -1;
	}

	if (vdb->watching & WATCH_send)
	{
		putchar('-');
		fflush(stdout);
	}

	return 0;
}

static void	deleteEltObjContent(Sdr sdr, Object elt, void *arg)
{
	sdr_free(sdr, sdr_list_data(sdr, elt));
}

void	deleteAdu(Sdr sdr, Object aduElt)
{
	Object		aduObj;
	Object		elt;
			OBJ_POINTER(OutAdu, adu);
			OBJ_POINTER(Topic, topic);

	aduObj = sdr_list_data(sdr, aduElt);
	GET_OBJ_POINTER(sdr, OutAdu, adu, aduObj);

	/*	Destroy the payloadRecords list from each topic
	 *	and then the topics list.				*/

	for (elt = sdr_list_first(sdr, adu->topics); elt;
				elt = sdr_list_next(sdr, elt))
	{
		GET_OBJ_POINTER(sdr, Topic, topic, sdr_list_data(sdr, elt));
		sdr_list_destroy(sdr, topic->payloadRecords,
					deleteEltObjContent, NULL);
	}

	sdr_list_destroy(sdr, adu->topics, deleteEltObjContent, NULL);

	/*	Next, remove the events and their elements.		*/

	if (adu->rtxEventElt)
	{
		sdr_list_delete(sdr, adu->rtxEventElt, deleteEltObjContent,
				NULL);
	}

	if (adu->delEventElt)
	{
		sdr_list_delete(sdr, adu->delEventElt, deleteEltObjContent,
				NULL);
	}

	/*	Finally, destroy the adu and its element.		*/

	zco_destroy(sdr, adu->aggregatedZCO);
	sdr_free(sdr, aduObj);
	sdr_list_delete(sdr, aduElt, NULL, NULL);
}

int	resendAdu(Sdr sdr, Object aduElt, time_t currentTime)
{
	DtpcDB		*dtpcConstants = _dtpcConstants();
	DtpcVdb		*vdb = getDtpcVdb();
	char		aduIsQueued = 0;	/*	Boolean		*/
	Object          aduObj;
	Object		elt;
	OutAdu		adu;
	DtpcEvent	newEvent;
			OBJ_POINTER(OutAggregator, outAggr);
			OBJ_POINTER(Profile, profile);
	int		nominalRtt;

	/*	Check to see if the adu is still in the applicable
	 *	Aggregator's queue of ADUs awaiting encapsulation
	 *	in bundles.  						*/

	aduObj = sdr_list_data(sdr, aduElt);
	sdr_stage(sdr, (char *) &adu, aduObj, sizeof(OutAdu));
	GET_OBJ_POINTER(sdr, OutAggregator, outAggr,
			sdr_list_data(sdr, adu.outAggrElt));
	for (elt = sdr_list_first(sdr, outAggr->queuedAdus); elt;
			elt = sdr_list_next(sdr, elt))
	{
		if (adu.bundleObj == sdr_list_data(sdr, elt))
		{
			aduIsQueued = 1; /*	Adu still in queue.	*/
			break;
		}
	}

	/*	Delete the retransmission event.			*/

	sdr_list_delete(sdr, adu.rtxEventElt, deleteEltObjContent, NULL);
	adu.rtxEventElt = 0;
	if (aduIsQueued == 1)	/*	Bundle containing adu exists.	*/
	{
		/*	The ADU is already queued for encapsulation
		 *	in an outbound bundle, so no need to enqueue
		 *	it again.  Just post another retransmission
		 *	event (if transmission is still plausible;
		 *	otherwise just let deletion event destroy
		 *	the ADU).					*/

		for (elt = sdr_list_first(sdr, dtpcConstants->profiles); elt;
				elt = sdr_list_next(sdr, elt))
		{
			GET_OBJ_POINTER(sdr, Profile, profile, 
					sdr_list_data(sdr, elt));
			if (outAggr->profileID == profile->profileID)
			{
				break;	/*	Found matching profile.	*/
			}
		}

		CHKERR(elt);	/*	Profile must be in database.	*/
		adu.rtxCount++;	/* Increase retransmission counter.	*/

		/* If the maxRtx limit defined in the profile being used
		 * has not been reached AND the expected time for the next
		 * retransmission is before the delEvent, create a new
		 * rtxEvent.						*/

		nominalRtt = profile->lifespan / (profile->maxRtx + 1);
		if (adu.rtxCount < (int) profile->maxRtx
		&& (currentTime + nominalRtt) < adu.expirationTime)
		{
			newEvent.type = ResendAdu;
			newEvent.scheduledTime = currentTime + nominalRtt;
			newEvent.aduElt = aduElt;
			adu.rtxEventElt = insertDtpcTimelineEvent(&newEvent);
			if (adu.rtxEventElt == 0)
			{
				putErrmsg("Can't schedule Dtpc retransmission \
event.", NULL);
				return -1;
			}
		}

		sdr_write(sdr, aduObj, (char *) &adu, sizeof(OutAdu));
	}
	else
	{
		/*	Bundle in which this adu was most recently
		 *	transmitted no longer exists, so that bundle
		 *	has been transmitted.  So now we need to
		 *	requeue this adu for transmission in a new
		 *	bundle.						*/

		CHKERR(sdr_list_insert_last(sdr, dtpcConstants->outboundAdus,
				aduObj));
		sdr_write(sdr, aduObj, (char *) &adu, sizeof(OutAdu));
		if (vdb->watching & WATCH_n)
		{
			putchar('n');
			fflush(stdout);
		}

		sm_SemGive(vdb->aduSemaphore);
	}

	return 0;
}

unsigned int     dtpcGetProfile(unsigned int maxRtx, unsigned int aggrSizeLimit,
			unsigned int aggrTimeLimit, unsigned int lifespan,
			BpAncillaryData *ancillaryData, unsigned char srrFlags,
			BpCustodySwitch custodySwitch, char *reportToEid,
			int classOfService)
{
	Sdr		sdr = getIonsdr();
	DtpcVdb		*vdb = getDtpcVdb();
	PsmPartition	wm = getIonwm();
	Profile		*profile = NULL;
	Object		elt;
	char		repToEid[SDRSTRING_BUFSZ];

	CHKZERO(sdr_begin_xn(sdr));	/*	Just to lock memory	*/
	for (elt = sm_list_first(wm, vdb->profiles); elt;
			elt = sm_list_next(wm, elt))
	{
		profile = (Profile *) psp(wm, sm_list_data(wm, elt));
		if (sdr_string_read(sdr, repToEid, profile->reportToEid) < 0)
		{
			putErrmsg("Failed reading reportToEid.", NULL);
			sdr_exit_xn(sdr);
			return 0;
		}

		if (profile->maxRtx == maxRtx
		&& profile->aggrSizeLimit == aggrSizeLimit
		&& profile->aggrTimeLimit == aggrTimeLimit
		&& profile->lifespan == lifespan
		&& profile->classOfService == classOfService
		&& profile->custodySwitch == custodySwitch
		&& profile->srrFlags == srrFlags
		&& strcmp(repToEid, reportToEid) == 0
		&& profile->ancillaryData.dataLabel == ancillaryData->dataLabel
		&& profile->ancillaryData.flags == ancillaryData->flags
		&& profile->ancillaryData.ordinal == ancillaryData->ordinal)
		{
			break;	/*	Found matching profile.		*/
		}

	}

	sdr_exit_xn(sdr);	/*	Unlock memory.			*/
	if (elt == 0)
	{
		return 0;
	}

	return profile->profileID;
}

static void     setFlag(int *srrFlags, char *arg)
{
        if (strcmp(arg, "rcv") == 0)
        {
                (*srrFlags) |= BP_RECEIVED_RPT;
        }

        if (strcmp(arg, "ct") == 0)
        {
                (*srrFlags) |= BP_CUSTODY_RPT;
        }

        if (strcmp(arg, "fwd") == 0)
        {
                (*srrFlags) |= BP_FORWARDED_RPT;
        }

        if (strcmp(arg, "dlv") == 0)
        {
                (*srrFlags) |= BP_DELIVERED_RPT;
        }

        if (strcmp(arg, "del") == 0)
        {
                (*srrFlags) |= BP_DELETED_RPT;
        }
}

static void     setFlags(int *srrFlags, char *flagString)
{
        char    *cursor = flagString;
        char    *comma;

        while (1)
        {
                comma = strchr(cursor, ',');
                if (comma)
                {
                        *comma = '\0';
                        setFlag(srrFlags, cursor);
                        *comma = ',';
                        cursor = comma + 1;
                        continue;
                }

                setFlag(srrFlags, cursor);
                return;
        }
}

int	addProfile(unsigned int profileID, unsigned int maxRtx,
		unsigned int aggrSizeLimit, unsigned int aggrTimeLimit,
		unsigned int lifespan, char *svcClass, char *reportToEid,
		char *flags)
{
	Sdr		sdr = getIonsdr();
	DtpcVdb		*vdb = getDtpcVdb();
	BpAncillaryData	ancillaryData = {0, 0, 0};
	BpCustodySwitch	custodySwitch = NoCustodyRequested;
	Profile		*vprofile;
	Profile		profile;
	Object		addr;
	Object		elt2;	
	int		priority = 0;
	int		srrFlags = 0;

	CHKERR(sdr_begin_xn(sdr));	/*	Just to lock memory.	*/
	if (svcClass == NULL || *svcClass == '\0' ||
		reportToEid == NULL || *reportToEid == '\0')
	{
		sdr_exit_xn(sdr);
		writeMemoNote("[?] Missing profile parameters.", 
				utoa(profileID));
		return 0;
	}

	if (profileID == 0 || lifespan == 0)
	{
		sdr_exit_xn(sdr);
		writeMemoNote("[?] Missing profile parameter(s).",
				utoa(profileID));
		return 0;
	}

	/*	Check if it is a known  profile.			*/

	vprofile = findProfileByNumber(profileID);
	if (vprofile != NULL)
	{
		sdr_exit_xn(sdr);
		writeMemo("[?] Duplicate profile.");
		return 0;
	}

	if (!bp_parse_quality_of_service(svcClass, &ancillaryData, 
			&custodySwitch, &priority))
        {
                sdr_exit_xn(sdr);
		putErrmsg("Can't parse class of service.", NULL);
                return 0;
        }

	if (flags)
	{	
        	setFlags(&srrFlags, flags);
	}

	if (dtpcGetProfile(maxRtx, aggrSizeLimit, aggrTimeLimit, lifespan,
			&ancillaryData, srrFlags, custodySwitch, reportToEid,
			priority) > 0)
	{
		sdr_exit_xn(sdr);
		writeMemo("[?] A profile with the same parameters exists.");
		return 0;	
	} 
	
	profile.profileID = profileID;
	profile.maxRtx = maxRtx;
	profile.aggrSizeLimit = aggrSizeLimit;
	profile.aggrTimeLimit = aggrTimeLimit;
	profile.lifespan = lifespan;
	profile.ancillaryData = ancillaryData;
	profile.custodySwitch = custodySwitch;
	profile.classOfService = priority;
	profile.srrFlags = srrFlags;	
	profile.reportToEid = sdr_string_create(sdr, reportToEid);
	addr = sdr_malloc(sdr, sizeof(Profile));
	if (addr == 0)
	{
		putErrmsg("No space for profile", NULL);
		sdr_cancel_xn(sdr);
		return -1;
	}

	sdr_write(sdr, addr, (char *) &profile, sizeof(Profile));
	elt2 = sdr_list_insert_last(sdr, (_dtpcConstants())->profiles, addr);
	if (raiseProfile(sdr, elt2, vdb) < 0)
	{
		putErrmsg("Can't load profile into the volatile db.", NULL);
		sdr_cancel_xn(sdr);
		return -1;
	}

	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't add profile.", NULL);
		return -1;
	}
	
	return 0;
}

int	removeProfile(unsigned int profileID)
{
	Sdr		sdr = getIonsdr();
	PsmPartition	wm = getIonwm();
	DtpcVdb		*vdb = getDtpcVdb();
	PsmAddress	vprofileAddr;
	PsmAddress	elt;
	Object		sdrElt;
	Object		profileAddr;
	Profile		profile;
	Profile		*vprofile;
	
	CHKERR(sdr_begin_xn(sdr));	/*	Just to lock memory.	*/

	for (elt = sm_list_first(wm, vdb->profiles); elt;
                        elt = sm_list_next(wm, elt))
        {
		vprofileAddr = sm_list_data(wm, elt);
		vprofile = (Profile *) psp(wm, vprofileAddr);
                if (vprofile->profileID == profileID)
                {
                        break;
                }
        }

	if (elt == 0)
	{	
		sdr_exit_xn(sdr);
		writeMemoNote("[?] Profile unkown.", itoa(profileID));
		return 0;	
	}
	
	/*	Remove profile from the volatile database.		*/

	if (vprofile->reportToEid)
	{
		sdr_free(sdr, vprofile->reportToEid);
	}	

	oK(sm_list_delete(wm, elt, NULL, NULL));
	psm_free(wm, vprofileAddr);

	/*		Remove profile from sdr.			*/

	for (sdrElt = sdr_list_first(sdr, (getDtpcConstants())->profiles);
		sdrElt; sdrElt = sdr_list_next(sdr, sdrElt))
	{
		profileAddr =  sdr_list_data(sdr, sdrElt);
		sdr_stage(sdr, (char *) &profile, profileAddr,
			sizeof(Profile));
		if (profile.profileID == profileID)
		{
			break;
		}
	}	
	
	if (sdrElt)
	{
		sdr_free(sdr, profileAddr);
		sdr_list_delete(sdr, sdrElt, NULL, NULL);
	}

	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't remove profile from sdr.", NULL);
		return -1;
	}

	return 0;
}

/*	DTPC reception aggregation is based on lists of received
 *	ADUs, one list per aggregator.  Each list is a sequence of
 *	ADUs in ascending sequence number order.  Two types of ADUs
 *	may appear in the list: true ADUs, bearing content, and
 *	"placeholder" ADUs that represent ADUs that were lost in
 *	transit and may ultimately be retransmitted, filling in the
 *	gaps in the sequence of true ADUs.  The last ADU in any
 *	list must be a true ADU.  The first ADU in a list may be
 *	either a true ADU or a placeholder.  Each placeholder MUST
 *	be immediately followed by a true ADU.  Each placeholder
 *	that is not at the start of the list MUST be immediately
 *	preceded by a true ADU whose sequence number is exactly 1
 *	less that that of the placeholder.  Each true ADU that is
 *	not at the start of the list MUST be immediately preceded
 *	by either (a) another true ADU, whose sequence number is
 *	exactly 1 less that that of this ADU, or (b) a placeholder
 *	ADU whose sequence number must be at least 1 less than
 *	that of this ADU; the difference between the placeholder's
 *	sequence number and that of the immediately following
 *	true ADU is the number of missing true ADUs represented
 *	by this placeholder.						*/

#if 0
static int	resetInAggregator(Sdr sdr, Object aggrElt)
{
	InAggregator	inAggr;
	Object		inAggrObj;
	Object		elt;
	Object		aduElt;
			OBJ_POINTER(InAdu, inAdu);

	inAggrObj = sdr_list_data(sdr, aggrElt);
	sdr_stage(sdr, (char *) &inAggr, inAggrObj, sizeof(InAggregator));
	for (elt = sdr_list_first(sdr, inAggr.inAdus); elt; )
	{
		/*	Remove all gaps in list of InAdus.		*/

		aduElt = elt;
		elt = sdr_list_next(sdr, elt);
		GET_OBJ_POINTER(sdr, InAdu, inAdu, sdr_list_data(sdr, aduElt));
		if (inAdu->aggregatedZCO == 0)
		{	
			deletePlaceholder(sdr, aduElt);
		}
	}

	if (parseInAdus(sdr) < 0)	/*	Process remaining adus.	*/
	{
		putErrmsg("Can't parse inbound adus.", NULL);
		return -1;
	}

	 /*	Reset aggregator.					*/

	loadScalar(&inAggr.nextExpected, 0);
	loadScalar(&inAggr.resetSeqNum, 0);
	inAggr.resetTimestamp = 0;
	sdr_write(sdr, inAggrObj, (char *) &inAggr, sizeof(InAggregator));
	writeMemo("[i] Reset an inbound aggregator.");
	if ((_dtpcvdb(NULL))->watching & WATCH_reset)
	{
		putchar('$');
		fflush(stdout);
	}
	
	return 0;
}
#endif

static int	insertAtPlaceholder(Sdr sdr, BpDelivery *dlv, Scalar seqNum,
			Object aggrElt, InAggregator *inAggr, Object phElt)
{
	Object	inAduObj;
	InAdu	inAdu;
	Object	sdrElt;
		OBJ_POINTER(InAdu, nextAdu);
	Scalar	tempScalar;
	int	result = 0;

	/*	If the PDU following the placeholder is a true PDU,
	 *	then we can simply transform the placeholder into the
	 *	missing ADU by setting its aggregatedZCO.
	 *
	 *	Otherwise, we indicate arrival of the missing ADU by
	 *	incrementing the sequence number of the placeholder
	 *	(so that it represents 1 fewer missing ADUs).		*/

	inAduObj = sdr_list_data(sdr, phElt);
	sdr_stage(sdr, (char *) &inAdu, inAduObj, sizeof(InAdu));
	sdrElt = sdr_list_next(sdr, phElt);
	GET_OBJ_POINTER(sdr, InAdu, nextAdu, sdr_list_data(sdr, sdrElt));
	copyScalar(&tempScalar, &seqNum);
	increaseScalar(&tempScalar, 1);
	result = compareScalars(&tempScalar, &nextAdu->seqNum);

	/*	Update the placeholder ADU one way or another.		*/

	if (result == 0)	/*	Only 1 missing ADU.		*/
	{
		inAdu.aggregatedZCO = dlv->adu;
		sdr_list_delete(sdr, inAdu.gapEventElt, deleteEltObjContent,
				NULL);
		inAdu.gapEventElt = 0;
		sdr_write(sdr, inAduObj, (char *) &inAdu, sizeof(InAdu));
	}
	else			/*	Still need placeholder.		*/
	{
		/*	Increment the placeholder sequence number,
		 *	reducing the size of the collection gap.	*/

		increaseScalar(&inAdu.seqNum, 1);
		sdr_write(sdr, inAduObj, (char *) &inAdu, sizeof(InAdu));

		/*	The new ADU didn't replace the placeholder, so
		 *	it must be inserted before it.			*/

		inAduObj = sdr_malloc(sdr, sizeof(InAdu));
		if (inAduObj == 0)
		{
			putErrmsg("No space for InAdu.",NULL);
			return -1;
		}

		sdr_list_insert_before(sdr, phElt, inAduObj);
		copyScalar(&inAdu.seqNum, &seqNum);
		inAdu.aggregatedZCO = dlv->adu;
		inAdu.inAggrElt = aggrElt;
		inAdu.gapEventElt = 0;
		sdr_write(sdr, inAduObj, (char *) &inAdu, sizeof(InAdu));
	}

	return 1;
}

static int	handleNextExpected(Sdr sdr, BpDelivery *dlv, Scalar seqNum,
			Object aggrElt, InAggregator *inAggr)
{
	Object	aduElt;
	Object	inAduObj;
	InAdu	inAdu;

	/*	If the aggregator's inAdus list is not empty, the
	 *	first element of the list must be the placeholder
	 *	for the next expected ADU.				*/

	aduElt = sdr_list_first(sdr, inAggr->inAdus);
	if (aduElt)
	{
		return insertAtPlaceholder(sdr, dlv, seqNum, aggrElt, inAggr,
				aduElt);
	}

	/*	The next expected ADU has arrived in order, before
	 *	any other ADUs with higher sequence number, so there
	 *	is no placeholder to insert at.  So insert at start
	 *	of list.						*/

	inAduObj = sdr_malloc(sdr, sizeof(InAdu));
	if (inAduObj == 0)
	{
		putErrmsg("No space for InAdu.",NULL);
		return -1;
	}

	sdr_list_insert_first(sdr, inAggr->inAdus, inAduObj);
	copyScalar(&inAdu.seqNum, &seqNum);
	inAdu.aggregatedZCO = dlv->adu;
	inAdu.inAggrElt = aggrElt;
	inAdu.gapEventElt = 0;
	sdr_write(sdr, inAduObj, (char *) &inAdu, sizeof(InAdu));
	return 1;
}

static time_t	getPlaceholderDeletionTime(BpDelivery *dlv)
{
	time_t	currentTime;
	time_t	deletionTime;

	currentTime = getCtime();
	deletionTime = (dlv->bundleCreationTime.seconds + EPOCH_2000_SEC
			+ dlv->timeToLive) - 1;
	if (deletionTime < currentTime)
	{
		deletionTime = currentTime;
	}

	return deletionTime;
}

static int	insertAduAtEnd(Sdr sdr, BpDelivery *dlv, Scalar seqNum,
			Profile *profile, Object aggrElt, InAggregator *inAggr,
			Scalar lastSeqNum)
{
	DtpcVdb		*vdb = getDtpcVdb();
	Object		inAduObj;
	InAdu		inAdu;
	Object		aduElt;
	DtpcEvent	event;

	/*	Append new ADU to end of aggregator's inAdus list,
	 *	preceded (as needed) by a placeholder for all prior
	 *	sequence numbers following the current last sequence
	 *	number.							*/

	increaseScalar(&lastSeqNum, 1);

	/*	We go through this loop at most twice:
	 *	  1.  Possibly once to append a placeholder.
	 *	  2.  Once to append the newly received ADU.		*/

	while (1)
	{
		inAduObj = sdr_malloc(sdr, sizeof(InAdu));
		if (inAduObj == 0)
		{
			putErrmsg("No space for InAdu.",NULL);
			return -1;
		}

		copyScalar(&inAdu.seqNum, &lastSeqNum);
		inAdu.inAggrElt = aggrElt;
		aduElt = sdr_list_insert_last(sdr, inAggr->inAdus, inAduObj);
		if (compareScalars(&seqNum, &lastSeqNum) == 0)
		{
			/*	Appending the received ADU.		*/

			inAdu.aggregatedZCO = dlv->adu;
			inAdu.gapEventElt = 0;
			sdr_write(sdr, inAduObj, (char *)
					&inAdu, sizeof(InAdu));
			break;	/*	Out of the loop.		*/
		}
		else
		{
			/*	This will be the placeholder ADU.	*/

			inAdu.aggregatedZCO = 0;
			event.type = DeleteGap;
			event.scheduledTime = getPlaceholderDeletionTime(dlv);
			event.aduElt = aduElt;
			inAdu.gapEventElt = insertDtpcTimelineEvent(&event);
			if (inAdu.gapEventElt == 0)
			{
				putErrmsg("Can't schedule deletion event.",
						NULL);
				return -1;
			}

			sdr_write(sdr, inAduObj, (char *)
					&inAdu, sizeof(InAdu));
			if (vdb->watching & WATCH_v)
			{
				putchar('v');
				fflush(stdout);
			}
		}

		/*	Ensure that received ADU is appended next time.	*/

		copyScalar(&lastSeqNum, &seqNum);
	}

	return 1;
}

static int	handleOutOfSeq(Sdr sdr, BpDelivery *dlv, Scalar seqNum,
			Profile *profile, Object aggrElt, InAggregator *inAggr)
{
	DtpcVdb		*vdb = getDtpcVdb();
	Object		aduElt;
	Object		sdrElt;
	InAdu		inAdu;
	Object		inAduObj = 0;		/*	To hush gcc	*/
	DtpcEvent	event;
	Scalar		lastSeqNum;
	int		result;
	Object		newAduElt;
			OBJ_POINTER(InAdu, nextAdu);
	Scalar		tempScalar;

	/*	The received ADU can only be inserted at the start
	 *	of the aggregator's inAdus list -- before or in
	 *	place of the first ADU in the list (which has to be
	 *	a placeholder) -- if it is the next expected ADU.
	 *	Since we know it is not, the received ADU has to be
	 *	inserted either at the end of the list or else
	 *	somewhere after the start of the list but before
	 *	the end.						*/

	aduElt = sdr_list_last(sdr, inAggr->inAdus);
	if (aduElt)
	{
		/*	The inAdus list is non-empty, so the last ADU
		 *	in the list has got to be a non-placeholder
		 *	ADU; every placeholder always has to be
		 *	immediately followed by a non-placeholder ADU.	*/

		inAduObj = sdr_list_data(sdr, aduElt);
		sdr_stage(sdr, (char *) &inAdu, inAduObj, sizeof(InAdu));
		copyScalar(&lastSeqNum, &inAdu.seqNum);
	}
	else
	{
		/*	The inAdus list is empty, so a	placeholder
		 *	must be inserted at the start of the list,
		 *	followed by the received ADU.
		 *
		 *	Reducing lastSeqNum by 1 forces the new
		 *	placeholder's sequence number to be the
		 *	next expected sequence number (after it is
		 *	incremented at start of insertAduAtEnd).	*/

		copyScalar(&lastSeqNum, &(inAggr->nextExpected));
		reduceScalar(&lastSeqNum, 1);
	}

	result = compareScalars(&seqNum, &lastSeqNum);
	if (result == 1)	/*	seqNum > lastSeqNum		*/
	{
		/*	The newly received ADU must go at the end of
		 *	the inAdus list, preceded by a placeholder as
		 *	needed.						*/

		result = insertAduAtEnd(sdr, dlv, seqNum, profile, aggrElt,
				inAggr, lastSeqNum);
		if (result < 0)
		{
			putErrmsg("Failed inserting ADU at end of list.",NULL);
			return -1;
		}

		return 1;
	}

	if (result == 0)	/*	seqNum == lastSeqNum		*/
	{
		/*	The last adu in the list can't be a placeholder,
		 *	so it's an ADU; so the newly received ADU must
		 *	be a duplicate.					*/

		zco_destroy(sdr, dlv->adu);
		if (vdb->watching & WATCH_discard)
		{
			putchar('?');
			fflush(stdout);
		}
	
		return 0;
	}

	/*	seqNum < lastSeqNum.  Must insert the received ADU
	 *	into the correct location in the inAdus list.		*/

	while (1)
	{
		aduElt = sdr_list_prev(sdr, aduElt);
		inAduObj = sdr_list_data(sdr, aduElt);
		sdr_stage(sdr, (char *) &inAdu, inAduObj, sizeof(InAdu));
		result = compareScalars(&seqNum, &inAdu.seqNum);
		if (result != 2)
		{
			/*	inAdu->seqNum <= seqNum, so insert
			 *	at this point.				*/

			break;
		}
	}

	if (result == 0)	/*	Exact match on sequence number.	*/
	{
		if (inAdu.aggregatedZCO)
		{
			/*	The ADU at the insertion point is
			 *	a true ADU (not a placeholder), so
			 *	the newly received ADU must be a
			 *	duplicate.  Discard it.			*/

			zco_destroy(sdr, dlv->adu);
			if (vdb->watching & WATCH_discard)
			{
				putchar('?');
				fflush(stdout);
			}

			return 0;
		}

		/*	The ADU at the insertion point is a
		 *	placeholder, the low end of whose collection
		 *	gap must be replaced by the newly received
		 *	ADU.						*/

		return insertAtPlaceholder(sdr, dlv, seqNum, aggrElt, inAggr,
				aduElt);
	}

	/*	result == 1; this ADU's sequence number is less than
	 *	the newly received ADU's sequence number, and no ADU
	 *	in the list has sequence number equal to the received
	 *	ADU's sequence number.  The ADU must be a placeholder:
	 *	if it were a true ADU, yet not the last element in
	 *	the list, then it would have been immediately followed
	 *	by a true ADU or placeholder whose sequence number
	 *	was equal to that of the received ADU, and we would
	 *	not be at this point in the list: the processing for
	 *	exact match would already have occurred.
	 *
	 *	So we must insert the newly arrived ADU immediately
	 *	after this placeholder, updating that placeholder's
	 *	deletion event time, and possibly add another
	 *	placeholder after it.
	 *
	 *	First we update the deletion event time of the
	 *	placeholder.						*/

	sdr_list_delete(sdr, inAdu.gapEventElt, deleteEltObjContent, NULL);
	event.type = DeleteGap;
	event.scheduledTime = getPlaceholderDeletionTime(dlv);
	event.aduElt = aduElt;
	inAdu.gapEventElt = insertDtpcTimelineEvent(&event);
	if (inAdu.gapEventElt == 0)
	{
		putErrmsg("Can't schedule deletion event.", NULL);
		return -1;
	}

	sdr_write(sdr, inAduObj, (char *) &inAdu, sizeof(InAdu));

	/*	Now we insert the newly received ADU after the
	 *	placeholder.						*/

	inAduObj = sdr_malloc(sdr, sizeof(InAdu));
	if (inAduObj == 0)
	{
		putErrmsg("No space for InAdu.",NULL);
		return -1;
	}

	newAduElt = sdr_list_insert_after(sdr, aduElt, inAduObj);
	copyScalar(&inAdu.seqNum, &seqNum);
	inAdu.aggregatedZCO = dlv->adu;
	inAdu.inAggrElt = aggrElt;
	inAdu.gapEventElt = 0;
	sdr_write(sdr, inAduObj, (char *) &inAdu, sizeof(InAdu));
		
	/*	Check to see if another placeholder is needed after
	 *	the newly inserted ADU, i.e., the sequence number
	 *	of the true ADU that immediately follows this one
	 *	is more than 1 greater than that of the newly
	 *	inserted ADU.						*/

	sdrElt = sdr_list_next(sdr, newAduElt);
	GET_OBJ_POINTER(sdr, InAdu, nextAdu, sdr_list_data(sdr, sdrElt));
	copyScalar(&tempScalar, &seqNum);
	increaseScalar(&tempScalar, 1);
	if (compareScalars(&tempScalar, &nextAdu->seqNum) == 2)
	{
		/*	Create placeholder at this point.		*/

		inAduObj = sdr_malloc(sdr, sizeof(InAdu));
		if (inAduObj == 0)
		{
			putErrmsg("No space for InAdu.",NULL);
			return -1;
		}

		sdrElt = sdr_list_insert_after(sdr, newAduElt, inAduObj);
		copyScalar(&inAdu.seqNum, &tempScalar);
		inAdu.aggregatedZCO = 0;
		inAdu.inAggrElt = aggrElt;
		event.type = DeleteGap;
		event.scheduledTime = getPlaceholderDeletionTime(dlv);
		event.aduElt = sdrElt;
		inAdu.gapEventElt = insertDtpcTimelineEvent (&event);
		if (inAdu.gapEventElt == 0)
		{
			putErrmsg("Can't schedule dtpc gap deletion event.",
					NULL);
			return -1;
		}

		sdr_write(sdr, inAduObj, (char *) &inAdu, sizeof(InAdu));
	}

	return 1;
}

int	handleInAdu(Sdr sdr, BpSAP txSap, BpDelivery *dlv, unsigned int profNum,
		Scalar seqNum)
{
	DtpcDB		*dtpcConstants = _dtpcConstants();
	DtpcVdb		*vdb = getDtpcVdb();
#if 0
	PsmPartition	wm = getIonwm();
	PsmAddress	psmElt;
#endif
	Profile		*profile;
	unsigned int	maxRtx;
	char		bogusEid[32];
	Object		aggrElt;
	Object		inAggrObj;
	InAggregator	inAggr;
	char		srcEid[SDRSTRING_BUFSZ];
	char		bogusReportToEid[SDRSTRING_BUFSZ];
	int		result;

	CHKERR(sdr_begin_xn(sdr));
	if (vdb->watching & WATCH_u)
	{
		putchar('u');
		fflush(stdout);
	}

	profile = findProfileByNumber(profNum);
	if (profile == NULL)	/*	Must add inferred profile.	*/
	{
		sdr_exit_xn(sdr);
		if (seqNum.units == 0 && seqNum.gigs == 0)
		{
			maxRtx = 0;	/*	No transport service.	*/
		}
		else
		{
			maxRtx = 1;	/*	Flag for transport.	*/
		}

		/*	Need bogus reportTo EID to ensure that the
 		 *	profile is added.  Otherwise it might have
 		 *	the same parameters as some existing profile
 		 *	and therefore be rejected; must be unique.	*/

		isprintf(bogusEid, sizeof bogusEid, "ipn:%u.2097151", profNum);
		if (addProfile(profNum, maxRtx, 0, 0, dlv->timeToLive, "0.1",
				bogusReportToEid, "") < 0)
		{
			putErrmsg("Can't add profile.", itoa(profNum));
			return -1;
		}

		profile = findProfileByNumber(profNum);
		if (profile == NULL)
		{
			putErrmsg("Didn't add profile.", itoa(profNum));
			return -1;
		}

		CHKERR(sdr_begin_xn(sdr));
	}

	/*	Look for InAggregator with the same ProfileID and
	 *	srcEid as the adu.					*/

	for (aggrElt = sdr_list_first(sdr, dtpcConstants->inAggregators);
		aggrElt; aggrElt = sdr_list_next(sdr, aggrElt))
	{
		inAggrObj = sdr_list_data(sdr, aggrElt);
		sdr_stage(sdr, (char *) &inAggr, inAggrObj,
				sizeof(InAggregator));
		if (inAggr.profileID != profNum)
		{
			continue;
		}

		if (sdr_string_read(sdr, srcEid, inAggr.srcEid) < 0)
		{
			sdr_exit_xn(sdr);
			putErrmsg("Failed reading source endpoint ID.", NULL);
			return -1;
		}

		if (strcmp(srcEid, dlv->bundleSourceEid) == 0)
		{
			break;	/*	Found matching aggregator.	*/
		}
	}

	if (aggrElt == 0)
	{
		/*	Did not find a suitable aggregator, must
		 *	create a new one.				*/

		memset((char *) &inAggr, 0, sizeof(InAggregator));
		inAggr.srcEid = sdr_string_create(sdr, dlv->bundleSourceEid);
		inAggr.profileID = profNum;
		if (profile->maxRtx == 0)	/*	No transport.	*/
		{
			loadScalar(&inAggr.nextExpected, 0);
		}
		else
		{
			loadScalar(&inAggr.nextExpected, 1);
		}

		inAggr.inAdus = sdr_list_create(sdr);
		inAggrObj = sdr_malloc(sdr, sizeof(InAggregator));
		if (inAggrObj == 0)
		{
			sdr_cancel_xn(sdr);
			putErrmsg("No space for inbound aggregator.",NULL);
			return -1;
		}

		sdr_write(sdr, inAggrObj, (char *) &inAggr,
				sizeof(InAggregator));
		aggrElt = sdr_list_insert_last(sdr,
				dtpcConstants->inAggregators, inAggrObj);
		if (aggrElt == 0)
		{
			sdr_cancel_xn(sdr);
			putErrmsg("Can't create inbound aggregator.", NULL);
			return -1;
		}
		
		if (vdb->watching & WATCH_i)
		{
			putchar('i');
			fflush(stdout);
		}	
	}
#if 0
	if (seqNum.units < 1000 && seqNum.gigs == 0)
	{
		/*	If this is the first item we receive just
			update aggregator, else check sequence number.	*/

		if (inAggr.resetTimestamp == 0)
		{
			result = 1;
		}
		else
		{
			result = compareScalars(&seqNum, &inAggr.resetSeqNum);
		}

		switch (result)
		{
		case 1:		/*	Update aggregator.		*/
			copyScalar(&inAggr.resetSeqNum, &seqNum);
			inAggr.resetTimestamp = dlv->bundleCreationTime.seconds
					+ EPOCH_2000_SEC + profile->lifespan;
			sdr_write(sdr, inAggrObj, (char *) &inAggr,
					sizeof(InAggregator));
			break;

		case 0:
		case 2:
			if ((dlv->bundleCreationTime.seconds + EPOCH_2000_SEC)
					< inAggr.resetTimestamp)
			{
				break;	/*	No reset, exit switch.	*/
			}

			/*	The sender reset, delete all gaps, send
				already acquired items and then reset
				InAggregator.				*/

			writeMemo("[i] Detected a reset in sender.");
			if (resetInAggregator(sdr, aggrElt) < 0)
			{
				sdr_cancel_xn(sdr);
				putErrmsg("Can't reset inbound aggregator.",
						NULL);
			}

			sdr_stage(sdr, (char *) &inAggr, inAggrObj,
					sizeof(InAggregator));
		default:
			break;
		}
	}
#endif
	if (profile->maxRtx == 0)	/*	No transport service.	*/
	{
		result = 0;		/*	Force "next expected".	*/
	}
	else	/*	Transport service requested for this profile.	*/
	{
		if (sendAck(txSap, profNum, seqNum, dlv) < 0)
		{
			putErrmsg("DTPC can't send acknowledgment.", NULL);
			return -1;
		}

		result = compareScalars(&seqNum, &inAggr.nextExpected);
	}

	/*	Insert the Adu into the collection sequence.		*/

	if (result == 2)	/*	seqNum < inAggr.nextExpected	*/
	{
		/*	We already have this item or it has expired.	*/
#if 0
		/*	or the sender/receiver have recently reset but
		 *	we received an item from before the reset.	*/
#endif
		zco_destroy(sdr, dlv->adu);
		if (vdb->watching & WATCH_discard)
		{
			putchar('?');
			fflush(stdout);
		}
		
		if (sdr_end_xn(sdr) < 0)
		{
			putErrmsg("Can't discard unusable inbound ADU.", NULL);
			return -1;
		}

		return 0;
	}
	else if (result == 0)	/*	seqNum == inAggr.nextExpected	*/
	{
		result = handleNextExpected(sdr, dlv, seqNum, aggrElt, &inAggr);
		if (result < 0)
		{
			sdr_cancel_xn(sdr);
			putErrmsg("Failed handling next expected ADU.",NULL);
			return -1;
		}
	}
	else			/*	seqNum > inAggr.nextExpected	*/
	{
		result = handleOutOfSeq(sdr, dlv, seqNum, profile, aggrElt,
				&inAggr);
		if (result < 0)
		{
			sdr_cancel_xn(sdr);
			putErrmsg("Failed handling out-of-sequence ADU.",NULL);
			return -1;
		}
	}
	
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't handle inbound adu.", NULL);
		return -1;
	}

	return result;
}

void	deletePlaceholder(Sdr sdr, Object aduElt)
{
	Object	aduObj;
		OBJ_POINTER(InAdu, adu);

	aduObj = sdr_list_data(sdr, aduElt);
	GET_OBJ_POINTER(sdr, InAdu, adu, aduObj);

	/*	Delete the event, the InAdu and their elements.		*/

	sdr_list_delete(sdr, adu->gapEventElt, deleteEltObjContent, NULL);
	sdr_free(sdr, aduObj);
	sdr_list_delete(sdr, aduElt, NULL, NULL);
}

static int	parseTopic(Sdr sdr, char *srcEid, ZcoReader *reader,
			unsigned char **cursor,	int buflen,
			unsigned int *bytesUnparsed)
{
	DtpcVdb		*vdb = getDtpcVdb();
	unsigned char	*buffer;
	int		parsedBytes = 0;
	int		bytesToRead;
	int		bytesReceived;
	uvast		topicNum;
	uvast		recordsCounter;
	uvast		payloadLength;
	int		remainingLength;
	int		sdnvLength;
	char		skipTopic = 0;		/*	Boolean		*/
	VSap		*vsap = 0;		/*	To hush gcc	*/
	PsmAddress	elt;
	PsmPartition	wm = getIonwm();
	Object		payloadObj = 0;		/*	To hush gcc	*/
	Object		dlvPayloadObj;
	Address		addr = 0;		/*	To hush gcc	*/
	DlvPayload	dlvPayload;

	buffer = (*cursor + *bytesUnparsed) - buflen;

	/* Since an Sdnv occupies at most 10 bytes, make sure there are
	 * at least 20 unparsed bytes in buffer to get the first 2
	 * sdnvs: topic id and number of records.			*/

	if (*bytesUnparsed < 20 && buflen == BUFMAXSIZE)
	{
		/*	Refill buffer					*/

		memmove(buffer, *cursor, *bytesUnparsed);
		*cursor = buffer + *bytesUnparsed;
		bytesToRead = buflen - *bytesUnparsed;
		bytesReceived = zco_receive_source(sdr, reader, bytesToRead,
				(char *) *cursor);

		/*	The buffer is now filled with unparsed data	*/

		*cursor = buffer;
		*bytesUnparsed += bytesReceived;
	} 

	/*	Get topic ID			*/

	sdnvLength = decodeSdnv(&topicNum, *cursor);
	*cursor += sdnvLength;
	*bytesUnparsed -= sdnvLength;
	parsedBytes += sdnvLength;

	/*	Get number of records in topic	*/

	sdnvLength = decodeSdnv(&recordsCounter, *cursor);
	*cursor += sdnvLength;
	*bytesUnparsed -= sdnvLength;
	parsedBytes += sdnvLength;

	/*	Find vsap for this topicID	*/

	for (elt = sm_list_first(wm, vdb->vsaps); elt;
			elt = sm_list_next(wm, elt))
	{
		vsap = (VSap *) psp(wm, sm_list_data(wm, elt));
		if (vsap->topicID == topicNum)
		{
			break;
		}
	}

	if (elt == 0 || vsap->appPid < 0 || sm_TaskExists(vsap->appPid) == 0)
	{
		/*	Skip this topic since there is no application
		 *	on this system to receive the payloads.		*/

		skipTopic = 1;
	}

	/* 		Parse all payload records.			*/

	for (; recordsCounter; recordsCounter--)
	{
		/*	Make sure the length sdnv is in the buffer	*/

		if (*bytesUnparsed < 10 && buflen == BUFMAXSIZE)
		{
			/*	Refill buffer				*/

			memmove(buffer, *cursor, *bytesUnparsed);
			*cursor = buffer + *bytesUnparsed;
			bytesToRead = buflen - *bytesUnparsed;
			bytesReceived = zco_receive_source(sdr, reader,
					bytesToRead, (char *) *cursor);

			/* The buffer is now filled with unparsed data	*/

			*cursor = buffer;
			*bytesUnparsed += bytesReceived;
		}

		/*	Get payload length				*/

		sdnvLength = decodeSdnv(&payloadLength, *cursor);
		*cursor += sdnvLength;
		*bytesUnparsed -= sdnvLength;
		parsedBytes += sdnvLength;
		if (skipTopic == 0)
		{
			payloadObj = sdr_malloc(sdr, payloadLength);
			if (payloadObj == 0)
			{
				putErrmsg("Can't allocate SDR for payload.",
						NULL);
				return -1;
			}

			addr = payloadObj;
		}

		remainingLength = payloadLength;
		while (remainingLength > 0)
		{
			if (remainingLength <= *bytesUnparsed)
			{
				/*	Remainder of payload is in
				 *	the buffer.			*/

				if (skipTopic == 0)
				{
					sdr_write(sdr, addr, (char *) *cursor,
							remainingLength);
					dlvPayloadObj = sdr_malloc(sdr,
							sizeof(DlvPayload));
					if (dlvPayloadObj == 0)
					{
						putErrmsg("Can't allocate SDR \
for DlvPayload.", NULL);
						return -1;
					}

					dlvPayload.srcEid =
						sdr_string_create(sdr, srcEid);
					dlvPayload.length = payloadLength;
					dlvPayload.content = payloadObj;
					sdr_write(sdr, dlvPayloadObj,
						(char *) &dlvPayload,
						sizeof(DlvPayload));
					oK(sdr_list_insert_last(sdr,
						vsap->dlvQueue, dlvPayloadObj));
				}

				parsedBytes += remainingLength;
				*bytesUnparsed -= remainingLength;
				*cursor += remainingLength;
				remainingLength = 0;
			}
			else
			{
				/*	Copy everything remaining in
				 *	buffer to the payload object,
				 *	then add more.			*/

				if (skipTopic == 0)
				{
					sdr_write(sdr, addr, (char *) *cursor,
							*bytesUnparsed);
					addr += *bytesUnparsed;
				}

				parsedBytes += *bytesUnparsed;
				remainingLength -= *bytesUnparsed;

				/*		Refill buffer		*/

				*cursor = buffer;
				bytesReceived = zco_receive_source(sdr, reader,
						buflen, (char *) *cursor);
				*bytesUnparsed = bytesReceived;
				if (*bytesUnparsed < remainingLength)
				{
					writeMemoNote("[?] DTPC user data \
item truncated", itoa((*bytesUnparsed) - remainingLength));
					break;
				}
			}
		}
	}

	if (skipTopic == 0)
	{
		sm_SemGive(vsap->semaphore);
	}

	return parsedBytes;
}

int	parseInAdus(Sdr sdr)
{
	DtpcDB		*dtpcConstants = _dtpcConstants();
	InAggregator	inAggr;
	Object		aggrElt;
	Object		aggrObj;
	Object		aduElt;
	Object		aduObj;
	int		remainingBytes;
	unsigned int	bytesUnparsed;
	vast		bytesReceived;
	int		parsedBytes;
	unsigned char	*buffer;
	unsigned char	*cursor;
	Scalar		parsedSeqNum;
	Scalar		tempScalar;
	int		buflen;
	char		srcEid[SDRSTRING_BUFSZ];
	ZcoReader	reader;
			OBJ_POINTER(InAdu, inAdu);

	CHKERR(sdr_begin_xn(sdr));
	for (aggrElt = sdr_list_first(sdr, dtpcConstants->inAggregators);
			aggrElt; aggrElt = sdr_list_next(sdr, aggrElt))
	{
		parsedSeqNum.gigs = -1;	/*	Note: underflow.	*/
		aggrObj = sdr_list_data(sdr, aggrElt);
		sdr_stage(sdr, (char *) &inAggr, aggrObj, sizeof(InAggregator));
		if (sdr_string_read(sdr, srcEid, inAggr.srcEid) < 0)
		{
			sdr_exit_xn(sdr);
			putErrmsg("Failed reading source endpoint ID.", NULL);
			return -1;
		}

		/*	Parse all non-empty adus of aggregator.		*/

		while (1)
		{
			aduElt = sdr_list_first(sdr, inAggr.inAdus);
			if (aduElt == 0)
			{
				break;	/*	List is empty.		*/
			}

			aduObj = sdr_list_data(sdr, aduElt);
			GET_OBJ_POINTER(sdr, InAdu, inAdu, aduObj);
			if (inAdu->aggregatedZCO == 0)
			{
				break;	/*	Found gap		*/ 
			}

			remainingBytes = zco_source_data_length(sdr,
					inAdu->aggregatedZCO);
			if (remainingBytes > BUFMAXSIZE)
			{
				buflen = BUFMAXSIZE;
			}
			else
			{
				buflen = remainingBytes;
			}

			buffer = MTAKE(buflen);
			if (buffer == NULL)
			{
				putErrmsg("Can't allocate memory to read ZCO.",
						NULL);
				return -1;
			}
			
			cursor = buffer;
			zco_start_receiving(inAdu->aggregatedZCO, &reader);
			bytesReceived = zco_receive_source(sdr, &reader, buflen,
					(char *) cursor);
			if (bytesReceived < 0)
			{
				putErrmsg("Error receiving adu.", NULL);
				sdr_cancel_xn(sdr);
				return -1;
			}

			bytesUnparsed = bytesReceived;
			while (remainingBytes)
			{
				parsedBytes = parseTopic(sdr, srcEid, &reader,
					&cursor, buflen, &bytesUnparsed);
				if (parsedBytes < 0)
				{
					putErrmsg("Can't parse adu topic.",
							NULL);
					sdr_cancel_xn(sdr);
					return -1;
				}

				remainingBytes -= parsedBytes;
			}

			copyScalar(&parsedSeqNum, &inAdu->seqNum);

			/*	Finished parsing adu, now clean up	*/

			MRELEASE(buffer);
			zco_destroy(sdr, inAdu->aggregatedZCO);
			sdr_free(sdr, aduObj);
			sdr_list_delete(sdr, aduElt, NULL, NULL);
		}

		if (parsedSeqNum.gigs < 0)
		{
			continue;	/*	No item was processed
						from this aggregator.	*/
		}

		copyScalar(&tempScalar, &parsedSeqNum);
		subtractFromScalar(&tempScalar, &inAggr.nextExpected);
		if (tempScalar.gigs == 0)
		{
			/*		Update Aggregator.		*/

			copyScalar(&inAggr.nextExpected, &parsedSeqNum);
			increaseScalar(&inAggr.nextExpected, 1);
			sdr_write(sdr, aggrObj, (char *) &inAggr,
					sizeof(InAggregator));
		}
	}

	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't parse inbound adus.", NULL);
		return -1;
	}

	return 0;
}

int	handleAck(Sdr sdr, BpDelivery *dlv, unsigned int profNum,
		Scalar seqNum)
{
	DtpcDB	*dtpcConstants = _dtpcConstants();
	Object	aggrElt;
	Object	aduElt;
	char	dstEid[SDRSTRING_BUFSZ];
	char	srcEid[64];
	uvast	nodeNbr;
		OBJ_POINTER(OutAggregator, outAggr);
		OBJ_POINTER(OutAdu, outAdu);

	CHKERR(sdr_begin_xn(sdr));
	zco_destroy(sdr, dlv->adu);	/*	The ZCO is not needed.	*/
	if (sscanf(dlv->bundleSourceEid, "%*[^:]:" UVAST_FIELDSPEC ".",
			&nodeNbr) < 1)
	{
		writeMemo("[?] Can't scan srcEid node number. Will not \
process ACK.");
		if (sdr_end_xn(sdr) < 0)
		{
			putErrmsg("Can't handle ACK.", NULL);
			return -1;
		}

		return 0;
	}

	isprintf(srcEid, sizeof srcEid, "ipn:" UVAST_FIELDSPEC ".%d", nodeNbr,
			DTPC_RECV_SVC_NBR);

	/*	Look for the OutAggregator, for this Profile, whose
	 *	dstEid is equal to the srcEid of the ACK.		*/

	for (aggrElt = sdr_list_first(sdr, dtpcConstants->outAggregators);
			aggrElt; aggrElt = sdr_list_next(sdr, aggrElt))
	{
		GET_OBJ_POINTER(sdr, OutAggregator, outAggr,
				sdr_list_data(sdr, aggrElt));
		if (outAggr->profileID != profNum)
		{
			continue;
		}

		if (sdr_string_read(sdr, dstEid, outAggr->dstEid) < 0)
		{
			sdr_cancel_xn(sdr);
			putErrmsg("Failed reading endpoint ID.", NULL);
			return -1;
		}

		if (strcmp(dstEid, srcEid) == 0)
		{
			break;	/*	Found matching aggregator.	*/
		}
	}

	if (aggrElt == 0)
	{
		writeMemoNote("[i] Received an ACK that does not match any \
outbound aggregator", srcEid);
		if (sdr_end_xn(sdr) < 0)
		{
			putErrmsg("Can't handle ACK.", NULL);
			return -1;
		}

		return 0;
	}

	for (aduElt = sdr_list_first(sdr, outAggr->outAdus); aduElt;
			aduElt = sdr_list_next(sdr, aduElt))
	{
		GET_OBJ_POINTER(sdr, OutAdu, outAdu,
				sdr_list_data(sdr, aduElt));
		if (outAdu->seqNum.gigs == seqNum.gigs
		&& outAdu->seqNum.units == seqNum.units)
		{
			break;	/*	Found matching ADU.		*/
		}
	}

	if (aduElt)
	{
		deleteAdu(sdr, aduElt);
	}

	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't handle ACK.", NULL);
		return -1;
	}

	return 0;
}

int	sendAck(BpSAP sap, unsigned int profileID, Scalar seqNum,
		BpDelivery *dlv)
{
	Sdr		sdr = getIonsdr();
	DtpcVdb		*vdb = getDtpcVdb();
	Sdnv		profileIdSdnv;
	Sdnv		seqNumSdnv;
	char		type;
	int		extentLength;
	unsigned char	*buffer;
	unsigned char	*cursor;
	Object		addr;
	Object		ackZco;
	Object		newBundle;
	BpAncillaryData	ancillaryData = { 0, 0, 0};
	BpCustodySwitch	custodySwitch = NoCustodyRequested;
	Profile		*profile;
	time_t		currentTime;
	int		lifetime;
	int		priority = 0;
	char		dstEid[64];
	uvast		nodeNbr;

	if (sscanf(dlv->bundleSourceEid, "%*[^:]:" UVAST_FIELDSPEC ".",
			&nodeNbr) < 1)
	{
		writeMemo("[?] Can't scan srcEid node number. Will not \
send ACK.");
		return 0;
	}

	isprintf(dstEid, sizeof dstEid, "ipn:" UVAST_FIELDSPEC ".%d", nodeNbr,
			DTPC_RECV_SVC_NBR);
	CHKERR(sdr_begin_xn(sdr));

	/*	Find profile.						*/

	profile = findProfileByNumber(profileID);
	if (profile == NULL)
	{
		/*	No profile found - Estimate lifetime.		*/

		currentTime = getCtime();
		lifetime = currentTime - (dlv->bundleCreationTime.seconds +
			 	EPOCH_2000_SEC) + 10; 	/* Add 10 seconds
							 * for safety.	*/							
	}
	else
	{
		lifetime = profile->lifespan;
	}

	if (lifetime < 0)
	{
		sdr_exit_xn(sdr);
		writeMemo("[?] Negative value for lifespan.");
		return 0;
	}

	/*	Construct DTPC acknowledgment.				*/

	type = 0x01;	/*	Top 2 bits are version number 00.	*/
	encodeSdnv(&profileIdSdnv, profileID);
	scalarToSdnv(&seqNumSdnv, &seqNum);
	extentLength = 1 + profileIdSdnv.length + seqNumSdnv.length;
	addr = sdr_malloc(sdr, extentLength);
	if (addr == 0)
	{
		sdr_cancel_xn(sdr);
		putErrmsg("No space in SDR for extent.", NULL);
		return -1;
	}

	buffer = MTAKE(extentLength);
	if (buffer == NULL)
	{
		sdr_cancel_xn(sdr);
		putErrmsg("Can't construct Dtpc ack header.", NULL);
		return -1;
	}

	cursor = buffer;
	*cursor = type;
	cursor++;
	memcpy(cursor, profileIdSdnv.text, profileIdSdnv.length);
	cursor += profileIdSdnv.length;
	memcpy(cursor, seqNumSdnv.text, seqNumSdnv.length);
	sdr_write(sdr, addr, (char *) buffer, extentLength);
	MRELEASE(buffer);

	/*	Create ZCO and send ACK.				*/

	ackZco = ionCreateZco(ZcoSdrSource, addr, 0, extentLength, priority,
			ancillaryData.ordinal, ZcoOutbound, NULL);
	if (ackZco == 0 || ackZco == (Object) ERROR)
	{
		putErrmsg("Can't create ack ZCO.", NULL);
		sdr_cancel_xn(sdr);
		return -1;
	}

	if (bp_send(sap, dstEid, NULL, lifetime, priority, custodySwitch,
			0, 0, &ancillaryData, ackZco, &newBundle) <= 0) 
	{
		putErrmsg("DTPC can't send ack.", NULL);
		sdr_cancel_xn(sdr);
		return -1;
	}

	/*	We don't track acknowledgments, so no need to detain
	 *	this bundle any further.				*/

	oK(bp_release(newBundle));
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("DTPC can't send ack.", NULL);
		return -1;
	}

	if (vdb->watching & WATCH_l)
	{
		putchar('l');
		fflush(stdout);
	}	

	return 0;
}

int	compareScalars(Scalar *scalar1, Scalar *scalar2)
{
	/*	Returns: 0 if scalars are equal, 1 if scalar1 >
	 *	scalar2, 2 if scalar2 > scalar1.			*/

	if (scalar1->units == scalar2->units && scalar1->gigs == scalar2->gigs)
	{
		return 0;
	}

	if ((scalar1->units > scalar2->units && scalar1->gigs == scalar2->gigs)
	|| scalar1->gigs > scalar2->gigs)
	{
		return 1;
	}

	return 2;
}

