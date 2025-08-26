dnl ===============================================================
dnl  Check for Linux DCCP sockets (kernel ≥ 3.2.0)
dnl ===============================================================
AC_DEFUN([CHECK_DCCP], [
  AC_MSG_CHECKING([for DCCP support on Linux >= 3.2.0])

  AC_RUN_IFELSE(
    [AC_LANG_PROGRAM(
       [[
         /* Strict POSIX environment                                       */
         #define _POSIX_C_SOURCE 200809L

         #include <sys/utsname.h>
         #include <sys/socket.h>
         #include <string.h>
         #include <stdlib.h>
         #include <errno.h>

         #ifndef SOCK_DCCP      /* Fallback for old headers               */
         # define SOCK_DCCP  6
         #endif
         #ifndef IPPROTO_DCCP
         # define IPPROTO_DCCP 33
         #endif

         enum { REQ_MAJOR = 3, REQ_MINOR = 2, REQ_REV = 0 };

         int main(void)
         {
             struct utsname u;
             if (uname(&u) != 0)
                 return EXIT_FAILURE;

             /* strtok() writes to its arg, so copy first                  */
             char rel[256];
             strncpy(rel, u.release, sizeof rel - 1);
             rel[sizeof rel - 1] = '\0';

             char *tok   = strtok(rel,  ".");      int major = tok ? atoi(tok) : 0;
             tok         = strtok(NULL, ".-");     int minor = tok ? atoi(tok) : 0;
             tok         = strtok(NULL, ".-");     int rev   = tok ? atoi(tok) : 0;

             if (!((major >  REQ_MAJOR)                          ||
                   (major == REQ_MAJOR && minor >  REQ_MINOR)    ||
                   (major == REQ_MAJOR && minor == REQ_MINOR &&
                    rev   >= REQ_REV)))
                 return EXIT_FAILURE;              /* kernel too old       */

             int fd = socket(AF_INET, SOCK_DCCP, IPPROTO_DCCP);
             if (fd < 0)
                 return EXIT_FAILURE;              /* no DCCP socket       */

             return EXIT_SUCCESS;
         }
       ]])],
    [  dnl ---------- success branch -------------------------------------
       AC_MSG_RESULT([yes])
       AC_DEFINE([build_dccp], [1],
                 [Define to 1 when DCCP socket support is available])
    ],
    [  dnl ---------- failure branch -------------------------------------
       AC_MSG_RESULT([no])
    ],
    [  dnl ---------- cross-compile branch -------------------------------
       AC_MSG_RESULT([cross-compiling … disabled])
    ])
])
