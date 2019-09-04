API = ../library
INCL = ../include
UTILS = ../utils
TEST = ../test
DAEMON = ../daemon
CGR = ../cgr
IPN = ../ipn
IMC = ../imc
BSSP = ../bssp
DTN2 = ../dtn2
LTP = ../ltp
TCP = ../tcp
DGR = ../dgr
BRS = ../brs
UDP = ../udp
DCCP = ../dccp

# OPT = -O -Dfreebsd
OPT = -g -Wall -Werror -Dfreebsd -DENABLE_BPACS -DENABLE_IMC
CC = cc $(OPT) -I$(IPN) -I$(IMC) -I$(DTN2) -I$(API) -I$(API)/acs -I$(API)/ext -I$(API)/ext/phn -I$(API)/ext/ecos -I$(API)/ext/meb -I$(API)/ext/bae -I$(API)/ext/bsp -I$(API)/ext/cteb -I$(INCL) -I$(ROOT)/include -DBP_EXTENDED
# -I$(API)/ext/snid
LDFLAGS = -fPIC -shared
LD = ld $(LDFLAGS)

PUBINCLS = \
	$(INCL)/bp.h

BPINCLS = \
	$(API)/bpP.h \
	$(IPN)/ipnfw.h \
	$(IMC)/imcfw.h \
	$(IMC)/imcP.h \
	$(BSSP)/bsspcla.h \
	$(DTN2)/dtn2fw.h \
	$(BRS)/brscla.h \
	$(UDP)/udpcla.h \
	$(TCP)/tcpcla.h \
	$(LTP)/ltpcla.h \
	$(DCCP)/dccpcla.h \
	$(API)/acs/acs.h

BPLIBS = \
	libbp.so

BPPOBJS = libbpP.o phn.o ecos.o meb.o bae.o bei.o extbsputil.o extbspbab.o extbsppcb.o extbsppib.o acstx.o acsrx.o acsserialize.o acsappend.o acsid.o cteb.o libimcfw.o
# snid.o

LBP = -lbp

RUNTIMES = bpadmin bpsink bpsource bpdriver bpecho bpcounter bpsendfile bprecvfile bpclock ipnadmin ipnfw ipnadminep dtn2admin dtn2fw dtn2adminep tcpcli tcpclo stcpcli stcpclo brsscla brsccla udpcli udpclo dgrcla ltpcli ltpclo lgsend lgagent bptrace bpstats bplist bpcancel hmackeys bping bpstats2 bpchat acsadmin acslist imcadmin imcfw bsspcli bsspclo bpnmtest
# dccpcli dccpclo

ALL = check $(BPLIBS) libcgr.so libipnfw.so libimcfw.so libdtn2fw.so libtcpcla.so libudpcla.so $(RUNTIMES)

all:		$(ALL)

check:		$(BPINCLS) $(PUBINCLS)
		rm -f *.o
		touch check
		mkdir -p lib
		mkdir -p bin

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
		$(CC) -o bpclock bpclock.o -L./lib -L$(ROOT)/lib $(LBP) -lici -lpthread
		cp bpclock ./bin

#	-	-	Utility executables	-	-	-	-

bpadmin:	bpadmin.o $(BPLIBS)
		$(CC) -o bpadmin bpadmin.o -L./lib -L$(ROOT)/lib $(LBP) -lici -lpthread
		cp bpadmin ./bin

ipnadmin:	ipnadmin.o $(BPLIBS) libipnfw.so
		$(CC) -o ipnadmin ipnadmin.o -L./lib -L$(ROOT)/lib -lipnfw $(LBP) -lici -lpthread
		cp ipnadmin ./bin

imcadmin:	imcadmin.o $(BPLIBS) libimcfw.so
		$(CC) -o imcadmin imcadmin.o -L./lib -L$(ROOT)/lib -limcfw $(LBP) -lici -lpthread
		cp imcadmin ./bin

dtn2admin:	dtn2admin.o $(BPLIBS) libdtn2fw.so
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

acsadmin:	acsadmin.o $(BPLIBS)
		$(CC) -o acsadmin acsadmin.o -L./lib -L$(ROOT)/lib $(LBP) -lici -lpthread
		cp acsadmin ./bin

acslist:	acslist.o $(BPLIBS)
		$(CC) -o acslist acslist.o -L./lib -L$(ROOT)/lib $(LBP) -lici -lpthread
		cp acslist ./bin

#	-	-	IPN executables	-	-	-	-	-

ipnfw:		ipnfw.o $(BPLIBS) libipnfw.so libcgr.so
		$(CC) -o ipnfw ipnfw.o -L./lib -L$(ROOT)/lib -lcgr -lipnfw $(LBP) -lici -lpthread
		cp ipnfw ./bin

ipnadminep:	ipnadminep.o $(BPLIBS) libipnfw.so
		$(CC) -o ipnadminep ipnadminep.o -L./lib -L$(ROOT)/lib -lipnfw $(LBP) -lici -lpthread
		cp ipnadminep ./bin

#	-	-	IMC executables	-	-	-	-	-

imcfw:		imcfw.o $(BPLIBS) libimcfw.so libipnfw.so
		$(CC) -o imcfw imcfw.o -L./lib -L$(ROOT)/lib -limcfw -lipnfw $(LBP) -lici -lpthread
		cp imcfw ./bin

#	-	-	DTN2 executables	-	-	-	-

dtn2fw:		dtn2fw.o $(BPLIBS) libdtn2fw.so
		$(CC) -o dtn2fw dtn2fw.o -L./lib -L$(ROOT)/lib -ldtn2fw $(LBP) -lici -lpthread
		cp dtn2fw ./bin

dtn2adminep:	dtn2adminep.o $(BPLIBS) libdtn2fw.so
		$(CC) -o dtn2adminep dtn2adminep.o -L./lib -L$(ROOT)/lib -ldtn2fw $(LBP) -lici -lpthread
		cp dtn2adminep ./bin

#	-	-	LTP executables	-	-	-	-	-

ltpcli:		ltpcli.o $(BPLIBS)
		$(CC) -o ltpcli ltpcli.o -L./lib -L$(ROOT)/lib $(LBP) -lltp -lltpP -lici -lpthread -lm
		cp ltpcli ./bin

ltpclo:		ltpclo.o $(BPLIBS)
		$(CC) -o ltpclo ltpclo.o -L./lib -L$(ROOT)/lib $(LBP) -lltp -lltpP -lici -lpthread -lm
		cp ltpclo ./bin

#	-	-	TCP executables	-	-	-	-	-

tcpcli:		tcpcli.o $(BPLIBS) libtcpcla.so
		$(CC) -o tcpcli tcpcli.o -L./lib -L$(ROOT)/lib -ltcpcla $(LBP) -lici -lpthread
		cp tcpcli ./bin

tcpclo:		tcpclo.o $(BPLIBS) libtcpcla.so
		$(CC) -o tcpclo tcpclo.o -L./lib -L$(ROOT)/lib -ltcpcla $(LBP) -lici -lpthread
		cp tcpclo ./bin

#	-	-	STCP executables	-	-	-	-

stcpcli:	stcpcli.o $(BPLIBS) libtcpcla.so
		$(CC) -o stcpcli stcpcli.o -L./lib -L$(ROOT)/lib -ltcpcla $(LBP) -lici -lpthread
		cp stcpcli ./bin

stcpclo:	stcpclo.o $(BPLIBS) libtcpcla.so
		$(CC) -o stcpclo stcpclo.o -L./lib -L$(ROOT)/lib -ltcpcla $(LBP) -lici -lpthread
		cp stcpclo ./bin

#	-	-	UDP executables	-	-	-	-	-

udpcli:		udpcli.o $(BPLIBS) libudpcla.so
		$(CC) -o udpcli udpcli.o -L./lib -L$(ROOT)/lib -ludpcla $(LBP) -lici -lpthread
		cp udpcli ./bin

udpclo:		udpclo.o $(BPLIBS) libudpcla.so
		$(CC) -o udpclo udpclo.o -L./lib -L$(ROOT)/lib -ludpcla $(LBP) -lici -lpthread
		cp udpclo ./bin

#	-	-	BSSP executables	-	-	-	-

bsspcli:	bsspcli.o $(BPLIBS)
		$(CC) -o bsspcli bsspcli.o -L./lib -L$(ROOT)/lib $(LBP) -lbssp -lbsspP -lici -lpthread
		cp bsspcli ./bin

bsspclo:	bsspclo.o $(BPLIBS)
		$(CC) -o bsspclo bsspclo.o -L./lib -L$(ROOT)/lib $(LBP) -lbssp -lbsspP -lici -lpthread
		cp bsspclo ./bin

#	-	-	DCCP executables	-	-	-	-

dccpcli:	dccpcli.o $(BPLIBS)
		$(CC) -o dccpcli dccpcli.o -L./lib -L$(ROOT)/lib $(LBP) -lici -lpthread
		cp dccpcli ./bin

dccpclo:	dccpclo.o $(BPLIBS)
		$(CC) -o dccpclo dccpclo.o -L./lib -L$(ROOT)/lib $(LBP) -lici -lpthread
		cp dccpclo ./bin

#	-	-	DGR executable	-	-	-	-	-

dgrcla:		dgrcla.o $(BPLIBS)
		$(CC) -o dgrcla dgrcla.o -L./lib -L$(ROOT)/lib $(LBP) -ldgr -lici -lpthread
		cp dgrcla ./bin

#	-	-	BRS executables	-	-	-	-

brsscla:	brsscla.o $(BPLIBS) libtcpcla.so
		$(CC) -o brsscla brsscla.o -L./lib -L$(ROOT)/lib -ltcpcla $(LBP) -lici -lpthread
		cp brsscla ./bin

brsccla:	brsccla.o $(BPLIBS) libtcpcla.so
		$(CC) -o brsccla brsccla.o -L./lib -L$(ROOT)/lib -ltcpcla $(LBP) -lici -lpthread
		cp brsccla ./bin

#	-	-	Test executables	-	-	-	-

bpsource:	bpsource.o libbp.so $(BPLIBS)
		$(CC) -o bpsource bpsource.o -L./lib -L$(ROOT)/lib $(LBP) -lici -lpthread
		cp bpsource ./bin

bpsink:		bpsink.o libbp.so $(BPLIBS)
		$(CC) -o bpsink bpsink.o -L./lib -L$(ROOT)/lib $(LBP) -lici -lpthread
		cp bpsink ./bin

bpdriver:	bpdriver.o libbp.so $(BPLIBS)
		$(CC) -o bpdriver bpdriver.o -L./lib -L$(ROOT)/lib $(LBP) -lici -lpthread
		cp bpdriver ./bin

bpecho:		bpecho.o libbp.so $(BPLIBS)
		$(CC) -o bpecho bpecho.o -L./lib -L$(ROOT)/lib $(LBP) -lici -lpthread
		cp bpecho ./bin

bpcounter:	bpcounter.o libbp.so $(BPLIBS)
		$(CC) -o bpcounter bpcounter.o -L./lib -L$(ROOT)/lib $(LBP) -lici -lpthread
		cp bpcounter ./bin

bpsendfile:	bpsendfile.o libbp.so $(BPLIBS)
		$(CC) -o bpsendfile bpsendfile.o -L./lib -L$(ROOT)/lib $(LBP) -lici -lpthread
		cp bpsendfile ./bin

bprecvfile:	bprecvfile.o libbp.so $(BPLIBS)
		$(CC) -o bprecvfile bprecvfile.o -L./lib -L$(ROOT)/lib $(LBP) -lici -lpthread
		cp bprecvfile ./bin

bpnmtest:	bpnmtest.o libbp.so $(BPLIBS)
		$(CC) -o bpnmtest bpnmtest.o -L./lib -L$(ROOT)/lib $(LBP) -lici -lpthread
		cp bpnmtest ./bin

#	-	-	Libraries	-	-	-	-	-

libbp.so:	libbp.o $(BPPOBJS) libbpnm.o
		$(LD) -o libbp.so libbp.o $(BPPOBJS) libbpnm.o
		cp libbp.so ./lib

libcgr.so:	libcgr.o
		$(LD) -o libcgr.so libcgr.o
		cp libcgr.so ./lib

libipnfw.so:	libipnfw.o
		$(LD) -o libipnfw.so libipnfw.o
		cp libipnfw.so ./lib

libimcfw.so:	libimcfw.o
		$(LD) -o libimcfw.so libimcfw.o
		cp libimcfw.so ./lib

libdtn2fw.so:	libdtn2fw.o
		$(LD) -o libdtn2fw.so libdtn2fw.o
		cp libdtn2fw.so ./lib

libtcpcla.so:	libtcpcla.o
		$(LD) -o libtcpcla.so libtcpcla.o
		cp libtcpcla.so ./lib

libudpcla.so:	libudpcla.o
		$(LD) -o libudpcla.so libudpcla.o
		cp libudpcla.so ./lib

#	-	-	Object modules	-	-	-	-	-

%.o:		$(API)/%.c
		$(CC) -c $<

#%.o:		$(API)/ext/snid/%.c
#		$(CC) -c $<

%.o:		$(API)/ext/ecos/%.c
		$(CC) -c $<

%.o:		$(API)/ext/meb/%.c
		$(CC) -c $<

%.o:		$(API)/ext/bae/%.c
		$(CC) -c $<

%.o:		$(API)/ext/bsp/%.c
		$(CC) -c $<

%.o:		$(API)/ext/phn/%.c
		$(CC) -c $<

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

%.o:		$(BSSP)/%.c
		$(CC) -c $<

%.o:		$(DTN2)/%.c
		$(CC) -c $<

%.o:		$(LTP)/%.c
		$(CC) -c $<

%.o:		$(TCP)/%.c
		$(CC) -c $<

%.o:		$(UDP)/%.c
		$(CC) -c $<

#%.o:		$(DCCP)/%.c
#		$(CC) -c $<

%.o:		$(BRS)/%.c
		$(CC) -c $<

%.o:		$(DGR)/%.c
		$(CC) -c $<

%.o:		$(API)/acs/%.c
		$(CC) -c $<

%.o:		$(API)/ext/cteb/%.c
		$(CC) -c $<
