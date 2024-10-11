INCL = ../include
ICI_INCL = ../../ici/include
ICI_LIB = ../../ici/i86_64-fedora/lib
BP_LIB = ../../bpv7/i86_64-fedora/lib
API = ../library
TEST = ../test

OPT = -g -Wall -Werror -Dlinux -fPIC -DSPACE_ORDER=3
CC = gcc $(OPT) $(ADD_FLAGS) -I$(API) -I$(TEST) -I$(INCL) -I$(ROOT)/include -I$(ICI_INCL) -I../../bpv7/include -I../../ici/library -I../../bpv7/library -L$(ICI_LIB) -L$(BP_LIB)
LDFLAGS = -fPIC -shared
LD = gcc $(LDFLAGS)

LIBBSSOBJS = \
	libbss.o \
	libbssP.o

PUBINCLS = \
	$(INCL)/bss.h

LBP = -lbp

RUNTIMES = bssrecv bssStreamingApp bsscounter bssdriver

ALL = check libbss.so $(RUNTIMES)

all:		$(ALL)

check:		$(PUBINCLS)
		rm -f *.o
		touch check

clean:
		rm -f *.o
		rm -f *.so
		rm -f $(ALL)
		rm -f ./lib/*
		rm -f ./bin/*

install:
		cp ../include/* $(ROOT)/include
		cp lib/* $(ROOT)/lib
		cp bin/* $(ROOT)/bin

uninstall:		
		rm $(ROOT)/lib/libbss.so
		for file in $(PUBINCLS); \
			do rm $(ROOT)/include/$(notdir $$file); done
		for file in $(RUNTIMES); \
			do rm $(ROOT)/bin/$(notdir $$file); done

#	-	-	Test executables	-	-	-	-

bssrecv:	bssrecv.o libbss.so
		$(CC) -o bssrecv bssrecv.o -L./lib -L$(ROOT)/lib -lbss $(LBP) -lici -lpthread -lm
		cp bssrecv ./bin

bssStreamingApp:	bssStreamingApp.o libbss.so
		$(CC) -o bssStreamingApp bssStreamingApp.o -L./lib -L$(ROOT)/lib -lbss $(LBP) -lici -lpthread -lm
		cp bssStreamingApp ./bin

bsscounter:	bsscounter.o libbss.so
		$(CC) -o bsscounter bsscounter.o -L./lib -L$(ROOT)/lib -lbss $(LBP) -lici -lpthread -lm
		cp bsscounter ./bin

bssdriver:	bssdriver.o libbss.so
		$(CC) -o bssdriver bssdriver.o -L./lib -L$(ROOT)/lib -lbss $(LBP) -lici -lpthread -lm
		cp bssdriver ./bin

#	-	-	Libraries	-	-	-	-	-

libbss.so:	$(LIBBSSOBJS) $(API)/bssP.h
		$(LD) -o libbss.so $(LIBBSSOBJS)
		cp libbss.so ./lib

#	-	-	Object modules	-	-	-	-	-

%.o:		$(API)/%.c
		$(CC) -c $<

%.o:		$(TEST)/%.c
		$(CC) -c $<
