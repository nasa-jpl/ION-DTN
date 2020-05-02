UTILS = ../utils

# OPT = -O -Dlinux
OPT = -g -Wall -Werror -Dlinux -DHEAP_PTRS=$(PTRS)
CC = gcc $(OPT) -I../../ici/include -I../../ici/sdr -I../../ltp/include -I../../$(BP)/include -I../../cfdp/include -I../../ltp/library -I../../$(BP)/library -I../../cfdp/library
LDFLAGS = -fPIC -shared -rdynamic
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

ionrestart:	ionrestart.o
		$(CC) -o ionrestart ionrestart.o -L./lib -L../../ici/i86-redhat/lib -L../../ltp/i86-redhat/lib -L../../$(BP)/i86-redhat/lib -L../../cfdp/i86-redhat/lib -lcfdp -lcgr $(LBP) -lltp -lici -lpthread -lm
		cp ionrestart ./bin

ionexit:	ionexit.o
		$(CC) -o ionexit ionexit.o -L./lib -L../../$(BP)/i86-mingw/lib -L../../ici/i86-mingw/lib -L../../ltp/i86-mingw/lib -lltp -lici -lm -lbp -lpthread
		cp ionexit ./bin
		
#	-	-	Object modules	-	-	-	-	-

%.o:		$(UTILS)/%.c
		$(CC) -c $<
