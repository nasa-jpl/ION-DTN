/****************************************************************************
 **
 ** File Name: adm_ltp_agent_agent.c
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
#include "lyst.h"
#include "platform.h"
#include "../shared/adm/adm_ltp_agent.h"
#include "../shared/utils/utils.h"
#include "../shared/primitives/report.h"
#include "../shared/primitives/blob.h"
#include "adm_ltp_agent_impl.h"
#include "rda.h"

#define _HAVE_LTP_AGENT_ADM_
#ifdef _HAVE_LTP_AGENT_ADM_

void adm_ltp_agent_init()
{
	adm_ltp_agent_setup();
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
	vec_idx_t nn_idx;
	VDB_ADD_NN(((ADM_ENUM_LTP_AGENT * 20) + ADM_EDD_OFFSET), &nn_idx);

	adm_add_edd(adm_build_reg_ari(0x82, nn_idx, ADM_LTP_AGENT_EDD_SPAN_REMOTE_ENGINE_NBR_ARI_NAME, NULL), adm_ltp_agent_get_span_remote_engine_nbr);
}

void adm_ltp_agent_init_vars()
{
}

void adm_ltp_agent_init_ctrldefs()
{
	vec_idx_t nn_idx;
	VDB_ADD_NN(((ADM_ENUM_LTP_AGENT * 20) + ADM_CTRL_OFFSET), &nn_idx);

	adm_add_ctrldef(adm_build_reg_ari(0x81, nn_idx, ADM_LTP_AGENT_CTRL_RESET_ARI_NAME, NULL), 0, ADM_ENUM_LTP_AGENT, adm_ltp_agent_ctrl_reset);
}

void adm_ltp_agent_init_constants()
{
}

void adm_ltp_agent_init_macros()
{
}

void adm_ltp_agent_init_metadata()
{
	vec_idx_t nn_idx;
	VDB_ADD_NN(((ADM_ENUM_LTP_AGENT * 20) + ADM_META_OFFSET), &nn_idx);

	/* Step 2: Register Metadata Information. */
	adm_add_edd(adm_build_reg_ari(0x80, nn_idx, ADM_LTP_AGENT_META_NAME_ARI_NAME, NULL), adm_ltp_agent_meta_name);
	adm_add_edd(adm_build_reg_ari(0x80, nn_idx, ADM_LTP_AGENT_META_NAMESPACE_ARI_NAME, NULL), adm_ltp_agent_meta_namespace);
	adm_add_edd(adm_build_reg_ari(0x80, nn_idx, ADM_LTP_AGENT_META_VERSION_ARI_NAME, NULL), adm_ltp_agent_meta_version);
	adm_add_edd(adm_build_reg_ari(0x80, nn_idx, ADM_LTP_AGENT_META_ORGANIZATION_ARI_NAME, NULL), adm_ltp_agent_meta_organization);
}

void adm_ltp_agent_init_ops()
{
}

void adm_ltp_agent_init_reports()
{
	ari_t *id;
	vec_idx_t nn_idx;
	rpttpl_t *def;
	VDB_ADD_NN(((ADM_ENUM_LTP_AGENT * 20) + ADM_RPTT_OFFSET), &nn_idx);

	id = adm_build_reg_ari(0x87, nn_idx, ADM_LTP_AGENT_RPT_ENDPOINTREPORT_ARI_NAME, NULL);
	def = rpttpl_create_id(id);

	rpttpl_add_item(def, adm_build_reg_ari(0x82, nn_idx, ADM_LTP_AGENT_RPT_ENDPOINTREPORT_ARI_NAME, NULL));

	adm_add_rpttpl(def);

}

#endif // _HAVE_LTP_AGENT_ADM_
