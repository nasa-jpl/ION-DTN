# configure.ac for ION
# Samuel Jero
# June 14, 2013
# Process this file with autoconf to produce a configure script.





AC_PREREQ([2.60])
AC_INIT([ion],[open source 4.1.3s],[https://github.com/nasa-jpl/ION-DTN/issues])

#  Josh Schendel removed the -Werror on 05/15/2012 per issue #355 to deal with
#  all of the portability warnings introduced by autoconf v2.69 and
#  automake v1.12.
#AM_INIT_AUTOMAKE([subdir-objects -Wall -Werror foreign])
AM_INIT_AUTOMAKE([subdir-objects -Wall foreign])

# some source file that will ensure that you are in the right directory.
AC_CONFIG_SRCDIR([ici/include/ion.h])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])


## -----checking wsl type------------------------------------------------------------------- ##
virtual_device_name=`uname -r`;
SUB="Microsoft";
SUB2="wsl2";
SUB3="WSL2";
     case $virtual_device_name in

     *"$SUB2"*)
    SUB=NULL;
	
    ;;
    esac
	 case $virtual_device_name in

     *"$SUB3"*)
    SUB=NULL;
	
    ;;
    esac
	
	# check uname -r contains wsl1 or not
    case $virtual_device_name in

     *"$SUB"*)
    echo -n "You can not install ION-DTN on WSL1 but you can install WSL2 and run ION-DTN on it."
	echo
	exit 0;
    ;;
    esac
	
## -----checking wsl type------------------------------------------------------------------- ##



dnl =============================================================
dnl Baseline tool checks
dnl =============================================================
AC_PROG_CC               dnl find a C compiler and set $CC
AM_PROG_CC_C_O           dnl enable “-c -o file.o file.c” portability
m4_ifdef([AM_PROG_AR],   dnl Automake ≥ 1.12 wants this
          [AM_PROG_AR])  dnl (harmless no-op on older Automake)


dnl =============================================================
dnl  Prefer ISO C11, fall back to ISO C99  (portable  ≥ Autoconf 2.60)
dnl =============================================================
AC_CACHE_CHECK([whether $CC supports C11], [ac_cv_c11],
  [save_CFLAGS="$CFLAGS"
   CFLAGS="$CFLAGS -std=c11"
   AC_COMPILE_IFELSE(
     [AC_LANG_SOURCE([[#include <stdatomic.h> int main() { return 0; }]])],
     [ac_cv_c11=yes],
     [ac_cv_c11=no])
   CFLAGS="$save_CFLAGS"])

if test "$ac_cv_c11" = yes; then
  AM_CFLAGS="$AM_CFLAGS -std=c11"
else
  AC_CACHE_CHECK([whether $CC supports C99], [ac_cv_c99],
    [save_CFLAGS="$CFLAGS"
     CFLAGS="$CFLAGS -std=c99"
     AC_COMPILE_IFELSE(
       [AC_LANG_SOURCE([[#include <stdint.h> int main() { return 0; }]])],
       [ac_cv_c99=yes],
       [ac_cv_c99=no])
     CFLAGS="$save_CFLAGS"])
  if test "$ac_cv_c99" = yes; then
    AM_CFLAGS="$AM_CFLAGS -std=c99"
  else
    AC_MSG_ERROR([Your compiler does not support ISO C11 or C99.])
  fi
fi


LT_INIT
AC_PROG_INSTALL
AC_PROG_MAKE_SET
LT_INIT


# Check for library functions
AC_CHECK_FUNCS([pow])


# Check Platform and Architecture
m4_include([m4/dccp_macro.m4])
AM_CFLAGS=""
AM_LDFLAGS=""
AM_LIBTOOLFLAGS=""
PLATFORM=""
S_LIB_END="so"
case $host_os in
	darwin*)
		AC_DEFINE([darwin],[1],[Build Darwin specific platform code.])
		AC_DEFINE([unix],[1],[Build Unix specific platform code.])
		PLATFORM="darwin"
		S_LIB_END="dylib"
		if test $host_cpu = "x86_64" ; then
			AM_CFLAGS="$AM_CFLAGS -Dunix -Ddarwin -DSPACE_ORDER=3 -m64 "
			echo "Build 64-bit Darwin specific platform code."
		elif test $host_cpu = "aarch64" ; then
			AM_CFLAGS="$AM_CFLAGS -Dunix -Ddarwin -DSPACE_ORDER=3 -m64"
			echo "Build 64-bit ARM Darwin specific platform code."	
		elif test $host_cpu = "arm" ; then
			AM_CFLAGS="$AM_CFLAGS -Dunix -Ddarwin -DSPACE_ORDER=3 -m64 "
			echo "Build 64-bit ARM Darwin specific platform code."
		else
			AM_CFLAGS="$AM_CFLAGS -Dunix -Ddarwin -DSPACE_ORDER=2 -m32 "
			echo "Build 32-bit Darwin specific platform code."

		fi
		;;
	linux*)
		PLATFORM="linux"
		if test $host_cpu = "x86_64" ; then
			AC_DEFINE([linux],[1],[Build 64-bit Linux specific platform code.])
			AM_CFLAGS="$AM_CFLAGS -Dunix -Dlinux -DSPACE_ORDER=3 -fno-strict-aliasing"
			echo "Build 64-bit Linux specific platform code."
		elif test $host_cpu = "aarch64" ; then
			AC_DEFINE([linux],[1],[Build 64-bit Linux specific platform code.])
			AM_CFLAGS="$AM_CFLAGS -Dunix -Dlinux -DSPACE_ORDER=3 -fno-strict-aliasing"
			echo "Build 64-bit Linux specific platform code."
		else
			AC_DEFINE([linux],[1],[Build 32-bit Linux specific platform code.])
			AM_CFLAGS="$AM_CFLAGS -Dunix -Dlinux -fno-strict-aliasing"
			echo "Build 32-bit Linux specific platform code."
		fi
		#check for DCCP support
		CHECK_DCCP()
		;;
	mingw*|cygwin*)
		AC_DEFINE([mingw],[1],[Build Mingw specific platform code.])
		AC_DEFINE([new_mingw],[1],[Build Mingw/Msys2 specific platform code.])
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
				AM_CFLAGS="$AM_CFLAGS -D__BSD_VISIBLE -Dunix -Dfreebsd -DSPACE_ORDER=3 -m64 "
				echo "Build 64-bit Freebsd specific platform code."
				;;
			*)
				AM_CFLAGS="$AM_CFLAGS -D__BSD_VISIBLE -Dunix -Dfreebsd -DSPACE_ORDER=2 -m32 "
				echo "Build 32-bit Freebsd specific platform code."
				;;
		esac
		;;
	solaris*)
		PLATFORM="solaris"
		build_solaris=yes
		
		# ISO C standard flags already selected above; avoid gnu99
		# If using non-GCC Solaris compilers (e.g., Solaris Studio), add -xc99=all
		if test "$GCC" != "yes"; then
			AC_MSG_NOTICE([Non-GCC Solaris compiler detected, using -xc99=all])
			AM_CFLAGS="$AM_CFLAGS -xc99=all"
		else
			AC_MSG_NOTICE([GCC on Solaris: relying on earlier -std=c11/c99 detection])
		fi

		
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
# Disable installation messaging
#
AC_ARG_ENABLE([install-messages],
  [AS_HELP_STRING([--disable-install-messages], [disable extraneous messages on install])],
  [enable_install_messages=$enableval],
  [enable_install_messages=yes])

AM_CONDITIONAL([ENABLE_INSTALL_MESSAGES], [test "$enable_install_messages" = yes])

#
# allow the user running configure to disable ACS when using bpv6
#
AC_ARG_ENABLE(
    acs,
    [AS_HELP_STRING([--enable-acs],[enable Aggregate Custody Signals])],
    [ENABLE_BPACS=yes],
    [ENABLE_BPACS=no])
AM_CONDITIONAL(ENABLE_BPACS, test "x$ENABLE_BPACS" = "xyes")

#
# allow the user running configure to build bpv6
#
AC_ARG_ENABLE(
    bpv6,
    [AS_HELP_STRING([--enable-bpv6],[force build of bpv6])],
    [VER="4.1.3s"
     VER="-DVNAME=ION-OPEN-SOURCE-$VER"
     AM_CFLAGS="$AM_CFLAGS $VER -DBUILD_BPv6"
     BUILD_BPv6=true
     BP_DIR="bpv6"
     AC_SUBST([BP_DIR])
     case "${ENABLE_BPACS}" in
		yes) AM_CFLAGS="$AM_CFLAGS -DENABLE_BPACS"
			 echo "Building ION with ACS";;
		no)  ENABLE_BPACS=no;;
     esac
     echo "Building ION with BP version 6"],
    [VER="4.1.3s"
     VER="-DVNAME=ION-OPEN-SOURCE-$VER"
     AM_CFLAGS="$AM_CFLAGS $VER -DBUILD_BPv7"
     BUILD_BPv7=true
     BP_DIR="bpv7"
     AC_SUBST([BP_DIR])
     ENABLE_BPACS=no
     echo "Building ION with BP version 7"])
AM_CONDITIONAL(BUILD_BPv7, test x$BUILD_BPv7 = xtrue)
AM_CONDITIONAL(BUILD_BPv6, test x$BUILD_BPv6 = xtrue)

#
# allow the user running configure to disable IMC
#
AC_ARG_ENABLE(
    imc,
    [AS_HELP_STRING([--disable-imc],[disable Interplanetary Multicast])],
    [],
    [AM_CFLAGS="$AM_CFLAGS -DENABLE_IMC"
     ENABLE_IMC=yes])
AM_CONDITIONAL(ENABLE_IMC, test "x$ENABLE_IMC" = "xyes")

#
# allow the user running configure to enable AMS_INDUSTRIAL
#
AC_ARG_ENABLE(
    ams-industrial,
    [AS_HELP_STRING([--enable-ams-industrial],[enable AMS_INDUSTRIAL])],
    [ENABLE_AMS_INDUSTRIAL=yes
	 AM_CFLAGS="$AM_CFLAGS -DAMS_INDUSTRIAL"],
    [ENABLE_AMS_INDUSTRIAL=no])
AM_CONDITIONAL(ENABLE_AMS_INDUSTRIAL, test "x$ENABLE_AMS_INDUSTRIAL" = "xyes")

#
# allow the user running configure to enable the enhanced watch character option
#
AC_ARG_ENABLE(
    ewchar,
    [AS_HELP_STRING([--enable-ewchar],[enable enhanced watch characters])],
    [ENABLE_EWCHAR=yes
	 AM_CFLAGS="$AM_CFLAGS -DEWCHAR"],
    [ENABLE_EWCHAR=no])
AM_CONDITIONAL(ENABLE_EWCHAR, test "x$ENABLE_EWCHAR" = "xyes")

#
# allow the user running configure to enable the command line history option
#
AC_ARG_ENABLE(
    commandline-history,
    [AS_HELP_STRING([--enable-commandline-history],[enable command line history])],
    [AC_CHECK_PROG([readline_present], [readline], [1], [0])
	 if test "$readline_present" = 1; then
		ENABLE_CMD_HIST=yes
	 	AM_CFLAGS="$AM_CFLAGS -DINPUT_HISTORY"
	 else
	 	ENABLE_CMD_HIST=no
	 	AC_MSG_ERROR([You need to have the readline executable installed to use this option.])
	 fi
	],
    [ENABLE_CMD_HIST=no
	 echo "Command line history disabled."
	])
AM_CONDITIONAL(ENABLE_CMD_HIST, test "x$ENABLE_CMD_HIST" = "xyes")

#
# allow the user running configure to enable high-speed ION 
#
AC_ARG_ENABLE(
    high-speed,
    [AS_HELP_STRING([--enable-high-speed],[Enable high-speed ION, increases memory usage.])],
    [AM_CFLAGS="$AM_CFLAGS -DENABLE_HIGH_SPEED"
     ENABLE_HIGH_SPEED=yes],
    [])
AM_CONDITIONAL(ENABLE_HIGH_SPEED, test x$ENABLE_HIGH_SPEED = xyes)

#
# allow the user running configure to force the use of Posix Named Semaphores over the 
# otherwise recommended system. This might not produce a running system on some platforms
AC_ARG_ENABLE(
    force-posix-named-semaphores,
    [AS_HELP_STRING([--enable-force-posix-named-semaphores],[Force the use of Posix Named Semaphores, which are faster then SVR4, but might not work on this platform])],
	[AM_CFLAGS="$AM_CFLAGS -DFORCE_POSIX_NAMED_SEMAPHORES"
		ENABLE_FORCE_POSIX_NAMED_SEMAPHORES=yes],
    [])

#
# allow the user running configure to force the use of SVR4 Semaphores over the 
# otherwise recommended system. They are supported on all process-based platforms
AC_ARG_ENABLE(
    force-svr4-semaphores,
    [AS_HELP_STRING([--enable-force-svr4-semaphores],[Force the use of SVR4 Semaphores, which are supported on most platforms, but might be slower])],
	[AM_CFLAGS="$AM_CFLAGS -DFORCE_SVR4_SEMAPHORES"
		ENABLE_FORCE_SVR4_SEMAPHORES=yes],
    [])

#
# allow the user running configure to enable bpsec debugging 
#
AC_ARG_ENABLE(
    bpsec-debugging,
    [AS_HELP_STRING([--enable-bpsec-debugging],[Enable bpsec debugging. Defaults to level 4, use "--enable-bpsec-debugging=num" to choose level.])],
    [case "${enableval}" in
		1) echo "BPSEC Debugging enabled. Using level 1."
		   AM_CFLAGS="$AM_CFLAGS -DBPSEC_DEBUGGING=1 -DBPSEC_DEBUG_LVL=1 -DBCB_DEBUGGING=1 -DBCB_TEST_LOGGING=1 -DBIB_DEBUGGING=1 -DBIB_TEST_LOGGING=1"
		   ENABLE_BPSEC_DEBUGGING=yes;;
		2) echo "BPSEC Debugging enabled. Using level 2."
		   AM_CFLAGS="$AM_CFLAGS -DBPSEC_DEBUGGING=1 -DBPSEC_DEBUG_LVL=2 -DBCB_DEBUGGING=1 -DBCB_TEST_LOGGING=1 -DBIB_DEBUGGING=1 -DBIB_TEST_LOGGING=1"
    	   ENABLE_BPSEC_DEBUGGING=yes;;
		3) echo "BPSEC Debugging enabled. Using level 3."
		   AM_CFLAGS="$AM_CFLAGS -DBPSEC_DEBUGGING=1 -DBPSEC_DEBUG_LVL=3 -DBCB_DEBUGGING=1 -DBCB_TEST_LOGGING=1 -DBIB_DEBUGGING=1 -DBIB_TEST_LOGGING=1"
    	   ENABLE_BPSEC_DEBUGGING=yes;;
		4) echo "BPSEC Debugging enabled. Using level 4."
		   AM_CFLAGS="$AM_CFLAGS -DBPSEC_DEBUGGING=1 -DBPSEC_DEBUG_LVL=4 -DBCB_DEBUGGING=1 -DBCB_TEST_LOGGING=1 -DBIB_DEBUGGING=1 -DBIB_TEST_LOGGING=1"
    	   ENABLE_BPSEC_DEBUGGING=yes;;
		*) echo "BPSEC Debugging enabled. Defaulting to level 4."
		   AM_CFLAGS="$AM_CFLAGS -DBPSEC_DEBUGGING=1 -DBPSEC_DEBUG_LVL=4 -DBCB_DEBUGGING=1 -DBCB_TEST_LOGGING=1 -DBIB_DEBUGGING=1 -DBIB_TEST_LOGGING=1"
		   ENABLE_BPSEC_DEBUGGING=yes;;
	 esac],
    [])

#
# Try to enable DCCP support
#
AC_ARG_ENABLE(
    dccp,
    [AS_HELP_STRING([--enable-dccp], [Enable Support for DCCP as a convergence layer])],
    [AC_DEFINE([build_dccp],[1],[DCCP only supported on Linux >=3.2.0])],
    [])

#
# allow the user running configure to enable UniBo CGR
#
AC_ARG_ENABLE(
    unibo-cgr,
    [AS_HELP_STRING([--enable-unibo-cgr], [enable University of Bologna CGR])],
    [if test "$BUILD_BPv7" == "true" ; then
        # Check if bpv7/cgr/Unibo-CGR directory exists and is non-empty
        AC_MSG_CHECKING([for Unibo-CGR directory under bpv7/cgr])
        if test -d "$srcdir/bpv7/cgr/Unibo-CGR" && ls "$srcdir/bpv7/cgr/Unibo-CGR"/* >/dev/null 2>&1; then
            AC_MSG_RESULT([yes])
            UNIBO_CGR=yes
            AC_DEFINE([UNIBO_CGR],[1],[Define to 1 to build Unibo-CGR])
            echo "Building ION with UniBo CGR code"
        else
            AC_MSG_RESULT([no])
            AC_MSG_ERROR([Unibo-CGR is not installed, please see the unibo-cgr module under /contrib folder for installation instructions.])
        fi
     else
        echo "ERROR: UniBo CGR is not available with BP version 6"
        AC_DEFINE([UNIBO_CGR],[0],[Define to 1 to build Unibo-CGR])
        exit 1
     fi],
    [UNIBO_CGR=no
     AC_DEFINE([UNIBO_CGR],[0],[Define to 1 to build Unibo-CGR])])
AM_CONDITIONAL(UNIBO_CGR, test "x$UNIBO_CGR" = "xyes")

#
# allow the user running configure to disable manpage creation
#
AC_ARG_ENABLE(
    manpages,
    [AS_HELP_STRING([--disable-manpages],[disable manpage creation])],
    [],
    [AM_CFLAGS="$AM_CFLAGS -DENABLE_MANPAGES"
     ENABLE_MANPAGES=yes])
AM_CONDITIONAL(ENABLE_MANPAGES, test "x$ENABLE_MANPAGES" = "xyes")

#
# Control building /contrib utlities
#
AC_ARG_ENABLE(
	contrib,
	[AS_HELP_STRING([--disable-contrib],[Disable building third-party contributed utilities])],
	[case "${enableval}" in
		yes) BUILD_CONTRIB=yes;;
		no)  BUILD_CONTRIB=no;;
		*) AC_MSG_ERROR([bad value ${enableval} for --enable-contrib]);;
	esac],
	[BUILD_CONTRIB=no])
AM_CONDITIONAL(BUILD_CONTRIB, test "x$BUILD_CONTRIB" = "xyes")

#
# Allow user to enable crypto
#
# To Enable crypto, use "./configure --enable-crypto-mbedtls"
AC_ARG_ENABLE(
	crypto-mbedtls,
	[AS_HELP_STRING([--enable-crypto-mbedtls],[Turn on cryptography using MBEDTLS])],
	[CRYPTO_MBEDTLS=true
	 AM_CFLAGS="$AM_CFLAGS -DMBEDTLS_CRYPTO_SUITES"],
	[echo "MBEDTLS Crypto disabled, building with NULL SUITES."
	 CRYPTO_MBEDTLS=false
	 AM_CFLAGS="$AM_CFLAGS -DNULL_CRYPTO_SUITES"])

AM_CONDITIONAL([CRYPTO_MBEDTLS], [test "x$CRYPTO_MBEDTLS" = "xtrue"])

if test x$CRYPTO_MBEDTLS = xtrue; then
	echo "------------------------------------------"
	echo "Turning on MBEDTLS Crypto"
	echo "------------------------------------------"

		if test -z "${MBED_LIB_PATH}"; then
			MBED_LIB_PATH="/usr/local/lib"
		fi

		if test -z "${MBED_INC_PATH}"; then
			MBED_INC_PATH="/usr/local/include"
		fi

		# EVALUATE: use -rpath for locating runtime library   
		# AM_LDFLAGS="$AM_LDFLAGS -Wl, -rpath, $MBED_LIB_PATH"

		AC_CHECK_FILES(
		[$MBED_LIB_PATH/libmbedtls.$S_LIB_END $MBED_LIB_PATH/libmbedx509.$S_LIB_END $MBED_LIB_PATH/libmbedcrypto.$S_LIB_END],
		[CRYPTO_LIBS="-lmbedtls -lmbedx509 -lmbedcrypto -L$MBED_LIB_PATH"
		 AM_CFLAGS="$AM_CFLAGS -I$MBED_INC_PATH"],
		[AC_MSG_WARN([If you have MbedTLS installed in a non-standard location, add MBED_LIB_PATH=<path-to-mbedtls-sharedlibrary> and MBED_INC_PATH=<path-to-mbedtls-header-files> to the ./configure command line.])
		 AC_MSG_ERROR([You need to have the MbedTLS dynamic library installed or use --disable-crypto.])]
		)

	#AC_DEFINE([CRYPTO_MBEDTLS],1, [Enable MBEDTLS Crypto])
	AC_SUBST( [CRYPTO_MBEDTLS], ["1"])
else
	AC_SUBST( [CRYPTO_LIBS], [""] )
	AC_SUBST( [CRYPTO_MBEDTLS], ["0"])
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
# Allow the user to configure for debugging with gdb
#
AC_ARG_WITH([debugging],
[AS_HELP_STRING([--with-debugging], [Remove compiler optimizations])],
[debugging=yes;],
[])
if test "x$debugging" = "xyes" ; then
	echo "Setting compiler optimization level to -O0"
	CFLAGS="$CFLAGS -g -O0"
fi
AM_CONDITIONAL([ENABLE_DEBUGGING], [test "x$debugging" = "xyes"])

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
AC_ARG_ENABLE([valgrind], AS_HELP_STRING([--enable-valgrind],[Enable valgrind debugging [default=check]]),
              [ enable_valgrind=yes], [ enable_valgrind=check ])

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

AC_ARG_ENABLE([sysctl-check], AS_HELP_STRING([--disable-sysctl-check],[Disable checking sysctl configuration on OSX and FreeBSD]),
              [], [enable_sysctl_check=yes])

if test "x$enable_sysctl_check" = xyes; then
    echo "checking sysctl configuration..."
    echo

    if ! ${ac_confdir}/sysctl_script.sh; then
        echo "Reconfigure with --disable-sysctl-check to bypass this check."
	echo "Update sysctl configuration by running ./install_macos_sysctl.sh"
        exit 1
    fi
fi

# AMP default (user can specify version N using this command: "AMP_VERSION=N ./configure")
# This addition resolves the prior GNU "AMP_VERSION ?= 8" lines in Makefile.am
AC_ARG_VAR([AMP_VERSION], [AMP version number])
test -z "$AMP_VERSION" && AMP_VERSION=8
AC_SUBST([AMP_VERSION])

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
        [AS_HELP_STRING([--enable-nmrest],[Enable REST API for NM Mgr])],
        [BUILD_NM_REST=true],
        [BUILD_NM_REST=false]
        )
AM_CONDITIONAL([BUILD_NM_REST], [test "x$BUILD_NM_REST" = "xtrue"])

#
# allow the user running configure to choose the TCPCL_LOW_CYCLE option
#
AC_ARG_ENABLE(
	tcpcl-low-cycle,
	[AS_HELP_STRING([--enable-tcpcl-low-cycle],[enable TCPCL_LOW_CYCLE option])],
	[AM_CFLAGS="$AM_CFLAGS -DTCPCL_LOW_CYCLE"
	 TCPCL_LOW_CYCLE=yes],
	[]
)
if test "x$TCPCL_LOW_CYCLE" = "xyes" ; then
	echo "Enabling TCPCL_LOW_CYCLE... yes"
fi
AM_CONDITIONAL([TCPCL_LOW_CYCLE], [test "x$TCPCL_LOW_CYCLE" = "xyes"])

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
    AC_MSG_RESULT([(skipping autodoc)])
# Comment out line above and uncomment the two below to enable automatic generation of ION.pdf
#    AC_MSG_RESULT([found])
#    buildautodoc="yes"
else
    AC_MSG_RESULT([not found (skipping autodoc)])
    AC_MSG_NOTICE([Ensure the ghostscript, psutils, groff-base and groff packages are installed.])
fi
AM_CONDITIONAL([ENABLE_AUTODOC], [ test "$buildautodoc" ])

# Add user defined CFLAGS
CFLAGS="$CFLAGS $CUSTOM_CFLAGS"

# Checks for header files.
AC_HEADER_DIRENT

AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS([fcntl.h malloc.h netdb.h netinet/in.h stddef.h stdlib.h string.h sys/ioctl.h sys/param.h sys/socket.h sys/time.h unistd.h])
AC_CHECK_HEADERS([execinfo.h])

# Check for <time.h> and <sys/time.h>, and whether they can be included together 
# This supports older builds while removing obsolete macros
AC_CHECK_HEADERS([time.h sys/time.h])
AC_CACHE_CHECK([can include <time.h> and <sys/time.h> together],
  [ac_cv_time_with_sys_time],
  [AC_COMPILE_IFELSE(
     [AC_LANG_PROGRAM([[#include <time.h>
#include <sys/time.h>]], [[return 0;]])],
     [ac_cv_time_with_sys_time=yes],
     [ac_cv_time_with_sys_time=no])])
if test "$ac_cv_time_with_sys_time" = yes; then
  AC_DEFINE([TIME_WITH_SYS_TIME],[1],
            [Define if <time.h> and <sys/time.h> can be included together])
fi


AC_C_CONST
AC_TYPE_SIZE_T

AC_STRUCT_TM
AC_TYPE_UID_T

# Determine whether signal handlers return void or int
# This supports older builds while removing obsolete macros
AC_RUN_IFELSE(
  [AC_LANG_PROGRAM([#include <signal.h>], [return 0;])],
  [AC_DEFINE([RETSIGTYPE],[void],[Return type of signal handlers])],
  [AC_DEFINE([RETSIGTYPE],[int],[Return type of signal handlers])])

# Checks for library functions.
# commented out library functions that should exist everywhere.
# otherwise, autoconf insisted that malloc.c, mktime.c were included in distro...
AC_FUNC_FORK
#AC_PROG_GCC_TRADITIONAL
#AC_FUNC_MALLOC
#AC_FUNC_MEMCMP
#AC_FUNC_MKTIME
#AC_FUNC_SELECT_ARGTYPES

#AC_FUNC_STAT
#AC_FUNC_STRFTIME
AC_CHECK_FUNCS([alarm ftruncate getcwd gethostbyaddr gethostbyname gethostname gettimeofday memset mkdir mkfifo rmdir select socket strcasecmp strchr strerror strncasecmp])

#Write AM_* flag variables
AC_SUBST([AM_LDFLAGS],"$AM_LDFLAGS")
AC_SUBST([AM_CFLAGS],"$AM_CFLAGS")
AC_SUBST([AM_LIBTOOLFLAGS],"$AM_LIBTOOLFLAGS")
AM_CONDITIONAL([LINUX_BUILD], [test "x$PLATFORM" = "xlinux"])
AM_CONDITIONAL([SOLARIS_STUDIO_CC], [test "$BUILD_32_ON_64_SOLARIS" = "1"])
AM_CONDITIONAL([WINDOWS], [test "$build_windows" = "yes"])
AM_CONDITIONAL([SOLARIS], [test "$build_solaris" = "yes"])

#Output!
AC_CONFIG_FILES([Makefile])
AC_CONFIG_FILES([contrib/Makefile])

dnl ===============================================================
dnl  Guard: create empty stubs so depfiles never fail
dnl ===============================================================
AC_CONFIG_COMMANDS_PRE([
  AS_MKDIR_P([tests])
  : > tests/testrules.mk      dnl portable “touch” 0-byte file
  : > pstrules.mk
  : > man-lists.mk
])

dnl ===============================================================
dnl  Autogenerate explicit man‑page rules (ISO‑C, no GNU % rules)
dnl ===============================================================

dnl ---- helper macro --------------------------------------------
m4_define([ADD_PODMAN],[
  AC_CONFIG_COMMANDS([$1/doc/podman.mk],[
      AS_MKDIR_P([$1/doc])
      $srcdir/generate-podman-rules.sh $1 > $1/doc/podman.mk
    ],[srcdir=$srcdir])
])dnl -------------------------------------------------------------


dnl ---------- Always‑built sub‑modules ----------------------------
ADD_PODMAN(ici)
ADD_PODMAN(ltp)
ADD_PODMAN(dgr)
ADD_PODMAN(nm)
ADD_PODMAN(bpv7)
ADD_PODMAN(tc)
ADD_PODMAN(ams)
ADD_PODMAN(cfdp)
ADD_PODMAN(bss)
ADD_PODMAN(bssp)
ADD_PODMAN(dtpc)

dnl ---------- Optional bpv6 ---------------------------------------
ADD_PODMAN(bpv6)




dnl ===============================================================
dnl  Autogenerate explicit test-<set> rules
dnl ===============================================================
AC_CONFIG_COMMANDS([tests/testrules.mk],[
    AS_MKDIR_P([tests])                       dnl ← create dir
    $srcdir/generate-test-rules.sh "$srcdir/tests" > tests/testrules.mk
  ],[srcdir=$srcdir])

dnl ===============================================================
dnl  Autogenerate explicit PS conversion rules
dnl ===============================================================
AC_CONFIG_COMMANDS([pstrules.mk],[
    :                                           dnl (in top dir already)
    $srcdir/generate-ps-rules.sh "$srcdir" > pstrules.mk
  ],[srcdir=$srcdir])

dnl ===============================================================
dnl  Autogenerate static man-page lists
dnl ===============================================================
AC_CONFIG_COMMANDS([man-lists.mk],[
    AS_MKDIR_P([$builddir])                   dnl ← “build” dir for out-of-tree
    "$srcdir/generate-man-lists.sh" "$srcdir" "$builddir"
  ],[srcdir=$srcdir builddir=$PWD])


dnl ===============================================================
dnl  Ensure required included *.mk files exist (POSIX make needs them)
dnl ===============================================================
AC_CONFIG_COMMANDS([podman-stubs], [
  for f in \
    pstrules.mk \
    man-lists.mk \
    tests/testrules.mk \
    ici/doc/podman.mk ltp/doc/podman.mk dgr/doc/podman.mk \
    nm/doc/podman.mk bpv7/doc/podman.mk tc/doc/podman.mk \
    ams/doc/podman.mk cfdp/doc/podman.mk bss/doc/podman.mk \
    bssp/doc/podman.mk dtpc/doc/podman.mk bpv6/doc/podman.mk; do
    test -f "$f" || echo "# stub" > "$f"
  done
])

AC_OUTPUT
