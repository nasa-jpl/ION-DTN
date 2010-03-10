API = ../library
INCL = ../include
UTILS = ../utils
TEST = ../test
DAEMON = ../daemon
IPN = ../ipn
TCP = ../tcp
DGR = ../dgr
BRS = ../brs

OPT = -g -Wall -Dinterix
CC = gcc $(OPT) -I$(API) -I$(INCL) -I$(ROOT)/include
LDFLAGS = -fPIC -shared
LD = gcc $(LDFLAGS)

PUBINCLS = \
	$(INCL)/bp.h

BPINCLS = \
	$(API)/bpP.h

IPNINCLS = \
	$(IPN)/ipnfw.h

CLINCLS = \
	$(BRS)/brscla.h \
	$(DGR)/dgrcla.h \
	$(TCP)/tcpcla.h

RUNTIMES = bpadmin bpsink bpsource bpdriver bpecho bpclock ipnadmin ipnfw ipnadminep tcpcli tcpclo brsscla brsccla
# dgrclo dgrcli

ALL = check libbpP.so libbp.so libipnfw.so libtcpcla.so $(RUNTIMES)

all:		$(ALL)

check:		$(BPINCLS) $(PUBINCLS) $(IPNINCLS) $(CLINCLS)
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

bpclock:	bpclock.o libbpP.so
		$(CC) -o bpclock bpclock.o -L./lib -L$(ROOT)/lib -lbp -lbpP -lici -lpthread
		cp bpclock ./bin

#	-	-	Utility executables	-	-	-	-

bpadmin:	bpadmin.o libbpP.so
		$(CC) -o bpadmin bpadmin.o -L./lib -L$(ROOT)/lib -lbp -lbpP -lici -lpthread
		cp bpadmin ./bin

ipnadmin:	ipnadmin.o libbpP.so libipnfw.so
		$(CC) -o ipnadmin ipnadmin.o -L./lib -L$(ROOT)/lib -lipnfw -lbp -lbpP -lici -lpthread
		cp ipnadmin ./bin

#	-	-	IPN executables	-	-	-	-	-

ipnfw:		ipnfw.o libbpP.so libipnfw.so
		$(CC) -o ipnfw ipnfw.o -L./lib -L$(ROOT)/lib -lipnfw -lbp -lbpP -lici -lpthread
		cp ipnfw ./bin

ipnadminep:	ipnadminep.o libbpP.so libipnfw.so
		$(CC) -o ipnadminep ipnadminep.o -L./lib -L$(ROOT)/lib -lipnfw -lbp -lbpP -lici -lpthread
		cp ipnadminep ./bin

#	-	-	TCP executables	-	-	-	-	-

tcpcli:		tcpcli.o libbpP.so libtcpcla.so
		$(CC) -o tcpcli tcpcli.o -L./lib -L$(ROOT)/lib -ltcpcla -lbp -lbpP -lici -lpthread
		cp tcpcli ./bin

tcpclo:		tcpclo.o libbpP.so libtcpcla.so
		$(CC) -o tcpclo tcpclo.o -L./lib -L$(ROOT)/lib -ltcpcla -lbp -lbpP -lici -lpthread
		cp tcpclo ./bin

#	-	-	DGR executables	-	-	-	-	-

dgrcli:		dgrcli.o libbpP.so
		$(CC) -o dgrcli dgrcli.o -L./lib -L$(ROOT)/lib -lbp -lbpP -ldgr -lici -lpthread
		cp dgrcli ./bin

dgrclo:		dgrclo.o libbpP.so libipnfw.so
		$(CC) -o dgrclo dgrclo.o -L./lib -L$(ROOT)/lib -lipnfw -lbp -lbpP -ldgr -lici -lpthread
		cp dgrclo ./bin

#	-	-	BRS executables	-	-	-	-

brsscla:	brsscla.o libbpP.so libtcpcla.so
		$(CC) -o brsscla brsscla.o -L./lib -L$(ROOT)/lib -ltcpcla -lbp -lbpP -lici -lpthread
		cp brsscla ./bin

brsccla:	brsccla.o libbpP.so libtcpcla.so
		$(CC) -o brsccla brsccla.o -L./lib -L$(ROOT)/lib -ltcpcla -lbp -lbpP -lici -lpthread
		cp brsccla ./bin

#	-	-	Test executables	-	-	-	-

bpsource:	bpsource.o libbp.so libbpP.so
		$(CC) -o bpsource bpsource.o -L./lib -L$(ROOT)/lib -lbp -lbpP -lici -lpthread
		cp bpsource ./bin

bpsink:	bpsink.o libbp.so libbpP.so
		$(CC) -o bpsink bpsink.o -L./lib -L$(ROOT)/lib -lbp -lbpP -lici -lpthread
		cp bpsink ./bin

bpdriver:	bpdriver.o libbp.so libbpP.so
		$(CC) -o bpdriver bpdriver.o -L./lib -L$(ROOT)/lib -lbp -lbpP -lici -lpthread
		cp bpdriver ./bin

bpecho:	bpecho.o libbp.so libbpP.so
		$(CC) -o bpecho bpecho.o -L./lib -L$(ROOT)/lib -lbp -lbpP -lici -lpthread
		cp bpecho ./bin

#	-	-	Libraries	-	-	-	-	-

libbpP.so:	libbpP.o
		$(LD) -o libbpP.so libbpP.o
		cp libbpP.so ./lib

libbp.so:	libbp.o
		$(LD) -o libbp.so libbp.o
		cp libbp.so ./lib

libipnfw.so:	libipnfw.o
		$(LD) -o libipnfw.so libipnfw.o
		cp libipnfw.so ./lib

libtcpcla.so:	libtcpcla.o
		$(LD) -o libtcpcla.so libtcpcla.o
		cp libtcpcla.so ./lib

#	-	-	Object modules	-	-	-	-	-

.SUFFIXES:	.c .o .h

.PATH.c:	$(API) $(DAEMON) $(UTILS) $(TEST) $(IPN) $(TCP) $(BRS)
.PATH.h:	$(INCL) $(API) $(DAEMON) $(UTILS) $(TEST) $(IPN) $(TCP) $(BRS)

.c.o:
		$(CC) -o $(.PREFIX).o -c $(.IMPSRC)
