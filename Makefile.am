# Makefile.am
# Samuel Jero
# June 14, 2013
# Adapted from work by David Young

ACLOCAL_AMFLAGS = -I m4

SUBDIRS= .
DIST_SUBDIRS= .

AM_CFLAGS += \
	-include config.h \
	-I$(srcdir)/ici/include \
	-I$(srcdir)/ltp/include \
	-I$(srcdir)/$(BP_DIR)/include \
	-I$(srcdir)/bss/include \
	-I$(srcdir)/ams/include \
	-I$(srcdir)/cfdp/include

if ENABLE_DGR
AM_CFLAGS += -I$(srcdir)/dgr/include
endif

if ENABLE_TC
AM_CFLAGS += -I$(srcdir)/tc/include
endif

if ENABLE_BSSP
AM_CFLAGS += -I$(srcdir)/bssp/include
endif

if ENABLE_DTPC
AM_CFLAGS += -I$(srcdir)/dtpc/include
endif

if SOLARIS_STUDIO_CC
AM_CFLAGS += \
	-errtags \
	-Kpic
NO_UNUSED_VARIABLE =
else
AM_CFLAGS += -Wall \
	-fPIC -Wextra \
	-Wstrict-prototypes \
	-pedantic \
	-Wshadow \
	-Wpointer-arith \
	-Wcast-qual \
	-Wcast-align

if !USING_GCC_ASAN
AM_CFLAGS += -Werror
endif

NO_UNUSED_VARIABLE = -Wno-unused-variable
endif # end SOLARIS_STUDIO_CC


# This will add CFLAGS to make the valgrind macros compile without
# warnings/errors on some systems with specific versions of valgrind
# and gcc (see configure.ac and issue #298 for details). If valgrind
# support is not present, this variable is empty.
AM_CFLAGS += $(VALGRIND_COMPAT_CFLAGS)

# ION pkg-config support
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = ion.pc

# EXTRA_DIST gets sent in the tarball, but isn't really a program
# anything NOT compiled: scripts, readmes, tar files, config files.
# An exception of something compiled which must be explicitly included is
# man page source files.
EXTRA_DIST =	\
	$(srcdir)/ion.pc.in \
	$(srcdir)/sysctl_script.sh \
	$(srcdir)/README.md \
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
	$(nmextra) \
	$(restartextra)
#	$(twonodestcp_DATA) \
#	$(threenodestcpltp_DATA)

# This hook now explicitly depends on the completion of all programs and
# libraries. It checks for a flag file to ensure it only runs ONCE per build.

if ENABLE_BSL
# BSL v1.1 builds as a standalone library (no ION dependencies).
# Build BSL first, then build ION linking against BSL.
all: all-bsl

all-bsl:
	@echo "========================================"
	@echo "Stage 1: Building BSL..."
	@echo "========================================"
	@$(srcdir)/build-bsl-for-ion.sh
	@echo "========================================"
	@echo "Stage 2: Building ION..."
	@echo "========================================"
	$(MAKE) $(AM_MAKEFLAGS) all-am

all-local: $(bin_PROGRAMS) $(lib_LTLIBRARIES) $(man_MANS)
else
all-local: $(bin_PROGRAMS) $(lib_LTLIBRARIES) $(man_MANS)
endif # end ENABLE_BSL
if ENABLE_INSTALL_MESSAGES
	@if test -f configure && [ ! -f .build_finished ]; then \
		echo ""; \
		echo '    ┌───────────────────────────────────────────────────────────┐'; \
		echo '    │                                                           │'; \
		echo '    │   BUILD COMPLETE.                                         │'; \
		echo '    │                                                           │'; \
		echo '    │   Next, install ION-DTN by running:                       │'; \
		echo '    │                                                           │'; \
		echo '    │   [sudo] make install                                     │'; \
		echo '    │                                                           │'; \
		echo '    └───────────────────────────────────────────────────────────┘'; \
		echo ""; \
		touch .build_finished; \
	fi
endif

if ENABLE_BSL
install-exec-hook:
	@echo "Installing BSL shared libraries to $(DESTDIR)$(libdir)..."
	@for d in $(BSL_LIB_DIR64) $(BSL_LIB_DIR32); do \
		test -d "$$d" || continue; \
		cp -pPR "$$d"/libbsl_*.so* $(DESTDIR)$(libdir)/; \
		cp -pPR "$$d"/libqcbor.so* $(DESTDIR)$(libdir)/; \
		break; \
	done
endif

# This hook contains the final message after a successful installation.
install-data-hook:
if ENABLE_INSTALL_MESSAGES
	@echo '    ───────────────────────────────────────────────────────────'
	@echo
	@echo '    ██╗ ██████╗ ███╗   ██╗         ██████╗ ████████╗███╗   ██╗'
	@echo '    ██║██╔═══██╗████╗  ██║         ██╔══██╗╚══██╔══╝████╗  ██║'
	@echo '    ██║██║   ██║██╔██╗ ██║ ██████╗ ██║  ██║   ██║   ██╔██╗ ██║'
	@echo '    ██║██║   ██║██║╚██╗██║ ╚═════╝ ██║  ██║   ██║   ██║╚██╗██║'
	@echo '    ██║╚██████╔╝██║ ╚████║         ██████╔╝   ██║   ██║ ╚████║'
	@echo '    ╚═╝ ╚═════╝ ╚═╝  ╚═══╝         ╚═════╝    ╚═╝   ╚═╝  ╚═══╝'
	@echo
	@echo '    INSTALLATION SUCCESSFUL.'
	@echo
	@echo '    [ Linux ]'
	@echo '    To register the new ION libraries, run:'
	@echo '    sudo ldconfig'
	@echo
	@echo '    [ macOS ]'
	@echo '    If you have runtime loading issues, run:'
	@echo '    sudo update_dyld_shared_cache -force'
	@echo
	@echo '    To fix mbedtls library paths specifically, run:'
	@echo '    fix-macos-mbedtls-lib-links.sh'
	@echo
	@echo '    ───────────────────────────────────────────────────────────'
	@echo
endif

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

# remove podman.mk, etc rules only if distclean is run (implies autoreconf -fi will be run next)
distclean-local:
	rm -f $(top_builddir)/ici/doc/podman.mk
	rm -f $(top_builddir)/ltp/doc/podman.mk
	rm -f $(top_builddir)/dgr/doc/podman.mk
	rm -f $(top_builddir)/nm/doc/podman.mk
	rm -f $(top_builddir)/bpv7/doc/podman.mk
	rm -f $(top_builddir)/tc/doc/podman.mk
	rm -f $(top_builddir)/ams/doc/podman.mk
	rm -f $(top_builddir)/cfdp/doc/podman.mk
	rm -f $(top_builddir)/bss/doc/podman.mk
	rm -f $(top_builddir)/bssp/doc/podman.mk
	rm -f $(top_builddir)/dtpc/doc/podman.mk
	rm -f $(top_builddir)/restart/doc/podman.mk
	rm -f $(top_builddir)/tests/testrules.mk
	rm -f $(top_builddir)/pstrules.mk
	rm -f $(top_builddir)/man-lists.mk


# clean-local refers to existing documents.
CLEAN_LOCAL_DEPS = iciclean-local ltpclean-local bpclean-local amsclean-local \
	cfdpclean-local restartclean-local bssclean-local nmclean-local

if ENABLE_BSL
bsl-clean-local:
	@echo "Cleaning BSL build artifacts..."
	@cd $(srcdir)/external/BSL && ./build.sh clean || true
CLEAN_LOCAL_DEPS += bsl-clean-local
endif

if ENABLE_DGR
dgrclean-local:
	rm -f $(top_builddir)/dgr/doc/*.1
	rm -f $(top_builddir)/dgr/doc/*.3
CLEAN_LOCAL_DEPS += dgrclean-local
endif

if ENABLE_TC
tcclean-local:
	rm -f $(top_builddir)/tc/doc/*.1
	rm -f $(top_builddir)/tc/doc/*.3
	rm -f $(top_builddir)/tc/doc/*.5
CLEAN_LOCAL_DEPS += tcclean-local
endif

if ENABLE_BSSP
bsspclean-local:
	rm -f $(top_builddir)/bssp/doc/*.1
	rm -f $(top_builddir)/bssp/doc/*.3
	rm -f $(top_builddir)/bssp/doc/*.5
CLEAN_LOCAL_DEPS += bsspclean-local
endif

if ENABLE_DTPC
dtpcclean-local:
	rm -f $(top_builddir)/dtpc/doc/*.1
	rm -f $(top_builddir)/dtpc/doc/*.3
	rm -f $(top_builddir)/dtpc/doc/*.5
CLEAN_LOCAL_DEPS += dtpcclean-local
endif

clean-local: $(CLEAN_LOCAL_DEPS)
	@rm -f .build_finished
	@rm -f ion.pc

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
	mkdir -p $(top_builddir)/restart/doc


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
	$(nmlib) \
	$(secryptlib) \
	$(metadatalib)

# Linenoise: lightweight readline replacement (BSD-2-Clause)
# Only built when --enable-commandline-history is used
if ENABLE_CMD_HIST
noinst_LTLIBRARIES = liblinenoise.la
liblinenoise_la_SOURCES = ici/linenoise/linenoise.c
liblinenoise_la_CFLAGS = $(AM_CFLAGS)
LINENOISE_LDADD = liblinenoise.la
LINENOISE_CFLAGS = -I$(srcdir)/ici/linenoise
else
LINENOISE_LDADD =
LINENOISE_CFLAGS =
endif

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
	$(nmmans) \
	$(restartmans)
endif # end ENABLE_MANPAGES

#################
#
# SCRIPTS SECTION
#
#################

# throw the helper scripts in the /usr/local/bin
# whatever directory with the rest of the programs.
stuffdir = $(bindir)
stuff_SCRIPTS = $(srcdir)/ionstart \
		$(srcdir)/ionrun \
		$(srcdir)/ionstop \
		$(srcdir)/ionscript \
		$(srcdir)/demos/ionlauncher/ionlauncher \
		$(srcdir)/demos/ionlauncher/net_model_gen \
		$(srcdir)/killm \
		$(srcdir)/ion-diagnostics \
		$(srcdir)/ionprocesses.txt
#comment stuff_SCRIPTS += $(srcdir)/killm if you want to build killm.exe


otherstuffdir = $(pkgdatadir)
otherstuff_DATA = \
	$(srcdir)/ionstart.awk

#################
#
# CONFIGS SECTION
#
#################

# example configuration items
# loopback section: remember that you need an XXXdir and XXX_DATA for each
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
	ionwatch \
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
	reversibility_correctness_test \
	robust_lock_recovery_test \
	psm_robust_recovery_test \
	ipc_lock_recovery_test \
	cbor_oob_test \
	sdrcancel \
	sdr_frag_stress \
	sdrmend \
	sdrwatch \
	sm2file \
	smlistsh \
	smrbtsh

icibin += \
	ionxnowner \
	owlttb

icilib = \
	libici.la

iciinclude = \
	ici/include/crypto.h \
	ici/include/icinm.h \
	ici/include/ion.h \
	ici/include/ion_admin.h \
	ici/include/ion_atomic.h \
	ici/include/ionsec.h \
	ici/include/llcv.h \
	ici/include/lyst.h \
	ici/include/memmgr.h \
	ici/include/platform.h \
	ici/include/platform_sm.h \
	ici/include/psm.h \
	ici/include/radix.h \
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
	ici/include/zco.h \
	ici/include/cbor.h \
	ici/include/crc.h \
	ici/include/bulk.h \
	ici/include/csi.h \
	ici/include/ion_network.h

icinoinst = \
	ici/library/lystP.h \
	ici/library/radixP.h \
	ici/sdr/sdrP.h \
	ici/test/file2sdr.h

iciextra = \
	ici/README.txt \
	ici/doc/pod1/ionwatch.pod \
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
	ici/doc/pod3/poll_entropy_src.pod \
	ici/doc/pod3/platform.pod \
	ici/doc/pod3/memmgr.pod \
	ici/doc/pod3/ion.pod \
	ici/doc/pod3/ion_admin.pod \
	ici/doc/pod3/llcv.pod \
	ici/doc/pod3/lyst.pod \
	ici/doc/pod3/psm.pod \
	ici/doc/pod3/zco.pod \
	ici/doc/pod3/secrypt.pod \
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
	ici/doc/ionwatch.1 \
	ici/doc/file2sdr.1 \
	ici/doc/file2sm.1 \
	ici/doc/ionadmin.1 \
	ici/doc/ionlog.1 \
	ici/doc/ionsecadmin.1 \
	ici/doc/ionunlock.1 \
	ici/doc/ionxnowner.1 \
	ici/doc/owltsim.1 \
	ici/doc/owlttb.1 \
	ici/doc/psmshell.1 \
	ici/doc/psmwatch.1 \
	ici/doc/rfxclock.1 \
	ici/doc/sdr2file.1 \
	ici/doc/sdrmend.1 \
	ici/doc/sdrwatch.1 \
	ici/doc/sm2file.1 \
	ici/doc/smlistsh.1 \
	ici/doc/smrbtsh.1 \
	ici/doc/ion.3 \
	ici/doc/ion_admin.3 \
	ici/doc/llcv.3 \
	ici/doc/lyst.3 \
	ici/doc/memmgr.3 \
	ici/doc/platform.3 \
	ici/doc/poll_entropy_src.3 \
	ici/doc/psm.3 \
	ici/doc/secrypt.3 \
	ici/doc/sdr.3 \
	ici/doc/sdrhash.3 \
	ici/doc/sdrlist.3 \
	ici/doc/sdrstring.3 \
	ici/doc/sdrtable.3 \
	ici/doc/smlist.3 \
	ici/doc/smrbt.3 \
	ici/doc/zco.3 \
	ici/doc/ionconfig.5 \
	ici/doc/ionrc.5 \
	ici/doc/ionsecrc.5
if BUILD_BPv7
icimans += \
	ici/doc/ioncbor.3 \
	ici/doc/ioncrc.3
endif # end BUILD_BPv7

iciclean-local:
	rm -f $(top_builddir)/ici/doc/*.1
	rm -f $(top_builddir)/ici/doc/*.3
	rm -f $(top_builddir)/ici/doc/*.5

# -- Libraries --- #

libici_la_SOURCES =	\
	ici/bulk/STUB_BULK/bulk.c \
	ici/library/cbor.c \
	ici/library/crc.c \
	ici/library/ion.c \
	ici/library/ion_admin.c \
	ici/library/ion_network.c \
	ici/library/ionsec.c \
	ici/library/libicinm.c \
	ici/library/llcv.c \
	ici/library/lyst.c \
	ici/library/memmgr.c \
	ici/library/platform.c \
	ici/library/platform_sm.c \
	ici/library/psm.c \
	ici/library/radix.c \
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
	ici/sdr/sdrxn.c \
	ici/sdr/sdr_perf.c \
	ici/crypto/entropy_src.c

# Always compile ion_atomic.c.  Its internal #if !ION_HAVE_C11_ATOMICS guard
# compiles the function bodies to nothing on C11 builds, so this is safe.
# The file must be unconditionally present so that CFLAGS-based tier overrides
# (ION_TEST_FORCE_FALLBACK) can switch to the mutex-backed fallback at compile
# time without a link failure.
libici_la_SOURCES += ici/library/ion_atomic.c

libici_la_CFLAGS = $(icicflags) $(AM_CFLAGS)
libici_la_LIBADD = $(CRYPTO_LIBS) -lm

# --- Utility Programs --- #

ionadmin_SOURCES = ici/utils/ionadmin.c
ionadmin_LDADD = libici.la -lm $(LINENOISE_LDADD)
ionadmin_CFLAGS = $(icicflags) $(AM_CFLAGS) $(LINENOISE_CFLAGS)

ionlog_SOURCES = ici/utils/ionlog.c
ionlog_LDADD = libici.la -lm
ionlog_CFLAGS = $(icicflags) $(AM_CFLAGS)

ionsecadmin_SOURCES = ici/utils/ionsecadmin.c
ionsecadmin_LDADD = libici.la -lm $(LINENOISE_LDADD)
ionsecadmin_CFLAGS = $(icicflags) $(AM_CFLAGS) $(LINENOISE_CFLAGS)

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

if ENABLE_BSSP
    ionwatchbssplib = libbssp.la
else
    ionwatchbssplib =
endif

if ENABLE_DTPC
    ionwatchdtpclib = libdtpc.la
else
    ionwatchdtpclib =
endif

ionwatch_SOURCES = ici/utils/ionwatch.c
ionwatch_LDADD = libici.la libltp.la $(bplib) libcfdp.la $(ionwatchdtpclib) $(ionwatchbssplib) -lm
ionwatch_CFLAGS = $(icicflags) $(AM_CFLAGS) -I$(srcdir)/ltp/library -I$(srcdir)/$(BP_DIR)/library -I$(srcdir)/cfdp/library -I$(srcdir)/dtpc/library -I$(srcdir)/bssp/library

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

reversibility_correctness_test_SOURCES = ici/test/reversibility_correctness_test.c
reversibility_correctness_test_LDADD = libici.la -lm
reversibility_correctness_test_CFLAGS = $(icicflags) $(AM_CFLAGS)

robust_lock_recovery_test_SOURCES = ici/test/robust_lock_recovery_test.c
robust_lock_recovery_test_LDADD = libici.la -lm
robust_lock_recovery_test_CFLAGS = $(icicflags) $(AM_CFLAGS)

psm_robust_recovery_test_SOURCES = ici/test/psm_robust_recovery_test.c
psm_robust_recovery_test_LDADD = libici.la -lm
psm_robust_recovery_test_CFLAGS = $(icicflags) $(AM_CFLAGS)

ipc_lock_recovery_test_SOURCES = ici/test/ipc_lock_recovery_test.c
ipc_lock_recovery_test_LDADD = libici.la -lm
ipc_lock_recovery_test_CFLAGS = $(icicflags) $(AM_CFLAGS)

cbor_oob_test_SOURCES = ici/test/cbor_oob_test.c
cbor_oob_test_LDADD = libici.la -lm
cbor_oob_test_CFLAGS = $(icicflags) $(AM_CFLAGS)

sdrcancel_SOURCES = ici/test/sdrcancel.c
sdrcancel_LDADD = libici.la -lm
sdrcancel_CFLAGS = $(icicflags) $(AM_CFLAGS)

sdr_frag_stress_SOURCES = ici/test/sdr_frag_stress.c
sdr_frag_stress_LDADD = libici.la -lm
sdr_frag_stress_CFLAGS = $(icicflags) $(AM_CFLAGS)

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
	ltpadmin \
	ltpcancel \
	ltpclock \
	ltpcounter \
	ltpdeliv \
	ltpdriver \
	ltpmeter \
	ltpsecadmin \
	ltpstats \
	ltpwatch \
	sdatest \
	udplsi \
	udplso \
	xlsi \
	xlso

ltplib = \
	libltp.la \
	libudplsa.la \
	libxlsa.la

ltpinclude = \
	ltp/include/ltp.h \
	ltp/include/ltp_admin.h \
	ltp/include/sda.h

ltpnoinst = \
	ltp/library/ltpP.h \
	ltp/library/ltpsec.h \
	ltp/udp/udplsa.h \
	ltp/xlsa/xlsa.h

ltpextra = \
	ltp/README.txt \
	ltp/doc/pod1/ltpadmin.pod \
	ltp/doc/pod1/ltpcancel.pod \
	ltp/doc/pod1/ltpclock.pod \
	ltp/doc/pod1/ltpcounter.pod \
	ltp/doc/pod1/ltpdriver.pod \
	ltp/doc/pod1/ltpmeter.pod \
	ltp/doc/pod1/ltpsecadmin.pod \
	ltp/doc/pod1/ltpstats.pod \
	ltp/doc/pod1/ltpwatch.pod \
	ltp/doc/pod1/udplsi.pod \
	ltp/doc/pod1/udplso.pod \
	ltp/doc/pod1/xlsi.pod \
	ltp/doc/pod1/xlso.pod \
	ltp/doc/pod3/ltp.pod \
	ltp/doc/pod3/ltp_admin.pod \
	ltp/doc/pod5/ltprc.pod

ltpmans = \
	ltp/doc/ltpadmin.1 \
	ltp/doc/ltpcancel.1 \
	ltp/doc/ltpclock.1 \
	ltp/doc/ltpcounter.1 \
	ltp/doc/ltpdriver.1 \
	ltp/doc/ltpmeter.1 \
	ltp/doc/ltpsecadmin.1 \
	ltp/doc/ltpstats.1 \
	ltp/doc/ltpwatch.1 \
	ltp/doc/udplsi.1 \
	ltp/doc/udplso.1 \
	ltp/doc/xlsi.1 \
	ltp/doc/xlso.1 \
	ltp/doc/ltp.3 \
	ltp/doc/ltp_admin.3 \
	ltp/doc/ltprc.5

ltpclean-local:
	rm -f $(top_builddir)/ltp/doc/*.1
	rm -f $(top_builddir)/ltp/doc/*.3
	rm -f $(top_builddir)/ltp/doc/*.5

# -- Libraries --- #

libltp_la_SOURCES = \
	ltp/library/libltp.c \
	ltp/library/libltpnm.c \
	ltp/library/libltpP.c \
	ltp/library/ltpei.c \
	ltp/library/ltpsec.c \
	ltp/sda/libsda.c
libltp_la_CPPFLAGS = -I$(srcdir)/ici/library
libltp_la_CFLAGS = $(ltpcflags) $(AM_CFLAGS)
libltp_la_LIBADD = libici.la -lm

libudplsa_la_SOURCES = ltp/udp/libudplsa.c
libudplsa_la_CFLAGS = $(ltpcflags) $(AM_CFLAGS) $(PTHREAD_LIBS)
libudplsa_la_LIBADD = libici.la libltp.la -lm

libxlsa_la_SOURCES = ltp/xlsa/libxlsa.c ltp/xlsa/xport_@XLSA_BACKEND@.c
libxlsa_la_CFLAGS = $(ltpcflags) -I$(srcdir)/ltp/xlsa $(AM_CFLAGS) $(PTHREAD_LIBS)
libxlsa_la_LIBADD = libici.la libltp.la $(XLSA_LIBS) -lm

# --- Utility Programs --- #

ltpadmin_SOURCES = ltp/utils/ltpadmin.c
ltpadmin_LDADD = libltp.la libici.la $(LINENOISE_LDADD)
ltpadmin_CFLAGS = $(ltpcflags) $(AM_CFLAGS) $(LINENOISE_CFLAGS)

ltpsecadmin_SOURCES = ltp/utils/ltpsecadmin.c
ltpsecadmin_LDADD = libltp.la libici.la
ltpsecadmin_CFLAGS = $(ltpcflags) $(AM_CFLAGS)

ltpstats_SOURCES = ltp/utils/ltpstats.c
ltpstats_LDADD = libltp.la libici.la
ltpstats_CFLAGS = $(ltpcflags) $(AM_CFLAGS)

ltpcancel_SOURCES = ltp/utils/ltpcancel.c
ltpcancel_LDADD = libici.la
ltpcancel_CFLAGS = $(AM_CFLAGS)

ltpwatch_SOURCES = ltp/utils/ltpwatch.c
ltpwatch_LDADD = libltp.la libici.la
ltpwatch_CFLAGS = $(ltpcflags) $(AM_CFLAGS)

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

xlsi_SOURCES = ltp/xlsa/xlsi.c
xlsi_LDADD = libltp.la libxlsa.la libici.la $(PTHREAD_LIBS)
xlsi_CFLAGS = $(ltpcflags) -I$(srcdir)/ltp/xlsa $(AM_CFLAGS)

xlso_SOURCES = ltp/xlsa/xlso.c
xlso_LDADD = libltp.la libxlsa.la libici.la $(PTHREAD_LIBS)
xlso_CFLAGS = $(ltpcflags) -I$(srcdir)/ltp/xlsa $(AM_CFLAGS)

# --- AOS Executables --- #

#aoslsi_SOURCES = ltp/aos/aoslsi.c
#aoslsi_LDADD = libltp.la libici.la $(PTHREAD_LIBS)
#aoslsi_CFLAGS = $(ltpcflags) $(AM_CFLAGS)

#aoslso_SOURCES = ltp/aos/aoslso.c
#aoslso_LDADD = libltp.la libici.la
#aoslso_CFLAGS = $(ltpcflags) $(AM_CFLAGS)

##########################
#
# DGR SECTION
#
##########################

if ENABLE_DGR

# Define pod2man stuff
dgrPODM1 = pod2man -s 1 -c "DGR executables"
dgrPODM3 = pod2man -s 3 -c "DGR library functions"

dgrcflags = -I$(srcdir)/dgr/library -I$(srcdir)/dgr/test

dgrbin = \
	dgr2file \
	file2dgr \
	file2tcp \
	tcp2file \
	udp2file \
	file2udp

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
	dgr/doc/pod1/dgr2file.pod \
	dgr/doc/pod1/file2dgr.pod \
	dgr/doc/pod3/dgr.pod

dgrmans = \
	dgr/doc/dgr2file.1 \
	dgr/doc/file2dgr.1 \
	dgr/doc/dgr.3

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

endif # ENABLE_DGR

##########################
#
# AMP VERSION
#
##########################

# Use outside conditional to avoid GNUism
# Default is 8 (both BPV6 and BPv7)
AMP_VERSION = @AMP_VERSION@

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

bpcflags = \
	-I$(BP_SRC_DIR)/bpsec/instr \
	-I$(BP_SRC_DIR)/bpsec/policy \
	-I$(BP_SRC_DIR)/bpsec/sci \
	-I$(BP_SRC_DIR)/bpsec/utils \
	-I$(BP_SRC_DIR)/library \
	-I$(BP_SRC_DIR)/library/ext \
	-I$(BP_SRC_DIR)/library/ext/meb \
	-I$(BP_SRC_DIR)/library/ext/bae \
	-I$(BP_SRC_DIR)/include \
	-I$(BP_SRC_DIR)/ipn \
	-I$(BP_SRC_DIR)/ipnd \
	-I$(srcdir)/ici/libbloom-master \
	-I$(srcdir)/ici/libbloom-master/murmur2 \
	-I$(BP_SRC_DIR)/dtn2 \
	-I$(BP_SRC_DIR)/library/crypto \
	-I$(BP_SRC_DIR)/tcp \
	-I$(BP_SRC_DIR)/udp \
	-I$(srcdir)/ici/include \
	-I$(srcdir)/ici/library \
	-I$(BP_SRC_DIR)/library/ext/snw \
	-I$(BP_SRC_DIR)/bibe \
	-I$(BP_SRC_DIR)/imc \
	-DBP_EXTENDED \
	-I$(BP_SRC_DIR)/saga \
	-I$(BP_SRC_DIR)/library/ext/bpsec \
	-I$(BP_SRC_DIR)/library/ext/hcb \
	-I$(BP_SRC_DIR)/library/ext/pnb \
	-I$(BP_SRC_DIR)/library/ext/bpq \
	-I$(BP_SRC_DIR)/library/ext/creb \
	-I$(BP_SRC_DIR)/library/ext/cteb \
	-I$(srcdir)/bpv7/metadata \
	-I$(srcdir)/ici/crypto/NULL_SUITES

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
	bpcrash_hard \
	bpdriver \
	bpecho \
	bping \
	bpinspect \
	bplist \
	bpnmtest \
	bprecvfile \
	bprecvmeta \
	bprecvtest \
	bpsendfile \
	bpsendmeta \
	bpsendtest \
	bpsink \
	bpsource \
	cbrcustodytest \
	bpstats \
	bpwatch \
	bptrace \
	bptracker \
	bptransit \
	brsccla \
	brsscla \
	cgrfetch \
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
	recvfile \
	sendfile \
	sppcli \
	sppclo \
	eppcli \
	eppclo \
	filecli \
	fileclo \
	stcpcli \
	stcpclo \
	tcpcli \
	tcpclo \
	udpcli \
	udpclo \
	bpchat \
	bpstats2

if ENABLE_IMC
bpbin += \
	cpsd \
	imcadminep \
	imcfw
endif # end ENABLE_IMC

if ENABLE_DGR
bpbin += \
	dgrcli \
	dgrclo
endif # end ENABLE_DGR

if ENABLE_BSSP
bpbin += \
	bsspcli \
	bsspclo
endif # end ENABLE_BSSP

bplib = \
	libbp.la \
	libcgr.la \
	libsppcla.la \
	libeppcla.la \
	libstcpcla.la \
	libudpcla.la

bpinclude = \
	bpv7/include/bp.h \
	bpv7/include/bp_admin.h \
	bpv7/include/eureka.h

bpnoinst = \
	bpv7/brs/brscla.h \
	bpv7/dtn2/dtn2fw.h \
	bpv7/ipn/ipnfw.h \
	bpv7/ipn/cbdedup.h \
	bpv7/ipnd/ipndP.h \
	bpv7/library/bpP.h \
	bpv7/ltp/ltpcla.h \
	bpv7/saga/saga.h \
	bpv7/spp/sppcla.h \
	bpv7/epp/eppcla.h \
	bpv7/stcp/stcpcla.h \
	bpv7/udp/udpcla.h

bpextra = \
	bpv7/README.txt \
	bpv7/bibe/bibe.h \
	bpv7/bpsec/policy/bpsec_policy.h \
	bpv7/bpsec/policy/bpsec_policy_eventset.h \
	bpv7/bpsec/policy/bpsec_policy_event.h \
	bpv7/bpsec/policy/bpsec_policy_rule.h \
	bpv7/doc/pod1/bibeadmin.pod \
	bpv7/doc/pod1/bibeclo.pod \
	bpv7/doc/pod1/bpadmin.pod \
	bpv7/doc/pod1/bpcancel.pod \
	bpv7/doc/pod1/bpchat.pod \
	bpv7/doc/pod1/bpclm.pod \
	bpv7/doc/pod1/bpclock.pod \
	bpv7/doc/pod1/bpcounter.pod \
	bpv7/doc/pod1/cbrcustodytest.pod \
	bpv7/doc/pod1/bpdriver.pod \
	bpv7/doc/pod1/bpecho.pod \
	bpv7/doc/pod1/bping.pod \
	bpv7/doc/pod1/bpinspect.pod \
	bpv7/doc/pod1/bplist.pod \
	bpv7/doc/pod1/bpnmtest.pod \
	bpv7/doc/pod1/bprecvfile.pod \
	bpv7/doc/pod1/bprecvtest.pod \
	bpv7/doc/pod1/bpsecadmin.pod \
	bpv7/doc/pod1/bpsendfile.pod \
	bpv7/doc/pod1/bprecvmeta.pod \
	bpv7/doc/pod1/bpsendmeta.pod \
	bpv7/doc/pod1/bpsendtest.pod \
	bpv7/doc/pod1/bpsink.pod \
	bpv7/doc/pod1/bpsource.pod \
	bpv7/doc/pod1/bpstats.pod \
	bpv7/doc/pod1/bpstats2.pod \
	bpv7/doc/pod1/bptrace.pod \
	bpv7/doc/pod1/bptracker.pod \
	bpv7/doc/pod1/bptransit.pod \
	bpv7/doc/pod1/bpcrash.pod \
	bpv7/doc/pod1/bpcrash_hard.pod \
	bpv7/doc/pod1/brsccla.pod \
	bpv7/doc/pod1/brsscla.pod \
	bpv7/doc/pod1/cgrfetch.pod \
	bpv7/doc/pod1/dgrcli.pod \
	bpv7/doc/pod1/dgrclo.pod \
	bpv7/doc/pod1/dtn2admin.pod \
	bpv7/doc/pod1/dtn2adminep.pod \
	bpv7/doc/pod1/dtn2fw.pod \
	bpv7/doc/pod1/eppcli.pod \
	bpv7/doc/pod1/eppclo.pod \
	bpv7/doc/pod1/filecli.pod \
	bpv7/doc/pod1/fileclo.pod \
	bpv7/doc/pod1/hmackeys.pod \
	bpv7/doc/pod1/ipnadmin.pod \
	bpv7/doc/pod1/ipnadminep.pod \
	bpv7/doc/pod1/ipnd.pod \
	bpv7/doc/pod1/ipnfw.pod \
	bpv7/doc/pod1/lgagent.pod \
	bpv7/doc/pod1/lgsend.pod \
	bpv7/doc/pod1/ltpcli.pod \
	bpv7/doc/pod1/ltpclo.pod \
	bpv7/doc/pod1/recvfile.pod \
	bpv7/doc/pod1/sendfile.pod \
	bpv7/doc/pod1/sppcli.pod \
	bpv7/doc/pod1/sppclo.pod \
	bpv7/doc/pod1/stcpcli.pod \
	bpv7/doc/pod1/stcpclo.pod \
	bpv7/doc/pod1/tcpcli.pod \
	bpv7/doc/pod1/tcpclo.pod \
	bpv7/doc/pod1/udpcli.pod \
	bpv7/doc/pod1/udpclo.pod \
	bpv7/doc/pod3/bp.pod \
	bpv7/doc/pod3/bp_admin.pod \
	bpv7/doc/pod3/bpextensions.pod \
	bpv7/doc/pod3/cbr.pod \
	bpv7/doc/pod3/metadata.pod \
	bpv7/doc/pod5/biberc.pod \
	bpv7/doc/pod5/bprc.pod \
	bpv7/doc/pod5/bpsecrc.pod \
	bpv7/doc/pod5/dtn2rc.pod \
	bpv7/doc/pod5/ipnrc.pod \
	bpv7/doc/pod5/lgfile.pod \
	bpv7/library/ext/bpextensions.c \
	bpv7/library/ext/noextensions.c \
	bpv7/library/ext/bae/bae.h \
	bpv7/library/ext/bpsec/bcb.h \
	bpv7/library/ext/bpsec/bib.h \
	bpv7/library/ext/creb/creb.h \
	bpv7/library/ext/cteb/cteb.h \
	bpv7/library/ext/hcb/hcb.h \
	bpv7/library/ext/meb/meb.h

if ENABLE_IMC
bpextra += \
	bpv7/doc/pod1/cpsd.pod \
	bpv7/doc/pod1/imcadminep.pod \
	bpv7/doc/pod1/imcfw.pod
endif # end ENABLE_IMC

bpmans = \
	bpv7/doc/bibeadmin.1 \
	bpv7/doc/bibeclo.1 \
	bpv7/doc/bpadmin.1 \
	bpv7/doc/bpcancel.1 \
	bpv7/doc/bpchat.1 \
	bpv7/doc/bpclm.1 \
	bpv7/doc/bpclock.1 \
	bpv7/doc/bpcounter.1 \
	bpv7/doc/cbrcustodytest.1 \
	bpv7/doc/bpdriver.1 \
	bpv7/doc/bpecho.1 \
	bpv7/doc/bping.1 \
	bpv7/doc/bpinspect.1 \
	bpv7/doc/bplist.1 \
	bpv7/doc/bpnmtest.1 \
	bpv7/doc/bprecvfile.1 \
	bpv7/doc/bprecvtest.1 \
	bpv7/doc/bpsecadmin.1 \
	bpv7/doc/bpsendfile.1 \
	bpv7/doc/bprecvmeta.1 \
	bpv7/doc/bpsendmeta.1 \
	bpv7/doc/bpsendtest.1 \
	bpv7/doc/bpsink.1 \
	bpv7/doc/bpsource.1 \
	bpv7/doc/bpstats.1 \
	bpv7/doc/bpstats2.1 \
	bpv7/doc/bptrace.1 \
	bpv7/doc/bptracker.1 \
	bpv7/doc/bptransit.1 \
	bpv7/doc/bpcrash.1 \
	bpv7/doc/bpcrash_hard.1 \
	bpv7/doc/brsccla.1 \
	bpv7/doc/brsscla.1 \
	bpv7/doc/cgrfetch.1 \
	bpv7/doc/dgrcli.1 \
	bpv7/doc/dgrclo.1 \
	bpv7/doc/dtn2admin.1 \
	bpv7/doc/dtn2adminep.1 \
	bpv7/doc/dtn2fw.1 \
	bpv7/doc/eppcli.1 \
	bpv7/doc/eppclo.1 \
	bpv7/doc/filecli.1 \
	bpv7/doc/fileclo.1 \
	bpv7/doc/hmackeys.1 \
	bpv7/doc/ipnadmin.1 \
	bpv7/doc/ipnadminep.1 \
	bpv7/doc/ipnd.1 \
	bpv7/doc/ipnfw.1 \
	bpv7/doc/lgagent.1 \
	bpv7/doc/lgsend.1 \
	bpv7/doc/ltpcli.1 \
	bpv7/doc/ltpclo.1 \
	bpv7/doc/recvfile.1 \
	bpv7/doc/sendfile.1 \
	bpv7/doc/sppcli.1 \
	bpv7/doc/sppclo.1 \
	bpv7/doc/stcpcli.1 \
	bpv7/doc/stcpclo.1 \
	bpv7/doc/tcpcli.1 \
	bpv7/doc/tcpclo.1 \
	bpv7/doc/udpcli.1 \
	bpv7/doc/udpclo.1 \
	bpv7/doc/bp.3 \
	bpv7/doc/bp_admin.3 \
	bpv7/doc/bpextensions.3 \
	bpv7/doc/cbr.3 \
	bpv7/doc/metadata.3 \
	bpv7/doc/biberc.5 \
	bpv7/doc/bprc.5 \
	bpv7/doc/bpsecrc.5 \
	bpv7/doc/dtn2rc.5 \
	bpv7/doc/ipnrc.5 \
	bpv7/doc/lgfile.5

if ENABLE_IMC
bpmans += \
	bpv7/doc/cpsd.1 \
	bpv7/doc/imcadminep.1 \
	bpv7/doc/imcfw.1
endif # end ENABLE_IMC


bpclean-local:
	rm -f $(top_builddir)/bpv7/doc/*.1
	rm -f $(top_builddir)/bpv7/doc/*.3
	rm -f $(top_builddir)/bpv7/doc/*.5

# -- Libraries --- #

libbp_la_SOURCES = \
	bpv7/bpsec/instr/bpsec_instr.c \
	bpv7/bpsec/instr/bpsec_instr.h \
	bpv7/bpsec/sci/bcb_aes_gcm_sc.c \
	bpv7/bpsec/sci/bcb_aes_gcm_sc.h \
	bpv7/bpsec/sci/bib_hmac_sha2_sc.c \
	bpv7/bpsec/sci/bib_hmac_sha2_sc.h \
	bpv7/bpsec/sci/ion_test_sc.c \
	bpv7/bpsec/sci/ion_test_sc.h \
	bpv7/bpsec/sci/rfc9173_utils.c \
	bpv7/bpsec/sci/rfc9173_utils.h \
	bpv7/bpsec/sci/sc_util.c \
	bpv7/bpsec/sci/sc_value.c \
	bpv7/bpsec/sci/sci.c \
	bpv7/bpsec/sci/sci.h \
	bpv7/bpsec/sci/sci_valmap.c \
	bpv7/bpsec/utils/bpsec_asb.c \
	bpv7/bpsec/utils/bpsec_asb.h \
	bpv7/bpsec/utils/bpsec_util.c \
	bpv7/bpsec/utils/bpsec_util.h \
	bpv7/bibe/bibe.c \
	bpv7/dtn2/libdtn2fw.c \
	bpv7/library/libbp.c \
	bpv7/library/libbpP.c \
	bpv7/library/libbpnm.c \
	bpv7/library/eureka.c \
	bpv7/library/bei.c \
	bpv7/library/cbr.c \
	bpv7/library/ext/bae/bae.c \
	bpv7/library/ext/bpq/bpq.c \
	bpv7/library/ext/hcb/hcb.c \
	bpv7/library/ext/meb/meb.c \
	bpv7/library/ext/creb/creb.c \
	bpv7/library/ext/cteb/cteb.c \
	bpv7/library/ext/pnb/pnb.c \
	bpv7/library/ext/pnb/pnb.h \
	bpv7/library/ext/snw/snw.c \
	bpv7/ipn/libipnfw.c \
	bpv7/saga/saga.c \
	bpv7/utils/jsmn.c \
	ici/libbloom-master/bloom.h \
	ici/libbloom-master/bloom.c \
	ici/libbloom-master/murmur2/MurmurHash2.c

# Native BPSec sources - only when BSL is NOT enabled
if !ENABLE_BSL
libbp_la_SOURCES += \
	bpv7/bpsec/policy/bpsec_policy.c \
	bpv7/bpsec/policy/bpsec_policy_eventset.c \
	bpv7/bpsec/policy/bpsec_policy_event.c \
	bpv7/bpsec/policy/bpsec_policy_rule.c \
	bpv7/library/ext/bpsec/bcb.c \
	bpv7/library/ext/bpsec/bib.c \
	bpv7/utils/bpsecadmin_config.c
endif


if CRYPTO_MBEDTLS
AM_CFLAGS+= \
	-I$(srcdir)/ici/crypto/MBEDTLS_SUITES \
	-I$(srcdir)/ici/crypto

libici_la_SOURCES += \
	ici/crypto/MBEDTLS_SUITES/crypto.c \
	ici/crypto/MBEDTLS_SUITES/csi.c \
	ici/crypto/MBEDTLS_SUITES/csi_ecdsa.c \
	ici/crypto/MBEDTLS_SUITES/csi_gcm.c \
	ici/crypto/MBEDTLS_SUITES/csi_hsha.c
else
AM_CFLAGS+= \
	-I$(srcdir)/ici/crypto/NULL_SUITES \
	-I$(srcdir)/ici/crypto

libici_la_SOURCES+= \
	ici/crypto/NULL_SUITES/crypto.c\
	ici/crypto/NULL_SUITES/csi.c
endif # end CRYPTO_MBEDTLS

libbp_la_CPPFLAGS = -I$(srcdir)/ici/library
libbp_la_CFLAGS = $(bpcflags) $(AM_CFLAGS) -I$(BP_SRC_DIR)/library/ext -I$(BP_SRC_DIR)
libbp_la_LIBADD = libici.la -lm $(CRYPTO_LIBS)

if ENABLE_IMC
libbp_la_SOURCES += \
	bpv7/imc/libimcfw.c \
	bpv7/library/ext/imc/imc.h \
	bpv7/library/ext/imc/imc.c
endif # end ENABLE_IMC

# BSL Integration - add BSL wrapper when enabled
if ENABLE_BSL
libbp_la_SOURCES += \
	bpv7/bsl/bsl.c \
	bpv7/bsl/ionpatch.c

libbp_la_LIBADD += $(BSL_LIBS)
libbp_la_LDFLAGS = $(AM_LDFLAGS) -Wl,-rpath,$(BSL_LIB_DIR64) -Wl,-rpath,$(BSL_LIB_DIR32)
endif # end ENABLE_BSL


libcgr_la_SOURCES = bpv7/cgr/libcgr.c

libcgr_la_CFLAGS = $(bpcflags) $(AM_CFLAGS)
libcgr_la_LIBADD = libbp.la libici.la

libstcpcla_la_SOURCES = bpv7/stcp/libstcpcla.c
libstcpcla_la_CFLAGS = $(bpcflags) $(AM_CFLAGS)
libstcpcla_la_LIBADD = libbp.la libici.la

libudpcla_la_SOURCES = bpv7/udp/libudpcla.c
libudpcla_la_CFLAGS = $(bpcflags) $(AM_CFLAGS)
libudpcla_la_LIBADD = libbp.la libici.la

libsppcla_la_SOURCES = bpv7/spp/libsppcla.c
libsppcla_la_CFLAGS = $(bpcflags) $(AM_CFLAGS)
libsppcla_la_LIBADD = libbp.la libici.la

libeppcla_la_SOURCES = bpv7/epp/libeppcla.c
libeppcla_la_CFLAGS = $(bpcflags) $(AM_CFLAGS)
libeppcla_la_LIBADD = libbp.la libici.la

# --- libmetadata --- #

metadatalib = libmetadata.la
libmetadata_la_SOURCES = bpv7/metadata/metadata.c
libmetadata_la_CFLAGS = $(bpcflags) $(AM_CFLAGS)
libmetadata_la_LIBADD = libbp.la libici.la

# --- libsecrypt CSI --- #

secryptlib = libsecrypt.la

if CRYPTO_MBEDTLS
    libsecrypt_la_SOURCES = ici/crypto/MBEDTLS_SUITES/secrypt.c
else
    libsecrypt_la_SOURCES = ici/crypto/NULL_SUITES/secrypt.c
endif

libsecrypt_la_CFLAGS = $(bpcflags) $(AM_CFLAGS)
libsecrypt_la_LIBADD = libbp.la libici.la

# --- Utility Programs --- #

bpadmin_SOURCES = bpv7/utils/bpadmin.c
bpadmin_LDADD = libici.la $(bplib) $(LINENOISE_LDADD)
bpadmin_CFLAGS = $(bpcflags) $(AM_CFLAGS) $(LINENOISE_CFLAGS)

bpcancel_SOURCES = bpv7/utils/bpcancel.c
bpcancel_LDADD = libici.la $(bplib)
bpcancel_CFLAGS = $(bpcflags) $(AM_CFLAGS)

bplist_SOURCES = bpv7/utils/bplist.c
bplist_LDADD = libici.la $(bplib)
bplist_CFLAGS = $(bpcflags) $(AM_CFLAGS)

bpinspect_SOURCES = bpv7/utils/bpinspect.c bpv7/utils/bpinspect_data.c bpv7/utils/bpinspect_filter.c bpv7/utils/bpinspect_ops.c
bpinspect_LDADD = libici.la $(bplib)
bpinspect_CFLAGS = $(bpcflags) $(AM_CFLAGS)

if !ENABLE_BSL
bin_PROGRAMS += bpsecadmin
bpsecadmin_SOURCES = bpv7/utils/bpsecadmin.c
bpsecadmin_LDADD = libici.la $(bplib) $(LINENOISE_LDADD)
bpsecadmin_CFLAGS = $(bpcflags) $(AM_CFLAGS) $(LINENOISE_CFLAGS)
endif

bpstats_SOURCES = bpv7/utils/bpstats.c
bpstats_LDADD = libici.la $(bplib)
bpstats_CFLAGS = $(bpcflags) $(AM_CFLAGS)

bpwatch_SOURCES = bpv7/utils/bpwatch.c
bpwatch_LDADD = libici.la $(bplib)
bpwatch_CFLAGS = $(bpcflags) $(AM_CFLAGS)

bptrace_SOURCES = bpv7/utils/bptrace.c
bptrace_LDADD = libici.la $(bplib)
bptrace_CFLAGS = $(bpcflags) $(AM_CFLAGS)

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
ipnadmin_LDADD = libici.la $(bplib) $(LINENOISE_LDADD)
ipnadmin_CFLAGS = $(bpcflags) $(AM_CFLAGS) $(LINENOISE_CFLAGS)

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

ipnfw_SOURCES = bpv7/ipn/ipnfw.c bpv7/ipn/cbdedup.c
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

if ENABLE_BSSP
bsspcli_SOURCES = bpv7/bssp/bsspcli.c
bsspcli_LDADD = libbssp.la libici.la $(bplib) $(PTHREAD_LIBS)
bsspcli_CFLAGS = $(bpcflags) $(AM_CFLAGS)

bsspclo_SOURCES = bpv7/bssp/bsspclo.c
bsspclo_LDADD = libbssp.la libici.la $(bplib)
bsspclo_CFLAGS = $(bpcflags) $(AM_CFLAGS)
endif # end ENABLE_BSSP

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

# --- SPP Executables --- #

sppcli_SOURCES = bpv7/spp/sppcli.c
sppcli_LDADD = libici.la $(bplib) $(PTHREAD_LIBS) $(LIBS)
sppcli_CFLAGS = $(bpcflags) $(AM_CFLAGS)

sppclo_SOURCES = bpv7/spp/sppclo.c
sppclo_LDADD = libici.la $(bplib) $(LIBS)
sppclo_CFLAGS = $(bpcflags) $(AM_CFLAGS)

# --- EPP Executables --- #

eppcli_SOURCES = bpv7/epp/eppcli.c
eppcli_LDADD = libici.la $(bplib) $(PTHREAD_LIBS) $(LIBS)
eppcli_CFLAGS = $(bpcflags) $(AM_CFLAGS)

eppclo_SOURCES = bpv7/epp/eppclo.c
eppclo_LDADD = libici.la $(bplib) $(LIBS)
eppclo_CFLAGS = $(bpcflags) $(AM_CFLAGS)

# --- File CLA Executables --- #

filecli_SOURCES = bpv7/file/filecli.c bpv7/file/libfilecla.c
filecli_LDADD = libici.la $(bplib) $(PTHREAD_LIBS) $(LIBS)
filecli_CFLAGS = $(bpcflags) $(AM_CFLAGS)

fileclo_SOURCES = bpv7/file/fileclo.c bpv7/file/libfilecla.c
fileclo_LDADD = libici.la $(bplib) $(LIBS)
fileclo_CFLAGS = $(bpcflags) $(AM_CFLAGS)

# --- BIBE Executables --- #

bibeadmin_SOURCES = bpv7/bibe/bibeadmin.c
bibeadmin_LDADD = libici.la $(bplib) $(LINENOISE_LDADD)
bibeadmin_CFLAGS = $(bpcflags) $(AM_CFLAGS) $(LINENOISE_CFLAGS)

bibeclo_SOURCES = bpv7/bibe/bibeclo.c
bibeclo_LDADD = libici.la $(bplib)
bibeclo_CFLAGS = $(bpcflags) $(AM_CFLAGS)

# --- DGR Executables --- #

if ENABLE_DGR
dgrcli_SOURCES = bpv7/dgr/dgrcli.c
dgrcli_LDADD = libdgr.la libici.la $(bplib) $(PTHREAD_LIBS)
dgrcli_CFLAGS = $(bpcflags) $(AM_CFLAGS)

dgrclo_SOURCES = bpv7/dgr/dgrclo.c
dgrclo_LDADD = libdgr.la libici.la $(bplib) $(PTHREAD_LIBS)
dgrclo_CFLAGS = $(bpcflags) $(AM_CFLAGS)
endif # end ENABLE_DGR

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

bpcrash_hard_SOURCES = bpv7/test/bpcrash_hard.c
bpcrash_hard_LDADD = libici.la $(bplib)
bpcrash_hard_CFLAGS = $(bpcflags) $(AM_CFLAGS)

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

bprecvtest_SOURCES = bpv7/utils/bprecvtest.c
bprecvtest_LDADD = libici.la $(bplib)
bprecvtest_CFLAGS = $(bpcflags) $(AM_CFLAGS)

bpsendfile_SOURCES = bpv7/utils/bpsendfile.c
bpsendfile_LDADD = libici.la $(bplib)
bpsendfile_CFLAGS = $(bpcflags) $(AM_CFLAGS)

bprecvmeta_SOURCES = bpv7/utils/bprecvmeta.c
bprecvmeta_LDADD = libici.la $(bplib)
bprecvmeta_CFLAGS = $(bpcflags) $(AM_CFLAGS)

bpsendmeta_SOURCES = bpv7/utils/bpsendmeta.c
bpsendmeta_LDADD = libici.la $(bplib)
bpsendmeta_CFLAGS = $(bpcflags) $(AM_CFLAGS)

bpsendtest_SOURCES = bpv7/utils/bpsendtest.c
bpsendtest_LDADD = libici.la $(bplib)
bpsendtest_CFLAGS = $(bpcflags) $(AM_CFLAGS)

bpsink_SOURCES = bpv7/test/bpsink.c
bpsink_LDADD = libici.la $(bplib)
bpsink_CFLAGS = $(bpcflags) $(AM_CFLAGS)

bpsource_SOURCES = bpv7/test/bpsource.c
bpsource_LDADD = libici.la $(bplib)
bpsource_CFLAGS = $(bpcflags) $(AM_CFLAGS)

cbrcustodytest_SOURCES = bpv7/test/cbrcustodytest.c
cbrcustodytest_LDADD = libici.la $(bplib)
cbrcustodytest_CFLAGS = $(bpcflags) $(AM_CFLAGS)

bpstats2_SOURCES = bpv7/test/bpstats2.c
bpstats2_LDADD = libici.la $(bplib)
bpstats2_CFLAGS = $(bpcflags) $(AM_CFLAGS)

bptracker_SOURCES = bpv7/utils/bptracker.c
bptracker_LDADD = libici.la $(bplib) $(PTHREAD_LIBS)
bptracker_CFLAGS = $(bpcflags) $(AM_CFLAGS)

cgrfetch_SOURCES = bpv7/utils/cgrfetch.c
cgrfetch_LDADD = libici.la $(bplib)
cgrfetch_CFLAGS = $(bpcflags) $(AM_CFLAGS)

recvfile_SOURCES = bpv7/utils/recvfile.c
recvfile_LDADD = libici.la $(bplib) $(secryptlib) $(metadatalib)
recvfile_CFLAGS = $(bpcflags) $(AM_CFLAGS)

sendfile_SOURCES = bpv7/utils/sendfile.c
sendfile_LDADD = libici.la $(bplib) $(secryptlib) $(metadatalib)
sendfile_CFLAGS = $(bpcflags) $(AM_CFLAGS)

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

if ENABLE_IMC
cpsd_SOURCES = bpv7/daemon/cpsd.c
cpsd_LDADD = libici.la $(bplib)
cpsd_CFLAGS = $(bpcflags) $(AM_CFLAGS)
endif

############################
#
# TRUSTED COLLECTIVE SECTION
#
############################

if ENABLE_TC

# Define pod2man stuff
tcPODM1 = pod2man -s 1 -c "TC executables"
tcPODM3 = pod2man -s 3 -c "TC library functions"
tcPODM5 = pod2man -s 5 -c "TC configuration files"

tccflags = -I$(srcdir)/tc/authority -I$(srcdir)/tc/client -I$(srcdir)/tc/include -I$(srcdir)/tc/jerasure/include -I$(srcdir)/tc/gf-complete/include -I$(BP_SRC_DIR)/library -I$(BP_SRC_DIR)/include

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
	libgfcomplete.la \
	libjerasure.la \
	libionfec.la

tcinclude = \
	tc/dtka/dtka.h \
	tc/authority/tcaP.h \
	tc/client/tcc.h \
	tc/client/tccP.h \
	tc/include/tc.h \
	tc/include/ion_fec.h

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
	tc/doc/dtka.1 \
	tc/doc/dtkaadmin.1 \
	tc/doc/tcaadmin.1 \
	tc/doc/tcaboot.1 \
	tc/doc/tcacompile.1 \
	tc/doc/tcapublish.1 \
	tc/doc/tcarecv.1 \
	tc/doc/tcc.1 \
	tc/doc/tccadmin.1 \
	tc/doc/tc.3 \
	tc/doc/dtkarc.5 \
	tc/doc/tcarc.5 \
	tc/doc/tccrc.5

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

# GF-Complete library (Galois Field arithmetic)
# Note: This is vendored third-party code; suppress warnings that are treated as errors
libgfcomplete_la_SOURCES = \
	tc/gf-complete/src/gf.c \
	tc/gf-complete/src/gf_w4.c \
	tc/gf-complete/src/gf_w8.c \
	tc/gf-complete/src/gf_w16.c \
	tc/gf-complete/src/gf_w32.c \
	tc/gf-complete/src/gf_w64.c \
	tc/gf-complete/src/gf_w128.c \
	tc/gf-complete/src/gf_wgen.c \
	tc/gf-complete/src/gf_general.c \
	tc/gf-complete/src/gf_rand.c \
	tc/gf-complete/src/gf_method.c \
	tc/gf-complete/src/gf_cpu.c
libgfcomplete_la_CFLAGS = -I$(srcdir)/tc/gf-complete/include -w

# Jerasure library (Reed-Solomon erasure coding)
# Note: This is vendored third-party code; suppress warnings that are treated as errors
libjerasure_la_SOURCES = \
	tc/jerasure/src/jerasure.c \
	tc/jerasure/src/reed_sol.c \
	tc/jerasure/src/galois.c \
	tc/jerasure/src/cauchy.c \
	tc/jerasure/src/liberation.c
libjerasure_la_CFLAGS = -I$(srcdir)/tc/jerasure/include -I$(srcdir)/tc/gf-complete/include -w
libjerasure_la_LIBADD = libgfcomplete.la

# ION FEC wrapper library
libionfec_la_SOURCES = tc/library/ion_fec.c
libionfec_la_CFLAGS = $(AM_CFLAGS) -I$(srcdir)/tc/include -I$(srcdir)/tc/jerasure/include -I$(srcdir)/tc/gf-complete/include
libionfec_la_LIBADD = libjerasure.la libgfcomplete.la


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

endif # ENABLE_TC

############################
#
# NETWORK MANAGEMENT SECTION
#
############################

# AMP version set at top

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
libamp_la_CFLAGS = -I$(srcdir)/ici/library -I$(srcdir)/bpv7/library -I$(srcdir)/bpv7/bpsec/utils -I$(srcdir)/bpv7/sci -I$(srcdir)/ltp/library -I$(srcdir)/nm \
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

NM_ADM_INCLUDES = -I$(srcdir)/nm/shared/adm -I$(BP_SRC_DIR)/adm -I$(BP_SRC_DIR) -I$(srcdir)/bpv7/bpsec/utils -I$(srcdir)/bpv7/sci

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
	nm/doc/nm_agent.1 \
	nm/doc/nm_mgr.1

nmclean-local:
	rm -f $(top_builddir)/nm/doc/*.1

libampAgentADM_la_SOURCES = $(nm_agent_ADM_SOURCES)
libampAgentADM_la_CFLAGS = -I$(srcdir)/nm -I$(srcdir)/nm/shared/adm -I$(BP_SRC_DIR) -I$(BP_SRC_DIR)/library/ext/bpsec -I$(BP_SRC_DIR)/library -I$(srcdir)/ltp/library $(NM_CBOR_CFLAGS) $(AM_CFLAGS) $(NO_UNUSED_VARIABLE)
libampAgentADM_la_LIBADD = libamp.la libici.la $(bplib) libltp.la


nm_agent_LDADD = libici.la $(bplib) libltp.la libamp.la libampAgentADM.la $(PTHREAD_LIBS)
nm_agent_LDFLAGS = $(AM_LDFLAGS) -L/usr/local/lib -lm -lpthread

nm_agent_CFLAGS = -I$(srcdir)/ici/library -I$(srcdir)/bpv7/library -I$(srcdir)/nm \
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

nm_mgr_LDADD = libamp.la libici.la $(bplib) libltp.la $(PTHREAD_LIBS)

# Note: NCURSES and MySql support are optional and controlled at configure time with --with-mysql or --with-ncurses
nm_mgr_LDFLAGS = $(AM_LDFLAGS) -lpthread
nm_mgr_CFLAGS = -I$(srcdir)/ici/library -I$(srcdir)/bpv7/library -I$(srcdir)/ltp/library -I$(srcdir)/nm -I$(srcdir)/nm/mgr $(AM_CFLAGS) $(NO_UNUSED_VARIABLE) $(NM_CBOR_CFLAGS) $(NM_ADM_INCLUDES)

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

if CRYPTO_MBEDTLS
    amscflags = -I$(srcdir)/ams/library -I$(srcdir)/ams/include -I$(srcdir)/ams/rams -I$(srcdir)/ici/crypto/MBEDTLS_SUITES -DUDPTS -DTCPTS
else
    amscflags = -I$(srcdir)/ams/library -I$(srcdir)/ams/include -I$(srcdir)/ams/rams -I$(srcdir)/ici/crypto/NULL_SUITES -DUDPTS -DTCPTS
endif

if ENABLE_DGR
    amscflags += -DDGRTS
    amsdgrlib = libdgr.la
else
    amsdgrlib =
endif


amsbin = \
	amsbenchr \
	amsbenchs \
	amsd \
	amslog \
	amslogprt \
	amsmib \
	amsshell \
	amsstop \
	ramsgate \
	amshello

amslib = \
	libams.la

amsinclude = \
	ams/include/ams.h

amsnoinst = \
	ams/library/amscommon.h \
	ams/library/amsP.h \
	ams/library/marshal.c \
	ams/rams/rams.h \
	ams/rams/ramscommon.h

amsextra = \
	ams/README.txt \
	ams/library/vmqts.c \
	ams/test/amspubsub.c

amsmans = \
	ams/doc/amsbenchr.1 \
	ams/doc/amsbenchs.1 \
	ams/doc/amsd.1 \
	ams/doc/amshello.1 \
	ams/doc/amslog.1 \
	ams/doc/amslogprt.1 \
	ams/doc/amsmib.1 \
	ams/doc/amspub.1 \
	ams/doc/amsshell.1 \
	ams/doc/amsstop.1 \
	ams/doc/amssub.1 \
	ams/doc/ramsgate.1 \
	ams/doc/ams.3 \
	ams/doc/amsrc.5 \
	ams/doc/amsxml.5 \
	ams/doc/petition_log.5

amsclean-local:
	rm -f $(top_builddir)/ams/doc/*.1
	rm -f $(top_builddir)/ams/doc/*.3
	rm -f $(top_builddir)/ams/doc/*.5

# -- Libraries --- #

libams_la_SOURCES = \
	ams/library/amscommon.c \
	ams/library/crypt.c \
	ams/library/libams.c \
	ams/library/loadmib.c \
	ams/library/tcpts.c \
	ams/library/udpts.c

if ENABLE_DGR
libams_la_SOURCES += \
	ams/library/dgrts.c
endif

if CRYPTO_MBEDTLS
libams_la_SOURCES += \
	ici/crypto/MBEDTLS_SUITES/secrypt.c
else
libams_la_SOURCES += \
	ici/crypto/NULL_SUITES/secrypt.c
endif

libams_la_LDFLAGS = -static
libams_la_CFLAGS = $(amscflags) $(AM_CFLAGS) $(EXPAT_CFLAGS)
libams_la_LIBADD = $(EXPAT_LIBS) $(bplib) $(amsdgrlib) libici.la $(secryptlib)

# --- Utility Programs --- #

amslog_SOURCES = ams/test/amslog.c
amslog_LDADD = libams.la $(amsdgrlib) libici.la -lm
amslog_CFLAGS = $(amscflags) $(AM_CFLAGS)

amslogprt_SOURCES = ams/test/amslogprt.c
amslogprt_LDADD = libams.la $(amsdgrlib) libici.la
amslogprt_CFLAGS = $(amscflags) $(AM_CFLAGS)

amsmib_SOURCES = ams/utils/amsmib.c
amsmib_LDADD = libams.la $(amsdgrlib) libici.la -lm
amsmib_CFLAGS = $(amscflags) $(AM_CFLAGS)

amsshell_SOURCES = ams/test/amsshell.c
amsshell_LDADD = libams.la $(amsdgrlib) libici.la -lm
amsshell_CFLAGS = $(amscflags) $(AM_CFLAGS)

# --- Test Executables --- #

amsbenchr_SOURCES = ams/test/amsbenchr.c
amsbenchr_LDADD = libams.la $(amsdgrlib) libici.la -lm
amsbenchr_CFLAGS = $(amscflags) $(AM_CFLAGS)

amsbenchs_SOURCES = ams/test/amsbenchs.c
amsbenchs_LDADD = libams.la $(amsdgrlib) libici.la -lm
amsbenchs_CFLAGS = $(amscflags) $(AM_CFLAGS)

amshello_SOURCES = ams/test/amshello.c
amshello_LDADD = libams.la $(amsdgrlib) libici.la -lm
amshello_CFLAGS = $(amscflags) $(AM_CFLAGS)

amsstop_SOURCES =ams/utils/amsstop.c
amsstop_LDADD = libams.la $(amsdgrlib) libici.la -lm
amsstop_CFLAGS = $(amscflags) $(AM_CFLAGS)

# --- Daemon Executables --- #

amsd_SOURCES = \
	ams/library/amscommon.c \
	ams/library/amsd.c \
	ams/library/crypt.c \
	ams/library/libams.c \
	ams/library/tcpts.c \
	ams/library/udpts.c

if ENABLE_DGR
amsd_SOURCES += \
	ams/library/dgrts.c
endif

amsd_CFLAGS = $(amscflags) $(AM_CFLAGS)
amsd_LDADD = libams.la $(amsdgrlib) libici.la -lm

# --- RAMS Executables --- #

ramsgate_SOURCES = \
	ams/rams/librams.c \
	ams/rams/ramscommon.c \
	ams/rams/ramsgate.c
ramsgate_LDADD = $(bplib) libams.la $(amsdgrlib) libici.la
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
	tcputa \
	bpcp \
	bpcpd

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
	cfdp/doc/bpcp.1 \
	cfdp/doc/bpcpd.1 \
	cfdp/doc/bputa.1 \
	cfdp/doc/cfdpadmin.1 \
	cfdp/doc/cfdpclock.1 \
	cfdp/doc/cfdptest.1 \
	cfdp/doc/cfdp.3 \
	cfdp/doc/cfdprc.5

cfdpclean-local:
	rm -f $(top_builddir)/cfdp/doc/*.1
	rm -f $(top_builddir)/cfdp/doc/*.3
	rm -f $(top_builddir)/cfdp/doc/*.5

# --- Libraries --- #

libcfdp_la_SOURCES = \
	cfdp/library/libcfdp.c \
	cfdp/library/libcfdpops.c \
	cfdp/library/libcfdpP.c
libcfdp_la_CPPFLAGS = -I$(srcdir)/ici/library
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
cfdpadmin_LDADD = libcfdp.la libici.la -lm $(bplib) $(LINENOISE_LDADD)
cfdpadmin_CFLAGS = $(cfdpcflags) $(AM_CFLAGS) $(LINENOISE_CFLAGS)

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
	bss/doc/bssrecv.1 \
	bss/doc/bssStreamingApp.1 \
	bss/doc/bss.3

bssclean-local:
	rm -f $(top_builddir)/bss/doc/*.1
	rm -f $(top_builddir)/bss/doc/*.3

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

if ENABLE_BSSP

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
	bssp/doc/bsspadmin.1 \
	bssp/doc/udpbso.1 \
	bssp/doc/bssp.3 \
	bssp/doc/bssprc.5

# --- Libraries --- #

libbssp_la_SOURCES = \
	bssp/library/libbssp.c \
	bssp/library/libbsspP.c
libbssp_la_CPPFLAGS = -I$(srcdir)/ici/library
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
bsspadmin_LDADD = libbssp.la libici.la -lm $(bplib) $(LINENOISE_LDADD)
bsspadmin_CFLAGS = $(bsspcflags) $(AM_CFLAGS) $(LINENOISE_CFLAGS)

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

endif # ENABLE_BSSP

##############
#
# DTPC SECTION
#
##############

if ENABLE_DTPC

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
	dtpc/doc/dtpcadmin.1 \
	dtpc/doc/dtpcclock.1 \
	dtpc/doc/dtpcd.1 \
	dtpc/doc/dtpcreceive.1 \
	dtpc/doc/dtpcsend.1 \
	dtpc/doc/dtpc.3 \
	dtpc/doc/dtpcrc.5

# --- Libraries --- #

libdtpc_la_SOURCES = \
	dtpc/library/libdtpc.c \
	dtpc/library/libdtpcP.c
libdtpc_la_CFLAGS = $(dtpccflags) $(AM_CFLAGS)
libdtpc_la_LDFLAGS = $(ION_LINK_FLAGS)
libdtpc_la_LIBADD = libici.la $(bplib) $(PTHREAD_LIBS)

# --- Utility Programs --- #

dtpcadmin_SOURCES = dtpc/utils/dtpcadmin.c
dtpcadmin_LDADD = libdtpc.la libici.la $(bplib) $(LINENOISE_LDADD)
dtpcadmin_CFLAGS = $(dtpccflags) $(AM_CFLAGS) $(LINENOISE_CFLAGS)

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

endif # ENABLE_DTPC

##########################
#
# RESTART SECTION
#
##########################

if ENABLE_BSSP
    ionexitbssplib = libbssp.la
else
    ionexitbssplib =
endif

if ENABLE_DTPC
    ionexitdtpclib = libdtpc.la
else
    ionexitdtpclib =
endif

if ENABLE_TC
    ionexittclib = libtcaP.la libtcc.la
else
    ionexittclib =
endif

restartbin = ionrestart ionexit

# --- Utility Programs --- #

ionexit_SOURCES = restart/utils/ionexit.c
ionexit_CPPFLAGS = -I$(srcdir)/cfdp/library
if ENABLE_BSSP
ionexit_CPPFLAGS += -I$(srcdir)/bssp/library
endif
ionexit_LDADD = libltp.la $(ionexitbssplib) libcfdp.la $(ionexitdtpclib) $(ionexittclib) libici.la -lm $(bplib)
ionexit_CFLAGS = $(icicflags) $(AM_CFLAGS) -I$(srcdir)/dtpc/include -I$(srcdir)/dtpc/library -I$(srcdir)/tc/include -I$(srcdir)/tc/authority -I$(srcdir)/tc/client

ionrestart_SOURCES = restart/utils/ionrestart.c
ionrestart_LDADD = libici.la $(bplib) libltp.la libcfdp.la
ionrestart_CFLAGS = -I$(srcdir)/ici/sdr -I$(srcdir)/ici/library -I$(srcdir)/$(BP_DIR)/library -I$(srcdir)/ltp/library -I$(srcdir)/ici/library -I$(srcdir)/cfdp/library $(AM_CFLAGS)

restartextra = restart/doc/pod1/ionrestart.pod
restartmans = restart/doc/ionrestart.1

restartclean-local:
	rm -f $(top_builddir)/restart/doc/*.1

##########################
#
# TESTS SECTION
#
##########################
.PHONY:	test check buildcheck test-all test-% retest bsl-clean-local all-bsl

# "make buildcheck" will compile any of the compiled tests, but won't run
# any of them. Also builds check_LTLIBRARIES (e.g., stub provider libraries).
buildcheck: $(check_PROGRAMS) $(check_LTLIBRARIES)

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

# Catch-all for tests, basically any string after "make test-" will run a
# testset file with the same name as that string.

# Explicit test-<name> rules generated by configure:
# See Generated Test Rules at bottom

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

# Unity Test Framework (http://throwtheswitch.org/unity) Utility
TESTUNITYOBJS = tests/library/libtestunity.la
TEST_UNITY_DIR = contrib/c-unity-harness/tests/unity
TESTUNITYCFLAGS = -I$(srcdir)/contrib/Unity/src
check_LTLIBRARIES += $(TESTUNITYOBJS)
tests_library_libtestunity_la_SOURCES = contrib/Unity/src/unity.c
tests_library_libtestunity_la_CFLAGS = $(TESTUNITYCFLAGS) $(AM_CFLAGS)

# SPP, EPP, and File loopback provider libraries for testing
# These are separate shared libraries that can be loaded by sppcli/sppclo, eppcli/eppclo, and filecli/fileclo
SPPSTUBLIB = bpv7/spp/stub/libspp_loopback_provider.la
EPPSTUBLIB = bpv7/epp/stub/libepp_loopback_provider.la
FILESTUBLIB = bpv7/file/stub/libfile_simple_provider.la
check_LTLIBRARIES += $(SPPSTUBLIB) $(EPPSTUBLIB) $(FILESTUBLIB)

bpv7_spp_stub_libspp_loopback_provider_la_SOURCES = bpv7/spp/stub/spp_loopback_provider.c
bpv7_spp_stub_libspp_loopback_provider_la_CFLAGS = $(AM_CFLAGS)
bpv7_spp_stub_libspp_loopback_provider_la_LDFLAGS = -module -avoid-version -rpath '$(libdir)'

bpv7_epp_stub_libepp_loopback_provider_la_SOURCES = bpv7/epp/stub/epp_loopback_provider.c
bpv7_epp_stub_libepp_loopback_provider_la_CFLAGS = $(AM_CFLAGS)
bpv7_epp_stub_libepp_loopback_provider_la_LDFLAGS = -module -avoid-version -rpath '$(libdir)'

bpv7_file_stub_libfile_simple_provider_la_SOURCES = bpv7/file/stub/file_simple_provider.c
bpv7_file_stub_libfile_simple_provider_la_CFLAGS = $(AM_CFLAGS)
bpv7_file_stub_libfile_simple_provider_la_LDFLAGS = -module -avoid-version -rpath '$(libdir)'

# TEST AUTHORS: README
# If a test does not require compilation, it should not be listed here. Just make a
#  script named "dotest" that is executable.
# If a test does require compilation, it should be listed here and have
#  the required _SOURCES, _LDADD, and _CFLAGS entries.
# If a test uses the testutil helpers, include $(TESTUTILOBJS) in _LDADD,
#  and $(TESTUTILCFLAGS) in _CFLAGS.
check_PROGRAMS = tests/1000.loopback/dotest \
	tests/1500.loopback-brs/dotest \
	tests/ionexit_test/checksemaphores \
	tests/issue-260-teach-valgrind-mtake/domtake \
	tests/issue-188-common-cos-syntax/dotest \
	tests/bug-0015-tcpclo-bpcp-sig-handling/test \
	tests/issue-330-cfdpclock-FDU-removal/cfdplisten \
	tests/issue-334-cfdp-transaction-id/dotest \
	tests/nm-unit/dotest \
	tests/nm-unit/utils/vector/dotest \
	tests/nm-unit/utils/rhht/dotest \
	tests/nm-unit/utils/radix_pt/dotest \
	tests/nm-unit/utils/radix_ut/dotest \
	tests/nm-unit-ipn2/dotest \
	tests/sm_subsystem/dotest \
	tests/admin_public_api/ltp_loopback/dotest \
	tests/admin_public_api/ltp_span_management/dotest \
	tests/admin_public_api/bp_plan_crash_recovery/dotest \
	tests/admin_public_api/ltp_span_crash_recovery/dotest \
	tests/admin_public_api/tcp_2nodes/dotest \
	tests/admin_public_api/bulk_removal/dotest \
	tests/bp_receive_concurrent_timer/dotest \
	tests/bp_sap_thread_ownership/dotest \
	tests/eid-format-preservation/dotest
#	tests/nm-unit/primitives/ari/dotest
# Note: SPP/EPP loopback tests use shell scripts, not C programs

# Only build semaphore deferred deletion test if using POSIX Named Semaphores
if USING_POSIX_NAMED_SEMAPHORES
check_PROGRAMS += tests/semaphore-deferred-deletion/dotest
endif

# Note: Built without shared nm libraries to allow compilation with USE_MALLOC flag.
tests_nm_unit_dotest_SOURCES = tests/nm-unit/dotest.c $(nm_agent_SOURCES) $(nm_shared_src) $(nm_agent_ADM_SOURCES)
tests_nm_unit_dotest_LDADD = libici.la $(bplib) libltp.la -lm $(TESTUTILOBJS) $(TESTUNITYOBJS)
tests_nm_unit_dotest_CFLAGS = $(nm_agent_CFLAGS) $(TESTUTILCFLAGS) $(TESTUNITYCFLAGS) -DTEST_MAIN -DUSE_MALLOC \
	 -I$(srcdir)/nm -I$(BP_SRC_DIR) -I$(srcdir)/$(BP_DIR)/library/ext/bpsec -I$(srcdir)/$(BP_DIR)/library -I$(srcdir)/ltp/library -I$(srcdir)/nm/shared

tests_nm_unit_ipn2_dotest_SOURCES = tests/nm-unit-ipn2/dotest.c $(nm_agent_SOURCES) $(nm_shared_src) $(nm_agent_ADM_SOURCES)
tests_nm_unit_ipn2_dotest_LDADD = libici.la $(bplib) libltp.la -lm $(TESTUTILOBJS) $(TESTUNITYOBJS)
tests_nm_unit_ipn2_dotest_CFLAGS = $(nm_agent_CFLAGS) $(TESTUTILCFLAGS) $(TESTUNITYCFLAGS) -DTEST_MAIN -DUSE_MALLOC \
	 -I$(srcdir)/nm -I$(BP_SRC_DIR) -I$(srcdir)/$(BP_DIR)/library/ext/bpsec -I$(srcdir)/$(BP_DIR)/library -I$(srcdir)/ltp/library -I$(srcdir)/nm/shared

tests_nm_unit_utils_vector_dotest_SOURCES = tests/nm-unit/utils/vector.c $(nm_shared_src)
tests_nm_unit_utils_vector_dotest_LDADD = -lm $(TESTUNITYOBJS) libici.la
tests_nm_unit_utils_vector_dotest_CFLAGS = $(libamp_la_CFLAGS) $(TESTUNITYCFLAGS) -DUSE_MALLOC

tests_nm_unit_utils_rhht_dotest_SOURCES = tests/nm-unit/utils/rhht.c $(nm_shared_src)
tests_nm_unit_utils_rhht_dotest_LDADD = -lm $(TESTUTILOBJS) $(TESTUNITYOBJS) libici.la $(bplib) libltp.la
tests_nm_unit_utils_rhht_dotest_CFLAGS = $(libamp_la_CFLAGS) $(TESTUTILCFLAGS) $(TESTUNITYCFLAGS) -DUSE_MALLOC -I$(srcdir)/nm/shared

tests_nm_unit_utils_radix_pt_dotest_SOURCES = tests/nm-unit/utils/radix_pt.c tests/nm-unit/utils/radix_gen.c ici/library/radix.c
tests_nm_unit_utils_radix_pt_dotest_LDADD = -lm $(TESTUTILOBJS) $(TESTUNITYOBJS) libici.la $(bplib) libltp.la
tests_nm_unit_utils_radix_pt_dotest_CFLAGS = $(libamp_la_CFLAGS) $(TESTUTILCFLAGS) $(TESTUNITYCFLAGS) -DUSE_MALLOC -I$(srcdir)/nm/shared -I$(srcdir)/ici/include

tests_nm_unit_utils_radix_ut_dotest_SOURCES = tests/nm-unit/utils/radix_ut.c tests/nm-unit/utils/radix_gen.c ici/library/radix.c
tests_nm_unit_utils_radix_ut_dotest_LDADD = -lm $(TESTUTILOBJS) $(TESTUNITYOBJS) libici.la $(bplib) libltp.la
tests_nm_unit_utils_radix_ut_dotest_CFLAGS = $(libamp_la_CFLAGS) $(TESTUTILCFLAGS) $(TESTUNITYCFLAGS) -DUSE_MALLOC -I$(srcdir)/nm/shared -I$(srcdir)/ici/include

#tests_nm_unit_primitives_ari_dotest_SOURCES = tests/nm-unit/primitives/ari.c $(nm_agent_SOURCES)
#tests_nm_unit_primitives_ari_dotest_LDADD = $(nm_agent_LDADD) -lm $(TESTUTILOBJS) $(TESTUNITYOBJS)
#tests_nm_unit_primitives_ari_dotest_CFLAGS = $(nm_agent_CFLAGS) $(TESTUTILCFLAGS) $(TESTUNITYCFLAGS)

tests_1000_loopback_dotest_SOURCES = tests/1000.loopback/dotest.c
tests_1000_loopback_dotest_LDADD = libici.la -lm $(bplib) $(ltplib) $(TESTUTILOBJS)
tests_1000_loopback_dotest_CFLAGS = $(bpcflags) $(AM_CFLAGS) $(TESTUTILCFLAGS)

tests_eid_format_preservation_dotest_SOURCES = tests/eid-format-preservation/dotest.c
tests_eid_format_preservation_dotest_LDADD = libici.la -lm $(bplib) $(ltplib) $(TESTUTILOBJS)
tests_eid_format_preservation_dotest_CFLAGS = $(bpcflags) $(AM_CFLAGS) $(TESTUTILCFLAGS)

tests_bp_receive_concurrent_timer_dotest_SOURCES = tests/bp_receive_concurrent_timer/dotest.c
tests_bp_receive_concurrent_timer_dotest_LDADD = libici.la -lm $(bplib) $(ltplib) $(TESTUTILOBJS) -lpthread
tests_bp_receive_concurrent_timer_dotest_CFLAGS = $(bpcflags) $(AM_CFLAGS) $(TESTUTILCFLAGS)

tests_bp_sap_thread_ownership_dotest_SOURCES = tests/bp_sap_thread_ownership/dotest.c
tests_bp_sap_thread_ownership_dotest_LDADD = libici.la -lm $(bplib) $(ltplib) $(TESTUTILOBJS) -lpthread
tests_bp_sap_thread_ownership_dotest_CFLAGS = $(bpcflags) $(AM_CFLAGS) $(TESTUTILCFLAGS)

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

# Note: SPP/EPP loopback tests (tests/loopback-spp, tests/loopback-epp) use
# shell scripts instead of C programs because they need to pass the provider
# library path to the CLI/CLO daemons.

tests_ionexit_test_checksemaphores_SOURCES = tests/ionexit_test/checksemaphores.c
tests_ionexit_test_checksemaphores_LDADD = -lpthread
tests_ionexit_test_checksemaphores_CFLAGS = $(AM_CFLAGS)

tests_issue_188_common_cos_syntax_dotest_SOURCES = tests/issue-188-common-cos-syntax/dotest.c
tests_issue_188_common_cos_syntax_dotest_LDADD = libici.la -lm $(bplib) $(ltplib) $(TESTUTILOBJS)
tests_issue_188_common_cos_syntax_dotest_CFLAGS = $(bpcflags) $(AM_CFLAGS) $(TESTUTILCFLAGS)

tests_issue_260_teach_valgrind_mtake_domtake_SOURCES = tests/issue-260-teach-valgrind-mtake/domtake.c
tests_issue_260_teach_valgrind_mtake_domtake_LDADD = libici.la -lm $(TESTUTILOBJS)
tests_issue_260_teach_valgrind_mtake_domtake_CFLAGS = $(icicflags) $(AM_CFLAGS) $(TESTUTILCFLAGS)

tests_bug_0015_tcpclo_bpcp_sig_handling_test_SOURCES=tests/bug-0015-tcpclo-bpcp-sig-handling/test.c
tests_bug_0015_tcpclo_bpcp_sig_handling_test_LDADD= libici.la -lm $(bplib)
tests_bug_0015_tcpclo_bpcp_sig_handling_test_CFLAGS=$(bpcflags) $(icicflags) $(AM_CFLAGS)

tests_issue_330_cfdpclock_FDU_removal_cfdplisten_SOURCES = tests/issue-330-cfdpclock-FDU-removal/cfdplisten.c
tests_issue_330_cfdpclock_FDU_removal_cfdplisten_LDADD = libcfdp.la libici.la -lm $(bplib)
tests_issue_330_cfdpclock_FDU_removal_cfdplisten_CFLAGS = $(bpcflags) $(icicflags) $(cfdpcflags) $(AM_CFLAGS)

tests_issue_334_cfdp_transaction_id_dotest_SOURCES = tests/issue-334-cfdp-transaction-id/dotest.c
tests_issue_334_cfdp_transaction_id_dotest_LDADD = libcfdp.la libici.la -lm $(TESTUTILOBJS)
tests_issue_334_cfdp_transaction_id_dotest_CFLAGS = $(icicflags) $(cfdpcflags) $(AM_CFLAGS) $(TESTUTILCFLAGS)

tests_sm_subsystem_dotest_SOURCES = tests/sm_subsystem/dotest.c
tests_sm_subsystem_dotest_LDADD = libici.la -lm $(bplib) $(TESTUTILOBJS)
tests_sm_subsystem_dotest_CFLAGS = $(bpcflags) $(AM_CFLAGS) $(TESTUTILCFLAGS) $(icicflags)

tests_admin_public_api_ltp_loopback_dotest_SOURCES = tests/admin_public_api/ltp_loopback/ltp_admin_api_test.c tests/admin_public_api/ltp_loopback/ion_test_utils.c
tests_admin_public_api_ltp_loopback_dotest_LDADD = libici.la -lm $(bplib) $(ltplib)
tests_admin_public_api_ltp_loopback_dotest_CFLAGS = $(icicflags) $(bpcflags) $(ltpcflags) $(AM_CFLAGS)

tests_admin_public_api_ltp_span_management_dotest_SOURCES = tests/admin_public_api/ltp_span_management/ltp_span_management_test.c tests/admin_public_api/ltp_loopback/ion_test_utils.c
tests_admin_public_api_ltp_span_management_dotest_LDADD = libici.la -lm $(bplib) $(ltplib)
tests_admin_public_api_ltp_span_management_dotest_CFLAGS = $(icicflags) $(bpcflags) $(ltpcflags) $(AM_CFLAGS)

tests_admin_public_api_bp_plan_crash_recovery_dotest_SOURCES = tests/admin_public_api/bp_plan_crash_recovery/bp_plan_crash_recovery_test.c tests/admin_public_api/ltp_loopback/ion_test_utils.c
tests_admin_public_api_bp_plan_crash_recovery_dotest_LDADD = libici.la -lm $(bplib) $(ltplib)
tests_admin_public_api_bp_plan_crash_recovery_dotest_CFLAGS = $(icicflags) $(bpcflags) $(ltpcflags) $(AM_CFLAGS)

tests_admin_public_api_ltp_span_crash_recovery_dotest_SOURCES = tests/admin_public_api/ltp_span_crash_recovery/ltp_span_crash_recovery_test.c tests/admin_public_api/ltp_loopback/ion_test_utils.c
tests_admin_public_api_ltp_span_crash_recovery_dotest_LDADD = libici.la -lm $(bplib) $(ltplib)
tests_admin_public_api_ltp_span_crash_recovery_dotest_CFLAGS = $(icicflags) $(bpcflags) $(ltpcflags) $(AM_CFLAGS)

# Helper program tcp_single_node for tcp_2nodes test.
# Must be noinst_PROGRAMS (not check_PROGRAMS) so it's built by default
# `make`; otherwise the launcher dotest cannot find it at runtime.
noinst_PROGRAMS = tests/admin_public_api/tcp_2nodes/tcp_single_node
tests_admin_public_api_tcp_2nodes_tcp_single_node_SOURCES = tests/admin_public_api/tcp_2nodes/tcp_single_node.c
tests_admin_public_api_tcp_2nodes_tcp_single_node_LDADD = libici.la -lm $(bplib)
tests_admin_public_api_tcp_2nodes_tcp_single_node_CFLAGS = $(icicflags) $(bpcflags) $(AM_CFLAGS)

tests_admin_public_api_tcp_2nodes_dotest_SOURCES = tests/admin_public_api/tcp_2nodes/tcp_2nodes_launcher.c
tests_admin_public_api_tcp_2nodes_dotest_LDADD = libici.la -lm $(bplib)
tests_admin_public_api_tcp_2nodes_dotest_CFLAGS = $(icicflags) $(bpcflags) $(AM_CFLAGS)

tests_admin_public_api_bulk_removal_dotest_SOURCES = tests/admin_public_api/bulk_removal/bulk_removal_test.c tests/admin_public_api/ltp_loopback/ion_test_utils.c
tests_admin_public_api_bulk_removal_dotest_LDADD = libici.la -lm $(bplib) $(ltplib)
tests_admin_public_api_bulk_removal_dotest_CFLAGS = $(icicflags) $(bpcflags) $(ltpcflags) $(AM_CFLAGS)

# Build rules for semaphore deferred deletion test (POSIX Named Semaphores only)
if USING_POSIX_NAMED_SEMAPHORES
tests_semaphore_deferred_deletion_dotest_SOURCES = tests/semaphore-deferred-deletion/test_deferred_deletion.c
tests_semaphore_deferred_deletion_dotest_LDADD = libici.la -lpthread
tests_semaphore_deferred_deletion_dotest_CFLAGS = $(icicflags) $(AM_CFLAGS)
endif


# ---- generated test rules (added by configure) ------------------
include $(top_builddir)/tests/testrules.mk

# ----  PS conversion & static lists-------------------------------
include $(top_builddir)/pstrules.mk    # am--include-marker
include $(top_builddir)/man-lists.mk   # am--include-marker

# ---- generated man‑page rules (added by configure) --------------
include $(top_builddir)/ici/doc/podman.mk      # am--include-marker
include $(top_builddir)/ltp/doc/podman.mk      # am--include-marker
include $(top_builddir)/dgr/doc/podman.mk      # am--include-marker
include $(top_builddir)/nm/doc/podman.mk       # am--include-marker
include $(top_builddir)/bpv7/doc/podman.mk     # am--include-marker
include $(top_builddir)/tc/doc/podman.mk       # am--include-marker
include $(top_builddir)/ams/doc/podman.mk      # am--include-marker
include $(top_builddir)/cfdp/doc/podman.mk     # am--include-marker
include $(top_builddir)/bss/doc/podman.mk      # am--include-marker
include $(top_builddir)/bssp/doc/podman.mk     # am--include-marker
include $(top_builddir)/dtpc/doc/podman.mk     # am--include-marker
include $(top_builddir)/restart/doc/podman.mk  # am--include-marker
