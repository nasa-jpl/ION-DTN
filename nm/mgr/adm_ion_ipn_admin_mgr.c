/****************************************************************************
 **
 ** File Name: adm_ion_ipn_admin_mgr.c
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
#include "metadata.h"
#include "nm_mgr_ui.h"




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

	ari_t *id = NULL;

	id = adm_build_ari(AMP_TYPE_CNST, 0, g_dtn_ion_ipnadmin_idx[ADM_META_IDX], DTN_ION_IPNADMIN_META_NAME);
	adm_add_cnst(id, NULL);
	meta_add_cnst(AMP_TYPE_STR, id, ADM_ENUM_DTN_ION_IPNADMIN, "name", "The human-readable name of the ADM.");

	id = adm_build_ari(AMP_TYPE_CNST, 0, g_dtn_ion_ipnadmin_idx[ADM_META_IDX], DTN_ION_IPNADMIN_META_NAMESPACE);
	adm_add_cnst(id, NULL);
	meta_add_cnst(AMP_TYPE_STR, id, ADM_ENUM_DTN_ION_IPNADMIN, "namespace", "The namespace of the ADM");

	id = adm_build_ari(AMP_TYPE_CNST, 0, g_dtn_ion_ipnadmin_idx[ADM_META_IDX], DTN_ION_IPNADMIN_META_VERSION);
	adm_add_cnst(id, NULL);
	meta_add_cnst(AMP_TYPE_STR, id, ADM_ENUM_DTN_ION_IPNADMIN, "version", "The version of the ADM");

	id = adm_build_ari(AMP_TYPE_CNST, 0, g_dtn_ion_ipnadmin_idx[ADM_META_IDX], DTN_ION_IPNADMIN_META_ORGANIZATION);
	adm_add_cnst(id, NULL);
	meta_add_cnst(AMP_TYPE_STR, id, ADM_ENUM_DTN_ION_IPNADMIN, "organization", "The name of the issuing organization of the ADM");

}

void dtn_ion_ipnadmin_init_cnst()
{

}

void dtn_ion_ipnadmin_init_edd()
{

	ari_t *id = NULL;

	id = adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_ion_ipnadmin_idx[ADM_EDD_IDX], DTN_ION_IPNADMIN_EDD_ION_VERSION);
	adm_add_edd(id, NULL);
	meta_add_edd(AMP_TYPE_STR, id, ADM_ENUM_DTN_ION_IPNADMIN, "ion_version", "This is the version of ion is that currently installed.");

}

void dtn_ion_ipnadmin_init_op()
{

}

void dtn_ion_ipnadmin_init_var()
{

}

void dtn_ion_ipnadmin_init_ctrl()
{

	ari_t *id = NULL;

	metadata_t *meta = NULL;


	/* EXIT_ADD */

	id = adm_build_ari(AMP_TYPE_CTRL, 1, g_dtn_ion_ipnadmin_idx[ADM_CTRL_IDX], DTN_ION_IPNADMIN_CTRL_EXIT_ADD);
	adm_add_ctrldef_ari(id, 3, NULL);
	meta = meta_add_ctrl(id, ADM_ENUM_DTN_ION_IPNADMIN, "exit_add", "This control establishes an exit for static default routing.");

	meta_add_parm(meta, "first_node_nbr", AMP_TYPE_UINT);
	meta_add_parm(meta, "last_node_nbr", AMP_TYPE_UINT);
	meta_add_parm(meta, "gateway_endpoint_id", AMP_TYPE_STR);

	/* EXIT_CHANGE */

	id = adm_build_ari(AMP_TYPE_CTRL, 1, g_dtn_ion_ipnadmin_idx[ADM_CTRL_IDX], DTN_ION_IPNADMIN_CTRL_EXIT_CHANGE);
	adm_add_ctrldef_ari(id, 3, NULL);
	meta = meta_add_ctrl(id, ADM_ENUM_DTN_ION_IPNADMIN, "exit_change", "This control changes the gateway node number for the exit identified by firstNodeNbr and lastNodeNbr.");

	meta_add_parm(meta, "first_node_nbr", AMP_TYPE_UINT);
	meta_add_parm(meta, "last_node_nbr", AMP_TYPE_UINT);
	meta_add_parm(meta, "gatewayEndpointId", AMP_TYPE_STR);

	/* EXIT_DEL */

	id = adm_build_ari(AMP_TYPE_CTRL, 1, g_dtn_ion_ipnadmin_idx[ADM_CTRL_IDX], DTN_ION_IPNADMIN_CTRL_EXIT_DEL);
	adm_add_ctrldef_ari(id, 2, NULL);
	meta = meta_add_ctrl(id, ADM_ENUM_DTN_ION_IPNADMIN, "exit_del", "This control deletes the exit identified by firstNodeNbr and lastNodeNbr.");

	meta_add_parm(meta, "first_node_nbr", AMP_TYPE_UINT);
	meta_add_parm(meta, "last_node_nbr", AMP_TYPE_UINT);

	/* PLAN_ADD */

	id = adm_build_ari(AMP_TYPE_CTRL, 1, g_dtn_ion_ipnadmin_idx[ADM_CTRL_IDX], DTN_ION_IPNADMIN_CTRL_PLAN_ADD);
	adm_add_ctrldef_ari(id, 2, NULL);
	meta = meta_add_ctrl(id, ADM_ENUM_DTN_ION_IPNADMIN, "plan_add", "This control establishes an egress plan for the bundles that must be transmitted to the neighboring node that is identified by it's nodeNbr.");

	meta_add_parm(meta, "node_nbr", AMP_TYPE_UVAST);
	meta_add_parm(meta, "xmit_rate", AMP_TYPE_UINT);

	/* PLAN_CHANGE */

	id = adm_build_ari(AMP_TYPE_CTRL, 1, g_dtn_ion_ipnadmin_idx[ADM_CTRL_IDX], DTN_ION_IPNADMIN_CTRL_PLAN_CHANGE);
	adm_add_ctrldef_ari(id, 2, NULL);
	meta = meta_add_ctrl(id, ADM_ENUM_DTN_ION_IPNADMIN, "plan_change", "This control changes the duct expression for the indicated plan.");

	meta_add_parm(meta, "node_nbr", AMP_TYPE_UINT);
	meta_add_parm(meta, "default_duct_expression", AMP_TYPE_STR);

	/* PLAN_DEL */

	id = adm_build_ari(AMP_TYPE_CTRL, 1, g_dtn_ion_ipnadmin_idx[ADM_CTRL_IDX], DTN_ION_IPNADMIN_CTRL_PLAN_DEL);
	adm_add_ctrldef_ari(id, 1, NULL);
	meta = meta_add_ctrl(id, ADM_ENUM_DTN_ION_IPNADMIN, "plan_del", "This control deletes the egress plan for the node that is identified by it's nodeNbr.");

	meta_add_parm(meta, "node_nbr", AMP_TYPE_UINT);
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

	def = tblt_create(adm_build_ari(AMP_TYPE_TBLT, 0, g_dtn_ion_ipnadmin_idx[ADM_TBLT_IDX], DTN_ION_IPNADMIN_TBLT_EXITS), NULL);
	tblt_add_col(def, AMP_TYPE_UVAST, "first_node_nbr");
	tblt_add_col(def, AMP_TYPE_UVAST, "last_node_nbr");
	tblt_add_col(def, AMP_TYPE_STR, "gateway_endpoint_id");
	adm_add_tblt(def);
	meta_add_tblt(def->id, ADM_ENUM_DTN_ION_IPNADMIN, "exits", "This table lists all of the exits that are defined in the IPN database for the local node.");

	/* PLANS */

	def = tblt_create(adm_build_ari(AMP_TYPE_TBLT, 0, g_dtn_ion_ipnadmin_idx[ADM_TBLT_IDX], DTN_ION_IPNADMIN_TBLT_PLANS), NULL);
	tblt_add_col(def, AMP_TYPE_UVAST, "node_nbr");
	tblt_add_col(def, AMP_TYPE_STR, "action");
	tblt_add_col(def, AMP_TYPE_STR, "spec");
	adm_add_tblt(def);
	meta_add_tblt(def->id, ADM_ENUM_DTN_ION_IPNADMIN, "plans", "This table lists all of the egress plans that are established in the IPN database for the local node.");
}

#endif // _HAVE_DTN_ION_IPNADMIN_ADM_
