# Makefile.am
# Samuel Jero
# June 14, 2013
# Adapted from work by David Young

#Control building of Contrib as determined by ./configure flags
if BUILD_CONTRIB
SUBDIRS= . contrib
else
SUBDIRS= .
endif # end BUILD_CONTRIB

DIST_SUBDIRS= . contrib

AM_CFLAGS += \
	-g \
	-include config.h \
	-I$(srcdir)/ici/include \
	-I$(srcdir)/ltp/include \
	-I$(srcdir)/dgr/include \
	-I$(srcdir)/$(BP_DIR)/include \
	-I$(srcdir)/tc/include \
	-I$(srcdir)/bss/include \
	-I$(srcdir)/dtpc/include \
	-I$(srcdir)/bssp/include \
	-DSBSP \
	-I$(srcdir)/ams/include \
	-I$(srcdir)/cfdp/include

if SOLARIS_STUDIO_CC
AM_CFLAGS += \
	-errtags \
	-Kpic
NO_UNUSED_VARIABLE = 
else
AM_CFLAGS += \
	-Wall \
	-fPIC -Werror

NO_UNUSED_VARIABLE = -Wno-unused-variable
endif # end SOLARIS_STUDIO_CC

if WINDOWS
LIBOBJS += -lws2_32 -lwsock32
endif

# This will add CFLAGS to make the valgrind macros compile without
# warnings/errors on some systems with specific versions of valgrind
# and gcc (see configure.ac and issue #298 for details). If valgrind
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
	$(node5_DATA) \
	$(node7_DATA) \
	$(node10_DATA) \
	$(iciextra) \
	$(ltpextra) \
	$(dgrextra) \
	$(bpextra) \
	$(tcextra) \
	$(bssextra) \
	$(dtpcextra) \
	$(bsspextra) \
	$(amsextra) \
	$(cfdpextra) \
	$(nmextra)
#	$(twonodestcp_DATA) \
#	$(threenodestcpltp_DATA)

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
#  @make cov-reset

#check:
#  @make cov

else
cov-clean:
	@echo "You must use \"configure --with-gcov\" to enable coverage testing features"

cov-report:
	@echo "You must use \"configure --with-gcov\" to enable coverage testing features"

cov:
	@echo "You must use \"configure --with-gcov\" to enable coverage testing features"
endif # end ENABLE_GCOV


# clean-local refers to existing documents.
clean-local: iciclean-local ltpclean-local dgrclean-local bpclean-local amsclean-local cfdpclean-local

build-man-directories:
	mkdir -p $(top_builddir)/ici/doc
	mkdir -p $(top_builddir)/ltp/doc
	mkdir -p $(top_builddir)/dgr/doc
	mkdir -p $(top_builddir)/$(BP_DIR)/doc
	mkdir -p $(top_builddir)/tc/doc
	mkdir -p $(top_builddir)/bss/doc
	mkdir -p $(top_builddir)/dtpc/doc
	mkdir -p $(top_builddir)/bssp/doc
	mkdir -p $(top_builddir)/ams/doc
	mkdir -p $(top_builddir)/cfdp/doc
	mkdir -p $(top_builddir)/nm/doc


bin_PROGRAMS = \
	$(icibin) \
	$(ltpbin) \
	$(dgrbin) \
	$(bpbin) \
	$(tcbin) \
	$(bssbin) \
	$(dtpcbin) \
	$(bsspbin) \
	$(restartbin) \
	$(nmbin) \
	$(amsbin) \
	$(cfdpbin)


lib_LTLIBRARIES = \
	$(icilib) \
	$(ltplib) \
	$(dgrlib) \
	$(bplib) \
	$(tclib) \
	$(bsslib) \
	$(dtpclib) \
	$(bssplib) \
	$(amslib) \
	$(cfdplib) \
	$(nmlib)

include_HEADERS = \
	$(iciinclude) \
	$(ltpinclude) \
	$(dgrinclude) \
	$(bpinclude) \
	$(tcinclude) \
	$(bssinclude) \
	$(dtpcinclude) \
	$(bsspinclude) \
	$(amsinclude) \
	$(cfdpinclude)

noinst_HEADERS = \
	$(icinoinst) \
	$(ltpnoinst) \
	$(dgrnoinst) \
	$(bpnoinst) \
	$(bssnoinst) \
	$(dtpcnoinst) \
	$(bsspnoinst) \
	$(amsnoinst) \
	$(cfdpnoinst)

if ENABLE_MANPAGES
man_MANS = \
	build-man-directories \
	$(icimans) \
	$(ltpmans) \
	$(dgrmans) \
	$(bpmans) \
	$(tcmans) \
	$(bssmans) \
	$(dtpcmans) \
	$(bsspmans) \
	$(amsmans) \
	$(cfdpmans) \
	$(nmmans)
endif # end ENABLE_MANPAGES

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
	$(srcdir)/ION.pdf \
	$(srcdir)/ION-LTP-configuration.pdf \
	$(srcdir)/ION-LTP-configuration.xls

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

icicflags = -I$(srcdir)/ici/library -I$(srcdir)/ici/crypto -I$(srcdir)/$(BP_DIR)/library -I$(srcdir)/ltp/library -I$(srcdir)/ici/test -I$(srcdir)/ici/sdr -I$(srcdir)/ici/libbloom-master -I$(srcdir)/ici/libbloom-master/murmur2

icibin = \
	file2sdr \
	file2sm \
	ionadmin \
	ionlog \
	ionsecadmin \
	ionunlock \
	ionwarn \
	owltsim \
	psmshell \
	psmwatch \
	rfxclock \
	sdr2file \
	sdrmend \
	sdrwatch \
	sm2file \
	smlistsh \
	smrbtsh

if !WINDOWS
icibin += \
	ionxnowner \
	owlttb
endif

icilib = \
	libici.la

iciinclude = \
	ici/include/crypto.h \
	ici/include/icinm.h \
	ici/include/ion.h \
	ici/include/ionsec.h \
	ici/include/llcv.h \
	ici/include/lyst.h \
	ici/include/memmgr.h \
	ici/include/platform.h \
	ici/include/platform_sm.h \
	ici/include/psm.h \
	ici/include/rfx.h \
	ici/include/sdr.h \
	ici/include/sdrhash.h \
	ici/include/sdrlist.h \
	ici/include/sdrmgt.h \
	ici/include/sdrstring.h \
	ici/include/sdrtable.h \
	ici/include/sdrxn.h \
	ici/include/smlist.h \
	ici/include/smrbt.h \
	ici/include/sptrace.h \
	ici/include/zco.h

icinoinst = \
	ici/library/lystP.h \
	ici/sdr/sdrP.h \
	ici/test/file2sdr.h

iciextra = \
	ici/README.txt \
	ici/doc/pod1/file2sdr.pod \
	ici/doc/pod1/file2sm.pod \
	ici/doc/pod1/ionadmin.pod \
	ici/doc/pod1/ionlog.pod \
	ici/doc/pod1/ionsecadmin.pod \
	ici/doc/pod1/ionunlock.pod \
	ici/doc/pod1/ionxnowner.pod \
	ici/doc/pod1/owltsim.pod \
	ici/doc/pod1/owlttb.pod \
	ici/doc/pod1/psmshell.pod \
	ici/doc/pod1/psmwatch.pod \
	ici/doc/pod1/rfxclock.pod \
	ici/doc/pod1/sdr2file.pod \
	ici/doc/pod1/sdrmend.pod \
	ici/doc/pod1/sdrwatch.pod \
	ici/doc/pod1/sm2file.pod \
	ici/doc/pod1/smlistsh.pod \
	ici/doc/pod1/smrbtsh.pod \
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
	ici/doc/pod3/smrbt.pod \
	ici/doc/pod3/sdrlist.pod \
	ici/doc/pod3/sdrstring.pod \
	ici/doc/pod3/sdrtable.pod \
	ici/doc/pod3/sdrhash.pod \
	ici/doc/pod3/sdr.pod \
	ici/library/symtab.c
if BUILD_BPv7
iciextra += \
	ici/doc/pod3/ioncbor.pod \
	ici/doc/pod3/ioncrc.pod
endif # end BUILD_BPv7

icimans = \
	$(top_builddir)/ici/doc/file2sdr.1 \
	$(top_builddir)/ici/doc/file2sm.1 \
	$(top_builddir)/ici/doc/ionadmin.1 \
	$(top_builddir)/ici/doc/ionlog.1 \
	$(top_builddir)/ici/doc/ionsecadmin.1 \
	$(top_builddir)/ici/doc/ionunlock.1 \
	$(top_builddir)/ici/doc/ionxnowner.1 \
	$(top_builddir)/ici/doc/owltsim.1 \
	$(top_builddir)/ici/doc/owlttb.1 \
	$(top_builddir)/ici/doc/psmshell.1 \
	$(top_builddir)/ici/doc/psmwatch.1 \
	$(top_builddir)/ici/doc/rfxclock.1 \
	$(top_builddir)/ici/doc/sdr2file.1 \
	$(top_builddir)/ici/doc/sdrmend.1 \
	$(top_builddir)/ici/doc/sdrwatch.1 \
	$(top_builddir)/ici/doc/sm2file.1 \
	$(top_builddir)/ici/doc/smlistsh.1 \
	$(top_builddir)/ici/doc/smrbtsh.1 \
	$(top_builddir)/ici/doc/ion.3 \
	$(top_builddir)/ici/doc/llcv.3 \
	$(top_builddir)/ici/doc/lyst.3 \
	$(top_builddir)/ici/doc/memmgr.3 \
	$(top_builddir)/ici/doc/platform.3 \
	$(top_builddir)/ici/doc/psm.3 \
	$(top_builddir)/ici/doc/sdr.3 \
	$(top_builddir)/ici/doc/sdrhash.3 \
	$(top_builddir)/ici/doc/sdrlist.3 \
	$(top_builddir)/ici/doc/sdrstring.3 \
	$(top_builddir)/ici/doc/sdrtable.3 \
	$(top_builddir)/ici/doc/smlist.3 \
	$(top_builddir)/ici/doc/smrbt.3 \
	$(top_builddir)/ici/doc/zco.3 \
	$(top_builddir)/ici/doc/ionconfig.5 \
	$(top_builddir)/ici/doc/ionrc.5 \
	$(top_builddir)/ici/doc/ionsecrc.5
if BUILD_BPv7
icimans += \
	$(top_builddir)/ici/doc/ioncbor.3 \
	$(top_builddir)/ici/doc/ioncrc.3
endif # end BUILD_BPv7

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

libici_la_SOURCES =	\
	ici/bulk/STUB_BULK/bulk.c \
	ici/library/cbor.c \
	ici/library/crc.c \
	ici/library/ion.c \
	ici/library/ionsec.c \
	ici/library/libicinm.c \
	ici/library/llcv.c \
	ici/library/lyst.c \
	ici/library/memmgr.c \
	ici/library/platform.c \
	ici/library/platform_sm.c \
	ici/library/psm.c \
	ici/library/rfx.c \
	ici/library/smlist.c \
	ici/library/smrbt.c \
	ici/library/sptrace.c \
	ici/library/zco.c \
	ici/sdr/sdrcatlg.c \
	ici/sdr/sdrhash.c \
	ici/sdr/sdrlist.c \
	ici/sdr/sdrmgt.c \
	ici/sdr/sdrstring.c \
	ici/sdr/sdrtable.c \
	ici/sdr/sdrxn.c

libici_la_CFLAGS = $(icicflags) $(AM_CFLAGS)
libici_la_LIBADD = -lm

# --- Utility Programs --- #

ionadmin_SOURCES = ici/utils/ionadmin.c
ionadmin_LDADD = libici.la -lm
ionadmin_CFLAGS = $(icicflags) $(AM_CFLAGS)

ionlog_SOURCES = ici/utils/ionlog.c
ionlog_LDADD = libici.la -lm
ionlog_CFLAGS = $(icicflags) $(AM_CFLAGS)

ionsecadmin_SOURCES = ici/utils/ionsecadmin.c
ionsecadmin_LDADD = libici.la -lm
ionsecadmin_CFLAGS = $(icicflags) $(AM_CFLAGS)

ionunlock_SOURCES = ici/utils/ionunlock.c
ionunlock_LDADD = libici.la -lm
ionunlock_CFLAGS = $(icicflags) $(AM_CFLAGS)

ionwarn_SOURCES = ici/utils/ionwarn.c
ionwarn_LDADD = libici.la -lm
ionwarn_CFLAGS = $(icicflags) $(AM_CFLAGS)

ionxnowner_SOURCES = ici/utils/ionxnowner.c
ionxnowner_LDADD = libici.la -lm
ionxnowner_CFLAGS = $(icicflags) $(AM_CFLAGS)

psmwatch_SOURCES = ici/utils/psmwatch.c
psmwatch_LDADD = libici.la -lm
psmwatch_CFLAGS = $(icicflags) $(AM_CFLAGS)

sdrmend_SOURCES = ici/utils/sdrmend.c
sdrmend_LDADD = libici.la -lm
sdrmend_CFLAGS = $(icicflags) $(AM_CFLAGS)

sdrwatch_SOURCES = ici/utils/sdrwatch.c
sdrwatch_LDADD = libici.la -lm
sdrwatch_CFLAGS = $(icicflags) $(AM_CFLAGS)

# --- Test Executables --- #

file2sdr_SOURCES = ici/test/file2sdr.c
file2sdr_LDADD = libici.la -lm
file2sdr_CFLAGS = $(icicflags) $(AM_CFLAGS)

file2sm_SOURCES = ici/test/file2sm.c
file2sm_LDADD = libici.la -lm
file2sm_CFLAGS = $(icicflags) $(AM_CFLAGS)

owltsim_SOURCES = ici/test/owltsim.c
owltsim_LDADD = libici.la $(PTHREAD_LIBS) -lm
owltsim_CFLAGS = $(icicflags) $(AM_CFLAGS)

owlttb_SOURCES = ici/test/owlttb.c
owlttb_LDADD = libici.la $(PTHREAD_LIBS) -lm
owlttb_CFLAGS = $(icicflags) $(AM_CFLAGS)

psmshell_SOURCES = ici/test/psmshell.c
psmshell_LDADD = libici.la -lm
psmshell_CFLAGS = $(icicflags) $(AM_CFLAGS)

sdr2file_SOURCES = ici/test/sdr2file.c
sdr2file_LDADD = libici.la -lm
sdr2file_CFLAGS = $(icicflags) $(AM_CFLAGS)

sm2file_SOURCES = ici/test/sm2file.c
sm2file_LDADD = libici.la -lm
sm2file_CFLAGS = $(icicflags) $(AM_CFLAGS)

smlistsh_SOURCES = ici/test/smlistsh.c
smlistsh_LDADD = libici.la -lm
smlistsh_CFLAGS = $(icicflags) $(AM_CFLAGS)

smrbtsh_SOURCES = ici/test/smrbtsh.c
smrbtsh_LDADD = libici.la -lm
smrbtsh_CFLAGS = $(icicflags) $(AM_CFLAGS)

# --- Daemon Executables --- #

rfxclock_SOURCES = ici/daemon/rfxclock.c
rfxclock_LDADD = libici.la -lm
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
	dccplsi \
	dccplso \
	ltpadmin \
	ltpclock \
	ltpcounter \
	ltpdeliv \
	ltpdriver \
	ltpmeter \
	ltpsecadmin \
	sdatest \
	udplsi \
	udplso

ltplib = \
	libltp.la \
	libudplsa.la

ltpinclude = \
	ltp/include/ltp.h \
	ltp/include/sda.h

ltpnoinst = \
	ltp/dccp/dccplsa.h \
	ltp/library/ltpP.h \
	ltp/library/ltpsec.h \
	ltp/udp/udplsa.h

ltpextra = \
	ltp/README.txt \
	ltp/doc/pod1/dccplsi.pod \
	ltp/doc/pod1/dccplso.pod \
	ltp/doc/pod1/ltpadmin.pod \
	ltp/doc/pod1/ltpclock.pod \
	ltp/doc/pod1/ltpcounter.pod \
	ltp/doc/pod1/ltpdriver.pod \
	ltp/doc/pod1/ltpmeter.pod \
	ltp/doc/pod1/ltpsecadmin.pod \
	ltp/doc/pod1/udplsi.pod \
	ltp/doc/pod1/udplso.pod \
	ltp/doc/pod3/ltp.pod \
	ltp/doc/pod5/ltprc.pod

ltpmans = \
	$(top_builddir)/ltp/doc/dccplsi.1 \
	$(top_builddir)/ltp/doc/dccplso.1 \
	$(top_builddir)/ltp/doc/ltpadmin.1 \
	$(top_builddir)/ltp/doc/ltpclock.1 \
	$(top_builddir)/ltp/doc/ltpcounter.1 \
	$(top_builddir)/ltp/doc/ltpdriver.1 \
	$(top_builddir)/ltp/doc/ltpmeter.1 \
	$(top_builddir)/ltp/doc/ltpsecadmin.1 \
	$(top_builddir)/ltp/doc/udplsi.1 \
	$(top_builddir)/ltp/doc/udplso.1 \
	$(top_builddir)/ltp/doc/ltp.3 \
	$(top_builddir)/ltp/doc/ltprc.5

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

# -- Libraries --- #

libltp_la_SOURCES = \
	ltp/library/libltp.c \
	ltp/library/libltpnm.c \
	ltp/library/libltpP.c \
	ltp/library/ltpei.c \
	ltp/library/ltpsec.c \
	ltp/sda/libsda.c

libltp_la_CFLAGS = $(ltpcflags) $(AM_CFLAGS)
libltp_la_LIBADD = libici.la -lm

libudplsa_la_SOURCES = ltp/udp/libudplsa.c
libudplsa_la_CFLAGS = $(ltpcflags) $(AM_CFLAGS) $(PTHREAD_LIBS)
libudplsa_la_LIBADD = libici.la libltp.la -lm

# --- Utility Programs --- #

ltpadmin_SOURCES = ltp/utils/ltpadmin.c
ltpadmin_LDADD = libltp.la libici.la 
ltpadmin_CFLAGS = $(ltpcflags) $(AM_CFLAGS)

ltpsecadmin_SOURCES = ltp/utils/ltpsecadmin.c
ltpsecadmin_LDADD = libltp.la libici.la 
ltpsecadmin_CFLAGS = $(ltpcflags) $(AM_CFLAGS)

# --- Test Executables --- #

ltpcounter_SOURCES = ltp/test/ltpcounter.c
ltpcounter_LDADD = libltp.la libici.la 
ltpcounter_CFLAGS = $(ltpcflags) $(AM_CFLAGS)

ltpdriver_SOURCES = ltp/test/ltpdriver.c
ltpdriver_LDADD = libltp.la libici.la 
ltpdriver_CFLAGS = $(ltpcflags) $(AM_CFLAGS)

sdatest_SOURCES = ltp/test/sdatest.c
sdatest_LDADD = libltp.la libici.la 
sdatest_CFLAGS = $(ltpcflags) $(AM_CFLAGS)

# --- Daemon Executables --- #

ltpclock_SOURCES = ltp/daemon/ltpclock.c
ltpclock_LDADD = libltp.la libici.la 
ltpclock_CFLAGS = $(ltpcflags) $(AM_CFLAGS)

ltpdeliv_SOURCES = ltp/daemon/ltpdeliv.c
ltpdeliv_LDADD = libltp.la libici.la 
ltpdeliv_CFLAGS = $(ltpcflags) $(AM_CFLAGS)

ltpmeter_SOURCES = ltp/daemon/ltpmeter.c
ltpmeter_LDADD = libltp.la libici.la 
ltpmeter_CFLAGS = $(ltpcflags) $(AM_CFLAGS)

# --- UDP Executables --- #

udplsi_SOURCES = ltp/udp/udplsi.c
udplsi_LDADD = libltp.la libudplsa.la libici.la $(PTHREAD_LIBS)
udplsi_CFLAGS = $(ltpcflags) $(AM_CFLAGS)

udplso_SOURCES = ltp/udp/udplso.c
udplso_LDADD = libltp.la libudplsa.la libici.la $(PTHREAD_LIBS)
udplso_CFLAGS = $(ltpcflags) $(AM_CFLAGS)

# --- AOS Executables --- #

#aoslsi_SOURCES = ltp/aos/aoslsi.c
#aoslsi_LDADD = libltp.la libici.la $(PTHREAD_LIBS)
#aoslsi_CFLAGS = $(ltpcflags) $(AM_CFLAGS)

#aoslso_SOURCES = ltp/aos/aoslso.c
#aoslso_LDADD = libltp.la libici.la 
#aoslso_CFLAGS = $(ltpcflags) $(AM_CFLAGS)

# --- DCCP Executables --- #

dccplsi_SOURCES = ltp/dccp/dccplsi.c
dccplsi_LDADD = libltp.la libici.la 
dccplsi_CFLAGS = $(ltpcflags) $(AM_CFLAGS) $(PTHREAD_LIBS)

dccplso_SOURCES = ltp/dccp/dccplso.c
dccplso_LDADD = libltp.la libici.la $(PTHREAD_LIBS)
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
	dgr2file \
	file2dgr \
	file2tcp \
	tcp2file \
	udp2file

if !WINDOWS
dgrbin += \
	file2udp
endif

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

dgr2file_SOURCES = dgr/test/dgr2file.c
dgr2file_LDADD = libdgr.la libici.la -lm
dgr2file_CFLAGS = $(dgrcflags) $(AM_CFLAGS)

file2dgr_SOURCES = dgr/test/file2dgr.c
file2dgr_LDADD = libdgr.la libici.la -lm
file2dgr_CFLAGS = $(dgrcflags) $(AM_CFLAGS)

file2tcp_SOURCES = dgr/test/file2tcp.c
file2tcp_LDADD = libici.la -lm
file2tcp_CFLAGS = $(dgrcflags) $(AM_CFLAGS)

file2udp_SOURCES = dgr/test/file2udp.c
file2udp_LDADD = libici.la -lm
file2udp_CFLAGS = $(dgrcflags) $(AM_CFLAGS)

tcp2file_SOURCES = dgr/test/tcp2file.c
tcp2file_LDADD = libici.la -lm
tcp2file_CFLAGS = $(dgrcflags) $(AM_CFLAGS)

udp2file_SOURCES = dgr/test/udp2file.c
udp2file_LDADD = libici.la -lm
udp2file_CFLAGS = $(dgrcflags) $(AM_CFLAGS)

if BUILD_BPv6
##########################
#
# BPv6 SECTION
#
##########################

BP_SRC_DIR = $(srcdir)/bpv6

# Define pod2man stuff
bpPODM1 = pod2man -s 1 -c "BP executables"
bpPODM3 = pod2man -s 3 -c "BP library functions"
bpPODM5 = pod2man -s 5 -c "BP configuration files"

if UNIBO_CGR
### UNIBO CGR EXTENSIONS RGR AND CGRR ###

bpcflags = -I$(BP_SRC_DIR)/library -I$(BP_SRC_DIR)/library/ext -I$(BP_SRC_DIR)/library/ext/meb -I$(BP_SRC_DIR)/library/ext/bae -I$(BP_SRC_DIR)/include -I$(BP_SRC_DIR)/ipn -I$(BP_SRC_DIR)/ipnd -I$(srcdir)/ici/libbloom-master -I$(srcdir)/ici/libbloom-master/murmur2 -I$(BP_SRC_DIR)/dtn2 -I$(BP_SRC_DIR)/library/crypto -I$(BP_SRC_DIR)/imc -I$(BP_SRC_DIR)/tcp -I$(BP_SRC_DIR)/udp -I$(srcdir)/ici/include -I$(BP_SRC_DIR)/library/ext/snw -I$(BP_SRC_DIR)/bibe -I$(BP_SRC_DIR)/imc -DBP_EXTENDED -I$(BP_SRC_DIR)/library/acs -I$(BP_SRC_DIR)/library/ext/ecos -I$(BP_SRC_DIR)/library/ext/sbsp -I$(BP_SRC_DIR)/library/ext/phn -I$(BP_SRC_DIR)/library/ext/sbsp/ciphersuites -I$(BP_SRC_DIR)/library/ext/cteb -I$(BP_SRC_DIR)/library/ext/rgr -I$(BP_SRC_DIR)/library/ext/cgrr

#########################################
else
bpcflags = -I$(BP_SRC_DIR)/library -I$(BP_SRC_DIR)/library/ext -I$(BP_SRC_DIR)/library/ext/meb -I$(BP_SRC_DIR)/library/ext/bae -I$(BP_SRC_DIR)/include -I$(BP_SRC_DIR)/ipn -I$(BP_SRC_DIR)/ipnd -I$(srcdir)/ici/libbloom-master -I$(srcdir)/ici/libbloom-master/murmur2 -I$(BP_SRC_DIR)/dtn2 -I$(BP_SRC_DIR)/library/crypto -I$(BP_SRC_DIR)/imc -I$(BP_SRC_DIR)/tcp -I$(BP_SRC_DIR)/udp -I$(srcdir)/ici/include -I$(BP_SRC_DIR)/library/ext/snw -I$(BP_SRC_DIR)/bibe -I$(BP_SRC_DIR)/imc -DBP_EXTENDED -I$(BP_SRC_DIR)/library/acs -I$(BP_SRC_DIR)/library/ext/ecos -I$(BP_SRC_DIR)/library/ext/sbsp -I$(BP_SRC_DIR)/library/ext/phn -I$(BP_SRC_DIR)/library/ext/sbsp/ciphersuites -I$(BP_SRC_DIR)/library/ext/cteb
endif

bpbin = \
	bibeadmin \
	bibeclo \
	bpadmin \
	bpcancel \
	bpclm \
	bpclock \
	bpcounter \
	bpcrash \
	bpdriver \
	bpecho \
	bping \
	bplist \
	bpnmtest \
	bprecvfile \
	bpsecadmin \
	bpsendfile \
	bpsink \
	bpsource \
	bpstats \
	bptrace \
	bptransit \
	bpversion \
	brsccla \
	brsscla \
	bsspcli \
	bsspclo \
	cgrfetch \
	dgrcli \
	dgrclo \
	dtn2admin \
	dtn2adminep \
	dtn2fw \
	hmackeys \
	ipnadmin \
	ipnadminep \
	ipnd \
	ipnfw \
	lgagent \
	lgsend \
	ltpcli \
	ltpclo \
	stcpcli \
	stcpclo \
	tcpcli \
	tcpclo \
	udpcli \
	udpclo

if !WINDOWS
bpbin += \
	bpchat \
	bpstats2 \
	dccpcli \
	dccpclo
endif

if ENABLE_BPACS
bpbin += \
	acsadmin \
	acslist
endif # end ENABLE_BPACS


if ENABLE_IMC
bpbin += \
	imcadmin \
	imcfw
endif # end ENABLE_IMC

bplib = \
	libbp.la \
	libcgr.la \
	libstcpcla.la \
	libudpcla.la

bpinclude = \
	bpv6/include/bp.h \
	bpv6/include/eureka.h

bpnoinst = \
	bpv6/brs/brscla.h \
	bpv6/dccp/dccpcla.h \
	bpv6/dtn2/dtn2fw.h \
	bpv6/include/bpsec.h \
	bpv6/ipn/ipnfw.h \
	bpv6/ipnd/ipndP.h \
	bpv6/library/bpP.h \
	bpv6/library/saga.h \
	bpv6/ltp/ltpcla.h \
	bpv6/stcp/stcpcla.h \
	bpv6/udp/udpcla.h

bpextra = \
	bpv6/README.txt \
	bpv6/bibe/bibe.h \
	bpv6/doc/pod1/bibeclo.pod \
	bpv6/doc/pod1/bpadmin.pod \
	bpv6/doc/pod1/bpcancel.pod \
	bpv6/doc/pod1/bpchat.pod \
	bpv6/doc/pod1/bpclm.pod \
	bpv6/doc/pod1/bpclock.pod \
	bpv6/doc/pod1/bpcounter.pod \
	bpv6/doc/pod1/bpdriver.pod \
	bpv6/doc/pod1/bpecho.pod \
	bpv6/doc/pod1/bping.pod \
	bpv6/doc/pod1/bplist.pod \
	bpv6/doc/pod1/bpnmtest.pod \
	bpv6/doc/pod1/bprecvfile.pod \
	bpv6/doc/pod1/bpsecadmin.pod \
	bpv6/doc/pod1/bpsendfile.pod \
	bpv6/doc/pod1/bpsink.pod \
	bpv6/doc/pod1/bpsource.pod \
	bpv6/doc/pod1/bpstats.pod \
	bpv6/doc/pod1/bpstats2.pod \
	bpv6/doc/pod1/bptrace.pod \
	bpv6/doc/pod1/bptransit.pod \
	bpv6/doc/pod1/brsccla.pod \
	bpv6/doc/pod1/brsscla.pod \
	bpv6/doc/pod1/cgrfetch.pod \
	bpv6/doc/pod1/dccpcli.pod \
	bpv6/doc/pod1/dccpclo.pod \
	bpv6/doc/pod1/dgrcli.pod \
	bpv6/doc/pod1/dgrclo.pod \
	bpv6/doc/pod1/dtn2admin.pod \
	bpv6/doc/pod1/dtn2adminep.pod \
	bpv6/doc/pod1/dtn2fw.pod \
	bpv6/doc/pod1/hmackeys.pod \
	bpv6/doc/pod1/ipnadmin.pod \
	bpv6/doc/pod1/ipnadminep.pod \
	bpv6/doc/pod1/ipnd.pod \
	bpv6/doc/pod1/ipnfw.pod \
	bpv6/doc/pod1/lgagent.pod \
	bpv6/doc/pod1/lgsend.pod \
	bpv6/doc/pod1/ltpcli.pod \
	bpv6/doc/pod1/ltpclo.pod \
	bpv6/doc/pod1/stcpcli.pod \
	bpv6/doc/pod1/stcpclo.pod \
	bpv6/doc/pod1/tcpcli.pod \
	bpv6/doc/pod1/tcpclo.pod \
	bpv6/doc/pod1/udpcli.pod \
	bpv6/doc/pod1/udpclo.pod \
	bpv6/doc/pod3/bp.pod \
	bpv6/doc/pod3/bpextensions.pod \
	bpv6/doc/pod5/bprc.pod \
	bpv6/doc/pod5/bpsecrc.pod \
	bpv6/doc/pod5/dtn2rc.pod \
	bpv6/doc/pod5/ipnrc.pod \
	bpv6/doc/pod5/lgfile.pod \
	bpv6/include/bpsec.h \
	bpv6/library/bei.c \
	bpv6/library/bpsec.c \
	bpv6/library/saga.h \
	bpv6/library/saga.c \
	bpv6/library/ext/bpextensions.c \
	bpv6/library/ext/noextensions.c \
	bpv6/library/ext/bae/bae.h \
	bpv6/library/ext/bae/bae.c \
	bpv6/library/ext/ecos/ecos.h \
	bpv6/library/ext/ecos/ecos.c \
	bpv6/library/ext/meb/meb.h \
	bpv6/library/ext/meb/meb.c \
	bpv6/library/ext/sbsp/profiles.h \
	bpv6/library/ext/sbsp/profiles.c \
	bpv6/library/ext/sbsp/sbsp_bcb.h \
	bpv6/library/ext/sbsp/sbsp_bcb.c \
	bpv6/library/ext/sbsp/sbsp_bib.h \
	bpv6/library/ext/sbsp/sbsp_bib.c \
	bpv6/library/ext/sbsp/sbsp_instr.h \
	bpv6/library/ext/sbsp/sbsp_instr.c \
	bpv6/library/ext/sbsp/sbsp_util.h \
	bpv6/library/ext/sbsp/sbsp_util.c \
	ici/include/crypto.h \
	ici/include/csi.h

if ENABLE_IMC
bpextra += \
	bpv6/doc/pod1/imcadmin.pod \
	bpv6/doc/pod1/imcfw.pod \
	bpv6/doc/pod5/imcrc.pod
endif # end ENABLE_IMC

bpmans = \
	$(top_builddir)/bpv6/doc/bibeclo.1 \
	$(top_builddir)/bpv6/doc/bpadmin.1 \
	$(top_builddir)/bpv6/doc/bpcancel.1 \
	$(top_builddir)/bpv6/doc/bpchat.1 \
	$(top_builddir)/bpv6/doc/bpclm.1 \
	$(top_builddir)/bpv6/doc/bpclock.1 \
	$(top_builddir)/bpv6/doc/bpcounter.1 \
	$(top_builddir)/bpv6/doc/bpdriver.1 \
	$(top_builddir)/bpv6/doc/bpecho.1 \
	$(top_builddir)/bpv6/doc/bping.1 \
	$(top_builddir)/bpv6/doc/bplist.1 \
	$(top_builddir)/bpv6/doc/bpnmtest.1 \
	$(top_builddir)/bpv6/doc/bprecvfile.1 \
	$(top_builddir)/bpv6/doc/bpsecadmin.1 \
	$(top_builddir)/bpv6/doc/bpsendfile.1 \
	$(top_builddir)/bpv6/doc/bpsink.1 \
	$(top_builddir)/bpv6/doc/bpsource.1 \
	$(top_builddir)/bpv6/doc/bpstats.1 \
	$(top_builddir)/bpv6/doc/bpstats2.1 \
	$(top_builddir)/bpv6/doc/bptrace.1 \
	$(top_builddir)/bpv6/doc/bptransit.1 \
	$(top_builddir)/bpv6/doc/brsccla.1 \
	$(top_builddir)/bpv6/doc/brsscla.1 \
	$(top_builddir)/bpv6/doc/cgrfetch.1 \
	$(top_builddir)/bpv6/doc/dccpcli.1 \
	$(top_builddir)/bpv6/doc/dccpclo.1 \
	$(top_builddir)/bpv6/doc/dgrcli.1 \
	$(top_builddir)/bpv6/doc/dgrclo.1 \
	$(top_builddir)/bpv6/doc/dtn2admin.1 \
	$(top_builddir)/bpv6/doc/dtn2adminep.1 \
	$(top_builddir)/bpv6/doc/dtn2fw.1 \
	$(top_builddir)/bpv6/doc/hmackeys.1 \
	$(top_builddir)/bpv6/doc/ipnadmin.1 \
	$(top_builddir)/bpv6/doc/ipnadminep.1 \
	$(top_builddir)/bpv6/doc/ipnd.1 \
	$(top_builddir)/bpv6/doc/ipnfw.1 \
	$(top_builddir)/bpv6/doc/lgagent.1 \
	$(top_builddir)/bpv6/doc/lgsend.1 \
	$(top_builddir)/bpv6/doc/ltpcli.1 \
	$(top_builddir)/bpv6/doc/ltpclo.1 \
	$(top_builddir)/bpv6/doc/stcpcli.1 \
	$(top_builddir)/bpv6/doc/stcpclo.1 \
	$(top_builddir)/bpv6/doc/tcpcli.1 \
	$(top_builddir)/bpv6/doc/tcpclo.1 \
	$(top_builddir)/bpv6/doc/udpcli.1 \
	$(top_builddir)/bpv6/doc/udpclo.1 \
	$(top_builddir)/bpv6/doc/bp.3 \
	$(top_builddir)/bpv6/doc/bpextensions.3 \
	$(top_builddir)/bpv6/doc/bprc.5 \
	$(top_builddir)/bpv6/doc/bpsecrc.5 \
	$(top_builddir)/bpv6/doc/dtn2rc.5 \
	$(top_builddir)/bpv6/doc/ipnrc.5 \
	$(top_builddir)/bpv6/doc/lgfile.5

# ACS only exists in BPv6 so only build the docs if BPv6 and ACS is enabled
if ENABLE_BPACS
bpmans += \
	$(top_builddir)/bpv6/doc/acsadmin.1 \
	$(top_builddir)/bpv6/doc/acslist.1 \
	$(top_builddir)/bpv6/doc/acsrc.5
endif # end ENABLE_BPACS


if ENABLE_IMC
bpmans += \
	$(top_builddir)/bpv6/doc/imcadmin.1 \
	$(top_builddir)/bpv6/doc/imcfw.1 \
	$(top_builddir)/bpv6/doc/imcrc.5
endif # end ENABLE_IMC


bpclean-local:
	rm -f $(top_builddir)/bpv6/doc/*.1
	rm -f $(top_builddir)/bpv6/doc/*.3
	rm -f $(top_builddir)/bpv6/doc/*.5

$(top_builddir)/bpv6/doc/%.1: bpv6/doc/pod1/%.pod
	$(bpPODM1) $< $@

$(top_builddir)/bpv6/doc/%.3: bpv6/doc/pod3/%.pod
	$(bpPODM3) $< $@

$(top_builddir)/bpv6/doc/%.5: bpv6/doc/pod5/%.pod
	$(bpPODM5) $< $@

# -- Libraries --- #

if UNIBO_CGR
### UNIBO CGR EXTENSIONS RGR AND CGRR ###

libbp_la_SOURCES = \
	bpv6/bibe/bibe.c \
	bpv6/dtn2/libdtn2fw.c \
	bpv6/ipn/libipnfw.c \
	bpv6/ipnd/ipnd.c \
	bpv6/ipnd/beacon.c \
	bpv6/ipnd/helper.c \
	bpv6/ipnd/bpa.c \
	bpv6/library/acs/acs.h \
	bpv6/library/bei.c \
	bpv6/library/bpsec.c \
	bpv6/library/eureka.c \
	bpv6/library/libbp.c \
	bpv6/library/libbpP.c \
	bpv6/library/libbpnm.c \
	bpv6/library/saga.c \
	bpv6/library/ext/bae/bae.c \
	bpv6/library/ext/ecos/ecos.c \
	bpv6/library/ext/meb/meb.c \
	bpv6/library/ext/phn/phn.c \
	bpv6/library/ext/sbsp/sbsp_instr.c \
	bpv6/library/ext/sbsp/sbsp_bcb.c \
	bpv6/library/ext/sbsp/sbsp_bib.c \
	bpv6/library/ext/sbsp/sbsp_util.c \
	bpv6/library/ext/sbsp/profiles.c \
	bpv6/library/ext/snid/snid.c \
	bpv6/library/ext/snw/snw.c \
	bpv6/library/ext/cgrr/cgrr.h \
	bpv6/library/ext/cgrr/cgrr.c \
	bpv6/library/ext/cgrr/cgrr_utils.h \
	bpv6/library/ext/cgrr/cgrr_utils.c \
	bpv6/library/ext/cgrr/cgrr_help.h \
	bpv6/library/ext/cgrr/cgrr_msr_utils.h \
	bpv6/library/ext/cgrr/cgrr_msr_utils.c \
	bpv6/library/ext/rgr/rgr.h \
	bpv6/library/ext/rgr/rgr.c \
	bpv6/library/ext/rgr/rgr_utils.h \
	bpv6/library/ext/rgr/rgr_utils.c \
	ici/libbloom-master/bloom.h \
	ici/libbloom-master/bloom.c \
	ici/libbloom-master/murmur2/MurmurHash2.c
	
#########################################
else
libbp_la_SOURCES = \
	bpv6/bibe/bibe.c \
	bpv6/dtn2/libdtn2fw.c \
	bpv6/ipn/libipnfw.c \
	bpv6/ipnd/ipnd.c \
	bpv6/ipnd/beacon.c \
	bpv6/ipnd/helper.c \
	bpv6/ipnd/bpa.c \
	bpv6/library/acs/acs.h \
	bpv6/library/bei.c \
	bpv6/library/bpsec.c \
	bpv6/library/eureka.c \
	bpv6/library/libbp.c \
	bpv6/library/libbpP.c \
	bpv6/library/libbpnm.c \
	bpv6/library/saga.c \
	bpv6/library/ext/bae/bae.c \
	bpv6/library/ext/ecos/ecos.c \
	bpv6/library/ext/meb/meb.c \
	bpv6/library/ext/phn/phn.c \
	bpv6/library/ext/sbsp/sbsp_instr.c \
	bpv6/library/ext/sbsp/sbsp_bcb.c \
	bpv6/library/ext/sbsp/sbsp_bib.c \
	bpv6/library/ext/sbsp/sbsp_util.c \
	bpv6/library/ext/sbsp/profiles.c \
	bpv6/library/ext/snid/snid.c \
	bpv6/library/ext/snw/snw.c \
	ici/libbloom-master/bloom.h \
	ici/libbloom-master/bloom.c \
	ici/libbloom-master/murmur2/MurmurHash2.c
endif

if CRYPTO
libici_la_SOURCES += \
	ici/crypto/NULL_SUITES/crypto.c\
	ici/crypto/NULL_SUITES/csi.c
else
libici_la_SOURCES += \
	ici/crypto/NULL_SUITES/crypto.c\
	ici/crypto/NULL_SUITES/csi.c
endif # end CRYPTO

libbp_la_CFLAGS = $(bpcflags) $(AM_CFLAGS) -I$(BP_SRC_DIR)/library/ext -I$(BP_SRC_DIR)
libbp_la_LIBADD = libici.la -lm $(CRYPTO_LIBS)

if ENABLE_BPACS
libbp_la_SOURCES += \
	bpv6/library/acs/acsappend.c \
	bpv6/library/acs/acsid.c \
	bpv6/library/acs/acsrx.c \
	bpv6/library/acs/acsserialize.c \
	bpv6/library/acs/acstx.c \
	bpv6/library/ext/cteb/cteb.c \
	$(NULL)
endif # end ENABLE_BPACS

if ENABLE_IMC
libbp_la_SOURCES += \
	bpv6/imc/libimcfw.c \
	$(NULL)
endif # end ENABLE_IMC

if UNIBO_CGR
### UNIBO CGR CORE AND INTERFACE ###

libcgr_la_SOURCES = \
	bpv6/cgr/libcgr.c \
	bpv6/cgr/Unibo-CGR/ion_bpv6/interface/interface_cgr_ion.c \
	bpv6/cgr/Unibo-CGR/core/library/commonFunctions.c \
	bpv6/cgr/Unibo-CGR/core/library/list/list.c \
	bpv6/cgr/Unibo-CGR/core/library/log/log.c \
	bpv6/cgr/Unibo-CGR/core/library_from_ion/rbt/rbt.c \
	bpv6/cgr/Unibo-CGR/core/library_from_ion/scalar/scalar.c \
	bpv6/cgr/Unibo-CGR/ion_bpv6/interface/utility_functions_from_ion/general_functions_ported_from_ion.c \
	bpv6/cgr/Unibo-CGR/core/contact_plan/contacts/contacts.c \
	bpv6/cgr/Unibo-CGR/core/contact_plan/ranges/ranges.c \
	bpv6/cgr/Unibo-CGR/core/contact_plan/nodes/nodes.c \
	bpv6/cgr/Unibo-CGR/core/contact_plan/contactPlan.c \
	bpv6/cgr/Unibo-CGR/core/routes/routes.c \
	bpv6/cgr/Unibo-CGR/core/cgr/phase_one.c \
	bpv6/cgr/Unibo-CGR/core/cgr/phase_two.c \
	bpv6/cgr/Unibo-CGR/core/cgr/phase_three.c \
	bpv6/cgr/Unibo-CGR/core/cgr/cgr.c \
	bpv6/cgr/Unibo-CGR/core/msr/msr.c \
	bpv6/cgr/Unibo-CGR/core/msr/msr_utils.c \
	bpv6/cgr/Unibo-CGR/core/bundles/bundles.c \
	bpv6/cgr/Unibo-CGR/core/time_analysis/time.c

####################################
else
libcgr_la_SOURCES = bpv6/cgr/libcgr.c
endif

libcgr_la_CFLAGS = $(bpcflags) $(AM_CFLAGS)
libcgr_la_LIBADD = libbp.la libici.la

libstcpcla_la_SOURCES = bpv6/stcp/libstcpcla.c
libstcpcla_la_CFLAGS = $(bpcflags) $(AM_CFLAGS)
libstcpcla_la_LIBADD = libbp.la libici.la

libudpcla_la_SOURCES = bpv6/udp/libudpcla.c
libudpcla_la_CFLAGS = $(bpcflags) $(AM_CFLAGS)
libudpcla_la_LIBADD = libbp.la libici.la

# --- Utility Programs --- #

# Only try to build ACS if BPv6 and ACS is enabled
if ENABLE_BPACS
acsadmin_SOURCES = bpv6/utils/acsadmin.c
acsadmin_LDADD = libbp.la libici.la $(LIBOBJS)
acsadmin_CFLAGS = $(bpcflags) $(AM_CFLAGS)

acslist_SOURCES = bpv6/utils/acslist.c
acslist_LDADD = libbp.la libici.la $(LIBOBJS)
acslist_CFLAGS = $(bpcflags) $(AM_CFLAGS)
endif # end ENABLE_BPACS

bpadmin_SOURCES = bpv6/utils/bpadmin.c
bpadmin_LDADD = libbp.la libici.la $(LIBOBJS)
bpadmin_CFLAGS = $(bpcflags) $(AM_CFLAGS)

bpcancel_SOURCES = bpv6/utils/bpcancel.c
bpcancel_LDADD = libbp.la libici.la $(LIBOBJS)
bpcancel_CFLAGS = $(bpcflags) $(AM_CFLAGS)

bplist_SOURCES = bpv6/utils/bplist.c
bplist_LDADD = libbp.la libici.la $(LIBOBJS)
bplist_CFLAGS = $(bpcflags) $(AM_CFLAGS)

bpsecadmin_SOURCES = bpv6/utils/bpsecadmin.c
bpsecadmin_LDADD = libbp.la libici.la $(LIBOBJS)
bpsecadmin_CFLAGS = $(bpcflags) $(AM_CFLAGS)

bpstats_SOURCES = bpv6/utils/bpstats.c
bpstats_LDADD = libbp.la libici.la $(LIBOBJS)
bpstats_CFLAGS = $(bpcflags) $(AM_CFLAGS)

bptrace_SOURCES = bpv6/utils/bptrace.c
bptrace_LDADD = libbp.la libici.la $(LIBOBJS)
bptrace_CFLAGS = $(bpcflags) $(AM_CFLAGS)

bpversion_SOURCES = bpv6/utils/bpversion.c
bpversion_LDADD = libbp.la libici.la $(LIBOBJS)
bpversion_CFLAGS = $(bpcflags) $(AM_CFLAGS)

dtn2admin_SOURCES = bpv6/dtn2/dtn2admin.c
dtn2admin_LDADD = libbp.la libici.la $(LIBOBJS)
dtn2admin_CFLAGS = $(bpcflags) $(AM_CFLAGS)

hmackeys_SOURCES = bpv6/utils/hmackeys.c
hmackeys_LDADD = libici.la -lm libbp.la $(LIBOBJS)
hmackeys_CFLAGS = $(bpcflags) $(AM_CFLAGS)

if ENABLE_IMC
imcadmin_SOURCES = bpv6/imc/imcadmin.c
imcadmin_LDADD = libbp.la libici.la $(LIBOBJS)
imcadmin_CFLAGS = $(bpcflags) $(AM_CFLAGS)

imcfw_SOURCES = bpv6/imc/imcfw.c
imcfw_LDADD = libbp.la libici.la $(LIBOBJS)
imcfw_CFLAGS = $(bpcflags) $(AM_CFLAGS)
endif # end ENABLE_IMC

ipnadmin_SOURCES = bpv6/ipn/ipnadmin.c
ipnadmin_LDADD = libbp.la libici.la $(LIBOBJS)
ipnadmin_CFLAGS = $(bpcflags) $(AM_CFLAGS)

lgagent_SOURCES = bpv6/utils/lgagent.c
lgagent_LDADD = libbp.la libici.la $(LIBOBJS)
lgagent_CFLAGS = $(bpcflags) $(AM_CFLAGS)

lgsend_SOURCES = bpv6/utils/lgsend.c
lgsend_LDADD = libbp.la libici.la $(LIBOBJS)
lgsend_CFLAGS = $(bpcflags) $(AM_CFLAGS)

# --- DTN Executables --- #

dtn2adminep_SOURCES = bpv6/dtn2/dtn2adminep.c
dtn2adminep_LDADD = libbp.la libici.la $(LIBOBJS)
dtn2adminep_CFLAGS = $(bpcflags) $(AM_CFLAGS)

dtn2fw_SOURCES = bpv6/dtn2/dtn2fw.c
dtn2fw_LDADD = libbp.la libici.la $(LIBOBJS)
dtn2fw_CFLAGS = $(bpcflags) $(AM_CFLAGS)

# --- IPN Executables --- #

ipnadminep_SOURCES = bpv6/ipn/ipnadminep.c
ipnadminep_LDADD = libbp.la libici.la $(LIBOBJS)
ipnadminep_CFLAGS = $(bpcflags) $(AM_CFLAGS)

ipnfw_SOURCES = bpv6/ipn/ipnfw.c
ipnfw_LDADD = libcgr.la libbp.la libici.la $(LIBOBJS)
ipnfw_CFLAGS = $(bpcflags) $(AM_CFLAGS)

# --- IPND Executables --- #

ipnd_SOURCES = \
	bpv6/ipnd/beacon.c \
	bpv6/ipnd/bpa.c \
	bpv6/ipnd/helper.c \
	bpv6/ipnd/ipnd.c
ipnd_LDADD = libici.la $(bplib)
ipnd_CFLAGS = $(bpcflags) $(AM_CFLAGS)

# --- BSSP Executables --- #

bsspcli_SOURCES = bpv6/bssp/bsspcli.c
bsspcli_LDADD = libbssp.la libbp.la libici.la $(LIBOBJS) $(PTHREAD_LIBS)
bsspcli_CFLAGS = $(bpcflags) $(AM_CFLAGS)

bsspclo_SOURCES = bpv6/bssp/bsspclo.c
bsspclo_LDADD = libbssp.la libbp.la libici.la $(LIBOBJS)
bsspclo_CFLAGS = $(bpcflags) $(AM_CFLAGS)

# --- LTP Executables --- #

ltpcli_SOURCES = bpv6/ltp/ltpcli.c
ltpcli_LDADD = libbp.la libltp.la libici.la $(LIBOBJS) $(PTHREAD_LIBS)
ltpcli_CFLAGS = $(bpcflags) $(AM_CFLAGS)

ltpclo_SOURCES = bpv6/ltp/ltpclo.c
ltpclo_LDADD = libbp.la libltp.la libici.la $(LIBOBJS)
ltpclo_CFLAGS = $(bpcflags) $(AM_CFLAGS)

# --- TCP Executables --- #

tcpcli_SOURCES = bpv6/tcp/tcpcli.c
tcpcli_LDADD = libbp.la libici.la $(LIBOBJS) $(PTHREAD_LIBS)
tcpcli_CFLAGS = $(bpcflags) $(AM_CFLAGS)

tcpclo_SOURCES = bpv6/tcp/tcpclo.c
tcpclo_LDADD = libbp.la libici.la $(LIBOBJS) $(PTHREAD_LIBS)
tcpclo_CFLAGS = $(bpcflags) $(AM_CFLAGS)

# --- STCP Executables --- #

stcpcli_SOURCES = bpv6/stcp/stcpcli.c
stcpcli_LDADD = libbp.la libici.la libstcpcla.la $(LIBOBJS) $(PTHREAD_LIBS)
stcpcli_CFLAGS = $(bpcflags) $(AM_CFLAGS)

stcpclo_SOURCES = bpv6/stcp/stcpclo.c
stcpclo_LDADD = libbp.la libici.la libstcpcla.la $(LIBOBJS) $(PTHREAD_LIBS)
stcpclo_CFLAGS = $(bpcflags) $(AM_CFLAGS)

# --- UDP Executables --- #

udpcli_SOURCES = bpv6/udp/udpcli.c
udpcli_LDADD = libbp.la libudpcla.la libici.la $(LIBOBJS) $(PTHREAD_LIBS)
udpcli_CFLAGS = $(bpcflags) $(AM_CFLAGS)

udpclo_SOURCES = bpv6/udp/udpclo.c
udpclo_LDADD = libbp.la libudpcla.la libici.la $(LIBOBJS)
udpclo_CFLAGS = $(bpcflags) $(AM_CFLAGS)

# --- BIBE Executables --- #

bibeadmin_SOURCES = bpv6/bibe/bibeadmin.c
bibeadmin_LDADD = libbp.la libici.la $(LIBOBJS)
bibeadmin_CFLAGS = $(bpcflags) $(AM_CFLAGS)

bibeclo_SOURCES = bpv6/bibe/bibeclo.c
bibeclo_LDADD = libbp.la libudpcla.la libici.la $(LIBOBJS)
bibeclo_CFLAGS = $(bpcflags) $(AM_CFLAGS)

# --- DCCP Executables --- #

dccpcli_SOURCES = bpv6/dccp/dccpcli.c
dccpcli_LDADD = libbp.la libici.la $(LIBOBJS) $(PTHREAD_LIBS)
dccpcli_CFLAGS = $(bpcflags) $(AM_CFLAGS)

dccpclo_SOURCES = bpv6/dccp/dccpclo.c
dccpclo_LDADD = libbp.la libici.la $(LIBOBJS) $(PTHREAD_LIBS)
dccpclo_CFLAGS = $(bpcflags) $(AM_CFLAGS)

# --- DGR Executables --- #

dgrcli_SOURCES = bpv6/dgr/dgrcli.c
dgrcli_LDADD = libbp.la libdgr.la libici.la $(LIBOBJS) $(PTHREAD_LIBS)
dgrcli_CFLAGS = $(bpcflags) $(AM_CFLAGS)

dgrclo_SOURCES = bpv6/dgr/dgrclo.c
dgrclo_LDADD = libbp.la libdgr.la libici.la $(LIBOBJS) $(PTHREAD_LIBS)
dgrclo_CFLAGS = $(bpcflags) $(AM_CFLAGS)

# --- BRS executables --- #

brsccla_SOURCES = bpv6/brs/brsccla.c
brsccla_LDADD = libbp.la libici.la libstcpcla.la $(LIBOBJS) $(PTHREAD_LIBS)
brsccla_CFLAGS = $(bpcflags) $(AM_CFLAGS)

brsscla_SOURCES = bpv6/brs/brsscla.c
brsscla_LDADD = libbp.la libici.la libstcpcla.la $(LIBOBJS) $(PTHREAD_LIBS)
brsscla_CFLAGS = $(bpcflags) $(AM_CFLAGS)

# --- Test Executables --- #

bpchat_SOURCES = bpv6/test/bpchat.c
bpchat_LDADD = libbp.la libici.la $(LIBOBJS) $(PTHREAD_LIBS)
bpchat_CFLAGS = $(bpcflags) $(AM_CFLAGS)

bpcounter_SOURCES = bpv6/test/bpcounter.c
bpcounter_LDADD = libbp.la libici.la $(LIBOBJS)
bpcounter_CFLAGS = $(bpcflags) $(AM_CFLAGS)

bpcrash_SOURCES = bpv6/test/bpcrash.c
bpcrash_LDADD = libbp.la libici.la $(LIBOBJS)
bpcrash_CFLAGS = $(bpcflags) $(AM_CFLAGS)

bpdriver_SOURCES = bpv6/test/bpdriver.c
bpdriver_LDADD = libbp.la libici.la $(LIBOBJS)
bpdriver_CFLAGS = $(bpcflags) $(AM_CFLAGS)

bpecho_SOURCES = bpv6/test/bpecho.c
bpecho_LDADD = libbp.la libici.la $(LIBOBJS)
bpecho_CFLAGS = $(bpcflags) $(AM_CFLAGS)

bping_SOURCES = bpv6/test/bping.c
bping_LDADD = libbp.la libici.la $(LIBOBJS) $(PTHREAD_LIBS)
bping_CFLAGS = $(bpcflags) $(AM_CFLAGS)

bpnmtest_SOURCES = bpv6/test/bpnmtest.c
bpnmtest_LDADD = libbp.la libici.la $(LIBOBJS)
bpnmtest_CFLAGS = $(bpcflags) $(AM_CFLAGS)

bprecvfile_SOURCES = bpv6/utils/bprecvfile.c
bprecvfile_LDADD = libbp.la libici.la $(LIBOBJS)
bprecvfile_CFLAGS = $(bpcflags) $(AM_CFLAGS)

bpsendfile_SOURCES = bpv6/utils/bpsendfile.c
bpsendfile_LDADD = libbp.la libici.la $(LIBOBJS)
bpsendfile_CFLAGS = $(bpcflags) $(AM_CFLAGS)

bpsink_SOURCES = bpv6/test/bpsink.c
bpsink_LDADD = libbp.la libici.la $(LIBOBJS)
bpsink_CFLAGS = $(bpcflags) $(AM_CFLAGS)

bpsource_SOURCES = bpv6/test/bpsource.c
bpsource_LDADD = libbp.la libici.la $(LIBOBJS)
bpsource_CFLAGS = $(bpcflags) $(AM_CFLAGS)

bpstats2_SOURCES = bpv6/test/bpstats2.c
bpstats2_LDADD = libbp.la libici.la $(LIBOBJS)
bpstats2_CFLAGS = $(bpcflags) $(AM_CFLAGS)

cgrfetch_SOURCES = bpv6/utils/cgrfetch.c
cgrfetch_LDADD = libbp.la libici.la libcgr.la $(LIBOBJS)
cgrfetch_CFLAGS = $(bpcflags) $(AM_CFLAGS)

# --- Daemon Executables --- #

bpclm_SOURCES = bpv6/daemon/bpclm.c
bpclm_LDADD = libbp.la libici.la $(LIBOBJS)
bpclm_CFLAGS = $(bpcflags) $(AM_CFLAGS)

bpclock_SOURCES = bpv6/daemon/bpclock.c
bpclock_LDADD = libbp.la libici.la $(LIBOBJS)
bpclock_CFLAGS = $(bpcflags) $(AM_CFLAGS)

bptransit_SOURCES = bpv6/daemon/bptransit.c
bptransit_LDADD = libbp.la libici.la $(LIBOBJS)
bptransit_CFLAGS = $(bpcflags) $(AM_CFLAGS)

############################
#
# NETWORK MANAGEMENT SECTION
#                                        
############################

nmbin= nm_agent nm_mgr
nmlib= libamp.la libampAgentADM.la

nmPODM1 = pod2man -s 1 -c "NM executables"
nmPODM3 = pod2man -s 1 -c "NM libraries"

# CBOR Library Sources
nm_cbor_src = \
	nm/contrib/QCBOR/src/UsefulBuf.c \
	nm/contrib/QCBOR/src/qcbor_encode.c \
	nm/contrib/QCBOR/src/qcbor_decode.c \
	nm/contrib/QCBOR/src/ieee754.c

AMP_VERSION ?= 8
#$(warning "AMP_VERSION is ${AMP_VERSION}")
NM_CBOR_CFLAGS = -I $(srcdir)/nm/contrib/QCBOR/inc -DAMP_VERSION=${AMP_VERSION}

# NM Shared library
#  This library is standalone (no dependencies on other ION components)
nm_shared_src =	\
	nm/shared/adm/adm.c \
	nm/shared/msg/msg.c \
	nm/shared/primitives/ari.c \
	nm/shared/primitives/blob.c \
	nm/shared/primitives/ctrl.c \
	nm/shared/primitives/edd_var.c \
	nm/shared/primitives/expr.c \
	nm/shared/primitives/report.c \
	nm/shared/primitives/rules.c \
	nm/shared/primitives/table.c \
	nm/shared/primitives/tnv.c \
	nm/shared/utils/cbor_utils.c \
	nm/shared/utils/db.c \
	nm/shared/utils/nm_types.c \
	nm/shared/utils/rhht.c \
	nm/shared/utils/utils.c \
	nm/shared/utils/vector.c \
	$(nm_cbor_src)

libamp_la_SOURCES = $(nm_shared_src)
libamp_la_CFLAGS = -I/usr/include -I$(srcdir)/ici/library -I$(srcdir)/bpv6/library -I$(srcdir)/ltp/library -I$(srcdir)/nm  \
                  -I$(srcdir)/nm/agent -I$(srcdir)/bpv6/library/ext/sbsp $(AM_CFLAGS) $(NO_UNUSED_VARIABLE) \
                  $(NM_CBOR_CFLAGS)
# Note: ici reference is used here only for memory management.  If -DUSE_MALLOC is used for NM (not recommended), this dependency would be unneeded.
libamp_la_LIBADD = libici.la

# Note: AMP Agent ADM must be built directly with the agent, whereas all other ADMs can be modularized as needed.
nm_agent_SOURCES = \
	nm/agent/ingest.c \
	nm/agent/instr.c \
	nm/agent/lcc.c \
	nm/agent/nmagent.c \
	nm/agent/ldc.c \
	nm/agent/rda.c \
	nm/shared/msg/ion_if.c \
	nm/agent/adm_amp_agent_impl.c \
	nm/agent/adm_amp_agent_agent.c \
	nm/shared/adm/adm_init.c

NM_ADM_INCLUDES = -I$(srcdir)/nm/shared/adm -I$(BP_SRC_DIR)/adm -I$(BP_SRC_DIR)

nm_agent_ADM_SOURCES = \
	bpv6/adm/adm_bp_agent_impl.c \
	bpv6/adm/adm_bp_agent_agent.c \
	bpv6/adm/adm_ion_bp_admin_impl.c \
	bpv6/adm/adm_ion_bp_admin_agent.c \
	bpv6/adm/adm_sbsp_impl.c \
	bpv6/adm/adm_sbsp_agent.c \
	nm/agent/adm_ion_admin_agent.c \
	nm/agent/adm_ion_admin_impl.c \
	nm/agent/adm_ionsec_admin_impl.c \
	nm/agent/adm_ionsec_admin_agent.c \
	nm/agent/adm_ion_ipn_admin_impl.c \
	nm/agent/adm_ion_ipn_admin_agent.c \
	nm/agent/adm_ion_ltp_admin_impl.c \
	nm/agent/adm_ion_ltp_admin_agent.c \
	nm/agent/adm_ltp_agent_impl.c \
	nm/agent/adm_ltp_agent_agent.c



nm_mgr_ADM_SOURCES = \
	bpv6/adm/adm_bp_agent_mgr.c \
	bpv6/adm/adm_ion_bp_admin_mgr.c \
	bpv6/adm/adm_sbsp_mgr.c \
	nm/mgr/adm_amp_agent_mgr.c \
	nm/mgr/adm_ion_admin_mgr.c \
	nm/mgr/adm_ionsec_admin_mgr.c \
	nm/mgr/adm_ion_ipn_admin_mgr.c \
	nm/mgr/adm_ion_ltp_admin_mgr.c \
	nm/mgr/adm_ltp_agent_mgr.c



nmextra = \
	nm/doc/pod1/nm_agent.pod \
	nm/doc/pod1/nm_mgr.pod

nmmans = \
	$(top_builddir)/nm/doc/nm_agent.1 \
	$(top_builddir)/nm/doc/nm_mgr.1

libampAgentADM_la_SOURCES = $(nm_agent_ADM_SOURCES)
libampAgentADM_la_CFLAGS = -I/usr/include -I$(srcdir)/nm -I$(srcdir)/nm/shared/adm -I$(BP_SRC_DIR)/library/ext/sbsp -I$(BP_SRC_DIR) -I$(BP_SRC_DIR)/library -I$(srcdir)/ltp/library $(NM_CBOR_CFLAGS) $(AM_CFLAGS) $(NO_UNUSED_VARIABLE) 
libampAgentADM_la_LIBADD = libamp.la libici.la libbp.la libltp.la


nm_agent_LDADD = libici.la libbp.la libltp.la libamp.la libampAgentADM.la $(LIBOBJS) $(PTHREAD_LIBS)
nm_agent_LDFLAGS = -L/usr/local/lib -lm -lpthread 

nm_agent_CFLAGS = -I/usr/include -I$(srcdir)/ici/library -I$(srcdir)/bpv6/library -I$(srcdir)/nm \
                  -I$(srcdir)/nm/agent -I$(srcdir)/nm/shared/adm $(AM_CFLAGS) $(NO_UNUSED_VARIABLE) \
                  -DAGENT_ROLE $(NM_CBOR_CFLAGS) $(NM_ADM_INCLUDES)


nm_mgr_SOURCES = \
	nm/mgr/agents.c \
	nm/mgr/metadata.c \
	nm/mgr/nm_mgr_print.c \
	nm/mgr/nm_mgr_rx.c \
	nm/mgr/nm_mgr_sql.c \
	nm/mgr/nm_mgr_ui.c \
	nm/mgr/nm_mgr.c \
	nm/mgr/ui_input.c \
	$(nm_mgr_ADM_SOURCES) \
	nm/shared/msg/ion_if.c \
	nm/shared/adm/adm_init.c

# TODO: Split cJSON as a seperate configurable supporting library
CIVETWEB_SOURCES = \
	nm/contrib/civetweb/src/civetweb.c \
	nm/contrib/civetweb/examples/rest/cJSON/cJSON.c \
	nm/contrib/civetweb/examples/rest/cJSON/cJSON_Utils.c \
	nm/mgr/nm_rest.c
CIVETWEB_INCLUDES = -I$(srcdir)/nm/contrib/civetweb/include -I$(srcdir)/nm/contrib/civetweb/examples/rest/cJSON
CIVETWEB_LDFLAGS = -ldl
CIVETWEB_CFLAGS = -DNO_SSL -DUSE_CIVETWEB -DUSE_JSON

nm_mgr_LDADD = libamp.la libici.la libbp.la libltp.la $(LIBOBJS) $(PTHREAD_LIBS)

# Note: NCURSES and MySql support are optional and controlled at configure time with --with-mysql or --with-ncurses
nm_mgr_LDFLAGS = -L/usr/local/mysql/lib -lpthread ${MYSQL_LIBS} ${NCURSES_LIBS}
nm_mgr_CFLAGS = -I/usr/local/mysql/include -I$(srcdir)/ici/library -I$(srcdir)/bpv6/library -I$(srcdir)/bpv6/library/ext/sbsp -I$(srcdir)/ltp/library -I$(srcdir)/nm  -I$(srcdir)/nm/mgr -I/usr/include/mysql $(AM_CFLAGS) $(NO_UNUSED_VARIABLE) $(NM_CBOR_CFLAGS) $(NM_ADM_INCLUDES)

if BUILD_NM_REST
nm_mgr_LDFLAGS += $(CIVETWEB_LDFLAGS)
nm_mgr_CFLAGS += $(CIVETWEB_INCLUDES) $(CIVETWEB_CFLAGS)
nm_mgr_SOURCES += $(CIVETWEB_SOURCES)

endif

$(top_builddir)/nm/doc/%.1: nm/doc/pod1/%.pod
	$(nmPODM1) $< $@

$(top_builddir)/nm/doc/%.3: nm/doc/pod3/%.pod
	$(nmPODM3) $< $@

else #BUILD_BPv7
##########################
#
# BPv7 SECTION
#
##########################

BP_SRC_DIR = $(srcdir)/bpv7

# Define pod2man stuff
bpPODM1 = pod2man -s 1 -c "BP executables"
bpPODM3 = pod2man -s 3 -c "BP library functions"
bpPODM5 = pod2man -s 5 -c "BP configuration files"

if UNIBO_CGR
### UNIBO CGR EXTENSIONS RGR AND CGRR ###

bpcflags = -I$(BP_SRC_DIR)/library -I$(BP_SRC_DIR)/library/ext -I$(BP_SRC_DIR)/library/ext/meb -I$(BP_SRC_DIR)/library/ext/bae -I$(BP_SRC_DIR)/include -I$(BP_SRC_DIR)/ipn -I$(BP_SRC_DIR)/ipnd -I$(srcdir)/ici/libbloom-master -I$(srcdir)/ici/libbloom-master/murmur2 -I$(BP_SRC_DIR)/dtn2 -I$(BP_SRC_DIR)/library/crypto -I$(BP_SRC_DIR)/tcp -I$(BP_SRC_DIR)/udp -I$(srcdir)/ici/include -I$(BP_SRC_DIR)/library/ext/snw -I$(BP_SRC_DIR)/bibe -I$(BP_SRC_DIR)/imc -DBP_EXTENDED -I$(BP_SRC_DIR)/saga -I$(BP_SRC_DIR)/library/ext/bpsec -I$(BP_SRC_DIR)/library/ext/hcb -I$(BP_SRC_DIR)/library/ext/pnb -I$(BP_SRC_DIR)/library/ext/bpq -I$(BP_SRC_DIR)/library/ext/rgr -I$(BP_SRC_DIR)/library/ext/cgrr

#########################################
else
bpcflags = -I$(BP_SRC_DIR)/library -I$(BP_SRC_DIR)/library/ext -I$(BP_SRC_DIR)/library/ext/meb -I$(BP_SRC_DIR)/library/ext/bae -I$(BP_SRC_DIR)/include -I$(BP_SRC_DIR)/ipn -I$(BP_SRC_DIR)/ipnd -I$(srcdir)/ici/libbloom-master -I$(srcdir)/ici/libbloom-master/murmur2 -I$(BP_SRC_DIR)/dtn2 -I$(BP_SRC_DIR)/library/crypto -I$(BP_SRC_DIR)/tcp -I$(BP_SRC_DIR)/udp -I$(srcdir)/ici/include -I$(BP_SRC_DIR)/library/ext/snw -I$(BP_SRC_DIR)/bibe -I$(BP_SRC_DIR)/imc -DBP_EXTENDED -I$(BP_SRC_DIR)/saga -I$(BP_SRC_DIR)/library/ext/bpsec -I$(BP_SRC_DIR)/library/ext/hcb -I$(BP_SRC_DIR)/library/ext/pnb -I$(BP_SRC_DIR)/library/ext/bpq
endif

if ENABLE_IMC
bpcflags += -I$(BP_SRC_DIR)/imc -I$(BP_SRC_DIR)/library/ext/imc
endif # end ENABLE_IMC

bpbin = \
	bibeadmin \
	bibeclo \
	bpadmin \
	bpcancel \
	bpclm \
	bpclock \
	bpcounter \
	bpcrash \
	bpdriver \
	bpecho \
	bping \
	bplist \
	bpnmtest \
	bprecvfile \
	bpsecadmin \
	bpsendfile \
	bpsink \
	bpsource \
	bpstats \
	bptrace \
	bptransit \
	bpversion \
	brsccla \
	brsscla \
	bsspcli \
	bsspclo \
	cgrfetch \
	dgrcli \
	dgrclo \
	dtn2admin \
	dtn2adminep \
	dtn2fw \
	hmackeys \
	ipnadmin \
	ipnadminep \
	ipnd \
	ipnfw \
	lgagent \
	lgsend \
	ltpcli \
	ltpclo \
	stcpcli \
	stcpclo \
	tcpcli \
	tcpclo \
	udpcli \
	udpclo

if !WINDOWS
bpbin += \
	bpchat \
	bpstats2 \
	dccpcli \
	dccpclo
endif

if ENABLE_IMC
bpbin += \
	imcadminep \
	imcfw
endif # end ENABLE_IMC

bplib = \
	libbp.la \
	libcgr.la \
	libstcpcla.la \
	libudpcla.la

bpinclude = \
	bpv7/include/bp.h \
	bpv7/include/eureka.h

bpnoinst = \
	bpv7/brs/brscla.h \
	bpv7/dccp/dccpcla.h \
	bpv7/dtn2/dtn2fw.h \
	bpv7/include/bpsec.h \
	bpv7/ipn/ipnfw.h \
	bpv7/ipnd/ipndP.h \
	bpv7/library/bpP.h \
	bpv7/ltp/ltpcla.h \
	bpv7/saga/saga.h \
	bpv7/stcp/stcpcla.h \
	bpv7/udp/udpcla.h

bpextra = \
	bpv7/README.txt \
	bpv7/bibe/bibe.h \
	bpv7/doc/pod1/bibeadmin.pod \
	bpv7/doc/pod1/bibeclo.pod \
	bpv7/doc/pod1/bpadmin.pod \
	bpv7/doc/pod1/bpcancel.pod \
	bpv7/doc/pod1/bpchat.pod \
	bpv7/doc/pod1/bpclm.pod \
	bpv7/doc/pod1/bpclock.pod \
	bpv7/doc/pod1/bpcounter.pod \
	bpv7/doc/pod1/bpdriver.pod \
	bpv7/doc/pod1/bpecho.pod \
	bpv7/doc/pod1/bping.pod \
	bpv7/doc/pod1/bplist.pod \
	bpv7/doc/pod1/bpnmtest.pod \
	bpv7/doc/pod1/bprecvfile.pod \
	bpv7/doc/pod1/bpsecadmin.pod \
	bpv7/doc/pod1/bpsendfile.pod \
	bpv7/doc/pod1/bpsink.pod \
	bpv7/doc/pod1/bpsource.pod \
	bpv7/doc/pod1/bpstats.pod \
	bpv7/doc/pod1/bpstats2.pod \
	bpv7/doc/pod1/bptrace.pod \
	bpv7/doc/pod1/bptransit.pod \
	bpv7/doc/pod1/brsccla.pod \
	bpv7/doc/pod1/brsscla.pod \
	bpv7/doc/pod1/cgrfetch.pod \
	bpv7/doc/pod1/dccpcli.pod \
	bpv7/doc/pod1/dccpclo.pod \
	bpv7/doc/pod1/dgrcli.pod \
	bpv7/doc/pod1/dgrclo.pod \
	bpv7/doc/pod1/dtn2admin.pod \
	bpv7/doc/pod1/dtn2adminep.pod \
	bpv7/doc/pod1/dtn2fw.pod \
	bpv7/doc/pod1/hmackeys.pod \
	bpv7/doc/pod1/ipnadmin.pod \
	bpv7/doc/pod1/ipnadminep.pod \
	bpv7/doc/pod1/ipnd.pod \
	bpv7/doc/pod1/ipnfw.pod \
	bpv7/doc/pod1/lgagent.pod \
	bpv7/doc/pod1/lgsend.pod \
	bpv7/doc/pod1/ltpcli.pod \
	bpv7/doc/pod1/ltpclo.pod \
	bpv7/doc/pod1/stcpcli.pod \
	bpv7/doc/pod1/stcpclo.pod \
	bpv7/doc/pod1/tcpcli.pod \
	bpv7/doc/pod1/tcpclo.pod \
	bpv7/doc/pod1/udpcli.pod \
	bpv7/doc/pod1/udpclo.pod \
	bpv7/doc/pod3/bp.pod \
	bpv7/doc/pod3/bpextensions.pod \
	bpv7/doc/pod5/biberc.pod \
	bpv7/doc/pod5/bprc.pod \
	bpv7/doc/pod5/bpsecrc.pod \
	bpv7/doc/pod5/dtn2rc.pod \
	bpv7/doc/pod5/ipnrc.pod \
	bpv7/doc/pod5/lgfile.pod \
	bpv7/include/bpsec.h \
	bpv7/library/bei.c \
	bpv7/library/bpsec.c \
	bpv7/library/saga.h \
	bpv7/library/saga.c \
	bpv7/library/ext/bpextensions.c \
	bpv7/library/ext/noextensions.c \
	bpv7/library/ext/bpsec/bcb.h \
	bpv7/library/ext/bpsec/bib.h \
	bpv7/library/ext/bpsec/bpsec_instr.h \
	bpv7/library/ext/bpsec/bpsec_util.h \
	bpv7/library/ext/bpsec/profiles.h \
	bpv7/library/ext/ecos/ecos.h \
	bpv7/library/ext/ecos/ecos.c \
	bpv7/library/ext/hcb/hcb.h \
	bpv7/library/ext/meb/meb.h \
	bpv7/library/ext/meb/meb.c \
	bpv7/library/ext/bae/bae.h \
	bpv7/library/ext/bae/bae.c \
	ici/include/crypto.h \
	ici/include/csi.h

if ENABLE_IMC
bpextra += \
	bpv7/doc/pod1/imcfw.pod
endif # end ENABLE_IMC

bpmans = \
	$(top_builddir)/bpv7/doc/bibeadmin.1 \
	$(top_builddir)/bpv7/doc/bibeclo.1 \
	$(top_builddir)/bpv7/doc/bpadmin.1 \
	$(top_builddir)/bpv7/doc/bpcancel.1 \
	$(top_builddir)/bpv7/doc/bpchat.1 \
	$(top_builddir)/bpv7/doc/bpclm.1 \
	$(top_builddir)/bpv7/doc/bpclock.1 \
	$(top_builddir)/bpv7/doc/bpcounter.1 \
	$(top_builddir)/bpv7/doc/bpdriver.1 \
	$(top_builddir)/bpv7/doc/bpecho.1 \
	$(top_builddir)/bpv7/doc/bping.1 \
	$(top_builddir)/bpv7/doc/bplist.1 \
	$(top_builddir)/bpv7/doc/bpnmtest.1 \
	$(top_builddir)/bpv7/doc/bprecvfile.1 \
	$(top_builddir)/bpv7/doc/bpsecadmin.1 \
	$(top_builddir)/bpv7/doc/bpsendfile.1 \
	$(top_builddir)/bpv7/doc/bpsink.1 \
	$(top_builddir)/bpv7/doc/bpsource.1 \
	$(top_builddir)/bpv7/doc/bpstats.1 \
	$(top_builddir)/bpv7/doc/bpstats2.1 \
	$(top_builddir)/bpv7/doc/bptrace.1 \
	$(top_builddir)/bpv7/doc/bptransit.1 \
	$(top_builddir)/bpv7/doc/brsccla.1 \
	$(top_builddir)/bpv7/doc/brsscla.1 \
	$(top_builddir)/bpv7/doc/cgrfetch.1 \
	$(top_builddir)/bpv7/doc/dccpcli.1 \
	$(top_builddir)/bpv7/doc/dccpclo.1 \
	$(top_builddir)/bpv7/doc/dgrcli.1 \
	$(top_builddir)/bpv7/doc/dgrclo.1 \
	$(top_builddir)/bpv7/doc/dtn2admin.1 \
	$(top_builddir)/bpv7/doc/dtn2adminep.1 \
	$(top_builddir)/bpv7/doc/dtn2fw.1 \
	$(top_builddir)/bpv7/doc/hmackeys.1 \
	$(top_builddir)/bpv7/doc/ipnadmin.1 \
	$(top_builddir)/bpv7/doc/ipnadminep.1 \
	$(top_builddir)/bpv7/doc/ipnd.1 \
	$(top_builddir)/bpv7/doc/ipnfw.1 \
	$(top_builddir)/bpv7/doc/lgagent.1 \
	$(top_builddir)/bpv7/doc/lgsend.1 \
	$(top_builddir)/bpv7/doc/ltpcli.1 \
	$(top_builddir)/bpv7/doc/ltpclo.1 \
	$(top_builddir)/bpv7/doc/stcpcli.1 \
	$(top_builddir)/bpv7/doc/stcpclo.1 \
	$(top_builddir)/bpv7/doc/tcpcli.1 \
	$(top_builddir)/bpv7/doc/tcpclo.1 \
	$(top_builddir)/bpv7/doc/udpcli.1 \
	$(top_builddir)/bpv7/doc/udpclo.1 \
	$(top_builddir)/bpv7/doc/bp.3 \
	$(top_builddir)/bpv7/doc/bpextensions.3 \
	$(top_builddir)/bpv7/doc/biberc.5 \
	$(top_builddir)/bpv7/doc/bprc.5 \
	$(top_builddir)/bpv7/doc/bpsecrc.5 \
	$(top_builddir)/bpv7/doc/dtn2rc.5 \
	$(top_builddir)/bpv7/doc/ipnrc.5 \
	$(top_builddir)/bpv7/doc/lgfile.5

if ENABLE_IMC
bpmans += \
	$(top_builddir)/bpv7/doc/imcfw.1
endif # end ENABLE_IMC


bpclean-local:
	rm -f $(top_builddir)/bpv7/doc/*.1
	rm -f $(top_builddir)/bpv7/doc/*.3
	rm -f $(top_builddir)/bpv7/doc/*.5

$(top_builddir)/bpv7/doc/%.1: bpv7/doc/pod1/%.pod
	$(bpPODM1) $< $@

$(top_builddir)/bpv7/doc/%.3: bpv7/doc/pod3/%.pod
	$(bpPODM3) $< $@

$(top_builddir)/bpv7/doc/%.5: bpv7/doc/pod5/%.pod
	$(bpPODM5) $< $@

# -- Libraries --- #

if UNIBO_CGR
### UNIBO CGR EXTENSIONS RGR AND CGRR ###

libbp_la_SOURCES = \
	bpv7/bibe/bibe.c \
	bpv7/dtn2/libdtn2fw.c \
	bpv7/library/libbp.c \
	bpv7/library/libbpP.c \
	bpv7/library/libbpnm.c \
	bpv7/library/eureka.c \
	bpv7/library/bei.c \
	bpv7/library/bpsec.c \
	bpv7/library/ext/bae/bae.c \
	bpv7/library/ext/bpq/bpq.c \
	bpv7/library/ext/bpsec/bcb.c \
	bpv7/library/ext/bpsec/bib.c \
	bpv7/library/ext/bpsec/bpsec_instr.c \
	bpv7/library/ext/bpsec/bpsec_util.c \
	bpv7/library/ext/bpsec/profiles.c \
	bpv7/library/ext/hcb/hcb.c \
	bpv7/library/ext/meb/meb.c \
	bpv7/library/ext/pnb/pnb.c \
	bpv7/library/ext/pnb/pnb.h \
	bpv7/library/ext/snw/snw.c \
	bpv7/ipn/libipnfw.c \
	bpv7/saga/saga.c \
	bpv7/library/ext/cgrr/cgrr.h \
	bpv7/library/ext/cgrr/cgrr.c \
	bpv7/library/ext/cgrr/cgrr_utils.h \
	bpv7/library/ext/cgrr/cgrr_utils.c \
	bpv7/library/ext/cgrr/cgrr_help.h \
	bpv7/library/ext/cgrr/cgrr_msr_utils.h \
	bpv7/library/ext/cgrr/cgrr_msr_utils.c \
	bpv7/library/ext/rgr/rgr.h \
	bpv7/library/ext/rgr/rgr.c \
	bpv7/library/ext/rgr/rgr_utils.h \
	bpv7/library/ext/rgr/rgr_utils.c \
	ici/libbloom-master/bloom.h \
	ici/libbloom-master/bloom.c \
	ici/libbloom-master/murmur2/MurmurHash2.c
	
#########################################
else
libbp_la_SOURCES = \
	bpv7/bibe/bibe.c \
	bpv7/dtn2/libdtn2fw.c \
	bpv7/library/libbp.c \
	bpv7/library/libbpP.c \
	bpv7/library/libbpnm.c \
	bpv7/library/eureka.c \
	bpv7/library/bei.c \
	bpv7/library/bpsec.c \
	bpv7/library/ext/bae/bae.c \
	bpv7/library/ext/bpq/bpq.c \
	bpv7/library/ext/bpsec/bcb.c \
	bpv7/library/ext/bpsec/bib.c \
	bpv7/library/ext/bpsec/bpsec_instr.c \
	bpv7/library/ext/bpsec/bpsec_util.c \
	bpv7/library/ext/bpsec/profiles.c \
	bpv7/library/ext/hcb/hcb.c \
	bpv7/library/ext/meb/meb.c \
	bpv7/library/ext/pnb/pnb.c \
	bpv7/library/ext/pnb/pnb.h \
	bpv7/library/ext/snw/snw.c \
	bpv7/ipn/libipnfw.c \
	bpv7/saga/saga.c \
	ici/libbloom-master/bloom.h \
	ici/libbloom-master/bloom.c \
	ici/libbloom-master/murmur2/MurmurHash2.c
endif

if CRYPTO
libici_la_SOURCES+= \
	ici/crypto/NULL_SUITES/crypto.c\
	ici/crypto/NULL_SUITES/csi.c
else
libici_la_SOURCES+= \
	ici/crypto/NULL_SUITES/crypto.c\
	ici/crypto/NULL_SUITES/csi.c
endif # end CRYPTO

libbp_la_CFLAGS = $(bpcflags) $(AM_CFLAGS) -I$(BP_SRC_DIR)/library/ext -I$(BP_SRC_DIR)
libbp_la_LIBADD = libici.la -lm $(CRYPTO_LIBS)

if ENABLE_IMC
libbp_la_SOURCES += \
	bpv7/imc/libimcfw.c \
	bpv7/library/ext/imc/imc.h \
	bpv7/library/ext/imc/imc.c
endif # end ENABLE_IMC

if UNIBO_CGR
### UNIBO CGR CORE AND INTERFACE ###

libcgr_la_SOURCES = \
	bpv7/cgr/libcgr.c \
	bpv7/cgr/Unibo-CGR/ion_bpv7/interface/interface_cgr_ion.c \
	bpv7/cgr/Unibo-CGR/core/library/commonFunctions.c \
	bpv7/cgr/Unibo-CGR/core/library/list/list.c \
	bpv7/cgr/Unibo-CGR/core/library/log/log.c \
	bpv7/cgr/Unibo-CGR/core/library_from_ion/rbt/rbt.c \
	bpv7/cgr/Unibo-CGR/core/library_from_ion/scalar/scalar.c \
	bpv7/cgr/Unibo-CGR/ion_bpv7/interface/utility_functions_from_ion/general_functions_ported_from_ion.c \
	bpv7/cgr/Unibo-CGR/core/contact_plan/contacts/contacts.c \
	bpv7/cgr/Unibo-CGR/core/contact_plan/ranges/ranges.c \
	bpv7/cgr/Unibo-CGR/core/contact_plan/nodes/nodes.c \
	bpv7/cgr/Unibo-CGR/core/contact_plan/contactPlan.c \
	bpv7/cgr/Unibo-CGR/core/routes/routes.c \
	bpv7/cgr/Unibo-CGR/core/cgr/phase_one.c \
	bpv7/cgr/Unibo-CGR/core/cgr/phase_two.c \
	bpv7/cgr/Unibo-CGR/core/cgr/phase_three.c \
	bpv7/cgr/Unibo-CGR/core/cgr/cgr.c \
	bpv7/cgr/Unibo-CGR/core/msr/msr.c \
	bpv7/cgr/Unibo-CGR/core/msr/msr_utils.c \
	bpv7/cgr/Unibo-CGR/core/bundles/bundles.c \
	bpv7/cgr/Unibo-CGR/core/time_analysis/time.c
	
####################################
else
libcgr_la_SOURCES = bpv7/cgr/libcgr.c
endif

libcgr_la_CFLAGS = $(bpcflags) $(AM_CFLAGS)
libcgr_la_LIBADD = libbp.la libici.la

libstcpcla_la_SOURCES = bpv7/stcp/libstcpcla.c
libstcpcla_la_CFLAGS = $(bpcflags) $(AM_CFLAGS)
libstcpcla_la_LIBADD = libbp.la libici.la

libudpcla_la_SOURCES = bpv7/udp/libudpcla.c
libudpcla_la_CFLAGS = $(bpcflags) $(AM_CFLAGS)
libudpcla_la_LIBADD = libbp.la libici.la

# --- Utility Programs --- #

bpadmin_SOURCES = bpv7/utils/bpadmin.c
bpadmin_LDADD = libici.la $(bplib)
bpadmin_CFLAGS = $(bpcflags) $(AM_CFLAGS)

bpcancel_SOURCES = bpv7/utils/bpcancel.c
bpcancel_LDADD = libici.la $(bplib)
bpcancel_CFLAGS = $(bpcflags) $(AM_CFLAGS)

bplist_SOURCES = bpv7/utils/bplist.c
bplist_LDADD = libici.la $(bplib)
bplist_CFLAGS = $(bpcflags) $(AM_CFLAGS)

bpsecadmin_SOURCES = bpv7/utils/bpsecadmin.c
bpsecadmin_LDADD = libici.la $(bplib)
bpsecadmin_CFLAGS = $(bpcflags) $(AM_CFLAGS)

bpstats_SOURCES = bpv7/utils/bpstats.c
bpstats_LDADD = libici.la $(bplib)
bpstats_CFLAGS = $(bpcflags) $(AM_CFLAGS)

bptrace_SOURCES = bpv7/utils/bptrace.c
bptrace_LDADD = libici.la $(bplib)
bptrace_CFLAGS = $(bpcflags) $(AM_CFLAGS)

bpversion_SOURCES = bpv7/utils/bpversion.c
bpversion_LDADD = libici.la $(bplib)
bpversion_CFLAGS = $(bpcflags) $(AM_CFLAGS)

dtn2admin_SOURCES = bpv7/dtn2/dtn2admin.c
dtn2admin_LDADD = libici.la $(bplib)
dtn2admin_CFLAGS = $(bpcflags) $(AM_CFLAGS)

hmackeys_SOURCES = bpv7/utils/hmackeys.c
hmackeys_LDADD = libici.la -lm $(bplib)
hmackeys_CFLAGS = $(bpcflags) $(AM_CFLAGS)

if ENABLE_IMC
imcadminep_SOURCES = bpv7/imc/imcadminep.c
imcadminep_LDADD = libici.la $(bplib)
imcadminep_CFLAGS = $(bpcflags) $(AM_CFLAGS)

imcfw_SOURCES = bpv7/imc/imcfw.c
imcfw_LDADD = libici.la $(bplib)
imcfw_CFLAGS = $(bpcflags) $(AM_CFLAGS)
endif # end ENABLE_IMC

ipnadmin_SOURCES = bpv7/ipn/ipnadmin.c
ipnadmin_LDADD = libici.la $(bplib)
ipnadmin_CFLAGS = $(bpcflags) $(AM_CFLAGS)

lgagent_SOURCES = bpv7/utils/lgagent.c
lgagent_LDADD = libici.la $(bplib)
lgagent_CFLAGS = $(bpcflags) $(AM_CFLAGS)

lgsend_SOURCES = bpv7/utils/lgsend.c
lgsend_LDADD = libici.la $(bplib)
lgsend_CFLAGS = $(bpcflags) $(AM_CFLAGS)

# --- DTN Executables --- #

dtn2adminep_SOURCES = bpv7/dtn2/dtn2adminep.c
dtn2adminep_LDADD = libici.la $(bplib)
dtn2adminep_CFLAGS = $(bpcflags) $(AM_CFLAGS)

dtn2fw_SOURCES = bpv7/dtn2/dtn2fw.c
dtn2fw_LDADD = libici.la $(bplib)
dtn2fw_CFLAGS = $(bpcflags) $(AM_CFLAGS)

# --- IPN Executables --- #

ipnadminep_SOURCES = bpv7/ipn/ipnadminep.c
ipnadminep_LDADD = libici.la $(bplib)
ipnadminep_CFLAGS = $(bpcflags) $(AM_CFLAGS)

ipnfw_SOURCES = bpv7/ipn/ipnfw.c
ipnfw_LDADD = libici.la $(bplib)
ipnfw_CFLAGS = $(bpcflags) $(AM_CFLAGS)

# --- IPND Executables --- #

ipnd_SOURCES = \
	bpv7/ipnd/beacon.c \
	bpv7/ipnd/bpa.c \
	bpv7/ipnd/helper.c \
	bpv7/ipnd/ipnd.c
ipnd_LDADD = libici.la $(bplib)
ipnd_CFLAGS = $(bpcflags) $(AM_CFLAGS)

# --- BSSP Executables --- #

bsspcli_SOURCES = bpv7/bssp/bsspcli.c
bsspcli_LDADD = libbssp.la libici.la $(bplib) $(PTHREAD_LIBS)
bsspcli_CFLAGS = $(bpcflags) $(AM_CFLAGS)

bsspclo_SOURCES = bpv7/bssp/bsspclo.c
bsspclo_LDADD = libbssp.la libici.la $(bplib)
bsspclo_CFLAGS = $(bpcflags) $(AM_CFLAGS)

# --- LTP Executables --- #

ltpcli_SOURCES = bpv7/ltp/ltpcli.c
ltpcli_LDADD = libltp.la libici.la $(bplib) $(PTHREAD_LIBS)
ltpcli_CFLAGS = $(bpcflags) $(AM_CFLAGS)

ltpclo_SOURCES = bpv7/ltp/ltpclo.c
ltpclo_LDADD = libltp.la libici.la $(bplib)
ltpclo_CFLAGS = $(bpcflags) $(AM_CFLAGS)

# --- TCP Executables --- #

tcpcli_SOURCES = bpv7/tcp/tcpcli.c
tcpcli_LDADD = libici.la $(bplib) $(PTHREAD_LIBS)
tcpcli_CFLAGS = $(bpcflags) $(AM_CFLAGS)

tcpclo_SOURCES = bpv7/tcp/tcpclo.c
tcpclo_LDADD = libici.la $(bplib) $(PTHREAD_LIBS)
tcpclo_CFLAGS = $(bpcflags) $(AM_CFLAGS)

# --- STCP Executables --- #

stcpcli_SOURCES = bpv7/stcp/stcpcli.c
stcpcli_LDADD = libici.la $(bplib) $(PTHREAD_LIBS)
stcpcli_CFLAGS = $(bpcflags) $(AM_CFLAGS)

stcpclo_SOURCES = bpv7/stcp/stcpclo.c
stcpclo_LDADD = libici.la $(bplib) $(PTHREAD_LIBS)
stcpclo_CFLAGS = $(bpcflags) $(AM_CFLAGS)

# --- UDP Executables --- #

udpcli_SOURCES = bpv7/udp/udpcli.c
udpcli_LDADD = libici.la $(bplib) $(PTHREAD_LIBS)
udpcli_CFLAGS = $(bpcflags) $(AM_CFLAGS)

udpclo_SOURCES = bpv7/udp/udpclo.c
udpclo_LDADD = libici.la $(bplib)
udpclo_CFLAGS = $(bpcflags) $(AM_CFLAGS)

# --- BIBE Executables --- #

bibeadmin_SOURCES = bpv7/bibe/bibeadmin.c
bibeadmin_LDADD = libici.la $(bplib)
bibeadmin_CFLAGS = $(bpcflags) $(AM_CFLAGS)

bibeclo_SOURCES = bpv7/bibe/bibeclo.c
bibeclo_LDADD = libici.la $(bplib)
bibeclo_CFLAGS = $(bpcflags) $(AM_CFLAGS)

# --- DCCP Executables --- #

dccpcli_SOURCES = bpv7/dccp/dccpcli.c
dccpcli_LDADD = libici.la $(bplib) $(PTHREAD_LIBS)
dccpcli_CFLAGS = $(bpcflags) $(AM_CFLAGS)

dccpclo_SOURCES = bpv7/dccp/dccpclo.c
dccpclo_LDADD = libici.la $(bplib) $(PTHREAD_LIBS)
dccpclo_CFLAGS = $(bpcflags) $(AM_CFLAGS)

# --- DGR Executables --- #

dgrcli_SOURCES = bpv7/dgr/dgrcli.c
dgrcli_LDADD = libdgr.la libici.la $(bplib) $(PTHREAD_LIBS)
dgrcli_CFLAGS = $(bpcflags) $(AM_CFLAGS)

dgrclo_SOURCES = bpv7/dgr/dgrclo.c
dgrclo_LDADD = libdgr.la libici.la $(bplib) $(PTHREAD_LIBS)
dgrclo_CFLAGS = $(bpcflags) $(AM_CFLAGS)

# --- BRS executables --- #

brsccla_SOURCES = bpv7/brs/brsccla.c
brsccla_LDADD = libici.la $(bplib) $(PTHREAD_LIBS)
brsccla_CFLAGS = $(bpcflags) $(AM_CFLAGS)

brsscla_SOURCES = bpv7/brs/brsscla.c
brsscla_LDADD = libici.la $(bplib) $(PTHREAD_LIBS)
brsscla_CFLAGS = $(bpcflags) $(AM_CFLAGS)

# --- Test Executables --- #

bpchat_SOURCES = bpv7/test/bpchat.c
bpchat_LDADD = libici.la $(bplib) $(PTHREAD_LIBS)
bpchat_CFLAGS = $(bpcflags) $(AM_CFLAGS)

bpcounter_SOURCES = bpv7/test/bpcounter.c
bpcounter_LDADD = libici.la $(bplib)
bpcounter_CFLAGS = $(bpcflags) $(AM_CFLAGS)

bpcrash_SOURCES = bpv7/test/bpcrash.c
bpcrash_LDADD = libici.la $(bplib)
bpcrash_CFLAGS = $(bpcflags) $(AM_CFLAGS)

bpdriver_SOURCES = bpv7/test/bpdriver.c
bpdriver_LDADD = libici.la $(bplib)
bpdriver_CFLAGS = $(bpcflags) $(AM_CFLAGS)

bpecho_SOURCES = bpv7/test/bpecho.c
bpecho_LDADD = libici.la $(bplib)
bpecho_CFLAGS = $(bpcflags) $(AM_CFLAGS)

bping_SOURCES = bpv7/test/bping.c
bping_LDADD = libici.la $(bplib) $(PTHREAD_LIBS)
bping_CFLAGS = $(bpcflags) $(AM_CFLAGS)

bpnmtest_SOURCES = bpv7/test/bpnmtest.c
bpnmtest_LDADD = libici.la $(bplib)
bpnmtest_CFLAGS = $(bpcflags) $(AM_CFLAGS)

bprecvfile_SOURCES = bpv7/utils/bprecvfile.c
bprecvfile_LDADD = libici.la $(bplib)
bprecvfile_CFLAGS = $(bpcflags) $(AM_CFLAGS)

bpsendfile_SOURCES = bpv7/utils/bpsendfile.c
bpsendfile_LDADD = libici.la $(bplib)
bpsendfile_CFLAGS = $(bpcflags) $(AM_CFLAGS)

bpsink_SOURCES = bpv7/test/bpsink.c
bpsink_LDADD = libici.la $(bplib)
bpsink_CFLAGS = $(bpcflags) $(AM_CFLAGS)

bpsource_SOURCES = bpv7/test/bpsource.c
bpsource_LDADD = libici.la $(bplib)
bpsource_CFLAGS = $(bpcflags) $(AM_CFLAGS)

bpstats2_SOURCES = bpv7/test/bpstats2.c
bpstats2_LDADD = libici.la $(bplib)
bpstats2_CFLAGS = $(bpcflags) $(AM_CFLAGS)

cgrfetch_SOURCES = bpv7/utils/cgrfetch.c
cgrfetch_LDADD = libici.la $(bplib)
cgrfetch_CFLAGS = $(bpcflags) $(AM_CFLAGS)

# --- Daemon Executables --- #

bpclm_SOURCES = bpv7/daemon/bpclm.c
bpclm_LDADD = libici.la $(bplib)
bpclm_CFLAGS = $(bpcflags) $(AM_CFLAGS)

bpclock_SOURCES = bpv7/daemon/bpclock.c
bpclock_LDADD = libici.la $(bplib)
bpclock_CFLAGS = $(bpcflags) $(AM_CFLAGS)

bptransit_SOURCES = bpv7/daemon/bptransit.c
bptransit_LDADD = libici.la $(bplib)
bptransit_CFLAGS = $(bpcflags) $(AM_CFLAGS)

############################
#
# TRUSTED COLLECTIVE SECTION
#                                        
############################

# Define pod2man stuff
tcPODM1 = pod2man -s 1 -c "TC executables"
tcPODM3 = pod2man -s 3 -c "TC library functions"
tcPODM5 = pod2man -s 5 -c "TC configuration files"

tccflags = -I$(srcdir)/tc/authority -I$(srcdir)/tc/client -I$(srcdir)/tc/include -I$(srcdir)/ici/zfec-1.4.24/zfec -I$(BP_SRC_DIR)/library -I$(ROOT)/include

tcbin = \
	dtka \
	dtkaadmin \
	tcaadmin \
	tcaboot \
	tcacompile \
	tcapublish \
	tcarecv \
	tcc \
	tccadmin

tclib = \
	libdtka.la \
	libtc.la \
	libtcaP.la \
	libtcc.la \
	libzfec.la

tcinclude = \
	tc/dtka/dtka.h \
	tc/authority/tcaP.h \
	tc/client/tcc.h \
	tc/client/tccP.h \
	tc/include/tc.h

tcextra = \
	tc/doc/pod1/dtka.pod \
	tc/doc/pod1/dtkaadmin.pod \
	tc/doc/pod1/tcaadmin.pod \
	tc/doc/pod1/tcaboot.pod \
	tc/doc/pod1/tcacompile.pod \
	tc/doc/pod1/tcapublish.pod \
	tc/doc/pod1/tcarecv.pod \
	tc/doc/pod1/tcc.pod \
	tc/doc/pod1/tccadmin.pod \
	tc/doc/pod3/tc.pod \
	tc/doc/pod5/dtkarc.pod \
	tc/doc/pod5/tcarc.pod \
	tc/doc/pod5/tccrc.pod

tcmans = \
	$(top_builddir)/tc/doc/dtka.1 \
	$(top_builddir)/tc/doc/dtkaadmin.1 \
	$(top_builddir)/tc/doc/tcaadmin.1 \
	$(top_builddir)/tc/doc/tcaboot.1 \
	$(top_builddir)/tc/doc/tcacompile.1 \
	$(top_builddir)/tc/doc/tcapublish.1 \
	$(top_builddir)/tc/doc/tcarecv.1 \
	$(top_builddir)/tc/doc/tcc.1 \
	$(top_builddir)/tc/doc/tccadmin.1 \
	$(top_builddir)/tc/doc/tc.3 \
	$(top_builddir)/tc/doc/dtkarc.5 \
	$(top_builddir)/tc/doc/tcarc.5 \
	$(top_builddir)/tc/doc/tccrc.5

tcclean-local:
	rm -f $(top_builddir)/tc/doc/*.1
	rm -f $(top_builddir)/tc/doc/*.3
	rm -f $(top_builddir)/tc/doc/*.5

$(top_builddir)/tc/doc/%.1: tc/doc/pod1/%.pod
	$(tcPODM1) $< $@

$(top_builddir)/tc/doc/%.3: tc/doc/pod3/%.pod
	$(tcPODM3) $< $@

$(top_builddir)/tc/doc/%.5: tc/doc/pod5/%.pod
	$(tcPODM5) $< $@

# -- Libraries --- #

libdtka_la_SOURCES = tc/dtka/libdtka.c
libdtka_la_CFLAGS = $(tccflags) $(AM_CFLAGS)
libdtka_la_LIBADD = libici.la libbp.la

libtc_la_SOURCES = tc/library/libtc.c
libtc_la_CFLAGS = $(tccflags) $(AM_CFLAGS)
libtc_la_LIBADD = libici.la

libtcaP_la_SOURCES = tc/authority/libtcaP.c
libtcaP_la_CFLAGS = $(tccflags) $(AM_CFLAGS)
libtcaP_la_LIBADD = libici.la libbp.la

libtcc_la_SOURCES = tc/client/libtcc.c tc/client/libtccP.c
libtcc_la_CFLAGS = $(tccflags) $(AM_CFLAGS)
libtcc_la_LIBADD = libici.la libbp.la

libzfec_la_SOURCES = ici/zfec-1.4.24/zfec/fec.c
if SOLARIS
libzfec_la_CFLAGS = $(tccflags) $(AM_CFLAGS) -Drestrict=
else
libzfec_la_CFLAGS = $(tccflags) $(AM_CFLAGS)
endif # end SOLARIS

# --- DTKA Executables --- #

dtka_SOURCES = tc/dtka/dtka.c
dtka_LDADD = libici.la $(tclib) $(bplib) -lm
dtka_CFLAGS = $(tccflags) $(AM_CFLAGS)

dtkaadmin_SOURCES = tc/dtka/dtkaadmin.c
dtkaadmin_LDADD = libici.la $(tclib) $(bplib) -lm
dtkaadmin_CFLAGS = $(tccflags) $(AM_CFLAGS)

# --- Authority Executables --- #

tcaadmin_SOURCES = tc/authority/tcaadmin.c
tcaadmin_LDADD = libici.la $(tclib) $(bplib) -lm
tcaadmin_CFLAGS = $(tccflags) $(AM_CFLAGS)

tcaboot_SOURCES = tc/authority/tcaboot.c
tcaboot_LDADD = libici.la $(tclib) $(bplib) -lm
tcaboot_CFLAGS = $(tccflags) $(AM_CFLAGS)

tcacompile_SOURCES = tc/authority/tcacompile.c
tcacompile_LDADD = libici.la $(tclib) $(bplib) -lm
tcacompile_CFLAGS = $(tccflags) $(AM_CFLAGS)

tcapublish_SOURCES = tc/authority/tcapublish.c
tcapublish_LDADD = libici.la $(tclib) $(bplib) -lm
tcapublish_CFLAGS = $(tccflags) $(AM_CFLAGS)

tcarecv_SOURCES = tc/authority/tcarecv.c
tcarecv_LDADD = libici.la $(tclib) $(bplib) -lm
tcarecv_CFLAGS = $(tccflags) $(AM_CFLAGS)

# --- Client Executables --- #

tcc_SOURCES = tc/client/tcc.c
tcc_LDADD = libici.la $(tclib) $(bplib) -lm
tcc_CFLAGS = $(tccflags) $(AM_CFLAGS)

tccadmin_SOURCES = tc/client/tccadmin.c
tccadmin_LDADD = libici.la $(tclib) $(bplib) -lm
tccadmin_CFLAGS = $(tccflags) $(AM_CFLAGS)

############################
#
# NETWORK MANAGEMENT SECTION
#                                        
############################

nmbin= nm_agent nm_mgr
nmlib= libamp.la libampAgentADM.la

nmPODM1 = pod2man -s 1 -c "NM executables"
nmPODM3 = pod2man -s 1 -c "NM libraries"

# CBOR Library Sources
nm_cbor_src = \
	nm/contrib/QCBOR/src/UsefulBuf.c \
	nm/contrib/QCBOR/src/qcbor_encode.c \
	nm/contrib/QCBOR/src/qcbor_decode.c \
	nm/contrib/QCBOR/src/ieee754.c

AMP_VERSION ?= 8
#$(warning "AMP_VERSION is ${AMP_VERSION}")
NM_CBOR_CFLAGS = -I $(srcdir)/nm/contrib/QCBOR/inc -DAMP_VERSION=${AMP_VERSION}

# NM Shared library
#  This library is standalone (no dependencies on other ION components)
nm_shared_src = \
	nm/shared/adm/adm.c \
	nm/shared/msg/msg.c \
	nm/shared/primitives/ari.c \
	nm/shared/primitives/blob.c \
	nm/shared/primitives/ctrl.c \
	nm/shared/primitives/edd_var.c \
	nm/shared/primitives/expr.c \
	nm/shared/primitives/report.c \
	nm/shared/primitives/rules.c \
	nm/shared/primitives/table.c \
	nm/shared/primitives/tnv.c \
	nm/shared/utils/cbor_utils.c \
	nm/shared/utils/db.c \
	nm/shared/utils/nm_types.c \
	nm/shared/utils/rhht.c \
	nm/shared/utils/utils.c \
	nm/shared/utils/vector.c \
	$(nm_cbor_src)

libamp_la_SOURCES = $(nm_shared_src)
libamp_la_CFLAGS = -I/usr/include -I$(srcdir)/ici/library -I$(srcdir)/bpv7/library -I$(srcdir)/ltp/library -I$(srcdir)/nm  \
                  -I$(srcdir)/nm/agent $(AM_CFLAGS) $(NO_UNUSED_VARIABLE) \
                  $(NM_CBOR_CFLAGS)
# Note: ici reference is used here only for memory management.  If -DUSE_MALLOC is used for NM (not recommended), this dependency would be unneeded.
libamp_la_LIBADD = libici.la

# Note: AMP Agent ADM must be built directly with the agent, whereas all other ADMs can be modularized as needed.
nm_agent_SOURCES = \
	nm/agent/ingest.c \
	nm/agent/instr.c \
	nm/agent/lcc.c \
	nm/agent/nmagent.c \
	nm/agent/ldc.c \
	nm/agent/rda.c \
	nm/shared/msg/ion_if.c \
	nm/agent/adm_amp_agent_impl.c \
	nm/agent/adm_amp_agent_agent.c \
	nm/shared/adm/adm_init.c

NM_ADM_INCLUDES = -I$(srcdir)/nm/shared/adm -I$(BP_SRC_DIR)/adm -I$(BP_SRC_DIR)

nm_agent_ADM_SOURCES = \
	bpv7/adm/adm_bp_agent_impl.c \
	bpv7/adm/adm_bp_agent_agent.c \
	bpv7/adm/adm_bpsec_impl.c \
	bpv7/adm/adm_bpsec_agent.c \
	bpv7/adm/adm_ion_bp_admin_impl.c \
	bpv7/adm/adm_ion_bp_admin_agent.c \
	nm/agent/adm_ion_admin_agent.c \
	nm/agent/adm_ion_admin_impl.c \
	nm/agent/adm_ionsec_admin_impl.c \
	nm/agent/adm_ionsec_admin_agent.c \
	nm/agent/adm_ion_ipn_admin_impl.c \
	nm/agent/adm_ion_ipn_admin_agent.c \
	nm/agent/adm_ion_ltp_admin_impl.c \
	nm/agent/adm_ion_ltp_admin_agent.c \
	nm/agent/adm_ltp_agent_impl.c \
	nm/agent/adm_ltp_agent_agent.c

nm_mgr_ADM_SOURCES = \
	bpv7/adm/adm_bp_agent_mgr.c \
	bpv7/adm/adm_bpsec_mgr.c \
	bpv7/adm/adm_ion_bp_admin_mgr.c \
	nm/mgr/adm_amp_agent_mgr.c \
	nm/mgr/adm_ion_admin_mgr.c \
	nm/mgr/adm_ionsec_admin_mgr.c \
	nm/mgr/adm_ion_ipn_admin_mgr.c \
	nm/mgr/adm_ion_ltp_admin_mgr.c \
	nm/mgr/adm_ltp_agent_mgr.c

nmextra = \
	nm/doc/pod1/nm_agent.pod \
	nm/doc/pod1/nm_mgr.pod

nmmans = \
	$(top_builddir)/nm/doc/nm_agent.1 \
	$(top_builddir)/nm/doc/nm_mgr.1

libampAgentADM_la_SOURCES = $(nm_agent_ADM_SOURCES)
libampAgentADM_la_CFLAGS = -I/usr/include -I$(srcdir)/nm -I$(srcdir)/nm/shared/adm -I$(BP_SRC_DIR) -I$(BP_SRC_DIR)/library/ext/bpsec -I$(BP_SRC_DIR)/library -I$(srcdir)/ltp/library $(NM_CBOR_CFLAGS) $(AM_CFLAGS) $(NO_UNUSED_VARIABLE) 
libampAgentADM_la_LIBADD = libamp.la libici.la $(bplib) libltp.la


nm_agent_LDADD = libici.la $(bplib) libltp.la libamp.la libampAgentADM.la $(PTHREAD_LIBS)
nm_agent_LDFLAGS = -L/usr/local/lib -lm -lpthread 

nm_agent_CFLAGS = -I/usr/include -I$(srcdir)/ici/library -I$(srcdir)/bpv7/library -I$(srcdir)/nm \
                  -I$(srcdir)/nm/agent -I$(srcdir)/nm/shared/adm $(AM_CFLAGS) $(NO_UNUSED_VARIABLE) \
                  -DAGENT_ROLE $(NM_CBOR_CFLAGS) $(NM_ADM_INCLUDES)


nm_mgr_SOURCES = \
	nm/mgr/agents.c \
	nm/mgr/metadata.c \
	nm/mgr/nm_mgr_print.c \
	nm/mgr/nm_mgr_rx.c \
	nm/mgr/nm_mgr_sql.c \
	nm/mgr/nm_mgr_ui.c \
	nm/mgr/nm_mgr.c \
	nm/mgr/ui_input.c \
	$(nm_mgr_ADM_SOURCES) \
	nm/shared/msg/ion_if.c \
	nm/shared/adm/adm_init.c

# TODO: Split cJSON as a seperate configurable supporting library
CIVETWEB_SOURCES = \
	nm/contrib/civetweb/src/civetweb.c \
	nm/contrib/civetweb/examples/rest/cJSON/cJSON.c \
	nm/contrib/civetweb/examples/rest/cJSON/cJSON_Utils.c \
	nm/mgr/nm_rest.c
CIVETWEB_INCLUDES = -I$(srcdir)/nm/contrib/civetweb/include -I$(srcdir)/nm/contrib/civetweb/examples/rest/cJSON
CIVETWEB_LDFLAGS = -ldl
CIVETWEB_CFLAGS = -DNO_SSL -DUSE_CIVETWEB -DUSE_JSON

nm_mgr_LDADD = libamp.la libici.la $(bplib) libltp.la $(PTHREAD_LIBS)

# Note: NCURSES and MySql support are optional and controlled at configure time with --with-mysql or --with-ncurses
nm_mgr_LDFLAGS = -L/usr/local/mysql/lib -lpthread ${MYSQL_LIBS} ${NCURSES_LIBS}
nm_mgr_CFLAGS = -I/usr/local/mysql/include -I$(srcdir)/ici/library -I$(srcdir)/bpv7/library -I$(srcdir)/ltp/library -I$(srcdir)/nm  -I$(srcdir)/nm/mgr -I/usr/include/mysql $(AM_CFLAGS) $(NO_UNUSED_VARIABLE) $(NM_CBOR_CFLAGS) $(NM_ADM_INCLUDES)

if BUILD_NM_REST
nm_mgr_LDFLAGS += $(CIVETWEB_LDFLAGS)
nm_mgr_CFLAGS += $(CIVETWEB_INCLUDES) $(CIVETWEB_CFLAGS)
nm_mgr_SOURCES += $(CIVETWEB_SOURCES)

endif

$(top_builddir)/nm/doc/%.1: nm/doc/pod1/%.pod
	$(nmPODM1) $< $@

$(top_builddir)/nm/doc/%.3: nm/doc/pod3/%.pod
	$(nmPODM3) $< $@
endif #end BUILD_BPv6

##########################
#
# AMS SECTION
#
##########################

# Define pod2man stuff
amsPODM1 = pod2man -s 1 -c "AMS executables"
amsPODM3 = pod2man -s 3 -c "AMS library functions"
amsPODM5 = pod2man -s 5 -c "AMS configuration files"
#amsPODH = pod2html --noindex

amscflags = -I$(srcdir)/ams/library -I$(srcdir)/ams/include -I$(srcdir)/ams/rams -DUDPTS -DTCPTS -DDGRTS

amsbin = \
	amsbenchr \
	amsbenchs \
	amsd \
	amslog \
	amslogprt \
	amsmib \
	amsshell \
	amsstop \
	ramsgate

if !WINDOWS
amsbin += \
	amshello
endif

amslib = \
	libams.la

amsinclude = \
	ams/include/ams.h

amsnoinst = \
	ams/library/amscommon.h \
	ams/library/amsP.h \
	ams/library/default.crypt.c \
	ams/library/default.marshal.c \
	ams/library/marshal.c \
	ams/rams/rams.h \
	ams/rams/ramscommon.h

amsextra = \
	ams/README.txt \
	ams/library/vmqts.c \
	ams/test/amspubsub.c

amsmans = \
	$(top_builddir)/ams/doc/amsbenchr.1 \
	$(top_builddir)/ams/doc/amsbenchs.1 \
	$(top_builddir)/ams/doc/amsd.1 \
	$(top_builddir)/ams/doc/amshello.1 \
	$(top_builddir)/ams/doc/amslog.1 \
	$(top_builddir)/ams/doc/amslogprt.1 \
	$(top_builddir)/ams/doc/amsmib.1 \
	$(top_builddir)/ams/doc/amspub.1 \
	$(top_builddir)/ams/doc/amsshell.1 \
	$(top_builddir)/ams/doc/amsstop.1 \
	$(top_builddir)/ams/doc/amssub.1 \
	$(top_builddir)/ams/doc/ramsgate.1 \
	$(top_builddir)/ams/doc/ams.3 \
	$(top_builddir)/ams/doc/amsrc.5 \
	$(top_builddir)/ams/doc/amsxml.5 \
	$(top_builddir)/ams/doc/petition_log.5

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
	ams/library/amscommon.c \
	ams/library/crypt.c \
	ams/library/dgrts.c \
	ams/library/libams.c \
	ams/library/loadmib.c \
	ams/library/tcpts.c \
	ams/library/udpts.c
libams_la_LDFLAGS = -static
libams_la_CFLAGS = $(amscflags) $(AM_CFLAGS)
libams_la_LIBADD = $(EXPAT_LIBS) libdgr.la libici.la

# --- Utility Programs --- #

amslog_SOURCES = ams/test/amslog.c
amslog_LDADD = libams.la libdgr.la libici.la -lm 
amslog_CFLAGS = $(amscflags) $(AM_CFLAGS)

amslogprt_SOURCES = ams/test/amslogprt.c
amslogprt_LDADD = libams.la libdgr.la libici.la 
amslogprt_CFLAGS = $(amscflags) $(AM_CFLAGS)

amsmib_SOURCES = ams/utils/amsmib.c
amsmib_LDADD = libams.la libdgr.la libici.la -lm 
amsmib_CFLAGS = $(amscflags) $(AM_CFLAGS)

amsshell_SOURCES = ams/test/amsshell.c
amsshell_LDADD = libams.la libdgr.la libici.la -lm 
amsshell_CFLAGS = $(amscflags) $(AM_CFLAGS)

# --- Test Executables --- #

amsbenchr_SOURCES = ams/test/amsbenchr.c
amsbenchr_LDADD = libams.la libdgr.la libici.la -lm 
amsbenchr_CFLAGS = $(amscflags) $(AM_CFLAGS)

amsbenchs_SOURCES = ams/test/amsbenchs.c
amsbenchs_LDADD = libams.la libdgr.la libici.la -lm 
amsbenchs_CFLAGS = $(amscflags) $(AM_CFLAGS)

amshello_SOURCES = ams/test/amshello.c
amshello_LDADD = libams.la libdgr.la libici.la -lm 
amshello_CFLAGS = $(amscflags) $(AM_CFLAGS)

amsstop_SOURCES =ams/utils/amsstop.c
amsstop_LDADD = libams.la libdgr.la libici.la -lm 
amsstop_CFLAGS = $(amscflags) $(AM_CFLAGS)

# --- Daemon Executables --- #

amsd_SOURCES = \
	ams/library/amscommon.c \
	ams/library/amsd.c \
	ams/library/crypt.c \
	ams/library/dgrts.c \
	ams/library/libams.c \
	ams/library/tcpts.c \
	ams/library/udpts.c
amsd_CFLAGS = $(amscflags) $(AM_CFLAGS)
amsd_LDADD = libams.la libdgr.la libici.la -lm 

# --- RAMS Executables --- #

ramsgate_SOURCES = \
	ams/rams/librams.c \
	ams/rams/ramscommon.c \
	ams/rams/ramsgate.c
ramsgate_LDADD = $(bplib) libams.la libdgr.la libici.la 
ramsgate_CFLAGS = $(amscflags) $(AM_CFLAGS)

##############
#
# CFDP SECTION
#
##############

# Define pod2man stuff
cfdpPODM1 = pod2man -s 1 -c "CFDP executables"
cfdpPODM3 = pod2man -s 3 -c "CFDP library functions"
cfdpPODM5 = pod2man -s 5 -c "CFDP configuration files"

cfdpcflags = -I$(srcdir)/cfdp/library -I$(srcdir)/cfdp/include

cfdpbin = \
	bputa \
	cfdpadmin \
	cfdpclock \
	cfdptest \
	tcputa

if !WINDOWS
cfdpbin +=\
	bpcp \
	bpcpd
endif

cfdplib = \
	libcfdp.la

cfdpinclude = \
	cfdp/include/cfdp.h \
	cfdp/include/cfdpops.h

cfdpnoinst = \
	cfdp/include/bputa.h \
	cfdp/library/cfdpP.h \
	cfdp/utils/bpcp.h

cfdpextra = \
	cfdp/README.txt \
	cfdp/doc/pod1/bpcp.pod \
	cfdp/doc/pod1/bpcpd.pod \
	cfdp/doc/pod1/bputa.pod \
	cfdp/doc/pod1/cfdpadmin.pod \
	cfdp/doc/pod1/cfdpclock.pod \
	cfdp/doc/pod1/cfdptest.pod \
	cfdp/doc/pod3/cfdp.pod \
	cfdp/doc/pod5/cfdprc.pod

cfdpmans = \
	$(top_builddir)/cfdp/doc/bpcp.1 \
	$(top_builddir)/cfdp/doc/bpcpd.1 \
	$(top_builddir)/cfdp/doc/bputa.1 \
	$(top_builddir)/cfdp/doc/cfdpadmin.1 \
	$(top_builddir)/cfdp/doc/cfdpclock.1 \
	$(top_builddir)/cfdp/doc/cfdptest.1 \
	$(top_builddir)/cfdp/doc/cfdp.3 \
	$(top_builddir)/cfdp/doc/cfdprc.5

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

bpcp_SOURCES = cfdp/utils/bpcp.c
bpcp_LDADD = libcfdp.la libici.la -lm $(PTHREAD_LIBS)
bpcp_CFLAGS = $(cfdpcflags) $(AM_CFLAGS)

bpcpd_SOURCES = cfdp/utils/bpcpd.c
bpcpd_LDADD = libcfdp.la libici.la -lm 
bpcpd_CFLAGS = $(cfdpcflags) $(AM_CFLAGS)

cfdpadmin_SOURCES = cfdp/utils/cfdpadmin.c
cfdpadmin_LDADD = libcfdp.la libici.la -lm $(bplib) 
cfdpadmin_CFLAGS = $(cfdpcflags) $(AM_CFLAGS)

# --- BP Programs --- #

bputa_SOURCES = cfdp/bp/bputa.c
bputa_LDADD = libcfdp.la libici.la $(bplib) $(PTHREAD_LIBS)
bputa_CFLAGS = $(cfdpcflags) $(AM_CFLAGS)

# --- Test Programs --- #

cfdptest_SOURCES = cfdp/test/cfdptest.c
cfdptest_LDADD = libcfdp.la libici.la -lm $(bplib) $(PTHREAD_LIBS)
cfdptest_CFLAGS = $(cfdpcflags) $(AM_CFLAGS)

# --- Daemon Executables --- #

cfdpclock_SOURCES = cfdp/daemon/cfdpclock.c
cfdpclock_LDADD = libcfdp.la libici.la $(bplib) 
cfdpclock_CFLAGS = $(cfdpcflags) $(AM_CFLAGS)

# --- TCP Executables --- #

tcputa_SOURCES = cfdp/tcp/tcputa.c
tcputa_LDADD = libcfdp.la libici.la $(bplib) 
tcputa_CFLAGS = $(cfdpcflags) $(AM_CFLAGS)

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
	bsscounter \
	bssdriver \
	bssrecv \
	bssStreamingApp

bsslib = \
	libbss.la

bssinclude = \
	bss/include/bss.h

bssnoinst = \
	bss/library/bssP.h

bssextra = \
	bss/README.txt \
	bss/doc/pod1/bssrecv.pod \
	bss/doc/pod1/bssStreamingApp.pod \
	bss/doc/pod3/bss.pod

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
libbss_la_LIBADD = libici.la $(bplib)

# --- Test Programs --- #

bsscounter_SOURCES = bss/test/bsscounter.c
bsscounter_LDADD = libbss.la libici.la $(bplib) 
bsscounter_CFLAGS = $(bsscflags) $(AM_CFLAGS)

bssdriver_SOURCES = bss/test/bssdriver.c
bssdriver_LDADD = libbss.la libici.la $(bplib) 
bssdriver_CFLAGS = $(bsscflags) $(AM_CFLAGS)

bssrecv_SOURCES = bss/test/bssrecv.c
bssrecv_LDADD = libbss.la libici.la $(bplib) 
bssrecv_CFLAGS = $(bsscflags) $(AM_CFLAGS)

bssStreamingApp_SOURCES = bss/test/bssStreamingApp.c
bssStreamingApp_LDADD = $(bplib) libbss.la libici.la 
bssStreamingApp_CFLAGS = $(bsscflags) $(AM_CFLAGS)

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
	bsspadmin \
	bsspclock \
	tcpbsi \
	tcpbso \
	udpbsi \
	udpbso

bssplib = \
	libbssp.la \
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

libbssp_la_SOURCES = \
	bssp/library/libbssp.c \
	bssp/library/libbsspP.c
libbssp_la_CFLAGS = $(bsspcflags) $(AM_CFLAGS)
libbssp_la_LDFLAGS = $(ION_LINK_FLAGS)
libbssp_la_LIBADD = libici.la -lm $(PTHREAD_LIBS)

libtcpbsa_la_SOURCES = bssp/tcp/libtcpbsa.c
libtcpbsa_la_CFLAGS = $(bsspcflags) $(AM_CFLAGS)
libtcpbsa_la_LDFLAGS = $(ION_LINK_FLAGS)
libtcpbsa_la_LIBADD = libici.la -lm $(PTHREAD_LIBS)

# --- Daemon Executables --- #

bsspclock_SOURCES = bssp/daemon/bsspclock.c
bsspclock_LDADD = libbssp.la libici.la -lm $(bplib) 
bsspclock_CFLAGS = $(bsspcflags) $(AM_CFLAGS)

# --- Utility Programs --- #

bsspadmin_SOURCES = bssp/utils/bsspadmin.c
bsspadmin_LDADD = libbssp.la libici.la -lm $(bplib) 
bsspadmin_CFLAGS = $(bsspcflags) $(AM_CFLAGS)

# --- UDP Executables --- #

udpbsi_SOURCES = bssp/udp/udpbsi.c
udpbsi_LDADD = libbssp.la libici.la -lm $(bplib) $(PTHREAD_LIBS)
udpbsi_CFLAGS = $(bsspcflags) $(AM_CFLAGS)

udpbso_SOURCES = bssp/udp/udpbso.c
udpbso_LDADD = libbssp.la libici.la -lm $(bplib) $(PTHREAD_LIBS)
udpbso_CFLAGS = $(bsspcflags) $(AM_CFLAGS)

# --- TCP Executables --- #

tcpbsi_SOURCES = bssp/tcp/tcpbsi.c
tcpbsi_LDADD = libbssp.la libici.la -lm $(bplib) $(PTHREAD_LIBS)
tcpbsi_CFLAGS = $(bsspcflags) $(AM_CFLAGS)

tcpbso_SOURCES = bssp/tcp/tcpbso.c
tcpbso_LDADD = libbssp.la libtcpbsa.la libici.la -lm $(bplib) $(PTHREAD_LIBS)
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

dtpcbin = \
	dtpcadmin \
	dtpcclock \
	dtpcd \
	dtpcreceive \
	dtpcsend

dtpclib = \
	libdtpc.la

dtpcinclude = \
	dtpc/include/dtpc.h

dtpcnoinst = \
	dtpc/library/dtpcP.h

dtpcextra = \
	dtpc/doc/pod1/dtpcadmin.pod \
	dtpc/doc/pod1/dtpcclock.pod \
	dtpc/doc/pod1/dtpcd.pod \
	dtpc/doc/pod1/dtpcreceive.pod \
	dtpc/doc/pod1/dtpcsend.pod \
	dtpc/doc/pod3/dtpc.pod \
	dtpc/doc/pod5/dtpcrc.pod

dtpcmans = \
	$(top_builddir)/dtpc/doc/dtpcadmin.1 \
	$(top_builddir)/dtpc/doc/dtpcclock.1 \
	$(top_builddir)/dtpc/doc/dtpcd.1 \
	$(top_builddir)/dtpc/doc/dtpcreceive.1 \
	$(top_builddir)/dtpc/doc/dtpcsend.1 \
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
libdtpc_la_LIBADD = libici.la $(bplib) $(PTHREAD_LIBS)

# --- Utility Programs --- #

dtpcadmin_SOURCES = dtpc/utils/dtpcadmin.c
dtpcadmin_LDADD = libdtpc.la libici.la $(bplib) 
dtpcadmin_CFLAGS = $(dtpccflags) $(AM_CFLAGS)

# --- Test Programs --- #

dtpcreceive_SOURCES = dtpc/test/dtpcreceive.c
dtpcreceive_LDADD = libdtpc.la libici.la $(bplib) 
dtpcreceive_CFLAGS = $(dtpccflags) $(AM_CFLAGS)

dtpcsend_SOURCES = dtpc/test/dtpcsend.c
dtpcsend_LDADD = libdtpc.la libici.la $(bplib) 
dtpcsend_CFLAGS = $(dtpccflags) $(AM_CFLAGS)

# --- Daemon Executables --- #

dtpcclock_SOURCES = dtpc/daemon/dtpcclock.c
dtpcclock_LDADD = libdtpc.la libici.la $(bplib) 
dtpcclock_CFLAGS = $(dtpccflags) $(AM_CFLAGS)

dtpcd_SOURCES = dtpc/daemon/dtpcd.c
dtpcd_LDADD = libdtpc.la libici.la $(bplib) 
dtpcd_CFLAGS = $(dtpccflags) $(AM_CFLAGS)


##########################
#
# RESTART SECTION
#
##########################

restartbin = ionrestart ionexit

# --- Utility Programs --- #

ionexit_SOURCES = restart/utils/ionexit.c
ionexit_LDADD = libltp.la libici.la -lm $(bplib) 
ionexit_CFLAGS = $(icicflags) $(AM_CFLAGS)

ionrestart_SOURCES = restart/utils/ionrestart.c
ionrestart_LDADD = libici.la $(bplib) libltp.la libcfdp.la 
ionrestart_CFLAGS = -I$(srcdir)/ici/sdr -I$(srcdir)/ici/library -I$(srcdir)/$(BP_DIR)/library -I$(srcdir)/ltp/library -I$(srcdir)/ici/library -I$(srcdir)/cfdp/library $(AM_CFLAGS)

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
# executable inside the tests/ directory. This allows ION to have both
# compiled tests and tests written in shell script.
#test: buildcheck
#	cd tests && ./runtests

# Runs the tests listed in "normaltests"
test: buildcheck
	cd $(srcdir)/tests && ./runtestset normaltests

# Uses the runtest script that simply runs "dotest" including compiled tests.
# Could also use ./runtestset alltests, they should be equivalent.
test-all: buildcheck
	cd $(srcdir)/tests && ./runtests

# Builds a testset file with the same name as a mercurial branch.
# This is intended as a per-branch unit test.
test-branch: buildcheck
	@echo
	@echo "You need mercurial (hg) installed for this."
	@echo
	cd $(srcdir)/tests && hg branch | xargs -L1 ./runtestset

# Catch-all for tests, basically any string after "make test-" will run a
# testset file with the same name as that string.
test-%: buildcheck
	cd $(srcdir)/tests && ./runtestset $*

# Runtest will make a file called "retest" containing all "failed" tests.
# The file will not exist at first (resulting in an error).
# When there are no failed tests, retest will exist (no error), but contains
# an empty test list (resulting in a test passage).
# This ensures that "make restest" will only pass when some testing has
# already been performed with total success.
retest: buildcheck
	cd $(srcdir)/tests && ./runtestset retest

# "make check" is the same as "make test"
check: test

# This library contains utility code that's helpful for tests in C.
TESTUTILOBJS = tests/library/libtestutil.la
TESTUTILCFLAGS = -I$(srcdir)/tests/library
check_LTLIBRARIES = $(TESTUTILOBJS)
tests_library_libtestutil_la_SOURCES = \
	tests/library/check.c \
	tests/library/paths.c \
	tests/library/ionstart.c \
	tests/library/ionstop.c
tests_library_libtestutil_la_CFLAGS = $(bpcflags) $(AM_CFLAGS)

# C TAP Harness Library. Provides enhanced functions for writing unit tests and outputting TAP-conformant output
# Note: TAP = Test anything Protocol
# Note: This library includes a harness to run tests similar to runtests, but which is ot being used at this time.
# TESTTAPOBJS = tests/library/libtesttap.la
# TEST_TAP_DIR = contrib/c-tap-harness/tests/tap
# TESTTAPCFLAGS = -I$(srcdir)/contrib/c-tap-harness
# check_LTLIBRARIES = $(TESTTAPOBJS)
# tests_library_libtesttap_la_SOURCES = \
# 	$(TEST_TAP_DIR)/basic.c \
# 	$(TEST_TAP_DIR)/float.c
# tests_library_libtesttap_la_CFLAGS = $(TESTTAPCFLAGS) $(AM_CFLAGS)

# Unity Test Framework (http://throwtheswitch.org/unity) Utility
TESTUNITYOBJS = tests/library/libtestunity.la
TEST_UNITY_DIR = contrib/c-unity-harness/tests/unity
TESTUNITYCFLAGS = -I$(srcdir)/contrib/Unity/src
check_LTLIBRARIES += $(TESTUNITYOBJS)
tests_library_libtestunity_la_SOURCES = contrib/Unity/src/unity.c
tests_library_libtestunity_la_CFLAGS = $(TESTUNITYCFLAGS) $(AM_CFLAGS)

# TEST AUTHORS: README
# If a test does not require compilation, it should not be listed here. Just make a
#  script named "dotest" that is executable.
# If a test does require compilation, it should be listed here and have
#  the required _SOURCES, _LDADD, and _CFLAGS entries.
# If a test uses the testutil helpers, include $(TESTUTILOBJS) in _LDADD,
#  and $(TESTUTILCFLAGS) in _CFLAGS.
check_PROGRAMS = tests/1000.loopback/dotest \
	tests/1500.loopback-brs/dotest \
	tests/issue-260-teach-valgrind-mtake/domtake \
	tests/issue-188-common-cos-syntax/dotest \
	tests/bug-0015-tcpclo-bpcp-sig-handling/test \
	tests/issue-330-cfdpclock-FDU-removal/cfdplisten \
	tests/issue-334-cfdp-transaction-id/dotest \
	tests/nm-unit/dotest \
	tests/nm-unit/utils/vector/dotest \
	tests/nm-unit/utils/rhht/dotest
#	tests/nm-unit/primitives/ari/dotest

if BUILD_BPv6
check_PROGRAMS += \
	tests/issue-279-bpMemo-timeline/driver
endif #end BUILD_BPv6

# Note: Built without shared nm libraries to allow compilation with USE_MALLOC flag.
tests_nm_unit_dotest_SOURCES = tests/nm-unit/dotest.c $(nm_agent_SOURCES) $(nm_shared_src) $(nm_agent_ADM_SOURCES)
#tests_nm_unit_dotest_LDADD = $(nm_agent_LDADD) -lm $(TESTUTILOBJS) $(TESTUNITYOBJS)
tests_nm_unit_dotest_LDADD = libici.la $(bplib) libltp.la -lm $(TESTUTILOBJS) $(TESTUNITYOBJS)
tests_nm_unit_dotest_CFLAGS = $(nm_agent_CFLAGS) $(TESTUTILCFLAGS) $(TESTUNITYCFLAGS) -DTEST_MAIN -DUSE_MALLOC \
	-I/usr/include -I$(srcdir)/nm -I$(srcdir)/$(BP_DIR)/library/ext/sbsp -I$(BP_SRC_DIR) -I$(srcdir)/$(BP_DIR)/library/ext/bpsec -I$(srcdir)/$(BP_DIR)/library -I$(srcdir)/ltp/library -I$(srcdir)/nm/shared

tests_nm_unit_utils_vector_dotest_SOURCES = tests/nm-unit/utils/vector.c $(nm_shared_src)
tests_nm_unit_utils_vector_dotest_LDADD = -lm $(TESTUNITYOBJS) libici.la
tests_nm_unit_utils_vector_dotest_CFLAGS = $(libamp_la_CFLAGS) $(TESTUNITYCFLAGS) -DUSE_MALLOC

tests_nm_unit_utils_rhht_dotest_SOURCES = tests/nm-unit/utils/rhht.c $(nm_shared_src)
tests_nm_unit_utils_rhht_dotest_LDADD = -lm $(TESTUNITYOBJS) libici.la
tests_nm_unit_utils_rhht_dotest_CFLAGS = $(libamp_la_CFLAGS) $(TESTUNITYCFLAGS) -DUSE_MALLOC -I$(srcdir)/nm/shared

#tests_nm_unit_primitives_ari_dotest_SOURCES = tests/nm-unit/primitives/ari.c $(nm_agent_SOURCES)
#tests_nm_unit_primitives_ari_dotest_LDADD = $(nm_agent_LDADD) -lm $(TESTUTILOBJS) $(TESTUNITYOBJS)
#tests_nm_unit_primitives_ari_dotest_CFLAGS = $(nm_agent_CFLAGS) $(TESTUTILCFLAGS) $(TESTUNITYCFLAGS)


tests_1000_loopback_dotest_SOURCES = tests/1000.loopback/dotest.c
tests_1000_loopback_dotest_LDADD = libici.la -lm $(bplib) $(ltplib) $(TESTUTILOBJS)
tests_1000_loopback_dotest_CFLAGS = $(bpcflags) $(AM_CFLAGS) $(TESTUTILCFLAGS)

#Disabled this test in ION 3.6.0 because tcpcl does not support loopback
#tests_1300_loopback_tcp_dotest_SOURCES = tests/1300.loopback-tcp/dotest.c
#tests_1300_loopback_tcp_dotest_LDADD = libici.la -lm $(bplib) $(ltplib) $(TESTUTILOBJS)
#tests_1300_loopback_tcp_dotest_CFLAGS = $(bpcflags) $(AM_CFLAGS) $(TESTUTILCFLAGS)

# Disabled this test in ION 3.2.2 due to crashes. Restore after fixing crashes.
# tests_1400_loopback_stcp_dotest_SOURCES = tests/1400.loopback-stcp/dotest.c
# tests_1400_loopback_stcp_dotest_LDADD = $(bplib) libici.la $(ltplib) $(TESTUTILOBJS)
# tests_1400_loopback_stcp_dotest_CFLAGS = $(bpcflags) $(AM_CFLAGS) $(TESTUTILCFLAGS)

tests_1500_loopback_brs_dotest_SOURCES = tests/1500.loopback-brs/dotest.c
tests_1500_loopback_brs_dotest_LDADD = libici.la -lm $(bplib) $(ltplib) $(TESTUTILOBJS)
tests_1500_loopback_brs_dotest_CFLAGS = $(bpcflags) $(AM_CFLAGS) $(TESTUTILCFLAGS)

#Disabled this test in ION 3.7.0 due to routing changes
tests_issue_188_common_cos_syntax_dotest_SOURCES = tests/issue-188-common-cos-syntax/dotest.c
tests_issue_188_common_cos_syntax_dotest_LDADD = libici.la -lm $(bplib) $(ltplib) $(TESTUTILOBJS)
tests_issue_188_common_cos_syntax_dotest_CFLAGS = $(bpcflags) $(AM_CFLAGS) $(TESTUTILCFLAGS)

tests_issue_260_teach_valgrind_mtake_domtake_SOURCES = tests/issue-260-teach-valgrind-mtake/domtake.c
tests_issue_260_teach_valgrind_mtake_domtake_LDADD = libici.la -lm $(TESTUTILOBJS)
tests_issue_260_teach_valgrind_mtake_domtake_CFLAGS = $(icicflags) $(AM_CFLAGS) $(TESTUTILCFLAGS)

# issue_279_bpMemo_timeline_driver is bpv6 only
# bpMemo function does not exist for bpv7
if BUILD_BPv6
tests_issue_279_bpMemo_timeline_driver_SOURCES = tests/issue-279-bpMemo-timeline/driver.c
tests_issue_279_bpMemo_timeline_driver_LDADD = libici.la -lm $(bplib) $(TESTUTILOBJS)
tests_issue_279_bpMemo_timeline_driver_CFLAGS = $(bpcflags) -I$(BP_DIR)/library/ $(icicflags) $(AM_CFLAGS) $(TESTUTILCFLAGS)
endif # end BUILD_BPv6

tests_bug_0015_tcpclo_bpcp_sig_handling_test_SOURCES=tests/bug-0015-tcpclo-bpcp-sig-handling/test.c
tests_bug_0015_tcpclo_bpcp_sig_handling_test_LDADD= libici.la -lm $(bplib) 
tests_bug_0015_tcpclo_bpcp_sig_handling_test_CFLAGS=$(bpcflags) $(icicflags) $(AM_CFLAGS)

tests_issue_330_cfdpclock_FDU_removal_cfdplisten_SOURCES = tests/issue-330-cfdpclock-FDU-removal/cfdplisten.c
tests_issue_330_cfdpclock_FDU_removal_cfdplisten_LDADD = libcfdp.la libici.la -lm $(bplib) 
tests_issue_330_cfdpclock_FDU_removal_cfdplisten_CFLAGS = $(cfdpcflags) $(AM_CFLAGS)

tests_issue_334_cfdp_transaction_id_dotest_SOURCES = tests/issue-334-cfdp-transaction-id/dotest.c
tests_issue_334_cfdp_transaction_id_dotest_LDADD = libcfdp.la libici.la -lm $(TESTUTILOBJS)
tests_issue_334_cfdp_transaction_id_dotest_CFLAGS = $(icicflags) $(cfdpcflags) $(AM_CFLAGS) $(TESTUTILCFLAGS)


##########################
#
# AUTODOC SECTION
#
##########################

if ENABLE_AUTODOC

ION.pdf: doc/ION-beginning.pdf doc/ION-manpages.ps
	$(PS2PDF) doc/ION-manpages.ps
	gs -dBATCH -dNOPAUSE -q -sDEVICE=pdfwrite -sOutputFile=ION.pdf doc/ION-beginning.pdf ION-manpages.pdf

# The targets in man_MANS that are actually manpages (and not phony targets
# for making directories, etc.)
justmans := $(filter %.1 %.3 %.5,$(man_MANS))

mans_as_ps := $(patsubst %,%.ps,$(justmans))
mans_1_as_ps := $(sort $(filter %.1.ps,$(mans_as_ps)))
mans_3_as_ps := $(sort $(filter %.3.ps,$(mans_as_ps)))
mans_5_as_ps := $(sort $(filter %.5.ps,$(mans_as_ps)))
mans_as_ps_sorted := $(mans_1_as_ps) $(mans_3_as_ps) $(mans_5_as_ps)


%.1.ps: %.1
	groff -mandoc $^ > $@

%.3.ps: %.3
	groff -mandoc $^ > $@

%.5.ps: %.5
	groff -mandoc $^ > $@

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
#	@echo "ION.pdf will remain unchanged."
	@echo "ION.pdf will not have appendix."
	cp $^ $@

endif #ENABLE_AUTODOC

##########################
#
# DISTRIBUTION SECTION
#
##########################

DOC_VERSION_NUMBER := $(shell cat $(srcdir)/$(BP_DIR)version.txt)
DOC_VERSION_MAJOR := $(shell cat $(srcdir)/$(BP_DIR)version.txt | sed 's/\..*//')
DOC_VERSION_MINOR := $(shell cat $(srcdir)/$(BP_DIR)version.txt | sed 's/[0-9]*\.//')
DOC_DATE := $(shell date '+%Y-%m-%d')

distribution: doc/ION-beginning.pdf doc/ION-manpages.ps
	@echo ""
	@echo "UPDATING VERSION NUMBER IN:"
	@echo "configure.ac"
	sed -r -i.bak 's/ion, open source [0-9]+\.[0-9]+\.[0-9]+[a-Z]*/ion, open source $(DOC_VERSION_NUMBER)/' configure.ac
	@echo "arch-uClib/ion.package/ion.mk"
	sed -r -i.bak 's/ION_VERSION_MAJOR=[0-9]+/ION_VERSION_MAJOR=$(DOC_VERSION_MAJOR)/' ./arch-uClibc/ion.package/ion.mk
	sed -r -i.bak 's/ION_VERSION_MINOR=[0-9]+\.[0-9]+[a-Z]*+/ION_VERSION_MINOR=$(DOC_VERSION_MINOR)/' ./arch-uClibc/ion.package/ion.mk
	@echo ""
	@echo "Archiving the directory to ~/releases/ion-$(DOC_VERSION_NUMBER)."
	@echo ""
	hg archive ~/releases/ion-$(DOC_VERSION_NUMBER)
	@echo "Now creating ION.pdf"
	$(PS2PDF) doc/ION-manpages.ps
	gs -dBATCH -dNOPAUSE -q -sDEVICE=pdfwrite -sOutputFile=ION.pdf doc/ION-beginning.pdf ION-manpages.pdf
	rm ION-manpages.pdf
