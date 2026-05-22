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
/*	Posix Named Semaphore code by Shawn Ostermann, Ohio University, Sept 2023	*/


#include <platform.h>
#include "ion_atomic.h"

static void	takeIpcLock(void);
static void	giveIpcLock(void);


/* shared action arguments for SVR4 and Posix Named Sems */
#define IPC_ACTION_LOOKUP	0
#define IPC_ACTION_STOP		1
#define IPC_ACTION_DETACH	-111111   /* historic */


/* bounds for the GetUniqueKey for process architectures */
/* can't be zero and can't be negative as a signed 32-bit number */
#define UNIQUE_KEY_PROCESSES_INITIAL	0x00001000
#define UNIQUE_KEY_PROCESSES_MAX	0x7fffffff


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

	int shmid; /* Use a signed int for the return value */
	if ((shmid  = shmget(key, size, IPC_CREAT | 0666)) == -1)
	{
		putSysErrmsg("Can't get shared memory segment", utoa(size));
		switch (errno)
		{
			case EACCES:
				fprintf(stderr, "Error: Insufficient permissions.\n");
				break;
			case EINVAL:
				fprintf(stderr, "Error: Invalid size or key. key = %d ; size = %zu\n", key, size);
				fprintf(stderr, "Hint: The requested size may exceed the system's shmmax limit.\n");
				fprintf(stderr, "  Linux:   sysctl kern.sysv.shmmax or kernel.shmmax\n");
				fprintf(stderr, "  macOS:   sysctl kern.sysv.shmmax\n");
				fprintf(stderr, "  Solaris: prctl -n project.max-shm-memory $$\n");
				break;
			case ENOMEM:
				fprintf(stderr, "Error: Insufficient memory.\n");
				break;
			case ENOSPC:
				fprintf(stderr, "Error: Resource limits exceeded.\n");
				break;
			case EEXIST:
				fprintf(stderr, "Error: Key collision with different size.\n");
				break;
			default:
				fprintf(stderr, "Error: Unknown error (errno = %d).\n", errno);
				break;
		}
		return -1;
	}

	else
	{
	*id = shmid; /* On success, assign the valid ID */
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

/* Does a SVR4 shared memory block with the keyvalue of key exist? */
/* ION doesn't keep a table of them, so we'll rely on the OS to tell us if it exists */
static int _shmKeyExists(int key)
{
	int id;
	if ((id = shmget(key, 0, 0)) == -1)
	{
		if (errno == ENOENT) { /* doesn't exist */
			return(0);
		} else { /* other cases, and they all mean that it exists */
			return(1);
		}
	}
	/* this will only succeed if the memory already exists and I can attach to it */
	return(1);
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

static SEM_ID	_ipcSemaphore(int action)
{
	static SEM_ID	ipcSem = NULL;

	if (action == IPC_ACTION_STOP)
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
	SEM_ID	sem = _ipcSemaphore(IPC_ACTION_LOOKUP);

	if (sem == NULL)
	{
		putErrmsg("Can't initialize IPC.", NULL);
		return -1;
	}

	return 0;
}

void	sm_ipc_stop()
{
	oK(_ipcSemaphore(IPC_ACTION_STOP));
}

static void	takeIpcLock()
{
	semTake(_ipcSemaphore(IPC_ACTION_LOOKUP), WAIT_FOREVER);
}

static void	giveIpcLock()
{
	semGive(_ipcSemaphore(IPC_ACTION_LOOKUP));
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

int	sm_SemTakeTimed(sm_SemId i, int timeoutSeconds)
{
	SmSem	*semTbl = _semTbl();
	SmSem	*sem;
	int	ticks;

	CHKERR(i >= 0);
	CHKERR(i < nSemIds);
	sem = semTbl + i;
	if (timeoutSeconds < 1) timeoutSeconds = 1;
	ticks = timeoutSeconds * sysClkRateGet();
	if (semTake(sem->id, ticks) == ERROR)
	{
		if (errno == S_objLib_OBJ_TIMEOUT)
		{
			return 1;	/*	Timed out.		*/
		}

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

static sem_t	*_ipcSemaphore(int action)
{
	static sem_t	ipcSem;
	static int	ipcSemInitialized = 0;

	if (action == IPC_ACTION_STOP)
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
	if (_ipcSemaphore(IPC_ACTION_LOOKUP) == NULL)
	{
		putErrmsg("Can't initialize IPC.", NULL);
		return -1;
	}

	return 0;
}

void	sm_ipc_stop()
{
	oK(_ipcSemaphore(IPC_ACTION_STOP));
}

static void	takeIpcLock()
{
	oK(sem_wait(_ipcSemaphore(IPC_ACTION_LOOKUP)));

}

static void	giveIpcLock()
{
	oK(sem_post(_ipcSemaphore(IPC_ACTION_LOOKUP)));
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

int	sm_SemTakeTimed(sm_SemId i, int timeoutSeconds)
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
		if (errno == EINTR)
		{
			continue;
		}

		if (errno == ETIMEDOUT)
		{
			return 1;	/*	Timed out.		*/
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

	/* global process-side, ION instance wide value for GetUniqueKey() */
	/* to be protected by the same global semaphore as this table */
	unsigned int ipcUniqueKey;

} SemaphoreBase;

/* for use internally for semaphore/shm routines called with a request to pick an unused key */
static int _sm_GetUniqueKey_internal(SemaphoreBase	*semaphoreBase);

static SemaphoreBase	*_sembase(int action)
{
	static SemaphoreBase	*semaphoreBase = NULL;
	static uaddr		sembaseId = 0;
	int			semSetIdx;
	IciSemaphoreSet		*semset;
	int			i;

	/* 	detach & reset, but not stopping	*/
	if (action == IPC_ACTION_DETACH)
	{
		/* if sembase exists, detach from shared memory */
		if (semaphoreBase != NULL)
		{
			oK(shmdt(semaphoreBase));
		}
		semaphoreBase = NULL;
		sembaseId = 0;
		return NULL;
	}

	if (action == IPC_ACTION_STOP)
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
			/* initialize global counter for GetUniqueKey */
			semaphoreBase->ipcUniqueKey = UNIQUE_KEY_PROCESSES_INITIAL;

			semaphoreBase->idsAllocated = 0;
			semaphoreBase->currSemSet = 0;
			for (i = 0; i < MAX_SEM_SETS; i++)
			{
				semaphoreBase->semSets[i].semid = -1;
			}

			/*	Acquire initial semaphore set.		*/

			semset = semaphoreBase->semSets
					+ semaphoreBase->currSemSet;
			semset->semid = semget(_sm_GetUniqueKey_internal(semaphoreBase), SEMMSL,
					IPC_CREAT | 0666);
			if (semset->semid < 0)
			{
				putSysErrmsg("Can't get initial semaphore set",
						NULL);
				sm_ShmDestroy(sembaseId);
				semaphoreBase = NULL;
				break;
			}

			writeMemoNote("Initializing semaphores to use: SVR4   Pid", itoa(getpid()));

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

static int	_ipcSemaphore(int action)
{
	static int	ipcSem = -1;

	/* 	reset but not stopping	*/
	if (action == IPC_ACTION_DETACH)
	{
		/* if semaphore exists */
		if (ipcSem != -1)
		{
			oK(_sembase(IPC_ACTION_DETACH));
			ipcSem = -1;
		}
		return ipcSem;
	}

	if (action == IPC_ACTION_STOP)
	{
		oK(_sembase(IPC_ACTION_STOP));
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
			if (_sembase(IPC_ACTION_LOOKUP) == NULL)
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
	if (_ipcSemaphore(IPC_ACTION_LOOKUP) == -1)
	{
		putErrmsg("Can't initialize IPC.", NULL);
		return -1;
	}

	return 0;
}

void	sm_ipc_stop()
{
	oK(_ipcSemaphore(IPC_ACTION_STOP));
}

void 	sm_ipc_detach()
{
	oK(_ipcSemaphore(IPC_ACTION_DETACH));
}

static void	takeIpcLock()
{
	struct sembuf	sem_op[2] = { {0,0,0}, {0,1,0} };

	oK(semop(_ipcSemaphore(IPC_ACTION_LOOKUP), sem_op, 2));
}

static void	giveIpcLock()
{
	struct sembuf	sem_op = { 0, -1, IPC_NOWAIT };

	oK(semop(_ipcSemaphore(IPC_ACTION_LOOKUP), &sem_op, 1));
}

/* check if it's already been created by some ION process */
/* assumes that IpcLock is held */
static int _semKeyExists(int key) {
	SemaphoreBase	*sembase;
	IciSemaphore	*sem;
	int i;

	sembase = _sembase(IPC_ACTION_LOOKUP);

	for (i = 0, sem = sembase->semaphores; i < sembase->idsAllocated; i++, sem++) {
		if (sem->key == key) {
			return(1);	/*	already exists */
		}
	}

	return(0); /* not found */
}



sm_SemId	sm_SemCreate(int key, int semType)
{
	SemaphoreBase	*sembase;
	int		i;
	IciSemaphore	*sem;
	IciSemaphoreSet	*semset;
	int		semSetIdx;
	int		semid;

	/*	Look through list of all existing ICI semaphores.	*/

	takeIpcLock();
	sembase = _sembase(IPC_ACTION_LOOKUP);
	if (sembase == NULL)
	{
		giveIpcLock();
		putErrmsg("No semaphore base.", NULL);
		return SM_SEM_NONE;
	}

	/*	If key is not specified, invent one.			*/

	if (key == SM_NO_KEY)
	{
		key = _sm_GetUniqueKey_internal(sembase);
	}
	else   /* If key is specified, check if semaphore already exists */
	{
		for (i = 0, sem = sembase->semaphores; i < sembase->idsAllocated;
				i++, sem++)
		{
			if (sem->key == key)
			{
				giveIpcLock();
				return i;	/*	already created		*/
			}
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

			semid = semget(_sm_GetUniqueKey_internal(sembase), SEMMSL,
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
	SemaphoreBase	*sembase = _sembase(IPC_ACTION_LOOKUP);
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
	SemaphoreBase	*sembase = _sembase(IPC_ACTION_LOOKUP);
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

int	sm_SemTakeTimed(sm_SemId i, int timeoutSeconds)
{
	SemaphoreBase	*sembase = _sembase(IPC_ACTION_LOOKUP);
	IciSemaphore	*sem;
	IciSemaphoreSet	*semset;
	struct sembuf	sem_op[2] = { {0,0,0}, {0,1,0} };
	struct timespec	timeout;

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
	if (timeoutSeconds < 1) timeoutSeconds = 1;
	timeout.tv_sec = timeoutSeconds;
	timeout.tv_nsec = 0;
	while (semtimedop(semset->semid, sem_op, 2, &timeout) < 0)
	{
		if (errno == EINTR)
		{
			continue;
		}

		if (errno == EAGAIN)
		{
			return 1;	/*	Timed out.		*/
		}

		putSysErrmsg("Can't take semaphore", itoa(i));
		return -1;
	}

	return 0;
}

void	sm_SemGive(sm_SemId i)
{
	SemaphoreBase	*sembase = _sembase(IPC_ACTION_LOOKUP);
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
	SemaphoreBase	*sembase = _sembase(IPC_ACTION_LOOKUP);
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
	SemaphoreBase	*sembase = _sembase(IPC_ACTION_LOOKUP);
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
	SemaphoreBase	*sembase = _sembase(IPC_ACTION_LOOKUP);
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
	SemaphoreBase	*sembase = _sembase(IPC_ACTION_LOOKUP);
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

	/* Parameter intentionally unused. */
	(void)name;

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

#if defined(__linux__)
	pthread_setname_np(*thread, name);
#elif defined(freebsd)
	pthread_set_name_np(*thread,name);
#endif	/*	End of #if __linux__.					*/
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
		putErrmsg("Can't spawn task: no parms cleared yet.", name);
		return -1;
	}

	parms->threadMainFunction = entryPoint;
	parms->arg1 = (saddr) arg1;
	parms->arg2 = (saddr) arg2;
	parms->arg3 = (saddr) arg3;
	parms->arg4 = (saddr) arg4;
	parms->arg5 = (saddr) arg5;
	parms->arg6 = (saddr) arg6;
	parms->arg7 = (saddr) arg7;
	parms->arg8 = (saddr) arg8;
	parms->arg9 = (saddr) arg9;
	parms->arg10 = (saddr) arg10;
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

#ifdef UNIX_TASKS

	/* ---- IPC services access control (Unix) -------------------- */

#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sched.h>

	/* ---- Task Control services (Unix) -------------------------- */

int	sm_TaskIdSelf(void)
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

void	sm_TaskSuspend(void)
{
	pause();
}

void	sm_TaskDelay(int seconds)
{
	sleep(seconds);
}

void	sm_TaskYield(void)
{
	sched_yield();
}

static void	closeAllFileDescriptors(void)
{
	struct rlimit	limit;
	rlim_t		i;

	if (getrlimit(RLIMIT_NOFILE, &limit) < 0)
	{
		limit.rlim_cur = 0;
	}

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

	/* Acknowledge unused parameters. */
	(void)priority;
	(void)stackSize;

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
	if (task <= 0)
	{
		/*	task == 0 sends to the entire process group;
		 *	task == -1 sends to every process the caller
		 *	owns (kill(-1, sig)).  Both are catastrophic
		 *	in this context.  ERROR is -1 on POSIX.	*/

		writeMemoNote("[?] sm_TaskKill: refusing invalid PID",
				itoa(task));
		return;
	}

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

void	sm_Abort(void)
{
	TRACK_DIED(getpid());
	abort();
}

#endif	/*	End of #ifdef UNIX_TASKS				*/

/*	Portable per-process-instance cookie.  sm_TaskIdSelf() returns
 *	the OS PID, which is reused after a process exits.  Two distinct
 *	processes can therefore observe the same task id at different
 *	times.  This function returns a value that distinguishes "this
 *	process instance" from any other process instance, even one with
 *	the same recycled PID.  Used by code that needs to detect stale
 *	shared-memory ownership records left behind by a now-dead process
 *	whose PID has since been reassigned.
 *
 *	The cookie is process-local: every thread in the same process
 *	gets the same value, so it does not distinguish threads (PID
 *	already fails to distinguish them and code that needs thread
 *	identity must use pthread_self() or equivalent directly).	*/

uvast	sm_ProcessCookie(void)
{
	static uvast	cookie = 0;

	if (cookie == 0)
	{
		struct timeval	tv;

		/*	XOR the PID with the high-resolution wall clock at
		 *	first call.  Two processes cannot both start at the
		 *	same microsecond with the same PID, so the tuple is
		 *	unique in practice even if the collapsed 64-bit
		 *	value is not theoretically collision-free.	*/

		getCurrentTime(&tv);
		cookie = ((uvast) sm_TaskIdSelf() << 32)
			^ ((uvast) tv.tv_sec * 1000000ULL + (uvast) tv.tv_usec);
		if (cookie == 0)
		{
			cookie = 1;	/*	0 means "unset".	*/
		}
	}

	return cookie;
}


#ifdef POSIX_NAMED_SEMAPHORES
/* ---- Semaphore services (POSIX NAMED SEMAPHORES) ---------	*/
/* for process-based OSs where available - quite a bit faster than SVR4 semaphores */


/* maximum name to store Posix Named Semaphore names */
/* (not stored, but temporarily on the stack)*/
#define MAX_NAMED_SEM_KEYLENGTH 100


/* unlike the SVR4 code, the posix named semaphore code does NOT store a random/unique name */
/* in the semaphore table if no key was specified. That makes the code clearer, and since there */
/* is no way to discover the chosen key through the API, shouldn't cause problems */
#define SEM_ANON_KEY 0xfffffff0


/* the shared memory key that Posix Named Semaphores uses for the shared semaphore table that */
/* is shared by __ALL__ the ION instances in a particular node (just like the SVR4 code) */
#ifndef SM_SEMTBLKEY
#define SM_SEMTBLKEY	(0xee08)
#endif

/* file mode to use for ION posix named semaphores */
/* Note that the ION Posix Named Semaphores are global to the computer, being used by */
/* ALL ION instances on the computer.  That can cause problems if multiple USERS are creating */
/* ION instances on a single computer because ION can't clean up the semaphores of another */
/* user unless those semaphores are 'world deletable' */

/* this file mode is the safest - meaning that other users on the computer can't */
/* manipulate/delete the ION shared semaphores, but it will fail if multuple users */
/* run ION after one-another because they can't clean up the global semaphores */
// #define POSIX_NAMED_SEMAPHORES_FILEMODE	(S_IRUSR | S_IWUSR)

/* this file mode is easier to use if multiple _FRIENDLY_ users are using ION at the same time */
#define POSIX_NAMED_SEMAPHORES_FILEMODE		(S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)


/* For ensuring that the per-process sem table is in sync with the global sem table */
/* This can overflow - but we only check for equivalence */
/* This number is incremented every time there is a change in ANY semaphore (creation or deletion)  */
/* across all processes of all ION instances on the computer.  */
/* Assuming a 64-bit "long int" and one semaphore change per millisecond (unthinkable in production), */
/* this could wrap in 2**64 / 1000 / 60 / 60 / 24 / 365 == 584 Million years */
/* Assuming a 32-bit "long int" and one semaphore change per second (unlikely in production), */
/* this could wrap in 2**32 / 60 / 60 / 24 / 365 == 136 years */
typedef unsigned long int smSequence;

/* this structure makes up the GLOBAL - all processes - all Ion instances - semaphore table */
typedef struct
{
	char		inUse;
	ion_ipc_atomic_t	ended;		/* Atomic: accessed from multiple processes */
	int		key;
	ion_ipc_atomic_t	gseq;		/* Atomic: sequence number for cache invalidation */
	ion_ipc_atomic_t	refCount;	/* Number of active users across all processes (atomic for lock-free access) */
	ion_ipc_atomic_t	pendingDelete;	/* Atomic: marked for deletion when refCount reaches 0 */
} SmGlobalSem;

/* this structure makes up the process-local semaphore table */
/* needed because the sem_t for semaphore operations is process-local */
typedef struct
{
	sem_t		*id;
	SmGlobalSem	*semgl;		/* pointer to ION-wide shared master semtable entry (to avoid multiplication)*/
	smSequence	lseq;
	int		localRefCount;	/* Per-process reference count for tracking */
	int		handleOpened;	/* Flag to track if handle has been opened (1=opened, 0=not opened) */
} SmLocalSem;

/* the data structure shared by ALL processes/threads for all ION Instances */
/* kept in SVR4 shared memory */
typedef struct {
	/* the id of the shared memory that maps this structure */
	uaddr		sembaseId;

	/* the global semaphore table */
	SmGlobalSem	gsemtable[SEM_NSEMS_MAX];

	/* low-overhead statistics for tuning memory if desired  - reported on shutdown (if DEBUG_POSIX_NAMED_SEMAPHORES is set)*/
	unsigned opensems_current;
	unsigned opensems_max;

	/* global process-side, ION instance wide value for GetUniqueKey() */
	/* to be protected by the same global semaphore as this table */
	unsigned int ipcUniqueKey;

#ifdef ION_HAVE_ROBUST_MUTEX
	/*	On platforms with robust-mutex support, the global IPC
	 *	lock (formerly a POSIX named semaphore) is a process-
	 *	shared robust pthread_mutex_t living here in shared
	 *	memory.  This is the same orphan-recovery treatment that
	 *	was applied to the SDR transaction lock and the PSM
	 *	partition lock; without it, a process killed while
	 *	holding the IPC lock (e.g. by SIGKILL during shutdown)
	 *	orphans the POSIX named sem and every subsequent
	 *	sm_SemCreate/sm_SemDelete wedges the node forever.	*/
	pthread_mutex_t	ipcMutex;
	int		ipcMutexCreated;	/*	Boolean.	*/
#endif

	/* is initialization complete for this structure? */
	int initialized;
} SmGlobalSemtable;

/* the data structure shared by ALL threads for a single ION Process */
typedef struct {
	SmLocalSem	lsemtable[SEM_NSEMS_MAX];
	SmGlobalSemtable *semtablegl;	/* pointer to ION-wide shared master semtable */
} SmProcessSemtable;


static SmProcessSemtable *_semTbl(int action);
static SmGlobalSemtable	*_sembase(int action);
static sem_t *_ipcSemaphore(int action);
static void _semEraseNamedSems(void);
static SmLocalSem *_semGetSem(SmProcessSemtable *psemtable, sm_SemId semnum, int semlocked);
static char *_semGenPosixSemname(char *namebuf, unsigned bufsize, int semnum);
static int _semKeyExists(int key);
static void _sm_SemCompleteDeletePosix(SmProcessSemtable *semTbl, sm_SemId i);

void _semPrintTable(void)  // Only for debugging purposes
{
#ifdef DEBUGGING
	SmProcessSemtable	*semTbl = _semTbl(IPC_ACTION_LOOKUP);
	char sem_name[MAX_NAMED_SEM_KEYLENGTH];
	int	i;

	if (!semTbl) {
		return;
	}

	fprintf(stderr,"=========== Semaphore Table - pid %d =================\n", getpid());

	fprintf(stderr,"  Global sem: %p (%s)\n", _ipcSemaphore(IPC_ACTION_LOOKUP),
		_semGenPosixSemname(sem_name,sizeof(sem_name),-1));
	fprintf(stderr,"  Semaphore current usage: %u	max: %u   configured: %u\n",
		semTbl->semtablegl->opensems_current, semTbl->semtablegl->opensems_max, SEM_NSEMS_MAX);

	fprintf(stderr,"  SemNum InUse Key        ID    LocSeq     GloSeq     SemPath\n");
	fprintf(stderr,"  ------ ----- ---------- ----- ---------- ---------- -----------------------\n");

	for (i = 0; i < SEM_NSEMS_MAX; i++) {
		SmLocalSem *psem  = &semTbl->lsemtable[i];

		if (psem->semgl->inUse || (ion_ipc_atomic_get(&psem->semgl->gseq) > 0)) {
			fprintf(stderr,"  %-6d ", i);
			fprintf(stderr,"%-5d ", psem->semgl->inUse);
			if (!psem->semgl->inUse) {
				fprintf(stderr,"       DELETED   ");
			} else {
				if (psem->semgl->key == SEM_ANON_KEY) {
					fprintf(stderr,"    ANON   ");
				} else {
					fprintf(stderr,"0x%08x ", psem->semgl->key);
				}
				if (psem->lseq == (smSequence)ion_ipc_atomic_get(&psem->semgl->gseq)) {
					fprintf(stderr,"%5p ", psem->id);
				} else {
					/* out of sync locally, so not valid */
					fprintf(stderr," ---  ");
				}
			}
			fprintf(stderr,"%10lu ", psem->lseq);
			fprintf(stderr,"%10llu ", (unsigned long long)ion_ipc_atomic_get(&psem->semgl->gseq));
			if (psem->semgl->inUse) {
				fprintf(stderr,"%s ", _semGenPosixSemname(sem_name,sizeof(sem_name),i));
			}

			fprintf(stderr,"\n");
		}
	}
	fprintf(stderr,"===========================================================\n");
#endif /* DEBUGGING */
}

/* check if it's already been created by some process */
/* assumes that IpcLock is held */
static int _semKeyExists(int key) {
	SmProcessSemtable *semTbl = _semTbl(IPC_ACTION_LOOKUP);
	int i;

	for (i = 0; i < SEM_NSEMS_MAX; i++)
	{
		SmGlobalSem	*gsem = &semTbl->semtablegl->gsemtable[i];

		if (!gsem->inUse) {
			continue;
		}

		if (gsem->key == key) {
			return(1);
		}
	}
	return(0);
}


/* ensure that the process local and shared global semaphores are in sync */
/* N.B.: assumes global semaphore table semaphore is NOT held (unless semlocked is true) */
static int _semSync(SmProcessSemtable *plocal, sm_SemId semnum, int semlocked)
{
	char sem_name[MAX_NAMED_SEM_KEYLENGTH];
	sem_t *psem;
	mode_t oldmask;  /* save current umode() mask so we can restore it after open() */

	SmLocalSem  *plocalSem = &plocal->lsemtable[semnum];
	SmGlobalSem *pglobalSem = plocalSem->semgl;

	if (plocalSem->lseq == (smSequence)ion_ipc_atomic_get(&pglobalSem->gseq)) {
		/* local copy is up to date */
		return(1);
	}

	/* above is the expected case, and it needs to be fast... */

	if (!semlocked)
		takeIpcLock();  /* lock global table across ALL Ion instances */

	/* open the global semaphore locally */
	if (pglobalSem->inUse) {
		/* MUST already exist */
		/* Only open handle once per semaphore per process to prevent race conditions */

		if (!plocalSem->handleOpened) {
			/* Handle not yet opened - open it for the first time */
			/* ensure that we're using EXACTLY the mode bits in POSIX_NAMED_SEMAPHORES_FILEMODE regardless */
			/* of the account's setting of umask() */
			oldmask = umask(0);

			_semGenPosixSemname(sem_name,sizeof(sem_name),semnum);
			if ((psem = sem_open(sem_name, O_CREAT, POSIX_NAMED_SEMAPHORES_FILEMODE, 0 )) == SEM_FAILED) {
				perror("sem_open");
				putSysErrmsg("Can't initialize IPC semaphore", sem_name);
				umask(oldmask);  /* restore previous umask() */
				if (!semlocked)
					giveIpcLock();
				return 0;
			}
			umask(oldmask);  /* restore previous umask() */

			plocalSem->id = psem;
			plocalSem->handleOpened = 1;
		}
		/* else: Handle already opened - just mark sequence as up to date */

	} else {
		/* NOT (or no longer) in use globally, close our version if opened */

		if (plocalSem->handleOpened && plocalSem->id != NULL && plocalSem->id != SEM_FAILED) {
			oK(sem_close(plocalSem->id));
			plocalSem->handleOpened = 0;
		}

		plocalSem->id = NULL;
		plocalSem->lseq = (smSequence)ion_ipc_atomic_get(&pglobalSem->gseq);
	}

	plocalSem->lseq = (smSequence)ion_ipc_atomic_get(&pglobalSem->gseq);  /* now up to date */
	if (!semlocked)
		giveIpcLock();
	return(1);
}


static SmProcessSemtable *_semTbl(int action)
{
	static SmProcessSemtable semStruct;  /* local to each (new) process running Ion */
	static int	semTableInitialized = 0;

	if ((action == IPC_ACTION_STOP) || (action == IPC_ACTION_DETACH)) {
#ifdef DEBUG_POSIX_NAMED_SEMAPHORES
		if (semTableInitialized) {
			/* for memory usage profiling */
			writeMemoNote("Posix Named Semaphores - Total Configured", itoa(SEM_NSEMS_MAX));
			writeMemoNote("Posix Named Semaphores - Current Usage", itoa(semStruct.semtablegl->opensems_current));
			writeMemoNote("Posix Named Semaphores - Maximum Usage", itoa(semStruct.semtablegl->opensems_max));
		}
#endif /* DEBUG_POSIX_NAMED_SEMAPHORES */

		/* Close all open semaphores before detaching/stopping */
		if (semTableInitialized) {
			int i;
			for (i = 0; i < SEM_NSEMS_MAX; i++) {
				SmLocalSem *sem = &semStruct.lsemtable[i];
				if (sem->id != NULL && sem->id != SEM_FAILED) {
					oK(sem_close(sem->id));
					sem->id = NULL;
				}
			}
		}

		semTableInitialized = 0;
		return NULL;
	}

	/* ... else ... case is IPC_ACTION_LOOKUP */
	if (!semTableInitialized)
	{
		static SmGlobalSemtable	 *psemGlobal;
		int i;

		/* make sure that the global shared structure is set up */
		psemGlobal = _sembase(IPC_ACTION_LOOKUP);
		CHKNULL(psemGlobal);

		/* create the process-local version of the global semaphore table */
		memset((char *) &semStruct, 0, sizeof(SmProcessSemtable));

		/* local table points to global table entried */
		/* a little more space needs VS. less multiplications later */
		for (i = 0; i < SEM_NSEMS_MAX; i++) {
			semStruct.lsemtable[i].semgl = &psemGlobal->gsemtable[i];
			semStruct.lsemtable[i].lseq = 0;
			semStruct.lsemtable[i].id = NULL;
			semStruct.lsemtable[i].localRefCount = 0;
		}

		semStruct.semtablegl = psemGlobal;
		semTableInitialized = 1;
	}

	return &semStruct;
}

/* return the Local semaphore structure that goes with ION semaphore number "semnum" */
/* N.B. Assumes that global semaphore is NOT held (unless semlocked is true) */
static SmLocalSem *_semGetSem(SmProcessSemtable *psemtable, sm_SemId semnum, int semlocked)
{
	SmLocalSem *psemLocal;

	CHKNULL(psemtable);
	CHKNULL(semnum >= 0);
	CHKNULL(semnum < SEM_NSEMS_MAX);

	psemLocal  = &psemtable->lsemtable[semnum];

	if (!_semSync(psemtable, semnum, semlocked)) {
		writeMemoNote("Couldn't sync local semaphore", itoa(semnum));
		return(NULL);
	}

	if (!psemLocal->semgl->inUse) {
		writeMemoNote("Operation attempted on semaphore that is no longer in use", itoa(semnum));
		return(NULL);
	}

	return(psemLocal);
}


/* return the name used for the semaphore whole key is key through buffer passed */
static char *_semGenPosixSemname(char *namebuf, unsigned bufsize, int semnum)
{
	if (semnum == -1 ) {
		snprintf(namebuf, bufsize, "/ion:GLOBAL:ipcSem");
	} else {
		snprintf(namebuf, bufsize, "/ion:GLOBAL:%u", (unsigned) semnum);
	}
	return(namebuf);
}


/* unlink the names of all POSIX named semaphores that could belong to ION instance ionId */
static void _semEraseNamedSems(void)
{
	char sem_name[MAX_NAMED_SEM_KEYLENGTH];

	/* MUST unlink all possible named semaphores that could have been created in a previous run */
	for (int semnum=0; semnum < SEM_NSEMS_MAX; ++semnum) {
		_semGenPosixSemname(sem_name,sizeof(sem_name),semnum);
		sem_unlink(sem_name); /* doesn't matter if it fails */
	}

	/* and also delete the master semaphore table semaphore */
	_semGenPosixSemname(sem_name,sizeof(sem_name),-1);
	sem_unlink(sem_name); /* doesn't matter if it fails */
}


/* Posix Named Semaphores Version */
static SmGlobalSemtable	*_sembase(int action)
{
	static SmGlobalSemtable *psemGlobal = NULL;
	static uaddr sembaseId = 0;

	/* detach & reset, but not stopping	*/
	if (action == IPC_ACTION_DETACH)
	{
		sembaseId = 0;

		/* if sembase exists, detach from shared memory */
		if (psemGlobal != NULL) {
			psemGlobal->sembaseId = 0;
			oK(shmdt(psemGlobal));
		}
		psemGlobal = NULL;
		return NULL;
	}

	if (action == IPC_ACTION_STOP) {
		if (psemGlobal != NULL) {
			_semEraseNamedSems();
#ifdef ION_HAVE_ROBUST_MUTEX
			/*	Best-effort drain of the global IPC mutex
			 *	before SHM destruction.  EOWNERDEAD is
			 *	expected if the last holder died; marking
			 *	consistent lets pthread_mutex_destroy
			 *	accept the mutex cleanly.		*/

			if (psemGlobal->ipcMutexCreated)
			{
				int rc = pthread_mutex_lock(&psemGlobal->ipcMutex);

				if (rc == EOWNERDEAD)
				{
					oK(pthread_mutex_consistent(&psemGlobal->ipcMutex));
				}

				if (rc == 0 || rc == EOWNERDEAD)
				{
					oK(pthread_mutex_unlock(&psemGlobal->ipcMutex));
				}

				oK(pthread_mutex_destroy(&psemGlobal->ipcMutex));
				psemGlobal->ipcMutexCreated = 0;
			}
#endif
			sm_ShmDestroy(sembaseId);
			psemGlobal = NULL;
		}
		return NULL;
	}

	/* ... else ... case is IPC_ACTION_LOOKUP */
	/* create/join the shared memory structure that ALL ION instances will share */
	if (psemGlobal == NULL)
	{
		uint32_t shmemkey = SM_SEMTBLKEY;
		switch(sm_ShmAttach(shmemkey, sizeof(SmGlobalSemtable), (char **) &psemGlobal, &sembaseId))
		{
			case -1:
				putErrmsg("Can't create global semaphore table.", NULL);
				break;

			case 0:
				{
					/* race condition - semaphore and shared memory initialization depend on each other. That */
					/* means that there is no access to semaphores to ensure that this structure is initialized yet (default case below). */
					/* If multiple processes get here at the same time, we'll have to do it old-school */
					int snooze_usecs = 10000;  /* start at 10ms and back off by powers of 2 */
					if (!psemGlobal->initialized) {
						while (!psemGlobal->initialized) {
							microsnooze(snooze_usecs);
							snooze_usecs *= 2;
							CHKNULL(snooze_usecs <= 10000000);  /* max 10 seconds */
						}
					}
				}
				break;		/*	Semaphore table exists.	*/

			default:		/*	New SemaphoreTable - clean it up (see note on race condition above) */
				writeMemoNote("Initializing semaphores to use: Posix Named Semaphores - max ", itoa(SEM_NSEMS_MAX));
				memset((char *) psemGlobal, 0, sizeof(SmGlobalSemtable));
				psemGlobal->sembaseId = sembaseId;
				_semEraseNamedSems();

				/* initialize global counter for GetUniqueKey as with RtEMS */
				psemGlobal->ipcUniqueKey = UNIQUE_KEY_PROCESSES_INITIAL;

				/* Explicitly initialize IPC atomics for all semaphores */
				for (int j = 0; j < SEM_NSEMS_MAX; j++) {
					ion_ipc_atomic_init(&psemGlobal->gsemtable[j].ended, 0);
					ion_ipc_atomic_init(&psemGlobal->gsemtable[j].gseq, 0);
					ion_ipc_atomic_init(&psemGlobal->gsemtable[j].refCount, 0);
					ion_ipc_atomic_init(&psemGlobal->gsemtable[j].pendingDelete, 0);
				}

#ifdef ION_HAVE_ROBUST_MUTEX
				/* Initialize the robust process-shared IPC
				 * mutex.  Replaces the legacy POSIX named
				 * sem used by takeIpcLock/giveIpcLock. */
				{
					pthread_mutexattr_t	attr;

					if (pthread_mutexattr_init(&attr) != 0
					|| pthread_mutexattr_setpshared(&attr,
							PTHREAD_PROCESS_SHARED) != 0
					|| pthread_mutexattr_setrobust(&attr,
							PTHREAD_MUTEX_ROBUST) != 0
					|| pthread_mutex_init(&psemGlobal->ipcMutex,
							&attr) != 0)
					{
						oK(pthread_mutexattr_destroy(&attr));
						putErrmsg("Can't create robust mutex \
for global IPC lock.", NULL);
						return NULL;
					}
					oK(pthread_mutexattr_destroy(&attr));
					psemGlobal->ipcMutexCreated = 1;
				}
#endif

				psemGlobal->initialized = 1;  /* must be the last step */
		}
	}

	return psemGlobal;
}


static sem_t	*_ipcSemaphore(int action)
{
	static sem_t	*ipcSemPtr = NULL;
	static int		ipcSemInitialized = 0;

	/* 	reset but not stopping	*/
	if (action == IPC_ACTION_DETACH) {
		_semTbl(IPC_ACTION_DETACH);
		if (ipcSemPtr != NULL) {
			oK(_sembase(IPC_ACTION_DETACH));
			ipcSemPtr = NULL;
			ipcSemInitialized = 0;
		}
		return NULL;
	}

	if (action == IPC_ACTION_STOP) {
		_semTbl(IPC_ACTION_STOP);
		if (ipcSemInitialized) {
			oK(sem_close(ipcSemPtr));
			oK(_sembase(IPC_ACTION_STOP));
			ipcSemPtr = NULL;
			ipcSemInitialized = 0;
		}
		return NULL;
	}

	if (ipcSemInitialized == 0) {
		char sem_name[MAX_NAMED_SEM_KEYLENGTH];
		sem_t *psem;
		mode_t oldmask;  /* save current umode() mask so we can restore it after open() */

		oK(_semTbl(IPC_ACTION_LOOKUP));  /* create the shared memory if not already done */

		/* ensure that we're using EXACTLY the mode bits in POSIX_NAMED_SEMAPHORES_FILEMODE regardless */
		/* of the account's setting of umask() */
		oldmask = umask(0);

		_semGenPosixSemname(sem_name,sizeof(sem_name),-1); /* make the semaphore that all ION instances/procs will use */
		if ((psem = sem_open(sem_name, O_CREAT | O_EXCL, POSIX_NAMED_SEMAPHORES_FILEMODE, 0 )) != SEM_FAILED)
		{
			/* specified '| O_EXCL', so we are the first to open it */
			if (sem_post(psem) == -1) {
				putSysErrmsg("Can't initialize IPC semaphore as mutex", sem_name);
				umask(oldmask);  /* restore umask() */
				return NULL;
			}
		} else if ((psem = sem_open(sem_name, O_CREAT, POSIX_NAMED_SEMAPHORES_FILEMODE, 0 )) != SEM_FAILED) {
			/* we joined a semaphore that already exists */
			/* Note that there's a race condition of sorts here, but it's OK... */
			/* the previous block might NOT have been done yet (which will set the */
			/* counter to 1 == unlocked), but that's OK, because anybody using this version */
			/* will just find it locked until the first process to open it (above) sets its value */
			/* to unlocked above (which will allow the first blocked process to start). */
		} else {
			/* failed, can't open it at all - shouldn't happen */
			putSysErrmsg("Can't initialize IPC semaphore", sem_name);
			umask(oldmask);  /* restore umask() */
			return NULL;
		}

		umask(oldmask);  /* restore umask() */

		ipcSemPtr = psem;

		/*	Initialize the semaphore system.		*/
		oK(_semTbl(IPC_ACTION_LOOKUP));
		ipcSemInitialized = 1;
	}

	return ipcSemPtr;
}


int	sm_ipc_init(void)
{
	if (_ipcSemaphore(IPC_ACTION_LOOKUP) == NULL) {
		putErrmsg("Can't initialize IPC.", NULL);
		return -1;
	}
	return 0;
}


void 	sm_ipc_detach(void)
{
	oK(_ipcSemaphore(IPC_ACTION_DETACH));
}


void	sm_ipc_stop(void)
{
	oK(_ipcSemaphore(IPC_ACTION_STOP));
}


#ifdef ION_HAVE_ROBUST_MUTEX
/*	Per-thread saved signal mask for takeIpcLock/giveIpcLock.  We
 *	block SIGTERM/SIGINT around the IPC-lock-held region so that
 *	a shutdown signal arriving mid-critical-section can never
 *	interrupt this thread and re-enter takeIpcLock from its
 *	handler — a self-deadlock seen in ltpdeliv/bptransit/bpclm
 *	whose SIGTERM handlers call sm_SemEnd, which calls
 *	takeIpcLock on the very mutex this thread already holds.
 *	Deferred signals are delivered the moment giveIpcLock restores
 *	the mask.							*/

static ION_THREAD_LOCAL sigset_t	ipcLockSavedMask;
static ION_THREAD_LOCAL int		ipcLockMaskDepth = 0;

static void	blockShutdownSignals(void)
{
	sigset_t	toBlock;

	if (ipcLockMaskDepth++ > 0)
	{
		return;	/*	Already blocked at outer scope.		*/
	}

	sigemptyset(&toBlock);
	sigaddset(&toBlock, SIGTERM);
	sigaddset(&toBlock, SIGINT);
	oK(pthread_sigmask(SIG_BLOCK, &toBlock, &ipcLockSavedMask));
}

static void	restoreShutdownSignals(void)
{
	if (ipcLockMaskDepth == 0)
	{
		return;	/*	Unbalanced restore (should not happen).	*/
	}

	if (--ipcLockMaskDepth > 0)
	{
		return;	/*	Still nested; restore at outermost only.*/
	}

	oK(pthread_sigmask(SIG_SETMASK, &ipcLockSavedMask, NULL));
}
#endif	/*	ION_HAVE_ROBUST_MUTEX					*/

static void	takeIpcLock(void)
{
#ifdef ION_HAVE_ROBUST_MUTEX
	SmGlobalSemtable	*psemGlobal = _sembase(IPC_ACTION_LOOKUP);
	int			rc;

	CHKVOID(psemGlobal != NULL);
	CHKVOID(psemGlobal->ipcMutexCreated);

	blockShutdownSignals();
	rc = pthread_mutex_lock(&psemGlobal->ipcMutex);
	if (rc == EOWNERDEAD)
	{
		/*	Previous holder died with the global IPC lock
		 *	held -- typically a SIGKILL'd daemon that was
		 *	mid-sm_SemCreate or mid-sm_SemDelete.  Recover
		 *	(mark consistent + continue) rather than abort.
		 *
		 *	Unlike PSM allocator metadata (free lists, in-use
		 *	bits -- a misthread can corrupt unboundedly), the
		 *	global sem-table failure surface is small and
		 *	bounded: refCounts, in-use flags, pendingDelete
		 *	bits, and slot-allocation cursor.  The worst-case
		 *	outcome of recovering from an inconsistent state
		 *	here is an orphaned sem-table slot that will be
		 *	reclaimed at next sm_ipc_stop -- far better than
		 *	aborting and breaking the deliberate `ionexit k n`
		 *	restart flow that ION relies on for relay
		 *	intermediates and similar use cases.		*/

		writeMemo("[?] Global IPC lock owner died mid-operation; \
marking lock consistent and continuing (sem-table may have an orphaned slot \
until next sm_ipc_stop).");
		oK(pthread_mutex_consistent(&psemGlobal->ipcMutex));
		/*	fall through holding the now-consistent lock.	*/
	}
	else if (rc == ENOTRECOVERABLE)
	{
		/*	A previous EOWNERDEAD acquirer exited (or aborted)
		 *	without marking the mutex consistent.  This is
		 *	unrecoverable for everyone -- abort with
		 *	diagnostics, flushing stdio first so the message
		 *	actually reaches ion.log before abort() kills
		 *	the process.					*/

		putErrmsg("Global IPC lock is unrecoverable (a previous \
holder died mid-operation and recovery did not complete). Aborting.", NULL);
		printStackTrace();
		fflush(NULL);
		sm_Abort();
	}
	else if (rc != 0)
	{
		putSysErrmsg("Can't lock global IPC mutex", itoa(rc));
		printStackTrace();
		fflush(NULL);
		sm_Abort();
	}
#else
	sem_t *ipcsem = _ipcSemaphore(IPC_ACTION_LOOKUP);

	while (sem_wait(ipcsem) == -1) {
		if (errno == EINTR) {
			continue;  /* not expected, but not fatal*/
		} else {
			putSysErrmsg("takeIpcLock failed", NULL);
			CHKVOID(0);   /* at least this will show up in the log */
		}
	}
#endif
}


static void	giveIpcLock(void)
{
#ifdef ION_HAVE_ROBUST_MUTEX
	SmGlobalSemtable	*psemGlobal = _sembase(IPC_ACTION_LOOKUP);

	CHKVOID(psemGlobal != NULL);
	if (psemGlobal->ipcMutexCreated)
	{
		oK(pthread_mutex_unlock(&psemGlobal->ipcMutex));
	}

	/*	Restore the signal mask saved by takeIpcLock.  Any
	 *	SIGTERM/SIGINT delivered to this thread while the lock
	 *	was held is now delivered, including the shutdown
	 *	handlers' calls back into sm_SemEnd that the blocking
	 *	specifically protected against.				*/

	restoreShutdownSignals();
#else
	if (sem_post(_ipcSemaphore(IPC_ACTION_LOOKUP)) == -1) {
		putSysErrmsg("giveIpcLock failed", NULL);
	}
#endif
}

#ifdef ION_HAVE_ROBUST_MUTEX
/*	Test-only helper: take the global IPC lock and leave it held.
 *	Used by ici/test/ipc_lock_recovery_test to set up an orphan.
 *	Not declared in any header; the test declares it manually.   */

void	_sm_test_take_ipc_lock_unreleased(void)
{
	takeIpcLock();
	/*	Intentionally not released; caller dies with lock held. */
}
#endif


sm_SemId	sm_SemCreate(int key, int semType)
{
	SmProcessSemtable	*semTbl = _semTbl(IPC_ACTION_LOOKUP);
	int	i;
	int freeslot;
	SmLocalSem	*sem;
	char sem_name[MAX_NAMED_SEM_KEYLENGTH];
	sem_t *psem;
	mode_t oldmask;  /* save current umode() mask so we can restore it after open() */

	/* Acknowledge unused parameter. */
	(void)semType;

	takeIpcLock();  /* lock global table across ALL Ion instances */\
	/*	If key was specified, try to find it  */
	if (key != SM_NO_KEY) {
		/* check if it's already been created by some process */
		for (i = 0; i < SEM_NSEMS_MAX; i++)
		{
			SmGlobalSem	*gsem = &semTbl->semtablegl->gsemtable[i];

			if (!gsem->inUse)
				continue;

			if (gsem->key == key) {
				giveIpcLock();
				sem = _semGetSem(semTbl,i,0);  /* this will open and sync to global copy */
				return i;
			}
		}
	}

	/* at this point, the KEY was either specified and not found, or unspecified. */
	freeslot = -1;
	for (i = 0; i < SEM_NSEMS_MAX; i++)
	{
		SmGlobalSem	*gsem = &semTbl->semtablegl->gsemtable[i];
		if (!gsem->inUse) {
			freeslot = i;
			break;
		}
	}

	if (freeslot == -1) {
		putErrmsg("Too many semaphores. Recompile to increase SEM_NSEMS_MAX", itoa(SEM_NSEMS_MAX));
		giveIpcLock();
		return SM_SEM_NONE;
	}

	/* at this point, it's a new semaphore and it goes in "freeslot" */
	sem  = &semTbl->lsemtable[freeslot];

	if (key == SM_NO_KEY) {
		key = SEM_ANON_KEY;
	}
	_semGenPosixSemname(sem_name,sizeof(sem_name),freeslot);

	/* ensure that we're using EXACTLY the mode bits in POSIX_NAMED_SEMAPHORES_FILEMODE regardless */
	/* of the account's setting of umask() */
	oldmask = umask(0);

	/* at this point, it's a new key and the name "sem_name" shouldn't be in use */
	psem = sem_open(sem_name, O_CREAT | O_EXCL, POSIX_NAMED_SEMAPHORES_FILEMODE, 0);
	if (psem == SEM_FAILED && errno == EEXIST) {
		/* The slot is free in the global semtable (verified above
		 * under the IPC lock), but a stale named-sem file remains
		 * on disk -- typically leaked by a prior process that died
		 * before unlinking, or by a multi-node "ionexit n" path.
		 * Unlink and retry. */
		writeMemoNote("[i] sm_SemCreate: unlinking stale named sem", sem_name);
		if (sem_unlink(sem_name) == -1 && errno != ENOENT) {
			putSysErrmsg("Can't unlink stale sem file", sem_name);
			umask(oldmask);
			giveIpcLock();
			return SM_SEM_NONE;
		}
		psem = sem_open(sem_name, O_CREAT | O_EXCL, POSIX_NAMED_SEMAPHORES_FILEMODE, 0);
	}
	if (psem == SEM_FAILED) {
		putSysErrmsg("Semaphore open failed for sem file ", sem_name);
		umask(oldmask);  /* restore umask() */
		giveIpcLock();
		return SM_SEM_NONE;
	}

	umask(oldmask);  /* restore umask() */

	/* semaphore named key didn't already exist, but now it does */
	sem->id = psem;
	sem->semgl->key = key;
	sem->semgl->inUse = 1;
	ion_ipc_atomic_set(&sem->semgl->ended, 0);
	ion_ipc_atomic_set(&sem->semgl->refCount, 0);  /* Initialize to 0 - no active users yet */
	ion_ipc_atomic_set(&sem->semgl->pendingDelete, 0);
	sem->handleOpened = 1;  /* Mark handle as opened since we just opened it */

	/* gather usage statistics for memory tuning */
	++semTbl->semtablegl->opensems_current;
	if (semTbl->semtablegl->opensems_current > semTbl->semtablegl->opensems_max)
		semTbl->semtablegl->opensems_max = semTbl->semtablegl->opensems_current;

	/* tell other ION processes that their local copy is out of date */
	sem->lseq = (smSequence)ion_ipc_atomic_get_and_increment(&sem->semgl->gseq, 1) + 1;

	/* Initialize semaphore value (first taker succeeds)
	 * Call sem_post directly instead of sm_SemGive to avoid deadlock
	 * since we're already holding the IPC lock */
	if (sem_post(psem) == -1)
	{
		putSysErrmsg("Can't initialize semaphore", itoa(freeslot));
	}

	giveIpcLock();

	return freeslot;
}

/* Internal helper - must be called with IPC lock held */
static void _sm_SemCompleteDeletePosix(SmProcessSemtable *semTbl, sm_SemId i)
{
	SmLocalSem *sem = &semTbl->lsemtable[i];
	SmGlobalSem *gsem = sem->semgl;
	char sem_name[MAX_NAMED_SEM_KEYLENGTH];

	/* Close local handle if open */
	if (sem->handleOpened && sem->id != NULL && sem->id != SEM_FAILED)
	{
		if (sem_close(sem->id) == -1)
		{
			putSysErrmsg("Can't close semaphore", itoa(i));
		}
		sem->id = NULL;
		sem->handleOpened = 0;
	}

	/* Unlink the named semaphore */
	_semGenPosixSemname(sem_name, sizeof(sem_name), i);
	if (sem_unlink(sem_name) == -1)
	{
		if (errno != ENOENT)
		{
			putSysErrmsg("Can't unlink semaphore", sem_name);
		}
	}

	/* Update global state */
	gsem->inUse = 0;
	ion_ipc_atomic_set(&gsem->ended, 0);
	gsem->key = SM_NO_KEY;
	ion_ipc_atomic_set(&gsem->refCount, 0);
	ion_ipc_atomic_set(&gsem->pendingDelete, 0);
	ion_ipc_atomic_get_and_increment(&gsem->gseq, 1);  /* Invalidate all cached local copies */

	/* Update local state */
	sem->lseq = 0;
	sem->handleOpened = 0;  /* Reset handle opened flag */

	/* Update statistics */
	semTbl->semtablegl->opensems_current--;
}

void	sm_SemDelete(sm_SemId i)
{
	SmProcessSemtable *semTbl = _semTbl(IPC_ACTION_LOOKUP);
	SmLocalSem *sem;
	SmGlobalSem *gsem;

	if (i < 0)
	{
		writeMemoNote("[?] sm_SemDelete negative semaphore ID (may be normal during shutdown)", itoa(i));
		return;
	}

	if (i >= SEM_NSEMS_MAX)
	{
		writeMemoNote("[!] sm_SemDelete semaphore ID out of range (bug)", itoa(i));
		return;
	}

	takeIpcLock();

	/* Look up the semaphore */
	sem = _semGetSem(semTbl, i, 1);  /* 1 = already locked */

	if (sem == NULL)
	{
		/* Not currently in use - nothing to do */
		giveIpcLock();
		return;
	}

	gsem = sem->semgl;

	/* Check if anyone is using the semaphore */
	if (ion_ipc_atomic_get(&gsem->refCount) > 0)
	{
		/* Defer deletion until all users release it */
		ion_ipc_atomic_set(&gsem->pendingDelete, 1);
		giveIpcLock();
#ifdef DEBUG_POSIX_NAMED_SEMAPHORES
		writeMemoNote("Semaphore deletion deferred, refCount",
				itoa(ion_ipc_atomic_get(&gsem->refCount)));
#endif
		return;
	}

	/* No active users - delete immediately */
	_sm_SemCompleteDeletePosix(semTbl, i);
	giveIpcLock();
}


int	sm_SemTake(sm_SemId i)
{
	SmProcessSemtable *semTbl = _semTbl(IPC_ACTION_LOOKUP);
	SmLocalSem *sem;
	SmGlobalSem *gsem;

	CHKERR(semTbl);
	CHKERR(i >= 0);
	CHKERR(i < SEM_NSEMS_MAX);

	/* Sync local semaphore state with global (takes IPC lock internally if needed) */
	sem = _semGetSem(semTbl, i, 0);
	if (sem == NULL)
	{
		putErrmsg("Can't access semaphore", itoa(i));
		return -1;
	}

	gsem = sem->semgl;

	/* Check if semaphore is deleted or pending deletion */
	if (!gsem->inUse || ion_ipc_atomic_get(&gsem->pendingDelete))
	{
		putErrmsg("Can't take deleted or pending-delete semaphore", itoa(i));
		return -1;
	}

	/* Atomically increment reference count (lock-free) */
	ion_ipc_atomic_get_and_increment(&gsem->refCount, 1);

	/* Take the semaphore */
	if (sem == NULL || sem->id == NULL)
	{
		putErrmsg("Semaphore or semaphore handle is NULL", itoa(i));
		/* Atomically decrement reference count before returning */
		ion_ipc_atomic_get_and_decrement(&gsem->refCount, 1);
		return -1;
	}

	/* Validate handle is properly opened */
	if (!sem->handleOpened)
	{
		putErrmsg("Semaphore handle not opened", itoa(i));
		/* Atomically decrement reference count before returning */
		ion_ipc_atomic_get_and_decrement(&gsem->refCount, 1);
		return -1;
	}

	while (1)
	{
		int wait_result = sem_wait(sem->id);
		if (wait_result == 0)
			break;
		if (errno == EINTR)
		{
			/* Check if semaphore was ended during signal interruption.
			 * If so, return success so caller can check sm_SemEnded()
			 * and handle graceful shutdown. This matches the behavior
			 * of other semaphore implementations (VxWorks, SVR4, etc.) */
			if (ion_ipc_atomic_get(&gsem->ended))
			{
				return 0;
			}
			continue;  /* Retry on signal interruption */
		}

		/* Error - decrement refCount atomically before returning */
		ion_ipc_atomic_get_and_decrement(&gsem->refCount, 1);

		putSysErrmsg("Can't take semaphore", itoa(i));
		return -1;
	}

	/* Check if semaphore was ended while we were waiting.
	 * If so, return success so caller can check sm_SemEnded() and
	 * handle graceful shutdown. This matches the behavior of other
	 * semaphore implementations (VxWorks, SVR4, etc.) where sm_SemTake
	 * returns 0 and the caller is expected to check sm_SemEnded(). */
	if (ion_ipc_atomic_get(&gsem->ended))
	{
		return 0;
	}

#ifdef DEBUG_SEMAPHORE_HANG
	writeMemoNote("[DEBUG] sm_SemTake: got sem", itoa(i));
#endif
	return 0;
}

void	sm_SemGive(sm_SemId i)
{
	SmProcessSemtable *semTbl = _semTbl(IPC_ACTION_LOOKUP);
	SmLocalSem *sem;
	SmGlobalSem *gsem;

	if (semTbl == NULL)
	{
		return;
	}

	sem = _semGetSem(semTbl, i, 0);
	if (sem == NULL)
	{
		writeMemoNote("[?] sm_SemGive on invalid semaphore (may be normal during shutdown)", itoa(i));
		return;
	}

#ifdef DEBUG_SEMAPHORE_HANG
	writeMemoNote("[DEBUG] sm_SemGive: giving sem", itoa(i));
#endif
	/* Give the semaphore first */
	if (sem_post(sem->id) == -1)
	{
		putSysErrmsg("Can't give semaphore", itoa(i));
		/* Still need to decrement refCount even if post fails */
	}

	gsem = sem->semgl;

	/* Atomically decrement reference count (lock-free) */
	ion_ipc_atomic_get_and_decrement(&gsem->refCount, 1);

	/* If a deferred delete is pending and we just released the last
	 * reference, complete it now -- otherwise the slot stays inUse=1
	 * forever and the named-sem file is never unlinked, leading to
	 * stale-file drift across runs. */
	if (ion_ipc_atomic_get(&gsem->refCount) == 0
			&& ion_ipc_atomic_get(&gsem->pendingDelete))
	{
		takeIpcLock();
		/* Re-check under the IPC lock: another thread may have
		 * raised refCount or already completed the delete. */
		if (gsem->inUse
				&& ion_ipc_atomic_get(&gsem->refCount) == 0
				&& ion_ipc_atomic_get(&gsem->pendingDelete))
		{
			_sm_SemCompleteDeletePosix(semTbl, i);
		}
		giveIpcLock();
	}

#ifdef DEBUG_SEMAPHORE_HANG
	writeMemoNote("[DEBUG] sm_SemGive: gave sem", itoa(i));
#endif
}

void	sm_SemEnd(sm_SemId i)
{
	SmProcessSemtable *semTbl = _semTbl(IPC_ACTION_LOOKUP);
	SmLocalSem *sem;

	// to match semantics of SVR4 code when calling this on a closed semaphore,
	// we don't check for that, only that the semphore index is valid.
	if (i < 0)
	{
		writeMemoNote("[?] sm_SemEnd negative semaphore ID (may be normal during shutdown)", itoa(i));
		return;
	}

	if (i >= SEM_NSEMS_MAX)
	{
		writeMemoNote("[!] sm_SemEnd semaphore ID out of range (bug)", itoa(i));
		return;
	}

	/* Sync local semaphore first, then mark as ended and wake waiters.
	 * We do this all under a single IPC lock hold to prevent races. */
	takeIpcLock();
	sem = _semGetSem(semTbl, i, 1);  /* 1 = already locked */
	if (sem == NULL)
	{
		/* Semaphore not in use - just mark ended in global table */
		ion_ipc_atomic_set(&semTbl->lsemtable[i].semgl->ended, 1);
		giveIpcLock();
		return;
	}

	ion_ipc_atomic_set(&sem->semgl->ended, 1);

	/* Wake up any waiting threads/processes without changing refCount.
	 * Note: We use sem_post() directly here instead of sm_SemGive()
	 * because sm_SemGive() decrements refCount, but sm_SemEnd() never
	 * called sm_SemTake(), so there's no refCount to decrement.
	 * Any threads blocked on sm_SemTake() will wake up and see the
	 * 'ended' flag, then call sm_SemGive() themselves to balance
	 * their refCount.
	 *
	 * We post multiple times to wake ALL waiting threads, since a single
	 * sem_post() only wakes one waiter. Use refCount since that represents
	 * the number of threads that could be waiting. Always post at least
	 * once to ensure any waiter is woken. */
	if (sem->id != NULL)
	{
		int waiters = ion_ipc_atomic_get(&sem->semgl->refCount);
		if (waiters < 1) waiters = 1;    /* Always post at least once */

		for (int j = 0; j < waiters; j++)
		{
			if (sem_post(sem->id) == -1)
			{
				/* If sem_post fails, semaphore might be full or invalid */
				if (errno != EOVERFLOW)
				{
					putSysErrmsg("Can't wake waiters for ended semaphore", itoa(i));
					break;  /* Stop trying if we get a real error */
				}
				/* EOVERFLOW means semaphore value would exceed SEM_VALUE_MAX,
				 * which is fine - it means we've posted enough to wake waiters */
				break;
			}
		}
	}

	giveIpcLock();
}

int	sm_SemEnded(sm_SemId i)
{
	SmProcessSemtable *semTbl = _semTbl(IPC_ACTION_LOOKUP);
	SmLocalSem *sem;
	int	ended;

	// to match semantics of SVR4 code when calling this on a closed semaphore,
	// we don't check for that, only that the semphore index is valid.
	if (i < 0)
	{
		writeMemoNote("[?] sm_SemEnded negative semaphore ID (may be normal during shutdown)", itoa(i));
		return 0;
	}

	if (i >= SEM_NSEMS_MAX)
	{
		writeMemoNote("[!] sm_SemEnded semaphore ID out of range (bug)", itoa(i));
		return 0;
	}

	sem = &semTbl->lsemtable[i];
	ended = ion_ipc_atomic_get(&sem->semgl->ended);
	if (ended)
	{
		sm_SemGive(i);	/*	Enable multiple tests.		*/
	}

	return ended;
}
void	sm_SemUnend(sm_SemId i)
{
	SmProcessSemtable *semTbl = _semTbl(IPC_ACTION_LOOKUP);
	SmLocalSem *sem;

	// to match semantics of SVR4 code when calling this on a closed semaphore,
	// we don't check for that, only that the semphore index is valid.
	if (i < 0)
	{
		writeMemoNote("[?] sm_SemUnend negative semaphore ID (may be normal during shutdown)", itoa(i));
		return;
	}

	if (i >= SEM_NSEMS_MAX)
	{
		writeMemoNote("[!] sm_SemUnend semaphore ID out of range (bug)", itoa(i));
		return;
	}

	sem = &semTbl->lsemtable[i];
	ion_ipc_atomic_set(&sem->semgl->ended, 0);
}

/* many posix semaphore systems that implement "named semaphores" do NOT implement sem_timedwait() */
/* ... so we'll have to do it old school with an alarm clock signal */
static volatile sig_atomic_t	semTakeTimedOut = 0;

static void	handleTimeout(int signum)
{
	/* Acknowledge unused parameter. */
	(void)signum;
	semTakeTimedOut = 1;
	return;
}

int	sm_SemTakeTimed(sm_SemId i, int timeoutSeconds)
{
	SmProcessSemtable	*semTbl = _semTbl(IPC_ACTION_LOOKUP);
	SmLocalSem		*sem;
	SmGlobalSem		*gsem;

	CHKERR(semTbl);
	CHKERR(i >= 0);
	CHKERR(i < SEM_NSEMS_MAX);

	sem = _semGetSem(semTbl, i, 0);
	if (sem == NULL)
	{
		putErrmsg("Can't access semaphore", itoa(i));
		return -1;
	}

	gsem = sem->semgl;

	if (!gsem->inUse || ion_ipc_atomic_get(&gsem->pendingDelete))
	{
		putErrmsg("Can't take deleted or pending-delete semaphore",
				itoa(i));
		return -1;
	}

	if (sem->id == NULL || !sem->handleOpened)
	{
		putErrmsg("Semaphore handle not valid", itoa(i));
		return -1;
	}

	/* Atomically increment reference count before attempting take. */
	ion_ipc_atomic_get_and_increment(&gsem->refCount, 1);

	if (timeoutSeconds < 1) timeoutSeconds = 1;
	semTakeTimedOut = 0;
	isignal(SIGALRM, handleTimeout);
	oK(alarm(timeoutSeconds));

	while (sem_wait(sem->id) < 0)
	{
		if (errno == EINTR)
		{
			if (semTakeTimedOut)
			{
				oK(alarm(0));
				isignal(SIGALRM, SIG_DFL);
				ion_ipc_atomic_get_and_decrement(&gsem->refCount,
						1);
				return 1;	/*	Timed out.	*/
			}

			if (ion_ipc_atomic_get(&gsem->ended))
			{
				oK(alarm(0));
				isignal(SIGALRM, SIG_DFL);
				return 0;	/*	Ended.		*/
			}

			continue;
		}

		oK(alarm(0));
		isignal(SIGALRM, SIG_DFL);
		ion_ipc_atomic_get_and_decrement(&gsem->refCount, 1);
		putSysErrmsg("Can't take semaphore", itoa(i));
		return -1;
	}

	oK(alarm(0));
	isignal(SIGALRM, SIG_DFL);
	return 0;
}

int	sm_SemUnwedge(sm_SemId i, int timeoutSeconds)
{
	SmProcessSemtable *semTbl = _semTbl(IPC_ACTION_LOOKUP);
	SmLocalSem *sem;

	if (semTbl == NULL)
	{
		return -1;
	}

	sem = _semGetSem(semTbl, i, 0);
	if (sem == NULL)
	{
		putErrmsg("Can't unwedge deleted semaphore (may be normal during shutdown)", itoa(i));
		return -1;
	}

	if (timeoutSeconds < 1) timeoutSeconds = 1;

	/* sem_timedwait() not usually provided - use signals instead */
	isignal(SIGALRM, handleTimeout);
	oK(alarm(timeoutSeconds));

	if (sem_wait(sem->id) == -1)
	{
		switch (errno) {
			case EINTR:
				/* timeout alarm went off (or another signal that we don't understand) */
				break;  /* we'll assume it was the timer - can't tell */

			default:
				putSysErrmsg("Can't unwedge semaphore", itoa(i));
				oK(alarm(0));
				isignal(SIGALRM, SIG_DFL);
				return -1;
		}
	}

	oK(alarm(0));
	isignal(SIGALRM, SIG_DFL);

	if (sem_post(sem->id) < 0) {
		putSysErrmsg("Can't unwedge semaphore", itoa(i));
		return -1;
	}

	return 0;
}


#endif /* POSIX_NAMED_SEMAPHORES */



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

#else  /* not RTOS_SHM */


/* ---- Unique IPC key system for "process" architectures Linux/Macos/Solaris ------ */

#if defined(SVR4_SEMAPHORES) && defined(POSIX_NAMED_SEMAPHORES)
#error SVR4_SEMAPHORES and POSIX_NAMED_SEMAPHORES defined - pick one
#endif


#if defined(POSIX_NAMED_SEMAPHORES) || defined(SVR4_SEMAPHORES)
/* This is only for SVR4 / Posix Named Semaphores */
/*  Because we already have an ION-wide semaphore table shared by all ION instances and processes,
    We will use that table to store a GLOBAL "unique" key, much like the RTEMS version does.  However
    Because the ION code uses that key, this code ensures that it will not return a "unique" key
    that is already the key of an ION semaphore or the key of an SVR4 shared memory region (since)
    that's what the random keys are used to name.  Note that this is only a heuristic; it's possible
    that the unique key that wasn't in use when returned (by a memory region or semaphore), WILL be in use
    by the time the code gets around to using that key to actually create such a thing.
    Note that the ION code won't be able to cause that failure,
    but some other process (that is NOT part of ION) might. */

/* internal version - assumes that IpcLock is already held!! */
static int	_sm_GetUniqueKey_internal(
#if defined(SVR4_SEMAPHORES)
	SemaphoreBase	*sembase
#else
	SmProcessSemtable *sembase
#endif
)
{
	unsigned tryKey;
	unsigned int *p_ipcUniqueKey;  /* initialized during semaphore initization routines */

#if defined(SVR4_SEMAPHORES)
	p_ipcUniqueKey = &sembase->ipcUniqueKey;				/* In semaphore structure for SVR4 */
#elif defined(POSIX_NAMED_SEMAPHORES)
	p_ipcUniqueKey = &sembase->semtablegl->ipcUniqueKey;	/* In semaphore structure for Posix Named Semaphores */
#else
#error _sm_GetUniqueKey_internal NOT updated to support this environment
#endif

	for (int retries=0; ; ++retries) {
		/*	In the expected case, only one iteration is required.
			In the worst case, you retry for a number of iterations
			whose max is the sum of the number of shared memory segments and total semaphores.
			In the fatal case, some new ION function does something really wrong
			and we'll check just so there's a record in the log that we're looping 'a lot' */
		CHKERR(retries < 10000);

		tryKey = ++(*p_ipcUniqueKey);		/*	can wrap around	*/

		/* keep it to 31 bits and not zero during wrap around */
		if ((tryKey > UNIQUE_KEY_PROCESSES_MAX) || (tryKey == 0)) { /* zero test is redundant given code logic, but clearer */
			tryKey = (*p_ipcUniqueKey) = UNIQUE_KEY_PROCESSES_INITIAL; /* start over at the bottom */
		}

		if (_semKeyExists(tryKey)) {
			/* loop around and try another */
		} else if (_shmKeyExists(tryKey)) {
			/* loop around and try another */
		} else {
			/* we can use this one */
			break;
		}
	}

	return(tryKey);
}
/* when called externally, we need to grab the IPC lock */
int	sm_GetUniqueKey(void)
{
	int ret;
#if defined(SVR4_SEMAPHORES)
	SemaphoreBase	*sembase = _sembase(IPC_ACTION_LOOKUP);
#else
	SmProcessSemtable *sembase = _semTbl(IPC_ACTION_LOOKUP);
#endif

	CHKERR(sembase);

	takeIpcLock();
	ret = _sm_GetUniqueKey_internal(sembase);
	giveIpcLock();

	return(ret);
}

#endif /* end of POSIX_NAMED_SEMAPHORES || SVR4_SEMAPHORES */

/* ----- back to NOT STOS_SHM --------- */

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
#ifdef ION_LWT
	int	argc = 0;
#endif
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
#ifdef ION_LWT
			if (argv[i] != NULL)
			{
				argc++;
			}
#endif
		}
	}

	/*	Skip over any trailing whitespace.			*/

	while (isspace((unsigned char) *cursor))
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

void sm_TaskKillWait(int taskId, char *text, char *note)
{
	int i;

	/*
	 * Reap a daemon that has already been told to stop (via SIGTERM or
	 * sm_SemEnd) but might not honor it -- e.g. a snooze() loop on RTEMS
	 * whose SIGTERM handler never runs. Wait up to ~5 seconds for a
	 * graceful exit; if the task is still alive, log 'text' (with optional
	 * 'note' appended) and force termination with SIGKILL, then wait up to
	 * ~1 more second for it to die.
	 */

	if (taskId == ERROR)
	{
		return;
	}

	for (i = 0; i < 50 && sm_TaskExists(taskId); i++)
	{
		microsnooze(100000);
	}

	if (sm_TaskExists(taskId))
	{
		if (note)
		{
			writeMemoNote(text, note);
		}
		else
		{
			writeMemo(text);
		}

		sm_TaskKill(taskId, SIGKILL);
		for (i = 0; i < 10 && sm_TaskExists(taskId); i++)
		{
			microsnooze(100000);
		}
	}
}
