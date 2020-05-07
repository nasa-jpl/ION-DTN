SRC = ../library
INCL = ../include
TEST = ../test
UTILS = ../utils
DAEMON = ../daemon
SDR = ../sdr
CRYPTO = ../crypto/NULL_SUITES
BULK = ../bulk/STUB_BULK

OPT = -g -Wall -Dmingw -DVNAME=ION-OPEN-SOURCE-4.0.0
CC = gcc $(OPT) -I$(SRC) -I$(TEST) -I$(SDR) -I$(INCL) -I../crypto -I../../$(BPv)/include/ -I../../$(BPv)/library/ -I../../ltp/include/ -I../../ltp/library/
LDFLAGS = -fPIC -shared
LD = gcc $(LDFLAGS)

LIBICIOBJS = \
	cbor.o \
	crc.o \
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
	csi.o \
	bulk.o \
	zco.o \
	sdrxn.o \
	sdrmgt.o \
	sdrstring.o \
	sdrlist.o \
	sdrtable.o \
	sdrhash.o \
	sdrcatlg.o

PUBINCLS = \
	$(INCL)/cbor.h \
	$(INCL)/crc.h \
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
	$(INCL)/csi.h \
	$(INCL)/bulk.h \
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

UTILITIES = sdrmend sdrwatch psmwatch ionadmin ionsecadmin ionlog ionwarn ionunlock ionxnowner

TESTPGMS = file2sm sm2file file2sdr sdr2file psmshell smlistsh smrbtsh owltsim
# owlttb - requires SIGPIPE which is not supported by Windows

ALL = check libici.dll rfxclock killm winion $(UTILITIES) $(TESTPGMS)

all:		$(ALL)

check:		$(ICIINCLS) $(PUBINCLS)
		rm -f *.o
		touch check

clean:
		rm -f *.o
		rm -f *.exe
		rm -f *.def
		rm -f $(ALL)
		rm -f ./lib/*
		rm -f ./bin/*

install:
		cp ../include/*.h $(ROOT)/include
		cp lib/* $(ROOT)/lib
		cp bin/* $(ROOT)/bin

#	-	-	Utility executables	-	-	-	-

sdrwatch:	sdrwatch.o libici.dll
		$(CC) -o sdrwatch sdrwatch.o -L./lib -lici -lpthread -lm
		cp sdrwatch ./bin

sdrmend:	sdrmend.o libici.dll
		$(CC) -o sdrmend sdrmend.o -L./lib -lici -lpthread -lm
		cp sdrmend ./bin

psmwatch:	psmwatch.o libici.dll
		$(CC) -o psmwatch psmwatch.o -L./lib -lici -lpthread -lm
		cp psmwatch ./bin

ionadmin:	ionadmin.o libici.dll
		$(CC) -o ionadmin ionadmin.o -L./lib -lici -lpthread -lm
		cp ionadmin ./bin

ionwarn:	ionwarn.o libici.dll
		$(CC) -o ionwarn ionwarn.o -L./lib -lici -lpthread -lm
		cp ionwarn ./bin

ionsecadmin:	ionsecadmin.o libici.dll
		$(CC) -o ionsecadmin ionsecadmin.o -L./lib -lici -lpthread -lm
		cp ionsecadmin ./bin

killm:		killm.o libici.dll
		$(CC) -o killm killm.o -L./lib -lici -lpthread -lm
		cp killm ./bin

ionunlock:	ionunlock.o libici.dll
		$(CC) -o ionunlock ionunlock.o -L./lib -lici -lpthread -lm
		cp ionunlock ./bin

ionxnowner:	ionxnowner.o libici.dll
		$(CC) -o ionxnowner ionxnowner.o -L./lib -lici -lpthread -lm
		cp ionxnowner ./bin
		
ionlog:	ionlog.o libici.dll
		$(CC) -o ionlog ionlog.o -L./lib -lici -lpthread -lm
		cp ionlog ./bin

#	-	-	Test executables	-	-	-	-

psmshell:	psmshell.o libici.dll
		$(CC) -o psmshell psmshell.o -L./lib -lici -lpthread -lm
		cp psmshell ./bin

smlistsh:	smlistsh.o libici.dll
		$(CC) -o smlistsh smlistsh.o -L./lib -lici -lpthread -lm
		cp smlistsh ./bin

smrbtsh:	smrbtsh.o libici.dll
		$(CC) -o smrbtsh smrbtsh.o -L./lib -lici -lpthread -lm
		cp smrbtsh ./bin

file2sm:	file2sm.o libici.dll
		$(CC) -I. -o file2sm file2sm.o -L./lib -lici -lpthread -lm
		cp file2sm ./bin

sm2file:	sm2file.o libici.dll
		$(CC) -I. -o sm2file sm2file.o -L./lib -lici -lpthread -lm
		cp sm2file ./bin

file2sdr:	file2sdr.o libici.dll
		$(CC) -I. -o file2sdr file2sdr.o -L./lib -lici -lpthread -lm
		cp file2sdr ./bin

sdr2file:	sdr2file.o libici.dll
		$(CC) -o sdr2file sdr2file.o -L./lib -lici -lpthread -lm
		cp sdr2file ./bin

owltsim:	owltsim.o libici.dll
		$(CC) -o owltsim owltsim.o -L./lib -lici -lpthread -lws2_32 -lm
		cp owltsim ./bin

owlttb:		owlttb.o libici.dll
		$(CC) -o owlttb owlttb.o -L./lib -lici -lpthread -lm
		cp owlttb ./bin

#	-	-	Daemon executables	-	-	-	-

rfxclock:	rfxclock.o libici.dll
		$(CC) -I. -o rfxclock rfxclock.o -L./lib -lici -lpthread -lm
		cp rfxclock ./bin

winion:		winion.o libici.dll
		$(CC) -I. -o winion winion.o -L./lib -lici -lpthread -lm
		cp winion ./bin

#	-	-	Libraries	-	-	-	-	-

libici.dll:	$(LIBICIOBJS) libicinm.o
		$(LD) -o libici.dll $(LIBICIOBJS) libicinm.o -Wl,--output-def,libici.def -lpthread -lws2_32 -lm
		cp libici.dll ./lib

#	-	-	Object modules	-	-	-	-	-

%.o:		$(SRC)/%.c
		$(CC) -c $<

%.o:		$(SDR)/%.c
		$(CC) -c $<

%.o:		$(CRYPTO)/%.c
		$(CC) -c $<

%.o:		$(BULK)/%.c
		$(CC) -c $<

%.o:		$(UTILS)/%.c
		$(CC) -c $<

%.o:		$(DAEMON)/%.c
		$(CC) -c $<

%.o:		$(TEST)/%.c
		$(CC) -c $<


