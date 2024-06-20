#include "bpsecadmin_config.h"

BpSecMap gSvcMap[] = {
	{"bib-integrity", 		SC_SVC_BIBINT},
	{"bib", 				SC_SVC_BIBINT},
	{"integrity", 			SC_SVC_BIBINT},
	{"bcb-confidentiality", SC_SVC_BCBCONF},
	{"bcb", 				SC_SVC_BCBCONF},
	{"confidentiality", 	SC_SVC_BCBCONF},
	{NULL,0}
};

BpSecMap gRoleMap[] = {
	{"s",                BPRF_SRC_ROLE},
	{"source",           BPRF_SRC_ROLE},
	{"sec_source",       BPRF_SRC_ROLE},
	{"v",                BPRF_VER_ROLE},
	{"verifier",         BPRF_VER_ROLE},
	{"sec_verifier",     BPRF_VER_ROLE},
	{"a",                BPRF_ACC_ROLE},
	{"acceptor",         BPRF_ACC_ROLE},
	{"sec_acceptor",     BPRF_ACC_ROLE},
	{NULL,0}
};

BpSecMap gActionMap[] = {
	{"remove_sop",             BSLACT_REMOVE_SOP},
	{"remove_sop_target",      BSLACT_REMOVE_SOP_TARGET},
	{"remove_all_target_sops", BSLACT_REMOVE_ALL_TARGET_SOPS},
	{"do_not_forward",         BSLACT_DO_NOT_FORWARD},
	{"request_storage",        BSLACT_NOT_IMPLEMENTED}, //BSLACT_REQUEST_STORAGE},
	{"report_reason_code",     BSLACT_REPORT_REASON_CODE},
	{"override_target_bpcf",   BSLACT_NOT_IMPLEMENTED}, //BSLACT_OVERRIDE_TARGET_BPCF},
	{"override_sop_bpcf",      BSLACT_NOT_IMPLEMENTED}, //BSLACT_OVERRIDE_SOP_BPCF},
	{NULL,0}
};

BpSecMap gScParmMap[] = {
	{"key_name", CSI_PARM_KEYINFO},
	{"iv",       CSI_PARM_IV},
	{"salt",     CSI_PARM_SALT},
	{"icv",      CSI_PARM_ICV},
	{"intsig",   CSI_PARM_INTSIG},
	{"bek",      CSI_PARM_BEK},
	{"bekicv",   CSI_PARM_BEKICV},
	{NULL,0}
};

BpSecMap gKeyWords[] = {
		{KNS_NAME, KID_NAME},
		{KNS_DESC, KID_DESC},
		{KNS_ES_REF, KID_ES_REF},
		{KNS_EVENT_ID, KID_EVENT_ID},
		{KNS_ACTIONS, KID_ACTIONS},
		{KNS_ID, KID_ID},
		{KNS_REASON_CODE, KID_REASON_CODE},
		{KNS_NEW_VALUE, KID_NEW_VALUE},
		{KNS_MASK, KID_MASK},
		{KNS_FILTER, KID_FILTER},
		{KNS_SRC, KID_SRC},
		{KNS_DEST, KID_DEST},
		{KNS_SEC_SRC, KID_SEC_SRC},
		{KNS_SPEC, KID_SPEC},
		{KNS_ROLE, KID_ROLE},
		{KNS_TGT, KID_TGT},
		{KNS_TYPE, KID_TYPE},
		{KNS_SC_ID, KID_SC_ID},
		{KNS_SVC, KID_SVC},
		{KNS_RULE_ID, KID_RULE_ID},
		{KNS_SC_PARMS, KID_SC_PARMS},
		{KNS_POLICYRULE, KID_POLICYRULE},
		{KNS_EVENT_SET, KID_EVENT_SET},
		{KNS_EVENT, KID_EVENT}
};