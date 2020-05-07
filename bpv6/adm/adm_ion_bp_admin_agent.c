/****************************************************************************
 **
 ** File Name: adm_ion_bp_admin_agent.c
 **
 ** Description: TODO
 **
 ** Notes: TODO
 **
 ** Assumptions: TODO
 **
 ** Modification History: 
 **  YYYY-MM-DD  AUTHOR           DESCRIPTION
 **  ----------  --------------   --------------------------------------------
 **  2020-04-13  AUTO             Auto-generated c file 
 **
 ****************************************************************************/


#include "ion.h"
#include "platform.h"
#include "adm_ion_bp_admin.h"
#include "shared/utils/utils.h"
#include "shared/primitives/report.h"
#include "shared/primitives/blob.h"
#include "adm_ion_bp_admin_impl.h"
#include "agent/rda.h"



#define _HAVE_DTN_ION_BPADMIN_ADM_
#ifdef _HAVE_DTN_ION_BPADMIN_ADM_

static vec_idx_t g_dtn_ion_bpadmin_idx[11];

void dtn_ion_bpadmin_init()
{
	adm_add_adm_info("dtn_ion_bpadmin", ADM_ENUM_DTN_ION_BPADMIN);

	VDB_ADD_NN(((ADM_ENUM_DTN_ION_BPADMIN * 20) + ADM_META_IDX), &(g_dtn_ion_bpadmin_idx[ADM_META_IDX]));
	VDB_ADD_NN(((ADM_ENUM_DTN_ION_BPADMIN * 20) + ADM_TBLT_IDX), &(g_dtn_ion_bpadmin_idx[ADM_TBLT_IDX]));
	VDB_ADD_NN(((ADM_ENUM_DTN_ION_BPADMIN * 20) + ADM_EDD_IDX), &(g_dtn_ion_bpadmin_idx[ADM_EDD_IDX]));
	VDB_ADD_NN(((ADM_ENUM_DTN_ION_BPADMIN * 20) + ADM_CTRL_IDX), &(g_dtn_ion_bpadmin_idx[ADM_CTRL_IDX]));


	dtn_ion_bpadmin_setup();
	dtn_ion_bpadmin_init_meta();
	dtn_ion_bpadmin_init_cnst();
	dtn_ion_bpadmin_init_edd();
	dtn_ion_bpadmin_init_op();
	dtn_ion_bpadmin_init_var();
	dtn_ion_bpadmin_init_ctrl();
	dtn_ion_bpadmin_init_mac();
	dtn_ion_bpadmin_init_rpttpl();
	dtn_ion_bpadmin_init_tblt();
}

void dtn_ion_bpadmin_init_meta()
{

	adm_add_cnst(adm_build_ari(AMP_TYPE_CNST, 0, g_dtn_ion_bpadmin_idx[ADM_META_IDX], DTN_ION_BPADMIN_META_NAME), dtn_ion_bpadmin_meta_name);
	adm_add_cnst(adm_build_ari(AMP_TYPE_CNST, 0, g_dtn_ion_bpadmin_idx[ADM_META_IDX], DTN_ION_BPADMIN_META_NAMESPACE), dtn_ion_bpadmin_meta_namespace);
	adm_add_cnst(adm_build_ari(AMP_TYPE_CNST, 0, g_dtn_ion_bpadmin_idx[ADM_META_IDX], DTN_ION_BPADMIN_META_VERSION), dtn_ion_bpadmin_meta_version);
	adm_add_cnst(adm_build_ari(AMP_TYPE_CNST, 0, g_dtn_ion_bpadmin_idx[ADM_META_IDX], DTN_ION_BPADMIN_META_ORGANIZATION), dtn_ion_bpadmin_meta_organization);
}

void dtn_ion_bpadmin_init_cnst()
{

}

void dtn_ion_bpadmin_init_edd()
{

	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_ion_bpadmin_idx[ADM_EDD_IDX], DTN_ION_BPADMIN_EDD_BP_VERSION), dtn_ion_bpadmin_get_bp_version);
}

void dtn_ion_bpadmin_init_op()
{

}

void dtn_ion_bpadmin_init_var()
{

}

void dtn_ion_bpadmin_init_ctrl()
{

	adm_add_ctrldef(g_dtn_ion_bpadmin_idx[ADM_CTRL_IDX], DTN_ION_BPADMIN_CTRL_ENDPOINT_ADD, 3, dtn_ion_bpadmin_ctrl_endpoint_add);
	adm_add_ctrldef(g_dtn_ion_bpadmin_idx[ADM_CTRL_IDX], DTN_ION_BPADMIN_CTRL_ENDPOINT_CHANGE, 3, dtn_ion_bpadmin_ctrl_endpoint_change);
	adm_add_ctrldef(g_dtn_ion_bpadmin_idx[ADM_CTRL_IDX], DTN_ION_BPADMIN_CTRL_ENDPOINT_DEL, 1, dtn_ion_bpadmin_ctrl_endpoint_del);
	adm_add_ctrldef(g_dtn_ion_bpadmin_idx[ADM_CTRL_IDX], DTN_ION_BPADMIN_CTRL_INDUCT_ADD, 3, dtn_ion_bpadmin_ctrl_induct_add);
	adm_add_ctrldef(g_dtn_ion_bpadmin_idx[ADM_CTRL_IDX], DTN_ION_BPADMIN_CTRL_INDUCT_CHANGE, 3, dtn_ion_bpadmin_ctrl_induct_change);
	adm_add_ctrldef(g_dtn_ion_bpadmin_idx[ADM_CTRL_IDX], DTN_ION_BPADMIN_CTRL_INDUCT_DEL, 2, dtn_ion_bpadmin_ctrl_induct_del);
	adm_add_ctrldef(g_dtn_ion_bpadmin_idx[ADM_CTRL_IDX], DTN_ION_BPADMIN_CTRL_INDUCT_START, 2, dtn_ion_bpadmin_ctrl_induct_start);
	adm_add_ctrldef(g_dtn_ion_bpadmin_idx[ADM_CTRL_IDX], DTN_ION_BPADMIN_CTRL_INDUCT_STOP, 2, dtn_ion_bpadmin_ctrl_induct_stop);
	adm_add_ctrldef(g_dtn_ion_bpadmin_idx[ADM_CTRL_IDX], DTN_ION_BPADMIN_CTRL_MANAGE_HEAP_MAX, 1, dtn_ion_bpadmin_ctrl_manage_heap_max);
	adm_add_ctrldef(g_dtn_ion_bpadmin_idx[ADM_CTRL_IDX], DTN_ION_BPADMIN_CTRL_OUTDUCT_ADD, 4, dtn_ion_bpadmin_ctrl_outduct_add);
	adm_add_ctrldef(g_dtn_ion_bpadmin_idx[ADM_CTRL_IDX], DTN_ION_BPADMIN_CTRL_OUTDUCT_CHANGE, 4, dtn_ion_bpadmin_ctrl_outduct_change);
	adm_add_ctrldef(g_dtn_ion_bpadmin_idx[ADM_CTRL_IDX], DTN_ION_BPADMIN_CTRL_OUTDUCT_DEL, 2, dtn_ion_bpadmin_ctrl_outduct_del);
	adm_add_ctrldef(g_dtn_ion_bpadmin_idx[ADM_CTRL_IDX], DTN_ION_BPADMIN_CTRL_OUTDUCT_START, 2, dtn_ion_bpadmin_ctrl_outduct_start);
	adm_add_ctrldef(g_dtn_ion_bpadmin_idx[ADM_CTRL_IDX], DTN_ION_BPADMIN_CTRL_EGRESS_PLAN_BLOCK, 1, dtn_ion_bpadmin_ctrl_egress_plan_block);
	adm_add_ctrldef(g_dtn_ion_bpadmin_idx[ADM_CTRL_IDX], DTN_ION_BPADMIN_CTRL_EGRESS_PLAN_UNBLOCK, 1, dtn_ion_bpadmin_ctrl_egress_plan_unblock);
	adm_add_ctrldef(g_dtn_ion_bpadmin_idx[ADM_CTRL_IDX], DTN_ION_BPADMIN_CTRL_OUTDUCT_STOP, 2, dtn_ion_bpadmin_ctrl_outduct_stop);
	adm_add_ctrldef(g_dtn_ion_bpadmin_idx[ADM_CTRL_IDX], DTN_ION_BPADMIN_CTRL_PROTOCOL_ADD, 4, dtn_ion_bpadmin_ctrl_protocol_add);
	adm_add_ctrldef(g_dtn_ion_bpadmin_idx[ADM_CTRL_IDX], DTN_ION_BPADMIN_CTRL_PROTOCOL_DEL, 1, dtn_ion_bpadmin_ctrl_protocol_del);
	adm_add_ctrldef(g_dtn_ion_bpadmin_idx[ADM_CTRL_IDX], DTN_ION_BPADMIN_CTRL_PROTOCOL_START, 1, dtn_ion_bpadmin_ctrl_protocol_start);
	adm_add_ctrldef(g_dtn_ion_bpadmin_idx[ADM_CTRL_IDX], DTN_ION_BPADMIN_CTRL_PROTOCOL_STOP, 1, dtn_ion_bpadmin_ctrl_protocol_stop);
	adm_add_ctrldef(g_dtn_ion_bpadmin_idx[ADM_CTRL_IDX], DTN_ION_BPADMIN_CTRL_SCHEME_ADD, 3, dtn_ion_bpadmin_ctrl_scheme_add);
	adm_add_ctrldef(g_dtn_ion_bpadmin_idx[ADM_CTRL_IDX], DTN_ION_BPADMIN_CTRL_SCHEME_CHANGE, 3, dtn_ion_bpadmin_ctrl_scheme_change);
	adm_add_ctrldef(g_dtn_ion_bpadmin_idx[ADM_CTRL_IDX], DTN_ION_BPADMIN_CTRL_SCHEME_DEL, 1, dtn_ion_bpadmin_ctrl_scheme_del);
	adm_add_ctrldef(g_dtn_ion_bpadmin_idx[ADM_CTRL_IDX], DTN_ION_BPADMIN_CTRL_SCHEME_START, 1, dtn_ion_bpadmin_ctrl_scheme_start);
	adm_add_ctrldef(g_dtn_ion_bpadmin_idx[ADM_CTRL_IDX], DTN_ION_BPADMIN_CTRL_SCHEME_STOP, 1, dtn_ion_bpadmin_ctrl_scheme_stop);
	adm_add_ctrldef(g_dtn_ion_bpadmin_idx[ADM_CTRL_IDX], DTN_ION_BPADMIN_CTRL_WATCH, 2, dtn_ion_bpadmin_ctrl_watch);
}

void dtn_ion_bpadmin_init_mac()
{

}

void dtn_ion_bpadmin_init_rpttpl()
{

}

void dtn_ion_bpadmin_init_tblt()
{

	tblt_t *def = NULL;

	/* ENDPOINTS */

	def = tblt_create(adm_build_ari(AMP_TYPE_TBLT, 0, g_dtn_ion_bpadmin_idx[ADM_TBLT_IDX], DTN_ION_BPADMIN_TBLT_ENDPOINTS), dtn_ion_bpadmin_tblt_endpoints);
	tblt_add_col(def, AMP_TYPE_STR, "scheme_name");
	tblt_add_col(def, AMP_TYPE_STR, "endpoint_nss");
	tblt_add_col(def, AMP_TYPE_UINT, "app_pid");
	tblt_add_col(def, AMP_TYPE_STR, "recv_rule");
	tblt_add_col(def, AMP_TYPE_STR, "rcv_script");
	adm_add_tblt(def);

	/* INDUCTS */

	def = tblt_create(adm_build_ari(AMP_TYPE_TBLT, 0, g_dtn_ion_bpadmin_idx[ADM_TBLT_IDX], DTN_ION_BPADMIN_TBLT_INDUCTS), dtn_ion_bpadmin_tblt_inducts);
	tblt_add_col(def, AMP_TYPE_STR, "protocol_name");
	tblt_add_col(def, AMP_TYPE_STR, "duct_name");
	tblt_add_col(def, AMP_TYPE_STR, "cli_control");
	adm_add_tblt(def);

	/* OUTDUCTS */

	def = tblt_create(adm_build_ari(AMP_TYPE_TBLT, 0, g_dtn_ion_bpadmin_idx[ADM_TBLT_IDX], DTN_ION_BPADMIN_TBLT_OUTDUCTS), dtn_ion_bpadmin_tblt_outducts);
	tblt_add_col(def, AMP_TYPE_STR, "protocol_name");
	tblt_add_col(def, AMP_TYPE_STR, "duct_name");
	tblt_add_col(def, AMP_TYPE_UINT, "clo_pid");
	tblt_add_col(def, AMP_TYPE_STR, "clo_control");
	tblt_add_col(def, AMP_TYPE_UINT, "max_payload_length");
	adm_add_tblt(def);

	/* PROTOCOLS */

	def = tblt_create(adm_build_ari(AMP_TYPE_TBLT, 0, g_dtn_ion_bpadmin_idx[ADM_TBLT_IDX], DTN_ION_BPADMIN_TBLT_PROTOCOLS), dtn_ion_bpadmin_tblt_protocols);
	tblt_add_col(def, AMP_TYPE_STR, "name");
	tblt_add_col(def, AMP_TYPE_UINT, "payload_bpf");
	tblt_add_col(def, AMP_TYPE_UINT, "overhead_bpf");
	tblt_add_col(def, AMP_TYPE_UINT, "protocol class");
	adm_add_tblt(def);

	/* SCHEMES */

	def = tblt_create(adm_build_ari(AMP_TYPE_TBLT, 0, g_dtn_ion_bpadmin_idx[ADM_TBLT_IDX], DTN_ION_BPADMIN_TBLT_SCHEMES), dtn_ion_bpadmin_tblt_schemes);
	tblt_add_col(def, AMP_TYPE_STR, "scheme_name");
	tblt_add_col(def, AMP_TYPE_UINT, "fwd_pid");
	tblt_add_col(def, AMP_TYPE_STR, "fwd_cmd");
	tblt_add_col(def, AMP_TYPE_UINT, "admin_app_pid");
	tblt_add_col(def, AMP_TYPE_STR, "admin_app_cmd");
	adm_add_tblt(def);

	/* EGRESS_PLANS */

	def = tblt_create(adm_build_ari(AMP_TYPE_TBLT, 0, g_dtn_ion_bpadmin_idx[ADM_TBLT_IDX], DTN_ION_BPADMIN_TBLT_EGRESS_PLANS), dtn_ion_bpadmin_tblt_egress_plans);
	tblt_add_col(def, AMP_TYPE_STR, "neighbor_eid");
	tblt_add_col(def, AMP_TYPE_UINT, "clm_pid");
	tblt_add_col(def, AMP_TYPE_UINT, "nominal_rate");
	adm_add_tblt(def);
}

#endif // _HAVE_DTN_ION_BPADMIN_ADM_
