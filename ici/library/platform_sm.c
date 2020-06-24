/*
	platform_sm.c:	more platform-dependent implementation of
			common functions, to simplify porting.
									*/
/*	Copyright (c) 1997, California Institute of Technology.		*/
/*	ALL RIGHTS RESERVED.  U.S. Government Sponsorship		*/
/*	acknowledged.							*/
/*									*/
/*	Author: Alan Schlutsmeyer, Jet Propulsion Laboratory		*/
/*	        Scott Burleigh, Jet Propulsion Laboratory		*/
/*									*/

#include <platform.h>

static void	takeIpcLock();
static void	giveIpcLock();

/************************* Shared-memory services *****************************/

	/*	sm_ShmAttach returns have the following meanings:
	 *		1 - created a new memory segment
	 *		0 - memory segment already existed
	 *	       -1 - could not attach to memory segment
	 */

#ifdef RTOS_SHM

	/* ---- Shared Memory services (RTOS) ------------------------- */

#define nShmIds	50

typedef struct
{
	int		key;
	char		*ptr;
	unsigned int	freeNeeded:1;		/*	Boolean.	*/
	unsigned int	nUsers:31;
} SmShm;

static SmShm	*_shmTbl()
{
	static SmShm	shmTable[nShmIds];

	return shmTable;
}

int
sm_ShmAttach(int key, size_t size, char **shmPtr, uaddr *id)
{
	int	i;
	SmShm	*shm;

	CHKERR(shmPtr);
	CHKERR(id);

    /* If shared memory segment exists, return its location */
	if (key != SM_NO_KEY)
	{
		for (i = 0, shm = _shmTbl(); i < nShmIds; i++, shm++)
		{
			if (shm->key == key)
			{
				*shmPtr = shm->ptr;
				shm->nUsers++;
				*id = i;
				return 0;
			}
		}
	}

    /* create a new "shared memory segment" */
	for (i = 0, shm = _shmTbl(); i < nShmIds; i++, shm++)
	{
		if (shm->ptr == NULL)
		{
			/*	(To prevent dynamic allocation of
			 *	the required memory segment, pre-
			 *	allocate it and place a pointer to
			 *	the previously allocated memory
			 *	into *shmPtr.)				*/

			if (*shmPtr == NULL)
			{
				*shmPtr = (char *) acquireSystemMemory(size);
				if (*shmPtr == NULL)
				{
					putErrmsg("Memory attachment failed.",
							NULL);
					return -1;
				}

				shm->freeNeeded = 1;
			}
			else
			{
				shm->freeNeeded = 0;
			}

			shm->ptr = *shmPtr;
			shm->key = key;
			shm->nUsers = 1;
			*id = i;
			return 1;
		}
	}

	putErrmsg("Too many shared memory segments.", itoa(nShmIds));
	return -1;
}

void
sm_ShmDetach(char *shmPtr)
{
	int	i;
	SmShm	*shm;

	for (i = 0, shm = _shmTbl(); i < nShmIds; i++, shm++)
	{
		if (shm->ptr == shmPtr)
		{
			shm->nUsers--;
			return;
		}
	}
}

void
sm_ShmDestroy(uaddr i)
{
	SmShm	*shm;

	CHKVOID(i >= 0);
	CHKVOID(i < nShmIds);
	shm = _shmTbl() + i;
	if (shm->freeNeeded)
	{
		TRACK_FREE(shm->ptr);
		free(shm->ptr);
		shm->freeNeeded = 0;
	}

	shm->ptr = NULL;
	shm->key = SM_NO_KEY;
	shm->nUsers = 0;
}

#endif			/*	end of #ifdef RTOS_SHM			*/

#ifdef MINGW_SHM

static int	trackIpc(int type, int key)
{
	char	*pipeName = "\\\\.\\pipe\\ion.pipe";
	DWORD	keyDword = (DWORD) key;
	char	msg[1 + sizeof(DWORD)];
	int	startedWinion = 0;
	HANDLE	hPipe;
	DWORD	dwMode;
	BOOL	fSuccess = FALSE; 
	DWORD	bytesWritten;
	char	reply[1];
	DWORD	bytesRead;

	msg[0] = type;
	memcpy(msg + 1, (char *) &keyDword, sizeof(DWORD));

	/*	Keep trying to open pipe to winion until succeed.	*/
 
	while (1) 
	{
      		if (WaitNamedPipe(pipeName, 100) == 0) 	/*	Failed.	*/
		{
			if (GetLastError() != ERROR_FILE_NOT_FOUND)
			{
				putErrmsg("Timed out opening pipe to winion.",
						NULL);
				return -1;
			}

			/*	Pipe doesn't exist, so start winion.	*/

			if (startedWinion)	/*	Already did.	*/
			{
				putErrmsg("Can't keep winion runnning.", NULL);
				return -1;
			}

			startedWinion = 1;
			pseudoshell("winion");
			Sleep(100);	/*	Let winion start.	*/
			continue;
		}

		/*	Pipe exists, winion is waiting for connection.	*/

		hPipe = CreateFile(pipeName, GENERIC_READ | GENERIC_WRITE,
				0, NULL, OPEN_EXISTING, 0, NULL);
		if (hPipe != INVALID_HANDLE_VALUE) 
		{
			break; 		/*	Got it.			*/
		}
 
		if (GetLastError() != ERROR_PIPE_BUSY) 
		{
			putErrmsg("Can't open pipe to winion.",
					itoa(GetLastError()));
			return -1;
		}
	}
 
	/*	Connected to pipe.  Change read-mode to message(?!).	*/
 
	dwMode = PIPE_READMODE_MESSAGE; 
	fSuccess = SetNamedPipeHandleState(hPipe, &dwMode, NULL, NULL);
	if (!fSuccess) 
	{
		putErrmsg("Can't change pipe's read mode.",
				itoa(GetLastError()));
		return -1;
	}
 
	fSuccess = WriteFile(hPipe, msg, sizeof msg, &bytesWritten, NULL);
	if (!fSuccess) 
	{
		putErrmsg("Can't write to pipe.", itoa(GetLastError()));
		return -1;
	}
 
	fSuccess = ReadFile(hPipe, reply, 1, &bytesRead, NULL);
	if (!fSuccess) 
	{
		putErrmsg("Can't read from pipe.", itoa(GetLastError()));
		return -1;
	}
 
	CloseHandle(hPipe); 
	if (reply[0] == 0 && type != '?')
	{
		return -1;
	}

	return 0;
}

	/* ---- Shared Memory services (mingw -- Windows) ------------- */

typedef struct
{
	char	*shmPtr;
	int	key;
} SmSegment;

#define	MAX_SM_SEGMENTS	20

static void	_smSegment(char *shmPtr, int *key)
{
	static SmSegment	segments[MAX_SM_SEGMENTS];
	static int		segmentsNoted = 0;
	int			i;

	CHKVOID(key);
	for (i = 0; i < segmentsNoted; i++)
	{
		if (segments[i].shmPtr == shmPtr)
		{
			/*	Segment previously noted.		*/

			if (*key == SM_NO_KEY)	/*	Lookup.		*/
			{
				*key = segments[i].key;
			}

			return;
		}
	}

	/*	This is not a previously noted shared memory segment.	*/

	if (*key == SM_NO_KEY)		/*	No key provided.	*/
	{
		return;			/*	Can't record segment.	*/
	}

	/*	Newly noting a shared memory segment.			*/

	if (segmentsNoted == MAX_SM_SEGMENTS)
	{
		puts("No more room for shared memory segments.");
		return;
	}

	segments[segmentsNoted].shmPtr = shmPtr;
	segments[segmentsNoted].key = *key;
	segmentsNoted += 1;
}

int
sm_ShmAttach(int key, size_t size, char **shmPtr, uaddr *id)
{
	char	memName[32];
	size_t	minSegSize = 16;
	HANDLE	mappingObj;
	void	*mem;
	int	newSegment = 0;

	CHKERR(shmPtr);
	CHKERR(id);

    	/*	If key is not specified, make one up.			*/

	if (key == SM_NO_KEY)
	{
		key = sm_GetUniqueKey();
	}

	sprintf(memName, "%d.mmap", key);
	if (size != 0)	/*	Want to create segment if not present.	*/
	{
		if (size < minSegSize)
		{
			size = minSegSize;
		}
	}

	/*	Locate the shared memory segment.  If doesn't exist
	 *	yet, create it.						*/

	mappingObj = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, memName);
	if (mappingObj == NULL)		/*	Not found.		*/
	{
		if (size == 0)		/*	Just attaching.		*/
		{
			putErrmsg("Can't open shared memory segment.",
					utoa(GetLastError()));
			return -1;
		}

		/*	Need to create this shared memory segment.	*/

		mappingObj = CreateFileMapping(INVALID_HANDLE_VALUE, NULL,
				PAGE_READWRITE, 0, size, memName);
		if (mappingObj == NULL)
		{
			putErrmsg("Can't create shared memory segment.",
					utoa(GetLastError()));
			return -1;
		}

		if (trackIpc(WIN_NOTE_SM, key) < 0)
		{
			putErrmsg("Can't preserve shared memory.", NULL);
			return -1;
		}

		newSegment = 1;
	}

	mem = MapViewOfFile(mappingObj, FILE_MAP_ALL_ACCESS, 0, 0, 0);
	if (mem == NULL)
	{
		putErrmsg("Can't map shared memory segment.",
				utoa(GetLastError()));
		return -1;
	}

	/*	Record the ID of this segment in case the segment
	 *	must be detached later.					*/

	_smSegment(mem, &key);
	*shmPtr = (char *) mem;
	*id = (uaddr) mappingObj;
	if (newSegment)
	{
		memset(mem, 0, size);	/*	Initialize to zeroes.	*/
		return 1;
	}

	return 0;
}

void
sm_ShmDetach(char *shmPtr)
{
	return;		/*	Closing last handle detaches segment.	*/
}

void
sm_ShmDestroy(uaddr id)
{
	return;		/*	Closing last handle destroys mapping.	*/
}

#endif			/*	end of #ifdef MINGW_SHM			*/

#ifdef SVR4_SHM

	/* ---- Shared Memory services (Unix) ------------------------- */

int
sm_ShmAttach(int key, size_t size, char **shmPtr, uaddr *id)
{
	size_t		minSegSize = 16;
	int		result;
	char		*mem;
	struct shmid_ds	stat;

	CHKERR(shmPtr);
	CHKERR(id);

    /* if key is not specified, make up one */
	if (key == SM_NO_KEY)
	{
		key = sm_GetUniqueKey();
	}

	if (size != 0)	/*	Want to create region if not present.	*/
	{
		if (size < minSegSize)
		{
			size = minSegSize;
		}
	}

    /* create a new shared memory segment, or attach to an existing one */
	if ((*id = shmget(key, size, IPC_CREAT | 0666)) == -1)
	{
		putSysErrmsg("Can't get shared memory segment", itoa(size));
		return -1;
	}

    /* determine if the segment has been initialized yet */
	if (shmctl(*id, IPC_STAT, &stat) == -1)
	{
		putSysErrmsg("Can't get status of shared memory segment",
				itoa(key));
		return -1;
	}

	result = (stat.shm_atime == 0);	/*	If never attached, 1.	*/

	/*	Normally, *shmPtr should be set to NULL prior to
	 *	calling sm_ShmAttach, to let shmat determine the
	 *	attachment point for the memory segment.		*/

	if ((mem = (char *) shmat(*id, *shmPtr, 0)) == ((char *) -1))
	{
		putSysErrmsg("Can't attach shared memory segment", itoa(key));
		return -1;
	}

	if (result == 1)	/*	Newly allocated data segment.	*/
	{
		memset(mem, 0, size);	/*	Initialize to zeroes.	*/
	}

	*shmPtr = mem;
	return result;
}

void
sm_ShmDetach(char *shmPtr)
{
	if (shmdt(shmPtr) < 0)
	{
		putSysErrmsg("Can't detach shared memory segment", NULL);
	}
}

void
sm_ShmDestroy(uaddr id)
{
	if (shmctl(id, IPC_RMID, NULL) < 0)
	{
		putSysErrmsg("Can't destroy shared memory segment", itoa(id));
	}
}

#endif			/*	End of #ifdef SVR4_SHM			*/

/****************** Argument buffer services **********************************/

#ifdef ION_LWT

#define	ARG_BUFFER_CT	256
#define	MAX_ARG_LENGTH	127

typedef struct
{
	int		ownerTid;
	char		arg[MAX_ARG_LENGTH + 1];
} ArgBuffer;

static ArgBuffer	*_argBuffers()
{
	static ArgBuffer argBufTable[ARG_BUFFER_CT];

	return argBufTable;
}

static int	_argBuffersAvbl(int *val)
{
	static int	argBufsAvbl = -1;
	ArgBuffer	*argBuffer;
	int		i;

	if (argBufsAvbl < 0)	/*	Not initialized yet.		*/
	{
		/*	Initialize argument copying.			*/

		argBuffer = _argBuffers();
		for (i = 0; i < ARG_BUFFER_CT; i++)
		{
			argBuffer->ownerTid = 0;
			argBuffer++;
		}

		argBufsAvbl = ARG_BUFFER_CT;
	}

	if (val == NULL)
	{
		return argBufsAvbl;
	}

	argBufsAvbl = *val;
	return 0;
}

static int	copyArgs(int argc, char **argv)
{
	int		i;
	int		j;
	ArgBuffer	*buf;
	char		*arg;
	int		argLen;

	if (argc > _argBuffersAvbl(NULL))
	{
		putErrmsg("No available argument buffers.", NULL);
		return -1;
	}

	/*	Copy each argument into the next available argument
	 *	buffer, tagging each consumed buffer with -1 so that
	 *	it can be permanently tagged when the ownerTid is
	 *	known, and replace each original argument with a
	 *	pointer to its copy in the argBuffers.			*/

	for (i = 0, buf = _argBuffers(), j = 0; j < argc; j++)
	{
		arg = argv[j];
		argLen = strlen(arg);
		if (argLen > MAX_ARG_LENGTH)
		{
			argLen = MAX_ARG_LENGTH;
		}

		while (1)
		{
			CHKERR(i < ARG_BUFFER_CT);
			if (buf->ownerTid != 0)	/*	Unavailable.	*/
			{
				i++;
				buf++;
				continue;
			}

			/*	Copy argument into this buffer.		*/

			memcpy(buf->arg, arg, argLen);
			buf->arg[argLen] = '\0';
			buf->ownerTid = -1;
			argv[j] = buf->arg;

			/*	Skip over this buffer for next arg.	*/

			i++;
			buf++;
			break;
		}
	}

	return 0;
}

static void	tagArgBuffers(int tid)
{
	int		avbl;
	int		i;
	ArgBuffer	*buf;

	avbl = _argBuffersAvbl(NULL);
	for (i = 0, buf = _argBuffers(); i < ARG_BUFFER_CT; i++, buf++)
	{
		if (buf->ownerTid == -1)
		{
			buf->ownerTid = tid;
			if (tid != 0)
			{
				avbl--;
			}
		}
#if !(defined (VXWORKS))
		else	/*	An opportunity to release arg buffers.	*/
		{
			if (buf->ownerTid != 0 && !sm_TaskExists(buf->ownerTid))
			{
				buf->ownerTid = 0;
				avbl++;
			}
		}
#endif
	}

	oK(_argBuffersAvbl(&avbl));
}

#endif		/*	End of #ifdef ION_LWT				*/

/****************** Semaphore services **********************************/

#ifdef VXWORKS_SEMAPHORES

	/* ---- IPC services access control (VxWorks) ----------------- */

#include <vxWorks.h>
#include <semLib.h>
#include <taskLib.h>
#include <timers.h>
#include <sysSymTbl.h>
#include <taskVarLib.h>
#include <dbgLib.h>

#define nSemIds 200

typedef struct
{
	int	key;
	SEM_ID	id;
	int	ended;
} SmSem;

static SmSem	*_semTbl()
{
	static SmSem	semTable[nSemIds];

	return semTable;
}

	/* ---- Semaphore services (VxWorks) --------------------------- */

static void	releaseArgBuffers(WIND_TCB *pTcb)
{
	int		tid = (int) pTcb;
	int		avbl;
	int		i;
	ArgBuffer	*buf;

	avbl = _argBuffersAvbl(NULL);
	for (i = 0, buf = _argBuffers(); i < ARG_BUFFER_CT; i++, buf++)
	{
		if (buf->ownerTid == tid)
		{
			buf->ownerTid = 0;
			avbl++;
		}
	}

	oK(_argBuffersAvbl(&avbl));
}

static int	initializeIpc()
{
	SmSem		*semTbl = _semTbl();
	SmSem		*sem;
	int		i;
	SmShm		*shmTbl = _shmTbl();
	SmShm		*shm;

	for (i = 0, sem = semTbl; i < nSemIds; i++, sem++)
	{
		sem->key = SM_NO_KEY;
		sem->id = NULL;
		sem->ended = 0;
	}

	for (i = 0, shm = shmTbl; i < nShmIds; i++, shm++)
	{
		shm->key = SM_NO_KEY;
		shm->ptr = NULL;
		shm->freeNeeded = 0;
		shm->nUsers = 0;
	}

	/*	Note: we are abundantly aware that the
	 *	prototype for the function that must be
	 *	passed to taskDeleteHookAdd, according to
	 *	the VxWorks 5.4 Reference Manual, is NOT
	 *	of the same type as FUNCPTR, which returns
	 *	int rather than void.  We do this cast only
	 *	to get rid of a compiler warning which is,
	 *	at bottom, due to a bug in Wind River's
	 *	function prototype for taskDeleteHookAdd.	*/

	if (taskDeleteHookAdd((FUNCPTR) releaseArgBuffers) == ERROR)
	{
		putSysErrmsg("Can't register releaseArgBuffers", NULL);
		return -1;
	}

	giveIpcLock();
	return 0;
}

/*	Note that the ipcSemaphore is allocated using the VxWorks
 *	semBLib functions directly rather than the ICI VxWorks
 *	semaphore system.  This is necessary for bootstrapping the
 *	ICI semaphore system: only after the ipcSemaphore exists
 *	can we initialize the semaphore tables, enabling subsequent
 *	semaphores to be allocated in a more portable fashion.		*/

static SEM_ID	_ipcSemaphore(int stop)
{
	static SEM_ID	ipcSem = NULL;

	if (stop)
	{
		if (ipcSem)
		{
			semDelete(ipcSem);
			ipcSem = NULL;
		}

		return NULL;
	}

	if (ipcSem == NULL)
	{
		ipcSem = semBCreate(SEM_Q_FIFO, SEM_EMPTY);
		if (ipcSem == NULL)
		{
			putSysErrmsg("Can't initialize IPC semaphore", NULL);
		}
		else
		{
			if (initializeIpc() < 0)
			{
				semDelete(ipcSem);
				ipcSem = NULL;
			}
		}
	}

	return ipcSem;
}

int	sm_ipc_init()
{
	SEM_ID	sem = _ipcSemaphore(0);

	if (sem == NULL)
	{
		putErrmsg("Can't initialize IPC.", NULL);
		return -1;
	}

	return 0;
}

void	sm_ipc_stop()
{
	oK(_ipcSemaphore(1));
}

static void	takeIpcLock()
{
	semTake(_ipcSemaphore(0), WAIT_FOREVER);
}

static void	giveIpcLock()
{
	semGive(_ipcSemaphore(0));
}

sm_SemId	sm_SemCreate(int key, int semType)
{
	SmSem	*semTbl = _semTbl();
	SmSem	*sem;
	SEM_ID	semId;
	int	i;

	/*	If key is not specified, invent one.			*/

	if (key == SM_NO_KEY)
	{
		key = sm_GetUniqueKey();
	}

	takeIpcLock();
    /* If semaphore exists, return its ID */
	for (i = 0; i < nSemIds; i++)
	{
		if (semTbl[i].key == key)
		{
			giveIpcLock();
			return i;
		}
	}

    /* create a new semaphore */
	for (i = 0, sem = semTbl; i < nSemIds; i++, sem++)
	{
		if (sem->id == NULL)
		{
			if (semType == SM_SEM_PRIORITY)
			{
				semId = semBCreate(SEM_Q_PRIORITY, SEM_EMPTY);
			}
			else
			{
				semId = semBCreate(SEM_Q_FIFO, SEM_EMPTY);
			}

			if (semId == NULL)
			{
				giveIpcLock();
				putSysErrmsg("Can't create semaphore",
						itoa(key));
				return SM_SEM_NONE;
			}

			sem->id = semId;
			sem->key = key;
			sem->ended = 0;
			sm_SemGive(i);	/*	(First taker succeeds.)	*/
			giveIpcLock();
			return i;
		}
	}

	giveIpcLock();
	putErrmsg("Too many semaphores.", itoa(nSemIds));
	return SM_SEM_NONE;
}

void	sm_SemDelete(sm_SemId i)
{
	SmSem	*semTbl = _semTbl();
	SmSem	*sem;

	CHKVOID(i >= 0);
	CHKVOID(i < nSemIds);
	sem = semTbl + i;
	takeIpcLock();
	if (semDelete(sem->id) == ERROR)
	{
		giveIpcLock();
		putSysErrmsg("Can't delete semaphore", itoa(i));
		return;
	}

	sem->id = NULL;
	sem->key = SM_NO_KEY;
	giveIpcLock();
}

int	sm_SemTake(sm_SemId i)
{
	SmSem	*semTbl = _semTbl();
	SmSem	*sem;

	CHKERR(i >= 0);
	CHKERR(i < nSemIds);
	sem = semTbl + i;
	if (semTake(sem->id, WAIT_FOREVER) == ERROR)
	{
		putSysErrmsg("Can't take semaphore", itoa(i));
		return -1;
	}

	return 0;
}

void	sm_SemGive(sm_SemId i)
{
	SmSem	*semTbl = _semTbl();
	SmSem	*sem;

	CHKVOID(i >= 0);
	CHKVOID(i < nSemIds);
	sem = semTbl + i;
	if (semGive(sem->id) == ERROR)
	{
		putSysErrmsg("Can't give semaphore", itoa(i));
	}
}

void	sm_SemEnd(sm_SemId i)
{
	SmSem	*semTbl = _semTbl();
	SmSem	*sem;

	CHKVOID(i >= 0);
	CHKVOID(i < nSemIds);
	sem = semTbl + i;
	sem->ended = 1;
	sm_SemGive(i);
}

int	sm_SemEnded(sm_SemId i)
{
	SmSem	*semTbl = _semTbl();
	SmSem	*sem;
	int	ended;

	CHKZERO(i >= 0);
	CHKZERO(i < nSemIds);
	sem = semTbl + i;
	ended = sem->ended;
	if (ended)
	{
		sm_SemGive(i);	/*	Enable multiple tests.		*/
	}

	return ended;
}

void	sm_SemUnend(sm_SemId i)
{
	SmSem	*semTbl = _semTbl();
	SmSem	*sem;

	CHKVOID(i >= 0);
	CHKVOID(i < nSemIds);
	sem = semTbl + i;
	sem->ended = 0;
}

int	sm_SemUnwedge(sm_SemId i, int timeoutSeconds)
{
	SmSem	*semTbl = _semTbl();
	SmSem	*sem;
	int	ticks;

	CHKERR(i >= 0);
	CHKERR(i < nSemIds);
	sem = semTbl + i;
	if (timeoutSeconds < 1) timeoutSeconds = 1;
	ticks = sysClkRateGet() * timeoutSeconds;
	if (semTake(sem->id, ticks) == ERROR)
	{
		if (errno != S_objLib_OBJ_TIMEOUT)
		{
			putSysErrmsg("Can't unwedge semaphore", itoa(i));
			return -1;
		}
	}

	if (semGive(sem->id) == ERROR)
	{
		putSysErrmsg("Can't unwedge semaphore", itoa(i));
		return -1;
	}

	return 0;
}

#endif			/*	End of #ifdef VXWORKS_SEMAPHORES	*/

#ifdef MINGW_SEMAPHORES

	/* ---- Semaphore services (mingw) --------------------------- */

#ifndef SM_SEMKEY
#define SM_SEMKEY	(0xee01)
#endif
#ifndef SM_SEMTBLKEY
#define SM_SEMTBLKEY	(0xee02)
#endif

typedef struct
{
	int	key;
	int	inUse;
	int	ended;
} IciSemaphore;

typedef struct
{
	IciSemaphore	semaphores[SEMMNS];
	int		semaphoresCreated;
} SemaphoreTable;

static SemaphoreTable	*_semTbl(int stop)
{
	static SemaphoreTable	*semaphoreTable = NULL;
	static uaddr		semtblId = 0;

	if (stop)
	{
		if (semaphoreTable != NULL)
		{
			sm_ShmDetach((char *) semaphoreTable);
			semaphoreTable = NULL;
		}

		return NULL;
	}

	if (semaphoreTable == NULL)
	{
		switch(sm_ShmAttach(SM_SEMTBLKEY, sizeof(SemaphoreTable),
				(char **) &semaphoreTable, &semtblId))
		{
		case -1:
			putErrmsg("Can't create semaphore table.", NULL);
			break;

		case 0:
			break;		/*	Semaphore table exists.	*/

		default:		/*	New SemaphoreTable.	*/
			memset((char *) semaphoreTable, 0,
					sizeof(semaphoreTable));
		}
	}

	return semaphoreTable;
}

int	sm_ipc_init()
{
	char	semName[32];
	HANDLE	ipcSemaphore;

	oK(_semTbl(0));

	/*	Create the IPC semaphore and preserve it.		*/

	sprintf(semName, "%d.event", SM_SEMKEY);
	ipcSemaphore = CreateEvent(NULL, FALSE, FALSE, semName);
	if (ipcSemaphore == NULL)
	{
		putErrmsg("Can't create IPC semaphore.", NULL);
		return -1;
	}

	if (GetLastError() != ERROR_ALREADY_EXISTS)
	{
		oK(SetEvent(ipcSemaphore));

		/*	Preserve the IPC semaphore.			*/

		if (trackIpc(WIN_NOTE_SEMAPHORE, SM_SEMKEY) < 0)
		{
			putErrmsg("Can't preserve IPC semaphore.", NULL);
			sm_ipc_stop();
			return -1;
		}
	}

	return 0;
}

void	sm_ipc_stop()
{
	oK(trackIpc(WIN_STOP_ION, 0));
}

static HANDLE	getSemaphoreHandle(int key)
{
	char	semName[32];

	sprintf(semName, "%d.event", key);
	return OpenEvent(EVENT_ALL_ACCESS, FALSE, semName);
}

static void	takeIpcLock()
{
	HANDLE	ipcSemaphore = getSemaphoreHandle(SM_SEMKEY);

	oK(WaitForSingleObject(ipcSemaphore, INFINITE));
	CloseHandle(ipcSemaphore);
}

static void	giveIpcLock()
{
	HANDLE	ipcSemaphore = getSemaphoreHandle(SM_SEMKEY);

	oK(SetEvent(ipcSemaphore));
	CloseHandle(ipcSemaphore);
}

sm_SemId	sm_SemCreate(int key, int semType)
{
	SemaphoreTable	*semTbl;
	int		i;
	IciSemaphore	*sem;
	char		semName[32];
	HANDLE		semId;

	/*	If key is not specified, invent one.			*/

	if (key == SM_NO_KEY)
	{
		key = sm_GetUniqueKey();
	}

	/*	Look through list of all existing ICI semaphores.	*/

	takeIpcLock();
	semTbl = _semTbl(0);
	if (semTbl == NULL)
	{
		giveIpcLock();
		putErrmsg("No semaphore table.", NULL);
		return SM_SEM_NONE;
	}

	for (i = 0, sem = semTbl->semaphores; i < semTbl->semaphoresCreated;
			i++, sem++)
	{
		if (sem->key == key)
		{
			giveIpcLock();
			return i;	/*	already created		*/
		}
	}

	/*	No existing semaphore for this key; allocate new one.	*/

	for (i = 0, sem = semTbl->semaphores; i < SEMMNS; i++, sem++)
	{
		if (sem->inUse)
		{
			continue;
		}

		sprintf(semName, "%d.event", key);
		semId = CreateEvent(NULL, FALSE, FALSE, semName);
		if (semId == NULL)
		{
			giveIpcLock();
			putErrmsg("Can't create semaphore.",
					utoa(GetLastError()));
			return SM_SEM_NONE;
		}

		if (GetLastError() != ERROR_ALREADY_EXISTS)
		{
			if (trackIpc(WIN_NOTE_SEMAPHORE, key) < 0)
			{
				CloseHandle(semId);
				giveIpcLock();
				putErrmsg("Can't preserve semaphore.", NULL);
				return SM_SEM_NONE;
			}
		}

		CloseHandle(semId);
		sem->inUse = 1;
		sem->key = key;
		sem->ended = 0;
		if (!(i < semTbl->semaphoresCreated))
		{
			semTbl->semaphoresCreated++;
		}

		sm_SemGive(i);		/*	(First taker succeeds.)	*/
		giveIpcLock();
		return i;
	}

	giveIpcLock();
	putErrmsg("Can't add any more semaphores.", NULL);
	return SM_SEM_NONE;
}

void	sm_SemDelete(sm_SemId i)
{
	SemaphoreTable	*semTbl = _semTbl(0);
	IciSemaphore	*sem;

	CHKVOID(i >= 0);
	CHKVOID(i < SEMMNS);
	sem = semTbl->semaphores + i;
	takeIpcLock();
	if (sem->inUse)
	{
		if (trackIpc(WIN_FORGET_SEMAPHORE, sem->key) < 0)
		{
			putErrmsg("Can't detach from semaphore.", NULL);
		}

		sem->inUse = 0;
		sem->key = SM_NO_KEY;
	}

	giveIpcLock();
}

int	sm_SemTake(sm_SemId i)
{
	SemaphoreTable	*semTbl = _semTbl(0);
	IciSemaphore	*sem;
	HANDLE		semId;

	CHKERR(i >= 0);
	CHKERR(i < SEMMNS);
	sem = semTbl->semaphores + i;
	CHKERR(sem->inUse);
	semId = getSemaphoreHandle(sem->key);
	if (semId == NULL)
	{
		putSysErrmsg("Can't take semaphore", itoa(i));
		return -1;
	}

	oK(WaitForSingleObject(semId, INFINITE));
	CloseHandle(semId);
	return 0;
}

void	sm_SemGive(sm_SemId i)
{
	SemaphoreTable	*semTbl = _semTbl(0);
	IciSemaphore	*sem;
	HANDLE		semId;

	CHKVOID(i >= 0);
	CHKVOID(i < SEMMNS);
	sem = semTbl->semaphores + i;
	CHKVOID(sem->inUse);
	semId = getSemaphoreHandle(sem->key);
	if (semId == NULL)
	{
		putSysErrmsg("Can't give semaphore", itoa(i));
		return;
	}

	oK(SetEvent(semId));
	CloseHandle(semId);
}

void	sm_SemEnd(sm_SemId i)
{
	SemaphoreTable	*semTbl = _semTbl(0);
	IciSemaphore	*sem;

	CHKVOID(i >= 0);
	CHKVOID(i < SEMMNS);
	sem = semTbl->semaphores + i;
	CHKVOID(sem->inUse);
	sem->ended = 1;
	sm_SemGive(i);
}

int	sm_SemEnded(sm_SemId i)
{
	SemaphoreTable	*semTbl = _semTbl(0);
	IciSemaphore	*sem;
	int		ended;

	CHKZERO(i >= 0);
	CHKZERO(i < SEMMNS);
	sem = semTbl->semaphores + i;
	CHKZERO(sem->inUse);
	ended = sem->ended;
	if (ended)
	{
		sm_SemGive(i);	/*	Enable multiple tests.		*/
	}

	return ended;
}

void	sm_SemUnend(sm_SemId i)
{
	SemaphoreTable	*semTbl = _semTbl(0);
	IciSemaphore	*sem;

	CHKVOID(i >= 0);
	CHKVOID(i < SEMMNS);
	sem = semTbl->semaphores + i;
	CHKVOID(sem->inUse);
	sem->ended = 0;
}

int	sm_SemUnwedge(sm_SemId i, int timeoutSeconds)
{
	SemaphoreTable	*semTbl = _semTbl(0);
	IciSemaphore	*sem;
	HANDLE		semId;
	DWORD		millisec;

	CHKERR(i >= 0);
	CHKERR(i < SEMMNS);
	sem = semTbl->semaphores + i;
	CHKERR(sem->inUse);
	semId = getSemaphoreHandle(sem->key);
	if (semId == NULL)
	{
		putSysErrmsg("Can't unwedge semaphore", itoa(i));
		return -1;
	}

	if (timeoutSeconds < 1) timeoutSeconds = 1;
	millisec = timeoutSeconds * 1000;
	oK(WaitForSingleObject(semId, millisec));
	oK(SetEvent(semId));
	CloseHandle(semId);
	return 0;
}

#endif			/*	End of #ifdef MINGW_SEMAPHORES		*/

#ifdef POSIX_SEMAPHORES

	/* ---- Semaphore services (POSIX, including RTEMS) ---------	*/

typedef struct
{
	int		key;
	sem_t		semobj;
	sem_t		*id;
	int		ended;
} SmSem;

static SmSem	*_semTbl()
{
	static SmSem	semTable[SEM_NSEMS_MAX];
	static int	semTableInitialized = 0;

	if (!semTableInitialized)
	{
		memset((char *) semTable, 0, sizeof semTable);
		semTableInitialized = 1;
	}

	return semTable;
}

static sem_t	*_ipcSemaphore(int stop)
{
	static sem_t	ipcSem;
	static int	ipcSemInitialized = 0;

	if (stop)
	{
		if (ipcSemInitialized)
		{
			oK(sem_destroy(&ipcSem));
			ipcSemInitialized = 0;
		}

		return NULL;
	}

	if (ipcSemInitialized == 0)
	{
		if (sem_init(&ipcSem, 0, 0) < 0)
		{
			putSysErrmsg("Can't initialize IPC semaphore", NULL);
			return NULL;
		}

		/*	Initialize the semaphore system.		*/

		oK(_semTbl());
		ipcSemInitialized = 1;
		giveIpcLock();
	}

	return &ipcSem;
}

int	sm_ipc_init()
{
	if (_ipcSemaphore(0) == NULL)
	{
		putErrmsg("Can't initialize IPC.", NULL);
		return -1;
	}

	return 0;
}

void	sm_ipc_stop()
{
	oK(_ipcSemaphore(1));
}

static void	takeIpcLock()
{
	oK(sem_wait(_ipcSemaphore(0)));

}

static void	giveIpcLock()
{
	oK(sem_post(_ipcSemaphore(0)));
}

sm_SemId	sm_SemCreate(int key, int semType)
{
	SmSem	*semTbl = _semTbl();
	int	i;
	SmSem	*sem;

	/*	If key is not specified, invent one.			*/

	if (key == SM_NO_KEY)
	{
		key = sm_GetUniqueKey();
	}

	takeIpcLock();
	for (i = 0, sem = semTbl; i < SEM_NSEMS_MAX; i++, sem++)
	{
		if (sem->key == key)
		{
			giveIpcLock();
			return i;
		}
	}

	for (i = 0, sem = semTbl; i < SEM_NSEMS_MAX; i++, sem++)
	{
		if (sem->id == NULL)	/*	Not in use.		*/
		{
			if (sem_init(&(sem->semobj), 0, 0) < 0)
			{
				giveIpcLock();
				putSysErrmsg("Can't init semaphore", NULL);
				return SM_SEM_NONE;
			}

			sem->id = &sem->semobj;
			sem->key = key;
			sem->ended = 0;
			sm_SemGive(i);	/*	(First taker succeeds.)	*/
			giveIpcLock();
			return i;
		}
	}

	giveIpcLock();
	putErrmsg("Too many semaphores.", itoa(SEM_NSEMS_MAX));
	return SM_SEM_NONE;
}

void	sm_SemDelete(sm_SemId i)
{
	SmSem	*semTbl = _semTbl();
	SmSem	*sem = semTbl + i;

	CHKVOID(i >= 0);
	CHKVOID(i < SEM_NSEMS_MAX);
	takeIpcLock();
	if (sem_destroy(&(sem->semobj)) < 0)
	{
		giveIpcLock();
		putSysErrmsg("Can't destroy semaphore", itoa(i));
		return;
	}

	sem->id = NULL;
	sem->key = SM_NO_KEY;
	giveIpcLock();
}

int	sm_SemTake(sm_SemId i)
{
	SmSem	*semTbl = _semTbl();
	SmSem	*sem = semTbl + i;

	CHKERR(i >= 0);
	CHKERR(i < SEM_NSEMS_MAX);
	while (sem_wait(sem->id) < 0)
	{
		if (errno == EINTR)
		{
			continue;
		}

		putSysErrmsg("Can't take semaphore", itoa(i));
		return -1;
	}

	return 0;
}

void	sm_SemGive(sm_SemId i)
{
	SmSem	*semTbl = _semTbl();
	SmSem	*sem = semTbl + i;

	CHKVOID(i >= 0);
	CHKVOID(i < SEM_NSEMS_MAX);
	if (sem_post(sem->id) < 0)
	{
		putSysErrmsg("Can't give semaphore", itoa(i));
	}
}

void	sm_SemEnd(sm_SemId i)
{
	SmSem	*semTbl = _semTbl();
	SmSem	*sem = semTbl + i;

	CHKVOID(i >= 0);
	CHKVOID(i < SEM_NSEMS_MAX);
	sem->ended = 1;
	sm_SemGive(i);
}

int	sm_SemEnded(sm_SemId i)
{
	SmSem	*semTbl = _semTbl();
	SmSem	*sem = semTbl + i;
	int	ended;

	CHKZERO(i >= 0);
	CHKZERO(i < SEM_NSEMS_MAX);
	ended = sem->ended;
	if (ended)
	{
		sm_SemGive(i);	/*	Enable multiple tests.		*/
	}

	return ended;
}

void	sm_SemUnend(sm_SemId i)
{
	SmSem	*semTbl = _semTbl();
	SmSem	*sem = semTbl + i;

	CHKVOID(i >= 0);
	CHKVOID(i < SEM_NSEMS_MAX);
	sem->ended = 0;
}

int	sm_SemUnwedge(sm_SemId i, int timeoutSeconds)
{
	SmSem		*semTbl = _semTbl();
	SmSem		*sem = semTbl + i;
	struct timespec	timeout;

	CHKERR(i >= 0);
	CHKERR(i < SEM_NSEMS_MAX);
	if (timeoutSeconds < 1) timeoutSeconds = 1;
	oK(clock_gettime(CLOCK_REALTIME, &timeout));
	timeout.tv_sec += timeoutSeconds;
	while (sem_timedwait(sem->id, &timeout) < 0)
	{
		switch (errno)
		{
		case EINTR:
			continue;
			
		case ETIMEDOUT:
			break;	/*	Out of switch.			*/

		default:
			putSysErrmsg("Can't unwedge semaphore", itoa(i));
			return -1;
		}

		break;		/*	Out of loop.			*/
	}

	if (sem_post(sem->id) < 0)
	{
		putSysErrmsg("Can't unwedge semaphore", itoa(i));
		return -1;
	}

	return 0;
}

#endif			/*	End of #ifdef POSIX_SEMAPHORES		*/

#ifdef SVR4_SEMAPHORES

	/* ---- Semaphore services (SVR4) -----------------------------	*/

#ifndef SM_SEMKEY
#define SM_SEMKEY	(0xee01)
#endif
#ifndef SM_SEMBASEKEY
#define SM_SEMBASEKEY	(0xee02)
#endif

/*	Note: one semaphore set is consumed by the ipcSemaphore.	*/
#define MAX_SEM_SETS	(SEMMNI - 1)

typedef struct
{
	int		semid;
	int		idsAllocated;
} IciSemaphoreSet;

/*	Note: we can actually always compute a semaphore's semSetIdx
 *	and semNbr from a sm_SemId (they are sm_SemId/SEMMSL and
 *	sm_SemId%SEMMSL), but we store the precomputed values to
 *	avoid having to do all that integer division; should make
 *	taking and releasing semaphores somewhat faster.		*/

typedef struct
{
	int		key;
	int		semSetIdx;
	int		semNbr;
	int		inUse;
	int		ended;
} IciSemaphore;

typedef struct
{
	IciSemaphoreSet	semSets[MAX_SEM_SETS];
	int		currSemSet;
	IciSemaphore	semaphores[SEMMNS];
	int		idsAllocated;
} SemaphoreBase;

static SemaphoreBase	*_sembase(int stop)
{
	static SemaphoreBase	*semaphoreBase = NULL;
	static uaddr		sembaseId = 0;
	int			semSetIdx;
	IciSemaphoreSet		*semset;
	int			i;

	if (stop)
	{
		if (semaphoreBase != NULL)
		{
			semSetIdx = 0;
			while (semSetIdx < MAX_SEM_SETS)
			{
				semset = semaphoreBase->semSets + semSetIdx;
				oK(semctl(semset->semid, 0, IPC_RMID, NULL));
            			semSetIdx++;
			}

			sm_ShmDestroy(sembaseId);
			semaphoreBase = NULL;
		}

		return NULL;
	}

	if (semaphoreBase == NULL)
	{
		switch (sm_ShmAttach(SM_SEMBASEKEY, sizeof(SemaphoreBase),
				(char **) &semaphoreBase, &sembaseId))
		{
		case -1:
			putErrmsg("Can't create semaphore base.", NULL);
			break;

		case 0:	
			break;		/*	SemaphoreBase exists.	*/

		default:		/*	New SemaphoreBase.	*/
			semaphoreBase->idsAllocated = 0;
			semaphoreBase->currSemSet = 0;
			for (i = 0; i < MAX_SEM_SETS; i++)
			{
				semaphoreBase->semSets[i].semid = -1;
			}

			/*	Acquire initial semaphore set.		*/

			semset = semaphoreBase->semSets
					+ semaphoreBase->currSemSet;
			semset->semid = semget(sm_GetUniqueKey(), SEMMSL,
					IPC_CREAT | 0666);
			if (semset->semid < 0)
			{
				putSysErrmsg("Can't get initial semaphore set",
						NULL);
				sm_ShmDestroy(sembaseId);
				semaphoreBase = NULL;
				break;
			}

			semset->idsAllocated = 0;
		}
	}

	return semaphoreBase;
}

/*	Note that the ipcSemaphore gets an entire semaphore set for
 *	itself.  This is necessary for bootstrapping the ICI svr4-
 *	based semaphore system: only after the ipcSemaphore exists
 *	can we initialize the semaphore base, enabling subsequent
 *	semaphores to be allocated more efficiently.			*/

static int	_ipcSemaphore(int stop)
{
	static int	ipcSem = -1;

	if (stop)
	{
		oK(_sembase(1));
		if (ipcSem != -1)
		{
			oK(semctl(ipcSem, 0, IPC_RMID, NULL));
			ipcSem = -1;
		}

		return ipcSem;
	}

	if (ipcSem == -1)
	{
		ipcSem = semget(SM_SEMKEY, 1, IPC_CREAT | 0666);
		if (ipcSem == -1)
		{
			putSysErrmsg("Can't initialize IPC semaphore",
					itoa(SM_SEMKEY));
		}
		else
		{
			if (_sembase(0) == NULL)
			{
				oK(semctl(ipcSem, 0, IPC_RMID, NULL));
				ipcSem = -1;
			}
		}
	}

	return ipcSem;
}

int	sm_ipc_init()
{
	if (_ipcSemaphore(0) == -1)
	{
		putErrmsg("Can't initialize IPC.", NULL);
		return -1;
	}

	return 0;
}

void	sm_ipc_stop()
{
	oK(_ipcSemaphore(1));
}

static void	takeIpcLock()
{
	struct sembuf	sem_op[2] = { {0,0,0}, {0,1,0} };

	oK(semop(_ipcSemaphore(0), sem_op, 2));
}

static void	giveIpcLock()
{
	struct sembuf	sem_op = { 0, -1, IPC_NOWAIT };

	oK(semop(_ipcSemaphore(0), &sem_op, 1));
}

sm_SemId	sm_SemCreate(int key, int semType)
{
	SemaphoreBase	*sembase;
	int		i;
	IciSemaphore	*sem;
	IciSemaphoreSet	*semset;
	int		semSetIdx;
	int		semid;

	/*	If key is not specified, invent one.			*/

	if (key == SM_NO_KEY)
	{
		key = sm_GetUniqueKey();
	}

	/*	Look through list of all existing ICI semaphores.	*/

	takeIpcLock();
	sembase = _sembase(0);
	if (sembase == NULL)
	{
		giveIpcLock();
		putErrmsg("No semaphore base.", NULL);
		return SM_SEM_NONE;
	}

	for (i = 0, sem = sembase->semaphores; i < sembase->idsAllocated;
			i++, sem++)
	{
		if (sem->key == key)
		{
			giveIpcLock();
			return i;	/*	already created		*/
		}
	}

	/*	No existing semaphore for this key; repurpose one
	 *	that is unused or allocate the next one in the current
	 *	semaphore set.						*/

	semset = sembase->semSets + sembase->currSemSet;
	for (i = 0, sem = sembase->semaphores; i < SEMMNS; i++, sem++)
	{
		if (sem->inUse)
		{
			continue;
		}

		/*	Found available slot in table.			*/

		sem->inUse = 1;
		sem->key = key;
		sem->ended = 0;
		if (i >= sembase->idsAllocated)
		{
			/*	Must allocate new semaphore ID in slot.	*/

			sem->semSetIdx = sembase->currSemSet;
			sem->semNbr = semset->idsAllocated;
			semset->idsAllocated++;
			sembase->idsAllocated++;
		}

		sm_SemGive(i);		/*	(First taker succeeds.)	*/

		/*	Acquire next semaphore set if necessary.	*/

		if (semset->idsAllocated == SEMMSL)
		{
			/*	Must acquire another semaphore set.	*/

			semSetIdx = sembase->currSemSet + 1;
			if (semSetIdx == MAX_SEM_SETS)
			{
				giveIpcLock();
				putErrmsg("Too many semaphore sets, can't \
manage the new one.", NULL);
				return SM_SEM_NONE;
			}

			semid = semget(sm_GetUniqueKey(), SEMMSL,
					IPC_CREAT | 0666);
			if (semid < 0)
			{
				giveIpcLock();
				putSysErrmsg("Can't get semaphore set", NULL);
				return SM_SEM_NONE;
			}

			sembase->currSemSet = semSetIdx;
			semset = sembase->semSets + semSetIdx;
			semset->semid = semid;
			semset->idsAllocated = 0;
		}

		giveIpcLock();
		return i;
	}

	giveIpcLock();
	putErrmsg("Can't add any more semaphores; table full.", NULL);
	return SM_SEM_NONE;
}


void	sm_SemDelete(sm_SemId i)
{
	SemaphoreBase	*sembase = _sembase(0);
	IciSemaphore	*sem;

	CHKVOID(sembase);
	CHKVOID(i >= 0);
	CHKVOID(i < sembase->idsAllocated);
	sem = sembase->semaphores + i;
	takeIpcLock();

	/*	Note: the semSetIdx and semNbr in the semaphore
	 *	don't need to be deleted; they constitute a
	 *	semaphore ID that will be reassigned later when
	 *	this semaphore object is allocated to a new use.	*/

	sem->inUse = 0;
	sem->key = SM_NO_KEY;
	giveIpcLock();
}

int	sm_SemTake(sm_SemId i)
{
	SemaphoreBase	*sembase = _sembase(0);
	IciSemaphore	*sem;
	IciSemaphoreSet	*semset;
	struct sembuf	sem_op[2] = { {0,0,0}, {0,1,0} };

	CHKERR(sembase);
	CHKERR(i >= 0);
	CHKERR(i < sembase->idsAllocated);
	sem = sembase->semaphores + i;
	if (sem->key == -1)	/*	semaphore deleted		*/
	{
		putErrmsg("Can't take deleted semaphore.", itoa(i));
		return -1;
	}

	semset = sembase->semSets + sem->semSetIdx;
	sem_op[0].sem_num = sem_op[1].sem_num = sem->semNbr;
	while (semop(semset->semid, sem_op, 2) < 0)
	{
		if (errno == EINTR)
		{
			/*Retry on Interruption by signal*/
			continue;
		} else {
			putSysErrmsg("Can't take semaphore", itoa(i));
			return -1;
		}
	}

	return 0;
}

void	sm_SemGive(sm_SemId i)
{
	SemaphoreBase	*sembase = _sembase(0);
	IciSemaphore	*sem;
	IciSemaphoreSet	*semset;
	struct sembuf	sem_op = { 0, -1, IPC_NOWAIT };

	CHKVOID(sembase);
	CHKVOID(i >= 0);
	CHKVOID(i < sembase->idsAllocated);
	sem = sembase->semaphores + i;
	if (sem->key == -1)	/*	semaphore deleted		*/
	{
		return;
	}

	semset = sembase->semSets + sem->semSetIdx;
	sem_op.sem_num = sem->semNbr;
	if (semop(semset->semid, &sem_op, 1) < 0)
	{
		if (errno != EAGAIN)
		{
			putSysErrmsg("Can't give semaphore", itoa(i));
		}
	}
}

void	sm_SemEnd(sm_SemId i)
{
	SemaphoreBase	*sembase = _sembase(0);
	IciSemaphore	*sem;

	CHKVOID(sembase);
	CHKVOID(i >= 0);
	CHKVOID(i < sembase->idsAllocated);
	sem = sembase->semaphores + i;
	sem->ended = 1;
	sm_SemGive(i);
}

int	sm_SemEnded(sm_SemId i)
{
	SemaphoreBase	*sembase = _sembase(0);
	IciSemaphore	*sem;
	int		ended;

	CHKZERO(sembase);
	CHKZERO(i >= 0);
	CHKZERO(i < sembase->idsAllocated);
	sem = sembase->semaphores + i;
	ended = sem->ended;
	if (ended)
	{
		sm_SemGive(i);	/*	Enable multiple tests.		*/
	}

	return ended;
}

void	sm_SemUnend(sm_SemId i)
{
	SemaphoreBase	*sembase = _sembase(0);
	IciSemaphore	*sem;

	CHKVOID(sembase);
	CHKVOID(i >= 0);
	CHKVOID(i < sembase->idsAllocated);
	sem = sembase->semaphores + i;
	sem->ended = 0;
}

static void	handleTimeout(int signum)
{
	return;
}

int	sm_SemUnwedge(sm_SemId i, int timeoutSeconds)
{
	SemaphoreBase	*sembase = _sembase(0);
	IciSemaphore	*sem;
	IciSemaphoreSet	*semset;
	struct sembuf	sem_op[3] = { {0,0,0}, {0,1,0}, {0,-1,IPC_NOWAIT} };

	CHKERR(sembase);
	CHKERR(i >= 0);
	CHKERR(i < sembase->idsAllocated);
	sem = sembase->semaphores + i;
	if (sem->key == -1)	/*	semaphore deleted		*/
	{
		putErrmsg("Can't unwedge deleted semaphore.", itoa(i));
		return -1;
	}

	semset = sembase->semSets + sem->semSetIdx;
	sem_op[0].sem_num = sem_op[1].sem_num = sem_op[2].sem_num = sem->semNbr;
	if (timeoutSeconds < 1) timeoutSeconds = 1;
	isignal(SIGALRM, handleTimeout);
	oK(alarm(timeoutSeconds));
	if (semop(semset->semid, sem_op, 2) < 0)
	{
		if (errno != EINTR)
		{
			putSysErrmsg("Can't take semaphore", itoa(i));
			return -1;
		}
		/*Intentionally don't retry if EINTR... That means the
		 *alarm we just set went off... We're going to proceed anyway.*/
	}

	oK(alarm(0));
	isignal(SIGALRM, SIG_DFL);
	if (semop(semset->semid, sem_op + 2, 1) < 0)
	{
		if (errno != EAGAIN)
		{
			putSysErrmsg("Can't give semaphore", itoa(i));
			return -1;
		}
	}

	return 0;
}

#endif			/*	End of #ifdef SVR4_SEMAPHORES		*/

/************************* Symbol table services  *****************************/

#ifdef PRIVATE_SYMTAB

extern FUNCPTR	sm_FindFunction(char *name, int *priority, int *stackSize);

#if defined (FSWSYMTAB) || defined (GDSSYMTAB)
#include "mysymtab.c"
#else
#include "symtab.c"
#endif

#endif

/****************** Task control services *************************************/

#ifdef VXWORKS_TASKS

	/* ---- Task Control services (VxWorks) ----------------------- */

int	sm_TaskIdSelf()
{
	return (taskIdSelf());
}

int	sm_TaskExists(int task)
{
	if (taskIdVerify(task) == OK)
	{
		return 1;
	}

	return 0;
}

void	*sm_TaskVar(void **arg)
{
	static void	*value;

	if (arg != NULL)
	{
		/*	Set value by dereferencing argument.		*/

		value = *arg;
		taskVarAdd(0, (int *) &value);
	}

	return value;
}

void	sm_TaskSuspend()
{
	if (taskSuspend(sm_TaskIdSelf()) == ERROR)
	{
		putSysErrmsg("Can't suspend task (self)", NULL);
	}
}

void	sm_TaskDelay(int seconds)
{
	if (taskDelay(seconds * sysClkRateGet()) == ERROR)
	{
		putSysErrmsg("Can't pause task", itoa(seconds));
	}
}

void	sm_TaskYield()
{
	taskDelay(0);
}

int	sm_TaskSpawn(char *name, char *arg1, char *arg2, char *arg3,
		char *arg4, char *arg5, char *arg6, char *arg7, char *arg8,
		char *arg9, char *arg10, int priority, int stackSize)
{
	char	namebuf[33];
	FUNCPTR	entryPoint;
	int	result;
#ifdef PRIVATE_SYMTAB

	CHKERR(name);
	if ((entryPoint = sm_FindFunction(name, &priority, &stackSize)) == NULL)
	{
		isprintf(namebuf, sizeof namebuf, "_%s", name);
		if ((entryPoint = sm_FindFunction(namebuf, &priority,
				&stackSize)) == NULL)
		{
			putErrmsg("Can't spawn task; function not in private \
symbol table; must be added to mysymtab.c.", name);
			return -1;
		}
	}
#else
	SYM_TYPE	type;

	CHKERR(name);
	if (symFindByName(sysSymTbl, name, (char **) &entryPoint, &type)
			== ERROR)
	{
		isprintf(namebuf, sizeof namebuf, "_%s", name);
		if (symFindByName(sysSymTbl, namebuf, (char **) &entryPoint,
				&type) == ERROR)
		{
			putSysErrmsg("Can't spawn task; function not in \
VxWorks symbol table", name);
			return -1;
		}
	}

	if (priority <= 0)
	{
		priority = ICI_PRIORITY;
	}

	if (stackSize <= 0)
	{
		stackSize = 32768;
	}
#endif

#ifdef FSWSCHEDULER
#include "fswspawn.c"
#else
	result = taskSpawn(name, priority, VX_FP_TASK, stackSize, entryPoint,
			(int) arg1, (int) arg2, (int) arg3, (int) arg4,
			(int) arg5, (int) arg6, (int) arg7, (int) arg8,
			(int) arg9, (int) arg10);
#endif
	if (result == ERROR)
	{
		putSysErrmsg("Failed spawning task", name);
	}
	else
	{
		TRACK_BORN(result);
	}

	return result;
}

void	sm_TaskKill(int task, int sigNbr)
{
	oK(kill(task, sigNbr));
}

void	sm_TaskDelete(int task)
{
	if (taskIdVerify(task) != OK)
	{
		writeMemoNote("[?] Can't delete nonexistent task", itoa(task));
		return;
	}

	TRACK_DIED(task);
	oK(taskDelete(task));
}

void	sm_Abort()
{
	oK(tt(taskIdSelf()));
	snooze(2);
	TRACK_DIED(task);
	oK(taskDelete(taskIdSelf()));
}

#endif			/*	End of #ifdef VXWORKS_TASKS		*/

/*	Thread management machinery for bionic and uClibc, both of
 *	which lack pthread_cancel.					*/

#if defined (bionic) || defined (uClibc)

typedef struct
{
	void	*(*function)(void *);
	void	*arg;
} IonPthreadParm;

static void	posixTaskExit(int sig)
{
#if defined(bionic)
	int task_id = sm_TaskIdSelf();
	sm_TaskForget(task_id);
#endif
	pthread_exit(0);
}

static void	sm_ArmPthread()
{
	struct sigaction	actions;

	memset((char *) &actions, 0, sizeof actions);
	sigemptyset(&actions.sa_mask);
	actions.sa_flags = 0;
	actions.sa_handler = posixTaskExit;
	oK(sigaction(SIGUSR2, &actions, NULL));
}

void	sm_EndPthread(pthread_t threadId)
{
	/*	NOTE that this is NOT a faithful implementation of
	 *	pthread_cancel(); there is no support for deferred
	 *	thread cancellation in Bionic (the Android subset
	 *	of Linux).  It's just a code simplification, solely
	 *	for the express, limited purpose of shutting down a
	 *	task immediately, under the highly constrained
	 *	circumstances defined by sm_TaskSpawn, sm_TaskDelete,
	 *	and sm_Abort, below.					*/

	oK(pthread_kill(threadId, SIGUSR2));
}

static void	*posixTaskEntrance(void *taskArg)
{
	IonPthreadParm	*parm = (IonPthreadParm *) taskArg;
	void 		*(*function)(void *);
	void		*arg;

	/*	Copy the information in parm into local stack
	 *	variables, then free space allocated to parm.		*/

	CHKNULL(parm);
	function = parm->function;
	arg = parm->arg;
	free(parm);

	/*	Now initiate processing in the new task.		*/

	sm_ArmPthread();
	return (function)(arg);
}

int	sm_BeginPthread(pthread_t *threadId, const pthread_attr_t *attr,
		void *(*function)(void *), void *arg)
{
	IonPthreadParm	*parm;
	int		result;

	/*	Store thread parameters in space allocated from
	 *	main memory, in case caller exits.			*/

	parm = (IonPthreadParm *) malloc(sizeof(IonPthreadParm));
	if (parm == NULL)
	{
		putErrmsg("Can't allocate space for thread parameters.", NULL);
		return -1;
	}

	parm->function = function;
	parm->arg = arg;
	result = pthread_create(threadId, attr, posixTaskEntrance, parm);

	/*	Free the memory allocated for parm if the creation
	 *	of the new thread has failed.  Need to do this to
	 *	prevent memory leak.					*/

	if (result != 0)
	{
		free(parm);
	}

	return result;
}

int	sm_BeginPthread_named(pthread_t *threadId, const pthread_attr_t *attr,
		void *(*function)(void *), void *arg, const char *name)
{
	int		result;

	result = sm_BeginPthread(threadId, attr, function, arg);
#if defined(bionic)
	pthread_setname_np(*threadId, name);
#endif

	return result;
}

#else		/*	Not bionic and not uClibc.			*/

#ifdef darwin
/* struct used to wrap start_routine with naming_start_routine */
typedef struct
{
	char name[100];
	void *arg;
	void *(*start_routine) (void *);
} NamingParms;
/* protect multiple threads from accessing NamingParms in darwin */
static pthread_mutex_t NamingParmsSem = PTHREAD_MUTEX_INITIALIZER;


static void *naming_start_routine(void *parm){
	NamingParms	*nmp = (NamingParms *) parm;
	const char *name = nmp->name;
	void *arg = nmp->arg; 
	void *(*start_routine) (void *) = nmp->start_routine;
	void* ret;
	pthread_setname_np(name);
	/* release the mutex protecting the shared naming structure */
	pthread_mutex_unlock(&NamingParmsSem);
	
	ret = (*start_routine)(arg);
	return ret;
}
#endif

int pthread_begin_named(pthread_t *thread, const pthread_attr_t *attr,
		void *(*start_routine) (void *), void *arg, const char *name)
{
	int result;

	/*	VxWorks uses a different method of naming threads.	*/
#ifdef vxworks
	if(attr){
		pthread_attr_setname(attr, name);
		result = pthread_begin(thread, attr, start_routine, arg);
	}else{
		pthread_attr_t tattr;
		pthread_attr_init(&tattr);
		pthread_attr_setname(tattr, name);
		result = pthread_begin(thread, &tattr, start_routine, arg);
	}
	
	/*	Supported platforms for naming threads			*/
#elif darwin
	static NamingParms nmp;
	/*	In OSX, pthread_setname_np must be called within the 
	 *	the thread you wish to name. Achieved by wrapping 
	 *	the start_routine of pthread_begin.			*/

	/* protect the global naming structure from concurrent access	*/
	pthread_mutex_lock(&NamingParmsSem);

	nmp.start_routine = start_routine;
	nmp.arg = arg;
	strncpy(nmp.name, name, sizeof(nmp.name)-1);
	result = pthread_begin(thread, attr, naming_start_routine, &nmp);
#else		/*	Not vxworks and not darwin.			*/

#ifdef SOLARIS_COMPILER
	result = pthread_create(thread, attr, start_routine, arg);
#else
	result = pthread_begin(thread, attr, start_routine, arg);
#endif

#if defined(linux) || defined(mingw)
	pthread_setname_np(*thread, name);
#elif defined(freebsd)
	pthread_set_name_np(*thread,name);
#endif	/*	End of #if linux || mingw.				*/
#endif	/*	End of #ifdef vxworks.					*/

	return result;
}

#endif	/*	End of #if defined bionic || uClibc			*/

#ifdef POSIX_TASKS

/*	Note: the RTEMS API is UNIX-like except that it omits all SVR4
 *	features.  RTEMS uses POSIX semaphores, and its shared-memory
 *	mechanism is the same as the one we use for VxWorks.  The same
 *	is true of Bionic.  CFS may be either UNIX or VXWORKS, but its
 *	task model is always threads just like RTEMS and bionic.	*/

#include <sys/stat.h>
#include <sched.h>

	/* ---- Task Control services (POSIX) ------------------------- */

#ifndef	MAX_POSIX_TASKS
#define MAX_POSIX_TASKS	50
#endif

typedef struct
{
	int		inUse;		/*	Boolean.		*/
	pthread_t	threadId;
	void		*value;		/*	Task variable value.	*/
} PosixTask;

static void	*_posixTasks(int *taskId, pthread_t *threadId, void **arg)
{
	static PosixTask	tasks[MAX_POSIX_TASKS];
	static int		initialized = 0;/*	Boolean.	*/
	static ResourceLock	tasksLock;
	pthread_t		ownThreadId;
	int			i;
	int			vacancy;
	PosixTask		*task;
	void			*value;

	/*	NOTE: the taskId for a PosixTask is 1 more than
	 *	the index value for that task in the tasks table.
	 *	That is, taskIds range from 1 through MAX_POSIX_TASKS
	 *	and -1 is an invalid task ID signifying "none".		*/

	if (!initialized)
	{
		memset((char *) tasks, 0, sizeof tasks);
		if (initResourceLock(&tasksLock) < 0)
		{
			putErrmsg("Can't initialize POSIX tasks table.", NULL);
			return NULL;
		}

		initialized = 1;
	}

#if defined(bionic)

	/*	Special case for bionic: need to clear the task
	 *	table at ION shutdown, signaled by NULL taskId.		*/

	if (taskId == NULL && threadId == NULL)
	{
		lockResource(&tasksLock);

		/*	Let threads shut down properly.			*/

		microsnooze(2500);

		/*	Stop all tasks.					*/

		for (i = 0, task = tasks; i < MAX_POSIX_TASKS; i++, task++)
		{
			if (task->inUse
			&& task->threadId != 0
			&& task->threadId != pthread_self())
			{
				writeMemo("[?] Task still running, sending \
SIGTERM.");
				pthread_kill(task->threadId, SIGTERM);
			}
		}

		/*	Now reinitialize the task table.		*/

		memset((char *) tasks, 0, sizeof tasks);
		unlockResource(&tasksLock);
		return NULL;
	}
#endif

	/*	taskId must never be NULL; it is always needed.		*/

	CHKNULL(taskId);
	lockResource(&tasksLock);

	/*	When *taskId is 0, processing depends on the value
	 *	of threadID.  If threadId is NULL, then the task ID
	 *	of the calling thread (0 if the thread doesn't have
	 *	an assigned task ID) is written into *taskId.
	 *	Otherwise, the thread identified by *threadId is
	 *	added as a new task and the ID of that task (-1
	 *	if the thread could not be assigned a task ID) is
	 *	written into *taskId.  In either case, NULL is
	 *	returned.
	 *
	 *	Otherwise, *taskId must be in the range 1 through
	 *	MAX_POSIX_TASKS inclusive and processing again
	 *	depends on the value of threadId.  If threadId is
	 *	NULL then the indicated task ID is unassigned and
	 *	is available for reassignment to another thread;
	 *	-1 is written into *taskId and NULL is returned.
	 *	Otherwise:
	 *
	 *		The thread ID for the indicated task is
	 *		written into *threadId.
	 *
	 *		If arg is non-NULL, then the task variable
	 *		value for the indicated task is set to *arg.
	 *
	 *		The current value of the indicated task's
	 *		task variable is returned.			*/

	if (*taskId == 0)
	{
		if (threadId == NULL)	/*	Look up own task ID.	*/
		{
			ownThreadId = pthread_self();
			for (i = 0, task = tasks; i < MAX_POSIX_TASKS;
					i++, task++)
			{
				if (task->inUse == 0)
				{
					continue;
				}

				if (pthread_equal(task->threadId, ownThreadId))
				{
					*taskId = i + 1;
					unlockResource(&tasksLock);
					return NULL;
				}
			}

			/*	No task ID for this thread; sub-thread
			 *	of a task.				*/

			unlockResource(&tasksLock);
			return NULL;
		}

		/*	Assigning a task ID to this thread.		*/

		vacancy = -1;
		for (i = 0, task = tasks; i < MAX_POSIX_TASKS; i++, task++)
		{
			if (task->inUse == 0)
			{
				if (vacancy == -1)
				{
					vacancy = i;
				}
			}
			else
			{
				if (pthread_equal(task->threadId, *threadId))
				{
					/*	Already assigned.	*/

					*taskId = i + 1;
					unlockResource(&tasksLock);
					return NULL;
				}
			}
		}

		if (vacancy == -1)
		{
			putErrmsg("Can't start another task.", NULL);
			*taskId = -1;
			unlockResource(&tasksLock);
			return NULL;
		}

		task = tasks + vacancy;
		task->inUse = 1;
		task->threadId = *threadId;
		task->value = NULL;
		*taskId = vacancy + 1;
		unlockResource(&tasksLock);
		return NULL;
	}

	/*	Operating on a previously assigned task ID.		*/

	CHKNULL((*taskId) > 0 && (*taskId) <= MAX_POSIX_TASKS);
	task = tasks + ((*taskId) - 1);
	if (threadId == NULL)	/*	Unassigning this task ID.	*/
	{
		if (task->inUse)
		{
			task->inUse = 0;
		}

		*taskId = -1;
		unlockResource(&tasksLock);
		return NULL;
	}

	/*	Just looking up the thread ID for this task ID and/or
	 *	operating on task variable.				*/

	if (task->inUse == 0)	/*	Invalid task ID.		*/
	{
		*taskId = -1;
		unlockResource(&tasksLock);
		return NULL;
	}

	*threadId = task->threadId;
	if (arg)
	{
		task->value = *arg;
	}

	value = task->value;
	unlockResource(&tasksLock);
	return value;
}

int	sm_TaskIdSelf()
{
	int		taskId = 0;
	pthread_t	threadId;

	oK(_posixTasks(&taskId, NULL, NULL));
	if (taskId > 0)
	{
		return taskId;
	}

	/*	May be a newly spawned task.  Give sm_TaskSpawn
	 *	an opportunity to register the thread as a task.	*/

	sm_TaskYield();
	oK(_posixTasks(&taskId, NULL, NULL));
	if (taskId > 0)
	{
		return taskId;
	}

	/*	This is a subordinate thread of some other task.
	 *	It needs to register itself as a task.			*/

	threadId = pthread_self();
	oK(_posixTasks(&taskId, &threadId, NULL));
	return taskId;
}

int	sm_TaskExists(int taskId)
{
	pthread_t	threadId;

	oK(_posixTasks(&taskId, &threadId, NULL));
	if (taskId < 0)
	{
		return 0;		/*	No such task.		*/
	}

#if defined(bionic)
	return 1;			/*	Assume thread running.	*/
#endif

	/*	(Signal 0 in pthread_kill is rejected by RTEMS 4.9.)	*/

	if (pthread_kill(threadId, SIGCONT) == 0)
	{
		return 1;		/*	Thread is running.	*/
	}

	/*	Note: RTEMS 4.9 implementation of pthread_kill does
	 *	not return a valid errno on failure; can't print
	 *	system error message.					*/

	return 0;	/*	No such thread, or some other failure.	*/
}

void	*sm_TaskVar(void **arg)
{
	int		taskId = sm_TaskIdSelf();
	pthread_t	threadId;

	return _posixTasks(&taskId, &threadId, arg);
}

void	sm_TaskSuspend()
{
	pause();
}

void	sm_TaskDelay(int seconds)
{
	sleep(seconds);
}

void	sm_TaskYield()
{
	sched_yield();
}

#ifndef MAX_SPAWNS
#if defined(bionic)
#define	MAX_SPAWNS	16
#else
#define	MAX_SPAWNS	8
#endif
#endif

typedef struct
{
	FUNCPTR	threadMainFunction;
	saddr	arg1;
	saddr	arg2;
	saddr	arg3;
	saddr	arg4;
	saddr	arg5;
	saddr	arg6;
	saddr	arg7;
	saddr	arg8;
	saddr	arg9;
	saddr	arg10;
} SpawnParms;

static void	*posixDriverThread(void *parm)
{
	SpawnParms	parms;

	/*	Make local copy of spawn parameters.			*/

	memcpy((char *) &parms, parm, sizeof(SpawnParms));

	/*	Clear spawn parameters for use by next sm_TaskSpawn().	*/

	memset((char *) parm, 0, sizeof(SpawnParms));

#if defined (bionic)
	/*	Set up SIGUSR2 handler to enable clean task shutdown.	*/

	sm_ArmPthread();
#endif
	/*	Run main function of thread.				*/

	parms.threadMainFunction(parms.arg1, parms.arg2, parms.arg3,
			parms.arg4, parms.arg5, parms.arg6,
			parms.arg7, parms.arg8, parms.arg9, parms.arg10);
#if defined(bionic)
	int task_id = sm_TaskIdSelf();
	sm_TaskForget(task_id);
#endif
	return NULL;
}

int	sm_TaskSpawn(char *name, char *arg1, char *arg2, char *arg3,
		char *arg4, char *arg5, char *arg6, char *arg7, char *arg8,
		char *arg9, char *arg10, int priority, int stackSize)
{
	char			namebuf[33];
	FUNCPTR			entryPoint;
	static SpawnParms	spawnsArray[MAX_SPAWNS];
	int			i;
	SpawnParms		*parms;
	pthread_attr_t		attr;
	pthread_t		threadId;
	int			taskId;

#ifdef PRIVATE_SYMTAB
	CHKERR(name);
	if ((entryPoint = sm_FindFunction(name, &priority, &stackSize)) == NULL)
	{
		isprintf(namebuf, sizeof namebuf, "_%s", name);
		if ((entryPoint = sm_FindFunction(namebuf, &priority,
				&stackSize)) == NULL)
		{
			putErrmsg("Can't spawn task; function not in \
private symbol table; must be added to mysymtab.c.", name);
			return -1;
		}
	}
#else
	putErrmsg("Can't spawn task; no ION private symbol table.", name);
	return -1;
#endif
	for (i = 0, parms = spawnsArray; i < MAX_SPAWNS; i++, parms++)
	{
		if (parms->threadMainFunction == NULL)
		{
			break;
		}
	}

	if (i == MAX_SPAWNS)
	{
		putErrmsg("Can't spawn task: no parms cleared yet.", NULL);
		return -1;
	}

	parms->threadMainFunction = entryPoint;
	parms->arg1 = (int) arg1;
	parms->arg2 = (int) arg2;
	parms->arg3 = (int) arg3;
	parms->arg4 = (int) arg4;
	parms->arg5 = (int) arg5;
	parms->arg6 = (int) arg6;
	parms->arg7 = (int) arg7;
	parms->arg8 = (int) arg8;
	parms->arg9 = (int) arg9;
	parms->arg10 = (int) arg10;
	sm_ConfigurePthread(&attr, stackSize);
	errno = pthread_create(&threadId, &attr, posixDriverThread,
			(void *) parms);
	if (errno)
	{
		putSysErrmsg("Failed spawning task", name);
		return -1;
	}

	taskId = 0;	/*	Requesting new task ID for thread.	*/
	oK(_posixTasks(&taskId, &threadId, NULL));
	if (taskId < 0)		/*	Too many tasks running.		*/
	{
		if (pthread_kill(threadId, SIGTERM) == 0)
		{
			oK(pthread_end(threadId));
			pthread_join(threadId, NULL);
		}

		return -1;
	}

	TRACK_BORN(taskId);
	return taskId;
}

void	sm_TaskForget(int taskId)
{
	oK(_posixTasks(&taskId, NULL, NULL));
}

void	sm_TaskKill(int taskId, int sigNbr)
{
	pthread_t	threadId;

	oK(_posixTasks(&taskId, &threadId, NULL));
	if (taskId < 0)
	{
		return;		/*	No such task.			*/
	}

	oK(pthread_kill(threadId, sigNbr));
}

void	sm_TaskDelete(int taskId)
{
	pthread_t	threadId;

	oK(_posixTasks(&taskId, &threadId, NULL));
	if (taskId < 0)
	{
		return;		/*	No such task.			*/
	}

	TRACK_DIED(taskId);
	if (pthread_kill(threadId, SIGTERM) == 0)
	{
		oK(pthread_end(threadId));
	}

	oK(_posixTasks(&taskId, NULL, NULL));
}

void	sm_TasksClear()
{
	_posixTasks(NULL, NULL, NULL);
}

void	sm_Abort()
{
	int		taskId = sm_TaskIdSelf();
	pthread_t	threadId;

	if (taskId < 0)		/*	Can't register as task.		*/
	{
		/*	Just terminate.					*/

		threadId = pthread_self();
		if (pthread_kill(threadId, SIGTERM) == 0)
		{
			oK(pthread_end(threadId));
		}

		return;
	}

	sm_TaskDelete(taskId);
}

#endif	/*	End of #ifdef POSIX_TASKS				*/

#ifdef MINGW_TASKS

	/* ---- Task Control services (mingw) ----------------------- */

int	sm_TaskIdSelf()
{
	return _getpid();
}

int	sm_TaskExists(int task)
{
	DWORD	processId = task;
	HANDLE	process;
	DWORD	status;
	BOOL	result;

	process = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);
	if (process == NULL)
	{
		return 0;
	}

	result = GetExitCodeProcess(process, &status);
	CloseHandle(process);
       	if (result == 0 || status != STILL_ACTIVE)
	{
		return 0;		/*	No such process.	*/
	}

	return 1;
}

void	*sm_TaskVar(void **arg)
{
	static void	*value;

	/*	Each Windows process has its own distinct instance
	 *	of each global variable, so all global variables
	 *	are automatically "task variables".			*/

	if (arg != NULL)
	{
		/*	Set value by dereferencing argument.		*/

		value = *arg;
	}

	return value;
}

void	sm_TaskSuspend()
{
	writeMemo("[?] ION for Windows doesn't support sm_TaskSuspend().");
}

void	sm_TaskDelay(int seconds)
{
	Sleep(seconds * 1000);
}

void	sm_TaskYield()
{
	Sleep(0);
}

int	sm_TaskSpawn(char *name, char *arg1, char *arg2, char *arg3,
		char *arg4, char *arg5, char *arg6, char *arg7, char *arg8,
		char *arg9, char *arg10, int priority, int stackSize)
{
	STARTUPINFO		si;
	PROCESS_INFORMATION	pi;
	char			cmdLine[256];

	CHKERR(name);
	ZeroMemory(&si, sizeof si);
	si.cb = sizeof si;
	ZeroMemory(&pi, sizeof pi);
	if (arg1 == NULL) arg1 = "";
	if (arg2 == NULL) arg2 = "";
	if (arg3 == NULL) arg3 = "";
	if (arg4 == NULL) arg4 = "";
	if (arg5 == NULL) arg5 = "";
	if (arg6 == NULL) arg6 = "";
	if (arg7 == NULL) arg7 = "";
	if (arg8 == NULL) arg8 = "";
	if (arg9 == NULL) arg9 = "";
	if (arg10 == NULL) arg10 = "";
	isprintf(cmdLine, sizeof cmdLine,
			"\"%s\" %s %s %s %s %s %s %s %s %s %s",
			name, arg1, arg2, arg3, arg4, arg5,
			arg6, arg7, arg8, arg9, arg10);
	if (CreateProcess(NULL, cmdLine, NULL, NULL, FALSE, 0, NULL, NULL,
			&si, &pi) == 0)
	{
		putSysErrmsg("Can't create process", cmdLine);
		return -1;
	}

	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
	return pi.dwProcessId;
}

void	sm_TaskKill(int task, int sigNbr)
{
	char	eventName[32];
	HANDLE	event;
	BOOL	result;

	if (task <= 1)
	{
		writeMemoNote("[?] Can't delete invalid process ID",
				itoa(task));
		return;
	}

	if (sigNbr != SIGTERM)
	{
		writeMemoNote("[?] ION for Windows only delivers SIGTERM",
				itoa(sigNbr));
		return;
	}

	sprintf(eventName, "%d.sigterm", task);
	event = OpenEvent(EVENT_ALL_ACCESS, FALSE, eventName);
	if (event)
	{
		result = SetEvent(event);
		CloseHandle(event);
		if (result == 0)
		{
			putErrmsg("Can't set SIGTERM event.",
					utoa(GetLastError()));
		}
	}
	else
	{
		putErrmsg("Can't open SIGTERM event.", utoa(GetLastError()));
	}
}

void	sm_TaskDelete(int task)
{
	DWORD	processId = task;
	HANDLE	process;
	BOOL	result;

	sm_TaskKill(task, SIGTERM);
	Sleep(1000);
	process = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);
	if (process)
	{
		result = TerminateProcess(process, 0);
		CloseHandle(process);
		if (result == 0)
		{
			putErrmsg("Can't terminate process.",
					utoa(GetLastError()));
		}
	}
	else
	{
		putErrmsg("Can't open process.", utoa(GetLastError()));
	}
}

void	sm_Abort()
{
	abort();
}

void	sm_WaitForWakeup(int seconds)
{
	DWORD	millisec;
	char	eventName[32];
	HANDLE	event;

	if (seconds < 0)
	{
		millisec = INFINITE;
	}
	else
	{
		millisec = seconds * 1000;
	}

	sprintf(eventName, "%u.wakeup", (unsigned int) GetCurrentProcessId());
	event = CreateEvent(NULL, FALSE, FALSE, eventName);
	if (event)
	{
		oK(WaitForSingleObject(event, millisec));
		CloseHandle(event);
	}
	else
	{
		putErrmsg("Can't open wakeup event.", utoa(GetLastError()));
	}
}

void	sm_Wakeup(DWORD processId)
{
	char	eventName[32];
	HANDLE	event;
	int	result;

	sprintf(eventName, "%u.wakeup", (unsigned int) processId);
	event = OpenEvent(EVENT_ALL_ACCESS, FALSE, eventName);
	if (event)
	{
		result = SetEvent(event);
		CloseHandle(event);
		if (result == 0)
		{
			putErrmsg("Can't set wakeup event.",
					utoa(GetLastError()));
		}
	}
	else
	{
		putErrmsg("Can't open wakeup event.", utoa(GetLastError()));
	}
}

#endif			/*	End of #ifdef MINGW_TASKS		*/

#ifdef UNIX_TASKS

	/* ---- IPC services access control (Unix) -------------------- */

#include <sys/stat.h>
#include <sys/ipc.h>
#include <sched.h>

	/* ---- Task Control services (Unix) -------------------------- */

int	sm_TaskIdSelf()
{
	static int	taskId = 0;

	if (taskId == 0)
	{
		taskId = getpid();
	}

	return taskId;
}

int	sm_TaskExists(int task)
{
	waitpid(task, NULL, WNOHANG);	/*	In case it's a zombie.	*/
	if (kill(task, 0) < 0)
	{
		return 0;		/*	No such process.	*/
	}

	return 1;
}

void	*sm_TaskVar(void **arg)
{
	static void	*value;

	/*	Each UNIX process has its own distinct instance
	 *	of each global variable, so all global variables
	 *	are automatically "task variables".			*/

	if (arg != NULL)
	{
		/*	Set value by dereferencing argument.		*/

		value = *arg;
	}

	return value;
}

void	sm_TaskSuspend()
{
	pause();
}

void	sm_TaskDelay(int seconds)
{
	sleep(seconds);
}

void	sm_TaskYield()
{
	sched_yield();
}

static void	closeAllFileDescriptors()
{
	struct rlimit	limit;
	int		i;

	oK(getrlimit(RLIMIT_NOFILE, &limit));
	for (i = 3; i < limit.rlim_cur; i++)
	{
		oK(close(i));
	}

	writeMemo("");	/*	Tell logger that log file is closed.	*/
}

int	sm_TaskSpawn(char *name, char *arg1, char *arg2, char *arg3,
		char *arg4, char *arg5, char *arg6, char *arg7, char *arg8,
		char *arg9, char *arg10, int priority, int stackSize)
{
	int	pid;
#ifdef VALGRIND_PROFILING
	char	targ1[32];
	char	targ2[32];
	char	targ3[32];
	char	targ4[32];
#endif
	CHKERR(name);

	/*	Ignoring SIGCHLD signals causes the parent process
	 *	to ignore the fate of the child process, so the child
	 *	process cannot become a zombie: when it terminates,
	 *	it is removed immediately rather than waiting for
	 *	the parent to wait() on it.				*/

	isignal(SIGCHLD, SIG_IGN);	
	switch (pid = fork())
	{
	case -1:		/*	Error.				*/
		putSysErrmsg("Can't fork new process", name);
		return -1;

	case 0:			/*	This is the child process.	*/
		closeAllFileDescriptors();
#ifdef VALGRIND_PROFILING
		if (arg1)
		{
			istrcpy(targ1, arg1, sizeof targ1);
			arg1 = targ1;
		}

		if (arg2)
		{
			istrcpy(targ2, arg2, sizeof targ2);
			arg2 = targ2;
		}

		if (arg3)
		{
			istrcpy(targ3, arg3, sizeof targ3);
			arg3 = targ3;
		}

		if (arg4)
		{
			istrcpy(targ4, arg4, sizeof targ4);
			arg4 = targ4;
		}

		execlp("valgrind", "valgrind", "-tool=callgrind", name,
				arg1, arg2, arg3, arg4,
				arg7, arg8, arg9, arg10, NULL);
#else
		execlp(name, name, arg1, arg2, arg3, arg4, arg5, arg6,
				arg7, arg8, arg9, arg10, NULL);
#endif
		/*	Can only get to this code if execlp fails.	*/

		putSysErrmsg("Can't execute new process, exiting...", name);
		exit(1);

	default:		/*	This is the parent process.	*/
		TRACK_BORN(pid);
		return pid;
	}
}

void	sm_TaskKill(int task, int sigNbr)
{
	oK(kill(task, sigNbr));
}

void	sm_TaskDelete(int task)
{
	if (task <= 1)
	{
		writeMemoNote("[?] Can't delete invalid process ID",
				itoa(task));
		return;
	}

	TRACK_DIED(task);
	oK(kill(task, SIGTERM));
	oK(waitpid(task, NULL, 0));
}

void	sm_Abort()
{
	TRACK_DIED(getpid());
	abort();
}

#endif	/*	End of #ifdef UNIX_TASKS				*/

/************************ Unique IPC key services *****************************/

#ifdef RTOS_SHM

	/* ----- Unique IPC key system for "task" architecture --------- */

int	sm_GetUniqueKey()
{
	static unsigned long	ipcUniqueKey = 0x80000000;
	int			result;

	takeIpcLock();
	ipcUniqueKey++;
	result = ipcUniqueKey;		/*	Truncates as necessary.	*/
	giveIpcLock();
	return result;
}

sm_SemId	sm_GetTaskSemaphore(int taskId)
{
	return sm_SemCreate(taskId, SM_SEM_FIFO);
}

#else

	/* ---- Unique IPC key system for "process" architecture ------ */

int	sm_GetUniqueKey()
{
	static int	ipcUniqueKey = 0;
	int		result;

	/*	Compose unique key: low-order 16 bits of process ID
		followed by low-order 16 bits of process-specific
		sequence count.						*/

	ipcUniqueKey = (ipcUniqueKey + 1) & 0x0000ffff;
#ifdef mingw
	result = (_getpid() << 16) + ipcUniqueKey;
#else
	result = (getpid() << 16) + ipcUniqueKey;
#endif
	return result;
}

sm_SemId	sm_GetTaskSemaphore(int taskId)
{
	return sm_SemCreate((taskId << 16), SM_SEM_FIFO);
}

#endif	/*	End of #ifdef RTOS_SHM					*/

/******************* platform-independent functions ***************************/

void	sm_ConfigurePthread(pthread_attr_t *attr, size_t stackSize)
{
#if (!defined(bionic))
	struct sched_param	parms;
#endif

	CHKVOID(attr);
	oK(pthread_attr_init(attr));
#if (!defined(bionic))
	oK(pthread_attr_setschedpolicy(attr, SCHED_FIFO));
	parms.sched_priority = sched_get_priority_min(SCHED_FIFO);
	oK(pthread_attr_setschedparam(attr, &parms));
#endif
	oK(pthread_attr_setdetachstate(attr, PTHREAD_CREATE_JOINABLE));
	if (stackSize > 0)
	{
		oK(pthread_attr_setstacksize(attr, stackSize));
	}
}

int	pseudoshell(char *commandLine)
{
	int	length;
	char	buffer[256];
	char	*cursor;
	int	i;
	char	*argv[11];
	int	argc = 0;
	int	pid;

	if (commandLine == NULL)
	{
		return ERROR;
	}

	length = strlen(commandLine);
	if (length > 255)		/*	Too long to parse.	*/
	{
		putErrmsg("Command length exceeds 255 bytes.", itoa(length));
		return -1;
	}

	istrcpy(buffer, commandLine, sizeof buffer);
	for (cursor = buffer, i = 0; i < 11; i++)
	{
		if (*cursor == '\0')
		{
			argv[i] = NULL;
		}
		else
		{
			findToken(&cursor, &(argv[i]));
			if (argv[i] != NULL)
			{
				argc++;
			}
		}
	}

	/*	Skip over any trailing whitespace.			*/

	while (isspace((int) *cursor))
	{
		cursor++;
	}

	if (*cursor != '\0')		/*	Too many args.	*/
	{
		putErrmsg("More than 11 args in command.", commandLine);
		return -1;
	}
#ifdef ION_LWT
	takeIpcLock();
	if (copyArgs(argc, argv) < 0)
	{
		giveIpcLock();
		putErrmsg("Can't copy args of command.", commandLine);
		return -1;
	}
#endif
	pid = sm_TaskSpawn(argv[0], argv[1], argv[2], argv[3],
			argv[4], argv[5], argv[6], argv[7], argv[8],
			argv[9], argv[10], 0, 0);
#ifdef ION_LWT
	if (pid == -1)
	{
		tagArgBuffers(0);
	}
	else
	{
		tagArgBuffers(pid);
	}

	giveIpcLock();
#endif
	return pid;
}
