INCL = ../include
ICI_INCL = ../../ici/include
API = ../library
TEST = ../test
ICI_LIB = ../../ici/i86_64-fedora/lib

# OPT = -O -Dlinux
# OPT = -g -Wall -Dlinux -DINITIAL_RETARD=2
OPT = -g -Wall -Werror -Dlinux -fPIC -DSPACE_ORDER=3
CC = gcc $(OPT) $(ADD_FLAGS) -I$(API) -I$(TEST) -I$(INCL) -I${ROOT}/include -I$(ICI_INCL) -I../../ici/library -L$(ICI_LIB)
LDFLAGS = -fPIC -shared
LD = gcc $(LDFLAGS)

LIBDGROBJS = \
	libdgr.o

PUBINCLS = \
	$(INCL)/dgr.h

RUNTIMES = file2dgr dgr2file file2tcp tcp2file file2udp udp2file

ALL = check libdgr.so $(RUNTIMES)

all:		$(ALL)

check:		$(PUBINCLS)
		rm -f *.o
		touch check

clean:
		rm -f *.o
		rm -f *.so
		rm -f $(ALL)
		rm -f ./lib/*
		rm -f ./bin/*

install:
		cp ../include/* $(ROOT)/include
		cp lib/* $(ROOT)/lib
		cp bin/* $(ROOT)/bin

uninstall:		
		rm $(ROOT)/lib/libdgr.so
		for file in $(PUBINCLS); \
			do rm $(ROOT)/include/$(notdir $$file); done
		for file in $(RUNTIMES); \
			do rm $(ROOT)/bin/$(notdir $$file); done

#	-	-	Test executables	-	-	-	-

file2dgr:	file2dgr.o libdgr.so $(TEST)/file2dgr.h
		$(CC) -o file2dgr file2dgr.o -L./lib -L${ROOT}/lib -ldgr -lici -lpthread -lm
		cp file2dgr ./bin

dgr2file:	dgr2file.o libdgr.so $(TEST)/file2dgr.h
		$(CC) -o dgr2file dgr2file.o -L./lib -L${ROOT}/lib -ldgr -lici -lpthread -lm
		cp dgr2file ./bin

tcp2file:	tcp2file.o $(TEST)/file2tcp.h
		$(CC) -o tcp2file tcp2file.o -L${ROOT}/lib -L./lib -lici -lpthread -lm
		cp tcp2file ./bin

file2tcp:	file2tcp.o $(TEST)/file2tcp.h
		$(CC) -o file2tcp file2tcp.o -L${ROOT}/lib -L./lib -lici -lpthread -lm
		cp file2tcp ./bin

udp2file:	udp2file.o $(TEST)/file2udp.h
		$(CC) -o udp2file udp2file.o -L${ROOT}/lib -L./lib -lici -lpthread -lm
		cp udp2file ./bin

file2udp:	file2udp.o $(TEST)/file2udp.h
		$(CC) -o file2udp file2udp.o -L${ROOT}/lib -L./lib -lici -lpthread -lm
		cp file2udp ./bin

#	-	-	Libraries	-	-	-	-	-

libdgr.so:	$(LIBDGROBJS)
		$(LD) -o libdgr.so $(LIBDGROBJS)
		cp libdgr.so ./lib

#	-	-	Object modules	-	-	-	-	-

%.o:		$(API)/%.c
		$(CC) -c $<

%.o:		$(TEST)/%.c
		$(CC) -c $<
