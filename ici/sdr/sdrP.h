/*
 *	sdrP.h:	private definitions for spacecraft data recorder
 *		management library.
 *
 *	    Modification history:
 *		01-02-01  SCB	Revised for Solaris shm, multiple logs.
 *		03-08-96  APS	Abstracted the IPC services.
 *
 *	Copyright (c) 2001, California Institute of Technology.
 *	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
 *	acknowledged.
 */

#ifndef _SDRP_H_
#define _SDRP_H_

#ifndef NO_SDR_TRACE
#define	SDR_TRACE
#endif

#include "platform.h"
#include "memmgr.h"
#include "psm.h"
#include "lyst.h"
#include "smlist.h"
#include "sdrxn.h"

#ifdef SDR_TRACE
#include "sptrace.h"
#endif

#define SDR_SM_KEY	(255 * 256)
#define SDR_SM_NAME	"sdrwm"

/*	The structure of an SDR dataspace (DS) is as follows, where:
		M = sizeof(SdrMap)
		D = total size of SDR DS (includes map and heap)

	From offset	Until offset		Content is:

	0		M			map of sdr heap
	M		D			sdr heap (objects)	*/


#define	INITIALIZED	(0x99999999)

/*	Memory management abstraction.					*/
#define MTAKE(size)	allocFromSdrMemory(__FILE__, __LINE__, size)
#define MRELEASE(addr)	releaseToSdrMemory(__FILE__, __LINE__, addr)
extern void		*allocFromSdrMemory(const char *, int, size_t);
extern void		releaseToSdrMemory(const char *, int, void *);
extern void		*sdrMemAtoP(uaddr);
extern uaddr		sdrMemPtoA(void *);

/*	SdrControlHeader is the object in the root of the SDR working
 *	memory (a shared memory partition) that enables multiple
 *	applications to use the SDR system on a single computer
 *	concurrently.							*/

typedef struct
{
	PsmAddress	sdrs;	/*	An SmList of (SdrState *).	*/
} SdrControlHeader;

/*	SdrState is an object that encapsulates the volatile state of
 *	a single SDR.  It resides in SDR working memory (a shared
 *	memory partition), in the control header's list of sdrs.	*/

typedef struct sdr_str
{
		/*	General SDR operational parameters.	*/

	char		name[32];
	PsmAddress	sdrsElt;		/*	In sch->sdrs.	*/
	int		configFlags;
	size_t		initHeapWords;		/*	In FULL WORDS.	*/
	size_t		heapSize;		/*	dsSize - map	*/
	size_t		dsSize;			/*	heap + map	*/
	int		dsKey;			/*	RAM DS shmKey	*/
	size_t		logSize;		/*	(if in memory)	*/
	int		logKey;			/*	RAM log shmKey	*/

		/*	Parameters of current transaction.	*/

	sm_SemId	sdrSemaphore;
	int		sdrOwnerTask;		/*	Task ID.	*/
	pthread_t	sdrOwnerThread;		/*	Thread ID.	*/
	int		xnDepth;
	int		xnCanceled;		/*	Boolean.	*/
	int		logLength;		/*	All entries.	*/
	int		maxLogLength;		/*	Max Log Length  */
	PsmAddress	logEntries;		/*	Offsets in log.	*/

		/*	SDR trace data access.			*/

	int		traceKey;		/*	trace shmKey	*/
	size_t		traceSize;		/*	0 = disabled	*/

		/*	Path to directory for files (log, ds).	*/

	char		pathName[MAXPATHLEN];

		/*	Parameters for restart.				*/

	int		halted;			/*	boolean		*/
	char		restartCmd[32];
	time_t		restartTime;
} SdrState;

typedef struct
{
	Address		firstFreeBlock;
	size_t		freeBlocks;
} SmallFreeBucket;

typedef struct
{
	Address		firstFreeBlock;
	size_t		freeBlocks;
	size_t		freeBytes;
} LargeFreeBucket;

/*	SdrMap is an object that encapsulates the potentially non-
 *	volatile space management state of a single SDR.  It resides
 *	at the front of the SDR DS itself, preceding the SDR's heap.
 *	Since an SDR may be written to a file in addition to (or
 *	even rather than) occupying a shared-memory partition, the
 *	SDR's map can persist after reboot of the computer in which
 *	the SDR resides.  When an SDR is to be added to the sdrs
 *	list of the SDR control header, we can to some small extent
 *	assure that the SDR being added is the one we think we're
 *	adding; to do this, we check the dataspace size declared in
 *	sdr_load_profile() against the dsSize in the SDR's map.		*/

typedef struct	/*	Non-volatile state at front of SDR.		*/
{
	Object		catalogue;		/*	partition root	*/
	u_int		status;			/*	INITIALIZED?	*/
	size_t		dsSize;			/*	Map + heap.	*/
	size_t		heapSize;

		/*	For dynamic management of heap space.	*/

	Address		startOfSmallPool;
	Address		endOfSmallPool;
	SmallFreeBucket	smallPoolFree[SMALL_SIZES];
	Address		startOfLargePool;
	Address		endOfLargePool;
	LargeFreeBucket	largePoolFree[LARGE_ORDERS];
	unsigned int	largePoolSearchLimit;
	size_t		unassignedSpace;
} SdrMap;

/*	SdrView is an object that encapsulates a single process's
 *	transient private access to a single SDR.  It resides in
 *	SDR working memory (a shared memory partition) but is private
 *	to the process.  This is the structure that is returned by
 *	the sdr_start_using() function.					*/

typedef struct sdrv_str
{
	SdrState	*sdr;		/*	Local SDR state access.	*/

	int		dsfile;		/*	DS in file (fd).	*/
	char		*dssm;		/*	DS in shared memory.	*/
	uaddr		dssmId;		/*	DS shmId if applicable.	*/

	int		logfile;	/*	Xn log file (fd).	*/
	char		*logsm;		/*	Log in shared memory.	*/
	uaddr		logsmId;	/*	Log shmId if applicable.*/

	Lyst		knownObjects;	/*	ObjectExtents.		*/
	int		modified;	/*	Boolean.		*/

	PsmView		traceArea;	/*	local access to trace	*/
	PsmView		*trace;		/*	local access to trace	*/
	const char	*currentSourceFileName;	/*	for tracing	*/
	int		currentSourceFileLine;	/*	for tracing	*/
} SdrView;

typedef enum { UserPut = 0, SystemPut } PutSrc;

extern int		takeSdr(SdrState *sdr);
extern void		releaseSdr(SdrState *sdr);

extern void		joinTrace(Sdr, const char *, int);

extern SdrMap		*_mapImage();

#ifndef SDR_TRACE
extern char		*_noTraceMsg();
#endif

extern char		*_notInXnMsg();
extern char		*_apiErrMsg();
extern char		*_noMemoryMsg();
extern char		*_violationMsg();

#define ADDRESS_OF(X)	(((char *) &(map->X)) - ((char *) map))

extern void		_sdrput(const char*, int, Sdr, Address, char*, size_t,
				PutSrc);
#define	sdrPatch(A,V)	_sdrput(__FILE__, __LINE__, sdrv, (A), (char *) &(V), \
sizeof (V), SystemPut)
#define	patchMap(X,V)	_sdrput(__FILE__, __LINE__, sdrv, ADDRESS_OF(X), \
(char *) &(V), sizeof map->X, SystemPut)
#define	sdrPut(A,V)	_sdrput(file, line, sdrv, (A), (char *) &(V), \
sizeof (V), SystemPut)

extern void		_sdrfetch(Sdr, char *, Address, size_t);
#define sdrFetch(V,A)	_sdrfetch(sdrv, (char *) &(V), (A), sizeof (V))

extern Object		_sdrzalloc(Sdr, size_t);
extern Object		_sdrmalloc(Sdr, size_t);
extern void		_sdrfree(Sdr, Object, PutSrc);
#define sdrFree(Obj)	_sdrfree(sdrv, Obj, SystemPut)

extern int		sdrBoundaryViolated(Sdr, Address, size_t);
extern int		sdrFetchSafe(Sdr);

extern void		crashXn(Sdr);

#endif  /* _SDRP_H_ */
