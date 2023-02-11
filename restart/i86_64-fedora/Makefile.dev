UTILS = ../utils

# OPT = -O -Dlinux
OPT = -g -Wall -Werror -Dlinux -fPIC -DSPACE_ORDER=3
CC = gcc $(OPT) -I../../ici/include -I../../ici/sdr -I../../ltp/include -I../../$(BP)/include -I../../cfdp/include -I../../ltp/library -I../../$(BP)/library -I../../cfdp/library
LDFLAGS = -fPIC -shared
LD = gcc $(LDFLAGS)

LBP = -lbp -limcfw -lipnfw -ldtn2fw

UTILITIES = ionrestart ionexit

ALL = $(UTILITIES)

all:		$(ALL)

clean:
		rm -f *.o
		rm -f $(ALL)
		rm -f ./bin/*

install:
		cp bin/* $(ROOT)/bin

#	-	-	Utility executables	-	-	-	-

ionrestart:	ionrestart.o
		$(CC) -o ionrestart ionrestart.o -L./lib -L../../ici/i86_64-fedora/lib -L../../ltp/i86_64-fedora/lib -L../../$(BP)/i86_64-fedora/lib -L../../cfdp/i86_64-fedora/lib -lcfdp -lcgr $(LBP) -lltp -lici -lpthread -lm
		cp ionrestart ./bin

ionexit:	ionexit.o
		$(CC) -o ionexit ionexit.o -L./lib -L../../$(BP)/i86-mingw/lib -L../../ici/i86-mingw/lib -L../../ltp/i86-mingw/lib $(LBP) -lltp -lici -lpthread -lm
		cp ionexit ./bin

#	-	-	Object modules	-	-	-	-	-

%.o:		$(UTILS)/%.c
		$(CC) -c $<
