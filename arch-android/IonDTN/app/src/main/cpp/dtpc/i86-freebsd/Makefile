INCL = ../include
API = ../library
TEST = ../test
DAEMON = ../daemon
UTILS = ../utils

OPT = -g -Wall -Werror -Dfreebsd
CC = cc $(OPT) -I$(API) -I$(INCL) -I$(ROOT)/include
LDFLAGS = -fPIC -shared
LD = ld $(LDFLAGS)

LIBDTPCOBJS = \
	libdtpc.o \
	libdtpcP.o

PUBINCLS = \
	$(INCL)/dtpc.h

RUNTIMES = dtpcd dtpcclock dtpcsend dtpcreceive dtpcadmin

ALL = check libdtpc.so $(RUNTIMES)

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

#	-	-	Daemon executables	-	-	-	-

dtpcd:		dtpcd.o libdtpc.so
		$(CC) -o dtpcd dtpcd.o -L./lib -L$(ROOT)/lib -ldtpc -lbp -lici -lpthread
		cp dtpcd ./bin

dtpcclock:	dtpcclock.o libdtpc.so
		$(CC) -o dtpcclock dtpcclock.o -L./lib -L$(ROOT)/lib -ldtpc -lbp -lici -lpthread
		cp dtpcclock ./bin

#	-	-	Utility executables	-	-	-	-

dtpcadmin:	dtpcadmin.o libdtpc.so
		$(CC) -o dtpcadmin dtpcadmin.o -L./lib -L$(ROOT)/lib -ldtpc -lbp -lici -lpthread
		cp dtpcadmin ./bin

#	-	-	Test executables	-	-	-	-

dtpcsend:	dtpcsend.o libdtpc.so
		$(CC) -o dtpcsend dtpcsend.o -L./lib -L$(ROOT)/lib -ldtpc -lbp -lici -lpthread
		cp dtpcsend ./bin

dtpcreceive:	dtpcreceive.o libdtpc.so
		$(CC) -o dtpcreceive dtpcreceive.o -L./lib -L$(ROOT)/lib -ldtpc -lbp -lici -lpthread
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
