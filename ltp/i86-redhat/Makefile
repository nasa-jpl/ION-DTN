API = ../library
INCL = ../include
UTILS = ../utils
DAEMON = ../daemon
UDP = ../udp
DCCP = ../dccp
SDA = ../sda
TEST = ../test

# OPT = -O -Dlinux
OPT = -g -Wall -Werror -Dlinux -DHEAP_PTRS=$(PTRS)
CC = gcc $(OPT) -I$(API) -I$(API)/ext -I$(API)/ext/auth -I$(INCL) -I$(ROOT)/include
LDFLAGS = -fPIC -shared -rdynamic
LD = gcc $(LDFLAGS)

PUBINCLS = \
	$(INCL)/ltp.h \
	$(INCL)/sda.h

LTPINCLS = \
	$(API)/ltpP.h \
	$(API)/ltpsec.h \
	$(UDP)/udplsa.h \
	$(DCCP)/dccplsa.h

RUNTIMES = ltpadmin ltpsecadmin ltpclock ltpdeliv ltpmeter udplsi udplso ltpdriver ltpcounter sdatest
#dccplsi dccplso

ALL = check libltp.so $(RUNTIMES)

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

ltpclock:	ltpclock.o libltp.so
		$(CC) -o ltpclock ltpclock.o -L./lib -L$(ROOT)/lib -lltp -lici -lpthread -lm
		cp ltpclock ./bin

ltpdeliv:	ltpdeliv.o libltp.so
		$(CC) -o ltpdeliv ltpdeliv.o -L./lib -L$(ROOT)/lib -lltp -lici -lpthread -lm
		cp ltpdeliv ./bin

ltpmeter:	ltpmeter.o libltp.so
		$(CC) -o ltpmeter ltpmeter.o -L./lib -L$(ROOT)/lib -lltp -lici -lpthread -lm
		cp ltpmeter ./bin

#	-	-	Utility executables	-	-	-	-

ltpadmin:	ltpadmin.o libltp.so
		$(CC) -o ltpadmin ltpadmin.o -L./lib -L$(ROOT)/lib -lltp -lici -lpthread -lm
		cp ltpadmin ./bin

ltpsecadmin:	ltpsecadmin.o libltp.so
		$(CC) -o ltpsecadmin ltpsecadmin.o -L./lib -L$(ROOT)/lib -lltp -lici -lpthread -lm
		cp ltpsecadmin ./bin

#	-	-	Test executables	-	-	-	-

ltpdriver:	ltpdriver.o libltp.so
		$(CC) -o ltpdriver ltpdriver.o -L./lib -L$(ROOT)/lib -lltp -lici -lpthread -lm
		cp ltpdriver ./bin

ltpcounter:	ltpcounter.o libltp.so
		$(CC) -o ltpcounter ltpcounter.o -L./lib -L$(ROOT)/lib -lltp -lici -lpthread -lm
		cp ltpcounter ./bin

sdatest:	sdatest.o libltp.so
		$(CC) -o sdatest sdatest.o -L./lib -L$(ROOT)/lib -lltp -lici -lpthread -lm
		cp sdatest ./bin

#	-	-	UDP executables	-	-	-	-	-

udplsi:		udplsi.o libltp.so
		$(CC) -o udplsi udplsi.o -L./lib -L$(ROOT)/lib -lltp -lici -lpthread -lm
		cp udplsi ./bin

udplso:		udplso.o libltp.so
		$(CC) -o udplso udplso.o -L./lib -L$(ROOT)/lib -lltp -lici -lpthread -lm
		cp udplso ./bin

#	-	-	DCCP executables-	-	-	-	-
dccplsi:	dccplsi.o libltp.so
		$(CC) -o dccplsi dccplsi.o -L./lib -L$(ROOT)/lib -lltp -lici -lpthread -lm
		cp dccplsi ./bin

dccpplso:	dccplso.o libltp.so
		$(CC) -o dccplso dccplso.o -L./lib -L$(ROOT)/lib -lltp -lici -lpthread -lm
		cp dccplso ./bin

#	-	-	Libraries	-	-	-	-	-

libltp.so:	libltp.o libltpP.o ltpsec.o ltpei.o auth.o libltpnm.o libsda.o
		$(LD) -o libltp.so libltp.o libltpP.o ltpsec.o ltpei.o auth.o libltpnm.o libsda.o
		cp libltp.so ./lib

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
