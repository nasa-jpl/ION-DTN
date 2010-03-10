# Makefile.am
#
# David Young
# July 7, 2008
#
# Makefile.am

# subdirs are listed in the order they are made
#SUBDIRS = ici ltp dgr bp ams icix doc configs
#start with ici only
#SCRIPTDIR = scripts
#SUBDIRS lead to recursion, which leads to slow building.
# we will now continue with the MONOLITHIC makefile.am

AM_CFLAGS = \
	$(ION_CFLAGS) \
	-Wall -g \
	-include config.h \
	-I$(srcdir)/ici/include \
	-I$(srcdir)/ltp/include \
	-I$(srcdir)/dgr/include \
	-I$(srcdir)/bp/include \
	-I$(srcdir)/ams/include \
	-I$(srcdir)/cfdp/include
#	-I$(srcdir)/icix/include

# don't use AM_LDFLAGS because autoconf sets it INCORRECTLY and it will
# segfault anything using libraries... which is everything.
#AM_LDFLAGS = $(ION_LINK_FLAGS) 

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
	$(amsextra) \
	$(cfdpextra)
#	$(icixextra) 
#	$(usercontribextra)
#	$(twonodestcp_DATA) \
#$(threenodestcpltp_DATA) \
#$(srcdir)/makefile.tar.gz 

# needed for make test
TESTS = loopbacktest.sh \
	sysctl_script.sh
# fill in with special testing environment variables
# TESTS_ENVIRONMENT

install-exec-hook: 
	@echo
	@echo "You should now run 'sudo ldconfig' if available."
	@echo

install-data-hook:
	@echo
	@echo "You should now run 'sudo ldconfig' if available."
	@echo

.PHONY:	test
test:
	cd tests && ./runtests
# create the makefile.tar.gz
# this script, makefile-tar.sh, doesn't get distributed.
#makefile.tar.gz:
#	$(srcdir)/makefile-tar.sh $(srcdir)

# create tutorial.pdf on-the-fly 
#tutorial.pdf: 
#	cd $(srcdir)/unreleased-doc && pdflatex tutorial.tex && pdflatex tutorial.tex 
#	mv $(srcdir)/unreleased-doc/tutorial.pdf ./tutorial.pdf

# clean-local refers to existing documents. there are no manpages for ams
clean-local: iciclean-local ltpclean-local dgrclean-local bpclean-local cfdpclean-local
#icixclean-local

# ams has no man pages
build-man-directories:
	mkdir -p $(icimandir)
	mkdir -p $(ltpmandir)
	mkdir -p $(dgrmandir)
	mkdir -p $(bpmandir)
	mkdir -p $(cfdpmandir)
#	mkdir -p $(icixmandir)

bin_PROGRAMS = \
	$(icibin) \
	$(ltpbin) \
	$(dgrbin) \
	$(bpbin) \
	$(amsbin) \
	$(cfdpbin)
#	$(usercontribbin)
#	$(icixbin)
#	yeah, icix text programs are still excluded from makefile.

lib_LTLIBRARIES = \
	$(icilib) \
	$(ltplib) \
	$(dgrlib) \
	$(bplib) \
	$(amslib) \
	$(cfdplib)
#	$(icixlib)

include_HEADERS = \
	$(iciinclude) \
	$(ltpinclude) \
	$(dgrinclude) \
	$(bpinclude) \
	$(amsinclude) \
	$(cfdpinclude)
#	$(icixinclude)

noinst_HEADERS = \
	$(icinoinst) \
	$(ltpnoinst) \
	$(dgrnoinst) \
	$(bpnoinst) \
	$(amsnoinst) \
	$(cfdpnoinst)
#	$(icixnoinst) 
#	$(usercontribnoinst)

man_MANS = \
	build-man-directories \
	$(icimans) \
	$(ltpmans) \
	$(dgrmans) \
	$(bpmans) \
	$(cfdpmans)
#	$(icixmans)

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
	$(srcdir)/ION-LTP-configuration.doc \
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
	$(srcdir)/configs/loopback-udp/loopback-udp.ionrc \
	$(srcdir)/configs/loopback-udp/loopback-udp.bprc \
	$(srcdir)/configs/loopback-udp/loopback-udp.ipnrc \
	$(srcdir)/configs/loopback-udp/loopback-udp.rc 
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
	$(srcdir)/configs/3node-ltp-brs/iontest.ipn.node5/ionstop \
	$(srcdir)/configs/3node-ltp-brs/iontest.ipn.node5/node5.bprc \
	$(srcdir)/configs/3node-ltp-brs/iontest.ipn.node5/node5.ionrc \
	$(srcdir)/configs/3node-ltp-brs/iontest.ipn.node5/node5.ipnrc \
	$(srcdir)/configs/3node-ltp-brs/iontest.ipn.node5/node5.ltprc 
node7dir = $(pkgdatadir)/configs/3node-ltp-brs/iontest.ipn.node7
node7_DATA = \
	$(srcdir)/configs/3node-ltp-brs/iontest.ipn.node7/ionconfig \
	$(srcdir)/configs/3node-ltp-brs/iontest.ipn.node7/ionstart \
	$(srcdir)/configs/3node-ltp-brs/iontest.ipn.node7/ionstop \
	$(srcdir)/configs/3node-ltp-brs/iontest.ipn.node7/node7.bprc \
	$(srcdir)/configs/3node-ltp-brs/iontest.ipn.node7/node7.ionrc \
	$(srcdir)/configs/3node-ltp-brs/iontest.ipn.node7/node7.ipnrc \
	$(srcdir)/configs/3node-ltp-brs/iontest.ipn.node7/node7.ltprc 
node10dir = $(pkgdatadir)/configs/3node-ltp-brs/iontest.ipn.node10
node10_DATA = \
	$(srcdir)/configs/3node-ltp-brs/iontest.ipn.node10/ionconfig \
	$(srcdir)/configs/3node-ltp-brs/iontest.ipn.node10/ionstart \
	$(srcdir)/configs/3node-ltp-brs/iontest.ipn.node10/ionstop \
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
# can we use pod2html in the release? or should we just
# compile these for the website use only?
# iciPODH = pod2html --noindex

icibindir = $(srcdir)/ici/library
iciincludedir = $(srcdir)/ici/include
icitestdir = $(srcdir)/ici/test
iciutilsdir = $(srcdir)/ici/utils
icidaemondir = $(srcdir)/ici/daemon
icisdrdir = $(srcdir)/ici/sdr
icidocdir = $(srcdir)/ici/doc
icimandir = $(top_builddir)/ici/doc

icicflags = -I$(icibindir) -I$(icitestdir) -I$(icisdrdir) 

icibin = \
	sdrwatch \
	psmwatch \
	ionadmin \
	ionsecadmin \
	sdrmend \
	file2sm \
	sm2file \
	file2sdr \
	sdr2file \
	psmshell \
	smlistsh \
	rfxclock \
	owlttb \
	owltsim

icilib = \
	libici.la 
#	librfx.la

iciinclude = \
	$(iciincludedir)/llcv.h \
	$(iciincludedir)/platform.h \
	$(iciincludedir)/platform_sm.h \
	$(iciincludedir)/memmgr.h \
	$(iciincludedir)/lyst.h \
	$(iciincludedir)/psm.h \
	$(iciincludedir)/smlist.h \
	$(iciincludedir)/sptrace.h \
	$(iciincludedir)/ion.h \
	$(iciincludedir)/rfx.h \
	$(iciincludedir)/ionsec.h \
	$(iciincludedir)/zco.h \
	$(iciincludedir)/sdrxn.h \
	$(iciincludedir)/sdrmgt.h \
	$(iciincludedir)/sdrstring.h \
	$(iciincludedir)/sdrlist.h \
	$(iciincludedir)/sdrtable.h \
	$(iciincludedir)/sdrhash.h \
	$(iciincludedir)/sdr.h

icinoinst = \
	$(icibindir)/lystP.h \
	$(icisdrdir)/sdrP.h \
	$(icitestdir)/file2sdr.h

iciextra = \
	$(srcdir)/ici/README.txt \
	$(icibindir)/symtab.c \
	$(icidocdir)/pod1/ionadmin.pod \
	$(icidocdir)/pod1/ionsecadmin.pod \
	$(icidocdir)/pod1/rfxclock.pod \
	$(icidocdir)/pod1/psmwatch.pod \
	$(icidocdir)/pod1/sdrwatch.pod \
	$(icidocdir)/pod1/sdrmend.pod \
	$(icidocdir)/pod1/file2sdr.pod \
	$(icidocdir)/pod1/file2sm.pod \
	$(icidocdir)/pod1/psmshell.pod \
	$(icidocdir)/pod1/sdr2file.pod \
	$(icidocdir)/pod1/sm2file.pod \
	$(icidocdir)/pod1/smlistsh.pod \
	$(icidocdir)/pod1/owltsim.pod \
	$(icidocdir)/pod1/owlttb.pod \
	$(icidocdir)/pod5/ionconfig.pod \
	$(icidocdir)/pod5/ionrc.pod \
	$(icidocdir)/pod5/ionsecrc.pod \
	$(icidocdir)/pod3/platform.pod \
	$(icidocdir)/pod3/memmgr.pod \
	$(icidocdir)/pod3/ion.pod \
	$(icidocdir)/pod3/llcv.pod \
	$(icidocdir)/pod3/lyst.pod \
	$(icidocdir)/pod3/psm.pod \
	$(icidocdir)/pod3/zco.pod \
	$(icidocdir)/pod3/smlist.pod \
	$(icidocdir)/pod3/sdrlist.pod \
	$(icidocdir)/pod3/sdrstring.pod \
	$(icidocdir)/pod3/sdrtable.pod \
	$(icidocdir)/pod3/sdrhash.pod \
	$(icidocdir)/pod3/sdr.pod

icimans = \
	$(icimandir)/ionadmin.1 \
	$(icimandir)/ionsecadmin.1 \
	$(icimandir)/rfxclock.1 \
	$(icimandir)/psmwatch.1 \
	$(icimandir)/sdrwatch.1 \
	$(icimandir)/sdrmend.1 \
	$(icimandir)/file2sdr.1 \
	$(icimandir)/file2sm.1 \
	$(icimandir)/psmshell.1 \
	$(icimandir)/sdr2file.1 \
	$(icimandir)/sm2file.1 \
	$(icimandir)/smlistsh.1 \
	$(icimandir)/owltsim.1 \
	$(icimandir)/owlttb.1 \
	$(icimandir)/ionconfig.5 \
	$(icimandir)/ionrc.5 \
	$(icimandir)/ionsecrc.5 \
	$(icimandir)/platform.3 \
	$(icimandir)/memmgr.3 \
	$(icimandir)/ion.3 \
	$(icimandir)/llcv.3 \
	$(icimandir)/lyst.3 \
	$(icimandir)/psm.3 \
	$(icimandir)/zco.3 \
	$(icimandir)/smlist.3 \
	$(icimandir)/sdrlist.3 \
	$(icimandir)/sdrstring.3 \
	$(icimandir)/sdrtable.3 \
	$(icimandir)/sdrhash.3 \
	$(icimandir)/sdr.3

iciclean-local:
	rm -f $(icimandir)/*.1
	rm -f $(icimandir)/*.3
	rm -f $(icimandir)/*.5

$(icimandir)/%.1: $(icidocdir)/pod1/%.pod
	$(iciPODM1) $< $@

$(icimandir)/%.3: $(icidocdir)/pod3/%.pod
	$(iciPODM3) $< $@

$(icimandir)/%.5: $(icidocdir)/pod5/%.pod
	$(iciPODM5) $< $@

# -- Libraries --- #

libici_la_SOURCES =	$(icibindir)/llcv.c \
			$(icibindir)/platform.c \
			$(icibindir)/platform_sm.c \
			$(icibindir)/memmgr.c \
			$(icibindir)/lyst.c \
			$(icibindir)/psm.c \
			$(icibindir)/smlist.c \
			$(icibindir)/sptrace.c \
			$(icibindir)/rfx.c \
			$(icibindir)/ion.c \
			$(icibindir)/ionsec.c \
			$(icibindir)/zco.c \
			$(icisdrdir)/sdrxn.c \
			$(icisdrdir)/sdrmgt.c \
			$(icisdrdir)/sdrstring.c \
			$(icisdrdir)/sdrlist.c \
			$(icisdrdir)/sdrtable.c \
			$(icisdrdir)/sdrhash.c \
			$(icisdrdir)/sdrcatlg.c
libici_la_CFLAGS = $(icicflags) $(AM_CFLAGS)
libici_la_LDFLAGS = $(ION_LINK_FLAGS)

#librfx_la_SOURCES = $(icibindir)/rfx.c
#librfx_la_CFLAGS = $(icicflags) $(AM_CFLAGS)
#librfx_la_LDFLAGS = $(ION_LINK_FLAGS)

# --- Utility Programs --- #

sdrwatch_SOURCES = $(iciutilsdir)/sdrwatch.c
sdrwatch_LDADD = libici.la $(LIBOBJS) 
sdrwatch_CFLAGS = $(icicflags) $(AM_CFLAGS)

sdrmend_SOURCES = $(iciutilsdir)/sdrmend.c
sdrmend_LDADD = libici.la $(LIBOBJS) 
sdrmend_CFLAGS = $(icicflags) $(AM_CFLAGS)

psmwatch_SOURCES = $(iciutilsdir)/psmwatch.c
psmwatch_LDADD = libici.la $(LIBOBJS)
psmwatch_CFLAGS = $(icicflags) $(AM_CFLAGS)

ionadmin_SOURCES = $(iciutilsdir)/ionadmin.c
ionadmin_LDADD = libici.la $(LIBOBJS) 
ionadmin_CFLAGS = $(icicflags) $(AM_CFLAGS)

ionsecadmin_SOURCES = $(iciutilsdir)/ionsecadmin.c
ionsecadmin_LDADD = libici.la $(LIBOBJS) 
ionsecadmin_CFLAGS = $(icicflags) $(AM_CFLAGS)

# --- Test Executables --- #

psmshell_SOURCES = $(icitestdir)/psmshell.c
psmshell_LDADD = libici.la $(LIBOBJS)
psmshell_CFLAGS = $(icicflags) $(AM_CFLAGS)

smlistsh_SOURCES = $(icitestdir)/smlistsh.c
smlistsh_LDADD = libici.la $(LIBOBJS)
smlistsh_CFLAGS = $(icicflags) $(AM_CFLAGS)

file2sm_SOURCES = $(icitestdir)/file2sm.c
file2sm_LDADD = libici.la $(LIBOBJS)
file2sm_CFLAGS = $(icicflags) $(AM_CFLAGS)

sm2file_SOURCES = $(icitestdir)/sm2file.c
sm2file_LDADD = libici.la $(LIBOBJS)
sm2file_CFLAGS = $(icicflags) $(AM_CFLAGS)

file2sdr_SOURCES = $(icitestdir)/file2sdr.c 
#file2sdr_SOURCES = $(icitestdir)/file2sdr.c $(icitestdir)/file2sdr.h
file2sdr_LDADD = libici.la $(LIBOBJS)
file2sdr_CFLAGS = $(icicflags) $(AM_CFLAGS)

sdr2file_SOURCES = $(icitestdir)/sdr2file.c
sdr2file_LDADD = libici.la $(LIBOBJS)
sdr2file_CFLAGS = $(icicflags) $(AM_CFLAGS)

owltsim_SOURCES = $(icitestdir)/owltsim.c
owltsim_LDADD = libici.la $(LIBOBJS)
owltsim_CFLAGS = $(icicflags) $(AM_CFLAGS)

owlttb_SOURCES = $(icitestdir)/owlttb.c
owlttb_LDADD = libici.la $(LIBOBJS)
owlttb_CFLAGS = $(icicflags) $(AM_CFLAGS)

# --- Daemon Executables --- #

rfxclock_SOURCES = $(icidaemondir)/rfxclock.c
#rfxclock_LDADD = libici.la librfx.la $(LIBOBJS)
rfxclock_LDADD = libici.la $(LIBOBJS)
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
# can we use pod2html in the release? or should we just
# compile these for the website use only?
#ltpPODH = pod2html --noindex

ltpbindir = $(srcdir)/ltp/library
ltpincludedir = $(srcdir)/ltp/include
ltptestdir = $(srcdir)/ltp/test
ltputilsdir = $(srcdir)/ltp/utils
ltpdaemondir = $(srcdir)/ltp/daemon
ltpudpdir = $(srcdir)/ltp/udp
ltpdocdir = $(srcdir)/ltp/doc
ltpmandir = $(top_builddir)/ltp/doc

ltpcflags = -I$(ltpbindir) 

ltpbin = \
	ltpadmin \
	ltpclock \
	ltpmeter \
	udplsi \
	udplso \
	ltpdriver \
	ltpcounter

ltplib = \
	libltpP.la \
	libltp.la

ltpinclude = \
	$(ltpincludedir)/ltp.h

ltpnoinst = \
	$(ltpbindir)/ltpP.h \
	$(ltpudpdir)/udplsa.h 

ltpextra = \
	$(srcdir)/ltp/README.txt \
	$(ltpdocdir)/pod1/ltpadmin.pod \
	$(ltpdocdir)/pod1/ltpclock.pod \
	$(ltpdocdir)/pod1/ltpcounter.pod \
	$(ltpdocdir)/pod1/ltpdriver.pod \
	$(ltpdocdir)/pod1/ltpmeter.pod \
	$(ltpdocdir)/pod1/udplsi.pod \
	$(ltpdocdir)/pod1/udplso.pod \
	$(ltpdocdir)/pod5/ltprc.pod \
	$(ltpdocdir)/pod3/ltp.pod

ltpmans = \
	$(ltpmandir)/ltpadmin.1 \
	$(ltpmandir)/ltpclock.1 \
	$(ltpmandir)/ltpcounter.1 \
	$(ltpmandir)/ltpdriver.1 \
	$(ltpmandir)/ltpmeter.1 \
	$(ltpmandir)/udplsi.1 \
	$(ltpmandir)/udplso.1 \
	$(ltpmandir)/ltprc.5 \
	$(ltpmandir)/ltp.3 

ltpclean-local:
	rm -f $(ltpmandir)/*.1
	rm -f $(ltpmandir)/*.3
	rm -f $(ltpmandir)/*.5

$(ltpmandir)/%.1: $(ltpdocdir)/pod1/%.pod
	$(ltpPODM1) $< $@

$(ltpmandir)/%.3: $(ltpdocdir)/pod3/%.pod
	$(ltpPODM3) $< $@

$(ltpmandir)/%.5: $(ltpdocdir)/pod5/%.pod
	$(ltpPODM5) $< $@

# -- Libraries --- #

libltp_la_SOURCES = $(ltpbindir)/libltp.c
libltp_la_CFLAGS = $(ltpcflags) $(AM_CFLAGS)
libltp_la_LDFLAGS = $(ION_LINK_FLAGS)

libltpP_la_SOURCES = $(ltpbindir)/libltpP.c
libltpP_la_CFLAGS = $(ltpcflags) $(AM_CFLAGS)
libltpP_la_LDFLAGS = $(ION_LINK_FLAGS)

# --- Utility Programs --- #

ltpadmin_SOURCES = $(ltputilsdir)/ltpadmin.c
ltpadmin_LDADD = libltpP.la libici.la $(LIBOBJS) 
ltpadmin_CFLAGS = $(ltpcflags) $(AM_CFLAGS)

# --- Test Executables --- #

ltpdriver_SOURCES = $(ltptestdir)/ltpdriver.c
ltpdriver_LDADD = libltp.la libltpP.la libici.la $(LIBOBJS)
ltpdriver_CFLAGS = $(ltpcflags) $(AM_CFLAGS)

ltpcounter_SOURCES = $(ltptestdir)/ltpcounter.c
ltpcounter_LDADD = libltp.la libltpP.la libici.la $(LIBOBJS)
ltpcounter_CFLAGS = $(ltpcflags) $(AM_CFLAGS)

# --- Daemon Executables --- #

ltpclock_SOURCES = $(ltpdaemondir)/ltpclock.c
#ltpclock_LDADD = libltpP.la libici.la librfx.la $(LIBOBJS)
ltpclock_LDADD = libltpP.la libici.la $(LIBOBJS)
ltpclock_CFLAGS = $(ltpcflags) $(AM_CFLAGS)

ltpmeter_SOURCES = $(ltpdaemondir)/ltpmeter.c
ltpmeter_LDADD = libltpP.la libici.la $(LIBOBJS)
ltpmeter_CFLAGS = $(ltpcflags) $(AM_CFLAGS)

# --- UDP Executables --- #

udplsi_SOURCES = $(ltpudpdir)/udplsi.c
udplsi_LDADD = libltpP.la libici.la $(LIBOBJS) 
udplsi_CFLAGS = $(ltpcflags) $(AM_CFLAGS)

udplso_SOURCES = $(ltpudpdir)/udplso.c
udplso_LDADD = libltpP.la libici.la $(LIBOBJS) 
udplso_CFLAGS = $(ltpcflags) $(AM_CFLAGS)

##########################
#
# DGR SECTION
#
##########################

# Define pod2man stuff
dgrPODM1 = pod2man -s 1 -c "DGR executables"
dgrPODM3 = pod2man -s 3 -c "DGR library functions"
# can we use pod2html in the release? or should we just
# compile these for the website use only?
#dgrPODH = pod2html --noindex

dgrbindir = $(srcdir)/dgr/library
dgrincludedir = $(srcdir)/dgr/include
dgrtestdir = $(srcdir)/dgr/test
dgrdocdir = $(srcdir)/dgr/doc
dgrmandir = $(top_builddir)/dgr/doc

dgrcflags = -I$(dgrbindir) -I$(dgrtestdir)

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
	$(dgrincludedir)/dgr.h

dgrnoinst = \
	$(dgrtestdir)/file2dgr.h \
	$(dgrtestdir)/file2tcp.h \
	$(dgrtestdir)/file2udp.h 

dgrextra = \
	$(srcdir)/dgr/README.txt \
	$(dgrdocdir)/pod1/dgr2file.pod \
	$(dgrdocdir)/pod1/file2dgr.pod \
	$(dgrdocdir)/pod3/dgr.pod 

dgrmans = \
	$(dgrmandir)/dgr2file.1 \
	$(dgrmandir)/file2dgr.1 \
	$(dgrmandir)/dgr.3 

dgrclean-local:
	rm -f $(dgrmandir)/*.1
	rm -f $(dgrmandir)/*.3

$(dgrmandir)/%.1: $(dgrdocdir)/pod1/%.pod
	$(dgrPODM1) $< $@

$(dgrmandir)/%.3: $(dgrdocdir)/pod3/%.pod
	$(dgrPODM3) $< $@

# -- Libraries --- #

libdgr_la_SOURCES = $(dgrbindir)/libdgr.c
libdgr_la_CFLAGS = $(dgrcflags) $(AM_CFLAGS)
libdgr_la_LDFLAGS = $(ION_LINK_FLAGS)

# --- Test Executables --- #

file2dgr_SOURCES = $(dgrtestdir)/file2dgr.c 
#file2dgr_SOURCES = $(dgrtestdir)/file2dgr.c $(dgrtestdir)/file2dgr.h
file2dgr_LDADD = libdgr.la libici.la $(LIBOBJS)
file2dgr_CFLAGS = $(dgrcflags) $(AM_CFLAGS)

dgr2file_SOURCES = $(dgrtestdir)/dgr2file.c
#dgr2file_SOURCES = $(dgrtestdir)/dgr2file.c $(dgrtestdir)/file2dgr.h
dgr2file_LDADD = libdgr.la libici.la $(LIBOBJS)
dgr2file_CFLAGS = $(dgrcflags) $(AM_CFLAGS)

file2udp_SOURCES = $(dgrtestdir)/file2udp.c
#file2udp_SOURCES = $(dgrtestdir)/file2udp.c $(dgrtestdir)/file2udp.h
file2udp_LDADD = libici.la $(LIBOBJS)
file2udp_CFLAGS = $(dgrcflags) $(AM_CFLAGS)

udp2file_SOURCES = $(dgrtestdir)/udp2file.c
#udp2file_SOURCES = $(dgrtestdir)/udp2file.c $(dgrtestdir)/file2udp.h
udp2file_LDADD = libici.la $(LIBOBJS)
udp2file_CFLAGS = $(dgrcflags) $(AM_CFLAGS)

#file2tcp_SOURCES = $(dgrtestdir)/file2tcp.c $(dgrtestdir)/file2tcp.h
file2tcp_SOURCES = $(dgrtestdir)/file2tcp.c
file2tcp_LDADD = libici.la $(LIBOBJS)
file2tcp_CFLAGS = $(dgrcflags) $(AM_CFLAGS)

#tcp2file_SOURCES = $(dgrtestdir)/tcp2file.c $(dgrtestdir)/file2tcp.h
tcp2file_SOURCES = $(dgrtestdir)/tcp2file.c
tcp2file_LDADD = libici.la $(LIBOBJS)
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
# can we use pod2html in the release? or should we just
# compile these for the website use only?
#bpPODH = pod2html --noindex

bpbindir = $(srcdir)/bp/library
bpincludedir = $(srcdir)/bp/include
bptestdir = $(srcdir)/bp/test
bputilsdir = $(srcdir)/bp/utils
bpdaemondir = $(srcdir)/bp/daemon
bpudpdir = $(srcdir)/bp/udp
bpbrsdir = $(srcdir)/bp/brs
bpdgrdir = $(srcdir)/bp/dgr
bpcgrdir = $(srcdir)/bp/cgr
bptcpdir = $(srcdir)/bp/tcp
bpltpdir = $(srcdir)/bp/ltp
bpdtn2dir = $(srcdir)/bp/dtn2
bpipndir = $(srcdir)/bp/ipn
bpdocdir = $(srcdir)/bp/doc
bpmandir = $(top_builddir)/bp/doc

bpcflags = -I$(bpbindir) -I$(bpincludedir) -I$(bpipndir) -I$(bpdtn2dir) -DBP_EXTENDED

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
	bpclock \
	ipnadmin \
	ipnfw \
	ipnadminep \
	dtn2admin \
	dtn2fw \
	dtn2adminep \
	tcpcli \
	tcpclo \
	stcpcli \
	stcpclo \
	brsscla \
	brsccla \
	udpcli \
	udpclo \
	dgrcla \
	ltpcli \
	ltpclo \
	lgsend \
	lgagent \
	bptrace \
	bpstats

bplib = \
	libbpP.la \
	libbp.la \
	libcgr.la \
	libipnfw.la \
	libdtn2fw.la \
	libtcpcla.la \
	libudpcla.la
#	libphn.la 
#	libecos.la

bpinclude = \
	$(bpincludedir)/bp.h

#ecos.h goes here?
#$(bpbindir)/ecos.h 
bpnoinst = \
	$(bpbindir)/bpP.h \
	$(bpbindir)/phn.h \
	$(bpipndir)/ipnfw.h \
	$(bpdtn2dir)/dtn2fw.h \
	$(bpbrsdir)/brscla.h \
	$(bpudpdir)/udpcla.h \
	$(bpltpdir)/ltpcla.h \
	$(bptcpdir)/tcpcla.h 

bpextra = \
	$(bpdocdir)/pod1/bpadmin.pod \
	$(bpdocdir)/pod1/bpclock.pod \
	$(bpdocdir)/pod1/bpcounter.pod \
	$(bpdocdir)/pod1/bpcancel.pod \
	$(bpdocdir)/pod1/bplist.pod \
	$(bpdocdir)/pod1/bpdriver.pod \
	$(bpdocdir)/pod1/bpecho.pod \
	$(bpdocdir)/pod1/bpsendfile.pod \
	$(bpdocdir)/pod1/bprecvfile.pod \
	$(bpdocdir)/pod1/bpsink.pod \
	$(bpdocdir)/pod1/bpsource.pod \
	$(bpdocdir)/pod1/bpstats.pod \
	$(bpdocdir)/pod1/bptrace.pod \
	$(bpdocdir)/pod1/brsccla.pod \
	$(bpdocdir)/pod1/brsscla.pod \
	$(bpdocdir)/pod1/dgrcla.pod \
	$(bpdocdir)/pod1/dtn2adminep.pod \
	$(bpdocdir)/pod1/dtn2admin.pod \
	$(bpdocdir)/pod1/dtn2fw.pod \
	$(bpdocdir)/pod1/ipnadminep.pod \
	$(bpdocdir)/pod1/ipnadmin.pod \
	$(bpdocdir)/pod1/ipnfw.pod \
	$(bpdocdir)/pod1/lgagent.pod \
	$(bpdocdir)/pod1/lgsend.pod \
	$(bpdocdir)/pod1/ltpcli.pod \
	$(bpdocdir)/pod1/ltpclo.pod \
	$(bpdocdir)/pod1/stcpcli.pod \
	$(bpdocdir)/pod1/tcpcli.pod \
	$(bpdocdir)/pod1/stcpclo.pod \
	$(bpdocdir)/pod1/tcpclo.pod \
	$(bpdocdir)/pod1/udpcli.pod \
	$(bpdocdir)/pod1/udpclo.pod \
	$(bpdocdir)/pod5/bprc.pod \
	$(bpdocdir)/pod5/ipnrc.pod \
	$(bpdocdir)/pod5/dtn2rc.pod \
	$(bpdocdir)/pod5/lgfile.pod \
	$(bpdocdir)/pod3/bp.pod \
	$(srcdir)/bp/README.txt \
	$(bpdocdir)/pod3/bpextensions.pod \
	$(bpbindir)/bpextensions.c \
	$(bpbindir)/noextensions.c \
	$(bpbindir)/ecos.h \
	$(bpbindir)/ecos.c \
	$(bpbindir)/bsp.h \
	$(bpbindir)/bsp.c \
	$(bpbindir)/sha1.c \
	$(bpbindir)/sha1.h \
	$(bpbindir)/hmac.c \
	$(bpbindir)/hmac.h \
	$(bpbindir)/NULL_BAB_HMAC/sha1.c \
	$(bpbindir)/NULL_BAB_HMAC/sha1.h \
	$(bpbindir)/NULL_BAB_HMAC/hmac.c \
	$(bpbindir)/NULL_BAB_HMAC/hmac.h

# noextensions.c is C code, but it isn't explicitly compiled.
# instead, the whole file is #included into libbpP.c
# For this reason, it is extra_DIST.
# ecos and myextensions are optional and it is unclear if they must be
# installed in the normal case- so we shall leave them extra_DIST for now.

bpmans = \
	$(bpmandir)/bpadmin.1 \
	$(bpmandir)/bpclock.1 \
	$(bpmandir)/bpcounter.1 \
	$(bpmandir)/bpcancel.1 \
	$(bpmandir)/bplist.1 \
	$(bpmandir)/bpdriver.1 \
	$(bpmandir)/bpecho.1 \
	$(bpmandir)/bpsendfile.1 \
	$(bpmandir)/bprecvfile.1 \
	$(bpmandir)/bpsink.1 \
	$(bpmandir)/bpsource.1 \
	$(bpmandir)/bpstats.1 \
	$(bpmandir)/bptrace.1 \
	$(bpmandir)/brsccla.1 \
	$(bpmandir)/brsscla.1 \
	$(bpmandir)/dgrcla.1 \
	$(bpmandir)/dtn2adminep.1 \
	$(bpmandir)/dtn2admin.1 \
	$(bpmandir)/dtn2fw.1 \
	$(bpmandir)/ipnadminep.1 \
	$(bpmandir)/ipnadmin.1 \
	$(bpmandir)/ipnfw.1 \
	$(bpmandir)/lgagent.1 \
	$(bpmandir)/lgsend.1 \
	$(bpmandir)/ltpcli.1 \
	$(bpmandir)/ltpclo.1 \
	$(bpmandir)/stcpcli.1 \
	$(bpmandir)/tcpcli.1 \
	$(bpmandir)/stcpclo.1 \
	$(bpmandir)/tcpclo.1 \
	$(bpmandir)/udpcli.1 \
	$(bpmandir)/udpclo.1 \
	$(bpmandir)/bprc.5 \
	$(bpmandir)/ipnrc.5 \
	$(bpmandir)/dtn2rc.5 \
	$(bpmandir)/lgfile.5 \
	$(bpmandir)/bp.3 \
	$(bpmandir)/bpextensions.3

bpclean-local:
	rm -f $(bpmandir)/*.1
	rm -f $(bpmandir)/*.3
	rm -f $(bpmandir)/*.5

$(bpmandir)/%.1: $(bpdocdir)/pod1/%.pod
	$(bpPODM1) $< $@

$(bpmandir)/%.3: $(bpdocdir)/pod3/%.pod
	$(bpPODM3) $< $@

$(bpmandir)/%.5: $(bpdocdir)/pod5/%.pod
	$(bpPODM5) $< $@

# -- Libraries --- #

libipnfw_la_SOURCES = $(bpipndir)/libipnfw.c 
libipnfw_la_CFLAGS = $(bpcflags) $(AM_CFLAGS)
libipnfw_la_LDFLAGS = $(ION_LINK_FLAGS)

libdtn2fw_la_SOURCES = $(bpdtn2dir)/libdtn2fw.c
libdtn2fw_la_CFLAGS = $(bpcflags) $(AM_CFLAGS)
libdtn2fw_la_LDFLAGS = $(ION_LINK_FLAGS)

libtcpcla_la_SOURCES = $(bptcpdir)/libtcpcla.c
libtcpcla_la_CFLAGS = $(bpcflags) $(AM_CFLAGS)
libtcpcla_la_LDFLAGS = $(ION_LINK_FLAGS)

libudpcla_la_SOURCES = $(bpudpdir)/libudpcla.c
libudpcla_la_CFLAGS = $(bpcflags) $(AM_CFLAGS)
libudpcla_la_LDFLAGS = $(ION_LINK_FLAGS)

libbp_la_SOURCES = $(bpbindir)/libbp.c
libbp_la_CFLAGS = $(bpcflags) $(AM_CFLAGS)
libbp_la_LDFLAGS = $(ION_LINK_FLAGS)

# ecos, phn, and bpP had a circular dependency;
# now they are one.
#libecos_la_SOURCES = $(bpbindir)/ecos.c
#libecos_la_CFLAGS = $(bpcflags) $(AM_CFLAGS)
#libecos_la_LDFLAGS = $(ION_LINK_FLAGS)
#
#libphn_la_SOURCES = $(bpbindir)/phn.c
#libphn_la_CFLAGS = $(bpcflags) $(AM_CFLAGS)
#libphn_la_LDFLAGS = $(ION_LINK_FLAGS)

libcgr_la_SOURCES = $(bpcgrdir)/libcgr.c
libcgr_la_CFLAGS = $(bpcflags) $(AM_CFLAGS)
libcgr_la_LDFLAGS = $(ION_LINK_FLAGS)

libbpP_la_SOURCES = \
	$(bpbindir)/libbpP.c \
	$(bpbindir)/phn.c \
	$(bpbindir)/ecos.c \
	$(bpbindir)/bsp.c \
	$(bpbindir)/hmac.c \
	$(bpbindir)/sha1.c
libbpP_la_CFLAGS = $(bpcflags) $(AM_CFLAGS)
libbpP_la_LDFLAGS = $(ION_LINK_FLAGS)

# --- Utility Programs --- #

bpadmin_SOURCES = $(bputilsdir)/bpadmin.c
bpadmin_LDADD = libbpP.la libbp.la libici.la $(LIBOBJS) 
bpadmin_CFLAGS = $(bpcflags) $(AM_CFLAGS)

ipnadmin_SOURCES = $(bpipndir)/ipnadmin.c
ipnadmin_LDADD = libipnfw.la libbp.la libbpP.la libici.la $(LIBOBJS) 
ipnadmin_CFLAGS = $(bpcflags) $(AM_CFLAGS)

dtn2admin_SOURCES = $(bpdtn2dir)/dtn2admin.c
dtn2admin_LDADD = libdtn2fw.la libbp.la libbpP.la libici.la $(LIBOBJS) 
dtn2admin_CFLAGS = $(bpcflags) $(AM_CFLAGS)

lgsend_SOURCES = $(bputilsdir)/lgsend.c
lgsend_LDADD = libbp.la libbpP.la libici.la $(LIBOBJS) 
lgsend_CFLAGS = $(bpcflags) $(AM_CFLAGS)

lgagent_SOURCES = $(bputilsdir)/lgagent.c
lgagent_LDADD = libbp.la libbpP.la libici.la $(LIBOBJS) 
lgagent_CFLAGS = $(bpcflags) $(AM_CFLAGS)

bpstats_SOURCES = $(bputilsdir)/bpstats.c
bpstats_LDADD = libbp.la libbpP.la libici.la $(LIBOBJS) 
bpstats_CFLAGS = $(bpcflags) $(AM_CFLAGS)

bptrace_SOURCES = $(bputilsdir)/bptrace.c
bptrace_LDADD = libbp.la libbpP.la libici.la $(LIBOBJS) 
bptrace_CFLAGS = $(bpcflags) $(AM_CFLAGS)

bpcancel_SOURCES = $(bputilsdir)/bpcancel.c
bpcancel_LDADD = libbp.la libbpP.la libici.la $(LIBOBJS) 
bpcancel_CFLAGS = $(bpcflags) $(AM_CFLAGS)

bplist_SOURCES = $(bputilsdir)/bplist.c
bplist_LDADD = libbp.la libbpP.la libici.la $(LIBOBJS) 
bplist_CFLAGS = $(bpcflags) $(AM_CFLAGS)

# --- IPN Executables --- #

ipnfw_SOURCES = $(bpipndir)/ipnfw.c
ipnfw_LDADD = libipnfw.la libcgr.la libbp.la libbpP.la libici.la $(LIBOBJS)
ipnfw_CFLAGS = $(bpcflags) $(AM_CFLAGS)

ipnadminep_SOURCES = $(bpipndir)/ipnadminep.c
ipnadminep_LDADD = libipnfw.la libbp.la libbpP.la libici.la $(LIBOBJS)
ipnadminep_CFLAGS = $(bpcflags) $(AM_CFLAGS)

# --- DTN Executables --- #

dtn2fw_SOURCES = $(bpdtn2dir)/dtn2fw.c
dtn2fw_LDADD = libdtn2fw.la libbp.la libbpP.la libici.la $(LIBOBJS)
dtn2fw_CFLAGS = $(bpcflags) $(AM_CFLAGS)

dtn2adminep_SOURCES = $(bpdtn2dir)/dtn2adminep.c
dtn2adminep_LDADD = libdtn2fw.la libbp.la libbpP.la libici.la $(LIBOBJS)
dtn2adminep_CFLAGS = $(bpcflags) $(AM_CFLAGS)

# --- LTP Executables --- #

ltpcli_SOURCES = $(bpltpdir)/ltpcli.c
ltpcli_LDADD = libipnfw.la libdtn2fw.la libbp.la libbpP.la libltp.la libltpP.la libici.la $(LIBOBJS) 
ltpcli_CFLAGS = $(bpcflags) $(AM_CFLAGS)

ltpclo_SOURCES = $(bpltpdir)/ltpclo.c
ltpclo_LDADD = libbp.la libbpP.la libltp.la libltpP.la libici.la $(LIBOBJS) 
ltpclo_CFLAGS = $(bpcflags) $(AM_CFLAGS)

# --- TCP Executables --- #

tcpcli_SOURCES = $(bptcpdir)/tcpcli.c
tcpcli_LDADD = libipnfw.la libdtn2fw.la libbp.la libbpP.la libtcpcla.la libici.la $(LIBOBJS) 
tcpcli_CFLAGS = $(bpcflags) $(AM_CFLAGS)

tcpclo_SOURCES = $(bptcpdir)/tcpclo.c
tcpclo_LDADD = libbp.la libbpP.la libtcpcla.la libici.la $(LIBOBJS) 
tcpclo_CFLAGS = $(bpcflags) $(AM_CFLAGS)

# --- STCP Executables --- #

stcpcli_SOURCES = $(bptcpdir)/stcpcli.c
stcpcli_LDADD = libipnfw.la libdtn2fw.la libbp.la libbpP.la libtcpcla.la libici.la $(LIBOBJS) 
stcpcli_CFLAGS = $(bpcflags) $(AM_CFLAGS)

stcpclo_SOURCES = $(bptcpdir)/stcpclo.c
stcpclo_LDADD = libbp.la libbpP.la libtcpcla.la libici.la $(LIBOBJS) 
stcpclo_CFLAGS = $(bpcflags) $(AM_CFLAGS)

# --- UDP Executables --- #

udpcli_SOURCES = $(bpudpdir)/udpcli.c
udpcli_LDADD = libipnfw.la libdtn2fw.la libbp.la libbpP.la libudpcla.la libici.la $(LIBOBJS) 
udpcli_CFLAGS = $(bpcflags) $(AM_CFLAGS)

udpclo_SOURCES = $(bpudpdir)/udpclo.c
udpclo_LDADD = libbp.la libbpP.la libudpcla.la libici.la $(LIBOBJS) 
udpclo_CFLAGS = $(bpcflags) $(AM_CFLAGS)

# --- DGR Executables --- #

dgrcla_SOURCES = $(bpdgrdir)/dgrcla.c
dgrcla_LDADD = libipnfw.la libdtn2fw.la libbp.la libbpP.la libdgr.la libici.la $(LIBOBJS) 
dgrcla_CFLAGS = $(bpcflags) $(AM_CFLAGS)

# --- BRS executables --- #

brsscla_SOURCES = $(bpbrsdir)/brsscla.c
brsscla_LDADD = libipnfw.la libdtn2fw.la libbp.la libbpP.la libtcpcla.la libici.la $(LIBOBJS) 
brsscla_CFLAGS = $(bpcflags) $(AM_CFLAGS)

brsccla_SOURCES = $(bpbrsdir)/brsccla.c
brsccla_LDADD = libipnfw.la libdtn2fw.la libbp.la libbpP.la libtcpcla.la libici.la $(LIBOBJS) 
brsccla_CFLAGS = $(bpcflags) $(AM_CFLAGS)

# --- Test Executables --- #

bpsource_SOURCES = $(bptestdir)/bpsource.c
bpsource_LDADD = libbp.la libbpP.la libici.la $(LIBOBJS)
bpsource_CFLAGS = $(bpcflags) $(AM_CFLAGS)

bpsink_SOURCES = $(bptestdir)/bpsink.c
bpsink_LDADD = libbp.la libbpP.la libici.la $(LIBOBJS)
bpsink_CFLAGS = $(bpcflags) $(AM_CFLAGS)

bpdriver_SOURCES = $(bptestdir)/bpdriver.c
bpdriver_LDADD = libbp.la libbpP.la libici.la $(LIBOBJS)
bpdriver_CFLAGS = $(bpcflags) $(AM_CFLAGS)

bpecho_SOURCES = $(bptestdir)/bpecho.c
bpecho_LDADD = libbp.la libbpP.la libici.la $(LIBOBJS)
bpecho_CFLAGS = $(bpcflags) $(AM_CFLAGS)

bpcounter_SOURCES = $(bptestdir)/bpcounter.c
bpcounter_LDADD = libbp.la libbpP.la libici.la $(LIBOBJS)
bpcounter_CFLAGS = $(bpcflags) $(AM_CFLAGS)

bpsendfile_SOURCES = $(bputilsdir)/bpsendfile.c
bpsendfile_LDADD = libbp.la libbpP.la libici.la $(LIBOBJS)
bpsendfile_CFLAGS = $(bpcflags) $(AM_CFLAGS)

bprecvfile_SOURCES = $(bputilsdir)/bprecvfile.c
bprecvfile_LDADD = libbp.la libbpP.la libici.la $(LIBOBJS)
bprecvfile_CFLAGS = $(bpcflags) $(AM_CFLAGS)

# --- Daemon Executables --- #

bpclock_SOURCES = $(bpdaemondir)/bpclock.c
bpclock_LDADD = libbpP.la libbp.la libici.la $(LIBOBJS)
bpclock_CFLAGS = $(bpcflags) $(AM_CFLAGS)

##########################
#
# AMS SECTION
#
##########################

amsbindir = $(srcdir)/ams/library
amsincludedir = $(srcdir)/ams/include
amstestdir = $(srcdir)/ams/test
amsutilsdir = $(srcdir)/ams/utils
amsramsdir = $(srcdir)/ams/rams

amscflags = -I$(amsbindir) -I$(amsincludedir) -I$(amsramsdir)

amsbin = \
	amsd \
	amshello \
	amsshell \
	amslog \
	amslogprt \
	amsbenchs \
	amsbenchr \
	ramstest

amslib = \
	libams.la

amsinclude = \
	$(amsincludedir)/ams.h

amsnoinst = \
	$(amsbindir)/amscommon.h \
	$(amsbindir)/amsP.h \
	$(amsramsdir)/rams.h \
	$(amsramsdir)/ramscommon.h

amsextra = \
	$(srcdir)/ams/README.txt \
	$(amsbindir)/vmqts.c \
	$(amstestdir)/amspubsub.c

# -- Libraries --- #

libams_la_SOURCES = \
	$(amsbindir)/libams.c \
	$(amsbindir)/amscommon.c \
	$(amsbindir)/loadmib.c \
	$(amsbindir)/nullcrypt.c \
	$(amsbindir)/dgrts.c \
	$(amsbindir)/udpts.c \
	$(amsbindir)/tcpts.c
libams_la_LDFLAGS = $(ION_LINK_FLAGS)
libams_la_CFLAGS = $(amscflags) $(AM_CFLAGS)

# --- Utility Programs --- #

amsshell_SOURCES = $(amstestdir)/amsshell.c
amsshell_LDADD = libams.la libdgr.la libici.la $(LIBOBJS) 
amsshell_CFLAGS = $(amscflags) $(AM_CFLAGS)

amslog_SOURCES = $(amstestdir)/amslog.c
amslog_LDADD = libams.la libdgr.la libici.la $(LIBOBJS) 
amslog_CFLAGS = $(amscflags) $(AM_CFLAGS)

amslogprt_SOURCES = $(amstestdir)/amslogprt.c
amslogprt_LDADD = libams.la libdgr.la libici.la $(LIBOBJS) 
amslogprt_CFLAGS = $(amscflags) $(AM_CFLAGS)

# --- Test Executables --- #

amshello_SOURCES = $(amstestdir)/amshello.c
amshello_LDADD = libams.la libdgr.la libici.la $(LIBOBJS)
amshello_CFLAGS = $(amscflags) $(AM_CFLAGS)

amsbenchs_SOURCES = $(amstestdir)/amsbenchs.c
amsbenchs_LDADD = libams.la libdgr.la libici.la $(LIBOBJS)
amsbenchs_CFLAGS = $(amscflags) $(AM_CFLAGS)

amsbenchr_SOURCES = $(amstestdir)/amsbenchr.c
amsbenchr_LDADD = libams.la libdgr.la libici.la $(LIBOBJS)
amsbenchr_CFLAGS = $(amscflags) $(AM_CFLAGS)

# --- Daemon Executables --- #

amsd_SOURCES = \
	$(amsbindir)/amsd.c \
	$(amsbindir)/libams.c \
	$(amsbindir)/amscommon.c \
	$(amsbindir)/loadmib.c \
	$(amsbindir)/nullcrypt.c \
	$(amsbindir)/dgrts.c \
	$(amsbindir)/udpts.c \
	$(amsbindir)/tcpts.c
amsd_LDADD = libdgr.la libici.la $(LIBOBJS)
amsd_CFLAGS = $(amscflags) $(AM_CFLAGS)

# --- RAMS Executables --- #

ramstest_SOURCES = \
	$(amsramsdir)/librams.c \
	$(amsramsdir)/ramscommon.c \
	$(amsramsdir)/ramsTest.c 
ramstest_LDADD = libbp.la libbpP.la libams.la libdgr.la libici.la $(LIBOBJS)
ramstest_CFLAGS = $(amscflags) $(AM_CFLAGS)

##############
#
# CFDP SECTION
#
##############

# Define pod2man stuff
cfdpPODM1 = pod2man -s 1 -c "CFDP executables"
cfdpPODM3 = pod2man -s 3 -c "CFDP library functions"
cfdpPODM5 = pod2man -s 5 -c "CFDP configuration files"
#cfdpPODH = pod2html --noindex

cfdpbindir = $(srcdir)/cfdp/library
cfdpincludedir = $(srcdir)/cfdp/include
cfdptestdir = $(srcdir)/cfdp/test
cfdputilsdir = $(srcdir)/cfdp/utils
cfdpdaemondir = $(srcdir)/cfdp/daemon
cfdpbpdir = $(srcdir)/cfdp/bp
cfdpdocdir = $(srcdir)/cfdp/doc
cfdpmandir = $(top_builddir)/cfdp/doc

cfdpcflags = -I$(cfdpbindir) -I$(cfdpincludedir) 

cfdpbin = \
	cfdpadmin \
	cfdpclock \
	bputa \
	cfdptest

cfdplib = \
	libcfdp.la \
	libcfdpP.la

cfdpinclude = \
	$(cfdpincludedir)/cfdp.h

cfdpnoinst = \
	$(cfdpbindir)/cfdpP.h \
	$(cfdpincludedir)/cfdpops.h

cfdpextra = \
	$(cfdpdocdir)/pod3/cfdp.pod \
	$(cfdpdocdir)/pod5/cfdprc.pod \
	$(cfdpdocdir)/pod1/cfdptest.pod \
	$(cfdpdocdir)/pod1/cfdpclock.pod \
	$(cfdpdocdir)/pod1/bputa.pod \
	$(cfdpdocdir)/pod1/cfdpadmin.pod \
	$(srcdir)/cfdp/README.txt

cfdpmans = \
	$(cfdpmandir)/cfdp.3 \
	$(cfdpmandir)/cfdprc.5 \
	$(cfdpmandir)/cfdptest.1 \
	$(cfdpmandir)/cfdpclock.1 \
	$(cfdpmandir)/bputa.1 \
	$(cfdpmandir)/cfdpadmin.1

cfdpclean-local:
	rm -f $(cfdpmandir)/*.1
	rm -f $(cfdpmandir)/*.3
	rm -f $(cfdpmandir)/*.5

$(cfdpmandir)/%.1: $(cfdpdocdir)/pod1/%.pod
	$(cfdpPODM1) $< $@

$(cfdpmandir)/%.3: $(cfdpdocdir)/pod3/%.pod
	$(cfdpPODM3) $< $@

$(cfdpmandir)/%.5: $(cfdpdocdir)/pod5/%.pod
	$(cfdpPODM5) $< $@

# --- Libraries --- #

libcfdpP_la_SOURCES = $(cfdpbindir)/libcfdpP.c
libcfdpP_la_CFLAGS = $(cfdpcflags) $(AM_CFLAGS)
libcfdpP_la_LDFLAGS = $(ION_LINK_FLAGS)

libcfdp_la_SOURCES = $(cfdpbindir)/libcfdp.c $(cfdpbindir)/libcfdpops.c
libcfdp_la_CFLAGS = $(cfdpcflags) $(AM_CFLAGS)
libcfdp_la_LDFLAGS = $(ION_LINK_FLAGS)

# --- Utility Programs --- #

cfdpadmin_SOURCES = $(cfdputilsdir)/cfdpadmin.c
cfdpadmin_LDADD = libcfdp.la libcfdpP.la libici.la libbp.la libbpP.la $(LIBOBJS)
cfdpadmin_CFLAGS = $(cfdpcflags) $(AM_CFLAGS)

# --- BP Programs --- #

bputa_SOURCES = $(cfdpbpdir)/bputa.c
bputa_LDADD = libcfdp.la libcfdpP.la libici.la libbp.la libbpP.la $(LIBOBJS)
bputa_CFLAGS = $(cfdpcflags) $(AM_CFLAGS)

# --- Test Programs --- #

cfdptest_SOURCES = $(cfdptestdir)/cfdptest.c
cfdptest_LDADD = libcfdp.la libcfdpP.la libici.la libbp.la libbpP.la $(LIBOBJS)
cfdptest_CFLAGS = $(cfdpcflags) $(AM_CFLAGS)

# --- Daemon Executables --- #

cfdpclock_SOURCES = $(cfdpdaemondir)/cfdpclock.c
cfdpclock_LDADD = libcfdp.la libcfdpP.la libici.la libbp.la libbpP.la $(LIBOBJS)
cfdpclock_CFLAGS = $(cfdpcflags) $(AM_CFLAGS)

##############
#
# ICIX SECTION
#
##############

# Define pod2man stuff
#icixPODM3 = pod2man -s 3 -c "ICIX library functions"
# can we use pod2html in the release? or should we just
# compile these for the website use only?
#icixPODH = pod2html --noindex

#icixbindir = $(srcdir)/icix/library
#icixincludedir = $(srcdir)/icix/include
#icixtestdir = $(srcdir)/icix/test
#icixdocdir = $(srcdir)/icix/doc
#icixmandir = $(top_builddir)/icix/doc

#icixcflags = -I$(icixbindir) -I$(icixtestdir)

#icixbin = \
#	sdrpiper \
#	sdrpipew \
#	sdrpiped

#icixlib = \
#	libicix.la \
#	libipcx.la

#icixinclude = \
#	$(icixincludedir)/vf.h \
#	$(icixincludedir)/search.h \
#	$(icixincludedir)/ipcx.h

#icixnoinst = \
#	$(icixincludedir)/sdrpipe.h \
#	$(icixincludedir)/chute.h \
#	$(icixincludedir)/sdrcv.h 

#icixextra = \
#	$(srcdir)/icix/README.txt \
#	$(icixdocdir)/search.pod \
#	$(icixdocdir)/vf.pod \
#	$(icixbindir)/chute.c \
#	$(icixbindir)/sdrcv.c \
#	$(icixbindir)/sdrpipe.c \
#	$(icixbindir)/sdrpiped.c \
#	$(icixtestdir)/sdrpiper.c \
#	$(icixtestdir)/sdrpipew.c

#icixmans = \
#	$(icixmandir)/search.3 \
#	$(icixmandir)/vf.3

#icixclean-local:
#	rm -f $(icixmandir)/*.3

#$(icixmandir)/%.3: $(icixdocdir)/%.pod
#	$(icixPODM3) $< $@

# -- Libraries --- #

#libicix_la_SOURCES = \
#	$(icixbindir)/vf.c \
#	$(icixbindir)/search.c
#libicix_la_CFLAGS = $(icixcflags) $(AM_CFLAGS)
#libicix_la_LDFLAGS = $(ION_LINK_FLAGS)

#libipcx_la_SOURCES = $(icixbindir)/ipcx.c
#libipcx_la_CFLAGS = $(icixcflags) $(AM_CFLAGS)
#libipcx_la_LDFLAGS = $(ION_LINK_FLAGS)

# --- Test Executables --- #

#sdrpipew_SOURCES = $(icixtestdir)/sdrpipew.c
#sdrpipew_LDADD = libicix.la libici.la $(LIBOBJS)
#sdrpipew_CFLAGS = $(icixcflags) $(AM_CFLAGS)

#sdrpiper_SOURCES = $(icixtestdir)/sdrpiper.c
#sdrpiper_LDADD = libicix.la libici.la $(LIBOBJS)
#sdrpiper_CFLAGS = $(icixcflags) $(AM_CFLAGS)

#sdrpiped_SOURCES = $(icixbindir)/sdrpiped.c
#sdrpiped_LDADD = libicix.la libici.la $(LIBOBJS)
#sdrpiped_CFLAGS = $(icixcflags) $(AM_CFLAGS)

##########################
#
# USER-CONTRIB SECTION
#
##########################

# Define pod2man stuff
#bpPODM1 = pod2man -s 1 -c "BP executables"
#bpPODM3 = pod2man -s 3 -c "BP library functions"
#bpPODM5 = pod2man -s 5 -c "BP configuration files"
# can we use pod2html in the release? or should we just
# compile these for the website use only?
#bpPODH = pod2html --noindex

#bpincludedir = $(srcdir)/bp/include
#usercontribbpalivedir = $(srcdir)/user-contrib/bpalive
#usercontribbpingdir = $(srcdir)/user-contrib/bping
#usercontribbpmondir = $(srcdir)/user-contrib/bpmon
#bpdocdir = $(srcdir)/bp/doc
#bpmandir = $(top_builddir)/bp/doc

#sbpapiinclude = -I$(srcdir)/user-contrib/sbp_api -I$(srcdir)/bp/dtn2 -DSBP_USE_ION
#sbpapifiles = $(srcdir)/user-contrib/sbp_api/sbp_socket.c $(srcdir)/user-contrib/sbp_api/sbp_ion.c

#usercontribbin = \
	#bpalive \
	#bpmon_server \
	#bpmon_query \
	#bpmon_client \
	#bping \
	#bpong 
#
#usercontribnoinst = \
	#$(srcdir)/user-contrib/sbp_api/sbp_api.h \
	#$(srcdir)/user-contrib/sbp_api/sbp_ion.h \
	#$(srcdir)/user-contrib/sbp_api/sbp_socket.h \
	#$(srcdir)/user-contrib/bpmon/bpmon_server.h \
	#$(srcdir)/user-contrib/bpmon/termutil.h \
	#$(srcdir)/user-contrib/bpmon/debug.h
#
#usercontribextra = \
	#$(srcdir)/user-contrib/README.txt 

# --- bpalive --- #

#bpalive_SOURCES = $(srcdir)/user-contrib/bpalive/bpalive.c
#bpalive_LDADD = libecos.la libbp.la libbpP.la libici.la $(LIBOBJS)
#bpalive_CFLAGS = $(bpcflags) $(AM_CFLAGS)

# --- bpmon --- #

#bpmon_client_SOURCES = $(srcdir)/user-contrib/bpmon/bpmon_client.c $(srcdir)/user-contrib/bpmon/termutil.c $(sbpapifiles)
#bpmon_client_LDADD = libecos.la libdtn2fw.la libbp.la libbpP.la libici.la $(LIBOBJS)
#bpmon_client_CFLAGS = $(bpcflags) $(sbpapiinclude) $(AM_CFLAGS)

#bpmon_query_SOURCES = $(srcdir)/user-contrib/bpmon/bpmon_query.c $(sbpapifiles)
#bpmon_query_LDADD = libecos.la libdtn2fw.la libbp.la libbpP.la libici.la $(LIBOBJS)
#bpmon_query_CFLAGS = $(bpcflags) $(sbpapiinclude) $(AM_CFLAGS)

#bpmon_server_SOURCES = $(srcdir)/user-contrib/bpmon/bpmon_server.c $(srcdir)/user-contrib/bpmon/debug.c $(srcdir)/user-contrib/bpmon/bpmon_callback.c $(sbpapifiles)
#bpmon_server_LDADD = libecos.la libdtn2fw.la libbp.la libbpP.la libici.la $(LIBOBJS)
#bpmon_server_CFLAGS = $(bpcflags) $(sbpapiinclude) $(AM_CFLAGS)

# --- bping --- #

#bping_SOURCES = $(srcdir)/user-contrib/bping/bping.c $(sbpapifiles)
#bping_LDADD = libecos.la libdtn2fw.la libbp.la libbpP.la libici.la $(LIBOBJS)
#bping_CFLAGS = $(bpcflags) $(sbpapiinclude) $(AM_CFLAGS)

#bpong_SOURCES = $(srcdir)/user-contrib/bping/bpong.c $(sbpapifiles)
#bpong_LDADD = libecos.la libdtn2fw.la libbp.la libbpP.la libici.la $(LIBOBJS)
#bpong_CFLAGS = $(bpcflags) $(sbpapiinclude) $(AM_CFLAGS)

