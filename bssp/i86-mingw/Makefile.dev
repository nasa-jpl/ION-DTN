API = ../library
INCL = ../include
UTILS = ../utils
DAEMON = ../daemon
UDP = ../udp
TCP = ../tcp

OPT = -g -Wall -Dmingw
CC = gcc $(OPT) -I$(API) -I$(INCL) -I$(UDP) -I$(TCP) -I$(ROOT)/include
LDFLAGS = -fPIC -shared
LD = gcc $(LDFLAGS)

PUBINCLS = \
	$(INCL)/bssp.h

LTPINCLS = \
	$(API)/bsspP.h \
	$(UDP)/udpbsa.h \
	$(TCP)/tcpbsa.h

RUNTIMES = bsspadmin bsspclock udpbso udpbsi tcpbso tcpbsi

ALL = check libbssp.dll libtcpbsa.dll $(RUNTIMES)

all:		$(ALL)

check:		$(LTPINCLS) $(PUBINCLS)
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

bsspclock:	bsspclock.o libbssp.dll
		$(CC) -o bsspclock bsspclock.o -L./lib -L$(ROOT)/lib -lbssp -lici -lpthread -lm
		cp bsspclock ./bin

#	-	-	Utility executables	-	-	-	-

bsspadmin:	bsspadmin.o libbssp.dll
		$(CC) -o bsspadmin bsspadmin.o -L./lib -L$(ROOT)/lib -lbssp -lbssp -lici -lpthread -lm
		cp bsspadmin ./bin

#	-	-	UDP executables	-	-	-	-	-

udpbsi:		udpbsi.o libbssp.dll
		$(CC) -o udpbsi udpbsi.o -L./lib -L$(ROOT)/lib -lbssp -lici -lpthread -lws2_32 -lm
		cp udpbsi ./bin

udpbso:		udpbso.o libbssp.dll
		$(CC) -o udpbso udpbso.o -L./lib -L$(ROOT)/lib -lbssp -lici -lpthread -lws2_32 -lm
		cp udpbso ./bin

#	-	-	TCP executables-	-	-	-	-

tcpbsi:		tcpbsi.o libbssp.dll
		$(CC) -o tcpbsi tcpbsi.o -L./lib -L$(ROOT)/lib -ltcpbsa -lbssp -lici -lpthread -lws2_32 -lm
		cp tcpbsi ./bin

tcpbso:		tcpbso.o libbssp.dll
		$(CC) -o tcpbso tcpbso.o -L./lib -L$(ROOT)/lib -ltcpbsa -lbssp -lici -lpthread -lws2_32 -lm
		cp tcpbso ./bin

#	-	-	Libraries	-	-	-	-	-

libbssp.dll:	libbsspP.o libbssp.o
		$(LD) -o libbssp.dll libbsspP.o libbssp.o -Wl,--output-def,libbssp.def -L$(ROOT)/lib -lici -lpthread -lm
		cp libbssp.dll ./lib

libtcpbsa.dll:	libtcpbsa.o
		$(LD) -o libtcpbsa.dll libtcpbsa.o -Wl,--output-def,libtcpbsa.def -L./lib -L$(ROOT)/lib -lbssp -lici -lpthread -lws2_32 -lm
		cp libtcpbsa.dll ./lib

#	-	-	Object modules	-	-	-	-	-

%.o:		$(API)/%.c
		$(CC) -c $<

%.o:		$(UTILS)/%.c
		$(CC) -c $<

%.o:		$(DAEMON)/%.c
		$(CC) -c $<

%.o:		$(UDP)/%.c
		$(CC) -c $<

%.o:		$(TCP)/%.c
		$(CC) -c $<
