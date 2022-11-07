SRC = ../library
INCL = ../include
TEST = ../test
UTILS = ../utils
DAEMON = ../daemon
SDR = ../sdr
CRYPTO = ../crypto

# OPT = -O -Dfreebsd
OPT = -g -Wall -Werror -Dfreebsd
CC = cc $(OPT) -I$(SRC) -I$(TEST) -I$(SDR) -I$(INCL) -I$(CRYPTO)
LDFLAGS = -fPIC -shared
LD = ld $(LDFLAGS)

LIBICIOBJS = \
	llcv.o \
	platform.o \
	platform_sm.o \
	memmgr.o \
	lyst.o \
	psm.o \
	smlist.o \
	smrbt.o \
	sptrace.o \
	ion.o \
	rfx.o \
	ionsec.o \
	crypto.o \
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
	$(INCL)/smrbt.h \
	$(INCL)/sptrace.h \
	$(INCL)/ion.h \
	$(INCL)/rfx.h \
	$(INCL)/ionsec.h \
	$(INCL)/crypto.h \
	$(INCL)/zco.h \
	$(INCL)/sdrxn.h \
	$(INCL)/sdrmgt.h \
	$(INCL)/sdrstring.h \
	$(INCL)/sdrlist.h \
	$(INCL)/sdrtable.h \
	$(INCL)/sdrhash.h \
	$(INCL)/sdr.h

ICIINCLS = \
	$(SRC)/lystP.h \
	$(SDR)/sdrP.h

UTILITIES = sdrwatch psmwatch ionadmin ionsecadmin sdrmend ionwarn

TESTPGMS = file2sm sm2file file2sdr sdr2file psmshell smlistsh smrbtsh owltsim owlttb

ALL = check libici.so libicinm.so rfxclock $(UTILITIES) $(TESTPGMS)

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

ionadmin:	ionadmin.o libici.so
		$(CC) -o ionadmin ionadmin.o -L./lib -lici -lpthread
		cp ionadmin ./bin

ionwarn:	ionwarn.o libici.so
		$(CC) -o ionwarn ionwarn.o -L./lib -lici -lpthread
		cp ionwarn ./bin

ionsecadmin:	ionsecadmin.o libici.so
		$(CC) -o ionsecadmin ionsecadmin.o -L./lib -lici -lpthread
		cp ionsecadmin ./bin

#	-	-	Test executables	-	-	-	-

psmshell:	psmshell.o libici.so
		$(CC) -o psmshell psmshell.o -L./lib -lici -lpthread
		cp psmshell ./bin

smlistsh:	smlistsh.o libici.so
		$(CC) -o smlistsh smlistsh.o -L./lib -lici -lpthread
		cp smlistsh ./bin

smrbtsh:	smrbtsh.o libici.so
		$(CC) -o smrbtsh smrbtsh.o -L./lib -lici -lpthread
		cp smrbtsh ./bin

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

owltsim:	owltsim.o libici.so
		$(CC) -o owltsim owltsim.o -L./lib -lici -lpthread
		cp owltsim ./bin

owlttb:		owlttb.o libici.so
		$(CC) -o owlttb owlttb.o -L./lib -lici -lpthread
		cp owlttb ./bin

#	-	-	Daemon executable	-	-	-	-

rfxclock:	rfxclock.o libici.so
		$(CC) -I. -o rfxclock rfxclock.o -L./lib -lici -lpthread
		cp rfxclock ./bin

#	-	-	Libraries	-	-	-	-	-

libici.so:	$(LIBICIOBJS)
		$(LD) -o libici.so $(LIBICIOBJS)
		cp libici.so ./lib

libicinm.so:	libicinm.o $(INCL)/icinm.h
		$(LD) -o libicinm.so libicinm.o
		cp libicinm.so ./lib

#	-	-	Object modules	-	-	-	-	-

%.o:		$(SRC)/%.c
		$(CC) -c $<

%.o:		$(SDR)/%.c
		$(CC) -c $<

%.o:		$(CRYPTO)/NULL_SUITES/%.c
		$(CC) -c $<

%.o:		$(UTILS)/%.c
		$(CC) -c $<

%.o:		$(DAEMON)/%.c
		$(CC) -c $<

%.o:		$(TEST)/%.c
		$(CC) -c $<
