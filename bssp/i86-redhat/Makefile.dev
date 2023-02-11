API = ../library
INCL = ../include
UTILS = ../utils
DAEMON = ../daemon
UDP = ../udp
TCP = ../tcp

# OPT = -O -Dlinux
OPT = -g -Wall -Werror -Dlinux -DHEAP_PTRS=$(PTRS)
CC = gcc $(OPT) -I$(API) -I$(INCL) -I$(UDP) -I$(TCP) -I$(ROOT)/include
LDFLAGS = -fPIC -shared -rdynamic
LD = gcc $(LDFLAGS)

PUBINCLS = \
	$(INCL)/bssp.h

LTPINCLS = \
	$(API)/bsspP.h \
	$(UDP)/udpbsa.h \
	$(TCP)/tcpbsa.h

RUNTIMES = bsspadmin bsspclock udpbso udpbsi tcpbso tcpbsi

ALL = check libbsspP.so libbssp.so libtcpbsa.so $(RUNTIMES)

all:		$(ALL)

check:		$(LTPINCLS) $(PUBINCLS)
		rm -f *.o
		touch check

clean:
		rm -f *.o
		rm -f $(ALL)
		rm -f ./lib/*
		rm -f ./bin/*

install:
		cp ../include/* $(ROOT)/include
		cp lib/* $(ROOT)/lib
		cp bin/* $(ROOT)/bin

#	-	-	Daemon executables	-	-	-	-

bsspclock:	bsspclock.o libbsspP.so
		$(CC) -o bsspclock bsspclock.o -L./lib -L$(ROOT)/lib -lbsspP -lici -lpthread -lm
		cp bsspclock ./bin

#	-	-	Utility executables	-	-	-	-

bsspadmin:	bsspadmin.o libbsspP.so
		$(CC) -o bsspadmin bsspadmin.o -L./lib -L$(ROOT)/lib -lbssp -lbsspP -lici -lpthread -lm
		cp bsspadmin ./bin

#	-	-	UDP executables	-	-	-	-	-

udpbsi:		udpbsi.o libbsspP.so
		$(CC) -o udpbsi udpbsi.o -L./lib -L$(ROOT)/lib -lbsspP -lici -lpthread -lm
		cp udpbsi ./bin

udpbso:		udpbso.o libbsspP.so
		$(CC) -o udpbso udpbso.o -L./lib -L$(ROOT)/lib -lbsspP -lici -lpthread -lm
		cp udpbso ./bin

#	-	-	TCP executables-	-	-	-	-
tcpbsi:		tcpbsi.o libbsspP.so
		$(CC) -o tcpbsi tcpbsi.o -L./lib -L$(ROOT)/lib -ltcpbsa -lbsspP -lici -lpthread -lm
		cp tcpbsi ./bin

tcpbso:		tcpbso.o libbsspP.so
		$(CC) -o tcpbso tcpbso.o -L./lib -L$(ROOT)/lib -ltcpbsa -lbsspP -lici -lpthread -lm
		cp tcpbso ./bin

#	-	-	Libraries	-	-	-	-	-

libbsspP.so:	libbsspP.o
		$(LD) -o libbsspP.so libbsspP.o
		cp libbsspP.so ./lib

libbssp.so:	libbssp.o
		$(LD) -o libbssp.so libbssp.o
		cp libbssp.so ./lib

libtcpbsa.so:	libtcpbsa.o
		$(LD) -o libtcpbsa.so libtcpbsa.o
		cp libtcpbsa.so ./lib

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
