API = ../library
INCL = ../include
UTILS = ../utils
TEST = ../test
DAEMON = ../daemon
CGR = ../cgr
IPN = ../ipn
IMC = ../imc
BSS = ../bss
DTN2 = ../dtn2
LTP = ../ltp
TCP = ../tcp
DGR = ../dgr
BRS = ../brs
UDP = ../udp
CRYPTO = $(API)/crypto/NULL_SUITES

OPT = -g -Wall -Werror -Dmingw -DENABLE_IMC
# -DENABLE_BPACS
CC = gcc $(OPT) -I$(IPN) -I$(IMC) -I$(DTN2) -I$(API) -I$(API)/crypto -I$(API)/ext -I$(API)/ext/ecos -I$(API)/ext/bae -I$(API)/ext/bsp -I$(INCL) -I$(ROOT)/include -DBP_EXTENDED
LDFLAGS = -fPIC -shared
LD = gcc $(LDFLAGS)

PUBINCLS = \
	$(INCL)/bp.h

BPINCLS = \
	$(API)/bpP.h \
	$(IPN)/ipnfw.h \
	$(IMC)/imcfw.h \
	$(IMC)/imcP.h \
	$(BSS)/bssfw.h \
	$(DTN2)/dtn2fw.h \
	$(BRS)/brscla.h \
	$(UDP)/udpcla.h \
	$(TCP)/tcpcla.h \
	$(LTP)/ltpcla.h
#	$(API)/acs/acs.h

BPLIBS = \
	libbp.dll

BPPOBJS = libbpP.o ecos.o bae.o bei.o extbsputil.o extbspbab.o extbsppcb.o extbsppib.o crypto.o libimcfw.o
# acstx.o acsrx.o acsserialize.o acsappend.o acsid.o cteb.o
# phn.o

LFW = -lbssfw -ldtn2fw
# -lipnfw

LBP = -lbp

RUNTIMES = bpadmin bpsink bpsource bpdriver bpecho bpcounter bpsendfile bprecvfile bpclock ipnadmin ipnfw ipnadminep dtn2admin dtn2fw dtn2adminep tcpcli tcpclo stcpcli stcpclo brsscla brsccla udpcli udpclo dgrcla ltpcli ltpclo lgsend lgagent bptrace bpstats bplist bpcancel hmackeys imcadmin imcfw
# acsadmin acslist 
# bping bpstats2 bpchat

ALL = check $(BPLIBS) libcgr.dll libipnfw.dll libimcfw.dll libdtn2fw.dll libbssfw.dll libtcpcla.dll libudpcla.dll $(RUNTIMES)

all:		$(ALL)

check:		$(BPINCLS) $(PUBINCLS)
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

#	-	-	Daemon executables	-	-	-	-

bpclock:	bpclock.o $(BPLIBS)
		$(CC) -o bpclock bpclock.o -L./lib -L$(ROOT)/lib $(LBP) -lici -lpthread
		cp bpclock ./bin

#	-	-	Utility executables	-	-	-	-

bpadmin:	bpadmin.o $(BPLIBS)
		$(CC) -o bpadmin bpadmin.o -L./lib -L$(ROOT)/lib $(LBP) -lici -lpthread
		cp bpadmin ./bin

ipnadmin:	ipnadmin.o $(BPLIBS) libipnfw.dll
		$(CC) -o ipnadmin ipnadmin.o -L./lib -L$(ROOT)/lib -lipnfw $(LBP) -lici -lpthread
		cp ipnadmin ./bin

imcadmin:	imcadmin.o $(BPLIBS) libimcfw.dll
		$(CC) -o imcadmin imcadmin.o -L./lib -L$(ROOT)/lib -lbssfw $(LBP) -lici -lpthread
		cp imcadmin ./bin

bssadmin:	bssadmin.o $(BPLIBS) libbssfw.dll
		$(CC) -o bssadmin bssadmin.o -L./lib -L$(ROOT)/lib -lbssfw $(LBP) -lici -lpthread
		cp bssadmin ./bin

dtn2admin:	dtn2admin.o $(BPLIBS) libdtn2fw.dll
		$(CC) -o dtn2admin dtn2admin.o -L./lib -L$(ROOT)/lib -ldtn2fw $(LBP) -lici -lpthread
		cp dtn2admin ./bin

lgsend:		lgsend.o $(BPLIBS)
		$(CC) -o lgsend lgsend.o -L./lib -L$(ROOT)/lib $(LBP) -lici -lpthread
		cp lgsend ./bin

lgagent:	lgagent.o $(BPLIBS)
		$(CC) -o lgagent lgagent.o -L./lib -L$(ROOT)/lib $(LBP) -lici -lpthread
		cp lgagent ./bin

bptrace:	bptrace.o $(BPLIBS)
		$(CC) -o bptrace bptrace.o -L./lib -L$(ROOT)/lib $(LBP) -lici -lpthread
		cp bptrace ./bin

bpstats:	bpstats.o $(BPLIBS)
		$(CC) -o bpstats bpstats.o -L./lib -L$(ROOT)/lib $(LBP) -lici -lpthread
		cp bpstats ./bin

bpstats2:	bpstats2.o $(BPLIBS)
		$(CC) -o bpstats2 bpstats2.o -L./lib -L$(ROOT)/lib $(LBP) -lici -lpthread
		cp bpstats2 ./bin

bping:		bping.o $(BPLIBS)
		$(CC) -o bping bping.o -L./lib -L$(ROOT)/lib $(LBP) -lici -lpthread
		cp bping ./bin

bpchat:		bpchat.o $(BPLIBS)
		$(CC) -o bpchat bpchat.o -L./lib -L$(ROOT)/lib $(LBP) -lici -lpthread
		cp bpchat ./bin

bplist:		bplist.o $(BPLIBS)
		$(CC) -o bplist bplist.o -L./lib -L$(ROOT)/lib $(LBP) -lici -lpthread
		cp bplist ./bin

bpcancel:	bpcancel.o $(BPLIBS)
		$(CC) -o bpcancel bpcancel.o -L./lib -L$(ROOT)/lib $(LBP) -lici -lpthread
		cp bpcancel ./bin

hmackeys:	hmackeys.o $(BPLIBS)
		$(CC) -o hmackeys hmackeys.o -L$(ROOT)/lib -lici -lpthread
		cp hmackeys ./bin

#acsadmin:	acsadmin.o $(BPLIBS)
#		$(CC) -o acsadmin acsadmin.o -L./lib -L$(ROOT)/lib $(LBP) -lici -lpthread
#		cp acsadmin ./bin

#acslist:	acslist.o $(BPLIBS)
#		$(CC) -o acslist acslist.o -L./lib -L$(ROOT)/lib $(LBP) -lici -lpthread
#		cp acslist ./bin

#	-	-	IPN executables	-	-	-	-	-

ipnfw:		ipnfw.o $(BPLIBS) libipnfw.dll libcgr.dll
		$(CC) -o ipnfw ipnfw.o -L./lib -L$(ROOT)/lib -lipnfw -lcgr $(LBP) -lici -lpthread
		cp ipnfw ./bin

ipnadminep:	ipnadminep.o $(BPLIBS) libipnfw.dll
		$(CC) -o ipnadminep ipnadminep.o -L./lib -L$(ROOT)/lib -lipnfw $(LBP) -lici -lpthread
		cp ipnadminep ./bin

#	-	-	IMC executables	-	-	-	-	-

imcfw:		imcfw.o $(BPLIBS) libimcfw.dll libipnfw.dll
		$(CC) -o imcfw imcfw.o -L./lib -L$(ROOT)/lib -limcfw -lipnfw $(LBP) -lici -lpthread
		cp imcfw ./bin

#	-	-	BSS executables	-	-	-	-	-

bssfw:		bssfw.o $(BPLIBS) libbssfw.dll libcgr.dll
		$(CC) -o bssfw bssfw.o -L./lib -L$(ROOT)/lib -lbssfw -lcgr $(LBP) -lici -lpthread
		cp bssfw ./bin

#	-	-	DTN2 executables	-	-	-	-

dtn2fw:		dtn2fw.o $(BPLIBS) libdtn2fw.dll
		$(CC) -o dtn2fw dtn2fw.o -L./lib -L$(ROOT)/lib -ldtn2fw $(LBP) -lici -lpthread
		cp dtn2fw ./bin

dtn2adminep:	dtn2adminep.o $(BPLIBS) libdtn2fw.dll
		$(CC) -o dtn2adminep dtn2adminep.o -L./lib -L$(ROOT)/lib -ldtn2fw $(LBP) -lici -lpthread
		cp dtn2adminep ./bin

#	-	-	LTP executables	-	-	-	-	-

ltpcli:		ltpcli.o $(BPLIBS)
		$(CC) -o ltpcli ltpcli.o -L./lib -L$(ROOT)/lib $(LFW) $(LBP) -lltp -lici -lpthread
		cp ltpcli ./bin

ltpclo:		ltpclo.o $(BPLIBS)
		$(CC) -o ltpclo ltpclo.o -L./lib -L$(ROOT)/lib $(LBP) -lltp -lici -lpthread
		cp ltpclo ./bin

#	-	-	TCP executables	-	-	-	-	-

tcpcli:		tcpcli.o $(BPLIBS) libtcpcla.dll
		$(CC) -o tcpcli tcpcli.o -L./lib -L$(ROOT)/lib $(LFW) -ltcpcla $(LBP) -lici -lpthread -lws2_32
		cp tcpcli ./bin

tcpclo:		tcpclo.o $(BPLIBS) libtcpcla.dll
		$(CC) -o tcpclo tcpclo.o -L./lib -L$(ROOT)/lib -ltcpcla $(LBP) -lici -lpthread -lws2_32
		cp tcpclo ./bin

#	-	-	STCP executables	-	-	-	-

stcpcli:	stcpcli.o $(BPLIBS) libtcpcla.dll
		$(CC) -o stcpcli stcpcli.o -L./lib -L$(ROOT)/lib $(LFW) -ltcpcla $(LBP) -lici -lpthread -lws2_32
		cp stcpcli ./bin

stcpclo:	stcpclo.o $(BPLIBS) libtcpcla.dll
		$(CC) -o stcpclo stcpclo.o -L./lib -L$(ROOT)/lib -ltcpcla $(LBP) -lici -lpthread -lws2_32
		cp stcpclo ./bin

#	-	-	UDP executables	-	-	-	-	-

udpcli:		udpcli.o $(BPLIBS) libudpcla.dll
		$(CC) -o udpcli udpcli.o -L./lib -L$(ROOT)/lib $(LFW) -ludpcla $(LBP) -lici -lpthread -lws2_32
		cp udpcli ./bin

udpclo:		udpclo.o $(BPLIBS) libudpcla.dll
		$(CC) -o udpclo udpclo.o -L./lib -L$(ROOT)/lib -ludpcla $(LBP) -lici -lpthread -lws2_32
		cp udpclo ./bin

#	-	-	DGR executable	-	-	-	-	-

dgrcla:		dgrcla.o $(BPLIBS)
		$(CC) -o dgrcla dgrcla.o -L./lib -L$(ROOT)/lib $(LFW) $(LBP) -ldgr -lici -lpthread -lws2_32
		cp dgrcla ./bin

#	-	-	BRS executables	-	-	-	-

brsscla:	brsscla.o $(BPLIBS) libtcpcla.dll
		$(CC) -o brsscla brsscla.o -L./lib -L$(ROOT)/lib $(LFW) -ltcpcla $(LBP) -lici -lpthread -lws2_32
		cp brsscla ./bin

brsccla:	brsccla.o $(BPLIBS) libtcpcla.dll
		$(CC) -o brsccla brsccla.o -L./lib -L$(ROOT)/lib $(LFW) -ltcpcla $(LBP) -lici -lpthread -lws2_32
		cp brsccla ./bin

#	-	-	Test executables	-	-	-	-

bpsource:	bpsource.o libbp.dll $(BPLIBS)
		$(CC) -o bpsource bpsource.o -L./lib -L$(ROOT)/lib $(LBP) -lici -lpthread
		cp bpsource ./bin

bpsink:		bpsink.o libbp.dll $(BPLIBS)
		$(CC) -o bpsink bpsink.o -L./lib -L$(ROOT)/lib $(LBP) -lici -lpthread
		cp bpsink ./bin

bpdriver:	bpdriver.o libbp.dll $(BPLIBS)
		$(CC) -o bpdriver bpdriver.o -L./lib -L$(ROOT)/lib $(LBP) -lici -lpthread
		cp bpdriver ./bin

bpecho:		bpecho.o libbp.dll $(BPLIBS)
		$(CC) -o bpecho bpecho.o -L./lib -L$(ROOT)/lib $(LBP) -lici -lpthread
		cp bpecho ./bin

bpcounter:	bpcounter.o libbp.dll $(BPLIBS)
		$(CC) -o bpcounter bpcounter.o -L./lib -L$(ROOT)/lib $(LBP) -lici -lpthread
		cp bpcounter ./bin

bpsendfile:	bpsendfile.o libbp.dll $(BPLIBS)
		$(CC) -o bpsendfile bpsendfile.o -L./lib -L$(ROOT)/lib $(LBP) -lici -lpthread
		cp bpsendfile ./bin

bprecvfile:	bprecvfile.o libbp.dll $(BPLIBS)
		$(CC) -o bprecvfile bprecvfile.o -L./lib -L$(ROOT)/lib $(LBP) -lici -lpthread
		cp bprecvfile ./bin

#	-	-	Libraries	-	-	-	-	-

libbp.dll:	libbp.o $(BPPOBJS) libbpnm.o
		$(LD) -o libbp.dll libbp.o $(BPPOBJS) libbpnm.o -L$(ROOT)/lib -lici -lpthread
		cp libbp.dll ./lib

libcgr.dll:	libcgr.o
		$(LD) -o libcgr.dll libcgr.o -L./lib -L$(ROOT)/lib $(LBP) -lici -lpthread
		cp libcgr.dll ./lib

libipnfw.dll:	libipnfw.o
		$(LD) -o libipnfw.dll libipnfw.o -L./lib -L$(ROOT)/lib $(LBP) -lici -lpthread
		cp libipnfw.dll ./lib

libimcfw.dll:	libimcfw.o
		$(LD) -o libimcfw.dll libimcfw.o -L./lib -L$(ROOT)/lib $(LBP) -lici -lpthread
		cp libimcfw.dll ./lib

libbssfw.dll:	libbssfw.o
		$(LD) -o libbssfw.dll libbssfw.o -L./lib -L$(ROOT)/lib $(LBP) -lici -lpthread
		cp libbssfw.dll ./lib

libdtn2fw.dll:	libdtn2fw.o
		$(LD) -o libdtn2fw.dll libdtn2fw.o -L./lib -L$(ROOT)/lib $(LBP) -lici -lpthread
		cp libdtn2fw.dll ./lib

libtcpcla.dll:	libtcpcla.o
		$(LD) -o libtcpcla.dll libtcpcla.o -L./lib -L$(ROOT)/lib $(LBP) -lici -lpthread -lws2_32
		cp libtcpcla.dll ./lib

libudpcla.dll:	libudpcla.o
		$(LD) -o libudpcla.dll libudpcla.o -L./lib -L$(ROOT)/lib $(LBP) -lici -lpthread -lws2_32
		cp libudpcla.dll ./lib

#	-	-	Object modules	-	-	-	-	-

%.o:		$(API)/%.c
		$(CC) -c $<

%.o:		$(CRYPTO)/%.c
		$(CC) -c $<

%.o:		$(API)/ext/ecos/%.c
		$(CC) -c $<

%.o:		$(API)/ext/bae/%.c
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

%.o:		$(IMC)/%.c
		$(CC) -c $<

%.o:		$(BSS)/%.c
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

#%.o:		$(API)/acs/%.c
#		$(CC) -c $<

%.o:		$(API)/ext/cteb/%.c
		$(CC) -c $<
