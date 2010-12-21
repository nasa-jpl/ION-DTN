SRC      = ../library
INCL     = ../include
UTILS    = ../utils
TEST     = ../test
DAEMON   = ../daemon
UDP      = ../udp
ICIINCL  = ../../ici/include
BPINCL   = ../../bp/include
DGRINCL  = ../../dgr/include

CPU       = PPC604
TOOL      = gnu
OPT       = -mlongcall -DPRIVATE_SYMTAB -DVXWORKS 

include $(WIND_BASE)/target/h/make/defs.bsp
include $(WIND_BASE)/target/h/make/make.$(CPU)$(TOOL)
include $(WIND_BASE)/target/h/make/defs.$(WIND_HOST_TYPE)

LDFLAGS = -r -o
LINKER  = ldppc
CFLAGS  = -mstrict-align -fvolatile -fno-builtin  -Wall -Werror

MAKEOBJECTS = $(CC) $(CFLAGS) $(OPT) -I$(INCL) -I$(ICIINCL) -I$(DGRINCL) -I$(UDP) -I$(SRC)  -c

LTPSOURCES =		\
$(TEST)/ltpcounter.c	\
$(TEST)/ltpdriver.c	\
$(UTILS)/ltpadmin.c	\
$(DAEMON)/ltpclock.c	\
$(DAEMON)/ltpmeter.c	\
$(SRC)/libltp.c		\
$(SRC)/libltpP.c	\
$(UDP)/udplsi.c		\
$(UDP)/udplso.c

ALLLTPOBJS =		\
	ltpcounter.o	\
	ltpdriver.o	\
	ltpadmin.o	\
	ltpclock.o	\
	ltpmeter.o	\
	libltp.o	\
	libltpP.o	\
	udplsi.o	\
	udplso.o

SOURCES = $(LTPSOURCES)

OBJECTS = $(SOURCES:.c=.o)

all	: default
default	: $(OBJECTS)
	$(LINKER) $(LDFLAGS) ltplib.o $(ALLLTPOBJS) 
	chmod 755 ltplib.o

exe	:

.c.o:	$(SOURCES) $(PUBINCLS) $(TESTINCLS)
	$(MAKEOBJECTS) $< 

install:
	echo "DO NOTHING"

clean:
	rm *.o
