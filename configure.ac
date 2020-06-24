# configure.ac for ION
# Samuel Jero
# June 14, 2013
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.69)
AC_INIT(ion, open source 3.7.1/4.0.0, ion-dtn-support@lists.sourceforge.net)
IS_NASA_B=0

#  Josh Schendel removed the -Werror on 05/15/2012 per issue #355 to deal with
#  all of the portability warnings introduced by autoconf v2.69 and
#  automake v1.12.
#AM_INIT_AUTOMAKE([subdir-objects -Wall -Werror foreign])
AM_INIT_AUTOMAKE([subdir-objects no-dependencies -Wall foreign])

# some source file that will ensure that you are in the right directory.
AC_CONFIG_SRCDIR([ici/include/ion.h])
AC_CONFIG_HEADER([config.h])
AC_CONFIG_MACRO_DIR([m4])


# Checks for programs.
AC_PROG_CC
AM_PROG_CC_C_O
#Automake v1.12 wants AM_PROG_AR, but it doesn't exist <v1.12 so do this
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])
AC_PROG_LIBTOOL
AC_PROG_INSTALL
AC_PROG_MAKE_SET
LT_INIT


# Check for library functions
AC_CHECK_FUNCS([pow])


# Check Platform and Architecture
m4_include([./dccp_macro.m4])
AM_CFLAGS=""
AM_LDFLAGS=""
AM_LIBTOOLFLAGS=""
PLATFORM=""
case $host_os in
	darwin*)
		AC_DEFINE([darwin],[1],[Build Darwin specific platform code.])
		AC_DEFINE([unix],[1],[Build Unix specific platform code.])
		PLATFORM="darwin"
		if test $host_cpu = "x86_64" ; then
			AM_CFLAGS="$AM_CFLAGS -Dunix -Ddarwin -DSPACE_ORDER=3 -m64 "
			echo "Build 64-bit Darwin specific platform code."
		else
			AM_CFLAGS="$AM_CFLAGS -Dunix -Ddarwin -DSPACE_ORDER=2 -m32 "
			echo "Build 32-bit Darwin specific platform code."

		fi
		;;
	linux*)
		PLATFORM="linux"
		if test $host_cpu = "x86_64" ; then
			AC_DEFINE([linux],[1],[Build 64-bit Linux specific platform code.])
			AM_CFLAGS="$AM_CFLAGS -Dlinux -DSPACE_ORDER=3 -fno-strict-aliasing"
			echo "Build 64-bit Linux specific platform code."
		else
			AC_DEFINE([linux],[1],[Build 32-bit Linux specific platform code.])
			AM_CFLAGS="$AM_CFLAGS -Dlinux -fno-strict-aliasing"
			echo "Build 32-bit Linux specific platform code."
		fi
		#check for DCCP support
		CHECK_DCCP()
		;;
	mingw*|cygwin*)
		AC_DEFINE([mingw],[1],[Build Mingw specific platform code.])
		PLATFORM="mingw"
                AM_CFLAGS="$AM_CFLAGS -Dmingw"
                build_windows=yes
		;;

	interix*)
		AC_DEFINE([interix],[1],[Build Interix specific platform code.])
		PLATFORM="iterix"
		;;
  	*bsd*)
		AC_DEFINE([freebsd],[1],[Build Freebsd specific platform code.])
		PLATFORM="bsd"
		case $host_cpu in
			x86_64|amd64)
				AM_CFLAGS="$AM_CFLAGS -Dfreebsd -DSPACE_ORDER=3 -m64 "
				echo "Build 64-bit Freebsd specific platform code."
				;;
			*)
				AM_CFLAGS="$AM_CFLAGS  -Dfreebsd -DSPACE_ORDER=2 -m32 "
				echo "Build 32-bit Freebsd specific platform code."
				;;
		esac
		;;
	solaris*)
		PLATFORM="solaris"
		if test "$BUILD_32_ON_64_SOLARIS" = "1"; then
			if test "$CC" = "gcc"; then
				AC_MSG_ERROR([You must compile with Solaris Studio CC when building 32-bit on 64-bit Solaris (configure with CC={path to Solaris Studio CC})])
			fi
			NO_WALL_WERROR_WALL=1
			AC_DEFINE([solaris],[1],[Build Solaris specific platform code.])
			AM_LDFLAGS="$AM_LDFLAGS -lrt -lsocket -lnsl"

			AM_CFLAGS="$AM_CFLAGS -Dsolaris -Dunix -D__SVR4 -D_REENTRANT -DLONG_LONG_OKAY=0 -xmemalign=4i  -features=extensions  -xalias_level=any -xbuiltin=%none -DSPACE_ORDER=2"	
			echo "Build 32-bit ION on 64-bit solaris"
		elif test $host_cpu = "sparc" ; then
			AC_DEFINE([solaris],[1],[Build Solaris specific platform code.])
			AM_LDFLAGS="$AM_LDFLAGS -lrt -lsocket -lnsl"
			AM_CFLAGS="$AM_CFLAGS  -Dsolaris -Dunix -D__SVR4 -D_REENTRANT -fno-strict-aliasing -fno-builtin -DSPACE_ORDER=3"
			echo "Build Solaris/SPARC specific platform code."
		else
			AC_DEFINE([solaris],[1],[Build Solaris specific platform code.])
			AM_LDFLAGS="$AM_LDFLAGS  -lrt -lsocket -lnsl"
			AM_CFLAGS="$AM_CFLAGS  -Dsolaris -Dunix -D__SVR4 -D_REENTRANT -fno-strict-aliasing -DSPACE_ORDER=3"
			echo "Build Solaris specific platform code."
		fi
		# Do bitness check
		AS_IF([test "x$ISAINFO" != "xno"],
      		[isainfo_b=`isainfo -b`],
      		[isainfo_b="x"])
      	if test "$BUILD_32_ON_64_SOLARIS" = "1"; then
		 	AM_CFLAGS="$AM_CFLAGS -m32"
			echo "Build 32-bit code on 64-bit machine"
      	elif test "$isainfo_b" = "64"; then
      		AM_CFLAGS="$AM_CFLAGS -m64"
      		echo "Build 64-bit code"
      	else
      		AM_CFLAGS="$AM_CFLAGS -m32"
      		echo "Build 32-bit code"
      	fi
		;;
	vxworks*)
		AC_DEFINE([vxworks],[1],[Build VXWorks specific platform code.])
		PLATFORM="vxworks"
		;;
        *)
                AC_MSG_ERROR(["OS $host_os is not supported"])
esac


##Process user flags

#
# allow the user running configure to disable ACS when using bpv6
#
AC_ARG_ENABLE(
    acs,
    [AC_HELP_STRING([--disable-acs], [disable Aggregate Custody Signals])],
    [ENABLE_BPACS=no],
    [ENABLE_BPACS=yes])
AM_CONDITIONAL(ENABLE_BPACS, test "x$ENABLE_BPACS" = "xyes")


#
# allow the user running configure to build bpv6
#
AC_ARG_ENABLE(
    bpv7,
    [AC_HELP_STRING([--enable-bpv6], [force build of bpv6])],
    [VER=$(cat bpv7version.txt)
     VER="-DVNAME=ION-OPEN-SOURCE-$VER"
     AM_CFLAGS="$AM_CFLAGS $VER -DBUILD_BPv7"
     BUILD_BPv7=true
     ENABLE_BPACS=no
     echo "Building ION with BP version 7"],
    [VER=$(cat bpv6version.txt)
     VER="-DVNAME=ION-OPEN-SOURCE-$VER"
     AM_CFLAGS="$AM_CFLAGS $VER -DBUILD_BPv6"
     BUILD_BPv6=true
     case "${ENABLE_BPACS}" in
		yes) AM_CFLAGS="$AM_CFLAGS -DENABLE_BPACS";;
		no)  ENABLE_BPACS=no;;
     esac
     echo "Building ION with BP version 6"])
AM_CONDITIONAL(BUILD_BPv7, test x$BUILD_BPv7 = xtrue)
AM_CONDITIONAL(BUILD_BPv6, test x$BUILD_BPv6 = xtrue)


#
# allow the user running configure to disable IMC
#
AC_ARG_ENABLE(
    imc,
    [AC_HELP_STRING([--disable-imc], [disable Interplanetary Multicast])],
    [],
    [AM_CFLAGS="$AM_CFLAGS -DENABLE_IMC"
     ENABLE_IMC=yes])
AM_CONDITIONAL(ENABLE_IMC, test "x$ENABLE_IMC" = "xyes")

#
# Try to enable DCCP support
#
AC_ARG_ENABLE(
    dccp,
    [AC_HELP_STRING([--enable-dccp], [Enable Support for DCCP as a convergence layer])],
    [AC_DEFINE([build_dccp],[1],[DCCP only supported on Linux >=3.2.0])],
    [])

#
# allow the user running configure to disable manpage creation
#
AC_ARG_ENABLE(
    manpages,
    [AC_HELP_STRING([--disable-manpages], [disable manpage creation])],
    [],
    [AM_CFLAGS="$AM_CFLAGS -DENABLE_MANPAGES"
     ENABLE_MANPAGES=yes])
AM_CONDITIONAL(ENABLE_MANPAGES, test "x$ENABLE_MANPAGES" = "xyes")

#
# Control building /contrib utlities
#
AC_ARG_ENABLE(
	contrib,
	[AC_HELP_STRING([--disable-contrib],[Disable building third-party contributed utilities])],
	[case "${enableval}" in
		yes) BUILD_CONTRIB=yes;;
		no)  BUILD_CONTRIB=no;;
		*) AC_MSG_ERROR([bad value ${enableval} for --enable-contrib]);;
	esac],
	[BUILD_CONTRIB=no])
AM_CONDITIONAL(BUILD_CONTRIB, test "x$BUILD_CONTRIB" = "xyes")

#
# Allow user to disable crypto... if crypto is even enabled
#
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

	if test x$crypto = xtrue; then
		echo ""
		#DoStuff
	else
		AC_SUBST( [CRYPTO_LIBS], [""] )
	fi

	AC_DEFINE([NASA_PROTECTED_FLIGHT_CODE],[1],[Build ION NASA protected flight code])
else
	AM_CONDITIONAL([CRYPTO], [false])
	AC_SUBST( [CRYPTO_LIBS], [""] )
fi

#
# Control whether AMS debugging info is printed.
#
	AC_ARG_ENABLE([ams-debug],
		[  --enable-ams-debug    Enable AMS debugging info],
		[ AC_DEFINE([AMSDEBUG], 1, [Enable AMS debugging info]) ])

#
# Dynamically choose whether or not to instrument code for gcov code coverage
# Default is not to instrument the code for gcov code coverage.
#
AC_ARG_WITH([gcov],
[AS_HELP_STRING([--with-gcov], [Turn on code coverage instrumentation])],
[gcov=yes;],
[])
if test "x$gcov" = "xyes" ; then
	echo "Instrument code for gcov coverage analysis... yes"
	CFLAGS="$CFLAGS -g -O0 -fprofile-arcs -ftest-coverage"
	LDFLAGS="$LDFLAGS -fprofile-arcs -ftest-coverage -lgcov"
else
	echo "Instrument code for gcov coverage analysis... no"
fi
AM_CONDITIONAL([ENABLE_GCOV], [test "x$gcov" = "xyes"])

#
# Dynamically choose whether or not to build against expat libraries.
# Expat is needed by AMS if XML configuration files are used.
# Default is not to build against libexpat.
#
AC_ARG_WITH([expat],
[AS_HELP_STRING([--with-expat], [Turn on libexpat requirements])],
[expat=yes;],
[])
if test "x$expat" = "xyes" ; then
	echo "Build against libexpat... yes"
else
	echo "Build against libexpat... no"
	CFLAGS="$CFLAGS -DNOEXPAT"
fi
AM_CONDITIONAL([ENABLE_EXPAT], [test "x$expat" = "xyes"])


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

AC_ARG_ENABLE([sysctl-check], AC_HELP_STRING([--disable-sysctl-check],
                  [Disable checking sysctl configuration on OSX and FreeBSD]),
              [], [enable_sysctl_check=yes])

if test "x$enable_sysctl_check" = xyes; then
    echo "checking sysctl configuration..."
    echo

    if ! ${ac_confdir}/sysctl_script.sh; then
        echo "Reconfigure with --disable-sysctl-check to bypass this check."
        exit 1
    fi
fi

AC_ARG_WITH([mysql],
[AS_HELP_STRING([--with-mysql], [Enable MYSQL support for NM MGR])],
[mysql=yes;],
[])
if test "x$mysql" = "xyes" ; then
	echo "Build against libmysql... yes"
	CFLAGS="$CFLAGS -DHAVE_MYSQL"
else
	echo "Build against libmysql... no"
fi
AM_CONDITIONAL([ENABLE_MYSQL], [test "x$mysql" = "xyes"])


AC_ARG_WITH([ncurses],
[AS_HELP_STRING([--with-ncurses], [Enable NCURSES support for NM MGR])],
[ncurses=yes;],
[])
if test "x$ncurses" = "xyes" ; then
	echo "Build against libncurses... yes"
	CFLAGS="$CFLAGS -DUSE_NCURSES"
else
	echo "Build against libncurses... no"
fi
AM_CONDITIONAL([ENABLE_NCURSES], [test "x$ncurses" = "xyes"])

AC_ARG_ENABLE(
        nmrest,
        [AC_HELP_STRING([--enable-nmrest], [Enable REST API for NM Mgr])],
        [BUILD_NM_REST=true],
        [BUILD_NM_REST=false]
        )
AM_CONDITIONAL([BUILD_NM_REST], [test "x$BUILD_NM_REST" = "xtrue"])


# Check for libraries.
ORIGCFLAGS="$CFLAGS"
CFLAGS="$AM_CFLAGS $CFLAGS"
AC_CHECK_LIB(
	[expat],
	[XML_GetCurrentLineNumber],
	[ AC_SUBST( [EXPAT_LIBS], ["-lexpat"] ) ],
	[ AM_COND_IF([ENABLE_EXPAT], AC_MSG_ERROR([You need to install the libexpat1-dev library to build with expat.]), AC_MSG_WARN([If you wish to use XML configuration files for AMS you will need to compile against the Expat XML development libraries.  To do this install the libexpat1-dev library then recompile ION using the "./configure --with-expat" flag.]) )]
)

AC_CHECK_LIB(
	[pthread],
	[pthread_create],
	[ AM_LDFLAGS="$AM_LDFLAGS -lpthread" ],
	[AC_MSG_ERROR([You need pthread library.]) ]
)
CFLAGS="$ORIGCFLAGS"

AC_CHECK_LIB(
	[mysqlclient],
	[mysql_real_connect],
	[ AC_SUBST( [MYSQL_LIBS], ["-lmysqlclient"] ) ],
    [ AM_COND_IF([ENABLE_MYSQL], AC_MSG_ERROR([You need to install the mysqlclient library to build with MySQL.  Run configure with --with-mysql to turn on this feature.]), AC_MSG_WARN([If you wish to use MySQL for NM you will need to install the libmysqlclient.]) )]
)

AC_CHECK_LIB(
	[ncurses],
	[initscr],
	[ AC_SUBST( [NCURSES_LIBS], ["-lncurses -lform -lmenu -lpanel"] ) ],
	[ AM_COND_IF([ENABLE_NCURSES], AC_MSG_ERROR([You need to install the NCURSES library to build with NCURSES TUI support.  Run configure with --with-ncurses to turn on this feature.]), AC_MSG_WARN([If you wish to use the NCURSES TUI for NM you will need to install the NCURSES library.]) )]
)

# Check for programs to build documentation
AC_CHECK_PROGS([POD_DOCUMENTATION], [pod2man pod2html], [1] )
if test "$POD_DOCUMENTATION" = 1; then
	echo "ERROR: You need to have pod2man for documentation."
	exit -1
fi



# Check for tools to compile documentation into ION.pdf
AC_PATH_PROG([PS2PDF], [ps2pdf])
AC_PATH_PROG([PDF2PS], [pdf2ps])
AC_PATH_PROG([PSJOIN], [psjoin])
AC_PATH_PROG([PDFJOIN], [pdfjoin])
AC_MSG_CHECKING([for fallback included psjoin])
if test -x "$PWD/doc/psjoin"; then
    PSJOIN="$PWD/doc/psjoin"
    AC_MSG_RESULT($PSJOIN)
else
    AC_MSG_RESULT([not found])
fi
AC_MSG_CHECKING([for fallback included pdfjoin])
if test -x "$PWD/doc/pdfjoin"; then
    PDFJOIN="$PWD/doc/pdfjoin"
    AC_MSG_RESULT($PDFJOIN)
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
#    AC_MSG_RESULT([(skipping autodoc)])
# Comment out line above and uncomment the two below to enable automatic generation of ION.pdf
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

#Write AM_* flag variables
AC_SUBST([AM_LDFLAGS],"$AM_LDFLAGS")
AC_SUBST([AM_CFLAGS],"$AM_CFLAGS")
AC_SUBST([AM_LIBTOOLFLAGS],"$AM_LIBTOOLFLAGS")
AM_CONDITIONAL([ION_NASA_B],[test "$IS_NASA_B" = "1"])
AM_CONDITIONAL([LINUX_BUILD], [test "x$PLATFORM" = "xlinux"])
AM_CONDITIONAL([SOLARIS_STUDIO_CC], [test "$BUILD_32_ON_64_SOLARIS" = "1"])
AM_CONDITIONAL([WINDOWS], [test "$build_windows" = "yes"])

#Output!
AC_CONFIG_FILES([Makefile])
AC_CONFIG_FILES([contrib/Makefile])
AC_OUTPUT
