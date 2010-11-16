################################################################################
# File: Makefile
# Date: February 10 2009
# Build Platform:   SunOS 5.9 Generic_122300-02 sun4u sparc SUNW,Sun-Blade-2500
# Compiler Version: gcc version gcc-2.96 (2.96+) 19990621 AltiVec VxWorks 5.5
#
# Description: Build procedure for ION
# 
# Author: Amalaye Oyake (original effort by Joseph Moholt)
# Email:  amalaye.oyake@jpl.nasa.gov
# Phone:  (818) 393 7168
#
# Notes: This file contains the build procedure for the AMS within the ION tree
#        ION is the JPL implementation of DTN and is written by Scott Burleigh.
#       
#        This build is for the PPC using the gcc compiler. However it follows
#        Windriver standard Makefile conventions, so to migrate this
#        makefile to a new environment, just change the CPU and TOOL macros. 
#
#        Furthermore for CISC processors like the x86 remove the -mlongcall flag.
#        RISC processors like MIPS and ARM will likely use -mlongcall or 
#        some variant thereof.
#
#################################################################################


SRC      = ../library
INCL     = ../include
UTILS    = ../utils
TEST     = ../test
DAEMON   = ../daemon
CGR      = ../cgr
IPN      = ../ipn
UDP	 = ../udp
TCP      = ../tcp
LTP	 = ../ltp
DGR      = ../dgr
BRS      = ../brs
ICIINCL  = ../../ici/include
DGRINCL  = ../../dgr/include
LTPINCL  = ../../ltp/include

CPU       = PPC604
TOOL      = gnu
OPT       = -mlongcall -DPRIVATE_SYMTAB -DVXWORKS 

include $(WIND_BASE)/target/h/make/defs.bsp
include $(WIND_BASE)/target/h/make/make.$(CPU)$(TOOL)
include $(WIND_BASE)/target/h/make/defs.$(WIND_HOST_TYPE)

LDFLAGS = -r -o
LINKER  = ldppc
CFLAGS  = -mstrict-align -fvolatile -fno-builtin  -Wall -Werror

MAKEOBJECTS = $(CC) $(CFLAGS) $(OPT) -I$(INCL) -I$(ICIINCL) -I$(DGRINCL) -I$(LTPINCL) -I$(TEST) -I$(SRC)  -c

#----------------------
# Source files to be used in the compilatbp process
#----------------------

PUBINCLS =			\
        $(INCL)/bp.h

BPINCLS =			\
        $(SRC)/bpP.h

IPNINCLS =			\
	$(IPN)/ipnfw.h		\
	$(BRS)/brs.h		\
	$(DGR)/dgrcla.h		\
	$(TCP)/tcpcla.h

BPSOURCES =			\
	$(SRC)/libbp.c		\
	$(SRC)/libbpP.c		\
	$(SRC)/ecos.c		\
	$(SRC)/phn.c		\
	$(DAEMON)/bpclock.c	\
	$(IPN)/ipnadminep.c	\
	$(IPN)/ipnadmin.c	\
	$(IPN)/ipnfw.c		\
	$(IPN)/libipnfw.c	\
	$(CGR)/libcgr.c		\
	$(UDP)/udpcli.c		\
	$(UDP)/udpclo.c		\
	$(UDP)/libudpcla.c	\
	$(TCP)/tcpcli.c		\
	$(TCP)/tcpclo.c		\
	$(TCP)/libtcpcla.c	\
	$(LTP)/ltpcli.c		\
	$(LTP)/ltpclo.c		\
	$(BRS)/brsscla.c	\
	$(BRS)/brsccla.c	\
	$(UTILS)/lgsend.c	\
	$(UTILS)/lgagent.c	\
	$(UTILS)/bpsendfile.c	\
	$(UTILS)/bpadmin.c	\
	$(TEST)/bpdriver.c	\
	$(TEST)/bpcounter.c	\
	$(TEST)/bpecho.c	\
	$(TEST)/bpsink.c	\
	$(TEST)/bpsource.c


ALLBPOBJS =			\
	libbp.o			\
	libbpP.o		\
	ecos.o			\
	phn.o			\
	bpclock.o		\
	ipnadminep.o		\
	ipnadmin.o		\
	ipnfw.o			\
	libipnfw.o		\
	libcgr.o		\
	udpcli.o		\
	udpclo.o		\
	libudpcla.o		\
	tcpcli.o		\
	tcpclo.o		\
	libtcpcla.o		\
	ltpcli.o		\
	ltpclo.o		\
	brsscla.o		\
	brsccla.o		\
	bpadmin.o		\
	bpsendfile.o		\
	lgsend.o		\
	lgagent.o		\
	bpdriver.o		\
	bpcounter.o		\
	bpecho.o		\
	bpsink.o		\
	bpsource.o

SOURCES = $(BPSOURCES)

OBJECTS = $(SOURCES:.c=.o)

all	: default
default	: $(OBJECTS)
	$(LINKER) $(LDFLAGS) bplib.o $(ALLBPOBJS) 
	chmod 755 bplib.o

exe	:

.c.o:	$(SOURCES) $(PUBINCLS) $(TESTINCLS)
	$(MAKEOBJECTS) $< 

install:
	echo "DO NOTHING"

clean:
	rm *.o
