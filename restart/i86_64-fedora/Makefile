UTILS = ../utils

# OPT = -O -Dlinux
OPT = -g -Wall -Werror -Dlinux -fPIC -DSPACE_ORDER=3
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
		$(CC) -o ionrestart ionrestart.o -L./lib -L../../ici/i86_64-fedora/lib -L../../ltp/i86_64-fedora/lib -L../../bp/i86_64-fedora/lib -L../../cfdp/i86_64-fedora/lib -lcfdp -lcgr $(LBP) -lltp -lici -lpthread -lm
		cp ionrestart ./bin
