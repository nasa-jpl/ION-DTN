/*
 *	bssP.h:	private definitions supporting the implementation
 *		of BSS receiver operations API.
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

#ifndef _BSSP_H_
#define _BSSP_H_

#include "bss.h"
#include "bp.h"

#ifndef WINDOW
#define	WINDOW	28800	/*	Default window size is eight hours.	*/
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct 
{
	char		eid[32];
	int		dat;
	int		tbl;
	int		lst;
	RTBHandler 	function;
	char*		buffer;
	long		bufLength;
} bss_thread_data;

/* 
 ************************** BSS DATABASE analysis ******************************
 * BSS API operations are supported by the implementation of a fully customized 
 * database. The main role of the database is to store the actual data of the   
 * stream (without any bundle header information) and support the inherent      
 * capability of BSS for playback control over the last WINDOW seconds of the
 * stream.
 *
 * The BSS database consists of three different files: *.tbl, *.lst and *.dat.
 *
 * Table file (*.tbl) holds information about the last WINDOW seconds of the
 * stream. Each of these seconds is associated with four variables; the  
 * firstEntryOffset, lastEntryOffset, hgstCountVal and lwstCountVal variable. 
 * These variables are stored in four tables, each one of size WINDOW, where
 * each row of the table contains the relevant valuse for the corresponding
 * second.
 *
 * The firstEntryOffset and lastEntryOffset variables contain the offset of
 * the first and last element respectively of a doubly-linked list which
 * contains information on the ADUs of a single second. The hgstCountVal and
 * lwstCountVal values hold the highest and lowest count value respectively,
 * encountered through the reception of the stream for a particular second.
 *
 * The table file also holds four additional variables; the oldestTime,
 * oldestRowIndex, newestTime and newestRowIndex. The oldestTime and newestTime
 * hold the second with the lowest and highest value respectively among the
 * last WINDOW received seconds, while the oldestRowIndex and newestRowIndex
 * variables hold the rows in which the information for those two seconds are
 * stored.
 *
 * List entries file (*.lst) stores an array of doubly-linked lists;
 * each list corresponds to a single second of the received stream.  Each
 * element of each doubly-linked list contains information on the offset
 * within the *.dat file at which the actual data of some ADU are stored,
 * as well as information on the offset of the previous and the next element
 * of the doubly-linked list.
 *
 * Finally, the data file (*.dat) stores the actual data of the stream. Every
 * new ADU received by the BSS receiver application is stored in a new record
 * that is appended at the end of the file. It should be clearly stated that
 * the stream in its entirety is stored in the *.dat file, NOT only the last
 * WINDOW seconds.      
 *******************************************************************************
 */

/*  
 *  A structure that describes the format of the header of the *.tbl file:
 *    +-------------+-------------------+-------------+------------------+    
 *    | oldest time | oldest row index  | newest time | newest row index |    
 *    +-------------+-------------------+-------------+------------------+    
 */

typedef struct 
{
	time_t		oldestTime;
	long		oldestRowIndex;
	time_t		newestTime;
	long		newestRowIndex;
} tblHeader;

/*  
 *  A structure that describes the format of a single row (corresponding
 *  to a single second of streaming activity) in the *.tbl file:
 *    +-------------+-------------------+-------------+------------------+    
 *    |     first entry offset          |    last entry offset           |    
 *    +---------------------------------+--------------------------------+    
 *    |     highest count value         |    lowest count value          |    
 *    +---------------------------------+--------------------------------+    
 */

typedef struct 
{
	long		firstEntryOffset;
	long		lastEntryOffset;
	unsigned long	hgstCountVal;
	unsigned long	lwstCountVal;
} tblRow;

/*  
 *  A structure that describes the format of *.tbl file. A single instance of 
 *  tblIndex structure is stored in *.tbl file and has the following format:  
 *    +-------------+-------------------+-------------+------------------+    
 *    |                            table header                          |    
 *    +-------------+-------------------+-------------+------------------+    
 *    |                          table row[WINDOW]                       |    
 *    +---------------------------------+--------------------------------+    
 */

typedef struct 
{
	tblHeader	header;
	tblRow		rows[WINDOW];
} tblIndex;

/*   
 *  A structure that describes the format in which entries are written to   
 *  *.lst file. Each entry in *.lst file has the following format:    	    
 * 	+---------------+-------------+----------------+	     
 * 	| creation time | data offset | payload length | 	     
 * 	+---------------+-------------+----------------+ 	     
 * 	|     previous entry    |      next entry      | 	     
 * 	+-----------------------+----------------------+  	     
 */

typedef struct 
{
	BpTimestamp 	crtnTime;
	off_t		datOffset;
	long		pLen;
	long		prev;
	long		next;
	
} lstEntry;

/*  
 *  A structure that describes the format in which raw data are written to  
 *  *.dat file. Each record in *.dat file has the following format:	    
 * 		+---------------+----------------+----------+ 		    
 * 		| creation time | payload length | raw data | 		    
 * 		+---------------+----------------+----------+ 		    
 */

typedef struct
{
	BpTimestamp 	crtnTime;
	long		pLen;
} dataRecord;

extern int		_running(int *newValue);
extern int		_recvThreadId(pthread_t *id, int control);
extern int		_datFile(int control, int fileDescriptor);
extern int		_lstFile(int control, int fileDescriptor);
extern int		_tblFile(int control, int fileDescriptor);
extern tblIndex		*_tblIndex(int *control);
extern int		_lockMutex(int value);
extern BpSAP		_bpsap(BpSAP *newSAP);

extern int 		getLstEntry(int fileD, lstEntry *entry,
				long lstEntryOffset);

extern int 		readRecord(int fileD, dataRecord *rec, off_t datOffset);
extern int 		readPayload(int fileD, char* buffer, long length);

extern int 		loadRDWRDB(char* bssName, char* path, int* dat, 
				int* lst, int* tbl);
extern int 		loadRDonlyDB(char* bssName, char* path);
extern void		*recvBundles(void *args);

extern void		findIndexRow(time_t time, long *position);
#ifdef __cplusplus
}
#endif

#endif  /* _BSSP_H_ */

