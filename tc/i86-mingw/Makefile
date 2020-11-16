API = ../library
INCL = ../include
AUTHORITY = ../authority
CLIENT = ../client
DTKA = ../dtka
ZFEC = ../../ici/zfec-1.4.24/zfec
BP = ../../bpv7

OPT = -g -Wall -Dmingw -DNDEBUG
CC = gcc $(OPT) -I$(AUTHORITY) -I$(CLIENT) -I$(INCL) -I$(ZFEC) -I$(BP)/library -I$(ROOT)/include
LDFLAGS = -fPIC -shared
LD = gcc $(LDFLAGS)

TCINCLS = \
	$(DTKA)/dtka.h \
	$(AUTHORITY)/tcaP.h \
	$(CLIENT)/tcc.h \
	$(CLIENT)/tccP.h \
	$(INCL)/tc.h

LBP = -lbp -limcfw -lipnfw -ldtn2fw -lcgr

RUNTIMES = tcaboot tcarecv tcacompile tcapublish tcaadmin tcc tccadmin dtka dtkaadmin

ALL = check libtc.dll libtcaP.dll libtcc.dll libtccP.dll libdtka.dll libzfec.dll $(RUNTIMES)

all:		$(ALL)

check:		$(TCINCLS)
		rm -f *.o
		touch check

clean:
		rm -f *.o
		rm -f $(ZFEC)/fec.o
		rm -f *.exe
		rm -f *.def
		rm -f $(ALL)
		rm -f ./lib/*
		rm -f ./bin/*

install:
		cp ../include/* $(ROOT)/include
		cp lib/* $(ROOT)/lib
		cp bin/* $(ROOT)/bin

#	-	-	Authority executables	-	-	-	-

tcarecv:		tcarecv.o libtcaP.dll
		$(CC) -o tcarecv tcarecv.o -L./lib -L$(ROOT)/lib -ltcaP -ltc $(LBP) -lici -lpthread -lm
		cp tcarecv ./bin

tcacompile:	tcacompile.o libtcaP.dll
		$(CC) -o tcacompile tcacompile.o -L./lib -L$(ROOT)/lib -ltcaP -ltc $(LBP) -lws2_32 -lici -lpthread -lm
		cp tcacompile ./bin

tcapublish:	tcapublish.o libtcaP.dll
		$(CC) -o tcapublish tcapublish.o -L./lib -L$(ROOT)/lib -lws2_32 -ltcaP -ltc $(LBP) -lici -lzfec -lpthread -lm
		cp tcapublish ./bin

tcaadmin:	tcaadmin.o libtcaP.dll
		$(CC) -o tcaadmin tcaadmin.o -L./lib -L$(ROOT)/lib -ltcaP -ltc $(LBP) -lici -lpthread -lm
		cp tcaadmin ./bin

tcaboot:	tcaboot.o libtcaP.dll
		$(CC) -o tcaboot tcaboot.o -L./lib -L$(ROOT)/lib -ltcaP -ltc $(LBP) -lici -lpthread -lm
		cp tcaboot ./bin

libtcaP.dll:	libtcaP.o
		$(LD) -o libtcaP.dll libtcaP.o -Wl,--output-def,libtcaP.def -L$(ROOT)/lib -lws2_32 -lici -lbp
		cp libtcaP.dll ./lib

#	-	-	Client executables	-	-	-	-

tcc:		tcc.o libtc.dll libtcc.dll libtccP.dll
		$(CC) -o tcc tcc.o -L./lib -L$(ROOT)/lib -lws2_32 -ltccP -ltcc -ltc $(LBP) -lici -lzfec -lpthread -lm
		cp tcc ./bin

tccadmin:	tccadmin.o libtc.dll libtcc.dll libtccP.dll
		$(CC) -o tccadmin tccadmin.o -L./lib -L$(ROOT)/lib -ltccP -ltcc -ltc $(LBP) -lici -lpthread -lm
		cp tccadmin ./bin

libtccP.dll:	libtccP.o
		$(LD) -o libtccP.dll libtccP.o -Wl,--output-def,libtccP.def -L$(ROOT)/lib -lws2_32 -lici -lbp
		cp libtccP.dll ./lib

libtcc.dll:	libtcc.o libtccP.dll
		$(LD) -o libtcc.dll libtcc.o -Wl,--output-def,libtcc.def -L./lib -L$(ROOT)/lib -lws2_32 -lici -ltccP
		cp libtcc.dll ./lib

#	-	-	Library	-	-	-	-	-	-

libtc.dll:	libtc.o
		$(LD) -o libtc.dll libtc.o -Wl,--output-def,libtc.def -L$(ROOT)/lib -lws2_32 -lici
		cp libtc.dll ./lib

#	-	-	zfec	-	-	-	-	-	-

libzfec.dll:	$(ZFEC)/fec.o
		$(LD) -o libzfec.dll $(ZFEC)/fec.o -Wl,--output-def,libzfec.def -L$(ROOT)/lib -lws2_32
		cp libzfec.dll ./lib

#	-	-	Application -	-	-	-	-	-

dtka:		dtka.o libdtka.dll libtcc.dll
		$(CC) -o dtka dtka.o -L./lib -L$(ROOT)/lib -ldtka -ltccP -ltcc -ltc $(LBP) -lici -lpthread -lm
		cp dtka ./bin

dtkaadmin:	dtkaadmin.o libdtka.dll
		$(CC) -o dtkaadmin dtkaadmin.o -L./lib -L$(ROOT)/lib -ldtka -ltccP -ltcc -ltc $(LBP) -lici -lpthread -lm
		cp dtkaadmin ./bin

libdtka.dll:	libdtka.o
		$(LD) -o libdtka.dll libdtka.o -Wl,--output-def,libdtka.def -L$(ROOT)/lib -lws2_32 -lici -lbp
		cp libdtka.dll ./lib

#	-	-	Object modules	-	-	-	-	-

%.o:		$(API)/%.c
		$(CC) -c $<

%.o:		$(AUTHORITY)/%.c
		$(CC) -c $<

%.o:		$(CLIENT)/%.c
		$(CC) -c $<

%.o:		$(DTKA)/%.c
		$(CC) -c $<

$(ZFEC)/fec.o: $(ZFEC)/fec.c
		$(CC) -Drestrict= -c $(ZFEC)/fec.c -o $(ZFEC)/fec.o
