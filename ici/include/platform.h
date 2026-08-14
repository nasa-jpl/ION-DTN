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

#ifndef PLATFORM_H
#define PLATFORM_H


#ifdef __cplusplus
extern "C" {
#endif

#if defined(FORCE_SVR4_SEMAPHORES) && defined(FORCE_POSIX_NAMED_SEMAPHORES)
#error Both FORCE_SVR4_SEMAPHORES and FORCE_POSIX_NAMED_SEMAPHORES defined - pick one
#endif

#if defined (VXWORKS) || defined (RTEMS) || defined (bionic) || defined (AESCFS) || defined (STRSOE)
#define ION_LWT
#else
#undef ION_LWT
#endif

/* Feature test macros - must come before any system header */
#ifdef RTEMS
#define __BSD_VISIBLE 1
#define __MISC_VISIBLE 1
#include <sys/types.h>
#endif

/* Feature test macros for Unix - must come before any system header */
#ifdef __unix__			/****	Feature test macros for UNIX	****/

/* Check FreeBSD and Solaris first to exempt them from strict POSIX */
#if !defined(freebsd) && !defined(solaris)
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L	/****	POSIX.1-2008 functions ****/
#endif
#endif

#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE		/****	glibc default functions ****/
#endif

#ifdef __linux__
#ifndef _BSD_SOURCE
#define _BSD_SOURCE
#endif
#ifndef _SVID_SOURCE
#define _SVID_SOURCE
#endif
#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 700
#endif
#endif

#ifdef freebsd
#ifndef __BSD_VISIBLE
#define __BSD_VISIBLE 1
#endif
/* Ensure all BSD extensions are available with C11 */
#ifndef _BSD_SOURCE
#define _BSD_SOURCE
#endif
/* Use XPG6/UNIX 03 for broader compatibility than strict POSIX.1-2008 */
#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 600
#endif
/* Ensure gettimeofday and other time functions are available */
#ifndef _XOPEN_SOURCE_EXTENDED
#define _XOPEN_SOURCE_EXTENDED 1
#endif
#endif

#ifdef darwin
#ifndef _DARWIN_C_SOURCE
#define _DARWIN_C_SOURCE
#endif
#ifndef _BSD_SOURCE
#define _BSD_SOURCE
#endif
/* Ensure BSD types (u_char, u_short, etc.) are available */
#include <sys/types.h>
#endif

#endif				/****	End of feature test macros	for __unix__ ****/

/* Feture test macro for Solaris */
#ifdef solaris
#ifndef __EXTENSIONS__
#define __EXTENSIONS__		/****	Solaris extensions ****/
#endif
#ifndef _POSIX_PTHREAD_SEMANTICS
#define _POSIX_PTHREAD_SEMANTICS	/****	POSIX pthreads ****/
#endif
/* Ensure clock_gettime is available in C11 mode */
#ifndef _POSIX_TIMERS
#define _POSIX_TIMERS
#endif
/* Alternative approach - force XPG6 compliance */
#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 600
#endif
#endif /* solaris */



#ifdef uClibc
#ifndef __linux__
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
#define	ADDR_FIELDSPEC_INT	"%lu"
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
#define	VAST_FIELDSPEC		"%lld"
#define	UVAST_FIELDSPEC		"%llu"
#define UVAST_HEX_FIELDSPEC	"%llx"
#define	ADDR_FIELDSPEC		"%#lx"
#define	ADDR_FIELDSPEC_INT	"%lu"
#define ilseek(a, b, c)		lseek(a, b, c)
#define	strtovast(x)		strtoll(x, NULL, 0)
#define	strtouvast(x)		strtoull(x, NULL, 0)
#define	strtoaddr(x)		strtoul(x, NULL, 0)
#define LARGE1			1UL
#else			/*	64-bit machines.			*/
typedef long			vast;
typedef unsigned long		uvast;
typedef long			saddr;	/*	Pointer-sized integer.	*/
typedef unsigned long		uaddr;	/*	Pointer-sized integer.	*/
#define	VAST_FIELDSPEC		"%ld"
#define	UVAST_FIELDSPEC		"%lu"
#define UVAST_HEX_FIELDSPEC	"%lx"
#define	ADDR_FIELDSPEC		"%#lx"
#define	ADDR_FIELDSPEC_INT	"%lu"
#define ilseek(a, b, c)		lseek(a, b, c)
#define	strtovast(x)		strtol(x, NULL, 0)
#define	strtouvast(x)		strtoul(x, NULL, 0)
#define	strtoaddr(x)		strtoul(x, NULL, 0)
#define LARGE1			1UL
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
#define CORE_FILE_NEEDED	(1)
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

/* Add headers for getaddrinfo on Linux, FreeBSD, macOS, RTEMS */
#if defined(__linux__) || defined(freebsd) || defined(darwin) || defined(RTEMS)
#include <netdb.h>
#include <sys/socket.h>
#endif

/* POSIX.1 */
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>

#ifdef freebsd
#include <sys/types.h>
#endif

#include <sys/times.h>
#include <limits.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#ifndef RTEMS
#include <netinet/ip.h>
#include <netinet/udp.h>
#endif

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

/*
** End of Standard Headers
*/

/*	Handy definitions that are mostly platform-independent.		*/

#define itoa			iToa
#define utoa			uToa
#define vasttoa			vastToa
#define uvasttoa		uvastToa
#define sizetoa		sizeToa

#ifdef ERROR
#undef ERROR
#endif
#define ERROR			(-1)

/* Feature Test for Thread-Local Storage Support */
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L && !defined(__STDC_NO_THREADS__)
	#include <threads.h>
	#define ION_THREAD_LOCAL thread_local
#else
	#define ION_THREAD_LOCAL __thread
#endif

/*
 * Safely leverages compiler static analysis for format strings across all
 * toolchains without breaking strict ISO C or POSIX compliance.
 * This is exclusively a diagnostic build-time tool; it does not interject
 * any runtime dependencies.
 */
#if defined(__GNUC__) || defined(__clang__)
#define ION_FORMAT_PRINTF(fmt_arg, first_vararg) __attribute__((format(printf, fmt_arg, first_vararg)))
#else
#define ION_FORMAT_PRINTF(fmt_arg, first_vararg)
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

#ifdef TORNADO_2_0_2
/* Tornado uses older GCC-style variadic macros */
#define isprintf(buffer, bufsize, format, args...)	\
oK(_isprintf(__FILE__, __LINE__, buffer, bufsize, format, args))
#else
/* Strict C99 / C18 / POSIX compliant macro */
#define isprintf(buffer, bufsize, ...)		\
oK(_isprintf(__FILE__, __LINE__, buffer, bufsize, __VA_ARGS__))
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

/*	NON_INTERACTIVE: When defined, disables interactive (stdin-based)
 *	command loops in admin utilities and test programs. This switch
 *	is independent of FSWLOGGER; define it explicitly when stdin is
 *	not available (e.g., embedded systems without a console).	*/

/*	Configure for platform-specific headers and IPC services.	*/

#define POSIX_NAMED_SEMAPHORES	/****	default			*********/
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
#undef	POSIX_NAMED_SEMAPHORES
#define POSIX_SEMAPHORES

#undef	UNIX_TASKS
#define POSIX_TASKS

/*
 * Args are pointer-sized (saddr), not int: pseudoshell passes task argument
 * string pointers through here, and on 64-bit RTEMS targets (e.g. riscv rv64,
 * aarch64) int would truncate them.
 */
typedef void (*FUNCPTR)(saddr, saddr, saddr, saddr, saddr, saddr, saddr, saddr,
		saddr, saddr);

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


#ifdef __unix__			/****	All UNIX platforms	     ****/

/*
** *NIX Headers: Common to All Supported *NIX Platforms
*/
#include <sys/utsname.h>
#include <pwd.h>
#include <netdb.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <pthread.h>
/*
** End of *NIX Headers
*/

#ifdef AESCFS
#undef	UNIX_TASKS
#define POSIX_TASKS

typedef void	(*FUNCPTR)(saddr, saddr, saddr, saddr, saddr, saddr, saddr,
			saddr, saddr, saddr);
#endif				/*	End of #ifdef AESCFS	     ****/

#ifdef __SVR4			/****	All Sys 5 Rev 4 UNIX systems ****/

#include <sys/param.h>		/****	...to get MAXPATHLEN         ****/

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

#ifdef solaris			/****	Solaris (SunOS 5+)	     ****/
#include <ucontext.h>		/****	For printstack() on Solaris  ****/

/* semaphore options */
/* POSIX_NAMED_SEMAPHORES are the default on Solaris */
#undef	SVR4_SEMAPHORES
#undef  POSIX_SEMAPHORES
#define  POSIX_NAMED_SEMAPHORES
#ifdef FORCE_SVR4_SEMAPHORES
/* but SVR4_SEMAPHORES are also still supported on Solaris */
#define	SVR4_SEMAPHORES
#undef  POSIX_SEMAPHORES
#undef  POSIX_NAMED_SEMAPHORES
#elif defined(FORCE_POSIX_NAMED_SEMAPHORES)
/* but POSIX_NAMED_SEMAPHORES have been tested to be faster on Solaris */
#undef	SVR4_SEMAPHORES
#undef  POSIX_SEMAPHORES
#define POSIX_NAMED_SEMAPHORES
#endif /* FORCE_SVR4_SEMAPHORES */
#ifdef  POSIX_NAMED_SEMAPHORES
#ifdef  DEBUG_POSIX_NAMED_SEMAPHORES
#pragma message("**  Using NEW Posix Named Semaphores on Solaris")
#endif  /* DEBUG_POSIX_NAMED_SEMAPHORES */
#endif  /* POSIX_NAMED_SEMAPHORES */


#ifndef SEM_NSEMS_MAX
// larger because these are global on the node across ALL Ion instances - 256 is fine for a single instance
#define	SEM_NSEMS_MAX		8192
#endif

#endif				/****	End of #ifdef solaris	     ****/


#else				/****	Not __SVR4 at all (BSD?)     ****/

#define FIFO_READ_MODE          (O_RDWR)
#define FIFO_WRITE_MODE         (O_RDWR)
#define	FDTABLE_SIZE		(getdtablesize())

#ifdef __linux__			/****	Linux			     ****/

#include <malloc.h>

#include <pthread.h>
int pthread_setname_np(pthread_t thread, const char *name);


/* semaphore options */
/* POSIX_NAMED_SEMAPHORES is the default on Linux */
#undef	SVR4_SEMAPHORES
#undef  POSIX_SEMAPHORES
#define  POSIX_NAMED_SEMAPHORES
#ifdef FORCE_SVR4_SEMAPHORES
/* not the default, but SVR4_SEMAPHORES are also supported on Linux */
#define	SVR4_SEMAPHORES
#undef  POSIX_SEMAPHORES
#undef  POSIX_NAMED_SEMAPHORES
#elif defined(FORCE_POSIX_NAMED_SEMAPHORES)
/* FORCE_POSIX_NAMED_SEMAPHORES is the default on Linux */
#undef	SVR4_SEMAPHORES
#undef  POSIX_SEMAPHORES
#define POSIX_NAMED_SEMAPHORES
#endif /* FORCE_SVR4_SEMAPHORES */
#ifdef  POSIX_NAMED_SEMAPHORES
#ifdef  DEBUG_POSIX_NAMED_SEMAPHORES
#pragma message("**  Using NEW Posix Named Semaphores on Linux")
#endif  /* DEBUG_POSIX_NAMED_SEMAPHORES */
#endif  /* POSIX_NAMED_SEMAPHORES */

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

/* allow the default to be overwritten */
#ifndef SEM_NSEMS_MAX
// larger because these are global on the node across ALL Ion instances - 256 is fine for a single instance
#define	SEM_NSEMS_MAX		8192
#endif

#include <asm/param.h>		/****	...to get MAXHOSTNAMELEN     ****/
#include <sys/param.h>		/****	...to get MAXPATHLEN	     ****/
#endif				/****	End of #ifdef bionic	     ****/

#define	_MULTITHREADED

#endif				/****	End of #ifdef __linux__	     ****/

#ifdef freebsd			/****	FreeBSD			     ****/

#include <sys/param.h>		/****	...to get MAXHOSTNAMELEN     ****/
#include <pthread.h>
int pthread_set_name_np(pthread_t thread, const char *name);

#define	_MULTITHREADED

#define	O_LARGEFILE	0

/* semaphore options */
/* POSIX_NAMED_SEMAPHORES is the default on FreeBSD */
#undef  POSIX_SEMAPHORES
#undef  SVR4_SEMAPHORES
#define  POSIX_NAMED_SEMAPHORES

/* ADD THIS SECTION: */
#ifndef SEM_NSEMS_MAX
#define	SEM_NSEMS_MAX		8192
#endif


#endif				/****	End of #ifdef freebsd	     ****/


#ifdef darwin			/****	Mac OS X		     ****/

#include <sys/malloc.h>
#include <stdlib.h>
#include <sys/param.h>		/****	...to get MAXHOSTNAMELEN     ****/
#include <pthread.h>

/* semaphore options */
/* POSIX_NAMED_SEMAPHORES is the default on MacOS */
#define POSIX_NAMED_SEMAPHORES
#undef	SVR4_SEMAPHORES
#undef POSIX_SEMAPHORES
#ifdef FORCE_SVR4_SEMAPHORES
/* NOT the default on darwin/MacOS, but FORCE_SVR4_SEMAPHORES are also supported */
#define	SVR4_SEMAPHORES
#undef  POSIX_SEMAPHORES
#undef  POSIX_NAMED_SEMAPHORES
#elif defined(FORCE_POSIX_NAMED_SEMAPHORES)
/* POSIX_NAMED_SEMAPHORES is the default on MacOS */
#undef	SVR4_SEMAPHORES
#undef  POSIX_SEMAPHORES
#define POSIX_NAMED_SEMAPHORES
#endif /* FORCE_SVR4_SEMAPHORES */
#ifdef  POSIX_NAMED_SEMAPHORES
#ifdef  DEBUG_POSIX_NAMED_SEMAPHORES
#pragma message("**  Using NEW Posix Named Semaphores on MacOS")
#endif  /* DEBUG_POSIX_NAMED_SEMAPHORES */
#endif  /* POSIX_NAMED_SEMAPHORES */

/* allow the default to be overwritten */
#ifndef SEM_NSEMS_MAX
// larger because these are global on the node across ALL Ion instances - 256 is fine for a single instance
#define	SEM_NSEMS_MAX		8192
#endif

int pthread_setname_np(const char *name);

#include <sys/msg.h>
#define	msgbuf		mymsg	/****	Mac OS X has no msgbuf,	but  ****/
				/****	it has mymsg (same thing).   ****/
#define	_MULTITHREADED

#define	O_LARGEFILE	0

#endif				/****	End of #ifdef darwin	     ****/

#endif				/****	End of #ifdef (__SVR4)       ****/

#endif				/****	End of #ifdef (__unix__)         ****/

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

#elif defined (POSIX_NAMED_SEMAPHORES)

#include <semaphore.h>

#endif				/****	End #if defined SVR4_SEMAPHORES */


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

/**
 * ResourceLock: a platform-independent recursive mutex.
 *
 * The opaque[] buffer is sized to accommodate the largest `Rlock`
 * representation across all supported platforms:
 *
 *   - VxWorks / pre-C11 ION: `{ SEM_ID; int owner; short count; short init; }`
 *     (a few pointer/int words)
 *   - POSIX / glibc / musl:  `{ pthread_mutex_t mutex; int initialized; }`
 *     (pthread_mutex_t is 40 bytes on glibc x86_64, 48 bytes on RTEMS 5,
 *     up to ~160 bytes on RTEMS 6 AArch64)
 *
 * The size (192 bytes at opaque[24] on a 64-bit target) was chosen to
 * fit RTEMS 6's larger pthread_mutex_t.  A compile-time assertion in
 * `ici/library/platform.c` (`verify_sufficient_semaphore_space[]`)
 * fails the build if the chosen size is ever too small for the
 * target's Rlock layout, so the header will loudly refuse to compile
 * on a new platform rather than silently corrupting memory.
 *
 * ABI note: growing opaque[] changes the `sizeof(ResourceLock)` and
 * therefore the layout of any struct that embeds one.  ResourceLock
 * is only ever used in process-local memory — every instance in the
 * tree is either a function/file-scope static variable (see
 * errmsgsLock, memosLock, tasksLock, logFileLock, mibLock, gMemMutex)
 * or a field inside a struct allocated from a single daemon's heap
 * (IPND configuration/neighbours, NM vector/rhht/sql lock).  No
 * ResourceLock is embedded in SDR storage, in the SM working-memory
 * partition, or in any struct exchanged between processes, so a size
 * change only requires a clean rebuild of all ION libraries and
 * binaries, not a migration of any persistent state.
 *
 * Callers must therefore rebuild every `.a`, `.so`, and ION executable
 * whenever this size changes.  Linking a freshly compiled header
 * against a pre-built library with the old opaque[] size will produce
 * silent memory corruption.
 */
typedef struct
{
	uvast			opaque[24];
} ResourceLock;

/*	Prototypes for standard ION platform functions.			*/

typedef void			(* Logger)(char *);
typedef void			(* Watcher)(char *);

extern void			*acquireSystemMemory(size_t);
extern int			createFile(const char*, int);
extern char			*system_error_msg(void);
extern void			setLogger(Logger);
extern void			writeMemo(char *);
extern void			writeErrMemo(char *);
extern void			writeMemoNote(char *, char *);
extern void			setWatcher(Watcher);
extern void			iwatch(char);
extern void			iwatch_str(char *);
extern void			snooze(unsigned int);
extern void			microsnooze(unsigned int);
extern void			getCurrentTime(struct timeval *);
extern unsigned long		getClockResolution(void);	/*	usec	*/
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
extern char			*vasttoa(vast);
extern char			*uvasttoa(uvast);
extern char			*sizetoa(size_t);
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
extern void			writeErrmsgMemos(void);
extern void			discardErrmsgs(void);

#define iEnd(arg)		_iEnd(__FILE__, __LINE__, arg)
extern int			_iEnd(const char *, int, const char *);
extern int			_coreFileNeeded(int *);

/* check arg for NULL and return requested return value.  As side effect,   */
/* proves to the compiler that the pointer is not NULL afterward. 			*/
#define CHKERR(e)		if (!(e) && (iEnd(#e)||1)) return ERROR
#define CHKZERO(e)		if (!(e) && (iEnd(#e)||1)) return 0
#define CHKNULL(e)		if (!(e) && (iEnd(#e)||1)) return NULL
#define CHKVOID(e)		if (!(e) && (iEnd(#e)||1)) return

extern void			printStackTrace(void);

#ifndef DEBUG_PRINT
#define DEBUG_PRINT		(0)
#endif
#ifndef DEBUG_PRINT_LOG
#define DEBUG_PRINT_LOG		(0)
#endif
#ifndef DEBUG_RFX
#define DEBUG_RFX		(0)
#endif
extern void			debugPrint(const char *format, ...) ION_FORMAT_PRINTF(1, 2);

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
extern size_t			decodeSdnvBounded(uvast *, unsigned char *,
					size_t length);

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
extern void			scalarToSdnv(Sdnv *sdnv, Scalar *scalar);
extern int			sdnvToScalar(Scalar *scalar, unsigned char *sdnvText);

extern uvast			htonv(uvast hostvast);
extern uvast			ntohv(uvast netvast);

extern int			_isprintf(const char *, int, char *, int,
					const char *, ...)
					ION_FORMAT_PRINTF(5, 6);
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

extern unsigned int		getAddressOfHost(void);
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
extern void			itcp_handleConnectionLoss(int signum);

extern int			fullyQualified(char *fileName);
extern int			qualifyFileName(char *fileName, char *buffer,
					int buflen);
extern void			findToken(char **cursorPtr, char **token);

/*
 * Safe string-to-numeric parsers for configuration/CLI inputs.
 *
 * Each returns 0 on success and -1 on any error (NULL/empty input,
 * trailing garbage, ERANGE, or out-of-range for the destination type);
 * on error *result is left unmodified, so callers MUST check the return
 * value before using *result.
 *
 * Convention for narrower or unsigned destinations (unsigned int, short,
 * uint8_t, etc.): there is intentionally no per-type parser.  Parse into
 * the widest matching type, then bounds-check against the destination's
 * limit BEFORE downcasting, e.g.
 *
 *     uvast v;
 *     if (platform_parse_uvast(tok, &v) < 0 || v > UINT_MAX) { ...error... }
 *     duct->xmitRate = (unsigned int) v;
 *
 * This is the remediation pattern for CodeSonar "Coercion Alters Value"
 * findings at ION configuration inputs (see
 * https://github.com/nasa-jpl/ion-ios-dev/issues/526): convert with a
 * range-aware utility and reject values that cannot be represented in the
 * destination type, rather than silently truncating.
 */
extern int			platform_parse_uvast(const char *nptr, uvast *result);
extern int 			platform_parse_int(const char *nptr, int *result);
extern int			platform_parse_double(const char *str, double *result);

#include "platform_sm.h"

#ifdef __cplusplus
}
#endif

#ifndef MAXPATHLEN
#error "No value defined for MAXPATHLEN. Compiler invocation must supply preprocessor flags indicating the target platform, e.g. -Dlinux, -Dsolaris, etc... See configuration of AM_CFLAGS per 'host_os' in ./configure.ac in the ION source directory."
#endif

#endif /* PLATFORM_H */
