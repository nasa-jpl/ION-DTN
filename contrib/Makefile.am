# Makefile.am
#
# Samuel Jero
# July 22, 2013
#
# Makefile.am

AM_CFLAGS += \
	-Wall -Werror -g \
	-include $(srcdir)/../config.h

# This will add CFLAGS to make the valgrind macros compile without
# warnings/errors on some systems with specific versions of valgrind
# and gcc (see configure.ac and issue #298 for details).  If valgrind
# support is not present, this variable is empty.
AM_CFLAGS += $(VALGRIND_COMPAT_CFLAGS) 

# EXTRA_DIST gets sent in the tarball, but isn't really a program
# anything NOT compiled: scripts, readmes, tar files, config files.
# An exception of something compiled which must be explicitly included is
# man page source files.
#EXTRA_DIST = 	\
#	README.txt

install-exec-hook: 
	@echo
	@echo "You should now run 'sudo ldconfig' if available."
	@echo

install-data-hook:
	@echo
	@echo "You should now run 'sudo ldconfig' if available."
	@echo

if LINUX_BUILD
bin_PROGRAMS = \
	$(dtnperf_bin)
		

lib_LTLIBRARIES = \
	$(dtnperf_lib)

#include_HEADERS =

noinst_HEADERS = \
	$(dtnperf_noinst)

#man_MANS =
endif

################
## DTN Perf   ##
## Linux Only ##
################

dtnperf_bin= \
	dtnperf_vION

dtnperf_lib=\
	libal_bp_vION.la

dtnperf_noinst= \
	dtnperf/dtnperf/src/bundle_tools.h \
	dtnperf/dtnperf/src/csv_tools.h \
	dtnperf/dtnperf/src/definitions.h \
	dtnperf/dtnperf/src/dtnperf_types.h \
	dtnperf/dtnperf/src/file_transfer_tools.h \
	dtnperf/dtnperf/src/includes.h \
	dtnperf/dtnperf/src/utils.h \
	dtnperf/dtnperf/src/dtnperf_modes/dtnperf_client.h \
	dtnperf/dtnperf/src/dtnperf_modes/dtnperf_monitor.h \
	dtnperf/dtnperf/src/dtnperf_modes/dtnperf_server.h \
	dtnperf/al_bp/src/al_bp_api.h \
	dtnperf/al_bp/src/al_bp_types.h \
	dtnperf/al_bp/src/includes.h \
	dtnperf/al_bp/src/types.h \
	dtnperf/al_bp/src/bp_implementations/al_bp_dtn_conversions.h \
	dtnperf/al_bp/src/bp_implementations/al_bp_dtn.h \
	dtnperf/al_bp/src/bp_implementations/al_bp_ion_conversions.h \
	dtnperf/al_bp/src/bp_implementations/al_bp_ion.h

dtnperf_vION_SOURCES = dtnperf/dtnperf/src/dtnperf_modes/dtnperf_client.c \
			dtnperf/dtnperf/src/dtnperf_modes/dtnperf_monitor.c \
			dtnperf/dtnperf/src/dtnperf_modes/dtnperf_server.c \
			dtnperf/dtnperf/src/bundle_tools.c \
			dtnperf/dtnperf/src/csv_tools.c \
			dtnperf/dtnperf/src/file_transfer_tools.c \
			dtnperf/dtnperf/src/dtnperf_main.c \
			dtnperf/dtnperf/src/utils.c
dtnperf_vION_LDADD = libal_bp_vION.la ../libici.la ../libbp.la $(LIBSOBJS)
dtnperf_vION_CFLAGS =  -I$(srcdir)/dtnperf/al_bp/src/bp_implementations \
			-I$(srcdir)/dtnperf/al_bp/src \
			-I$(srcdir)/../bp/include \
			-I$(srcdir)/../bp/library \
			-I$(srcdir)/../bp/ipn \
			-I$(srcdir)/../bp/dtn2 \
			-fmessage-length=0 $(AM_CFLAGS)

libal_bp_vION_la_LIBADD = ../libici.la ../libbp.la $(LIBSOBJS)
libal_bp_vION_la_SOURCES = dtnperf/al_bp/src/al_bp_api.c \
			   dtnperf/al_bp/src/bp_implementations/al_bp_dtn.c \
			   dtnperf/al_bp/src/bp_implementations/al_bp_dtn_conversions.c \
			   dtnperf/al_bp/src/bp_implementations/al_bp_ion.c \
			   dtnperf/al_bp/src/bp_implementations/al_bp_ion_conversions.c
libal_bp_vION_la_CFLAGS = -I$(srcdir)/dtnperf/al_bp/src/bp_implementations \
			  -I$(srcdir)/../ici/include \
			  -I$(srcdir)/../bp/include \
			  -I$(srcdir)/../bp/library \
			  -I$(srcdir)/../bp/ipn \
			  -I$(srcdir)/../bp/dtn2 \
			  -I$(srcdir)/dtnperf/al_bp/src \
			  -DION_IMPLEMENTATION $(AM_CFLAGS)
libal_bp_vION_la_NAME = libal_bp_vION.la
