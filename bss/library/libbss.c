/*
 *	libbss.c:	BSS API, functions enabling the implementation 
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

void	bssStop()
{
	int		destroy = 0;
	int		stopLoop = 0;
	pthread_t	stopThread = 0;
	pthread_t	recvThread;

	if (_datFile(0, 0) == -1)	/*	Must destroy tblIndex.	*/
	{
		oK(_tblIndex(&destroy));
		ionDetach();
	}

	recvThread = _recvThreadId(NULL);
	oK(_running(&stopLoop));
	if (recvThread != 0)
	{
		bp_interrupt(_bpsap(NULL));
		oK(_recvThreadId(&stopThread));
		oK(pthread_join(recvThread, NULL));
	}
	else
	{
		PUTS("No active thread detected");
		return;
	}

	PUTS("BSS receiving thread has been stopped");
}

void	bssClose()
{
	int	destroy = 0;

	if (_recvThreadId(NULL) == 0)	/*	Must destroy tblIndex.	*/
	{
		oK(_tblIndex(&destroy));
		ionDetach();
	}

	if (_datFile(0,0) != -1)
	{
		close(_datFile(-1,0));
		close(_lstFile(-1,0));
		close(_tblFile(-1,0));
	}
	else
	{
		PUTS("No BSS database RONLY files are opened");
		return;
	}

	PUTS("BSS database RONLY files were successfully closed");
}

void	bssExit()
{
	PUTS("BSS is exiting...");
	bssStop();
	bssClose();
}

/* Start and terminate BSS receiver operations */
int	bssOpen(char* bssName, char* path)
{
	CHKERR(bssName);
	CHKERR(path); 
	/*
	 *  This function loads BSS receiver's database with	
	 *  RDONLY access rights that is necessary for stream's 	
	 *  playback functionality.				
	 */

	ionAttach();
	if (_datFile(0,0) == -1 && _lstFile(0,0) == -1 && _tblFile(0,0) == -1)
	{
		if (loadRDonlyDB(bssName, path)!=0)
		{	
			putErrmsg("BSS library: Database creation failed.", 
				   path);
			bssClose();
			ionDetach();
			return -1;
		}
	}
	else
	{
		PUTS("An active playback session was detected.  If you \
wish to initiate a new one, please first close the active playback session.");
		ionDetach();
		return -1;
	}
	
	return 0;
}

int	bssStart(char* bssName, char* path, char* eid, char* buffer,
		int bufLength, RTBHandler display)
{	
	int 			dat;
	int			lst;
	int 			tbl;
	static bss_thread_data	DB;
 	pthread_t    		bssRecvThread;
	int			enableLoop = 1;

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

	ionAttach();
	if (_recvThreadId(NULL) == 0)
	{
		if (loadRDWRDB(bssName, path, &dat, &lst, &tbl) != 0)
		{	
			putErrmsg("BSS library: Database creation failed.", 
				   path);
			bssStop();
			ionDetach();
			return -1;
		}
	}
	else
	{
		PUTS("Please terminate the already active real-time \
session in order to initiate a new one.");
		ionDetach();
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
		ionDetach();
		return -1;
	}

	oK(_recvThreadId(&bssRecvThread));
	return 0;
}

int	bssRun(char* bssName, char* path, char* eid, char* buffer,
		int bufLength, RTBHandler display)
{
	if (_datFile(0,0) == -1 && _lstFile(0,0) == -1 && _tblFile(0,0) == -1 
		&& _recvThreadId(NULL) == 0)
	{
		if (bssStart(bssName, path, eid, buffer, bufLength, display)
				< 0)
		{
			return -1;
		}
		
		if (bssOpen(bssName, path) < 0)
		{
			return -1;
		}
	}
	else
	{
		PUTS("A real-time and/or a playback session is/are already \
active.  Please terminate them in order to initiate a new one.");
		return -1;
	}

	return 0;
}

long	bssRead(bssNav nav, char* data, int dataLen)
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

static void	updateNavInfo(bssNav *nav, int position, long datOffset,
			long prev, long next)
{
	nav->curPosition = position;
	nav->datOffset = datOffset;
	nav->prevOffset = prev;
	nav->nextOffset = next;
}

long	 bssSeek(bssNav *nav, time_t time, time_t *curTime,
		unsigned long *count)
{
	tblIndex 	*index = _tblIndex(NULL);
	long 		position;
	long 		lstEntryOffset;
	lstEntry 	entry;

	CHKERR(nav);
	CHKERR(time > 0);
	CHKERR(curTime); 
	CHKERR(count);
	CHKERR(index);
	
	if (_lockMutex(1) == -1)	/*	Protecting transaction.	*/
	{
		return -1;
	}

	findIndexRow(time, &position);
	if (position == -1)
	{
		PUTS("Cannot seek to the specified time. No match was found");
		_lockMutex(0);
		return -1;
	}

	lstEntryOffset = index->rows[position].firstEntryOffset;
	if (getLstEntry(_lstFile(0,0), &entry, lstEntryOffset) == -1)
	{
		oK(_lockMutex(0));
		return -1;
	}
	
	updateNavInfo(nav, position, entry.datOffset, entry.prev, entry.next);
	*curTime = (time_t) entry.crtnTime.seconds;
	*count = entry.crtnTime.count;
	oK(_lockMutex(0));
		
	return entry.pLen;
}

long	 bssSeek_read(bssNav *nav, time_t time, time_t *curTime,
		unsigned long *count, char* data, int dataLen)
{
	long	pLen;

	pLen = bssSeek(nav, time, curTime, count);
	if (pLen < 0)
	{
		return -1;
	}

	pLen = bssRead(*nav, data, dataLen);
	if (pLen < 0)
	{
		return -1;
	}

	return pLen;
}

long	bssNext(bssNav *nav, time_t *curTime, unsigned long *count)
{
	tblIndex 	*index = _tblIndex(NULL);
	tblHeader	*hdr;
	tblRow		*row;
	lstEntry 	entry;
	int		curPosition = nav->curPosition;
	int 		i = 0;
	unsigned long	nextTime;

	CHKERR(nav);
	CHKERR(curTime); 
	CHKERR(count);
	CHKERR(index);
	hdr = &(index->header);

	if (_lockMutex(1) == -1)	/*	Protecting transaction.	*/
	{
		return -1;
	}

	if (nav->nextOffset == -1) 
	{
		/*	The end of the current doubly-linked list was
		 *	reached.
		 *
		 *	The following check ensures that bssNext 
		 *	 function will either break or return.		*/

		if (hdr->oldestTime == 0)
		{
			oK(_lockMutex(0));
			return -1;
		}

		/* 	Move to the next position 			*/
		curPosition = curPosition + 1;

		while (i < WINDOW)
		{
			/*	 
			 *  Each row of the tables stored in .tbl file
			 *  represents a specific second (from the last
			 *  WINDOW seconds stored).  The curPosition value
			 *  refers to these rows.  The position in
			 *  which the information for each second is
			 *  stored is relative to the value of the
			 *  second stored in the hdr->oldestRowIndex
			 *  position. The time (in seconds) represented
			 *  by the curPosition is calculated by
			 *  comparing the curPosition with the 
			 *  hdr->oldestRowIndex value.
			 */

			if (curPosition >= hdr->oldestRowIndex)
			{
				nextTime = hdr->oldestTime
					+ (curPosition % WINDOW)
					- hdr->oldestRowIndex;
			}
			else
			{
				nextTime = hdr->oldestTime + WINDOW
					- hdr->oldestRowIndex
					+ (curPosition % WINDOW);	
			}
			
			if (nextTime <= *curTime)
			{
				oK(_lockMutex(0));
				return -2;	/* 	end of list	*/
			}

			row = index->rows + (curPosition % WINDOW);
			if (row->firstEntryOffset == -1)
			{
				/*  
				 *  If there is no doubly-linked list
				 *  created for that particular second,
				 *  move to the next position. 
				 */
				curPosition = curPosition + 1;
			}
			else
			{
				curPosition = curPosition % WINDOW;
				break;
			}
			
			i++;
		}

		row = index->rows + (curPosition % WINDOW);
		if (getLstEntry(_lstFile(0,0), &entry, row->firstEntryOffset)
				== -1)
		{
			oK(_lockMutex(0));
			return -1;
		}

		updateNavInfo(nav, curPosition % WINDOW, entry.datOffset,
				entry.prev, entry.next);
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
		
	*curTime = (time_t) entry.crtnTime.seconds;
	*count = entry.crtnTime.count;

	oK(_lockMutex(0));

	return entry.pLen;
}

long	bssNext_read(bssNav *nav, time_t *curTime, unsigned long *count,
		char* data, int dataLen)
{
	long	pLen;

	pLen = bssNext(nav, curTime, count); 
	if (pLen == -2)
	{
		return -2;	/*	Indicates end of list.		*/
	}
	else if (pLen < 0)
	{
		return -1;
	}

	pLen = bssRead(*nav, data, dataLen);
	if (pLen < 0)
	{
		return -1;
	}

	return pLen;
}

long	bssPrev(bssNav *nav, time_t *curTime, unsigned long *count)
{
	tblIndex 	*index = _tblIndex(NULL);
	tblHeader	*hdr;
	tblRow		*row;
	lstEntry 	entry;
	int		curPosition = nav->curPosition;
	int 		i=0;
	unsigned long	prevTime;
	
	CHKERR(nav);
	CHKERR(curTime); 
	CHKERR(count);
	CHKERR(index);
	hdr = &(index->header);

	if (_lockMutex(1) == -1)	/*	Protecting transaction.	*/
	{
		return -1;
	}

	if (nav->prevOffset == -1) 
	{
		/*	The following check ensures that bssPrev  	*
		 *	 function will either break or return.		*/

		if (hdr->oldestTime == 0)
		{
			oK(_lockMutex(0));
			return -1;
		}

		curPosition = curPosition - 1;
		if (curPosition < 0) curPosition = -1 * curPosition;

		while (i < WINDOW)
		{
			/*
		 	 *  The time (in seconds) represented by the
			 *  curPosition is calculated by comparing
			 *  the curPosition with the 
			 *  hdr->oldestRowIndex value.
			 */

			if (curPosition >= hdr->oldestRowIndex)
			{
				prevTime = hdr->oldestTime
					+ (curPosition % WINDOW) 
					- hdr->oldestRowIndex;
			}
			else
			{
				prevTime = hdr->oldestTime + WINDOW 
					- hdr->oldestRowIndex
					+ (curPosition % WINDOW);	
			}

			if (prevTime >= (unsigned long) *curTime)
			{
				oK(_lockMutex(0));
				return -2;	/* 	end of list	*/
			}

			row = index->rows + (curPosition % WINDOW);
			if (row->firstEntryOffset == -1)
			{
				/*  
				 *  If there is no doubly-linked list
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
				curPosition = curPosition % WINDOW;
				break;
			}

			i++;
		}

		row = index->rows + (curPosition % WINDOW);
		if (getLstEntry(_lstFile(0,0), &entry, row->lastEntryOffset)
				== -1)
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

	*curTime = (time_t) entry.crtnTime.seconds;
	*count = entry.crtnTime.count;
	oK(_lockMutex(0));
	return entry.pLen;
}

long	bssPrev_read(bssNav *nav, time_t *curTime, unsigned long *count,
		char* data, int dataLen)
{
	long	pLen;

	pLen = bssPrev(nav, curTime, count); 
	if (pLen == -2)
	{
		return -2;	/*	Indicates start of list.	*/
	}
	else if (pLen < 0)
	{
		return -1;
	}

	pLen = bssRead(*nav, data, dataLen);
	if (pLen < 0)
	{
		return -1;
	}

	return pLen;
}
