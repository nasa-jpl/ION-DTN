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
STCP = ../stcp
DGR = ../dgr
BRS = ../brs
UDP = ../udp
DCCP = ../dccp
BIBE = ../bibe
IPND = ../ipnd

# OPT = -O -Dlinux
OPT = -g -Wall -Werror -Dlinux -fPIC -DSPACE_ORDER=3 -DENABLE_BPACS -DENABLE_IMC -DSBSP
CC = gcc $(OPT) -I$(IPN) -I$(IMC) -I$(DTN2) -I$(API) -I$(API)/acs -I$(API)/ext -I$(API)/ext/phn -I$(API)/ext/ecos -I$(API)/ext/meb -I$(API)/ext/bae -I$(API)/ext/sbsp -I$(API)/ext/cteb -I$(API)/ext/snw -I$(BIBE) -I$(STCP) -I$(UDP) -I$(INCL) -I$(ROOT)/include -DBP_EXTENDED
# -I$(API)/ext/snid
LDFLAGS = -fPIC -shared
LD = gcc $(LDFLAGS)

PUBINCLS = \
	$(INCL)/bp.h

BPINCLS = \
	$(API)/bpP.h \
	$(INCL)/bpsec.h \
	$(API)/saga.h \
	$(IPN)/ipnfw.h \
	$(IMC)/imcfw.h \
	$(IMC)/imcP.h \
	$(BSSP)/bsspcla.h \
	$(DTN2)/dtn2fw.h \
	$(BRS)/brscla.h \
	$(UDP)/udpcla.h \
	$(STCP)/stcpcla.h \
	$(LTP)/ltpcla.h \
	$(DCCP)/dccpcla.h \
	$(API)/acs/acs.h

BPLIBS = \
	libbp.so

BPPOBJS = libbpP.o bpsec.o eureka.o saga.o bei.o phn.o ecos.o meb.o bae.o acstx.o acsrx.o acsserialize.o acsappend.o acsid.o cteb.o snw.o sbsp_util.o sbsp_instr.o profiles.o sbsp_bib.o sbsp_bcb.o bibe.o
# snid.o

IPNDOBJS = ipnd.o beacon.o helper.o bpa.o
LBP = -lbp -limcfw -lipnfw -ldtn2fw

RUNTIMES = bpadmin bpsecadmin bpsink bpsource bpdriver bpecho bpcounter bpsendfile bprecvfile bpcrash bpclock bptransit bpclm ipnadmin ipnfw ipnadminep dtn2admin dtn2fw dtn2adminep tcpcli tcpclo stcpcli stcpclo brsscla brsccla udpcli udpclo dgrcli dgrclo ltpcli ltpclo lgsend lgagent bptrace bpstats bplist bpcancel hmackeys bping bpstats2 bpchat acsadmin acslist imcadmin imcfw bsspcli bsspclo bibeclo bibeadmin bpnmtest ipnd cgrfetch bpversion
# dccpcli dccpclo

ALL = check $(BPLIBS) libcgr.so libipnfw.so libimcfw.so libdtn2fw.so libstcpcla.so libudpcla.so $(RUNTIMES)

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
		$(CC) -o bpclock bpclock.o -L./lib -L$(ROOT)/lib $(LBP) -lici -lpthread -lm
		cp bpclock ./bin

bptransit:	bptransit.o $(BPLIBS)
		$(CC) -o bptransit bptransit.o -L./lib -L$(ROOT)/lib $(LBP) -lici -lpthread -lm
		cp bptransit ./bin

bpclm:		bpclm.o $(BPLIBS)
		$(CC) -o bpclm bpclm.o -L./lib -L$(ROOT)/lib $(LBP) -lici -lpthread -lm
		cp bpclm ./bin

#	-	-	Utility executables	-	-	-	-

bpadmin:	bpadmin.o $(BPLIBS)
		$(CC) -o bpadmin bpadmin.o -L./lib -L$(ROOT)/lib $(LBP) -lici -lpthread -lm
		cp bpadmin ./bin

bibeadmin:	bibeadmin.o $(BPLIBS)
		$(CC) -o bibeadmin bibeadmin.o -L./lib -L$(ROOT)/lib $(LBP) -lici -lpthread -lm
		cp bibeadmin ./bin

bpsecadmin:	bpsecadmin.o $(BPLIBS)
		$(CC) -o bpsecadmin bpsecadmin.o -L./lib -L$(ROOT)/lib $(LBP) -lici -lpthread -lm
		cp bpsecadmin ./bin

ipnadmin:	ipnadmin.o $(BPLIBS) libipnfw.so
		$(CC) -o ipnadmin ipnadmin.o -L./lib -L$(ROOT)/lib $(LBP) -lici -lpthread -lm
		cp ipnadmin ./bin

imcadmin:	imcadmin.o $(BPLIBS) libimcfw.so
		$(CC) -o imcadmin imcadmin.o -L./lib -L$(ROOT)/lib $(LBP) -lici -lpthread -lm
		cp imcadmin ./bin

dtn2admin:	dtn2admin.o $(BPLIBS) libdtn2fw.so
		$(CC) -o dtn2admin dtn2admin.o -L./lib -L$(ROOT)/lib $(LBP) -lici -lpthread -lm
		cp dtn2admin ./bin

lgsend:		lgsend.o $(BPLIBS)
		$(CC) -o lgsend lgsend.o -L./lib -L$(ROOT)/lib $(LBP) -lici -lpthread -lm
		cp lgsend ./bin

lgagent:	lgagent.o $(BPLIBS)
		$(CC) -o lgagent lgagent.o -L./lib -L$(ROOT)/lib $(LBP) -lici -lpthread -lm
		cp lgagent ./bin

bptrace:	bptrace.o $(BPLIBS)
		$(CC) -o bptrace bptrace.o -L./lib -L$(ROOT)/lib $(LBP) -lici -lpthread -lm
		cp bptrace ./bin

bpstats:	bpstats.o $(BPLIBS)
		$(CC) -o bpstats bpstats.o -L./lib -L$(ROOT)/lib $(LBP) -lici -lpthread -lm
		cp bpstats ./bin

bpstats2:	bpstats2.o $(BPLIBS)
		$(CC) -o bpstats2 bpstats2.o -L./lib -L$(ROOT)/lib $(LBP) -lici -lpthread -lm
		cp bpstats2 ./bin

bping:		bping.o $(BPLIBS)
		$(CC) -o bping bping.o -L./lib -L$(ROOT)/lib $(LBP) -lici -lpthread -lm
		cp bping ./bin

bpchat:		bpchat.o $(BPLIBS)
		$(CC) -o bpchat bpchat.o -L./lib -L$(ROOT)/lib $(LBP) -lici -lpthread -lm
		cp bpchat ./bin

bplist:		bplist.o $(BPLIBS)
		$(CC) -o bplist bplist.o -L./lib -L$(ROOT)/lib $(LBP) -lici -lpthread -lm
		cp bplist ./bin

bpcancel:	bpcancel.o $(BPLIBS)
		$(CC) -o bpcancel bpcancel.o -L./lib -L$(ROOT)/lib $(LBP) -lici -lpthread -lm
		cp bpcancel ./bin

hmackeys:	hmackeys.o $(BPLIBS)
		$(CC) -o hmackeys hmackeys.o -L$(ROOT)/lib -lici -lpthread -lm
		cp hmackeys ./bin

acsadmin:	acsadmin.o $(BPLIBS)
		$(CC) -o acsadmin acsadmin.o -L./lib -L$(ROOT)/lib $(LBP) -lici -lpthread -lm
		cp acsadmin ./bin

acslist:	acslist.o $(BPLIBS)
		$(CC) -o acslist acslist.o -L./lib -L$(ROOT)/lib $(LBP) -lici -lpthread -lm
		cp acslist ./bin

cgrfetch:	cgrfetch.o $(BPLIBS)
		$(CC) -o cgrfetch cgrfetch.o -L./lib -L$(ROOT)/lib -lcgr $(LBP) -lici -lpthread -lm
		cp cgrfetch ./bin

bpversion:	bpversion.o
		$(CC) -o bpversion bpversion.o -L./lib -L$(ROOT)/lib $(LBP) -lici -lpthread -lm
		cp bpversion ./bin

#	-	-	IPN executables	-	-	-	-	-

ipnfw:		ipnfw.o $(BPLIBS) libipnfw.so libcgr.so
		$(CC) -o ipnfw ipnfw.o -L./lib -L$(ROOT)/lib -lcgr $(LBP) -lici -lpthread -lm
		cp ipnfw ./bin

ipnadminep:	ipnadminep.o $(BPLIBS) libipnfw.so
		$(CC) -o ipnadminep ipnadminep.o -L./lib -L$(ROOT)/lib $(LBP) -lici -lpthread -lm
		cp ipnadminep ./bin

#	-	-	IMC executables	-	-	-	-	-

imcfw:		imcfw.o $(BPLIBS) libimcfw.so libipnfw.so
		$(CC) -o imcfw imcfw.o -L./lib -L$(ROOT)/lib $(LBP) -lici -lpthread -lm
		cp imcfw ./bin

#	-	-	DTN2 executables	-	-	-	-

dtn2fw:		dtn2fw.o $(BPLIBS) libdtn2fw.so
		$(CC) -o dtn2fw dtn2fw.o -L./lib -L$(ROOT)/lib $(LBP) -lici -lpthread -lm
		cp dtn2fw ./bin

dtn2adminep:	dtn2adminep.o $(BPLIBS) libdtn2fw.so
		$(CC) -o dtn2adminep dtn2adminep.o -L./lib -L$(ROOT)/lib $(LBP) -lici -lpthread -lm
		cp dtn2adminep ./bin

#	-	-	LTP executables	-	-	-	-	-

ltpcli:		ltpcli.o $(BPLIBS)
		$(CC) -o ltpcli ltpcli.o -L./lib -L$(ROOT)/lib $(LBP) -lltp -lici -lpthread -lm
		cp ltpcli ./bin

ltpclo:		ltpclo.o $(BPLIBS)
		$(CC) -o ltpclo ltpclo.o -L./lib -L$(ROOT)/lib $(LBP) -lltp -lici -lpthread -lm
		cp ltpclo ./bin

#	-	-	TCP executables	-	-	-	-	-

tcpcli:		tcpcli.o $(BPLIBS)
		$(CC) -o tcpcli tcpcli.o -L./lib -L$(ROOT)/lib $(LBP) -lici -lpthread -lm
		cp tcpcli ./bin

tcpclo:		tcpclo.o $(BPLIBS)
		$(CC) -o tcpclo tcpclo.o -L./lib -L$(ROOT)/lib $(LBP) -lici -lpthread -lm
		cp tcpclo ./bin

#	-	-	STCP executables	-	-	-	-

stcpcli:	stcpcli.o $(BPLIBS) libstcpcla.so
		$(CC) -o stcpcli stcpcli.o -L./lib -L$(ROOT)/lib -lstcpcla $(LBP) -lici -lpthread -lm
		cp stcpcli ./bin

stcpclo:	stcpclo.o $(BPLIBS) libstcpcla.so
		$(CC) -o stcpclo stcpclo.o -L./lib -L$(ROOT)/lib -lstcpcla $(LBP) -lici -lpthread -lm
		cp stcpclo ./bin

#	-	-	UDP executables	-	-	-	-	-

udpcli:		udpcli.o $(BPLIBS) libudpcla.so
		$(CC) -o udpcli udpcli.o -L./lib -L$(ROOT)/lib -ludpcla $(LBP) -lici -lpthread -lm
		cp udpcli ./bin

udpclo:		udpclo.o $(BPLIBS) libudpcla.so
		$(CC) -o udpclo udpclo.o -L./lib -L$(ROOT)/lib -ludpcla $(LBP) -lici -lpthread -lm
		cp udpclo ./bin

#	-	-	BSSP executables	-	-	-	-

bsspcli:	bsspcli.o $(BPLIBS)
		$(CC) -o bsspcli bsspcli.o -L./lib -L$(ROOT)/lib $(LBP) -lbssp -lbsspP -lici -lpthread -lm
		cp bsspcli ./bin

bsspclo:	bsspclo.o $(BPLIBS)
		$(CC) -o bsspclo bsspclo.o -L./lib -L$(ROOT)/lib $(LBP) -lbssp -lbsspP -lici -lpthread -lm
		cp bsspclo ./bin

#	-	-	DCCP executables	-	-	-	-

dccpcli:	dccpcli.o $(BPLIBS)
		$(CC) -o dccpcli dccpcli.o -L./lib -L$(ROOT)/lib $(LBP) -lici -lpthread -lm
		cp dccpcli ./bin

dccpclo:	dccpclo.o $(BPLIBS)
		$(CC) -o dccpclo dccpclo.o -L./lib -L$(ROOT)/lib $(LBP) -lici -lpthread -lm
		cp dccpclo ./bin

#	-	-	DGR executables	-	-	-	-	-

dgrcli:		dgrcli.o $(BPLIBS)
		$(CC) -o dgrcli dgrcli.o -L./lib -L$(ROOT)/lib $(LBP) -ldgr -lici -lpthread -lm
		cp dgrcli ./bin

dgrclo:		dgrclo.o $(BPLIBS)
		$(CC) -o dgrclo dgrclo.o -L./lib -L$(ROOT)/lib $(LBP) -ldgr -lici -lpthread -lm
		cp dgrclo ./bin

#	-	-	BRS executables	-	-	-	-

brsscla:	brsscla.o $(BPLIBS) libstcpcla.so
		$(CC) -o brsscla brsscla.o -L./lib -L$(ROOT)/lib -lstcpcla $(LBP) -lici -lpthread -lm
		cp brsscla ./bin

brsccla:	brsccla.o $(BPLIBS) libstcpcla.so
		$(CC) -o brsccla brsccla.o -L./lib -L$(ROOT)/lib -lstcpcla $(LBP) -lici -lpthread -lm
		cp brsccla ./bin

#	-	-	BIBE executable	-	-	-	-	-

bibeclo:	bibeclo.o $(BPLIBS)
		$(CC) -o bibeclo bibeclo.o -L./lib -L$(ROOT)/lib $(LBP) -lici -lpthread -lm
		cp bibeclo ./bin

#	-	-	IPND executable	-	-	-	-	-

ipnd:		$(IPNDOBJS) $(BPLIBS)
		$(CC) -o ipnd $(IPNDOBJS) -L./lib -L$(ROOT)/lib $(LBP) -lici -lbloom -lpthread -lm
		cp ipnd ./bin

#	-	-	Test executables	-	-	-	-

bpsource:	bpsource.o $(BPLIBS)
		$(CC) -o bpsource bpsource.o -L./lib -L$(ROOT)/lib $(LBP) -lici -lpthread -lm
		cp bpsource ./bin

bpsink:		bpsink.o $(BPLIBS)
		$(CC) -o bpsink bpsink.o -L./lib -L$(ROOT)/lib $(LBP) -lici -lpthread -lm
		cp bpsink ./bin

bpdriver:	bpdriver.o $(BPLIBS)
		$(CC) -o bpdriver bpdriver.o -L./lib -L$(ROOT)/lib $(LBP) -lici -lpthread -lm
		cp bpdriver ./bin

bpecho:		bpecho.o $(BPLIBS)
		$(CC) -o bpecho bpecho.o -L./lib -L$(ROOT)/lib $(LBP) -lici -lpthread -lm
		cp bpecho ./bin

bpcounter:	bpcounter.o $(BPLIBS)
		$(CC) -o bpcounter bpcounter.o -L./lib -L$(ROOT)/lib $(LBP) -lici -lpthread -lm
		cp bpcounter ./bin

bpsendfile:	bpsendfile.o $(BPLIBS)
		$(CC) -o bpsendfile bpsendfile.o -L./lib -L$(ROOT)/lib $(LBP) -lici -lpthread -lm
		cp bpsendfile ./bin

bprecvfile:	bprecvfile.o $(BPLIBS)
		$(CC) -o bprecvfile bprecvfile.o -L./lib -L$(ROOT)/lib $(LBP) -lici -lpthread -lm
		cp bprecvfile ./bin

bpnmtest:	bpnmtest.o $(BPLIBS)
		$(CC) -o bpnmtest bpnmtest.o -L./lib -L$(ROOT)/lib $(LBP) -lici -lpthread -lm
		cp bpnmtest ./bin

bpcrash:	bpcrash.o $(BPLIBS)
		$(CC) -o bpcrash bpcrash.o -L./lib -L$(ROOT)/lib $(LBP) -lici -lpthread -lm
		cp bpcrash ./bin

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

libstcpcla.so:	libstcpcla.o
		$(LD) -o libstcpcla.so libstcpcla.o
		cp libstcpcla.so ./lib

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

%.o:		$(API)/ext/sbsp/%.c
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

%.o:		$(STCP)/%.c
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

%.o:		$(BIBE)/%.c
		$(CC) -c $<

%.o:		$(API)/acs/%.c
		$(CC) -c $<

%.o:		$(API)/ext/cteb/%.c
		$(CC) -c $<

%.o:		$(API)/ext/snw/%.c
		$(CC) -c $<

%.o:		$(IPND)/%.c
		$(CC) -I$(IPND) -c $<
