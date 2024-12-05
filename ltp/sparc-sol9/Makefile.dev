API = ../library
INCL = ../include
UTILS = ../utils
DAEMON = ../daemon
UDP = ../udp
TEST = ../test

OPT = -g -Wall -Werror -Dsol5 -Dunix -D__SVR4 -D_REENTRANT -fPIC
CC = gcc $(OPT) -I$(API) -I$(INCL) -I$(ROOT)/include
LDFLAGS = -fPIC -shared
LD = gcc $(LDFLAGS)

PUBINCLS = \
	$(INCL)/ltp.h

LTPINCLS = \
	$(API)/ltpP.h \
	$(UDP)/udplsa.h

RUNTIMES = ltpadmin ltpclock ltpmeter udplsi udplso ltpdriver ltpcounter

ALL = check libltpP.so libltp.so $(RUNTIMES)

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
		$(CC) -o ltpclock ltpclock.o -L./lib -L$(ROOT)/lib -lltpP -lici -lpthread -lrt -lsocket
		cp ltpclock ./bin

ltpmeter:	ltpmeter.o libltpP.so
		$(CC) -o ltpmeter ltpmeter.o -L./lib -L$(ROOT)/lib -lltpP -lici -lpthread -lrt -lsocket
		cp ltpmeter ./bin

#	-	-	Utility executables	-	-	-	-

ltpadmin:	ltpadmin.o libltpP.so
		$(CC) -o ltpadmin ltpadmin.o -L./lib -L$(ROOT)/lib -lltp -lltpP -lici -lpthread -lrt -lsocket
		cp ltpadmin ./bin

#	-	-	Test executables	-	-	-	-

ltpdriver:	ltpdriver.o libltpP.so libltp.so
		$(CC) -o ltpdriver ltpdriver.o -L./lib -L$(ROOT)/lib -lltp -lltpP -lici -lpthread -lrt -lsocket
		cp ltpdriver ./bin

ltpcounter:	ltpcounter.o libltpP.so libltp.so
		$(CC) -o ltpcounter ltpcounter.o -L./lib -L$(ROOT)/lib -lltp -lltpP -lici -lpthread -lrt -lsocket
		cp ltpcounter ./bin

#	-	-	UDP executables	-	-	-	-	-

udplsi:		udplsi.o libltpP.so
		$(CC) -o udplsi udplsi.o -L./lib -L$(ROOT)/lib -lltpP -lici -lpthread -lrt -lsocket
		cp udplsi ./bin

udplso:		udplso.o libltpP.so
		$(CC) -o udplso udplso.o -L./lib -L$(ROOT)/lib -lltpP -lici -lpthread -lrt -lsocket
		cp udplso ./bin

#	-	-	Libraries	-	-	-	-	-

libltpP.so:	libltpP.o
		$(LD) -o libltpP.so libltpP.o
		cp libltpP.so ./lib

libltp.so:	libltp.o
		$(LD) -o libltp.so libltp.o
		cp libltp.so ./lib

#	-	-	Object modules	-	-	-	-	-

%.o:		$(API)/%.c
		$(CC) -c $<

%.o:		$(UTILS)/%.c
		$(CC) -c $<

%.o:		$(TEST)/%.c
		$(CC) -c $<

%.o:		$(DAEMON)/%.c
		$(CC) -c $<

%.o:		$(UDP)/%.c
		$(CC) -c $<
