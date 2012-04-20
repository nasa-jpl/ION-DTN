INCL = ../include
API = ../library
TEST = ../test

OPT = -g -Wall -Dmingw
CC = gcc $(OPT) -I$(API) -I$(TEST) -I$(INCL) -I$(ROOT)/include
LDFLAGS = -fPIC -shared
LD = gcc $(LDFLAGS)

LIBBSSOBJS = \
	libbss.o \
	libbssP.o

PUBINCLS = \
	$(INCL)/bss.h

RUNTIMES = bssrecv bssStreamingApp

ALL = check libbss.dll $(RUNTIMES)

all:		$(ALL)

check:		$(PUBINCLS)
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

#	-	-	Test executables	-	-	-	-

bssrecv:	bssrecv.o libbss.dll
		$(CC) -o bssrecv bssrecv.o -L./lib -L$(ROOT)/lib -lbss -lbp -lici -lpthread -lws2_32
		cp bssrecv ./bin

bssStreamingApp:	bssStreamingApp.o libbss.dll
		$(CC) -o bssStreamingApp bssStreamingApp.o -L./lib -L$(ROOT)/lib -lbss -lbp -lici -lpthread -lws2_32
		cp bssStreamingApp ./bin

#	-	-	Libraries	-	-	-	-	-

libbss.dll:	$(LIBBSSOBJS) $(API)/bssP.h
		$(LD) -o libbss.dll $(LIBBSSOBJS) -L$(ROOT)/lib -lbp -lici -lpthread -lws2_32
		cp libbss.dll ./lib

#	-	-	Object modules	-	-	-	-	-

%.o:		$(API)/%.c
		$(CC) -c $<

%.o:		$(TEST)/%.c
		$(CC) -c $<
