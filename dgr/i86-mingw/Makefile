INCL = ../include
API = ../library
TEST = ../test

# OPT = -g -Wall -Dmingw -DINITIAL_RETARD=2
OPT = -g -Wall -Dmingw
CC = gcc $(OPT) -I$(API) -I$(TEST) -I$(INCL) -I$(ROOT)/include
LDFLAGS = -fPIC -shared
LD = gcc $(LDFLAGS)

LIBDGROBJS = \
	libdgr.o

PUBINCLS = \
	$(INCL)/dgr.h

RUNTIMES = file2dgr dgr2file file2tcp tcp2file
# file2udp udp2file

ALL = check libdgr.dll $(RUNTIMES)

all:		$(ALL)

check:		$(PUBINCLS)
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

#	-	-	Test executables	-	-	-	-

file2dgr:	file2dgr.o libdgr.dll $(TEST)/file2dgr.h
		$(CC) -o file2dgr file2dgr.o -L./lib -L$(ROOT)/lib -ldgr -lici -lpthread -lws2_32 -lm
		cp file2dgr ./bin

dgr2file:	dgr2file.o libdgr.dll $(TEST)/file2dgr.h
		$(CC) -o dgr2file dgr2file.o -L./lib -L$(ROOT)/lib -ldgr -lici -lpthread -lws2_32 -lm
		cp dgr2file ./bin

tcp2file:	tcp2file.o $(TEST)/file2tcp.h
		$(CC) -o tcp2file tcp2file.o -L$(ROOT)/lib -L./lib -lici -lpthread -lws2_32 -lm
		cp tcp2file ./bin

file2tcp:	file2tcp.o $(TEST)/file2tcp.h
		$(CC) -o file2tcp file2tcp.o -L$(ROOT)/lib -L./lib -lici -lpthread -lws2_32 -lm
		cp file2tcp ./bin

udp2file:	udp2file.o $(TEST)/file2udp.h
		$(CC) -o udp2file udp2file.o -L$(ROOT)/lib -L./lib -lici -lpthread -lws2_32 -lm
		cp udp2file ./bin

file2udp:	file2udp.o $(TEST)/file2udp.h
		$(CC) -o file2udp file2udp.o -L$(ROOT)/lib -L./lib -lici -lpthread -lws2_32 -lm
		cp file2udp ./bin

#	-	-	Libraries	-	-	-	-	-

libdgr.dll:	$(LIBDGROBJS)
		$(LD) -o libdgr.dll $(LIBDGROBJS) -Wl,--output-def,libdgr.def -L$(ROOT)/lib -lici -lpthread -lws2_32 -lm
		cp libdgr.dll ./lib

#	-	-	Object modules	-	-	-	-	-

%.o:		$(API)/%.c
		$(CC) -c $<

%.o:		$(TEST)/%.c
		$(CC) -c $<
