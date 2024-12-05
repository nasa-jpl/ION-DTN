API = ../library
INCL = ../include
UTILS = ../utils
TEST = ../test
DAEMON = ../daemon
CGR = ../cgr
IPN = ../ipn
DTN2 = ../dtn2
LTP = ../ltp
TCP = ../tcp
DGR = ../dgr
BRS = ../brs
UDP = ../udp
CRYPTO = $(API)/crypto/NULL_SUITES

OPT = -g -Wall -Werror -Dsol5 -Dunix -D__SVR4 -D_REENTRANT -fPIC
CC = gcc $(OPT) -I$(IPN) -I$(DTN2) -I$(API) -I$(INCL) -I$(ROOT)/include -DBP_EXTENDED
LDFLAGS = -fPIC -shared
LD = gcc $(LDFLAGS)

PUBINCLS = \
	$(INCL)/bp.h

BPINCLS = \
	$(API)/bpP.h \
	$(IPN)/ipnfw.h \
	$(DTN2)/dtn2fw.h \
	$(BRS)/brscla.h \
	$(UDP)/udpcla.h \
	$(TCP)/stcpcla.h \
	$(LTP)/ltpcla.h

BPLIBS = \
	libbp.so \
	libbpP.so 

BPPOBJS = libbpP.o ecos.o bei.o extbsputil.o extbspbab.o crypto.o
# phn.o
#
LFW = -lipnfw -ldtn2fw

LBP = -lbp -lbpP

RUNTIMES = bpadmin bpsink bpsource bpdriver bpecho bpcounter bpsendfile bprecvfile bpclock ipnadmin ipnfw ipnadminep dtn2admin dtn2fw dtn2adminep tcpcli tcpclo stcpcli stcpclo brsscla brsccla udpcli udpclo dgrcla ltpcli ltpclo lgsend lgagent bptrace bpstats bplist bpcancel hmackeys bping bpstats2 bpchat

ALL = check $(BPLIBS) libcgr.so libipnfw.so libdtn2fw.so libstcpcla.so libudpcla.so $(RUNTIMES)

all:		$(ALL)

check:		$(BPINCLS) $(PUBINCLS)
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

bpclock:	bpclock.o $(BPLIBS)
		$(CC) -o bpclock bpclock.o -L./lib -L$(ROOT)/lib $(LBP) -lici -lpthread -lrt -lsocket
		cp bpclock ./bin

#	-	-	Utility executables	-	-	-	-

bpadmin:	bpadmin.o $(BPLIBS)
		$(CC) -o bpadmin bpadmin.o -L./lib -L$(ROOT)/lib $(LBP) -lici -lpthread -lrt -lsocket
		cp bpadmin ./bin

ipnadmin:	ipnadmin.o $(BPLIBS) libipnfw.so
		$(CC) -o ipnadmin ipnadmin.o -L./lib -L$(ROOT)/lib -lipnfw $(LBP) -lici -lpthread -lrt -lsocket
		cp ipnadmin ./bin

dtn2admin:	dtn2admin.o $(BPLIBS) libdtn2fw.so
		$(CC) -o dtn2admin dtn2admin.o -L./lib -L$(ROOT)/lib -ldtn2fw $(LBP) -lici -lpthread -lrt -lsocket
		cp dtn2admin ./bin

lgsend:		lgsend.o $(BPLIBS)
		$(CC) -o lgsend lgsend.o -L./lib -L$(ROOT)/lib $(LBP) -lici -lpthread -lrt -lsocket
		cp lgsend ./bin

lgagent:	lgagent.o $(BPLIBS)
		$(CC) -o lgagent lgagent.o -L./lib -L$(ROOT)/lib $(LBP) -lici -lpthread -lrt -lsocket
		cp lgagent ./bin

bptrace:	bptrace.o $(BPLIBS)
		$(CC) -o bptrace bptrace.o -L./lib -L$(ROOT)/lib $(LBP) -lici -lpthread -lrt -lsocket
		cp bptrace ./bin

bpstats:	bpstats.o $(BPLIBS)
		$(CC) -o bpstats bpstats.o -L./lib -L$(ROOT)/lib $(LBP) -lici -lpthread -lrt -lsocket
		cp bpstats ./bin

bpstats2:	bpstats2.o $(BPLIBS)
		$(CC) -o bpstats2 bpstats2.o -L./lib -L$(ROOT)/lib $(LBP) -lici -lpthread -lrt -lsocket
		cp bpstats2 ./bin

bping:		bping.o $(BPLIBS)
		$(CC) -o bping bping.o -L./lib -L$(ROOT)/lib $(LBP) -lici -lpthread -lrt -lsocket
		cp bping ./bin

bpchat:		bpchat.o $(BPLIBS)
		$(CC) -o bpchat bpchat.o -L./lib -L$(ROOT)/lib $(LBP) -lici -lpthread -lrt -lsocket
		cp bpchat ./bin

bplist:		bplist.o $(BPLIBS)
		$(CC) -o bplist bplist.o -L./lib -L$(ROOT)/lib $(LBP) -lici -lpthread -lrt -lsocket
		cp bplist ./bin

bpcancel:	bpcancel.o $(BPLIBS)
		$(CC) -o bpcancel bpcancel.o -L./lib -L$(ROOT)/lib $(LBP) -lici -lpthread -lrt -lsocket
		cp bpcancel ./bin

hmackeys:	hmackeys.o $(BPLIBS)
		$(CC) -o hmackeys hmackeys.o -L$(ROOT)/lib -lici -lpthread -lrt -lsocket
		cp hmackeys ./bin

#	-	-	IPN executables	-	-	-	-	-

ipnfw:		ipnfw.o $(BPLIBS) libipnfw.so libcgr.so
		$(CC) -o ipnfw ipnfw.o -L./lib -L$(ROOT)/lib -lipnfw -lcgr $(LBP) -lici -lpthread -lrt -lsocket
		cp ipnfw ./bin

ipnadminep:	ipnadminep.o $(BPLIBS) libipnfw.so
		$(CC) -o ipnadminep ipnadminep.o -L./lib -L$(ROOT)/lib -lipnfw $(LBP) -lici -lpthread -lrt -lsocket
		cp ipnadminep ./bin

#	-	-	DTN2 executables	-	-	-	-

dtn2fw:		dtn2fw.o $(BPLIBS) libdtn2fw.so
		$(CC) -o dtn2fw dtn2fw.o -L./lib -L$(ROOT)/lib -ldtn2fw $(LBP) -lici -lpthread -lrt -lsocket
		cp dtn2fw ./bin

dtn2adminep:	dtn2adminep.o $(BPLIBS) libdtn2fw.so
		$(CC) -o dtn2adminep dtn2adminep.o -L./lib -L$(ROOT)/lib -ldtn2fw $(LBP) -lici -lpthread -lrt -lsocket
		cp dtn2adminep ./bin

#	-	-	LTP executables	-	-	-	-	-

ltpcli:		ltpcli.o $(BPLIBS)
		$(CC) -o ltpcli ltpcli.o -L./lib -L$(ROOT)/lib $(LFW) $(LBP) -lltp -lltpP -lici -lpthread -lrt -lsocket
		cp ltpcli ./bin

ltpclo:		ltpclo.o $(BPLIBS)
		$(CC) -o ltpclo ltpclo.o -L./lib -L$(ROOT)/lib $(LBP) -lltp -lltpP -lici -lpthread -lrt -lsocket
		cp ltpclo ./bin

#	-	-	TCP executables	-	-	-	-	-

tcpcli:		tcpcli.o $(BPLIBS)
		$(CC) -o tcpcli tcpcli.o -L./lib -L$(ROOT)/lib $(LFW) $(LBP) -lici -lpthread -lrt -lsocket
		cp tcpcli ./bin

tcpclo:		tcpclo.o $(BPLIBS)
		$(CC) -o tcpclo tcpclo.o -L./lib -L$(ROOT)/lib $(LBP) -lici -lpthread -lrt -lsocket
		cp tcpclo ./bin

#	-	-	STCP executables	-	-	-	-

stcpcli:	stcpcli.o $(BPLIBS) libstcpcla.so
		$(CC) -o stcpcli stcpcli.o -L./lib -L$(ROOT)/lib $(LFW) -lstcpcla $(LBP) -lici -lpthread -lrt -lsocket
		cp stcpcli ./bin

stcpclo:	stcpclo.o $(BPLIBS) libstcpcla.so
		$(CC) -o stcpclo stcpclo.o -L./lib -L$(ROOT)/lib -lstcpcla $(LBP) -lici -lpthread -lrt -lsocket
		cp stcpclo ./bin

#	-	-	UDP executables	-	-	-	-	-

udpcli:		udpcli.o $(BPLIBS) libudpcla.so
		$(CC) -o udpcli udpcli.o -L./lib -L$(ROOT)/lib $(LFW) -ludpcla $(LBP) -lici -lpthread -lrt -lsocket
		cp udpcli ./bin

udpclo:		udpclo.o $(BPLIBS) libudpcla.so
		$(CC) -o udpclo udpclo.o -L./lib -L$(ROOT)/lib -ludpcla $(LBP) -lici -lpthread -lrt -lsocket
		cp udpclo ./bin

#	-	-	DGR executable	-	-	-	-	-

dgrcla:		dgrcla.o $(BPLIBS)
		$(CC) -o dgrcla dgrcla.o -L./lib -L$(ROOT)/lib $(LFW) $(LBP) -ldgr -lici -lpthread -lrt -lsocket
		cp dgrcla ./bin

#	-	-	BRS executables	-	-	-	-

brsscla:	brsscla.o $(BPLIBS) libstcpcla.so
		$(CC) -o brsscla brsscla.o -L./lib -L$(ROOT)/lib $(LFW) -lstcpcla $(LBP) -lici -lpthread -lrt -lsocket
		cp brsscla ./bin

brsccla:	brsccla.o $(BPLIBS) libstcpcla.so
		$(CC) -o brsccla brsccla.o -L./lib -L$(ROOT)/lib $(LFW) -lstcpcla $(LBP) -lici -lpthread -lrt -lsocket
		cp brsccla ./bin

#	-	-	Test executables	-	-	-	-

bpsource:	bpsource.o libbp.so $(BPLIBS)
		$(CC) -o bpsource bpsource.o -L./lib -L$(ROOT)/lib $(LBP) -lici -lpthread -lrt -lsocket
		cp bpsource ./bin

bpsink:		bpsink.o libbp.so $(BPLIBS)
		$(CC) -o bpsink bpsink.o -L./lib -L$(ROOT)/lib $(LBP) -lici -lpthread -lrt -lsocket
		cp bpsink ./bin

bpdriver:	bpdriver.o libbp.so $(BPLIBS)
		$(CC) -o bpdriver bpdriver.o -L./lib -L$(ROOT)/lib $(LBP) -lici -lpthread -lrt -lsocket
		cp bpdriver ./bin

bpecho:		bpecho.o libbp.so $(BPLIBS)
		$(CC) -o bpecho bpecho.o -L./lib -L$(ROOT)/lib $(LBP) -lici -lpthread -lrt -lsocket
		cp bpecho ./bin

bpcounter:	bpcounter.o libbp.so $(BPLIBS)
		$(CC) -o bpcounter bpcounter.o -L./lib -L$(ROOT)/lib $(LBP) -lici -lpthread -lrt -lsocket
		cp bpcounter ./bin

bpsendfile:	bpsendfile.o libbp.so $(BPLIBS)
		$(CC) -o bpsendfile bpsendfile.o -L./lib -L$(ROOT)/lib $(LBP) -lici -lpthread -lrt -lsocket
		cp bpsendfile ./bin

bprecvfile:	bprecvfile.o libbp.so $(BPLIBS)
		$(CC) -o bprecvfile bprecvfile.o -L./lib -L$(ROOT)/lib $(LBP) -lici -lpthread -lrt -lsocket
		cp bprecvfile ./bin

#	-	-	Libraries	-	-	-	-	-

libbpP.so:	$(BPPOBJS)
		$(LD) -o libbpP.so $(BPPOBJS)
		cp libbpP.so ./lib

libbp.so:	libbp.o
		$(LD) -o libbp.so libbp.o
		cp libbp.so ./lib

libcgr.so:	libcgr.o
		$(LD) -o libcgr.so libcgr.o
		cp libcgr.so ./lib

libipnfw.so:	libipnfw.o
		$(LD) -o libipnfw.so libipnfw.o
		cp libipnfw.so ./lib

libdtn2fw.so:	libdtn2fw.o
		$(LD) -o libdtn2fw.so libdtn2fw.o
		cp libdtn2fw.so ./lib

libstcpcla.so:	libstcpcla.o
		$(LD) -o libstcpcla.so libstcpcla.o
		cp libstcpcla.so ./lib

libudpcla.so:	libudpcla.o
		$(LD) -o libudpcla.so libudpcla.o
		cp libudpcla.so ./lib

#	-	-	Object modules	-	-	-	-	-

%.o:		$(API)/%.c
		$(CC) -c $<

%.o:		$(CRYPTO)/%.c
		$(CC) -c $<

%.o:		$(API)/ext/ecos/%.c
		$(CC) -c $<

%.o:		$(API)/ext/bsp/%.c
		$(CC) -c $<

#%.o:		$(API)/ext/phn/%.c
#		$(CC) -c $<

%.o:		$(TEST)/%.c
		$(CC) -c $<

%.o:		$(UTILS)/%.c
		$(CC) -c $<

%.o:		$(DAEMON)/%.c
		$(CC) -c $<

%.o:		$(CGR)/%.c
		$(CC) -c $<

%.o:		$(IPN)/%.c
		$(CC) -c $<

%.o:		$(DTN2)/%.c
		$(CC) -c $<

%.o:		$(LTP)/%.c
		$(CC) -c $<

%.o:		$(TCP)/%.c
		$(CC) -c $<

%.o:		$(UDP)/%.c
		$(CC) -c $<

%.o:		$(BRS)/%.c
		$(CC) -c $<

%.o:		$(DGR)/%.c
		$(CC) -c $<
