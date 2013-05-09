# configure.ac for ION
#
# David Young
# Feb 12, 2008
#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

#is this high a version ok? it was 2.61 in ubuntu, but that was unavailable to
# fink, so we dropped it down
AC_PREREQ(2.60)
AC_INIT(ion, open source 3.1.2, http://korgano.eecs.ohiou.edu/mailman/listinfo/ion-bugs)
IS_NASA_B=0

#  Josh Schendel removed the -Werror on 05/15/2012 per issue #355 to deal with
#  all of the portability warnings introduced by autoconf v2.69 and
#  automake v1.12.
#AM_INIT_AUTOMAKE([subdir-objects -Wall -Werror foreign])
AM_INIT_AUTOMAKE([subdir-objects -Wall foreign])

# shall we put extra scripts in a special directory?
# AC_CONFIG_AUX_DIR(build-aux)
# some source file that will ensure that you are in the right directory.
AC_CONFIG_SRCDIR([ici/include/ion.h])
AC_CONFIG_HEADER([config.h])

# More issue #355 changes
AC_CONFIG_MACRO_DIR([m4])
LT_INIT

#
# allow the user running configure to disable ACS
#
AC_ARG_ENABLE(
    acs,
    AC_HELP_STRING([--disable-acs], [disable Aggregate Custody Signals]),
    [],
    [CFLAGS="-DENABLE_BPACS $CFLAGS"
     ENABLE_BPACS=yes])
AM_CONDITIONAL(ENABLE_BPACS, test "x$ENABLE_BPACS" = "xyes")

#
# allow the user running configure to disable IMC
#
AC_ARG_ENABLE(
    imc,
    AC_HELP_STRING([--disable-imc], [disable Interplanetary Multicast]),
    [],
    [CFLAGS="-DENABLE_IMC $CFLAGS"
     ENABLE_IMC=yes])
AM_CONDITIONAL(ENABLE_IMC, test "x$ENABLE_IMC" = "xyes")



# Checks for programs.
AC_PROG_CC
AC_PROG_LIBTOOL
AC_PROG_INSTALL
AC_PROG_MAKE_SET
# it should set pod_documentation to 1 if pod2man exists. 
# you could then build documentation or not.
AC_CHECK_PROGS([POD_DOCUMENTATION], [pod2man pod2html], [1] )
if test "$POD_DOCUMENTATION" = 1; then
#	echo "------------------- WARNING -------------------"
	echo "ERROR: You need to have pod2man for documentation."
#	echo "-----------------------------------------------"
#	sleep 3
	exit -1
#else
#	AC_SUBST([MAKE_POD_MANS],'doc')
# if pod2man is present, then this 'doc' variable is used in ici's makefile as a
# subdir (otherwise ici/doc is ignored)
fi

# Checks for libraries.
# these are libraries that ALREADY EXIST BEFORE BUILDING
AC_CHECK_LIB(
	[expat], 
	[XML_GetCurrentLineNumber], 
	[ AC_SUBST( [EXPAT_LIBS], ["-lexpat"] ) ],
	[AC_MSG_ERROR([You need to have the Expat XML development library, libexpat1-dev])]
)

AC_CHECK_LIB(
	[pthread], 
	[pthread_create], 
	[ AC_SUBST( [PTHREAD_LIBS], ["-lpthread"] ) ],
	[AC_MSG_ERROR([You need pthread library.]) ]
)


#
# If valgrind is present, allow special MTAKE/MRELEASE valgrind tutors.
#
AC_ARG_ENABLE([valgrind], AC_HELP_STRING([--disable-valgrind],[Do not enable valgrind debugging [default=check]]),
              [ ], [ enable_valgrind=check ])

if test "x$enable_valgrind" != xno; then
  AC_CHECK_HEADER(valgrind/valgrind.h, 
                  [AC_DEFINE([HAVE_VALGRIND_VALGRIND_H], [1], [enable valgrind macros])
                   valgrind_present=yes],
                  [valgrind_present=no])

  if test "x$enable_valgrind" == xyes && test "x$valgrind_present" == xno; then
    # User specifically requested valgrind but we can't find it.
    AC_MSG_ERROR([valgrind/valgrind.h not found but --enable-valgrind requested])
  fi

  if test "x$valgrind_present" == xyes; then
    # Valgrind < 3.6.1-1 (Debian/Ubuntu) and gcc >= 4.6 triggers "unused-but-set-variable"
    # warnings in the valgrind macros.  This version is present in debian unstable
    # 10/11, and Ubuntu 11.10.
    AC_MSG_CHECKING([if valgrind is compatible with -Wunused-but-set])
    ORIGCFLAGS=$CFLAGS
    CFLAGS="$CFLAGS -Wall -Werror"
    AC_COMPILE_IFELSE(
    [AC_LANG_PROGRAM([[#include <valgrind/valgrind.h>]],
                     [[VALGRIND_MALLOCLIKE_BLOCK(1, 100, 0, 1);
                       VALGRIND_FREELIKE_BLOCK(1, 0);]])],
    [AC_MSG_RESULT([yes]); valgrind_wunused_but_set_compat=yes],
    [AC_MSG_RESULT([no]); valgrind_wunused_but_set_compat=no])
    CFLAGS="$ORIGCFLAGS"

    # If not compatible with -Wset-but-not-used, then disable this warning.
    if test "x$valgrind_wunused_but_set_compat" == "xno"; then
      AC_MSG_NOTICE([adding -Wno-unused-but-set-variable to VALGRIND_COMPAT_CFLAGS])
      AC_SUBST([VALGRIND_COMPAT_CFLAGS], [-Wno-unused-but-set-variable])
    fi
  fi
fi

if test "$IS_NASA_B" = "0"; then
	# Control whether AMS debugging info is printed.
	# Default is that the AMS debugging info is not printed.
	# To enable AMS debugging info, use "./configure --enable-ams-debug"
	AC_ARG_ENABLE([ams-debug],
		[  --enable-ams-debug    Enable AMS debugging info],
		[ AC_DEFINE([AMSDEBUG], 1, [Enable AMS debugging info]) ])
fi

if test "$IS_NASA_B" = "1"; then
	# To Disable crypto, use "./configure --disable-crypto"
	AC_ARG_ENABLE([crypto],
		[  --disable-crypto    Turn off cryptography],
		[case "${enableval}" in
			yes) crypto=true ;;
			no)  crypto=false ;;
		*) AC_MSG_ERROR([bad value ${enableval} for --disable-crypto]) ;;
		esac],[crypto=true])
	
	AM_CONDITIONAL([CRYPTO], [test x$crypto = xtrue])

	AC_DEFINE([NASA_PROTECTED_FLIGHT_CODE],[1],[Build ION NASA protected flight code])
else
	AM_CONDITIONAL([CRYPTO], [false])
fi

AM_CONDITIONAL([ION_NASA_B],[test "$IS_NASA_B" = "1"])

# Dynamically choose whether or not to instrument code for gcov code coverage
# Default is not to instrument the code for gcov code coverage.
# To use gcov code coverage instrumentation, use "./configure --enable-gcov"
AC_ARG_WITH([gcov],
[AS_HELP_STRING([--with-gcov], [Turn on code coverage instrumentation])],
[gcov=yes;],
[])
if test "x$gcov" = "xyes" ; then
	echo "Instrument code for gcov coverage analysis... yes"
	CFLAGS="${CFLAGS} -g -O0 -fprofile-arcs -ftest-coverage"
	LDFLAGS="${LDFLAGS} -fprofile-arcs -ftest-coverage -lgcov"
else
	echo "Instrument code for gcov coverage analysis... no"
fi
AM_CONDITIONAL([ENABLE_GCOV], [test "x$gcov" = "xyes"])


# Find tools for compiling documentation.
AC_PATH_PROG([PS2PDF], [ps2pdf])
AC_PATH_PROG([PDF2PS], [pdf2ps])
AC_PATH_PROG([PSJOIN], [psjoin])
AC_MSG_CHECKING([for fallback included psjoin])
if test -x "$PWD/doc/psjoin"; then
    PSJOIN="$PWD/doc/psjoin"
    AC_MSG_RESULT($PSJOIN)
else
    AC_MSG_RESULT([not found])
fi
AC_PATH_PROG([GROFF], [groff])
AC_MSG_CHECKING([if groff suports -ms])
if test "$GROFF" && echo | $GROFF -ms - >/dev/null 2>/dev/null; then
    AC_MSG_RESULT($GROFF)
    GROFFMS="$GROFF"
else
    AC_MSG_RESULT([not found])
    AC_MSG_NOTICE([try installing groff and the ms plugin])
fi
AC_SUBST([GROFFMS], $GROFFMS)

AC_PATH_PROG([MANOPTS], [man])
AC_MSG_CHECKING([if man supports -l])
if test "$MANOPTS" && echo | $MANOPTS -l - >/dev/null 2>/dev/null; then
    AC_MSG_RESULT($MANOPTS)
else
    MANOPTS=""
    AC_MSG_RESULT([not found])
    AC_MSG_NOTICE([try installing a version of man that supports -l])
fi
AC_SUBST([MANOPTS], $MANOPTS)

AC_MSG_CHECKING([for tools to build autodoc])
if test "$PS2PDF" && test "$PDF2PS" && test "$PSJOIN" && test "$GROFFMS" && test -n "$MANOPTS"; then
    AC_MSG_RESULT([found])
    buildautodoc="yes"
else
    AC_MSG_RESULT([not found (skipping autodoc)])
    AC_MSG_NOTICE([Ensure the ghostscript, psutils, groff-base and groff packages are installed.])
fi
AM_CONDITIONAL([ENABLE_AUTODOC], [ test "$buildautodoc" ]) 

# Checks for header files.
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS([fcntl.h malloc.h netdb.h netinet/in.h stddef.h stdlib.h string.h sys/ioctl.h sys/param.h sys/socket.h sys/time.h unistd.h])

m4_include([./dccp_macro.m4])

# Checks for typedefs, structures, and compiler characteristics.
case $host_os in
	darwin*)
		AC_DEFINE([darwin],[1],[Build Darwin specific platform code.])
		AC_DEFINE([unix],[1],[Build Unix specific platform code.]) 
		AC_SUBST([ION_LINK_FLAGS]," -dynamiclib -undefined dynamic_lookup")
		if test $host_cpu = "x86_64" ; then
			AC_SUBST([ION_CFLAGS]," -Dunix -Ddarwin -DSPACE_ORDER=3 -m64 ")
			echo "Build 64-bit Darwin specific platform code."
		else
			AC_SUBST([ION_CFLAGS]," -Dunix -Ddarwin -DSPACE_ORDER=2 -m32 ")
			echo "Build 32-bit Darwin specific platform code."
		fi
		;;
	linux*) 
		#check for DCCP support
		CHECK_DCCP()

		if test $host_cpu = "x86_64" ; then
			AC_DEFINE([linux],[1],[Build 64-bit Linux specific platform code.]) 
			AC_SUBST([ION_CFLAGS]," -Dlinux -DSPACE_ORDER=3 -fno-strict-aliasing")
			echo "Build 64-bit Linux specific platform code."
		else
			AC_DEFINE([linux],[1],[Build 32-bit Linux specific platform code.]) 
			AC_SUBST([ION_CFLAGS]," -Dlinux -fno-strict-aliasing")
			echo "Build 32-bit Linux specific platform code."
		fi
		;;

	cygwin*)  AC_DEFINE([cygwin],[1],[Build Cygwin specific platform code.]) ;;
	interix*) AC_DEFINE([interix],[1],[Build Interix specific platform code.]) ;;
  *bsd*)    
    AC_DEFINE([freebsd],[1],[Build Freebsd specific platform code.])
    if test $host_cpu = "x86_64" ; then
      AC_SUBST([ION_CFLAGS]," -Dfreebsd -DSPACE_ORDER=3 -m64 ")
      echo "Build 64-bit Freebsd specific platform code."
    else
      AC_SUBST([ION_CFLAGS]," -Dfreebsd -DSPACE_ORDER=2 -m32 ")
      echo "Build 32-bit Freebsd specific platform code."
    fi

    ;;
	solaris*) 
		if test $host_cpu = "sparc" ; then
			AC_DEFINE([solaris],[1],[Build Solaris specific platform code.])
			AC_SUBST([ION_LINK_FLAGS],"-lrt -lsocket -lnsl")
			AC_SUBST([ION_CFLAGS]," -Dsolaris -Dunix -D__SVR4 -D_REENTRANT -fno-strict-aliasing -fno-builtin -DSPACE_ORDER=3 -m64")
			echo "Build Solaris/SPARC specific platform code."
		else
			AC_DEFINE([solaris],[1],[Build Solaris specific platform code.])
			AC_SUBST([ION_LINK_FLAGS],"-lrt -lsocket -lnsl")
			AC_SUBST([ION_CFLAGS]," -Dsolaris -Dunix -D__SVR4 -D_REENTRANT -fno-strict-aliasing -DSPACE_ORDER=3 -m64")
			echo "Build Solaris specific platform code."
		fi
		;;
	vxworks*) AC_DEFINE([vxworks],[1],[Build VXWorks specific platform code.]) ;;
esac

AC_C_CONST
AC_TYPE_SIZE_T
AC_HEADER_TIME
AC_STRUCT_TM
AC_TYPE_UID_T

# Checks for library functions.
# commented out library functions that should exist everywhere.
# otherwise, autoconf insisted that malloc.c, mktime.c were included in distro...
AC_FUNC_FORK
AC_PROG_GCC_TRADITIONAL
#AC_FUNC_MALLOC
#AC_FUNC_MEMCMP
#AC_FUNC_MKTIME
#AC_FUNC_SELECT_ARGTYPES
AC_TYPE_SIGNAL
#AC_FUNC_STAT
#AC_FUNC_STRFTIME
AC_CHECK_FUNCS([alarm ftruncate getcwd gethostbyaddr gethostbyname gethostname gettimeofday memset mkdir mkfifo rmdir select socket strcasecmp strchr strerror strncasecmp strstr strtoul uname])

#ALL_INCLUDES a huge list of all includable directories.  It helps you from
# having to type too much in the individual makefile.am's
#AC_SUBST([ALL_INCLUDES],'-include config.h -I$(srcdir)/ici/include -I$(srcdir)/ltp/include -I$(srcdir)/dgr/include -I$(srcdir)/bp/include -I$(srcdir)/ams/include -I$(srcdir)/icix/include')
#AC_SUBST([ALL_LIB_PATHS],'-L$(srcdir)/ici -L$(srcdir)/dgr -L$(srcdir)/ltp -L$(srcdir)/bp')
# add in the locations for future makefiles (you must have one Makefile.am in each location)
AC_CONFIG_FILES([Makefile])
	#	ici/Makefile
	#	ici/doc/Makefile
	#	ltp/Makefile
	#	ltp/doc/Makefile
#		dgr/Makefile
#		dgr/doc/Makefile
#		bp/Makefile
#		bp/doc/Makefile
#		ams/Makefile
#		icix/Makefile
#		icix/doc/Makefile
AC_OUTPUT
