UTILS = ../utils

# OPT = -O -Dmingw
OPT = -g -Wall -Dmingw
CC = gcc $(OPT) -I../../ici/include -I../../ici/sdr -I../../ltp/include -I../../$(BPv)/include -I../../cfdp/include -I../../ltp/library -I../../$(BPv)/library -I../../cfdp/library
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
		$(CC) -o ionrestart ionrestart.o -L./lib -L../../ici/i86-mingw/lib -L../../ltp/i86-mingw/lib -L../../$(BPv)/i86-mingw/lib -L../../cfdp/i86-mingw/lib -lcfdp -lcgr $(LBP) -lltp -lici -lpthread -lws2_32 -lm
		cp ionrestart ./bin

ionexit:	ionexit.o
		$(CC) -o ionexit ionexit.o -L./lib -L../../$(BPv)/i86-mingw/lib -L../../ici/i86-mingw/lib -L../../ltp/i86-mingw/lib -lltp -lici -lm -lbp -lpthread
		cp ionexit ./bin
		
#	-	-	Object modules	-	-	-	-	-

%.o:		$(UTILS)/%.c
		$(CC) -c $<
