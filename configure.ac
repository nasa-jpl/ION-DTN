# configure.ac for ION
# Samuel Jero
# June 14, 2013
# Process this file with autoconf to produce a configure script.
# Refactored by JPL ION development team


AC_PREREQ([2.60])
AC_INIT([ion],[4.1.4],[https://github.com/nasa-jpl/ION-DTN/issues])

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
dnl  Prefer ISO C11/C18, fall back to ISO C99  (portable  ≥ Autoconf 2.60)
dnl =============================================================
AC_CACHE_CHECK([whether $CC supports C18 (iso9899:2018)], [ac_cv_c11],
  [save_CFLAGS="$CFLAGS"

  CFLAGS="$CFLAGS -std=iso9899:2018"
  AC_COMPILE_IFELSE(
    [AC_LANG_SOURCE([[#include <stdatomic.h> int main() { return 0; }]])],
    [ac_cv_c11=yes],
    [ac_cv_c11=no])
  CFLAGS="$save_CFLAGS"])

if test "$ac_cv_c11" = yes;
then
  AM_CFLAGS="$AM_CFLAGS -std=iso9899:2018"
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


AC_PROG_INSTALL
AC_PROG_MAKE_SET
LT_INIT


# Check for library functions
AC_CHECK_FUNCS([pow])


# Check Platform and Architecture
PLATFORM=""
S_LIB_END="so"
# Initialize PKG_CONFIG_CFLAGS for external applications
PKG_CONFIG_CFLAGS=""

# Template for linux define - comment set once to avoid m4 macro processing issue
# where all AC_DEFINE comments are seen regardless of shell if-elif-else logic
AH_TEMPLATE([linux],[Build Linux specific platform code.])

case $host_os in
  darwin*)
    AC_DEFINE([darwin],[1],[Build Darwin specific platform code.])
    AC_DEFINE([unix],[1],[Build Unix specific platform code.])
    PLATFORM="darwin"
    S_LIB_END="dylib"
    if test $host_cpu = "x86_64" ; then
      AM_CFLAGS="$AM_CFLAGS -Dunix -Ddarwin -DSPACE_ORDER=3 -m64"
      PKG_CONFIG_CFLAGS="$PKG_CONFIG_CFLAGS -Dunix -Ddarwin -DSPACE_ORDER=3"
      echo "Build 64-bit Darwin specific platform code."
    elif test $host_cpu = "aarch64" ; then
      AM_CFLAGS="$AM_CFLAGS -Dunix -Ddarwin -DSPACE_ORDER=3 -m64"
      PKG_CONFIG_CFLAGS="$PKG_CONFIG_CFLAGS -Dunix -Ddarwin -DSPACE_ORDER=3"
      echo "Build 64-bit ARM Darwin specific platform code."
    elif test $host_cpu = "arm" ; then
      AM_CFLAGS="$AM_CFLAGS -Dunix -Ddarwin -DSPACE_ORDER=3 -m64"
      PKG_CONFIG_CFLAGS="$PKG_CONFIG_CFLAGS -Dunix -Ddarwin -DSPACE_ORDER=3"
      echo "Build 64-bit ARM Darwin specific platform code."
    else
      AM_CFLAGS="$AM_CFLAGS -Dunix -Ddarwin -DSPACE_ORDER=2 -m32"
      PKG_CONFIG_CFLAGS="$PKG_CONFIG_CFLAGS -Dunix -Ddarwin -DSPACE_ORDER=2"
      echo "Build 32-bit Darwin specific platform code."
    fi
    ;;
  linux*)
    PLATFORM="linux"
    # Add to PKG_CONFIG_CFLAGS
    PKG_CONFIG_CFLAGS="$PKG_CONFIG_CFLAGS -Dunix -Dlinux"
    if test $host_cpu = "x86_64" ; then
      AC_DEFINE([linux],[1])
      AM_CFLAGS="$AM_CFLAGS -Dunix -Dlinux -DSPACE_ORDER=3 -fno-strict-aliasing"
      PKG_CONFIG_CFLAGS="$PKG_CONFIG_CFLAGS -Dunix -Dlinux -DSPACE_ORDER=3"
      echo "Build 64-bit Linux specific platform code."
    elif test $host_cpu = "aarch64" ; then
      AC_DEFINE([linux],[1])
      AM_CFLAGS="$AM_CFLAGS -Dunix -Dlinux -DSPACE_ORDER=3 -fno-strict-aliasing"
      PKG_CONFIG_CFLAGS="$PKG_CONFIG_CFLAGS -Dunix -Dlinux -DSPACE_ORDER=3"
      echo "Build 64-bit Linux specific platform code."
    else
      AC_DEFINE([linux],[1])
      AM_CFLAGS="$AM_CFLAGS -Dunix -Dlinux -DSPACE_ORDER=2 -fno-strict-aliasing"
      PKG_CONFIG_CFLAGS="$PKG_CONFIG_CFLAGS -Dunix -Dlinux -DSPACE_ORDER=2"
      echo "Build 32-bit Linux specific platform code."
    fi
    ;;
  mingw*|cygwin*)
    AC_DEFINE([mingw],[1],[Build Mingw specific platform code.])
    AC_DEFINE([new_mingw],[1],[Build Mingw/Msys2 specific platform code.])
    PLATFORM="mingw"
    AM_CFLAGS="$AM_CFLAGS -Dmingw"
    PKG_CONFIG_CFLAGS="$PKG_CONFIG_CFLAGS -Dmingw"
    build_windows=yes
    ;;
  interix*)
    AC_DEFINE([interix],[1],[Build Interix specific platform code.])
    PLATFORM="iterix"
    ;;
  *bsd*)
    AC_DEFINE([freebsd],[1],[Build Freebsd specific platform code.])
    PLATFORM="bsd"
    AM_LDFLAGS="$AM_LDFLAGS -lexecinfo"
    case $host_cpu in
      x86_64|amd64)
        AM_CFLAGS="$AM_CFLAGS -D__BSD_VISIBLE -Dunix -Dfreebsd -DSPACE_ORDER=3 -m64"
        PKG_CONFIG_CFLAGS="$PKG_CONFIG_CFLAGS -D__BSD_VISIBLE -Dunix -Dfreebsd -DSPACE_ORDER=3"
        echo "Build 64-bit Freebsd specific platform code."
        ;;
      *)
        AM_CFLAGS="$AM_CFLAGS -D__BSD_VISIBLE -Dunix -Dfreebsd -DSPACE_ORDER=2 -m32"
        PKG_CONFIG_CFLAGS="$PKG_CONFIG_CFLAGS -D__BSD_VISIBLE -Dunix -Dfreebsd -DSPACE_ORDER=2"
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

      AM_CFLAGS="$AM_CFLAGS -Dsolaris -Dunix -D__SVR4 -D_REENTRANT -DLONG_LONG_OKAY=0 -DSPACE_ORDER=2 -xmemalign=4i -features=extensions -xalias_level=any -xbuiltin=%none"
      PKG_CONFIG_CFLAGS="$PKG_CONFIG_CFLAGS -Dsolaris -Dunix -D__SVR4 -D_REENTRANT -DLONG_LONG_OKAY=0 -DSPACE_ORDER=2"
      echo "Build 32-bit ION on 64-bit solaris"
    elif test $host_cpu = "sparc" ; then
      AC_DEFINE([solaris],[1],[Build Solaris specific platform code.])
      AM_LDFLAGS="$AM_LDFLAGS -lrt -lsocket -lnsl"
      AM_CFLAGS="$AM_CFLAGS -Dsolaris -Dunix -D__SVR4 -D_REENTRANT -fno-strict-aliasing -fno-builtin -DSPACE_ORDER=3"
      PKG_CONFIG_CFLAGS="$PKG_CONFIG_CFLAGS -Dsolaris -Dunix -D__SVR4 -D_REENTRANT -DSPACE_ORDER=3"
      echo "Build Solaris/SPARC specific platform code."
    else
      AC_DEFINE([solaris],[1],[Build Solaris specific platform code.])
      AM_LDFLAGS="$AM_LDFLAGS -lrt -lsocket -lnsl"
      AM_CFLAGS="$AM_CFLAGS -Dsolaris -Dunix -D__SVR4 -D_REENTRANT -fno-strict-aliasing -DSPACE_ORDER=3"
      PKG_CONFIG_CFLAGS="$PKG_CONFIG_CFLAGS -Dsolaris -Dunix -D__SVR4 -D_REENTRANT -DSPACE_ORDER=3"
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
# ION will build BPv7 only
#

VER="4.1.4"
VER="-DVNAME=ION-OPEN-SOURCE-$VER"
AM_CFLAGS="$AM_CFLAGS $VER -DBUILD_BPv7"
PKG_CONFIG_CFLAGS="$PKG_CONFIG_CFLAGS $VER -DBUILD_BPv7"
BUILD_BPv7=true
BP_DIR="bpv7"
AC_SUBST([BP_DIR])
echo "Building ION with BP version 7"
AM_CONDITIONAL(BUILD_BPv7, test x$BUILD_BPv7 = xtrue)
#
# allow the user running configure to disable IMC
#
AC_ARG_ENABLE(
  imc,
  [AS_HELP_STRING([--disable-imc],[disable Interplanetary Multicast])],
  [],
  [AM_CFLAGS="$AM_CFLAGS -DENABLE_IMC"
   PKG_CONFIG_CFLAGS="$PKG_CONFIG_CFLAGS -DENABLE_IMC"
   ENABLE_IMC=yes])
AM_CONDITIONAL(ENABLE_IMC, test "x$ENABLE_IMC" = "xyes")

#
# allow the user running configure to disable DGR
#
AC_ARG_ENABLE(
  dgr,
  [AS_HELP_STRING([--disable-dgr],[disable DGR convergence layer])],
  [],
  [AM_CFLAGS="$AM_CFLAGS -DENABLE_DGR"
   PKG_CONFIG_CFLAGS="$PKG_CONFIG_CFLAGS -DENABLE_DGR"
   ENABLE_DGR=yes])
AM_CONDITIONAL(ENABLE_DGR, test "x$ENABLE_DGR" = "xyes")

#
# allow the user running configure to disable BSSP
#
AC_ARG_ENABLE(
  bssp,
  [AS_HELP_STRING([--disable-bssp],[disable BSSP convergence layer])],
  [],
  [AM_CFLAGS="$AM_CFLAGS -DENABLE_BSSP"
   PKG_CONFIG_CFLAGS="$PKG_CONFIG_CFLAGS -DENABLE_BSSP"
   ENABLE_BSSP=yes])
AM_CONDITIONAL(ENABLE_BSSP, test "x$ENABLE_BSSP" = "xyes")

#
# allow the user running configure to disable DTPC
#
AC_ARG_ENABLE(
  dtpc,
  [AS_HELP_STRING([--disable-dtpc],[disable DTPC protocol])],
  [],
  [AM_CFLAGS="$AM_CFLAGS -DENABLE_DTPC"
   PKG_CONFIG_CFLAGS="$PKG_CONFIG_CFLAGS -DENABLE_DTPC"
   ENABLE_DTPC=yes])
AM_CONDITIONAL(ENABLE_DTPC, test "x$ENABLE_DTPC" = "xyes")

#
# allow the user running configure to disable TC (Trusted Collective TCA/TCC)
#
AC_ARG_ENABLE(
  tc,
  [AS_HELP_STRING([--disable-tc],[disable Trusted Collective (TCA/TCC)])],
  [],
  [AM_CFLAGS="$AM_CFLAGS -DENABLE_TC"
   PKG_CONFIG_CFLAGS="$PKG_CONFIG_CFLAGS -DENABLE_TC"
   ENABLE_TC=yes])
AM_CONDITIONAL(ENABLE_TC, test "x$ENABLE_TC" = "xyes")

#
# allow the user running configure to enable AMS_INDUSTRIAL
#
AC_ARG_ENABLE(
  ams-industrial,
  [AS_HELP_STRING([--enable-ams-industrial],[enable AMS_INDUSTRIAL])],
  [ENABLE_AMS_INDUSTRIAL=yes
   AM_CFLAGS="$AM_CFLAGS -DAMS_INDUSTRIAL"
   AGS="$PKG_CONFIG_CFLAGS -DAMS_INDUSTRIAL"],
  [ENABLE_AMS_INDUSTRIAL=no])
AM_CONDITIONAL(ENABLE_AMS_INDUSTRIAL, test "x$ENABLE_AMS_INDUSTRIAL" = "xyes")

#
# allow the user running configure to enable the enhanced watch character option
#
AC_ARG_ENABLE(
  ewchar,
  [AS_HELP_STRING([--enable-ewchar],[enable enhanced watch characters])],
  [ENABLE_EWCHAR=yes
   AM_CFLAGS="$AM_CFLAGS -DEWCHAR"
   PKG_CONFIG_CFLAGS="$PKG_CONFIG_CFLAGS -DEWCHAR"],
  [ENABLE_EWCHAR=no])
AM_CONDITIONAL(ENABLE_EWCHAR, test "x$ENABLE_EWCHAR" = "xyes")

#
# allow the user running configure to enable the command line history option
# Uses bundled linenoise library (BSD-2-Clause)
#
AC_ARG_ENABLE(
  commandline-history,
  [AS_HELP_STRING([--enable-commandline-history],[enable command line history using bundled linenoise])],
  [ENABLE_CMD_HIST=yes
   AM_CFLAGS="$AM_CFLAGS -DINPUT_HISTORY"
   PKG_CONFIG_CFLAGS="$PKG_CONFIG_CFLAGS -DINPUT_HISTORY"
   echo "Command line history enabled (using bundled linenoise)."
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
   PKG_CONFIG_CFLAGS="$PKG_CONFIG_CFLAGS -DENABLE_HIGH_SPEED"
   ENABLE_HIGH_SPEED=yes],
  [])
AM_CONDITIONAL(ENABLE_HIGH_SPEED, test x$ENABLE_HIGH_SPEED = xyes)

#
# allow the user running configure to enable LTP UDP multisend (sendmmsg batching)
#
AC_ARG_ENABLE(
  ltp-udp-multisend,
  [AS_HELP_STRING([--enable-ltp-udp-multisend],[Enable LTP UDP multisend (sendmmsg batching, Linux only)])],
  [AM_CFLAGS="$AM_CFLAGS -DUDP_MULTISEND"
   PKG_CONFIG_CFLAGS="$PKG_CONFIG_CFLAGS -DUDP_MULTISEND"
   ENABLE_LTP_UDP_MULTISEND=yes],
  [])
AM_CONDITIONAL(ENABLE_LTP_UDP_MULTISEND, test x$ENABLE_LTP_UDP_MULTISEND = xyes)

#
# allow the user running configure to enable SDR performance instrumentation
#
AC_ARG_ENABLE(
  sdr-perf,
  [AS_HELP_STRING([--enable-sdr-perf],[Enable SDR transaction performance instrumentation])],
  [AM_CFLAGS="$AM_CFLAGS -DSDR_PERF_INSTRUMENTATION"
   PKG_CONFIG_CFLAGS="$PKG_CONFIG_CFLAGS -DSDR_PERF_INSTRUMENTATION"
   ENABLE_SDR_PERF=yes],
  [])
AM_CONDITIONAL(ENABLE_SDR_PERF, test x$ENABLE_SDR_PERF = xyes)

#
# allow the user running configure to force the use of Posix Named Semaphores over the
# otherwise recommended system. This might not produce a running system on some platforms
AC_ARG_ENABLE(
  force-posix-named-semaphores,
  [AS_HELP_STRING([--enable-force-posix-named-semaphores],[Force the use of Posix Named Semaphores, which are faster then SVR4, but might not work on this platform])],
  [AM_CFLAGS="$AM_CFLAGS -DFORCE_POSIX_NAMED_SEMAPHORES"
   PKG_CONFIG_CFLAGS="$PKG_CONFIG_CFLAGS -DFORCE_POSIX_NAMED_SEMAPHORES"
   ENABLE_FORCE_POSIX_NAMED_SEMAPHORES=yes],
  [])

#
# allow the user running configure to force the use of SVR4 Semaphores over the
# otherwise recommended system. They are supported on all process-based platforms
AC_ARG_ENABLE(
  force-svr4-semaphores,
  [AS_HELP_STRING([--enable-force-svr4-semaphores],[Force the use of SVR4 Semaphores, which are supported on most platforms, but might be slower])],
  [AM_CFLAGS="$AM_CFLAGS -DFORCE_SVR4_SEMAPHORES"
   PKG_CONFIG_CFLAGS="$PKG_CONFIG_CFLAGS -DFORCE_SVR4_SEMAPHORES"
   ENABLE_FORCE_SVR4_SEMAPHORES=yes],
  [])

# Set up conditional for POSIX Named Semaphores
# By default (on Linux, FreeBSD, macOS, Solaris), POSIX Named Semaphores are used
# unless --enable-force-svr4-semaphores was specified
AM_CONDITIONAL([USING_POSIX_NAMED_SEMAPHORES],
  [test x$ENABLE_FORCE_SVR4_SEMAPHORES != xyes])

#
# allow the user running configure to enable bpsec debugging
#
AC_ARG_ENABLE(
  bpsec-debugging,
  [AS_HELP_STRING([--enable-bpsec-debugging],[Enable bpsec debugging. Defaults to level 4, use "--enable-bpsec-debugging=num" to choose level.])],
  [case "${enableval}" in
    1) echo "BPSEC Debugging enabled. Using level 1."
      AM_CFLAGS="$AM_CFLAGS -DBPSEC_DEBUGGING=1 -DBPSEC_DEBUG_LVL=1 -DBCB_DEBUGGING=1 -DBCB_TEST_LOGGING=1 -DBIB_DEBUGGING=1 -DBIB_TEST_LOGGING=1"
      PKG_CONFIG_CFLAGS="$PKG_CONFIG_CFLAGS -DBPSEC_DEBUGGING=1 -DBPSEC_DEBUG_LVL=1 -DBCB_DEBUGGING=1 -DBCB_TEST_LOGGING=1 -DBIB_DEBUGGING=1 -DBIB_TEST_LOGGING=1"
      ENABLE_BPSEC_DEBUGGING=yes;;
    2) echo "BPSEC Debugging enabled. Using level 2."
      AM_CFLAGS="$AM_CFLAGS -DBPSEC_DEBUGGING=1 -DBPSEC_DEBUG_LVL=2 -DBCB_DEBUGGING=1 -DBCB_TEST_LOGGING=1 -DBIB_DEBUGGING=1 -DBIB_TEST_LOGGING=1"
      PKG_CONFIG_CFLAGS="$PKG_CONFIG_CFLAGS -DBPSEC_DEBUGGING=1 -DBPSEC_DEBUG_LVL=2 -DBCB_DEBUGGING=1 -DBCB_TEST_LOGGING=1 -DBIB_DEBUGGING=1 -DBIB_TEST_LOGGING=1"
      ENABLE_BPSEC_DEBUGGING=yes;;
    3) echo "BPSEC Debugging enabled. Using level 3."
      AM_CFLAGS="$AM_CFLAGS -DBPSEC_DEBUGGING=1 -DBPSEC_DEBUG_LVL=3 -DBCB_DEBUGGING=1 -DBCB_TEST_LOGGING=1 -DBIB_DEBUGGING=1 -DBIB_TEST_LOGGING=1"
      PKG_CONFIG_CFLAGS="$PKG_CONFIG_CFLAGS -DBPSEC_DEBUGGING=1 -DBPSEC_DEBUG_LVL=3 -DBCB_DEBUGGING=1 -DBCB_TEST_LOGGING=1 -DBIB_DEBUGGING=1 -DBIB_TEST_LOGGING=1"
      ENABLE_BPSEC_DEBUGGING=yes;;
    4) echo "BPSEC Debugging enabled. Using level 4."
      AM_CFLAGS="$AM_CFLAGS -DBPSEC_DEBUGGING=1 -DBPSEC_DEBUG_LVL=4 -DBCB_DEBUGGING=1 -DBCB_TEST_LOGGING=1 -DBIB_DEBUGGING=1 -DBIB_TEST_LOGGING=1"
      PKG_CONFIG_CFLAGS="$PKG_CONFIG_CFLAGS -DBPSEC_DEBUGGING=1 -DBPSEC_DEBUG_LVL=4 -DBCB_DEBUGGING=1 -DBCB_TEST_LOGGING=1 -DBIB_DEBUGGING=1 -DBIB_TEST_LOGGING=1"
      ENABLE_BPSEC_DEBUGGING=yes;;
    *) echo "BPSEC Debugging enabled. Defaulting to level 4."
      AM_CFLAGS="$AM_CFLAGS -DBPSEC_DEBUGGING=1 -DBPSEC_DEBUG_LVL=4 -DBCB_DEBUGGING=1 -DBCB_TEST_LOGGING=1 -DBIB_DEBUGGING=1 -DBIB_TEST_LOGGING=1"
      PKG_CONFIG_CFLAGS="$PKG_CONFIG_CFLAGS -DBPSEC_DEBUGGING=1 -DBPSEC_DEBUG_LVL=4 -DBCB_DEBUGGING=1 -DBCB_TEST_LOGGING=1 -DBIB_DEBUGGING=1 -DBIB_TEST_LOGGING=1"
      ENABLE_BPSEC_DEBUGGING=yes;;
    esac],
  [])

#
# allow the user running configure to enable UniBo CGR
#
AC_ARG_ENABLE(
  unibo-cgr,
  [AS_HELP_STRING([--enable-unibo-cgr], [enable University of Bologna CGR])],
  [
    if test "$BUILD_BPv7" == "true" ; then
      # Check if bpv7/cgr/Unibo-CGR directory exists and is non-empty
      AC_MSG_CHECKING([for Unibo-CGR directory under bpv7/cgr])
      if test -d "$srcdir/bpv7/cgr/Unibo-CGR" && ls "$srcdir/bpv7/cgr/Unibo-CGR"/* >/dev/null 2>&1; then
        AC_MSG_RESULT([yes])
        UNIBO_CGR=yes
        AC_DEFINE([UNIBO_CGR],[1],[Define to 1 to build Unibo-CGR])
        PKG_CONFIG_CFLAGS="$PKG_CONFIG_CFLAGS -DUNIBO_CGR"
        echo "Building ION with UniBo CGR code"
      else
        AC_MSG_RESULT([no])
        AC_MSG_ERROR([Unibo-CGR is not installed, please see the unibo-cgr module under /contrib folder for installation instructions.])
      fi
    else
        echo "ERROR: UniBo CGR is not available with BP version 6"
        AC_DEFINE([UNIBO_CGR],[0],[Define to 1 to build Unibo-CGR])
        exit 1
    fi
  ],
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
# Allow user to enable crypto
#
# To Enable crypto, use "./configure --enable-crypto-mbedtls"
AC_ARG_ENABLE(
  crypto-mbedtls,
  [AS_HELP_STRING([--enable-crypto-mbedtls],[Turn on cryptography using MBEDTLS])],
  [CRYPTO_MBEDTLS=true
   AM_CFLAGS="$AM_CFLAGS -DMBEDTLS_CRYPTO_SUITES"
   PKG_CONFIG_CFLAGS="$PKG_CONFIG_CFLAGS -DMBEDTLS_CRYPTO_SUITES"],
  [echo "MBEDTLS Crypto disabled, building with NULL SUITES."
   CRYPTO_MBEDTLS=false
   AM_CFLAGS="$AM_CFLAGS -DNULL_CRYPTO_SUITES"
   PKG_CONFIG_CFLAGS="$PKG_CONFIG_CFLAGS -DNULL_CRYPTO_SUITES"])

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
     AM_CFLAGS="$AM_CFLAGS -I$MBED_INC_PATH"
     PKG_CONFIG_CFLAGS="$PKG_CONFIG_CFLAGS -I$MBED_INC_PATH"],
    [AC_MSG_WARN([If you have MbedTLS installed in a non-standard location, add MBED_LIB_PATH=<path-to-mbedtls-sharedlibrary> and MBED_INC_PATH=<path-to-mbedtls-header-files> to the ./configure command line.])
     AC_MSG_ERROR([You need to have the MbedTLS dynamic library installed or use --disable-crypto.])]
  )

  #AC_DEFINE([CRYPTO_MBEDTLS],1, [Enable MBEDTLS Crypto])
  AC_SUBST([CRYPTO_MBEDTLS], ["1"])
else
  AC_SUBST([CRYPTO_LIBS], [""] )
  AC_SUBST([CRYPTO_MBEDTLS], ["0"])
fi

#
# Allow user to enable BSL (Bundle Protocol Security Library)
#
# To Enable BSL, use "./configure --enable-bsl"
AC_ARG_ENABLE(
  bsl,
  [AS_HELP_STRING([--enable-bsl],[Turn on BPSec using external BSL library])],
  [
    # Check if external/BSL directory exists
    AC_MSG_CHECKING([for BSL submodule at external/BSL])
    if test -d "$srcdir/external/BSL" && ls "$srcdir/external/BSL"/* >/dev/null 2>&1; then
      AC_MSG_RESULT([yes])
      ENABLE_BSL=yes

      # Set BSL_HOME to default location
      if test -z "${BSL_HOME}"; then
        BSL_HOME="${srcdir}/external/BSL/testroot/usr"
      fi

      # Add preprocessor flag for conditional compilation
      AM_CFLAGS="$AM_CFLAGS -DUSING_BSL=1"
      PKG_CONFIG_CFLAGS="$PKG_CONFIG_CFLAGS -DUSING_BSL=1"

      # Add BSL include paths (including bpv7/bsl for ION's BSL wrapper headers)
      AM_CFLAGS="$AM_CFLAGS -I${srcdir}/bpv7/bsl -I${srcdir}/external/BSL/src -I${srcdir}/external/BSL/build/default/src -I${BSL_HOME}/include/bsl -I${BSL_HOME}/include/bsl/policy_provider -I${BSL_HOME}/include/bsl/security_context -I${BSL_HOME}/include/bsl/ION_integration -I${BSL_HOME}/include/m-lib"
      PKG_CONFIG_CFLAGS="$PKG_CONFIG_CFLAGS -I${BSL_HOME}/include/bsl"

      # Set BSL library flags
      BSL_LIBS="-lbsl_crypto -lbsl_default_sc -lbsl_dynamic -lbsl_front -lbsl_mock_bpa -lbsl_sample_pp -lqcbor -ljansson -L${BSL_HOME}/lib"

      AC_MSG_NOTICE([BSL will be built with ION integration])
      AC_MSG_NOTICE([BSL_HOME: ${BSL_HOME}])
    else
      AC_MSG_RESULT([no])
      AC_MSG_ERROR([BSL submodule not found. Initialize with: git submodule update --init external/BSL])
    fi
  ],
  [echo "BSL disabled (using ION native BPSec)"
   ENABLE_BSL=no])

AM_CONDITIONAL([ENABLE_BSL], [test "x$ENABLE_BSL" = "xyes"])

if test "x$ENABLE_BSL" = "xyes"; then
  # Check for required BSL dependencies
  AC_CHECK_HEADER([jansson.h],
    [],
    [AC_MSG_ERROR([jansson development headers not found. Install libjansson-dev (Debian/Ubuntu) or jansson-devel (RHEL/Fedora)])])

  AC_CHECK_LIB([jansson], [json_object],
    [],
    [AC_MSG_ERROR([jansson library not found. Install libjansson-dev (Debian/Ubuntu) or jansson-devel (RHEL/Fedora)])])

  AC_SUBST([BSL_LIBS], ["$BSL_LIBS"])
  AC_SUBST([BSL_HOME], ["$BSL_HOME"])
else
  AC_SUBST([BSL_LIBS], [""])
  AC_SUBST([BSL_HOME], [""])
fi

#
# Control whether AMS debugging info is printed.
#
AC_ARG_ENABLE([ams-debug],
  AS_HELP_STRING([--enable-ams-debug], [Enable AMS debugging info])
  [AC_DEFINE([AMSDEBUG], 1, [Enable AMS debugging info])
   PKG_CONFIG_CFLAGS="$PKG_CONFIG_CFLAGS -DAMSDEBUG"])

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
  PKG_CONFIG_CFLAGS="$PKG_CONFIG_CFLAGS -DNOEXPAT"
fi
AM_CONDITIONAL([ENABLE_EXPAT], [test "x$expat" = "xyes"])


#
# If valgrind is present, allow special MTAKE/MRELEASE valgrind tutors.
#
AC_ARG_ENABLE([valgrind], AS_HELP_STRING([--enable-valgrind],[Enable valgrind debugging [default=check]]),
              [enable_valgrind=yes], [enable_valgrind=check])

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

#
# Allow user to disable fail-fast mode (CORE_FILE_NEEDED)
# By default, assertion failures cause immediate termination with core dump.
# Use --disable-core-file-needed to return error values instead of aborting.
#
AC_ARG_ENABLE([core-file-needed],
  [AS_HELP_STRING([--disable-core-file-needed],[Disable fail-fast mode; assertion failures return errors instead of aborting])],
  [case "${enableval}" in
    yes) AM_CFLAGS="$AM_CFLAGS -DCORE_FILE_NEEDED=1"
      PKG_CONFIG_CFLAGS="$PKG_CONFIG_CFLAGS -DCORE_FILE_NEEDED=1"
      CORE_FILE_NEEDED=yes;;
    no)  AM_CFLAGS="$AM_CFLAGS -DCORE_FILE_NEEDED=0"
      PKG_CONFIG_CFLAGS="$PKG_CONFIG_CFLAGS -DCORE_FILE_NEEDED=0"
      CORE_FILE_NEEDED=no;;
    *) AC_MSG_ERROR([bad value ${enableval} for --enable-core-file-needed]);;
  esac],
  [CORE_FILE_NEEDED=yes])

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
  PKG_CONFIG_CFLAGS="$PKG_CONFIG_CFLAGS -DUSE_NCURSES"
else
  echo "Build against libncurses... no"
fi
AM_CONDITIONAL([ENABLE_NCURSES], [test "x$ncurses" = "xyes"])

#
# allow the user running configure to choose the TCPCL_LOW_CYCLE option
#
AC_ARG_ENABLE(
  tcpcl-low-cycle,
  [AS_HELP_STRING([--enable-tcpcl-low-cycle],[enable TCPCL_LOW_CYCLE option])],
  [AM_CFLAGS="$AM_CFLAGS -DTCPCL_LOW_CYCLE"
   PKG_CONFIG_CFLAGS="$PKG_CONFIG_CFLAGS -DTCPCL_LOW_CYCLE"
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
  [AC_SUBST([EXPAT_LIBS], ["-lexpat"])],
  [AM_COND_IF([ENABLE_EXPAT], AC_MSG_ERROR([You need to install the libexpat1-dev library to build with expat.]), AC_MSG_WARN([If you wish to use XML configuration files for AMS you will need to compile against the Expat XML development libraries.  To do this install the libexpat1-dev library then recompile ION using the "./configure --with-expat" flag.]) )]
)

AC_CHECK_LIB(
  [pthread],
  [pthread_create],
  [AM_LDFLAGS="$AM_LDFLAGS -pthread"
   PTHREAD_LIBS="-pthread"
   AC_SUBST([PTHREAD_LIBS])],
  [AC_MSG_ERROR([You need pthread library.]) ]
)

# Check for dynamic linking library and header
AC_CHECK_HEADER([dlfcn.h], [], [
  AC_MSG_ERROR([required header dlfcn.h not found. Please install the appropriate C library development package (e.g., glibc-devel or libc6-dev).])
])
AC_SEARCH_LIBS([dlopen], [dl], [], [
  AC_MSG_ERROR([required function dlopen not found in C library or libdl.])
])

CFLAGS="$ORIGCFLAGS"

AC_CHECK_LIB(
  [mysqlclient],
  [mysql_real_connect],
  [AC_SUBST([MYSQL_LIBS], ["-lmysqlclient"])],
  [AM_COND_IF([ENABLE_MYSQL], AC_MSG_ERROR([You need to install the mysqlclient library to build with MySQL.  Run configure with --with-mysql to turn on this feature.]), AC_MSG_WARN([If you wish to use MySQL for NM you will need to install the libmysqlclient.]))]
)

AC_CHECK_LIB(
  [ncurses],
  [initscr],
  [AC_SUBST([NCURSES_LIBS], ["-lncurses -lform -lmenu -lpanel"])],
  [AM_COND_IF([ENABLE_NCURSES], AC_MSG_ERROR([You need to install the NCURSES library to build with NCURSES TUI support.  Run configure with --with-ncurses to turn on this feature.]), AC_MSG_WARN([If you wish to use the NCURSES TUI for NM you will need to install the NCURSES library.]))]
)

# Check for programs to build documentation
AC_CHECK_PROGS([POD_DOCUMENTATION], [pod2man pod2html], [1] )
if test "$POD_DOCUMENTATION" = 1; then
  echo "ERROR: You need to have pod2man for documentation."
  exit -1
fi




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
AC_SUBST([PKG_CONFIG_CFLAGS])
AC_SUBST([CRYPTO_LIBS],"$CRYPTO_LIBS")
AM_CONDITIONAL([LINUX_BUILD], [test "x$PLATFORM" = "xlinux"])
AM_CONDITIONAL([SOLARIS_STUDIO_CC], [test "$BUILD_32_ON_64_SOLARIS" = "1"])
AM_CONDITIONAL([WINDOWS], [test "$build_windows" = "yes"])
AM_CONDITIONAL([SOLARIS], [test "$build_solaris" = "yes"])

#Output!
AC_CONFIG_FILES([Makefile])
AC_CONFIG_FILES([ion.pc])

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
ADD_PODMAN(restart)




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
    bssp/doc/podman.mk dtpc/doc/podman.mk restart/doc/podman.mk; do
    test -f "$f" || echo "# stub" > "$f"
  done
])

dnl ===============================================================
dnl  Configuration Summary
dnl ===============================================================

# Set native BPSec status based on ENABLE_BSL
if test "x$ENABLE_BSL" = "xyes"; then
  NATIVE_BPSEC_STATUS="no"
else
  NATIVE_BPSEC_STATUS="yes (default)"
fi

AC_MSG_NOTICE([
========================================================================
ION Configuration Summary
========================================================================

  Bundle Protocol Version: BPv7

  BPSec Implementation:
    BSL (Bundle Protocol Security Library):  ${ENABLE_BSL:-no}
    Native ION BPSec:                        ${NATIVE_BPSEC_STATUS}

  Optional Convergence Layers:
    DGR (Datagram Retransmission):           ${ENABLE_DGR:-no}
    BSSP (Bundle Streaming Service):         ${ENABLE_BSSP:-no}
    IMC (Interplanetary Multicast):          ${ENABLE_IMC:-no}

  Optional Protocols:
    DTPC (Delay Tolerant Payload Conditioning): ${ENABLE_DTPC:-no}
    TC (Trusted Collective TCA/TCC):            ${ENABLE_TC:-no}

  Security:
    Crypto Library:                          ${CRYPTO_MBEDTLS:-no (NULL suites)}

  To enable BSL:    ./configure --enable-bsl
  To disable DGR:   ./configure --disable-dgr
  To disable BSSP:  ./configure --disable-bssp
  To disable DTPC:  ./configure --disable-dtpc
  To disable TC:    ./configure --disable-tc

========================================================================
])

AC_OUTPUT
