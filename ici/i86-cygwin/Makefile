SRC = ../library
INCL = ../include
TEST = ../test

#	OPT = -g -Wall -Dcygwin -Dnoipc
OPT = -g -Wall -Werror -Dcygwin
CC = gcc $(OPT) -I$(SRC) -I$(TEST) -I$(INCL)
LDFLAGS = -shared
LD = gcc $(LDFLAGS)

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

TESTPGMS = psmshell.exe smlistsh.exe sdrwatch.exe
# file2sdr.exe sdr2file.exe file2sm.exe sm2file.exe

ALL = check libici.dll sdrd.exe smunwedge.exe $(TESTPGMS)

all:		$(ALL)

check:		$(ICIINCLS) $(PUBINCLS)
		rm -f *.o
		touch check

clean:
		rm -f *.o
		rm -f *.a
		rm -f $(ALL)
		rm -f ./lib/*.a
		rm -f ./bin/*.dll
		rm -f ./bin/*.exe

install:
		cp ../include/*.h $(ROOT)/include
		cp lib/*.a $(ROOT)/lib
		cp bin/*.dll /c/windows/system32
		cp bin/*.exe /c/windows/system32

#       -       -       Executables     -       -       -       -       -

sdrd.exe:	sdrd.o libici.dll
		$(CC) -o sdrd.exe sdrd.o -L./lib -lici -lpthread
		cp sdrd.exe ./bin

smunwedge.exe:	smunwedge.o libici.dll
		$(CC) -o smunwedge.exe smunwedge.o -L./lib -lici -lpthread
		cp smunwedge.exe ./bin

#	-	-	Test executables	-	-	-	-

psmshell.exe:	psmshell.o libici.dll
		$(CC) -o psmshell.exe psmshell.o -L./lib -lici -lpthread
		cp psmshell.exe ./bin

smlistsh.exe:	smlistsh.o libici.dll
		$(CC) -o smlistsh.exe smlistsh.o -L./lib -lici -lpthread
		cp smlistsh.exe ./bin

sdrwatch.exe:	sdrwatch.o libici.dll
		$(CC) -o sdrwatch.exe sdrwatch.o -L./lib -lici -lpthread
		cp sdrwatch.exe ./bin

file2sdr.exe:	file2sdr.o libici.dll
		$(CC) -I. -o file2sdr.exe file2sdr.o -L./lib -lici -lpthread
		cp file2sdr.exe ./bin

sdr2file.exe:	sdr2file.o libici.dll
		$(CC) -o sdr2file.exe sdr2file.o -L./lib -lici -lpthread
		cp sdr2file.exe ./bin

file2sm.exe:	file2sm.o libici.dll
		$(CC) -I. -o file2sm.exe file2sm.o -L./lib -lici -lpthread
		cp file2sm.exe ./bin

sm2file.exe:	sm2file.o libici.dll
		$(CC) -I. -o sm2file.exe sm2file.o -L./lib -lici -lpthread
		cp sm2file.exe ./bin

#	-	-	Library		-	-	-	-	-

libici.dll:	$(LIBICIOBJS)
		$(LD) -o libici.dll $(LIBICIOBJS) -Wl,--out-implib,libici.a
		cp libici.dll ./bin
		cp libici.a ./lib

#	-	-	Object modules	-	-	-	-	-

%.o:		$(SRC)/%.c
		$(CC) -c $<

%.o:		$(TEST)/%.c
		$(CC) -c $<
