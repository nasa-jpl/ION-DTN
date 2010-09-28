API = ../library
INCL = ../include
UTILS = ../utils
DAEMON = ../daemon
IP = ../ip

# OPT = -O -Dlinux
OPT = -g -Wall -Werror -Dlinux

ARMROOT = /arm
ARMDIR = /usr/local/arm/2.95.3
CC = $(ARMDIR)/bin/arm-linux-gcc $(OPT) -I$(ARMDIR)/include -I$(API) -I$(INCL) -I$(ARMROOT)/include
LDFLAGS = -fPIC -shared
LD = $(ARMDIR)/bin/arm-linux-gcc $(LDFLAGS)

PUBINCLS = \
	$(INCL)/ion.h

IONINCLS = \
	$(API)/ionP.h

IPINCLS = \
	$(IP)/iprfw.h \
	$(IP)/dgrcla.h \
	$(IP)/tcpcla.h

RUNTIMES = ionadmin ionsink ionsource iondriver ionecho ionclock ipadmin iprfw tcpcli tcpclo
# dgrclo dgrcli

ALL = check libionfw.so libion.so libiprfw.so $(RUNTIMES)

all:		$(ALL)

check:		$(IONINCLS) $(PUBINCLS) $(DAEMONINCLS)
		rm -f *.o
		touch check

clean:
		rm -f *.o
		rm -f $(ALL)
		rm -f ./lib/*
		rm -f ./bin/*

install:
		cp ../include/* $(ARMROOT)/include
		cp lib/* $(ARMROOT)/lib
		cp bin/* $(ARMROOT)/bin

#	-	-	Daemon executables	-	-	-	-

ionclock:	ionclock.o libionfw.so
		$(CC) -o ionclock ionclock.o -L./lib -L$(ARMDIR)/lib -L$(ARMROOT)/lib -lion -lionfw -lzco -lici -lpthread
		cp ionclock ./bin

#	-	-	Utility executables	-	-	-	-

ionadmin:	ionadmin.o libionfw.so
		$(CC) -o ionadmin ionadmin.o -L./lib -L$(ARMDIR)/lib -L$(ARMROOT)/lib -lion -lionfw -lzco -lici -lpthread
		cp ionadmin ./bin

#	-	-	IP executables	-	-	-	-

iprfw:		iprfw.o libionfw.so libiprfw.so
		$(CC) -o iprfw iprfw.o -L./lib -L$(ARMDIR)/lib -L$(ARMROOT)/lib -lion -lionfw -lzco -lici -lpthread
		cp iprfw ./bin

tcpcli:		tcpcli.o libionfw.so libiprfw.so
		$(CC) -o tcpcli tcpcli.o -L./lib -L$(ARMDIR)/lib -L$(ARMROOT)/lib -lion -lionfw -lzco -lici -lpthread
		cp tcpcli ./bin

tcpclo:		tcpclo.o libionfw.so libiprfw.so
		$(CC) -o tcpclo tcpclo.o -L./lib -L$(ARMDIR)/lib -L$(ARMROOT)/lib -lion -lionfw -lzco -lici -lpthread
		cp tcpclo ./bin

dgrcli:		dgrcli.o libionfw.so libiprfw.so
		$(CC) -o dgrcli dgrcli.o -L./lib -L$(ARMDIR)/lib -L$(ARMROOT)/lib -lion -lionfw -lzco -lici -lpthread
		cp dgrcli ./bin

dgrclo:		dgrclo.o libionfw.so libiprfw.so
		$(CC) -o dgrclo dgrclo.o -L./lib -L$(ARMDIR)/lib -L$(ARMROOT)/lib -lion -lionfw -lzco -lici -lpthread
		cp dgrclo ./bin

ipadmin:	ipadmin.o libionfw.so libiprfw.so
		$(CC) -o ipadmin ipadmin.o -L./lib -L$(ARMDIR)/lib -L$(ARMROOT)/lib -lion -lionfw -lzco -lici -lpthread
		cp ipadmin ./bin

#	-	-	Test executables	-	-	-	-

ionsource:	ionsource.o libion.so libionfw.so
		$(CC) -o ionsource ionsource.o -L./lib -L$(ARMDIR)/lib -L$(ARMROOT)/lib -lion -lionfw -lzco -lici -lpthread
		cp ionsource ./bin

ionsink:	ionsink.o libion.so libionfw.so
		$(CC) -o ionsink ionsink.o -L./lib -L$(ARMDIR)/lib -L$(ARMROOT)/lib -lion -lionfw -lzco -lici -lpthread
		cp ionsink ./bin

iondriver:	iondriver.o libion.so libionfw.so
		$(CC) -o iondriver iondriver.o -L./lib -L$(ARMDIR)/lib -L$(ARMROOT)/lib -lion -lionfw -lzco -lici -lpthread
		cp iondriver ./bin

ionecho:	ionecho.o libion.so libionfw.so
		$(CC) -o ionecho ionecho.o -L./lib -L$(ARMDIR)/lib -L$(ARMROOT)/lib -lion -lionfw -lzco -lici -lpthread
		cp ionecho ./bin

#	-	-	Libraries	-	-	-	-	-

libionfw.so:	libionfw.o
		$(LD) -o libionfw.so libionfw.o
		cp libionfw.so ./lib

libion.so:	libion.o
		$(LD) -o libion.so libion.o
		cp libion.so ./lib

libiprfw.so:	libiprfw.o
		$(LD) -o libiprfw.so libiprfw.o
		cp libiprfw.so ./lib

#	-	-	Object modules	-	-	-	-	-

%.o:		$(API)/%.c
		$(CC) -c $<

%.o:		$(UTILS)/%.c
		$(CC) -c $<

%.o:		$(DAEMON)/%.c
		$(CC) -c $<

%.o:		$(IP)/%.c
		$(CC) -c $<
