AGENT = ../agent
MGR = ../mgr
SHARED = ../shared
TEST = ../test

ADM_SOURCES =	$(SHARED)/adm/adm.c \
		$(SHARED)/adm/adm_bp.c \
		$(SHARED)/adm/adm_ion.c \
		$(SHARED)/adm/adm_ltp.c \
		$(SHARED)/adm/adm_agent.c \
		$(SHARED)/adm/adm_sbsp.c

MSG_SOURCES =	$(SHARED)/msg/msg_admin.c \
		$(SHARED)/msg/msg_ctrl.c \
		$(SHARED)/msg/pdu.c

PRIMITIVE_SOURCES = $(SHARED)/primitives/var.c \
		$(SHARED)/primitives/blob.c \
		$(SHARED)/primitives/admin.c \
		$(SHARED)/primitives/def.c \
		$(SHARED)/primitives/mid.c \
		$(SHARED)/primitives/oid.c \
		$(SHARED)/primitives/report.c \
		$(SHARED)/primitives/rules.c \
		$(SHARED)/primitives/dc.c \
		$(SHARED)/primitives/value.c \
		$(SHARED)/primitives/lit.c \
		$(SHARED)/primitives/nn.c \
		$(SHARED)/primitives/tdc.c \
		$(SHARED)/primitives/ctrl.c \
		$(SHARED)/primitives/table.c

UTIL_SOURCES =	$(SHARED)/utils/ion_if.c \
		$(SHARED)/utils/utils.c \
		$(SHARED)/utils/db.c \
		$(SHARED)/utils/nm_types.c

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

MGR_SOURCES =	$(MGR)/nm_mgr.c \
		$(MGR)/nm_mgr_rx.c \
		$(MGR)/nm_mgr_ui.c \
		$(MGR)/nm_mgr_names.c \
		$(MGR)/nm_mgr_sql.c \
		$(MGR)/ui_input.c \
		$(MGR)/nm_mgr_print.c \
		$(MGR)/mgr_db.c

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

# OPT = -O -DuClibc
OPT = -g -fPIC -Wall -Werror -Wno-unused-variable -DuClibc
# For database support, append -DHAVE_MYSQL

INCLDIRS = -I.. -I../.. -I$(AGENT) -I$(MGR) -I$(SHARED)/adm -I$(SHARED)/msg -I$(SHARED)/primitives -I$(SHARED)/utils -I../../ici/library -I../../ltp/library -I../../bp/library -I$(ROOT)/include
# For database support, append -I/usr/local/mysql/include -I/user/include/mysql

CC = $(TCC) $(OPT) $(INCLDIRS)

LIBDIRS = -L$(ROOT)/lib

# For database support, append -L/usr/local/mysql/lib -lmysqlclient

LBP = -lbp -limcfw -lipnfw -ldtn2fw

LD = $(TCC) $(OPT) $(LIBDIRS)

NM = nm_agent nm_mgr

ALL = $(NM)

all:		$(ALL)

clean:
		rm -f *.o
		rm -f $(ALL)
		rm -f ./bin/*

install:
		cp bin/* $(ROOT)/bin

#	-	-	NM executables	-	-	-	-	-

nm_agent:	adm_objs msg_objs primitive_objs util_objs agent_objs
		$(LD) -DAGENT_ROLE -o nm_agent $(AGENT_OBJS) $(LIBDIRS) -lcfdp $(LBP) -lltp -lltpP -lici -lpthread -lm
		cp nm_agent ./bin

nm_mgr:		adm_objs msg_objs primitive_objs util_objs mgr_objs
		$(LD) -o nm_mgr $(MGR_OBJS) $(LIBDIRS) -lcfdp $(LBP) -lltp -lltpP -lici -lpthread -lm
		cp nm_mgr ./bin

#	-	-	Object modules	-	-	-	-	-

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
