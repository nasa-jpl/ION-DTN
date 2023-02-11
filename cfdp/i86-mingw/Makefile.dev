API = ../library
INCL = ../include
UTILS = ../utils
DAEMON = ../daemon
BP = ../bp
TCP = ../tcp
TEST = ../test

OPT = -g -Wall -Dmingw
# -DNO_PROXY -DNO_DIRLIST
CC = gcc $(OPT) -I$(API) -I$(INCL) -I$(ROOT)/include
LDFLAGS = -fPIC -shared
LD = gcc $(LDFLAGS)

PUBINCLS = \
	$(INCL)/cfdp.h

CFDPINCLS = \
	$(API)/cfdpP.h \
	$(INCL)/bputa.h
#	$(UTILS)/bpcp.h \

LBP = -lbp -limcfw -lipnfw -ldtn2fw

RUNTIMES = cfdpadmin cfdpclock bputa tcputa cfdptest
# bpcp bpcpd - rely on system calls not implemented in mingw

ALL = check libcfdp.dll $(RUNTIMES)

all:		$(ALL)

check:		$(CFDPINCLS) $(PUBINCLS)
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
		cp ../include/* $(ROOT)/include
		cp lib/* $(ROOT)/lib
		cp bin/* $(ROOT)/bin

#	-	-	Daemon executables	-	-	-	-

cfdpclock:	cfdpclock.o libcfdp.dll
		$(CC) -o cfdpclock cfdpclock.o -L./lib -L$(ROOT)/lib -lcfdp $(LBP) -lici -lpthread -lm
		cp cfdpclock ./bin

#	-	-	Utility executables	-	-	-	-

cfdpadmin:	cfdpadmin.o libcfdp.dll
		$(CC) -o cfdpadmin cfdpadmin.o -L./lib -L$(ROOT)/lib -lcfdp $(LBP) -lici -lpthread -lm
		cp cfdpadmin ./bin

#	-	-	Test executables	-	-	-	-

cfdptest:	cfdptest.o libcfdp.dll
		$(CC) -o cfdptest cfdptest.o -L./lib -L$(ROOT)/lib -lcfdp -lcfdp $(LBP) -lici -lpthread -lm
		cp cfdptest ./bin

#	-	-	BP executables	-	-	-	-	-

bputa:		bputa.o libcfdp.dll
		$(CC) -o bputa bputa.o -L./lib -L$(ROOT)/lib -lcfdp $(LBP) -lici -lpthread -lm
		cp bputa ./bin

#	-	-	TCP executables	-	-	-	-	-

tcputa:		tcputa.o libcfdp.dll
		$(CC) -o tcputa tcputa.o -L./lib -L$(ROOT)/lib -lcfdp -lici -lpthread -lws2_32 -lm
		cp tcputa ./bin

#	-	-	BPCP executables	-	-	-	-

bpcp:		bpcp.o
		$(CC) -o bpcp bpcp.o -L./lib -L$(ROOT)/lib -lici -lcfdp -lpthread -I$(UTILS) -lm
		cp bpcp ./bin

bpcpd:		bpcpd.o
		$(CC) -o bpcpd bpcpd.o -L./lib -L$(ROOT)/lib -lici -lcfdp -lpthread -I$(UTILS) -lm
		cp bpcpd ./bin

#	-	-	Libraries	-	-	-	-	-

libcfdp.dll:	libcfdpP.o libcfdp.o libcfdpops.o
		$(LD) -o libcfdp.dll libcfdpP.o libcfdp.o libcfdpops.o -Wl,--output-def,libcfdp.def -L$(ROOT)/lib -lici -lpthread -lws2_32 -lm
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

%.o:		$(TCP)/%.c
		$(CC) -c $<

