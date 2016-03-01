SRC = .
MSRC = murmur2
INCL = .
MINCL = murmur2
TEST = ../test

OPT = -g -Wall -Dmingw
CC = gcc $(OPT) -I$(INCL) -I$(MINCL)
LDFLAGS = -fPIC -shared
LD = gcc $(LDFLAGS)

LIBBLOOMOBJS = \
	bloom.o \
	MurmurHash2.o \

ALL = libbloom.dll 

all:		$(ALL)

clean:
		rm -f *.o
		rm -f *.exe
		rm -f *.def
		rm -f $(ALL)
		rm -f ./lib/*
		rm -f *.dll

install:
		cp bloom.h $(ROOT)/include
		cp lib/* $(ROOT)/lib


#	-	-	Test executables	-	-	-	-

# not building libbloom test

#	-	-	Libraries	-	-	-	-	-

libbloom.dll:	$(LIBBLOOMOBJS) 
		$(LD) -o libbloom.dll $(LIBBLOOMOBJS) -Wl,--output-def,libbloom.def -lpthread -lws2_32
		mkdir -p ./lib
		cp libbloom.dll ./lib

#	-	-	Object modules	-	-	-	-	-

%.o:		$(SRC)/%.c
		$(CC) -c $<

%.o:		$(MSRC)/%.c
		$(CC) -c $<

