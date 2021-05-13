# Makefile.am

ACLOCAL_AMFLAGS = -I aux_build/m4

SUBDIRS= 
DIST_SUBDIRS=

if ECLSA_CODEC_OPENFEC
SUBDIRS+= ec/openfec
DIST_SUBDIRS+= ec/openfec
endif

SUBDIRS+= .
DIST_SUBDIRS+= .

AM_CFLAGS= -g -include config.h

# TODO probably platform-dependent
AM_CFLAGS+= -Wall -fPIC
# AM_CFLAGS+= -Wall -fPIC -Werror

bin_PROGRAMS = \
	$(eclsabin)


lib_LTLIBRARIES = \
	$(eclsalib)

eclsabin = eclso \
	eclsi

eclsalib = libeclsa.la

# --- ECLSA Executables and Library --- #

# libeclsa_la_CFLAGS = $(AM_CFLAGS)

if BUILD_ION

libeclsa_la_SOURCES =   $(srcdir)/ec/eclsa/elements/sys/eclsaTimer.c \
	                    $(srcdir)/ec/eclsa/elements/sys/eclsaLogger.c \
	                    $(srcdir)/ec/eclsa/elements/sys/eclsaMemoryManager.c \
		                $(srcdir)/ec/eclsa/elements/matrix/eclsaCodecMatrix.c \
		                $(srcdir)/ec/eclsa/elements/matrix/eclsaMatrix.c \
		                $(srcdir)/ec/eclsa/elements/matrix/eclsaMatrixPool.c \
		                $(srcdir)/ec/eclsa/elements/fec/eclsaFecManager.c \
		                $(srcdir)/ec/eclsa/elements/packet/eclsaSendingSequence.c \
		                $(srcdir)/ec/eclsa/elements/packet/eclsaBlacklist.c \
		                $(srcdir)/ec/eclsa/elements/packet/eclsaPacket.c \
		                $(srcdir)/ec/eclsa/elements/packet/eclsaFeedback.c \
		                $(srcdir)/ec/eclsa/extensions/HSLTP/eclso_ext.c \
                        $(srcdir)/ec/eclsa/extensions/HSLTP/eclsi_ext.c 

libeclsa_la_CFLAGS = $(AM_CFLAGS) -I$(ionpath)/ici/include \
		                -I$(ionpath)/ltp/library \
		                -I$(ionpath)/$(BP_DIR)/library \
				-I$(ionpath)/$(BP_DIR)/include

eclsX_LDADD = libeclsa.la 

eclso_SOURCES = $(srcdir)/ec/eclsa/eclso.c 
eclso_LDADD = $(eclsX_LDADD) -lltp -lici -lbp -lpthread $(LIBOBJS) -lrt -L$(srcdir)/ec/openfec/bin/Debug
eclso_CFLAGS = $(AM_CFLAGS) $(eclsX_CFLAGS) -I$(ionpath)/ltp/library -I$(ionpath)/ltp/library/ext 

eclsi_SOURCES = $(srcdir)/ec/eclsa/eclsi.c 
eclsi_LDADD = $(eclsX_LDADD) -lltp -lici -lbp -lpthread $(LIBOBJS) -lrt -L$(srcdir)/ec/openfec/bin/Debug
eclsi_CFLAGS = $(AM_CFLAGS) $(eclsX_CFLAGS) -I$(ionpath)/ltp/library -I$(ionpath)/ltp/library/ext


if ECLSA_CODEC_DUMMYDEC
libeclsa_la_SOURCES +=	$(srcdir)/ec/eclsa/adapters/codec/eclsaCodecAdapter_DummyDEC.c		
endif

if ECLSA_CODEC_OPENFEC
libeclsa_la_SOURCES +=  $(srcdir)/ec/eclsa/adapters/codec/eclsaCodecAdapter_OpenFEC.c
libeclsa_la_CFLAGS += -I$(srcdir)/ec/openfec/src/lib_common
eclso_LDADD += -lopenfec
eclsi_LDADD += -lopenfec
endif

if ECLSA_CODEC_LIBECDLR
libeclsa_la_SOURCES +=	$(srcdir)/ec/libec/decoder.c \
			            $(srcdir)/ec/libec/encoder.c \
			            $(srcdir)/ec/libec/ec_graph.c \
			            $(srcdir)/ec/eclsa/adapters/codec/eclsaCodecAdapter_LibecDLR.c

libeclsa_la_CFLAGS += -I$(srcdir)/ec/libec 
endif

if ECLSA_LP_UDP
libeclsa_la_SOURCES +=	$(srcdir)/ec/eclsa/adapters/protocol/eclsaProtocolAdapter_LP_UDP.c 
endif

if ECLSA_UP_LTP
libeclsa_la_SOURCES +=	$(srcdir)/ec/eclsa/adapters/protocol/eclsaProtocolAdapter_UP_ION_LTP.c \
                        $(srcdir)/ec/eclsa/extensions/HSLTP/hsltpProtocolAdapter_UP_ION_LTP.c

endif

if ECLSA_UP_UNIBO_LTP
libeclsa_la_SOURCES +=	$(srcdir)/ec/eclsa/adapters/protocol/eclsaProtocolAdapter_UP_Unibo_LTP.c \
			$(srcdir)/ec/eclsa/extensions/HSLTP/hsltpProtocolAdapter_UP_UniboLTP.c
endif

endif

