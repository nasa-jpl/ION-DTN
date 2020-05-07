/****************************************************************************
 **
 ** File Name: adm_ion_ipn_admin_agent.c
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
#include "adm_ion_ipn_admin.h"
#include "shared/utils/utils.h"
#include "shared/primitives/report.h"
#include "shared/primitives/blob.h"
#include "adm_ion_ipn_admin_impl.h"
#include "agent/rda.h"



#define _HAVE_DTN_ION_IPNADMIN_ADM_
#ifdef _HAVE_DTN_ION_IPNADMIN_ADM_

static vec_idx_t g_dtn_ion_ipnadmin_idx[11];

void dtn_ion_ipnadmin_init()
{
	adm_add_adm_info("dtn_ion_ipnadmin", ADM_ENUM_DTN_ION_IPNADMIN);

	VDB_ADD_NN(((ADM_ENUM_DTN_ION_IPNADMIN * 20) + ADM_META_IDX), &(g_dtn_ion_ipnadmin_idx[ADM_META_IDX]));
	VDB_ADD_NN(((ADM_ENUM_DTN_ION_IPNADMIN * 20) + ADM_TBLT_IDX), &(g_dtn_ion_ipnadmin_idx[ADM_TBLT_IDX]));
	VDB_ADD_NN(((ADM_ENUM_DTN_ION_IPNADMIN * 20) + ADM_EDD_IDX), &(g_dtn_ion_ipnadmin_idx[ADM_EDD_IDX]));
	VDB_ADD_NN(((ADM_ENUM_DTN_ION_IPNADMIN * 20) + ADM_CTRL_IDX), &(g_dtn_ion_ipnadmin_idx[ADM_CTRL_IDX]));


	dtn_ion_ipnadmin_setup();
	dtn_ion_ipnadmin_init_meta();
	dtn_ion_ipnadmin_init_cnst();
	dtn_ion_ipnadmin_init_edd();
	dtn_ion_ipnadmin_init_op();
	dtn_ion_ipnadmin_init_var();
	dtn_ion_ipnadmin_init_ctrl();
	dtn_ion_ipnadmin_init_mac();
	dtn_ion_ipnadmin_init_rpttpl();
	dtn_ion_ipnadmin_init_tblt();
}

void dtn_ion_ipnadmin_init_meta()
{

	adm_add_cnst(adm_build_ari(AMP_TYPE_CNST, 0, g_dtn_ion_ipnadmin_idx[ADM_META_IDX], DTN_ION_IPNADMIN_META_NAME), dtn_ion_ipnadmin_meta_name);
	adm_add_cnst(adm_build_ari(AMP_TYPE_CNST, 0, g_dtn_ion_ipnadmin_idx[ADM_META_IDX], DTN_ION_IPNADMIN_META_NAMESPACE), dtn_ion_ipnadmin_meta_namespace);
	adm_add_cnst(adm_build_ari(AMP_TYPE_CNST, 0, g_dtn_ion_ipnadmin_idx[ADM_META_IDX], DTN_ION_IPNADMIN_META_VERSION), dtn_ion_ipnadmin_meta_version);
	adm_add_cnst(adm_build_ari(AMP_TYPE_CNST, 0, g_dtn_ion_ipnadmin_idx[ADM_META_IDX], DTN_ION_IPNADMIN_META_ORGANIZATION), dtn_ion_ipnadmin_meta_organization);
}

void dtn_ion_ipnadmin_init_cnst()
{

}

void dtn_ion_ipnadmin_init_edd()
{

	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_ion_ipnadmin_idx[ADM_EDD_IDX], DTN_ION_IPNADMIN_EDD_ION_VERSION), dtn_ion_ipnadmin_get_ion_version);
}

void dtn_ion_ipnadmin_init_op()
{

}

void dtn_ion_ipnadmin_init_var()
{

}

void dtn_ion_ipnadmin_init_ctrl()
{

	adm_add_ctrldef(g_dtn_ion_ipnadmin_idx[ADM_CTRL_IDX], DTN_ION_IPNADMIN_CTRL_EXIT_ADD, 3, dtn_ion_ipnadmin_ctrl_exit_add);
	adm_add_ctrldef(g_dtn_ion_ipnadmin_idx[ADM_CTRL_IDX], DTN_ION_IPNADMIN_CTRL_EXIT_CHANGE, 3, dtn_ion_ipnadmin_ctrl_exit_change);
	adm_add_ctrldef(g_dtn_ion_ipnadmin_idx[ADM_CTRL_IDX], DTN_ION_IPNADMIN_CTRL_EXIT_DEL, 2, dtn_ion_ipnadmin_ctrl_exit_del);
	adm_add_ctrldef(g_dtn_ion_ipnadmin_idx[ADM_CTRL_IDX], DTN_ION_IPNADMIN_CTRL_PLAN_ADD, 2, dtn_ion_ipnadmin_ctrl_plan_add);
	adm_add_ctrldef(g_dtn_ion_ipnadmin_idx[ADM_CTRL_IDX], DTN_ION_IPNADMIN_CTRL_PLAN_CHANGE, 2, dtn_ion_ipnadmin_ctrl_plan_change);
	adm_add_ctrldef(g_dtn_ion_ipnadmin_idx[ADM_CTRL_IDX], DTN_ION_IPNADMIN_CTRL_PLAN_DEL, 1, dtn_ion_ipnadmin_ctrl_plan_del);
}

void dtn_ion_ipnadmin_init_mac()
{

}

void dtn_ion_ipnadmin_init_rpttpl()
{

}

void dtn_ion_ipnadmin_init_tblt()
{

	tblt_t *def = NULL;

	/* EXITS */

	def = tblt_create(adm_build_ari(AMP_TYPE_TBLT, 0, g_dtn_ion_ipnadmin_idx[ADM_TBLT_IDX], DTN_ION_IPNADMIN_TBLT_EXITS), dtn_ion_ipnadmin_tblt_exits);
	tblt_add_col(def, AMP_TYPE_UVAST, "first_node_nbr");
	tblt_add_col(def, AMP_TYPE_UVAST, "last_node_nbr");
	tblt_add_col(def, AMP_TYPE_STR, "gateway_endpoint_id");
	adm_add_tblt(def);

	/* PLANS */

	def = tblt_create(adm_build_ari(AMP_TYPE_TBLT, 0, g_dtn_ion_ipnadmin_idx[ADM_TBLT_IDX], DTN_ION_IPNADMIN_TBLT_PLANS), dtn_ion_ipnadmin_tblt_plans);
	tblt_add_col(def, AMP_TYPE_UVAST, "node_nbr");
	tblt_add_col(def, AMP_TYPE_STR, "action");
	tblt_add_col(def, AMP_TYPE_STR, "spec");
	adm_add_tblt(def);
}

#endif // _HAVE_DTN_ION_IPNADMIN_ADM_
