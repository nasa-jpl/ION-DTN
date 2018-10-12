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

	id = adm_build_reg_ari(0x82, nn_idx, ADM_LTP_AGENT_EDD_SPAN_REMOTE_ENGINE_NBR_ARI_NAME, NULL);
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

	id = adm_build_reg_ari(0x81, nn_idx, ADM_LTP_AGENT_CTRL_RESET_ARI_NAME, NULL);

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

	id = adm_build_reg_ari(0x80, nn_idx, ADM_LTP_AGENT_META_NAME_ARI_NAME, NULL);
	meta = meta_create(id, ADM_ENUM_LTP_AGENT, "NAME", "The human-readable name of the ADM.");
	meta_store_obj(meta);

	id = adm_build_reg_ari(0x80, nn_idx, ADM_LTP_AGENT_META_NAMESPACE_ARI_NAME, NULL);
	meta = meta_create(id, ADM_ENUM_LTP_AGENT, "NAMESPACE", "The namespace of the ADM.");
	meta_store_obj(meta);

	id = adm_build_reg_ari(0x80, nn_idx, ADM_LTP_AGENT_META_VERSION_ARI_NAME, NULL);
	meta = meta_create(id, ADM_ENUM_LTP_AGENT, "VERSION", "The version of the ADM.");
	meta_store_obj(meta);

	id = adm_build_reg_ari(0x80, nn_idx, ADM_LTP_AGENT_META_ORGANIZATION_ARI_NAME, NULL);
	meta = meta_create(id, ADM_ENUM_LTP_AGENT, "ORGANIZATION", "The name of the issuing organization of the ADM.");
	meta_store_obj(meta);
}


void adm_ltp_agent_init_ops()
{
}


void adm_ltp_agent_init_reports()
{
	ari_t *id;
	vec_idx_t nn_idx;
	rpttpl_t *def;
	metadata_t *meta;


	VDB_ADD_NN(((ADM_ENUM_LTP_AGENT * 20) + ADM_RPTT_OFFSET), &nn_idx);

	id = adm_build_reg_ari(0x87, nn_idx, ADM_LTP_AGENT_RPT_ENDPOINTREPORT_ARI_NAME, NULL);
	def = rpttpl_create_id(id);
	rpttpl_add_item(def, adm_build_reg_ari(0x82, nn_idx, ADM_LTP_AGENT_EDD_SPAN_REMOTE_ENGINE_NBR_ARI_NAME, NULL));

	meta = meta_create(id, ADM_ENUM_LTP_AGENT, "ENDPOINTREPORT", "This is all known endpoint information.");
	meta_add_parm(meta, "ltp_span", AMP_TYPE_UINT);
	meta_store_rpttpl(meta, def);
}

#endif // _HAVE_LTP_AGENT_ADM_
