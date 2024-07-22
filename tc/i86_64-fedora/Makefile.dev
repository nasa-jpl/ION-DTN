API = ../library
INCL = ../include
AUTHORITY = ../authority
CLIENT = ../client
DTKA = ../dtka
ZFEC = ../../ici/zfec-1.4.24/zfec
BP_V = ../../bpv7

# OPT = -O -Dlinux
OPT = -g -Wall -Werror -Dlinux -fPIC -DSPACE_ORDER=3 -DNDEBUG
CC = gcc $(OPT) -I$(AUTHORITY) -I$(CLIENT) -I$(INCL) -I$(ZFEC) -I$(BP_V)/library -I$(ROOT)/include
LDFLAGS = -fPIC -shared
LD = gcc $(LDFLAGS)

TCINCLS = \
	$(DTKA)/dtka.h \
	$(AUTHORITY)/tcaP.h \
	$(CLIENT)/tcc.h \
	$(CLIENT)/tccP.h \
	$(INCL)/tc.h

LBP = -lbp

RUNTIMES = tcaboot tcarecv tcacompile tcapublish tcaadmin tcc tccadmin dtka dtkaadmin

ALL = check libtc.so libtcaP.so libtcc.so libtccP.so libdtka.so libzfec.so $(RUNTIMES)

all:		$(ALL)

check:		$(TCINCLS)
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

#	-	-	Authority executables	-	-	-	-

tcarecv:		tcarecv.o libtcaP.so
		$(CC) -o tcarecv tcarecv.o -L./lib -L$(ROOT)/lib -ltcaP -ltc $(LBP) -lici -lpthread -lm
		cp tcarecv ./bin

tcacompile:	tcacompile.o libtcaP.so
		$(CC) -o tcacompile tcacompile.o -L./lib -L$(ROOT)/lib -ltcaP -ltc $(LBP) -lici -lpthread -lm
		cp tcacompile ./bin

tcapublish:	tcapublish.o libtcaP.so
		$(CC) -o tcapublish tcapublish.o -L./lib -L$(ROOT)/lib -ltcaP -ltc $(LBP) -lici -lzfec -lpthread -lm
		cp tcapublish ./bin

tcaadmin:	tcaadmin.o libtcaP.so
		$(CC) -o tcaadmin tcaadmin.o -L./lib -L$(ROOT)/lib -ltcaP -ltc $(LBP) -lici -lpthread -lm
		cp tcaadmin ./bin

tcaboot:	tcaboot.o libtcaP.so
		$(CC) -o tcaboot tcaboot.o -L./lib -L$(ROOT)/lib -ltcaP -ltc $(LBP) -lici -lpthread -lm
		cp tcaboot ./bin

libtcaP.so:	libtcaP.o
		$(LD) -o libtcaP.so libtcaP.o
		cp libtcaP.so ./lib

#	-	-	Client executables	-	-	-	-

tcc:		tcc.o libtc.so libtcc.so libtccP.so
		$(CC) -o tcc tcc.o -L./lib -L$(ROOT)/lib -ltccP -ltcc -ltc $(LBP) -lici -lzfec -lpthread -lm
		cp tcc ./bin

tccadmin:	tccadmin.o libtc.so libtcc.so libtccP.so
		$(CC) -o tccadmin tccadmin.o -L./lib -L$(ROOT)/lib -ltccP -ltcc -ltc $(LBP) -lici -lpthread -lm
		cp tccadmin ./bin

libtcc.so:	libtcc.o
		$(LD) -o libtcc.so libtcc.o
		cp libtcc.so ./lib

libtccP.so:	libtccP.o
		$(LD) -o libtccP.so libtccP.o
		cp libtccP.so ./lib

#	-	-	Library	-	-	-	-	-	-

libtc.so:	libtc.o
		$(LD) -o libtc.so libtc.o
		cp libtc.so ./lib

#	-	-	zfec	-	-	-	-	-	-

libzfec.so:	$(ZFEC)/fec.o
		$(LD) -o libzfec.so $(ZFEC)/fec.o
		cp libzfec.so ./lib

#	-	-	Application -	-	-	-	-	-

dtka:		dtka.o libdtka.so libtcc.so
		$(CC) -o dtka dtka.o -L./lib -L$(ROOT)/lib -ldtka -ltccP -ltcc -ltc $(LBP) -lici -lpthread -lm
		cp dtka ./bin

dtkaadmin:	dtkaadmin.o libdtka.so
		$(CC) -o dtkaadmin dtkaadmin.o -L./lib -L$(ROOT)/lib -ldtka -ltccP -ltcc -ltc $(LBP) -lici -lpthread -lm
		cp dtkaadmin ./bin

libdtka.so:	libdtka.o
		$(LD) -o libdtka.so libdtka.o
		cp libdtka.so ./lib

#	-	-	Object modules	-	-	-	-	-

%.o:		$(API)/%.c
		$(CC) -c $<

%.o:		$(AUTHORITY)/%.c
		$(CC) -c $<

%.o:		$(CLIENT)/%.c
		$(CC) -c $<

%.o:		$(DTKA)/%.c
		$(CC) -c $<

%.o:		$(ZFEC)/%.c
		$(CC) -c $(ZFEC)/%.c
