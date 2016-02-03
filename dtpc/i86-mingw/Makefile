API = ../library
INCL = ../include
UTILS = ../utils
DAEMON = ../daemon
TEST = ../test

OPT = -g -Wall -Dmingw
CC = gcc $(OPT) -I$(API) -I$(INCL) -I$(ROOT)/include
LDFLAGS = -fPIC -shared
LD = gcc $(LDFLAGS)

LIBDTPCOBJS = \
	libdtpc.o \
	libdtpcP.o

PUBINCLS = \
	$(INCL)/dtpc.h

LBP = -lbp -limcfw -lipnfw -ldtn2fw

RUNTIMES = dtpcd dtpcclock dtpcsend dtpcreceive dtpcadmin

ALL = check libdtpc.dll $(RUNTIMES)

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

#	-	-	Daemon executables	-	-	-	-

dtpcd:		dtpcd.o libdtpc.dll
		$(CC) -o dtpcd dtpcd.o -L./lib -L$(ROOT)/lib -ldtpc -lici -lbp -lpthread -lm
		cp dtpcd ./bin

dtpcclock:	dtpcclock.o libdtpc.dll
		$(CC) -o dtpcclock dtpcclock.o -L./lib -L$(ROOT)/lib -ldtpc -lici -lbp -lpthread -lm
		cp dtpcclock ./bin

#	-	-	Utility executables	-	-	-	-

dtpcadmin:	dtpcadmin.o libdtpc.dll
		$(CC) -o dtpcadmin dtpcadmin.o -L./lib -L$(ROOT)/lib -ldtpc -lici -lbp -lpthread -lm
		cp dtpcadmin ./bin

#	-	-	Test executables	-	-	-	-

dtpcsend:	dtpcsend.o libdtpc.dll
		$(CC) -o dtpcsend dtpcsend.o -L./lib -L$(ROOT)/lib -ldtpc -lici -lbp -lpthread -lm
		cp dtpcsend ./bin

dtpcreceive:	dtpcreceive.o libdtpc.dll
		$(CC) -o dtpcreceive dtpcreceive.o -L./lib -L$(ROOT)/lib -ldtpc -lici -lbp -lpthread -lm
		cp dtpcreceive ./bin

#	-	-	Libraries	-	-	-	-	-

libdtpc.dll:	$(LIBDTPCOBJS) $(API)/dtpcP.h
		$(LD) -o libdtpc.dll $(LIBDTPCOBJS) -Wl,--output-def,libdtpc.def -L$(ROOT)/lib -lici -lbp -lpthread -lm
		cp libdtpc.dll ./lib

#	-	-	Object modules	-	-	-	-	-

%.o:		$(API)/%.c
		$(CC) -c $<

%.o:		$(UTILS)/%.c
		$(CC) -c $<

%.o:		$(TEST)/%.c
		$(CC) -c $<

%.o:		$(DAEMON)/%.c
		$(CC) -c $<

