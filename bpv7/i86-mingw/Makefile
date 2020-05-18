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
DGR_MAIN = ../../dgr
BRS = ../brs
UDP = ../udp
DCCP = ../dccp
BIBE = ../bibe
IPND = ../ipnd
ICI = ../../ici
SAGA = ../saga

OPT = -g -Wall -Dmingw -DENABLE_IMC -DSBSP
# -Werror
CC = gcc $(OPT) -I$(ICI)/incude -I$(IPN) -I$(IMC) -I$(DTN2) -I$(SAGA) -I$(BIBE) -I$(API) -I$(API)/ext -I$(API)/ext/bpq -I$(API)/ext/snw -I$(API)/ext/hcb -I$(API)/ext/pnb -I$(API)/ext/meb -I$(API)/ext/bae -I$(API)/ext/bpsec -I$(STCP) -I$(UDP) -I$(INCL) -I $(DGR_MAIN)/include -I$(ROOT)/include -DBP_EXTENDED


LDFLAGS = -fPIC -shared
LD = gcc $(LDFLAGS)

PUBINCLS = \
	$(INCL)/bp.h

BPINCLS = \
	$(API)/bpP.h \
	$(INCL)/bpsec.h \
	$(BIBE)/bibe.h \
	$(SAGA)/saga.h \
	$(ICI)/include/cbor.h \
	$(IPN)/ipnfw.h \
	$(IMC)/imcfw.h \
	$(IMC)/imcP.h \
	$(BSSP)/bsspcla.h \
	$(DTN2)/dtn2fw.h \
	$(BRS)/brscla.h \
	$(UDP)/udpcla.h \
	$(STCP)/stcpcla.h \
	$(LTP)/ltpcla.h \
	$(API)/ext/snw/snw.h \
	$(DGR_MAIN)/include/dgr.h \
	$(DCCP)/dccpcla.h

BPLIBS = \
	libbp.dll

BPPOBJS = libbpP.o eureka.o bei.o bibe.o bpsec.o saga.o snw.o pnb.o hcb.o bpq.o meb.o bae.o bpsec_util.o bpsec_instr.o profiles.o bib.o bcb.o

IPNDOBJS = ipnd.o beacon.o helper.o bpa.o
LBP = -lbp -limcfw -lipnfw -ldtn2fw -lcgr

RUNTIMES = beacon bibeadmin bibeclo bpadmin bpcancel bpchat bpclm bpclock bpcounter bpcrash bpdriver bpecho bping bplist bpnmtest bprecvfile bpsendfile bpsink bpsource bpstats bptrace bptransit bpversion brsccla brsscla bsspcli bsspclo cgrfetch dgrcli dgrclo dtn2admin dtn2adminep dtn2fw hmackeys imcadmin imcfw ipnadmin ipnadminep ipnd ipnfw lgagent lgsend ltpcli ltpclo node stcpcli stcpclo tcpcli tcpclo udpcli udpclo
# bpstats2 - needs SIGUSR1 which is not implemented in mingw
# bpchat - executable will build but the standard test relies on mkfifo which is not implemented for mingw
# dccpcli dccpclo - need dccplsi and dccplso that cannot be built in ltp


ALL = check $(BPLIBS) libcgr.dll libipnfw.dll libimcfw.dll libdtn2fw.dll libstcpcla.dll libudpcla.dll $(RUNTIMES)

all:		$(ALL)

check:		$(BPINCLS) $(PUBINCLS)
		rm -f *.o
		touch check

clean:
		rm -f *.o
		rm -f *.exe
		rm -f *.def
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

ipnadmin:	ipnadmin.o $(BPLIBS) libipnfw.dll
		$(CC) -o ipnadmin ipnadmin.o -L./lib -L$(ROOT)/lib -lipnfw $(LBP) -lici -lpthread -lm
		cp ipnadmin ./bin

imcadmin:	imcadmin.o $(BPLIBS) libimcfw.dll
		$(CC) -o imcadmin imcadmin.o -L./lib -L$(ROOT)/lib -lipnfw $(LBP) -lici -lpthread -lm
		cp imcadmin ./bin

dtn2admin:	dtn2admin.o $(BPLIBS) libdtn2fw.dll
		$(CC) -o dtn2admin dtn2admin.o -L./lib -L$(ROOT)/lib -ldtn2fw $(LBP) -lici -lpthread -lm
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

bpversion:	bpversion.o $(BPLIBS)
		$(CC) -o bpversion bpversion.o -L./lib -L$(ROOT)/lib $(LBP) -lici -lpthread -lm
		cp bpversion ./bin

hmackeys:	hmackeys.o $(BPLIBS)
		$(CC) -o hmackeys hmackeys.o -L$(ROOT)/lib -lici -lpthread -lm
		cp hmackeys ./bin

#	-	-	IPN executables	-	-	-	-	-

ipnfw:		ipnfw.o $(BPLIBS) libipnfw.dll libcgr.dll
		$(CC) -o ipnfw ipnfw.o -L./lib -L$(ROOT)/lib -lipnfw -lcgr $(LBP) -lici -lpthread -lm
		cp ipnfw ./bin

ipnadminep:	ipnadminep.o $(BPLIBS) libipnfw.dll
		$(CC) -o ipnadminep ipnadminep.o -L./lib -L$(ROOT)/lib -lipnfw $(LBP) -lici -lpthread -lm
		cp ipnadminep ./bin

#	-	-	IMC executables	-	-	-	-	-

imcfw:		imcfw.o $(BPLIBS) libimcfw.dll libipnfw.dll
		$(CC) -o imcfw imcfw.o -L./lib -L$(ROOT)/lib -limcfw -lipnfw $(LBP) -lici -lpthread -lm
		cp imcfw ./bin

#	-	-	DTN2 executables	-	-	-	-

dtn2fw:		dtn2fw.o $(BPLIBS) libdtn2fw.dll
		$(CC) -o dtn2fw dtn2fw.o -L./lib -L$(ROOT)/lib -ldtn2fw $(LBP) -lici -lpthread -lm
		cp dtn2fw ./bin

dtn2adminep:	dtn2adminep.o $(BPLIBS) libdtn2fw.dll
		$(CC) -o dtn2adminep dtn2adminep.o -L./lib -L$(ROOT)/lib -ldtn2fw $(LBP) -lici -lpthread -lm
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
		$(CC) -o tcpcli tcpcli.o -L./lib -L$(ROOT)/lib $(LBP) -lici -lpthread -lws2_32 -lm
		cp tcpcli ./bin

tcpclo:		tcpclo.o $(BPLIBS)
		$(CC) -o tcpclo tcpclo.o -L./lib -L$(ROOT)/lib $(LBP) -lici -lpthread -lws2_32 -lm
		cp tcpclo ./bin

#	-	-	STCP executables	-	-	-	-

stcpcli:	stcpcli.o $(BPLIBS) libstcpcla.dll
		$(CC) -o stcpcli stcpcli.o -L./lib -L$(ROOT)/lib -lstcpcla $(LBP) -lici -lpthread -lws2_32 -lm
		cp stcpcli ./bin

stcpclo:	stcpclo.o $(BPLIBS) libstcpcla.dll
		$(CC) -o stcpclo stcpclo.o -L./lib -L$(ROOT)/lib -lstcpcla $(LBP) -lici -lpthread -lws2_32 -lm
		cp stcpclo ./bin

#	-	-	UDP executables	-	-	-	-	-

udpcli:		udpcli.o $(BPLIBS) libudpcla.dll
		$(CC) -o udpcli udpcli.o -L./lib -L$(ROOT)/lib -ludpcla $(LBP) -lici -lpthread -lws2_32 -lm
		cp udpcli ./bin

udpclo:		udpclo.o $(BPLIBS) libudpcla.dll
		$(CC) -o udpclo udpclo.o -L./lib -L$(ROOT)/lib -ludpcla $(LBP) -lici -lpthread -lws2_32 -lm
		cp udpclo ./bin

#	-	-	BSSP executables	-	-	-	-

bsspcli:	bsspcli.o $(BPLIBS)
		$(CC) -o bsspcli bsspcli.o -L./lib -L$(ROOT)/lib $(LBP) -lbssp -lici -lpthread -lm
		cp bsspcli ./bin

bsspclo:	bsspclo.o $(BPLIBS)
		$(CC) -o bsspclo bsspclo.o -L./lib -L$(ROOT)/lib $(LBP) -lbssp -lici -lpthread -lm
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

brsscla:	brsscla.o $(BPLIBS) libstcpcla.dll
		$(CC) -o brsscla brsscla.o -L./lib -L$(ROOT)/lib -lstcpcla $(LBP) -lici -lpthread -lws2_32 -lm
		cp brsscla ./bin

brsccla:	brsccla.o $(BPLIBS) libstcpcla.dll
		$(CC) -o brsccla brsccla.o -L./lib -L$(ROOT)/lib -lstcpcla $(LBP) -lici -lpthread -lws2_32 -lm
		cp brsccla ./bin

#	-	-	BIBE executables	-	-	-	-	-

bibeadmin:	bibeadmin.o $(BPLIBS)
		$(CC) -o bibeadmin bibeadmin.o -L./lib -L$(ROOT)/lib $(LBP) -lici -lpthread -lws2_32 -lm
		cp bibeadmin ./bin

bibeclo:	bibeclo.o $(BPLIBS)
		$(CC) -o bibeclo bibeclo.o -L./lib -L$(ROOT)/lib $(LBP) -lici -lpthread -lws2_32 -lm
		cp bibeclo ./bin

#	-	-	IPND executables	-	-	-	-	-

ipnd:		$(IPNDOBJS) $(BPLIBS)
		$(CC) -o ipnd $(IPNDOBJS) -L./lib -L$(ROOT)/lib $(LBP) -lici -lbloom -lpthread -lbp -lws2_32 -lm
		cp ipnd ./bin

beacon:		$(IPNDOBJS) $(BPLIBS)
		$(CC) -o beacon $(IPNDOBJS) -L./lib -L$(ROOT)/lib $(LBP) -lici -lbloom -lpthread -lbp -lws2_32 -lm
		cp beacon ./bin
		
node:		$(IPNDOBJS) $(BPLIBS)
		$(CC) -o node $(IPNDOBJS) -L./lib -L$(ROOT)/lib $(LBP) -lici -lbloom -lpthread -lbp -lws2_32 -lm
		cp node ./bin

#	-	-	Test executables	-	-	-	-

bpsource:	bpsource.o libbp.dll $(BPLIBS)
		$(CC) -o bpsource bpsource.o -L./lib -L$(ROOT)/lib $(LBP) -lici -lpthread -lm
		cp bpsource ./bin

bpsink:		bpsink.o libbp.dll $(BPLIBS)
		$(CC) -o bpsink bpsink.o -L./lib -L$(ROOT)/lib $(LBP) -lici -lpthread -lm
		cp bpsink ./bin

bpcrash:	bpcrash.o libbp.dll $(BPLIBS)
		$(CC) -o bpcrash bpcrash.o -L./lib -L$(ROOT)/lib $(LBP) -lici -lpthread -lm
		cp bpcrash ./bin

bpdriver:	bpdriver.o libbp.dll $(BPLIBS)
		$(CC) -o bpdriver bpdriver.o -L./lib -L$(ROOT)/lib $(LBP) -lici -lpthread -lm
		cp bpdriver ./bin

bpecho:		bpecho.o libbp.dll $(BPLIBS)
		$(CC) -o bpecho bpecho.o -L./lib -L$(ROOT)/lib $(LBP) -lici -lpthread -lm
		cp bpecho ./bin

bpcounter:	bpcounter.o libbp.dll $(BPLIBS)
		$(CC) -o bpcounter bpcounter.o -L./lib -L$(ROOT)/lib $(LBP) -lici -lpthread -lm
		cp bpcounter ./bin

bpsendfile:	bpsendfile.o libbp.dll $(BPLIBS)
		$(CC) -o bpsendfile bpsendfile.o -L./lib -L$(ROOT)/lib $(LBP) -lici -lpthread -lm
		cp bpsendfile ./bin

bprecvfile:	bprecvfile.o libbp.dll $(BPLIBS)
		$(CC) -o bprecvfile bprecvfile.o -L./lib -L$(ROOT)/lib $(LBP) -lici -lpthread -lm
		cp bprecvfile ./bin

bpnmtest:	bpnmtest.o libbp.dll $(BPLIBS)
		$(CC) -o bpnmtest bpnmtest.o -L./lib -L$(ROOT)/lib $(LBP) -lici -lpthread -lm
		cp bpnmtest ./bin

cgrfetch:	cgrfetch.o $(BPLIBS)
		$(CC) -o cgrfetch cgrfetch.o -L./lib -L$(ROOT)/lib $(LBP) -lici -lpthread -lm
		cp cgrfetch ./bin

#	-	-	Libraries	-	-	-	-	-

libbp.dll:	libbp.o $(BPPOBJS) libbpnm.o libimcfw.o libipnfw.o libdtn2fw.o
		$(LD) -o libbp.dll libbp.o $(BPPOBJS) libbpnm.o libimcfw.o libipnfw.o libdtn2fw.o -Wl,--output-def,libbp.def -L$(ROOT)/lib -lici -lpthread -lws2_32 -lm
		cp libbp.dll ./lib

libcgr.dll:	libcgr.o
		$(LD) -o libcgr.dll libcgr.o -Wl,--output-def,libcgr.def -L./lib -L$(ROOT)/lib -lbp -lici -lpthread -lm
		cp libcgr.dll ./lib

libipnfw.dll:	libipnfw.o
		$(LD) -o libipnfw.dll libipnfw.o -Wl,--output-def,libipnfw.def -L./lib -L$(ROOT)/lib -lici -lpthread -lbp -lm
		cp libipnfw.dll ./lib

libimcfw.dll:	libimcfw.o
		$(LD) -o libimcfw.dll libimcfw.o -Wl,--output-def,libimcfw.def -L./lib -L$(ROOT)/lib -lbp -lici -lpthread -lm
		cp libimcfw.dll ./lib

libdtn2fw.dll:	libdtn2fw.o
		$(LD) -o libdtn2fw.dll libdtn2fw.o -Wl,--output-def,libdtn2fw.def -L./lib -L$(ROOT)/lib -lbp -limcfw -lipnfw -lici -lpthread -lm
		cp libdtn2fw.dll ./lib

libstcpcla.dll:	libstcpcla.o
		$(LD) -o libstcpcla.dll libstcpcla.o -Wl,--output-def,libstcpcla.def -L./lib -L$(ROOT)/lib $(LBP) -lici -lpthread -lws2_32 -lm
		cp libstcpcla.dll ./lib

libudpcla.dll:	libudpcla.o
		$(LD) -o libudpcla.dll libudpcla.o -Wl,--output-def,libudpcla.def -L./lib -L$(ROOT)/lib $(LBP) -lici -lpthread -lws2_32 -lm
		cp libudpcla.dll ./lib

#	-	-	Object modules	-	-	-	-	-

%.o:		$(API)/%.c
		$(CC) -c $<

%.o:		$(API)/ext/bae/%.c
		$(CC) -c $<

%.o:		$(API)/ext/bpq/%.c
		$(CC) -c $<

%.o:		$(API)/ext/bpsec/%.c
		$(CC) -c $<

%.o:		$(API)/ext/hcb/%.c
		$(CC) -c $<

%.o:		$(API)/ext/meb/%.c
		$(CC) -c $<

%.o:		$(API)/ext/pnb/%.c
		$(CC) -c $<

%.o:		$(API)/ext/snw/%.c
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

%.o:		$(SAGA)/%.c
		$(CC) -c $<

%.o:		$(API)/ext/cteb/%.c
		$(CC) -c $<

%.o:		$(IPND)/%.c
		$(CC) -I$(IPND) -I$(ICI)\libbloom-master -c $<
