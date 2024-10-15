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

LBP = -lbp -limcfw -lipnfw -ldtn2fw

RUNTIMES = bssrecv bssStreamingApp bsscounter bssdriver

ALL = check libbss.dll $(RUNTIMES)

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

bssrecv:	bssrecv.o libbss.dll
		$(CC) -o bssrecv bssrecv.o -L./lib -L$(ROOT)/lib -lbss $(LBP) -lici -lpthread -lws2_32 -lm
		cp bssrecv ./bin

bssStreamingApp:	bssStreamingApp.o libbss.dll
		$(CC) -o bssStreamingApp bssStreamingApp.o -L./lib -L$(ROOT)/lib -lbss $(LBP) -lici -lpthread -lws2_32 -lm
		cp bssStreamingApp ./bin

bsscounter:	bsscounter.o libbss.dll
		$(CC) -o bsscounter bsscounter.o -L./lib -L$(ROOT)/lib -lbss $(LBP) -lici -lpthread -lws2_32 -lm
		cp bsscounter ./bin

bssdriver:	bssdriver.o libbss.dll
		$(CC) -o bssdriver bssdriver.o -L./lib -L$(ROOT)/lib -lbss $(LBP) -lici -lpthread -lws2_32 -lm
		cp bssdriver ./bin

#	-	-	Libraries	-	-	-	-	-

libbss.dll:	$(LIBBSSOBJS) $(API)/bssP.h
		$(LD) -o libbss.dll $(LIBBSSOBJS) -Wl,--output-def,libbss.def -L$(ROOT)/lib $(LBP) -lici -lpthread -lws2_32 -lm
		cp libbss.dll ./lib

#	-	-	Object modules	-	-	-	-	-

%.o:		$(API)/%.c
		$(CC) -c $<

%.o:		$(TEST)/%.c
		$(CC) -c $<
