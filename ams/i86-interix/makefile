INCL = ../include
API = ../library
TEST = ../test
UTILS = ../utils
RAMS = ../rams

OPT = -g -Wall -Dinterix
CC = gcc $(OPT) -I$(API) -I$(INCL) -I$(RAMS) -I/opt/include -I/usr/local/include
LDFLAGS = -fPIC -shared
LD = gcc $(LDFLAGS)

LIBAMSOBJS = \
	libams.o \
	amscommon.o \
	loadmib.o \
	nullcrypt.o \
	dgrts.o \
	tcpts.o

AMSDOBJS = \
	amsd.o \
	libams.o \
	amscommon.o \
	loadmib.o \
	nullcrypt.o \
	dgrts.o \
	tcpts.o

RAMSTESTOBJS = \
	librams.o \
	ramscommmon.o \
	ramsgate.o

PUBINCLS = \
	$(INCL)/ams.h

RUNTIMES = amsd amshello amsshell amslog amslogprt amsbenchs amsbenchr ramsgate

ALL = check libams.so $(RUNTIMES)

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

#	-	-	Daemon executable	-	-	-	-

amsd:		$(AMSDOBJS) $(API)/amscommon.h
		$(CC) -o amsd $(AMSDOBJS) -L./lib -L/opt/lib -L/usr/local/lib -ldgr -lici -lpthread -lexpat
		cp amsd ./bin

#	-	-	Test executables	-	-	-	-

amshello:	amshello.o libams.so $(INCL)/ams.h
		$(CC) -o amshello amshello.o -L./lib -L/opt/lib -L/usr/local/lib -lams -ldgr -lici -lpthread -lexpat
		cp amshello ./bin

amsbenchs:	amsbenchs.o libams.so $(INCL)/ams.h
		$(CC) -o amsbenchs amsbenchs.o -L./lib -L/opt/lib -L/usr/local/lib -lams -ldgr -lici -lpthread -lexpat
		cp amsbenchs ./bin

amsbenchr:	amsbenchr.o libams.so $(INCL)/ams.h
		$(CC) -o amsbenchr amsbenchr.o -L./lib -L/opt/lib -L/usr/local/lib -lams -ldgr -lici -lpthread -lexpat
		cp amsbenchr ./bin

#	-	-	Utility executables	-	-	-	-

amsshell:	amsshell.o libams.so $(INCL)/ams.h
		$(CC) -o amsshell amsshell.o -L./lib -L/opt/lib -L/usr/local/lib -lams -ldgr -lici -lpthread -lexpat
		cp amsshell ./bin

amslog:		amslog.o libams.so $(INCL)/ams.h
		$(CC) -o amslog amslog.o -L./lib -L/opt/lib -L/usr/local/lib -lams -ldgr -lici -lpthread -lexpat
		cp amslog ./bin

amslogprt:	amslogprt.o
		$(CC) -o amslogprt amslogprt.o -L./lib -L/opt/lib
		cp amslogprt ./bin

#	-	-	RAMS executable 	-	-	-	-

ramsgate:	$(RAMSTESTOBJS) $(RAMS)/rams.h $(RAMS)/ramscommmon.h libams.so $(INCL)/ams.h
		$(CC) -o ramsgate $(RAMSTESTOBJS) -L./lib -L/opt/lib -L/usr/local/lib -lion -lionfw -lams -ldgr -lici -lpthread -lexpat
		cp ramsgate ./bin

#	-	-	Libraries	-	-	-	-	-

libams.so:	$(LIBAMSOBJS) $(API)/amsP.h
		$(LD) -o libams.so $(LIBAMSOBJS)
		cp libams.so ./lib

#	-	-	Object modules	-	-	-	-	-

.SUFFIXES:	.c .o .h

.PATH.c:	$(API) $(TEST) $(UTILS) $(RAMS)
.PATH.h:	$(INCL) $(API) $(TEST) $(RAMS)

.c.o:
		$(CC) -o $(.PREFIX).o -c $(.IMPSRC)
