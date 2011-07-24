INCL = ../include
API = ../library
TEST = ../test
UTILS = ../utils
RAMS = ../rams

OPT = -g -Wall -Dmingw -DUDPTS -DTCPTS -DDGRTS -DNOEXPAT
CC = gcc $(OPT) -I$(API) -I$(INCL) -I$(RAMS) -I$(ROOT)/include
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

LBP = -lbp

RUNTIMES = amsd amsshell amslog amslogprt amsbenchs amsbenchr ramsgate amsstop amsmib
# amshello

ALL = check libams.dll $(RUNTIMES)

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

#	-	-	Daemon executable	-	-	-	-

amsd:		$(AMSDOBJS) $(API)/amscommon.h
		$(CC) -o amsd $(AMSDOBJS) -L./lib -L$(ROOT)/lib -ldgr -lici -lpthread -lws2_32
		cp amsd ./bin

#	-	-	Test executables	-	-	-	-

amshello:	amshello.o libams.dll $(INCL)/ams.h
		$(CC) -o amshello amshello.o -L./lib -L$(ROOT)/lib -lams -ldgr -lici -lpthread -lws2_32
		cp amshello ./bin

amsbenchs:	amsbenchs.o libams.dll $(INCL)/ams.h
		$(CC) -o amsbenchs amsbenchs.o -L./lib -L$(ROOT)/lib -lams -ldgr -lici -lpthread -lws2_32
		cp amsbenchs ./bin

amsbenchr:	amsbenchr.o libams.dll $(INCL)/ams.h
		$(CC) -o amsbenchr amsbenchr.o -L./lib -L$(ROOT)/lib -lams -ldgr -lici -lpthread -lws2_32
		cp amsbenchr ./bin

#	-	-	Utility executables	-	-	-	-

amsshell:	amsshell.o libams.dll $(INCL)/ams.h
		$(CC) -o amsshell amsshell.o -L./lib -L$(ROOT)/lib -lams -ldgr -lici -lpthread -lws2_32
		cp amsshell ./bin

amslog:		amslog.o libams.dll $(INCL)/ams.h
		$(CC) -o amslog amslog.o -L./lib -L$(ROOT)/lib -lams -ldgr -lici -lpthread -lws2_32
		cp amslog ./bin

amslogprt:	amslogprt.o
		$(CC) -o amslogprt amslogprt.o -L$(ROOT)/lib
		cp amslogprt ./bin

amsstop:	amsstop.o
		$(CC) -o amsstop amsstop.o -L./lib -L$(ROOT)/lib -lams -ldgr -lici -lpthread -lws2_32
		cp amsstop ./bin

amsmib:		amsmib.o
		$(CC) -o amsmib amsmib.o -L./lib -L$(ROOT)/lib -lams -ldgr -lici -lpthread -lws2_32
		cp amsmib ./bin

#	-	-	RAMS executable		-	-	-	-

ramsgate:	$(RAMSTESTOBJS) $(RAMS)/rams.h $(RAMS)/ramscommon.h libams.dll $(INCL)/ams.h
		$(CC) -o ramsgate $(RAMSTESTOBJS) -L./lib -L$(ROOT)/lib $(LBP) -lams -ldgr -lici -lpthread -lws2_32
		cp ramsgate ./bin

#	-	-	Libraries	-	-	-	-	-

libams.dll:	$(LIBAMSOBJS) $(API)/amsP.h
		$(LD) -o libams.dll $(LIBAMSOBJS) -L$(ROOT)/lib -ldgr -lici -lpthread -lws2_32
		cp libams.dll ./lib

#	-	-	Object modules	-	-	-	-	-

%.o:		$(API)/%.c
		$(CC) -c $<

%.o:		$(TEST)/%.c
		$(CC) -c $<

%.o:		$(UTILS)/%.c
		$(CC) -c $<

%.o:		$(RAMS)/%.c
		$(CC) -c $<
