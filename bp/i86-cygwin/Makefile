API = ../library
INCL = ../include
UTILS = ../utils
TEST = ../test
DAEMON = ../daemon
IP = ../ip

OPT = -g -Wall -Werror -Dcygwin
CC = gcc $(OPT) -I$(API) -I$(INCL) -I$(ROOT)/include
LDFLAGS = -shared
LD = gcc $(LDFLAGS)

PUBINCLS = \
	$(INCL)/bp.h

BPINCLS = \
	$(API)/bpP.h

IPINCLS = \
	$(IP)/iprfw.h \
	$(IP)/dgrcla.h \
	$(IP)/tcpcla.h

RUNTIMES = bpadmin.exe bpsink.exe bpsource.exe bpdriver.exe bpecho.exe bpclock.exe ipadmin.exe iprfw.exe ipadminep.exe tcpcli.exe tcpclo.exe
# dgrclo.exe dgrcli.exe

ALL = check libbpP.dll libbp.dll libiprfw.dll $(RUNTIMES)

all:		$(ALL)

check:		$(BPINCLS) $(PUBINCLS) $(IPINCLS)
		rm -f *.o
		touch check

clean:
		rm -f *.o
		rm -f *.a
		rm -f $(ALL)
		rm -f ./lib/*.a
		rm -f ./bin/*.dll
		rm -f ./bin/*.exe

install:
		cp ../include/* $(ROOT)/include
		cp lib/*.a $(ROOT)/lib
		cp bin/*.dll /c/windows/system32
		cp bin/*.exe /c/windows/system32

#	-	-	Daemon executables	-	-	-	-

bpclock.exe:	bpclock.o libbpP.dll
		$(CC) -o bpclock.exe bpclock.o -L./lib -L$(ROOT)/lib -lbp -lbpP -lici -lpthread
		cp bpclock.exe ./bin

#	-	-	Utility executables	-	-	-	-

bpadmin.exe:	bpadmin.o libbpP.dll
		$(CC) -o bpadmin.exe bpadmin.o -L./lib -L$(ROOT)/lib -lbp -lbpP -lici -lpthread
		cp bpadmin.exe ./bin

#	-	-	IP executables	-	-	-	-	-

iprfw.exe:	iprfw.o libbpP.dll libiprfw.dll
		$(CC) -o iprfw.exe iprfw.o -L./lib -L$(ROOT)/lib -liprfw -lbp -lbpP -lici -lpthread
		cp iprfw.exe ./bin

ipadminep.exe:	ipadminep.o libbpP.dll libiprfw.dll
		$(CC) -o ipadminep.exe ipadminep.o -L./lib -L$(ROOT)/lib -liprfw -lbp -lbpP -lici -lpthread
		cp ipadminep.exe ./bin

tcpcli.exe:	tcpcli.o libbpP.dll libiprfw.dll
		$(CC) -o tcpcli.exe tcpcli.o -L./lib -L$(ROOT)/lib -liprfw -lbp -lbpP -lici -lpthread
		cp tcpcli.exe ./bin

tcpclo.exe:	tcpclo.o libbpP.dll libiprfw.dll
		$(CC) -o tcpclo.exe tcpclo.o -L./lib -L$(ROOT)/lib -liprfw -lbp -lbpP -lici -lpthread
		cp tcpclo.exe ./bin

dgrcli.exe:	dgrcli.o libbpP.dll libiprfw.dll
		$(CC) -o dgrcli.exe dgrcli.o -L./lib -L$(ROOT)/lib -liprfw -lbp -lbpP -ldgr -lici -lpthread
		cp dgrcli.exe ./bin

dgrclo.exe:	dgrclo.o libbpP.dll libiprfw.dll
		$(CC) -o dgrclo.exe dgrclo.o -L./lib -L$(ROOT)/lib -liprfw -lbp -lbpP -ldgr -lici -lpthread
		cp dgrclo.exe ./bin

ipadmin.exe:	ipadmin.o libbpP.dll libiprfw.dll
		$(CC) -o ipadmin.exe ipadmin.o -L./lib -L$(ROOT)/lib -liprfw -lbp -lbpP -lici -lpthread
		cp ipadmin.exe ./bin

#	-	-	Test executables	-	-	-	-

bpsource.exe:	bpsource.o libbp.dll libbpP.dll
		$(CC) -o bpsource.exe bpsource.o -L./lib -L$(ROOT)/lib -lbp -lbpP -lici -lpthread
		cp bpsource.exe ./bin

bpsink.exe:	bpsink.o libbp.dll libbpP.dll
		$(CC) -o bpsink.exe bpsink.o -L./lib -L$(ROOT)/lib -lbp -lbpP -lici -lpthread
		cp bpsink.exe ./bin

bpdriver.exe:	bpdriver.o libbp.dll libbpP.dll
		$(CC) -o bpdriver.exe bpdriver.o -L./lib -L$(ROOT)/lib -lbp -lbpP -lici -lpthread
		cp bpdriver.exe ./bin

bpecho.exe:	bpecho.o libbp.dll libbpP.dll
		$(CC) -o bpecho.exe bpecho.o -L./lib -L$(ROOT)/lib -lbp -lbpP -lici -lpthread
		cp bpecho.exe ./bin

#	-	-	Libraries	-	-	-	-	-

libbpP.dll:	libbpP.o libbp.o
		$(LD) -o libbpP.dll libbpP.o libbp.o -Wl,--out-implib,libbpP.a -L/opt/lib -lici
		cp libbpP.dll ./bin
		cp libbpP.a ./lib

libbp.dll:	libbp.o libbpP.o
		$(LD) -o libbp.dll libbp.o libbpP.o -Wl,--out-implib,libbp.a -L/opt/lib -lici
		cp libbp.dll ./bin
		cp libbp.a ./lib

libiprfw.dll:	libiprfw.o libbp.o libbpP.o
		$(LD) -o libiprfw.dll libiprfw.o libbp.o libbpP.o -Wl,--out-implib,libiprfw.a -L/opt/lib -lici
		cp libiprfw.dll ./bin
		cp libiprfw.a ./lib

#	-	-	Object modules	-	-	-	-	-

%.o:		$(API)/%.c
		$(CC) -c $<

%.o:		$(UTILS)/%.c
		$(CC) -c $<

%.o:		$(TEST)/%.c
		$(CC) -c $<

%.o:		$(DAEMON)/%.c
		$(CC) -c $<

%.o:		$(IP)/%.c
		$(CC) -c $<
