SRC = ../library
INCL = ../include
TEST = ../test
UTILS = ../utils
DAEMON = ../daemon
SDR = ../sdr

OPT = -g -Wall -Dinterix
CC = gcc $(OPT) -I$(SRC) -I$(TEST) -I$(INCL)
LDFLAGS = -fPIC -shared
LD = gcc $(LDFLAGS)

LIBICIOBJS = \
	llcv.o \
	platform.o \
	platform_sm.o \
	memmgr.o \
	lyst.o \
	psm.o \
	smlist.o \
	sptrace.o \
	ion.o \
	zco.o \
	sdrxn.o \
	sdrmgt.o \
	sdrstring.o \
	sdrlist.o \
	sdrtable.o \
	sdrhash.o \
	sdrcatlg.o

PUBINCLS = \
	$(INCL)/llcv.h \
	$(INCL)/platform.h \
	$(INCL)/platform_sm.h \
	$(INCL)/memmgr.h \
	$(INCL)/lyst.h \
	$(INCL)/psm.h \
	$(INCL)/smlist.h \
	$(INCL)/sptrace.h \
	$(INCL)/ion.h \
	$(INCL)/zco.h \
	$(INCL)/rfx.h \
	$(INCL)/sdrxn.h \
	$(INCL)/sdrmgt.h \
	$(INCL)/sdrstring.h \
	$(INCL)/sdrlist.h \
	$(INCL)/sdrtable.h \
	$(INCL)/sdrhash.h \
	$(INCL)/sdr.h

ICIINCLS = \
	$(SRC)/lystP.h \
	$(SRC)/sdrP.h

UTILITIES = sdrwatch psmwath ionadmin sdrmend

TESTPGMS = psmshell smlistsh file2sm sm2file file2sdr sdr2file

ALL = check libici.so librfx.so rfxclock $(UTILITIES) $(TESTPGMS)

all:		$(ALL)

check:		$(ICIINCLS) $(PUBINCLS)
		rm -f *.o
		touch check

clean:
		rm -f *.o
		rm -f $(ALL)
		rm -f ./lib/*
		rm -f ./bin/*

install:
		cp ../include/*.h $(ROOT)/include
		cp lib/* $(ROOT)/lib
		cp bin/* $(ROOT)/bin

#	-	-	Utility executables	-	-	-	-

sdrwatch:	sdrwatch.o libici.so
		$(CC) -o sdrwatch sdrwatch.o -L./lib -lici -lpthread
		cp sdrwatch ./bin

sdrmend:	sdrmend.o libici.so
		$(CC) -o sdrmend sdrmend.o -L./lib -lici -lpthread
		cp sdrmend ./bin

psmwatch:	psmwatch.o libici.so
		$(CC) -o psmwatch psmwatch.o -L./lib -lici -lpthread
		cp psmwatch ./bin

ionadmin:	ionadmin.o libici.so librfx.so
		$(CC) -o ionadmin ionadmin.o -L./lib -lrfx -lici -lpthread
		cp ionadmin ./bin

#	-	-	Test executables	-	-	-	-

psmshell:	psmshell.o libici.so
		$(CC) -o psmshell psmshell.o -L./lib -lici -lpthread
		cp psmshell ./bin

smlistsh:	smlistsh.o libici.so
		$(CC) -o smlistsh smlistsh.o -L./lib -lici -lpthread
		cp smlistsh ./bin

file2sm:	file2sm.o libici.so
		$(CC) -I. -o file2sm file2sm.o -L./lib -lici -lpthread
		cp file2sm ./bin

sm2file:	sm2file.o libici.so
		$(CC) -I. -o sm2file sm2file.o -L./lib -lici -lpthread
		cp sm2file ./bin

file2sdr:	file2sdr.o libici.so
		$(CC) -I. -o file2sdr file2sdr.o -L./lib -lici -lpthread
		cp file2sdr ./bin

sdr2file:	sdr2file.o libici.so
		$(CC) -o sdr2file sdr2file.o -L./lib -lici -lpthread
		cp sdr2file ./bin

#	-	-	Daemon executable	-	-	-	-

rfxclock:	rfxclock.o libici.so librfx.so
		$(CC) -o rfxclock rfxclock.o -L./lib -lrfx -lici -lpthread
		cp rfxclock ./bin

#	-	-	Libraries	-	-	-	-	-

libici.so:	$(LIBICIOBJS)
		$(LD) -o libici.so $(LIBICIOBJS)
		cp libici.so ./lib

librfx.so:	rfx.o
		$(LD) -o librfx.so rfx.o
		cp librfx.so ./lib

#	-	-	Object modules	-	-	-	-	-

.SUFFIXES:	.c .o .h

.PATH.c:	$(SRC) $(SDR) $(UTILS) $(DAEMON) $(TEST)
.PATH.h:	$(INCL) $(SRC) $(SDR) $(UTILS) $(DAEMON) $(TEST)

.c.o:
		$(CC) -o $(.PREFIX).o -c $(.IMPSRC)
