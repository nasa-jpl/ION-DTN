IONFLAGS_T = -DRTEMS -DBP_EXTENDED -DGDSSYMTAB -DGDSLOGGER \
-DUSING_SDR_POINTERS  -DION_NO_DNS -DNO_SDR_TRACE -DNO_PSM_TRACE \
-DNO_PROXY -DNO_DIRLIST
# -DENABLE_IMC -DENABLE_BPACS

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
   ionsec.c      \
   ionsecadmin.c \
   crypto.c      \
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
	ltpei.c      \
	ltpclock.c   \
	ltpmeter.c   \
	pmqlsi.c     \
	pmqlso.c     \
	ltpadmin.c

BPSOURCES =      \
	libbp.c      \
	libbpP.c     \
	bpclock.c    \
	bptransit.c  \
	bpadmin.c    \
	bpstats.c    \
	bptrace.c    \
	bplist.c     \
	lgagent.c    \
	ipnadmin.c   \
	ipnfw.c      \
	libcgr.c     \
	eureka.c     \
	ipnadminep.c \
	libipnfw.c   \
	libdtn2fw.c  \
	ltpcli.c     \
	ltpclo.c     \
	bibeclo.c    \
	bei.c        \
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

#	Not yet included in srclinks
#	libbssp.c    \
#	libbsspP.c   \
#	bsspclock.c  \
#	udpbsi.c     \
#	udpbso.c     \
#	tcpbsi.c     \
#	tcpbso.c     \
#	libtcpbsa.c  \
#	bsspadmin.c  \

BSPSOURCES =     \
	ciphersuites.c \
	bab_hmac_sha1.c \
	bib_hmac_sha256.c \
	bcb_arc4.c \
	bsputil.c \
	bspbab.c  \
	bspbib.c  \
	bspbcb.c

BSSSOURCES =      \
	libbss.c     \
	libbssP.c    \

CFDPSOURCES =      \
	bputa.c      \
	cfdpclock.c  \
	cfdpadmin.c  \
	libcfdp.c    \
	libcfdpP.c   \
	libcfdpops.c 

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

CSRCS_I = ionrtems.c $(ICISOURCES) $(LTPSOURCES) $(BPSOURCES) $(BSPSOURCES) $(TESTSOURCES)
# $(BSSSOURCES)

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

INCLUDES_T= -I. -I../ici/include -I../ltp/include -I../bp/include
#	-I../dgr/include
#	-I../bss/include
#	-I../bssp/include

ifeq "$(ION_OPEN_SOURCE)" "1"
INCLUDES = $(INCLUDES_T) -I../cfdp/include -I../ams/include
else
INCLUDES = $(INCLUDES_T)
endif

#CFLAGS = -g -Wall -Werror $(IONFLAGS) $(INCLUDES) -O2
CFLAGS = -g -Wall -Werror $(IONFLAGS) $(INCLUDES)

all:    ${ARCH} $(PGM)

$(PGM):	$(OBJS)
	$(make-exe)

clean:
	rm -f $(COBJS) $(PGM)
