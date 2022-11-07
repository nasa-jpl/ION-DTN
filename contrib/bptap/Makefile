IONPATH=/usr/local
CC=gcc
OPTFLAGS=-g
#OPTFLAGS=-O3 -fomit-frame-pointer -ffast-math -fmerge-all-constants -maccumulate-outgoing-args -march=core2 -U_FORTIFY_SOURCE -fno-stack-protector
CFLAGS=-I$(IONPATH)/include -std=gnu99 -Wall $(OPTFLAGS)
LDFLAGS=-pthread -L$(IONPATH)/lib -lipnfw -limcfw -ldtn2fw -lbp -lici

all: bptap

bptap: bptap.o
	$(CC) bptap.o -o bptap $(LDFLAGS)

bptap.o: bptap.c
	$(CC) -c bptap.c $(CFLAGS)

clean:
	rm -rf *.o bptap
