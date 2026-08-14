/*
	platform.c:	platform-dependent implementation of common
			functions, to simplify porting.
									*/
/*	Copyright (c) 1997, California Institute of Technology.		*/
/*	ALL RIGHTS RESERVED. U.S. Government Sponsorship		*/
/*	acknowledged.							*/
/*									*/
/*	Author: Scott Burleigh, Jet Propulsion Laboratory		*/
/*									*/
/*	Scalar/SDNV conversion functions written by			*/
/*	Ioannis Alexiadis, Democritus University of Thrace, 2011.	*/
/*									*/
#include "platform.h"
#include "ion_atomic.h"
#include "ion_network.h"
#include "ion.h"		/*	getIonsdr (used by _iEnd defense)*/
#include "sdrxn.h"		/*	sdr_in_xn, sdr_drop_xn		*/

/* Only for Ubuntu as of ION 4.1.2 */
#if defined (TCPCL_LOW_CYCLE)
#include <netinet/tcp.h>
#endif

#ifdef HAVE_EXECINFO_H
#include <execinfo.h>
#endif

#define	ABORT_AS_REQD		if (_coreFileNeeded(NULL)) sm_Abort()

void	icopy(char *fromPath, char *toPath)
{
#if defined (VXWORKS)
	oK(copy(fromPath, toPath));
#elif defined (RTEMS)
	int	argc = 2;
	char	*argv[2];

	argv[0] = fromPath;
	argv[1] = toPath;
	oK(rtems_shell_main_cp(argc, argv));
#else
	int	pid = fork();
	int	status;

	if (pid)	/*	Parent process.				*/
	{
		waitpid(pid, &status, 0);
	}
	else		/*	Child process.				*/
	{
		execlp("cp", "cp", "--", fromPath, toPath, (char *) 0);
	}
#endif
}

#if defined (VXWORKS)

typedef struct rlock_str
{
	SEM_ID	semaphore;
	int	owner;
	short	count;
	short	init;
} Rlock;		/*	Private-memory semaphore.		*/
/* the next line won't compile if the semaphore structure isn't large enough -  increase size of ResourceLock in platform.h */
int verify_sufficient_semaphore_space[(sizeof(Rlock) <= sizeof(ResourceLock))?1:-1];    /* compile-time assertion check */

int	createFile(const char *filename, int flags)
{
	int	result;

	if (filename == NULL)
	{
		ABORT_AS_REQD;
		return ERROR;
	}

	/*	VxWorks open(2) will only create a file on an NFS
	 *	network device.  The only portable flag values are
	 *	O_WRONLY and O_RDWR.  See creat(2) and open(2).		*/

	result = creat(filename, flags);
	if (result < 0)
	{
		putSysErrmsg("can't create file", filename);
	}

	return result;
}

int	initResourceLock(ResourceLock *rl)
{
	Rlock	*lock = (Rlock *) rl;

	if (lock == NULL)
	{
		ABORT_AS_REQD;
		return ERROR;
	}

	if (lock->init)
	{
		return 0;
	}

	lock->semaphore = semBCreate(SEM_Q_PRIORITY, SEM_FULL);
	if (lock->semaphore == NULL)
	{
		return ERROR;
	}

	lock->owner = NONE;
	lock->count = 0;
	lock->init = 1;
	return 0;
}

void	killResourceLock(ResourceLock *rl)
{
	Rlock	*lock = (Rlock *) rl;

	if (lock && lock->init && lock->count == 0)
	{
		oK(semDelete(lock->semaphore));
		lock->semaphore = NULL;
		lock->init = 0;
	}
}

void	lockResource(ResourceLock *rl)
{
	Rlock	*lock = (Rlock *) rl;
	int	tid;

	if (lock && lock->init)
	{
		tid = taskIdSelf();
		if (tid != lock->owner)
		{
			oK(semTake(lock->semaphore, WAIT_FOREVER));
			lock->owner = tid;
		}

		(lock->count)++;
	}
}

void	unlockResource(ResourceLock *rl)
{
	Rlock	*lock = (Rlock *) rl;
	int	tid;

	if (lock && lock->init)
	{
		tid = taskIdSelf();
		if (tid == lock->owner)
		{
			(lock->count)--;
			if (lock->count == 0)
			{
				lock->owner = NONE;
				oK(semGive(lock->semaphore));
			}
		}
	}
}

void	closeOnExec(int fd)
{
	return;		/*	N/A for non-Unix operating system.	*/
}

void	snooze(unsigned int seconds)
{
	struct timespec	ts;

	ts.tv_sec = seconds;
	ts.tv_nsec = 0;
	oK(nanosleep(&ts, NULL));
}

void	microsnooze(unsigned int usec)
{
	struct timespec	ts;

	ts.tv_sec = usec / 1000000;
	ts.tv_nsec = (usec % 1000000) * 1000;
	oK(nanosleep(&ts, NULL));
}

char	*system_error_msg()
{
	return strerror(errno);
}

#ifndef VXWORKS6
int	getpid()
{
	return taskIdSelf();
}
#endif

int	gettimeofday(struct timeval *tvp, void *tzp)
{
	struct timespec	cur_time;

	CHKERR(tvp);

#ifdef FSWTIME
#include "fswtime.c"
#else
	/*	Use the internal POSIX timer.				*/

	clock_gettime(CLOCK_REALTIME, &cur_time);
	tvp->tv_sec = cur_time.tv_sec;
	tvp->tv_usec = cur_time.tv_nsec / 1000;
#endif
	return 0;
}

void	getCurrentTime(struct timeval *tvp)
{
	gettimeofday(tvp, NULL);
}

unsigned long	getClockResolution()
{
	struct timespec	ts;

	clock_getres(CLOCK_REALTIME, &ts);
	return ts.tv_nsec / 1000;
}

#ifdef ION_NO_DNS
#ifdef FSWLAN
#include "fswlan.c"
#endif
#else
unsigned int	getInternetAddress(char *hostName)
{
	int	hostNbr;

	CHKZERO(hostName);
	hostNbr = hostGetByName(hostName);
	if (hostNbr == ERROR)
	{
		putSysErrmsg("can't get address for host", hostName);
		return BAD_HOST_NAME;
	}

	return (unsigned int) ntohl(hostNbr);
}

char	*getInternetHostName(unsigned int hostNbr, char *buffer)
{
	CHKNULL(buffer);
	if (hostGetByAddr((int) hostNbr, buffer) < 0)
	{
		putSysErrmsg("can't get name for host", utoa(hostNbr));
		return NULL;
	}

	return buffer;
}

int	getNameOfHost(char *buffer, int bufferLength)
{
	int	result;

	CHKERR(buffer);
	result = gethostname(buffer, bufferLength);
	if (result < 0)
	{
		putSysErrmsg("can't get local host name", NULL);
	}

	return result;
}

char	*getNameOfUser(char *buffer)
{
	CHKNULL(buffer);
#ifdef FSWUSER
#include "fswuser.c"
#else
	remCurIdGet(buffer, NULL);
	return buffer;
#endif
}

int	reUseAddress(int fd)
{
#ifdef REUSEADDR_UNAVBL
	return 0;
#else
	int	result;
	int	i = 1;

	result = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *) &i,
			sizeof i);
	if (result < 0)
	{
		putSysErrmsg("can't make socket's address reusable", NULL);
	}

	return result;
#endif
}

int	watchSocket(int fd)
{
	int		result;
	struct linger	lctrl = {0, 0};
	int		kctrl = 1;

	result = setsockopt(fd, SOL_SOCKET, SO_LINGER, (char *) &lctrl,
			sizeof lctrl);
	if (result < 0)
	{
		putSysErrmsg("can't set linger on socket", NULL);
		return result;
	}

	result = setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (char *) &kctrl,
			sizeof kctrl);
	if (result < 0)
	{
		putSysErrmsg("can't set keepalive on socket", NULL);
	}

	return result;
}
#endif	/*	ION_NO_DNS						*/

int	makeIoNonBlocking(int fd)
{
	int	result;
	int	setting = 1;

	result = ioctl(fd, FIONBIO, (int) &setting);
	if (result < 0)
	{
		putSysErrmsg("can't make IO non-blocking", NULL);
	}

	return result;
}

int	strcasecmp(const char *s1, const char *s2)
{
	register int c1, c2;

	CHKZERO(s1);
	CHKZERO(s2);
	for ( ; ; )
	{
		/* STDC requires tolower(3) to work for all
		** ints. Unfortunately, not all C's are STDC.
		*/
		c1 = *s1; if (isupper(c1)) c1 = tolower(c1);
		c2 = *s2; if (isupper(c2)) c2 = tolower(c2);
		if (c1 != c2) return c1 - c2;
		if (c1 == 0) return 0;
		++s1; ++s2;
	}
}

int	strncasecmp(const char *s1, const char *s2, size_t n)
{
	register int c1, c2;

	CHKZERO(s1);
	CHKZERO(s2);
	for ( ; n > 0; --n)
	{
		/* STDC requires tolower(3) to work for all
		** ints. Unfortunately, not all C's are STDC.
		*/
		c1 = *s1; if (isupper(c1)) c1 = tolower(c1);
		c2 = *s2; if (isupper(c2)) c2 = tolower(c2);
		if (c1 != c2) return c1 - c2;
		if (c1 == 0) return 0;
		++s1; ++s2;
	}

	return 0;
}

#endif	/*	End of #if defined VXWORKS				*/

#if defined (darwin) || defined (freebsd)

void	*memalign(size_t boundary, size_t size)
{
	(void)boundary;  /* Unused parameter */
	return malloc(size);
}

#endif

#ifndef VXWORKS			/*	Common for all O/S but VXWORKS.	*/

int	createFile(const char *filename, int flags)
{
	int	result;

	/*	POSIX-UNIX creat(2) will only create a file for
	 *	writing.  The only portable flag values are
	 *	O_WRONLY and O_RDWR.  See creat(2) and open(2).		*/

	if (filename == NULL)
	{
		ABORT_AS_REQD;
		return ERROR;
	}

	result = iopen(filename, (flags | O_CREAT | O_TRUNC), 0666);
	if (result < 0)
	{
		putSysErrmsg("can't create file", filename);
	}

	return result;
}

#ifdef _MULTITHREADED

typedef struct rlock_str
{
	pthread_mutex_t mutex;
	/* * WARNING: Do NOT use ion_atomic_t for this flag.
	 * Legacy ION code frequently initializes ResourceLocks via
	 * memset(&lock, 0, sizeof(ResourceLock)). Under the C99 fallback,
	 * zero-filling an ion_atomic_t corrupts its hidden POSIX mutex,
	 * causing immediate deadlocks/segfaults on access.
	 * This plain int is safely protected from TOCTOU races by the
	 * global g_ResourceLockInitMutex in initResourceLock().
	 */
	int		initialized;
} Rlock;

/* the next line won't compile if the mutex structure isn't large enough -  increase size of ResourceLock in platform.h */
int verify_sufficient_semaphore_space[(sizeof(Rlock) <= sizeof(ResourceLock))?1:-1];    /* compile-time assertion check */

/*
 * This global "meta-lock" is the core of the thread-safe
 * initialization pattern for all ResourceLock instances. It prevents
 * a race condition where multiple threads attempt to initialize the
 * same lock simultaneously.
 *
 * By acquiring this mutex first, the check for the 'init' flag
 * and subsequent calls to pthread_mutex_init() become an atomic
 * operation. It is initialized statically using PTHREAD_MUTEX_INITIALIZER,
 * which is guaranteed by the POSIX standard to be thread-safe.
 */
static pthread_mutex_t  g_ResourceLockInitMutex = PTHREAD_MUTEX_INITIALIZER;


/*
 * Initialize the given ResourceLock as a POSIX recursive mutex.
 */
int initResourceLock(ResourceLock *rl)
{
	Rlock   *lock = (Rlock *) rl;
	pthread_mutexattr_t attr;

	if (lock == NULL)
	{
		/* Cannot initialize a NULL lock. */
		return -1;
	}

	/*
	 * Acquire the global initialization lock. This creates a critical
	 * section, ensuring only one thread can check the 'initialized'
	 * flag and initialize the mutex at any given time.
	 */
	pthread_mutex_lock(&g_ResourceLockInitMutex);

	/*
	* Now that we hold the meta-lock, it is safe to check the flag.
	*/
	if (lock->initialized)
	{
		/* This lock is already initialized. Nothing more to do. */
		pthread_mutex_unlock(&g_ResourceLockInitMutex);
		return 0;
	}

	/*
	* If we are here, we are the first thread to initialize this
	* specific lock. Proceed with recursive mutex initialization.
	*/
	if (pthread_mutexattr_init(&attr) != 0)
	{
		pthread_mutex_unlock(&g_ResourceLockInitMutex);
		return -1;
	}

	if (pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE) != 0)
	{
		pthread_mutexattr_destroy(&attr);
		pthread_mutex_unlock(&g_ResourceLockInitMutex);
		return -1;
	}

	/* Initialize the 'mutex' member. */
	if (pthread_mutex_init(&lock->mutex, &attr) != 0)
	{
		pthread_mutexattr_destroy(&attr);
		pthread_mutex_unlock(&g_ResourceLockInitMutex);
		return -1;
	}

	/* The attributes object is no longer needed after initialization. */
	pthread_mutexattr_destroy(&attr);

	/* * WARNING: Intentionally avoiding ion_atomic_set() here.
	 * Legacy ION code zero-fills ResourceLocks using memset(), which
	 * destroys the internal POSIX mutex used by the C99 atomic fallback.
	 * This plain int assignment is safely protected from TOCTOU races
	 * by the global g_ResourceLockInitMutex meta-lock.
	 */
	lock->initialized = 1;

	/* Release the global initialization lock. */
	pthread_mutex_unlock(&g_ResourceLockInitMutex);

	return 0;
}

void killResourceLock(ResourceLock *rl)
{
	Rlock   *lock = (Rlock *) rl;

	if (lock == NULL)
	{
		return;
	}

	/*
	 * Acquire the global meta-lock to serialize with initResourceLock
	 * and prevent TOCTOU races during destruction.
	 */
	pthread_mutex_lock(&g_ResourceLockInitMutex);

	if (lock->initialized == 0)
	{
		pthread_mutex_unlock(&g_ResourceLockInitMutex);
		return;
	}

	/*
	 * Mark as uninitialized FIRST. This prevents any new lockResource
	 * calls from proceeding while we destroy the mutex.
	 *
	 * WARNING: Intentionally avoiding ion_atomic_set() for the same
	 * memset() corruption reasons as above. The meta-lock ensures
	 * this assignment is race-free.
	 */
	lock->initialized = 0;

	pthread_mutex_unlock(&g_ResourceLockInitMutex);

	/*
	 * pthread_mutex_destroy has undefined behavior if the mutex
	 * is locked. A trylock can safely check this.
	 */
	if (pthread_mutex_trylock(&lock->mutex) == 0)
	{
		/*
		 * We successfully acquired the lock, proving it was not held by another
		 * thread. We must release it before destroying it.
		 */
		pthread_mutex_unlock(&lock->mutex);
		pthread_mutex_destroy(&lock->mutex);
	}
	else
	{
		/*
		 * The mutex is currently locked by another thread. It is unsafe
		 * to destroy it. Restore the initialized flag since we couldn't
		 * complete destruction.
		 *
		 * WARNING: Kept as a plain int to prevent C99 fallback corruption.
		 */
		lock->initialized = 1;
		writeMemo("[!] killResourceLock: Attempted to destroy a locked mutex.");
	}
}

void lockResource(ResourceLock *rl)
{
	Rlock   *lock = (Rlock *) rl;

	if (lock == NULL || lock->initialized == 0)
	{
		return;
	}

	pthread_mutex_lock(&lock->mutex);
}

void unlockResource(ResourceLock *rl)
{
	Rlock   *lock = (Rlock *) rl;

	if (lock == NULL || lock->initialized == 0)
	{
		return;
	}

	pthread_mutex_unlock(&lock->mutex);
}

#else	/*	Only one thread of control in address space.		*/

int	initResourceLock(ResourceLock *rl)
{
	return 0;
}

void	killResourceLock(ResourceLock *rl)
{
	return;
}

void	lockResource(ResourceLock *rl)
{
	return;
}

void	unlockResource(ResourceLock *rl)
{
	return;
}

#endif	/*	end #ifdef _MULTITHREADED				*/

#if (!defined(__linux__) && !defined (freebsd) && !defined (darwin) && !defined (RTEMS))
/*	These things are defined elsewhere for Linux-like op systems.	*/

#ifdef solaris
char	*system_error_msg()
{
	return strerror(errno);
}
#else
extern int	sys_nerr;
extern char	*sys_errlist[];

char	*system_error_msg()
{
	if (errno > sys_nerr)
	{
		return "cause unknown";
	}

	return sys_errlist[errno];
}
#endif	/*	end #ifdef solaris					*/

char	*getNameOfUser(char *buffer)
{
	CHKNULL(buffer);
#ifdef FSWUSER
#include "fswuser.c"
#else
	return cuserid(buffer);
#endif
}

#endif	/*	end #if (!defined(__linux__, freebsd, darwin, RTEMS))	*/

void	closeOnExec(int fd)
{
	oK(fcntl(fd, F_SETFD, FD_CLOEXEC));
}

void	snooze(unsigned int seconds)
{
	struct timespec	ts;

	ts.tv_sec = seconds;
	ts.tv_nsec = 0;
	oK(nanosleep(&ts, NULL));
}

void	microsnooze(unsigned int usec)
{
	struct timespec	ts;

	ts.tv_sec = usec / 1000000;
	ts.tv_nsec = (usec % 1000000) * 1000;
	oK(nanosleep(&ts, NULL));
}

void	getCurrentTime(struct timeval *tvp)
{
	CHKVOID(tvp);
	oK(gettimeofday(tvp, NULL));
}

unsigned long	getClockResolution(void)
{
	/*	Linux clock resolution of Alpha is 1 ms, as is
	 *	Windows XP standard clock resolution, and Solaris
	 *	clock resolution can be configured.  But minimum
	 *	clock resolution in all cases appears to be 10 ms,
	 *	so we use that value since it seems likely to be
	 *	safe in all cases.					*/

	return 10000;
}

#endif	/*	End of #ifndef VXWORKS					*/

#if defined (__SVR4)

int	getNameOfHost(char *buffer, int bufferLength)
{
	struct utsname	name;

	CHKERR(buffer);
	CHKERR(bufferLength > 0);
	if (uname(&name) < 0)
	{
		*buffer = '\0';
		putSysErrmsg("can't get local host name", NULL);
		return -1;
	}

	strncpy(buffer, name.nodename, bufferLength - 1);
	*(buffer + bufferLength - 1) = '\0';
	return 0;
}

int	makeIoNonBlocking(int fd)
{
	int	result;

	result = fcntl(fd, F_SETFL, O_NDELAY);
	if (result < 0)
	{
		putSysErrmsg("can't make IO non-blocking", NULL);
	}

	return result;
}

#if defined (_REENTRANT)	/*	SVR4 multithreaded.		*/

#ifdef ION_NO_DNS
#ifdef FSWLAN
#include "fswlan.c"
#endif
#else
unsigned int	getInternetAddress(char *hostName)
{
	struct hostent	hostInfoBuffer;
	struct hostent	*hostInfo;
	unsigned int	hostInetAddress;
	char		textBuffer[1024];
	int		hostInfoErrno = -1;

	CHKZERO(hostName);
	hostInfo = gethostbyname_r(hostName, &hostInfoBuffer, textBuffer,
			sizeof textBuffer, &hostInfoErrno);
	if (hostInfo == NULL)
	{
		putSysErrmsg("can't get host info", hostName);
		return BAD_HOST_NAME;
	}

	if (hostInfo->h_length != sizeof hostInetAddress)
	{
		putErrmsg("Address length invalid in host info.", hostName);
		return BAD_HOST_NAME;
	}

	memcpy((char *) &hostInetAddress, hostInfo->h_addr, 4);
	return ntohl(hostInetAddress);
}

char	*getInternetHostName(unsigned int hostNbr, char *buffer)
{
	struct hostent	hostInfoBuffer;
	struct hostent	*hostInfo;
	char		textBuffer[128];
	int		hostInfoErrno;

	CHKNULL(buffer);
	hostNbr = htonl(hostNbr);
	hostInfo = gethostbyaddr_r((char *) &hostNbr, sizeof hostNbr, AF_INET,
			&hostInfoBuffer, textBuffer, sizeof textBuffer,
			&hostInfoErrno);
	if (hostInfo == NULL)
	{
		putSysErrmsg("can't get host info", utoa(hostNbr));
		return NULL;
	}

	strncpy(buffer, hostInfo->h_name, MAXHOSTNAMELEN);
	return buffer;
}

int	reUseAddress(int fd)
{
#ifdef REUSEADDR_UNAVBL
	return 0;
#else
	int	result;
	int	i = 1;

	result = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *) &i,
			sizeof i);
#if (defined (SO_REUSEPORT))
#if (!defined(bionic))
	result += setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, (char *) &i,
			sizeof i);
#endif
#endif
	if (result < 0)
	{
		putSysErrmsg("can't make socket address reusable", NULL);
	}

	return result;
#endif
}

int	watchSocket(int fd)
{
	int		result;
	struct linger	lctrl = {0, 0};
	int		kctrl = 1;

	result = setsockopt(fd, SOL_SOCKET, SO_LINGER, (char *) &lctrl,
			sizeof lctrl);
	if (result < 0)
	{
		putSysErrmsg("can't set linger on socket", NULL);
		return result;
	}

	result = setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (char *) &kctrl,
			sizeof kctrl);
	if (result < 0)
	{
		putSysErrmsg("can't set keepalive on socket", NULL);
	}

	return result;
}
#endif	/*	ION_NO_DNS						*/

#else	/*	SVR4 but not multithreaded.				*/

#ifdef ION_NO_DNS
#ifdef FSWLAN
#include "fswlan.c"
#endif
#else
unsigned int	getInternetAddress(char *hostName)
{
	struct hostent	*hostInfo;
	unsigned int	hostInetAddress;

	CHKZERO(hostName);
	hostInfo = gethostbyname(hostName);
	if (hostInfo == NULL)
	{
		putSysErrmsg("can't get host info", hostName);
		return BAD_HOST_NAME;
	}

	if (hostInfo->h_length != sizeof hostInetAddress)
	{
		putErrmsg("Address length invalid in host info.", hostName);
		return BAD_HOST_NAME;
	}

	memcpy((char *) &hostInetAddress, hostInfo->h_addr, 4);
	return ntohl(hostInetAddress);
}

char	*getInternetHostName(unsigned int hostNbr, char *buffer)
{
	struct hostent	*hostInfo;

	CHKNULL(buffer);
	hostNbr = htonl(hostNbr);
	hostInfo = gethostbyaddr((char *) &hostNbr, sizeof hostNbr, AF_INET);
	if (hostInfo == NULL)
	{
		putSysErrmsg("can't get host info", utoa(hostNbr));
		return NULL;
	}

	strncpy(buffer, hostInfo->h_name, MAXHOSTNAMELEN);
	return buffer;
}

int	reUseAddress(int fd)
{
#ifdef REUSEADDR_UNAVBL
	return 0;
#else
	int	result;
	int	i = 1;

	result = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &i, sizeof i);
#if (defined (SO_REUSEPORT))
#if (!defined(bionic))
	result += setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &i, sizeof i);
#endif
#endif
	if (result < 0)
	{
		putSysErrmsg("can't make socket address reusable", NULL);
	}

	return result;
#endif
}

int	watchSocket(int fd)
{
	int		result;
	struct linger	lctrl = {0, 0};
	int		kctrl = 1;

	result = setsockopt(fd, SOL_SOCKET, SO_LINGER, (void *) &lctrl,
			sizeof lctrl);
	if (result < 0)
	{
		putSysErrmsg("can't set linger on socket", NULL);
		return result;
	}

	result = setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (void *) &kctrl,
			sizeof kctrl);
	if (result < 0)
	{
		putSysErrmsg("can't set keepalive on socket", NULL);
	}

	return result;
}
#endif	/*	ION_NO_DNS						*/

#endif	/*	end of #if defined _REENTRANT				*/

#endif	/*	end of #if defined _SVR4				*/

#if (defined(__linux__) || defined (freebsd) || defined (darwin) || defined (RTEMS))

char	*system_error_msg(void)
{
	return strerror(errno);
}

char	*getNameOfUser(char *buffer)
{
	CHKNULL(buffer);
#ifdef FSWUSER
#include "fswuser.c"
#else
	uid_t		euid;
	struct passwd	*pwd;

	/*	Note: buffer is in argument list for portability but
	 *	is not used and therefore is not checked for non-NULL.	*/

	euid = geteuid();
	pwd = getpwuid(euid);
	if (pwd)
	{
		return pwd->pw_name;
	}

	return "";
#endif
}

#ifdef ION_NO_DNS
#ifdef FSWLAN
#include "fswlan.c"
#endif
#else
/* use getaddrinfo for Linux, FreeBSD, macOS, RTEMS */
unsigned int getInternetAddress(char *hostName)
{
	struct addrinfo hints, *res;
	unsigned int hostInetAddress = BAD_HOST_NAME;
	int status;

	CHKZERO(hostName);

	/* Set up hints for IPv4-only resolution */
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;      /* IPv4 only */
	hints.ai_socktype = SOCK_STREAM; /* TCP, consistent with existing usage */
	hints.ai_flags = 0;

	/* Resolve hostname */
	status = getaddrinfo(hostName, NULL, &hints, &res);
	if (status != 0)
	{
		putSysErrmsg("can't get address for host", gai_strerror(status));
		return BAD_HOST_NAME;
	}

	/* Extract IPv4 address from first result */
	if (res->ai_addrlen >= sizeof(struct sockaddr_in))
	{
		struct sockaddr_in *addr = (struct sockaddr_in *)(void *) res->ai_addr;
		hostInetAddress = ntohl(addr->sin_addr.s_addr);
	}
	else
	{
		putErrmsg("Address length invalid.", hostName);
	}

	freeaddrinfo(res);
	return hostInetAddress;
}

char	*getInternetHostName(unsigned int hostNbr, char *buffer)
{
	struct hostent	*hostInfo;

	CHKNULL(buffer);
	hostNbr = htonl(hostNbr);
	hostInfo = gethostbyaddr((char *) &hostNbr, sizeof hostNbr, AF_INET);
	if (hostInfo == NULL)
	{
		putSysErrmsg("can't get host info", utoa(hostNbr));
		return NULL;
	}

	strncpy(buffer, hostInfo->h_name, MAXHOSTNAMELEN);
	return buffer;
}

int	getNameOfHost(char *buffer, int bufferLength)
{
	int	result;

	CHKERR(buffer);
	CHKERR(bufferLength > 0);
	result = gethostname(buffer, bufferLength);
	if (result < 0)
	{
		putSysErrmsg("can't local host name", NULL);
	}

	return result;
}

int	reUseAddress(int fd)
{
#ifdef REUSEADDR_UNAVBL
	return 0;
#else
	int	result;
	int	i = 1;

	result = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (void *) &i,
			sizeof i);
#if (defined (SO_REUSEPORT))
#if (!defined(bionic))
	result += setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, (char *) &i,
			sizeof i);
#endif
#endif
	if (result < 0)
	{
		putSysErrmsg("can't make socket address reusable", NULL);
	}

	return result;
#endif
}
#endif	/*	ION_NO_DNS						*/

int	makeIoNonBlocking(int fd)
{
	int	result;
	int	setting = 1;

	result = ioctl(fd, FIONBIO, &setting);
	if (result < 0)
	{
		putSysErrmsg("can't make IO non-blocking", NULL);
	}

	return result;
}

int	watchSocket(int fd)
{
	int		result;
	struct linger	lctrl = {0, 0};
	int		kctrl = 1;

	result = setsockopt(fd, SOL_SOCKET, SO_LINGER, (void *) &lctrl,
			sizeof lctrl);
	if (result < 0)
	{
		putSysErrmsg("can't set linger on socket", NULL);
		return result;
	}

	result = setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (void *) &kctrl,
			sizeof kctrl);
	if (result < 0)
	{
		putSysErrmsg("can't set keepalive on socket", NULL);
	}

	return result;
}

#endif	/*	end #if (defined(__linux__, freebsd, darwin, RTEMS))	*/

/******************* platform-independent functions *********************/

void	*acquireSystemMemory(size_t size)
{
	void	*block;

	if (size <= 0)
	{
		return NULL;
	}

	size = size + ((sizeof(void *)) - (size % (sizeof(void *))));
#if defined (RTEMS)
	block = malloc(size);	/*	try posix_memalign?		*/
#else
	block = memalign((size_t) (sizeof(void *)), size);
#endif
	if (block)
	{
		TRACK_MALLOC(block);
		memset((char *) block, 0, size);
	}
	else
	{
		putSysErrmsg("Memory allocation failed", itoa(size));
	}

	return block;
}

static void	watchToStdout(char *token)
{
	/*  now handles string */
	printf("%s",token);

	/* previous single char wchar
		oK(putchar(token));
	 */

	oK(fflush(stdout));
}

static Watcher	_watchOneEvent(Watcher *watchFunction)
{
	static Watcher	watcher = watchToStdout;

	if (watchFunction)
	{
		watcher = *watchFunction;
	}

	return watcher;
}

void	setWatcher(Watcher watchFunction)
{
	if (watchFunction)
	{
		oK(_watchOneEvent(&watchFunction));
	}
}

void	iwatch(char token)
{
	char token_str[2] = " ";
	token_str[0] = token;
	(_watchOneEvent(NULL))(token_str);
}

void	iwatch_str(char *token_str)
{
	(_watchOneEvent(NULL))(token_str);
}

static void	logToStdout(char *text)
{
	if (text)
	{
		fprintf(stdout, "%s\n", text);
		fflush(stdout);
	}
}

static Logger	_logOneMessage(Logger *logFunction)
{
	static Logger	logger = logToStdout;

	if (logFunction)
	{
		logger = *logFunction;
	}

	return logger;
}

void	setLogger(Logger logFunction)
{
	if (logFunction)
	{
		oK(_logOneMessage(&logFunction));
	}
}

void	writeMemo(char *text)
{
	if (text)
	{
		(_logOneMessage(NULL))(text);
	}
}

void	writeMemoNote(char *text, char *note)
{
	char	*noteText = note ? note : "";
	char	textBuffer[1024];

	if (text)
	{
		isprintf(textBuffer, sizeof textBuffer, "%.500s: %.500s",
				text, noteText);
		(_logOneMessage(NULL))(textBuffer);
	}
}

void	writeErrMemo(char *text)
{
	writeMemoNote(text, system_error_msg());
}

char	*iToa(int arg)
{
	static ION_THREAD_LOCAL char itoa_str[33];

	isprintf(itoa_str, sizeof itoa_str, "%d", arg);
	return itoa_str;
}

char	*uToa(unsigned int arg)
{
	static ION_THREAD_LOCAL char utoa_str[33];

	isprintf(utoa_str, sizeof utoa_str, "%u", arg);
	return utoa_str;
}

/* For vast values (which may be long or long long depending on platform) */
char *vastToa(vast arg)
{
	static ION_THREAD_LOCAL char vast_str[33];
	isprintf(vast_str, sizeof vast_str, VAST_FIELDSPEC, arg);
	return vast_str;
}

/* For uvast values */
char *uvastToa(uvast arg)
{
	static ION_THREAD_LOCAL char uvast_str[33];
	isprintf(uvast_str, sizeof uvast_str, UVAST_FIELDSPEC, arg);
	return uvast_str;
}

/* For size_t values */
char *sizeToa(size_t arg)
{
	static ION_THREAD_LOCAL char size_str[33];
	isprintf(size_str, sizeof size_str, "%zu", arg);
	return size_str;
}

static int	clipFileName(const char *qualifiedFileName, const char **fileName)
{
	int	fileNameLength;
	int	excessLength;

	fileNameLength = strlen(qualifiedFileName);
	excessLength = fileNameLength - MAX_SRC_FILE_NAME;
	if (excessLength < 0)
	{
		excessLength = 0;
	}

	/*	Clip excessLength bytes off the front of the file
	 *	name by adding excessLength to the string pointer.	*/

	(*fileName) = (qualifiedFileName) + excessLength;
	fileNameLength -= excessLength;
	return fileNameLength;
}

static int	_errmsgs(int lineNbr, const char *qualifiedFileName,
			const char *text, const char *arg, char *buffer)
{
	static char		errmsgs[ERRMSGS_BUFSIZE];
	static int		errmsgsLength = 0;
	static ResourceLock	errmsgsLock;
	static ion_atomic_t	errmsgsLockInit = ION_ATOMIC_INIT(0);	/* Atomic to prevent race */
	int			msgLength;
	int			spaceFreed;
	int			fileNameLength;
	const char		*fileName;
	char			lineNbrBuffer[32];
	int			spaceAvbl;
	int			spaceForText;
	int			spaceNeeded;

	if (!ion_atomic_get(&errmsgsLockInit))
	{
		memset((char *) &errmsgsLock, 0, sizeof(ResourceLock));
		if (initResourceLock(&errmsgsLock) < 0)
		{
			ABORT_AS_REQD;
			return 0;
		}

		ion_atomic_set(&errmsgsLockInit, 1);
	}

	if (buffer)		/*	Retrieving an errmsg.		*/
	{
		if (errmsgsLength == 0)	/*	No more msgs in pool.	*/
		{
			return 0;
		}

		lockResource(&errmsgsLock);
		msgLength = strlen(errmsgs);
		if (msgLength == 0)	/*	No more msgs in pool.	*/
		{
			unlockResource(&errmsgsLock);
			return msgLength;
		}

		/*	Getting a message removes it from the pool,
		 *	releasing space for more messages.		*/

		spaceFreed = msgLength + 1;	/*	incl. last NULL	*/
		memcpy(buffer, errmsgs, spaceFreed);
		errmsgsLength -= spaceFreed;
		memcpy(errmsgs, errmsgs + spaceFreed, errmsgsLength);
		memset(errmsgs + errmsgsLength, 0, spaceFreed);
		unlockResource(&errmsgsLock);
		return msgLength;
	}

	/*	Posting an errmsg.					*/

	if (qualifiedFileName == NULL || text == NULL || *text == '\0')
	{
		return 0;	/*	Ignored.			*/
	}

	fileNameLength = clipFileName(qualifiedFileName, &fileName);
	lockResource(&errmsgsLock);
	isprintf(lineNbrBuffer, sizeof lineNbrBuffer, "%d", lineNbr);
	spaceAvbl = ERRMSGS_BUFSIZE - errmsgsLength;
	spaceForText = 8 + strlen(lineNbrBuffer) + 4 + fileNameLength
			+ 2 + strlen(text);
	spaceNeeded = spaceForText + 1;
	if (arg)
	{
		spaceNeeded += (2 + strlen(arg) + 1);
	}

	if (spaceNeeded > spaceAvbl)	/*	Can't record message.	*/
	{
		if (spaceAvbl < 2)
		{
			/*	Can't even note that it was omitted.	*/

			spaceNeeded = 0;
		}
		else
		{
			/*	Write a single newline message to
			 *	note that this message was omitted.	*/

			spaceNeeded = 2;
			errmsgs[errmsgsLength] = '\n';
			errmsgs[errmsgsLength + 1] = '\0';
		}
	}
	else
	{
		isprintf(errmsgs + errmsgsLength, spaceAvbl,
			"at line %s of %s, %s", lineNbrBuffer, fileName, text);
		if (arg)
		{
			isprintf(errmsgs + errmsgsLength + spaceForText,
				spaceAvbl - spaceForText, " (%s)", arg);
		}
	}

	errmsgsLength += spaceNeeded;
	unlockResource(&errmsgsLock);
	return 0;
}

void	_postErrmsg(const char *fileName, int lineNbr, const char *text,
		const char *arg)
{
	oK(_errmsgs(lineNbr, fileName, text, arg, NULL));
}

void	_putErrmsg(const char *fileName, int lineNbr, const char *text,
		const char *arg)
{
	_postErrmsg(fileName, lineNbr, text, arg);
	writeErrmsgMemos();
}

void	_postSysErrmsg(const char *fileName, int lineNbr, const char *text,
		const char *arg)
{
	char	*sysmsg;
	int	textLength;
	int	maxTextLength;
	char	textBuffer[1024];

	if (text)
	{
		textLength = strlen(text);
		sysmsg = system_error_msg();
		maxTextLength = sizeof textBuffer - (2 + strlen(sysmsg) + 1);
		if (textLength > maxTextLength)
		{
			textLength = maxTextLength;
		}

		isprintf(textBuffer, sizeof textBuffer, "%.*s: %s",
				textLength, text, sysmsg);
		_postErrmsg(fileName, lineNbr, textBuffer, arg);
	}
}

void	_putSysErrmsg(const char *fileName, int lineNbr, const char *text,
		const char *arg)
{
	_postSysErrmsg(fileName, lineNbr, text, arg);
	writeErrmsgMemos();
}

int	getErrmsg(char *buffer)
{
	if (buffer == NULL)
	{
		ABORT_AS_REQD;
		return 0;
	}

	return _errmsgs(0, NULL, NULL, NULL, buffer);
}

void	writeErrmsgMemos(void)
{
	static ResourceLock	memosLock;
	static ion_atomic_t	memosLockInit = ION_ATOMIC_INIT(0);	/* Atomic to prevent race */
	static char		msgwritebuf[ERRMSGS_BUFSIZE];
	static char		*omissionMsg = "[?] message omitted due to \
excessive length";

	/*	Because buffer is static, it is shared.  So access
	 *	to it must be mutexed.					*/

	if (!ion_atomic_get(&memosLockInit))
	{
		memset((char *) &memosLock, 0, sizeof(ResourceLock));
		if (initResourceLock(&memosLock) < 0)
		{
			ABORT_AS_REQD;
			return;
		}

		ion_atomic_set(&memosLockInit, 1);
	}

	lockResource(&memosLock);
	while (1)
	{
		if (getErrmsg(msgwritebuf) == 0)
		{
			break;
		}

		if (msgwritebuf[0] == '\n')
		{
			writeMemo(omissionMsg);
		}
		else
		{
			writeMemo(msgwritebuf);
		}
	}

	unlockResource(&memosLock);
}

void	discardErrmsgs(void)
{
	static char	msgdiscardbuf[ERRMSGS_BUFSIZE];

	/*	The discard buffer is static, therefore shared, but
	 *	its contents are never used for any purpose.  So no
	 *	need to protect it from multiple concurrent users.	*/

	while (1)
	{
		if (getErrmsg(msgdiscardbuf) == 0)
		{
			return;
		}
	}
}

int	_coreFileNeeded(int *ctrl)
{
	static ion_atomic_t	coreFileNeeded = ION_ATOMIC_INIT(CORE_FILE_NEEDED);

	if (ctrl)
	{
		ion_atomic_set(&coreFileNeeded, *ctrl);
	}

	return (int) ion_atomic_get(&coreFileNeeded);
}

int	_iEnd(const char *fileName, int lineNbr, const char *arg)
{
	static int	inIend = 0;

	_postErrmsg(fileName, lineNbr, "Assertion failed.", arg);
	writeErrmsgMemos();

	/*	Defense against cascading SDR unrecoverable errors
	 *	(#983, #1010).  If a CHK macro fires inside an open
	 *	transaction that has already modified non-reversible
	 *	SDR state, returning from the macro's expansion leaves
	 *	sdr->dirty=1 with neither sdr_end_xn nor sdr_cancel_xn
	 *	called -- the next daemon's sdr_begin_xn then hits
	 *	"Orphaned transaction modified a non-reversible SDR;
	 *	cannot recover" and the whole node wedges.
	 *
	 *	Commit the partial state via sdr_drop_xn so the
	 *	transaction closes cleanly.  The partial state may
	 *	itself be inconsistent (half-constructed object,
	 *	half-deleted reference, etc.), but a bounded SDR leak
	 *	is dramatically preferable to a node-wide cascade --
	 *	the daemon keeps running and the next bundle operation
	 *	works normally.
	 *
	 *	inIend guards against re-entry: sdr_drop_xn's own CHK
	 *	macros could fail on a corrupted handle and call back
	 *	into _iEnd.  Single-process static is sufficient; in
	 *	the worst-case race two threads both skip the drop,
	 *	which is no worse than the pre-patch behavior.		*/

	if (!inIend)
	{
		Sdr	sdr;

		inIend = 1;
		sdr = getIonsdr();
		if (sdr != NULL && sdr_in_xn(sdr))
		{
			sdr_drop_xn(sdr, "iEnd: assertion drop");
		}

		inIend = 0;
	}

	printStackTrace();

	/*	Flush stdio after the stack trace so the trace lands
	 *	on disk before sm_Abort -> SIGABRT can race the
	 *	logger.  Without this, fast aborts inside short SDR
	 *	transactions (#1010: 32 us between sdr_begin_xn and
	 *	process death) leave no diagnostic in ion.log.		*/

	fflush(NULL);

	if (_coreFileNeeded(NULL))
	{
		sm_Abort();
	}

	return 1;
}

void	printStackTrace(void)
{
#if defined(HAVE_BACKTRACE) && defined(HAVE_BACKTRACE_SYMBOLS) \
		&& !defined(solaris)
#define MAX_TRACE_DEPTH 100
	void	*returnAddresses[MAX_TRACE_DEPTH];
	size_t	stackFrameCount;
	char	**functionNames;
	size_t	i;

	stackFrameCount = backtrace(returnAddresses, MAX_TRACE_DEPTH);
	functionNames = backtrace_symbols(returnAddresses, stackFrameCount);
	if (functionNames == NULL)
	{
		writeMemo("[!] Can't print backtrace function names.");
		return;
	}

	writeMemo("[i] Current stack trace:");
	for (i = 0; i < stackFrameCount; i++)
	{
		writeMemoNote("[i] ", functionNames[i]);
	}

	free(functionNames);
#undef	MAX_TRACE_DEPTH
#elif defined(solaris)
	/*	Solaris uses printstack() from <ucontext.h>.		*/
	writeMemo("[i] Current stack trace:");
	printstack(STDERR_FILENO);
#else
	writeMemo("[?] No stack trace available on this platform.");
#endif
}

void    debugPrint(const char *format, ...)
{
#if DEBUG_PRINT
	va_list     args;

	va_start(args, format);
#if DEBUG_PRINT_LOG
	char        buffer[256];

	vsnprintf(buffer, sizeof buffer, format, args);
	writeMemo(buffer);
#endif
	vprintf(format, args);
	putchar('\n');
	va_end(args);
#else
	/* When DEBUG_PRINT is disabled, tell the compiler we
	 * are not using 'format' */
	(void)format;
#endif
}

void	encodeSdnv(Sdnv *sdnv, uvast val)
{
	static uvast	sdnvMask = ((uvast) -1) / 128;
	uvast		remnant = val;
	char		result[10];
	int		length = 1;
	unsigned char	*text;

	/*	Thanks to Cheol Koo of KARI for optimizing this
	 *	function.  29 August 2019				*/

	CHKVOID(sdnv);

	/*	First extract the value of what will become the low-
	 *	order byte of the SDNV text; its high-order bit is 0.	*/

	result[0] = remnant & (uvast) 0x7f;
	remnant = (remnant >> 7) & sdnvMask;

	/*	Now extract the values of all remaining bytes, in
	 *	increasing order, setting high-order bit to 1 for
	 *	each one.  The results array will contain the values
	 *	of the bytes of the SDNV text in reverse order.		*/

	while (remnant)
	{
		result[length] = (remnant & (uvast) 0x7f) | 0x80;
		remnant = (remnant >> 7) & sdnvMask;
		length++;
	}

	/*	Now copy the extracted values into the text of the
	 *	SDNV, starting with the highest-order value.		*/

	sdnv->length = length;
	text = sdnv->text;
	while (length)
	{
		length--;
		*text = result[length];
		text++;
	}
}

int	decodeSdnv(uvast *val, unsigned char *sdnvTxt)
{
	int		sdnvLength = 0;
	unsigned char	*cursor;

	CHKZERO(val);
	CHKZERO(sdnvTxt);
	*val = 0;
	cursor = sdnvTxt;

	while (1)
	{
		sdnvLength++;
		if (sdnvLength > 10)
		{
			return 0;	/*	More than 70 bits.	*/
		}

		/*	Shift numeric value 7 bits to the left (that
		 *	is, multiply by 128) to make room for 7 bits
		 *	of SDNV byte value.				*/

		*val <<= 7;

		/*	Insert SDNV text byte value (with its high-
		 *	order bit masked off) as low-order 7 bits of
		 *	the numeric value.				*/

		*val |= (*cursor & 0x7f);

		/*	If this SDNV text byte's high-order bit is
		 *	1, then it's the last byte of the SDNV text.	*/

		if (((*cursor) & 0x80) == 0)	/*	Last SDNV byte.	*/
		{
			return sdnvLength;
		}

		/*	Haven't reached the end of the SDNV yet.	*/

		cursor++;
	}
}

size_t decodeSdnvBounded(uvast *val, unsigned char *sdnvTxt, size_t length)
{
	size_t	       sdnvLength = 0;
	unsigned char *cursor;

	CHKZERO(val);
	CHKZERO(sdnvTxt);
	*val = 0;
	cursor = sdnvTxt;

	/*
	 * Identical to decodeSdnv() except that the SDNV must be fully
	 * contained within the first "length" bytes of the buffer.  Callers
	 * parsing untrusted, length-delimited input (e.g. bytes received from
	 * a convergence layer) use this so a truncated SDNV cannot drive an
	 * out-of-bounds read past the end of the buffer, and so the returned
	 * length never exceeds the caller's remaining byte count. The return
	 * value is the SDNV length in bytes (1-10), or 0 if the SDNV is
	 * truncated within "length" bytes or runs longer than 70 bits.
	 */

	while (sdnvLength < length)
	{
		sdnvLength++;
		if (sdnvLength > 10)
		{
			return 0; /* More than 70 bits. */
		}

		*val <<= 7;
		*val |= (*cursor & 0x7f);
		if (((*cursor) & 0x80) == 0) /* Last SDNV byte. */
		{
			return sdnvLength;
		}

		cursor++;
	}

	return 0; /* Truncated within "length" bytes. */
}

void	loadScalar(Scalar *s, signed int i)
{
	CHKVOID(s);
	if (i < 0)
	{
		i = 0 - i;
	}

	s->gigs = 0;
	s->units = i;
	while (s->units >= ONE_GIG)
	{
		s->gigs++;
		s->units -= ONE_GIG;
	}
}

void	increaseScalar(Scalar *s, signed int i)
{
	CHKVOID(s);
	if (i < 0)
	{
		i = 0 - i;
	}

	while (i >= ONE_GIG)
	{
		i -= ONE_GIG;
		s->gigs++;
	}

	s->units += i;
	while (s->units >= ONE_GIG)
	{
		s->gigs++;
		s->units -= ONE_GIG;
	}
}

void	reduceScalar(Scalar *s, signed int i)
{
	CHKVOID(s);
	if (i < 0)
	{
		i = 0 - i;
	}

	while (i >= ONE_GIG)
	{
		i -= ONE_GIG;
		s->gigs--;
	}

	while (i > s->units)
	{
		s->units += ONE_GIG;
		s->gigs--;
	}

	s->units -= i;
}

void	multiplyScalar(Scalar *s, signed int i)
{
	double	product;

	CHKVOID(s);
	if (i < 0)
	{
		i = 0 - i;
	}

	product = ((((double)(s->gigs)) * ONE_GIG) + (s->units)) * i;
	s->gigs = (int) (product / ONE_GIG);
	s->units = (int) (product - (((double)(s->gigs)) * ONE_GIG));
}

void	divideScalar(Scalar *s, signed int i)
{
	double	quotient;

	CHKVOID(s);
	CHKVOID(i != 0);
	if (i < 0)
	{
		i = 0 - i;
	}

	quotient = ((((double)(s->gigs)) * ONE_GIG) + (s->units)) / i;
	s->gigs = (int) (quotient / ONE_GIG);
	s->units = (int) (quotient - (((double)(s->gigs)) * ONE_GIG));
}

void	copyScalar(Scalar *to, Scalar *from)
{
	CHKVOID(to);
	CHKVOID(from);
	to->gigs = from->gigs;
	to->units = from->units;
}

void	addToScalar(Scalar *s, Scalar *increment)
{
	CHKVOID(s);
	CHKVOID(increment);
	increaseScalar(s, increment->units);
	s->gigs += increment->gigs;
}

void	subtractFromScalar(Scalar *s, Scalar *decrement)
{
	CHKVOID(s);
	CHKVOID(decrement);
	reduceScalar(s, decrement->units);
	s->gigs -= decrement->gigs;
}

int	scalarIsValid(Scalar *s)
{
	CHKZERO(s);
	return (s->gigs >= 0);
}

void	scalarToSdnv(Sdnv *sdnv, Scalar *scalar)
{
	int		gigs;
	int		units;
	int		i;
	unsigned char	flag = 0;
	unsigned char	*cursor;

	CHKVOID(scalarIsValid(scalar));
	CHKVOID(sdnv);
	sdnv->length = 0;

	/*		Calculate sdnv length				*/

	gigs = scalar->gigs;
	units = scalar->units;
	if (gigs)
	{
		/*	The scalar is greater than 2^30 - 1, so start
		 *	with the length occupied by all 30 bits of
		 *	"units" in the scalar.  This will occupy 5
		 *	bytes in the sdnv with room for an additional
		 *	5 high-order bits.  These bits will be the
		 *	low-order 5 bits of gigs.  If the value in
		 *	gigs is greater than 2^5 -1, increase sdnv
		 *	length accordingly.				*/

		sdnv->length += 5;
		gigs >>= 5;
		while (gigs)
		{
			gigs >>= 7;
			sdnv->length++;
		}
	}
	else
	{
		/*	gigs = 0, so calculate the sdnv length from
			units only.					*/

		do
		{
			units >>= 7;
			sdnv->length++;
		} while (units);
	}

	/*		Fill the sdnv text.				*/

	cursor = sdnv->text + sdnv->length;
	i = sdnv->length;
	gigs = scalar->gigs;
	units = scalar->units;
	do
	{
		cursor--;

		/*	Start filling the sdnv text from the last byte.
			Get 7 low-order bits from units and add the
			flag to the high-order bit. Flag is 0 for the
			last byte and 1 for all the previous bytes.	*/

		*cursor = (units & 0x7f) | flag;
		units >>= 7;
		flag = 0x80;		/*	Flag is now 1.		*/
		i--;
	} while (units);

	if (gigs)
	{
		while (sdnv->length - i < 5)
		{
			cursor--;

			/* Fill remaining sdnv bytes corresponding to
			   units with zeroes.				*/

			*cursor = 0x00 | flag;
			i--;
		}

		/*	Place the 5 low-order bits of gigs in the
			current	sdnv byte.				*/

		*cursor |= ((gigs & 0x1f) << 2);
		gigs >>= 5;
		while (i)
		{
			cursor--;

			/*	Now fill the remaining sdnv bytes
				from gigs.				*/

			*cursor = (gigs & 0x7f) | flag;
			gigs >>= 7;
			i--;
		}
	}
}

int	sdnvToScalar(Scalar *scalar, unsigned char *sdnvText)
{
	int		sdnvLength;
	int		i;
	int		numSize = 0; /* Size of stored number in bits.	*/
	unsigned char	*cursor;
	unsigned char	flag;
	unsigned char	k;

	CHKZERO(scalar);
	CHKZERO(sdnvText);
	cursor = sdnvText;

	/*	Find out the sdnv length and size of stored number,
	 *	stripping off all leading zeroes.			*/

	flag = (*cursor & 0x80);/*	Get flag of 1st byte.		*/
	k = *cursor << 1;	/*	Discard the flag bit.		*/
	i = 7;
	while (i)
	{
		if (k & 0x80)
		{
			break;	/*	Loop until a '1' is found.	*/
		}

		i--;
		k <<= 1;
	}

	numSize += i;	/*	Add significant bits from first byte.	*/
	if (flag)	/*	Not end of SDNV.			*/
	{
		/*	Sdnv has more than one byte.  Add 7 bits for
		 *	the last byte and advance cursor to add the
		 *	bits for all intermediate bytes.		*/

		numSize += 7;
		cursor++;
		while (*cursor & 0x80)
		{
			numSize += 7;
			cursor++;
		}
	}

	if (numSize > 61)
	{
		return 0;	/*	Too long to fit in a Scalar.	*/
	}

	sdnvLength = (cursor - sdnvText) + 1;

	/*		Now start filling gigs and units.		*/

	scalar->gigs = 0;
	scalar->units = 0;
	cursor = sdnvText;
	i = sdnvLength;

	while (i > 5)
	{	/*	Sdnv bytes containing gigs only.		*/

		scalar->gigs <<= 7;
		scalar->gigs |= (*cursor & 0x7f);
		cursor++;
		i--;
	}

	if (i == 5)
	{	/* Sdnv byte containing units and possibly gigs too.	*/

		if (numSize > 30)
		{
			/* Fill the gigs bits after shifting out
			   the 2 bits that belong to units.		*/

			scalar->gigs <<= 5;
			scalar->gigs |= ((*cursor >> 2) & 0x1f);
		}

		/*		Fill the units bits.			*/

		scalar->units = (*cursor & 0x03);
		cursor++;
		i--;
	}

	while (i)
	{	/*	Sdnv bytes containing units only.		*/

		scalar->units <<= 7;
		scalar->units |= (*cursor & 0x7f);
		cursor++;
		i--;
	}

	return sdnvLength;
}

uvast	htonv(uvast hostvast)
{
	static const int	fortyTwo = 42;

	if ((*(const char *) &fortyTwo) == 0)	/*	Check first byte.	*/
	{
		/*	Small-endian (network byte order) machine.	*/

		return hostvast;
	}

	/*	Must  reverse the byte order of this number.		*/

#if (!LONG_LONG_OKAY)
	return htonl(hostvast);
#else
	static const vast	mask = 0xffffffff;
	unsigned int		big_part;
	unsigned int		small_part;
	uvast			result;

	big_part = hostvast >> 32;
	small_part = hostvast & mask;
	big_part = htonl(big_part);
	small_part = htonl(small_part);
	result = small_part;
	return (result << 32) | big_part;
#endif
}

uvast	ntohv(uvast netvast)
{
	return htonv(netvast);
}

int	fullyQualified(char *fileName)
{
	CHKZERO(fileName);

#if (defined(VXWORKS))
	if (strncmp("host:", fileName, 5) == 0)
	{
		fileName += 5;
	}

	if (isalpha((int)*fileName) && *(fileName + 1) == ':')
	{
		return 1;
	}

	if (*fileName == '/')
	{
		return 1;
	}

	return 0;

#elif defined(DOS_PATH_DELIMITER)
	if (isalpha(*fileName) && *(fileName + 1) == ':')
	{
		return 1;
	}

	return 0;
#else
	if (*fileName == '/')
	{
		return 1;
	}

	return 0;
#endif
}

int	qualifyFileName(char *fileName, char *buffer, int buflen)
{
	char	pathDelimiter = ION_PATH_DELIMITER;
	int	nameLen;
	int	cwdLen;

	CHKERR(fileName);
	CHKERR(buffer);
	CHKERR(buflen> 0);
	nameLen = strlen(fileName);
	if (fullyQualified(fileName))
	{
		if (nameLen < buflen)
		{
			istrcpy(buffer, fileName, buflen);
			return 0;
		}

		writeMemoNote("[?] File name is too long for qual. buffer.",
				fileName);
		return -1;
	}

	/*	This is a relative path name; must insert cwd.		*/

	if (igetcwd(buffer, buflen) == NULL)
	{
		putErrmsg("Can't get cwd.", NULL);
		return -1;
	}

	cwdLen = strlen(buffer);
	if ((cwdLen + 1 + nameLen + 1) > buflen)
	{
		writeMemoNote("Qualified file name would be too long.",
				fileName);
		return -1;
	}

	*(buffer + cwdLen) = pathDelimiter;
	cwdLen++;		/*	cwdname including delimiter	*/
	istrcpy(buffer + cwdLen, fileName, buflen - cwdLen);
	return 0;
}

void	findToken(char **cursorPtr, char **token)
{
	char	*cursor;

	if (token == NULL)
	{
		ABORT_AS_REQD;
		return;
	}

	*token = NULL;		/*	The default.			*/
	if (cursorPtr == NULL || (*cursorPtr) == NULL)
	{
		ABORT_AS_REQD;
		return;
	}

	cursor = *cursorPtr;

	/*	Skip over any leading whitespace.			*/

	while (isspace((unsigned char) *cursor))
	{
		cursor++;
	}

	if (*cursor == '\0')	/*	Nothing but whitespace.		*/
	{
		*cursorPtr = cursor;
		return;
	}

	/*	Token delimited by quotes or braces {} is the complicated case.	*/

	if ((*cursor == '\'') || (*cursor == '{'))	/*	Quote-delimited token. */
	{
		/*	Token is everything after this single quote,
		 *	up to (but not including) the next non-escaped
		 *	single quote.					*/

		/*  Or, token is everything after this open brace '{',
		 *  up to (but not including) the closing brace '}'.    */

		cursor++;
		while (*cursor != '\0')
		{
			if (*token == NULL)
			{
				*token = cursor;
			}

			if (*cursor == '\\')	/*	Escape.		*/
			{
				/*	Include the escape character
				 *	plus the following (escaped)
				 *	character (unless it's the end
				 *	of the string) in the token.	*/

				cursor++;
				if (*cursor == '\0')
				{
					*cursorPtr = cursor;
					return;	/*	unmatched quote	*/
				}

				cursor++;
				continue;
			}

			if (*cursor == '\'')	/*	End of token.	*/
			{
				*cursor = '\0';
				cursor++;
				*cursorPtr = cursor;
				return;		/*	matched quote	*/
			}

			if (*cursor == '}')	/*	End of token.	*/
			{
				*cursor = '\0';
				cursor++;
				*cursorPtr = cursor;
				return;		/*	closing brace found	*/
			}

			cursor++;
		}

		/*	If we get here it's another case of unmatched
		 *	quote, but okay.				*/

		*cursorPtr = cursor;
		return;
	}

	/*	The normal case: a simple whitespace-delimited token.
	 *	Token is this character and all successive characters
	 *	up to (but not including) the next whitespace.		*/

	*token = cursor;
	cursor++;
	while (*cursor != '\0')
	{
		if (isspace((unsigned char) *cursor))	/*	End of token.	*/
		{
			*cursor = '\0';
			cursor++;
			break;
		}

		cursor++;
	}

	*cursorPtr = cursor;
}

/*
 * Parses a string into a uvast with strict POSIX validation.
 * Rejects negative inputs, catches overflow, and rejects trailing garbage.
 * Returns 0 on success, -1 on failure.
 */
int platform_parse_uvast(const char *nptr, uvast *result)
{
	const char *s = nptr;
	char *endptr;
	uvast temp_val;

	if (s == NULL || *s == '\0')
	{
		return -1;
	}

	while (isspace((unsigned char)*s))
	{
		s++;
	}

	/* Reject negative numbers for unsigned parsing */
	if (*s == '-')
	{
		return -1;
	}

	errno = 0;

	#if (!LONG_LONG_OKAY) || (SPACE_ORDER >= 3)
	temp_val = (uvast) strtoul(s, &endptr, 0);
	#else
	temp_val = (uvast) strtoull(s, &endptr, 0);
	#endif

	/* Defensive check: do not modify *result on any error path */
	if (endptr == s || errno == ERANGE || *endptr != '\0')
	{
		return -1;
	}

	*result = temp_val;
	return 0;
}

/*
 * Parses a string into a signed integer with strict POSIX validation.
 * Enforces architecture-specific integer boundaries (INT_MIN to INT_MAX),
 * catches overflow/underflow, and rejects trailing garbage.
 * Returns 0 on success, -1 on failure.
 */
int platform_parse_int(const char *nptr, int *result)
{
	const char *s = nptr;
	char *endptr;
	long temp_val;

	if (s == NULL || *s == '\0')
	{
		return -1;
	}

	while (isspace((unsigned char)*s))
	{
		s++;
	}

	errno = 0;
	temp_val = strtol(s, &endptr, 0);

	/* Defensive check: do not modify *result on any error path */
	if (endptr == s || errno == ERANGE || *endptr != '\0')
	{
		return -1;
	}

	/* Enforce strict 32-bit bounds before assignment */
	if (temp_val < INT_MIN || temp_val > INT_MAX)
	{
		return -1;
	}

	*result = (int)temp_val;
	return 0;
}

/*
 * platform_parse_double
 * Safely parses a string into a double, catching overflows, underflows,
 * NaN, Infinity, and invalid trailing characters.
 * Returns 0 on success, -1 on failure.
 */
int platform_parse_double(const char *str, double *result)
{
	char	*endptr;
	double	temp_val;

	if (str == NULL || result == NULL)
	{
		return -1;
	}

	/* Skip leading whitespace to ensure strict trailing character checks work */
	while (isspace((unsigned char)*str))
	{
		str++;
	}

	if (*str == '\0')
	{
		return -1; /* Empty string */
	}

	/* Reset errno before the call to accurately detect ERANGE */
	errno = 0;
	temp_val = strtod(str, &endptr);

	/* Check for fundamental parsing failure (no digits found) */
	if (str == endptr)
	{
		return -1;
	}

	/* Check for overflow (HUGE_VAL/-HUGE_VAL) or underflow (0.0) */
	if (errno == ERANGE)
	{
		return -1;
	}

	/* Explicitly reject NaN and Infinity if they aren't valid CLI states */
	if (isnan(temp_val) || isinf(temp_val))
	{
		return -1;
	}

	/* Ensure there is no trailing garbage (ignoring trailing whitespace) */
	while (isspace((unsigned char)*endptr))
	{
		endptr++;
	}

	if (*endptr != '\0')
	{
		return -1; /* Invalid characters left over (e.g., "3.14abc") */
	}

	*result = temp_val;
	return 0;
}

#ifdef ION_NO_DNS
unsigned int	getAddressOfHost()
{
	return 0;
}

char	*addressToString(struct in_addr address, char *buffer)
{
	CHKNULL(buffer);

	*buffer = 0;
	putErrmsg("Can't convert IP address to string.", NULL);
	return buffer;
}

#else

unsigned int	getAddressOfHost(void)
{
	char	hostnameBuf[MAXHOSTNAMELEN + 1];
	getNameOfHost(hostnameBuf, sizeof(hostnameBuf));
	unsigned int inetaddr = getInternetAddress(hostnameBuf);
	if(inetaddr == 0){
			putErrmsg("Couldn't look up own IP, defaulting to 127.0.0.1", NULL);
			inetaddr = ntohl(0x100007F);
	}
	return inetaddr;

}

char	*addressToString(struct in_addr address, char *buffer)
{
	char	*result;

	CHKNULL(buffer);
	*buffer = 0;
#if defined (VXWORKS)
	inet_ntoa_b(address, buffer);
#else
	result = inet_ntoa(address);
	if (result == NULL)
	{
		putSysErrmsg("inet_ntoa() returned NULL", NULL);
	}
	else
	{
		istrcpy(buffer, result, 16);
	}
#endif
	return buffer;
}
#endif	/*	ION_NO_DNS						*/

#if (defined(FSWLAN) || !(defined(ION_NO_DNS)))
int parseSocketSpec(char *socketSpec, unsigned short *portNbr,
	unsigned int *ipAddress)
{
	char		*delimiter;
	char		*hostname;
	char		hostnameBuf[MAXHOSTNAMELEN + 1];
	unsigned int	i4;
	int		portValid = 0;
	int		ipValid = 0;

	CHKERR(portNbr);
	CHKERR(ipAddress);
	*portNbr = 0;			/*	Use default port nbr.	*/
	*ipAddress = INADDR_ANY;	/*	Use local host address.	*/

	if (socketSpec == NULL || *socketSpec == '\0')
	{
		writeMemoNote("[?] parseSocketSpec: Empty or NULL socketSpec", socketSpec);
		return -1;		/*	Error: invalid input.	*/
	}

	/*	Parse port number first, so it's set even if DNS fails.	*/

	delimiter = strchr(socketSpec, ':');
	if (delimiter)
	{
		*delimiter = '\0';	/*	Delimit host name.	*/
		hostname = socketSpec;	/*	Hostname without port.	*/
		i4 = atoi(delimiter + 1);	/*	Get port number.	*/
		if (i4 == 0)
		{
			writeMemoNote("[?] parseSocketSpec: Non-numeric or missing port", socketSpec);
		}
		else if (i4 < 1024 || i4 > 65535)
		{
			writeMemoNote("[?] parseSocketSpec: Invalid port number", utoa(i4));
		}
		else
		{
			*portNbr = (unsigned short) i4;
			portValid = 1;
		}
	}
	else
	{
		hostname = socketSpec;	/*	No port, use full string.	*/
		writeMemoNote("[?] parseSocketSpec: No port specified", socketSpec);
	}

	/*	Now figure out the IP address.  @ is local host.	*/

	if (strlen(hostname) != 0)
	{
		if (strcmp(hostname, "0.0.0.0") == 0)
		{
			*ipAddress = INADDR_ANY;
			ipValid = 1;
		}
		else if (strcmp(hostname, "@") == 0)
		{
			if (getNameOfHost(hostnameBuf, sizeof hostnameBuf) < 0)
			{
				writeMemoNote("[?] parseSocketSpec: Can't get local hostname", NULL);
				*ipAddress = BAD_HOST_NAME;
			}
			else
			{
				hostname = hostnameBuf;
				i4 = getInternetAddress(hostname);
				if (i4 < 1)	/*	Invalid hostname.	*/
				{
					writeMemoNote("[?] parseSocketSpec: Can't get IP address", hostname);
					*ipAddress = BAD_HOST_NAME;
				}
				else
				{
					*ipAddress = i4;
					ipValid = 1;
				}
			}
		}
		else
		{
			i4 = getInternetAddress(hostname);
			if (i4 < 1)	/*	Invalid hostname.	*/
			{
				writeMemoNote("[?] parseSocketSpec: Can't get IP address", hostname);
				*ipAddress = BAD_HOST_NAME;
			}
			else
			{
				*ipAddress = i4;
				ipValid = 1;
			}
		}
	}

	/*	Restore socketSpec for logging and caller.	*/
	if (delimiter)
	{
		*delimiter = ':';
	}

	/*	Return -1 if either port or IP parsing failed.	*/
	if (!portValid || !ipValid)
	{
		return -1;
	}

	writeMemoNote("[i] parseSocketSpec: Parsed", socketSpec);
	return 0;
}
#else
int	parseSocketSpec(char *socketSpec, unsigned short *portNbr,
		unsigned int *ipAddress)
{
	return 0;
}
#endif	/*	defined(FSWLAN || !(defined(ION_NO_DNS)))		*/

void	printDottedString(unsigned int hostNbr, char *buffer)
{
	CHKVOID(buffer);
	isprintf(buffer, 16, "%u.%u.%u.%u", (hostNbr >> 24) & 0xff,
		(hostNbr >> 16) & 0xff, (hostNbr >> 8) & 0xff, hostNbr & 0xff);
}

/*	Portable implementation of a safe snprintf: always NULL-
 *	terminates the content of the string composition buffer.	*/

#define SN_FMT_SIZE		64

/*	Flag array indices	*/
#define	SN_LEFT_JUST		0
#define	SN_SIGNED		1
#define	SN_SPACE_PREFIX		2
#define	SN_PAD_ZERO		3
#define	SN_ALT_OUTPUT		4

int _isprintf(const char *file, int line, char *buffer, int bufSize, const char *format, ...)
{
	va_list args;
	int ret;

	if (buffer == NULL || bufSize < 1)
	{
		ABORT_AS_REQD;
		return 0;
	}

	if (format == NULL)
	{
		ABORT_AS_REQD;
		if (bufSize < 2)
		{
			*buffer = '\0';
		}
		else
		{
			*buffer = '?';
			*(buffer + 1) = '\0';
		}

		return 0;
	}

	/*
	 * Delegate variadic argument extraction and string formatting to
	 * the C99 standard library.
	 */
	va_start(args, format);
	ret = vsnprintf(buffer, (size_t)bufSize, format, args);
	va_end(args);

	/*
	 * Fulfill platform(3) man page promise: log on overrun or error.
	 * We bypass putErrmsg completely to prevent infinite recursion,
	 * sending a highly constrained, safe diagnostic string directly
	 * to the registered logger.
	 */
	if (ret < 0 || ret >= bufSize)
	{
		char diagBuf[64];

		if (ret < 0)
		{
			snprintf(diagBuf, sizeof(diagBuf),
				"[?] isprintf encoding error in %s:%d.", file, line);
		}
		else
		{
			snprintf(diagBuf, sizeof(diagBuf),
				"[?] isprintf overrun in %s:%d (lim %d, req %d).",
				file, line, bufSize, ret);
		}

		writeMemo(diagBuf);

		/*
		 * Protect caller pointer math. If vsnprintf fails with an
		 * encoding error, buffer contents are indeterminate. We
		 * terminate the string safely and return 0 to prevent
		 * upstream out-of-bounds strlen calls.
		 */
		if (ret < 0)
		{
			buffer[0] = '\0';
			return 0;
		}
	}

	/*
	 * Return the length that would have been written, matching both
	 * the C99 standard and the legacy function behavior.
	 */
	return ret;
}

/*	*	*	Other portability adaptations	*	*	*/

size_t	istrlen(const char *from, size_t maxlen)
{
	size_t	length;
	const char	*cursor;

	if (from == NULL)
	{
		ABORT_AS_REQD;
		return 0;
	}

	length = 0;
	if (maxlen > 0)
	{
		for (cursor = from; *cursor; cursor++)
		{
			length++;
			if (length == maxlen)
			{
				break;
			}
		}
	}

	return length;
}

char	*istrcpy(char *buffer, const char *from, size_t bufSize)
{
	int	maxText;
	int	copySize;

	if (buffer == NULL || from == NULL || bufSize < 1)
	{
		ABORT_AS_REQD;
		return NULL;
	}

	maxText = bufSize - 1;
	copySize = istrlen(from, maxText);
	memcpy(buffer, from, copySize);
	*(buffer + copySize) = '\0';
	return buffer;
}

char	*istrcat(char *buffer, char *from, size_t bufSize)
{
	int	maxText;
	int	currTextSize;
	int	maxCopy;
	int	copySize;

	if (buffer == NULL || from == NULL || bufSize < 1)
	{
		ABORT_AS_REQD;
		return NULL;
	}

	maxText = bufSize - 1;
	currTextSize = istrlen(buffer, maxText);
	maxCopy = maxText - currTextSize;
	copySize = istrlen(from, maxCopy);
	memcpy(buffer + currTextSize, from, copySize);
	*(buffer + currTextSize + copySize) = '\0';
	return buffer;
}

char	*igetcwd(char *buf, size_t size)
{
#ifdef FSWWDNAME
#include "wdname.c"
#else
	char	*cwdName;

	CHKNULL(buf);
	CHKNULL(size > 0);
	cwdName = getcwd(buf, size);
	if (cwdName == NULL)
	{
		putSysErrmsg("Can't get CWD name", sizetoa(size));
	}

	return cwdName;
#endif
}

#ifdef POSIX_TASKS

#ifndef SIGNAL_RULE_CT
#define SIGNAL_RULE_CT	100
#endif

typedef struct
{
	int		declared;	/*	Boolean.		*/
	pthread_t	tid;
	int		signbr;
	SignalHandler	handler;
} SignalRule;

static SignalHandler	_signalRules(int signbr, SignalHandler handler)
{
	static SignalRule	rules[SIGNAL_RULE_CT];
	static int		rulesInitialized = 0;
	int			i;
	pthread_t		tid = sm_TaskIdSelf();
	SignalRule		*rule;

	if (!rulesInitialized)
	{
		memset((char *) rules, 0, sizeof rules);
		rulesInitialized = 1;
	}

	if (handler)	/*	Declaring a new signal rule.		*/
	{
		/*	We take this as an opportunity to clear out any
		 *	existing rules that are no longer needed, due to
		 *	termination of the threads that declared them.	*/

		for (i = 0, rule = rules; i < SIGNAL_RULE_CT; i++, rule++)
		{
			if (rule->declared == 0)	/*	Clear.	*/
			{
				if (handler == NULL)	/*	Noted.	*/
				{
					continue;
				}

				/*	Declare new signal rule here.	*/

				rule->declared = 1;
				rule->tid = tid;
				rule->signbr = signbr;
				rule->handler = handler;
				handler = NULL;		/*	Noted.	*/
				continue;
			}

			/*	This is a declared signal rule.		*/

			if (pthread_equal(rule->tid, tid))
			{
				/*	One of thread's own rules.	*/

				if (rule->signbr != signbr)
				{
					continue;	/*	Okay.	*/
				}

				/*	New handler for tid/signbr.	*/

				if (handler)	/*	Not noted yet.	*/
				{
					rule->handler = handler;
					handler = NULL;	/*	Noted.	*/
				}
				else	/*	Noted in another rule.	*/
				{
					rule->declared = 0;
				}

				continue;
			}

			/*	Signal rule for another thread.		*/

			if (!sm_TaskExists(rule->tid))
			{
				/*	Obsolete rule; thread is gone.	*/

				rule->declared = 0;	/*	Clear.	*/
			}
		}

		return NULL;
	}

	/*	Just looking up applicable signal rule for tid/signbr.	*/

	for (i = 0, rule = rules; i < SIGNAL_RULE_CT; i++, rule++)
	{
		if (pthread_equal(rule->tid, tid) && rule->signbr == signbr)
		{
			return rule->handler;
		}
	}

	return NULL;	/*	No applicable signal rule.		*/
}

static void	threadSignalHandler(int signbr)
{
	SignalHandler	handler = _signalRules(signbr, NULL);

	if (handler)
	{
		handler(signbr);
	}
}
#endif	/*	end of #ifdef POSIX_TASKS				*/

void	isignal(int signbr, void (*handler)(int))
{
	struct sigaction	action;
#ifdef POSIX_TASKS
	sigset_t		signals;

	oK(sigemptyset(&signals));
	oK(sigaddset(&signals, signbr));
	oK(pthread_sigmask(SIG_UNBLOCK, &signals, NULL));
	oK(_signalRules(signbr, handler));
	handler = threadSignalHandler;
#endif	/*	end of #ifdef POSIX_TASKS				*/
	memset((char *) &action, 0, sizeof(struct sigaction));
	action.sa_handler = handler;
	oK(sigaction(signbr, &action, NULL));
#ifdef freebsd
	oK(siginterrupt(signbr, 1));
#endif
}

void	iblock(int signbr)
{
	sigset_t	signals;

	oK(sigemptyset(&signals));
	oK(sigaddset(&signals, signbr));
	oK(pthread_sigmask(SIG_BLOCK, &signals, NULL));
}

int	ifopen(const char *fileName, int flags, int pmode)
{
	int		fd;
	struct stat	statbuf;

	fd = iopen(fileName, flags, pmode);
	if (fd < 0)
	{
		putSysErrmsg("Open failed.", fileName);
		return -1;
	}

	if (fstat(fd, &statbuf) < 0)
	{
		close(fd);
		putSysErrmsg("Can't stat file.", fileName);
		return -1;
	}

	if (S_ISREG(statbuf.st_mode))
	{
		return fd;
	}

	close(fd);
	putErrmsg("Not a regular file.", fileName);
	return -1;
}

char	*igets(int fd, char *buffer, int buflen, int *lineLen)
{
	char	*cursor = buffer;
	int	maxLine = buflen - 1;
	int	len;

	if (buffer == NULL)
	{
		ABORT_AS_REQD;
		putErrmsg("No buffer passed to igets().", NULL);
		return NULL;
	}

	*buffer = '\0';				/*	Default.	*/
	if (fd < 0 || buflen < 1 || lineLen == NULL)
	{
		ABORT_AS_REQD;
		putErrmsg("Invalid argument(s) passed to igets().", NULL);
		return NULL;
	}

	len = 0;
	while (1)
	{
		switch (read(fd, cursor, 1))
		{
		case 0:		/*	End of file; also end of line.	*/
			if (len == 0)		/*	Nothing more.	*/
			{
				*(buffer + len) = '\0';
				*lineLen = len;
				return NULL;	/*	Indicate EOF.	*/
			}

			/*	End of last line.			*/

			break;			/*	Out of switch.	*/

		case -1:
			if (errno == EINTR)	/*	Treat as EOF.	*/
			{
				*(buffer + len) = '\0';
				*lineLen = 0;
				return NULL;
			}

			putSysErrmsg("Failed reading line", itoa(len));
			*(buffer + len) = '\0';
			*lineLen = -1;
			return NULL;

		default:
			if (*cursor == 0x0a)		/*	LF (nl)	*/
			{
				/*	Have reached end of line.	*/

				if (len > 0
				&& *(buffer + (len - 1)) == 0x0d)
				{
					len--;		/*	Lose CR	*/
				}

				break;		/*	Out of switch.	*/
			}

			/*	Have not reached end of line yet.	*/

			if (len == maxLine)	/*	Must truncate.	*/
			{
				break;		/*	Out of switch.	*/
			}

			/*	Okay, include this char in the line...	*/

			len++;

			/*	...and read the next character.		*/

			cursor++;
			continue;
		}

		break;				/*	Out of loop.	*/
	}

	*(buffer + len) = '\0';
	*lineLen = len;
	return buffer;
}

int	iputs(int fd, char *string)
{
	int	totalBytesWritten = 0;
	int	length;
	int	bytesWritten;

	if (fd < 0 || string == NULL)
	{
		ABORT_AS_REQD;
		putErrmsg("Invalid argument(s) passed to iputs().", NULL);
		return -1;
	}

	length = strlen(string);
	while (totalBytesWritten < length)
	{
		bytesWritten = write(fd, string + totalBytesWritten,
				length - totalBytesWritten);
		if (bytesWritten < 0)
		{
			putSysErrmsg("Failed writing line",
					itoa(totalBytesWritten));
			return -1;
		}

		totalBytesWritten += bytesWritten;
	}

	return totalBytesWritten;
}

/*	*	*	Standard TCP functions	*	*	*	*/

void	itcp_handleConnectionLoss(int signum)
{
	/* Parameter intentionally unused. */
	(void)signum;
	isignal(SIGPIPE, itcp_handleConnectionLoss);
}

int	itcp_connect(char *socketSpec, unsigned short defaultPort, int *sock)
{
	unsigned short		portNbr;
	unsigned int		hostNbr;
	struct sockaddr		socketName;
	struct sockaddr_in	*inetName;
	char			dottedString[16];
	char			socketTag[32];
	static int iciTcpConnectionOK = 1;

	CHKERR(socketSpec);
	CHKERR(sock);
	*sock = -1;		/*	Default value.			*/
	if (*socketSpec == '\0')
	{
		return 0;	/*	Don't try to connect.		*/
	}

	/*	Construct socket name.					*/

	parseSocketSpec(socketSpec, &portNbr, &hostNbr);
	if (hostNbr == 0)
	{
		putErrmsg("Can't get IP address for host.", socketSpec);
		return 0;
	}

	if (portNbr == 0)
	{
		portNbr = defaultPort;
	}

	printDottedString(hostNbr, dottedString);
	isprintf(socketTag, sizeof socketTag, "%s:%hu", dottedString, portNbr);
	hostNbr = htonl(hostNbr);
	portNbr = htons(portNbr);
	memset((char *) &socketName, 0, sizeof socketName);
	inetName = (struct sockaddr_in *)(void *) &socketName;
	inetName->sin_family = AF_INET;
	inetName->sin_port = portNbr;
	memcpy((char *) &(inetName->sin_addr.s_addr), (char *) &hostNbr, 4);
	*sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (*sock < 0)
	{
		putSysErrmsg("Can't open TCP socket", socketTag);
		return -1;
	}

#if defined (TCPCL_LOW_CYCLE)
	/* set lower SYN retries */
	int syncnt = 1;
	int syncnt_sz = sizeof(syncnt);
	setsockopt(*sock, IPPROTO_TCP, TCP_SYNCNT, &syncnt, syncnt_sz);
#endif

	if (connect(*sock, &socketName, sizeof(struct sockaddr)) < 0)
	{
		if (errno == ECONNREFUSED)
		{
			if (iciTcpConnectionOK == 1){
				writeMemoNote("[i] Can't connect to TCP socket \
(refused)", socketTag);
				iciTcpConnectionOK = 0;
			}
		}
		else
		{
			if (iciTcpConnectionOK == 1){
				putSysErrmsg("Can't connect to TCP socket", socketTag);
				iciTcpConnectionOK = 0;
			}

		}

		closesocket(*sock);
		*sock = -1;
		return 0;
	}

	iciTcpConnectionOK = 1;
	writeMemoNote("[i] Connected to TCP socket", socketTag);
	return 1;	/*	Connected to remote socket.		*/
}

/*	Dual-stack (IPv4/IPv6) version of itcp_connect using ion_network framework */
int	itcp_connect_dualstack(char *socketSpec, unsigned short defaultPort,
		int *sock, IonNetworkAddress *remoteAddr)
{
	IonEndpointSpec		spec;
	IonNetworkAddress	resolved_addr;
	char			addrStr[INET6_ADDR_WITH_PORT_STRLEN];
	static int		iciTcpConnectionOK = 1;

	CHKERR(socketSpec);
	CHKERR(sock);
	*sock = -1;		/*	Default value.			*/
	if (*socketSpec == '\0')
	{
		return 0;	/*	Don't try to connect.		*/
	}

	/*	Parse endpoint specification				*/
	if (parseNetworkEndpoint(socketSpec, &spec) < 0)
	{
		putErrmsg("Can't parse socket specification", socketSpec);
		return -1;
	}

	/*	Apply default port if none specified			*/
	if (spec.port == 0)
	{
		spec.port = defaultPort;
		snprintf(spec.service, sizeof(spec.service), "%hu", defaultPort);
	}

	/*	Resolve address using dual-stack resolver		*/
	if (resolveNetworkAddressTCP(&spec, &resolved_addr) < 0)
	{
		putErrmsg("Can't resolve TCP address", socketSpec);
		return -1;
	}

	/*	Create socket (family automatically determined)		*/
	*sock = socket(resolved_addr.family, SOCK_STREAM, IPPROTO_TCP);
	if (*sock < 0)
	{
		formatNetworkAddress(&resolved_addr, addrStr, sizeof(addrStr));
		putSysErrmsg("Can't open TCP socket", addrStr);
		return -1;
	}

#if defined (TCPCL_LOW_CYCLE)
	/*	Set lower SYN retries					*/
	int syncnt = 1;
	int syncnt_sz = sizeof(syncnt);
	setsockopt(*sock, IPPROTO_TCP, TCP_SYNCNT, &syncnt, syncnt_sz);
#endif

	/*	Connect to remote socket				*/
	if (connect(*sock, (struct sockaddr *)&resolved_addr.addr,
			resolved_addr.addr_len) < 0)
	{
		formatNetworkAddress(&resolved_addr, addrStr, sizeof(addrStr));
		if (errno == ECONNREFUSED)
		{
			if (iciTcpConnectionOK == 1)
			{
				writeMemoNote("[i] Can't connect to TCP socket (refused)",
						addrStr);
				iciTcpConnectionOK = 0;
			}
		}
		else
		{
			if (iciTcpConnectionOK == 1)
			{
				putSysErrmsg("Can't connect to TCP socket", addrStr);
				iciTcpConnectionOK = 0;
			}
		}

		closesocket(*sock);
		*sock = -1;
		return 0;
	}

	/*	Store remote address if requested			*/
	if (remoteAddr != NULL)
	{
		*remoteAddr = resolved_addr;
	}

	iciTcpConnectionOK = 1;
	formatNetworkAddress(&resolved_addr, addrStr, sizeof(addrStr));
	writeMemoNote("[i] Connected to TCP socket", addrStr);
	return 1;	/*	Connected to remote socket.		*/
}

static int	itcpSendBytes(int *sock, char *from, int length)
{
	int	bytesWritten;

	/*	This is a single transmission.  It's in a loop only
	 *	so that we can deal with interruptions.			*/

	while (1)	/*	Continue until not interrupted.		*/
	{
		if (*sock == -1)	/*	Socket has been closed.	*/
		{
			return 0;
		}

		bytesWritten = isend(*sock, from, length, 0);
		if (bytesWritten < 0)
		{
			switch (errno)
			{
			case EINTR:	/*	Interrupted; retry.	*/
				continue;

			case EPIPE:	/*	Lost connection.	*/
			case EBADF:
			case ETIMEDOUT:
			case ECONNRESET:
			case EHOSTUNREACH:
				bytesWritten = 0;
			}

			putSysErrmsg("isend error on TCP socket", itoa(*sock));
		}

		return bytesWritten;
	}
}

int	itcp_send(int *sock, char *from, int length)
{
	int	totalBytesSent = 0;
	int	bytesToSend = length;
	int	bytesSent;

	CHKERR(sock);
	CHKERR(from);
	CHKERR(length > 0);

	/*	It's valid for TCP to accept for transmission only
	 *	a subset of the data presented, so we have to loop
	 *	until the entire buffer has been transmitted.		*/

	while (bytesToSend > 0)
	{
		bytesSent = itcpSendBytes(sock, from, bytesToSend);
		switch (bytesSent)
		{
		case -1:	/*	Big problem; shut down.		*/
			return -1;

		case 0:		/*	Connection closed.		*/
			return 0;

		default:
			totalBytesSent += bytesSent;
			from += bytesSent;
			bytesToSend -= bytesSent;
		}
	}

	return totalBytesSent;
}

static const char	*errnoName(int errnum)
{
	switch (errnum)
	{
	case EINTR:		return "EINTR";
	case EBADF:		return "EBADF";
	case ECONNRESET:	return "ECONNRESET";
	case ETIMEDOUT:		return "ETIMEDOUT";
	case ECONNREFUSED:	return "ECONNREFUSED";
	case EHOSTUNREACH:	return "EHOSTUNREACH";
	case ENETUNREACH:	return "ENETUNREACH";
	case EPIPE:		return "EPIPE";
	case ENOTCONN:		return "ENOTCONN";
	case ENOTSOCK:		return "ENOTSOCK";
	default:		return itoa(errnum);
	}
}

int	itcp_recv(int *sock, char *into, int length)
{
	int	totalBytesReceived = 0;
	int	bytesToRecv = length;
	int	bytesRead;

	CHKERR(sock);
	CHKERR(into);
	CHKERR(length > 0);

	/*	It's valid for TCP to deliver on demand only a
	 *	subset of the data received, so we have to loop
	 *	until the entire buffer has been acquired.		*/

	while (bytesToRecv > 0)
	{
		if (*sock == -1)	/*	Socket has been closed.	*/
		{
			return 0;
		}

		bytesRead = irecv(*sock, into, bytesToRecv, 0);
		switch (bytesRead)
		{
		case -1:
			switch (errno)
			{
			/*	The recv() call may have been
			 *	interrupted by arrival of SIGTERM,
			 *	in which case reception should simply
			 *	report that it's time to shut down.	*/
			case EINTR:		/*	Shutdown.	*/
			case EBADF:
			case ECONNRESET:
				bytesRead = 0;

			/* FALLTHROUGH */

			default:
				putSysErrmsg("irecv() error on TCP socket",
						errnoName(errno));
				return bytesRead;
			}

		case 0:			/*	Connection closed.	*/
			return 0;

		default:
			totalBytesReceived += bytesRead;
			into += bytesRead;
			bytesToRecv -= bytesRead;
		}
	}

	return totalBytesReceived;
}
