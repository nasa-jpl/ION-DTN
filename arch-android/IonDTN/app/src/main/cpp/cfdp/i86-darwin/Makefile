API = ../library
INCL = ../include
UTILS = ../utils
DAEMON = ../daemon
BP = ../bp
TEST = ../test

# OPT = -O -Dlinux
OPT = -g -Wall -Werror -Dunix -Ddarwin
CC = gcc $(OPT) -I$(API) -I$(INCL) -I$(ROOT)/include
LDFLAGS = -dynamiclib -undefined dynamic_lookup
LD = gcc $(LDFLAGS)

PUBINCLS = \
	$(INCL)/cfdp.h

CFDPINCLS = \
	$(API)/cfdpP.h

LBP = -lbp -lbpP

RUNTIMES = cfdpadmin cfdpclock bputa cfdptest bpcp bpcpd

ALL = check libcfdpP.so libcfdp.so $(RUNTIMES)

all:		$(ALL)

check:		$(CFDPINCLS) $(PUBINCLS)
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

cfdpclock:	cfdpclock.o libcfdpP.so
		$(CC) -o cfdpclock cfdpclock.o -L./lib -L$(ROOT)/lib -lcfdp -lcfdpP $(LBP) -lici -lpthread
		cp cfdpclock ./bin

#	-	-	Utility executables	-	-	-	-

cfdpadmin:	cfdpadmin.o libcfdpP.so
		$(CC) -o cfdpadmin cfdpadmin.o -L./lib -L$(ROOT)/lib -lcfdp -lcfdpP $(LBP) -lici -lpthread
		cp cfdpadmin ./bin

#	-	-	Test executables	-	-	-	-

cfdptest:	cfdptest.o libcfdpP.so libcfdp.so
		$(CC) -o cfdptest cfdptest.o -L./lib -L$(ROOT)/lib -lcfdp -lcfdp -lcfdpP $(LBP) -lici -lpthread
		cp cfdptest ./bin

#	-	-	BP executables	-	-	-	-	-

bputa:		bputa.o libcfdpP.so
		$(CC) -o bputa bputa.o -L./lib -L$(ROOT)/lib -lcfdp -lcfdpP $(LBP) -lici -lpthread
		cp bputa ./bin
		
#	-	-	BPCP executables	-	-	-	-	-
bpcp: bpcp.o bpcp.h
		$(CC) -o bpcp bpcp.o -L./lib -L$(ROOT)/lib -lici -lcfdp -lpthread -I$(UTILS)
		cp bpcp ./bin
		
bpcpd: bpcpd.o bpcp.h
		$(CC) -o bpcpd bpcpd.o -L./lib -L$(ROOT)/lib -lici -lcfdp -lpthread -I$(UTILS)
		cp bpcpd ./bin

#	-	-	Libraries	-	-	-	-	-

libcfdpP.so:	libcfdpP.o
		$(LD) -o libcfdpP.so libcfdpP.o
		cp libcfdpP.so ./lib

libcfdp.so:	libcfdp.o libcfdpops.o
		$(LD) -o libcfdp.so libcfdp.o libcfdpops.o
		cp libcfdp.so ./lib

#	-	-	Object modules	-	-	-	-	-

%.o:		$(API)/%.c
		$(CC) -c $<

%.o:		$(UTILS)/%.c
		$(CC) -c $<

%.o:		$(TEST)/%.c
		$(CC) -c $<

%.o:		$(DAEMON)/%.c
		$(CC) -c $<

%.o:		$(BP)/%.c
		$(CC) -c $<
