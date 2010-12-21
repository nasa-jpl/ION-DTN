
################################################################################
# File: Makefile
# Date: February 10 2009
# Build Platform:   SunOS 5.9 Generic_122300-02 sun4u sparc SUNW,Sun-Blade-2500
# Compiler Version: gcc version gcc-2.96 (2.96+) 19990621 AltiVec VxWorks 5.5
#
# Description: Build procedure for DGR
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
IPN      = ../ipn
TCP      = ../tcp
DGR      = ../dgr
BRS      = ../brs
ICIINCL  = ../../ici/include

CPU       = PPC604
TOOL      = gnu
OPT       = -mlongcall -DPRIVATE_SYMTAB -DVXWORKS 

include $(WIND_BASE)/target/h/make/defs.bsp
include $(WIND_BASE)/target/h/make/make.$(CPU)$(TOOL)
include $(WIND_BASE)/target/h/make/defs.$(WIND_HOST_TYPE)

LDFLAGS = -r -o
LINKER  = ldppc
CFLAGS  = -g -mstrict-align -fvolatile -fno-builtin  -Wall -Werror

MAKEOBJECTS = $(CC) $(CFLAGS) $(OPT) -I$(INCL) -I$(ICIINCL)  -I$(TEST) -I$(SRC)  -c

DGRSOURCES =			\
	$(SRC)/libdgr.c		\
	$(TEST)/dgr2file.c	\
	$(TEST)/file2dgr.c   

ALLDGROBJS =   \
	libdgr.o   \
	dgr2file.o \
	file2dgr.o
#	file2tcp.o \
#	file2udp.o \
#	tcp2file.o \
#	udp2file.o     


SOURCES = $(DGRSOURCES)

OBJECTS = $(SOURCES:.c=.o)

all	: default
default	: $(OBJECTS)
	$(LINKER) $(LDFLAGS) dgrlib.o $(ALLDGROBJS) 
	chmod 755 dgrlib.o

exe	:

.c.o:	$(SOURCES) $(PUBINCLS) $(TESTINCLS)
	$(MAKEOBJECTS) $< 

install:
	echo "DO NOTHING"

clean:
	rm *.o
