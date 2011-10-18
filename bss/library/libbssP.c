/*
 *	libbssP.c:	functions enabling the implementation of
 *			BSS API.
 *								
 *									
 *	Copyright (c) 2011, Space Internetworking Center,
 *	Democritus University of Thrace.
 *	Copyright (c) 2011, California Institute of Technology.	
 *
 *	All rights reserved.						
 *	
 *	Author: Sotirios-Angelos Lenas, Space Internetworking Center (SPICE)
 */

#include "bssP.h"

#ifndef BSSLIBDEBUG
#define BSSLIBDEBUG	0
#endif

static int	stopLoop = 0;

int	_running(int *newValue)
{
	static int    running = -1;

	if (newValue)
	{
		running = *newValue;
	}

	return  running;	
}

pthread_t	_recvThreadId(pthread_t *newValue)
{
	static int    recvThreadId = -1;

	if (newValue)
	{
		recvThreadId = *newValue;
	}

	return  recvThreadId;		
}

int	_lockMutex(int value)
{
	static pthread_mutex_t      dbmutex = PTHREAD_MUTEX_INITIALIZER;

	if (value == 1)
	{
		if(_recvThreadId(NULL) != -1)
		{
			if(pthread_mutex_lock(&dbmutex) != 0)
			{
				putErrmsg("Couldn't take db mutex.", NULL);
				return -1;
			}
		}
	}

	if (value == 0)
	{
		if(_recvThreadId(NULL) != -1)
		{
			pthread_mutex_unlock(&dbmutex);
		}
	}

	return 0;		
}

int	_datFile(int control, int fileDescriptor)
{
	static int	dat = -1;

	switch (control)
	{
		case 1: 
			dat = fileDescriptor;
			break;
		case -1:
			close(dat);
			dat = -1;
			break;
		default:
			break;
	}
	return  dat;		
}

int	_lstFile(int control, int fileDescriptor)
{
	static int	lst=-1;

	switch (control)
	{
	case 1: 
		lst = fileDescriptor;
		break;

	case -1:
		close(lst);
		lst = -1;
		break;

	default:
		break;
	}
	return  lst;		
}

int	_tblFile(int control, int fileDescriptor)
{
	static int	tbl=-1;

	switch (control)
	{
	case 1: 
		tbl = fileDescriptor;
		break;

	case -1:
		close(tbl);
		tbl = -1;
		break;

	default:
		break;
	}
	return  tbl;		
}

BpSAP	_bpsap(BpSAP *newSAP)
{
	static BpSAP	sap = NULL;

	if (newSAP)
	{
		sap = *newSAP;
		sm_TaskVarAdd((int *) &sap);
	}

	return sap;
}

/* .tbl database file management functions */

static void initializeDB(tblIndex *index)
{	
	int i;

	index->oldestTime = 0;
	index->oldestRowIndex = 0;
	index->newestTime = 0;
	index->newestRowIndex = 0;

	for(i=0; i < SOD; i++)
	{
		index->firstEntryOffset[i] = -1;
		index->lastEntryOffset[i] = -1;
		index->hgstCountVal[i] = -1;
		index->lwstCountVal[i] = 0;
	}
}

int readTblFile(int fileD, tblIndex *index)
{
	if ((lseek(fileD, 0, SEEK_SET) < 0) || 
	    (read(fileD, index, sizeof(tblIndex)) < 0))
	{
		putSysErrmsg("BSS library: can't seek or read .tbl file", 
				NULL);
		return -1;
	}
	return 0;		
}

static int saveTblFile(int fileD, tblIndex *index)
{
	if ((lseek(fileD, 0, SEEK_SET) < 0) || 
	    (write(fileD, index, sizeof(tblIndex)) < 0))
	{
		putSysErrmsg("BSS library: can't seek or write to .tbl file", 
				NULL);
		return -1;
	}
	return 0;		
}

static void updateTbl(tblIndex *index, int rowNum, int _switch, long offset,  
		BpTimestamp time)
{
	/*  
	 *  this function updates the values of tblIndex structure based      
	 *  on _switch value. In the first case, it updates the variables     
	 *  that hold the details for the bundle with the oldest creation     
	 *  time of a particular second. In the second case it updates the    
	 *  variables that hold the details for the bundle with the newest    
	 *  creation time of a particular second. Finally, in the third case, 
	 *  the bundle that just got received, is the first recieved bundle,  
	 *  of a particular second so both variables that hold the details    
	 *  for the oldest and newest bundle are being initialized with the   
	 *  same value.		     			               
	 */

	if(index != NULL)
	{
		if (_switch == 0)
		{
			index->firstEntryOffset[rowNum] = offset;
			index->lwstCountVal[rowNum] = time.count;
		}
		else if (_switch == 1)
		{
			index->lastEntryOffset[rowNum] = offset;
			index->hgstCountVal[rowNum] = time.count;
		}
		else if (_switch == 2)
		{
			index->firstEntryOffset[rowNum] = offset;
			index->lwstCountVal[rowNum] = time.count;
			index->lastEntryOffset[rowNum] = offset;
			index->hgstCountVal[rowNum] = time.count;
		}
		
	}
}

/* .lst database file management functions */

int getLstEntry(int fileD, lstEntry *entry, long lstEntryOffset)
{
	if ((lseek(fileD, lstEntryOffset, SEEK_SET) < 0) ||
	    (read(fileD, entry, sizeof(lstEntry)) < 0))
	{
		putSysErrmsg("BSS library: can't seek or write to .lst file", 
				NULL);
		return -1;
	}
	return 0;
}

static int addEntry(int fileD, BpTimestamp time, long datOffset,
		long prev, long next, long dataLength)
{
	lstEntry entry;
	
	entry.crtnTime = time;
	entry.datOffset = datOffset;
	entry.prev = prev;
	entry.next = next;
	entry.pLen = dataLength;
		
	if ((lseek(fileD, 0, SEEK_END) < 0) ||
	    (write(fileD, &entry, sizeof(lstEntry)) < 0))
	{
		putSysErrmsg("BSS library: can't seek or write to .lst file", 
				NULL);
		return -1;
	}
	return 0;		
}

static int updateEntry(int fileD, lstEntry *entry, long prev, long next, 
		long offset)
{
	entry->prev = prev;
	entry->next = next;
		
	if ((lseek(fileD, offset, SEEK_SET) < 0) ||
	    (write(fileD, entry, sizeof(lstEntry)) < 0))
	{
		putSysErrmsg("BSS library: can't seek or write to .lst file", 
				NULL);
		return -1;
	}
	return 0;		
}

static int insertLstEdge(int fileD, lstEntry *curEntry, long curPrev, 
		long curNext, long offset, BpTimestamp time, long datOffset, 
		long prev, long next, long dataLength)
{
	if(updateEntry(fileD, curEntry, curPrev,
			 curNext, offset) < 0)
		{
			putErrmsg("Update of .lst file failed", NULL);
			return -1;
		}
			
	if(addEntry(fileD, time, datOffset, prev, next, dataLength) < 0)
	{
		putErrmsg("Update of .lst file failed", NULL);
		return -1;
	}

	return 1;		
}

static int insertLstIntrmd(int fileD, lstEntry *curEntry, long lstEntryOffset, 
		long newEntryOffset, BpTimestamp time, long datOffset, 
		long dataLength)
{
	lstEntry nextEntry;
	
	if(addEntry(fileD, time, datOffset, lstEntryOffset, curEntry->next, 
		dataLength) < 0)
	{
		putErrmsg("Update of .lst file failed", NULL);
		return -1;
	}
						
	if (getLstEntry(fileD, &nextEntry, curEntry->next) < 0)
	{
		putErrmsg("Retrieval of .lst entry failed", NULL);
		return -1;
	}

	if(updateEntry(fileD, &nextEntry, newEntryOffset, nextEntry.next, 
		curEntry->next) < 0)
	{
		putErrmsg("Update of .lst file failed", NULL);
		return -1;
	}

	if(updateEntry(fileD, curEntry, curEntry->prev, newEntryOffset, 
		lstEntryOffset) < 0)
	{
		putErrmsg("Update of .lst file failed", NULL);
		return -1;
	}
#if BSSLIBDEBUG
printf("Adding an intermediate entry into the list for %lu second\n", 
	time.seconds);
printf("ADD // new Entry Offset: %ld - previous offset: %ld - next offset: %ld\n", 
	newEntryOffset, lstEntryOffset, curEntry->next);
printf("UPDATE NEXT // offset: %ld - previous offset: %ld - next offset: %ld\n", 
	curEntry->next,  newEntryOffset, nextEntry.next);
printf("UPDATE CURRENT // offset: %ld - previous offset: %ld - next offset: %ld\n", 
	lstEntryOffset, curEntry->prev,  newEntryOffset);
#endif

	return 1;	
}

/* .dat database file management functions */

int readRecord(int fileD, dataRecord *rec, long datOffset)
{
	if ((lseek(fileD, datOffset, SEEK_SET) < 0) ||
	    (read(fileD, rec, sizeof(dataRecord)) < 0))
	{
		putSysErrmsg("BSS library: can't seek to / read from .dat file", 
				NULL);
		return -1;
	}
	return 0;
}

int readPayload(int fileD, char* buffer, int length)
{
	if (read(fileD, buffer, length*sizeof(char)) < 0)
	{
		putSysErrmsg("BSS library: can't read payload from .dat file", 
				NULL);
		return -1;
	}
	return 0;
}

static long addDataRecord(int fileD, BpTimestamp time, int payloadLength)
{
	long datOffset;
	dataRecord data;

	data.crtnTime = time;
	data.pLen = payloadLength;
	
	datOffset = (long) lseek(fileD, 0, SEEK_END);

#if BSSLIBDEBUG
printf("New data record added to the database\n");
printf("-------------------------------------\n");
printf("data Offset: %ld - creation time: %lu - length: %d\n", datOffset, 
	time.seconds, payloadLength);
#endif

	if ((datOffset < 0) || (write(fileD, &data, sizeof(dataRecord)) < 0))
	{
		putSysErrmsg("BSS library: can't seek or write to .dat file", 
				NULL);
		return -1;
	}

	return datOffset;		
}



/* receiver operations - section */

static int updateLstEntries(int lstFile, long lstEntryOffset,
		long newEntryOffset, long datOffset, BpTimestamp time,
		long dataLength)
{
	lstEntry 	curEntry;
	
	/*
	 *  this function adds new entries and applies appropriate updates    
	 *  on the contents of *.lst file, as needed. The fact of providing   
	 *  updateLstEntries function with a value of lstEntryOffset that     
	 *  equals zero, means that there are no entries related to the       
	 *  specified time, so a new entry, that isn't connected to any other 
	 *  entries, is created  and appended at the end of the file. In case 
	 *  that lstEntryOffset has a value other than zero, that means there 
	 *  is already a doubly-linked list created for the specified time,   
         *  so a new entry with the proper contents is created and appended   
         *  to the end of the file while the other entries of the particular  
	 *  doubly-linked list get updated (based on their count value) in    
         *  order for the new entry to be placed at the right spot.	       
	 */

	if (lstEntryOffset == 0)
	{
#if BSSLIBDEBUG
printf("List is empty. Adding the first entry for %lu second\n", time.seconds);
printf("ADD // new Entry Offset: %ld - previous offset: -1 - next offset: -1\n",
	newEntryOffset);
#endif
		if(addEntry(lstFile, time, datOffset, -1, -1, dataLength) < 0)
		{
			putErrmsg("Update of .lst file failed", NULL);
			return -1;
		}

		return 1;
	}
	
	if (getLstEntry(lstFile, &curEntry, lstEntryOffset) < 0)
	{
		putErrmsg("Retrieval of .lst entry failed", NULL);
		return -1;
	}
	
	if (curEntry.crtnTime.count < time.count)
	{
#if BSSLIBDEBUG
printf("Same as end of list. Adding an entry at the end of the list for %lu second\n", 
	time.seconds);
printf("UPDATE // offset: %ld - previous offset: %ld - next offset: %ld\n", 
	lstEntryOffset, curEntry.prev, newEntryOffset);
printf("ADD // new Entry Offset: %ld - previous offset: %ld - next offset: -1\n", 
	newEntryOffset, lstEntryOffset);
#endif

		if (insertLstEdge(lstFile, &curEntry, curEntry.prev, 
			newEntryOffset, lstEntryOffset, time, datOffset, 
			lstEntryOffset, -1, dataLength) < 0)
		{
			return -1;
		}
		

		return 1;
	}

	while (curEntry.prev != -1)
	{
		lstEntryOffset = curEntry.prev;

		if (getLstEntry(lstFile, &curEntry, lstEntryOffset) < 0)
		{
			putErrmsg("Retrieval of .lst entry failed", NULL);
			return -1;
		}

		if (curEntry.crtnTime.count < time.count)
		{
			if (insertLstIntrmd(lstFile, &curEntry, lstEntryOffset, 
		 			newEntryOffset, time, datOffset,
					dataLength) < 0)
			{
				return -1;	
			}

			return 1;
		}
	}
#if BSSLIBDEBUG
printf("Adding an entry at the beginning of the list for %lu second\n", 
	time.seconds);
printf("UPDATE // offset: %ld - previous offset: %ld - next offset: %ld\n", 
	lstEntryOffset, newEntryOffset,  curEntry.next);
printf("ADD // Entry Offset: %ld - previous offset: -1 - next offset: %ld\n", 
	newEntryOffset, lstEntryOffset);
#endif
	if (insertLstEdge(lstFile, &curEntry, newEntryOffset, 
			curEntry.next, lstEntryOffset, time, datOffset, 
			-1, lstEntryOffset, dataLength) < 0)
	{
		return -1;
	}

	return 1;
}

static int counterCheck(tblIndex *index, int position, long offset, 
		BpTimestamp time)
{
	/*
	 *  this function checks whether the contents of *.tbl file 
	 *  need an update based on the values of lwstCountVal and  
	 *  hgstCountVal variable. 				      
	 */

	if (time.count < index->lwstCountVal[position])
	{
		updateTbl(index, position, 0, offset, time);
	}
	else if (time.count > index->hgstCountVal[position])
	{
		updateTbl(index, position, 1, offset, time);
	}

	return 0;
}

static void eraseIntrmdRows (tblIndex *index, int bgn, int end)
{
	int diff, i=1;
	
	diff = end-bgn;
	if(diff < 0) diff = diff + SOD;
	while (i < diff)
	{
		index->firstEntryOffset[(bgn+i)%SOD]=-1;
		index->lastEntryOffset[(bgn+i)%SOD]=-1;
		index->hgstCountVal[(bgn+i)%SOD]=0;
		index->lwstCountVal[(bgn+i)%SOD]=0;
#if BSSLIBDEBUG
printf("Row (%d) was erased\n", (bgn+i)%SOD);
#endif
		i++;
	}
}

static long getEntryPosition(int tblFile, BpTimestamp time, long offset)
{
	tblIndex	index;
	long		lastOffset; 	/* last offset of every second */
	int		relativeTime;
	int		position;
	int		newestRow = -1;
	int		oldestRow = -1;
	int		flag=0;
	
	/* 
	 *  this function calculates in which position (row number),  
	 *  of the lists stored in .tbl, the new entry should be     
	 *  added and updates the proper elements, as needed.         
	 */

	if (readTblFile(tblFile, &index) < 0)
	{
		return -1;
	}
#if BSSLIBDEBUG
printf("(BEFORE) Creation time of the oldest frame stored in *.tbl file: %lu\n",
	index.oldestTime);
printf("(BEFORE) Creation time of the newest frame stored in *.tbl file: %lu\n",
	index.newestTime);
#endif

	/*  In case getEntryPosition function is executed for the first  
	 *  time, initialize the values of oldestTime and newestTime. */
	if (index.oldestTime == 0)
		index.oldestTime = time.seconds;
	if (index.newestTime == 0)
		index.newestTime = time.seconds;
	
	relativeTime = time.seconds - index.oldestTime;
	position = relativeTime + index.oldestRowIndex;

#if BSSLIBDEBUG
printf("(before calculation) position value: %d - NewestRowIndex value %d\n",
position, index.newestRowIndex);
#endif

	if (position < 0)
	{
		/* 
		 *  bundle's creation time is too old. A dataRecord will be  
		 *  inserted to *.dat file but it will not be trackable by   
		 *  *.tbl or *.lst files. 				    
		 */
		return -2; 
	}
	else if (position > (SOD-1))
	{
		/* In case position's value is a multiple of SOD, detract  
		 *  SOD from position. */
		while (position > (SOD-1)) 
		{	
			position = position - SOD;
			flag++; /* it is used to track how many times 
				 * SOD was detracted from position. */
		}
		
		/* 
		 *  Based on flag value, the age gap between the creation   
		 *  time of the current frame and the creation time of the   
		 *  already stored frames can be estimated. If the creation  
		 *  time of the current frame is so far in the future, all    
		 *  the intermediate entries must be deleted in order for   
		 *  the time scale of *.tbl lists to be retained. The        
		 *  contents of *.tbl file are being reinitialized.          
		 */
		if (position >= index.newestRowIndex && flag>=2)
		{
			initializeDB(&index);
			position = 0;
			newestRow = 0;
			oldestRow = 0;
		}
		else
		{
			/*  intermediate entries must be deleted in order for  
			 *  the time scale of *.tbl lists to be retained. */
			eraseIntrmdRows(&index, index.newestRowIndex, position);
			oldestRow = position+1;
			newestRow = position;			
		}
	}
	else
	{
		if (position > index.newestRowIndex)
		{ 
			/* intermediate entries must be deleted in order for 
			 *  the time scale of *.tbl lists to be retained   */			
			eraseIntrmdRows(&index, index.newestRowIndex, position);
			newestRow = position;
		}
	}

#if BSSLIBDEBUG
printf("(after calculation) position value: %d - NewestRowIndex value %d\n",
position, index.newestRowIndex);
#endif
	/*
	 *  Checks if there is already a doubly-linked list created for this   
	 *  particular second. In case that a doubly-linked list already       
	 *  exists, it returns the position (offset) of the doubly-linked list 
	 *  and updates accordingly the .tbl file. Otherwise, it returns a     
	 *  zero value to indicate that a doubly-linked list for this          
	 *  particular second doesn't exist and updates accordingly the .tbl   
	 *  file. 					      		       
	 */	
	if (index.lastEntryOffset[position] != -1)
	{
		lastOffset = index.lastEntryOffset[position];
		counterCheck(&index, position, offset, time);	
	}
	else
	{
		lastOffset = 0; //no records for this second yet
		updateTbl(&index, position, 2, offset, time);	
	}

	/*
	 *  if either newest or oldest row values got updated throughout the  
	 *  execution of this function, update index structure values in order   
	 *  to save changes later. 					      
	 */
	if (oldestRow!=-1)
	{
		index.oldestTime = index.oldestTime + oldestRow;
		index.oldestRowIndex = 	oldestRow;
	}
	
	if (newestRow!=-1)
	{
		index.newestRowIndex = newestRow;
		index.newestTime = time.seconds;
	
	}

	if (saveTblFile(tblFile, &index) < 0)
	{
		putSysErrmsg("Write to .tbl file failed", NULL);
		return -1;
	}

#if BSSLIBDEBUG
printf("(AFTER) Creation time of the oldest frame stored in *.tbl file: %lu\n", 
	index.oldestTime);
printf("(AFTER) Creation time of the newest frame stored in *.tbl file: %lu\n", 
	index.newestTime);
printf("firstEntryOffset: %lu - lastEntryOffset: %lu  returned value: %ld\n", 
index.firstEntryOffset[position], index.lastEntryOffset[position], firstOffset);
#endif

	return lastOffset;
}

static int	receiveFrame(Sdr sdr, BpDelivery *dlv, int datFile, int lstFile,
			int tblFile, BpTimestamp *lastDis, char *buffer, 
			int bufLength, RTBHandler display)
{
	ZcoReader       reader;
	int		contentLength;
	char		error[2] = "-1";
	long		datOffset;
	long 		newEntryOffset;
	long		lstEntryOffset;

	memset(buffer, '\0', bufLength);
	contentLength = zco_source_data_length(sdr, dlv->adu);
	if (contentLength <= bufLength)
	{
		sdr_begin_xn(sdr);
		zco_start_receiving(sdr, dlv->adu, &reader);
	
		if (zco_receive_source(sdr, &reader, bufLength, buffer) < 0)
		{
			zco_stop_receiving(sdr, &reader);
			sdr_cancel_xn(sdr);
			putErrmsg("bss: can't receive bundle content.",
				NULL);
			return -1;
		}

		zco_stop_receiving(sdr, &reader);
		if(sdr_end_xn(sdr) < 0)
		{
			putErrmsg("Can't handle delivery.", NULL);
			return -1;
		}

		if(_lockMutex(1) == -1)
		{
			return -1;
		}

		/*	write dataRecord information into .dat file 	*/
		datOffset = addDataRecord(datFile, dlv->bundleCreationTime, 
				contentLength);
		if (datOffset < 0)
		{
			sdr_cancel_xn(sdr);
			oK(_lockMutex(0));
			return -1;
		}
#if BSSLIBDEBUG
printf(".dat file offset that was returned from addDataRecord: %ld\n", datOffset);
#endif
		/*	write bundle's payload into .dat file	*/
		if (write(datFile, buffer, contentLength) < 1)
		{
			sdr_cancel_xn(sdr);
			putSysErrmsg("bss: can't write to .dat file",
				NULL);
			oK(_lockMutex(0));
			return -1;
		}
	
		newEntryOffset = (long) lseek(lstFile, 0, SEEK_END);
		if (newEntryOffset < 0)
		{
			putSysErrmsg("BSS library can't get the last offset of \
					.lst file", NULL);
			oK(_lockMutex(0));
			return -1;
		}
#if BSSLIBDEBUG
printf("new Entry Offset returned from seeking to the end of .lst file: %ld\n", 
newEntryOffset);
#endif	
		lstEntryOffset = getEntryPosition(tblFile,
				dlv->bundleCreationTime, newEntryOffset);
#if BSSLIBDEBUG
printf(".lst file Entry Offset that was returned from getEntryPosition function: \
	%ld\n", lstEntryOffset);
#endif
		if (lstEntryOffset == -2)
		{
			oK(_lockMutex(0));
			return 0; 	/*	frame too old, discard	*/
		}
		else if (lstEntryOffset == -1)
		{
			oK(_lockMutex(0));
			return -1;
		}

		if (updateLstEntries(lstFile, lstEntryOffset, newEntryOffset, 
			datOffset, dlv->bundleCreationTime, contentLength) < 0)
		{
			oK(_lockMutex(0));
			return -1;
		}

		oK(_lockMutex(0));	/*	unlock mutex 	*/
		
#if BSSLIBDEBUG
printf("from this point on, the execution of the provided display function starts\n");
#endif
		/*  display function is called only if the current frame has 
		 *  a creation time greater than the last displayed frame.  */ 
			
		if (dlv->bundleCreationTime.seconds > lastDis->seconds)
		{
			display(dlv->bundleCreationTime.seconds, 
				dlv->bundleCreationTime.count, buffer, 
				strlen(buffer));
			*lastDis = dlv->bundleCreationTime;
		}
		else if (dlv->bundleCreationTime.seconds == lastDis->seconds)
		{
			if(dlv->bundleCreationTime.count > lastDis->count)
			{
				display(dlv->bundleCreationTime.seconds, 
					dlv->bundleCreationTime.count, buffer, 
					strlen(buffer));
				*lastDis = dlv->bundleCreationTime;
			}
		}
	}
	else
	{
		display(0, 0, error, sizeof(error));
	}

	return 0;	
}

void*	recvBundles(void *args)
{
	BpSAP			sap;
	Sdr			sdr;
	BpDelivery		dlv;
	bss_thread_data 	*db;
	BpTimestamp		lastDis;
	
	/* initialize the variable that holds the time of the 
	 *  last displayed frame from receiving thread. */	
	lastDis.seconds = 0;
	lastDis.count = 0;

   	db = (bss_thread_data *) args;

	if (bp_attach() < 0)
	{
		putErrmsg("Can't attach to BP.", NULL);
		close(db->dat);
		close(db->lst);
		close(db->tbl);
		bssStop();
		return NULL;
	}

	if (bp_open(db->eid, &sap) < 0)
	{
		putErrmsg("Can't open own endpoint.", db->eid);
		close(db->dat);
		close(db->lst);
		close(db->tbl);
		bssStop();
		return NULL;
	}

	oK(_bpsap(&sap));
	sdr = bp_get_sdr();
	writeMemo("[i] bss is running.");
	while (_running(NULL))
	{
		if (bp_receive(sap, &dlv, BP_BLOCKING) < 0)
		{
			putErrmsg("bprecvfile bundle reception failed.", NULL);
			_running(&stopLoop);
			continue;
		}

		switch (dlv.result)
		{
		case BpEndpointStopped:
			_running(&stopLoop);
			break;		/*	Out of switch.		*/

		case BpPayloadPresent:
			if (receiveFrame(sdr, &dlv, db->dat, db->lst, db->tbl,
				&lastDis, db->buffer, db->bufLength, 
				db->function) < 0)
			{
				putErrmsg("bss failed.", NULL);
				_running(&stopLoop);
			}

			/*	Intentional fall-through to default.	*/

		default:
			break;		/*	Out of switch.		*/
		}

		bp_release_delivery(&dlv, 1);
	}

	/* close the files that recv thread had opened and has rd/rw access */
	close(db->dat);
	close(db->lst);
	close(db->tbl);
	bp_close(sap);
	writeErrmsgMemos();
	writeMemo("[i] Stopping bss.");
	bp_detach();
	bssStop();
	return NULL;
}

/* Create/Load database - section */

static int checkDb(int dat, int lst, int tbl)
{	
	dataRecord 	data;
	lstEntry 	entry;
	tblIndex 	index;
	char*		payload;

	if (_lockMutex(1) == -1)
	{
		return -1;
	}
	
	if (readTblFile(tbl, &index) <0)
	{
		return -1;
	}

	if (index.newestRowIndex != 0)
	{
		 if (getLstEntry(lst, &entry, 
			index.firstEntryOffset[index.oldestRowIndex]) < 0)
		{
			return -1;
		}

		if (readRecord(dat, &data, entry.datOffset) < 0)
		{
			return -1;
		}
			
		payload = calloc(data.pLen,sizeof(char));
		if (payload == NULL)
		{
			free(payload);
			return -2;
		}

		if (readPayload(dat, payload, data.pLen) < 0)
		{
			free(payload);		/* unresolved error  */
			return -1;
		}

		if ((index.oldestTime != entry.crtnTime.seconds) ||
		    (entry.crtnTime.seconds != data.crtnTime.seconds) ||
		    (entry.crtnTime.count != data.crtnTime.count))
		{
			free(payload);  /* database corruption detected  */
			return -1;
		}

		free(payload);			
	}

	oK(_lockMutex(0));
	return 0;
}

int loadRDWRDB(char* bssName, char* path, int* dat, int* lst, int* tbl)
{
	char 		fileName[255];
	long		tblSize;
	tblIndex	index;
	
	isprintf(fileName, sizeof(fileName), "%s/%s.dat", path, bssName);
	*dat = open(fileName, O_RDWR | O_CREAT, 0666);
	if (dat < 0)
	{
		putSysErrmsg("BSS Library: can't open .dat file", fileName);
		return -1;
	}

	isprintf(fileName, sizeof(fileName), "%s/%s.lst", path, bssName);
	*lst = open(fileName, O_RDWR | O_CREAT, 0666);
	if (lst < 0)
	{
		putSysErrmsg("BSS Library: can't open .lst file", fileName);
		return -1;
	}

	isprintf(fileName, sizeof(fileName), "%s/%s.tbl", path, bssName);
	*tbl = open(fileName, O_RDWR | O_CREAT, 0666);
	if (tbl < 0)
	{
		putSysErrmsg("BSS Library: can't open .tbl file", fileName);
		return -1;
	}
	
	/* Checks if DB already exists. If not, inits *.tbl file contents. */
	tblSize = (long) lseek(*tbl, 0, SEEK_END);
	if (tblSize < 0)
	{
		putSysErrmsg("BSS library: can't seek to the end to .tbl file",
				NULL);
		close(*dat);
		close(*lst);
		close(*tbl);
		return -1;
	}
	else if (tblSize == 0)
	{
		initializeDB(&index);
		
		if (saveTblFile(*tbl, &index) < 0)
		{
			putSysErrmsg("Write to .tbl file failed", NULL);
			return -1;
		}
	}
	
	/* database's integrity check*/
	if(checkDb(*dat, *lst, *tbl) == -1)
	{
		putErrmsg("Database is corrupted. Use recovery\
				 	mode", NULL);
		close(*dat);
		close(*lst);
		close(*tbl);
		return -1;
	}

	return 0;
}

int loadRDonlyDB(char* bssName, char* path)
{

	int	datRO;
	int	lstRO;
	int	tblRO;
	char	fileName[255];
	
	isprintf(fileName, sizeof(fileName), "%s/%s.dat", path, bssName);
	datRO = open(fileName, O_RDONLY, 0666);
	if (datRO < 0)
	{
		putSysErrmsg("BSS Library: can't open .dat file", fileName);
		return -1;
	}

	isprintf(fileName, sizeof(fileName), "%s/%s.lst", path, bssName);
	lstRO = open(fileName, O_RDONLY, 0666);
	if (lstRO < 0)
	{
		putSysErrmsg("BSS Library: can't open .lst file", fileName);
		return -1;
	}

	isprintf(fileName, sizeof(fileName), "%s/%s.tbl", path, bssName);
	tblRO = open(fileName, O_RDONLY, 0666);
	if (tblRO < 0)
	{
		putSysErrmsg("BSS Library: can't open .tbl file", fileName);
		return -1;
	}

	_datFile(1,datRO);
	_lstFile(1,lstRO);
	_tblFile(1,tblRO);
	
	/* database's integrity check */
	if (checkDb(_datFile(0,0),_lstFile(0,0), _tblFile(0,0)) == -1)
	{
		putErrmsg("Database is corrupted. Use recovery mode", NULL);
		return -1;
	}

	return 0;
}

/*database navigation - section*/

void updateNavInfo(bssNav *nav, int position, long datOffset, long prev, 
		long next)
{
	nav->curPosition = position;
	nav->datOffset = datOffset;
	nav->prevOffset = prev;
	nav->nextOffset = next;
}

int findLstEntryOffset(tblIndex index, time_t time, long *entryOffset)
{
	/*
	 *  this function locates the starting offset of the doubly-linked   
	 *  list  for the specified time and returns the position (row number) 
	 *  in which the offset was founded. 				 
	 */
	int	i;
	int	position = -1;

	for (i = 0; i < SOD; i++)
	{
		#if BSSLIBDEBUG
		printf("time1: %lu - time2: %lu\n", index.time[i].seconds,u
				time);
		#endif
		if ((index.oldestTime + i) >= time)
		{
			position=index.oldestRowIndex + i;
			if (position >= SOD) position = position - SOD;
			*entryOffset = index.firstEntryOffset[position];
			break;
		}
	}

	return position;
}
