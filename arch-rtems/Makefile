IONFLAGS_T = -DRTEMS -DBP_EXTENDED -DGDSSYMTAB -DGDSLOGGER \
-DUSING_SDR_POINTERS -DNO_SDR_TRACE -DNO_PSM_TRACE \
-DNO_PROXY -DNO_DIRLIST -DNOEXPAT -DUDPTS -DTCPTS -DDGRTS
# -DION_NO_DNS 
# Add -DSBSP if building with sbsp.
# -DSBSP
# Add -DAGENT_ROLE if building with nm.
# -DAGENT_ROLE
# -DENABLE_IMC
# -DENABLE_BPACS

PGM=${ARCH}/ion.exe

ION_OPEN_SOURCE = 1

# optional managers required
MANAGERS = io semaphore

ICISOURCES =     \
   platform.c    \
   platform_sm.c \
   memmgr.c      \
   llcv.c        \
   lyst.c        \
   psm.c         \
   smlist.c      \
   smrbt.c       \
   ion.c         \
   rfx.c         \
   zco.c         \
   sdrtable.c    \
   sdrhash.c     \
   sdrxn.c       \
   sdrmgt.c      \
   sdrstring.c   \
   sdrlist.c     \
   sdrcatlg.c    \
   rfxclock.c    \
   ionadmin.c    \
   sdrmend.c     \
   ionunlock.c   \
   ionxnowner.c  \
   ionlog.c      \
   ionsec.c      \
   ionsecadmin.c \
   crypto.c      \
   csi.c         \
   bulk.c        \
   ionwarn.c     \
   ionrestart.c  \
   ionexit.c

#  bloom.c       \
#  MurmurHash2.c \

LTPSOURCES =     \
	libltp.c     \
	libltpP.c    \
	libltpnm.c   \
	ltpsec.c     \
	ltpei.c      \
	ltpclock.c   \
	ltpdeliv.c   \
	ltpmeter.c   \
	pmqlsi.c     \
	pmqlso.c     \
	ltpadmin.c   \
	ltpsecadmin.c

BPSOURCES =      \
	libbp.c      \
	libbpP.c     \
	libbpnm.c    \
	bpsec.c      \
	bpclock.c    \
	bptransit.c  \
	bpclm.c      \
	bpadmin.c    \
	bpsecadmin.c \
	bpstats.c    \
	bptrace.c    \
	bplist.c     \
	bpcrash.c    \
	lgagent.c    \
	ipnadmin.c   \
	ipnfw.c      \
	libcgr.c     \
	eureka.c     \
	saga.c 	     \
	ipnadminep.c \
	libipnfw.c   \
	libdtn2fw.c  \
	ltpcli.c     \
	ltpclo.c     \
	bibeclo.c    \
	bei.c        \
	snw.c        \
	phn.c        \
	ecos.c       \
	meb.c        \
	bae.c

#	imcadmin.c   \
#	imcfw.c      \
#	libimcfw.c   \
#	cteb.c       \
#       acsappend.c  \
#	acsid.c      \
#	acstx.c      \
#	acsrx.c      \
#	acsserialize.c \
#	acsadmin.c   \
#	acslist.c    \
#	beacon.c     \
#	helper.c     \
#	ipnd.c       \
#	node.c       \

NMSOURCES =	     \
	adm.c \
	adm_agent.c \
	adm_bp.c \
	adm_sbsp.c \
	adm_ion.c \
	adm_ltp.c \
	msg_admin.c \
	msg_ctrl.c \
	pdu.c \
	var.c \
	expr.c \
	blob.c \
	admin.c \
	def.c \
	mid.c \
	oid.c \
	report.c \
	rules.c \
	dc.c \
	value.c \
	lit.c \
	nn.c \
	tdc.c \
	ctrl.c \
	table.c \
	ion_if.c \
	utils.c \
	db.c \
	nm_types.c \
	ingest.c \
	instr.c \
	lcc.c \
	ldc.c \
	rda.c \
	agent_db.c \
	adm_ion_priv.c \
	adm_ltp_priv.c \
	adm_agent_impl.c \
	adm_bp_impl.c \
	adm_sbsp_impl.c \
	nmagent.c

SBSPSOURCES =       \
	sbsp_util.c \
	sbsp_instr.c\
	profiles.c   \
	sbsp_bib.c  \
	sbsp_bcb.c

BSSSOURCES =      \
	libbss.c     \
	libbssP.c    

BSSPSOURCES =      \
	libbssp.c     \
	libbsspP.c    \
	bsspclock.c   \
	bsspadmin.c   \
	udpbsi.c      \
	udpbso.c      \
	libtcpbsa.c   \
	tcpbsi.c      \
	tcpbso.c 

CFDPSOURCES =      \
	bputa.c      \
	cfdpclock.c  \
	cfdpadmin.c  \
	libcfdp.c    \
	libcfdpP.c   \
	libcfdpops.c 

AMSSOURCES =      \
	libdgr.c	   \
	amscommon.c        \
	amsd.c             \
	crypt.c            \
	libams.c           \
	loadmib.c          \
	dgrts.c            \
	tcpts.c            \
	udpts.c            \
	librams.c          \
	ramscommon.c       \
	ramsgate.c         \
	amsshell.c         \
	amslog.c           \
	amslogprt.c        \
	amsmib.c           \
	amsstop.c

#	vmqts.c

TESTSOURCES =      \
	bpsource.c   \
	bpsink.c

#	sptrace.c    \
#	bpstats2.c   \
#	bping.c      \
#	bpchat.c     \
#	bpcounter.c  \
#	bpdriver.c   \
#	bpecho.c     \
#	bpsendfile.c \
#	bprecvfile.c 

CSRCS_I = ionrtems.c $(ICISOURCES) $(LTPSOURCES) $(BPSOURCES) $(TESTSOURCES) $(BSSSOURCES) $(BSSPSOURCES) $(AMSSOURCES)
#	*** NOTE *** ION will build with SBSPSOURCES and NMSOURCES included.
#	But IT WON'T RUN.  The object file uses up too much of the RTEMS
#	memory allocation in this configuration; there's not enough memory
#	left for the ION node initialization to complete.
# $(SBSPSOURCES)
# $(NMSOURCES)

ifeq "$(ION_OPEN_SOURCE)" "1"
CSRCS = $(CSRCS_I) $(CFDPSOURCES)
IONFLAGS = $(IONFLAGS_T)
else
CSRCS = $(CSRCS_I)
IONFLAGS = $(IONFLAGS_T) -DNASA_PROTECTED_FLIGHT_CODE
endif

COBJS = $(CSRCS:%.c=${ARCH}/%.o)

include $(RTEMS_MAKEFILE_PATH)/Makefile.inc
include $(RTEMS_CUSTOM)
LD_LIBS += $(RTEMS_MAKEFILE_PATH)/../lib/libm.a
include $(PROJECT_ROOT)/make/leaf.cfg

OBJS = $(COBJS) $(CXXOBJS) $(ASOBJS)

INCLUDES_T= -I. -I../ici/include -I../ltp/include -I../bp/include \
-I../bp \
-I../bp/library \
-I../bp/library/ext \
-I../bp/library/ext/sbsp \
-I.. \
-I../nm \
-I../nm/agent \
-I../nm/shared/adm \
-I../nm/shared/msg \
-I../nm/shared/primitives \
-I../nm/shared/utils
#	-I../dgr/include
#	-I../bss/include
#	-I../bssp/include

ifeq "$(ION_OPEN_SOURCE)" "1"
INCLUDES = $(INCLUDES_T) -I../cfdp/include -I../ams/include
else
INCLUDES = $(INCLUDES_T)
endif

#CFLAGS = -g -Wall -Werror $(IONFLAGS) $(INCLUDES) -O2
#CFLAGS = -g -Wall -Werror $(IONFLAGS) $(INCLUDES)
CFLAGS = -g -Wall -Werror -Wno-unused-variable $(IONFLAGS) $(INCLUDES)

all:    ${ARCH} $(PGM)

$(PGM):	$(OBJS)
	$(make-exe)

clean:
	rm -f $(COBJS) $(PGM)
