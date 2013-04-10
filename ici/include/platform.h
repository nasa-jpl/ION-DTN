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

#ifdef uClibc
#ifndef linux
#define linux
#endif
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

#if (defined(RTEMS) || defined(uClibc))
/*	In RTEMS 4.9, defining the first field of a struct as
 *	"long long" apparently doesn't cause the struct (nor that
 *	first field) to be aligned on a "long long" boundary, so
 *	we get alignment errors.  For now, we get around this by
 *	simply defining "vast" as "long"; node numbers larger than
 *	4G won't be processed properly on an RTEMS platform.  At
 *	some point somebody may figure out a workaround in the
 *	compiler so that we can fix this.
 *
 *	In uClibc, support for "long long" integers apparently
 *	requires that libgcc_s.so.1 be installed.  Because our
 *	test environment doesn't include this library, we have
 *	to define "vast" as "long"; node numbers larger than 4G
 *	won't be processed properly on a uClibc platform.  System
 *	integrators who can provide libgcc_s.so.1 should be able
 *	to restore this functionality by revising this conditional
 *	compilation.							*/
typedef long			vast;
typedef unsigned long		uvast;
#define	VAST_FIELDSPEC		"%l"
#define	UVAST_FIELDSPEC		"%lu"
#define	strtovast(x)		strtol(x, NULL, 0)
#define	strtouvast(x)		strtoul(x, NULL, 0)
#elif (SPACE_ORDER < 3)	/*	32-bit machines.			*/
typedef long long		vast;
typedef unsigned long long	uvast;
#ifdef mingw
#define	VAST_FIELDSPEC		"%I64d"
#define	UVAST_FIELDSPEC		"%I64u"
#else
#define	VAST_FIELDSPEC		"%ll"
#define	UVAST_FIELDSPEC		"%llu"
#endif
#define	strtovast(x)		strtoll(x, NULL, 0)
#define	strtouvast(x)		strtoull(x, NULL, 0)
#else			/*	64-bit machines.			*/
typedef long			vast;
typedef unsigned long		uvast;
#define	VAST_FIELDSPEC		"%l"
#define	UVAST_FIELDSPEC		"%lu"
#define	strtovast(x)		strtol(x, NULL, 0)
#define	strtouvast(x)		strtoul(x, NULL, 0)
#endif

#define WORD_SIZE	(1 << SPACE_ORDER)
#define SMALL_SIZES	(64)

#define LARGE_ORDER1	(SPACE_ORDER + 1)	/*	double word	*/
#define LARGE_ORDERn	((WORD_SIZE * 8) - 1)	/*	8 bits/byte	*/
#define LARGE_ORDERS	((LARGE_ORDERn - LARGE_ORDER1) + 1)

#define	ONE_GIG			(1 << 30)

#ifndef ERRMSGS_BUFSIZE
#define ERRMSGS_BUFSIZE		(256*16)
#endif

#ifdef  DOS_PATH_DELIMITER
#define ION_PATH_DELIMITER	'\\'
#else
#define ION_PATH_DELIMITER	'/'
#endif

/*	Return values for error conditions.				*/
#ifndef CORE_FILE_NEEDED
#define CORE_FILE_NEEDED	(0)
#endif

#ifdef RTEMS			/****	RTEMS			*********/
typedef unsigned long		n_long;	/*	long as rec'd from net	*/
#endif

/*
** Standard Headers: Common to All Supported Platforms (incl. RTOS & Windows)
*/
			/* STDC.88 */
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <errno.h>
#include <stdarg.h>
			/* POSIX.1 */
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>

#ifdef mingw			/****   Windows vs all others	*********/
#include <windows.h>
#include <process.h>
#include <Winbase.h>
#include <sys/time.h>
#include <sys/types.h>
#include <ws2tcpip.h>
#define MAXHOSTNAMELEN		256
#ifndef SOCK_CLOEXEC
#define SOCK_CLOEXEC		0
#endif
#define	ECONNREFUSED		WSAECONNREFUSED
#define ECONNRESET		WSAECONNRESET
#define EWOULDBLOCK		WSAEWOULDBLOCK
#define	O_LARGEFILE		0
#else				/****	not Windows		*********/
#include <sys/times.h>
#include <limits.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#ifndef SOCK_CLOEXEC
#define SOCK_CLOEXEC		0
#endif
#define closesocket(x)		close(x)
#define iopen(a,b,c)		open(a,b,c)
#define isend(a,b,c,d)		send(a,b,c,d)
#define irecv(a,b,c,d)		recv(a,b,c,d)
#define isendto(a,b,c,d,e,f)	sendto(a,b,c,d,e,f)
#define irecvfrom(a,b,c,d,e,f)	recvfrom(a,b,c,d,e,f)
#endif				/****   End of #ifdef mingw	*********/
/*
** End of Standard Headers
*/

/*	Handy definitions that are mostly platform-independent.		*/

#define itoa			iToa
#define utoa			uToa

#ifdef ERROR
#undef ERROR
#endif
#define ERROR			(-1)

#ifdef __GNUC__
#define UNUSED  __attribute__((unused))
#else
#define UNUSED
#endif

#ifndef MIN
#define MIN(x, y) ((x) < (y) ? (x) : (y))
#endif
#ifndef MAX
#define MAX(x, y) ((x) > (y) ? (x) : (y))
#endif

#ifndef LONG_MAX

#if defined (_ILP32)
#define LONG_MAX 0x7fffffffL
#elif defined (_LP64)
#define LONG_MAX 0x7fffffffffffffffL
#elif (SIZEOF_LONG == 4)
#define LONG_MAX 0x7fffffffL
#elif (SIZEOF_LONG == 8)
#define LONG_MAX 0x7fffffffffffffffL
#endif

#endif				/****	End of #ifndef LONG_MAX   *******/

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
	uvast			opaque[8];
} ResourceLock;

#ifdef TORNADO_2_0_2
#define isprintf(buffer, bufsize, format, args...)	\
oK(_isprintf(buffer, bufsize, format, args))
#else
#define isprintf(buffer, bufsize, format, ...)		\
oK(_isprintf(buffer, bufsize, format, __VA_ARGS__))
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

/*	Configure for platform-specific headers and IPC services.	*/

#define SVR4_SEMAPHORES		/****	default			*********/
#define SVR4_SHM		/****	default			*********/

#ifdef VXWORKS			/****	VxWorks			*********/

#undef	SVR4_SHM
#define RTOS_SHM

#undef	SVR4_SEMAPHORES
#define VXWORKS_SEMAPHORES

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

#endif				/****   End of #ifdef VXWORKS	*********/

#ifdef RTEMS			/****	RTEMS			*********/

#undef	SVR4_SHM
#define RTOS_SHM

#undef	SVR4_SEMAPHORES
#define POSIX1B_SEMAPHORES

typedef void	(*FUNCPTR)(int, int, int, int, int, int, int, int, int, int);

#define PRIVATE_SYMTAB

#include <bsp.h>
#include <rtems.h>
#include <pthread.h>
#include <pwd.h>
#include <netdb.h>
#include <mqueue.h>
#include <sys/utsname.h>
#include <sys/param.h>		/****	...to get MAXHOSTNAMELEN	*/
#include <sys/resource.h>
#include <sys/time.h>

#define	_MULTITHREADED		/*	To pick up resource lock code.	*/

#endif				/****	End of #ifdef (RTEMS)	     ****/

#ifdef mingw			/****	Windows			     ****/

#undef	SVR4_SHM
#define MINGW_SHM

#undef	SVR4_SEMAPHORES
#define MINGW_SEMAPHORES

#include <pthread.h>

#ifndef gmtime_r
#define gmtime_r(_clock, _result) \
	(*(_result) = *gmtime(_clock), (_result))
#endif

#ifndef localtime_r
#define localtime_r(_clock, _result) \
	(*(_result) = *localtime(_clock), (_result))
#endif

#ifndef rand_r
#define rand_r(_seed) (rand())
#endif

#define	_MULTITHREADED
#define	MAXPATHLEN		(MAX_PATH)

extern int	_winsock(int stopping);
extern int	iopen(const char *fileName, int flags, int pmode);
extern int	isend(int sockfd, char *buf, int len, int flags);
extern int	irecv(int sockfd, char *buf, int len, int flags);
extern int	isendto(int sockfd, char *buf, int len, int flags,
			const struct sockaddr *to, int tolen);
extern int	irecvfrom(int sockfd, char *buf, int len, int flags,
			struct sockaddr *from, int *fromlen);

#endif				/****	End of #ifdef mingw          ****/

#ifdef unix			/****	All UNIX platforms	     ****/

#define __GNU_SOURCE		/****	Needed for Linux & Darwin    ****/

/*
** *NIX Headers: Common to All Supported *NIX Platforms
*/
#include <sys/utsname.h>
#include <pwd.h>
#include <netdb.h>
#include <sys/resource.h>
#include <sys/time.h>
/*
** End of *NIX Headers
*/

#ifdef __SVR4			/****	All Sys 5 Rev 4 UNIX systems ****/

#define FIFO_READ_MODE		(O_RDWR)
#define FIFO_WRITE_MODE		(O_WRONLY)

#define	FDTABLE_SIZE		(sysconf(_SC_OPEN_MAX))

#ifdef _REENTRANT		/****	SVR4 multithreaded	     ****/

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
#endif				/****	End of #ifdef _REENTRANT     ****/

#ifdef sparc			/****	Solaris (SunOS 5+)	     ****/
#ifdef sol5			/****	Solaris 5.5.x		     ****/
extern int gettimeofday(struct timeval*, void*);
extern int getpriority(int, id_t);
#endif				/****	End of #ifdef (sol5)         ****/
#endif				/****	End of #ifdef (sparc)        ****/

#else				/****	Not __SVR4 at all (BSD?)     ****/

#define FIFO_READ_MODE          (O_RDWR)
#define FIFO_WRITE_MODE         (O_RDWR)
#define	FDTABLE_SIZE		(getdtablesize())

#ifdef linux			/****	Linux			     ****/

#include <malloc.h>

#include <pthread.h>

#ifdef bionic			/****	Bionic subset of Linux      ****/

#undef	SVR4_SHM
#define RTOS_SHM

#undef	SVR4_SEMAPHORES
#define POSIX1B_SEMAPHORES

#include <sys/param.h>		/****	...to get MAXPATHLEN         ****/

#ifndef SEM_NSEMS_MAX
#define	SEM_NSEMS_MAX		256
#endif

typedef void	(*FUNCPTR)(int, int, int, int, int, int, int, int, int, int);

#define PRIVATE_SYMTAB

#else				/****	Not bionic		     ****/
#ifdef uClibc
#include <asm/param.h>		/****	...to get MAXHOSTNAMELEN     ****/
#include <sys/param.h>		/****	...to get MAXPATHLEN	     ****/
#else				/****	Not bionic and not uClibc    ****/
#include <rpc/types.h>		/****	...to get MAXHOSTNAMELEN     ****/
#include <execinfo.h>		/****	...to get backtrace	     ****/
#endif				/*	End of #ifdef uClibc	     ****/
#endif				/****	End of #ifdef bionic	     ****/

#define	_MULTITHREADED

#endif				/****	End of #ifdef linux	     ****/

#ifdef freebsd			/****	FreeBSD			     ****/

#include <sys/param.h>		/****	...to get MAXHOSTNAMELEN     ****/
#include <pthread.h>

#define	_MULTITHREADED

#endif				/****	End of #ifdef freebsd	     ****/

#ifdef darwin			/****	Mac OS X		     ****/

#include <sys/malloc.h>
#include <stdlib.h>
#include <sys/param.h>		/****	...to get MAXHOSTNAMELEN     ****/
#include <pthread.h>

#include <sys/msg.h>
#define	msgbuf		mymsg	/****	Mac OS X has no msgbuf,	but  ****/
				/****	it has mymsg (same thing).   ****/
#define	_MULTITHREADED

#define	O_LARGEFILE	0

#endif				/****	End of #ifdef darwin	     ****/

#endif				/****	End of #ifdef (__SVR4)       ****/

#endif				/****	End of #ifdef (unix)         ****/

#if defined (SVR4_SHM)		/****	SVR4_SHM		     ****/
#include <sys/shm.h>
#elif defined (POSIX1B_SHM)
#include <sys/mman.h>
#endif				/****	End of #ifdef SVR4-SHM	     ****/

#if defined (SVR4_SEMAPHORES)	/****	SVR4_SEMAPHORES		     ****/

#include <sys/ipc.h>
#include <sys/sem.h>

/*	Override these macros with -D option on gcc command line
 *	if system's parameters differ from these.  SEMMNI is the
 *	maximum number of semaphore sets in the system.  SEMMSL is
 *	the maximum number of semaphores per set (i.e., per semid).
 *	SEMMNS is the maximum number of semaphores, which cannot
 *	exceed SEMMNI * SEMMSL.						*/

#ifndef SEMMNI			/****	SEMMNI			     ****/
#if defined (freebsd)
#define SEMMNI			10
#else
#define SEMMNI			128
#endif
#endif				/****	End of #ifndef SEMMNI	     ****/

#ifndef SEMMSL			/****	SEMMSL			     ****/
#if defined (freebsd)
#define SEMMSL			6
#else
#define SEMMSL			250
#endif
#endif				/****	End of #ifndef SEMMSL	     ****/

#ifndef SEMMNS			/****	SEMMNS			     ****/
#if defined (freebsd)
#define SEMMNS			60
#else
#define SEMMNS			32000
#endif
#endif				/****	End of #ifndef SEMMNS	     ****/

#elif defined (POSIX1B_SEMAPHORES)

#include <semaphore.h>

#endif				/****	End #if defined SVR4_SEMAPHORES */

#ifdef HAVE_VALGRIND_VALGRIND_H
#include "valgrind/valgrind.h"
#endif

/*	Prototypes for standard ION platform functions.			*/

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
extern unsigned int		getAddressOfHost();
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

#define iEnd(arg)		_iEnd(__FILE__, __LINE__, arg)
extern int			_iEnd(const char *, int, const char *);
extern int			_coreFileNeeded(int *);

#define CHKERR(e)    		if (!(e) && iEnd(#e)) return ERROR
#define CHKZERO(e)    		if (!(e) && iEnd(#e)) return 0
#define CHKNULL(e)    		if (!(e) && iEnd(#e)) return NULL
#define CHKVOID(e)    		if (!(e) && iEnd(#e)) return

extern void			printStackTrace();

/*	The following macro deals with irrelevant return codes.		*/
#define oK(x)			(void)(x)

/*	Standard SDNV operations.					*/

typedef struct
{
	int		length;
	unsigned char	text[10];
} Sdnv;

extern void			encodeSdnv(Sdnv *, uvast);
extern int			decodeSdnv(uvast *, unsigned char *);

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

extern int			_isprintf(char *, int, char *, ...);
extern size_t			istrlen(char *, size_t);
extern char			*istrcpy(char *, char *, size_t);
extern char			*istrcat(char *, char *, size_t);
extern char			*igetcwd(char *, size_t);
extern void			isignal(int, void (*)(int));
extern void			iblock(int);
extern char			*igets(int, char *, int, int *);
extern int			iputs(int, char *);

extern void			findToken(char **cursorPtr, char **token);
extern int			parseSocketSpec(char *socketSpec,
					unsigned short *portNbr,
					unsigned int *ipAddress);
extern void			printDottedString(unsigned int hostNbr,
					char *buffer);
#include "platform_sm.h"

#ifdef __cplusplus
}
#endif

#endif  /* _PLATFORM_H_ */
