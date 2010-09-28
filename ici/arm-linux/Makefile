SRC = ../library
INCL = ../include
TEST = ../test

# OPT = -O -Dlinux
OPT = -g -Wall -Werror -Dlinux
# -DDEBUG=1

ARMROOT = /arm
ARMDIR = /usr/local/arm/2.95.3
CC = $(ARMDIR)/bin/arm-linux-gcc $(OPT) -I$(ARMDIR)/include -I$(SRC) -I$(TEST) -I$(INCL)
LDFLAGS = -fPIC -shared
LD = $(ARMDIR)/bin/arm-linux-gcc $(LDFLAGS)

LIBICIOBJS = \
	llcv.o \
	vf.o \
	search.o \
	platform.o \
	platform_sm.o \
	memmgr.o \
	list.o \
	smlist.o \
	psm.o \
	sptrace.o \
	sdrs.o \
	sdr_dram.o \
	sdr_file.o \
	sdr.o

PUBINCLS = \
	$(INCL)/llcv.h \
	$(INCL)/vf.h \
	$(INCL)/search.h \
	$(INCL)/platform.h \
	$(INCL)/platform_sm.h \
	$(INCL)/memmgr.h \
	$(INCL)/icilist.h \
	$(INCL)/smlist.h \
	$(INCL)/psm.h \
	$(INCL)/sptrace.h \
	$(INCL)/sdr.h

ICIINCLS = \
	$(SRC)/listP.h \
	$(SRC)/sdrP.h

TESTPGMS = psmshell smlistsh file2sdr sdr2file file2sm sm2file sdrwatch

ALL = check libici.so sdrd smunwedge $(TESTPGMS)

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
		cp ../include/* $(ARMROOT)/include
		cp lib/* $(ARMROOT)/lib
		cp bin/* $(ARMROOT)/bin

#	-	-	Executables	-	-	-	-	-

sdrd:		sdrd.o libici.so
		$(CC) -o sdrd sdrd.o -L./lib -L$(ARMDIR)/lib -L$(ARMROOT)/lib -lici -lpthread
		cp sdrd ./bin

smunwedge:	smunwedge.o libici.so
		$(CC) -o smunwedge smunwedge.o -L./lib -L$(ARMDIR)/lib -L$(ARMROOT)/lib -lici -lpthread
		cp smunwedge ./bin

#	-	-	Test executables	-	-	-	-

psmshell:	psmshell.o libici.so
		$(CC) -o psmshell psmshell.o -L./lib -L$(ARMDIR)/lib -L$(ARMROOT)/lib -lici -lpthread
		cp psmshell ./bin

smlistsh:	smlistsh.o libici.so
		$(CC) -o smlistsh smlistsh.o -L./lib -L$(ARMDIR)/lib -L$(ARMROOT)/lib -lici -lpthread
		cp smlistsh ./bin

sdrwatch:	sdrwatch.o libici.so
		$(CC) -o sdrwatch sdrwatch.o -L./lib -L$(ARMDIR)/lib -L$(ARMROOT)/lib -lici -lpthread
		cp sdrwatch ./bin

file2sdr:	file2sdr.o libici.so
		$(CC) -I. -o file2sdr file2sdr.o -L./lib -L$(ARMDIR)/lib -L$(ARMROOT)/lib -lici -lpthread
		cp file2sdr ./bin

sdr2file:	sdr2file.o libici.so
		$(CC) -o sdr2file sdr2file.o -L./lib -L$(ARMDIR)/lib -L$(ARMROOT)/lib -lici -lpthread
		cp sdr2file ./bin

file2sm:	file2sm.o libici.so
		$(CC) -I. -o file2sm file2sm.o -L./lib -L$(ARMDIR)/lib -L$(ARMROOT)/lib -lici -lpthread
		cp file2sm ./bin

sm2file:	sm2file.o libici.so
		$(CC) -I. -o sm2file sm2file.o -L./lib -L$(ARMDIR)/lib -L$(ARMROOT)/lib -lici -lpthread
		cp sm2file ./bin

#	-	-	Library		-	-	-	-	-

libici.so:	$(LIBICIOBJS)
		$(LD) -o libici.so $(LIBICIOBJS)
		cp libici.so ./lib

#	-	-	Object modules	-	-	-	-	-

%.o:		$(SRC)/%.c
		$(CC) -c $<

%.o:		$(TEST)/%.c
		$(CC) -c $<
