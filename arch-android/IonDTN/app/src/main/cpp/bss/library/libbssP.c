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

int	_running(int *newValue)
{
	static int    running = -1;

	if (newValue)
	{
		running = *newValue;
	}

	return  running;	
}

int	_recvThreadId(pthread_t *id, int control)
{
	static int		threadIdValid = 0;
	static pthread_t	recvThreadId;

	switch (control)
	{
	case -1:		/*	Unregister.			*/
		threadIdValid = 0;
		break;

	case 0:			/*	Query.				*/
		break;

	default:		/*	Register.			*/
		recvThreadId = *id;
		threadIdValid = 1;
	}

	if (id)
	{
		*id = recvThreadId;
	}

	return  threadIdValid;
}

int	_lockMutex(int value)
{
	static pthread_mutex_t      dbmutex = PTHREAD_MUTEX_INITIALIZER;

	if (value == 1)
	{
		if(_recvThreadId(NULL, 0))
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
		if(_recvThreadId(NULL, 0))
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
	void	*value;
	BpSAP	sap;

	if (newSAP)			/*	Add task variable.	*/
	{
		value = (void *) (*newSAP);
		sap = (BpSAP) sm_TaskVar(&value);
	}
	else				/*	Retrieve task variable.	*/
	{
		sap = (BpSAP) sm_TaskVar(NULL);
	}

	return sap;
}

/* .tbl database file management functions */

static int	writeTblFile(int fd, long offset, char *from, long length)
{
	if (lseek(fd, offset, SEEK_SET) < 0)
	{
		putSysErrmsg("BSS library: can't seek in .tbl file", itoa(fd));
		return -1;
	}

	if (write(fd, (char *) from, length) < 0)
	{
		putSysErrmsg("BSS library: can't write to .tbl file", itoa(fd));
		return -1;
	}

	return 0;
}

static int	initializeTblIndex(int fd, tblIndex *index)
{	
	tblHeader	*hdr = &(index->header);
	int		i;
	tblRow		*row;

	hdr->oldestTime = 0;
	hdr->oldestRowIndex = 0;
	hdr->newestTime = 0;
	hdr->newestRowIndex = 0;

	for (i = 0; i < WINDOW; i++)
	{
		row = index->rows + i;
		row->firstEntryOffset = -1;
		row->lastEntryOffset = -1;
		row->hgstCountVal = 0;
		row->lwstCountVal = 0;
	}

	return writeTblFile(fd, 0, (char *) index, sizeof(tblIndex));
}

static int	loadTblIndex(int fd, tblIndex *index)
{
	if (lseek(fd, 0, SEEK_SET) < 0)
	{
		putSysErrmsg("BSS library: can't seek in .tbl file", itoa(fd));
		return -1;
	}

	if (read(fd, (char *) index, sizeof(tblIndex)) < 0)
	{
		putSysErrmsg("BSS library: can't read .tbl file", itoa(fd));
		return -1;
	}

	return 0;
}

tblIndex	*_tblIndex(int *control)
{
	static tblIndex	*index = NULL;

	if (control)
	{
		if (index)		/*	Destroy if it's loaded	.*/
		{
			MRELEASE(index);
			index = NULL;
		}

		if (*control)		/*	Creating.		*/
		{
			index = MTAKE(sizeof(tblIndex));
		}
	}

	return index;
}

/* .lst database file management functions */

int	getLstEntry(int fileD, lstEntry *entry, long lstEntryOffset)
{
	if ((lseek(fileD, (off_t) lstEntryOffset, SEEK_SET) < 0) ||
	    (read(fileD, entry, sizeof(lstEntry)) < 0))
	{
		putSysErrmsg("BSS library: can't seek or read from .lst file", 
				NULL);
		return -1;
	}

	return 0;
}

static int	addEntry(int fileD, BpTimestamp time, off_t datOffset,
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

static int	updateEntry(int fileD, lstEntry *entry, long prev, long next, 
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

static int	insertLstEdge(int fileD, lstEntry *curEntry, long curPrev, 
			long curNext, long offset, BpTimestamp time,
			off_t datOffset, long prev, long next, long dataLength)
{
	if (updateEntry(fileD, curEntry, curPrev, curNext, offset) < 0)
	{
		putErrmsg("Update of .lst file failed", NULL);
		return -1;
	}
			
	if (addEntry(fileD, time, datOffset, prev, next, dataLength) < 0)
	{
		putErrmsg("Update of .lst file failed", NULL);
		return -1;
	}

	return 1;		
}

static int	insertLstIntrmd(int fileD, lstEntry *curEntry,
			long lstEntryOffset, long newEntryOffset,
			BpTimestamp time, off_t datOffset, long dataLength)
{
	lstEntry	nextEntry;

	if (addEntry(fileD, time, datOffset, lstEntryOffset, curEntry->next, 
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

	if (updateEntry(fileD, &nextEntry, newEntryOffset, nextEntry.next, 
			curEntry->next) < 0)
	{
		putErrmsg("Update of .lst file failed", NULL);
		return -1;
	}

	if (updateEntry(fileD, curEntry, curEntry->prev, newEntryOffset, 
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

int	readRecord(int fileD, dataRecord *rec, off_t datOffset)
{
	if ((lseek(fileD, datOffset, SEEK_SET) < 0) ||
	    (read(fileD, rec, sizeof(dataRecord)) < 0))
	{
		putSysErrmsg("BSS library: can't seek to read from .dat file",
				NULL);
		return -1;
	}

	return 0;
}

int	readPayload(int fileD, char* buffer, long length)
{
	if (read(fileD, buffer, length*sizeof(char)) < 0)
	{
		putSysErrmsg("BSS library: can't read payload from .dat file", 
				NULL);
		return -1;
	}

	return 0;
}

static long	addDataRecord(int fileD, off_t datOffset, BpTimestamp time,
			long payloadLength)
{
	dataRecord data;

	data.crtnTime = time;
	data.pLen = payloadLength;

	if (write(fileD, &data, sizeof(dataRecord)) < 0)
	{
		putSysErrmsg("BSS library: can't write to .dat file", NULL);
		return -1;
	}
#if BSSLIBDEBUG
printf("New data record added to the database\n");
printf("-------------------------------------\n");
printf("creation time: %lu - length: %ld\n", time.seconds, payloadLength);
#endif
	return 0;		
}

/* Receiver's operations - section */

static int	updateLstEntries(int lstFile, long lstEntryOffset,
			long newEntryOffset, off_t datOffset, BpTimestamp time,
			long dataLength)
{
	lstEntry 	curEntry;
	
	/*
	 *  This function adds new entries and applies appropriate
	 *  updates on the contents of *.lst file, as needed. The
	 *  operation of the function depends on the value of
	 *  lstEntryOffset.
	 *
	 *  If lstEntryOffset is 1 then it can't be a legitimate
	 *  list entry offset, since the size of a list entry is
	 *  greater than 1.  (The first entry in the .lst file is
	 *  at offset zero, the second is at offset N where N is the
	 *  size of a list entry, etc.)  Therefore this value of
	 *  lstEntryOffset is used to indicate that the linked list
	 *  of entries for the indicated time is currently empty.
	 *  A new entry that isn't connected to any other entries
	 *  is created and appended at the end of the file.
	 *
	 *  If lstEntryOffset has a value other than 1, that means
	 *  that the linked list of entries for the indicated time
	 *  is not empty.  A new entry with the proper contents is
	 *  created and appended to the end of the file and the
	 *  pointer to lstEntryOffset is used as a starting point
	 *  for stepping through the other entries of that doubly-
	 *  linked list so that they can get updated (based on their
	 *  count value), placing the new entry at the right spot.	       
	 */

	if (lstEntryOffset == 1)
	{
		/*	Doubly-linked list is empty.	*/
		if (addEntry(lstFile, time, datOffset, -1, -1, dataLength) < 0)
		{
			putErrmsg("Update of .lst file failed", NULL);
			return -1;
		}
#if BSSLIBDEBUG
printf("ADD // new Entry Offset: %ld - previous offset: -1 - next offset: -1\n",
	newEntryOffset);
#endif
		return 1;
	}
	
	if (getLstEntry(lstFile, &curEntry, lstEntryOffset) < 0)
	{
		putErrmsg("Retrieval of .lst entry failed", NULL);
		return -1;
	}
	
	if (curEntry.crtnTime.count < time.count)
	{
		if (insertLstEdge(lstFile, &curEntry, curEntry.prev, 
			newEntryOffset, lstEntryOffset, time, datOffset, 
			lstEntryOffset, -1, dataLength) < 0)
		{
			return -1;
		}
#if BSSLIBDEBUG
printf("Same as end of list. Adding an entry at the end of the list for %lu \
second\n", time.seconds);
printf("UPDATE // offset: %ld - previous offset: %ld - next offset: %ld\n", 
lstEntryOffset, curEntry.prev, newEntryOffset);
printf("ADD // new Entry Offset: %ld - previous offset: %ld - next offset: -1\n", newEntryOffset, lstEntryOffset);
#endif
		return 1;
	}
	else if (curEntry.crtnTime.count == time.count)
	{
		return 0;		/*	Avoid duplicate entry.	*/
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
		else if (curEntry.crtnTime.count == time.count)
		{
			return 0;	/*	Avoid duplicate entry.	*/
		}
	}

	if (curEntry.crtnTime.count == time.count)
	{
		return 0;		/*	Avoid duplicate entry.	*/
	}

	if (insertLstEdge(lstFile, &curEntry, newEntryOffset, 
			curEntry.next, lstEntryOffset, time, datOffset, 
			-1, lstEntryOffset, dataLength) < 0)
	{
		return -1;
	}
#if BSSLIBDEBUG
printf("Adding an entry at the beginning of the list for %lu second\n", 
	time.seconds);
printf("UPDATE // offset: %ld - previous offset: %ld - next offset: %ld\n", 
	lstEntryOffset, newEntryOffset,  curEntry.next);
printf("ADD // Entry Offset: %ld - previous offset: -1 - next offset: %ld\n", 
	newEntryOffset, lstEntryOffset);
#endif
	return 1;
}

static void	updateTbl(tblRow *row, int _switch, long offset,
			BpTimestamp time)
{
	/*  
	 *  This function updates the values of tblIndex structure based      
	 *  on _switch value. In the first case, it updates the variables     
	 *  that hold the details for the bundle with the oldest creation     
	 *  time of a particular second. In the second case it updates the    
	 *  variables that hold the details for the bundle with the newest    
	 *  creation time of a particular second. Finally, in the third case, 
	 *  the bundle that just got received is the first received bundle  
	 *  of a particular second, so both variables that hold the details    
	 *  for the oldest and newest bundle are being initialized with the   
	 *  same value.		     			               
	 */

	if (_switch == 0)
	{
		row->firstEntryOffset = offset;
		row->lwstCountVal = time.count;
	}
	else if (_switch == 1)
	{
		row->lastEntryOffset = offset;
		row->hgstCountVal = time.count;
	}
	else if (_switch == 2)
	{
		row->firstEntryOffset = offset;
		row->lwstCountVal = time.count;
		row->lastEntryOffset = offset;
		row->hgstCountVal = time.count;
	}
}

static long	getEntryPosition(int tblFile, BpTimestamp time, long entry)
{
	tblIndex	*index = _tblIndex(NULL);
	tblHeader	*hdr = &(index->header);
	tblRow		*row;
	long		elapsed;
	long		relativeTime;
	long		position;
	long		offset;
	long		newestRow = -1;
	time_t		newestTime = 0;
	long		oldestRow = -1;
	time_t		oldestTime = 0;
	long		lastEntry; 	/*	offset of last in list	*/

	/* 
	 *  This function calculates in which position (row number),  
	 *  of the lists stored in .tbl, the new entry should be     
	 *  inserted, updates the proper elements as needed and returns
	 *  last entry's offset in .lst file for the indicated second.         
	 */

#if BSSLIBDEBUG
printf("(BEFORE) Creation time of the oldest frame stored in *.tbl file: %lu\n",
	hdr->oldestTime);
printf("(BEFORE) Creation time of the newest frame stored in *.tbl file: %lu\n",
	hdr->newestTime);
#endif

	/*	In case getEntryPosition function is executed for
	 *	the first time, initialize the values of oldestTime
	 *	and newestTime in the index file.			*/

	if (hdr->oldestTime == 0)
	{
		hdr->oldestTime = time.seconds;
		if (writeTblFile(tblFile, 0, (char *) index, sizeof(tblHeader))
				< 0)
		{
			return -1;
		}
	}

	if (hdr->newestTime == 0)
	{
		hdr->newestTime = time.seconds;
		if (writeTblFile(tblFile, 0, (char *) index, sizeof(tblHeader))
				< 0)
		{
			return -1;
		}
	}

	/*	Determine correct .tbl row for this bundle.		*/

	if (time.seconds < hdr->oldestTime)
	{
		/* Bundle is too old. A dataRecord will be inserted to
		 * *.dat file but it will not be trackable by *.tbl or
		 * *.lst files.			    			*/

		return -2; 
	}

#if BSSLIBDEBUG
printf("(before calculation) position value: %ld - NewestRowIndex value %ld\n",
position, hdr->newestRowIndex);
#endif

	if (time.seconds <= hdr->newestTime)	/*	Data from past.	*/
	{
		/*	No change to either oldest or newest time.	*/

		relativeTime = time.seconds - hdr->oldestTime;
		position = hdr->oldestRowIndex + relativeTime;
		if (position >= WINDOW)
		{
			position -= WINDOW;
		}
	}
	else	/*	Time has advanced.				*/
	{
		elapsed = time.seconds - hdr->newestTime;
		if (elapsed >= WINDOW)
		{
			/*	The time advance is too large to be
			 *	contained in the database's index,
			 *	so we wipe out the whole index and
			 *	start over again at this time.		*/

			initializeTblIndex(tblFile, index);
			position = 0;
			newestRow = 0;
			newestTime = time.seconds;
			oldestRow = 0;
			oldestTime = time.seconds;
		}
		else	/*	Time advance fits within index.		*/
		{
			position = hdr->newestRowIndex + 1;
			oldestRow = hdr->oldestRowIndex;
			oldestTime = hdr->oldestTime;

			/*	Must clear out old data for any rows
			 *	(seconds) between the previous newest
			 *	time and the time to which we have
			 *	now advanced.  If the time advance is
			 *	just one second, no need to erase
			 *	anything -- we're going to write into
			 *	the first row after the previous newest
			 *	time anyway.				*/

			while (1)
			{
				if (position == WINDOW)
				{
					position = 0;
				}

				/*	May be overwriting the oldest
				 *	row in the table.		*/

				if (position == oldestRow)
				{
					oldestTime++;
					oldestRow++;
					if (oldestRow == WINDOW)
					{
						oldestRow = 0;
					}
				}

				/*	Erase this intervening second.	*/

				row = index->rows + position;
				row->firstEntryOffset = -1;
				row->lastEntryOffset = -1;
				row->hgstCountVal = 0;
				row->lwstCountVal = 0;
				offset = ((char *) row) - ((char *) index);
				if (writeTblFile(tblFile, offset, (char *) row,
						sizeof(tblRow)) < 0)
				{
					return -1;
				}
#if BSSLIBDEBUG
printf("Row (%lu) was erased\n", position);
#endif
				elapsed -= 1;
				if (elapsed == 0)
				{
					break;	/*	Done erasing.	*/
				}

				position++;
			}

			newestRow = position;
			newestTime = time.seconds;
		}
	}

#if BSSLIBDEBUG
printf("(after calculation) position value: %ld - NewestRowIndex value %ld\n",
position, hdr->newestRowIndex);
#endif
	/*
	 *  Checks if there is already a doubly-linked list created
	 *  for this second. If a doubly-linked list already exists,
	 *  returns the position (offset) of the last entry in the
	 *  doubly-linked list and updates accordingly the .tbl file.
	 *  Otherwise, returns a zero value to indicate that a doubly-
	 *  linked list for this second doesn't exist and updates
	 *  accordingly the .tbl file.
	 */

	row = index->rows + position;
	if (row->lastEntryOffset == -1)
	{
		lastEntry = 1;	 /*	no entries for this second yet	*/
		updateTbl(row, 2, entry, time);	
	}
	else
	{
		lastEntry = row->lastEntryOffset;
		if (time.count < row->lwstCountVal)
		{
			updateTbl(row, 0, entry, time);
		}
		else if (time.count > row->hgstCountVal)
		{
			updateTbl(row, 1, entry, time);
		}
	}

	offset = ((char *) row) - ((char *) index);
	if (writeTblFile(tblFile, offset, (char *) row, sizeof(tblRow)) < 0)
	{
		return -1;
	}

	/*
	 *  If either newest or oldest row values got updated during
	 *  the execution of this function, update index structure
	 *  values in order to save changes later.
	 */

	if (oldestRow != -1)
	{
		hdr->oldestTime = oldestTime;
		hdr->oldestRowIndex = oldestRow;
	}
	
	if (newestRow != -1)
	{
		hdr->newestTime = newestTime;
		hdr->newestRowIndex = newestRow;
	}

	if (writeTblFile(tblFile, 0, (char *) index, sizeof(tblHeader)) < 0)
	{
		return -1;
	}

#if BSSLIBDEBUG
printf("(AFTER) Creation time of the oldest frame stored in *.tbl file: %lu\n", 
	hdr->oldestTime);
printf("(AFTER) Creation time of the newest frame stored in *.tbl file: %lu\n", 
	hdr->newestTime);
printf("firstEntryOffset: %lu - lastEntryOffset: %lu  returned value: %ld\n", 
index->rows[position].firstEntryOffset, index->rows[position].lastEntryOffset, lastEntry);
#endif

	return lastEntry;
}

static int	receiveFrame(Sdr sdr, BpDelivery *dlv, int datFile, int lstFile,
			int tblFile, BpTimestamp *lastDis, char *buffer, 
			long bufLength, RTBHandler display)
{
	ZcoReader       reader;
	long		contentLength;
	char		error[3] = "-1";
	off_t		datOffset;
	long 		newEntryOffset;
	long		lstEntryOffset;
	int		updateStat;
	int		res = 0;

	memset(buffer, '\0', bufLength);
	contentLength = (long) zco_source_data_length(sdr, dlv->adu);
	if (contentLength <= bufLength)
	{
		CHKERR(sdr_begin_xn(sdr));
		zco_start_receiving(dlv->adu, &reader);
		if (zco_receive_source(sdr, &reader, bufLength, buffer) < 0)
		{
			sdr_cancel_xn(sdr);
			putErrmsg("bss: can't receive bundle content.", NULL);
			return -1;
		}

		if (sdr_end_xn(sdr) < 0)
		{
			putErrmsg("Can't handle delivery.", NULL);
			return -1;
		}

		if (_lockMutex(1) == -1)
		{
			return -1;
		}

		/*	Locate the offset within lstFile at which the
		 *	new entry will be inserted.			*/

		newEntryOffset = (long) lseek(lstFile, 0, SEEK_END);
		if (newEntryOffset < 0)
		{
			putSysErrmsg("BSS library can't seek to the end of \
					.lst file", NULL);
			oK(_lockMutex(0));
			return -1;
		}
#if BSSRECVLIBDEBUG
printf("new Entry Offset returned from seeking to the end of .lst file: %ld\n", 
newEntryOffset);
#endif
		/*	Locate the offset within datFile at which the
		 *	new data record will be inserted.		*/

		datOffset = (off_t) lseek(datFile, 0, SEEK_END);
		if (datOffset < 0)
		{
			putSysErrmsg("BSS library can't seek to the end of \
					.dat file", NULL);
			oK(_lockMutex(0));
			return -1;
		}
	
		/* 
		 *  Find offset of last entry in the doubly-linked list
		 *  that  holds the entries for the second on which this
		 *  bundle was created. 
		 */
		lstEntryOffset = getEntryPosition(tblFile,
				dlv->bundleCreationTime, newEntryOffset);
		switch (lstEntryOffset)
		{
		case -1:
			oK(_lockMutex(0));
			return -1;	/*	Unresolved error.	*/

		case -2:
			break; /* 
				*  Bundle is too old. It will be 
				*  discarded from .lst file but its 
 				*  payload will be saved for later 
				*  processing in .dat file.			
				*/
		default:
			updateStat = updateLstEntries(lstFile, lstEntryOffset, 
						newEntryOffset, datOffset, 
						dlv->bundleCreationTime, 
						contentLength);
			switch (updateStat)
			{
			case -1:
				oK(_lockMutex(0));
				return -1;	/*	Unsuccessful.	*/

			case 0:
				/*	Already received.  Discard.	*/

				oK(_lockMutex(0));
				return 0;

			default:
				break;
			}
		}
#if BSSRECVLIBDEBUG
printf(".lst file Entry Offset that was returned from getEntryPosition function: \
	%ld\n", lstEntryOffset);
#endif
	
		/*	OK to write dataRecord information and payload
		 *	into .dat file.					*/

		if (addDataRecord(datFile, datOffset, dlv->bundleCreationTime, 
				contentLength) < 0
		|| write(datFile, buffer, contentLength) < 0)
		{
			putSysErrmsg("bss: can't write to .dat file", NULL);
			oK(_lockMutex(0));
			return -1;	/*	Unresolved error.	*/
		}

		oK(_lockMutex(0));	/*	Unlock mutex 		*/
		
#if BSSLIBDEBUG
printf("from this point on, the execution of the provided display function begins\n");
#endif
		/*	Display function is called only if the current
		 *	frame has a creation time greater than the last
		 *	displayed frame.  				*/ 

		if (dlv->bundleCreationTime.seconds > lastDis->seconds)
		{
			res = display(dlv->bundleCreationTime.seconds, 
				dlv->bundleCreationTime.count, buffer, 
				contentLength);
			*lastDis = dlv->bundleCreationTime;
		}
		else if (dlv->bundleCreationTime.seconds == lastDis->seconds)
		{
			if (dlv->bundleCreationTime.count > lastDis->count)
			{
				res = display(dlv->bundleCreationTime.seconds,
					dlv->bundleCreationTime.count, buffer, 
					contentLength);
				*lastDis = dlv->bundleCreationTime;
			}
		}
	}
	else
	{
		res = display((time_t) 0, 0, error, sizeof(error));
	}

	return res;
}

void	*recvBundles(void *args)
{
	int		stopLoop = 0;
	BpSAP		sap;
	Sdr		sdr;
	BpDelivery	dlv;
	bss_thread_data	*db;
	BpTimestamp	lastDis;
	
	/*	Initialize the variable that holds the time of the 
	 *	last displayed frame from receiving thread.		*/	

	lastDis.seconds = 0;
	lastDis.count = 0;

   	db = (bss_thread_data *) args;

	if (bp_attach() < 0)
	{
		putErrmsg("Can't attach to BP.", NULL);
		close(db->dat);
		close(db->lst);
		close(db->tbl);
		oK(_recvThreadId(NULL, -1));
		pthread_exit(NULL);
	}

	if (bp_open(db->eid, &sap) < 0)
	{
		putErrmsg("Can't open own endpoint.", db->eid);
		close(db->dat);
		close(db->lst);
		close(db->tbl);
		oK(_recvThreadId(NULL, -1));
		pthread_exit(NULL);
	}

	oK(_bpsap(&sap));
	sdr = bp_get_sdr();
	writeMemo("[i] bss reception thread is running.");
	while (_running(NULL))
	{
		if (bp_receive(sap, &dlv, BP_BLOCKING) < 0)
		{
			putErrmsg("bss bundle reception failed.", NULL);
			oK(_running(&stopLoop));
			continue;
		}

		switch (dlv.result)
		{
		case BpEndpointStopped:
			oK(_running(&stopLoop));
			break;		/*	Out of switch.		*/

		case BpPayloadPresent:
			if (receiveFrame(sdr, &dlv, db->dat, db->lst, db->tbl,
				&lastDis, db->buffer, db->bufLength, 
				db->function) < 0)
			{
				putErrmsg("bss failed.", NULL);
				oK(_running(&stopLoop));
			}

			/*	Intentional fall-through to default.	*/

		default:
			break;		/*	Out of switch.		*/
		}

		bp_release_delivery(&dlv, 1);
	}

	/*	Close the files that recv thread had opened and had
	 *	rd/rw access.						*/

	close(db->dat);
	close(db->lst);
	close(db->tbl);
	bp_close(sap);
	writeErrmsgMemos();
	writeMemo("[i] Stopping bss reception thread.");
	bp_detach();
	oK(_recvThreadId(NULL, -1));
	return NULL;
}

/*	Create/Load database - section		*/

static int	checkDb(int dat, int lst, int tbl)
{	
	dataRecord 	data;
	lstEntry 	entry;
	tblIndex 	*index = _tblIndex(NULL);
	tblHeader	*hdr = &(index->header);
	char*		payload;
	int		result;

	if (_lockMutex(1) == -1)
	{
		return -1;
	}
	
	if (hdr->newestRowIndex != 0)
	{
		if (getLstEntry(lst, &entry,
			index->rows[hdr->oldestRowIndex].firstEntryOffset) < 0)
		{
			oK(_lockMutex(0));
			return -1;
		}

		if (readRecord(dat, &data, entry.datOffset) < 0)
		{
			oK(_lockMutex(0));
			return -1;
		}
			
		payload = MTAKE(data.pLen);
		if (payload == NULL)
		{
			oK(_lockMutex(0));
			return -2;
		}

		result = readPayload(dat, payload, data.pLen);
		MRELEASE(payload);			
		if (result < 0)
		{
			oK(_lockMutex(0));
			return -1;	/*	Unresolved error.	*/
		}

		if ((hdr->oldestTime != entry.crtnTime.seconds)
		|| (entry.crtnTime.seconds != data.crtnTime.seconds)
		|| (entry.crtnTime.count != data.crtnTime.count))
		{
			/*	Database corruption detected.		*/

			oK(_lockMutex(0));
			return -1;
		}
	}

	oK(_lockMutex(0));
	return 0;
}

int	loadRDWRDB(char* bssName, char* path, int* dat, int* lst, int* tbl)
{
	char 		fileName[255];
	long		tblSize;
	int		create = 1;
	int		destroy = 0;
	tblIndex	*index;

	isprintf(fileName, sizeof(fileName), "%s/%s.dat", path, bssName);
	*dat = open(fileName, O_RDWR | O_CREAT | O_LARGEFILE, 0666);
	if (*dat < 0)
	{
		putSysErrmsg("BSS Library: can't open .dat file", fileName);
		return -1;
	}

	isprintf(fileName, sizeof(fileName), "%s/%s.lst", path, bssName);
	*lst = open(fileName, O_RDWR | O_CREAT, 0666);
	if (*lst < 0)
	{
		putSysErrmsg("BSS Library: can't open .lst file", fileName);
		close(*dat);
		return -1;
	}

	isprintf(fileName, sizeof(fileName), "%s/%s.tbl", path, bssName);
	*tbl = open(fileName, O_RDWR | O_CREAT, 0666);
	if (*tbl < 0)
	{
		putSysErrmsg("BSS Library: can't open .tbl file", fileName);
		close(*lst);
		close(*dat);
		return -1;
	}
	
	/*	Checks if DB already exists. If not, initializes the
	 *	*.tbl file contents.					*/

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

	index = _tblIndex(&create);
	if (index == NULL)
	{
		putErrmsg("BSS library: can't create table index image.", NULL);
		close(*dat);
		close(*lst);
		close(*tbl);
		return -1;
	}

	if (tblSize == 0)
	{
		if (initializeTblIndex(*tbl, index) < 0)
		{
			oK(_tblIndex(&destroy));
			close(*dat);
			close(*lst);
			close(*tbl);
			return -1;
		}
	}
	else
	{
		if (loadTblIndex(*tbl, index) < 0)
		{
			oK(_tblIndex(&destroy));
			close(*dat);
			close(*lst);
			close(*tbl);
			return -1;
		}
	}

	/*	Database's integrity check				*/

	if (checkDb(*dat, *lst, *tbl) == -1)
	{
		putErrmsg("Database is corrupted. Use recovery mode.", NULL);
		oK(_tblIndex(&destroy));
		close(*dat);
		close(*lst);
		close(*tbl);
		return -1;
	}

	return 0;
}

int	loadRDonlyDB(char* bssName, char* path)
{
	char		fileName[255];
	int		datRO;
	int		lstRO;
	int		tblRO;
	tblIndex	*index;
	int		create = 1;
	int		destroy = 0;

	isprintf(fileName, sizeof(fileName), "%s/%s.dat", path, bssName);
	datRO = open(fileName, O_RDONLY | O_LARGEFILE, 0666);
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
		close(datRO);
		return -1;
	}

	isprintf(fileName, sizeof(fileName), "%s/%s.tbl", path, bssName);
	tblRO = open(fileName, O_RDONLY, 0666);
	if (tblRO < 0)
	{
		putSysErrmsg("BSS Library: can't open .tbl file", fileName);
		close(lstRO);
		close(datRO);
		return -1;
	}

	oK(_datFile(1, datRO));
	oK(_lstFile(1, lstRO));
	oK(_tblFile(1, tblRO));
	index = _tblIndex(&create);
	if (index == NULL)
	{
		putErrmsg("BSS library: can't create table index image.", NULL);
		oK(_datFile(-1,0));
		oK(_lstFile(-1,0));
		oK(_tblFile(-1,0));
		return -1;
	}

	if (loadTblIndex(tblRO, index) < 0)
	{
		oK(_tblIndex(&destroy));
		return -1;
	}
	
	/*	Database's integrity check	*/

	if (checkDb(_datFile(0,0), _lstFile(0,0), _tblFile(0,0)) == -1)
	{
		putErrmsg("Database is corrupted. Use recovery mode.", NULL);
		oK(_tblIndex(&destroy));
		return -1;
	}

	return 0;
}

/*	Database navigation - section	*/

void	findIndexRow(time_t time, long *position)
{
	/*
	 *  This function locates the first active second in the database
	 *  that is on or after the specified time, placing that second's
	 *  row number in *position.  If no such second exists in the database,
	 *  *position is set to -1.
	 */

	tblIndex	*index = _tblIndex(NULL);
	tblHeader	*hdr;
	int		i;

	CHKVOID(position);
	CHKVOID(index);
	hdr = &(index->header);
	if (hdr->oldestTime == 0	/*	Empty database.		*/
	|| time > hdr->newestTime)	/*	Hasn't happened yet.	*/
	{
		*position = -1;
		return;			/*	No such entry.		*/
	}

	/*	Find earliest second on or after the specified time.	*/

	if (time < hdr->oldestTime)
	{
		i = hdr->oldestRowIndex;
	}
	else
	{
		i = hdr->oldestRowIndex + (time - hdr->oldestTime);
		if (i >= WINDOW)		/*	Wrap around.	*/
		{
			i -= WINDOW;
		}
	}

	/*	Search for a second with some activity.			*/

	while (1)
	{
		if (index->rows[i].firstEntryOffset < 0)
		{
			/*	Inactive.  Try next second, if any.	*/

			if (i == hdr->newestRowIndex)
			{
				*position = -1;
				return;		/*	No such entry.	*/
			}

			i++;
			if (i == WINDOW)	/*	Wrap around.	*/
			{
				i = 0;
			}

			continue;
		}

		break;				/*	Found it.	*/
	}
#if BSSLIBDEBUG
printf("oldestTime: %lu >= time: %lu\n", hdr->oldestTime + i, time);
#endif
	*position = i;
}
