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

	id = adm_build_reg_ari(0x83, nn_idx, ADM_LTP_AGENT_CTRL_RESET_ARI_NAME, NULL);

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

	vec_idx_t nn_idx;
	VDB_ADD_NN(((ADM_ENUM_LTP_AGENT * 20) + ADM_RPTT_OFFSET), &nn_idx);

	ac_t entries;

	ac_insert(&entries, adm_build_reg_ari(0x80, nn_idx, ADM_LTP_AGENT_EDD_SPAN_REMOTE_ENGINE_NBR_ARI_NAME, NULL));

	//adm_add_rpttpl(adm_build_reg_ari(0x87, nn_idx, ADM_LTP_AGENT_RPT_ENDPOINTREPORT_ARI_NAME, NULL), entries);
/**

	Lyst rpt = NULL;
	uint32_t used= 0;
	rpt = lyst_create();
	lyst_insert_last(rpt,rpttpl_item_create(mid_from_value(ADM_LTP_AGENT_EDD_SPAN_REMOTE_ENGINE_NBR_MID), 0));
	lyst_insert_last(rpt,rpttpl_item_create(mid_from_value(ADM_LTP_AGENT_EDD_SPAN_CUR_EXPT_SESS_MID), 0));
	lyst_insert_last(rpt,rpttpl_item_create(mid_from_value(ADM_LTP_AGENT_EDD_SPAN_CUR_OUT_SEG_MID), 0));
	lyst_insert_last(rpt,rpttpl_item_create(mid_from_value(ADM_LTP_AGENT_EDD_SPAN_CUR_IMP_SESS_MID), 0));
	lyst_insert_last(rpt,rpttpl_item_create(mid_from_value(ADM_LTP_AGENT_EDD_SPAN_CUR_IN_SEG_MID), 0));
	lyst_insert_last(rpt,rpttpl_item_create(mid_from_value(ADM_LTP_AGENT_EDD_SPAN_RESET_TIME_MID), 0));
	lyst_insert_last(rpt,rpttpl_item_create(mid_from_value(ADM_LTP_AGENT_EDD_SPAN_OUT_SEG_Q_CNT_MID), 0));
	lyst_insert_last(rpt,rpttpl_item_create(mid_from_value(ADM_LTP_AGENT_EDD_SPAN_OUT_SEG_Q_BYTES_MID), 0));
	lyst_insert_last(rpt,rpttpl_item_create(mid_from_value(ADM_LTP_AGENT_EDD_SPAN_OUT_SEG_POP_CNT_MID), 0));
	lyst_insert_last(rpt,rpttpl_item_create(mid_from_value(ADM_LTP_AGENT_EDD_SPAN_OUT_SEG_POP_BYTES_MID), 0));
	lyst_insert_last(rpt,rpttpl_item_create(mid_from_value(ADM_LTP_AGENT_EDD_SPAN_OUT_CKPT_XMIT_CNT_MID), 0));
	lyst_insert_last(rpt,rpttpl_item_create(mid_from_value(ADM_LTP_AGENT_EDD_SPAN_OUT_POS_ACK_RX_CNT_MID), 0));
	lyst_insert_last(rpt,rpttpl_item_create(mid_from_value(ADM_LTP_AGENT_EDD_SPAN_OUT_NEG_ACK_RX_CNT_MID), 0));
	lyst_insert_last(rpt,rpttpl_item_create(mid_from_value(ADM_LTP_AGENT_EDD_SPAN_OUT_CANCEL_RX_CNT_MID), 0));
	lyst_insert_last(rpt,rpttpl_item_create(mid_from_value(ADM_LTP_AGENT_EDD_SPAN_OUT_CKPT_REXMIT_CNT_MID), 0));
	lyst_insert_last(rpt,rpttpl_item_create(mid_from_value(ADM_LTP_AGENT_EDD_SPAN_OUT_CANCEL_XMIT_CNT_MID), 0));
	lyst_insert_last(rpt,rpttpl_item_create(mid_from_value(ADM_LTP_AGENT_EDD_SPAN_OUT_COMPLETE_CNT_MID), 0));
	lyst_insert_last(rpt,rpttpl_item_create(mid_from_value(ADM_LTP_AGENT_EDD_SPAN_IN_SEG_RX_RED_CNT_MID), 0));
	lyst_insert_last(rpt,rpttpl_item_create(mid_from_value(ADM_LTP_AGENT_EDD_SPAN_IN_SEG_RX_RED_BYTES_MID), 0));
	lyst_insert_last(rpt,rpttpl_item_create(mid_from_value(ADM_LTP_AGENT_EDD_SPAN_IN_SEG_RX_GREEN_CNT_MID), 0));
	lyst_insert_last(rpt,rpttpl_item_create(mid_from_value(ADM_LTP_AGENT_EDD_SPAN_IN_SEG_RX_GREEN_BYTES_MID), 0));
	lyst_insert_last(rpt,rpttpl_item_create(mid_from_value(ADM_LTP_AGENT_EDD_SPAN_IN_SEG_RX_REDUNDANT_CNT_MID), 0));
	lyst_insert_last(rpt,rpttpl_item_create(mid_from_value(ADM_LTP_AGENT_EDD_SPAN_IN_SEG_RX_REDUNDANT_BYTES_MID), 0));
	lyst_insert_last(rpt,rpttpl_item_create(mid_from_value(ADM_LTP_AGENT_EDD_SPAN_IN_SEG_RX_MAL_CNT_MID), 0));
	lyst_insert_last(rpt,rpttpl_item_create(mid_from_value(ADM_LTP_AGENT_EDD_SPAN_IN_SEG_RX_MAL_BYTES_MID), 0));
	lyst_insert_last(rpt,rpttpl_item_create(mid_from_value(ADM_LTP_AGENT_EDD_SPAN_IN_SEG_RX_UNK_SENDER_CNT_MID), 0));
	lyst_insert_last(rpt,rpttpl_item_create(mid_from_value(ADM_LTP_AGENT_EDD_SPAN_IN_SEG_RX_UNK_SENDER_BYTES_MID), 0));
	lyst_insert_last(rpt,rpttpl_item_create(mid_from_value(ADM_LTP_AGENT_EDD_SPAN_IN_SEG_RX_UNK_CLIENT_CNT_MID), 0));
	lyst_insert_last(rpt,rpttpl_item_create(mid_from_value(ADM_LTP_AGENT_EDD_SPAN_IN_SEG_RX_UNK_CLIENT_BYTES_MID), 0));
	lyst_insert_last(rpt,rpttpl_item_create(mid_from_value(ADM_LTP_AGENT_EDD_SPAN_IN_SEG_STRAY_CNT_MID), 0));
	lyst_insert_last(rpt,rpttpl_item_create(mid_from_value(ADM_LTP_AGENT_EDD_SPAN_IN_SEG_STRAY_BYTES_MID), 0));
	lyst_insert_last(rpt,rpttpl_item_create(mid_from_value(ADM_LTP_AGENT_EDD_SPAN_IN_SEG_MISCOLOR_CNT_MID), 0));
	lyst_insert_last(rpt,rpttpl_item_create(mid_from_value(ADM_LTP_AGENT_EDD_SPAN_IN_SEG_MISCOLOR_BYTES_MID), 0));
	lyst_insert_last(rpt,rpttpl_item_create(mid_from_value(ADM_LTP_AGENT_EDD_SPAN_IN_SEG_CLOSED_CNT_MID), 0));
	lyst_insert_last(rpt,rpttpl_item_create(mid_from_value(ADM_LTP_AGENT_EDD_SPAN_IN_SEG_CLOSED_BYTES_MID), 0));
	lyst_insert_last(rpt,rpttpl_item_create(mid_from_value(ADM_LTP_AGENT_EDD_SPAN_IN_CKPT_RX_CNT_MID), 0));
	lyst_insert_last(rpt,rpttpl_item_create(mid_from_value(ADM_LTP_AGENT_EDD_SPAN_IN_POS_ACK_TX_CNT_MID), 0));
	lyst_insert_last(rpt,rpttpl_item_create(mid_from_value(ADM_LTP_AGENT_EDD_SPAN_IN_NEG_ACK_TX_CNT_MID), 0));
	lyst_insert_last(rpt,rpttpl_item_create(mid_from_value(ADM_LTP_AGENT_EDD_SPAN_IN_CANCEL_TX_CNT_MID), 0));
	lyst_insert_last(rpt,rpttpl_item_create(mid_from_value(ADM_LTP_AGENT_EDD_SPAN_IN_ACK_RETX_CNT_MID), 0));
	lyst_insert_last(rpt,rpttpl_item_create(mid_from_value(ADM_LTP_AGENT_EDD_SPAN_IN_CANCEL_RX_CNT_MID), 0));
	lyst_insert_last(rpt,rpttpl_item_create(mid_from_value(ADM_LTP_AGENT_EDD_SPAN_IN_COMPLETE_CNT_MID), 0));

	adm_add_rpttpl(mid_from_value(ADM_LTP_AGENT_RPT_ENDPOINTREPORT_MID), rpt);

	names_add_name("ENDPOINTREPORT", "This is all known endpoint information", ADM_LTP_AGENT, ADM_LTP_AGENT_RPT_ENDPOINTREPORT_MID);
**/
}

#endif // _HAVE_LTP_AGENT_ADM_
