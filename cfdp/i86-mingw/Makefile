API = ../library
INCL = ../include
UTILS = ../utils
DAEMON = ../daemon
BP = ../bp
TEST = ../test

OPT = -g -Wall -Dmingw
CC = gcc $(OPT) -I$(API) -I$(INCL) -I$(ROOT)/include -DNO_PROXY -DNO_DIRLIST
LDFLAGS = -fPIC -shared
LD = gcc $(LDFLAGS)

PUBINCLS = \
	$(INCL)/cfdp.h

CFDPINCLS = \
	$(API)/cfdpP.h

LBP = -lbp

RUNTIMES = cfdpadmin cfdpclock bputa cfdptest

ALL = check libcfdp.dll $(RUNTIMES)

all:		$(ALL)

check:		$(CFDPINCLS) $(PUBINCLS)
		rm -f *.o
		touch check

clean:
		rm -f *.o
		rm -f *.exe
		rm -f $(ALL)
		rm -f ./lib/*
		rm -f ./bin/*

install:
		cp ../include/* $(ROOT)/include
		cp lib/* $(ROOT)/lib
		cp bin/* $(ROOT)/bin

#	-	-	Daemon executables	-	-	-	-

cfdpclock:	cfdpclock.o libcfdp.dll
		$(CC) -o cfdpclock cfdpclock.o -L./lib -L$(ROOT)/lib -lcfdp $(LBP) -lici -lpthread
		cp cfdpclock ./bin

#	-	-	Utility executables	-	-	-	-

cfdpadmin:	cfdpadmin.o libcfdp.dll
		$(CC) -o cfdpadmin cfdpadmin.o -L./lib -L$(ROOT)/lib -lcfdp $(LBP) -lici -lpthread
		cp cfdpadmin ./bin

#	-	-	Test executables	-	-	-	-

cfdptest:	cfdptest.o libcfdp.dll libcfdp.dll
		$(CC) -o cfdptest cfdptest.o -L./lib -L$(ROOT)/lib -lcfdp -lcfdp $(LBP) -lici -lpthread
		cp cfdptest ./bin

#	-	-	BP executables	-	-	-	-	-

bputa:		bputa.o libcfdp.dll
		$(CC) -o bputa bputa.o -L./lib -L$(ROOT)/lib -lcfdp $(LBP) -lici -lpthread
		cp bputa ./bin

#	-	-	Libraries	-	-	-	-	-

libcfdp.dll:	libcfdpP.o libcfdp.o libcfdpops.o
		$(LD) -o libcfdp.dll libcfdpP.o libcfdp.o libcfdpops.o -L$(ROOT)/lib -lici -lpthread -lws2_32
		cp libcfdp.dll ./lib

#	-	-	Object modules	-	-	-	-	-

%.o:		$(API)/%.c
		$(CC) -c $<

%.o:		$(UTILS)/%.c
		$(CC) -c $<

%.o:		$(TEST)/%.c
		$(CC) -c $<

%.o:		$(DAEMON)/%.c
		$(CC) -c $<

%.o:		$(BP)/%.c
		$(CC) -c $<
