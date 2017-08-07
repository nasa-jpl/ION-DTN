API = ../library
INCL = ../include
UTILS = ../utils
DAEMON = ../daemon
UDP = ../udp
SDA = ../sda
TEST = ../test

OPT = -g -Wall -Dmingw
CC = gcc $(OPT) -I$(API) -I$(API)/ext -I$(API)/ext/auth -I$(INCL) -I$(ROOT)/include
LDFLAGS = -fPIC -shared
LD = gcc $(LDFLAGS)

PUBINCLS = \
	$(INCL)/ltp.h \
	$(INCL)/sda.h

LTPINCLS = \
	$(API)/ltpP.h \
	$(UDP)/udplsa.h

RUNTIMES = ltpadmin ltpclock ltpmeter ltpdeliv udplsi udplso ltpdriver ltpcounter sdatest

ALL = check libltp.dll $(RUNTIMES)

all:		$(ALL)

check:		$(LTPINCLS) $(PUBINCLS)
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

ltpclock:	ltpclock.o libltp.dll
		$(CC) -o ltpclock ltpclock.o -L./lib -L$(ROOT)/lib -lltp -lici -lpthread -lm
		cp ltpclock ./bin

ltpmeter:	ltpmeter.o libltp.dll
		$(CC) -o ltpmeter ltpmeter.o -L./lib -L$(ROOT)/lib -lltp -lici -lpthread -lm
		cp ltpmeter ./bin
ltpdeliv:	ltpdeliv.o libltp.dll
		$(CC) -o ltpdeliv ltpdeliv.o -L./lib -L$(ROOT)/lib -lltp -lici -lpthread -lm
		cp ltpdeliv ./bin

#	-	-	Utility executables	-	-	-	-

ltpadmin:	ltpadmin.o libltp.dll
		$(CC) -o ltpadmin ltpadmin.o -L./lib -L$(ROOT)/lib -lltp -lici -lpthread -lm
		cp ltpadmin ./bin

#	-	-	Test executables	-	-	-	-

ltpdriver:	ltpdriver.o libltp.dll
		$(CC) -o ltpdriver ltpdriver.o -L./lib -L$(ROOT)/lib -lltp -lici -lpthread -lm
		cp ltpdriver ./bin

ltpcounter:	ltpcounter.o libltp.dll
		$(CC) -o ltpcounter ltpcounter.o -L./lib -L$(ROOT)/lib -lltp -lici -lpthread -lm
		cp ltpcounter ./bin

sdatest:	sdatest.o libltp.dll
		$(CC) -o sdatest sdatest.o -L./lib -L$(ROOT)/lib -lltp -lici -lpthread -lm
		cp sdatest ./bin

#	-	-	UDP executables	-	-	-	-	-

udplsi:		udplsi.o libltp.dll
		$(CC) -o udplsi udplsi.o -L./lib -L$(ROOT)/lib -lltp -lici -lpthread -lws2_32 -lm
		cp udplsi ./bin

udplso:		udplso.o libltp.dll
		$(CC) -o udplso udplso.o -L./lib -L$(ROOT)/lib -lltp -lici -lpthread -lws2_32 -lm
		cp udplso ./bin

#	-	-	Libraries	-	-	-	-	-

libltp.dll:	libltpP.o ltpei.o auth.o libltp.o libltpnm.o libsda.o
		$(LD) -o libltp.dll libltpP.o ltpei.o auth.o libltp.o libltpnm.o libsda.o -Wl,--output-def,libltp.def -L$(ROOT)/lib -lici -lpthread
		cp libltp.dll ./lib

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
