# configure.ac for ION
#
# David Young
# Feb 12, 2008
#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

#is this high a version ok? it was 2.61 in ubuntu, but that was unavailable to
# fink, so we dropped it down
AC_PREREQ(2.60)
AC_INIT(ion, 2.5.0, http://korgano.eecs.ohiou.edu/mailman/listinfo/ion-bugs)
AM_INIT_AUTOMAKE([subdir-objects -Wall -Werror foreign])
# shall we put extra scripts in a special directory?
# AC_CONFIG_AUX_DIR(build-aux)
# some source file that will ensure that you are in the right directory.
AC_CONFIG_SRCDIR([ici/include/ion.h])
AC_CONFIG_HEADER([config.h])


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
AC_CHECK_HEADERS(valgrind/valgrind.h)


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
		AC_SUBST([ION_LINK_FLAGS]," -dynamiclib -undefined dynamic_lookup ")
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
			AC_SUBST([ION_LINK_FLAGS]," -fPIC -shared ")
			AC_SUBST([ION_CFLAGS]," -Dlinux -fPIC -DSPACE_ORDER=3 -fno-strict-aliasing")
			echo "Build 64-bit Linux specific platform code."
		else
			AC_DEFINE([linux],[1],[Build 32-bit Linux specific platform code.]) 
			AC_SUBST([ION_LINK_FLAGS]," -fPIC -shared ")
			AC_SUBST([ION_CFLAGS]," -Dlinux -fno-strict-aliasing -DNO_PROXY -DNO_DIRLIST")
			echo "Build 32-bit Linux specific platform code."
		fi
		;;

	cygwin*)  AC_DEFINE([cygwin],[1],[Build Cygwin specific platform code.]) ;;
	interix*) AC_DEFINE([interix],[1],[Build Interix specific platform code.]) ;;
	solaris*) 
		if test $host_cpu = "sparc" ; then
			AC_DEFINE([solaris],[1],[Build Solaris specific platform code.])
			AC_SUBST([ION_LINK_FLAGS]," -fPIC -shared -lrt -lsocket -lnsl")
			AC_SUBST([ION_CFLAGS]," -Dsol -Dunix -D__SVR4 -D_REENTRANT -fPIC -fno-strict-aliasing -fno-builtin")
			echo "Build Solaris/SPARC specific platform code."
		else
			AC_DEFINE([solaris],[1],[Build Solaris specific platform code.])
			AC_SUBST([ION_LINK_FLAGS]," -fPIC -shared -lrt -lsocket -lnsl")
			AC_SUBST([ION_CFLAGS]," -Dsol -Dunix -D__SVR4 -D_REENTRANT -fPIC -fno-strict-aliasing")
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
