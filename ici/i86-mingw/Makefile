SRC = ../library
INCL = ../include
TEST = ../test
UTILS = ../utils
DAEMON = ../daemon
SDR = ../sdr

OPT = -g -Wall -Dmingw
CC = gcc $(OPT) -I$(SRC) -I$(TEST) -I$(SDR) -I$(INCL)
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
	smrbt.o \
	sptrace.o \
	ion.o \
	rfx.o \
	ionsec.o \
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

TESTPGMS = file2sm sm2file file2sdr sdr2file psmshell smlistsh smrbtsh owltsim
# owlttb

ALL = check libici.dll rfxclock killm winion $(UTILITIES) $(TESTPGMS)

all:		$(ALL)

check:		$(ICIINCLS) $(PUBINCLS)
		rm -f *.o
		touch check

clean:
		rm -f *.o
		rm -f *.exe
		rm -f $(ALL)
		rm -f ./lib/*
		rm -f ./bin/*

install:
		cp ../include/*.h $(ROOT)/include
		cp lib/* $(ROOT)/lib
		cp bin/* $(ROOT)/bin

#	-	-	Utility executables	-	-	-	-

sdrwatch:	sdrwatch.o libici.dll
		$(CC) -o sdrwatch sdrwatch.o -L./lib -lici -lpthread
		cp sdrwatch ./bin

sdrmend:	sdrmend.o libici.dll
		$(CC) -o sdrmend sdrmend.o -L./lib -lici -lpthread
		cp sdrmend ./bin

psmwatch:	psmwatch.o libici.dll
		$(CC) -o psmwatch psmwatch.o -L./lib -lici -lpthread
		cp psmwatch ./bin

ionadmin:	ionadmin.o libici.dll
		$(CC) -o ionadmin ionadmin.o -L./lib -lici -lpthread
		cp ionadmin ./bin

ionwarn:	ionwarn.o libici.dll
		$(CC) -o ionwarn ionwarn.o -L./lib -lici -lpthread
		cp ionwarn ./bin

ionsecadmin:	ionsecadmin.o libici.dll
		$(CC) -o ionsecadmin ionsecadmin.o -L./lib -lici -lpthread
		cp ionsecadmin ./bin

killm:		killm.o libici.dll
		$(CC) -o killm killm.o -L./lib -lici -lpthread
		cp killm ./bin

#	-	-	Test executables	-	-	-	-

psmshell:	psmshell.o libici.dll
		$(CC) -o psmshell psmshell.o -L./lib -lici -lpthread
		cp psmshell ./bin

smlistsh:	smlistsh.o libici.dll
		$(CC) -o smlistsh smlistsh.o -L./lib -lici -lpthread
		cp smlistsh ./bin

smrbtsh:	smrbtsh.o libici.dll
		$(CC) -o smrbtsh smrbtsh.o -L./lib -lici -lpthread
		cp smrbtsh ./bin

file2sm:	file2sm.o libici.dll
		$(CC) -I. -o file2sm file2sm.o -L./lib -lici -lpthread
		cp file2sm ./bin

sm2file:	sm2file.o libici.dll
		$(CC) -I. -o sm2file sm2file.o -L./lib -lici -lpthread
		cp sm2file ./bin

file2sdr:	file2sdr.o libici.dll
		$(CC) -I. -o file2sdr file2sdr.o -L./lib -lici -lpthread
		cp file2sdr ./bin

sdr2file:	sdr2file.o libici.dll
		$(CC) -o sdr2file sdr2file.o -L./lib -lici -lpthread
		cp sdr2file ./bin

owltsim:	owltsim.o libici.dll
		$(CC) -o owltsim owltsim.o -L./lib -lici -lpthread -lws2_32
		cp owltsim ./bin

owlttb:		owlttb.o libici.dll
		$(CC) -o owlttb owlttb.o -L./lib -lici -lpthread
		cp owlttb ./bin

#	-	-	Daemon executables	-	-	-	-

rfxclock:	rfxclock.o libici.dll
		$(CC) -I. -o rfxclock rfxclock.o -L./lib -lici -lpthread
		cp rfxclock ./bin

winion:		winion.o libici.dll
		$(CC) -I. -o winion winion.o -L./lib -lici -lpthread
		cp winion ./bin

#	-	-	Libraries	-	-	-	-	-

libici.dll:	$(LIBICIOBJS) libicinm.o
		$(LD) -o libici.dll $(LIBICIOBJS) libicinm.o -lpthread -lws2_32
		cp libici.dll ./lib

#	-	-	Object modules	-	-	-	-	-

%.o:		$(SRC)/%.c
		$(CC) -c $<

%.o:		$(SDR)/%.c
		$(CC) -c $<

%.o:		$(UTILS)/%.c
		$(CC) -c $<

%.o:		$(DAEMON)/%.c
		$(CC) -c $<

%.o:		$(TEST)/%.c
		$(CC) -c $<
