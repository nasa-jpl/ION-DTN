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

	/*	Copy profile to volatile database	*/

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

	/* Recover the DTPC database, creating it if necessary.		*/

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

int	initOutAdu(Object outAggrAddr, Object outAggrElt, Object *outAduObj,
		Object *outAduElt)
{
	Sdr		sdr = getIonsdr();	
	OutAggregator	outAggr;
	OutAdu		outAduBuf;

        sdr_stage(sdr, (char *) &outAggr, outAggrAddr, sizeof(OutAggregator));
	memset((char *) &outAduBuf, 0, sizeof(OutAdu));
	outAduBuf.ageOfAdu = -1;
	outAduBuf.rtxCount = -1;
	copyScalar(&outAduBuf.seqNum, &outAggr.aduCounter);
	outAduBuf.outAggrElt = outAggrElt;
	outAduBuf.topics = sdr_list_create(sdr);

	*outAduObj = sdr_malloc(sdr, sizeof(OutAdu));
	*outAduElt = 0;

	if (*outAduObj == 0)
	{
		putErrmsg("No space for OutAdu.", NULL);
		return -1;
	}

	sdr_write(sdr, *outAduObj, (char *) &outAduBuf, sizeof(OutAdu)); 
	*outAduElt = sdr_list_insert_last(sdr, outAggr.outAdus, *outAduObj);
	increaseScalar(&outAggr.aduCounter, 1);
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
		currentTime = getUTCTime();
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

int	insertRecord (DtpcSAP sap, char *dstEid, unsigned int profileID,
		unsigned int topicID, Object item, int length)
{
	DtpcVdb		*vdb = getDtpcVdb();
	Sdr		sdr = getIonsdr();
	DtpcDB		*dtpcConstants = _dtpcConstants();
	PsmPartition	wm = getIonwm();
	PayloadRecord	record;
	Object		recordObj;
	OutAdu		outAdu;
	Object		outAduObj;
	Object		outAduElt;
	OutAggregator	outAggr;
	Object		outAggrAddr;
	PsmAddress	elt;
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

	for (elt = sm_list_first(wm, vdb->profiles); elt; 
			elt = sm_list_next(wm, elt))
	{
		vprofile = (Profile *) psp(wm, sm_list_data(wm, elt));

		if (vprofile->profileID == profileID)
		{	
			break;
		}
	}

	/*	No profile found	*/

	if (elt == 0)
	{
		writeMemo("[?] Profile does not exist.");
		return 0;
	}

	/*      Create payload record   */

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

	/* An outbound payload aggregator was not found - Create one.	*/

	if (sdrElt == 0)
	{
		memset((char *) &outAggr, 0, sizeof(OutAggregator));
		outAggr.dstEid = sdr_string_create(sdr, dstEid);
		outAggr.profileID = profileID;
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
		if (initOutAdu(outAggrAddr, sdrElt, &outAduObj,
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
	record.topicElt = insertToTopic(topicID, outAduObj, outAduElt,
			recordObj, vprofile->lifespan, &record, sap);
	if (record.topicElt == 0)
	{
		sdr_cancel_xn(sdr);
		return -1;
	}

	/* Estimate the resulting total length of the aggregated outAdu */

	sdr_stage(sdr, (char *) &outAdu, outAduObj, sizeof(OutAdu));
	totalLength = estimateLength(&outAdu);

	/*	If the estimated length equals or exceeds the
	 *	aggregation size limit then finish aggregation and
	 *	create an empty outbound ADU.				*/

	if (totalLength >= vprofile->aggrSizeLimit)
	{
		if (createAdu(vprofile, outAduObj, outAduElt) < 0)
		{
			putErrmsg("Can't send outbound adu.", NULL);
			sdr_cancel_xn(sdr);
			return -1;
		}

		if (initOutAdu(outAggrAddr, sdrElt, &outAduObj, &outAduElt) < 0)
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

	zco = ionCreateZco(ZcoSdrSource, addr, 0, extentLength, NULL);
	if (zco == 0 || zco == (Object) -1)
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
				extentLength, NULL);
		if (extent == 0 || extent == (Object) -1)
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
					0, payloadRec->length.length, NULL);
			if (extent == 0 || extent == (Object) -1)
			{
				putErrmsg("Can't create ZCO extent.", NULL);
				return -1;
			}

			extent = ionAppendZcoExtent(zco, ZcoSdrSource,
					payloadRec->payload, 0,
					payloadDataLength, NULL);
			if (extent == 0 || extent == (Object) -1)
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
	PsmPartition	wm = getIonwm();
	DtpcEvent	event;
	OutAdu		outAdu;
	time_t		currentTime;
	Object		bundleElt;
	Object		sdrElt;
	Object		outAduElt;
	Object		zco;
	Object		outAduObj;
	PsmAddress	elt;
	Profile		*profile;
	char		reportToEid[SDRSTRING_BUFSZ];
	char		dstEid[SDRSTRING_BUFSZ];
			OBJ_POINTER(OutAggregator, outAggr);

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

	/*	Create a new reference for the aggregated ZCO
	 *	and give this to bp.					*/

	zco = zco_clone(sdr, outAdu.aggregatedZCO, 0,
			zco_length(sdr, outAdu.aggregatedZCO));
	if (zco == 0)
	{
		sdr_exit_xn(sdr);
		putErrmsg("Can't clone aggregated ZCO.", NULL);
		return -1;
	}

	/* 	Find profile	*/

	for (elt = sm_list_first(wm, vdb->profiles); elt;
				elt = sm_list_next(wm, elt))
	{
		profile = (Profile *) psp(wm, sm_list_data(wm, elt));
		if (profile->profileID == outAggr->profileID)
		{
			break;
		}	
	}	

	if (elt == 0)
	{
		sdr_exit_xn(sdr);
		putErrmsg("Profile has been removed.", NULL);
		return -1;
	}

	if (sdr_string_read(sdr, reportToEid, profile->reportToEid) < 0 ||
		sdr_string_read(sdr, dstEid, outAggr->dstEid) < 0)
	{
		sdr_exit_xn(sdr);
		putErrmsg("Failed reading endpoint ID.", NULL);
		return -1;
	}

	currentTime = getUTCTime();
	while(1)
	{
		switch (bp_send(sap, dstEid, reportToEid,
			(outAdu.expirationTime - currentTime),
			profile->classOfService, profile->custodySwitch,
			profile->srrFlags, 1, &(profile->extendedCOS),
			zco, &outAdu.bundleObj))
		{
		case 0:		/*	No space for bundle.	*/
			if (errno == EWOULDBLOCK)
			{
				if (sdr_end_xn(sdr) < 0)
				{
					putErrmsg("DTPC xn failed.",
							NULL);
					return -1;
				}
				
				microsnooze(500000);
				CHKERR(sdr_begin_xn(sdr));
				currentTime = getUTCTime();
				if (outAdu.expirationTime < currentTime)
				{
					zco_destroy(sdr, zco);
					sdr_list_delete(sdr, sdrElt, NULL,
							NULL);
					deleteAdu(sdr, outAduObj);
					writeMemo("[i] OutAdu expired before \
being handed over to BP for transmission.");
					if (sdr_end_xn(sdr) < 0)
					{
						putErrmsg("Can't delete \
queued outAdu.", NULL);
						return -1;
					}

					return 0;
				}

				continue;
			}

		case -1:	/*	Intentional fall-through.	*/
			sdr_exit_xn(sdr);
			putErrmsg("DTPC can't send adu.", NULL);
			return -1;

		default:
			break;	/*	Out of switch.		*/
		}

		break;	/*	Out of loop.			*/
	}
	
	/*	Track bundle to avoid unnecessary retransmissions.	*/

	bundleElt = sdr_list_insert_last(sdr, outAggr->queuedAdus,
			outAdu.bundleObj);
	if (bp_track(outAdu.bundleObj, bundleElt) < 0)
	{
		sdr_cancel_xn(sdr);
		putErrmsg("Can't track bundle.", NULL);
		return -1;
	}

	/*		Get outAduElt from aggregator.			*/

	for (outAduElt = sdr_list_first(sdr, outAggr->outAdus); outAduElt;
		outAduElt = sdr_list_next(sdr, outAduElt))
	{
		if (outAduObj == sdr_list_data(sdr, outAduElt))
		{
			break;	/*	Found element.			*/
		}
	}

	outAdu.rtxCount++;

	/*		Create retransmission event only if there
	 *		is time for one.				*/

	if (outAdu.rtxCount < (int) profile->maxRtx && (currentTime +
	profile->lifespan / (profile->maxRtx + 1)) < outAdu.expirationTime)
	{
		event.type = ResendAdu;
		event.scheduledTime = currentTime + profile->lifespan /
					(profile->maxRtx + 1);
		event.aduElt = outAduElt;

		/*		Insert rtx event into list.		*/

		outAdu.rtxEventElt = insertDtpcTimelineEvent(&event);
		if (outAdu.rtxEventElt == 0)
		{
			sdr_cancel_xn(sdr);
			putErrmsg("Can't schedule Dtpc retransmission event.",
					NULL);
			return -1;
		}
	}

	/*		Create deletion event.				*/

	if (outAdu.delEventElt == 0)
	{
		event.type = DeleteAdu;
		event.scheduledTime = outAdu.expirationTime;
		event.aduElt = outAduElt;

		/*	Insert del event into list.			*/

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

	/*	Check if the adu is still in the bp queue and if
	 *	that is the case, do not resend.			*/

	aduObj = sdr_list_data(sdr, aduElt);
	sdr_stage(sdr, (char *) &adu, aduObj, sizeof(OutAdu));
	GET_OBJ_POINTER(sdr, OutAggregator, outAggr,
			sdr_list_data(sdr, adu.outAggrElt));
	for (elt = sdr_list_first(sdr, outAggr->queuedAdus); elt;
			elt = sdr_list_next(sdr, elt))
	{
		if (adu.bundleObj == sdr_list_data(sdr, elt))
		{
			aduIsQueued = 1; /* Adu still in queue.		*/
			break;
		}
	}

	/*		Delete the old rtxEvent.			*/

	sdr_list_delete(sdr, adu.rtxEventElt, deleteEltObjContent, NULL);
	adu.rtxEventElt = 0;
	if (aduIsQueued == 1)
	{
		for (elt = sdr_list_first(sdr, dtpcConstants->profiles); elt;
				elt = sdr_list_next(sdr, elt))
		{
			GET_OBJ_POINTER(sdr, Profile, profile, 
				sdr_list_data(sdr, elt));
			if (outAggr->profileID == profile->profileID)
			{
				break;	/* Found matching profile.	*/
			}
		}

		adu.rtxCount++;	/* Increase retransmission counter.	*/

		/* If the maxRtx limit defined in the profile being used
		 * has not been reached AND the expected time for the next
		 * retransmission is before the delEvent, create a new
		 * rtxEvent.						*/

		if (adu.rtxCount < (int) profile->maxRtx && (currentTime +
			profile->lifespan / (profile->maxRtx + 1)) <
			adu.expirationTime)
		{
			newEvent.type = ResendAdu;
			newEvent.scheduledTime = currentTime +
				(profile->lifespan / (profile->maxRtx + 1));
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
		/*	Adu was not found in BP queue, so requeue this
		 *	adu for transmission.				*/
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
			BpExtendedCOS *extendedCOS, unsigned char srrFlags,
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
		&& profile->extendedCOS.flowLabel == extendedCOS->flowLabel
		&& profile->extendedCOS.flags == extendedCOS->flags
		&& profile->extendedCOS.ordinal == extendedCOS->ordinal)
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
	PsmPartition	wm = getIonwm();
	DtpcVdb		*vdb = getDtpcVdb();
	BpExtendedCOS	extendedCOS = {0, 0, 0};
	BpCustodySwitch	custodySwitch = NoCustodyRequested;
	Profile		*vprofile;
	Profile		profile;
	Object		addr;
	PsmAddress	elt;
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

	if (profileID == 0 || maxRtx == 0 || lifespan ==0 
	|| aggrSizeLimit == 0 || aggrTimeLimit == 0)
	{
		sdr_exit_xn(sdr);
		writeMemoNote("[?] Missing profile parameter(s).",
				utoa(profileID));
		return 0;
	}

	/*		Check if it is a known  profile.		*/

	for (elt = sm_list_first(wm, vdb->profiles); elt;
			elt = sm_list_next(wm, elt))
	{
		vprofile = (Profile *) psp(wm, sm_list_data(wm, elt));
		if (vprofile->profileID == profileID)
		{
			break;
		}
	}

	if (elt != 0)
	{
		sdr_exit_xn(sdr);
		writeMemo("[?] Duplicate profile.");
		return 0;
	}

	if (!bp_parse_class_of_service(svcClass, &extendedCOS, 
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
			&extendedCOS, srrFlags, custodySwitch, reportToEid,
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
	profile.extendedCOS = extendedCOS;
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
			deleteGap(sdr, aduElt);
		}
	}

	if (parseInAdus(sdr) < 0)	/*	Process remaining adus.	*/
	{
		putErrmsg("Can't parse inbound adus.", NULL);
		return -1;
	}

	 /*		Reset aggregator.				*/

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

int	handleInAdu(Sdr sdr, BpDelivery *dlv, unsigned int profNum,
		Scalar seqNum)
{
	DtpcDB		*dtpcConstants = _dtpcConstants();
	DtpcVdb		*vdb = getDtpcVdb();
	PsmPartition	wm = getIonwm();
	PsmAddress	psmElt;
	Object		aggrElt;
	Object		aduElt;
	Object		sdrElt;
	InAggregator	inAggr;
	InAdu		inAdu;
	Object		inAggrObj;
	Object		inAduObj = 0;		/*	To hush gcc	*/
	DtpcEvent	event;
	Profile		*profile;
	Scalar		lastSeqNum;
	Scalar		tempScalar;
	char		srcEid[SDRSTRING_BUFSZ];
	int		result;
			OBJ_POINTER(InAdu, tempAdu);

	CHKERR(sdr_begin_xn(sdr));
	if (vdb->watching & WATCH_u)
	{
		putchar('u');
		fflush(stdout);
	}

	/* Read the profile parameters to get the lifespan,
	 * as it may be needed.						*/

	for (psmElt = sm_list_first(wm, vdb->profiles); psmElt; 
			psmElt = sm_list_next(wm, psmElt))
	{
		profile = (Profile *) psp(wm, sm_list_data(wm, psmElt));
		if (profile->profileID == profNum)
		{	
			break;	/*	Found matching profile.		*/
		}
	}

	if (psmElt == 0)
	{
		zco_destroy(sdr, dlv->adu);
		if (sdr_end_xn(sdr) < 0)
		{
			putErrmsg("Can't destroy invalid inbound adu.", NULL);
			return -1;
		}

		writeMemoNote("[?] Profile not found, discarded adu",
				utoa(profNum));
		if (vdb->watching & WATCH_discard)
		{
			putchar('?');
			fflush(stdout);
		}		

		return 0;
	}

	/* Look for InAggregator with the same ProfileID and srcEid
	 * as the adu.							*/

	for (aggrElt = sdr_list_first(sdr, dtpcConstants->inAggregators);
		aggrElt; aggrElt = sdr_list_next(sdr, aggrElt))
	{
		inAggrObj = sdr_list_data(sdr, aggrElt);
		sdr_stage(sdr, (char *) &inAggr, inAggrObj,
				sizeof(InAggregator));
		if (sdr_string_read(sdr, srcEid, inAggr.srcEid) < 0)
		{
			sdr_exit_xn(sdr);
			putErrmsg("Failed reading source endpoint ID.", NULL);
			return -1;
		}

		if (profNum == inAggr.profileID &&
			strcmp(srcEid, dlv->bundleSourceEid) == 0)
		{
			break;	/*	Found matching aggregator.	*/
		}
	}

	if (aggrElt == 0)
	{
		/*	Did not find a suitable aggregator,
		 *	will create a new one.				*/

		memset((char *) &inAggr, 0, sizeof(InAggregator));
		inAggr.srcEid = sdr_string_create(sdr, dlv->bundleSourceEid);
		inAggr.profileID = profNum;
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

	result = compareScalars(&seqNum, &inAggr.nextExpected);
	if (result == 2)	/*	seqNum < inAggr.nextExpected	*/
	{
		/*	We already have this item or it has expired or
			the sender/receiver have recently reset but we
			received an item from before the reset.		*/

		zco_destroy(sdr, dlv->adu);
		if (sdr_end_xn(sdr) < 0)
		{
			putErrmsg("Can't destroy duplicate/expired inbound \
item.", NULL);
			return -1;
		}

		if (vdb->watching & WATCH_discard)
		{
			putchar('?');
			fflush(stdout);
		}
		
		return 0;
	}
	else if (result == 0)	/* seqNum == inAggr.nextExpected	*/
	{
		/* If the inAdus list is not empty, the nextExpected
		 * seqNum will always be the first element of the list.	*/

		aduElt = sdr_list_first(sdr, inAggr.inAdus);
		if (aduElt)
		{
			inAduObj = sdr_list_data(sdr, aduElt);
			sdr_stage(sdr, (char *) &inAdu, inAduObj,
					sizeof(InAdu));

			/*	Check if the next item has the next
				sequence number and fill gap, else
				update it.				*/

			sdrElt = sdr_list_next(sdr, aduElt);
			GET_OBJ_POINTER(sdr, InAdu, tempAdu,
					sdr_list_data(sdr, sdrElt));
			copyScalar(&tempScalar, &seqNum);
			increaseScalar(&tempScalar, 1);
			result = compareScalars(&tempScalar, &tempAdu->seqNum);
			if (result == 0)
			{	/*		Fill Gap		*/
				inAdu.aggregatedZCO = dlv->adu;
				sdr_list_delete(sdr, inAdu.gapEventElt,
						deleteEltObjContent, NULL);
				inAdu.gapEventElt = 0;
			}
			else
			{	/* Increase gap seqnum since we received
				   the item for the current seqnum.	*/

				increaseScalar(&inAdu.seqNum, 1);
			}

			sdr_write(sdr, inAduObj, (char *) &inAdu,
					sizeof(InAdu));
		}

		if (aduElt == 0 || (aduElt && result))
		{	/* Insert item in the beginning of the list.	*/

			inAduObj = sdr_malloc(sdr, sizeof(InAdu));
			if (inAduObj == 0)
			{
				sdr_cancel_xn(sdr);
				putErrmsg("No space for InAdu.",NULL);
				return -1;
			}

			sdr_list_insert_first(sdr, inAggr.inAdus, inAduObj);
			copyScalar(&inAdu.seqNum, &seqNum);
			inAdu.aggregatedZCO = dlv->adu;
			inAdu.inAggrElt = aggrElt;
			inAdu.gapEventElt = 0;
			sdr_write(sdr, inAduObj, (char *) &inAdu,
					sizeof(InAdu));
		}
	}
	else if (result == 1)	/*	seqNum > inAggr.nextExpected	*/
	{
		aduElt = sdr_list_last(sdr, inAggr.inAdus);
		if (aduElt)
		{
			inAduObj = sdr_list_data(sdr, aduElt);
			sdr_stage(sdr, (char *) &inAdu, inAduObj,
					sizeof(InAdu));
			copyScalar(&lastSeqNum, &inAdu.seqNum);
		}
		else
		{
			copyScalar(&lastSeqNum, &inAggr.nextExpected);

			/* Note: If nextExpected is zero, the
			   following reduction will make lastSeqNum
			   underflow. This is expected behavior.	*/

			reduceScalar(&lastSeqNum, 1);
		}

		result = compareScalars(&seqNum, &lastSeqNum);
		if (result == 1)	/* seqNum > lastSeqNum		*/
		{
			increaseScalar(&lastSeqNum, 1);
			while (1)
			{
				inAduObj = sdr_malloc(sdr, sizeof(InAdu));
				if (inAduObj == 0)
				{
					sdr_cancel_xn(sdr);
					putErrmsg("No space for InAdu.",NULL);
					return -1;
				}

				copyScalar(&inAdu.seqNum, &lastSeqNum);
				inAdu.aggregatedZCO = 0;
				inAdu.inAggrElt = aggrElt;
				inAdu.gapEventElt = 0;
				aduElt = sdr_list_insert_last(sdr,
						inAggr.inAdus, inAduObj);
				if (compareScalars(&seqNum, &lastSeqNum) == 0)
				{
					inAdu.aggregatedZCO = dlv->adu;
					sdr_write(sdr, inAduObj, (char *)
							&inAdu, sizeof(InAdu));
					break;
				}
				else
				{
					event.type = DeleteGap;
					event.scheduledTime =
						dlv->bundleCreationTime.seconds
						+ EPOCH_2000_SEC
						+ profile->lifespan - 1;
					event.aduElt = aduElt;
					inAdu.gapEventElt =
						insertDtpcTimelineEvent(&event);
					if (inAdu.gapEventElt == 0)
					{
						sdr_cancel_xn(sdr);
						putErrmsg("Can't schedule dtpc \
gap deletion event.", NULL);
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

				copyScalar(&lastSeqNum, &seqNum);
			}
		}
		else if (result == 0)	/* seqNum == lastSeqNum		*/
		{
			/* The last adu in the list can't be a gap,
			 * so this is a duplicate adu.			*/

			zco_destroy(sdr, dlv->adu);
			if (sdr_end_xn(sdr) < 0)
			{
				putErrmsg("Can't destroy duplicate inbound \
adu.", NULL);
				return -1;
			}

			if (vdb->watching & WATCH_discard)
			{
				putchar('?');
				fflush(stdout);
			}
		
			return 0;
		}
		else if (result == 2)	/* seqNum < lastSeqNum		*/
		{
			while (1)
			{
				aduElt = sdr_list_prev(sdr, aduElt);
				inAduObj = sdr_list_data(sdr, aduElt);
				sdr_stage(sdr, (char *) &inAdu, inAduObj,
						sizeof(InAdu));
				result = compareScalars(&seqNum, &inAdu.seqNum);
				if (result != 2)
				{	/*	tempAdu->seqNum<=seqNum	*/
					break;
				}
			}

			if (result == 0)
			{
				if (inAdu.aggregatedZCO)
				{
					/* We already have this adu.	*/

					zco_destroy(sdr, dlv->adu);
					if (sdr_end_xn(sdr) < 0)
					{
						putErrmsg("Can't destroy \
duplicate inbound adu.", NULL);
						return -1;
					}

					if (vdb->watching & WATCH_discard)
					{
						putchar('?');
						fflush(stdout);
					}

					return 0;
				}

				/* Check if the next item has the next
				   sequence number and fill gap, else
				   update it.				*/

				sdrElt = sdr_list_next(sdr, aduElt);
				GET_OBJ_POINTER(sdr, InAdu, tempAdu,
						sdr_list_data(sdr, sdrElt));
				copyScalar(&tempScalar, &seqNum);
				increaseScalar(&tempScalar, 1);
				result = compareScalars(&tempScalar,
						&tempAdu->seqNum);
				if (result == 0)
				{	/*	Fill gap.		*/
					inAdu.aggregatedZCO = dlv->adu;
					sdr_list_delete(sdr, inAdu.gapEventElt,
							deleteEltObjContent,
							NULL);
					inAdu.gapEventElt = 0;
					sdr_write(sdr, inAduObj, (char *)
							&inAdu, sizeof(InAdu));
				}
				else
				{	/* Increase gap seqnum since we
					   received the item for the
					   current seqnum.		*/

					increaseScalar(&inAdu.seqNum, 1);
					sdr_write(sdr, inAduObj, (char *)
							&inAdu, sizeof(InAdu));

					/* Insert item before the gap.	*/

					inAduObj = sdr_malloc(sdr,
							sizeof(InAdu));
					if (inAduObj == 0)
					{
						sdr_cancel_xn(sdr);
						putErrmsg("No space for InAdu.",
								NULL);
						return -1;
					}

					sdr_list_insert_before(sdr, aduElt,
							inAduObj);
					copyScalar(&inAdu.seqNum, &seqNum);
					inAdu.aggregatedZCO = dlv->adu;
					inAdu.inAggrElt = aggrElt;
					inAdu.gapEventElt = 0;
					sdr_write(sdr, inAduObj, (char *)
							&inAdu, sizeof(InAdu));
				}
			}
			else	/*	result == 1			*/
			{
				/* Create gap with same seqnum as the
				   current one, but different event
				   time.				*/

				inAduObj = sdr_malloc(sdr, sizeof(InAdu));
				if (inAduObj == 0)
				{
					sdr_cancel_xn(sdr);
					putErrmsg("No space for InAdu.",NULL);
					return -1;
				}

				sdrElt = sdr_list_insert_before(sdr, aduElt,
						inAduObj);

				/*	Create event for new gap.	*/

				event.type = DeleteGap;
				event.scheduledTime =
						dlv->bundleCreationTime.seconds
						+ EPOCH_2000_SEC
						+ profile->lifespan - 1;
				event.aduElt = sdrElt;
				inAdu.gapEventElt = insertDtpcTimelineEvent
						(&event);
				if (inAdu.gapEventElt == 0)
				{
					sdr_cancel_xn(sdr);
					putErrmsg("Can't schedule dtpc gap \
deletion event.", NULL);
					return -1;
				}

				sdr_write(sdr, inAduObj, (char *) &inAdu,
						sizeof(InAdu));

				/* Insert item before the original gap.	*/

				inAduObj = sdr_malloc(sdr, sizeof(InAdu));
				if (inAduObj == 0)
				{
					sdr_cancel_xn(sdr);
					putErrmsg("No space for InAdu.",NULL);
					return -1;
				}

				sdr_list_insert_before(sdr, aduElt, inAduObj);
				copyScalar(&inAdu.seqNum, &seqNum);
				inAdu.aggregatedZCO = dlv->adu;
				inAdu.inAggrElt = aggrElt;
				inAdu.gapEventElt = 0;
				sdr_write(sdr, inAduObj, (char *) &inAdu,
						sizeof(InAdu));
				
				/* Check if there should be a gap after
				   the item and update original gap
				   seqnum or delete the gap.		*/

				sdrElt = sdr_list_next(sdr, aduElt);
				GET_OBJ_POINTER(sdr, InAdu, tempAdu,
						sdr_list_data(sdr, sdrElt));
				copyScalar(&tempScalar, &seqNum);
				increaseScalar(&tempScalar, 1);
				if (compareScalars(&tempScalar,
						&tempAdu->seqNum))
				{
					inAduObj = sdr_list_data(sdr, aduElt);
					sdr_stage(sdr, (char *) &inAdu,
						inAduObj, sizeof(InAdu));
					copyScalar(&inAdu.seqNum, &seqNum);
					increaseScalar(&inAdu.seqNum, 1);
					sdr_write(sdr, inAduObj, (char *)
						&inAdu, sizeof(InAdu));
				}
				else
				{
					deleteGap(sdr, aduElt);
				}
			}
		}
	}
	
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't handle inbound adu.", NULL);
		return -1;
	}

	return 1;
}

void	deleteGap(Sdr sdr, Object aduElt)
{
	Object		aduObj;
			OBJ_POINTER(InAdu, adu);

	aduObj = sdr_list_data(sdr, aduElt);
	GET_OBJ_POINTER(sdr, InAdu, adu, aduObj);

	/*	Delete the event, the InAdu and their elements.		*/

	sdr_list_delete(sdr, adu->gapEventElt, deleteEltObjContent, NULL);
	sdr_free(sdr, aduObj);
	sdr_list_delete(sdr, aduElt, NULL, NULL);
}

static int	parseTopic(Sdr sdr, char *srcEid, ZcoReader *reader,
			unsigned char **cursor,	int buflen, int *bytesUnparsed)
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
		/* Skip this topic since there is no application
		 * on this system to get the payloads.			*/

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
	int		bytesUnparsed;
	int		bytesReceived;
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
	zco_destroy(sdr, dlv->adu);	/* The ZCO is not needed.	*/
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

	/* Look for OutAggregator with the same ProfileID and dstEid
	 * same as the srcEid of the ACK.				*/

	for (aggrElt = sdr_list_first(sdr, dtpcConstants->outAggregators);
		aggrElt; aggrElt = sdr_list_next(sdr, aggrElt))
	{
		GET_OBJ_POINTER(sdr, OutAggregator, outAggr,
				sdr_list_data(sdr, aggrElt));

		if (sdr_string_read(sdr, dstEid, outAggr->dstEid) < 0)
		{
			sdr_cancel_xn(sdr);
			putErrmsg("Failed reading endpoint ID.", NULL);
			return -1;
		}

		if (profNum == outAggr->profileID
		&& strcmp(dstEid, srcEid) == 0)
		{
			break;	/*	Found matching aggregator.	*/
		}
	}

	if (aggrElt == 0)
	{
		writeMemo("[i] Received an ACK that does not match any \
				outbound aggregator");
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
		if (outAdu->seqNum.gigs == seqNum.gigs &&
			outAdu->seqNum.units == seqNum.units)
		{
			break;		/*	Found adu.		*/
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

int	sendAck (BpSAP sap, unsigned int profileID, Scalar seqNum,
		BpDelivery *dlv)
{
	Sdr		sdr = getIonsdr();
	DtpcVdb		*vdb = getDtpcVdb();
	PsmPartition	wm = getIonwm();
	Sdnv		profileIdSdnv;
	Sdnv		seqNumSdnv;
	char		type;
	int		extentLength;
	unsigned char	*buffer;
	unsigned char	*cursor;
	Object		addr;
	Object		ackZco;
	Object		newBundle;
	BpExtendedCOS	extendedCOS = { 0, 0, 0};
	BpCustodySwitch	custodySwitch = NoCustodyRequested;
	Profile		*profile;
	PsmAddress	elt;
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

	/*			Find profile.				*/

	for (elt = sm_list_first(wm, vdb->profiles); elt;
			elt = sm_list_next(wm, elt))
	{
		profile = (Profile *) psp(wm, sm_list_data(wm, elt));
		if (profile->profileID == profileID)
		{
			break;
		}
	}

	if (elt == 0)
	{
		/*	No profile found - Estimate lifetime.		*/

		currentTime = getUTCTime();
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

	/*		Construct DTPC acknowledgment.			*/

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

	/*		Create ZCO and send ACK.			*/

	ackZco = ionCreateZco(ZcoSdrSource, addr, 0, extentLength, NULL);
	if (ackZco == 0 || ackZco == (Object) -1)
	{
		putErrmsg("Can't create ack ZCO.", NULL);
		sdr_cancel_xn(sdr);
		return -1;
	}

	while (1)
	{
		switch (bp_send(sap, dstEid, NULL, lifetime, priority,
			custodySwitch, 0, 0, &extendedCOS, ackZco, &newBundle)) 
		{
		case 0:		/*	No space for bundle.		*/
			if (errno == EWOULDBLOCK)
			{
				if (sdr_end_xn(sdr) < 0)
				{
					putErrmsg("DTPC xn failed.", NULL);
					return -1;
				}

				microsnooze(500000);
				CHKERR(sdr_begin_xn(sdr));
				continue;
			}

		case -1:	/*	Intentional fall-through.	*/
			sdr_exit_xn(sdr);
			putErrmsg("DTPC can't send ack.", NULL);
			return -1;
	
		default:
			break;	/*	Out of switch.			*/
		}

		break;	/*		Out of loop.			*/
	}

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

