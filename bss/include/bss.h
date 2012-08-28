/*
 *	bss.h:	Bundle Streaming Service API.
 *		For more details please refer to bssrecvP.h
 *		and libbssrecv.c files.
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

#ifndef _BSS_H_
#define _BSS_H_

#define _GNU_SOURCE

#ifndef _FILE_OFFSET_BITS
#define _FILE_OFFSET_BITS	64
#endif

#ifndef _LARGEFILE_SOURCE
#define _LARGEFILE_SOURCE
#endif

#ifndef _LARGEFILE64_SOURCE
#define _LARGEFILE64_SOURCE
#endif

#include <ion.h>

#define EPOCH_2000_SEC		946684800

#ifdef __cplusplus
extern "C" {
#endif

/* 
 *  bssNav is a structure that provides navigation assistance to BSS        
 *  applications. It consists of four variables that provide several        
 *  stream parsing information. The curPosition variable holds the          
 *  current position of the parser. That position refers to the row of      
 *  the firstEntryOffset, lastEntryOffset, hgstCountVal and lwstCountVal,   
 *  tables stored in *.tbl file. The prevOffset and nextOffset variables    
 *  hold the offset of previous and next element of the current doubly-     
 *  linked list element that is stored in the *.lst file. Finally, the      
 *  datOffset variable holds the offset of the actual data, stored in
 *  the *.dat file   						       
 */

typedef struct 
{
	long 		curPosition;
	long 		prevOffset; 
	long 		nextOffset;
	off_t 		datOffset;	
} bssNav;

/*	Callback function declaration		*/
typedef int		(*RTBHandler)(time_t time, unsigned long count, 
					char* buffer, unsigned long bufLength);

extern int		bssOpen(char* bssName, char* path);
extern int		bssStart(char* bssName, char* path, char* eid, 
				char* buffer, long bufLen, RTBHandler handler);
extern int		bssRun(char* bssName, char* path, char* eid,
				char* buffer, long bufLen, RTBHandler handler);

extern void		bssClose();
extern void		bssStop();
extern void		bssExit();

extern long		bssRead(bssNav nav, char* data, long dataLen);
extern long		bssSeek(bssNav *nav, time_t time, time_t *curTime, 
				unsigned long *count);
extern long		bssSeek_read(bssNav *nav, time_t time, time_t *curTime, 
				unsigned long *count, char* data, long dataLen);
extern long		bssNext(bssNav *nav, time_t *curTime, 
				unsigned long *count);
extern long		bssNext_read(bssNav *nav, time_t *curTime, 
				unsigned long *count, char* data, long dataLen);
extern long		bssPrev(bssNav *nav, time_t *curTime, 
				unsigned long *count);
extern long		bssPrev_read(bssNav *nav, time_t *curTime, 
				unsigned long *count, char* data, long dataLen);

/*	extern int		bssRecover();  - Under Development	*/
#ifdef __cplusplus
}
#endif

#endif  /* _BSS_H_ */

