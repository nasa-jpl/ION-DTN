INCL = ../include
API = ../library
TEST = ../test

OPT = -g -Wall -Werror -Dunix -Ddarwin
CC = gcc $(OPT) -I$(API) -I$(TEST) -I$(INCL) -I$(ROOT)/include
LDFLAGS = -fPIC -shared
LD = gcc $(LDFLAGS)

LIBBSSOBJS = \
	libbss.o \
	libbssP.o

PUBINCLS = \
	$(INCL)/bss.h

RUNTIMES = bssrecv bssStreamingApp

ALL = check libbss.so $(RUNTIMES)

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

bssrecv:	bssrecv.o libbss.so
		$(CC) -o bssrecv bssrecv.o -L./lib -L$(ROOT)/lib -lbss -lbp -lici -lpthread
		cp bssrecv ./bin

bssStreamingApp:	bssStreamingApp.o libbss.so
		$(CC) -o bssStreamingApp bssStreamingApp.o -L./lib -L$(ROOT)/lib -lbss -lbp -lici -lpthread
		cp bssStreamingApp ./bin

#	-	-	Libraries	-	-	-	-	-

libbss.so:	$(LIBBSSOBJS) $(API)/bssP.h
		$(LD) -o libbss.so $(LIBBSSOBJS)
		cp libbss.so ./lib

#	-	-	Object modules	-	-	-	-	-

%.o:		$(API)/%.c
		$(CC) -c $<

%.o:		$(TEST)/%.c
		$(CC) -c $<
