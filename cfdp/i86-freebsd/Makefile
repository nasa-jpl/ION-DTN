API = ../library
INCL = ../include
UTILS = ../utils
DAEMON = ../daemon
BP = ../bp
TEST = ../test

# OPT = -O -Dfreebsd
OPT = -g -Wall -Werror -Dfreebsd
# -DNO_PROXY -DNO_DIRLIST
CC = cc $(OPT) -I$(API) -I$(INCL) -I$(ROOT)/include
LDFLAGS = -fPIC -shared
LD = ld $(LDFLAGS)

PUBINCLS = \
	$(INCL)/cfdp.h

CFDPINCLS = \
	$(API)/cfdpP.h \
	$(UTILS)/bpcp.h

LBP = -lbp

RUNTIMES = cfdpadmin cfdpclock bputa cfdptest bpcp bpcpd

ALL = check libcfdp.so $(RUNTIMES)

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

cfdpclock:	cfdpclock.o libcfdp.so
		$(CC) -o cfdpclock cfdpclock.o -L./lib -L$(ROOT)/lib -lcfdp $(LBP) -lici -lpthread
		cp cfdpclock ./bin

#	-	-	Utility executables	-	-	-	-

cfdpadmin:	cfdpadmin.o libcfdp.so
		$(CC) -o cfdpadmin cfdpadmin.o -L./lib -L$(ROOT)/lib -lcfdp $(LBP) -lici -lpthread
		cp cfdpadmin ./bin

#	-	-	Test executables	-	-	-	-

cfdptest:	cfdptest.o libcfdp.so
		$(CC) -o cfdptest cfdptest.o -L./lib -L$(ROOT)/lib -lcfdp -lcfdp $(LBP) -lici -lpthread
		cp cfdptest ./bin

#	-	-	BP executables	-	-	-	-	-

bputa:		bputa.o libcfdp.so
		$(CC) -o bputa bputa.o -L./lib -L$(ROOT)/lib -lcfdp $(LBP) -lici -lpthread
		cp bputa ./bin
		
#	-	-	BPCP executables	-	-	-	-	-
bpcp: 		bpcp.o
		$(CC) -o bpcp bpcp.o -L./lib -L$(ROOT)/lib -lici -lcfdp -lpthread -I$(UTILS)
		cp bpcp ./bin
		
bpcpd: 		bpcpd.o
		$(CC) -o bpcpd bpcpd.o -L./lib -L$(ROOT)/lib -lici -lcfdp -lpthread -I$(UTILS)
		cp bpcpd ./bin

#	-	-	Libraries	-	-	-	-	-

libcfdp.so:	libcfdp.o libcfdpP.o libcfdpops.o
		$(LD) -o libcfdp.so libcfdp.o libcfdpP.o libcfdpops.o
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
