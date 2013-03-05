API = ../library
INCL = ../include
UTILS = ../utils
DAEMON = ../daemon
UDP = ../udp
DCCP = ../dccp
TEST = ../test

# OPT = -O -Dlinux
OPT = -g -Wall -Werror -Dlinux -fPIC -DSPACE_ORDER=3
CC = gcc $(OPT) -I$(API) -I$(INCL) -I$(ROOT)/include
LDFLAGS = -fPIC -shared
LD = gcc $(LDFLAGS)

PUBINCLS = \
	$(INCL)/ltp.h

LTPINCLS = \
	$(API)/ltpP.h \
	$(UDP)/udplsa.h \
	$(DCCP)/dccplsa.h

RUNTIMES = ltpadmin ltpclock ltpmeter udplsi udplso ltpdriver ltpcounter
#dccplsi dccplso

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
		$(CC) -o ltpclock ltpclock.o -L./lib -L$(ROOT)/lib -lltpP -lici -lpthread
		cp ltpclock ./bin

ltpmeter:	ltpmeter.o libltpP.so
		$(CC) -o ltpmeter ltpmeter.o -L./lib -L$(ROOT)/lib -lltpP -lici -lpthread
		cp ltpmeter ./bin

#	-	-	Utility executables	-	-	-	-

ltpadmin:	ltpadmin.o libltpP.so
		$(CC) -o ltpadmin ltpadmin.o -L./lib -L$(ROOT)/lib -lltp -lltpP -lici -lpthread
		cp ltpadmin ./bin

#	-	-	Test executables	-	-	-	-

ltpdriver:	ltpdriver.o libltpP.so libltp.so
		$(CC) -o ltpdriver ltpdriver.o -L./lib -L$(ROOT)/lib -lltp -lltpP -lici -lpthread
		cp ltpdriver ./bin

ltpcounter:	ltpcounter.o libltpP.so libltp.so
		$(CC) -o ltpcounter ltpcounter.o -L./lib -L$(ROOT)/lib -lltp -lltpP -lici -lpthread
		cp ltpcounter ./bin

#	-	-	UDP executables	-	-	-	-	-

udplsi:		udplsi.o libltpP.so
		$(CC) -o udplsi udplsi.o -L./lib -L$(ROOT)/lib -lltpP -lici -lpthread
		cp udplsi ./bin

udplso:		udplso.o libltpP.so
		$(CC) -o udplso udplso.o -L./lib -L$(ROOT)/lib -lltpP -lici -lpthread
		cp udplso ./bin

#	-	-	DCCP executables-	-	-	-	-
dccplsi:	dccplsi.o libltpP.so
		$(CC) -o dccplsi dccplsi.o -L./lib -L$(ROOT)/lib -lltpP -lici -lpthread
		cp dccplsi ./bin

dccpplso:	dccplso.o libltpP.so
		$(CC) -o dccplso dccplso.o -L./lib -L$(ROOT)/lib -lltpP -lici -lpthread
		cp dccplso ./bin

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

#%.o:		$(DCCP)/%.c
#		$(CC) -c $<
