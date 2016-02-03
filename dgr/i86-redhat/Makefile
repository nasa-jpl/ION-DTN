INCL = ../include
API = ../library
TEST = ../test

# OPT = -O -Dlinux
# OPT = -g -Wall -Dlinux -DINITIAL_RETARD=2
OPT = -g -Wall -Werror -Dlinux -DHEAP_PTRS=$(PTRS)
CC = gcc $(OPT) -I$(API) -I$(TEST) -I$(INCL) -I$(ROOT)/include
LDFLAGS = -fPIC -shared -rdynamic
LD = gcc $(LDFLAGS)

LIBDGROBJS = \
	libdgr.o

PUBINCLS = \
	$(INCL)/dgr.h

RUNTIMES = file2dgr dgr2file file2tcp tcp2file file2udp udp2file

ALL = check libdgr.so $(RUNTIMES)

all:		$(ALL)

check:		$(PUBINCLS)
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

#	-	-	Test executables	-	-	-	-

file2dgr:	file2dgr.o libdgr.so $(TEST)/file2dgr.h
		$(CC) -o file2dgr file2dgr.o -L./lib -L$(ROOT)/lib -ldgr -lici -lpthread -lm
		cp file2dgr ./bin

dgr2file:	dgr2file.o libdgr.so $(TEST)/file2dgr.h
		$(CC) -o dgr2file dgr2file.o -L./lib -L$(ROOT)/lib -ldgr -lici -lpthread -lm
		cp dgr2file ./bin

tcp2file:	tcp2file.o $(TEST)/file2tcp.h
		$(CC) -o tcp2file tcp2file.o -L$(ROOT)/lib -L./lib -lici -lpthread -lm
		cp tcp2file ./bin

file2tcp:	file2tcp.o $(TEST)/file2tcp.h
		$(CC) -o file2tcp file2tcp.o -L$(ROOT)/lib -L./lib -lici -lpthread -lm
		cp file2tcp ./bin

udp2file:	udp2file.o $(TEST)/file2udp.h
		$(CC) -o udp2file udp2file.o -L$(ROOT)/lib -L./lib -lici -lpthread -lm
		cp udp2file ./bin

file2udp:	file2udp.o $(TEST)/file2udp.h
		$(CC) -o file2udp file2udp.o -L$(ROOT)/lib -L./lib -lici -lpthread -lm
		cp file2udp ./bin

#	-	-	Libraries	-	-	-	-	-

libdgr.so:	$(LIBDGROBJS)
		$(LD) -o libdgr.so $(LIBDGROBJS)
		cp libdgr.so ./lib

#	-	-	Object modules	-	-	-	-	-

%.o:		$(API)/%.c
		$(CC) -c $<

%.o:		$(TEST)/%.c
		$(CC) -c $<
