# Makefile for NM Manager and Agent
#
# This Makefile should be valid or any native GCC compiler.
#
# The manager supports two optional features that can be enabled at compile time:
#   make MYSQL=1      Enable MYSQL Database Support
#   make NCURSES=1    Enable enhanced NCURSES-based UI
#   make NCURSES=1 MYSQL=1     Enable all optional features.

# Check GCC Version.  GCC7 currently requires additional flags
GCC_VERSION := "`gcc -dumpversion`"
ifeq ($(shell test $(GCC_VERSION) -gt 6; echo $$?),0)
	OPT = -Wno-maybe-uninitialized -Wformat-truncation=0 -Wno-format
endif

AGENT = ../agent
MGR = ../mgr
SHARED = ../shared
TEST = ../test
CONTRIB = ../contrib
ADM = ../../$(BPv)/adm

SHARED_SOURCES = \
	$(SHARED)/adm/adm.c		\
	$(SHARED)/adm/adm_init.c	\
	$(SHARED)/msg/msg.c		\
	$(SHARED)/msg/ion_if.c		\
	$(SHARED)/primitives/ari.c	\
	$(SHARED)/primitives/blob.c	\
	$(SHARED)/primitives/ctrl.c	\
	$(SHARED)/primitives/edd_var.c	\
	$(SHARED)/primitives/expr.c	\
	$(SHARED)/primitives/report.c 	\
	$(SHARED)/primitives/rules.c 	\
	$(SHARED)/primitives/table.c 	\
	$(SHARED)/primitives/tnv.c	\
	$(SHARED)/utils/cbor_utils.c	\
	$(SHARED)/utils/db.c		\
	$(SHARED)/utils/nm_types.c 	\
	$(SHARED)/utils/rhht.c	 	\
	$(SHARED)/utils/utils.c        	\
	$(SHARED)/utils/vector.c 	\
	$(CONTRIB)/QCBOR/src/UsefulBuf.c \
	$(CONTRIB)/QCBOR/src/qcbor_encode.c \
	$(CONTRIB)/QCBOR/src/qcbor_decode.c \
	$(CONTRIB)/QCBOR/src/ieee754.c

AGENT_SOURCES = \
	$(AGENT)/ingest.c  		\
	$(AGENT)/instr.c		\
	$(AGENT)/lcc.c     		\
	$(AGENT)/nmagent.c 		\
	$(AGENT)/ldc.c 			\
	$(AGENT)/rda.c 			\
	$(AGENT)/adm_amp_agent_impl.c   \
	$(AGENT)/adm_amp_agent_agent.c	\
	$(ADM)/adm_bp_agent_impl.c	\
	$(ADM)/adm_bp_agent_agent.c 	\
	$(AGENT)/adm_ion_admin_agent.c 	\
	$(AGENT)/adm_ion_admin_impl.c	\
	$(ADM)/adm_ion_bp_admin_impl.c	\
	$(ADM)/adm_ion_bp_admin_agent.c	\
	$(AGENT)/adm_ionsec_admin_impl.c	\
	$(AGENT)/adm_ionsec_admin_agent.c	\
	$(AGENT)/adm_ion_ipn_admin_impl.c	\
	$(AGENT)/adm_ion_ipn_admin_agent.c	\
	$(AGENT)/adm_ion_ltp_admin_impl.c	\
	$(AGENT)/adm_ion_ltp_admin_agent.c	\
	$(AGENT)/adm_ltp_agent_impl.c	\
	$(AGENT)/adm_ltp_agent_agent.c 

ifeq ($(BPv), bpv6)
AGENT_SOURCES += \
	$(ADM)/adm_sbsp_impl.c	\
	$(ADM)/adm_sbsp_agent.c	
else
AGENT_SOURCES += \
	$(ADM)/adm_bpsec_impl.c	\
	$(ADM)/adm_bpsec_agent.c	
endif

MGR_SOURCES = \
	$(MGR)/agents.c 		\
	$(MGR)/metadata.c		\
	$(MGR)/nm_mgr_print.c 	 	\
	$(MGR)/nm_mgr_rx.c             	\
	$(MGR)/nm_mgr_sql.c             \
	$(MGR)/nm_mgr_ui.c 		\
	$(MGR)/nm_mgr.c			\
	$(MGR)/ui_input.c 		\
	$(MGR)/adm_amp_agent_mgr.c      \
	$(ADM)/adm_bp_agent_mgr.c	\
	$(MGR)/adm_ion_admin_mgr.c 	\
	$(ADM)/adm_ion_bp_admin_mgr.c	\
	$(MGR)/adm_ionsec_admin_mgr.c	\
	$(MGR)/adm_ion_ipn_admin_mgr.c	\
	$(MGR)/adm_ion_ltp_admin_mgr.c	\
	$(MGR)/adm_ltp_agent_mgr.c

ifeq ($(BPv), bpv6)
MGR_SOURCES += \
	$(ADM)/adm_sbsp_mgr.c
else
MGR_SOURCES += \
	$(ADM)/adm_bpsec_mgr.c
endif


SHARED_OBJS := ${SHARED_SOURCES:c=o}
MGR_OBJS := ${MGR_SOURCES:c=o}
AGENT_OBJS := ${AGENT_SOURCES:c=o}

TGT_SHARED_OBJS := $(notdir ${SHARED_OBJS})
TGT_MGR_OBJS := $(notdir ${MGR_OBJS})
TGT_AGENT_OBJS := $(notdir ${AGENT_OBJS})

# OPT = -O -Dlinux
OPT += -g -Wall -Werror -Wno-unused-variable -Dmingw -DAMP_VERSION=8
#-DHEAP_PTRS=$(PTRS)  # Leave this at default for now
# For database support, append -DHAVE_MYSQL

ifeq ($(BPv), bpv6)
OPT += -DBUILD_BPv6
else
OPT += -DBUILD_BPv7
endif

INCLDIRS = -I.. -I../.. -I$(ADM) -I$(AGENT) -I$(CONTRIB)/QCBOR/inc -I$(MGR) -I$(SHARED)/adm -I$(SHARED)/msg -I$(SHARED)/primitives -I$(SHARED)/utils -I../../ici/library -I../../ici/include -I../../ltp/library -I../../$(BPv) -I../../$(BPv)/library -I../../$(BPv)/library/ext/sbsp -I../../$(BPv)/library/ext/bpsec -I$(ROOT)/include -I/usr/include/mysql -I/usr/local/mysql/include

CC = gcc $(OPT) $(INCLDIRS)

LIBDIRS = -L$(ROOT)/lib -L/usr/local/mysql/lib -L/

LIBS = -lici -lbp -lltp -lpthread -lm

LD = gcc $(OPT) $(LIBDIRS)

NM = nm_agent nm_mgr


ifdef MYSQL
	LIBS += -lmysqlclient
	OPT += -DHAVE_MYSQL
endif
ifdef NCURSES
	LIBS += -lncurses -lform -lmenu -lpanel
	OPT += -DUSE_NCURSES
endif

ALL = $(NM)

all:		$(ALL)

clean:
		rm -f *.o
		rm -f $(ALL)
		rm -f ./bin/*

install:
		cp bin/* $(ROOT)/bin

#	-	-	NM executables	-	-	-	-	-

nm_agent:	$(SHARED_OBJS) $(AGENT_OBJS)
#shared_objs agent_objs
		$(LD) -DAGENT_ROLE -o nm_agent $(TGT_SHARED_OBJS) $(TGT_AGENT_OBJS) $(LIBDIRS) $(LIBS)
		cp nm_agent ./bin

nm_mgr:		$(SHARED_OBJS) $(MGR_OBJS)
#shared_objs mgr_objs
		$(LD) -o nm_mgr $(TGT_SHARED_OBJS) $(TGT_MGR_OBJS) $(LIBDIRS) $(LIBS)
		cp nm_mgr ./bin

%.o: %.c
	${CC} -c $<
