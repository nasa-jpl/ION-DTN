/*****************************************************************************
 **
 ** File Name: adm_sbsp.c
 **
 ** Description: This implements the public portions of a AMP SBSP ADM.
 **
 ** Notes:
 **
 ** Assumptions:
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **            E. Birrane     Initial Implementation (Secure DTN - NASA: NNX14CS58P)
 **  08/21/16  E. Birrane     Updated to Agent ADM v0.2 (Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/


#include "ion.h"
#include "platform.h"


#include "../adm/adm_sbsp.h"
#include "../utils/utils.h"
#include "../primitives/def.h"
#include "../primitives/nn.h"
#include "../primitives/report.h"
#include "../primitives/blob.h"

#ifdef AGENT_ROLE
#include "../../agent/adm_sbsp_impl.h"
#include "../../agent/rda.h"
#else
#include "../../mgr/nm_mgr_names.h"
#include "../../mgr/nm_mgr_ui.h"
#endif

#define _HAVE_SBSP_ADM_

#ifdef _HAVE_SBSP_ADM_


void adm_sbsp_init()
{
	adm_sbsp_init_atomic();
	adm_sbsp_init_computed();
	adm_sbsp_init_controls();
	adm_sbsp_init_literals();
	adm_sbsp_init_macros();
	adm_sbsp_init_metadata();
	adm_sbsp_init_ops();
	adm_sbsp_init_reports();
}



void adm_sbsp_init_atomic()
{

#ifdef AGENT_ROLE
	adm_add_datadef(ADM_SBSP_AD_TOT_GOOD_TX_BCB_BLKS,  AMP_TYPE_UVAST, 0, adm_sbsp_get_tot_good_tx_bcb_blks,  NULL, NULL);
	adm_add_datadef(ADM_SBSP_AD_TOT_FAIL_TX_BCB_BLKS,  AMP_TYPE_UVAST, 0, adm_sbsp_get_tot_fail_tx_bcb_blks, NULL, NULL);
	adm_add_datadef(ADM_SBSP_AD_TOT_GOOD_RX_BCB_BLKS,  AMP_TYPE_UVAST, 0, adm_sbsp_get_tot_good_rx_bcb_blks,  NULL, NULL);
	adm_add_datadef(ADM_SBSP_AD_TOT_FAIL_RX_BCB_BLKS,  AMP_TYPE_UVAST, 0, adm_sbsp_get_tot_fail_rx_bcb_blks,  NULL, NULL);
	adm_add_datadef(ADM_SBSP_AD_TOT_MISSING_BCB_BLKS,  AMP_TYPE_UVAST, 0, adm_sbsp_get_tot_missing_bcb_blks,  NULL, NULL);
	adm_add_datadef(ADM_SBSP_AD_TOT_FORWARD_BCB_BLKS,  AMP_TYPE_UVAST, 0, adm_sbsp_get_tot_forward_bcb_blks,  NULL, NULL);
	adm_add_datadef(ADM_SBSP_AD_TOT_GOOD_TX_BCB_BYTES, AMP_TYPE_UVAST, 0, adm_sbsp_get_tot_good_tx_bcb_bytes,  NULL, NULL);
	adm_add_datadef(ADM_SBSP_AD_TOT_FAIL_TX_BCB_BYTES, AMP_TYPE_UVAST, 0, adm_sbsp_get_tot_fail_tx_bcb_bytes, NULL, NULL);
	adm_add_datadef(ADM_SBSP_AD_TOT_GOOD_RX_BCB_BYTES, AMP_TYPE_UVAST, 0, adm_sbsp_get_tot_good_rx_bcb_bytes,  NULL, NULL);
	adm_add_datadef(ADM_SBSP_AD_TOT_FAIL_RX_BCB_BYTES, AMP_TYPE_UVAST, 0, adm_sbsp_get_tot_fail_rx_bcb_bytes,  NULL, NULL);
	adm_add_datadef(ADM_SBSP_AD_TOT_MISSING_BCB_BYTES, AMP_TYPE_UVAST, 0, adm_sbsp_get_tot_missing_bcb_bytes, NULL, NULL);
	adm_add_datadef(ADM_SBSP_AD_TOT_FORWARD_BCB_BYTES, AMP_TYPE_UVAST, 0, adm_sbsp_get_tot_forward_bcb_bytes, NULL, NULL);

	adm_add_datadef(ADM_SBSP_AD_TOT_GOOD_TX_BIB_BLKS, AMP_TYPE_UVAST, 0, adm_sbsp_get_tot_good_tx_bib_blks, NULL, NULL);
	adm_add_datadef(ADM_SBSP_AD_TOT_FAIL_TX_BIB_BLKS, AMP_TYPE_UVAST, 0, adm_sbsp_get_tot_fail_tx_bib_blks, NULL, NULL);
	adm_add_datadef(ADM_SBSP_AD_TOT_GOOD_RX_BIB_BLKS, AMP_TYPE_UVAST, 0, adm_sbsp_get_tot_good_rx_bib_blks, NULL, NULL);
	adm_add_datadef(ADM_SBSP_AD_TOT_FAIL_RX_BIB_BLKS, AMP_TYPE_UVAST, 0, adm_sbsp_get_tot_fail_rx_bib_blks, NULL, NULL);
	adm_add_datadef(ADM_SBSP_AD_TOT_MISSING_BIB_BLKS, AMP_TYPE_UVAST, 0, adm_sbsp_get_tot_missing_bib_blks, NULL, NULL);
	adm_add_datadef(ADM_SBSP_AD_TOT_FORWARD_BIB_BLKS, AMP_TYPE_UVAST, 0, adm_sbsp_get_tot_forward_bib_blks, NULL, NULL);
	adm_add_datadef(ADM_SBSP_AD_TOT_GOOD_TX_BIB_BYTES, AMP_TYPE_UVAST, 0, adm_sbsp_get_tot_good_tx_bib_bytes, NULL, NULL);
	adm_add_datadef(ADM_SBSP_AD_TOT_FAIL_TX_BIB_BYTES, AMP_TYPE_UVAST, 0, adm_sbsp_get_tot_fail_tx_bib_bytes, NULL, NULL);
	adm_add_datadef(ADM_SBSP_AD_TOT_GOOD_RX_BIB_BYTES, AMP_TYPE_UVAST, 0, adm_sbsp_get_tot_good_rx_bib_bytes, NULL, NULL);
	adm_add_datadef(ADM_SBSP_AD_TOT_FAIL_RX_BIB_BYTES, AMP_TYPE_UVAST, 0, adm_sbsp_get_tot_fail_rx_bib_bytes, NULL, NULL);
	adm_add_datadef(ADM_SBSP_AD_TOT_MISSING_BIB_BYTES, AMP_TYPE_UVAST, 0, adm_sbsp_get_tot_missing_bib_bytes, NULL, NULL);
	adm_add_datadef(ADM_SBSP_AD_TOT_FORWARD_BIB_BYTES, AMP_TYPE_UVAST, 0, adm_sbsp_get_tot_forward_bib_bytes, NULL, NULL);
	adm_add_datadef(ADM_SBSP_AD_LAST_UPDATE, AMP_TYPE_UINT, 0, adm_sbsp_get_last_update, NULL, NULL);
	adm_add_datadef(ADM_SBSP_AD_NUM_KEYS, AMP_TYPE_UINT, 0, adm_sbsp_get_num_keys, NULL, NULL);
	adm_add_datadef(ADM_SBSP_AD_KEYS, AMP_TYPE_STRING, 0, adm_sbsp_get_keys, NULL, NULL);
	adm_add_datadef(ADM_SBSP_AD_CIPHS, AMP_TYPE_STRING, 0, adm_sbsp_get_ciphs, NULL, NULL);
	adm_add_datadef(ADM_SBSP_AD_SRCS, AMP_TYPE_STRING, 0, adm_sbsp_get_srcs, NULL, NULL);
	adm_add_datadef(ADM_SBSP_AD_SRC_GOOD_TX_BCB_BLKS, AMP_TYPE_UVAST, 0, adm_sbsp_get_src_good_tx_bcb_blks, NULL, NULL);
	adm_add_datadef(ADM_SBSP_AD_SRC_FAIL_TX_BCB_BLKS, AMP_TYPE_UVAST, 0, adm_sbsp_get_src_fail_tx_bcb_blks, NULL, NULL);
	adm_add_datadef(ADM_SBSP_AD_SRC_GOOD_RX_BCB_BLKS, AMP_TYPE_UVAST, 0, adm_sbsp_get_src_good_rx_bcb_blks, NULL, NULL);
	adm_add_datadef(ADM_SBSP_AD_SRC_FAIL_RX_BCB_BLKS, AMP_TYPE_UVAST, 0, adm_sbsp_get_src_fail_rx_bcb_blks, NULL, NULL);
	adm_add_datadef(ADM_SBSP_AD_SRC_MISSING_BCB_BLKS, AMP_TYPE_UVAST, 0, adm_sbsp_get_src_missing_bcb_blks, NULL, NULL);
	adm_add_datadef(ADM_SBSP_AD_SRC_FORWARD_BCB_BLKS, AMP_TYPE_UVAST, 0, adm_sbsp_get_src_forward_bcb_blks, NULL, NULL);

	adm_add_datadef(ADM_SBSP_AD_SRC_GOOD_TX_BCB_BYTES, AMP_TYPE_UVAST, 0, adm_sbsp_get_src_good_tx_bcb_bytes, NULL, NULL);
	adm_add_datadef(ADM_SBSP_AD_SRC_FAIL_TX_BCB_BYTES, AMP_TYPE_UVAST, 0, adm_sbsp_get_src_fail_tx_bcb_bytes, NULL, NULL);
	adm_add_datadef(ADM_SBSP_AD_SRC_GOOD_RX_BCB_BYTES, AMP_TYPE_UVAST, 0, adm_sbsp_get_src_good_rx_bcb_bytes, NULL, NULL);
	adm_add_datadef(ADM_SBSP_AD_SRC_FAIL_RX_BCB_BYTES, AMP_TYPE_UVAST, 0, adm_sbsp_get_src_fail_rx_bcb_bytes, NULL, NULL);
	adm_add_datadef(ADM_SBSP_AD_SRC_MISSING_BCB_BYTES, AMP_TYPE_UVAST, 0, adm_sbsp_get_src_missing_bcb_bytes, NULL, NULL);
	adm_add_datadef(ADM_SBSP_AD_SRC_FORWARD_BCB_BYTES, AMP_TYPE_UVAST, 0, adm_sbsp_get_src_forward_bcb_bytes, NULL, NULL);
	adm_add_datadef(ADM_SBSP_AD_SRC_GOOD_TX_BIB_BLKS, AMP_TYPE_UVAST, 0, adm_sbsp_get_src_good_tx_bib_blks, NULL, NULL);
	adm_add_datadef(ADM_SBSP_AD_SRC_FAIL_TX_BIB_BLKS, AMP_TYPE_UVAST, 0, adm_sbsp_get_src_fail_tx_bib_blks, NULL, NULL);
	adm_add_datadef(ADM_SBSP_AD_SRC_GOOD_RX_BIB_BLKS, AMP_TYPE_UVAST, 0, adm_sbsp_get_src_good_rx_bib_blks, NULL, NULL);
	adm_add_datadef(ADM_SBSP_AD_SRC_FAIL_RX_BIB_BLKS, AMP_TYPE_UVAST, 0, adm_sbsp_get_src_fail_rx_bib_blks, NULL, NULL);
	adm_add_datadef(ADM_SBSP_AD_SRC_MISSING_BIB_BLKS, AMP_TYPE_UVAST, 0, adm_sbsp_get_src_missing_bib_blks, NULL, NULL);
	adm_add_datadef(ADM_SBSP_AD_SRC_FORWARD_BIB_BLKS, AMP_TYPE_UVAST, 0, adm_sbsp_get_src_forward_bib_blks, NULL, NULL);
	adm_add_datadef(ADM_SBSP_AD_SRC_GOOD_TX_BIB_BYTES, AMP_TYPE_UVAST, 0, adm_sbsp_get_src_good_tx_bib_bytes, NULL, NULL);
	adm_add_datadef(ADM_SBSP_AD_SRC_FAIL_TX_BIB_BYTES, AMP_TYPE_UVAST, 0, adm_sbsp_get_src_fail_tx_bib_bytes, NULL, NULL);
	adm_add_datadef(ADM_SBSP_AD_SRC_GOOD_RX_BIB_BYTES, AMP_TYPE_UVAST, 0, adm_sbsp_get_src_good_rx_bib_bytes, NULL, NULL);
	adm_add_datadef(ADM_SBSP_AD_SRC_FAIL_RX_BIB_BYTES, AMP_TYPE_UVAST, 0, adm_sbsp_get_src_fail_rx_bib_bytes, NULL, NULL);
	adm_add_datadef(ADM_SBSP_AD_SRC_MISSING_BIB_BYTES, AMP_TYPE_UVAST, 0, adm_sbsp_get_src_missing_bib_bytes, NULL, NULL);
	adm_add_datadef(ADM_SBSP_AD_SRC_FORWARD_BIB_BYTES, AMP_TYPE_UVAST, 0, adm_sbsp_get_src_forward_bib_bytes, NULL, NULL);

	adm_add_datadef(ADM_SBSP_AD_SRC_LAST_UPDATE, AMP_TYPE_UINT, 0, adm_sbsp_get_src_last_update, NULL, NULL);

	adm_add_datadef(ADM_SBSP_AD_LAST_RESET, AMP_TYPE_TS, 0, adm_sbsp_get_last_reset, NULL, NULL);

	#else

	adm_add_datadef(ADM_SBSP_AD_TOT_GOOD_TX_BCB_BLKS,  AMP_TYPE_UVAST, 0, NULL,  NULL, NULL);
	names_add_name("ADM_SBSP_AD_TOT_GOOD_TX_BCB_BLKS", "Total BCB Block Tx Success", ADM_SBSP, ADM_SBSP_AD_TOT_GOOD_TX_BCB_BLKS);

	adm_add_datadef(ADM_SBSP_AD_TOT_FAIL_TX_BCB_BLKS,  AMP_TYPE_UVAST, 0, NULL, NULL, NULL);
	names_add_name("ADM_SBSP_AD_TOT_FAIL_TX_BCB_BLKS", "Total BCB Block Tx Failures", ADM_SBSP, ADM_SBSP_AD_TOT_FAIL_TX_BCB_BLKS);

	adm_add_datadef(ADM_SBSP_AD_TOT_GOOD_RX_BCB_BLKS,  AMP_TYPE_UVAST, 0, NULL,  NULL, NULL);
	names_add_name("ADM_SBSP_AD_TOT_GOOD_RX_BCB_BLKS", "Total BCB Block Rx Success", ADM_SBSP, ADM_SBSP_AD_TOT_GOOD_RX_BCB_BLKS);

	adm_add_datadef(ADM_SBSP_AD_TOT_FAIL_RX_BCB_BLKS,  AMP_TYPE_UVAST, 0, NULL,  NULL, NULL);
	names_add_name("ADM_SBSP_AD_TOT_FAIL_RX_BCB_BLKS", "Total BCB Block Rx Failure", ADM_SBSP, ADM_SBSP_AD_TOT_FAIL_RX_BCB_BLKS);

	adm_add_datadef(ADM_SBSP_AD_TOT_MISSING_BCB_BLKS,  AMP_TYPE_UVAST, 0, NULL,  NULL, NULL);
	names_add_name("ADM_SBSP_AD_TOT_MISSING_BCB_BLKS", "Total BCB Blocks Missing", ADM_SBSP, ADM_SBSP_AD_TOT_MISSING_BCB_BLKS);

	adm_add_datadef(ADM_SBSP_AD_TOT_FORWARD_BCB_BLKS,  AMP_TYPE_UVAST, 0, NULL,  NULL, NULL);
	names_add_name("ADM_SBSP_AD_TOT_FORWARD_BCB_BLKS", "Total BCB Blocks Forwarded", ADM_SBSP, ADM_SBSP_AD_TOT_FORWARD_BCB_BLKS);

	adm_add_datadef(ADM_SBSP_AD_TOT_GOOD_TX_BCB_BYTES, AMP_TYPE_UVAST, 0, NULL,  NULL, NULL);
	names_add_name("ADM_SBSP_AD_TOT_GOOD_TX_BCB_BYTES", "Total BCB Bytes Tx Success", ADM_SBSP, ADM_SBSP_AD_TOT_GOOD_TX_BCB_BYTES);

	adm_add_datadef(ADM_SBSP_AD_TOT_FAIL_TX_BCB_BYTES, AMP_TYPE_UVAST, 0, NULL, NULL, NULL);
	names_add_name("ADM_SBSP_AD_TOT_FAIL_TX_BCB_BYTES", "Total BCB Bytes Tx Failure", ADM_SBSP, ADM_SBSP_AD_TOT_FAIL_TX_BCB_BYTES);

	adm_add_datadef(ADM_SBSP_AD_TOT_GOOD_RX_BCB_BYTES, AMP_TYPE_UVAST, 0, NULL,  NULL, NULL);
	names_add_name("ADM_SBSP_AD_TOT_GOOD_RX_BCB_BYTES", "Total BCB Bytes Rx Success", ADM_SBSP, ADM_SBSP_AD_TOT_GOOD_RX_BCB_BYTES);

	adm_add_datadef(ADM_SBSP_AD_TOT_FAIL_RX_BCB_BYTES, AMP_TYPE_UVAST, 0, NULL,  NULL, NULL);
	names_add_name("ADM_SBSP_AD_TOT_FAIL_RX_BCB_BYTES", "Total BCB Bytes Rx Failure", ADM_SBSP, ADM_SBSP_AD_TOT_FAIL_RX_BCB_BYTES);

	adm_add_datadef(ADM_SBSP_AD_TOT_MISSING_BCB_BYTES, AMP_TYPE_UVAST, 0, NULL, NULL, NULL);
	names_add_name("ADM_SBSP_AD_TOT_MISSING_BCB_BYTES", "Total BCB Bytes Missing", ADM_SBSP, ADM_SBSP_AD_TOT_MISSING_BCB_BYTES);

	adm_add_datadef(ADM_SBSP_AD_TOT_FORWARD_BCB_BYTES, AMP_TYPE_UVAST, 0, NULL, NULL, NULL);
	names_add_name("ADM_SBSP_AD_TOT_FORWARD_BCB_BYTES", "Total BCB Bytes Forwarded", ADM_SBSP, ADM_SBSP_AD_TOT_FORWARD_BCB_BYTES);

	adm_add_datadef(ADM_SBSP_AD_TOT_GOOD_TX_BIB_BLKS, AMP_TYPE_UVAST, 0, NULL, NULL, NULL);
	names_add_name("ADM_SBSP_AD_TOT_GOOD_TX_BIB_BLKS", "Total BIB Block Tx Success", ADM_SBSP, ADM_SBSP_AD_TOT_GOOD_TX_BIB_BLKS);

	adm_add_datadef(ADM_SBSP_AD_TOT_FAIL_TX_BIB_BLKS, AMP_TYPE_UVAST, 0, NULL, NULL, NULL);
	names_add_name("ADM_SBSP_AD_TOT_FAIL_TX_BIB_BLKS", "Total BIB Block Tx Failure", ADM_SBSP, ADM_SBSP_AD_TOT_FAIL_TX_BIB_BLKS);

	adm_add_datadef(ADM_SBSP_AD_TOT_GOOD_RX_BIB_BLKS, AMP_TYPE_UVAST, 0, NULL, NULL, NULL);
	names_add_name("ADM_SBSP_AD_TOT_GOOD_RX_BIB_BLKS", "Total BIB Block Rx Success", ADM_SBSP, ADM_SBSP_AD_TOT_GOOD_RX_BIB_BLKS);

	adm_add_datadef(ADM_SBSP_AD_TOT_FAIL_RX_BIB_BLKS, AMP_TYPE_UVAST, 0, NULL, NULL, NULL);
	names_add_name("ADM_SBSP_AD_TOT_FAIL_RX_BIB_BLKS", "Total BIB Block Rx Failure", ADM_SBSP, ADM_SBSP_AD_TOT_FAIL_RX_BIB_BLKS);

	adm_add_datadef(ADM_SBSP_AD_TOT_MISSING_BIB_BLKS, AMP_TYPE_UVAST, 0, NULL, NULL, NULL);
	names_add_name("ADM_SBSP_AD_TOT_MISSING_BIB_BLKS", "Total BIB Blocks Missing", ADM_SBSP, ADM_SBSP_AD_TOT_MISSING_BIB_BLKS);

	adm_add_datadef(ADM_SBSP_AD_TOT_FORWARD_BIB_BLKS, AMP_TYPE_UVAST, 0, NULL, NULL, NULL);
	names_add_name("ADM_SBSP_AD_TOT_FORWARD_BIB_BLKS", "Total BIB Blocks Forwarded", ADM_SBSP, ADM_SBSP_AD_TOT_FORWARD_BIB_BLKS);

	adm_add_datadef(ADM_SBSP_AD_TOT_GOOD_TX_BIB_BYTES, AMP_TYPE_UVAST, 0, NULL, NULL, NULL);
	names_add_name("ADM_SBSP_AD_TOT_GOOD_TX_BIB_BYTES", "Total BIB Bytes Tx Success", ADM_SBSP, ADM_SBSP_AD_TOT_GOOD_TX_BIB_BYTES);

	adm_add_datadef(ADM_SBSP_AD_TOT_FAIL_TX_BIB_BYTES, AMP_TYPE_UVAST, 0, NULL, NULL, NULL);
	names_add_name("ADM_SBSP_AD_TOT_FAIL_TX_BIB_BYTES", "Total BIB Bytes Tx Failure", ADM_SBSP, ADM_SBSP_AD_TOT_FAIL_TX_BIB_BYTES);

	adm_add_datadef(ADM_SBSP_AD_TOT_GOOD_RX_BIB_BYTES, AMP_TYPE_UVAST, 0, NULL, NULL, NULL);
	names_add_name("ADM_SBSP_AD_TOT_GOOD_RX_BIB_BYTES", "Total BIB Bytes Rx Success", ADM_SBSP, ADM_SBSP_AD_TOT_GOOD_RX_BIB_BYTES);

	adm_add_datadef(ADM_SBSP_AD_TOT_FAIL_RX_BIB_BYTES, AMP_TYPE_UVAST, 0, NULL, NULL, NULL);
	names_add_name("ADM_SBSP_AD_TOT_FAIL_RX_BIB_BYTES", "Total BIB Bytes Rx Failure", ADM_SBSP, ADM_SBSP_AD_TOT_FAIL_RX_BIB_BYTES);

	adm_add_datadef(ADM_SBSP_AD_TOT_MISSING_BIB_BYTES, AMP_TYPE_UVAST, 0, NULL, NULL, NULL);
	names_add_name("ADM_SBSP_AD_TOT_MISSING_BIB_BYTES", "Total BIB Bytes Missing", ADM_SBSP, ADM_SBSP_AD_TOT_MISSING_BIB_BYTES);

	adm_add_datadef(ADM_SBSP_AD_TOT_FORWARD_BIB_BYTES, AMP_TYPE_UVAST, 0, NULL, NULL, NULL);
	names_add_name("ADM_SBSP_AD_TOT_FORWARD_BIB_BYTES", "Total BIB Bytes Forwarded", ADM_SBSP, ADM_SBSP_AD_TOT_FORWARD_BIB_BYTES);

	adm_add_datadef(ADM_SBSP_AD_LAST_UPDATE, AMP_TYPE_TS, 0, NULL, NULL, NULL);
	names_add_name("ADM_SBSP_AD_LAST_UPDATE", "SBSP Last Update", ADM_SBSP, ADM_SBSP_AD_LAST_UPDATE);

	adm_add_datadef(ADM_SBSP_AD_NUM_KEYS, AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("ADM_SBSP_AD_NUM_KEYS", "SBSP # Keys", ADM_SBSP, ADM_SBSP_AD_NUM_KEYS);

	adm_add_datadef(ADM_SBSP_AD_KEYS, AMP_TYPE_STRING, 0, NULL, NULL, NULL);
	names_add_name("ADM_SBSP_AD_KEYS", "Key Names", ADM_SBSP, ADM_SBSP_AD_KEYS);

	adm_add_datadef(ADM_SBSP_AD_CIPHS, AMP_TYPE_STRING, 0, NULL, NULL, NULL);
	names_add_name("ADM_SBSP_AD_CIPHS", "SBSP Ciphersuites", ADM_SBSP, ADM_SBSP_AD_CIPHS);

	adm_add_datadef(ADM_SBSP_AD_SRCS, AMP_TYPE_STRING, 0, NULL, NULL, NULL);
	names_add_name("ADM_SBSP_AD_SRCS", "SBSP Sources", ADM_SBSP, ADM_SBSP_AD_SRCS);



	adm_add_datadef(ADM_SBSP_AD_SRC_GOOD_TX_BCB_BLKS, AMP_TYPE_UVAST, 0, NULL, NULL, NULL);
	names_add_name("ADM_SBSP_AD_SRC_GOOD_TX_BCB_BLKS", "Src BCB Block Tx Success", ADM_SBSP, ADM_SBSP_AD_SRC_GOOD_TX_BCB_BLKS);
	UI_ADD_PARMSPEC_1(ADM_SBSP_AD_SRC_GOOD_TX_BCB_BLKS, "Src Name", AMP_TYPE_STRING);

	adm_add_datadef(ADM_SBSP_AD_SRC_FAIL_TX_BCB_BLKS, AMP_TYPE_UVAST, 0, NULL, NULL, NULL);
	names_add_name("ADM_SBSP_AD_SRC_FAIL_TX_BCB_BLKS", "Src BCB Block Tx Failures", ADM_SBSP, ADM_SBSP_AD_SRC_FAIL_TX_BCB_BLKS);
	UI_ADD_PARMSPEC_1(ADM_SBSP_AD_SRC_FAIL_TX_BCB_BLKS, "Src Name", AMP_TYPE_STRING);

	adm_add_datadef(ADM_SBSP_AD_SRC_GOOD_RX_BCB_BLKS, AMP_TYPE_UVAST, 0, NULL, NULL, NULL);
	names_add_name("ADM_SBSP_AD_SRC_GOOD_RX_BCB_BLKS", "Src BCB Block Rx Success", ADM_SBSP, ADM_SBSP_AD_SRC_GOOD_RX_BCB_BLKS);
	UI_ADD_PARMSPEC_1(ADM_SBSP_AD_SRC_GOOD_RX_BCB_BLKS, "Src Name", AMP_TYPE_STRING);

	adm_add_datadef(ADM_SBSP_AD_SRC_FAIL_RX_BCB_BLKS, AMP_TYPE_UVAST, 0, NULL, NULL, NULL);
	names_add_name("ADM_SBSP_AD_SRC_FAIL_RX_BCB_BLKS", "Src BCB Block Rx Failures", ADM_SBSP, ADM_SBSP_AD_SRC_FAIL_RX_BCB_BLKS);
	UI_ADD_PARMSPEC_1(ADM_SBSP_AD_SRC_FAIL_RX_BCB_BLKS, "Src Name", AMP_TYPE_STRING);

	adm_add_datadef(ADM_SBSP_AD_SRC_MISSING_BCB_BLKS, AMP_TYPE_UVAST, 0, NULL, NULL, NULL);
	names_add_name("ADM_SBSP_AD_SRC_MISSING_BCB_BLKS", "Src BCB Block Missing", ADM_SBSP, ADM_SBSP_AD_SRC_MISSING_BCB_BLKS);
	UI_ADD_PARMSPEC_1(ADM_SBSP_AD_SRC_MISSING_BCB_BLKS, "Src Name", AMP_TYPE_STRING);

	adm_add_datadef(ADM_SBSP_AD_SRC_FORWARD_BCB_BLKS, AMP_TYPE_UVAST, 0, NULL, NULL, NULL);
	names_add_name("ADM_SBSP_AD_SRC_FORWARD_BCB_BLKS", "Src BCB Block Forwarded", ADM_SBSP, ADM_SBSP_AD_SRC_FORWARD_BCB_BLKS);
	UI_ADD_PARMSPEC_1(ADM_SBSP_AD_SRC_FORWARD_BCB_BLKS, "Src Name", AMP_TYPE_STRING);

	adm_add_datadef(ADM_SBSP_AD_SRC_GOOD_TX_BCB_BYTES, AMP_TYPE_UVAST, 0, NULL, NULL, NULL);
	names_add_name("ADM_SBSP_AD_SRC_GOOD_TX_BCB_BYTES", "Src BCB Bytes Tx Success", ADM_SBSP, ADM_SBSP_AD_SRC_GOOD_TX_BCB_BYTES);
	UI_ADD_PARMSPEC_1(ADM_SBSP_AD_SRC_GOOD_TX_BCB_BYTES, "Src Name", AMP_TYPE_STRING);

	adm_add_datadef(ADM_SBSP_AD_SRC_FAIL_TX_BCB_BYTES, AMP_TYPE_UVAST, 0, NULL, NULL, NULL);
	names_add_name("ADM_SBSP_AD_SRC_FAIL_TX_BCB_BYTES", "Src BCB Bytes Tx Failure", ADM_SBSP, ADM_SBSP_AD_SRC_FAIL_TX_BCB_BYTES);
	UI_ADD_PARMSPEC_1(ADM_SBSP_AD_SRC_FAIL_TX_BCB_BYTES, "Src Name", AMP_TYPE_STRING);

	adm_add_datadef(ADM_SBSP_AD_SRC_GOOD_RX_BCB_BYTES, AMP_TYPE_UVAST, 0, NULL, NULL, NULL);
	names_add_name("ADM_SBSP_AD_SRC_GOOD_RX_BCB_BYTES", "Src BCB Bytes Rx Success", ADM_SBSP, ADM_SBSP_AD_SRC_GOOD_RX_BCB_BYTES);
	UI_ADD_PARMSPEC_1(ADM_SBSP_AD_SRC_GOOD_RX_BCB_BYTES, "Src Name", AMP_TYPE_STRING);

	adm_add_datadef(ADM_SBSP_AD_SRC_FAIL_RX_BCB_BYTES, AMP_TYPE_UVAST, 0, NULL, NULL, NULL);
	names_add_name("ADM_SBSP_AD_SRC_FAIL_RX_BCB_BYTES", "Src BCB Bytes Rx Failure", ADM_SBSP, ADM_SBSP_AD_SRC_FAIL_RX_BCB_BYTES);
	UI_ADD_PARMSPEC_1(ADM_SBSP_AD_SRC_FAIL_RX_BCB_BYTES, "Src Name", AMP_TYPE_STRING);

	adm_add_datadef(ADM_SBSP_AD_SRC_MISSING_BCB_BYTES, AMP_TYPE_UVAST, 0, NULL, NULL, NULL);
	names_add_name("ADM_SBSP_AD_SRC_MISSING_BCB_BYTES", "Src BCB Bytes Missing", ADM_SBSP, ADM_SBSP_AD_SRC_MISSING_BCB_BYTES);
	UI_ADD_PARMSPEC_1(ADM_SBSP_AD_SRC_MISSING_BCB_BYTES, "Src Name", AMP_TYPE_STRING);

	adm_add_datadef(ADM_SBSP_AD_SRC_FORWARD_BCB_BYTES, AMP_TYPE_UVAST, 0, NULL, NULL, NULL);
	names_add_name("ADM_SBSP_AD_SRC_FORWARD_BCB_BYTES", "Src BCB Bytes Forwarded", ADM_SBSP, ADM_SBSP_AD_SRC_FORWARD_BCB_BYTES);
	UI_ADD_PARMSPEC_1(ADM_SBSP_AD_SRC_FORWARD_BCB_BYTES, "Src Name", AMP_TYPE_STRING);

	adm_add_datadef(ADM_SBSP_AD_SRC_GOOD_TX_BIB_BLKS, AMP_TYPE_UVAST, 0, NULL, NULL, NULL);
	names_add_name("ADM_SBSP_AD_SRC_GOOD_TX_BIB_BLKS", "Src BIB Block Tx Success", ADM_SBSP, ADM_SBSP_AD_SRC_GOOD_TX_BIB_BLKS);
	UI_ADD_PARMSPEC_1(ADM_SBSP_AD_SRC_GOOD_TX_BIB_BLKS, "Src Name", AMP_TYPE_STRING);

	adm_add_datadef(ADM_SBSP_AD_SRC_FAIL_TX_BIB_BLKS, AMP_TYPE_UVAST, 0, NULL, NULL, NULL);
	names_add_name("ADM_SBSP_AD_SRC_FAIL_TX_BIB_BLKS", "Src BIB Block Tx Failure", ADM_SBSP, ADM_SBSP_AD_SRC_FAIL_TX_BIB_BLKS);
	UI_ADD_PARMSPEC_1(ADM_SBSP_AD_SRC_FAIL_TX_BIB_BLKS, "Src Name", AMP_TYPE_STRING);

	adm_add_datadef(ADM_SBSP_AD_SRC_GOOD_RX_BIB_BLKS, AMP_TYPE_UVAST, 0, NULL, NULL, NULL);
	names_add_name("ADM_SBSP_AD_SRC_GOOD_RX_BIB_BLKS", "Src BIB Block Rx Success", ADM_SBSP, ADM_SBSP_AD_SRC_GOOD_RX_BIB_BLKS);
	UI_ADD_PARMSPEC_1(ADM_SBSP_AD_SRC_GOOD_RX_BIB_BLKS, "Src Name", AMP_TYPE_STRING);

	adm_add_datadef(ADM_SBSP_AD_SRC_FAIL_RX_BIB_BLKS, AMP_TYPE_UVAST, 0, NULL, NULL, NULL);
	names_add_name("ADM_SBSP_AD_SRC_FAIL_RX_BIB_BLKS", "Src BIB Block Rx Failure", ADM_SBSP, ADM_SBSP_AD_SRC_FAIL_RX_BIB_BLKS);
	UI_ADD_PARMSPEC_1(ADM_SBSP_AD_SRC_FAIL_RX_BIB_BLKS, "Src Name", AMP_TYPE_STRING);

	adm_add_datadef(ADM_SBSP_AD_SRC_MISSING_BIB_BLKS, AMP_TYPE_UVAST, 0, NULL, NULL, NULL);
	names_add_name("ADM_SBSP_AD_SRC_MISSING_BIB_BLKS", "Src BIB Block Missing", ADM_SBSP, ADM_SBSP_AD_SRC_MISSING_BIB_BLKS);
	UI_ADD_PARMSPEC_1(ADM_SBSP_AD_SRC_MISSING_BIB_BLKS, "Src Name", AMP_TYPE_STRING);

	adm_add_datadef(ADM_SBSP_AD_SRC_FORWARD_BIB_BLKS, AMP_TYPE_UVAST, 0, NULL, NULL, NULL);
	names_add_name("ADM_SBSP_AD_SRC_FORWARD_BIB_BLKS", "Src BIB Block Forwarded", ADM_SBSP, ADM_SBSP_AD_SRC_FORWARD_BIB_BLKS);
	UI_ADD_PARMSPEC_1(ADM_SBSP_AD_SRC_FORWARD_BIB_BLKS, "Src Name", AMP_TYPE_STRING);

	adm_add_datadef(ADM_SBSP_AD_SRC_GOOD_TX_BIB_BYTES, AMP_TYPE_UVAST, 0, NULL, NULL, NULL);
	names_add_name("ADM_SBSP_AD_SRC_GOOD_TX_BIB_BYTES", "Src BIB Bytes Tx Success", ADM_SBSP, ADM_SBSP_AD_SRC_GOOD_TX_BIB_BYTES);
	UI_ADD_PARMSPEC_1(ADM_SBSP_AD_SRC_GOOD_TX_BIB_BYTES, "Src Name", AMP_TYPE_STRING);

	adm_add_datadef(ADM_SBSP_AD_SRC_FAIL_TX_BIB_BYTES, AMP_TYPE_UVAST, 0, NULL, NULL, NULL);
	names_add_name("ADM_SBSP_AD_SRC_FAIL_TX_BIB_BYTES", "Src BIB Bytes Tx Failure", ADM_SBSP, ADM_SBSP_AD_SRC_FAIL_TX_BIB_BYTES);
	UI_ADD_PARMSPEC_1(ADM_SBSP_AD_SRC_FAIL_TX_BIB_BYTES, "Src Name", AMP_TYPE_STRING);

	adm_add_datadef(ADM_SBSP_AD_SRC_GOOD_RX_BIB_BYTES, AMP_TYPE_UVAST, 0, NULL, NULL, NULL);
	names_add_name("ADM_SBSP_AD_SRC_GOOD_RX_BIB_BYTES", "Src BIB Bytes Rx Success", ADM_SBSP, ADM_SBSP_AD_SRC_GOOD_RX_BIB_BYTES);
	UI_ADD_PARMSPEC_1(ADM_SBSP_AD_SRC_GOOD_RX_BIB_BYTES, "Src Name", AMP_TYPE_STRING);

	adm_add_datadef(ADM_SBSP_AD_SRC_FAIL_RX_BIB_BYTES, AMP_TYPE_UVAST, 0, NULL, NULL, NULL);
	names_add_name("ADM_SBSP_AD_SRC_FAIL_RX_BIB_BYTES", "Src BIB Bytes Rx Failure", ADM_SBSP, ADM_SBSP_AD_SRC_FAIL_RX_BIB_BYTES);
	UI_ADD_PARMSPEC_1(ADM_SBSP_AD_SRC_FAIL_RX_BIB_BYTES, "Src Name", AMP_TYPE_STRING);

	adm_add_datadef(ADM_SBSP_AD_SRC_MISSING_BIB_BYTES, AMP_TYPE_UVAST, 0, NULL, NULL, NULL);
	names_add_name("ADM_SBSP_AD_SRC_MISSING_BIB_BYTES", "Src BIB Bytes Missing", ADM_SBSP, ADM_SBSP_AD_SRC_MISSING_BIB_BYTES);
	UI_ADD_PARMSPEC_1(ADM_SBSP_AD_SRC_MISSING_BIB_BYTES, "Src Name", AMP_TYPE_STRING);

	adm_add_datadef(ADM_SBSP_AD_SRC_FORWARD_BIB_BYTES, AMP_TYPE_UVAST, 0, NULL, NULL, NULL);
	names_add_name("ADM_SBSP_AD_SRC_FORWARD_BIB_BYTES", "Src BIB Bytes Forwarded", ADM_SBSP, ADM_SBSP_AD_SRC_FORWARD_BIB_BYTES);
	UI_ADD_PARMSPEC_1(ADM_SBSP_AD_SRC_FORWARD_BIB_BYTES, "Src Name", AMP_TYPE_STRING);

	adm_add_datadef(ADM_SBSP_AD_SRC_LAST_UPDATE, AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("ADM_SBSP_AD_SRC_LAST_UPDATE", "Src Last Update", ADM_SBSP, ADM_SBSP_AD_SRC_LAST_UPDATE);
	UI_ADD_PARMSPEC_1(ADM_SBSP_AD_SRC_LAST_UPDATE, "Src Name", AMP_TYPE_STRING);

	adm_add_datadef(ADM_SBSP_AD_LAST_RESET, AMP_TYPE_TS, 0, NULL, NULL, NULL);
	names_add_name("ADM_SBSP_AD_LAST_RESET", "Last Reset Time", ADM_SBSP, ADM_SBSP_AD_LAST_RESET);
	UI_ADD_PARMSPEC_1(ADM_SBSP_AD_LAST_RESET, "Src Name", AMP_TYPE_STRING);

#endif

}


void adm_sbsp_init_computed()
{
}


void adm_sbsp_init_controls()
{

#ifdef AGENT_ROLE

	adm_add_ctrl(ADM_SBSP_CTL_RESET_ALL_MID, adm_sbsp_ctl_reset_all);

    adm_add_ctrl(ADM_SBSP_CTL_RESET_SRC_MID,  adm_sbsp_ctl_reset_src);

    adm_add_ctrl(ADM_SBSP_CTL_DEL_KEY_MID,  adm_sbsp_ctl_del_key);
    adm_add_ctrl(ADM_SBSP_CTL_ADD_KEY_MID,  adm_sbsp_ctl_add_key);

    adm_add_ctrl(ADM_SBSP_CTL_ADD_BIB_RULE_MID,  adm_sbsp_ctl_add_bibrule);
    adm_add_ctrl(ADM_SBSP_CTL_DEL_BIB_RULE_MID, adm_sbsp_ctl_del_bibrule);
    adm_add_ctrl(ADM_SBSP_CTL_LIST_BIB_RULE_MID, adm_sbsp_ctl_list_bibrule);

    adm_add_ctrl(ADM_SBSP_CTL_ADD_BCB_RULE_MID, adm_sbsp_ctl_add_bcbrule);
    adm_add_ctrl(ADM_SBSP_CTL_DEL_BCB_RULE_MID, adm_sbsp_ctl_del_bcbrule);
    adm_add_ctrl(ADM_SBSP_CTL_LIST_BCB_RULE_MID, adm_sbsp_ctl_list_bcbrule);

    adm_add_ctrl(ADM_SBSP_CTL_UP_BIB_RULE_MID, adm_sbsp_ctl_update_bibrule);
    adm_add_ctrl(ADM_SBSP_CTL_UP_BCB_RULE_MID, adm_sbsp_ctl_update_bcbrule);


#else

	adm_add_ctrl(ADM_SBSP_CTL_RESET_ALL_MID, NULL);
	names_add_name("ADM_SBSP_CTL_RESET_ALL_MID", "Reset All Counters", ADM_SBSP, ADM_SBSP_CTL_RESET_ALL_MID);

    adm_add_ctrl(ADM_SBSP_CTL_RESET_SRC_MID,  NULL);
	names_add_name("ADM_SBSP_CTL_RESET_SRC_MID", "Reset Counters for SRC", ADM_SBSP, ADM_SBSP_CTL_RESET_SRC_MID);
	UI_ADD_PARMSPEC_1(ADM_SBSP_CTL_RESET_SRC_MID, "Source", AMP_TYPE_STRING);

    adm_add_ctrl(ADM_SBSP_CTL_DEL_KEY_MID,  NULL);
	names_add_name("ADM_SBSP_CTL_DELKEY_MID", "Delete Key", ADM_SBSP, ADM_SBSP_CTL_DEL_KEY_MID);
	UI_ADD_PARMSPEC_1(ADM_SBSP_CTL_DEL_KEY_MID, "Key Name", AMP_TYPE_STRING);

    adm_add_ctrl(ADM_SBSP_CTL_ADD_KEY_MID,  NULL);
	names_add_name("ADM_SBSP_CTL_ADDKEY_MID", "Add Key", ADM_SBSP, ADM_SBSP_CTL_ADD_KEY_MID);
	UI_ADD_PARMSPEC_2(ADM_SBSP_CTL_ADD_KEY_MID, "Key Name", AMP_TYPE_STRING, "Key Value", AMP_TYPE_BLOB);

    adm_add_ctrl(ADM_SBSP_CTL_ADD_BIB_RULE_MID,  NULL);
	names_add_name("ADM_SBSP_CTL_ADD_BIB_RULE_MID", "Add BIB Rule", ADM_SBSP, ADM_SBSP_CTL_ADD_BIB_RULE_MID);

	UI_ADD_PARMSPEC_5(ADM_SBSP_CTL_ADD_BIB_RULE_MID, "SRC", AMP_TYPE_STRING, \
			                                          "DEST", AMP_TYPE_STRING, \
													  "Target", AMP_TYPE_INT, \
												      "Ciphersuite", AMP_TYPE_STRING, "Key Name", AMP_TYPE_STRING);

    adm_add_ctrl(ADM_SBSP_CTL_DEL_BIB_RULE_MID, NULL);
	names_add_name("ADM_SBSP_CTL_DEL_BIB_RULE_MID", "Delete BIB Rule", ADM_SBSP, ADM_SBSP_CTL_DEL_BIB_RULE_MID);
	UI_ADD_PARMSPEC_3(ADM_SBSP_CTL_DEL_BIB_RULE_MID, "SRC", AMP_TYPE_STRING, \
			                                          "DEST", AMP_TYPE_STRING, \
													  "Target", AMP_TYPE_INT);


    adm_add_ctrl(ADM_SBSP_CTL_LIST_BIB_RULE_MID, NULL);
	names_add_name("ADM_SBSP_CTL_LIST_BIB_RULE_MID", "List BIB Rules", ADM_SBSP, ADM_SBSP_CTL_LIST_BIB_RULE_MID);


    adm_add_ctrl(ADM_SBSP_CTL_ADD_BCB_RULE_MID,  NULL);
	names_add_name("ADM_SBSP_CTL_ADD_BCB_RULE_MID", "Add BCB Rule", ADM_SBSP, ADM_SBSP_CTL_ADD_BCB_RULE_MID);

	UI_ADD_PARMSPEC_5(ADM_SBSP_CTL_ADD_BCB_RULE_MID, "SRC", AMP_TYPE_STRING, \
			                                          "DEST", AMP_TYPE_STRING, \
													  "Target", AMP_TYPE_INT, \
												      "Ciphersuite", AMP_TYPE_STRING, "Key Name", AMP_TYPE_STRING);

    adm_add_ctrl(ADM_SBSP_CTL_DEL_BCB_RULE_MID, NULL);
	names_add_name("ADM_SBSP_CTL_DEL_BCB_RULE_MID", "Delete BCB Rule", ADM_SBSP, ADM_SBSP_CTL_DEL_BCB_RULE_MID);
	UI_ADD_PARMSPEC_3(ADM_SBSP_CTL_DEL_BCB_RULE_MID, "SRC", AMP_TYPE_STRING, \
				                                          "DEST", AMP_TYPE_STRING, \
														  "Target", AMP_TYPE_INT);
    adm_add_ctrl(ADM_SBSP_CTL_LIST_BCB_RULE_MID, NULL);
	names_add_name("ADM_SBSP_CTL_LIST_BCB_RULE_MID", "List BCB Rules", ADM_SBSP, ADM_SBSP_CTL_LIST_BCB_RULE_MID);

    adm_add_ctrl(ADM_SBSP_CTL_UP_BIB_RULE_MID,  NULL);
	names_add_name("ADM_SBSP_CTL_UP_BIB_RULE_MID", "Update BIB Rule", ADM_SBSP, ADM_SBSP_CTL_UP_BIB_RULE_MID);

	UI_ADD_PARMSPEC_5(ADM_SBSP_CTL_UP_BIB_RULE_MID, "SRC", AMP_TYPE_STRING, \
			                                          "DEST", AMP_TYPE_STRING, \
													  "Target", AMP_TYPE_INT, \
												      "Ciphersuite", AMP_TYPE_STRING, "Key Name", AMP_TYPE_STRING);


    adm_add_ctrl(ADM_SBSP_CTL_UP_BCB_RULE_MID,  NULL);
	names_add_name("ADM_SBSP_CTL_UP_BCB_RULE_MID", "Update BCB Rule", ADM_SBSP, ADM_SBSP_CTL_UP_BCB_RULE_MID);

	UI_ADD_PARMSPEC_5(ADM_SBSP_CTL_UP_BCB_RULE_MID, "SRC", AMP_TYPE_STRING, \
			                                          "DEST", AMP_TYPE_STRING, \
													  "Target", AMP_TYPE_INT, \
												      "Ciphersuite", AMP_TYPE_STRING, "Key Name", AMP_TYPE_STRING);

#endif

}


void adm_sbsp_init_literals()
{
}


void adm_sbsp_init_macros()
{
}


void adm_sbsp_init_metadata()
{
	/* Step 1: Register Nicknames */

	oid_nn_add_parm(SBSP_ADM_MD_NN_IDX,   SBSP_ADM_MD_NN_STR,   "SBSP", "2016_05_16");
	oid_nn_add_parm(SBSP_ADM_AD_NN_IDX,   SBSP_ADM_AD_NN_STR,   "SBSP", "2016_05_16");
	oid_nn_add_parm(SBSP_ADM_CD_NN_IDX,   SBSP_ADM_CD_NN_STR,   "SBSP", "2016_05_16");
	oid_nn_add_parm(SBSP_ADM_RPT_NN_IDX,  SBSP_ADM_RPT_NN_STR,  "SBSP", "2016_05_16");
	oid_nn_add_parm(SBSP_ADM_CTRL_NN_IDX, SBSP_ADM_CTRL_NN_STR, "SBSP", "2016_05_16");
	oid_nn_add_parm(SBSP_ADM_LTRL_NN_IDX, SBSP_ADM_LTRL_NN_STR, "SBSP", "2016_05_16");
	oid_nn_add_parm(SBSP_ADM_MAC_NN_IDX,  SBSP_ADM_MAC_NN_STR,  "SBSP", "2016_05_16");
	oid_nn_add_parm(SBSP_ADM_OP_NN_IDX,   SBSP_ADM_OP_NN_STR,   "SBSP", "2016_05_16");
	oid_nn_add_parm(SBSP_ADM_ROOT_NN_IDX, SBSP_ADM_ROOT_NN_STR, "SBSP", "2016_05_16");

	/* Step 2: Register Metadata Information. */
#ifdef AGENT_ROLE
	adm_add_datadef(ADM_SBSP_MD_NAME_MID, AMP_TYPE_STRING, 0, adm_sbsp_md_name, adm_print_string, adm_size_string);
	adm_add_datadef(ADM_SBSP_MD_VER_MID,  AMP_TYPE_STRING, 0, adm_sbsp_md_ver,  adm_print_string, adm_size_string);
#else
	adm_add_datadef(ADM_SBSP_MD_NAME_MID, AMP_TYPE_STRING, 0, NULL, adm_print_string, adm_size_string);
	names_add_name("ADM_SBSP_MD_NAME_MID", "ADM Name", ADM_SBSP, ADM_SBSP_MD_NAME_MID);

	adm_add_datadef(ADM_SBSP_MD_VER_MID,  AMP_TYPE_STRING, 0, NULL, adm_print_string, adm_size_string);
	names_add_name("ADM_SBSP_MD_VER_MID", "ADM Version", ADM_SBSP, ADM_SBSP_MD_VER_MID);
#endif
}


void adm_sbsp_init_ops()
{
}



void adm_sbsp_init_reports()
{
	uint32_t used = 0;
	Lyst rpt = lyst_create();

	lyst_insert_last(rpt,mid_deserialize_str(ADM_SBSP_AD_TOT_GOOD_TX_BCB_BLKS, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_SBSP_AD_TOT_FAIL_TX_BCB_BLKS, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_SBSP_AD_TOT_GOOD_RX_BCB_BLKS, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_SBSP_AD_TOT_FAIL_RX_BCB_BLKS, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_SBSP_AD_TOT_MISSING_BCB_BLKS, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_SBSP_AD_TOT_FORWARD_BCB_BLKS, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_SBSP_AD_TOT_GOOD_TX_BCB_BYTES, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_SBSP_AD_TOT_FAIL_TX_BCB_BYTES, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_SBSP_AD_TOT_GOOD_RX_BCB_BYTES, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_SBSP_AD_TOT_FAIL_RX_BCB_BYTES, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_SBSP_AD_TOT_MISSING_BCB_BYTES, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_SBSP_AD_TOT_FORWARD_BCB_BYTES, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_SBSP_AD_TOT_GOOD_TX_BIB_BLKS, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_SBSP_AD_TOT_FAIL_TX_BIB_BLKS, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_SBSP_AD_TOT_GOOD_RX_BIB_BLKS, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_SBSP_AD_TOT_FAIL_RX_BIB_BLKS, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_SBSP_AD_TOT_MISSING_BIB_BLKS, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_SBSP_AD_TOT_FORWARD_BIB_BLKS, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_SBSP_AD_TOT_GOOD_TX_BIB_BYTES, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_SBSP_AD_TOT_FAIL_TX_BIB_BYTES, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_SBSP_AD_TOT_GOOD_RX_BIB_BYTES, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_SBSP_AD_TOT_FAIL_RX_BIB_BYTES, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_SBSP_AD_TOT_MISSING_BIB_BYTES, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_SBSP_AD_TOT_FORWARD_BIB_BYTES, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_SBSP_AD_LAST_UPDATE, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_SBSP_AD_NUM_KEYS, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_SBSP_AD_KEYS, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_SBSP_AD_CIPHS, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_SBSP_AD_SRCS, ADM_MID_ALLOC, &used));

	adm_add_rpt(ADM_SBSP_RPT_FULL_MID, rpt);

	midcol_destroy(&rpt);

	rpt = lyst_create();
	lyst_insert_last(rpt,mid_deserialize_str(ADM_SBSP_AD_SRC_GOOD_TX_BCB_BLKS, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_SBSP_AD_SRC_FAIL_TX_BCB_BLKS, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_SBSP_AD_SRC_GOOD_RX_BCB_BLKS, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_SBSP_AD_SRC_FAIL_RX_BCB_BLKS, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_SBSP_AD_SRC_MISSING_BCB_BLKS, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_SBSP_AD_SRC_FORWARD_BCB_BLKS, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_SBSP_AD_SRC_GOOD_TX_BCB_BYTES, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_SBSP_AD_SRC_FAIL_TX_BCB_BYTES, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_SBSP_AD_SRC_GOOD_RX_BCB_BYTES, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_SBSP_AD_SRC_FAIL_RX_BCB_BYTES, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_SBSP_AD_SRC_MISSING_BCB_BYTES, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_SBSP_AD_SRC_FORWARD_BCB_BYTES, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_SBSP_AD_SRC_GOOD_TX_BIB_BLKS, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_SBSP_AD_SRC_FAIL_TX_BIB_BLKS, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_SBSP_AD_SRC_GOOD_RX_BIB_BLKS, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_SBSP_AD_SRC_FAIL_RX_BIB_BLKS, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_SBSP_AD_SRC_MISSING_BIB_BLKS, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_SBSP_AD_SRC_FORWARD_BIB_BLKS, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_SBSP_AD_SRC_GOOD_TX_BIB_BYTES, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_SBSP_AD_SRC_FAIL_TX_BIB_BYTES, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_SBSP_AD_SRC_GOOD_RX_BIB_BYTES, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_SBSP_AD_SRC_FAIL_RX_BIB_BYTES, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_SBSP_AD_SRC_MISSING_BIB_BYTES, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_SBSP_AD_SRC_FORWARD_BIB_BYTES, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_SBSP_AD_SRC_LAST_UPDATE, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_SBSP_AD_LAST_RESET, ADM_MID_ALLOC, &used));

	adm_add_rpt(ADM_SBSP_RPT_SRC_MID, rpt);

	midcol_destroy(&rpt);


#ifndef AGENT_ROLE
	names_add_name("ADM_SBSP_RPT_FULL_MID", "Full Report", ADM_SBSP, ADM_SBSP_RPT_FULL_MID);

	names_add_name("ADM_SBSP_RPT_SRC_MID", "Source Report", ADM_BP, ADM_SBSP_RPT_SRC_MID);
	UI_ADD_PARMSPEC_1(ADM_SBSP_RPT_SRC_MID, "Source Name", AMP_TYPE_STRING);
#endif

}


#endif // _HAVE_SBSP_ADM_
