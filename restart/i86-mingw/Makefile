UTILS = ../utils

# OPT = -O -Dmingw
OPT = -g -Wall -Dmingw
CC = gcc $(OPT)
LDFLAGS = -fPIC -shared
LD = gcc $(LDFLAGS)

LBP = -lbp -limcfw -lipnfw -ldtn2fw

UTILITIES = ionrestart

ALL = $(UTILITIES)

all:		$(ALL)

clean:
		rm -f *.o
		rm -f $(ALL)
		rm -f ./bin/*

install:
		cp bin/* $(ROOT)/bin

#	-	-	Restart executable	-	-	-	-

ionrestart.o:	$(UTILS)/ionrestart.c
		$(CC) -I../../ici/include -I../../ici/sdr -I../../ltp/include -I../../bp/include -I../../cfdp/include -I../../ltp/library -I../../bp/library -I../../cfdp/library -c $(UTILS)/ionrestart.c

ionrestart:	ionrestart.o
		$(CC) -o ionrestart ionrestart.o -L./lib -L../../ici/i86-mingw/lib -L../../ltp/i86-mingw/lib -L../../bp/i86-mingw/lib -L../../cfdp/i86-mingw/lib -lcfdp -lcgr $(LBP) -lltp -lici -lpthread -lws2_32 -lm
		cp ionrestart ./bin
