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
#include "smlist.h"
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
static void	ionProvideZcoSpace(ZcoAcct acct);

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
	static uaddr		ionSmId = 0;
	static PsmView		ionWorkingMemory;
	static PsmPartition	ionwm = NULL;
	static int		memmgrIdx;
	static MemAllocator	wmtake = allocFromIonMemory;
	static MemDeallocator	wmrelease = releaseToIonMemory;
	static MemAtoPConverter	wmatop = ionMemAtoP;
	static MemPtoAConverter	wmptoa = ionMemPtoA;

	if (parms)
	{
		if (parms->wmName == NULL)	/*	Destroy.	*/
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

void	*allocFromIonMemory(const char *fileName, int lineNbr, size_t length)
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

void	releaseToIonMemory(const char *fileName, int lineNbr, void *block)
{
	PsmPartition	ionwm = _ionwm(NULL);

	Psm_free(fileName, lineNbr, ionwm, psa(ionwm, (char *) block));
#ifdef HAVE_VALGRIND_VALGRIND_H
    VALGRIND_FREELIKE_BLOCK(block, 0);
#endif
}

void	*ionMemAtoP(uaddr address)
{
	return (void *) psp(_ionwm(NULL), address);
}

uaddr	ionMemPtoA(void *pointer)
{
	return (uaddr) psa(_ionwm(NULL), pointer);
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
		if ((vdb->nodes = sm_rbt_create(ionwm)) == 0
		|| (vdb->neighbors = sm_rbt_create(ionwm)) == 0
		|| (vdb->contactIndex = sm_rbt_create(ionwm)) == 0
		|| (vdb->rangeIndex = sm_rbt_create(ionwm)) == 0
		|| (vdb->timeline = sm_rbt_create(ionwm)) == 0
		|| (vdb->probes = sm_list_create(ionwm)) == 0
		|| (vdb->requisitions[0] = sm_list_create(ionwm)) == 0
		|| (vdb->requisitions[1] = sm_list_create(ionwm)) == 0
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
	time_t			currentTime = getCtime();
	char			timestampBuffer[20];
	int			textLen;
	static char		msgbuf[256];

	if (text == NULL) return;
	if (*text == '\0')	/*	Claims that log file is closed.	*/
	{
		if (ionLogFile != -1)
		{
			close(ionLogFile);	/*	To be sure.	*/
			ionLogFile = -1;
		}

		return;		/*	Ignore zero-length memo.	*/
	}

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
#if defined(bionic)
			isprintf(ionLogFileName, sizeof ionLogFileName,
					"%.255s%c..%cion.log",
					getIonWorkingDirectory(),
					ION_PATH_DELIMITER,
					ION_PATH_DELIMITER);
#else
			isprintf(ionLogFileName, sizeof ionLogFileName,
					"%.255s%cion.log",
					getIonWorkingDirectory(),
					ION_PATH_DELIMITER);
#endif
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

#if defined (FSWWATCHER)
#include "fswwatcher.c"
#elif defined (GDSWATCHER)
#include "gdswatcher.c"
#else
static void	ionRedirectWatchCharacters()
{
	setWatcher(NULL);		/*	Defaults to stdout.	*/
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

int	ionInitialize(IonParms *parms, uvast ownNodeNbr)
{
	char		wdname[256];
	Sdr		ionsdr;
	Object		iondbObject;
	IonDB		iondbBuf;
	double		limit;
	sm_WmParms	ionwmParms;
	char		*ionvdbName = _ionvdbName();
	ZcoCallback	notify = ionProvideZcoSpace;

	CHKERR(parms);
	CHKERR(ownNodeNbr);
#ifdef mingw
	if (_winsock(0) < 0)
	{
		return -1;
	}
#endif
	if (igetcwd(wdname, 256) == NULL)
	{
		putErrmsg("Can't get cwd name.", NULL);
		return -1;
	}

	if (parms->sdrWmSize <= 0)
	{
		parms->sdrWmSize = 1000000;	/*	Default.	*/
	}

	if (checkNodeListParms(parms, wdname, ownNodeNbr) < 0)
	{
		putErrmsg("Failed checking node list parms.", NULL);
		return -1;
	}

	if (sdr_initialize(parms->sdrWmSize, NULL, SM_NO_KEY, NULL) < 0)
	{
		putErrmsg("Can't initialize the SDR system.", NULL);
		return -1;
	}

	if (sdr_load_profile(parms->sdrName, parms->configFlags,
			parms->heapWords, parms->heapKey, parms->logSize,
			parms->logKey, parms->pathName, "ionrestart") < 0)
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
		iondbBuf.regions[0].regionNbr = -1;	/*	None.	*/
		iondbBuf.regions[0].members = sdr_list_create(ionsdr);
		iondbBuf.regions[0].contacts = sdr_list_create(ionsdr);
		iondbBuf.regions[1].regionNbr = -1;	/*	None.	*/
		iondbBuf.regions[1].members = sdr_list_create(ionsdr);
		iondbBuf.regions[1].contacts = sdr_list_create(ionsdr);
		iondbBuf.ranges = sdr_list_create(ionsdr);
		iondbBuf.productionRate = -1;	/*	Unknown.	*/
		iondbBuf.consumptionRate = -1;	/*	Unknown.	*/
		limit = (sdr_heap_size(ionsdr) / 100) * (100 - ION_SEQUESTERED);

		/*	By default, let outbound ZCOs occupy up to
		 *	half of the available heap space, leaving
		 *	the other half for inbound ZCO acquisition.	*/

		zco_set_max_heap_occupancy(ionsdr, limit/2, ZcoInbound);
		zco_set_max_heap_occupancy(ionsdr, limit/2, ZcoOutbound);

		/*	By default, the occupancy ceiling is 50% more
		 *	than the outbound ZCO allocation.		*/

		iondbBuf.occupancyCeiling = zco_get_max_file_occupancy(ionsdr,
				ZcoOutbound);
		iondbBuf.occupancyCeiling += (limit/4);
		iondbBuf.maxClockError = 1;
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

		/*	Set initial home region.			*/

		break;

	default:		/*	Found DB in the SDR.		*/
		sdr_exit_xn(ionsdr);
		writeMemo("[?] Attempting duplicate node initialization.");
		return -1;
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
	ionRedirectWatchCharacters();
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

	sm_list_destroy(partition, node->embargoes, rfx_erase_data, NULL);
	psm_free(partition, eltData);
}

static void	destroyNeighbor(PsmPartition partition, PsmAddress nodeData,
		void *argument)
{
	psm_free(partition, nodeData);
}

static void	dropVdb(PsmPartition wm, PsmAddress vdbAddress)
{
	IonVdb		*vdb;
	int		i;
	PsmAddress	elt;
	PsmAddress	nextElt;
	PsmAddress	addr;
	Requisition	*req;

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
	sm_rbt_destroy(wm, vdb->neighbors, destroyNeighbor, NULL);

	/*	Safely shut down the ZCO flow control system.		*/

	for (i = 0; i < 1; i++)
	{
		for (elt = sm_list_first(wm, vdb->requisitions[i]); elt;
				elt = nextElt)
		{
			nextElt = sm_list_next(wm, elt);
			addr = sm_list_data(wm, elt);
			req = (Requisition *) psp(wm, addr);
			if (req->semaphore != SM_SEM_NONE)
			{
				sm_SemEnd(req->semaphore);
			}

			psm_free(wm, addr);
			sm_list_delete(wm, elt, NULL, NULL);
		}
	}

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

void	ionRaiseVdb()				/*	For ionrestart.	*/
{
	char	*ionvdbName = _ionvdbName();

	if (_ionvdb(&ionvdbName) == NULL)	/*	Create new Vdb.	*/
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
	ZcoCallback	notify = ionProvideZcoSpace;

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
	ionRedirectWatchCharacters();
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
#if defined (ION_LWT)
#ifdef RTEMS
	sm_TaskForget(sm_TaskIdSelf());
#endif
	return;
#else	/*	Not ION_LWT, so can detach entire process.		*/
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
#endif	/*	end of #ifdef ION_LWT					*/
}

void	ionProd(uvast fromNode, uvast toNode, size_t xmitRate,
		unsigned int owlt)
{
	Sdr		ionsdr = _ionsdr(NULL);
	time_t		fromTime;
	time_t		toTime;
	char		textbuf[RFX_NOTE_LEN];
	PsmAddress	xaddr;

	if (ionsdr == NULL)
	{
		if (ionAttach() < 0)
		{
			writeMemo("[?] ionProd: node not initialized yet.");
			return;
		}
	}

	fromTime = getCtime();		/*	The current time.	*/
	toTime = fromTime + 14400;	/*	Four hours later.	*/
	if (rfx_insert_range(fromTime, toTime, fromNode, toNode, owlt,
			&xaddr) < 0 || xaddr == 0)
	{
		writeMemoNote("[?] ionProd: range insertion failed.",
				utoa(owlt));
		return;
	}

	writeMemo("ionProd: range inserted.");
	writeMemo(rfx_print_range(xaddr, textbuf));
	if (rfx_insert_contact(0, fromTime, toTime, fromNode, toNode, xmitRate,
			1.0, &xaddr) < 0 || xaddr == 0)
	{
		writeMemoNote("[?] ionProd: contact insertion failed.",
				utoa(xmitRate));
		return;
	}

	writeMemo("ionProd: contact inserted.");
	writeMemo(rfx_print_contact(xaddr, textbuf));
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
	ionwmParms.wmSize = 0;
	ionwmParms.wmAddress = NULL;
	ionwmParms.wmName = NULL;
	oK(_ionwm(&ionwmParms));
	oK(_ionvdb(&ionvdbName));
}

/*	Functions for managing region membership.			*/

int	ionPickRegion(vast regionNbr)
{
	Sdr	sdr = getIonsdr();
	Object	iondbObj;
	IonDB	iondb;
	int	i;

	if (regionNbr < 0)
	{
		return 2;	/*	Null region membership.		*/
	}

	iondbObj = getIonDbObject();
	CHKERR(iondbObj);
	sdr_read(sdr, (char *) &iondb, iondbObj, sizeof(IonDB));
	for (i = 0; i < 2; i++)
	{
		if (iondb.regions[i].regionNbr == regionNbr)
		{
			break;
		}
	}

	return i;
}

int	ionRegionOf(uvast nodeA, uvast nodeB)
{
	/*	This function determines the region in which nodeA
	 *	and nodeB both reside; if nodeB is zero, it just
	 *	determines the region in which nodeA resides.  If
	 *	we find the node(s) in both regions, the home region
	 *	is preferred.						*/

	Sdr	sdr = getIonsdr();
	Object	iondbObj;
	IonDB	iondb;
	int	regionMaskA = 0;
	int	regionMaskB = (nodeB == 0 ? 3 : 0);
	int	i;
	Object	elt;
	Object	addr;
		OBJ_POINTER(RegionMember, member);

	CHKERR(nodeA > 0);
	iondbObj = getIonDbObject();
	CHKERR(iondbObj);
	sdr_read(sdr, (char *) &iondb, iondbObj, sizeof(IonDB));
	for (i = 0; i < 2; i++)
	{
		for (elt = sdr_list_first(sdr, iondb.regions[i].members); elt;
			       elt = sdr_list_next(sdr, elt))
		{
			addr = sdr_list_data(sdr, elt);
			GET_OBJ_POINTER(sdr, RegionMember, member, addr);
			if (member->nodeNbr == nodeA)
			{
				regionMaskA |= (i + 1);
			}

			if (member->nodeNbr == nodeB)
			{
				regionMaskB |= (i + 1);
			}
		}
	}

	/*	Identify the common region.				*/

	i = (regionMaskA & regionMaskB) - 1;
	if (i == 2)	/*	Both; shouldn't happen.			*/
	{
		i = 0;	/*	Choose the home region.			*/
	}

	return i;	/*	May be -1 meaning "No common region".	*/
}

static void	leaveRegion(IonRegion *region)
{
	Sdr		sdr = getIonsdr();
	Object		elt;
	Object		obj;
	IonContact	contact;

	/*	Forget the node membership of the region.		*/

	while (1)
	{
		elt = sdr_list_first(sdr, region->members);
		if (elt == 0)
		{
			break;
		}

		sdr_free(sdr, sdr_list_data(sdr, elt));
		sdr_list_delete(sdr, elt, NULL, NULL);
	}

	/*	Forget the contact plan for the region.			*/

	while (1)
	{
		elt = sdr_list_first(sdr, region->contacts);
		if (elt == 0)
		{
			break;
		}

		obj = sdr_list_data(sdr, elt);
		sdr_read(sdr, (char *) &contact, obj, sizeof(IonContact));
		oK(rfx_remove_contact(&contact.fromTime, contact.fromNode,
				contact.toNode));

		/*	rfx_remove_contact deletes the contact from
		 *	both the volatile and non-volatile databases.
		 *	No need to do further deletion here.		*/
	}

	/*	Reinitialize.						*/

	region->regionNbr = -1;
}

static void	ionNoteNonMember(int regionIdx, uvast nodeNbr)
{
	Sdr		sdr = getIonsdr();
	Object		iondbObj = getIonDbObject();
	IonDB		iondb;
	Object		elt;
	Object		memberObj;
	RegionMember	member;

	sdr_read(sdr, (char *) &iondb, iondbObj, sizeof(IonDB));
	for (elt = sdr_list_first(sdr, iondb.regions[regionIdx].members); elt;
			elt = sdr_list_next(sdr, elt))
	{
		memberObj = sdr_list_data(sdr, elt);
		sdr_read(sdr, (char *) &member, memberObj,
				sizeof(RegionMember));
		if (member.nodeNbr == nodeNbr)
		{
			sdr_free(sdr, memberObj);
			sdr_list_delete(sdr, elt, NULL, NULL);
			return;
		}
	}
}

void	ionNoteMember(int regionIdx, uvast nodeNbr, vast homeRegionNbr,
		vast outerRegionNbr)
{
	Sdr		sdr = getIonsdr();
	Object		iondbObj = getIonDbObject();
	IonDB		iondb;
	Object		elt;
	Object		memberObj;
	RegionMember	member;
	uvast		otherRegionNbr;

	sdr_read(sdr, (char *) &iondb, iondbObj, sizeof(IonDB));
	for (elt = sdr_list_first(sdr, iondb.regions[regionIdx].members); elt;
			elt = sdr_list_next(sdr, elt))
	{
		memberObj = sdr_list_data(sdr, elt);
		sdr_read(sdr, (char *) &member, memberObj,
				sizeof(RegionMember));
		if (member.nodeNbr == nodeNbr)
		{
			break;
		}
	}

	if (elt == 0)	/*	Not in membership list.			*/
	{
		member.nodeNbr = nodeNbr;
		member.homeRegionNbr = homeRegionNbr;
		member.outerRegionNbr = outerRegionNbr;
		memberObj = sdr_malloc(sdr, sizeof(RegionMember));
		if (memberObj)
		{
			sdr_write(sdr, memberObj, (char *) &member,
					sizeof(RegionMember));
			sdr_list_insert_last(sdr,
					iondb.regions[regionIdx].members,
					memberObj);
		}
	}
	else		/*	A known member of this region.		*/
	{
		if (member.homeRegionNbr != homeRegionNbr
		|| member.outerRegionNbr != outerRegionNbr)
		{
			member.homeRegionNbr = homeRegionNbr;
			member.outerRegionNbr = outerRegionNbr;
			sdr_write(sdr, memberObj, (char *) &member,
					sizeof(RegionMember));
		}
	}

	/*	Remove from other region if necessary.			*/

	otherRegionNbr = iondb.regions[1 - regionIdx].regionNbr;
	if (homeRegionNbr != otherRegionNbr && outerRegionNbr != otherRegionNbr)
	{
		ionNoteNonMember(1 - regionIdx, nodeNbr);
	}
}

int	ionManageRegion(int idx, vast regionNbr)
{
	Sdr		sdr = getIonsdr();
	Object		iondbObj;
	IonDB		iondb;
	IonRegion	*region;
	RegionMember	member;
	Object		memberObj;

	CHKERR(idx == 0 || idx == 1);
	iondbObj = getIonDbObject();
	CHKERR(sdr_begin_xn(sdr));
	sdr_stage(sdr, (char *) &iondb, iondbObj, sizeof(IonDB));
	region = &(iondb.regions[idx]);
	if (regionNbr < 0)		/*	Removal from region.	*/
	{
		if (idx == 0)	/*	Trying to leave home region.	*/
		{
			sdr_exit_xn(sdr);
			writeMemo("[?] Tried to leave network, not supported.");
			return 0;
		}

		/*	Node is ceasing to be a passageway to its
		 *	outer region.					*/

		leaveRegion(region);
		sdr_write(sdr, iondbObj, (char *) &iondb, sizeof(IonDB));
		return sdr_end_xn(sdr);
	}

	/*	Node is joining a new region.				*/

	if (ionPickRegion(regionNbr) < 2)
	{
		/*	Node already resides in the indicated region.	*/

		sdr_exit_xn(sdr);
		writeMemo("[?] Tried to join a region the node is already in.");
		return 0;
	}

	if (region->regionNbr != -1)	/*	Region already defined.	*/
	{
		/*	Must leave old region first.			*/

		leaveRegion(region);
	}

	region->regionNbr = regionNbr;
	sdr_write(sdr, iondbObj, (char *) &iondb, sizeof(IonDB));

	/*	Node must be inserted into region's membership.		*/

	member.nodeNbr = getOwnNodeNbr();
	member.homeRegionNbr = iondb.regions[0].regionNbr;
	member.outerRegionNbr = iondb.regions[1].regionNbr;
	memberObj = sdr_malloc(sdr, sizeof(RegionMember));
	if (memberObj)
	{
		sdr_write(sdr, memberObj, (char *) &member,
				sizeof(RegionMember));
		oK(sdr_list_insert_last(sdr, region->members, memberObj));
	}

	return sdr_end_xn(sdr);
}

int	ionManagePassageway(uvast nodeNbr, vast homeRegionNbr,
		vast outerRegionNbr)
{
	Sdr	sdr = getIonsdr();
	int	regionIdx;

	if (homeRegionNbr == -1)	/*	Forget this node.	*/
	{
		CHKERR(sdr_begin_xn(sdr));
		ionNoteNonMember(0, nodeNbr);
		ionNoteNonMember(1, nodeNbr);
		return sdr_end_xn(sdr);
	}

	if (outerRegionNbr == -1)	/*	No longer a passageway.	*/
	{
		regionIdx = ionPickRegion(outerRegionNbr);
		if (regionIdx >= 0 && regionIdx <= 1)
		{
			/*	Node's former outer region is the
			 *	indicated region (idx = home or outer)
			 *	of the local node.			*/

			CHKERR(sdr_begin_xn(sdr));
			ionNoteNonMember(regionIdx, nodeNbr);
			if (sdr_end_xn(sdr) < 0)
			{
				putErrmsg("Can't update passageway.", NULL);
			}
		}
	}

	/*	Insert node into the correct region(s) according
	 *	to its stated new home and outer region numbers,
	 *	removing it from other regions as necessary.		*/

	regionIdx = ionPickRegion(homeRegionNbr);
	if (regionIdx >= 0 && regionIdx <= 1)
	{
		/*	Passageway's home region is the
		 *	indicated region (idx = home or outer)
		 *	of the local node.			*/

		CHKERR(sdr_begin_xn(sdr));
		ionNoteMember(regionIdx, nodeNbr, homeRegionNbr,
				outerRegionNbr);
		if (sdr_end_xn(sdr) < 0)
		{
			putErrmsg("Can't update passageway.", NULL);
		}
	}

	regionIdx = ionPickRegion(outerRegionNbr);
	if (regionIdx >= 0 && regionIdx <= 1)
	{
		/*	Passageway's outer region is the
		 *	indicated region (idx = home or outer)
		 *	of the local node.			*/

		CHKERR(sdr_begin_xn(sdr));
		ionNoteMember(regionIdx, nodeNbr, homeRegionNbr,
				outerRegionNbr);
		if (sdr_end_xn(sdr) < 0)
		{
			putErrmsg("Can't update passageway.", NULL);
		}
	}

	return 0;
}

/*	Utility functions.						*/

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

int	startIonMemTrace(size_t size)
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

time_t	getCtime()
{
	IonVdb	*ionvdb = _ionvdb(NULL);
	int	delta = ionvdb ? ionvdb->deltaFromUTC : 0;
	time_t	ctime;
#if defined(FSWCLOCK)
#include "fswctime.c"
#else
	ctime = time(NULL);
#endif
	return ctime - delta;
}

static time_t	readTimestamp(char *timestampBuffer, time_t referenceTime,
			int timestampIsUTC)
{
	long		interval = 0;
	time_t		result;
	struct tm	ts;
	int		count;

	if (timestampBuffer == NULL)
	{
		return 0;
	}

	if (*timestampBuffer == '+')	/*	Relative time.		*/
	{
		interval = strtol(timestampBuffer + 1, NULL, 0);
		result = referenceTime + interval;
		if (result < 0 || result > MAX_POSIX_TIME)
		{
			putErrmsg("Time value not supported (must be before \
19 January 2038).", timestampBuffer);
			return 0;
		}

		return result;
	}

	memset((char *) &ts, 0, sizeof ts);
	count = sscanf(timestampBuffer, timestampInFormat, &ts.tm_year,
		&ts.tm_mon, &ts.tm_mday, &ts.tm_hour, &ts.tm_min, &ts.tm_sec);
	if (count != 6)
	{
		putErrmsg("Timestamp format invalid.", timestampBuffer);
		return 0;
	}

	ts.tm_year -= 1900;
	ts.tm_mon -= 1;
	ts.tm_isdst = 0;	/*	Default is UTC.			*/
#ifndef VXWORKS
#ifdef mingw
	_tzset();	/*	Need to orient mktime properly.		*/
#else
	tzset();	/*	Need to orient mktime properly.		*/
#endif
	if (timestampIsUTC)
	{
		/*	Must convert UTC to local time for mktime.	*/

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
	result = mktime(&ts);
	if (result < 0 || result > MAX_POSIX_TIME)
	{
		putErrmsg("Time value not supported (must be before 19 January \
2038).", timestampBuffer);
		return 0;
	}

	return result;
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
#if defined (mingw)
	struct tm	*ts;
#else
	struct tm	tsbuf;
	struct tm	*ts = &tsbuf;
#endif

	CHKVOID(timestampBuffer);
#if defined (mingw)
	ts = localtime(&timestamp);
#else
	oK(localtime_r(&timestamp, &tsbuf));
#endif
	isprintf(timestampBuffer, 20, timestampOutFormat,
			ts->tm_year + 1900, ts->tm_mon + 1, ts->tm_mday,
			ts->tm_hour, ts->tm_min, ts->tm_sec);
}

void	writeTimestampUTC(time_t timestamp, char *timestampBuffer)
{
	struct tm	tsbuf;
	struct tm	*ts = &tsbuf;
	
	CHKVOID(timestampBuffer);
#if defined (mingw)
	ts = gmtime(&timestamp);
	oK(ts);
#else
	oK(gmtime_r(&timestamp, &tsbuf));
#endif
	isprintf(timestampBuffer, 20, timestampOutFormat,
			ts->tm_year + 1900, ts->tm_mon + 1, ts->tm_mday,
			ts->tm_hour, ts->tm_min, ts->tm_sec);
}

time_t	ionReferenceTime(time_t *newValue)
{
	IonVdb	*vdb = getIonVdb();

	CHKZERO(vdb);
	if (newValue)
	{
		vdb->refTime = *newValue;
	}

	return vdb->refTime;
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
	uvast	size;

	/*	Set defaults.						*/

	CHKERR(parms);
	memset((char *) parms, 0, sizeof(IonParms));
	parms->wmSize = 5000000;
	parms->wmAddress = 0;		/*	Dyamically allocated.	*/
	parms->configFlags = SDR_IN_DRAM | SDR_REVERSIBLE | SDR_BOUNDED;
	parms->heapWords = 250000;
	parms->heapKey = SM_NO_KEY;
	parms->logSize = 0;		/*	Log is in file.		*/
	parms->logKey = SM_NO_KEY;
	istrcpy(parms->pathName, "/tmp", sizeof parms->pathName);

	/*	Determine name of config file.				*/

	if (configFileName == NULL || *configFileName == 0)
	{
		writeMemo("[i] admin pgm using default SDR parms.");
		printIonParms(parms);
		return 0;
	}

	if (strcmp(configFileName, ".") == 0)
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
			size = strtouvast(tokens[1]);
			parms->wmSize = size;
			if (parms->wmSize != size)
			{
				size = parms->wmSize;
				isprintf(buffer, sizeof buffer, "[?] wmSize \
too large for this architecture, would have been truncated to " \
UVAST_FIELDSPEC ".", size);
				writeMemo(buffer);
				result = -1;
				break;
			}

			continue;
		}

		if (strcmp(tokens[0], "wmAddress") == 0)
		{
			parms->wmAddress = (char *) strtoaddr(tokens[1]);
			continue;
		}

		if (strcmp(tokens[0], "sdrName") == 0)
		{
			istrcpy(parms->sdrName, tokens[1],
					sizeof(parms->sdrName));
			continue;
		}

		if (strcmp(tokens[0], "sdrWmSize") == 0)
		{
			size = strtouvast(tokens[1]);
			parms->sdrWmSize = size;
			if (parms->sdrWmSize != size)
			{
				size = parms->sdrWmSize;
				isprintf(buffer, sizeof buffer, "[?] sdrWmSize \
too large for this architecture, would have been truncated to " \
UVAST_FIELDSPEC ".", size);
				writeMemo(buffer);
				result = -1;
				break;
			}

			continue;
		}

		if (strcmp(tokens[0], "configFlags") == 0)
		{
			parms->configFlags = atoi(tokens[1]);
			continue;
		}

		if (strcmp(tokens[0], "heapWords") == 0)
		{
			size = strtouvast(tokens[1]);
			parms->heapWords = size;
			if (parms->heapWords != size)
			{
				size = parms->heapWords;
				isprintf(buffer, sizeof buffer, "[?] heapWords \
too large for this architecture, would have been truncated to " \
UVAST_FIELDSPEC ".", size);
				writeMemo(buffer);
				result = -1;
				break;
			}

			continue;
		}

		if (strcmp(tokens[0], "heapKey") == 0)
		{
			parms->heapKey = atoi(tokens[1]);
			continue;
		}

		if (strcmp(tokens[0], "logSize") == 0)
		{
			size = strtouvast(tokens[1]);
			parms->logSize = size;
			if (parms->logSize != size)
			{
				size = parms->logSize;
				isprintf(buffer, sizeof buffer, "[?] logSize \
too large for this architecture, would have been truncated to " \
UVAST_FIELDSPEC ".", size);
				writeMemo(buffer);
				result = -1;
				break;
			}

			continue;
		}

		if (strcmp(tokens[0], "logKey") == 0)
		{
			parms->logKey = atoi(tokens[1]);
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
#if (SPACE_ORDER > 2 && defined(mingw))
	isprintf(buffer, sizeof buffer, "wmAddress:       %#I64x",
			(uaddr) (parms->wmAddress));
#else
	isprintf(buffer, sizeof buffer, "wmAddress:       %#lx",
			(uaddr) (parms->wmAddress));
#endif
	writeMemo(buffer);
	isprintf(buffer, sizeof buffer, "sdrName:        '%s'",
			parms->sdrName);
	writeMemo(buffer);
	isprintf(buffer, sizeof buffer, "sdrWmSize:       %ld",
			parms->sdrWmSize);
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
	isprintf(buffer, sizeof buffer, "logSize:         %d",
			parms->logSize);
	writeMemo(buffer);
	isprintf(buffer, sizeof buffer, "logKey:          %d",
			parms->logKey);
	writeMemo(buffer);
	isprintf(buffer, sizeof buffer, "pathName:       '%.256s'",
			parms->pathName);
	writeMemo(buffer);
}

/*	Functions for signaling the main threads of processes.	*	*/

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
		seconds = 1000000000;	/*	About 32 years.		*/
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

/*	Functions for flow-controlled ZCO space management.		*/

int	ionStartAttendant(ReqAttendant *attendant)
{
	CHKERR(attendant);
	attendant->semaphore = sm_SemCreate(SM_NO_KEY, SM_SEM_FIFO);
	return (attendant->semaphore == SM_SEM_NONE ? -1 : 0);
}

void	ionPauseAttendant(ReqAttendant *attendant)
{
	CHKVOID(attendant);
	sm_SemEnd(attendant->semaphore);
}

void	ionResumeAttendant(ReqAttendant *attendant)
{
	CHKVOID(attendant);
	sm_SemUnend(attendant->semaphore);
	sm_SemGive(attendant->semaphore);
}

void	ionStopAttendant(ReqAttendant *attendant)
{
	CHKVOID(attendant);
	sm_SemEnd(attendant->semaphore);
	microsnooze(50000);
	sm_SemDelete(attendant->semaphore);
}

void	ionShred(ReqTicket ticket)
{
	Sdr		sdr = getIonsdr();
	PsmPartition	ionwm = getIonwm();

	/*	Ticket is address of an sm_list element in a shared
	 *	memory list of requisitions in the IonVdb.		*/

	if (ticket == 0)
	{
		return;	/*	ZCO space request refused, not queued.	*/
	}

	CHKVOID(sdr_begin_xn(sdr));	/*	Must be atomic.		*/
	psm_free(ionwm, sm_list_data(ionwm, ticket));
	sm_list_delete(ionwm, ticket, NULL, NULL);
	sdr_exit_xn(sdr);	/*	End of critical section.	*/
}

int	ionRequestZcoSpace(ZcoAcct acct, vast fileSpaceNeeded,
			vast bulkSpaceNeeded, vast heapSpaceNeeded,
			unsigned char coarsePriority,
			unsigned char finePriority,
			ReqAttendant *attendant, ReqTicket *ticket)
{
	Sdr		sdr = getIonsdr();
	PsmPartition	ionwm = getIonwm();
	IonVdb		*vdb = getIonVdb();
	PsmAddress	reqAddr;
	Requisition	*req;
	PsmAddress	elt;
	PsmAddress	oldReqAddr;
	Requisition	*oldReq;

	CHKERR(acct == ZcoInbound || acct == ZcoOutbound);
	CHKERR(fileSpaceNeeded >= 0);
	CHKERR(bulkSpaceNeeded >= 0);
	CHKERR(heapSpaceNeeded >= 0);
	CHKERR(ticket);
	CHKERR(vdb);
	*ticket = 0;			/*	Default: refused.	*/
	oK(sdr_begin_xn(sdr));		/*	Just to lock memory.	*/
	reqAddr = psm_zalloc(ionwm, sizeof(Requisition));
	if (reqAddr == 0)
	{
		sdr_exit_xn(sdr);
		putErrmsg("Can't create ZCO space requisition.", NULL);
		return -1;
	}

	req = (Requisition *) psp(ionwm, reqAddr);
	req->fileSpaceNeeded = fileSpaceNeeded;
	req->bulkSpaceNeeded = bulkSpaceNeeded;
	req->heapSpaceNeeded = heapSpaceNeeded;
	if (attendant)
	{
		req->semaphore = attendant->semaphore;
	}
	else
	{
		req->semaphore = SM_SEM_NONE;
	}

	req->secondsUnclaimed = -1;	/*	Not yet serviced.	*/
	req->coarsePriority = coarsePriority;
	req->finePriority = finePriority;
	for (elt = sm_list_last(ionwm, vdb->requisitions[acct]); elt;
			elt = sm_list_prev(ionwm, elt))
	{
		oldReqAddr = sm_list_data(ionwm, elt);
		oldReq = (Requisition *) psp(ionwm, oldReqAddr);
		if (oldReq->coarsePriority > req->coarsePriority)
		{
			break;		/*	Insert after this one.	*/
		}

		if (oldReq->coarsePriority < req->coarsePriority)
		{
			continue;	/*	Move toward the start.	*/
		}

		/*	Same coarse priority.				*/

		if (oldReq->finePriority > req->finePriority)
		{
			break;		/*	Insert after this one.	*/
		}

		if (oldReq->finePriority < req->finePriority)
		{
			continue;	/*	Move toward the start.	*/
		}

		/*	Same priority, so FIFO; insert after this one.	*/

		break;
	}

	if (elt)
	{
		*ticket = sm_list_insert_after(ionwm, elt, reqAddr);
	}
	else	/*	Higher priority than all other requisitions.	*/
	{
		*ticket = sm_list_insert_first(ionwm,
				vdb->requisitions[acct], reqAddr);
	}

	if (*ticket == 0)
	{
		psm_free(ionwm, reqAddr);
		sdr_exit_xn(sdr);
		putErrmsg("Can't put ZCO space requisition into list.", NULL);
		return -1;
	}

	sdr_exit_xn(sdr);		/*	Unlock memory.		*/

	/*	Try to service the request immediately.			*/

	ionProvideZcoSpace(acct);
	if (req->secondsUnclaimed < 0)
	{
		/*	Request can't be serviced at this time.		*/

		if (attendant)	/*	Willing to wait.		*/
		{
			/*	Ready attendant to wait for service.	*/

			sm_SemGive(attendant->semaphore);
			sm_SemTake(attendant->semaphore);
		}
		else		/*	Request is simply refused.	*/
		{
			ionShred(*ticket);
			*ticket = 0;
		}
	}

	return 0;
}

int	ionSpaceAwarded(ReqTicket ticket)
{
	PsmPartition	ionwm = getIonwm();
	PsmAddress	reqAddr;
	Requisition	*req;

	if (ticket == 0)
	{
		return 0;	/*	Request refused, not queued.	*/
	}

	reqAddr = sm_list_data(ionwm, ticket);
	req = (Requisition *) psp(ionwm, reqAddr);
	if (req->secondsUnclaimed < 0)
	{
		return 0;	/*	Still waiting for service.	*/
	}

	return 1;		/*	Request has been serviced.	*/
}

static void	ionProvideZcoSpace(ZcoAcct acct)
{
	Sdr		sdr = getIonsdr();
	PsmPartition	ionwm = getIonwm();
	IonVdb		*vdb = getIonVdb();
	double		maxFileOccupancy;
	double		maxBulkOccupancy;
	double		maxHeapOccupancy;
	double		currentFileOccupancy;
	double		currentBulkOccupancy;
	double		currentHeapOccupancy;
	double		totalFileSpaceAvbl;
	double		totalBulkSpaceAvbl;
	double		totalHeapSpaceAvbl;
	double		fileSpaceAvbl;
	double		bulkSpaceAvbl;
	double		heapSpaceAvbl;
	PsmAddress	elt;
	PsmAddress	reqAddr;
	Requisition	*req;

	CHKVOID(vdb);
	oK(sdr_begin_xn(sdr));		/*	Just to lock memory.	*/
	maxFileOccupancy = zco_get_max_file_occupancy(sdr, acct);
	maxBulkOccupancy = zco_get_max_bulk_occupancy(sdr, acct);
	maxHeapOccupancy = zco_get_max_heap_occupancy(sdr, acct);
	currentFileOccupancy = zco_get_file_occupancy(sdr, acct);
	currentBulkOccupancy = zco_get_bulk_occupancy(sdr, acct);
	currentHeapOccupancy = zco_get_heap_occupancy(sdr, acct);
	totalFileSpaceAvbl = maxFileOccupancy - currentFileOccupancy;
	totalBulkSpaceAvbl = maxBulkOccupancy - currentBulkOccupancy;
	totalHeapSpaceAvbl = maxHeapOccupancy - currentHeapOccupancy;
	for (elt = sm_list_first(ionwm, vdb->requisitions[acct]); elt;
			elt = sm_list_next(ionwm, elt))
	{
		reqAddr = sm_list_data(ionwm, elt);
		req = (Requisition *) psp(ionwm, reqAddr);
		if (req->secondsUnclaimed >= 0)
		{
			/*	This request has already been serviced.
			 *	The requested space has been reserved
			 *	for it, so that space is not available
			 *	for any other requests.			*/

			totalFileSpaceAvbl -= req->fileSpaceNeeded;
			totalBulkSpaceAvbl -= req->bulkSpaceNeeded;
			totalHeapSpaceAvbl -= req->heapSpaceNeeded;
			continue;	/*	Req already serviced.	*/
		}

		fileSpaceAvbl = totalFileSpaceAvbl;
		bulkSpaceAvbl = totalBulkSpaceAvbl;
		heapSpaceAvbl = totalHeapSpaceAvbl;
		if (fileSpaceAvbl < 0)
		{
			fileSpaceAvbl = 0;
		}

		if (bulkSpaceAvbl < 0)
		{
			bulkSpaceAvbl = 0;
		}

		if (heapSpaceAvbl < 0)
		{
			heapSpaceAvbl = 0;
		}

		if (fileSpaceAvbl < req->fileSpaceNeeded
		|| bulkSpaceAvbl < req->bulkSpaceNeeded
		|| heapSpaceAvbl < req->heapSpaceNeeded)
		{
			/*	Can't provide ZCO space to this
			 *	requisition at this time.  Other
			 *	requisitions might be for smaller
			 *	amounts, but if we service those
			 *	requisitions we delay service to
			 *	this one.				*/

			break;
		}

		/*	Can service this requisition.			*/

		req->secondsUnclaimed = 0;
		if (req->semaphore != SM_SEM_NONE)
		{
			sm_SemGive(req->semaphore);
		}

		totalFileSpaceAvbl -= req->fileSpaceNeeded;
		totalBulkSpaceAvbl -= req->bulkSpaceNeeded;
		totalHeapSpaceAvbl -= req->heapSpaceNeeded;
	}

	sdr_exit_xn(sdr);		/*	Unlock memory.		*/
}

Object	ionCreateZco(ZcoMedium source, Object location, vast offset,
		vast length, unsigned char coarsePriority,
		unsigned char finePriority, ZcoAcct acct,
		ReqAttendant *attendant)
{
	Sdr		sdr = getIonsdr();
	IonVdb		*vdb = getIonVdb();
	vast		fileSpaceNeeded = 0;
	vast		bulkSpaceNeeded = 0;
	vast		heapSpaceNeeded = 0;
	ReqTicket	ticket;
	Object		zco;

	CHKERR(vdb);
	CHKERR(acct == ZcoInbound || acct == ZcoOutbound);
	if (location == 0)	/*	No initial extent to write.	*/
	{
		oK(sdr_begin_xn(sdr));
		zco = zco_create(sdr, source, 0, 0, 0, acct);
		if (sdr_end_xn(sdr) < 0 || zco == (Object) ERROR)
		{
			putErrmsg("Can't create ZCO.", NULL);
			return ((Object) ERROR);
		}

		return zco;
	}

	CHKERR(offset >= 0);
	CHKERR(length > 0);

	/*	Creating ZCO with its initial extent.			*/

	switch (source)
	{
	case ZcoFileSource:
		fileSpaceNeeded = length;
		break;

	case ZcoBulkSource:
		bulkSpaceNeeded = length;
		break;

	case ZcoSdrSource:
		heapSpaceNeeded = length;
		break;

	case ZcoZcoSource:
		oK(sdr_begin_xn(sdr));
		zco_get_aggregate_length(sdr, location, offset, length,
			&fileSpaceNeeded, &bulkSpaceNeeded, &heapSpaceNeeded);
		sdr_exit_xn(sdr);
		break;

	default:
		putErrmsg("Invalid ZCO source type.", itoa((int) source));
		return ((Object) ERROR);
	}

	if (ionRequestZcoSpace(acct, fileSpaceNeeded, bulkSpaceNeeded,
			heapSpaceNeeded, coarsePriority, finePriority,
			attendant, &ticket) < 0)
	{
		putErrmsg("Failed on ionRequest.", NULL);
		return ((Object) ERROR);
	}

	if (!(ionSpaceAwarded(ticket)))
	{
		/*	Couldn't service request immediately.		*/

		if (attendant == NULL)		/*	Non-blocking.	*/
		{
			ionShred(ticket);	/*	Cancel request.	*/
			return 0;		/*	No Zco created.	*/
		}

		/*	Ticket is req list element for the request.	*/

		if (sm_SemTake(attendant->semaphore) < 0)
		{
			putErrmsg("ionCreateZco can't take semaphore.", NULL);
			ionShred(ticket);	/*	Cancel request.	*/
			return ((Object) ERROR);
		}

		if (sm_SemEnded(attendant->semaphore))
		{
			writeMemo("[i] ZCO creation interrupted.");
			ionShred(ticket);	/*	Cancel request.	*/
			return 0;
		}

		/*	Request has been serviced; can now create ZCO.	*/
	}

	/*	Pass additive inverse of length to zco_create to
 	*	indicate that space has already been awarded.		*/

	oK(sdr_begin_xn(sdr));
	zco = zco_create(sdr, source, location, offset, 0 - length, acct);
	if (sdr_end_xn(sdr) < 0 || zco == (Object) ERROR || zco == 0)
	{
		putErrmsg("Can't create ZCO.", NULL);
		ionShred(ticket);		/*	Cancel request.	*/
		return ((Object) ERROR);
	}

	ionShred(ticket);	/*	Dismiss reservation.		*/
	return zco;
}

vast	ionAppendZcoExtent(Object zco, ZcoMedium source, Object location,
		vast offset, vast length, unsigned char coarsePriority,
		unsigned char finePriority, ReqAttendant *attendant)
{
	Sdr		sdr = getIonsdr();
	IonVdb		*vdb = _ionvdb(NULL);
	vast		fileSpaceNeeded = 0;
	vast		bulkSpaceNeeded = 0;
	vast		heapSpaceNeeded = 0;
	ReqTicket	ticket;
	vast		result;

	CHKERR(vdb);
	CHKERR(location);
	CHKERR(offset >= 0);
	CHKERR(length > 0);
	switch (source)
	{
	case ZcoFileSource:
		fileSpaceNeeded = length;
		break;

	case ZcoBulkSource:
		bulkSpaceNeeded = length;
		break;

	case ZcoSdrSource:	/*	Will become ZcoObjSource.	*/
		heapSpaceNeeded = length;
		break;

	case ZcoZcoSource:
		oK(sdr_begin_xn(sdr));
		zco_get_aggregate_length(sdr, location, offset, length,
			&fileSpaceNeeded, &bulkSpaceNeeded, &heapSpaceNeeded);
		sdr_exit_xn(sdr);
		break;

	default:
		putErrmsg("Invalid ZCO source type.", itoa((int) source));
		return ERROR;
	}

	if (ionRequestZcoSpace(zco_acct(sdr, zco), fileSpaceNeeded,
			bulkSpaceNeeded, heapSpaceNeeded, coarsePriority,
			finePriority, attendant, &ticket) < 0)
	{
		putErrmsg("Failed on ionRequest.", NULL);
		return ERROR;
	}

	if (!(ionSpaceAwarded(ticket)))
	{
		/*	Couldn't service request immediately.		*/

		if (attendant == NULL)		/*	Non-blocking.	*/
		{
			ionShred(ticket);	/*	Cancel request.	*/
			return 0;		/*	No extent.	*/
		}

		/*	Ticket is req list element for the request.	*/

		if (sm_SemTake(attendant->semaphore) < 0)
		{
			putErrmsg("ionAppendZcoExtent can't take semaphore.",
					NULL);
			ionShred(ticket);	/*	Cancel request.	*/
			return ERROR;
		}

		if (sm_SemEnded(attendant->semaphore))
		{
			writeMemo("[i] ZCO extent creation interrupted.");
			ionShred(ticket);	/*	Cancel request.	*/
			return 0;
		}

		/*	Request has been serviced; now create extent.	*/
	}

	/*	Pass additive inverse of length to zco_append_extent
	 *	to indicate that space has already been awarded.	*/

	oK(sdr_begin_xn(sdr));
	result = zco_append_extent(sdr, zco, source, location, offset,
			0 - length);
	if (sdr_end_xn(sdr) < 0 || result == ERROR || result == 0)
	{
		putErrmsg("Can't create ZCO extent.", NULL);
		ionShred(ticket);		/*	Cancel request.	*/
		return ERROR;
	}

	ionShred(ticket);	/*	Dismiss reservation.		*/
	return result;
}

int	ionSendZcoByTCP(int *sock, Object zco, char *buffer, int buflen)
{
	Sdr		sdr = getIonsdr();
	int		totalBytesSent = 0;
	ZcoReader	reader;
	uvast		bytesRemaining;
	uvast		bytesToLoad;
	int		bytesToSend;
	int		bytesSent;

	CHKERR(!(*sock < 0));
	CHKERR(zco);
	CHKERR(buffer);
	CHKERR(buflen > 0);
	zco_start_transmitting(zco, &reader);
	zco_track_file_offset(&reader);
	bytesRemaining = zco_length(sdr, zco);
	while (bytesRemaining > 0)
	{
		CHKERR(sdr_begin_xn(sdr));
		bytesToLoad = bytesRemaining;
		if (bytesToLoad > buflen)
		{
			bytesToLoad = buflen;
		}

		bytesToSend = zco_transmit(sdr, &reader, bytesToLoad, buffer);
		if (sdr_end_xn(sdr) < 0 || bytesToSend != bytesToLoad)
		{
			putErrmsg("Incomplete zco_transmit.", NULL);
			return -1;
		}

		bytesSent = itcp_send(sock, buffer, bytesToSend);
		switch (bytesSent)
		{
		case -1:
			/*	Big problem; shut down.			*/

			putErrmsg("Failed to send ZCO by TCP.", NULL);
			return -1;

		case 0:
			/*	Just lost connection; treat as a
			 *	transient anomaly, note the incomplete
			 *	transmission.				*/

			writeMemo("[?] TCP socket connection lost.");
			return 0;

		default:
			totalBytesSent += bytesSent;
			bytesRemaining -= bytesSent;
		}
	}

	return totalBytesSent;
}
