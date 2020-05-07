/****************************************************************************
 **
 ** File Name: adm_ion_ltp_admin_agent.c
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
#include "adm_ion_ltp_admin.h"
#include "shared/utils/utils.h"
#include "shared/primitives/report.h"
#include "shared/primitives/blob.h"
#include "adm_ion_ltp_admin_impl.h"
#include "agent/rda.h"



#define _HAVE_DTN_ION_LTPADMIN_ADM_
#ifdef _HAVE_DTN_ION_LTPADMIN_ADM_

static vec_idx_t g_dtn_ion_ltpadmin_idx[11];

void dtn_ion_ltpadmin_init()
{
	adm_add_adm_info("dtn_ion_ltpadmin", ADM_ENUM_DTN_ION_LTPADMIN);

	VDB_ADD_NN(((ADM_ENUM_DTN_ION_LTPADMIN * 20) + ADM_META_IDX), &(g_dtn_ion_ltpadmin_idx[ADM_META_IDX]));
	VDB_ADD_NN(((ADM_ENUM_DTN_ION_LTPADMIN * 20) + ADM_TBLT_IDX), &(g_dtn_ion_ltpadmin_idx[ADM_TBLT_IDX]));
	VDB_ADD_NN(((ADM_ENUM_DTN_ION_LTPADMIN * 20) + ADM_EDD_IDX), &(g_dtn_ion_ltpadmin_idx[ADM_EDD_IDX]));
	VDB_ADD_NN(((ADM_ENUM_DTN_ION_LTPADMIN * 20) + ADM_CTRL_IDX), &(g_dtn_ion_ltpadmin_idx[ADM_CTRL_IDX]));


	dtn_ion_ltpadmin_setup();
	dtn_ion_ltpadmin_init_meta();
	dtn_ion_ltpadmin_init_cnst();
	dtn_ion_ltpadmin_init_edd();
	dtn_ion_ltpadmin_init_op();
	dtn_ion_ltpadmin_init_var();
	dtn_ion_ltpadmin_init_ctrl();
	dtn_ion_ltpadmin_init_mac();
	dtn_ion_ltpadmin_init_rpttpl();
	dtn_ion_ltpadmin_init_tblt();
}

void dtn_ion_ltpadmin_init_meta()
{

	adm_add_cnst(adm_build_ari(AMP_TYPE_CNST, 0, g_dtn_ion_ltpadmin_idx[ADM_META_IDX], DTN_ION_LTPADMIN_META_NAME), dtn_ion_ltpadmin_meta_name);
	adm_add_cnst(adm_build_ari(AMP_TYPE_CNST, 0, g_dtn_ion_ltpadmin_idx[ADM_META_IDX], DTN_ION_LTPADMIN_META_NAMESPACE), dtn_ion_ltpadmin_meta_namespace);
	adm_add_cnst(adm_build_ari(AMP_TYPE_CNST, 0, g_dtn_ion_ltpadmin_idx[ADM_META_IDX], DTN_ION_LTPADMIN_META_VERSION), dtn_ion_ltpadmin_meta_version);
	adm_add_cnst(adm_build_ari(AMP_TYPE_CNST, 0, g_dtn_ion_ltpadmin_idx[ADM_META_IDX], DTN_ION_LTPADMIN_META_ORGANIZATION), dtn_ion_ltpadmin_meta_organization);
}

void dtn_ion_ltpadmin_init_cnst()
{

}

void dtn_ion_ltpadmin_init_edd()
{

	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_ion_ltpadmin_idx[ADM_EDD_IDX], DTN_ION_LTPADMIN_EDD_ION_VERSION), dtn_ion_ltpadmin_get_ion_version);
}

void dtn_ion_ltpadmin_init_op()
{

}

void dtn_ion_ltpadmin_init_var()
{

}

void dtn_ion_ltpadmin_init_ctrl()
{

	adm_add_ctrldef(g_dtn_ion_ltpadmin_idx[ADM_CTRL_IDX], DTN_ION_LTPADMIN_CTRL_MANAGE_HEAP, 1, dtn_ion_ltpadmin_ctrl_manage_heap);
	adm_add_ctrldef(g_dtn_ion_ltpadmin_idx[ADM_CTRL_IDX], DTN_ION_LTPADMIN_CTRL_MANAGE_MAX_BER, 1, dtn_ion_ltpadmin_ctrl_manage_max_ber);
	adm_add_ctrldef(g_dtn_ion_ltpadmin_idx[ADM_CTRL_IDX], DTN_ION_LTPADMIN_CTRL_MANAGE_OWN_QUEUE_TIME, 1, dtn_ion_ltpadmin_ctrl_manage_own_queue_time);
	adm_add_ctrldef(g_dtn_ion_ltpadmin_idx[ADM_CTRL_IDX], DTN_ION_LTPADMIN_CTRL_MANAGE_SCREENING, 1, dtn_ion_ltpadmin_ctrl_manage_screening);
	adm_add_ctrldef(g_dtn_ion_ltpadmin_idx[ADM_CTRL_IDX], DTN_ION_LTPADMIN_CTRL_SPAN_ADD, 8, dtn_ion_ltpadmin_ctrl_span_add);
	adm_add_ctrldef(g_dtn_ion_ltpadmin_idx[ADM_CTRL_IDX], DTN_ION_LTPADMIN_CTRL_SPAN_CHANGE, 8, dtn_ion_ltpadmin_ctrl_span_change);
	adm_add_ctrldef(g_dtn_ion_ltpadmin_idx[ADM_CTRL_IDX], DTN_ION_LTPADMIN_CTRL_SPAN_DEL, 1, dtn_ion_ltpadmin_ctrl_span_del);
	adm_add_ctrldef(g_dtn_ion_ltpadmin_idx[ADM_CTRL_IDX], DTN_ION_LTPADMIN_CTRL_STOP, 0, dtn_ion_ltpadmin_ctrl_stop);
	adm_add_ctrldef(g_dtn_ion_ltpadmin_idx[ADM_CTRL_IDX], DTN_ION_LTPADMIN_CTRL_WATCH_SET, 1, dtn_ion_ltpadmin_ctrl_watch_set);
}

void dtn_ion_ltpadmin_init_mac()
{

}

void dtn_ion_ltpadmin_init_rpttpl()
{

}

void dtn_ion_ltpadmin_init_tblt()
{

	tblt_t *def = NULL;

	/* SPANS */

	def = tblt_create(adm_build_ari(AMP_TYPE_TBLT, 0, g_dtn_ion_ltpadmin_idx[ADM_TBLT_IDX], DTN_ION_LTPADMIN_TBLT_SPANS), dtn_ion_ltpadmin_tblt_spans);
	tblt_add_col(def, AMP_TYPE_UVAST, "peer_engine_nbr");
	tblt_add_col(def, AMP_TYPE_UINT, "max_export_sessions");
	tblt_add_col(def, AMP_TYPE_UINT, "max_import_sessions");
	tblt_add_col(def, AMP_TYPE_UINT, "max_segment_size");
	tblt_add_col(def, AMP_TYPE_UINT, "aggregation_size_limit");
	tblt_add_col(def, AMP_TYPE_UINT, "aggregation_time_limit");
	tblt_add_col(def, AMP_TYPE_STR, "lso_control");
	tblt_add_col(def, AMP_TYPE_UINT, "queueing_latency");
	adm_add_tblt(def);
}

#endif // _HAVE_DTN_ION_LTPADMIN_ADM_
