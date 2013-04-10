SRC = ../library
INCL = ../include
TEST = ../test
UTILS = ../utils
DAEMON = ../daemon
SDR = ../sdr

OPT = -g -Wall -Werror -Dsol5 -Dunix -D__SVR4 -D_REENTRANT -fPIC
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
	sdrcatlg.o \
	smrbt.o

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
	$(INCL)/ionsec.h \
	$(INCL)/sdrxn.h \
	$(INCL)/sdrmgt.h \
	$(INCL)/sdrstring.h \
	$(INCL)/sdrlist.h \
	$(INCL)/sdrtable.h \
	$(INCL)/sdrhash.h \
	$(INCL)/sdr.h \
	$(INCL)/smrbt.h

ICIINCLS = \
	$(SRC)/lystP.h \
	$(SDR)/sdrP.h

UTILITIES = sdrwatch psmwatch ionadmin sdrmend ionsecadmin

TESTPGMS = file2sm sm2file file2sdr sdr2file psmshell smlistsh owltsim owlttb

ALL = check libici.so rfxclock $(UTILITIES) $(TESTPGMS)

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
		$(CC) -o sdrwatch sdrwatch.o -L./lib -lici -lpthread -lrt -lsocket
		cp sdrwatch ./bin

sdrmend:	sdrmend.o libici.so
		$(CC) -o sdrmend sdrmend.o -L./lib -lici -lpthread -lrt -lsocket
		cp sdrmend ./bin

psmwatch:	psmwatch.o libici.so
		$(CC) -o psmwatch psmwatch.o -L./lib -lici -lpthread -lrt -lsocket
		cp psmwatch ./bin

ionadmin:	ionadmin.o libici.so
		$(CC) -o ionadmin ionadmin.o -L./lib -lici -lpthread -lrt -lsocket
		cp ionadmin ./bin

ionsecadmin:	ionsecadmin.o libici.so
		$(CC) -o ionsecadmin ionsecadmin.o -L./lib -lici -lpthread -lrt -lsocket
		cp ionsecadmin ./bin

#	-	-	Test executables	-	-	-	-

psmshell:	psmshell.o libici.so
		$(CC) -o psmshell psmshell.o -L./lib -lici -lpthread -lrt -lsocket
		cp psmshell ./bin

smlistsh:	smlistsh.o libici.so
		$(CC) -o smlistsh smlistsh.o -L./lib -lici -lpthread -lrt -lsocket
		cp smlistsh ./bin

file2sm:	file2sm.o libici.so
		$(CC) -I. -o file2sm file2sm.o -L./lib -lici -lpthread -lrt -lsocket
		cp file2sm ./bin

sm2file:	sm2file.o libici.so
		$(CC) -I. -o sm2file sm2file.o -L./lib -lici -lpthread -lrt -lsocket
		cp sm2file ./bin

file2sdr:	file2sdr.o libici.so
		$(CC) -I. -o file2sdr file2sdr.o -L./lib -lici -lpthread -lrt -lsocket
		cp file2sdr ./bin

sdr2file:	sdr2file.o libici.so
		$(CC) -o sdr2file sdr2file.o -L./lib -lici -lpthread -lrt -lsocket
		cp sdr2file ./bin

owltsim:	owltsim.o libici.so
		$(CC) -o owltsim owltsim.o -L./lib -lici -lpthread -lrt -lsocket
		cp owltsim ./bin

owlttb:		owlttb.o libici.so
		$(CC) -o owlttb owlttb.o -L./lib -lici -lpthread -lrt -lnsl -lsocket
		cp owlttb ./bin

#	-	-	Daemon executable	-	-	-	-

rfxclock:	rfxclock.o libici.so
		$(CC) -I. -o rfxclock rfxclock.o -L./lib -lici -lpthread -lrt -lsocket
		cp rfxclock ./bin

#	-	-	Libraries	-	-	-	-	-

libici.so:	$(LIBICIOBJS)
		$(LD) -o libici.so $(LIBICIOBJS)
		cp libici.so ./lib

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
