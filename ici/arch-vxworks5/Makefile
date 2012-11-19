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


#----------------------
# location of files to be compiled and used for compiling
#----------------------
SRC    = ../library
INCL   = ../include
TEST   = ../test
UTILS  = ../utils
DAEMON = ../daemon
SDR    = ../sdr
RESTART = ../restart

CPU       = PPC604
TOOL      = gnu
OPT       = -mlongcall -DPRIVATE_SYMTAB -DVXWORKS 

include $(WIND_BASE)/target/h/make/defs.bsp
include $(WIND_BASE)/target/h/make/make.$(CPU)$(TOOL)
include $(WIND_BASE)/target/h/make/defs.$(WIND_HOST_TYPE)

LDFLAGS = -r -o
LINKER  = ldppc
CFLAGS  = -g -mstrict-align -fvolatile -fno-builtin  -Wall -Werror

MAKEOBJECTS = $(CC) $(CFLAGS) $(OPT) -I$(INCL) -I$(TEST) -I$(SRC) -I$(SDR) -c

TESTINCLS = $(TEST)/file2sdr.h

PUBINCLS = \
	$(INCL)/llcv.h		\
        $(INCL)/platform.h	\
        $(INCL)/platform_sm.h	\
        $(INCL)/memmgr.h	\
        $(INCL)/lyst.h		\
        $(INCL)/psm.h		\
        $(INCL)/smlist.h	\
        $(INCL)/sptrace.h	\
	$(INCL)/ion.h		\
	$(INCL)/rfx.h		\
	$(INCL)/ionsec.h	\
	$(INCL)/zco.h		\
        $(INCL)/sdrxn.h		\
        $(INCL)/sdrmgt.h	\
        $(INCL)/sdrstring.h	\
        $(INCL)/sdrlist.h	\
        $(INCL)/sdrtable.h	\
        $(INCL)/sdrhash.h	\
        $(INCL)/sdr.h		\
	$(INCL)/smrbt.h

ICIINCLS = \
        $(SRC)/lystP.h		\
        $(SRC)/sdrP.h

ICISOURCES = \
	$(SRC)/llcv.c		\
	$(SRC)/platform.c	\
	$(SRC)/platform_sm.c	\
	$(SRC)/memmgr.c		\
	$(SRC)/lyst.c		\
	$(SRC)/psm.c		\
	$(SRC)/smlist.c		\
	$(SRC)/sptrace.c	\
	$(SRC)/ion.c		\
	$(SRC)/rfx.c		\
	$(SRC)/ionsec.c		\
	$(SRC)/zco.c		\
	$(SDR)/sdrxn.c		\
	$(SDR)/sdrmgt.c		\
	$(SDR)/sdrstring.c	\
	$(SDR)/sdrlist.c	\
	$(SDR)/sdrtable.c	\
	$(SDR)/sdrhash.c	\
	$(SDR)/sdrcatlg.c	\
	$(SDR)/smrbt.c		\
	$(TEST)/file2sdr.c	\
	$(TEST)/file2sm.c	\
	$(TEST)/psmshell.c	\
	$(TEST)/sdr2file.c	\
	$(TEST)/sm2file.c	\
	$(TEST)/smlistsh.c	\
	$(UTILS)/ionadmin.c	\
	$(UTILS)/ionsecadmin.c	\
	$(UTILS)/psmwatch.c	\
	$(UTILS)/sdrmend.c	\
	$(UTILS)/sdrwatch.c	\
	$(DAEMON)/rfxclock.c	\
	$(RESTART)/ionrestart.c


ALLICIOBJS =		\
	llcv.o		\
	platform.o	\
	platform_sm.o	\
	memmgr.o	\
	lyst.o		\
	psm.o		\
	smlist.o	\
	sptrace.o	\
	ion.o		\
	rfx.o		\
	ionsec.o	\
	zco.o		\
	sdrxn.o		\
	sdrmgt.o	\
	sdrstring.o	\
	sdrlist.o	\
	sdrtable.o	\
	sdrhash.o	\
	sdrcatlg.o	\
	smrbt.o		\
	file2sdr.o	\
	file2sm.o	\
	psmshell.	\
	sdr2file.o	\
	sm2file.o	\
	smlistsh.o	\
	ionadmin.o	\
	ionsecadmin.o	\
	psmwatch.o	\
	sdrmend.o	\
	sdrwatch.o	\
	rfxclock.o	\
	ionrestart.o


SOURCES = $(ICISOURCES)

OBJECTS  = $(SOURCES:.c=.o)

all	: default
default	: $(OBJECTS)
	$(LINKER) $(LDFLAGS) icilib.o $(ALLICIOBJS) 
	chmod 755 icilib.o




exe	:

.c.o:	$(SOURCES) $(PUBINCLS) $(TESTINCLS)
	$(MAKEOBJECTS) $< 

install:
	echo "DO NOTHING"

clean:
	rm *.o

