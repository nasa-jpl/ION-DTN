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
   ionwarn.c     \
   ionrestart.c  \
   ionexit.c

LTPSOURCES =     \
	libltp.c     \
	libltpP.c    \
	ltpclock.c   \
	ltpmeter.c   \
	pmqlsi.c     \
	pmqlso.c     \
	ltpadmin.c

BPSOURCES =      \
	libbp.c      \
	libbpP.c     \
	bpclock.c    \
	bpadmin.c    \
	bpstats.c    \
	bptrace.c    \
	bplist.c     \
	lgagent.c    \
	ipnadmin.c   \
	ipnfw.c      \
	libcgr.c     \
	ipnadminep.c \
	libipnfw.c   \
	ltpcli.c     \
	ltpclo.c     \
	bei.c        \
	ecos.c       \
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
#	phn.c        \

BSPSOURCES =     \
	extbsputil.c \
	extbspbab.c  \
	extbsppib.c  \
	extbsppcb.c  \
	crypto.c          

DTN2SOURCES =        \
	libdtn2fw.c 

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

CSRCS_I = ionrtems.c $(ICISOURCES) $(LTPSOURCES) $(BPSOURCES) $(BSPSOURCES) $(DTN2SOURCES) $(PXSOURCES) $(TESTSOURCES)

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
include $(PROJECT_ROOT)/make/leaf.cfg

OBJS = $(COBJS) $(CXXOBJS) $(ASOBJS)

INCLUDES_T= -I. -I../ici/include -I../dgr/include -I../ltp/include -I../bp/include

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
