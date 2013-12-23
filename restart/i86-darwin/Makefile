UTILS = ../utils

# OPT = -O -Dunix -Ddarwin
OPT = -g -Wall -Werror -Dunix -Ddarwin
CC = gcc $(OPT)
LDFLAGS = -dynamiclib -undefined dynamic_lookup
LD = gcc $(LDFLAGS)

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
		$(CC) -o ionrestart ionrestart.o -L./lib -L../../ici/i86-darwin/lib -L../../ltp/i86-darwin/lib -L../../bp/i86-darwin/lib -L../../cfdp/i86-darwin/lib -lcfdp -lcgr -lbp -lltp -lltpP -lici -lpthread
		cp ionrestart ./bin
