/*
 *	libbssrecv.h:	BSS API, functions enabling the implementation 
 *			of BSS applications that support the reception
 *			of a bundle stream and are able to provide
 *			playback control over the stream.
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

bss_thread_data DB;

void bssStop()
{
	pthread_t	stopThread=-1;
	int		stopLoop = 0;

	_running(&stopLoop);

	if (_recvThreadId(NULL) != -1)
	{
		bp_interrupt(_bpsap(NULL));
		pthread_join(_recvThreadId(NULL), NULL);
		_recvThreadId(&stopThread);
	}
	else
	{
		PUTS("No active thread detected");
		return ;
	}

	PUTS("BSS receiving thread has been stopped");
}

void bssClose()
{
	if (_datFile(0,0) != -1)
	{
		close(_datFile(-1,0));
		close(_lstFile(-1,0));
		close(_tblFile(-1,0));
	}
	else
	{
		PUTS("No BSS database RONLY files are opened");
		return ;
	}

	PUTS("BSS database RONLY files were successfully closed");
}

void bssExit()
{
	bssStop();
	bssClose();
	PUTS("BSS is exiting...");
}

/* Start and terminate BSS receiver operations */
int bssOpen(char* bssName, char* path, char* eid)
{
	CHKERR(bssName);
	CHKERR(path); 
	CHKERR(eid);
	/*
	 *  This function loads BSS receiver's database with	
	 *  RDONLY access rights that is necessary for stream's 	
	 *  playback functionality.				
	 */
	
	if (_datFile(0,0) == -1 && _lstFile(0,0) == -1 && _tblFile(0,0) == -1)
	{
		if (loadRDonlyDB(bssName, path)!=0)
		{	
			putErrmsg("BSS library: Database creation failed.", 
				   path);
			bssClose();
			return -1;
		}
	}
	else
	{
		writeMemo("Please terminate the already active playback \
session in order to initiate a new one.");
		return -1;
	}
	
	return 0;
}

int bssStart(char* bssName, char* path, char* eid, char* buffer, int bufLength,
	RTBHandler display)
{	
	int 		dat;
	int		lst;
	int 		tbl;
 	pthread_t    	bssRecvThread;
	int		enableLoop = 1;

	CHKERR(bssName);
	CHKERR(path); 
	CHKERR(eid); 
	CHKERR(buffer); 
	CHKERR(bufLength > 0); 
	CHKERR(display);
	
	/*
	 *  This function loads BSS receiver's database with       
	 *  read/write access rights that are necessary for the     
	 *  real-time mode.					
         */

	if (_recvThreadId(NULL) == -1)
	{
		if (loadRDWRDB(bssName, path, &dat, &lst, &tbl) != 0)
		{	
			putErrmsg("BSS library: Database creation failed.", 
				   path);
			bssStop();
			return -1;
		}
	}
	else
	{
		writeMemo("Please terminate the already active real-time \
session in order to initiate a new one.");
		return -1;
	}

	istrcpy(DB.eid, eid, sizeof(DB.eid));
	DB.dat = dat;
	DB.lst = lst;
	DB.tbl = tbl;
	DB.buffer = buffer;
	DB.bufLength = bufLength; 
	DB.function = display;

	oK(_running(&enableLoop));

	if (pthread_create(&bssRecvThread, NULL, recvBundles, 
			  (void *) &DB) < 0) 
	{
		putSysErrmsg("Can't create recvBundles thread", NULL);
		bssStop();
		return -1;
	}

	oK(_recvThreadId(&bssRecvThread));

	return 0;
}

int bssRun(char* bssName, char* path, char* eid, char* buffer, int bufLength,
	RTBHandler display)
{
	if (_datFile(0,0) == -1 && _lstFile(0,0) == -1 && _tblFile(0,0) == -1 
		&& _recvThreadId(NULL) == -1)
	{
		if (bssStart(bssName, path, eid, buffer, bufLength, display)
				< 0)
		{
			PUTS("bssStart failed");
			return -1;
		}
		
		if (bssOpen(bssName, path, eid) < 0)
		{
			PUTS("bssOpen failed");
			return -1;
		}
	}
	else
	{
		writeMemo("Please terminate the already active session \
in order to initiate a new one.");
	}

	return 0;
}

int bssRead(bssNav nav, char* data, int dataLen)
{
	dataRecord rec;

	CHKERR(data); 
	CHKERR(dataLen > 0);

	/* 
	 *  This function copies the contents of a data record 
	 *  into a provided external buffer. 			
   	 */

	if (_lockMutex(1) == -1)
	{
		return -1;
	}

	if (readRecord(_datFile(0,0), &rec, nav.datOffset) < 0)
	{
		oK(_lockMutex(0));	/*	unlock mutex	*/
		return -1;
	}

	if (dataLen < rec.pLen)	/*	prevent buffer overflow		*/
	{
		oK(_lockMutex(0));
		return -1;	
	}	

	if (readPayload(_datFile(0,0), data, rec.pLen) < 0)
	{
		_lockMutex(0);
		return -1;
	}
	
	oK(_lockMutex(0));

	return rec.pLen;
}

int bssSeek(bssNav *nav, time_t time, time_t *curTime, unsigned long *count)
{
	tblIndex 	index;
	lstEntry 	entry;
	long 		lstEntryOffset;
	int 		position;

	CHKERR(nav);
	CHKERR(time > 0);
	CHKERR(curTime); 
	CHKERR(count);
	
	if (_lockMutex(1) == -1)	/*	Protecting transaction.	*/
	{
		return -1;
	}

	if (readTblFile(_tblFile(0,0), &index) < 0)
	{
		oK(_lockMutex(0));	/*	unlocking mutex		*/
		return -1;
	}

	if (time < index.oldestTime || index.oldestTime == 0)
	{
		PUTS("Cannot seek to the specified time. Entry does not exist");
		_lockMutex(0);
		return -1;
	}

	position = findLstEntryOffset(index, time, &lstEntryOffset);
	if (position == -1)
	{
		PUTS("Cannot seek to the specified time. No match was found");
		_lockMutex(0);
		return -1;
	}

	if (getLstEntry(_lstFile(0,0), &entry, lstEntryOffset)==-1)
	{
		oK(_lockMutex(0));
		return -1;
	}
	
	updateNavInfo(nav, position, entry.datOffset, entry.prev, entry.next);
	*curTime = entry.crtnTime.seconds;
	*count = entry.crtnTime.count;

	oK(_lockMutex(0));
		
	return entry.pLen;
}

int bssSeek_read(bssNav *nav, time_t time, time_t *curTime,
		unsigned long *count, char* data, int dataLen)
{
	if (bssSeek(nav, time, curTime, count) < 0)
	{
		return -1;
	}

	if (bssRead(*nav, data, dataLen) < 0)
	{
		return -1;
	}

	return strlen(data);
}

int bssNext(bssNav *nav, time_t *curTime, unsigned long *count)
{
	tblIndex 	index;
	lstEntry 	entry;
	int		curPosition = nav->curPosition;
	int 		i=0;
	unsigned long	nextTime;

	CHKERR(nav);
	CHKERR(curTime); 
	CHKERR(count);

	if (_lockMutex(1) == -1)	/*	Protecting transaction.	*/
	{
		return -1;
	}

	if (nav->nextOffset == -1) 
	{
		/* The end of the current doubly-linked list was reached */

		if (readTblFile(_tblFile(0,0),&index) < 0)
		{
			_lockMutex(0); /*	unlocking mutex		*/
			return -1;
		}
		
		/*	The following check ensures that bssNext  	*
		 *	 function will either break or return.		*/

		if (index.oldestTime == 0)
		{
			oK(_lockMutex(0));
			return -1;
		}

		/* 	move to the next position 	*/
		curPosition = curPosition + 1;

		while (i < SOD)
		{
			/*	 
			 *  Each row of the tables stored in .tbl file
			 *  represents a specific second (from the last
			 *  SOD seconds stored).  The curPosition value
			 *  refers to these rows.  The position in
			 *  which the information for each second is
			 *  stored is relative to the value of the
			 *  second stored in the index.oldestRowIndex
			 *  position. The time (in seconds) represented
			 *  by the curPosition is calculated by
			 *  comparing the curPosition with the 
			 *  index.oldestRowIndex value.
			 */

			if (curPosition >= index.oldestRowIndex)
			{
				nextTime = index.oldestTime
					+ (curPosition % SOD)
					- index.oldestRowIndex;
			}
			else
			{
				nextTime = index.oldestTime + SOD
					- index.oldestRowIndex
					+ (curPosition % SOD);	
			}
			
			if (nextTime <= *curTime)
			{
				oK(_lockMutex(0));
				return -2;	/* 	end of list	*/
			}

			if (index.firstEntryOffset[curPosition % SOD] == -1)
			{
				/*  
				 *  if there is no doubly-linked list
				 *  created for that particular second,
				 *  move to the next position. 
				 */
				curPosition = curPosition + 1;
			}
			else
			{
				curPosition = curPosition % SOD;
				break;
			}
			
			i++;
		}

		if (getLstEntry(_lstFile(0,0), &entry, 
			index.firstEntryOffset[curPosition%SOD]) == -1)
		{
			oK(_lockMutex(0));
			return -1;
		}

		updateNavInfo(nav, curPosition%SOD, entry.datOffset, entry.prev,
				entry.next);
	}
	else
	{
		if (getLstEntry(_lstFile(0,0), &entry, nav->nextOffset) == -1)
		{
			oK(_lockMutex(0));
			return -1;
		}
	
		updateNavInfo(nav, curPosition, entry.datOffset, entry.prev, 
			entry.next);
	}
		
	*curTime = entry.crtnTime.seconds;
	*count = entry.crtnTime.count;

	oK(_lockMutex(0));

	return entry.pLen;
}

int bssNext_read(bssNav *nav, time_t *curTime, unsigned long *count, char* data,
	int dataLen)
{
	int checkErr;
	
	checkErr = bssNext(nav, curTime, count); 
	if (checkErr == -2)
	{
		return -2;
	}
	else if (checkErr < 0)
	{
		return -1;
	}

	if (bssRead(*nav, data, dataLen) < 0)
	{
		return -1;
	}

	return strlen(data);
}

int bssPrev(bssNav *nav, time_t *curTime, unsigned long *count)
{
	tblIndex 	index;
	lstEntry 	entry;
	int		curPosition = nav->curPosition;
	int 		i=0;
	unsigned long	prevTime;
	
	CHKERR(nav);
	CHKERR(curTime); 
	CHKERR(count);

	if (_lockMutex(1) == -1)	/*	Protecting transaction.	*/
	{
		return -1;
	}

	if (nav->prevOffset == -1) 
	{
		if (readTblFile(_tblFile(0,0),&index) < 0)
		{
			oK(_lockMutex(0));
			return -1;
		}

		/*	The following check ensures that bssPrev  	*
		 *	 function will either break or return.		*/

		if (index.oldestTime == 0)
		{
			oK(_lockMutex(0));
			return -1;
		}

		curPosition = curPosition - 1;
		if (curPosition < 0) curPosition = -1 * curPosition;

		while (i < SOD)
		{
			/*
		 	 *  The time (in seconds) represented by the
			 *  curPosition is calculated by comparing
			 *  the curPosition with the 
			 *  index.oldestRowIndex value.
			 */

			if (curPosition >= index.oldestRowIndex)
			{
				prevTime = index.oldestTime
					+ (curPosition % SOD) 
					- index.oldestRowIndex;
			}
			else
			{
				prevTime = index.oldestTime + SOD 
					- index.oldestRowIndex
					+ (curPosition % SOD);	
			}

			if (prevTime >= *curTime)
			{
				oK(_lockMutex(0));
				return -2;	/* 	end of list	*/
			}

			if (index.firstEntryOffset[curPosition % SOD] == -1)
			{
				/*  
				 *  if there is no doubly-linked list
				 *  created for that particular second,
				 *  move to the next position. 
				 */
				curPosition = curPosition - 1;
				if (curPosition < 0)
				{
					curPosition = -1 * curPosition;
				}
			}
			else
			{
				curPosition = curPosition % SOD;
				break;
			}

			i++;
		}

		if (getLstEntry(_lstFile(0,0), &entry, 
			index.lastEntryOffset[curPosition % SOD]) == -1)
		{
			oK(_lockMutex(0));
			return -1;
		}
	
		updateNavInfo(nav, curPosition, entry.datOffset, entry.prev, 
			entry.next);
	}
	else
	{
		if (getLstEntry(_lstFile(0,0), &entry, nav->prevOffset) == -1)
		{
			oK(_lockMutex(0));
			return -1;
		}
	
		updateNavInfo(nav, curPosition, entry.datOffset, entry.prev, 
			entry.next);
	}

	*curTime = entry.crtnTime.seconds;
	*count = entry.crtnTime.count;
	oK(_lockMutex(0));
	return 0;
}

int bssPrev_read(bssNav *nav, time_t *curTime, unsigned long *count, char* data,
	int dataLen)
{
	int checkErr;
	
	checkErr = bssPrev(nav, curTime, count); 
	if (checkErr == -2)
	{
		return -2;
	}
	else if (checkErr < 0)
	{
		return -1;
	}

	if(bssRead(*nav, data, dataLen) < 0)
	{
		return -1;
	}

	return strlen(data);
}
