/****************************************************************************
 **
 ** File Name: adm_ltp_agent_mgr.c
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
 **  2018-01-08  AUTO             Auto-generated c file 
 **
 ****************************************************************************/


#include "ion.h"
#include "platform.h"
#include "../shared/adm/adm_ltp_agent.h"
#include "../shared/utils/utils.h"
#include "../shared/primitives/report.h"
#include "../shared/primitives/blob.h"
#include "metadata.h"
#include "nm_mgr_ui.h"

#define _HAVE_LTP_AGENT_ADM_
#ifdef _HAVE_LTP_AGENT_ADM_

void adm_ltp_agent_init()
{
	adm_ltp_agent_init_edd();
	adm_ltp_agent_init_vars();
	adm_ltp_agent_init_ctrldefs();
	adm_ltp_agent_init_constants();
	adm_ltp_agent_init_macros();
	adm_ltp_agent_init_metadata();
	adm_ltp_agent_init_ops();
	adm_ltp_agent_init_reports();
}


void adm_ltp_agent_init_edd()
{
	ari_t *id;
	metadata_t *meta;
	vec_idx_t nn_idx;

	VDB_ADD_NN(((ADM_ENUM_LTP_AGENT * 20) + ADM_EDD_OFFSET), &nn_idx);

	id = adm_build_reg_ari(AMP_TYPE_EDD, 1, nn_idx, ADM_LTP_AGENT_EDD_SPAN_REMOTE_ENGINE_NBR_ARI_NAME);
	meta = meta_create(id, ADM_ENUM_LTP_AGENT, "SPAN_REMOTE_ENGINE_NBR", "The remote engine number of this span.");
	meta_add_parm(meta, "ltp_span", AMP_TYPE_UINT);
	meta_store_obj(meta);




}


void adm_ltp_agent_init_vars()
{
}


void adm_ltp_agent_init_ctrldefs()
{
	ari_t *id;
	metadata_t *meta;
	vec_idx_t nn_idx;
	VDB_ADD_NN(((ADM_ENUM_LTP_AGENT * 20) + ADM_CTRL_OFFSET), &nn_idx);

	id = adm_build_reg_ari(AMP_TYPE_CTRL, 1, nn_idx, ADM_LTP_AGENT_CTRL_RESET_ARI_NAME);

	meta = meta_create(id, ADM_ENUM_LTP_AGENT, "RESET", "Resets the counters associated with the engine and updates the last reset time for the span to be the time when this control was run.");
	meta_add_parm(meta, "ltp_span", AMP_TYPE_UINT);
	meta_store_obj(meta);

}


void adm_ltp_agent_init_constants()
{
}


void adm_ltp_agent_init_macros()
{
}


void adm_ltp_agent_init_metadata()
{
	ari_t *id;
	metadata_t *meta;
	vec_idx_t nn_idx;
	VDB_ADD_NN(((ADM_ENUM_LTP_AGENT * 20) + ADM_META_OFFSET), &nn_idx);

	id = adm_build_reg_ari(AMP_TYPE_CNST, 0, nn_idx, ADM_LTP_AGENT_META_NAME_ARI_NAME);
	meta = meta_create(id, ADM_ENUM_LTP_AGENT, "NAME", "The human-readable name of the ADM.");
	meta_store_obj(meta);

	id = adm_build_reg_ari(AMP_TYPE_CNST, 0, nn_idx, ADM_LTP_AGENT_META_NAMESPACE_ARI_NAME);
	meta = meta_create(id, ADM_ENUM_LTP_AGENT, "NAMESPACE", "The namespace of the ADM.");
	meta_store_obj(meta);

	id = adm_build_reg_ari(AMP_TYPE_CNST, 0, nn_idx, ADM_LTP_AGENT_META_VERSION_ARI_NAME);
	meta = meta_create(id, ADM_ENUM_LTP_AGENT, "VERSION", "The version of the ADM.");
	meta_store_obj(meta);

	id = adm_build_reg_ari(AMP_TYPE_CNST, 0, nn_idx, ADM_LTP_AGENT_META_ORGANIZATION_ARI_NAME);
	meta = meta_create(id, ADM_ENUM_LTP_AGENT, "ORGANIZATION", "The name of the issuing organization of the ADM.");
	meta_store_obj(meta);
}


void adm_ltp_agent_init_ops()
{
}


void adm_ltp_agent_init_reports()
{
	ari_t *rpt_id;
	ari_t *rpt_entry_id;

	vec_idx_t nn_idx;
	rpttpl_t *def;
	metadata_t *meta;


	VDB_ADD_NN(((ADM_ENUM_LTP_AGENT * 20) + ADM_RPTT_OFFSET), &nn_idx);

	/* Create the Report Template Identity. */
	rpt_id = adm_build_reg_ari(AMP_TYPE_RPTTPL, 1, nn_idx, ADM_LTP_AGENT_RPT_ENDPOINTREPORT_ARI_NAME);

	/* Create the report template definition, including any entries. */
	def = rpttpl_create_id(rpt_id);

	rpt_entry_id = adm_build_reg_ari(AMP_TYPE_EDD, 1, nn_idx, ADM_LTP_AGENT_EDD_SPAN_REMOTE_ENGINE_NBR_ARI_NAME);

	/* Sarah: Here is an example of adding a regular paramdeter.
	 * Note in tnv.h we have multiple "tnv_from" functions.
	 */
	ari_add_parm_val(rpt_entry_id, tnv_from_uint(0));

	/*
	 * Sarah: Here is an example of adding a mapped parameter where
	 * the value of the parameter is taken from the value of some
	 * enclosing parm value.
	 *
	 * ari_add_parm_val_map(rpt_entry_id, tnv_from_map(AMP_TYPE_UINT, 0));
	 *
	 */

	rpttpl_add_item(def, rpt_entry_id);

	/* Create manager meta-data definition for this report template */
	meta = meta_create(rpt_id, ADM_ENUM_LTP_AGENT, "ENDPOINTREPORT", "This is all known endpoint information.");
	meta_add_parm(meta, "ltp_span", AMP_TYPE_UINT);
	meta_store_rpttpl(meta, def);
}

#endif // _HAVE_LTP_AGENT_ADM_

