IONFLAGS = -DRTEMS -DBP_EXTENDED -DGDSSYMTAB -DGDSLOGGER -DUSING_SDR_POINTERS \
-DION_NO_DNS -DNO_SDR_TRACE -DNO_PSM_TRACE -DNO_PROXY -DNO_DIRLIST

PGM=${ARCH}/ion.exe

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
   ionexit.c

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
	ecos.c

#	phn.c        \

LTPSOURCES =     \
	libltp.c     \
	libltpP.c    \
	ltpclock.c   \
	ltpmeter.c   \
	pmqlsi.c     \
	pmqlso.c     \
	ltpadmin.c

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

CSRCS = ionrtems.c $(ICISOURCES) $(BPSOURCES) $(LTPSOURCES) $(PXSOURCES) $(DTN2SOURCES) $(BSPSOURCES) $(CFDPSOURCES) $(TESTSOURCES)
COBJS = $(CSRCS:%.c=${ARCH}/%.o)

include $(RTEMS_MAKEFILE_PATH)/Makefile.inc
include $(RTEMS_CUSTOM)
include $(PROJECT_ROOT)/make/leaf.cfg

OBJS = $(COBJS) $(CXXOBJS) $(ASOBJS)

CFLAGS = -g -Wall -Werror $(IONFLAGS) -I../ici/include -I../dgr/include -I../ltp/include -I../bp/include -I../bp/library -I../bp/library/crypto -I../bp/library/ext/ecos -I../cfdp/include -I../ams/include -I. -O2

all:    ${ARCH} $(PGM)

$(PGM):	$(OBJS)
	$(make-exe)

clean:
	rm -f $(COBJS) $(PGM)
