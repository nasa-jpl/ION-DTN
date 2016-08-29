AGENT = ../agent
MGR = ../mgr
SHARED = ../shared
TEST = ../test

ADM_SOURCES =	$(SHARED)/adm/adm.c \
		$(SHARED)/adm/adm_bp.c \
		$(SHARED)/adm/adm_ion.c \
		$(SHARED)/adm/adm_ltp.c \
		$(SHARED)/adm/adm_agent.c \
		$(SHARED)/adm/adm_bpsec.c

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
		$(AGENT)/adm_bpsec_impl.c \
		$(AGENT)/instr.c

MGR_SOURCES =	$(MGR)/nm_mgr.c \
		$(MGR)/nm_mgr_rx.c \
		$(MGR)/nm_mgr_ui.c \ \
		$(MGR)/nm_mgr_names.c \
		$(MGR)/nm_mgr_sql.c \
		$(MGR)/ui_input.c \
		$(MGR)/nm_mgr_print.c \
		$(MGR)/mgr_db.c

AGENT_OBJS =	agt_adm_agent.o \
		agt_adm_bp.o \
		agt_adm_bpsec.o \
		agt_adm.o \
		agt_adm_ion.o \
		agt_adm_ltp.o \
		agt_msg_admin.o \
		agt_msg_ctrl.o \
		agt_pdu.o \
		agt_var.o \
		agt_expr.o \
		agt_blob.o \
		agt_admin.o \
		agt_def.o \
		agt_mid.o \
		agt_oid.o \
		agt_report.o \
		agt_rules.o \
		agt_dc.o \
		agt_value.o \
		agt_lit.o \
		agt_nn.o \
		agt_tdc.o \
		agt_ctrl.o \
		agt_table.o \
		agt_ion_if.o \
		agt_utils.o \
		agt_db.o \
		agt_nm_types.o \
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
		adm_bpsec_impl.o \
		nmagent.o

MGR_OBJS =	mgt_adm.o \
		mgt_adm_bp.o \
		mgt_adm_ltp.o \
		mgt_adm_ion.o \
		mgt_adm_agent.o \
		mgt_adm_bpsec.o \
		mgt_msg_admin.o \
		mgt_msg_ctrl.o \
		mgt_pdu.o \
		mgt_var.o \
		mgt_expr.o \
		mgt_admin.o \
		mgt_def.o \
		mgt_mid.o \
		mgt_oid.o \
		mgt_report.o \
		mgt_rules.o \
		mgt_dc.o \
		mgt_value.o \
		mgt_lit.o \
		mgt_nn.o \
		mgt_tdc.o \
		mgt_ctrl.o \
		mgt_blob.o \
		mgt_table.o \
		mgt_ion_if.o \
		mgt_utils.o \
		mgt_db.o \
		mgt_nm_types.o \
		nm_mgr_rx.o \
		nm_mgr_ui.o \
		nm_mgr_names.o \
		nm_mgr_sql.o \
		nm_mgr_print.o \
		mgr_db.o \
		ui_input.o \
		nm_mgr.o

# OPT = -O -Dlinux
OPT = -g -Wall -Werror -Wno-unused-variable -Dlinux -DHEAP_PTRS=$(PTRS) 
# For database support, append -DHAVE_MYSQL

INCLDIRS = -I.. -I../.. -I$(AGENT) -I$(MGR) -I$(SHARED)/adm -I$(SHARED)/msg -I$(SHARED)/primitives -I$(SHARED)/utils -I../../ici/library -I../../ltp/library -I../../bp/library -I$(ROOT)/include
# For database support, append -I/usr/local/mysql/include -I/user/include/mysql

CC = gcc $(OPT) $(INCLDIRS)

LIBDIRS = -L$(ROOT)/lib

# For database support, append -L/usr/local/mysql/lib -lmysqlclient

LBP = -lbp -limcfw -lipnfw -ldtn2fw

LD = gcc $(OPT) $(LIBDIRS)

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

adm_objs:
		$(CC) -DAGENT_ROLE -c $(SHARED)/adm/adm.c -o agt_adm.o
		$(CC) -c $(SHARED)/adm/adm.c -o mgt_adm.o
		$(CC) -DAGENT_ROLE -c $(SHARED)/adm/adm_bp.c -o agt_adm_bp.o
		$(CC) -c $(SHARED)/adm/adm_bp.c -o mgt_adm_bp.o
		$(CC) -DAGENT_ROLE -c $(SHARED)/adm/adm_ion.c -o agt_adm_ion.o
		$(CC) -c $(SHARED)/adm/adm_ion.c -o mgt_adm_ion.o
		$(CC) -DAGENT_ROLE -c $(SHARED)/adm/adm_ltp.c -o agt_adm_ltp.o
		$(CC) -c $(SHARED)/adm/adm_ltp.c -o mgt_adm_ltp.o
		$(CC) -DAGENT_ROLE -c $(SHARED)/adm/adm_agent.c -o agt_adm_agent.o
		$(CC) -c $(SHARED)/adm/adm_agent.c -o mgt_adm_agent.o
		$(CC) -DAGENT_ROLE -c $(SHARED)/adm/adm_bpsec.c -o agt_adm_bpsec.o
		$(CC) -c $(SHARED)/adm/adm_bpsec.c -o mgt_adm_bpsec.o

msg_objs:
		$(CC) -DAGENT_ROLE -c $(SHARED)/msg/msg_admin.c -o agt_msg_admin.o
		$(CC) -c $(SHARED)/msg/msg_admin.c -o mgt_msg_admin.o
		$(CC) -DAGENT_ROLE -c $(SHARED)/msg/msg_ctrl.c -o agt_msg_ctrl.o
		$(CC) -c $(SHARED)/msg/msg_ctrl.c -o mgt_msg_ctrl.o
		$(CC) -DAGENT_ROLE -c $(SHARED)/msg/pdu.c -o agt_pdu.o
		$(CC) -c $(SHARED)/msg/pdu.c -o mgt_pdu.o

primitive_objs:
		$(CC) -DAGENT_ROLE -c $(SHARED)/primitives/var.c -o agt_var.o
		$(CC) -c $(SHARED)/primitives/var.c -o mgt_var.o
		$(CC) -DAGENT_ROLE -c $(SHARED)/primitives/blob.c -o agt_blob.o
		$(CC) -c $(SHARED)/primitives/blob.c -o mgt_blob.o
		$(CC) -DAGENT_ROLE -c $(SHARED)/primitives/admin.c -o agt_admin.o
		$(CC) -c $(SHARED)/primitives/admin.c -o mgt_admin.o
		$(CC) -DAGENT_ROLE -c $(SHARED)/primitives/def.c -o agt_def.o
		$(CC) -c $(SHARED)/primitives/def.c -o mgt_def.o
		$(CC) -DAGENT_ROLE -c $(SHARED)/primitives/mid.c -o agt_mid.o
		$(CC) -c $(SHARED)/primitives/mid.c -o mgt_mid.o
		$(CC) -DAGENT_ROLE -c $(SHARED)/primitives/expr.c -o agt_expr.o
		$(CC) -c $(SHARED)/primitives/expr.c -o mgt_expr.o
		$(CC) -DAGENT_ROLE -c $(SHARED)/primitives/oid.c -o agt_oid.o
		$(CC) -c $(SHARED)/primitives/oid.c -o mgt_oid.o
		$(CC) -DAGENT_ROLE -c $(SHARED)/primitives/report.c -o agt_report.o
		$(CC) -c $(SHARED)/primitives/report.c -o mgt_report.o
		$(CC) -DAGENT_ROLE -c $(SHARED)/primitives/rules.c -o agt_rules.o
		$(CC) -c $(SHARED)/primitives/rules.c -o mgt_rules.o
		$(CC) -DAGENT_ROLE -c $(SHARED)/primitives/dc.c -o agt_dc.o
		$(CC) -c $(SHARED)/primitives/dc.c -o mgt_dc.o
		$(CC) -DAGENT_ROLE -c $(SHARED)/primitives/value.c -o agt_value.o
		$(CC) -c $(SHARED)/primitives/value.c -o mgt_value.o
		$(CC) -DAGENT_ROLE -c $(SHARED)/primitives/lit.c -o agt_lit.o
		$(CC) -c $(SHARED)/primitives/lit.c -o mgt_lit.o
		$(CC) -DAGENT_ROLE -c $(SHARED)/primitives/nn.c -o agt_nn.o
		$(CC) -c $(SHARED)/primitives/nn.c -o mgt_nn.o
		$(CC) -DAGENT_ROLE -c $(SHARED)/primitives/tdc.c -o agt_tdc.o
		$(CC) -c $(SHARED)/primitives/tdc.c -o mgt_tdc.o
		$(CC) -DAGENT_ROLE -c $(SHARED)/primitives/ctrl.c -o agt_ctrl.o
		$(CC) -c $(SHARED)/primitives/ctrl.c -o mgt_ctrl.o
		$(CC) -DAGENT_ROLE -c $(SHARED)/primitives/table.c -o agt_table.o
		$(CC) -c $(SHARED)/primitives/table.c -o mgt_table.o

util_objs:
		$(CC) -DAGENT_ROLE -c $(SHARED)/utils/ion_if.c -o agt_ion_if.o
		$(CC) -c $(SHARED)/utils/ion_if.c -o mgt_ion_if.o
		$(CC) -DAGENT_ROLE -c $(SHARED)/utils/utils.c -o agt_utils.o
		$(CC) -c $(SHARED)/utils/utils.c -o mgt_utils.o
		$(CC) -DAGENT_ROLE -c $(SHARED)/utils/db.c -o agt_db.o
		$(CC) -c $(SHARED)/utils/db.c -o mgt_db.o
		$(CC) -DAGENT_ROLE -c $(SHARED)/utils/nm_types.c -o agt_nm_types.o
		$(CC) -c $(SHARED)/utils/nm_types.c -o mgt_nm_types.o

agent_objs:
		$(CC) -c $(AGENT)/adm_agent_impl.c
		$(CC) -c $(AGENT)/adm_bp_impl.c
		$(CC) -c $(AGENT)/adm_bpsec_impl.c
		$(CC) -c $(AGENT)/adm_ion_priv.c
		$(CC) -c $(AGENT)/adm_ltp_priv.c
		$(CC) -c $(AGENT)/agent_db.c
		$(CC) -c $(AGENT)/ingest.c
		$(CC) -c $(AGENT)/instr.c
		$(CC) -c $(AGENT)/lcc.c
		$(CC) -c $(AGENT)/ldc.c
		$(CC) -c $(AGENT)/nmagent.c
		$(CC) -c $(AGENT)/rda.c

mgr_objs:
		$(CC) -c $(MGR)/mgr_db.c
		$(CC) -c $(MGR)/nm_mgr.c
		$(CC) -c $(MGR)/nm_mgr_names.c
		$(CC) -c $(MGR)/nm_mgr_print.c
		$(CC) -c $(MGR)/nm_mgr_rx.c
		$(CC) -c $(MGR)/nm_mgr_sql.c
		$(CC) -c $(MGR)/nm_mgr_ui.c
		$(CC) -c $(MGR)/ui_input.c
		
