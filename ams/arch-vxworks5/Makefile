#################################################################################
# File: Makefile
# Date: July 11th 2008
# Build Version: 1.0
# Build Platform: SunOS 5.9 Generic_122300-02 sun4u sparc SUNW,Sun-Blade-2500
# Compiler Version: gcc version gcc-2.96 (2.96+) 19990621 AltiVec VxWorks 5.5
#
# Description: Build procedure for AMS with ION library 
# 
# Author: Amalaye Oyake
# Email:  amalaye.oyake@jpl.nasa.gov
# Phone:  (818) 393 7168
#
# Notes: This file contains the build procedure for the AMS within the ION tree
#        ION is the JPL implementation of DTN and is written by Scott Burleigh.
#        
#################################################################################

API       = ../library
INCL      = ../include
UTILS     = ../utils
TEST      = ../test
REMOTEAMS = ../rams
ICIINCL   = ../../ici/include
DGRINCL   = ../../dgr/include
LTPINCL   = ../../ltp/include
BPINCL    = ../../bp/include
EXPAT     = ../../expat/lib

CPU       = PPC604
TOOL      = gnu
OPT       = -mlongcall -DPRIVATE_SYMTAB -DVXWORKS -DTCPTS -DDGRTS -DUDPTS -DVMQTS

include $(WIND_BASE)/target/h/make/defs.bsp
include $(WIND_BASE)/target/h/make/make.$(CPU)$(TOOL)
include $(WIND_BASE)/target/h/make/defs.$(WIND_HOST_TYPE)

LDFLAGS = -r -o
LINKER  = ldppc

CFLAGS = -g -mstrict-align -fvolatile -fno-builtin  -Wall -Werror

MAKEOBJECTS = $(CC) $(CFLAGS) $(DINETFLAGS) $(OPT)  \
	-I$(ICIINCL) -I$(DGRINCL) -I$(LTPINCL) -I$(API) \
	-I$(INCL) -I$(REMOTEAMS) -I$(BPINCL) -I$(EXPAT)  -c


PUBINCLS = $(INCL)/ams.h		\
	       $(API)/amscommon.h	\
	       $(API)/amsP.h


AMSSOURCES =				\
	$(API)/amscommon.c		\
	$(API)/dgrts.c			\
	$(API)/loadmib.c		\
	$(API)/tcpts.c			\
	$(API)/vmqts.c			\
	$(API)/amsd.c			\
	$(API)/libams.c			\
	$(API)/nullcrypt.c		\
	$(API)/udpts.c			\
	$(TEST)/amsbenchr.c		\
	$(TEST)/amsbenchs.c		\
	$(TEST)/amslog.c		\
	$(TEST)/amspubsub.c		\
	$(TEST)/amsshell.c  
	#$(TEST)/amslogprt.c #NOT COMPILED
	#$(TEST)/amshello.c  #NOT COMPILED


REMOTEAMSSOURCES =			\
	$(REMOTEAMS)/librams.c		\
	$(REMOTEAMS)/ramscommon.c

ALLAMSLIBS = amsbenchr.o amsbenchs.o amscommon.o amsd.o  \
	      amslog.o amspubsub.o amsshell.o \
		  dgrts.o libams.o  loadmib.o nullcrypt.o     \
          tcpts.o udpts.o vmqts.o librams.o ramscommon.o

SOURCES = $(AMSSOURCES) $(REMOTEAMSSOURCES)

OBJECTS  = $(SOURCES:.c=.o)

all	: default
default	: $(OBJECTS)
	$(LINKER) $(LDFLAGS) amslib.o $(ALLAMSLIBS)
	chmod 755 amslib.o

exe	:

.c.o:	$(SOURCES) $(PUBINCLS)
	$(MAKEOBJECTS) $< 

install:
	echo "DO NOTHING"

clean:
	rm *.o
