/*
 *	ion.c:	functions common to multiple protocols in the ION stack.
 *
 *	Copyright (c) 2007, California Institute of Technology.
 *	ALL RIGHTS RESERVED.  U.S. Government Sponsorship acknowledged.
 *
 *	Author: Scott Burleigh, JPL
 *
 */

#include "ion.h"
#include "rfx.h"

#define	ION_DEFAULT_SM_KEY	((255 * 256) + 1)
#define	ION_DEFAULT_SDR_NAME	"ion"
#define	ION_DBNAME		"iondb"
#define	ION_VDBNAME		"ionvdb"
#ifndef ION_SDR_MARGIN
#define	ION_SDR_MARGIN		(20)	/*	Percent.		*/
#endif
#ifndef ION_OPS_ALLOC
#define	ION_OPS_ALLOC		(20)	/*	Percent.		*/
#endif
#define	ION_SEQUESTERED		(ION_SDR_MARGIN + ION_OPS_ALLOC)
#define	MIN_SPIKE_RSRV		(100000)

#define timestampInFormat	"%4d/%2d/%2d-%2d:%2d:%2d"
#define timestampOutFormat	"%.4d/%.2d/%.2d-%.2d:%.2d:%.2d"

static char		*ionDbName = ION_DBNAME;
static Sdr		ionsdr = NULL;
static Object		iondbObject = 0;
static IonDB		ionConstantsBuf;
static IonDB		*ionConstants = &ionConstantsBuf;
static int		ionSmId = 0;
static PsmView		ionWorkingMemory;
static PsmPartition	ionwm = NULL;
static int		ionMemory = -1;	/*	Memory manager index	*/
static IonVdb		*ionvdb = NULL;

extern void		sdr_eject_xn(Sdr);

/*	*	*	Memory access	 *	*	*	*	*/

static void	*allocFromIonMemory(char *fileName, int lineNbr, size_t length)
{
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
	return block;
}

static void	releaseToIonMemory(char *fileName, int lineNbr, void *block)
{
	Psm_free(fileName, lineNbr, ionwm, psa(ionwm, (char *) block));
}

static void	*ionMemAtoP(unsigned long address)
{
	return (void *) psp(ionwm, address);
}

static unsigned long ionMemPtoA(void *pointer)
{
	return (unsigned long) psa(ionwm, pointer);
}

/*	Note that these global variables are NOT static.  They are
 *	referenced by functions throughout ION, through invocations
 *	of the MTAKE and MRELEASE macros.				*/
MemAllocator		wmtake = allocFromIonMemory;
MemDeallocator		wmrelease = releaseToIonMemory;
MemAtoPConverter	wmatop = ionMemAtoP;
MemPtoAConverter	wmptoa = ionMemPtoA;

static int	openIonMemory(int wmKey, long wmSize, char *wmAddress) 
{
	ionwm = &ionWorkingMemory;
	if (memmgr_open(wmKey, wmSize, &wmAddress, &ionSmId, "ionwm",
		&ionwm, &ionMemory, wmtake, wmrelease, wmatop, wmptoa) < 0)
	{
		putErrmsg("Can't open ION working memory.", NULL);
		return -1;
	}

	return ionMemory;
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

		ionLogFile = open(ionLogFileName, O_WRONLY | O_APPEND | O_CREAT,
				00777);
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

static int	initializeVdb()
{
	Sdr		sdr = getIonsdr();
	PsmAddress	vdbAddress;

	sdr_begin_xn(sdr);	/*	Just to lock memory.		*/

	/*	Create and catalogue the IonVdb object in ionwm.	*/

	vdbAddress = psm_zalloc(ionwm, sizeof(IonVdb));
	if (vdbAddress == 0)
	{
		sdr_cancel_xn(sdr);
		putErrmsg("Can't allocate for dynamic database.", NULL);
		return -1;
	}

	ionvdb = (IonVdb *) psp(ionwm, vdbAddress);
	memset((char *) ionvdb, 0, sizeof(IonVdb));
	if ((ionvdb->nodes = sm_list_create(ionwm)) == 0
	|| (ionvdb->neighbors = sm_list_create(ionwm)) == 0
	|| (ionvdb->probes = sm_list_create(ionwm)) == 0
	|| psm_catlg(ionwm, ION_VDBNAME, vdbAddress) < 1)
	{
		sdr_cancel_xn(sdr);
		putErrmsg("Can't initialize volatile database.", NULL);
		return -1;
	}

	ionvdb->deltaFromUTC = ionConstants->deltaFromUTC;
	sdr_cancel_xn(sdr);	/*	Unlock memory.			*/
	return 0;
}

static int	checkNodeListParms(IonParms *parms, char *wdName,
			unsigned long nodeNbr)
{
	char		*nodeListDir;
	char		nodeListFileName[265];
	int		nodeListFile;
	int		lineNbr = 0;
	int		lineLen;
	char		lineBuf[256];
	unsigned long	lineNodeNbr;
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

	isprintf(nodeListFileName, sizeof nodeListFileName, "%.255s%cion_nodes",
			nodeListDir, ION_PATH_DELIMITER);
	if (nodeNbr == 0)	/*	Just attaching.			*/
	{
		nodeListFile = open(nodeListFileName, O_RDONLY, 0);
	}
	else			/*	Initializing the node.		*/
	{
		nodeListFile = open(nodeListFileName, O_RDWR | O_CREAT, 00777);
	}

	if (nodeListFile < 0)
	{
		putSysErrmsg("Can't open ion_nodes file", nodeListFileName);
		writeMemo("[?] Remove ION_NODE_LIST_DIR from env?");
		return -1;
	}

	while (1)
	{
		lineLen = igets(nodeListFile, lineBuf, sizeof lineBuf);
		if (lineLen < 0)
		{
			close(nodeListFile);
			putSysErrmsg("Failed reading ion_nodes file",
					nodeListFileName);
			return -1;
		}

		if (lineLen == 0)		/*	End of file.	*/
		{
			break;			/*	Out of loop.	*/
		}

		lineNbr++;
		if (sscanf(lineBuf, "%lu %d %31s %255s", &lineNodeNbr,
				&lineWmKey, lineSdrName, lineWdName) < 4)
		{
			close(nodeListFile);
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

	isprintf(lineBuf, sizeof lineBuf, "%lu %d %.31s %.255s\n",
			nodeNbr, parms->wmKey, parms->sdrName, wdName);
	result = iputs(nodeListFile, lineBuf);
	close(nodeListFile);
	if (result < 0)
	{
		putSysErrmsg("Failed writing to ion_nodes file", NULL);
		return -1;
	}

	return 0;
}

int	ionInitialize(IonParms *parms, unsigned long ownNodeNbr)
{
	IonDB		iondbBuf;
	char		wdname[256];
	PsmAddress	vdbAddress;

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
			parms->heapWords, parms->heapKey, parms->pathName) < 0)
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

	/*	Recover the ION database, creating it if necessary.	*/

	sdr_begin_xn(ionsdr);
	iondbObject = sdr_find(ionsdr, ionDbName, NULL);
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
		iondbBuf.occupancyCeiling = ((sdr_heap_size(ionsdr) / 100)
			 	* (100 - ION_SEQUESTERED));
		iondbBuf.receptionSpikeReserve = iondbBuf.occupancyCeiling / 16;
		if (iondbBuf.receptionSpikeReserve < MIN_SPIKE_RSRV)
		{
			iondbBuf.receptionSpikeReserve = MIN_SPIKE_RSRV;
		}

		iondbBuf.contacts = sdr_list_create(ionsdr);
		iondbBuf.ranges = sdr_list_create(ionsdr);
		iondbBuf.maxClockError = 0;
		iondbObject = sdr_malloc(ionsdr, sizeof(IonDB));
		if (iondbObject == 0)
		{
			sdr_cancel_xn(ionsdr);
			putErrmsg("No space for database.", NULL);
			return -1;
		}

		sdr_write(ionsdr, iondbObject, (char *) &iondbBuf,
				sizeof(IonDB));
		sdr_catlg(ionsdr, ION_DBNAME, 0, iondbObject);
		if (sdr_end_xn(ionsdr))
		{
			putErrmsg("Can't create ION database.", NULL);
			return -1;
		}

		break;

	default:		/*	Found DB in the SDR.		*/
		sdr_exit_xn(ionsdr);
	}

	/*	Load constants into a conveniently accessed structure.
	 *	Note that this CANNOT be treated as a current database
	 *	image in later processing.				*/

	sdr_read(ionsdr, (char *) ionConstants, iondbObject, sizeof(IonDB));

	/*	Open ION shared-memory partition.			*/

	if (openIonMemory(parms->wmKey, parms->wmSize, parms->wmAddress) < 0)
	{
		putErrmsg("ION memory configuration failed.", NULL);
		return -1;
	}

	vdbAddress = psm_locate(ionwm, ION_VDBNAME);
	if (vdbAddress == 0)
	{
		if (initializeVdb() < 0)
		{
			putErrmsg("ION can't initialize vdb.", NULL);
			return -1;
		}
	}
	else
	{
		ionvdb = (IonVdb *) psp(ionwm, vdbAddress);
	}

	ionRedirectMemos();
	return 0;
}

int	ionAttach()
{
	char		*wdname;
	char		wdnamebuf[256];
	IonParms	parms;
	PsmAddress	vdbAddress;

	if (ionsdr && iondbObject && ionSmId && ionwm && ionMemory != -1)
	{
		return 0;	/*	Already attached.		*/
	}

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
	}

	sdr_begin_xn(ionsdr);		/*	Lock database.		*/
	if (iondbObject == 0)
	{
		iondbObject = sdr_find(ionsdr, ionDbName, NULL);
		if (iondbObject == 0)
		{
			sdr_exit_xn(ionsdr);
			putErrmsg("ION database not found.", NULL);
			return -1;
		}
	}

	sdr_exit_xn(ionsdr);		/*	Unlock database.	*/

	/*	Load constants into a conveniently accessed structure.
	 *	Note that this is NOT a current database image.		*/

	sdr_read(ionsdr, (char *) ionConstants, iondbObject, sizeof(IonDB));

	/*	Open ION shared-memory partition.			*/

	if (openIonMemory(parms.wmKey, 0, NULL) < 0)
	{
		putErrmsg("Can't open access to ION memory.", NULL);
		return -1;
	}

	vdbAddress = psm_locate(ionwm, ION_VDBNAME);
	if (vdbAddress == 0)
	{
		putErrmsg("ION volatile database not found.", NULL);
		return -1;
	}
	else
	{
		ionvdb = (IonVdb *) psp(ionwm, vdbAddress);
	}

	ionRedirectMemos();
	return 0;
}

void	ionDetach()
{
#if defined (VXWORKS) || defined (RTEMS)
	return;
#else
	if (ionsdr)
	{
		sdr_stop_using(ionsdr);
		ionsdr = NULL;
	}

	iondbObject = 0;
	ionSmId = 0;
	ionwm = NULL;
	ionMemory = -1;
	ionvdb = NULL;
#endif
}

void	ionProd(unsigned long fromNode, unsigned long toNode,
		unsigned long xmitRate, unsigned int owlt)
{
	time_t	fromTime;
	time_t	toTime;
	Object	elt;
	char	textbuf[RFX_NOTE_LEN];

	if (ionsdr == NULL)
	{
		if (ionAttach() < 0)
		{
			putErrmsg("Node not initialized yet.", NULL);
			return;
		}
	}

	fromTime = getUTCTime();	/*	The current time.	*/
	toTime = fromTime + 14400;	/*	Four hours later.	*/
	elt = rfx_insert_range(fromTime, toTime, fromNode, toNode, owlt);
       	if (elt == 0)
	{
		putErrmsg("ionProd: range insertion failed.", utoa(owlt));
		return;
	}

	writeMemo("ionProd: range inserted.");
	writeMemo(rfx_print_range(sdr_list_data(ionsdr, elt), textbuf));
	elt = rfx_insert_contact(fromTime, toTime, fromNode, toNode, xmitRate);
	if (elt == 0)
	{
		putErrmsg("ionProd: contact insertion failed.", utoa(xmitRate));
		return;
	}

	writeMemo("ionProd: contact inserted.");
	writeMemo(rfx_print_contact(sdr_list_data(ionsdr, elt), textbuf));
}

void	ionEject()
{
	sdr_eject_xn(ionsdr);
}

void	ionTerminate()
{
	if (ionwm)
	{
		memmgr_destroy(ionSmId, &ionwm);
	}

	sdr_destroy(ionsdr);
	ionsdr = NULL;
	iondbObject = 0;
	ionSmId = 0;
	ionwm = NULL;
	ionMemory = -1;
	ionvdb = NULL;
}

Sdr	getIonsdr()
{
	return ionsdr;
}

Object	getIonDbObject()
{
	return iondbObject;
}

PsmPartition	getIonwm()
{
	return ionwm;
}

int	getIonMemoryMgr()
{
	return ionMemory;
}

IonVdb	*getIonVdb()
{
	return ionvdb;
}

char	*getIonWorkingDirectory()
{
	if (ionConstants == NULL)
	{
		return ".";
	}

	return ionConstants->workingDirectoryName;
}

unsigned long	getOwnNodeNbr()
{
	if (ionConstants == NULL)
	{
		return 0;
	}

	return ionConstants->ownNodeNbr;
}

/*	*	*	Shared-memory tracing 	*	*	*	*/

int	startIonMemTrace(int size)
{
	return psm_start_trace(ionwm, size, NULL);
}

void	printIonMemTrace(int verbose)
{
	psm_print_trace(ionwm, verbose);
}

void	clearIonMemTrace(int verbose)
{
	psm_clear_trace(ionwm);
}

void	stopIonMemTrace(int verbose)
{
	psm_stop_trace(ionwm);
}

/*	*	*	Space management 	*	*	*	*/

void	ionOccupy(int size)
{
	IonDB	iondbBuf;

	REQUIRE(ionLocked());
	sdr_stage(ionsdr, (char *) &iondbBuf, iondbObject, sizeof(IonDB));
	if (iondbBuf.currentOccupancy + size < 0)/*	Overflow.	*/
	{
		iondbBuf.currentOccupancy = iondbBuf.occupancyCeiling;
	}
	else
	{
		iondbBuf.currentOccupancy += size;
	}

	sdr_write(ionsdr, iondbObject, (char *) &iondbBuf, sizeof(IonDB));
}

void	ionVacate(int size)
{
	IonDB	iondbBuf;

	REQUIRE(ionLocked());
	sdr_stage(ionsdr, (char *) &iondbBuf, iondbObject, sizeof(IonDB));
	if (size > iondbBuf.currentOccupancy)	/*	Underflow.	*/
	{
		iondbBuf.currentOccupancy = 0;
	}
	else
	{
		iondbBuf.currentOccupancy -= size;
	}

	sdr_write(ionsdr, iondbObject, (char *) &iondbBuf, sizeof(IonDB));
}

/*	*	*	Timestamp handling 	*	*	*	*/

void	setDeltaFromUTC(int newDelta)
{
	sdr_begin_xn(ionsdr);
	sdr_stage(ionsdr, (char *) ionConstants, iondbObject, sizeof(IonDB));
	ionConstants->deltaFromUTC = newDelta;
	sdr_write(ionsdr, iondbObject, (char *) ionConstants, sizeof(IonDB));
	if (sdr_end_xn(ionsdr) < 0)
	{
		putErrmsg("Can't change delta from UTC.", NULL);
	}

	ionvdb->deltaFromUTC = newDelta;
}

time_t	getUTCTime()
{
	time_t	clocktime;
#if defined(FSWCLOCK)
#include "fswutc.c"
#else

	clocktime = time(NULL);
#endif
	return clocktime - ionvdb->deltaFromUTC;
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
	tzset();	/*	Need to orient mktime properly.		*/
	if (timestampIsUTC)
	{
		/*	Must convert UTC time to local time for mktime.	*/

#if defined (freebsd)
		ts.tm_sec -= ts.tm_gmtoff;
#elif defined (RTEMS)
		/*	RTEMS has no concept of time zones.		*/
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

	localtime_r(&timestamp, &ts);
	sprintf(timestampBuffer, timestampOutFormat,
			ts.tm_year + 1900, ts.tm_mon + 1, ts.tm_mday,
			ts.tm_hour, ts.tm_min, ts.tm_sec);
}

void	writeTimestampUTC(time_t timestamp, char *timestampBuffer)
{
	struct tm	ts;

	gmtime_r(&timestamp, &ts);
	sprintf(timestampBuffer, timestampOutFormat,
			ts.tm_year + 1900, ts.tm_mon + 1, ts.tm_mday,
			ts.tm_hour, ts.tm_min, ts.tm_sec);
}

/*	*	*	Parsing 	*	*	*	*	*/

int	_extractSdnv(unsigned long *into, unsigned char **from, int *remnant,
		int lineNbr)
{
	int	sdnvLength;

	if (*remnant < 1)
	{
		errno = EMSGSIZE;
		putErrmsg("Missing SDNV at line...", itoa(lineNbr));
		return 0;
	}

	sdnvLength = decodeSdnv(into, *from);
	if (sdnvLength < 1)
	{
		errno = EMSGSIZE;
		putErrmsg("Invalid SDNV at line...", itoa(lineNbr));
		return 0;
	}

	(*from) += sdnvLength;
	(*remnant) -= sdnvLength;
	return sdnvLength;
}

/*	*	*	Debugging 	*	*	*	*	*/

int	ionLocked()
{
	return sdr_in_xn(ionsdr);	/*	Boolean.		*/
}

/*	*	*	SDR configuration	*	*	*	*/

int	readIonParms(char *configFileName, IonParms *parms)
{
	char	ownHostName[MAXHOSTNAMELEN + 1];
	char	*endOfHostName;
	char	configFileNameBuffer[PATHLENMAX + 1 + 9 + 1];
	FILE	*configFile;
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

	memset((char *) parms, 0, sizeof(IonParms));
	parms->wmSize = 5000000;
	parms->wmAddress = 0;		/*	Dyamically allocated.	*/
	parms->configFlags = SDR_IN_DRAM;
	parms->heapWords = 250000;
	parms->heapKey = SM_NO_KEY;
	strcpy(parms->pathName, "/usr/ion");

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

		sprintf(configFileNameBuffer, "%.256s.ionconfig", ownHostName);
		configFileName = configFileNameBuffer;
	}

	/*	Get overrides from config file.				*/

	configFile = fopen(configFileName, "r");
	if (configFile == NULL)
	{
		if (errno == ENOENT)	/*	No overrides apply.	*/
		{
			writeMemo("[i] admin pgm using default SDR parms.");
			printIonParms(parms);
			return 0;
		}

		sprintf(buffer, "[?] admin pgm can't open SDR config file \
'%.255s': %.64s", configFileName, system_error_msg());
		writeMemo(buffer);
		return -1;
	}

	sprintf(buffer, "[i] admin pgm using SDR parm overrides from %.255s.",
			configFileName);
	writeMemo(buffer);
	lineNbr = 0;
	while (1)
	{
		if (fgets(line, sizeof line, configFile) == NULL)
		{
			if (feof(configFile))
			{
				result = 0;
				printIonParms(parms);
				break;
			}

			writeErrMemo("admin pgm SDR config file fgets failed");
			result = -1;
			break;
		}

		lineNbr++;
		lineLength = strlen(line) - 1;	/*	lose newline	*/
		if (lineLength < 1)
		{
			continue;
		}

		line[lineLength] = '\0';	/*	delimit line	*/
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
			sprintf(buffer, "[?] incomplete SDR configuration \
file line (%d).", lineNbr);
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

		sprintf(buffer, "[?] unknown SDR config keyword '%.32s' at \
line %d.", tokens[0], lineNbr);
		writeMemo(buffer);
		result = -1;
		break;
	}

	fclose(configFile);
	return result;
}

void	printIonParms(IonParms *parms)
{
	char	buffer[512];

	sprintf(buffer, "wmKey:           %d", parms->wmKey);
	writeMemo(buffer);
	sprintf(buffer, "wmSize:          %ld", parms->wmSize);
	writeMemo(buffer);
	sprintf(buffer, "wmAddress:       %0lx", (unsigned long)
			parms->wmAddress);
	writeMemo(buffer);
	sprintf(buffer, "sdrName:        '%s'", parms->sdrName);
	writeMemo(buffer);
	sprintf(buffer, "configFlags:     %d", parms->configFlags);
	writeMemo(buffer);
	sprintf(buffer, "heapWords:       %ld", parms->heapWords);
	writeMemo(buffer);
	sprintf(buffer, "heapKey:         %d", parms->heapKey);
	writeMemo(buffer);
	sprintf(buffer, "pathName:       '%.256s'", parms->pathName);
	writeMemo(buffer);
}
