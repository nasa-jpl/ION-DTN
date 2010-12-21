/*

	platform.h:	platform-dependent porting adaptations.

	Copyright (c) 2001, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
									*/
/*	Author: Scott Burleigh, Jet Propulsion Laboratory		*/
/*      R. Carper: modified for Mac OS X platform (darwin)		*/
/*      J.Veregge: modified for all platforms to consolidate		*/
/*									*/
#ifndef _PLATFORM_H_
#define _PLATFORM_H_

#ifdef __cplusplus
extern "C" {
#endif

#define	MAX_POSIX_TIME	2147483644

/*	SPACE_ORDER is log2 of the number of bytes in an address, i.e.:

		1	for 16-bit machines (2 ** 1 = 2 bytes per address)
		2	for 32-bit machines (2 ** 2 = 4 bytes per address)
		3	for 64-bit machines (2 ** 3 = 8 bytes per address)

	etc.  If not specified as compiler option, defaults to 2.	*/

#ifndef SPACE_ORDER
#define SPACE_ORDER	2
#endif

#define WORD_SIZE	(1 << SPACE_ORDER)
#define SMALL_SIZES	(64)

#define LARGE_ORDER1	(SPACE_ORDER + 1)	/*	double word	*/
#define LARGE_ORDERn	((WORD_SIZE * 8) - 1)	/*	8 bits/byte	*/
#define LARGE_ORDERS	((LARGE_ORDERn - LARGE_ORDER1) + 1)

/*
** Standard Headers: Common to All Supported Platforms (incl. VxWorks and BSD)
*/
			/* STDC.88 */
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <time.h>
#include <errno.h>
#include <stdarg.h>
			/* POSIX.1 */
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/times.h>
//#include <sys/types.h>
#include <limits.h>
#include <sys/wait.h>
			/* Other */
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
/*
** End of Standard Headers
*/

#ifdef ERROR
#undef ERROR
#endif
#define ERROR			(-1)

#define	PATHLENMAX		(256)

#if defined (darwin) || defined (freebsd)
#define NONE			NULL
#else
#define NONE			(-1)
#endif

#define BAD_HOST_NAME		(0)

#define FD_BITMAP(x)		(&x)

typedef void			(*SignalHandler)(int);

typedef struct
{
	char			opaque[64];
} ResourceLock;

#ifdef TORNADO_2_0_2
#define isprintf(buffer, bufsize, format, args...)	\
oK(_isprintf(__FILE__, __LINE__, buffer, bufsize, format, args))
#else
#define isprintf(buffer, bufsize, format, ...)		\
oK(_isprintf(__FILE__, __LINE__, buffer, bufsize, format, __VA_ARGS__))
#endif

#ifdef FSWSOURCE
#define	FSWLOGGER
#define	FSWCLOCK
#define	FSWWDNAME
#define	FSWSYMTAB
#endif

#ifdef GDSSOURCE
#define	GDSLOGGER
#define	GDSSYMTAB
#endif

#ifndef MIN
#define MIN(x, y) ((x) < (y) ? (x) : (y))
#endif
#ifndef MAX
#define MAX(x, y) ((x) > (y) ? (x) : (y))
#endif

/*	Macros for expunging access to stdout and stderr.		*/

#ifdef FSWLOGGER
#define PUTS(text)		writeMemo(text)
#define PERROR(text)		writeMemoNote(text, system_error_msg())
#define PUTMEMO(text, memo)	writeMemoNote(text, memo)
#else
#define PUTS(text)		puts(text)
#define PERROR(text)		perror(text)
#define PUTMEMO(text, memo)	printf("%s: %s\n", text, memo)
#endif

#ifdef VXWORKS			/****	VxWorks				****/

#include <vxWorks.h>
#include <sockLib.h>
#include <taskLib.h>
#include <taskHookLib.h>
#include <msgQLib.h>
#include <semLib.h>
#include <timers.h>
#include <hostLib.h>
#include <ioLib.h>
#include <remLib.h>
#include <tickLib.h>
#include <sysLib.h>
#include <selectLib.h>
#include <rebootLib.h>
#include <pthread.h>

#define	FDTABLE_SIZE		(FD_SETSIZE)
#define	MAXPATHLEN		(MAX_FILENAME_LENGTH)

#ifndef VXWORKS6
typedef int			socklen_t;
#endif

#else				/****	Not VxWorks			****/

#if defined (RTEMS)             /****   RTEMS is UNIXy but not enough   ****/

#define POSIX1B_SEMAPHORES

typedef void	(*FUNCPTR)(int, int, int, int, int, int, int, int, int, int);

#define PRIVATE_SYMTAB

/** RTEMS has some SVR4 stuff  **/
#include <bsp.h>
#include <rtems.h>
#include <pthread.h>
#include <pwd.h>
#include <netdb.h>
#include <mqueue.h>
#include <semaphore.h>		/****	POSIX1B semaphores		****/
#include <sys/utsname.h>
#include <sys/param.h>		/****	...to pick up MAXHOSTNAMELEN	****/
#include <sys/resource.h>
#include <sys/time.h>

#define	_MULTITHREADED		/*	To pick up resource lock code.	*/

#else				/****	Neither VxWorks nor RTEMS	****/

#if defined (unix)		/****	All UNIX platforms		****/

#define __GNU_SOURCE		/****	Needed for Linux & Darwin	****/

/*
** *NIX Headers: Common to All Supported *NIX Platforms
*/
#include <sys/utsname.h>
#include <pwd.h>
#include <netdb.h>
#include <sys/resource.h>
#include <sys/time.h>

#define SVR4_MSGQS		/****	default				****/
#define SVR4_SEMAPHORES		/****	default				****/
#define SVR4_SHM		/****	default				****/

#ifdef noipc			/****	Cygwin without cygipc		****/
#undef SVR4_MSGQS
#undef SVR4_SEMAPHORES
#undef SVR4_SHM
#endif				/****	End of #ifdef noipc		****/

#ifdef SVR4_MSGQS
#include <sys/msg.h>
#elif defined (POSIX1B_MSGQS)
#include <sys/mqueue.h>
#endif

#ifdef SVR4_SEMAPHORES
#include <sys/ipc.h>
#include <sys/sem.h>
/*	Override these macros with -D option on gcc command line
 *	if system's parameters differ from these.  SEMMNI is the
 *	maximum number of semaphore sets in the system.  SEMMSL is
 *	the maximum number of semaphores per set (i.e., per semid).
 *	SEMMNS is the maximum number of semaphores, which cannot
 *	exceed SEMMNI * SEMMSL.						*/
#ifndef SEMMNI
#if defined (cygwin)
#define SEMMNI			10
#elif defined (freebsd)
#define SEMMNI			10
#else
#define SEMMNI			128
#endif
#endif

#ifndef SEMMSL
#if defined (cygwin)
#define SEMMSL			6
#elif defined (freebsd)
#define SEMMSL			6
#else
#define SEMMSL			250
#endif
#endif

#ifndef SEMMNS
#if defined (cygwin)
#define SEMMNS			60
#elif defined (freebsd)
#define SEMMNS			60
#else
#define SEMMNS			32000
#endif
#endif

#elif defined (POSIX1B_SEMAPHORES)
#include <semaphore.h>		/*	In case not already included.	*/
#endif

#ifdef SVR4_SHM
#include <sys/shm.h>
#elif defined (POSIX1B_SHM)
#include <sys/mman.h>
#endif

/*
** End of *NIX Headers
*/

#if defined (__SVR4)		/****	All Sys 5, Rev 4 UNIX systems	****/

#define FIFO_READ_MODE		(O_RDWR)
#define FIFO_WRITE_MODE		(O_WRONLY)

#define	FDTABLE_SIZE		(sysconf(_SC_OPEN_MAX))

#if defined (_REENTRANT)	/****	SVR4 multithreaded		****/

/*
** SVR4 Headers: Common to All Supported SVR4 Multithreaded Platforms
*/
#include <synch.h>
#include <pthread.h>
/*
** End of SVR4 Headers
*/

extern int			strcasecmp(const char*, const char*);
extern int			strncasecmp(const char*, const char*, size_t);

#define	_MULTITHREADED

#else				/****	Not SVR4 multithreaded		****/

#endif				/****	End of #if defined (_REENTRANT) ****/

#if defined (sparc)		/****	Solaris (SunOS 5+)		****/

#if defined (sol5)		/****	Solaris 5.5.x			****/
int gettimeofday(struct timeval*, void*);
int getpriority(int, id_t);
#endif				/****	End of #if defined (sol5)	****/

#else				/****	Not Solaris			****/

#endif				/****	End of #if defined (sparc)	****/

#else				/****	Not SVR4 at all (probably BSD)	****/

#define FIFO_READ_MODE          (O_RDWR)
#define FIFO_WRITE_MODE         (O_RDWR)

#define	FDTABLE_SIZE		(getdtablesize())

#if defined (sun)		/****	pre-Solaris SunOS (v. < 5)	****/

#include <malloc.h>
#include <sys/filio.h>

#else				/****	Not pre-Solaris SunOS		****/

#if defined (hpux)		/****	HPUX				****/

#define _INCLUDE_HPUX_SOURCE
#define _INCLUDE_POSIX_SOURCE
#define _INCLUDE_XOPEN_SOURCE

#else				/****	Not HPUX			****/

#if defined (linux)		/****	Linux				****/

#include <malloc.h>
#include <rpc/types.h>		/****	...to pick up MAXHOSTNAMELEN	****/
#include <pthread.h>

#define	_MULTITHREADED

#else                           /****	Not Linux			****/

#if defined (freebsd)		/****	FreeBSD				****/

#include <sys/param.h>		/****	...to pick up MAXHOSTNAMELEN	****/
#include <pthread.h>

#define	_MULTITHREADED

#else                           /****	Not FreeBSD			****/

#if defined (cygwin)		/****	Cygwin				****/

#include <malloc.h>
#include <sys/param.h>		/****	...to pick up MAXHOSTNAMELEN	****/
#include <pthread.h>

#define	_MULTITHREADED

struct msgbuf			/****	Might be defined in cygipc...	****/
{
	long 		mtype;		/*	type of rcvd/sent msg	*/
	char		mtext[1];	/*	text of the message	*/
};

#undef FDTABLE_SIZE
#define	FDTABLE_SIZE	(64)	/****	getdtablesize() is buggy?	****/

#else                           /****	Not Cygwin			****/

#if defined (darwin)		/****	Mac OS X			****/

#include <sys/malloc.h>
#include <stdlib.h>
#include <sys/param.h>		/****	...to pick up MAXHOSTNAMELEN	****/
#include <pthread.h>

#include <sys/msg.h>
#define	msgbuf		mymsg	/****	Mac OS X doesn't have msgbuf,	****/
				/****	but it has mymsg (same thing).	****/

#define	_MULTITHREADED

#else                           /****	Not Mac OS X			****/

#if defined (interix)		/****	Windows				****/

#include <pthread.h>

#define	_MULTITHREADED

#ifndef MSG_NOSIGNAL
#define	MSG_NOSIGNAL		0
#endif

typedef int			socklen_t;

#else				/****	Not Interix			****/

#error "Can't determine operating system to compile for."

#endif				/****	End of #if defined (interix)	****/
#endif				/****	End of #if defined (darwin)	****/
#endif				/****	End of #if defined (cygwin)	****/
#endif				/****	End of #if defined (freebsd)	****/
#endif				/****	End of #if defined (linux)	****/
#endif				/****	End of #if defined (hpux)	****/
#endif				/****	End of #if defined (sun)	****/
#endif				/****	End of #if defined (__SVR4)	****/
#endif				/****	End of #if defined (unix)	****/
#endif				/****	End of #if defined (RTEMS)	****/
#endif				/****	End of #if defined (VXWORKS)	****/

typedef void			(* Logger)(char *);

extern void			*acquireSystemMemory(size_t);
extern int			createFile(const char*, int);
extern char *			system_error_msg();
extern void			setLogger(Logger);
extern void			writeMemo(char *);
extern void			writeErrMemo(char *);
extern void			writeMemoNote(char *, char *);
extern void			snooze(unsigned int);
extern void			microsnooze(unsigned int);
extern void			getCurrentTime(struct timeval *);
extern unsigned long		getClockResolution();	/*	usec	*/
#ifndef ION_NO_DNS
extern unsigned int		getInternetAddress(char *);
extern char *			getInternetHostName(unsigned int, char *);
extern int			getNameOfHost(char *, int);
extern char *			getNameOfUser(char *);
extern int			reUseAddress(int);
extern int			watchSocket(int);
#endif
extern int			makeIoNonBlocking(int);
extern void			closeOnExec(int);
extern int			initResourceLock(ResourceLock *);
extern void			killResourceLock(ResourceLock *);
extern void			lockResource(ResourceLock *);
extern void			unlockResource(ResourceLock *);

#ifndef ERRMSGS_BUFSIZE
#define ERRMSGS_BUFSIZE		(256*16)
#endif

#ifdef  DOS_PATH_DELIMITER
#define ION_PATH_DELIMITER	'\\'
#else
#define ION_PATH_DELIMITER	'/'
#endif

extern char			*itoa(int);
extern char			*utoa(unsigned int);
#define postErrmsg(txt, arg)	_postErrmsg(__FILE__, __LINE__, txt, arg)
extern void			_postErrmsg(const char *, int, const char *,
					const char *);
#define postSysErrmsg(txt, arg) _postSysErrmsg(__FILE__, __LINE__, txt, arg)
extern void			_postSysErrmsg(const char *, int, const char *,
					const char *);
#define putErrmsg(txt, arg)	_putErrmsg(__FILE__, __LINE__, txt, arg)
extern void			_putErrmsg(const char *, int, const char *,
					const char *);
#define putSysErrmsg(txt, arg)	_putSysErrmsg(__FILE__, __LINE__, txt, arg)
extern void			_putSysErrmsg(const char *, int, const char *,
					const char *);
extern int			getErrmsg(char *buffer);
extern void			writeErrmsgMemos();
extern void			discardErrmsgs();

/*	Return values for error conditions.				*/
#ifndef CORE_FILE_NEEDED
#define CORE_FILE_NEEDED	(0)
#endif

#define iEnd(arg)		_iEnd(__FILE__, __LINE__, arg)
extern int			_iEnd(const char *, int, const char *);
extern int			_coreFileNeeded(int *);

#define CHKERR(e)    		if (!(e) && iEnd(#e)) return -1
#define CHKZERO(e)    		if (!(e) && iEnd(#e)) return 0
#define CHKNULL(e)    		if (!(e) && iEnd(#e)) return NULL
#define CHKVOID(e)    		if (!(e) && iEnd(#e)) return

/*	The following macro deals with irrelevant return codes.		*/
#define oK(x)			(void)(x)

/*	Standard SDNV operations.					*/

typedef struct
{
	int		length;
	unsigned char	text[10];
} Sdnv;

extern void			encodeSdnv(Sdnv *, unsigned long);
extern int			decodeSdnv(unsigned long *, unsigned char *);

#define	ONE_GIG			(1 << 30)

typedef struct
{
	signed int	gigs;
	signed int	units;
} Scalar;

extern void			loadScalar(Scalar *, signed int);
extern void			increaseScalar(Scalar *, signed int);
extern void			reduceScalar(Scalar *, signed int);
extern void			multiplyScalar(Scalar *, signed int);
extern void			divideScalar(Scalar *, signed int);
extern void			copyScalar(Scalar *to, Scalar *from);
extern void			addToScalar(Scalar *, Scalar *);
extern void			subtractFromScalar(Scalar *, Scalar *);
extern int			scalarIsValid(Scalar *);

extern int			_isprintf(const char *, int, char *, int,
					char *, ...);
extern char			*istrcpy(char *, char *, size_t);
extern char			*igetcwd(char *, size_t);
extern void			isignal(int, void (*)(int));
extern void			iblock(int);
extern char			*igets(int, char *, int, int *);
extern int			iputs(int, char *);

extern void			findToken(char **cursorPtr, char **token);
extern void			parseSocketSpec(char *socketSpec,
					unsigned short *portNbr,
					unsigned int *ipAddress);

#include "platform_sm.h"

#ifdef __cplusplus
}
#endif

#endif  /* _PLATFORM_H_ */
