# configure.ac for ION
# Samuel Jero
# June 14, 2013
# Process this file with autoconf to produce a configure script.
# Refactored by JPL ION development team

AC_PREREQ([2.69])

AC_INIT([ION], [4.2.0-b], [https://github.com/nasa-jpl/ION-DTN/issues], [],
        [https://github.com/nasa-jpl/ION-DTN])

AM_INIT_AUTOMAKE([1.16 subdir-objects -Wall foreign])
AM_SILENT_RULES([yes])
AM_MAINTAINER_MODE([enable])

# some source file that will ensure that you are in the right directory.
AC_CONFIG_SRCDIR([ici/include/ion.h])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])

m4_pattern_forbid([^PKG_])

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

using_gcc=no
AS_IF([test "x$GCC" = "xyes"],
  [AC_COMPILE_IFELSE([AC_LANG_SOURCE([[
#ifdef __clang__
#error
#endif
  ]])], [using_gcc=yes], [using_gcc=no])])

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

if test "$ac_cv_c11" = yes; then
  AM_CFLAGS="$AM_CFLAGS -std=iso9899:2018"
  AC_DEFINE([ION_HAVE_C11_ATOMICS],[1],
              [Define to 1 if the compiler supports <stdatomic.h>.])
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

dnl =============================================================
dnl  Expose to Automake   (needed for 'if !HAVE_C11_ATOMICS')
dnl =============================================================
AM_CONDITIONAL([HAVE_C11_ATOMICS], [test "x$ac_cv_c11" = xyes])

dnl =============================================================
dnl  Explicit atomic-backend selection (optional)
dnl =============================================================
dnl  Integrators may override the automatic tier selection in
dnl  ion_atomic.h by passing --with-atomics={c11,builtin,sync}.
dnl  When unset or "auto", the header's own auto-detection runs.
dnl =============================================================
AC_ARG_WITH([atomics],
  [AS_HELP_STRING([--with-atomics=@<:@auto|c11|builtin|sync@:>@],
     [Atomic backend: auto (default), c11 (<stdatomic.h>), builtin (__atomic_*), sync (__sync_*)])],
  [ion_atomics="$withval"],
  [ion_atomics="auto"])

case "$ion_atomics" in
  auto)    ;;
  c11)     AM_CFLAGS="$AM_CFLAGS -DION_ATOMIC_C11" ;;
  builtin) AM_CFLAGS="$AM_CFLAGS -DION_ATOMIC_BUILTIN" ;;
  sync)    AM_CFLAGS="$AM_CFLAGS -DION_ATOMIC_SYNC" ;;
  *)       AC_MSG_ERROR([Invalid --with-atomics value "$ion_atomics"; use auto, c11, builtin, or sync]) ;;
esac

dnl =============================================================
dnl  Feature-test macros (_POSIX_C_SOURCE / _XOPEN_SOURCE / _DEFAULT_SOURCE)
dnl  are intentionally NOT set globally via AC_DEFINE here.
dnl
dnl  ici/include/platform.h is the single source of truth for ION's
dnl  feature-test macro strategy: it sets the right combination per
dnl  platform (Linux, FreeBSD, Solaris, RTEMS, VxWorks) under
dnl  `#ifndef` guards so that any translation unit can override with
dnl  a more conservative value by defining the macro before including
dnl  platform.h.  Setting these macros in config.h (which is forced
dnl  onto every TU via `-include config.h`) would silently override
dnl  per-file conservative values such as the POSIX.1-2001 (200112L)
dnl  definitions used by bpv7/epp/stub/epp_loopback_provider.c and
dnl  bpv7/spp/stub/spp_loopback_provider.c, exposing a wider API
dnl  surface than those files requested.
dnl =============================================================

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
      AM_CFLAGS="$AM_CFLAGS -D__unix__ -Ddarwin -DSPACE_ORDER=3 -m64"
      PKG_CONFIG_CFLAGS="$PKG_CONFIG_CFLAGS -D__unix__ -Ddarwin -DSPACE_ORDER=3"
      echo "Build 64-bit Darwin specific platform code."
    elif test $host_cpu = "aarch64" ; then
      AM_CFLAGS="$AM_CFLAGS -D__unix__ -Ddarwin -DSPACE_ORDER=3 -m64"
      PKG_CONFIG_CFLAGS="$PKG_CONFIG_CFLAGS -D__unix__ -Ddarwin -DSPACE_ORDER=3"
      echo "Build 64-bit ARM Darwin specific platform code."
    elif test $host_cpu = "arm" ; then
      AM_CFLAGS="$AM_CFLAGS -D__unix__ -Ddarwin -DSPACE_ORDER=3 -m64"
      PKG_CONFIG_CFLAGS="$PKG_CONFIG_CFLAGS -D__unix__ -Ddarwin -DSPACE_ORDER=3"
      echo "Build 64-bit ARM Darwin specific platform code."
    else
      AM_CFLAGS="$AM_CFLAGS -D__unix__ -Ddarwin -DSPACE_ORDER=2 -m32"
      PKG_CONFIG_CFLAGS="$PKG_CONFIG_CFLAGS -D__unix__ -Ddarwin -DSPACE_ORDER=2"
      echo "Build 32-bit Darwin specific platform code."
    fi
    ;;
  linux*)
    PLATFORM="linux"
    # Add to PKG_CONFIG_CFLAGS
    PKG_CONFIG_CFLAGS="$PKG_CONFIG_CFLAGS -D__unix__ -D__linux__"
    if test $host_cpu = "x86_64" ; then
      AC_DEFINE([linux],[1])
      AM_CFLAGS="$AM_CFLAGS -D__unix__ -D__linux__ -DSPACE_ORDER=3 -fno-strict-aliasing"
      PKG_CONFIG_CFLAGS="$PKG_CONFIG_CFLAGS -D__unix__ -D__linux__ -DSPACE_ORDER=3"
      echo "Build 64-bit Linux specific platform code."
    elif test $host_cpu = "aarch64" ; then
      AC_DEFINE([linux],[1])
      AM_CFLAGS="$AM_CFLAGS -D__unix__ -D__linux__ -DSPACE_ORDER=3 -fno-strict-aliasing"
      PKG_CONFIG_CFLAGS="$PKG_CONFIG_CFLAGS -D__unix__ -D__linux__ -DSPACE_ORDER=3"
      echo "Build 64-bit Linux specific platform code."
    else
      AC_DEFINE([linux],[1])
      AM_CFLAGS="$AM_CFLAGS -D__unix__ -D__linux__ -DSPACE_ORDER=2 -fno-strict-aliasing"
      PKG_CONFIG_CFLAGS="$PKG_CONFIG_CFLAGS -D__unix__ -D__linux__ -DSPACE_ORDER=2"
      echo "Build 32-bit Linux specific platform code."
    fi
    ;;
  *bsd*)
    AC_DEFINE([freebsd],[1],[Build Freebsd specific platform code.])
    PLATFORM="bsd"
    AM_LDFLAGS="$AM_LDFLAGS -lexecinfo"
    case $host_cpu in
      x86_64|amd64)
        AM_CFLAGS="$AM_CFLAGS -D__BSD_VISIBLE -D__unix__ -Dfreebsd -DSPACE_ORDER=3 -m64"
        PKG_CONFIG_CFLAGS="$PKG_CONFIG_CFLAGS -D__BSD_VISIBLE -D__unix__ -Dfreebsd -DSPACE_ORDER=3"
        echo "Build 64-bit Freebsd specific platform code."
        ;;
      *)
        AM_CFLAGS="$AM_CFLAGS -D__BSD_VISIBLE -D__unix__ -Dfreebsd -DSPACE_ORDER=2 -m32"
        PKG_CONFIG_CFLAGS="$PKG_CONFIG_CFLAGS -D__BSD_VISIBLE -D__unix__ -Dfreebsd -DSPACE_ORDER=2"
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

      AM_CFLAGS="$AM_CFLAGS -Dsolaris -D__unix__ -D__SVR4 -D_REENTRANT -DLONG_LONG_OKAY=0 -DSPACE_ORDER=2 -xmemalign=4i -features=extensions -xalias_level=any -xbuiltin=%none"
      PKG_CONFIG_CFLAGS="$PKG_CONFIG_CFLAGS -Dsolaris -D__unix__ -D__SVR4 -D_REENTRANT -DLONG_LONG_OKAY=0 -DSPACE_ORDER=2"
      echo "Build 32-bit ION on 64-bit solaris"
    elif test $host_cpu = "sparc" ; then
      AC_DEFINE([solaris],[1],[Build Solaris specific platform code.])
      AM_LDFLAGS="$AM_LDFLAGS -lrt -lsocket -lnsl"
      AM_CFLAGS="$AM_CFLAGS -Dsolaris -D__unix__ -D__SVR4 -D_REENTRANT -fno-strict-aliasing -fno-builtin -DSPACE_ORDER=3"
      PKG_CONFIG_CFLAGS="$PKG_CONFIG_CFLAGS -Dsolaris -D__unix__ -D__SVR4 -D_REENTRANT -DSPACE_ORDER=3"
      echo "Build Solaris/SPARC specific platform code."
    else
      AC_DEFINE([solaris],[1],[Build Solaris specific platform code.])
      AM_LDFLAGS="$AM_LDFLAGS -lrt -lsocket -lnsl"
      AM_CFLAGS="$AM_CFLAGS -Dsolaris -D__unix__ -D__SVR4 -D_REENTRANT -fno-strict-aliasing -DSPACE_ORDER=3"
      PKG_CONFIG_CFLAGS="$PKG_CONFIG_CFLAGS -Dsolaris -D__unix__ -D__SVR4 -D_REENTRANT -DSPACE_ORDER=3"
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
  [AS_HELP_STRING([--disable-install-messages],
                  [disable extraneous messages on install])],
  [AS_CASE(["$enableval"],
     [yes|no], [],
     [AC_MSG_ERROR([bad value ${enableval} for --enable-install-messages])])],
  [enable_install_messages=yes])
AM_CONDITIONAL([ENABLE_INSTALL_MESSAGES],
               [test "x$enable_install_messages" != xno])

#
# ION will build BPv7 only
#

VER="4.2.0-b"
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
AC_ARG_ENABLE([imc],
  [AS_HELP_STRING([--disable-imc], [disable Interplanetary Multicast])],
  [AS_CASE(["$enableval"],
     [yes|no], [],
     [AC_MSG_ERROR([bad value ${enableval} for --enable-imc])])],
  [enable_imc=yes])

#
# allow the user running configure to disable TC (Trusted Collective TCA/TCC)
#
AC_ARG_ENABLE([tc],
  [AS_HELP_STRING([--disable-tc], [disable Trusted Collective (TCA/TCC)])],
  [AS_CASE(["$enableval"],
     [yes|no], [],
     [AC_MSG_ERROR([bad value ${enableval} for --enable-tc])])],
  [enable_tc=yes])

AS_IF([test "x$enable_tc" != xno && test "x$enable_imc" = xno],
  [AC_MSG_ERROR(
    [Invalid configuration: Trusted Collective (--enable-tc) \
cannot function without Interplanetary Multicast (--enable-imc).])])

AS_IF([test "x$enable_imc" != xno],
  [AM_CFLAGS="$AM_CFLAGS -DENABLE_IMC"
   PKG_CONFIG_CFLAGS="$PKG_CONFIG_CFLAGS -DENABLE_IMC"])
AM_CONDITIONAL([ENABLE_IMC], [test "x$enable_imc" != xno])

AS_IF([test "x$enable_tc" != xno],
  [AM_CFLAGS="$AM_CFLAGS -DENABLE_TC"
   PKG_CONFIG_CFLAGS="$PKG_CONFIG_CFLAGS -DENABLE_TC"])
AM_CONDITIONAL([ENABLE_TC], [test "x$enable_tc" != xno])

#
# allow the user running configure to disable DGR
#
AC_ARG_ENABLE([dgr],
  [AS_HELP_STRING([--disable-dgr], [disable DGR convergence layer])],
  [AS_CASE(["$enableval"],
     [yes|no], [],
     [AC_MSG_ERROR([bad value ${enableval} for --enable-dgr])])],
  [enable_dgr=yes])
AS_IF([test "x$enable_dgr" != xno],
  [AM_CFLAGS="$AM_CFLAGS -DENABLE_DGR"
   PKG_CONFIG_CFLAGS="$PKG_CONFIG_CFLAGS -DENABLE_DGR"])
AM_CONDITIONAL([ENABLE_DGR], [test "x$enable_dgr" != xno])

#
# allow the user running configure to enable BSSP (deprecated, off by default)
#
AC_ARG_ENABLE([bssp],
  [AS_HELP_STRING([--enable-bssp], [enable BSSP convergence layer (deprecated)])],
  [AS_CASE(["$enableval"],
     [yes|no], [],
     [AC_MSG_ERROR([bad value ${enableval} for --enable-bssp])])],
  [enable_bssp=no])
AS_IF([test "x$enable_bssp" = xyes],
  [AM_CFLAGS="$AM_CFLAGS -DENABLE_BSSP"
   PKG_CONFIG_CFLAGS="$PKG_CONFIG_CFLAGS -DENABLE_BSSP"])
AM_CONDITIONAL([ENABLE_BSSP], [test "x$enable_bssp" = xyes])

#
# allow the user running configure to disable DTPC
#
AC_ARG_ENABLE([dtpc],
  [AS_HELP_STRING([--disable-dtpc], [disable DTPC protocol])],
  [AS_CASE(["$enableval"],
     [yes|no], [],
     [AC_MSG_ERROR([bad value ${enableval} for --enable-dtpc])])],
  [enable_dtpc=yes])
AS_IF([test "x$enable_dtpc" != xno],
  [AM_CFLAGS="$AM_CFLAGS -DENABLE_DTPC"
   PKG_CONFIG_CFLAGS="$PKG_CONFIG_CFLAGS -DENABLE_DTPC"])
AM_CONDITIONAL([ENABLE_DTPC], [test "x$enable_dtpc" != xno])

#
# allow the user running configure to enable AMS_INDUSTRIAL
#
AC_ARG_ENABLE([ams-industrial],
  [AS_HELP_STRING([--enable-ams-industrial], [enable AMS_INDUSTRIAL])],
  [AS_CASE(["$enableval"],
     [yes|no], [],
     [AC_MSG_ERROR([bad value ${enableval} for --enable-ams-industrial])])],
  [enable_ams_industrial=no])
AS_IF([test "x$enable_ams_industrial" = xyes],
  [AM_CFLAGS="$AM_CFLAGS -DAMS_INDUSTRIAL"])

#
# allow the user running configure to enable the enhanced watch character option
#
AC_ARG_ENABLE([ewchar],
  [AS_HELP_STRING([--enable-ewchar], [enable enhanced watch characters])],
  [AS_CASE(["$enableval"],
     [yes|no], [],
     [AC_MSG_ERROR([bad value ${enableval} for --enable-ewchar])])],
  [enable_ewchar=no])
AS_IF([test "x$enable_ewchar" = xyes],
  [AM_CFLAGS="$AM_CFLAGS -DEWCHAR"
   PKG_CONFIG_CFLAGS="$PKG_CONFIG_CFLAGS -DEWCHAR"])
AM_CONDITIONAL([ENABLE_EWCHAR], [test "x$enable_ewchar" = xyes])

#
# allow the user running configure to enable the command line history option
# Uses bundled linenoise library (BSD-2-Clause)
#
AC_ARG_ENABLE([commandline-history],
  [AS_HELP_STRING([--enable-commandline-history],
                  [enable command line history using bundled linenoise])],
  [AS_CASE(["$enableval"],
     [yes|no], [],
     [AC_MSG_ERROR([bad value ${enableval} for --enable-commandline-history])])],
  [enable_commandline_history=no])
AS_IF([test "x$enable_commandline_history" = xyes],
  [AM_CFLAGS="$AM_CFLAGS -DINPUT_HISTORY"
   PKG_CONFIG_CFLAGS="$PKG_CONFIG_CFLAGS -DINPUT_HISTORY"])
AM_CONDITIONAL([ENABLE_CMD_HIST],
               [test "x$enable_commandline_history" = xyes])

#
# allow the user running configure to enable high-speed ION
#
AC_ARG_ENABLE([high-speed],
  [AS_HELP_STRING([--enable-high-speed],
                  [Enable high-speed ION, increases memory usage.])],
  [AS_CASE(["$enableval"],
     [yes|no], [],
     [AC_MSG_ERROR([bad value ${enableval} for --enable-high-speed])])],
  [enable_high_speed=no])
AS_IF([test "x$enable_high_speed" = xyes],
  [AM_CFLAGS="$AM_CFLAGS -DENABLE_HIGH_SPEED"
   PKG_CONFIG_CFLAGS="$PKG_CONFIG_CFLAGS -DENABLE_HIGH_SPEED"])
AM_CONDITIONAL([ENABLE_HIGH_SPEED], [test "x$enable_high_speed" = xyes])

AC_ARG_ENABLE([legacy-aliases],
  [AS_HELP_STRING([--disable-legacy-aliases], [Disable deprecated aliases])],
  [AS_CASE(["$enableval"],
     [yes|no], [],
     [AC_MSG_ERROR([bad value ${enableval} for --enable-legacy-aliases])])],
  [enable_legacy_aliases=yes])
AS_IF([test "x$enable_legacy_aliases" != xno],
  [AM_CPPFLAGS="$AM_CPPFLAGS -DION_USE_LEGACY_ALIASES"
   PKG_CONFIG_CFLAGS="$PKG_CONFIG_CFLAGS -DION_USE_LEGACY_ALIASES"])

#
# allow the user running configure to enable LTP UDP multisend (sendmmsg batching)
#
AC_ARG_ENABLE([ltp-udp-multisend],
  [AS_HELP_STRING([--enable-ltp-udp-multisend],
                  [Enable LTP UDP multisend (sendmmsg batching, Linux only)])],
  [AS_CASE(["${enableval}"],
     [yes],
     [AS_IF([test "x$PLATFORM" != xlinux],
            [AC_MSG_ERROR([--enable-ltp-udp-multisend requires Linux.])])],
     [no],
     [],
     [AC_MSG_ERROR([bad value ${enableval} for --enable-ltp-udp-multisend])])],
  [enable_ltp_udp_multisend=no])
AS_IF([test "x$enable_ltp_udp_multisend" = xyes],
  [AM_CFLAGS="$AM_CFLAGS -DUDP_MULTISEND"
   PKG_CONFIG_CFLAGS="$PKG_CONFIG_CFLAGS -DUDP_MULTISEND"])
AM_CONDITIONAL([ENABLE_LTP_UDP_MULTISEND],
               [test "x$enable_ltp_udp_multisend" = xyes])

#
# allow the user running configure to enable SDR performance instrumentation
#
AC_ARG_ENABLE([sdr-perf],
  [AS_HELP_STRING([--enable-sdr-perf],
                  [Enable SDR transaction performance instrumentation])],
  [AS_CASE(["$enableval"],
     [yes|no], [],
     [AC_MSG_ERROR([bad value ${enableval} for --enable-sdr-perf])])],
  [enable_sdr_perf=no])
AS_IF([test "x$enable_sdr_perf" = xyes],
  [AM_CFLAGS="$AM_CFLAGS -DSDR_PERF_INSTRUMENTATION"
   PKG_CONFIG_CFLAGS="$PKG_CONFIG_CFLAGS -DSDR_PERF_INSTRUMENTATION"])
AM_CONDITIONAL([ENABLE_SDR_PERF], [test "x$enable_sdr_perf" = xyes])

#
# allow the user running configure to force the use of Posix Named Semaphores over the
# otherwise recommended system. This might not produce a running system on some platforms
AC_ARG_ENABLE([force-posix-named-semaphores],
  [AS_HELP_STRING([--enable-force-posix-named-semaphores],
     [Force the use of Posix Named Semaphores,
      which are faster than SVR4,
      but might not work on this platform])],
  [AS_CASE(["$enableval"],
     [yes|no], [],
     [AC_MSG_ERROR([bad value ${enableval} \
for --enable-force-posix-named-semaphores])])],
  [enable_force_posix_named_semaphores=no])
AS_IF([test "x$enable_force_posix_named_semaphores" = xyes],
  [AM_CFLAGS="$AM_CFLAGS -DFORCE_POSIX_NAMED_SEMAPHORES"
   PKG_CONFIG_CFLAGS="$PKG_CONFIG_CFLAGS -DFORCE_POSIX_NAMED_SEMAPHORES"])
AM_CONDITIONAL([ENABLE_FORCE_POSIX_NAMED_SEMAPHORES],
               [test "x$enable_force_posix_named_semaphores" = xyes])

#
# allow the user running configure to force the use of SVR4 Semaphores over the
# otherwise recommended system. They are supported on all process-based platforms
AC_ARG_ENABLE([force-svr4-semaphores],
  [AS_HELP_STRING([--enable-force-svr4-semaphores],
     [Force the use of SVR4 Semaphores,
      which are supported on most platforms,
      but might be slower])],
  [AS_CASE(["$enableval"],
     [yes|no], [],
     [AC_MSG_ERROR([bad value ${enableval} \
for --enable-force-svr4-semaphores])])],
  [enable_force_svr4_semaphores=no])
AS_IF([test "x$enable_force_svr4_semaphores" = xyes],
  [AM_CFLAGS="$AM_CFLAGS -DFORCE_SVR4_SEMAPHORES"
   PKG_CONFIG_CFLAGS="$PKG_CONFIG_CFLAGS -DFORCE_SVR4_SEMAPHORES"])
AM_CONDITIONAL([ENABLE_FORCE_SVR4_SEMAPHORES],
               [test "x$enable_force_svr4_semaphores" = xyes])

AS_IF([test "x$enable_force_posix_named_semaphores" = xyes &&] \
      [test "x$enable_force_svr4_semaphores" = xyes],
  [AC_MSG_ERROR([cannot force both POSIX named semaphores \
and SVR4 semaphores simultaneously.])])

# Set up conditional for POSIX Named Semaphores
# By default (on Linux, FreeBSD, macOS, Solaris), POSIX Named Semaphores are used
# unless --enable-force-svr4-semaphores was specified
AM_CONDITIONAL([USING_POSIX_NAMED_SEMAPHORES],
  [test "x$enable_force_svr4_semaphores" != xyes])

#
# allow the user running configure to enable bpsec debugging
#
AC_ARG_ENABLE([bpsec-debugging],
  [AS_HELP_STRING([--enable-bpsec-debugging],
     [Enable bpsec debugging (levels 1-4).
      If level is omitted,
      defaults to level 4. @<:@default=no@:>@])],
  [AS_CASE(["$enableval"],
     [no], [],
     [1|2|3|4], [bpsec_lvl="$enableval"],
     [yes|*],
     [bpsec_lvl=4
      AS_IF([test "x$enableval" != xyes],
        [AC_MSG_WARN([invalid bpsec debugging level '$enableval'; \
defaulting to level 4])])])],
  [enable_bpsec_debugging=no])

AS_IF([test "x$enable_bpsec_debugging" != xno],
  [echo "BPSEC Debugging enabled. Using level $bpsec_lvl."
   bpsec_flags="-DBPSEC_DEBUGGING=1 -DBPSEC_DEBUG_LVL=$bpsec_lvl"
   bpsec_flags="$bpsec_flags -DBCB_DEBUGGING=1 -DBCB_TEST_LOGGING=1"
   bpsec_flags="$bpsec_flags -DBIB_DEBUGGING=1 -DBIB_TEST_LOGGING=1"
   AM_CFLAGS="$AM_CFLAGS $bpsec_flags"
   PKG_CONFIG_CFLAGS="$PKG_CONFIG_CFLAGS $bpsec_flags"])

#
# allow the user running configure to enable UniBo CGR
#
AC_ARG_ENABLE([unibo-cgr],
  [AS_HELP_STRING([--enable-unibo-cgr], [enable University of Bologna CGR])],
  [AS_CASE(["$enableval"],
     [yes|no], [],
     [AC_MSG_ERROR([bad value ${enableval} for --enable-unibo-cgr])])],
  [enable_unibo_cgr=no])
AS_IF([test "x$enable_unibo_cgr" = xyes],
  [# Check if bpv7/cgr/Unibo-CGR directory exists and is non-empty
   AC_MSG_CHECKING([for Unibo-CGR directory under bpv7/cgr])
   AS_IF([test -d "$srcdir/bpv7/cgr/Unibo-CGR" &&] \
         [ls "$srcdir/bpv7/cgr/Unibo-CGR"/* >/dev/null 2>&1],
     [AC_MSG_RESULT([yes])
      AC_DEFINE([UNIBO_CGR], [1], [Define to 1 to build Unibo-CGR])
      PKG_CONFIG_CFLAGS="$PKG_CONFIG_CFLAGS -DUNIBO_CGR"
      AC_MSG_NOTICE([Building ION with UniBo CGR code])],
     [AC_MSG_RESULT([no])
      AC_MSG_ERROR([Unibo-CGR is not installed, please see the unibo-cgr \
  module under /contrib folder for installation instructions.])])],
  [AC_DEFINE([UNIBO_CGR],[0],[Define to 1 to build Unibo-CGR])])
AM_CONDITIONAL([UNIBO_CGR], [test "x$enable_unibo_cgr" = xyes])

#
# allow the user running configure to disable manpage creation
#
AC_ARG_ENABLE([manpages],
  [AS_HELP_STRING([--disable-manpages],[disable manpage creation])],
  [AS_CASE(["$enableval"],
     [yes|no], [],
     [AC_MSG_ERROR([bad value ${enableval} for --enable-manpages])])],
  [enable_man_pages=yes])
AS_IF([test "x$enable_man_pages" != xno],
      [AM_CFLAGS="$AM_CFLAGS -DENABLE_MANPAGES"])
AM_CONDITIONAL([ENABLE_MANPAGES], [test "x$enable_man_pages" != xno])

#
# Allow user to enable crypto
#
# To Enable crypto, use "./configure --enable-crypto-mbedtls"
AC_ARG_ENABLE([crypto-mbedtls],
  [AS_HELP_STRING([--enable-crypto-mbedtls],[Turn on cryptography using MBEDTLS])],
  [AS_CASE(["$enableval"],
     [yes], [CRYPTO_MBEDTLS=true],
     [no], [CRYPTO_MBEDTLS=false],
     [AC_MSG_ERROR([bad value ${enableval} for --enable-crypto-mbedtls])])],
  [CRYPTO_MBEDTLS=false])
csi_suite_name="NULL Suites"
AS_IF([test "x$CRYPTO_MBEDTLS" = xtrue],
      [csi_suite_name="Mbed TLS Suites"],
      [AC_MSG_NOTICE([Mbed TLS Crypto disabled, building with NULL SUITES.])])
AC_DEFINE_UNQUOTED([CSI_SUITE_NAME], ["$csi_suite_name"], [Cipher suite name])
AM_CONDITIONAL([CRYPTO_MBEDTLS], [test "x$CRYPTO_MBEDTLS" = xtrue])

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
AC_ARG_ENABLE([bsl],
  [AS_HELP_STRING([--enable-bsl],[Turn on BPSec using external BSL library])],
  [AS_CASE(["$enableval"],
     [yes|no], [],
     [AC_MSG_ERROR([bad value ${enableval} for --enable-bsl])])],
  [enable_bsl=no])

# Test-only: use the fixed RFC 9173 Appendix A content-encryption key and
# IV vectors instead of a CSPRNG.  Requires --enable-bsl.  NOT for
# operational use (see ION_BSL_TEST_VECTORS handling below).
AC_ARG_ENABLE([bsl-test-vectors],
  [AS_HELP_STRING([--enable-bsl-test-vectors],
     [(BSL, test only) use fixed RFC 9173 Appendix A key/IV vectors
      instead of a CSPRNG; NOT for operational use])],
  [AS_CASE(["$enableval"],
     [yes|no], [],
     [AC_MSG_ERROR([bad value ${enableval} for --enable-bsl-test-vectors])])],
  [enable_bsl_test_vectors=no])

AS_IF([test "x$enable_bsl_test_vectors" = xyes && test "x$enable_bsl" != xyes],
  [AC_MSG_ERROR([--enable-bsl-test-vectors requires --enable-bsl])])

AS_IF([test "x$enable_bsl" = xyes],
  [
    # Check if external/BSL directory exists
    AC_MSG_CHECKING([for BSL submodule at external/BSL])
    if test -d "$srcdir/external/BSL" && ls "$srcdir/external/BSL"/* >/dev/null 2>&1; then
      AC_MSG_RESULT([yes])
      ENABLE_BSL=yes

      # BSL carries its own submodules under deps/.  A non-recursive
      # "git submodule update --init external/BSL" registers them without
      # populating them, and the resulting CMake failure ("deps/QCBOR does
      # not appear to contain CMakeLists.txt") does not identify the cause.
      AC_MSG_CHECKING([for BSL bundled dependencies])
      bsl_missing_deps=
      for bsl_dep in QCBOR mlib unity; do
        if ! ls "$srcdir/external/BSL/deps/$bsl_dep"/* >/dev/null 2>&1; then
          bsl_missing_deps="$bsl_missing_deps deps/$bsl_dep"
        fi
      done
      if test -n "$bsl_missing_deps"; then
        AC_MSG_RESULT([no])
        AC_MSG_ERROR([BSL dependencies not initialized:$bsl_missing_deps
  Initialize them with:
    git submodule update --init --recursive external/BSL])
      fi
      AC_MSG_RESULT([yes])

      # Set BSL_HOME to default location
      if test -z "${BSL_HOME}";
      then
        BSL_HOME="${srcdir}/external/BSL/testroot/usr"
      fi

      # Force BSL_HOME to be an absolute path to satisfy libtool.  It must
      # not be required to exist yet: "make" builds BSL as part of "all"
      # (see the all-bsl rule in Makefile.am) and "make clean" removes it
      # again, so configure legitimately runs both before and after BSL
      # exists.  Resolving this with `cd "$BSL_HOME" && pwd` assigned the
      # empty result of a failed cd, silently degrading every flag below to
      # -I/include/... and -L/lib and failing much later on a missing
      # header, so build the absolute path textually instead.
      case "$BSL_HOME" in
        /*) ;;
        *)  BSL_HOME="`pwd`/$BSL_HOME" ;;
      esac
      # Collapse the "/./" that a srcdir of "." would otherwise leave in
      # every -I, -L and -rpath built from this path.
      BSL_HOME=`echo "$BSL_HOME" | sed -e 's|/\./|/|g'`

      # BSL installs its libraries into lib64 on some platforms (RHEL
      # family) and lib on others, and which one it will use is not known
      # until BSL has been built.  Reference both; a -L or -rpath naming a
      # directory that does not exist is ignored.
      BSL_LIB_DIR64="${BSL_HOME}/lib64"
      BSL_LIB_DIR32="${BSL_HOME}/lib"

      # Add preprocessor flag for conditional compilation
      AM_CFLAGS="$AM_CFLAGS -DUSING_BSL=1"
      PKG_CONFIG_CFLAGS="$PKG_CONFIG_CFLAGS -DUSING_BSL=1"

      # BSL test vectors: replace the CSPRNG-backed content-encryption key
      # and IV generator with the fixed RFC 9173 Appendix A test vectors.
      # For interop/regression reproduction of the Appendix A byte-exact
      # vectors ONLY.  A build with this flag reuses a fixed key and IV,
      # which defeats confidentiality, so it must never be used
      # operationally; it is off unless explicitly requested.
      AS_IF([test "x$enable_bsl_test_vectors" = xyes],
        [AC_DEFINE([ION_BSL_TEST_VECTORS], [1],
           [(BSL, test only) use fixed RFC 9173 Appendix A key/IV vectors
            instead of a CSPRNG.  NOT for operational use.])
         AC_MSG_WARN([ION_BSL_TEST_VECTORS enabled: content-encryption keys \
and IVs are FIXED RFC 9173 test vectors, not random. \
Do not use this build operationally.])])

      # Add BSL include paths (including bpv7/bsl for ION's BSL wrapper headers)
      AM_CFLAGS="$AM_CFLAGS -I${srcdir}/bpv7/bsl -I${BSL_HOME}/include/m-lib -I${srcdir}/external/BSL/src -I${srcdir}/external/BSL/build/default/src -I${BSL_HOME}/include/bsl -I${BSL_HOME}/include/bsl/policy_provider -I${BSL_HOME}/include/bsl/security_context -I${BSL_HOME}/include/bsl/ION_integration"
      PKG_CONFIG_CFLAGS="$PKG_CONFIG_CFLAGS -I${BSL_HOME}/include/bsl"

      # Add jansson include path for platforms where it's in a subdirectory (e.g., Solaris)
      # Try common locations
      for jansson_inc_dir in /usr/include/jansson /usr/local/include/jansson; do
        if test -f "${jansson_inc_dir}/jansson.h"; then
          AM_CFLAGS="$AM_CFLAGS -I${jansson_inc_dir}"
          PKG_CONFIG_CFLAGS="$PKG_CONFIG_CFLAGS -I${jansson_inc_dir}"
          AC_MSG_NOTICE([Found jansson headers in ${jansson_inc_dir}])
          break
        fi
      done

      # Expose Solaris extensions
      if test "x$PLATFORM" = "xsolaris"; then
        CFLAGS="$CFLAGS -D__EXTENSIONS__"
      fi

      # Set BSL library flags.  -L must precede the -l options it applies to.
      BSL_LIBS="-L${BSL_LIB_DIR64} -L${BSL_LIB_DIR32} -lbsl_crypto -lbsl_default_sc -lbsl_dynamic -lbsl_front -lbsl_mock_bpa -lbsl_sample_pp -lqcbor -ljansson"

      # Executables linking against libbp need rpath-link to resolve
      # BSL's transitive dependencies (e.g. libqcbor) at link time
      # Use platform-specific linker flags
      case $host_os in
        solaris*)
          # Solaris linker uses -R for runtime library path
          AM_LDFLAGS="$AM_LDFLAGS -R${BSL_LIB_DIR64} -R${BSL_LIB_DIR32}"
          ;;
        *)
          # GNU ld (Linux, BSD, etc.) uses -rpath-link
          AM_LDFLAGS="$AM_LDFLAGS -Wl,-rpath-link,${BSL_LIB_DIR64} -Wl,-rpath-link,${BSL_LIB_DIR32}"
          ;;
      esac

      AC_MSG_NOTICE([BSL will be built with ION integration])
      AC_MSG_NOTICE([BSL_HOME: ${BSL_HOME}])
    else
      AC_MSG_RESULT([no])
      AC_MSG_ERROR([BSL submodule not found. Initialize with:
    git submodule update --init --recursive external/BSL])
    fi
  ],
  [echo "BSL disabled (using ION native BPSec)"
   ENABLE_BSL=no])

AM_CONDITIONAL([ENABLE_BSL], [test "x$ENABLE_BSL" = "xyes"])

if test "x$ENABLE_BSL" = "xyes"; then
  # Apply AM_CFLAGS to CFLAGS for header checks
  ORIGCFLAGS_BSL="$CFLAGS"
  CFLAGS="$AM_CFLAGS $CFLAGS"

  # Check for required BSL dependencies
  AC_CHECK_HEADER([jansson.h],
    [],
    [AC_MSG_ERROR([jansson development headers not found. Install libjansson-dev (Debian/Ubuntu) or jansson-devel (RHEL/Fedora)])])

  AC_CHECK_LIB([jansson], [json_object],
    [],
    [AC_MSG_ERROR([jansson library not found. Install libjansson-dev (Debian/Ubuntu) or jansson-devel (RHEL/Fedora)])])

  # Restore original CFLAGS
  CFLAGS="$ORIGCFLAGS_BSL"

  AC_SUBST([BSL_LIBS], ["$BSL_LIBS"])
  AC_SUBST([BSL_HOME], ["$BSL_HOME"])
  AC_SUBST([BSL_LIB_DIR64], ["$BSL_LIB_DIR64"])
  AC_SUBST([BSL_LIB_DIR32], ["$BSL_LIB_DIR32"])
else
  AC_SUBST([BSL_LIBS], [""])
  AC_SUBST([BSL_HOME], [""])
  AC_SUBST([BSL_LIB_DIR64], [""])
  AC_SUBST([BSL_LIB_DIR32], [""])
fi

#
# Control whether AMS debugging info is printed.
#
AC_ARG_ENABLE([ams-debug],
  [AS_HELP_STRING([--enable-ams-debug], [Enable AMS debugging info])],
  [AS_CASE(["$enableval"],
     [yes|no], [],
     [AC_MSG_ERROR([bad value ${enableval} for --enable-ams-debug])])],
  [enable_ams_debug=no])
AS_IF([test "x$enable_ams_debug" = xyes],
  [AC_DEFINE([AMSDEBUG], 1, [Enable AMS debugging info])
   PKG_CONFIG_CFLAGS="$PKG_CONFIG_CFLAGS -DAMSDEBUG"])

#
# Dynamically choose whether or not to instrument code for gcov code coverage
# Default is not to instrument the code for gcov code coverage.
#
AC_ARG_WITH([gcov],
  [AS_HELP_STRING([--with-gcov], [Turn on code coverage instrumentation])],
  [AS_CASE(["$withval"],
     [yes|no], [gcov="${withval}"],
     [AC_MSG_ERROR([bad value ${withval} for --with-gcov])])],
  [gcov=no])
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
  [AS_CASE(["$withval"],
     [yes|no], [],
     [AC_MSG_ERROR([bad value ${withval} for --with-expat])])],
  [with_expat=no])

#
# If valgrind is present, allow special MTAKE/MRELEASE valgrind tutors.
#
AC_ARG_ENABLE([valgrind],
  [AS_HELP_STRING([--enable-valgrind],
     [Enable valgrind debugging @<:@default=check@:>@])],
  [AS_CASE(["$enableval"],
     [yes|no|check], [],
     [AC_MSG_ERROR([bad value ${enableval} for --enable-valgrind])])],
  [enable_valgrind=check])

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

#
# AddressSanitizer build (diagnostics for memory errors).
# Adds -fsanitize=address to both compile and link flags.
# Off by default; intended for stress/debug runs, not production.
# Note: ION's PSM/SDR custom shared-memory allocators are not instrumented,
# but wild/NULL-pointer dereferences and stack overflow are caught.
#
AC_ARG_ENABLE([asan],
  [AS_HELP_STRING([--enable-asan],[Build with AddressSanitizer (-fsanitize=address); for debugging only])],
  [AS_CASE(["$enableval"],
     [yes|no], [],
     [AC_MSG_ERROR([bad value ${enableval} for --enable-asan])])],
  [enable_asan=no])
AS_IF([test "x$enable_asan" = xyes],
  [AC_MSG_CHECKING([if $CC accepts -fsanitize=address])
   saved_CFLAGS="$CFLAGS"
   saved_LDFLAGS="$LDFLAGS"
   CFLAGS="$CFLAGS -fsanitize=address"
   LDFLAGS="$LDFLAGS -fsanitize=address"
   AC_LINK_IFELSE(
     [AC_LANG_PROGRAM([], [])],
     [AC_MSG_RESULT([yes])],
     [AC_MSG_RESULT([no])
      AC_MSG_ERROR([--enable-asan requested, \
but -fsanitize=address is not supported])])
   CFLAGS="$saved_CFLAGS"
   LDFLAGS="$saved_LDFLAGS"

   AM_CFLAGS="$AM_CFLAGS -fsanitize=address -fno-omit-frame-pointer"
   AM_LDFLAGS="$AM_LDFLAGS -fsanitize=address"
   # Under -fsanitize=address ld becomes strict about every reference being
   # satisfied on the link line. Append -ldl (and -lpthread defensively) to
   # LIBS since dlsym needs it.
   LIBS="$LIBS -ldl -lpthread"
   AC_MSG_NOTICE([AddressSanitizer enabled (-fsanitize=address).])
   AC_MSG_NOTICE([  For reliable ASan stacks also pass CFLAGS='-O1 -g' (or -O0);])
   AC_MSG_NOTICE([  the default -O2 inlines frames and makes core backtraces misleading.])
   AC_MSG_NOTICE([  ION's in-process crash handler is auto-disabled under ASan so])
   AC_MSG_NOTICE([  ASan owns SIGSEGV reporting.])])
AM_CONDITIONAL([USING_GCC_ASAN],
  [test "x$using_gcc" = "xyes" && test "x$enable_asan" = "xyes"])

AC_ARG_ENABLE([sysctl-check],
  [AS_HELP_STRING([--disable-sysctl-check],
     [Disable checking sysctl configuration on macOS and FreeBSD])],
  [AS_CASE(["$enableval"],
     [yes|no], [],
     [AC_MSG_ERROR([bad value ${enableval} for --enable-sysctl-check])])],
  [enable_sysctl_check=yes])

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
  [AS_HELP_STRING([--with-mysql], [Enable MySQL support for NM MGR])],
  [AS_CASE(["$withval"],
     [yes|no], [mysql="${withval}"],
     [AC_MSG_ERROR([bad value ${withval} for --with-mysql])])],
  [mysql=no])
if test "x$mysql" = "xyes" ; then
  echo "Build against libmysql... yes"
  CFLAGS="$CFLAGS -DHAVE_MYSQL"
else
  echo "Build against libmysql... no"
fi
AM_CONDITIONAL([ENABLE_MYSQL], [test "x$mysql" = "xyes"])


AC_ARG_WITH([ncurses],
  [AS_HELP_STRING([--with-ncurses], [Enable NCURSES support for NM MGR])],
  [AS_CASE(["$withval"],
     [yes|no], [ncurses="${withval}"],
     [AC_MSG_ERROR([bad value ${withval} for --with-ncurses])])],
  [ncurses=no])
if test "x$ncurses" = "xyes" ; then
  echo "Build against libncurses... yes"
  CFLAGS="$CFLAGS -DUSE_NCURSES"
  PKG_CONFIG_CFLAGS="$PKG_CONFIG_CFLAGS -DUSE_NCURSES"
else
  echo "Build against libncurses... no"
fi
AM_CONDITIONAL([ENABLE_NCURSES], [test "x$ncurses" = "xyes"])

#
# Select the LTP xlsa (generic link service) transport backend.
# Default 'shm' is the shared-memory benchmark substrate; 'serial' is the
# length-framed byte-stream backend used for hardware adaptation and for
# loop testing against a socat pty pair.  See ltp/xlsa/doc/DESIGN.md.
#
AC_ARG_WITH([xlsa-backend],
  [AS_HELP_STRING([--with-xlsa-backend=BACKEND],
    [LTP xlsa transport backend: shm (default) or serial])],
  [XLSA_BACKEND=$withval],
  [XLSA_BACKEND=shm])
AS_CASE([$XLSA_BACKEND],
  [shm|serial], [],
  [AC_MSG_ERROR([invalid --with-xlsa-backend value: $XLSA_BACKEND (expected shm or serial)])])
echo "LTP xlsa backend... $XLSA_BACKEND"
AC_SUBST([XLSA_BACKEND])

# xport_shm calls shm_open / shm_unlink, which on Linux/Solaris live in
# librt; on macOS, BSD, and some other platforms they are in libc.  Probe
# for which we need and substitute the result into libxlsa's LIBADD so we
# do not force every other link line through -lrt.
XLSA_LIBS=""
AC_CHECK_FUNC([shm_open], [],
  [AC_CHECK_LIB([rt], [shm_open],
    [XLSA_LIBS="-lrt"],
    [AC_MSG_ERROR([cannot find shm_open in libc or librt -- required by ltp/xlsa])])])
AC_SUBST([XLSA_LIBS])

#
# allow the user running configure to choose the TCPCL_LOW_CYCLE option
#
AC_ARG_ENABLE([tcpcl-low-cycle],
  [AS_HELP_STRING([--enable-tcpcl-low-cycle], [enable TCPCL_LOW_CYCLE option])],
  [AS_CASE(["$enableval"],
     [yes|no], [],
     [AC_MSG_ERROR([bad value ${enableval} for --enable-tcpcl-low-cycle])])],
  [enable_tcpcl_low_cycle=no])
AS_IF([test "x$enable_tcpcl_low_cycle" = xyes],
  [AM_CFLAGS="$AM_CFLAGS -DTCPCL_LOW_CYCLE"
   PKG_CONFIG_CFLAGS="$PKG_CONFIG_CFLAGS -DTCPCL_LOW_CYCLE"
   AC_MSG_NOTICE([Enabling TCPCL_LOW_CYCLE... yes])]
)

# Check for libraries.
ORIGCFLAGS="$CFLAGS"
CFLAGS="$AM_CFLAGS $CFLAGS"

AS_IF([test "x$with_expat" != "xno"],
  [PKG_CHECK_MODULES([EXPAT], [expat],
                     [AC_DEFINE([HAVE_EXPAT], [1], [Use expat])])])

AC_CHECK_LIB(
  [pthread],
  [pthread_create],
  [AM_LDFLAGS="$AM_LDFLAGS -pthread"
   PTHREAD_LIBS="-pthread"
   AC_SUBST([PTHREAD_LIBS])],
  [AC_MSG_ERROR([You need pthread library.]) ]
)

dnl =============================================================
dnl  Robust-mutex support for the SDR transaction lock
dnl =============================================================
dnl  A process-shared PTHREAD_MUTEX_ROBUST mutex lets the SDR
dnl  transaction lock recover deterministically (the next locker
dnl  gets EOWNERDEAD) when a process dies mid-transaction, instead
dnl  of orphaning the lock and wedging the node forever.
dnl
dnl  Gated to Linux, FreeBSD and Solaris by an explicit platform
dnl  allowlist: even on platforms whose libc happens to implement
dnl  robust mutexes (e.g. recent RTEMS) the SDR lock deliberately
dnl  keeps the legacy semaphore path there.  Integrators may force
dnl  the legacy path anywhere with --disable-robust-sdr-lock.
dnl
dnl  This probe is independent of the C11-atomics tier selection
dnl  above; robust mutexes are a POSIX libc feature and do not
dnl  require C11.
dnl =============================================================
AC_ARG_ENABLE([robust-sdr-lock],
  [AS_HELP_STRING([--disable-robust-sdr-lock],
     [Do not use a robust pthread mutex for the SDR transaction lock,
      even where supported; fall back to the legacy semaphore.])],
  [AS_CASE(["$enableval"],
     [yes|no], [],
     [AC_MSG_ERROR([bad value ${enableval} for --enable-robust-sdr-lock])])],
  [enable_robust_sdr_lock="yes"])

AC_CACHE_CHECK([whether $CC supports robust process-shared mutexes],
  [ac_cv_robust_mutex],
  [save_LIBS="$LIBS"
   LIBS="$LIBS -pthread"
   AC_LINK_IFELSE(
     [AC_LANG_SOURCE([[
#define _GNU_SOURCE
#include <pthread.h>
int main(void)
{
	pthread_mutexattr_t	attr;
	pthread_mutex_t		mutex;

	if (pthread_mutexattr_init(&attr) != 0) return 1;
	if (pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED) != 0)
		return 1;
	if (pthread_mutexattr_setrobust(&attr, PTHREAD_MUTEX_ROBUST) != 0)
		return 1;
	if (pthread_mutex_init(&mutex, &attr) != 0) return 1;
	(void) pthread_mutex_consistent(&mutex);
	return 0;
}
]])],
     [ac_cv_robust_mutex=yes],
     [ac_cv_robust_mutex=no])
   LIBS="$save_LIBS"])

case $PLATFORM in
  linux|bsd|solaris)
    ion_robust_mutex_platform=yes ;;
  *)
    ion_robust_mutex_platform=no ;;
esac

if test "x$enable_robust_sdr_lock" = xyes \
     && test "x$ac_cv_robust_mutex" = xyes \
     && test "x$ion_robust_mutex_platform" = xyes; then
  ion_robust_mutex=yes
  AC_DEFINE([ION_HAVE_ROBUST_MUTEX],[1],
    [Define to 1 to use a robust process-shared pthread mutex for the
     SDR transaction lock, enabling EOWNERDEAD recovery when a process
     dies mid-transaction.  Gated to Linux, FreeBSD and Solaris.])
  AC_MSG_NOTICE([SDR transaction lock: robust-mutex recovery enabled])
else
  ion_robust_mutex=no
  if test "x$enable_robust_sdr_lock" != xyes; then
    AC_MSG_NOTICE([SDR transaction lock: robust mutex disabled by request])
  elif test "x$ion_robust_mutex_platform" != xyes; then
    AC_MSG_NOTICE([SDR transaction lock: robust mutex not enabled on this platform])
  else
    AC_MSG_NOTICE([SDR transaction lock: robust mutex unsupported by toolchain])
  fi
fi

AM_CONDITIONAL([HAVE_ROBUST_MUTEX], [test "x$ion_robust_mutex" = xyes])

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
AC_SEARCH_LIBS([backtrace], [execinfo], [
  AC_CHECK_FUNCS([backtrace backtrace_symbols])
])

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
AC_SUBST([AM_CPPFLAGS])
AC_SUBST([AM_CFLAGS],"$AM_CFLAGS")
AC_SUBST([AM_LIBTOOLFLAGS],"$AM_LIBTOOLFLAGS")
AC_SUBST([PKG_CONFIG_CFLAGS])
AC_SUBST([CRYPTO_LIBS],"$CRYPTO_LIBS")
AM_CONDITIONAL([LINUX_BUILD], [test "x$PLATFORM" = "xlinux"])
AM_CONDITIONAL([SOLARIS_STUDIO_CC], [test "$BUILD_32_ON_64_SOLARIS" = "1"])
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
    $srcdir/generate-podman-rules.sh $srcdir $1 > $1/doc/podman.mk
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
    DGR (Datagram Retransmission):           ${enable_dgr:-no}
    BSSP (Bundle Streaming Service):         ${enable_bssp:-no}
    IMC (Interplanetary Multicast):          ${enable_imc:-no}

  Optional Protocols:
    DTPC (Delay Tolerant Payload Conditioning): ${enable_dtpc:-no}
    TC (Trusted Collective TCA/TCC):            ${enable_tc:-no}

  Security:
    Crypto Library:                          ${CRYPTO_MBEDTLS:-no (NULL suites)}

  To enable BSL:    ./configure --enable-bsl
  To disable DGR:   ./configure --disable-dgr
  To enable BSSP:   ./configure --enable-bssp
  To disable DTPC:  ./configure --disable-dtpc
  To disable TC:    ./configure --disable-tc

========================================================================
])

AC_OUTPUT
