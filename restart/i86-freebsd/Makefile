UTILS = ../utils

# OPT = -O -Dfreebsd
OPT = -g -Wall -Werror -Dfreebsd
CC = cc $(OPT)
LDFLAGS = -fPIC -shared
LD = ld $(LDFLAGS)

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
		$(CC) -o ionrestart ionrestart.o -L./lib -L../../ici/i86-freebsd/lib -L../../ltp/i86-freebsd/lib -L../../bp/i86-freebsd/lib -L../../cfdp/i86-freebsd/lib -lcfdp -lcgr -lbp -lltp -lltpP -lici -lpthread -lm
		cp ionrestart ./bin
