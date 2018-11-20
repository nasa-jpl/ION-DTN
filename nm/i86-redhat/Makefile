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

<<<<<<< local
ADM_SOURCES =	$(SHARED)/adm/adm.c \
		$(SHARED)/adm/adm_bp.c \
		$(SHARED)/adm/adm_ion.c \
		$(SHARED)/adm/adm_ltp.c \
		$(SHARED)/adm/adm_agent.c \
		$(SHARED)/adm/adm_sbsp.c
=======
SHARED_SOURCES = \
	$(SHARED)/adm/adm.c			\
	$(SHARED)/msg/msg.c			\
	$(SHARED)/msg/ion_if.c			\
	$(SHARED)/primitives/ari.c		\
	$(SHARED)/primitives/blob.c		\
	$(SHARED)/primitives/ctrl.c		\
	$(SHARED)/primitives/edd_var.c		\
	$(SHARED)/primitives/expr.c		\
	$(SHARED)/primitives/report.c 		\
	$(SHARED)/primitives/rules.c 		\
	$(SHARED)/primitives/table.c 		\
	$(SHARED)/primitives/tnv.c		\
	$(SHARED)/utils/cbor_utils.c	\
	$(SHARED)/utils/db.c			\
	$(SHARED)/utils/nm_types.c 		\
	$(SHARED)/utils/rhht.c	 		\
	$(SHARED)/utils/utils.c        		\
	$(SHARED)/utils/vector.c 		\
	$(CONTRIB)/tinycbor/src/cborparser.c 	\
	$(CONTRIB)/tinycbor/src/cborencoder.c 
>>>>>>> other

AGENT_SOURCES = \
	$(AGENT)/ingest.c  		\
	$(AGENT)/instr.c		\
	$(AGENT)/lcc.c     		\
	$(AGENT)/nmagent.c 		\
	$(AGENT)/ldc.c 			\
	$(AGENT)/rda.c 			\
	$(AGENT)/adm_amp_agent_impl.c   \
	$(AGENT)/adm_amp_agent_agent.c	\
	$(AGENT)/adm_bp_agent_impl.c	\
	$(AGENT)/adm_bp_agent_agent.c \
	$(AGENT)/adm_sbsp_impl.c	\
	$(AGENT)/adm_sbsp_agent.c	\
	$(AGENT)/adm_ion_admin_agent.c 	\
	$(AGENT)/adm_ion_admin_impl.c	\
	$(AGENT)/adm_ion_bp_admin_impl.c	\
	$(AGENT)/adm_ion_bp_admin_agent.c	\
	$(AGENT)/adm_ionsec_admin_impl.c	\
	$(AGENT)/adm_ionsec_admin_agent.c	\
	$(AGENT)/adm_ion_ipn_admin_impl.c	\
	$(AGENT)/adm_ion_ipn_admin_agent.c	\
	$(AGENT)/adm_ion_ltp_admin_impl.c	\
	$(AGENT)/adm_ion_ltp_admin_agent.c	\
	$(AGENT)/adm_ltp_agent_impl.c	\
	$(AGENT)/adm_ltp_agent_agent.c 


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
	$(MGR)/adm_bp_agent_mgr.c	\
	$(MGR)/adm_sbsp_mgr.c	\
	$(MGR)/adm_ion_admin_mgr.c \
	$(MGR)/adm_ion_bp_admin_mgr.c	\
	$(MGR)/adm_ionsec_admin_mgr.c	\
	$(MGR)/adm_ion_ipn_admin_mgr.c	\
	$(MGR)/adm_ion_ltp_admin_mgr.c	\
	$(MGR)/adm_ltp_agent_mgr.c

<<<<<<< local
AGENT_SOURCES =	$(AGENT)/ingest.c \
		$(AGENT)/lcc.c \
		$(AGENT)/ldc.c \
		$(AGENT)/nmagent.c \
		$(AGENT)/rda.c \
		$(SHARED)/primitives/expr.c \
		$(AGENT)/agent_db.c \
		$(AGENT)/adm_ion_priv.c \
		$(AGENT)/adm_ltp_priv.c \
		$(AGENT)/adm_agent_impl.c \
		$(AGENT)/adm_bp_impl.c \
		$(AGENT)/adm_sbsp_impl.c \
		$(AGENT)/instr.c
=======
>>>>>>> other

<<<<<<< local
#	$(MGR)/adm_bpsec_mgr.c		

<<<<<<< local
AGENT_OBJS =	agt_adm.o \
		agt_adm_agent.o \
		agt_adm_bp.o \
		agt_adm_sbsp.o \
		adm_ion.o \
		adm_ltp.o \
		msg_admin.o \
		msg_ctrl.o \
		pdu.o \
		var.o \
		agt_expr.o \
		blob.o \
		admin.o \
		def.o \
		mid.o \
		oid.o \
		report.o \
		rules.o \
		dc.o \
		value.o \
		lit.o \
		nn.o \
		tdc.o \
		ctrl.o \
		table.o \
		ion_if.o \
		utils.o \
		db.o \
		nm_types.o \
		ingest.o \
		instr.o \
		lcc.o \
		ldc.o \
		rda.o \
		agent_db.o \
		adm_ion_priv.o \
		adm_ltp_priv.o \
		adm_agent_impl.o \
		adm_bp_impl.o \
		adm_sbsp_impl.o \
		nmagent.o
=======
>>>>>>> other
=======
>>>>>>> other

<<<<<<< local
MGR_OBJS =	mgt_adm.o \
		mgt_adm_agent.o \
		mgt_adm_bp.o \
		mgt_adm_sbsp.o \
		adm_ltp.o \
		adm_ion.o \
		msg_admin.o \
		msg_ctrl.o \
		pdu.o \
		var.o \
		mgt_expr.o \
		admin.o \
		def.o \
		mid.o \
		oid.o \
		report.o \
		rules.o \
		dc.o \
		value.o \
		lit.o \
		nn.o \
		tdc.o \
		ctrl.o \
		blob.o \
		table.o \
		ion_if.o \
		utils.o \
		db.o \
		nm_types.o \
		nm_mgr_rx.o \
		nm_mgr_ui.o \
		nm_mgr_names.o \
		nm_mgr_sql.o \
		nm_mgr_print.o \
		mgr_db.o \
		ui_input.o \
		nm_mgr.o
=======
SHARED_OBJS := ${SHARED_SOURCES:c=o}
MGR_OBJS := ${MGR_SOURCES:c=o}
AGENT_OBJS := ${AGENT_SOURCES:c=o}

TGT_SHARED_OBJS := $(notdir ${SHARED_OBJS})
TGT_MGR_OBJS := $(notdir ${MGR_OBJS})
TGT_AGENT_OBJS := $(notdir ${AGENT_OBJS})
>>>>>>> other

# OPT = -O -Dlinux
OPT += -g -Wall -Werror -Wno-unused-variable -Dlinux
#-DHEAP_PTRS=$(PTRS)  # Leave this at default for now
# For database support, append -DHAVE_MYSQL

<<<<<<< local
INCLDIRS = -I.. -I../.. -I$(AGENT) -I$(MGR) -I$(SHARED)/adm -I$(SHARED)/msg -I$(SHARED)/primitives -I$(SHARED)/utils -I../../ici/library -I../../ltp/library -I../../bp/library -I../../bp/library/ext/sbsp -I$(ROOT)/include
# For database support, append -I/usr/local/mysql/include -I/user/include/mysql
=======
INCLDIRS = -I.. -I../.. -I$(AGENT) -I$(MGR) -I$(SHARED)/adm -I$(SHARED)/msg -I$(SHARED)/primitives -I$(SHARED)/utils -I../../ici/library -I../../ici/include -I../../ltp/library -I../../bp/library -I../../bp/library/ext/sbsp -I../../bp/library/ext/bpsec -I$(ROOT)/include -I/usr/include/mysql -I/usr/local/mysql/include
>>>>>>> other

CC = gcc $(OPT) $(INCLDIRS)

LIBDIRS = -L$(ROOT)/lib -L/usr/local/mysql/lib

LIBS = -lcfdp -lbp -lltp -lici -lpthread -lm

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

<<<<<<< local
adm_objs:	$(ADM_SOURCES)
		$(CC) -DAGENT_ROLE -c $(SHARED)/adm/adm.c -o agt_adm.o
		$(CC) -c $(SHARED)/adm/adm.c -o mgt_adm.o
		$(CC) -DAGENT_ROLE -c $(SHARED)/adm/adm_agent.c -o agt_adm_agent.o
		$(CC) -c $(SHARED)/adm/adm_agent.c -o mgt_adm_agent.o
		$(CC) -DAGENT_ROLE -c $(SHARED)/adm/adm_bp.c -o agt_adm_bp.o
		$(CC) -c $(SHARED)/adm/adm_bp.c -o mgt_adm_bp.o
		$(CC) -DAGENT_ROLE -c $(SHARED)/adm/adm_sbsp.c -o agt_adm_sbsp.o
		$(CC) -c $(SHARED)/adm/adm_sbsp.c -o mgt_adm_sbsp.o
		$(CC) -c $(SHARED)/adm/adm_ion.c
		$(CC) -c $(SHARED)/adm/adm_ltp.c

msg_objs:	$(MSG_SOURCES)
		$(CC) -c $(SHARED)/msg/msg_admin.c
		$(CC) -c $(SHARED)/msg/msg_ctrl.c
		$(CC) -c $(SHARED)/msg/pdu.c

primitive_objs:	$(PRIMITIVE_SOURCES)
		$(CC) -DAGENT_ROLE -c $(SHARED)/primitives/expr.c -o agt_expr.o
		$(CC) -c $(SHARED)/primitives/expr.c -o mgt_expr.o
		$(CC) -c $(SHARED)/primitives/var.c
		$(CC) -c $(SHARED)/primitives/blob.c
		$(CC) -c $(SHARED)/primitives/admin.c
		$(CC) -c $(SHARED)/primitives/def.c
		$(CC) -c $(SHARED)/primitives/mid.c
		$(CC) -c $(SHARED)/primitives/oid.c
		$(CC) -c $(SHARED)/primitives/report.c
		$(CC) -c $(SHARED)/primitives/rules.c
		$(CC) -c $(SHARED)/primitives/dc.c
		$(CC) -c $(SHARED)/primitives/value.c
		$(CC) -c $(SHARED)/primitives/lit.c
		$(CC) -c $(SHARED)/primitives/nn.c
		$(CC) -c $(SHARED)/primitives/tdc.c
		$(CC) -c $(SHARED)/primitives/ctrl.c
		$(CC) -c $(SHARED)/primitives/table.c

util_objs:	$(UTIL_SOURCES)
		$(CC) -c $(SHARED)/utils/ion_if.c
		$(CC) -c $(SHARED)/utils/utils.c
		$(CC) -c $(SHARED)/utils/db.c
		$(CC) -c $(SHARED)/utils/nm_types.c

agent_objs:	$(AGENT_SOURCES)
		$(CC) -c $(AGENT)/adm_agent_impl.c
		$(CC) -c $(AGENT)/adm_bp_impl.c
		$(CC) -c $(AGENT)/adm_sbsp_impl.c
		$(CC) -c $(AGENT)/adm_ion_priv.c
		$(CC) -c $(AGENT)/adm_ltp_priv.c
		$(CC) -c $(AGENT)/agent_db.c
		$(CC) -c $(AGENT)/ingest.c
		$(CC) -c $(AGENT)/instr.c
		$(CC) -c $(AGENT)/lcc.c
		$(CC) -c $(AGENT)/ldc.c
		$(CC) -c $(AGENT)/nmagent.c
		$(CC) -c $(AGENT)/rda.c

mgr_objs:	$(MGR_SOURCES)
		$(CC) -c $(MGR)/mgr_db.c
		$(CC) -c $(MGR)/nm_mgr.c
		$(CC) -c $(MGR)/nm_mgr_names.c
		$(CC) -c $(MGR)/nm_mgr_print.c
		$(CC) -c $(MGR)/nm_mgr_rx.c
		$(CC) -c $(MGR)/nm_mgr_sql.c
		$(CC) -c $(MGR)/nm_mgr_ui.c
		$(CC) -c $(MGR)/ui_input.c
=======
>>>>>>> other
