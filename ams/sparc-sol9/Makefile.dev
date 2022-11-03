INCL = ../include
API = ../library
TEST = ../test
UTILS = ../utils
RAMS = ../rams

OPT = -g -Wall -Werror -Dsol5 -Dunix -D__SVR4 -D_REENTRANT -DUDPTS -DTCPTS -DDGRTS -fPIC
CC = gcc $(OPT) -I$(API) -I$(INCL) -I$(RAMS) -I$(ROOT)/include -I/tps/include
LDFLAGS = -fPIC -shared
LD = gcc $(LDFLAGS)

LIBAMSOBJS = \
	libams.o \
	amscommon.o \
	loadmib.o \
	crypt.o \
	dgrts.o \
	udpts.o \
	tcpts.o

AMSDOBJS = \
	amsd.o \
	libams.o \
	amscommon.o \
	loadmib.o \
	crypt.o \
	dgrts.o \
	udpts.o \
	tcpts.o

RAMSTESTOBJS = \
	librams.o \
	ramscommon.o \
	ramsgate.o

PUBINCLS = \
	$(INCL)/ams.h

LBP = -lbp -lbpP

RUNTIMES = amsd amshello amsshell amslog amslogprt amsbenchs amsbenchr ramsgate amsstop amsmib

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
		$(CC) -o amsd $(AMSDOBJS) -L./lib -L$(ROOT)/lib -ldgr -lici -lpthread -lexpat -lrt -lsocket
		cp amsd ./bin

#	-	-	Test executables	-	-	-	-

amshello:	amshello.o libams.so $(INCL)/ams.h
		$(CC) -o amshello amshello.o -L./lib -L$(ROOT)/lib -lams -ldgr -lici -lpthread -lexpat -lrt -lsocket
		cp amshello ./bin

amsbenchs:	amsbenchs.o libams.so $(INCL)/ams.h
		$(CC) -o amsbenchs amsbenchs.o -L./lib -L$(ROOT)/lib -lams -ldgr -lici -lpthread -lexpat -lrt -lsocket
		cp amsbenchs ./bin

amsbenchr:	amsbenchr.o libams.so $(INCL)/ams.h
		$(CC) -o amsbenchr amsbenchr.o -L./lib -L$(ROOT)/lib -lams -ldgr -lici -lpthread -lexpat -lrt -lsocket
		cp amsbenchr ./bin

#	-	-	Utility executables	-	-	-	-

amsshell:	amsshell.o libams.so $(INCL)/ams.h
		$(CC) -o amsshell amsshell.o -L./lib -L$(ROOT)/lib -lams -ldgr -lici -lpthread -lexpat -lrt -lsocket
		cp amsshell ./bin

amslog:		amslog.o libams.so $(INCL)/ams.h
		$(CC) -o amslog amslog.o -L./lib -L$(ROOT)/lib -lams -ldgr -lici -lpthread -lexpat -lrt -lsocket
		cp amslog ./bin

amslogprt:	amslogprt.o
		$(CC) -o amslogprt amslogprt.o -L./lib -L$(ROOT)/lib
		cp amslogprt ./bin

amsstop:	amsstop.o libams.so $(INCL)/ams.h
		$(CC) -o amsstop amsstop.o -L./lib -L$(ROOT)/lib -lams -ldgr -lici -lpthread -lexpat -lrt -lsocket
		cp amsstop ./bin

amsmib:		amsmib.o libams.so $(INCL)/ams.h
		$(CC) -o amsmib amsmib.o -L./lib -L$(ROOT)/lib -lams -ldgr -lici -lpthread -lexpat -lrt -lsocket
		cp amsmib ./bin

#	-	-	RAMS executable		-	-	-	-

ramsgate:	$(RAMSTESTOBJS) $(RAMS)/rams.h $(RAMS)/ramscommon.h libams.so $(INCL)/ams.h
		$(CC) -o ramsgate $(RAMSTESTOBJS) -L./lib -L$(ROOT)/lib $(LBP) -lams -ldgr -lici -lpthread -lexpat -lrt -lsocket
		cp ramsgate ./bin

#	-	-	Libraries	-	-	-	-	-

libams.so:	$(LIBAMSOBJS) $(API)/amsP.h
		$(LD) -o libams.so $(LIBAMSOBJS)
		cp libams.so ./lib

#	-	-	Object modules	-	-	-	-	-

%.o:		$(API)/%.c
		$(CC) -c $<

%.o:		$(TEST)/%.c
		$(CC) -c $<

%.o:		$(UTILS)/%.c
		$(CC) -c $<

%.o:		$(RAMS)/%.c
		$(CC) -c $<
