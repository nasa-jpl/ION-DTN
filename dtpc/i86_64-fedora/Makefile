INCL = ../include
BP_INCL = ../../bpv7/include
ICI_INCL = ../../ici/include
ICI_LIB = ../../ici/i86_64-fedora/lib
BP_LIB = ../../bpv7/i86_64-fedora/lib
API = ../library
TEST = ../test
DAEMON = ../daemon
UTILS = ../utils

OPT = -g -Wall -Werror -Dlinux -fPIC
CC = gcc $(OPT) $(ADD_FLAGS) -I$(API) -I$(INCL) -I$(ROOT)/include -I$(BP_INCL) -I$(ICI_INCL) -I../../ici/library -I../../bpv7/library -L$(ICI_LIB) -L$(BP_LIB)
LDFLAGS = -fPIC -shared
LD = gcc $(LDFLAGS)

LIBDTPCOBJS = \
	libdtpc.o \
	libdtpcP.o

PUBINCLS = \
	$(INCL)/dtpc.h

LBP = -lbp

RUNTIMES = dtpcd dtpcclock dtpcsend dtpcreceive dtpcadmin

ALL = check libdtpc.so $(RUNTIMES)

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
		rm $(ROOT)/lib/libdtpc.so
		for file in $(PUBINCLS); \
			do rm $(ROOT)/include/$(notdir $$file); done
		for file in $(RUNTIMES); \
			do rm $(ROOT)/bin/$(notdir $$file); done

#	-	-	Daemon executables	-	-	-	-

dtpcd:		dtpcd.o libdtpc.so
		$(CC) -o dtpcd dtpcd.o -L./lib -L$(ROOT)/lib -ldtpc $(LBP) -lici -lpthread -lm
		cp dtpcd ./bin

dtpcclock:	dtpcclock.o libdtpc.so
		$(CC) -o dtpcclock dtpcclock.o -L./lib -L$(ROOT)/lib -ldtpc $(LBP) -lici -lpthread -lm
		cp dtpcclock ./bin

#	-	-	Utility executables	-	-	-	-

dtpcadmin:	dtpcadmin.o libdtpc.so
		$(CC) -o dtpcadmin dtpcadmin.o -L./lib -L$(ROOT)/lib -ldtpc $(LBP) -lici -lpthread -lm
		cp dtpcadmin ./bin

#	-	-	Test executables	-	-	-	-

dtpcsend:	dtpcsend.o libdtpc.so
		$(CC) -o dtpcsend dtpcsend.o -L./lib -L$(ROOT)/lib -ldtpc $(LBP) -lici -lpthread -lm
		cp dtpcsend ./bin

dtpcreceive:	dtpcreceive.o libdtpc.so
		$(CC) -o dtpcreceive dtpcreceive.o -L./lib -L$(ROOT)/lib -ldtpc $(LBP) -lici -lpthread -lm
		cp dtpcreceive ./bin

#	-	-	Libraries	-	-	-	-	-

libdtpc.so:	$(LIBDTPCOBJS) $(API)/dtpcP.h
		$(LD) -o libdtpc.so $(LIBDTPCOBJS)
		cp libdtpc.so ./lib

#	-	-	Object modules	-	-	-	-	-

%.o:		$(API)/%.c
		$(CC) -c $<

%.o:		$(TEST)/%.c
		$(CC) -c $<

%.o:		$(DAEMON)/%.c
		$(CC) -c $<

%.o:		$(UTILS)/%.c
		$(CC) -c $<
