API = ../library
INCL = ../include
UTILS = ../utils
DAEMON = ../daemon
UDP = ../udp
DCCP = ../dccp
SDA = ../sda
TEST = ../test

# OPT = -O -Dfreebsd
OPT = -g -Wall -Werror -Dfreebsd
CC = cc $(OPT) -I$(API) -I$(API)/ext -I$(API)/ext/auth -I$(INCL) -I$(ROOT)/include
LDFLAGS = -fPIC -shared
LD = ld $(LDFLAGS)

PUBINCLS = \
	$(INCL)/ltp.h \
	$(INCL)/sda.h

LTPINCLS = \
	$(API)/ltpP.h \
	$(UDP)/udplsa.h \
	$(DCCP)/dccplsa.h

RUNTIMES = ltpadmin ltpclock ltpmeter udplsi udplso ltpdriver ltpcounter sdatest
#dccplsi dccplso

ALL = check libltpP.so libltp.so libltpnm.so libsda.so $(RUNTIMES)

all:		$(ALL)

check:		$(LTPINCLS) $(PUBINCLS)
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

ltpclock:	ltpclock.o libltpP.so
		$(CC) -o ltpclock ltpclock.o -L./lib -L$(ROOT)/lib -lltpP -lici -lpthread -lm
		cp ltpclock ./bin

ltpmeter:	ltpmeter.o libltpP.so
		$(CC) -o ltpmeter ltpmeter.o -L./lib -L$(ROOT)/lib -lltpP -lici -lpthread -lm
		cp ltpmeter ./bin

#	-	-	Utility executables	-	-	-	-

ltpadmin:	ltpadmin.o libltpP.so
		$(CC) -o ltpadmin ltpadmin.o -L./lib -L$(ROOT)/lib -lltp -lltpP -lici -lpthread -lm
		cp ltpadmin ./bin

#	-	-	Test executables	-	-	-	-

ltpdriver:	ltpdriver.o libltpP.so libltp.so
		$(CC) -o ltpdriver ltpdriver.o -L./lib -L$(ROOT)/lib -lltp -lltpP -lici -lpthread -lm
		cp ltpdriver ./bin

ltpcounter:	ltpcounter.o libltpP.so libltp.so
		$(CC) -o ltpcounter ltpcounter.o -L./lib -L$(ROOT)/lib -lltp -lltpP -lici -lpthread -lm
		cp ltpcounter ./bin

sdatest:	sdatest.o libltpP.so libltp.so libsda.so
		$(CC) -o sdatest sdatest.o -L./lib -L$(ROOT)/lib -lsda -lltp -lltpP -lici -lpthread -lm
		cp sdatest ./bin

#	-	-	UDP executables	-	-	-	-	-

udplsi:		udplsi.o libltpP.so
		$(CC) -o udplsi udplsi.o -L./lib -L$(ROOT)/lib -lltpP -lici -lpthread -lm
		cp udplsi ./bin

udplso:		udplso.o libltpP.so
		$(CC) -o udplso udplso.o -L./lib -L$(ROOT)/lib -lltpP -lici -lpthread -lm
		cp udplso ./bin

#	-	-	DCCP executables-	-	-	-	-
dccplsi:	dccplsi.o libltpP.so
		$(CC) -o dccplsi dccplsi.o -L./lib -L$(ROOT)/lib -lltpP -lici -lpthread -lm
		cp dccplsi ./bin

dccpplso:	dccplso.o libltpP.so
		$(CC) -o dccplso dccplso.o -L./lib -L$(ROOT)/lib -lltpP -lici -lpthread -lm
		cp dccplso ./bin

#	-	-	Libraries	-	-	-	-	-

libltpP.so:	libltpP.o ltpei.o auth.o
		$(LD) -o libltpP.so libltpP.o ltpei.o auth.o
		cp libltpP.so ./lib

libltp.so:	libltp.o ltpei.o auth.o
		$(LD) -o libltp.so libltp.o ltpei.o auth.o
		cp libltp.so ./lib

libltpnm.so:	libltpnm.o $(API)/ltpnm.h
		$(LD) -o libltpnm.so libltpnm.o
		cp libltpnm.so ./lib

libsda.so:	libsda.o $(INCL)/sda.h
		$(LD) -o libsda.so libsda.o
		cp libsda.so ./lib

#	-	-	Object modules	-	-	-	-	-

%.o:		$(API)/%.c
		$(CC) -c $<

%.o:		$(API)/ext/auth/%.c
		$(CC) -c $<

%.o:		$(UTILS)/%.c
		$(CC) -c $<

%.o:		$(TEST)/%.c
		$(CC) -c $<

%.o:		$(DAEMON)/%.c
		$(CC) -c $<

%.o:		$(UDP)/%.c
		$(CC) -c $<

%.o:		$(SDA)/%.c
		$(CC) -c $<

#%.o:		$(DCCP)/%.c
#		$(CC) -c $<
