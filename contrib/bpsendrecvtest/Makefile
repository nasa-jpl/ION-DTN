IONPATH=/usr/local
CC=gcc
OPTFLAGS=-g
CFLAGS=-I$(IONPATH)/include -std=gnu99 -Wall $(OPTFLAGS)
LDFLAGS= -L$(IONPATH)/lib -lbp -lici -l:libici.la -l:libbp.la -l:libcgr.la -l:libstcpcla.la -l:libudpcla.la

all: bpsendtest bprecvtest

bpsendtest: bpsendtest.o
	$(CC) bpsendtest.o -o bpsendtest $(LDFLAGS)

bpsendtest.o: bpsendtest.c
	$(CC) -c bpsendtest.c $(CFLAGS)


bprecvtest: bprecvtest.o
	$(CC) bprecvtest.o -o bprecvtest $(LDFLAGS)

bprecvtest.o: bprecvtest.c
	$(CC) -c bprecvtest.c $(CFLAGS)

clean:
	rm -rf *.o bpsendtest bprecvtest
