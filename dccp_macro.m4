dnl ------------------------------------------------------------
dnl CHECK_DCCP — test for DCCP socket support (SOCK_DCCP, IPPROTO_DCCP)
dnl ------------------------------------------------------------
AC_DEFUN([CHECK_DCCP], [
  AC_MSG_CHECKING([for DCCP support])
  AC_RUN_IFELSE(
    [AC_LANG_PROGRAM(
[[#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>]],
[[int main(void) {
    /* Return zero iff we can create a DCCP socket */
    return (socket(AF_INET, SOCK_DCCP, IPPROTO_DCCP) < 0);
  }]])],
    [AC_MSG_RESULT([yes])
     AC_DEFINE([build_dccp], [1],
               [Define if kernel supports DCCP sockets])],
    [AC_MSG_RESULT([no])],
    [AC_MSG_RESULT([cross-compiling... disabled])])
])
