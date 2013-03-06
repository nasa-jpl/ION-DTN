/*
 *	ion.c:	functions common to multiple protocols in the ION stack.
 *
 *	Copyright (c) 2007, California Institute of Technology.
 *	ALL RIGHTS RESERVED.  U.S. Government Sponsorship acknowledged.
 *
 *	Author: Scott Burleigh, JPL
 *
 */

#include "zco.h"
#include "ion.h"
#include "rfx.h"
#include "time.h"

#ifndef NODE_LIST_SEMKEY
#define NODE_LIST_SEMKEY	(0xeeee1)
#endif

#define	ION_DEFAULT_SM_KEY	((255 * 256) + 1)
#define	ION_SM_NAME		"ionwm"
#define	ION_DEFAULT_SDR_NAME	"ion"

#define timestampInFormat	"%4d/%2d/%2d-%2d:%2d:%2d"
#define timestampOutFormat	"%.4d/%.2d/%.2d-%.2d:%.2d:%.2d"

extern void	sdr_eject_xn(Sdr);

static char	*_iondbName()
{
	return "iondb";
}

static char	*_ionvdbName()
{
	return "ionvdb";
}

/*	*	*	Datatbase access	 *	*	*	*/

static Sdr	_ionsdr(Sdr *newSdr)
{
	static Sdr	sdr = NULL;

	if (newSdr)
	{
		if (*newSdr == NULL)	/*	Detaching.		*/
		{
			sdr = NULL;
		}
		else			/*	Initializing.		*/
		{
			if (sdr == NULL)
			{
				sdr = *newSdr;
			}
		}
	}

	return sdr;
}

static Object	_iondbObject(Object *newDbObj)
{
	static Object	obj = 0;

	if (newDbObj)
	{
		obj = *newDbObj;
	}

	return obj;
}

static IonDB	*_ionConstants()
{
	static IonDB	buf;
	static IonDB	*db = NULL;
	Sdr		sdr;
	Object		dbObject;

	if (db == NULL)
	{
		/*	Load constants into a conveniently accessed
		 *	structure.  Note that this CANNOT be treated
		 *	as a current database image in later
		 *	processing.					*/

		sdr = _ionsdr(NULL);
		CHKNULL(sdr);
		dbObject = _iondbObject(NULL);
		if (dbObject)
		{
			if (sdr_heap_is_halted(sdr))
			{
				sdr_read(sdr, (char *) &buf, dbObject,
						sizeof(IonDB));
			}
			else
			{
				CHKNULL(sdr_begin_xn(sdr));
				sdr_read(sdr, (char *) &buf, dbObject,
						sizeof(IonDB));
				sdr_exit_xn(sdr);
			}

			db = &buf;
		}
	}

	return db;
}

/*	*	*	Memory access	 *	*	*	*	*/

static int	_ionMemory(int *memmgrIdx)
{
	static int	idx = -1;

	if (memmgrIdx)
	{
		idx = *memmgrIdx;
	}

	return idx;
}

static PsmPartition	_ionwm(sm_WmParms *parms)
{
	static int		ionSmId = 0;
	static PsmView		ionWorkingMemory;
	static PsmPartition	ionwm = NULL;
	static int		memmgrIdx;
	static MemAllocator	wmtake = allocFromIonMemory;
	static MemDeallocator	wmrelease = releaseToIonMemory;
	static MemAtoPConverter	wmatop = ionMemAtoP;
	static MemPtoAConverter	wmptoa = ionMemPtoA;

	if (parms)
	{
		if (parms->wmSize == -1)	/*	Destroy.	*/
		{
			if (ionwm)
			{
				memmgr_destroy(ionSmId, &ionwm);
			}

			ionSmId = 0;
			ionwm = NULL;
			memmgrIdx = -1;
			oK(_ionMemory(&memmgrIdx));
			return NULL;
		}

		/*	Opening ION working memory.			*/

		if (ionwm)			/*	Redundant.	*/
		{
			return ionwm;
		}

		ionwm = &ionWorkingMemory;
		if (memmgr_open(parms->wmKey, parms->wmSize,
				&parms->wmAddress, &ionSmId, parms->wmName,
				&ionwm, &memmgrIdx, wmtake, wmrelease,
				wmatop, wmptoa) < 0)
		{
			putErrmsg("Can't open ION working memory.", NULL);
			return NULL;
		}

		oK(_ionMemory(&memmgrIdx));
	}

	return ionwm;
}

void	*allocFromIonMemory(char *fileName, int lineNbr, size_t length)
{
	PsmPartition	ionwm = _ionwm(NULL);
	PsmAddress	address;
	void		*block;

	address = Psm_zalloc(fileName, lineNbr, ionwm, length);
	if (address == 0)
	{
		putErrmsg("Can't allocate ION working memory.", itoa(length));
		return NULL;
	}

	block = psp(ionwm, address);
	memset(block, 0, length);
#ifdef HAVE_VALGRIND_VALGRIND_H
    VALGRIND_MALLOCLIKE_BLOCK(block, length, 0, 1);
#endif
	return block;
}

void	releaseToIonMemory(char *fileName, int lineNbr, void *block)
{
	PsmPartition	ionwm = _ionwm(NULL);

	Psm_free(fileName, lineNbr, ionwm, psa(ionwm, (char *) block));
#ifdef HAVE_VALGRIND_VALGRIND_H
    VALGRIND_FREELIKE_BLOCK(block, 0);
#endif
}

void	*ionMemAtoP(unsigned long address)
{
	return (void *) psp(_ionwm(NULL), address);
}

unsigned long ionMemPtoA(void *pointer)
{
	return (unsigned long) psa(_ionwm(NULL), pointer);
}

static IonVdb	*_ionvdb(char **name)
{
	static IonVdb	*vdb = NULL;
	PsmAddress	vdbAddress;
	PsmAddress	elt;
	Sdr		sdr;
	PsmPartition	ionwm;
	IonDB		iondb;

	if (name)
	{
		if (*name == NULL)	/*	Terminating.		*/
		{
			vdb = NULL;
			return vdb;
		}

		/*	Attaching to volatile database.			*/

		ionwm = _ionwm(NULL);
		if (psm_locate(ionwm, *name, &vdbAddress, &elt) < 0)
		{
			putErrmsg("Failed searching for vdb.", *name);
			return NULL;
		}

		if (elt)
		{
			vdb = (IonVdb *) psp(ionwm, vdbAddress);
			return vdb;
		}

		/*	ION volatile database doesn't exist yet.	*/

		sdr = _ionsdr(NULL);
		CHKNULL(sdr_begin_xn(sdr));	/*	To lock memory.	*/
		vdbAddress = psm_zalloc(ionwm, sizeof(IonVdb));
		if (vdbAddress == 0)
		{
			sdr_exit_xn(sdr);
			putErrmsg("No space for volatile database.", *name);
			return NULL;
		}

		vdb = (IonVdb *) psp(ionwm, vdbAddress);
		memset((char *) vdb, 0, sizeof(IonVdb));
		vdb->zcoSemaphore = sm_SemCreate(SM_NO_KEY, SM_SEM_FIFO);
		if (vdb->zcoSemaphore == SM_SEM_NONE)
		{
			sdr_exit_xn(sdr);
			putErrmsg("Can't initialize volatile database.", *name);
			return NULL;
		}

		sm_SemTake(vdb->zcoSemaphore);	/*	Lock it.	*/
		if ((vdb->nodes = sm_rbt_create(ionwm)) == 0
		|| (vdb->neighbors = sm_rbt_create(ionwm)) == 0
		|| (vdb->contactIndex = sm_rbt_create(ionwm)) == 0
		|| (vdb->rangeIndex = sm_rbt_create(ionwm)) == 0
		|| (vdb->timeline = sm_rbt_create(ionwm)) == 0
		|| (vdb->probes = sm_list_create(ionwm)) == 0
		|| psm_catlg(ionwm, *name, vdbAddress) < 0)
		{
			sdr_exit_xn(sdr);
			putErrmsg("Can't initialize volatile database.", *name);
			return NULL;
		}

		vdb->clockPid = ERROR;	/*	None yet.		*/
		sdr_read(sdr, (char *) &iondb, _iondbObject(NULL),
				sizeof(IonDB));
		vdb->deltaFromUTC = iondb.deltaFromUTC;
		sdr_exit_xn(sdr);	/*	Unlock memory.		*/
	}

	return vdb;
}

/*	*	*	Initialization	* 	*	*	*	*/

#if defined (FSWLOGGER)
#include "fswlogger.c"
#elif defined (GDSLOGGER)
#include "gdslogger.c"
#else

static void	writeMemoToIonLog(char *text)
{
	static ResourceLock	logFileLock;
	static char		ionLogFileName[264] = "";
	static int		ionLogFile = -1;
	time_t			currentTime = getUTCTime();
	char			timestampBuffer[20];
	int			textLen;
	static char		msgbuf[256];

	if (text == NULL) return;

	/*	The log file is shared, so access to it must be
	 *	mutexed.						*/

	if (initResourceLock(&logFileLock) < 0)
	{
		return;
	}

	lockResource(&logFileLock);
	if (ionLogFile == -1)
	{
		if (ionLogFileName[0] == '\0')
		{
			isprintf(ionLogFileName, sizeof ionLogFileName,
					"%.255s%cion.log",
					getIonWorkingDirectory(),
					ION_PATH_DELIMITER);
		}

		ionLogFile = iopen(ionLogFileName,
				O_WRONLY | O_APPEND | O_CREAT, 0666);
		if (ionLogFile == -1)
		{
			unlockResource(&logFileLock);
			perror("Can't redirect ION error msgs to log");
			return;
		}
	}

	writeTimestampLocal(currentTime, timestampBuffer);
	isprintf(msgbuf, sizeof msgbuf, "[%s] %s\n", timestampBuffer, text);
	textLen = strlen(msgbuf);
	if (write(ionLogFile, msgbuf, textLen) < 0)
	{
		perror("Can't write ION error message to log file");
	}
#ifdef TargetFFS
	close(ionLogFile);
	ionLogFile = -1;
#endif
	unlockResource(&logFileLock);
}

static void	ionRedirectMemos()
{
	setLogger(writeMemoToIonLog);
}
#endif

static int	checkNodeListParms(IonParms *parms, char *wdName, uvast nodeNbr)
{
	char		*nodeListDir;
	sm_SemId	nodeListMutex;
	char		nodeListFileName[265];
	int		nodeListFile;
	int		lineNbr = 0;
	int		lineLen;
	char		lineBuf[256];
	uvast		lineNodeNbr;
	int		lineWmKey;
	char		lineSdrName[MAX_SDR_NAME + 1];
	char		lineWdName[256];
	int		result;

	nodeListDir = getenv("ION_NODE_LIST_DIR");
	if (nodeListDir == NULL)	/*	Single node on machine.	*/
	{
		if (parms->wmKey == 0)
		{
			parms->wmKey = ION_DEFAULT_SM_KEY;
		}

		if (parms->wmKey != ION_DEFAULT_SM_KEY)
		{
			putErrmsg("Config parms wmKey != default.",
					itoa(ION_DEFAULT_SM_KEY));
			return -1;
		}

		if (parms->sdrName[0] == '\0')
		{
			istrcpy(parms->sdrName, ION_DEFAULT_SDR_NAME,
					sizeof parms->sdrName);
		}

		if (strcmp(parms->sdrName, ION_DEFAULT_SDR_NAME) != 0)
		{
			putErrmsg("Config parms sdrName != default.",
					ION_DEFAULT_SDR_NAME);
			return -1;
		}

		return 0;
	}

	/*	Configured for multi-node operation.			*/

	nodeListMutex = sm_SemCreate(NODE_LIST_SEMKEY, SM_SEM_FIFO);
	if (nodeListMutex == SM_SEM_NONE
	|| sm_SemUnwedge(nodeListMutex, 3) < 0 || sm_SemTake(nodeListMutex) < 0)
	{
		putErrmsg("Can't lock node list file.", NULL);
		return -1;
	}

	isprintf(nodeListFileName, sizeof nodeListFileName, "%.255s%cion_nodes",
			nodeListDir, ION_PATH_DELIMITER);
	if (nodeNbr == 0)	/*	Just attaching.			*/
	{
		nodeListFile = iopen(nodeListFileName, O_RDONLY, 0);
	}
	else			/*	Initializing the node.		*/
	{
		nodeListFile = iopen(nodeListFileName, O_RDWR | O_CREAT, 0666);
	}

	if (nodeListFile < 0)
	{
		sm_SemGive(nodeListMutex);
		putSysErrmsg("Can't open ion_nodes file", nodeListFileName);
		writeMemo("[?] Remove ION_NODE_LIST_DIR from env?");
		return -1;
	}

	while (1)
	{
		if (igets(nodeListFile, lineBuf, sizeof lineBuf, &lineLen)
				== NULL)
		{
			if (lineLen < 0)
			{
				close(nodeListFile);
				sm_SemGive(nodeListMutex);
				putErrmsg("Failed reading ion_nodes file.",
						nodeListFileName);
				return -1;
			}

			break;		/*	End of file.		*/
		}

		lineNbr++;
		if (sscanf(lineBuf, UVAST_FIELDSPEC " %d %31s %255s",
			&lineNodeNbr, &lineWmKey, lineSdrName, lineWdName) < 4)
		{
			close(nodeListFile);
			sm_SemGive(nodeListMutex);
			putErrmsg("Syntax error at line#", itoa(lineNbr));
			writeMemoNote("[?] Repair ion_nodes file.",
					nodeListFileName);
			return -1;
		}

		if (lineNodeNbr == nodeNbr)		/*	Match.	*/
		{
			/*	lineNodeNbr can't be zero (we never
			 *	write such lines to the file), so this
			 *	must be matching non-zero node numbers.
			 *	So we are re-initializing this node.	*/

			close(nodeListFile);
			if (strcmp(lineWdName, wdName) != 0)
			{
				sm_SemGive(nodeListMutex);
				putErrmsg("CWD conflict at line#",
						itoa(lineNbr));
				writeMemoNote("[?] Repair ion_nodes file.",
						nodeListFileName);
				return -1;
			}

			if (parms->wmKey == 0)
			{
				parms->wmKey = lineWmKey;
			}

			if (parms->wmKey != lineWmKey)
			{
				sm_SemGive(nodeListMutex);
				putErrmsg("WmKey conflict at line#",
						itoa(lineNbr));
				writeMemoNote("[?] Repair ion_nodes file.",
						nodeListFileName);
				return -1;
			}

			if (parms->sdrName[0] == '\0')
			{
				istrcpy(parms->sdrName, lineSdrName,
						sizeof parms->sdrName);
			}

			if (strcmp(parms->sdrName, lineSdrName) != 0)
			{
				sm_SemGive(nodeListMutex);
				putErrmsg("SdrName conflict at line#",
						itoa(lineNbr));
				writeMemoNote("[?] Repair ion_nodes file.",
						nodeListFileName);
				return -1;
			}

			return 0;
		}

		/*	lineNodeNbr does not match nodeNbr (which may
		 *	be zero).					*/

		if (strcmp(lineWdName, wdName) == 0)	/*	Match.	*/
		{
			close(nodeListFile);
			sm_SemGive(nodeListMutex);
			if (nodeNbr == 0)	/*	Attaching.	*/
			{
				parms->wmKey = lineWmKey;
				istrcpy(parms->sdrName, lineSdrName,
						MAX_SDR_NAME + 1);
				return 0;
			}

			/*	Reinitialization conflict.		*/

			putErrmsg("NodeNbr conflict at line#", itoa(lineNbr));
			writeMemoNote("[?] Repair ion_nodes file.",
					nodeListFileName);
			return -1;
		}

		/*	Haven't found matching line yet.  Continue.	*/
	}

	/*	No matching lines in file.				*/

	if (nodeNbr == 0)	/*	Attaching to existing node.	*/
	{
		close(nodeListFile);
		sm_SemGive(nodeListMutex);
		putErrmsg("No node has been initialized in this directory.",
				wdName);
		return -1;
	}

	/*	Initializing, so append line to the nodes list file.	*/

	if (parms->wmKey == 0)
	{
		parms->wmKey = ION_DEFAULT_SM_KEY;
	}

	if (parms->sdrName[0] == '\0')
	{
		istrcpy(parms->sdrName, ION_DEFAULT_SDR_NAME,
				sizeof parms->sdrName);
	}

	isprintf(lineBuf, sizeof lineBuf, UVAST_FIELDSPEC " %d %.31s %.255s\n",
			nodeNbr, parms->wmKey, parms->sdrName, wdName);
	result = iputs(nodeListFile, lineBuf);
	close(nodeListFile);
	sm_SemGive(nodeListMutex);
	if (result < 0)
	{
		putErrmsg("Failed writing to ion_nodes file.", NULL);
		return -1;
	}

	return 0;
}

#ifdef mingw
static DWORD WINAPI	waitForSigterm(LPVOID parm)
{
	DWORD	processId;
	char	eventName[32];
	HANDLE	event;

	processId = GetCurrentProcessId();
	sprintf(eventName, "%u.sigterm", (unsigned int) processId);
	event = CreateEvent(NULL, FALSE, FALSE, eventName);
	if (event == NULL)
	{
		putErrmsg("Can't create sigterm event.", utoa(GetLastError()));
		return 0;
	}

	oK(WaitForSingleObject(event, INFINITE));
	raise(SIGTERM);
	CloseHandle(event);
	return 0;
}
#endif

static int	ionWaitForZcoSpace(IonVdb *vdb)
{
	int	result;

	vdb->zcoClaimants += 1;
	result = sm_SemTake(vdb->zcoSemaphore);
	if (result == 0)
	{
		if (sm_SemEnded(vdb->zcoSemaphore))
		{
			writeMemo("[i] ZCO space semaphore ended.");
		}
		else
		{
			vdb->zcoClaims -= 1;
			result = 1;
		}
	}

	return result;
}

static void	ionReleaseZcoSpace(IonVdb *vdb)
{
	/*	If there are any remaining claims then let the
	 *	next claimant in the queue (possibly this claimant,
	 *	now re-appended to semaphore's FIFO) take a shot.	*/

	if (vdb->zcoClaims > 0)
	{
		oK(sm_SemGive(vdb->zcoSemaphore));
	}
}

Object	ionCreateZco(ZcoMedium source, Object location, vast offset, vast size,
		int *cancel)
{
	Sdr	sdr = getIonsdr();
	IonVdb	*vdb = _ionvdb(NULL);
	Object	zco;
	int	admissionDelayed = 0;

	CHKZERO(vdb);
	if (cancel)
	{
		*cancel = 0;		/*	Initialize.		*/
	}

	CHKZERO(sdr_begin_xn(sdr));
	while (1)
	{
		zco = zco_create(sdr, source, location, offset, size);
		if (sdr_end_xn(sdr) < 0 || zco == (Object) ERROR)
		{
			putErrmsg("Can't create ZCO.", NULL);
			return 0;
		}

		if (zco == 0)	/*	Not enough ZCO space.		*/
		{
			if (admissionDelayed)
			{
				ionReleaseZcoSpace(vdb);
			}

			admissionDelayed = 1;
			if (ionWaitForZcoSpace(vdb) == 1)
			{
				if (cancel && *cancel)
				{
					return 0;
				}

				CHKZERO(sdr_begin_xn(sdr));
				continue;
			}

			return 0;
		}

		/*	ZCO was created.				*/

		if (admissionDelayed)
		{
			ionReleaseZcoSpace(vdb);
		}

		return zco;
	}
}

vast	ionAppendZcoExtent(Object zco, ZcoMedium source, Object location,
		vast offset, vast size, int *cancel)
{
	Sdr	sdr = getIonsdr();
	IonVdb	*vdb = _ionvdb(NULL);
	Object	length;
	int	admissionDelayed = 0;

	CHKZERO(vdb);
	if (cancel)
	{
		*cancel = 0;		/*	Initialize.		*/
	}

	CHKZERO(sdr_begin_xn(sdr));
	while (1)
	{
		length = zco_append_extent(sdr, zco, source, location, offset,
				size);
		if (sdr_end_xn(sdr) < 0 || length == ERROR)
		{
			putErrmsg("Can't create ZCO.", NULL);
			return ERROR;
		}

		if (length == 0)	/*	Not enough ZCO space.	*/
		{
			if (admissionDelayed)
			{
				ionReleaseZcoSpace(vdb);
			}

			admissionDelayed = 1;
			if (ionWaitForZcoSpace(vdb) == 1)
			{
				if (cancel && *cancel)
				{
					return 0;
				}

				CHKZERO(sdr_begin_xn(sdr));
				continue;
			}

			return ERROR;
		}

		/*	ZCO extent was appended.			*/

		if (admissionDelayed)
		{
			ionReleaseZcoSpace(vdb);
		}

		return length;
	}
}

static void	ionOfferZcoSpace()
{
	IonVdb	*vdb = _ionvdb(NULL);

	if (vdb)
	{
		/*	Give all tasks currently waiting for ZCO
		 *	space a shot at claiming the space that
		 *	has now been made available.			*/

		vdb->zcoClaims += vdb->zcoClaimants;
		vdb->zcoClaimants = 0;
		if (vdb->zcoClaims > 0)
		{
			sm_SemGive(vdb->zcoSemaphore);
		}
	}
}

void	ionCancelZcoSpaceRequest(int *cancel)
{
	if (cancel)
	{
		*cancel = 1;			/*	Cancel.		*/
		ionOfferZcoSpace();
	}
}

int	ionInitialize(IonParms *parms, uvast ownNodeNbr)
{
	char		wdname[256];
	Sdr		ionsdr;
	Object		iondbObject;
	IonDB		iondbBuf;
	vast		limit;
	sm_WmParms	ionwmParms;
	char		*ionvdbName = _ionvdbName();
	ZcoCallback	notify = ionOfferZcoSpace;

	CHKERR(parms);
	CHKERR(ownNodeNbr);
#ifdef mingw
	if (_winsock(0) < 0)
	{
		return -1;
	}
#endif
	if (sdr_initialize(0, NULL, SM_NO_KEY, NULL) < 0)
	{
		putErrmsg("Can't initialize the SDR system.", NULL);
		return -1;
	}

	if (igetcwd(wdname, 256) == NULL)
	{
		putErrmsg("Can't get cwd name.", NULL);
		return -1;
	}

	if (checkNodeListParms(parms, wdname, ownNodeNbr) < 0)
	{
		putErrmsg("Failed checking node list parms.", NULL);
		return -1;
	}

	if (sdr_load_profile(parms->sdrName, parms->configFlags,
			parms->heapWords, parms->heapKey, parms->pathName,
			"ionrestart") < 0)
	{
		putErrmsg("Unable to load SDR profile for ION.", NULL);
		return -1;
	}

	ionsdr = sdr_start_using(parms->sdrName);
	if (ionsdr == NULL)
	{
		putErrmsg("Can't start using SDR for ION.", NULL);
		return -1;
	}

	ionsdr = _ionsdr(&ionsdr);

	/*	Recover the ION database, creating it if necessary.	*/

	CHKERR(sdr_begin_xn(ionsdr));
	iondbObject = sdr_find(ionsdr, _iondbName(), NULL);
	switch (iondbObject)
	{
	case -1:		/*	SDR error.			*/
		sdr_cancel_xn(ionsdr);
		putErrmsg("Can't seek ION database in SDR.", NULL);
		return -1;

	case 0:			/*	Not found; must create new DB.	*/
		if (ownNodeNbr == 0)
		{
			sdr_cancel_xn(ionsdr);
			putErrmsg("Must supply non-zero node number.", NULL);
			return -1;
		}

		memset((char *) &iondbBuf, 0, sizeof(IonDB));
		memcpy(iondbBuf.workingDirectoryName, wdname, 256);
		iondbBuf.ownNodeNbr = ownNodeNbr;
		iondbBuf.productionRate = -1;	/*	Unknown.	*/
		iondbBuf.consumptionRate = -1;	/*	Unknown.	*/
		limit = (sdr_heap_size(ionsdr) / 100) * (100 - ION_SEQUESTERED);
		zco_set_max_heap_occupancy(ionsdr, limit);
		iondbBuf.occupancyCeiling = zco_get_max_file_occupancy(ionsdr);
		iondbBuf.occupancyCeiling += limit;
		iondbBuf.contacts = sdr_list_create(ionsdr);
		iondbBuf.ranges = sdr_list_create(ionsdr);
		iondbBuf.maxClockError = 0;
		iondbBuf.clockIsSynchronized = 1;
                memcpy(&iondbBuf.parmcopy, parms, sizeof(IonParms));
		iondbObject = sdr_malloc(ionsdr, sizeof(IonDB));
		if (iondbObject == 0)
		{
			sdr_cancel_xn(ionsdr);
			putErrmsg("No space for database.", NULL);
			return -1;
		}

		sdr_write(ionsdr, iondbObject, (char *) &iondbBuf,
				sizeof(IonDB));
		sdr_catlg(ionsdr, _iondbName(), 0, iondbObject);
		if (sdr_end_xn(ionsdr))
		{
			putErrmsg("Can't create ION database.", NULL);
			return -1;
		}

		break;

	default:		/*	Found DB in the SDR.		*/
		sdr_exit_xn(ionsdr);
	}

	oK(_iondbObject(&iondbObject));
	oK(_ionConstants());

	/*	Open ION shared-memory partition.			*/

	ionwmParms.wmKey = parms->wmKey;
	ionwmParms.wmSize = parms->wmSize;
	ionwmParms.wmAddress = parms->wmAddress;
	ionwmParms.wmName = ION_SM_NAME;
	if (_ionwm(&ionwmParms) == NULL)
	{
		putErrmsg("ION memory configuration failed.", NULL);
		return -1;
	}

	if (_ionvdb(&ionvdbName) == NULL)
	{
		putErrmsg("ION can't initialize vdb.", NULL);
		return -1;
	}

	zco_register_callback(notify);
	ionRedirectMemos();
#ifdef mingw
	DWORD	threadId;
	HANDLE	thread = CreateThread(NULL, 0, waitForSigterm, NULL, 0,
			&threadId);
	if (thread == NULL)
	{
		putErrmsg("Can't create sigterm thread.", utoa(GetLastError()));
	}
	else
	{
		CloseHandle(thread);
	}
#endif
	return 0;
}

static void	destroyIonNode(PsmPartition partition, PsmAddress eltData,
			void *argument)
{
	IonNode	*node = (IonNode *) psp(partition, eltData);

	sm_list_destroy(partition, node->snubs, rfx_erase_data, NULL);
	psm_free(partition, eltData);
}

static void	dropVdb(PsmPartition wm, PsmAddress vdbAddress)
{
	IonVdb		*vdb;

	vdb = (IonVdb *) psp(wm, vdbAddress);

	/*	Time-ordered list of probes can simply be destroyed.	*/

	sm_list_destroy(wm, vdb->probes, rfx_erase_data, NULL);

	/*	Three of the red-black tables in the Vdb are
	 *	emptied and recreated by rfx_stop().  Destroy them.	*/

	sm_rbt_destroy(wm, vdb->contactIndex, NULL, NULL);
	sm_rbt_destroy(wm, vdb->rangeIndex, NULL, NULL);
	sm_rbt_destroy(wm, vdb->timeline, NULL, NULL);

	/*	cgr_stop clears all routing objects, so nodes and
	 *	neighbors themselves can now be deleted.		*/

	sm_rbt_destroy(wm, vdb->nodes, destroyIonNode, NULL);
	sm_rbt_destroy(wm, vdb->neighbors, rfx_erase_data, NULL);

	/*	Safely delete the ZCO availability semaphore.		*/

	sm_SemEnd(vdb->zcoSemaphore);
	sm_SemDelete(vdb->zcoSemaphore);
	vdb->zcoSemaphore = SM_SEM_NONE;
	vdb->zcoClaimants = 0;
	vdb->zcoClaims = 0;
	zco_unregister_callback();
}

void	ionDropVdb()
{
	PsmPartition	wm = getIonwm();
	char		*ionvdbName = _ionvdbName();
	PsmAddress	vdbAddress;
	PsmAddress	elt;
	char		*stop = NULL;

	if (psm_locate(wm, ionvdbName, &vdbAddress, &elt) < 0)
	{
		putErrmsg("Failed searching for vdb.", NULL);
		return;
	}

	if (elt)
	{
		dropVdb(wm, vdbAddress);	/*	Destroy Vdb.	*/
		psm_free(wm, vdbAddress);
		if (psm_uncatlg(wm, ionvdbName) < 0)
		{
			putErrmsg("Failed uncataloging vdb.", NULL);
		}
	}

	oK(_ionvdb(&stop));			/*	Forget old Vdb.	*/
}

void	ionRaiseVdb()
{
	char	*ionvdbName = _ionvdbName();

	if(_ionvdb(&ionvdbName) == NULL)	/*	Create new Vdb.	*/
	{
		putErrmsg("ION can't reinitialize vdb.", NULL);
	}
}

int	ionAttach()
{
	Sdr		ionsdr = _ionsdr(NULL);
	Object		iondbObject = _iondbObject(NULL);
	PsmPartition	ionwm = _ionwm(NULL);
	IonVdb		*ionvdb = _ionvdb(NULL);
	char		*wdname;
	char		wdnamebuf[256];
	IonParms	parms;
	sm_WmParms	ionwmParms;
	char		*ionvdbName = _ionvdbName();
	ZcoCallback	notify = ionOfferZcoSpace;

	if (ionsdr && iondbObject && ionwm && ionvdb)
	{
		return 0;	/*	Already attached.		*/
	}

#ifdef mingw
	if (_winsock(0) < 0)
	{
		return -1;
	}

	signal(SIGINT, SIG_IGN);
#endif

	if (sdr_initialize(0, NULL, SM_NO_KEY, NULL) < 0)
	{
		putErrmsg("Can't initialize the SDR system.", NULL);
		return -1;
	}

	wdname = getenv("ION_NODE_WDNAME");
	if (wdname == NULL)
	{
		if (igetcwd(wdnamebuf, 256) == NULL)
		{
			putErrmsg("Can't get cwd name.", NULL);
			return -1;
		}

		wdname = wdnamebuf;
	}

	memset((char *) &parms, 0, sizeof parms);
	if (checkNodeListParms(&parms, wdname, 0) < 0)
	{
		putErrmsg("Failed checking node list parms.", NULL);
		return -1;
	}

	if (ionsdr == NULL)
	{
		ionsdr = sdr_start_using(parms.sdrName);
		if (ionsdr == NULL)
		{
			putErrmsg("Can't start using SDR for ION.", NULL);
			return -1;
		}

		oK(_ionsdr(&ionsdr));
	}

	if (iondbObject == 0)
	{
		if (sdr_heap_is_halted(ionsdr))
		{
			iondbObject = sdr_find(ionsdr, _iondbName(), NULL);
		}
		else
		{
			CHKERR(sdr_begin_xn(ionsdr));
			iondbObject = sdr_find(ionsdr, _iondbName(), NULL);
			sdr_exit_xn(ionsdr);
		}

		if (iondbObject == 0)
		{
			putErrmsg("ION database not found.", NULL);
			return -1;
		}

		oK(_iondbObject(&iondbObject));
	}

	oK(_ionConstants());

	/*	Open ION shared-memory partition.			*/

	if (ionwm == NULL)
	{
		ionwmParms.wmKey = parms.wmKey;
		ionwmParms.wmSize = 0;
		ionwmParms.wmAddress = NULL;
		ionwmParms.wmName = ION_SM_NAME;
		ionwm = _ionwm(&ionwmParms);
		if (ionwm == NULL)
		{
			putErrmsg("Can't open access to ION memory.", NULL);
			return -1;
		}
	}

	if (ionvdb == NULL)
	{
		if (_ionvdb(&ionvdbName) == NULL)
		{
			putErrmsg("ION volatile database not found.", NULL);
			return -1;
		}
	}

	zco_register_callback(notify);
	ionRedirectMemos();
#ifdef mingw
	DWORD	threadId;
	HANDLE	thread = CreateThread(NULL, 0, waitForSigterm, NULL, 0,
			&threadId);
	if (thread == NULL)
	{
		putErrmsg("Can't create sigterm thread.", utoa(GetLastError()));
	}
	else
	{
		CloseHandle(thread);
	}
#endif
	return 0;
}

void	ionDetach()
{
#if defined (VXWORKS) || defined (bionic)
	return;
#elif defined (RTEMS)
	sm_TaskForget(sm_TaskIdSelf());
#else
	Sdr	ionsdr = _ionsdr(NULL);

	if (ionsdr)
	{
		sdr_stop_using(ionsdr);
		ionsdr = NULL;		/*	To reset to NULL.	*/
		oK(_ionsdr(&ionsdr));
	}
#ifdef mingw
	oK(_winsock(1));
#endif
#endif
}

void	ionProd(uvast fromNode, uvast toNode, unsigned int xmitRate,
		unsigned int owlt)
{
	Sdr	ionsdr = _ionsdr(NULL);
	time_t	fromTime;
	time_t	toTime;
	Object	elt;
	char	textbuf[RFX_NOTE_LEN];

	if (ionsdr == NULL)
	{
		if (ionAttach() < 0)
		{
			writeMemo("[?] ionProd: node not initialized yet.");
			return;
		}
	}

	fromTime = getUTCTime();	/*	The current time.	*/
	toTime = fromTime + 14400;	/*	Four hours later.	*/
	elt = rfx_insert_range(fromTime, toTime, fromNode, toNode, owlt);
       	if (elt == 0)
	{
		writeMemoNote("[?] ionProd: range insertion failed.",
				utoa(owlt));
		return;
	}

	writeMemo("ionProd: range inserted.");
	writeMemo(rfx_print_range(sdr_list_data(ionsdr, elt), textbuf));
	elt = rfx_insert_contact(fromTime, toTime, fromNode, toNode, xmitRate);
	if (elt == 0)
	{
		writeMemoNote("[?] ionProd: contact insertion failed.",
				utoa(xmitRate));
		return;
	}

	writeMemo("ionProd: contact inserted.");
	writeMemo(rfx_print_contact(sdr_list_data(ionsdr, elt), textbuf));
}

void	ionEject()
{
	sdr_eject_xn(_ionsdr(NULL));
}

void	ionTerminate()
{
	Sdr		sdr = _ionsdr(NULL);
	Object		obj = 0;
	sm_WmParms	ionwmParms;
	char		*ionvdbName = NULL;

	if (sdr)
	{
		sdr_destroy(sdr);
		sdr = NULL;
		oK(_ionsdr(&sdr));	/*	To reset to NULL.	*/
	}

	oK(_iondbObject(&obj));
	ionwmParms.wmKey = 0;
	ionwmParms.wmSize = -1;
	ionwmParms.wmAddress = NULL;
	oK(_ionwm(&ionwmParms));
	oK(_ionvdb(&ionvdbName));
}

Sdr	getIonsdr()
{
	return _ionsdr(NULL);
}

Object	getIonDbObject()
{
	return _iondbObject(NULL);
}

PsmPartition	getIonwm()
{
	return _ionwm(NULL);
}

int	getIonMemoryMgr()
{
	return _ionMemory(NULL);
}

IonVdb	*getIonVdb()
{
	return _ionvdb(NULL);
}

char	*getIonWorkingDirectory()
{
	IonDB	*snapshot = _ionConstants();

	if (snapshot == NULL)
	{
		return ".";
	}

	return snapshot->workingDirectoryName;
}

uvast	getOwnNodeNbr()
{
	IonDB	*snapshot = _ionConstants();

	if (snapshot == NULL)
	{
		return 0;
	}

	return snapshot->ownNodeNbr;
}

int	ionClockIsSynchronized()
{
	Sdr	ionsdr = _ionsdr(NULL);
	Object	iondbObject = _iondbObject(NULL);
	IonDB	iondbBuf;

	sdr_read(ionsdr, (char *) &iondbBuf, iondbObject, sizeof(IonDB));
	return iondbBuf.clockIsSynchronized;
}

/*	*	*	Shared-memory tracing 	*	*	*	*/

int	startIonMemTrace(int size)
{
	return psm_start_trace(_ionwm(NULL), size, NULL);
}

void	printIonMemTrace(int verbose)
{
	psm_print_trace(_ionwm(NULL), verbose);
}

void	clearIonMemTrace(int verbose)
{
	psm_clear_trace(_ionwm(NULL));
}

void	stopIonMemTrace(int verbose)
{
	psm_stop_trace(_ionwm(NULL));
}

/*	*	*	Timestamp handling 	*	*	*	*/

int	setDeltaFromUTC(int newDelta)
{
	Sdr	ionsdr = _ionsdr(NULL);
	Object	iondbObject = _iondbObject(NULL);
	IonVdb	*ionvdb = _ionvdb(NULL);
	IonDB	iondb;

	CHKERR(sdr_begin_xn(ionsdr));
	sdr_stage(ionsdr, (char *) &iondb, iondbObject, sizeof(IonDB));
	iondb.deltaFromUTC = newDelta;
	sdr_write(ionsdr, iondbObject, (char *) &iondb, sizeof(IonDB));
	if (sdr_end_xn(ionsdr) < 0)
	{
		putErrmsg("Can't change delta from UTC.", NULL);
		return -1;
	}

	ionvdb->deltaFromUTC = newDelta;
	return 0;
}

time_t	getUTCTime()
{
	IonVdb	*ionvdb = _ionvdb(NULL);
	int	delta = ionvdb ? ionvdb->deltaFromUTC : 0;
	time_t	clocktime;
#if defined(FSWCLOCK)
#include "fswutc.c"
#else

	clocktime = time(NULL);
#endif
	return clocktime - delta;
}

static time_t	readTimestamp(char *timestampBuffer, time_t referenceTime,
			int timestampIsUTC)
{
	long		interval = 0;
	struct tm	ts;
	int		count;

	if (timestampBuffer == NULL)
	{
		return 0;
	}

	if (*timestampBuffer == '+')	/*	Relative time.		*/
	{
		interval = strtol(timestampBuffer + 1, NULL, 0);
		return referenceTime + interval;
	}

	memset((char *) &ts, 0, sizeof ts);
	count = sscanf(timestampBuffer, timestampInFormat, &ts.tm_year,
		&ts.tm_mon, &ts.tm_mday, &ts.tm_hour, &ts.tm_min, &ts.tm_sec);
	if (count != 6)
	{
		return 0;
	}

	ts.tm_year -= 1900;
	ts.tm_mon -= 1;
	ts.tm_isdst = 0;		/*	Default is UTC.		*/
#ifndef VXWORKS
#ifdef mingw
	_tzset();	/*	Need to orient mktime properly.		*/
#else
	tzset();	/*	Need to orient mktime properly.		*/
#endif
	if (timestampIsUTC)
	{
		/*	Must convert UTC time to local time for mktime.	*/

#if defined (freebsd)
		ts.tm_sec -= ts.tm_gmtoff;
#elif defined (RTEMS)
		/*	RTEMS has no concept of time zones.		*/
#elif defined (mingw)
		ts.tm_sec -= _timezone;
#else
		ts.tm_sec -= timezone;
#endif
	}
	else	/*	Local time already; may or may not be DST.	*/
	{
		ts.tm_isdst = -1;
	}
#endif
	return mktime(&ts);
}

time_t	readTimestampLocal(char *timestampBuffer, time_t referenceTime)
{
	return readTimestamp(timestampBuffer, referenceTime, 0);
}

time_t	readTimestampUTC(char *timestampBuffer, time_t referenceTime)
{
	return readTimestamp(timestampBuffer, referenceTime, 1);
}

void	writeTimestampLocal(time_t timestamp, char *timestampBuffer)
{
	struct tm	ts;

	CHKVOID(timestampBuffer);
	oK(localtime_r(&timestamp, &ts));
	isprintf(timestampBuffer, 20, timestampOutFormat,
			ts.tm_year + 1900, ts.tm_mon + 1, ts.tm_mday,
			ts.tm_hour, ts.tm_min, ts.tm_sec);
}

void	writeTimestampUTC(time_t timestamp, char *timestampBuffer)
{
	struct tm	ts;

	CHKVOID(timestampBuffer);
	oK(gmtime_r(&timestamp, &ts));
	isprintf(timestampBuffer, 20, timestampOutFormat,
			ts.tm_year + 1900, ts.tm_mon + 1, ts.tm_mday,
			ts.tm_hour, ts.tm_min, ts.tm_sec);
}

/*	*	*	Parsing 	*	*	*	*	*/

int	_extractSdnv(uvast *into, unsigned char **from, int *remnant,
		int lineNbr)
{
	int	sdnvLength;

	CHKZERO(into && from && remnant);
	if (*remnant < 1)
	{
		writeMemoNote("[?] Missing SDNV at line...", itoa(lineNbr));
		return 0;
	}

	sdnvLength = decodeSdnv(into, *from);
	if (sdnvLength < 1)
	{
		writeMemoNote("[?] Invalid SDNV at line...", itoa(lineNbr));
		return 0;
	}

	(*from) += sdnvLength;
	(*remnant) -= sdnvLength;
	return sdnvLength;
}

int	_extractSmallSdnv(unsigned int *into, unsigned char **from,
		int *remnant, int lineNbr)
{
	int	sdnvLength;
	uvast	val;

	CHKZERO(into && from && remnant);
	if (*remnant < 1)
	{
		writeMemoNote("[?] Missing SDNV at line...", itoa(lineNbr));
		return 0;
	}

	sdnvLength = decodeSdnv(&val, *from);
	if (sdnvLength < 1)
	{
		writeMemoNote("[?] Invalid SDNV at line...", itoa(lineNbr));
		return 0;
	}

	*into = val;				/*	Truncate.	*/
	(*from) += sdnvLength;
	(*remnant) -= sdnvLength;
	return sdnvLength;
}

/*	*	*	Debugging 	*	*	*	*	*/

int	ionLocked()
{
	return sdr_in_xn(_ionsdr(NULL));	/*	Boolean.	*/
}

/*	*	*	SDR configuration	*	*	*	*/

int	readIonParms(char *configFileName, IonParms *parms)
{
	char	ownHostName[MAXHOSTNAMELEN + 1];
	char	*endOfHostName;
	char	configFileNameBuffer[PATHLENMAX + 1 + 9 + 1];
	int	configFile;
	char	buffer[512];
	int	lineNbr;
	char	line[256];
	int	lineLength;
	int	result;
	char	*cursor;
	int	i;
	char	*tokens[2];
	int	tokenCount;

	/*	Set defaults.						*/

	CHKERR(parms);
	memset((char *) parms, 0, sizeof(IonParms));
	parms->wmSize = 5000000;
	parms->wmAddress = 0;		/*	Dyamically allocated.	*/
	parms->configFlags = SDR_IN_DRAM | SDR_REVERSIBLE | SDR_BOUNDED;
	parms->heapWords = 250000;
	parms->heapKey = SM_NO_KEY;
	istrcpy(parms->pathName, "/tmp", sizeof parms->pathName);

	/*	Determine name of config file.				*/

	if (configFileName == NULL)
	{
#ifdef ION_NO_DNS
		ownHostName[0] = '\0';
#else
		if (getNameOfHost(ownHostName, MAXHOSTNAMELEN) < 0)
		{
			writeMemo("[?] Can't get name of local host.");
			return -1;
		}
#endif
		/*	Find end of high-order part of host name.	*/

		if ((endOfHostName = strchr(ownHostName, '.')) != NULL)
		{
			*endOfHostName = 0;
		}

		isprintf(configFileNameBuffer, sizeof configFileNameBuffer,
				"%.256s.ionconfig", ownHostName);
		configFileName = configFileNameBuffer;
	}

	/*	Get overrides from config file.				*/

	configFile = iopen(configFileName, O_RDONLY, 0777);
	if (configFile < 0)
	{
		if (errno == ENOENT)	/*	No overrides apply.	*/
		{
			writeMemo("[i] admin pgm using default SDR parms.");
			printIonParms(parms);
			return 0;
		}

		isprintf(buffer, sizeof buffer, "[?] admin pgm can't open SDR \
config file '%.255s': %.64s", configFileName, system_error_msg());
		writeMemo(buffer);
		return -1;
	}

	isprintf(buffer, sizeof buffer, "[i] admin pgm using SDR parm \
overrides from %.255s.", configFileName);
	writeMemo(buffer);
	lineNbr = 0;
	while (1)
	{
		if (igets(configFile, line, sizeof line, &lineLength) == NULL)
		{
			if (lineLength == 0)
			{
				result = 0;
				printIonParms(parms);
			}
			else
			{
				result = -1;
				writeErrMemo("admin pgm SDR config file igets \
failed");
			}

			break;			/*	Done.		*/
		}

		lineNbr++;
		if (lineLength < 1)
		{
			continue;		/*	Empty line.	*/
		}

		if (line[0] == '#')		/*	Comment only.	*/
		{
			continue;
		}

		tokenCount = 0;
		for (cursor = line, i = 0; i < 2; i++)
		{
			if (*cursor == '\0')
			{
				tokens[i] = NULL;
			}
			else
			{
				findToken((char **) &cursor, &(tokens[i]));
				tokenCount++;
			}
		}

		if (tokenCount != 2)
		{
			isprintf(buffer, sizeof buffer, "[?] incomplete SDR \
configuration file line (%d).", lineNbr);
			writeMemo(buffer);
			result = -1;
			break;
		}

		if (strcmp(tokens[0], "wmKey") == 0)
		{
			parms->wmKey = atoi(tokens[1]);
			continue;
		}

		if (strcmp(tokens[0], "wmSize") == 0)
		{
			parms->wmSize = atoi(tokens[1]);
			continue;
		}

		if (strcmp(tokens[0], "wmAddress") == 0)
		{
			parms->wmAddress = (char *) atol(tokens[1]);
			continue;
		}

		if (strcmp(tokens[0], "sdrName") == 0)
		{
			istrcpy(parms->sdrName, tokens[1],
					sizeof(parms->sdrName));
			continue;
		}

		if (strcmp(tokens[0], "configFlags") == 0)
		{
			parms->configFlags = atoi(tokens[1]);
			continue;
		}

		if (strcmp(tokens[0], "heapWords") == 0)
		{
			parms->heapWords = atoi(tokens[1]);
			continue;
		}

		if (strcmp(tokens[0], "heapKey") == 0)
		{
			parms->heapKey = atoi(tokens[1]);
			continue;
		}

		if (strcmp(tokens[0], "pathName") == 0)
		{
			istrcpy(parms->pathName, tokens[1],
					sizeof(parms->pathName));
			continue;
		}

		isprintf(buffer, sizeof buffer, "[?] unknown SDR config \
keyword '%.32s' at line %d.", tokens[0], lineNbr);
		writeMemo(buffer);
		result = -1;
		break;
	}

	close(configFile);
	return result;
}

void	printIonParms(IonParms *parms)
{
	char	buffer[512];

	CHKVOID(parms);
	isprintf(buffer, sizeof buffer, "wmKey:           %d",
			parms->wmKey);
	writeMemo(buffer);
	isprintf(buffer, sizeof buffer, "wmSize:          %ld",
			parms->wmSize);
	writeMemo(buffer);
	isprintf(buffer, sizeof buffer, "wmAddress:       %0lx",
			(unsigned long) parms->wmAddress);
	writeMemo(buffer);
	isprintf(buffer, sizeof buffer, "sdrName:        '%s'",
			parms->sdrName);
	writeMemo(buffer);
	isprintf(buffer, sizeof buffer, "configFlags:     %d",
		       parms->configFlags);
	writeMemo(buffer);
	isprintf(buffer, sizeof buffer, "heapWords:       %ld",
			parms->heapWords);
	writeMemo(buffer);
	isprintf(buffer, sizeof buffer, "heapKey:         %d",
			parms->heapKey);
	writeMemo(buffer);
	isprintf(buffer, sizeof buffer, "pathName:       '%.256s'",
			parms->pathName);
	writeMemo(buffer);
}

/*	*	*	Portable alarm functions	*	*	*/

#ifndef uClibc
static void	*alarmMain(void *parm)
{
	IonAlarm	*alarm = (IonAlarm *) parm;
	pthread_mutex_t	mutex;
	pthread_cond_t	cv;
	struct timeval	workTime;
	struct timespec	deadline;
	int		result;

	if (alarm->cycles == 0)
	{
		alarm->cycles -= 1;	/*	Underflow to max uint.	*/
	}

	memset((char *) &mutex, 0, sizeof mutex);
	if (pthread_mutex_init(&mutex, NULL))
	{
		putSysErrmsg("Can't start alarm, mutex init failed", NULL);
		return NULL;
	}

	memset((char *) &cv, 0, sizeof cv);
	if (pthread_cond_init(&cv, NULL))
	{
		putSysErrmsg("Can't start alarm, cond init failed", NULL);
		return NULL;
	}

	while (alarm->cycles > 0)
	{
		getCurrentTime(&workTime);
		deadline.tv_sec = workTime.tv_sec + alarm->term;
		deadline.tv_nsec = workTime.tv_usec * 1000;
		oK(pthread_mutex_lock(&mutex));
		result = pthread_cond_timedwait(&cv, &mutex, &deadline);
		pthread_mutex_unlock(&mutex);
		if (result != ETIMEDOUT)
		{
			putSysErrmsg("Alarm failure", NULL);
			break;
		}

		if ((alarm->proceed)(alarm->userData) < 0)
		{
			break;
		}

		alarm->cycles -= 1;
	}

	pthread_mutex_destroy(&mutex);
	pthread_cond_destroy(&cv);
	return NULL;
}
#endif

void	ionSetAlarm(IonAlarm *alarm, pthread_t *alarmThread)
{
#ifndef uClibc
	if (pthread_begin(alarmThread, NULL, alarmMain, alarm) < 0)
	{
		putSysErrmsg("Can't set alarm", NULL);
	}
#endif
}

void	ionCancelAlarm(pthread_t alarmThread)
{
#ifndef uClibc
	pthread_end(alarmThread);
	pthread_join(alarmThread, NULL);
#endif
}

#ifdef mingw
void	ionNoteMainThread(char *procName)
{
	return;		/*	Just for compatibility.			*/
}

void	ionPauseMainThread(int seconds)
{
	sm_WaitForWakeup(seconds);
}

void	ionKillMainThread(char *procName)
{
	sm_Wakeup(GetCurrentProcessId());
}
#else
#define	PROC_NAME_LEN	16
#define	MAX_PROCS	16

typedef struct
{
	char		procName[PROC_NAME_LEN];
	pthread_t	mainThread;
} IonProc;

static pthread_t	_mainThread(char *procName)
{
	static IonProc	proc[MAX_PROCS + 1];
	static int	procCount = 0;
	int		i;

	for (i = 0; i < procCount; i++)
	{
		if (strcmp(proc[i].procName, procName) == 0)
		{
			break;
		}
	}

	if (i == procCount)	/*	Registering new process.	*/
	{
		if (procCount == MAX_PROCS)
		{
			/*	Can't register process; return an
			 *	invalid value for mainThread.		*/

			return proc[MAX_PROCS].mainThread;
		}

		/*	Initial call to _mainThread for any process
		 *	must be from the main thread of that process.	*/

		procCount++;
		istrcpy(proc[i].procName, procName, PROC_NAME_LEN);
		proc[i].mainThread = pthread_self();
	}

	return proc[i].mainThread;
}

void	ionNoteMainThread(char *procName)
{
	CHKVOID(procName);
	oK(_mainThread(procName));
}

void	ionPauseMainThread(int seconds)
{
	if (seconds < 0)
	{
		seconds = 2000000000;
	}

	snooze(seconds);
}

void	ionKillMainThread(char *procName)
{
	pthread_t	mainThread;

	CHKVOID(procName);
       	mainThread = _mainThread(procName);
	if (!pthread_equal(mainThread, pthread_self()))
	{
		pthread_kill(mainThread, SIGTERM);
	}
}
#endif
