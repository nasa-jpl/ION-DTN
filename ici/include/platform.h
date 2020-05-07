/*

	platform.h:	platform-dependent porting adaptations.

	Copyright (c) 2001, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
									*/
/*	Author: Scott Burleigh, Jet Propulsion Laboratory		*/
/*      R. Carper: modified for Mac OS X platform (darwin)		*/
/*      J. Veregge: modified for all platforms to consolidate		*/
/*      S. Clancy: added STRSOE flag for building with JPL STRS OE	*/
/*									*/
#ifndef _PLATFORM_H_
#define _PLATFORM_H_

#ifdef __cplusplus
extern "C" {
#endif

#if defined (VXWORKS) || defined (RTEMS) || defined (bionic) || defined (AESCFS) || defined (STRSOE)
#define ION_LWT
#else
#undef ION_LWT
#endif

/*	NOTE: the -DION4WIN compiler switch is used to indicate that
 *	header code must be rendered suitable for compilation of ION-
 *	based Windows executables in a Visual Studio build environment.
 *	It usually overrides the "-Dmingw" compiler switch, which is
 *	otherwise required when building ION and ION-based software
 *	for Windows.  In some cases the effect of -Dmingw and -DION4WIN
 *	is the same: the affected code is suitable for Windows develop-
 *	ment (and only Windows development), whether within Visual
 *	Studio or not.							*/

#ifndef mingw
#if (defined(__MINGW32__))
#define mingw
#endif
#endif

#if (defined(__MINGW64__))
#undef	SPACE_ORDER
#define	SPACE_ORDER	3
#elif (defined(__MINGW32__))
#undef	SPACE_ORDER
#define	SPACE_ORDER	2
#endif

#ifdef uClibc
#ifndef linux
#define linux
#endif
#ifndef __UCLIBC__
#define __UCLIBC__
#endif
#endif

#define	MAX_POSIX_TIME	2147483647

/*	SPACE_ORDER is log2 of the number of bytes in an address, i.e.:

		1	for 16-bit machines (2 ** 1 = 2 bytes per address)
		2	for 32-bit machines (2 ** 2 = 4 bytes per address)
		3	for 64-bit machines (2 ** 3 = 8 bytes per address)

	etc.  If not specified as compiler option, defaults to 2.	*/

#ifndef SPACE_ORDER
#define SPACE_ORDER	2
#endif

/*	We define new data types "vast" and "uvast", which are always
 *	64-bit numbers regardless of the native machine architecture
 *	(except as noted below).					*/

#if (defined (RTEMS) || defined (uClibc)) || defined (STRSOE)
/*	In the RTEMS 4.9 development environment for Linux (for
 *	target sparc-rtems4.9), defining the first field of a struct
 *	as "long long" apparently doesn't cause the struct (nor that
 *	first field) to be aligned on a "long long" boundary, so in
 *	JPL's ION RTEMS development environment we get alignment
 *	errors.  For now, we get around this by simply defining "vast"
 *	as "long"; node numbers larger than 4G won't be processed
 *	properly on an RTEMS platform.  The solution seems to be that
 *	RTEMS needs to be built with the CPU_ALIGNMENT macro set to 8
 *	rather than 4.  ION/RTEMS system integrators who can build
 *	RTEMS in this configuration should set the -DLONG_LONG_OKAY
 *	compiler flag to 1 when building ION.
 *
 *	In uClibc, support for "long long" integers apparently
 *	requires that libgcc_s.so.1 be installed.  Because JPL's
 *	ION uClibc development environment doesn't include this
 *	library, we have to define "vast" as "long"; node numbers
 *	larger than 4G won't be processed properly on a uClibc
 *	platform.  ION/uClibc system integrators who can provide
 *	libgcc_s.so.1 should set the -DLONG_LONG_OKAY compiler flag
 *	to 1 when building ION.						*/

#ifndef	SEM_NSEMS_MAX
#define	SEM_NSEMS_MAX		(256)
#endif
#ifndef LONG_LONG_OKAY
#define	LONG_LONG_OKAY		0	/*	Default value.		*/
#endif

#else					/*	Not RTEMS or uClibc.	*/

#ifndef LONG_LONG_OKAY
#define	LONG_LONG_OKAY		1	/*	Default value.		*/
#endif

#endif	/*	RTEMS or uClibc	or STRSOE				*/

#if (!LONG_LONG_OKAY)
typedef long			vast;
typedef unsigned long		uvast;
typedef long			saddr;	/*	Pointer-sized integer.	*/
typedef unsigned long		uaddr;	/*	Pointer-sized integer.	*/
#define	VAST_FIELDSPEC		"%ld"
#define	UVAST_FIELDSPEC		"%lu"
#define UVAST_HEX_FIELDSPEC	"%lx"
#define	ADDR_FIELDSPEC		"%#lx"
#define ilseek(a, b, c)		lseek(a, b, c)
#define	strtovast(x)		strtol(x, NULL, 0)
#define	strtouvast(x)		strtoul(x, NULL, 0)
#define	strtoaddr(x)		strtoul(x, NULL, 0)
#define LARGE1			1UL
#elif (SPACE_ORDER < 3)	/*	32-bit machines.			*/
typedef long long		vast;
typedef unsigned long long	uvast;
typedef long			saddr;	/*	Pointer-sized integer.	*/
typedef unsigned long		uaddr;	/*	Pointer-sized integer.	*/
#if (defined(mingw) || defined(ION4WIN))
#define	VAST_FIELDSPEC		"%I64d"
#define	UVAST_FIELDSPEC		"%I64u"
#define UVAST_HEX_FIELDSPEC	"%I64x"
#define	ADDR_FIELDSPEC		"%#lx"
#define ilseek(a, b, c)		lseek64(a, b, c)
#else				/*	Not Windows.			*/
#define	VAST_FIELDSPEC		"%lld"
#define	UVAST_FIELDSPEC		"%llu"
#define UVAST_HEX_FIELDSPEC	"%llx"
#define	ADDR_FIELDSPEC		"%#lx"
#define ilseek(a, b, c)		lseek(a, b, c)
#endif				/*	end #ifdef mingw || ION4WIN	*/
#define	strtovast(x)		strtoll(x, NULL, 0)
#define	strtouvast(x)		strtoull(x, NULL, 0)
#define	strtoaddr(x)		strtoul(x, NULL, 0)
#define LARGE1			1UL
#else			/*	64-bit machines.			*/
#if (defined(mingw) || defined(ION4WIN))
typedef long long		vast;
typedef unsigned long long	uvast;
typedef long long		saddr;	/*	Pointer-sized integer.	*/
typedef unsigned long long	uaddr;	/*	Pointer-sized integer.	*/
#define	VAST_FIELDSPEC		"%I64d"
#define	UVAST_FIELDSPEC		"%I64u"
#define UVAST_HEX_FIELDSPEC	"%I64x"
#define	ADDR_FIELDSPEC		"%#I64x"
#define ilseek(a, b, c)		lseek64(a, b, c)
#define	strtovast(x)		strtoll(x, NULL, 0)
#define	strtouvast(x)		strtoull(x, NULL, 0)
#define	strtoaddr(x)		strtoull(x, NULL, 0)
#define LARGE1			1ULL
#else				/*	Not Windows.			*/
typedef long			vast;
typedef unsigned long		uvast;
typedef long			saddr;	/*	Pointer-sized integer.	*/
typedef unsigned long		uaddr;	/*	Pointer-sized integer.	*/
#define	VAST_FIELDSPEC		"%ld"
#define	UVAST_FIELDSPEC		"%lu"
#define UVAST_HEX_FIELDSPEC	"%lx"
#define	ADDR_FIELDSPEC		"%#lx"
#define ilseek(a, b, c)		lseek(a, b, c)
#define	strtovast(x)		strtol(x, NULL, 0)
#define	strtouvast(x)		strtoul(x, NULL, 0)
#define	strtoaddr(x)		strtoul(x, NULL, 0)
#define LARGE1			1UL
#endif				/*	end #ifdef mingw || ION4WIN	*/
#endif	/*	!LONG_LONG_OKAY						*/

#define WORD_SIZE	(1 << SPACE_ORDER)
#define SMALL_SIZES	(64)

#define LARGE_ORDER1	(SPACE_ORDER + 1)	/*	double word	*/
#define LARGE_ORDERn	((WORD_SIZE * 8) - 1)	/*	8 bits/byte	*/
#define LARGE_ORDERS	((LARGE_ORDERn - LARGE_ORDER1) + 1)

#define	ONE_GIG			(1 << 30)

#ifndef ERRMSGS_BUFSIZE
#define ERRMSGS_BUFSIZE		(256*16)
#endif

#ifndef	DEFAULT_CHECK_TIMEOUT
#define	DEFAULT_CHECK_TIMEOUT	(120)
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

#if defined RTEMS || defined (STRSOE)	/****	RTEMS or STRSOE	     ****/
typedef unsigned long		n_long;	/*	long as rec'd from net	*/
extern int			rtems_shell_main_cp(int argc, char *argv[]);

#define	O_LARGEFILE		(0)
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
#include <math.h>
#include <stdarg.h>
			/* POSIX.1 */
#ifndef ION4WIN			/*	No POSIX in Visual Studio.	*/
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>
#endif				/*	end of #ifndef ION4WIN		*/

#ifdef ION4WIN			/*	Visual Studio provides most.	*/

#include <sys/types.h>

#elif defined(mingw)		/****   Windows vs all others	*********/
#include <winsock2.h>
#include <process.h>
#include <Winbase.h>
#include <sys/time.h>
#include <sys/types.h>
#include <ws2tcpip.h>
#include <winerror.h>
  
#ifndef EMSGSIZE
#define EMSGSIZE WSAEMSGSIZE
#endif

#define MAXHOSTNAMELEN		256
#ifndef SOCK_CLOEXEC
#define SOCK_CLOEXEC		0
#endif
#ifndef ECONNREFUSED
#define	ECONNREFUSED		WSAECONNREFUSED
#endif
#ifndef ECONNRESET
#define ECONNRESET		WSAECONNRESET
#endif
#ifndef EWOULDBLOCK
#define EWOULDBLOCK		WSAEWOULDBLOCK
#endif
#ifndef SIGCONT
#define SIGCONT             0
#endif
#ifndef ENETUNREACH
#define ENETUNREACH		WSAENETUNREACH
#endif
#ifndef EHOSTUNREACH
#define EHOSTUNREACH		WSAEHOSTUNREACH
#endif
#define	O_LARGEFILE		0
#define	inet_pton(a,b,c)	InetPton(a,b,c)
#define	inet_ntop(a,b,c,d)	InetNtop(a,b,c,d)

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
#define	SD_BOTH			SHUT_RDWR

#endif				/*	end of #ifdef ION4WIN		*/

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
#define UNUSED			__attribute__((unused))
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
#define LONG_MAX		(0x7fffffffL)
#elif defined (_LP64)
#define LONG_MAX		(0x7fffffffffffffffL)
#elif (SIZEOF_LONG == 4)
#define LONG_MAX		(0x7fffffffL)
#elif (SIZEOF_LONG == 8)
#define LONG_MAX		(0x7fffffffffffffffL)
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
	uvast			opaque[12];
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

/*	Need MAXPATHLEN defined for Visual Studio compile.		*/

#ifdef ION4WIN
#ifdef PATH_MAX
#define MAXPATHLEN PATH_MAX
#else
#define MAXPATHLEN 260
#endif
#endif				/*	end of #ifdef ION4WIN		*/

/*	Configure for platform-specific headers and IPC services.	*/

#ifndef ION4WIN			/*	None of these apply in VS.	*/

#define SVR4_SEMAPHORES		/****	default			*********/
#define SVR4_SHM		/****	default			*********/
#define	UNIX_TASKS		/****	default			*********/

#ifdef VXWORKS			/****	VxWorks			*********/

#undef	SVR4_SHM
#define RTOS_SHM

#undef	SVR4_SEMAPHORES
#define VXWORKS_SEMAPHORES

#undef	UNIX_TASKS
#define VXWORKS_TASKS

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

#if defined (RTEMS) || defined (STRSOE)	/****	RTEMS or STRSOE	*********/

#undef	SVR4_SHM
#define RTOS_SHM

#undef	SVR4_SEMAPHORES
#define POSIX_SEMAPHORES

#undef	UNIX_TASKS
#define POSIX_TASKS

typedef void	(*FUNCPTR)(int, int, int, int, int, int, int, int, int, int);

#ifndef PRIVATE_SYMTAB
#define PRIVATE_SYMTAB
#endif

#ifndef STRSOE
#include <bsp.h>
#include <rtems.h>
#endif
#include <pthread.h>
#include <pwd.h>
#include <netdb.h>
#include <mqueue.h>
#include <strings.h>
#include <sys/utsname.h>
#include <sys/param.h>		/****	...to get MAXHOSTNAMELEN	*/
#include <sys/resource.h>
#include <sys/time.h>
#include <sys/select.h>

#define	_MULTITHREADED		/*	To pick up resource lock code.	*/

#endif				/****	End of #ifdef (RTEMS)	     ****/

#ifdef mingw			/****	Windows			     ****/

#undef	SVR4_SHM
#define MINGW_SHM

#undef	SVR4_SEMAPHORES
#define MINGW_SEMAPHORES
#ifndef SEMMNS
#define	SEMMNS			32000	/*	Max. nbr of semaphores	*/
#endif

#undef	UNIX_TASKS
#define MINGW_TASKS

#include <pthread.h>
int pthread_setname_np(pthread_t thread, const char *name);
#include <stdint.h>

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

/*	IPC tracking operations		*/
#define WIN_STOP_ION		0
#define WIN_NOTE_SM		1
#define WIN_NOTE_SEMAPHORE	2
#define WIN_FORGET_SM		3
#define WIN_FORGET_SEMAPHORE	4

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

#ifdef AESCFS
#undef	UNIX_TASKS
#define POSIX_TASKS

typedef void	(*FUNCPTR)(int, int, int, int, int, int, int, int, int, int);
#endif				/*	End of #ifdef AESCFS	     ****/

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
int pthread_setname_np(pthread_t thread, const char *name);

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
int pthread_setname_np(pthread_t thread, const char *name);

#ifdef bionic			/****	Bionic subset of Linux      ****/

#undef	SVR4_SHM
#define RTOS_SHM

#undef	SVR4_SEMAPHORES
#define POSIX_SEMAPHORES

#undef	UNIX_TASKS
#define POSIX_TASKS

typedef void	(*FUNCPTR)(saddr, saddr, saddr, saddr, saddr, saddr, saddr,
			saddr, saddr, saddr);

#include <sys/param.h>		/****	...to get MAXPATHLEN         ****/

#ifndef SEM_NSEMS_MAX
#define	SEM_NSEMS_MAX		256
#endif

#define PRIVATE_SYMTAB

#else				/****	Not bionic		     ****/
#include <asm/param.h>		/****	...to get MAXHOSTNAMELEN     ****/
#include <sys/param.h>		/****	...to get MAXPATHLEN	     ****/
#ifndef uClibc			/****	uClibc subset of Linux	     ****/
#include <execinfo.h>		/****	...to get backtrace	     ****/
#endif				/*	End of #ifndef uClibc	     ****/
#endif				/****	End of #ifdef bionic	     ****/

#define	_MULTITHREADED

#endif				/****	End of #ifdef linux	     ****/

#ifdef freebsd			/****	FreeBSD			     ****/

#include <sys/param.h>		/****	...to get MAXHOSTNAMELEN     ****/
#include <pthread.h>
int pthread_set_name_np(pthread_t thread, const char *name);

#define	_MULTITHREADED

#define	O_LARGEFILE	0

#endif				/****	End of #ifdef freebsd	     ****/

#ifdef darwin			/****	Mac OS X		     ****/

#include <sys/malloc.h>
#include <stdlib.h>
#include <sys/param.h>		/****	...to get MAXHOSTNAMELEN     ****/
#include <pthread.h>
int pthread_setname_np(const char *name);

#include <sys/msg.h>
#define	msgbuf		mymsg	/****	Mac OS X has no msgbuf,	but  ****/
				/****	it has mymsg (same thing).   ****/
#define	_MULTITHREADED

#define	O_LARGEFILE	0

#endif				/****	End of #ifdef darwin	     ****/

#endif				/****	End of #ifdef (__SVR4)       ****/

#endif				/****	End of #ifdef (unix)         ****/

#if defined (SVR4_SHM)
#include <sys/shm.h>
#endif

/*	Note: if we ever need POSIX shared-memory services, we
 *	need to #include <sys/mman.h>.					*/

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
#define SEMMNI			128
#endif				/****	End of #ifndef SEMMNI	     ****/

#ifndef SEMMSL			/****	SEMMSL			     ****/
#define SEMMSL			250
#endif				/****	End of #ifndef SEMMSL	     ****/

#ifndef SEMMNS			/****	SEMMNS			     ****/
#define SEMMNS			32000
#endif				/****	End of #ifndef SEMMNS	     ****/

#elif defined (POSIX_SEMAPHORES)

#include <semaphore.h>

#endif				/****	End #if defined SVR4_SEMAPHORES */

#endif				/****	End of #ifdef ION4WIN        ****/

#ifdef HAVE_VALGRIND_VALGRIND_H
#include "valgrind/valgrind.h"
#endif

#if (defined(AESCFS))
#define	FSWLOGGER
#define	FSWWATCHER
#define	FSWTIME
#define	FSWCLOCK
#define FSWLAN
#define FSWSCHEDULER
#define	FSWUSER
#include "ioncfs.h"
#endif

#ifndef	TRACK_MALLOC
#define	TRACK_MALLOC(x)
#endif

#ifndef	TRACK_FREE
#define	TRACK_FREE(x)
#endif

#ifndef	TRACK_BORN
#define	TRACK_BORN(x)
#endif

#ifndef	TRACK_DIED
#define	TRACK_DIED(x)
#endif

#ifndef	MAX_SRC_FILE_NAME
#define MAX_SRC_FILE_NAME	255
#endif

/*	Prototypes for standard ION platform functions.			*/

typedef void			(* Logger)(char *);
typedef void			(* Watcher)(char);

extern void			*acquireSystemMemory(size_t);
extern int			createFile(const char*, int);
extern char			*system_error_msg();
extern void			setLogger(Logger);
extern void			writeMemo(char *);
extern void			writeErrMemo(char *);
extern void			writeMemoNote(char *, char *);
extern void			setWatcher(Watcher);
extern void			iwatch(char);
extern void			snooze(unsigned int);
extern void			microsnooze(unsigned int);
extern void			getCurrentTime(struct timeval *);
extern unsigned long		getClockResolution();	/*	usec	*/
#if (defined(FSWLAN) || !(defined(ION_NO_DNS)))
extern unsigned int		getInternetAddress(char *);
extern char			*getInternetHostName(unsigned int, char *);
extern int			getNameOfHost(char *, int);
extern char			*getNameOfUser(char *);
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

#ifndef DEBUG_PRINT
#define DEBUG_PRINT		(0)
#endif
#ifndef DEBUG_PRINT_LOG
#define DEBUG_PRINT_LOG		(0)
#endif
extern void			debugPrint(const char *format, ...);

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

extern uvast			htonv(uvast hostvast);
extern uvast			ntohv(uvast netvast);

extern int			_isprintf(char *, int, char *, ...);
extern size_t			istrlen(const char *, size_t);
extern char			*istrcpy(char *, const char *, size_t);
extern char			*istrcat(char *, char *, size_t);
extern char			*igetcwd(char *, size_t);
extern void			isignal(int, void (*)(int));
extern void			iblock(int);
extern int			ifopen(const char *, int, int);
extern char			*igets(int, char *, int, int *);
extern int			iputs(int, char *);

extern void			icopy(char *fromPath, char *toPath);

extern unsigned int		getAddressOfHost();
extern char			*addressToString(struct in_addr, char *buf);
extern int			parseSocketSpec(char *socketSpec,
					unsigned short *portNbr,
					unsigned int *ipAddress);
extern void			printDottedString(unsigned int hostNbr,
					char *buffer);

extern int			itcp_connect(char *socketSpec,
					unsigned short defaultPort, int *sock);
extern int			itcp_send(int *sock, char *from, int length);
extern int			itcp_recv(int *sock, char *into, int length);
extern void			itcp_handleConnectionLoss();

extern int			fullyQualified(char *fileName);
extern int			qualifyFileName(char *fileName, char *buffer,
					int buflen);
extern void			findToken(char **cursorPtr, char **token);
#include "platform_sm.h"

#ifdef __cplusplus
}
#endif

#ifndef MAXPATHLEN
#define MAXPATHLEN 200 // debug
#endif

#endif  /* _PLATFORM_H_ */
