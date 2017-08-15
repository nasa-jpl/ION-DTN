# Makefile.am
# Samuel Jero
# June 14, 2013
# Adapted from work by David Young

############
##eclsa
###########
SUBDIRS= 
DIST_SUBDIRS=

if ECLSA_CODEC_OPENFEC
SUBDIRS+= contrib/ECLSAv2/ion-open-source/ltp/ec/openfec
DIST_SUBDIRS+= contrib/ECLSAv2/ion-open-source/ltp/ec/openfec
endif

SUBDIRS+= .

#Control building of Contrib as determined by ./configure flags
if BUILD_CONTRIB
SUBDIRS+= contrib 
endif

DIST_SUBDIRS+= . contrib 

############
##end eclsa
###########


AM_CFLAGS += \
	-Wall -Werror -g \
	-include config.h \
	-I$(srcdir)/ici/include \
	-I$(srcdir)/ltp/include \
	-I$(srcdir)/dgr/include \
	-I$(srcdir)/bp/include \
	-I$(srcdir)/bss/include \
	-I$(srcdir)/dtpc/include \
	-I$(srcdir)/bssp/include \
	-DBPSEC

if !ION_NASA_B
AM_CFLAGS+= \
	-I$(srcdir)/ams/include \
	-I$(srcdir)/cfdp/include
endif

# This will add CFLAGS to make the valgrind macros compile without
# warnings/errors on some systems with specific versions of valgrind
# and gcc (see configure.ac and issue #298 for details).  If valgrind
# support is not present, this variable is empty.
AM_CFLAGS += $(VALGRIND_COMPAT_CFLAGS)

# EXTRA_DIST gets sent in the tarball, but isn't really a program
# anything NOT compiled: scripts, readmes, tar files, config files.
# An exception of something compiled which must be explicitly included is
# man page source files.
EXTRA_DIST = 	\
	$(srcdir)/loopbacktest.sh \
	$(srcdir)/sysctl_script.sh \
	$(srcdir)/README.txt \
	$(srcdir)/release-notes.txt \
	$(srcdir)/configs/3node-ltp-brs/README.txt \
	$(stuff_SCRIPTS) \
	$(loopbackltp_DATA) \
	$(loopbackudp_DATA) \
	$(loopbackstcp_DATA) \
	$(iondtn2_DATA) \
	$(otherstuff_DATA) \
	$(tutorial_DATA) \
	$(node5_DATA) \
	$(node7_DATA) \
	$(node10_DATA) \
	$(iciextra) \
	$(ltpextra) \
	$(dgrextra) \
	$(bpextra) \
	$(bssextra) \
	$(dtpcextra) \
	$(bsspextra)
#	$(twonodestcp_DATA) \
#	$(threenodestcpltp_DATA)

if !ION_NASA_B
EXTRA_DIST+= \
	$(amsextra) \
	$(cfdpextra)
endif

install-exec-hook:
	@echo
	@echo "You should now run 'sudo ldconfig' if available."
	@echo

install-data-hook:
	@echo
	@echo "You should now run 'sudo ldconfig' if available."
	@echo

if ENABLE_GCOV
cov-clean:
	@rm -rf coverage
	@find . -name "*.gcda" -exec rm {} \;
	@find . -name "*.gcno" -exec rm {} \;
	@lcov --directory . --zerocounters

cov-report:
	@mkdir -p coverage
	@lcov --compat-libtool --directory . -b . --capture --output-file coverage/app.info
	@genhtml -o coverage/ coverage/app.info

cov:
	@make cov-report

#clean-local:
#    @make cov-reset

#check:
#    @make cov

else
cov-clean:
	@echo "You must use \"configure --with-gcov\" to enable coverage testing features"

cov-report:
	@echo "You must use \"configure --with-gcov\" to enable coverage testing features"

cov:
	@echo "You must use \"configure --with-gcov\" to enable coverage testing features"
endif


# clean-local refers to existing documents.
if !ION_NASA_B
clean-local: iciclean-local ltpclean-local dgrclean-local bpclean-local amsclean-local cfdpclean-local
else
clean-local: iciclean-local ltpclean-local dgrclean-local bpclean-local
endif

build-man-directories:
	mkdir -p $(top_builddir)/ici/doc
	mkdir -p $(top_builddir)/ltp/doc
	mkdir -p $(top_builddir)/dgr/doc
	mkdir -p $(top_builddir)/bp/doc
	mkdir -p $(top_builddir)/bss/doc
	mkdir -p $(top_builddir)/dtpc/doc
	mkdir -p $(top_builddir)/bssp/doc
if !ION_NASA_B
	mkdir -p $(top_builddir)/ams/doc
	mkdir -p $(top_builddir)/cfdp/doc
endif


bin_PROGRAMS = \
	$(icibin) \
	$(ltpbin) \
	$(dgrbin) \
	$(bpbin) \
	$(bssbin) \
	$(dtpcbin) \
	$(bsspbin) \
	$(restartbin) \
	$(nmbin)

if !ION_NASA_B
bin_PROGRAMS+= \
	$(amsbin) \
	$(cfdpbin)
endif


lib_LTLIBRARIES = \
	$(icilib) \
	$(ltplib) \
	$(dgrlib) \
	$(bplib) \
	$(bsslib) \
	$(dtpclib) \
	$(bssplib)

if !ION_NASA_B
lib_LTLIBRARIES+= \
	$(amslib) \
	$(cfdplib)
endif

include_HEADERS = \
	$(iciinclude) \
	$(ltpinclude) \
	$(dgrinclude) \
	$(bpinclude) \
	$(bssinclude) \
	$(dtpcinclude) \
	$(bsspinclude)
if !ION_NASA_B
include_HEADERS+= \
	$(amsinclude) \
	$(cfdpinclude)
endif

noinst_HEADERS = \
	$(icinoinst) \
	$(ltpnoinst) \
	$(dgrnoinst) \
	$(bpnoinst) \
	$(bssnoinst) \
	$(dtpcnoinst) \
	$(bsspnoinst)

if !ION_NASA_B
noinst_HEADERS+= \
	$(amsnoinst) \
	$(cfdpnoinst)
endif

if ENABLE_MANPAGES
man_MANS = \
	build-man-directories \
	$(icimans) \
	$(ltpmans) \
	$(dgrmans) \
	$(bpmans) \
	$(bssmans) \
	$(dtpcmans) \
	$(bsspmans)

if !ION_NASA_B
man_MANS += \
	$(amsmans) \
	$(cfdpmans)
endif
endif

#################
#
# SCRIPTS SECTION
#
#################

# throw the helper scripts in the /usr/local/bin
# whatever directory with the rest of the programs.
stuffdir = $(bindir)
stuff_SCRIPTS =	$(srcdir)/killm \
		$(srcdir)/ionstart \
		$(srcdir)/ionstop \
		$(srcdir)/ionscript

otherstuffdir = $(pkgdatadir)
otherstuff_DATA = \
	$(srcdir)/ionstart.awk \
	$(srcdir)/tutorial.html \
	$(srcdir)/ION.pdf \
	$(srcdir)/ION-LTP-configuration.pdf \
	$(srcdir)/ION-LTP-configuration.xls

tutorialdir = $(pkgdatadir)/tutorial_files
tutorial_DATA = \
	$(srcdir)/tutorial_files/2node.png \
	$(srcdir)/tutorial_files/3node.png \
	$(srcdir)/tutorial_files/loopback.png \
	$(srcdir)/tutorial_files/print.html


#################
#
# CONFIGS SECTION
#
#################

# example configuration items
# loopback section (remember that you need an XXXdir and XXX_DATA for each
# folder, otherwise everything gets installed in a flat folder
loopbackltpdir = $(pkgdatadir)/configs/loopback-ltp
loopbackltp_DATA = \
	$(srcdir)/configs/loopback-ltp/loopback.ionrc \
	$(srcdir)/configs/loopback-ltp/loopback.ltprc \
	$(srcdir)/configs/loopback-ltp/loopback.bprc \
	$(srcdir)/configs/loopback-ltp/loopback.ipnrc \
	$(srcdir)/configs/loopback-ltp/loopback.rc
# new loopback for ion2, stcp
loopbackstcpdir = $(pkgdatadir)/configs/loopback-stcp
loopbackstcp_DATA = \
	$(srcdir)/configs/loopback-stcp/loopback.ionrc \
	$(srcdir)/configs/loopback-stcp/loopback.bprc \
	$(srcdir)/configs/loopback-stcp/loopback.ipnrc \
	$(srcdir)/configs/loopback-stcp/loopback.rc
# loopback using the udp convergence layer
loopbackudpdir = $(pkgdatadir)/configs/loopback-udp
loopbackudp_DATA = \
	$(srcdir)/configs/loopback-udp/loopback.rc
# ion and dtn2 example
iondtn2dir = $(pkgdatadir)/configs/ion-dtn2-example
iondtn2_DATA = \
	$(srcdir)/configs/ion-dtn2-example/dtn2-host.conf \
	$(srcdir)/configs/ion-dtn2-example/ion-host.rc

# 2node testing- using stcp
#twonodestcpdir = $(pkgdatadir)/configs/2node-stcp
#twonodestcp_DATA = \
	#$(srcdir)/configs/2node-stcp/host1.bprc \
	#$(srcdir)/configs/2node-stcp/host1.ionrc \
	#$(srcdir)/configs/2node-stcp/host1.ipnrc \
	#$(srcdir)/configs/2node-stcp/host1.rc \
	#$(srcdir)/configs/2node-stcp/host2.rc \
	#$(srcdir)/configs/2node-stcp/host2.bprc \
	#$(srcdir)/configs/2node-stcp/host2.ionrc \
	#$(srcdir)/configs/2node-stcp/host2.ipnrc
## 3node testing- using stcp and ltp
#threenodestcpltpdir = $(pkgdatadir)/configs/3node-stcp-ltp
#threenodestcpltp_DATA = \
	#$(srcdir)/configs/3node-stcp-ltp/host1.ionrc \
	#$(srcdir)/configs/3node-stcp-ltp/host1.ipnrc \
	#$(srcdir)/configs/3node-stcp-ltp/host1.ltprc \
	#$(srcdir)/configs/3node-stcp-ltp/host1.bprc \
	#$(srcdir)/configs/3node-stcp-ltp/host1.rc \
	#$(srcdir)/configs/3node-stcp-ltp/host2.ltprc \
	#$(srcdir)/configs/3node-stcp-ltp/host2.ipnrc \
	#$(srcdir)/configs/3node-stcp-ltp/host2.ionrc \
	#$(srcdir)/configs/3node-stcp-ltp/host2.bprc \
	#$(srcdir)/configs/3node-stcp-ltp/host2.rc \
	#$(srcdir)/configs/3node-stcp-ltp/host3.ipnrc \
	#$(srcdir)/configs/3node-stcp-ltp/host3.ionrc \
	#$(srcdir)/configs/3node-stcp-ltp/host3.bprc \
	#$(srcdir)/configs/3node-stcp-ltp/host3.rc

# nasa-JPL's brs section has 3 directories inside, node10 node5 node7
node5dir = $(pkgdatadir)/configs/3node-ltp-brs/iontest.ipn.node5
node5_DATA = \
	$(srcdir)/configs/3node-ltp-brs/iontest.ipn.node5/ionconfig \
	$(srcdir)/configs/3node-ltp-brs/iontest.ipn.node5/ionstart \
	$(srcdir)/configs/3node-ltp-brs/iontest.ipn.node5/node5.bprc \
	$(srcdir)/configs/3node-ltp-brs/iontest.ipn.node5/node5.ionrc \
	$(srcdir)/configs/3node-ltp-brs/iontest.ipn.node5/node5.ipnrc \
	$(srcdir)/configs/3node-ltp-brs/iontest.ipn.node5/node5.ltprc
node7dir = $(pkgdatadir)/configs/3node-ltp-brs/iontest.ipn.node7
node7_DATA = \
	$(srcdir)/configs/3node-ltp-brs/iontest.ipn.node7/ionconfig \
	$(srcdir)/configs/3node-ltp-brs/iontest.ipn.node7/ionstart \
	$(srcdir)/configs/3node-ltp-brs/iontest.ipn.node7/node7.bprc \
	$(srcdir)/configs/3node-ltp-brs/iontest.ipn.node7/node7.ionrc \
	$(srcdir)/configs/3node-ltp-brs/iontest.ipn.node7/node7.ipnrc \
	$(srcdir)/configs/3node-ltp-brs/iontest.ipn.node7/node7.ltprc
node10dir = $(pkgdatadir)/configs/3node-ltp-brs/iontest.ipn.node10
node10_DATA = \
	$(srcdir)/configs/3node-ltp-brs/iontest.ipn.node10/ionconfig \
	$(srcdir)/configs/3node-ltp-brs/iontest.ipn.node10/ionstart \
	$(srcdir)/configs/3node-ltp-brs/iontest.ipn.node10/node10.bprc \
	$(srcdir)/configs/3node-ltp-brs/iontest.ipn.node10/node10.ionrc \
	$(srcdir)/configs/3node-ltp-brs/iontest.ipn.node10/node10.ipnrc \
	$(srcdir)/configs/3node-ltp-brs/iontest.ipn.node10/node10.ltprc

#############
#
# ICI SECTION
#
#############

# Define pod2man stuff
iciPODM1 = pod2man -s 1 -c "ICI executables"
iciPODM3 = pod2man -s 3 -c "ICI library functions"
iciPODM5 = pod2man -s 5 -c "ICI configuration files"

icicflags = -I$(srcdir)/ici/library -I$(srcdir)/ici/crypto -I$(srcdir)/bp/library -I$(srcdir)/ltp/library -I$(srcdir)/ici/test -I$(srcdir)/ici/sdr -I$(srcdir)/ici/libbloom-master -I$(srcdir)/ici/libbloom-master/murmur2

icibin = \
	sdrwatch \
	psmwatch \
	ionadmin \
	ionexit \
	ionsecadmin \
	ionwarn \
	sdrmend \
	file2sm \
	sm2file \
	file2sdr \
	sdr2file \
	psmshell \
	smlistsh \
	smrbtsh \
	rfxclock \
	owlttb \
	owltsim

icilib = \
	libici.la
#	librfx.la

iciinclude = \
	ici/include/llcv.h \
	ici/include/platform.h \
	ici/include/platform_sm.h \
	ici/include/memmgr.h \
	ici/include/lyst.h \
	ici/include/psm.h \
	ici/include/smlist.h \
	ici/include/sptrace.h \
	ici/include/ion.h \
	ici/include/rfx.h \
	ici/include/ionsec.h \
	ici/include/zco.h \
	ici/include/sdrxn.h \
	ici/include/sdrmgt.h \
	ici/include/sdrstring.h \
	ici/include/sdrlist.h \
	ici/include/sdrtable.h \
	ici/include/sdrhash.h \
	ici/include/sdr.h \
	ici/include/smrbt.h \
	ici/include/icinm.h

icinoinst = \
	ici/library/lystP.h \
	ici/sdr/sdrP.h \
	ici/test/file2sdr.h

iciextra = \
	ici/README.txt \
	ici/library/symtab.c \
	ici/doc/pod1/ionadmin.pod \
	ici/doc/pod1/ionsecadmin.pod \
	ici/doc/pod1/rfxclock.pod \
	ici/doc/pod1/psmwatch.pod \
	ici/doc/pod1/sdrwatch.pod \
	ici/doc/pod1/sdrmend.pod \
	ici/doc/pod1/file2sdr.pod \
	ici/doc/pod1/file2sm.pod \
	ici/doc/pod1/psmshell.pod \
	ici/doc/pod1/sdr2file.pod \
	ici/doc/pod1/sm2file.pod \
	ici/doc/pod1/smlistsh.pod \
	ici/doc/pod1/smrbtsh.pod \
	ici/doc/pod1/owltsim.pod \
	ici/doc/pod1/owlttb.pod \
	ici/doc/pod5/ionconfig.pod \
	ici/doc/pod5/ionrc.pod \
	ici/doc/pod5/ionsecrc.pod \
	ici/doc/pod3/platform.pod \
	ici/doc/pod3/memmgr.pod \
	ici/doc/pod3/ion.pod \
	ici/doc/pod3/llcv.pod \
	ici/doc/pod3/lyst.pod \
	ici/doc/pod3/psm.pod \
	ici/doc/pod3/zco.pod \
	ici/doc/pod3/smlist.pod \
	ici/doc/pod3/sdrlist.pod \
	ici/doc/pod3/sdrstring.pod \
	ici/doc/pod3/sdrtable.pod \
	ici/doc/pod3/sdrhash.pod \
	ici/doc/pod3/sdr.pod

icimans = \
	$(top_builddir)/ici/doc/ionadmin.1 \
	$(top_builddir)/ici/doc/ionsecadmin.1 \
	$(top_builddir)/ici/doc/rfxclock.1 \
	$(top_builddir)/ici/doc/psmwatch.1 \
	$(top_builddir)/ici/doc/sdrwatch.1 \
	$(top_builddir)/ici/doc/sdrmend.1 \
	$(top_builddir)/ici/doc/file2sdr.1 \
	$(top_builddir)/ici/doc/file2sm.1 \
	$(top_builddir)/ici/doc/psmshell.1 \
	$(top_builddir)/ici/doc/sdr2file.1 \
	$(top_builddir)/ici/doc/sm2file.1 \
	$(top_builddir)/ici/doc/smlistsh.1 \
	$(top_builddir)/ici/doc/smrbtsh.1 \
	$(top_builddir)/ici/doc/owltsim.1 \
	$(top_builddir)/ici/doc/owlttb.1 \
	$(top_builddir)/ici/doc/ionconfig.5 \
	$(top_builddir)/ici/doc/ionrc.5 \
	$(top_builddir)/ici/doc/ionsecrc.5 \
	$(top_builddir)/ici/doc/platform.3 \
	$(top_builddir)/ici/doc/memmgr.3 \
	$(top_builddir)/ici/doc/ion.3 \
	$(top_builddir)/ici/doc/llcv.3 \
	$(top_builddir)/ici/doc/lyst.3 \
	$(top_builddir)/ici/doc/psm.3 \
	$(top_builddir)/ici/doc/zco.3 \
	$(top_builddir)/ici/doc/smlist.3 \
	$(top_builddir)/ici/doc/sdrlist.3 \
	$(top_builddir)/ici/doc/sdrstring.3 \
	$(top_builddir)/ici/doc/sdrtable.3 \
	$(top_builddir)/ici/doc/sdrhash.3 \
	$(top_builddir)/ici/doc/sdr.3

iciclean-local:
	rm -f $(top_builddir)/ici/doc/*.1
	rm -f $(top_builddir)/ici/doc/*.3
	rm -f $(top_builddir)/ici/doc/*.5

$(top_builddir)/ici/doc/%.1: ici/doc/pod1/%.pod
	$(iciPODM1) $< $@

$(top_builddir)/ici/doc/%.3: ici/doc/pod3/%.pod
	$(iciPODM3) $< $@

$(top_builddir)/ici/doc/%.5: ici/doc/pod5/%.pod
	$(iciPODM5) $< $@

# -- Libraries --- #

libici_la_SOURCES =	ici/library/llcv.c \
			ici/library/libicinm.c \
			ici/library/platform.c \
			ici/library/platform_sm.c \
			ici/library/memmgr.c \
			ici/library/lyst.c \
			ici/library/psm.c \
			ici/library/smlist.c \
			ici/library/sptrace.c \
			ici/library/rfx.c \
			ici/library/ion.c \
			ici/library/ionsec.c \
			ici/library/zco.c \
			ici/library/smrbt.c \
			ici/sdr/sdrxn.c \
			ici/sdr/sdrmgt.c \
			ici/sdr/sdrstring.c \
			ici/sdr/sdrlist.c \
			ici/sdr/sdrtable.c \
			ici/sdr/sdrhash.c \
			ici/sdr/sdrcatlg.c \
			ici/bulk/STUB_BULK/bulk.c


libici_la_CFLAGS = $(icicflags) $(AM_CFLAGS)
libici_la_LIBADD = -lm

# --- Utility Programs --- #

sdrwatch_SOURCES = ici/utils/sdrwatch.c
sdrwatch_LDADD = libici.la $(LIBOBJS) -lm
sdrwatch_CFLAGS = $(icicflags) $(AM_CFLAGS)

sdrmend_SOURCES = ici/utils/sdrmend.c
sdrmend_LDADD = libici.la $(LIBOBJS) -lm
sdrmend_CFLAGS = $(icicflags) $(AM_CFLAGS)

psmwatch_SOURCES = ici/utils/psmwatch.c
psmwatch_LDADD = libici.la $(LIBOBJS) -lm
psmwatch_CFLAGS = $(icicflags) $(AM_CFLAGS)

ionadmin_SOURCES = ici/utils/ionadmin.c
ionadmin_LDADD = libici.la $(LIBOBJS) -lm
ionadmin_CFLAGS = $(icicflags) $(AM_CFLAGS)

ionexit_SOURCES = ici/utils/ionexit.c
ionexit_LDADD = libltp.la libici.la -lm libbp.la $(LIBOBJS)
ionexit_CFLAGS = $(icicflags) $(AM_CFLAGS)

ionsecadmin_SOURCES = ici/utils/ionsecadmin.c
ionsecadmin_LDADD = libici.la $(LIBOBJS) -lm
ionsecadmin_CFLAGS = $(icicflags) $(AM_CFLAGS)

ionwarn_SOURCES = ici/utils/ionwarn.c
ionwarn_LDADD = libici.la $(LIBOBJS) -lm
ionwarn_CFLAGS = $(icicflags) $(AM_CFLAGS)

# --- Test Executables --- #

psmshell_SOURCES = ici/test/psmshell.c
psmshell_LDADD = libici.la $(LIBOBJS) -lm
psmshell_CFLAGS = $(icicflags) $(AM_CFLAGS)

smlistsh_SOURCES = ici/test/smlistsh.c
smlistsh_LDADD = libici.la $(LIBOBJS) -lm
smlistsh_CFLAGS = $(icicflags) $(AM_CFLAGS)

smrbtsh_SOURCES = ici/test/smrbtsh.c
smrbtsh_LDADD = libici.la $(LIBOBJS) -lm
smrbtsh_CFLAGS = $(icicflags) $(AM_CFLAGS)

file2sm_SOURCES = ici/test/file2sm.c
file2sm_LDADD = libici.la $(LIBOBJS) -lm
file2sm_CFLAGS = $(icicflags) $(AM_CFLAGS)

sm2file_SOURCES = ici/test/sm2file.c
sm2file_LDADD = libici.la $(LIBOBJS) -lm
sm2file_CFLAGS = $(icicflags) $(AM_CFLAGS)

file2sdr_SOURCES = ici/test/file2sdr.c
file2sdr_LDADD = libici.la $(LIBOBJS) -lm
file2sdr_CFLAGS = $(icicflags) $(AM_CFLAGS)

sdr2file_SOURCES = ici/test/sdr2file.c
sdr2file_LDADD = libici.la $(LIBOBJS) -lm
sdr2file_CFLAGS = $(icicflags) $(AM_CFLAGS)

owltsim_SOURCES = ici/test/owltsim.c
owltsim_LDADD = libici.la $(LIBOBJS) $(PTHREAD_LIBS) -lm
owltsim_CFLAGS = $(icicflags) $(AM_CFLAGS)

owlttb_SOURCES = ici/test/owlttb.c
owlttb_LDADD = libici.la $(LIBOBJS) $(PTHREAD_LIBS) -lm
owlttb_CFLAGS = $(icicflags) $(AM_CFLAGS)

# --- Daemon Executables --- #

rfxclock_SOURCES = ici/daemon/rfxclock.c
rfxclock_LDADD = libici.la $(LIBOBJS) -lm
rfxclock_CFLAGS = $(icicflags) $(AM_CFLAGS)

##########################
#
# LTP SECTION
#
##########################

# Define pod2man stuff
ltpPODM1 = pod2man -s 1 -c "LTP executables"
ltpPODM3 = pod2man -s 3 -c "LTP library functions"
ltpPODM5 = pod2man -s 5 -c "LTP configuration files"

ltpmandir = $(top_builddir)/ltp/doc

ltpcflags = -I$(srcdir)/ltp/library -I$(srcdir)/ltp/library/ext

ltpbin = \
	ltpadmin \
	ltpclock \
	ltpmeter \
	udplsi \
	udplso \
	aoslsi \
	aoslso \
	ltpdriver \
	ltpcounter \
	sdatest \
	dccplsi	 \
	dccplso

ltplib = \
	libltp.la

ltpinclude = \
	ltp/include/ltp.h \
	ltp/include/sda.h

ltpnoinst = \
	ltp/library/ltpP.h \
	ltp/udp/udplsa.h \
	ltp/aos/aoslsa.h \
	ltp/dccp/dccplsa.h

ltpextra = \
	ltp/README.txt \
	ltp/doc/pod1/ltpadmin.pod \
	ltp/doc/pod1/ltpclock.pod \
	ltp/doc/pod1/ltpcounter.pod \
	ltp/doc/pod1/ltpdriver.pod \
	ltp/doc/pod1/ltpmeter.pod \
	ltp/doc/pod1/udplsi.pod \
	ltp/doc/pod1/udplso.pod \
	ltp/doc/pod1/dccplsi.pod \
	ltp/doc/pod1/dccplso.pod \
	ltp/doc/pod5/ltprc.pod \
	ltp/doc/pod3/ltp.pod

ltpmans = \
	$(top_builddir)/ltp/doc/ltpadmin.1 \
	$(top_builddir)/ltp/doc/ltpclock.1 \
	$(top_builddir)/ltp/doc/ltpcounter.1 \
	$(top_builddir)/ltp/doc/ltpdriver.1 \
	$(top_builddir)/ltp/doc/ltpmeter.1 \
	$(top_builddir)/ltp/doc/udplsi.1 \
	$(top_builddir)/ltp/doc/udplso.1 \
	$(top_builddir)/ltp/doc/dccplsi.1 \
	$(top_builddir)/ltp/doc/dccplso.1 \
	$(top_builddir)/ltp/doc/ltprc.5 \
	$(top_builddir)/ltp/doc/ltp.3

ltpclean-local:
	rm -f $(top_builddir)/ltp/doc/*.1
	rm -f $(top_builddir)/ltp/doc/*.3
	rm -f $(top_builddir)/ltp/doc/*.5

$(top_builddir)/ltp/doc/%.1: ltp/doc/pod1/%.pod
	$(ltpPODM1) $< $@

$(top_builddir)/ltp/doc/%.3: ltp/doc/pod3/%.pod
	$(ltpPODM3) $< $@

$(top_builddir)/ltp/doc/%.5: ltp/doc/pod5/%.pod
	$(ltpPODM5) $< $@



# -- ECLSA -- #

if BUILD_ECLSA

ltpbin += eclso \
	  eclsi

ltplib += libeclsa.la


ltpextra += ltp/doc/pod1/eclso.pod \
	    ltp/doc/pod1/eclsi.pod

endif





# -- Libraries --- #

libltp_la_SOURCES = \
	ltp/library/libltp.c \
	ltp/library/libltpP.c \
	ltp/library/ltpei.c \
	ltp/sda/libsda.c \
	ltp/library/libltpnm.c
libltp_la_CFLAGS = $(ltpcflags) $(AM_CFLAGS)
libltp_la_LIBADD = libici.la -lm

# --- Utility Programs --- #

ltpadmin_SOURCES = ltp/utils/ltpadmin.c
ltpadmin_LDADD = libltp.la libici.la $(LIBOBJS)
ltpadmin_CFLAGS = $(ltpcflags) $(AM_CFLAGS)

# --- Test Executables --- #

ltpdriver_SOURCES = ltp/test/ltpdriver.c
ltpdriver_LDADD = libltp.la libici.la $(LIBOBJS)
ltpdriver_CFLAGS = $(ltpcflags) $(AM_CFLAGS)

ltpcounter_SOURCES = ltp/test/ltpcounter.c
ltpcounter_LDADD = libltp.la libici.la $(LIBOBJS)
ltpcounter_CFLAGS = $(ltpcflags) $(AM_CFLAGS)

sdatest_SOURCES = ltp/test/sdatest.c
sdatest_LDADD = libltp.la libici.la $(LIBOBJS)
sdatest_CFLAGS = $(ltpcflags) $(AM_CFLAGS)

# --- Daemon Executables --- #

ltpclock_SOURCES = ltp/daemon/ltpclock.c
ltpclock_LDADD = libltp.la libici.la $(LIBOBJS)
ltpclock_CFLAGS = $(ltpcflags) $(AM_CFLAGS)

ltpmeter_SOURCES = ltp/daemon/ltpmeter.c
ltpmeter_LDADD = libltp.la libici.la $(LIBOBJS)
ltpmeter_CFLAGS = $(ltpcflags) $(AM_CFLAGS)

# --- UDP Executables --- #

udplsi_SOURCES = ltp/udp/udplsi.c
udplsi_LDADD = libltp.la libici.la $(LIBOBJS) $(PTHREAD_LIBS)
udplsi_CFLAGS = $(ltpcflags) $(AM_CFLAGS)

udplso_SOURCES = ltp/udp/udplso.c
udplso_LDADD = libltp.la libici.la $(LIBOBJS) $(PTHREAD_LIBS)
udplso_CFLAGS = $(ltpcflags) $(AM_CFLAGS)


if BUILD_ECLSA
# --- ECLSA Executables and Library --- #

libeclsa_la_SOURCES = contrib/ECLSAv2/ion-open-source/ltp/ec/eclsa/elements/sys/eclsaTimer.c \
	              contrib/ECLSAv2/ion-open-source/ltp/ec/eclsa/elements/sys/eclsaLogger.c \
		      contrib/ECLSAv2/ion-open-source/ltp/ec/eclsa/elements/matrix/eclsaCodecMatrix.c \
		      contrib/ECLSAv2/ion-open-source/ltp/ec/eclsa/elements/matrix/eclsaMatrix.c \
		      contrib/ECLSAv2/ion-open-source/ltp/ec/eclsa/elements/matrix/eclsaMatrixBuffer.c \
		      contrib/ECLSAv2/ion-open-source/ltp/ec/eclsa/elements/fec/eclsaFecManager.c \
		      contrib/ECLSAv2/ion-open-source/ltp/ec/eclsa/elements/packet/eclsaSendingSequence.c \
		      contrib/ECLSAv2/ion-open-source/ltp/ec/eclsa/elements/packet/eclsaBlacklist.c \
		      contrib/ECLSAv2/ion-open-source/ltp/ec/eclsa/elements/packet/eclsaPacket.c \
		      contrib/ECLSAv2/ion-open-source/ltp/ec/eclsa/elements/packet/eclsaFeedback.c 


libeclsa_la_CFLAGS = -I$(srcdir)/ici/include \
		     -I$(srcdir)/ltp/library 
		     

eclsX_LDADD = libeclsa.la 

eclso_SOURCES = contrib/ECLSAv2/ion-open-source/ltp/ec/eclsa/eclso.c 
eclso_LDADD = $(eclsX_LDADD) libltp.la libici.la $(LIBOBJS) -lrt -L$(srcdir)/contrib/ECLSAv2/ion-open-source/ltp/ec/openfec/bin/Debug
eclso_CFLAGS = $(eclsX_CFLAGS) $(ltpcflags) $(AM_CFLAGS) 

eclsi_SOURCES = contrib/ECLSAv2/ion-open-source/ltp/ec/eclsa/eclsi.c 
eclsi_LDADD = $(eclsX_LDADD) libltp.la libici.la $(LIBOBJS) -lrt -L$(srcdir)/contrib/ECLSAv2/ion-open-source/ltp/ec/openfec/bin/Debug
eclsi_CFLAGS = $(eclsX_CFLAGS) $(ltpcflags) $(AM_CFLAGS)


if ECLSA_CODEC_DUMMYDEC
libeclsa_la_SOURCES +=	contrib/ECLSAv2/ion-open-source/ltp/ec/eclsa/adapters/codec/eclsaCodecAdapter_DummyDEC.c		
endif

if ECLSA_CODEC_OPENFEC
libeclsa_la_SOURCES +=  contrib/ECLSAv2/ion-open-source/ltp/ec/eclsa/adapters/codec/eclsaCodecAdapter_OpenFEC.c
libeclsa_la_CFLAGS += -I$(srcdir)/contrib/ECLSAv2/ion-open-source/ltp/ec/openfec/src/lib_common
eclso_LDADD += -lopenfec
eclsi_LDADD += -lopenfec
endif

if ECLSA_CODEC_LIBECDLR
libeclsa_la_SOURCES +=	contrib/ECLSAv2/ion-open-source/ltp/ec/libec/decoder.c \
			contrib/ECLSAv2/ion-open-source/ltp/ec/libec/encoder.c \
			contrib/ECLSAv2/ion-open-source/ltp/ec/libec/ec_graph.c \
			contrib/ECLSAv2/ion-open-source/ltp/ec/eclsa/adapters/codec/eclsaCodecAdapter_LibecDLR.c

libeclsa_la_CFLAGS += -I$(srcdir)/contrib/ECLSAv2/ion-open-source/ltp/ec/libec 
endif

if ECLSA_LP_UDP
libeclsa_la_SOURCES +=	contrib/ECLSAv2/ion-open-source/ltp/ec/eclsa/adapters/protocol/eclsaProtocolAdapter_LP_UDP.c 
endif

if ECLSA_UP_LTP
libeclsa_la_SOURCES +=	contrib/ECLSAv2/ion-open-source/ltp/ec/eclsa/adapters/protocol/eclsaProtocolAdapter_UP_ION_LTP.c 
endif


endif


# --- AOS Executables --- #

aoslsi_SOURCES = ltp/aos/aoslsi.c
aoslsi_LDADD = libltp.la libici.la $(LIBOBJS) $(PTHREAD_LIBS)
aoslsi_CFLAGS = $(ltpcflags) $(AM_CFLAGS)

aoslso_SOURCES = ltp/aos/aoslso.c
aoslso_LDADD = libltp.la libici.la $(LIBOBJS)
aoslso_CFLAGS = $(ltpcflags) $(AM_CFLAGS)

# --- DCCP Executables --- #
dccplsi_SOURCES = ltp/dccp/dccplsi.c
dccplsi_LDADD = libltp.la libici.la $(LIBOBJS)
dccplsi_CFLAGS = $(ltpcflags) $(AM_CFLAGS) $(PTHREAD_LIBS)
dccplso_SOURCES = ltp/dccp/dccplso.c
dccplso_LDADD = libltp.la libici.la $(LIBOBJS) $(PTHREAD_LIBS)
dccplso_CFLAGS = $(ltpcflags) $(AM_CFLAGS)


##########################
#
# DGR SECTION
#
##########################

# Define pod2man stuff
dgrPODM1 = pod2man -s 1 -c "DGR executables"
dgrPODM3 = pod2man -s 3 -c "DGR library functions"

dgrcflags = -I$(srcdir)/dgr/library -I$(srcdir)/dgr/test

dgrbin = \
	file2dgr \
	dgr2file \
	file2tcp \
	tcp2file \
	file2udp \
	udp2file

dgrlib = \
	libdgr.la

dgrinclude = \
	dgr/include/dgr.h

dgrnoinst = \
	dgr/test/file2dgr.h \
	dgr/test/file2tcp.h \
	dgr/test/file2udp.h

dgrextra = \
	dgr/README.txt \
	dgr/doc/pod3/dgr.pod

dgrmans = \
	$(top_builddir)/dgr/doc/dgr.3

dgrclean-local:
	rm -f $(top_builddir)/dgr/doc/*.1
	rm -f $(top_builddir)/dgr/doc/*.3

$(top_builddir)/dgr/doc/%.1: dgr/doc/pod1/%.pod
	$(dgrPODM1) $< $@f

$(top_builddir)/dgr/doc/%.3: dgr/doc/pod3/%.pod
	$(dgrPODM3) $< $@

# -- Libraries --- #

libdgr_la_SOURCES = dgr/library/libdgr.c
libdgr_la_CFLAGS = $(dgrcflags) $(AM_CFLAGS)
libdgr_la_LIBADD = libici.la -lm

# --- Test Executables --- #

file2dgr_SOURCES = dgr/test/file2dgr.c
file2dgr_LDADD = libdgr.la libici.la $(LIBOBJS) -lm
file2dgr_CFLAGS = $(dgrcflags) $(AM_CFLAGS)

dgr2file_SOURCES = dgr/test/dgr2file.c
dgr2file_LDADD = libdgr.la libici.la $(LIBOBJS) -lm
dgr2file_CFLAGS = $(dgrcflags) $(AM_CFLAGS)

file2udp_SOURCES = dgr/test/file2udp.c
file2udp_LDADD = libici.la $(LIBOBJS) -lm
file2udp_CFLAGS = $(dgrcflags) $(AM_CFLAGS)

udp2file_SOURCES = dgr/test/udp2file.c
udp2file_LDADD = libici.la $(LIBOBJS) -lm
udp2file_CFLAGS = $(dgrcflags) $(AM_CFLAGS)

file2tcp_SOURCES = dgr/test/file2tcp.c
file2tcp_LDADD = libici.la $(LIBOBJS) -lm
file2tcp_CFLAGS = $(dgrcflags) $(AM_CFLAGS)

tcp2file_SOURCES = dgr/test/tcp2file.c
tcp2file_LDADD = libici.la $(LIBOBJS) -lm
tcp2file_CFLAGS = $(dgrcflags) $(AM_CFLAGS)

##########################
#
# BP SECTION
#
##########################

# Define pod2man stuff
bpPODM1 = pod2man -s 1 -c "BP executables"
bpPODM3 = pod2man -s 3 -c "BP library functions"
bpPODM5 = pod2man -s 5 -c "BP configuration files"

bpcflags = -I$(srcdir)/bp/library -I$(srcdir)/bp/library/acs -I$(srcdir)/bp/library/ext -I$(srcdir)/bp/library/ext/meb -I$(srcdir)/bp/library/ext/bae -I$(srcdir)/bp/library/ext/cteb -I$(srcdir)/bp/include -I$(srcdir)/bp/ipn -I$(srcdir)/bp/ipnd -I$(srcdir)/ici/libbloom-master -I$(srcdir)/ici/libbloom-master/murmur2 -I$(srcdir)/bp/dtn2 -I$(srcdir)/bp/library/crypto -I$(srcdir)/bp/library/ext/ecos -I$(srcdir)/bp/library/ext/bsp -I$(srcdir)/bp/library/ext/bsp/ciphersuites -I$(srcdir)/bp/library/ext/phn -I$(srcdir)/bp/imc -I$(srcdir)/bp/tcp -I$(srcdir)/bp/udp -I$(srcdir)/ici/include -I$(srcdir)/bp/library/ext/ecos -I$(srcdir)/bp/library/ext/bsp -I$(srcdir)/bp/imc -DBP_EXTENDED

bpbin = \
	bpadmin \
	bpsink \
	bpdriver \
	bpsource \
	bpecho \
	bpcancel \
	bpcounter \
	bplist \
	bpsendfile \
	bprecvfile \
	bpclm \
	bpclock \
	bptransit \
	ipnadmin \
	ipnfw \
	bsspcli \
	bsspclo \
	ipnadminep \
	dtn2admin \
	dtn2fw \
	dtn2adminep \
	ipnd \
	beacon \
	node \
	tcpcli \
	tcpclo \
	stcpcli \
	stcpclo \
	brsscla \
	brsccla \
	udpcli \
	udpclo \
	bibeclo \
	dccpcli \
	dccpclo	\
	dgrcli \
	dgrclo \
	ltpcli \
	ltpclo \
	lgsend \
	lgagent \
	bptrace \
	bping \
	bpnmtest \
	bpstats \
	bpstats2 \
	bpchat \
	hmackeys \
	cgrfetch

if ENABLE_BPACS
bpbin += acsadmin \
	acslist
endif

if ENABLE_IMC
bpbin += imcadmin \
	imcfw
endif

bplib = \
	libbp.la \
	libcgr.la \
	libstcpcla.la \
	libudpcla.la
#	libphn.la
#	libecos.la

bpinclude = \
	bp/include/bp.h \
	bp/include/eureka.h
#bp/include/bpP.h \
#bp/include/bei.h

#ecos.h goes here?
#bp/library/ecos.h
bpnoinst = \
	bp/library/bpP.h \
	bp/ipn/ipnfw.h \
	bp/ipnd/ipndP.h \
	bp/dtn2/dtn2fw.h \
	bp/brs/brscla.h \
	bp/udp/udpcla.h \
	bp/ltp/ltpcla.h \
	bp/stcp/stcpcla.h \
	bp/dccp/dccpcla.h
#	bp/library/ext/phn/phn.h

bpextra = \
	bp/doc/pod1/bpadmin.pod \
	bp/doc/pod1/bpclock.pod \
	bp/doc/pod1/bptransit.pod \
	bp/doc/pod1/bpcounter.pod \
	bp/doc/pod1/bpcancel.pod \
	bp/doc/pod1/bpclm.pod \
	bp/doc/pod1/bplist.pod \
	bp/doc/pod1/bpdriver.pod \
	bp/doc/pod1/bpecho.pod \
	bp/doc/pod1/bpsendfile.pod \
	bp/doc/pod1/bprecvfile.pod \
	bp/doc/pod1/bpsink.pod \
	bp/doc/pod1/bpsource.pod \
	bp/doc/pod1/bpstats.pod \
	bp/doc/pod1/bptrace.pod \
	bp/doc/pod1/brsccla.pod \
	bp/doc/pod1/brsscla.pod \
	bp/doc/pod1/dgrcli.pod \
	bp/doc/pod1/dgrclo.pod \
	bp/doc/pod1/dtn2adminep.pod \
	bp/doc/pod1/dtn2admin.pod \
	bp/doc/pod1/dtn2fw.pod \
	bp/doc/pod1/ipnadminep.pod \
	bp/doc/pod1/ipnadmin.pod \
	bp/doc/pod1/ipnfw.pod \
	bp/doc/pod1/lgagent.pod \
	bp/doc/pod1/lgsend.pod \
	bp/doc/pod1/ltpcli.pod \
	bp/doc/pod1/ltpclo.pod \
	bp/doc/pod1/stcpcli.pod \
	bp/doc/pod1/tcpcli.pod \
	bp/doc/pod1/stcpclo.pod \
	bp/doc/pod1/tcpclo.pod \
	bp/doc/pod1/udpcli.pod \
	bp/doc/pod1/udpclo.pod \
	bp/doc/pod1/bibeclo.pod \
	bp/doc/pod1/dccpcli.pod \
	bp/doc/pod1/dccpclo.pod \
	bp/doc/pod1/bping.pod \
	bp/doc/pod1/bpnmtest.pod \
	bp/doc/pod1/bpstats2.pod \
	bp/doc/pod1/bpchat.pod \
	bp/doc/pod1/hmackeys.pod \
	bp/doc/pod5/bprc.pod \
	bp/doc/pod5/ipnrc.pod \
	bp/doc/pod5/bssrc.pod \
	bp/doc/pod5/dtn2rc.pod \
	bp/doc/pod5/lgfile.pod \
	bp/doc/pod3/bp.pod \
	bp/README.txt \
	bp/doc/pod3/bpextensions.pod \
	bp/library/bpextensions.c \
	bp/library/noextensions.c \
	bp/library/ext/ecos/ecos.h \
	bp/library/ext/ecos/ecos.c \
	bp/library/ext/meb \
	bp/library/ext/meb/meb.h \
	bp/library/ext/meb/meb.c \
	bp/library/bei.c \
	bp/library/ext/bae/bae.h \
	bp/library/ext/bae/bae.c \
	bp/library/ext/bpsec/bpsec_instr.h \
	bp/library/ext/bpsec/bpsec_instr.c \
	bp/library/ext/bpsec/bpsec_bcb.h \
	bp/library/ext/bpsec/bpsec_bcb.c \
	bp/library/ext/bpsec/bpsec_bib.h \
	bp/library/ext/bpsec/bpsec_bib.c \
	bp/library/ext/bpsec/bpsec_util.h \
	bp/library/ext/bpsec/bpsec_util.c \
	bp/library/ext/bpsec/profiles.h \
	bp/library/ext/bpsec/profiles.c \
	ici/include/crypto.h \
	ici/include/csi.h 

# noextensions.c is C code, but it isn't explicitly compiled.
# instead, the whole file is #included into libbpP.c
# For this reason, it is extra_DIST.
# ecos and myextensions are optional and it is unclear if they must be
# installed in the normal case- so we shall leave them extra_DIST for now.

bpmans = \
	$(top_builddir)/bp/doc/bpadmin.1 \
	$(top_builddir)/bp/doc/bpclock.1 \
	$(top_builddir)/bp/doc/bptransit.1 \
	$(top_builddir)/bp/doc/bpcounter.1 \
	$(top_builddir)/bp/doc/bpcancel.1 \
	$(top_builddir)/bp/doc/bplist.1 \
	$(top_builddir)/bp/doc/bpdriver.1 \
	$(top_builddir)/bp/doc/bpecho.1 \
	$(top_builddir)/bp/doc/bpsendfile.1 \
	$(top_builddir)/bp/doc/bprecvfile.1 \
	$(top_builddir)/bp/doc/bpsink.1 \
	$(top_builddir)/bp/doc/bpsource.1 \
	$(top_builddir)/bp/doc/bpstats.1 \
	$(top_builddir)/bp/doc/bptrace.1 \
	$(top_builddir)/bp/doc/brsccla.1 \
	$(top_builddir)/bp/doc/brsscla.1 \
	$(top_builddir)/bp/doc/dgrcli.1 \
	$(top_builddir)/bp/doc/dgrclo.1 \
	$(top_builddir)/bp/doc/dtn2adminep.1 \
	$(top_builddir)/bp/doc/dtn2admin.1 \
	$(top_builddir)/bp/doc/dtn2fw.1 \
	$(top_builddir)/bp/doc/ipnadminep.1 \
	$(top_builddir)/bp/doc/ipnadmin.1 \
	$(top_builddir)/bp/doc/ipnfw.1 \
	$(top_builddir)/bp/doc/lgagent.1 \
	$(top_builddir)/bp/doc/lgsend.1 \
	$(top_builddir)/bp/doc/ltpcli.1 \
	$(top_builddir)/bp/doc/ltpclo.1 \
	$(top_builddir)/bp/doc/stcpcli.1 \
	$(top_builddir)/bp/doc/tcpcli.1 \
	$(top_builddir)/bp/doc/stcpclo.1 \
	$(top_builddir)/bp/doc/tcpclo.1 \
	$(top_builddir)/bp/doc/udpcli.1 \
	$(top_builddir)/bp/doc/udpclo.1 \
	$(top_builddir)/bp/doc/bibeclo.1 \
	$(top_builddir)/bp/doc/dccpcli.1 \
	$(top_builddir)/bp/doc/dccpclo.1 \
	$(top_builddir)/bp/doc/cgrfetch.1 \
	$(top_builddir)/bp/doc/bping.1 \
	$(top_builddir)/bp/doc/bpnmtest.1 \
	$(top_builddir)/bp/doc/bpstats2.1 \
	$(top_builddir)/bp/doc/bpchat.1 \
	$(top_builddir)/bp/doc/hmackeys.1 \
	$(top_builddir)/bp/doc/bprc.5 \
	$(top_builddir)/bp/doc/ipnrc.5 \
	$(top_builddir)/bp/doc/bssrc.5 \
	$(top_builddir)/bp/doc/dtn2rc.5 \
	$(top_builddir)/bp/doc/lgfile.5 \
	$(top_builddir)/bp/doc/bp.3 \
	$(top_builddir)/bp/doc/bpextensions.3

if ENABLE_BPACS
bpmans += \
	$(top_builddir)/bp/doc/acsadmin.1 \
	$(top_builddir)/bp/doc/acslist.1 \
	$(top_builddir)/bp/doc/acsrc.5
endif

if ENABLE_IMC
bpmans += \
	$(top_builddir)/bp/doc/imcadmin.1 \
	$(top_builddir)/bp/doc/imcfw.1 \
	$(top_builddir)/bp/doc/imcrc.5
endif


bpclean-local:
	rm -f $(top_builddir)/bp/doc/*.1
	rm -f $(top_builddir)/bp/doc/*.3
	rm -f $(top_builddir)/bp/doc/*.5

$(top_builddir)/bp/doc/%.1: bp/doc/pod1/%.pod
	$(bpPODM1) $< $@

$(top_builddir)/bp/doc/%.3: bp/doc/pod3/%.pod
	$(bpPODM3) $< $@

$(top_builddir)/bp/doc/%.5: bp/doc/pod5/%.pod
	$(bpPODM5) $< $@

# -- Libraries --- #

libstcpcla_la_SOURCES = bp/stcp/libstcpcla.c
libstcpcla_la_CFLAGS = $(bpcflags) $(AM_CFLAGS)
libstcpcla_la_LIBADD = libbp.la libici.la

libudpcla_la_SOURCES = bp/udp/libudpcla.c
libudpcla_la_CFLAGS = $(bpcflags) $(AM_CFLAGS)
libudpcla_la_LIBADD = libbp.la libici.la



libbp_la_SOURCES = \
	bp/dtn2/libdtn2fw.h \
	bp/dtn2/libdtn2fw.c \
	bp/library/libbp.c \
	bp/library/libbpP.c \
	bp/library/libbpnm.c \
	bp/library/eureka.c \
	bp/library/ext/phn/phn.c \
	bp/library/ext/ecos/ecos.c \
	bp/library/bei.c \
	bp/library/ext/meb/meb.c \
	bp/library/ext/bae/bae.c \
	bp/library/ext/bpsec/bpsec_instr.h \
	bp/library/ext/bpsec/bpsec_instr.c \
	bp/library/ext/bpsec/bpsec_bcb.h \
	bp/library/ext/bpsec/bpsec_bcb.c \
	bp/library/ext/bpsec/bpsec_bib.h \
	bp/library/ext/bpsec/bpsec_bib.c \
	bp/library/ext/bpsec/bpsec_util.h \
	bp/library/ext/bpsec/bpsec_util.c \
	bp/library/ext/bpsec/profiles.h \
	bp/library/ext/bpsec/profiles.c \
	bp/library/ext/snid/snid.c \
	bp/ipn/libipnfw.h \
	bp/ipn/libipnfw.c \
	bp/ipnd/ipnd.c \
	bp/ipnd/beacon.c \
	bp/ipnd/helper.c \
	bp/ipnd/node.c \
	ici/libbloom-master/bloom.h \
	ici/libbloom-master/bloom.c \
	ici/libbloom-master/murmur2/murmurhash2.h \
	ici/libbloom-master/murmur2/MurmurHash2.c

if CRYPTO
libici_la_SOURCES+= \
	ici/crypto/NULL_SUITES/crypto.c\
	ici/crypto/NULL_SUITES/csi.c
else
libici_la_SOURCES+= \
	ici/crypto/NULL_SUITES/crypto.c\
	ici/crypto/NULL_SUITES/csi.c
endif
libbp_la_CFLAGS = $(bpcflags) $(AM_CFLAGS) -I$(srcdir)/bp/library/ext/bpsec
libbp_la_LIBADD = libici.la -lm $(CRYPTO_LIBS)
#libbp_la_LIBADD = libici.la libipnfw.la libdtn2fw.la $(CRYPTO_LIBS) -lm
#EJB
if ENABLE_BPACS
libbp_la_SOURCES += \
	bp/library/acs/acsrx.c \
	bp/library/acs/acstx.c \
	bp/library/acs/acsserialize.c \
	bp/library/acs/acsappend.c \
	bp/library/acs/acsid.c \
	bp/library/ext/cteb/cteb.c \
	$(NULL)
endif

if ENABLE_IMC
libbp_la_SOURCES += \
	bp/imc/libimcfw.c \
	$(NULL)
endif

libcgr_la_SOURCES = bp/cgr/libcgr.c
libcgr_la_CFLAGS = $(bpcflags) $(AM_CFLAGS)
libcgr_la_LIBADD = libbp.la libici.la

# --- Utility Programs --- #

bpadmin_SOURCES = bp/utils/bpadmin.c
bpadmin_LDADD = libbp.la libici.la $(LIBOBJS)
bpadmin_CFLAGS = $(bpcflags) $(AM_CFLAGS)

ipnadmin_SOURCES = bp/ipn/ipnadmin.c
ipnadmin_LDADD = libbp.la libici.la $(LIBOBJS)
ipnadmin_CFLAGS = $(bpcflags) $(AM_CFLAGS)

if ENABLE_BPACS
acsadmin_SOURCES = bp/utils/acsadmin.c
acsadmin_LDADD = libbp.la libici.la $(LIBOBJS)
acsadmin_CFLAGS = $(bpcflags) $(AM_CFLAGS)

acslist_SOURCES = bp/utils/acslist.c
acslist_LDADD = libbp.la libici.la $(LIBOBJS)
acslist_CFLAGS = $(bpcflags) $(AM_CFLAGS)
endif

if ENABLE_IMC
imcadmin_SOURCES = bp/imc/imcadmin.c
imcadmin_LDADD = libbp.la libici.la $(LIBOBJS)
imcadmin_CFLAGS = $(bpcflags) $(AM_CFLAGS)

imcfw_SOURCES = bp/imc/imcfw.c
imcfw_LDADD = libbp.la libici.la $(LIBOBJS)
imcfw_CFLAGS = $(bpcflags) $(AM_CFLAGS)
endif

dtn2admin_SOURCES = bp/dtn2/dtn2admin.c
dtn2admin_LDADD = libbp.la libici.la $(LIBOBJS)
dtn2admin_CFLAGS = $(bpcflags) $(AM_CFLAGS)

lgsend_SOURCES = bp/utils/lgsend.c
lgsend_LDADD = libbp.la libici.la $(LIBOBJS)
lgsend_CFLAGS = $(bpcflags) $(AM_CFLAGS)

lgagent_SOURCES = bp/utils/lgagent.c
lgagent_LDADD = libbp.la libici.la $(LIBOBJS)
lgagent_CFLAGS = $(bpcflags) $(AM_CFLAGS)

bpstats_SOURCES = bp/utils/bpstats.c
bpstats_LDADD = libbp.la libici.la $(LIBOBJS)
bpstats_CFLAGS = $(bpcflags) $(AM_CFLAGS)

bptrace_SOURCES = bp/utils/bptrace.c
bptrace_LDADD = libbp.la libici.la $(LIBOBJS)
bptrace_CFLAGS = $(bpcflags) $(AM_CFLAGS)

bpcancel_SOURCES = bp/utils/bpcancel.c
bpcancel_LDADD = libbp.la libici.la $(LIBOBJS)
bpcancel_CFLAGS = $(bpcflags) $(AM_CFLAGS)

bplist_SOURCES = bp/utils/bplist.c
bplist_LDADD = libbp.la libici.la $(LIBOBJS)
bplist_CFLAGS = $(bpcflags) $(AM_CFLAGS)

hmackeys_SOURCES = bp/utils/hmackeys.c
hmackeys_LDADD = libici.la -lm libbp.la $(LIBOBJS)
hmackeys_CFLAGS = $(bpcflags) $(AM_CFLAGS)

# --- DTN Executables --- #

dtn2fw_SOURCES = bp/dtn2/dtn2fw.c
dtn2fw_LDADD = libbp.la libici.la $(LIBOBJS)
dtn2fw_CFLAGS = $(bpcflags) $(AM_CFLAGS)

dtn2adminep_SOURCES = bp/dtn2/dtn2adminep.c
dtn2adminep_LDADD = libbp.la libici.la $(LIBOBJS)
dtn2adminep_CFLAGS = $(bpcflags) $(AM_CFLAGS)

# --- IPN Executables --- #

ipnfw_SOURCES = bp/ipn/ipnfw.c
ipnfw_LDADD = libcgr.la libbp.la libici.la $(LIBOBJS)
ipnfw_CFLAGS = $(bpcflags) $(AM_CFLAGS)

ipnadminep_SOURCES = bp/ipn/ipnadminep.c
ipnadminep_LDADD = libbp.la libici.la $(LIBOBJS)
ipnadminep_CFLAGS = $(bpcflags) $(AM_CFLAGS)

# --- IPND Executables --- #

#helper_SOURCES = bp/ipnd/helper.c
#helper_LDADD = libbp.la libipnfw.la  libdtn2fw.la libici.la $(LIBOBJS)
#helper_CFLAGS = $(bpcflags) $(AM_CFLAGS)

ipnd_SOURCES = bp/ipnd/ipnd.c
ipnd_LDADD = libcgr.la libbp.la libici.la $(LIBOBJS)
ipnd_CFLAGS = $(bpcflags) $(AM_CFLAGS)

beacon_SOURCES = bp/ipnd/beacon.c
beacon_LDADD = libbp.la libici.la $(LIBOBJS)
beacon_CFLAGS = $(bpcflags) $(AM_CFLAGS)

node_SOURCES = bp/ipnd/node.c
node_LDADD = libbp.la libici.la $(LIBOBJS)
node_CFLAGS = $(bpcflags) $(AM_CFLAGS)

# --- BSSP Executables --- #

bsspcli_SOURCES = bp/bssp/bsspcli.c
bsspcli_LDADD = libbssp.la libbp.la libici.la $(LIBOBJS) $(PTHREAD_LIBS)
bsspcli_CFLAGS = $(bpcflags) $(AM_CFLAGS)

bsspclo_SOURCES = bp/bssp/bsspclo.c
bsspclo_LDADD = libbssp.la libbp.la libici.la $(LIBOBJS)
bsspclo_CFLAGS = $(bpcflags) $(AM_CFLAGS)

# --- LTP Executables --- #

ltpcli_SOURCES = bp/ltp/ltpcli.c
ltpcli_LDADD = libbp.la libltp.la libici.la $(LIBOBJS) $(PTHREAD_LIBS)
ltpcli_CFLAGS = $(bpcflags) $(AM_CFLAGS)

ltpclo_SOURCES = bp/ltp/ltpclo.c
ltpclo_LDADD = libbp.la libltp.la libici.la $(LIBOBJS)
ltpclo_CFLAGS = $(bpcflags) $(AM_CFLAGS)

# --- TCP Executables --- #

tcpcli_SOURCES = bp/tcp/tcpcli.c
tcpcli_LDADD = libbp.la libici.la $(LIBOBJS) $(PTHREAD_LIBS)
tcpcli_CFLAGS = $(bpcflags) $(AM_CFLAGS)

tcpclo_SOURCES = bp/tcp/tcpclo.c
tcpclo_LDADD = libbp.la libici.la $(LIBOBJS) $(PTHREAD_LIBS)
tcpclo_CFLAGS = $(bpcflags) $(AM_CFLAGS)

# --- STCP Executables --- #

stcpcli_SOURCES = bp/stcp/stcpcli.c
stcpcli_LDADD = libbp.la libici.la libstcpcla.la $(LIBOBJS) $(PTHREAD_LIBS)
stcpcli_CFLAGS = $(bpcflags) $(AM_CFLAGS)

stcpclo_SOURCES = bp/stcp/stcpclo.c
stcpclo_LDADD = libbp.la libici.la libstcpcla.la $(LIBOBJS) $(PTHREAD_LIBS)
stcpclo_CFLAGS = $(bpcflags) $(AM_CFLAGS)

# --- UDP Executables --- #

udpcli_SOURCES = bp/udp/udpcli.c
udpcli_LDADD = libbp.la libudpcla.la libici.la $(LIBOBJS) $(PTHREAD_LIBS)
udpcli_CFLAGS = $(bpcflags) $(AM_CFLAGS)

udpclo_SOURCES = bp/udp/udpclo.c
udpclo_LDADD = libbp.la libudpcla.la libici.la $(LIBOBJS)
udpclo_CFLAGS = $(bpcflags) $(AM_CFLAGS)

# --- UDP Executables --- #

bibeclo_SOURCES = bp/bibe/bibeclo.c
bibeclo_LDADD = libbp.la libudpcla.la libici.la $(LIBOBJS)
bibeclo_CFLAGS = $(bpcflags) $(AM_CFLAGS)

# --- DCCP Executables --- #

dccpcli_SOURCES = bp/dccp/dccpcli.c
dccpcli_LDADD = libbp.la libici.la $(LIBOBJS) $(PTHREAD_LIBS)
dccpcli_CFLAGS = $(bpcflags) $(AM_CFLAGS)

dccpclo_SOURCES = bp/dccp/dccpclo.c
dccpclo_LDADD = libbp.la libici.la $(LIBOBJS) $(PTHREAD_LIBS)
dccpclo_CFLAGS = $(bpcflags) $(AM_CFLAGS)

# --- DGR Executables --- #

dgrcli_SOURCES = bp/dgr/dgrcli.c
dgrcli_LDADD = libbp.la libdgr.la libici.la $(LIBOBJS) $(PTHREAD_LIBS)
dgrcli_CFLAGS = $(bpcflags) $(AM_CFLAGS)

dgrclo_SOURCES = bp/dgr/dgrclo.c
dgrclo_LDADD = libbp.la libdgr.la libici.la $(LIBOBJS) $(PTHREAD_LIBS)
dgrclo_CFLAGS = $(bpcflags) $(AM_CFLAGS)

# --- BRS executables --- #

brsscla_SOURCES = bp/brs/brsscla.c
brsscla_LDADD = libbp.la libici.la libstcpcla.la $(LIBOBJS) $(PTHREAD_LIBS)
brsscla_CFLAGS = $(bpcflags) $(AM_CFLAGS)

brsccla_SOURCES = bp/brs/brsccla.c
brsccla_LDADD = libbp.la libici.la libstcpcla.la $(LIBOBJS) $(PTHREAD_LIBS)
brsccla_CFLAGS = $(bpcflags) $(AM_CFLAGS)

# --- Test Executables --- #

bpsource_SOURCES = bp/test/bpsource.c
bpsource_LDADD = libbp.la libici.la $(LIBOBJS)
bpsource_CFLAGS = $(bpcflags) $(AM_CFLAGS)

bpsink_SOURCES = bp/test/bpsink.c
bpsink_LDADD = libbp.la libici.la $(LIBOBJS)
bpsink_CFLAGS = $(bpcflags) $(AM_CFLAGS)

bpdriver_SOURCES = bp/test/bpdriver.c
bpdriver_LDADD = libbp.la libici.la $(LIBOBJS)
bpdriver_CFLAGS = $(bpcflags) $(AM_CFLAGS)

bpecho_SOURCES = bp/test/bpecho.c
bpecho_LDADD = libbp.la libici.la $(LIBOBJS)
bpecho_CFLAGS = $(bpcflags) $(AM_CFLAGS)

bpcounter_SOURCES = bp/test/bpcounter.c
bpcounter_LDADD = libbp.la libici.la $(LIBOBJS)
bpcounter_CFLAGS = $(bpcflags) $(AM_CFLAGS)

bpsendfile_SOURCES = bp/utils/bpsendfile.c
bpsendfile_LDADD = libbp.la libici.la $(LIBOBJS)
bpsendfile_CFLAGS = $(bpcflags) $(AM_CFLAGS)

bprecvfile_SOURCES = bp/utils/bprecvfile.c
bprecvfile_LDADD = libbp.la libici.la $(LIBOBJS)
bprecvfile_CFLAGS = $(bpcflags) $(AM_CFLAGS)

bping_SOURCES = bp/test/bping.c
bping_LDADD = libbp.la libici.la $(LIBOBJS) $(PTHREAD_LIBS)
bping_CFLAGS = $(bpcflags) $(AM_CFLAGS)

bpnmtest_SOURCES = bp/test/bpnmtest.c
bpnmtest_LDADD = libbp.la libici.la $(LIBOBJS)
bpnmtest_CFLAGS = $(bpcflags) $(AM_CFLAGS)

bpstats2_SOURCES = bp/test/bpstats2.c
bpstats2_LDADD = libbp.la libici.la $(LIBOBJS)
bpstats2_CFLAGS = $(bpcflags) $(AM_CFLAGS)

bpchat_SOURCES = bp/test/bpchat.c
bpchat_LDADD = libbp.la libici.la $(LIBOBJS) $(PTHREAD_LIBS)
bpchat_CFLAGS = $(bpcflags) $(AM_CFLAGS)

cgrfetch_SOURCES = bp/utils/cgrfetch.c
cgrfetch_LDADD = libbp.la libici.la libcgr.la $(LIBOBJS)
cgrfetch_CFLAGS = $(bpcflags) $(AM_CFLAGS)

# --- Daemon Executables --- #

bpclock_SOURCES = bp/daemon/bpclock.c
bpclock_LDADD = libbp.la libici.la $(LIBOBJS)
bpclock_CFLAGS = $(bpcflags) $(AM_CFLAGS)

bptransit_SOURCES = bp/daemon/bptransit.c
bptransit_LDADD = libbp.la libici.la $(LIBOBJS)
bptransit_CFLAGS = $(bpcflags) $(AM_CFLAGS)

bpclm_SOURCES = bp/daemon/bpclm.c
bpclm_LDADD = libbp.la libici.la $(LIBOBJS)
bpclm_CFLAGS = $(bpcflags) $(AM_CFLAGS)

##########################
#
# AMS SECTION
#
##########################

if !ION_NASA_B

# Define pod2man stuff
amsPODM1 = pod2man -s 1 -c "AMS executables"
amsPODM3 = pod2man -s 3 -c "AMS library functions"
amsPODM5 = pod2man -s 5 -c "AMS configuration files"
#amsPODH = pod2html --noindex

amscflags = -I$(srcdir)/ams/library -I$(srcdir)/ams/include -I$(srcdir)/ams/rams -DUDPTS -DTCPTS -DDGRTS

amsbin = \
	amsd \
	amshello \
	amsshell \
	amslog \
	amslogprt \
	amsbenchs \
	amsbenchr \
	amsstop \
	ramsgate

amslib = \
	libams.la

amsinclude = \
	ams/include/ams.h

amsnoinst = \
	ams/library/amscommon.h \
	ams/library/amsP.h \
	ams/rams/rams.h \
	ams/library/marshal.c \
	ams/library/default.marshal.c \
	ams/library/default.crypt.c \
	ams/rams/ramscommon.h

amsextra = \
	ams/README.txt \
	ams/library/vmqts.c \
	ams/test/amspubsub.c

amsmans = \
	$(top_builddir)/ams/doc/amshello.1 \
	$(top_builddir)/ams/doc/amsstop.1 \
	$(top_builddir)/ams/doc/amslogprt.1 \
	$(top_builddir)/ams/doc/amsbenchs.1 \
	$(top_builddir)/ams/doc/amsmib.1 \
	$(top_builddir)/ams/doc/amsbenchr.1 \
	$(top_builddir)/ams/doc/amspub.1 \
	$(top_builddir)/ams/doc/amsd.1 \
	$(top_builddir)/ams/doc/amssub.1 \
	$(top_builddir)/ams/doc/ramsgate.1 \
	$(top_builddir)/ams/doc/amsshell.1 \
	$(top_builddir)/ams/doc/amslog.1 \
	$(top_builddir)/ams/doc/amsrc.5 \
	$(top_builddir)/ams/doc/amsxml.5 \
	$(top_builddir)/ams/doc/ams.3

amsclean-local:
	rm -f $(top_builddir)/ams/doc/*.1
	rm -f $(top_builddir)/ams/doc/*.3
	rm -f $(top_builddir)/ams/doc/*.5

$(top_builddir)/ams/doc/%.1: ams/doc/pod1/%.pod
	$(amsPODM1) $< $@

$(top_builddir)/ams/doc/%.3: ams/doc/pod3/%.pod
	$(amsPODM3) $< $@

$(top_builddir)/ams/doc/%.5: ams/doc/pod5/%.pod
	$(amsPODM5) $< $@

# -- Libraries --- #

libams_la_SOURCES = \
	ams/library/libams.c \
	ams/library/amscommon.c \
	ams/library/loadmib.c \
	ams/library/crypt.c \
	ams/library/dgrts.c \
	ams/library/udpts.c \
	ams/library/tcpts.c
libams_la_LDFLAGS = -static
libams_la_CFLAGS = $(amscflags) $(AM_CFLAGS)
libams_la_LIBADD = $(EXPAT_LIBS) libdgr.la libici.la

# --- Utility Programs --- #

amsshell_SOURCES = ams/test/amsshell.c
amsshell_LDADD = libams.la libdgr.la libici.la -lm $(LIBOBJS)
amsshell_CFLAGS = $(amscflags) $(AM_CFLAGS)

amslog_SOURCES = ams/test/amslog.c
amslog_LDADD = libams.la libdgr.la libici.la -lm $(LIBOBJS)
amslog_CFLAGS = $(amscflags) $(AM_CFLAGS)

amslogprt_SOURCES = ams/test/amslogprt.c
amslogprt_LDADD = libams.la libdgr.la libici.la $(LIBOBJS)
amslogprt_CFLAGS = $(amscflags) $(AM_CFLAGS)

# --- Test Executables --- #

amshello_SOURCES = ams/test/amshello.c
amshello_LDADD = libams.la libdgr.la libici.la -lm $(LIBOBJS)
amshello_CFLAGS = $(amscflags) $(AM_CFLAGS)

amsbenchs_SOURCES = ams/test/amsbenchs.c
amsbenchs_LDADD = libams.la libdgr.la libici.la -lm $(LIBOBJS)
amsbenchs_CFLAGS = $(amscflags) $(AM_CFLAGS)

amsbenchr_SOURCES = ams/test/amsbenchr.c
amsbenchr_LDADD = libams.la libdgr.la libici.la -lm $(LIBOBJS)
amsbenchr_CFLAGS = $(amscflags) $(AM_CFLAGS)

amsstop_SOURCES =ams/utils/amsstop.c
amsstop_LDADD = libams.la libdgr.la libici.la -lm $(LIBOBJS)
amsstop_CFLAGS = $(amscflags) $(AM_CFLAGS)

# --- Daemon Executables --- #

amsd_SOURCES = \
	ams/library/amsd.c \
	ams/library/libams.c \
	ams/library/amscommon.c \
	ams/library/crypt.c \
	ams/library/dgrts.c \
	ams/library/udpts.c \
	ams/library/tcpts.c
amsd_CFLAGS = $(amscflags) $(AM_CFLAGS)
amsd_LDADD = libams.la libdgr.la libici.la -lm $(LIBOBJS)

# --- RAMS Executables --- #

ramsgate_SOURCES = \
	ams/rams/librams.c \
	ams/rams/ramscommon.c \
	ams/rams/ramsgate.c
ramsgate_LDADD = libbp.la libams.la libdgr.la libici.la $(LIBOBJS)
ramsgate_CFLAGS = $(amscflags) $(AM_CFLAGS)

endif

##############
#
# CFDP SECTION
#
##############

if !ION_NASA_B

# Define pod2man stuff
cfdpPODM1 = pod2man -s 1 -c "CFDP executables"
cfdpPODM3 = pod2man -s 3 -c "CFDP library functions"
cfdpPODM5 = pod2man -s 5 -c "CFDP configuration files"

cfdpcflags = -I$(srcdir)/cfdp/library -I$(srcdir)/cfdp/include

cfdpbin = \
	cfdpadmin \
	cfdpclock \
	bputa \
	cfdptest \
	bpcp \
	bpcpd

cfdplib = \
	libcfdp.la

cfdpinclude = \
	cfdp/include/cfdp.h \
	cfdp/include/cfdpops.h

cfdpnoinst = \
	cfdp/library/cfdpP.h \
	cfdp/utils/bpcp.h

cfdpextra = \
	cfdp/doc/pod3/cfdp.pod \
	cfdp/doc/pod5/cfdprc.pod \
	cfdp/doc/pod1/cfdptest.pod \
	cfdp/doc/pod1/cfdpclock.pod \
	cfdp/doc/pod1/bputa.pod \
	cfdp/doc/pod1/cfdpadmin.pod \
	cfdp/doc/pod1/bpcp.pod \
	cfdp/doc/pod1/bpcpd.pod \
	cfdp/README.txt

cfdpmans = \
	$(top_builddir)/cfdp/doc/cfdp.3 \
	$(top_builddir)/cfdp/doc/cfdprc.5 \
	$(top_builddir)/cfdp/doc/cfdptest.1 \
	$(top_builddir)/cfdp/doc/cfdpclock.1 \
	$(top_builddir)/cfdp/doc/bputa.1 \
	$(top_builddir)/cfdp/doc/cfdpadmin.1 \
	$(top_builddir)/cfdp/doc/bpcp.1 \
	$(top_builddir)/cfdp/doc/bpcpd.1

cfdpclean-local:
	rm -f $(top_builddir)/cfdp/doc/*.1
	rm -f $(top_builddir)/cfdp/doc/*.3
	rm -f $(top_builddir)/cfdp/doc/*.5

$(top_builddir)/cfdp/doc/%.1: cfdp/doc/pod1/%.pod
	$(cfdpPODM1) $< $@

$(top_builddir)/cfdp/doc/%.3: cfdp/doc/pod3/%.pod
	$(cfdpPODM3) $< $@

$(top_builddir)/cfdp/doc/%.5: cfdp/doc/pod5/%.pod
	$(cfdpPODM5) $< $@

# --- Libraries --- #

libcfdp_la_SOURCES = \
	cfdp/library/libcfdp.c \
	cfdp/library/libcfdpops.c \
	cfdp/library/libcfdpP.c
libcfdp_la_CFLAGS = $(cfdpcflags) $(AM_CFLAGS)
libcfdp_la_LIBADD = libici.la -lm

# --- Utility Programs --- #

cfdpadmin_SOURCES = cfdp/utils/cfdpadmin.c
cfdpadmin_LDADD = libcfdp.la libici.la -lm libbp.la $(LIBOBJS)
cfdpadmin_CFLAGS = $(cfdpcflags) $(AM_CFLAGS)

bpcp_SOURCES = cfdp/utils/bpcp.c
bpcp_LDADD = libcfdp.la libici.la -lm $(LIBOBJS) $(PTHREAD_LIBS)
bpcp_CFLAGS = $(cfdpcflags) $(AM_CFLAGS)

bpcpd_SOURCES = cfdp/utils/bpcpd.c
bpcpd_LDADD = libcfdp.la libici.la -lm $(LIBOBJS)
bpcpd_CFLAGS = $(cfdpcflags) $(AM_CFLAGS)

# --- BP Programs --- #

bputa_SOURCES = cfdp/bp/bputa.c
bputa_LDADD = libcfdp.la libici.la libbp.la $(LIBOBJS) $(PTHREAD_LIBS)
bputa_CFLAGS = $(cfdpcflags) $(AM_CFLAGS)

# --- Test Programs --- #

cfdptest_SOURCES = cfdp/test/cfdptest.c
cfdptest_LDADD = libcfdp.la libici.la -lm libbp.la $(LIBOBJS) $(PTHREAD_LIBS)
cfdptest_CFLAGS = $(cfdpcflags) $(AM_CFLAGS)

# --- Daemon Executables --- #

cfdpclock_SOURCES = cfdp/daemon/cfdpclock.c
cfdpclock_LDADD = libcfdp.la libici.la libbp.la $(LIBOBJS)
cfdpclock_CFLAGS = $(cfdpcflags) $(AM_CFLAGS)

endif


##############
#
# BSS SECTION
#
##############

# Define pod2man stuff
bssPODM1 = pod2man -s 1 -c "BSS executables"
bssPODM3 = pod2man -s 3 -c "BSS library functions"

bsscflags = -I$(srcdir)/bss/library -I$(srcdir)/bss/include

bssbin = \
	bssrecv \
	bssStreamingApp \
	bsscounter \
	bssdriver

bsslib = \
	libbss.la

bssinclude = \
	bss/include/bss.h

bssnoinst = \
	bss/library/bssP.h

bssextra = \
	bss/doc/pod1/bssrecv.pod \
	bss/doc/pod1/bssStreamingApp.pod \
	bss/doc/pod3/bss.pod \
	bss/README.txt

bssmans = \
	$(top_builddir)/bss/doc/bssrecv.1 \
	$(top_builddir)/bss/doc/bssStreamingApp.1 \
	$(top_builddir)/bss/doc/bss.3

bssclean-local:
	rm -f $(top_builddir)/bss/doc/*.1
	rm -f $(top_builddir)/bss/doc/*.3

$(top_builddir)/bss/doc/%.1: bss/doc/pod1/%.pod
	$(bssPODM1) $< $@

$(top_builddir)/bss/doc/%.3: bss/doc/pod3/%.pod
	$(bssPODM3) $< $@

# --- Libraries --- #

libbss_la_SOURCES = \
	bss/library/libbss.c \
	bss/library/libbssP.c
libbss_la_CFLAGS = $(bsscflags) $(AM_CFLAGS)
libbss_la_LIBADD = libici.la libbp.la

# --- Test Programs --- #

bssrecv_SOURCES = bss/test/bssrecv.c
bssrecv_LDADD = libbss.la libici.la libbp.la $(LIBOBJS)
bssrecv_CFLAGS = $(bsscflags) $(AM_CFLAGS)

bssStreamingApp_SOURCES = bss/test/bssStreamingApp.c
bssStreamingApp_LDADD = libbp.la libbss.la libici.la $(LIBOBJS)
bssStreamingApp_CFLAGS = $(bsscflags) $(AM_CFLAGS)

bsscounter_SOURCES = bss/test/bsscounter.c
bsscounter_LDADD = libbss.la libici.la libbp.la $(LIBOBJS)
bsscounter_CFLAGS = $(bsscflags) $(AM_CFLAGS)

bssdriver_SOURCES = bss/test/bssdriver.c
bssdriver_LDADD = libbss.la libici.la libbp.la $(LIBOBJS)
bssdriver_CFLAGS = $(bsscflags) $(AM_CFLAGS)


##############
#
# BSSP SECTION
#
##############

# Define pod2man stuff
bsspPODM1 = pod2man -s 1 -c "BSSP executables"
bsspPODM3 = pod2man -s 3 -c "BSSP library functions"
bsspPODM5 = pod2man -s 5 -c "BSSP configuration files"

bsspcflags = -I$(srcdir)/bssp/library -I$(srcdir)/bssp/include

bsspbin = \
	bsspclock \
	bsspadmin \
	udpbsi \
	udpbso \
	tcpbsi \
	tcpbso

bssplib = \
	libbssp.la \
	libbsspP.la \
	libtcpbsa.la

bsspinclude = \
	bssp/include/bssp.h

bsspnoinst = \
	bssp/library/bsspP.h

bsspextra = \
	bssp/doc/pod1/bsspadmin.pod \
	bssp/doc/pod1/udpbso.pod \
	bssp/doc/pod3/bssp.pod \
	bssp/doc/pod5/bssprc.pod

bsspmans = \
	$(top_builddir)/bssp/doc/bsspadmin.1 \
	$(top_builddir)/bssp/doc/udpbso.1 \
	$(top_builddir)/bssp/doc/bssp.3 \
	$(top_builddir)/bssp/doc/bssprc.5

bsspclean-local:
	rm -f $(top_builddir)/bssp/doc/*.1
	rm -f $(top_builddir)/bssp/doc/*.3
	rm -f $(top_builddir)/bssp/doc/*.5

$(top_builddir)/bssp/doc/%.1: bssp/doc/pod1/%.pod
	$(bsspPODM1) $< $@

$(top_builddir)/bssp/doc/%.3: bssp/doc/pod3/%.pod
	$(bsspPODM3) $< $@

$(top_builddir)/bssp/doc/%.5: bssp/doc/pod5/%.pod
	$(bsspPODM5) $< $@

# --- Libraries --- #

libbsspP_la_SOURCES = \
	bssp/library/libbssp.c \
	bssp/library/libbsspP.c
libbsspP_la_CFLAGS = $(bsspcflags) $(AM_CFLAGS)
libbsspP_la_LDFLAGS = $(ION_LINK_FLAGS)
libbsspP_la_LIBADD = libici.la -lm $(PTHREAD_LIBS)

libbssp_la_SOURCES = \
	bssp/library/libbssp.c \
	bssp/library/libbsspP.c
libbssp_la_CFLAGS = $(bsspcflags) $(AM_CFLAGS)
libbssp_la_LDFLAGS = $(ION_LINK_FLAGS)
libbssp_la_LIBADD = libici.la -lm $(PTHREAD_LIBS)

libtcpbsa_la_SOURCES = \
	bssp/tcp/libtcpbsa.c
libtcpbsa_la_CFLAGS = $(bsspcflags) $(AM_CFLAGS)
libtcpbsa_la_LDFLAGS = $(ION_LINK_FLAGS)
libtcpbsa_la_LIBADD = libici.la -lm $(PTHREAD_LIBS)

# --- Daemon Executables --- #

bsspclock_SOURCES = bssp/daemon/bsspclock.c
bsspclock_LDADD = libbsspP.la libici.la -lm libbp.la $(LIBOBJS)
bsspclock_CFLAGS = $(bsspcflags) $(AM_CFLAGS)

# --- Utility Programs --- #

bsspadmin_SOURCES = bssp/utils/bsspadmin.c
bsspadmin_LDADD = libbsspP.la libici.la -lm libbp.la $(LIBOBJS)
bsspadmin_CFLAGS = $(bsspcflags) $(AM_CFLAGS)

# --- UDP Executables --- #

udpbsi_SOURCES = bssp/udp/udpbsi.c
udpbsi_LDADD = libbsspP.la libici.la -lm libbp.la $(LIBOBJS) $(PTHREAD_LIBS)
udpbsi_CFLAGS = $(bsspcflags) $(AM_CFLAGS)

udpbso_SOURCES = bssp/udp/udpbso.c
udpbso_LDADD = libbsspP.la libici.la -lm libbp.la $(LIBOBJS) $(PTHREAD_LIBS)
udpbso_CFLAGS = $(bsspcflags) $(AM_CFLAGS)

# --- TCP Executables --- #

tcpbsi_SOURCES = bssp/tcp/tcpbsi.c
tcpbsi_LDADD = libbsspP.la libici.la -lm libbp.la $(LIBOBJS) $(PTHREAD_LIBS)
tcpbsi_CFLAGS = $(bsspcflags) $(AM_CFLAGS)

tcpbso_SOURCES = bssp/tcp/tcpbso.c
tcpbso_LDADD = libbsspP.la libtcpbsa.la libici.la -lm libbp.la $(LIBOBJS) $(PTHREAD_LIBS)
tcpbso_CFLAGS = $(bsspcflags) $(AM_CFLAGS)

##############
#
# DTPC SECTION
#
##############

# Define pod2man stuff
dtpcPODM1 = pod2man -s 1 -c "DTPC executables"
dtpcPODM3 = pod2man -s 3 -c "DTPC library functions"
dtpcPODM5 = pod2man -s 5 -c "DTPC configuration files"

dtpccflags = -I$(srcdir)/dtpc/library -I$(srcdir)/dtpc/include


##########################
#
# NETWORK MANAGEMENT SECTION
#
##########################

nmbin= nm_agent nm_mgr

nm_agent_SOURCES =  nm/agent/ingest.c            nm/agent/lcc.c                 nm/agent/ldc.c \
                    nm/agent/nmagent.c           nm/agent/rda.c                 nm/shared/primitives/expr.c \
                    nm/agent/agent_db.c          nm/agent/adm_ion_priv.c        nm/agent/adm_ltp_priv.c \ 
                    nm/shared/adm/adm.c          nm/shared/adm/adm_bp.c         nm/shared/adm/adm_ion.c \
					nm/shared/adm/adm_ltp.c 	 nm/agent/adm_agent_impl.c nm/agent/adm_bp_impl.c \
					nm/shared/adm/adm_agent.c nm/shared/adm/adm_bpsec.c	nm/agent/adm_bpsec_impl.c \
					nm/shared/msg/msg_admin.c    nm/shared/msg/msg_ctrl.c       nm/shared/msg/pdu.c \
					nm/shared/primitives/var.c 	 nm/shared/primitives/blob.c \
					nm/shared/primitives/admin.c nm/shared/primitives/def.c     nm/shared/primitives/mid.c \
					nm/shared/primitives/oid.c   nm/shared/primitives/report.c  nm/shared/primitives/rules.c \
					nm/shared/primitives/dc.c  	 nm/shared/primitives/value.c   nm/shared/primitives/lit.c \
					nm/shared/primitives/nn.c 	 nm/agent/instr.c   nm/shared/primitives/tdc.c\ 
					nm/shared/primitives/ctrl.c 	nm/shared/primitives/table.c \
					nm/shared/utils/ion_if.c     nm/shared/utils/utils.c        nm/shared/utils/db.c \
					nm/shared/utils/nm_types.c
				
					
nm_agent_LDADD = libici.la libbp.la libltp.la $(LIBOBJS) $(PTHREAD_LIBS)
nm_agent_LDFLAGS = -lm -lpthread
nm_agent_CFLAGS = -I/usr/include -I$(srcdir)/ici/library -I$(srcdir)/bp/library -I$(srcdir)/ltp/library -I$(srcdir)/nm  \
                  -I$(srcdir)/nm/agent -I$(srcdir)/bp/library/ext/bpsec $(AM_CFLAGS) -Wno-unused-variable \
                  -DAGENT_ROLE
					
					
nm_mgr_SOURCES =    nm/mgr/nm_mgr.c              nm/mgr/nm_mgr_rx.c             nm/mgr/nm_mgr_ui.c \
					nm/mgr/nm_mgr_names.c        nm/mgr/nm_mgr_sql.c             nm/mgr/ui_input.c \ 
					nm/mgr/nm_mgr_print.c 		 nm/mgr/mgr_db.c \
					nm/shared/adm/adm.c          nm/shared/adm/adm_bp.c         \
					nm/shared/adm/adm_ltp.c  	 nm/shared/adm/adm_ion.c        nm/shared/adm/adm_agent.c \
					nm/shared/adm/adm_bpsec.c	\
					nm/shared/msg/msg_admin.c    nm/shared/msg/msg_ctrl.c       nm/shared/msg/pdu.c \
					nm/shared/primitives/var.c    nm/shared/primitives/expr.c \
					nm/shared/primitives/admin.c nm/shared/primitives/def.c     nm/shared/primitives/mid.c \
					nm/shared/primitives/oid.c   nm/shared/primitives/report.c  nm/shared/primitives/rules.c \
					nm/shared/primitives/dc.c  	 nm/shared/primitives/value.c   nm/shared/primitives/lit.c \
					nm/shared/primitives/nn.c 	  nm/shared/primitives/tdc.c\ 
					nm/shared/primitives/ctrl.c  nm/shared/primitives/blob.c nm/shared/primitives/table.c \
					nm/shared/utils/ion_if.c     nm/shared/utils/utils.c        nm/shared/utils/db.c \
					nm/shared/utils/nm_types.c
					
nm_mgr_LDADD = libici.la libbp.la libltp.la $(LIBOBJS) $(PTHREAD_LIBS)

# Use this to build the DTNMP Manager WITHOUT DB Support. 
nm_mgr_CFLAGS = -I/usr/include -I$(srcdir)/ici/library -I$(srcdir)/bp/library -I$(srcdir)/ltp/library -I$(srcdir)/nm  -I$(srcdir)/nm/mgr -I$(srcdir)/bp/library/ext/bpsec $(AM_CFLAGS) -Wno-unused-variable 

# Use this to build the DTNMP Manager WITH DB Support.
#nm_mgr_LDFLAGS = -L/usr/local/mysql/lib -lmysqlclient -lpthread
#nm_mgr_CFLAGS = -I/usr/local/mysql/include -I$(srcdir)/ici/library -I$(srcdir)/bp/library -I$(srcdir)/ltp/library -I$(srcdir)/nm  -I$(srcdir)/nm/mgr -I/usr/include/mysql $(AM_CFLAGS) -Wno-unused-variable -DHAVE_MYSQL



dtpcbin = \
	dtpcreceive \
	dtpcsend \
	dtpcadmin \
	dtpcclock \
	dtpcd

dtpclib = \
	libdtpc.la

dtpcinclude = \
	dtpc/include/dtpc.h

dtpcnoinst = \
	dtpc/library/dtpcP.h

dtpcextra = \
	dtpc/doc/pod1/dtpcclock.pod \
	dtpc/doc/pod1/dtpcadmin.pod \
	dtpc/doc/pod1/dtpcsend.pod \
	dtpc/doc/pod1/dtpcreceive.pod \
	dtpc/doc/pod1/dtpcd.pod \
	dtpc/doc/pod3/dtpc.pod \
	dtpc/doc/pod5/dtpcrc.pod

dtpcmans = \
	$(top_builddir)/dtpc/doc/dtpcclock.1 \
	$(top_builddir)/dtpc/doc/dtpcadmin.1 \
	$(top_builddir)/dtpc/doc/dtpcsend.1 \
	$(top_builddir)/dtpc/doc/dtpcreceive.1 \
	$(top_builddir)/dtpc/doc/dtpcd.1 \
	$(top_builddir)/dtpc/doc/dtpc.3 \
	$(top_builddir)/dtpc/doc/dtpcrc.5

dtpcclean-local:
	rm -f $(top_builddir)/dtpc/doc/*.1
	rm -f $(top_builddir)/dtpc/doc/*.3
	rm -f $(top_builddir)/dtpc/doc/*.5

$(top_builddir)/dtpc/doc/%.1: dtpc/doc/pod1/%.pod
	$(dtpcPODM1) $< $@

$(top_builddir)/dtpc/doc/%.3: dtpc/doc/pod3/%.pod
	$(dtpcPODM3) $< $@

$(top_builddir)/dtpc/doc/%.5: dtpc/doc/pod5/%.pod
	$(dtpcPODM5) $< $@

# --- Libraries --- #

libdtpc_la_SOURCES = \
	dtpc/library/libdtpc.c \
	dtpc/library/libdtpcP.c
libdtpc_la_CFLAGS = $(dtpccflags) $(AM_CFLAGS)
libdtpc_la_LDFLAGS = $(ION_LINK_FLAGS)
libdtpc_la_LIBADD = libici.la libbp.la $(PTHREAD_LIBS)

# --- Utility Programs --- #

dtpcadmin_SOURCES = dtpc/utils/dtpcadmin.c
dtpcadmin_LDADD = libdtpc.la libici.la libbp.la $(LIBOBJS)
dtpcadmin_CFLAGS = $(dtpccflags) $(AM_CFLAGS)

# --- Test Programs --- #

dtpcreceive_SOURCES = dtpc/test/dtpcreceive.c
dtpcreceive_LDADD = libdtpc.la libici.la libbp.la  $(LIBOBJS)
dtpcreceive_CFLAGS = $(dtpccflags) $(AM_CFLAGS)

dtpcsend_SOURCES = dtpc/test/dtpcsend.c
dtpcsend_LDADD = libdtpc.la libici.la libbp.la  $(LIBOBJS)
dtpcsend_CFLAGS = $(dtpccflags) $(AM_CFLAGS)

# --- Daemon Executables --- #

dtpcclock_SOURCES = dtpc/daemon/dtpcclock.c
dtpcclock_LDADD = libdtpc.la libici.la libbp.la $(LIBOBJS)
dtpcclock_CFLAGS = $(dtpccflags) $(AM_CFLAGS)

dtpcd_SOURCES = dtpc/daemon/dtpcd.c
dtpcd_LDADD = libdtpc.la libici.la libbp.la $(LIBOBJS)
dtpcd_CFLAGS = $(dtpccflags) $(AM_CFLAGS)

##########################
#
# RESTART SECTION
#
##########################

restartbin = ionrestart

# --- Utility Programs --- #
if ION_NASA_B
ionrestart_SOURCES = restart/utils/ionrestart.c
ionrestart_LDADD = libici.la libcgr.la libbp.la libltp.la $(LIBOBJS)
ionrestart_CFLAGS = -I$(srcdir)/ici/sdr  -I$(srcdir)/ici/library -I$(srcdir)/bp/library -I$(srcdir)/ltp/library -I$(srcdir)/ici/library $(AM_CFLAGS)
else
ionrestart_SOURCES = restart/utils/ionrestart.c
ionrestart_LDADD = libici.la libcgr.la libbp.la libltp.la libcfdp.la $(LIBOBJS)
ionrestart_CFLAGS = -I$(srcdir)/ici/sdr  -I$(srcdir)/ici/library -I$(srcdir)/bp/library -I$(srcdir)/ltp/library -I$(srcdir)/ici/library -I$(srcdir)/cfdp/library $(AM_CFLAGS)
endif


##########################
#
# TESTS SECTION
#
##########################
.PHONY:	test check buildcheck test-all test-branch test-% retest

# "make buildcheck" will compile any of the compiled tests, but won't run
# any of them.
buildcheck: $(check_PROGRAMS)

# The runtests test runner script runs anything named "dotest" that is
# executable inside the tests/ directory.  This allows ION to have both
# compiled tests and tests written in shell script.
#test: buildcheck
#	cd tests && ./runtests

# Runs the tests listed in "normaltests"
test: buildcheck
	cd tests && ./runtestset normaltests

# Uses the runtest script that simply runs "dotest" including compiled tests.
# Could also use ./runtestset alltests, they should be equivalent.
test-all: buildcheck
	cd tests && ./runtests

# Builds a testset file with the same name as a mercurial branch.
# This is intended as a per-branch unit test.
test-branch: buildcheck
	@echo
	@echo "You need mercurial (hg) installed for this."
	@echo
	cd tests && hg branch | xargs -L1 ./runtestset

# Catch-all for tests, basically any string after "make test-" will run a
# testset file with the same name as that string.
test-%: buildcheck
	cd tests && ./runtestset $*

# Runtest will make a file called "retest" containing all "failed" tests.
# The file will not exist at first (resulting in an error).
# When there are no failed tests, retest will exist (no error), but contains
# an empty test list (resulting in a test passage).
# This ensures that "make restest" will only pass when some testing has
# already been performed with total success.
retest: buildcheck
	cd tests && ./runtestset retest

# "make check" is the same as "make test"
check: test

# This library contains utility code that's helpful for tests in C.
TESTUTILOBJS = tests/library/libtestutil.la
TESTUTILCFLAGS = -Itests/library
check_LTLIBRARIES = $(TESTUTILOBJS)
tests_library_libtestutil_la_SOURCES = \
	tests/library/check.c \
	tests/library/paths.c \
	tests/library/ionstart.c \
	tests/library/ionstop.c
tests_library_libtestutil_la_CFLAGS = $(bpcflags) $(AM_CFLAGS)

# TEST AUTHORS: README
# If a test does not require compilation, it should not be listed here.  Just make a
#    script named "dotest" that is executable.
# If a test does require compilation, it should be listed here and have
#    the required _SOURCES, _LDADD, and _CFLAGS entries.
# If a test uses the testutil helpers, include $(TESTUTILOBJS) in _LDADD,
#    and $(TESTUTILCFLAGS) in _CFLAGS.
check_PROGRAMS = tests/1000.loopback/dotest \
	tests/1500.loopback-brs/dotest \
	tests/issue-188-common-cos-syntax/dotest \
	tests/issue-260-teach-valgrind-mtake/domtake \
	tests/issue-279-bpMemo-timeline/driver \
	tests/bug-0015-tcpclo-bpcp-sig-handling/test

if !ION_NASA_B
check_PROGRAMS+= \
	tests/issue-330-cfdpclock-FDU-removal/cfdplisten \
	tests/issue-334-cfdp-transaction-id/dotest
endif

tests_1000_loopback_dotest_SOURCES = tests/1000.loopback/dotest.c
tests_1000_loopback_dotest_LDADD = libici.la -lm libbp.la $(ltplib) $(LIBOBJS) $(TESTUTILOBJS)
tests_1000_loopback_dotest_CFLAGS = $(bpcflags) $(AM_CFLAGS) $(TESTUTILCFLAGS)

#Disabled this test in ION 3.6.0 because tcpcl does not support loopback
#tests_1300_loopback_tcp_dotest_SOURCES = tests/1300.loopback-tcp/dotest.c
#tests_1300_loopback_tcp_dotest_LDADD = libici.la -lm libbp.la $(ltplib) $(LIBOBJS) $(TESTUTILOBJS)
#tests_1300_loopback_tcp_dotest_CFLAGS = $(bpcflags) $(AM_CFLAGS) $(TESTUTILCFLAGS)

# Disabled this test in ION 3.2.2 due to crashes.  Restore after fixing crashes.
# tests_1400_loopback_stcp_dotest_SOURCES = tests/1400.loopback-stcp/dotest.c
# tests_1400_loopback_stcp_dotest_LDADD = libbp.la libici.la $(ltplib) $(LIBOBJS) $(TESTUTILOBJS)
# tests_1400_loopback_stcp_dotest_CFLAGS = $(bpcflags) $(AM_CFLAGS) $(TESTUTILCFLAGS)

tests_1500_loopback_brs_dotest_SOURCES = tests/1500.loopback-brs/dotest.c
tests_1500_loopback_brs_dotest_LDADD =  libici.la -lm libbp.la $(ltplib) $(LIBOBJS) $(TESTUTILOBJS)
tests_1500_loopback_brs_dotest_CFLAGS = $(bpcflags) $(AM_CFLAGS) $(TESTUTILCFLAGS)

tests_issue_188_common_cos_syntax_dotest_SOURCES = tests/issue-188-common-cos-syntax/dotest.c
tests_issue_188_common_cos_syntax_dotest_LDADD = libici.la -lm libbp.la $(ltplib) $(LIBOBJS) $(TESTUTILOBJS)
tests_issue_188_common_cos_syntax_dotest_CFLAGS = $(bpcflags) $(AM_CFLAGS) $(TESTUTILCFLAGS)

tests_issue_260_teach_valgrind_mtake_domtake_SOURCES = tests/issue-260-teach-valgrind-mtake/domtake.c
tests_issue_260_teach_valgrind_mtake_domtake_LDADD = libici.la -lm $(LIBOBJS) $(TESTUTILOBJS)
tests_issue_260_teach_valgrind_mtake_domtake_CFLAGS = $(icicflags) $(AM_CFLAGS) $(TESTUTILCFLAGS)

tests_issue_279_bpMemo_timeline_driver_SOURCES = tests/issue-279-bpMemo-timeline/driver.c
tests_issue_279_bpMemo_timeline_driver_LDADD =  libici.la -lm libbp.la $(LIBOBJS) $(TESTUTILOBJS)
tests_issue_279_bpMemo_timeline_driver_CFLAGS = $(bpcflags) $(icicflags) $(AM_CFLAGS) $(TESTUTILCFLAGS)

tests_bug_0015_tcpclo_bpcp_sig_handling_test_SOURCES=tests/bug-0015-tcpclo-bpcp-sig-handling/test.c
tests_bug_0015_tcpclo_bpcp_sig_handling_test_LDADD= libici.la -lm libbp.la $(LIBOBJS)
tests_bug_0015_tcpclo_bpcp_sig_handling_test_CFLAGS=$(bpcflags) $(icicflags) $(AM_CFLAGS)

if !ION_NASA_B
tests_issue_330_cfdpclock_FDU_removal_cfdplisten_SOURCES = tests/issue-330-cfdpclock-FDU-removal/cfdplisten.c
tests_issue_330_cfdpclock_FDU_removal_cfdplisten_LDADD = libcfdp.la libici.la -lm libbp.la $(LIBOBJS)
tests_issue_330_cfdpclock_FDU_removal_cfdplisten_CFLAGS = $(cfdpcflags) $(AM_CFLAGS)
endif

if !ION_NASA_B
tests_issue_334_cfdp_transaction_id_dotest_SOURCES = tests/issue-334-cfdp-transaction-id/dotest.c
tests_issue_334_cfdp_transaction_id_dotest_LDADD = libcfdp.la libici.la -lm $(LIBOBJS) $(TESTUTILOBJS)
tests_issue_334_cfdp_transaction_id_dotest_CFLAGS = $(icicflags) $(AM_CFLAGS) $(TESTUTILCFLAGS)
endif


##########################
#
# AUTODOC SECTION
#
##########################

if ENABLE_AUTODOC

ION.pdf: doc/ION-beginning.pdf doc/ION-manpages.ps
	$(PS2PDF) doc/ION-manpages.ps
	$(PDFJOIN) doc/ION-beginning.pdf ION-manpages.pdf --outfile ION.pdf

# The targets in man_MANS that are actually manpages (and not phony targets
# for making directories, etc.)
justmans := $(filter %.1 %.3 %.5,$(man_MANS))

mans_as_ps := $(patsubst %,%.ps,$(justmans))
mans_1_as_ps := $(sort $(filter %.1.ps,$(mans_as_ps)))
mans_3_as_ps := $(sort $(filter %.3.ps,$(mans_as_ps)))
mans_5_as_ps := $(sort $(filter %.5.ps,$(mans_as_ps)))
mans_as_ps_sorted := $(mans_1_as_ps) $(mans_3_as_ps) $(mans_5_as_ps)


%.1.ps: %.1
	man -l -Tps $^ > $@

%.3.ps: %.3
	man -l -Tps $^ > $@

%.5.ps: %.5
	man -l -Tps $^ > $@

doc/appendix-a-contents.txt: $(mans_as_ps)
	@rm -f $@
	@echo ".tl 'Appendix A: ION Manpages''$$(hg id)" >> $@
	@echo "" >> $@
	@echo ".2C" >> $@
	@echo ".ps 10" >> $@
	@echo ".nf" >> $@
	@echo ".ps 14" >> $@
	@echo ".pvs +3p" >> $@
	@echo "\fBExecutables (man section 1)\fP" >> $@
	@echo ".pvs" >> $@
	@echo ".ps 10" >> $@
	@echo $(strip $(mans_1_as_ps)) | sed -e 's/ /\n/g' | sed -e 's/.*\///' -e 's/\.[0-9]\.ps//' >> $@
	@echo ".ps 14" >> $@
	@echo ".vs +10p" >> $@
	@echo ".sp 0.1i" >> $@
	@echo "\fBLibraries (man section 3)\fP" >> $@
	@echo ".vs" >> $@
	@echo ".ps 10" >> $@
	@echo $(strip $(mans_3_as_ps)) | sed -e 's/ /\n/g' | sed -e 's/.*\///' -e 's/\.[0-9]\.ps//' >> $@
	@echo ".ps 14" >> $@
	@echo ".vs +10p" >> $@
	@echo ".sp 0.1i" >> $@
	@echo "\fBConfiguration files (man section 5)\fP" >> $@
	@echo ".vs" >> $@
	@echo ".ps 10" >> $@
	@echo $(strip $(mans_5_as_ps)) | sed -e 's/ /\n/g' | sed -e 's/.*\///' -e 's/\.[0-9]\.ps//' >> $@
	@echo ".br" >> $@

doc/appendix-a-contents.ps: doc/appendix-a-contents.txt
	$(GROFFMS) -ms $^ > $@

doc/ION-manpages.ps: doc/appendix-a-contents.ps $(mans_as_ps_sorted)
	$(PSJOIN) $^ > $@

doc/ION-beginning.ps: doc/ION-beginning.pdf
	$(PDF2PS) $^ $@

else

ION.pdf: doc/ION-beginning.pdf
	@echo "ION.pdf will not have appendix."
	cp $^ $@

endif #ENABLE_AUTODOC
