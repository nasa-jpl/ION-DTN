/****************************************************************************
 **
 ** File Name: adm_bpsec_mgr.c
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
#include "../shared/adm/adm_bpsec.h"
#include "../shared/utils/utils.h"
#include "../shared/primitives/def.h"
#include "../shared/primitives/nn.h"
#include "../shared/primitives/report.h"
#include "../shared/primitives/blob.h"
#include "nm_mgr_names.h"
#include "nm_mgr_ui.h"

#define _HAVE_BPSEC_ADM_
#ifdef _HAVE_BPSEC_ADM_

void adm_bpsec_init()
{
	adm_bpsec_init_edd();
	adm_bpsec_init_variables();
	adm_bpsec_init_controls();
	adm_bpsec_init_constants();
	adm_bpsec_init_macros();
	adm_bpsec_init_metadata();
	adm_bpsec_init_ops();
	adm_bpsec_init_reports();
}


void adm_bpsec_init_edd()
{
	adm_add_edd(mid_from_value(ADM_BPSEC_EDD_NUM_GOOD_TX_BCB_BLK_MID), AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("NUM_GOOD_TX_BCB_BLK", "Total successfully Tx BCB blocks", ADM_BPSEC, ADM_BPSEC_EDD_NUM_GOOD_TX_BCB_BLK_MID);

	adm_add_edd(mid_from_value(ADM_BPSEC_EDD_NUM_BAD_TX_BCB_BLK_MID), AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("NUM_BAD_TX_BCB_BLK", "Total unsuccessfully Tx BCB blocks", ADM_BPSEC, ADM_BPSEC_EDD_NUM_BAD_TX_BCB_BLK_MID);

	adm_add_edd(mid_from_value(ADM_BPSEC_EDD_NUM_GOOD_RX_BCB_BLK_MID), AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("NUM_GOOD_RX_BCB_BLK", "Total successfully Rx BCB blocks", ADM_BPSEC, ADM_BPSEC_EDD_NUM_GOOD_RX_BCB_BLK_MID);

	adm_add_edd(mid_from_value(ADM_BPSEC_EDD_NUM_BAD_RX_BCB_BLK_MID), AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("NUM_BAD_RX_BCB_BLK", "Total unsuccessfully Rx BCB blocks", ADM_BPSEC, ADM_BPSEC_EDD_NUM_BAD_RX_BCB_BLK_MID);

	adm_add_edd(mid_from_value(ADM_BPSEC_EDD_NUM_MISSING_RX_BCB_BLKS_MID), AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("NUM_MISSING_RX_BCB_BLKS", "Total missing-on-RX BCB blocks", ADM_BPSEC, ADM_BPSEC_EDD_NUM_MISSING_RX_BCB_BLKS_MID);

	adm_add_edd(mid_from_value(ADM_BPSEC_EDD_NUM_FWD_BCB_BLKS_MID), AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("NUM_FWD_BCB_BLKS", "Total forward BCB blocks", ADM_BPSEC, ADM_BPSEC_EDD_NUM_FWD_BCB_BLKS_MID);

	adm_add_edd(mid_from_value(ADM_BPSEC_EDD_NUM_GOOD_TX_BCB_BYTES_MID), AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("NUM_GOOD_TX_BCB_BYTES", "Total successfully Tx bcb bytes", ADM_BPSEC, ADM_BPSEC_EDD_NUM_GOOD_TX_BCB_BYTES_MID);

	adm_add_edd(mid_from_value(ADM_BPSEC_EDD_NUM_BAD_TX_BCB_BYTES_MID), AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("NUM_BAD_TX_BCB_BYTES", "Total unsuccessfully Tx bcb bytes", ADM_BPSEC, ADM_BPSEC_EDD_NUM_BAD_TX_BCB_BYTES_MID);

	adm_add_edd(mid_from_value(ADM_BPSEC_EDD_NUM_GOOD_RX_BCB_BYTES_MID), AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("NUM_GOOD_RX_BCB_BYTES", "Total successfully Rx bcb bytes", ADM_BPSEC, ADM_BPSEC_EDD_NUM_GOOD_RX_BCB_BYTES_MID);

	adm_add_edd(mid_from_value(ADM_BPSEC_EDD_NUM_BAD_RX_BCB_BYTES_MID), AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("NUM_BAD_RX_BCB_BYTES", "Total unsuccessfully Rx bcb bytes", ADM_BPSEC, ADM_BPSEC_EDD_NUM_BAD_RX_BCB_BYTES_MID);

	adm_add_edd(mid_from_value(ADM_BPSEC_EDD_NUM_MISSING_RX_BCB_BYTES_MID), AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("NUM_MISSING_RX_BCB_BYTES", "Total missing-on-Rx bcb bytes", ADM_BPSEC, ADM_BPSEC_EDD_NUM_MISSING_RX_BCB_BYTES_MID);

	adm_add_edd(mid_from_value(ADM_BPSEC_EDD_NUM_FWD_BCB_BYTES_MID), AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("NUM_FWD_BCB_BYTES", "Total forwarded bcb bytes", ADM_BPSEC, ADM_BPSEC_EDD_NUM_FWD_BCB_BYTES_MID);

	adm_add_edd(mid_from_value(ADM_BPSEC_EDD_NUM_GOOD_TX_BIB_BLKS_MID), AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("NUM_GOOD_TX_BIB_BLKS", "Total successfully Tx BIB blocks", ADM_BPSEC, ADM_BPSEC_EDD_NUM_GOOD_TX_BIB_BLKS_MID);

	adm_add_edd(mid_from_value(ADM_BPSEC_EDD_NUM_BAD_TX_BIB_BLKS_MID), AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("NUM_BAD_TX_BIB_BLKS", "Total unsuccessfully Tx BIB blocks", ADM_BPSEC, ADM_BPSEC_EDD_NUM_BAD_TX_BIB_BLKS_MID);

	adm_add_edd(mid_from_value(ADM_BPSEC_EDD_NUM_GOOD_RX_BIB_BLKS_MID), AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("NUM_GOOD_RX_BIB_BLKS", "Total successfully Rx BIB blocks", ADM_BPSEC, ADM_BPSEC_EDD_NUM_GOOD_RX_BIB_BLKS_MID);

	adm_add_edd(mid_from_value(ADM_BPSEC_EDD_NUM_BAD_RX_BIB_BLKS_MID), AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("NUM_BAD_RX_BIB_BLKS", "Total unsuccessfully Rx BIB blocks", ADM_BPSEC, ADM_BPSEC_EDD_NUM_BAD_RX_BIB_BLKS_MID);

	adm_add_edd(mid_from_value(ADM_BPSEC_EDD_NUM_MISS_RX_BIB_BLKS_MID), AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("NUM_MISS_RX_BIB_BLKS", "Total missing-on-Rx BIB blocks", ADM_BPSEC, ADM_BPSEC_EDD_NUM_MISS_RX_BIB_BLKS_MID);

	adm_add_edd(mid_from_value(ADM_BPSEC_EDD_NUM_FWD_BIB_BLKS_MID), AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("NUM_FWD_BIB_BLKS", "Total forwarded BIB blocks", ADM_BPSEC, ADM_BPSEC_EDD_NUM_FWD_BIB_BLKS_MID);

	adm_add_edd(mid_from_value(ADM_BPSEC_EDD_NUM_GOOD_TX_BIB_BYTES_MID), AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("NUM_GOOD_TX_BIB_BYTES", "Total successfully Tx BIB bytes", ADM_BPSEC, ADM_BPSEC_EDD_NUM_GOOD_TX_BIB_BYTES_MID);

	adm_add_edd(mid_from_value(ADM_BPSEC_EDD_NUM_BAD_TX_BIB_BYTES_MID), AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("NUM_BAD_TX_BIB_BYTES", "Total unsuccessfully Tx BIB bytes", ADM_BPSEC, ADM_BPSEC_EDD_NUM_BAD_TX_BIB_BYTES_MID);

	adm_add_edd(mid_from_value(ADM_BPSEC_EDD_NUM_GOOD_RX_BIB_BYTES_MID), AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("NUM_GOOD_RX_BIB_BYTES", "Total successfully Rx BIB bytes", ADM_BPSEC, ADM_BPSEC_EDD_NUM_GOOD_RX_BIB_BYTES_MID);

	adm_add_edd(mid_from_value(ADM_BPSEC_EDD_NUM_BAD_RX_BIB_BYTES_MID), AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("NUM_BAD_RX_BIB_BYTES", "Total unsuccessfully Rx BIB bytes", ADM_BPSEC, ADM_BPSEC_EDD_NUM_BAD_RX_BIB_BYTES_MID);

	adm_add_edd(mid_from_value(ADM_BPSEC_EDD_NUM_MISS_RX_BIB_BYTES_MID), AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("NUM_MISS_RX_BIB_BYTES", "Total missing-on-Rx BIB bytes", ADM_BPSEC, ADM_BPSEC_EDD_NUM_MISS_RX_BIB_BYTES_MID);

	adm_add_edd(mid_from_value(ADM_BPSEC_EDD_NUM_FWD_BIB_BYTES_MID), AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("NUM_FWD_BIB_BYTES", "Total forwarded BIB bytes", ADM_BPSEC, ADM_BPSEC_EDD_NUM_FWD_BIB_BYTES_MID);

	adm_add_edd(mid_from_value(ADM_BPSEC_EDD_LAST_UPDATE_MID), AMP_TYPE_TS, 0, NULL, NULL, NULL);
	names_add_name("LAST_UPDATE", "Last BPSEC update", ADM_BPSEC, ADM_BPSEC_EDD_LAST_UPDATE_MID);

	adm_add_edd(mid_from_value(ADM_BPSEC_EDD_NUM_KNOWN_KEYS_MID), AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("NUM_KNOWN_KEYS", "Number of known keys", ADM_BPSEC, ADM_BPSEC_EDD_NUM_KNOWN_KEYS_MID);

	adm_add_edd(mid_from_value(ADM_BPSEC_EDD_KEY_NAMES_MID), AMP_TYPE_STRING, 0, NULL, NULL, NULL);
	names_add_name("KEY_NAMES", "Known key names", ADM_BPSEC, ADM_BPSEC_EDD_KEY_NAMES_MID);

	adm_add_edd(mid_from_value(ADM_BPSEC_EDD_CIPHERSUITE_NAMES_MID), AMP_TYPE_STRING, 0, NULL, NULL, NULL);
	names_add_name("CIPHERSUITE_NAMES", "Known ciphersuite names", ADM_BPSEC, ADM_BPSEC_EDD_CIPHERSUITE_NAMES_MID);

	adm_add_edd(mid_from_value(ADM_BPSEC_EDD_RULE_SOURCE_MID), AMP_TYPE_STRING, 0, NULL, NULL, NULL);
	names_add_name("RULE_SOURCE", "Known rule sources", ADM_BPSEC, ADM_BPSEC_EDD_RULE_SOURCE_MID);

	adm_add_edd(mid_from_value(ADM_BPSEC_EDD_NUM_GOOD_TX_BCB_BLKS_SRC_MID), AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("NUM_GOOD_TX_BCB_BLKS_SRC", "Successfully Tx BCB blocks from SRC", ADM_BPSEC, ADM_BPSEC_EDD_NUM_GOOD_TX_BCB_BLKS_SRC_MID);
	UI_ADD_PARMSPEC_1(ADM_BPSEC_EDD_NUM_GOOD_TX_BCB_BLKS_SRC_MID, "Src", AMP_TYPE_STR);

	adm_add_edd(mid_from_value(ADM_BPSEC_EDD_NUM_BAD_TX_BCB_BLKS_SRC_MID), AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("NUM_BAD_TX_BCB_BLKS_SRC", "Failed TX BCB blocks from SRC", ADM_BPSEC, ADM_BPSEC_EDD_NUM_BAD_TX_BCB_BLKS_SRC_MID);
	UI_ADD_PARMSPEC_1(ADM_BPSEC_EDD_NUM_BAD_TX_BCB_BLKS_SRC_MID, "Src", AMP_TYPE_STR);

	adm_add_edd(mid_from_value(ADM_BPSEC_EDD_NUM_GOOD_RX_BCB_BLKS_SRC_MID), AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("NUM_GOOD_RX_BCB_BLKS_SRC", "Successfully Rx BCB blocks from SRC", ADM_BPSEC, ADM_BPSEC_EDD_NUM_GOOD_RX_BCB_BLKS_SRC_MID);
	UI_ADD_PARMSPEC_1(ADM_BPSEC_EDD_NUM_GOOD_RX_BCB_BLKS_SRC_MID, "Src", AMP_TYPE_STR);

	adm_add_edd(mid_from_value(ADM_BPSEC_EDD_NUM_BAD_RX_BCB_BLKS_SRC_MID), AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("NUM_BAD_RX_BCB_BLKS_SRC", "Failed RX BCB blocks from SRC", ADM_BPSEC, ADM_BPSEC_EDD_NUM_BAD_RX_BCB_BLKS_SRC_MID);
	UI_ADD_PARMSPEC_1(ADM_BPSEC_EDD_NUM_BAD_RX_BCB_BLKS_SRC_MID, "Src", AMP_TYPE_STR);

	adm_add_edd(mid_from_value(ADM_BPSEC_EDD_NUM_MISSING_RX_BCB_BLKS_SRC_MID), AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("NUM_MISSING_RX_BCB_BLKS_SRC", "Missing-onRX BCB blocks from SRC", ADM_BPSEC, ADM_BPSEC_EDD_NUM_MISSING_RX_BCB_BLKS_SRC_MID);
	UI_ADD_PARMSPEC_1(ADM_BPSEC_EDD_NUM_MISSING_RX_BCB_BLKS_SRC_MID, "Src", AMP_TYPE_STR);

	adm_add_edd(mid_from_value(ADM_BPSEC_EDD_NUM_FWD_BCB_BLKS_SRC_MID), AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("NUM_FWD_BCB_BLKS_SRC", "Forwarded BCB blocks from SRC", ADM_BPSEC, ADM_BPSEC_EDD_NUM_FWD_BCB_BLKS_SRC_MID);
	UI_ADD_PARMSPEC_1(ADM_BPSEC_EDD_NUM_FWD_BCB_BLKS_SRC_MID, "Src", AMP_TYPE_STR);

	adm_add_edd(mid_from_value(ADM_BPSEC_EDD_NUM_GOOD_TX_BCB_BYTES_SRC_MID), AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("NUM_GOOD_TX_BCB_BYTES_SRC", "Successfullt Tx bcb bytes from SRC", ADM_BPSEC, ADM_BPSEC_EDD_NUM_GOOD_TX_BCB_BYTES_SRC_MID);
	UI_ADD_PARMSPEC_1(ADM_BPSEC_EDD_NUM_GOOD_TX_BCB_BYTES_SRC_MID, "Src", AMP_TYPE_STR);

	adm_add_edd(mid_from_value(ADM_BPSEC_EDD_NUM_BAD_TX_BCB_BYTES_SRC_MID), AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("NUM_BAD_TX_BCB_BYTES_SRC", "Failed Tx bcb bytes from SRC", ADM_BPSEC, ADM_BPSEC_EDD_NUM_BAD_TX_BCB_BYTES_SRC_MID);
	UI_ADD_PARMSPEC_1(ADM_BPSEC_EDD_NUM_BAD_TX_BCB_BYTES_SRC_MID, "Src", AMP_TYPE_STR);

	adm_add_edd(mid_from_value(ADM_BPSEC_EDD_NUM_GOOD_RX_BCB_BYTES_SRC_MID), AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("NUM_GOOD_RX_BCB_BYTES_SRC", "Successfully Rx bcb bytes from SRC", ADM_BPSEC, ADM_BPSEC_EDD_NUM_GOOD_RX_BCB_BYTES_SRC_MID);
	UI_ADD_PARMSPEC_1(ADM_BPSEC_EDD_NUM_GOOD_RX_BCB_BYTES_SRC_MID, "Src", AMP_TYPE_STR);

	adm_add_edd(mid_from_value(ADM_BPSEC_EDD_NUM_BAD_RX_BCB_BYTES_SRC_MID), AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("NUM_BAD_RX_BCB_BYTES_SRC", "Failed Rx bcb bytes from SRC", ADM_BPSEC, ADM_BPSEC_EDD_NUM_BAD_RX_BCB_BYTES_SRC_MID);
	UI_ADD_PARMSPEC_1(ADM_BPSEC_EDD_NUM_BAD_RX_BCB_BYTES_SRC_MID, "Src", AMP_TYPE_STR);

	adm_add_edd(mid_from_value(ADM_BPSEC_EDD_NUM_MISSING_RX_BCB_BYTES_SRC_MID), AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("NUM_MISSING_RX_BCB_BYTES_SRC", "Missin-on-Rx bcb bytes from SRC", ADM_BPSEC, ADM_BPSEC_EDD_NUM_MISSING_RX_BCB_BYTES_SRC_MID);
	UI_ADD_PARMSPEC_1(ADM_BPSEC_EDD_NUM_MISSING_RX_BCB_BYTES_SRC_MID, "Src", AMP_TYPE_STR);

	adm_add_edd(mid_from_value(ADM_BPSEC_EDD_NUM_FWD_BCB_BYTES_SRC_MID), AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("NUM_FWD_BCB_BYTES_SRC", "Forwarded bcb bytes from SRC", ADM_BPSEC, ADM_BPSEC_EDD_NUM_FWD_BCB_BYTES_SRC_MID);
	UI_ADD_PARMSPEC_1(ADM_BPSEC_EDD_NUM_FWD_BCB_BYTES_SRC_MID, "Src", AMP_TYPE_STR);

	adm_add_edd(mid_from_value(ADM_BPSEC_EDD_NUM_GOOD_TX_BIB_BLKS_SRC_MID), AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("NUM_GOOD_TX_BIB_BLKS_SRC", "Successfully Tx BIB blocks from SRC", ADM_BPSEC, ADM_BPSEC_EDD_NUM_GOOD_TX_BIB_BLKS_SRC_MID);
	UI_ADD_PARMSPEC_1(ADM_BPSEC_EDD_NUM_GOOD_TX_BIB_BLKS_SRC_MID, "Src", AMP_TYPE_STR);

	adm_add_edd(mid_from_value(ADM_BPSEC_EDD_NUM_BAD_TX_BIB_BLKS_SRC_MID), AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("NUM_BAD_TX_BIB_BLKS_SRC", "Failed Tx BIB blocks from SRC", ADM_BPSEC, ADM_BPSEC_EDD_NUM_BAD_TX_BIB_BLKS_SRC_MID);
	UI_ADD_PARMSPEC_1(ADM_BPSEC_EDD_NUM_BAD_TX_BIB_BLKS_SRC_MID, "Src", AMP_TYPE_STR);

	adm_add_edd(mid_from_value(ADM_BPSEC_EDD_NUM_GOOD_RX_BIB_BLKS_SRC_MID), AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("NUM_GOOD_RX_BIB_BLKS_SRC", "Successfully Rx BIB blocks from SRC", ADM_BPSEC, ADM_BPSEC_EDD_NUM_GOOD_RX_BIB_BLKS_SRC_MID);
	UI_ADD_PARMSPEC_1(ADM_BPSEC_EDD_NUM_GOOD_RX_BIB_BLKS_SRC_MID, "Src", AMP_TYPE_STR);

	adm_add_edd(mid_from_value(ADM_BPSEC_EDD_NUM_BAD_RX_BIB_BLKS_SRC_MID), AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("NUM_BAD_RX_BIB_BLKS_SRC", "Failed Rx BIB blocks from SRC", ADM_BPSEC, ADM_BPSEC_EDD_NUM_BAD_RX_BIB_BLKS_SRC_MID);
	UI_ADD_PARMSPEC_1(ADM_BPSEC_EDD_NUM_BAD_RX_BIB_BLKS_SRC_MID, "Src", AMP_TYPE_STR);

	adm_add_edd(mid_from_value(ADM_BPSEC_EDD_NUM_MISS_RX_BIB_BLKS_SRC_MID), AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("NUM_MISS_RX_BIB_BLKS_SRC", "Missing-on-Rx BIB blocks from SRC", ADM_BPSEC, ADM_BPSEC_EDD_NUM_MISS_RX_BIB_BLKS_SRC_MID);
	UI_ADD_PARMSPEC_1(ADM_BPSEC_EDD_NUM_MISS_RX_BIB_BLKS_SRC_MID, "Src", AMP_TYPE_STR);

	adm_add_edd(mid_from_value(ADM_BPSEC_EDD_NUM_FWD_BIB_BLKS_SRC_MID), AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("NUM_FWD_BIB_BLKS_SRC", "Forwarded BIB blocks from SRC", ADM_BPSEC, ADM_BPSEC_EDD_NUM_FWD_BIB_BLKS_SRC_MID);
	UI_ADD_PARMSPEC_1(ADM_BPSEC_EDD_NUM_FWD_BIB_BLKS_SRC_MID, "Src", AMP_TYPE_STR);

	adm_add_edd(mid_from_value(ADM_BPSEC_EDD_NUM_GOOD_TX_BIB_BYTES_SRC_MID), AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("NUM_GOOD_TX_BIB_BYTES_SRC", "Successfully Tx BIB bytes from SRC", ADM_BPSEC, ADM_BPSEC_EDD_NUM_GOOD_TX_BIB_BYTES_SRC_MID);
	UI_ADD_PARMSPEC_1(ADM_BPSEC_EDD_NUM_GOOD_TX_BIB_BYTES_SRC_MID, "Src", AMP_TYPE_STR);

	adm_add_edd(mid_from_value(ADM_BPSEC_EDD_NUM_BAD_TX_BIB_BYTES_SRC_MID), AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("NUM_BAD_TX_BIB_BYTES_SRC", "Failed Tx BIB bytes from SRC", ADM_BPSEC, ADM_BPSEC_EDD_NUM_BAD_TX_BIB_BYTES_SRC_MID);
	UI_ADD_PARMSPEC_1(ADM_BPSEC_EDD_NUM_BAD_TX_BIB_BYTES_SRC_MID, "Src", AMP_TYPE_STR);

	adm_add_edd(mid_from_value(ADM_BPSEC_EDD_NUM_GOOD_RX_BIB_BYTES_SRC_MID), AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("NUM_GOOD_RX_BIB_BYTES_SRC", "Successfully Rx BIB bytes from SRC", ADM_BPSEC, ADM_BPSEC_EDD_NUM_GOOD_RX_BIB_BYTES_SRC_MID);
	UI_ADD_PARMSPEC_1(ADM_BPSEC_EDD_NUM_GOOD_RX_BIB_BYTES_SRC_MID, "Src", AMP_TYPE_STR);

	adm_add_edd(mid_from_value(ADM_BPSEC_EDD_NUM_BAD_RX_BIB_BYTES_SRC_MID), AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("NUM_BAD_RX_BIB_BYTES_SRC", "Failed Rx BIB bytes from SRC", ADM_BPSEC, ADM_BPSEC_EDD_NUM_BAD_RX_BIB_BYTES_SRC_MID);
	UI_ADD_PARMSPEC_1(ADM_BPSEC_EDD_NUM_BAD_RX_BIB_BYTES_SRC_MID, "Src", AMP_TYPE_STR);

	adm_add_edd(mid_from_value(ADM_BPSEC_EDD_NUM_MISSING_RX_BIB_BYTES_SRC_MID), AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("NUM_MISSING_RX_BIB_BYTES_SRC", "Missing-on-Rx BIB bytes from SRC", ADM_BPSEC, ADM_BPSEC_EDD_NUM_MISSING_RX_BIB_BYTES_SRC_MID);
	UI_ADD_PARMSPEC_1(ADM_BPSEC_EDD_NUM_MISSING_RX_BIB_BYTES_SRC_MID, "Src", AMP_TYPE_STR);

	adm_add_edd(mid_from_value(ADM_BPSEC_EDD_NUM_FWD_BIB_BYTES_SRC_MID), AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("NUM_FWD_BIB_BYTES_SRC", "Forwarded BIB bytes from SRC", ADM_BPSEC, ADM_BPSEC_EDD_NUM_FWD_BIB_BYTES_SRC_MID);
	UI_ADD_PARMSPEC_1(ADM_BPSEC_EDD_NUM_FWD_BIB_BYTES_SRC_MID, "Src", AMP_TYPE_STR);

	adm_add_edd(mid_from_value(ADM_BPSEC_EDD_LAST_UPDATE_SRC_MID), AMP_TYPE_TS, 0, NULL, NULL, NULL);
	names_add_name("LAST_UPDATE_SRC", "Last BPSEC Update from SRC", ADM_BPSEC, ADM_BPSEC_EDD_LAST_UPDATE_SRC_MID);
	UI_ADD_PARMSPEC_1(ADM_BPSEC_EDD_LAST_UPDATE_SRC_MID, "Src", AMP_TYPE_STR);

	adm_add_edd(mid_from_value(ADM_BPSEC_EDD_LAST_RESET_MID), AMP_TYPE_TS, 0, NULL, NULL, NULL);
	names_add_name("LAST_RESET", "Last reset", ADM_BPSEC, ADM_BPSEC_EDD_LAST_RESET_MID);
	UI_ADD_PARMSPEC_1(ADM_BPSEC_EDD_LAST_RESET_MID, "Src", AMP_TYPE_STR);

}


void adm_bpsec_init_variables()
{
	adm_add_edd(mid_from_value(ADM_BPSEC_VAR_TOTAL_BAD_TX_BLKS_MID), AMP_TYPE_UINT, 0, NULL, NULL, NULL);
		names_add_name("TOTAL_BAD_TX_BLKS", "This is the number of failed TX blocks (# failed BIB + # failed bcb).", ADM_BPSEC, ADM_BPSEC_VAR_TOTAL_BAD_TX_BLKS_MID);
}


void adm_bpsec_init_controls()
{
	adm_add_ctrl(mid_from_value(ADM_BPSEC_CTRL_RST_ALL_CNTS_MID), NULL);
	names_add_name("RST_ALL_CNTS", "This control causes the Agent to reset all counts associated with block or byte statistics and to set the Last Reset Time of the BPsec EDD data to the time when the control was run.", ADM_BPSEC, ADM_BPSEC_CTRL_RST_ALL_CNTS_MID);

	adm_add_ctrl(mid_from_value(ADM_BPSEC_CTRL_RST_SRC_CNTS_MID), NULL);
	names_add_name("RST_SRC_CNTS", "This control causes the Agent to reset all counts (blocks and bytes) associated with a given bundle source and set the Last Reset Time of the source statistics to the time when the control was run.", ADM_BPSEC, ADM_BPSEC_CTRL_RST_SRC_CNTS_MID);
	UI_ADD_PARMSPEC_1(ADM_BPSEC_CTRL_RST_SRC_CNTS_MID, "src", AMP_TYPE_STR);

	adm_add_ctrl(mid_from_value(ADM_BPSEC_CTRL_DELETE_KEY_MID), NULL);
	names_add_name("DELETE_KEY", "This control deletes a key from the BPsec system.", ADM_BPSEC, ADM_BPSEC_CTRL_DELETE_KEY_MID);
	UI_ADD_PARMSPEC_1(ADM_BPSEC_CTRL_DELETE_KEY_MID, "key_name", AMP_TYPE_STR);

	adm_add_ctrl(mid_from_value(ADM_BPSEC_CTRL_ADD_KEY_MID), NULL);
	names_add_name("ADD_KEY", "This control adds a key to the BPsec system.", ADM_BPSEC, ADM_BPSEC_CTRL_ADD_KEY_MID);
	UI_ADD_PARMSPEC_2(ADM_BPSEC_CTRL_ADD_KEY_MID, "key_name", AMP_TYPE_STR, "keyData", AMP_TYPE_BLOB);

	adm_add_ctrl(mid_from_value(ADM_BPSEC_CTRL_ADD_BIB_RULE_MID), NULL);
	names_add_name("ADD_BIB_RULE", "This control configures policy on the BPsec protocol implementation that describes how BIB blocks should be applied to bundles in the system. This policy is captured as a rule which states when transmitting a bundle from the given source endpoint ID to the given destination endpoint ID, blocks of type target should have a BIB added to them using the given ciphersuite and the given key.", ADM_BPSEC, ADM_BPSEC_CTRL_ADD_BIB_RULE_MID);
	UI_ADD_PARMSPEC_5(ADM_BPSEC_CTRL_ADD_BIB_RULE_MID, "source", AMP_TYPE_STR, "destination", AMP_TYPE_STR, "target", AMP_TYPE_INT, "ciphersuiteId", AMP_TYPE_STR, "key_name", AMP_TYPE_STR);

	adm_add_ctrl(mid_from_value(ADM_BPSEC_CTRL_DEL_BIB_RULE_MID), NULL);
	names_add_name("DEL_BIB_RULE", "This control removes any configured policy on the BPsec protocol implementation that describes how BIB blocks should be applied to bundles in the system. A BIB policy is uniquely identified by a source endpoint Id, a destination Id, and a target block type.", ADM_BPSEC, ADM_BPSEC_CTRL_DEL_BIB_RULE_MID);
	UI_ADD_PARMSPEC_3(ADM_BPSEC_CTRL_DEL_BIB_RULE_MID, "source", AMP_TYPE_STR, "destination", AMP_TYPE_STR, "target", AMP_TYPE_INT);

	adm_add_ctrl(mid_from_value(ADM_BPSEC_CTRL_LIST_BIB_RULES_MID), NULL);
	names_add_name("LIST_BIB_RULES", "This control returns a table describinng all of the BIB policy rules that are known to the BPsec implementation.", ADM_BPSEC, ADM_BPSEC_CTRL_LIST_BIB_RULES_MID);

	adm_add_ctrl(mid_from_value(ADM_BPSEC_CTRL_ADD_BCB_RULE_MID), NULL);
	names_add_name("ADD_BCB_RULE", "This control configures policy on the BPsec protocol implementation that describes how BCB blocks should be applied to bundles in the system. This policy is captured as a rule which states when transmitting a bundle from the given source endpoint id to the given destination endpoint id, blocks of type target should have a bcb added to them using the given ciphersuite and the given key.", ADM_BPSEC, ADM_BPSEC_CTRL_ADD_BCB_RULE_MID);
	UI_ADD_PARMSPEC_5(ADM_BPSEC_CTRL_ADD_BCB_RULE_MID, "source", AMP_TYPE_STR, "destination", AMP_TYPE_STR, "target", AMP_TYPE_INT, "ciphersuiteId", AMP_TYPE_STR, "key_name", AMP_TYPE_STR);

	adm_add_ctrl(mid_from_value(ADM_BPSEC_CTRL_DEL_BCB_RULE_MID), NULL);
	names_add_name("DEL_BCB_RULE", "This control removes any configured policy on the BPsec protocol implementation that describes how BCB blocks should be applied to bundles in the system. A bcb policy is uniquely identified by a source endpoint id, a destination endpoint id, and a target block type.", ADM_BPSEC, ADM_BPSEC_CTRL_DEL_BCB_RULE_MID);
	UI_ADD_PARMSPEC_3(ADM_BPSEC_CTRL_DEL_BCB_RULE_MID, "source", AMP_TYPE_STR, "destination", AMP_TYPE_STR, "target", AMP_TYPE_INT);

	adm_add_ctrl(mid_from_value(ADM_BPSEC_CTRL_LIST_BCB_RULES_MID), NULL);
	names_add_name("LIST_BCB_RULES", "This control returns a table describing all of the bcb policy rules that are known to the BPsec implementation", ADM_BPSEC, ADM_BPSEC_CTRL_LIST_BCB_RULES_MID);

}


void adm_bpsec_init_constants()
{
}


void adm_bpsec_init_macros()
{
}


void adm_bpsec_init_metadata()
{
	/* Step 1: Register Nicknames */
	oid_nn_add_parm(BPSEC_ADM_META_NN_IDX, BPSEC_ADM_META_NN_STR, "BPSEC", "2017-08-17");
	oid_nn_add_parm(BPSEC_ADM_EDD_NN_IDX, BPSEC_ADM_EDD_NN_STR, "BPSEC", "2017-08-17");
	oid_nn_add_parm(BPSEC_ADM_VAR_NN_IDX, BPSEC_ADM_VAR_NN_STR, "BPSEC", "2017-08-17");
	oid_nn_add_parm(BPSEC_ADM_RPT_NN_IDX, BPSEC_ADM_RPT_NN_STR, "BPSEC", "2017-08-17");
	oid_nn_add_parm(BPSEC_ADM_CTRL_NN_IDX, BPSEC_ADM_CTRL_NN_STR, "BPSEC", "2017-08-17");
	oid_nn_add_parm(BPSEC_ADM_CONST_NN_IDX, BPSEC_ADM_CONST_NN_STR, "BPSEC", "2017-08-17");
	oid_nn_add_parm(BPSEC_ADM_MACRO_NN_IDX, BPSEC_ADM_MACRO_NN_STR, "BPSEC", "2017-08-17");
	oid_nn_add_parm(BPSEC_ADM_OP_NN_IDX, BPSEC_ADM_OP_NN_STR, "BPSEC", "2017-08-17");
	oid_nn_add_parm(BPSEC_ADM_ROOT_NN_IDX, BPSEC_ADM_ROOT_NN_STR, "BPSEC", "2017-08-17");

	/* Step 2: Register Metadata Information. */
	adm_add_edd(mid_from_value(ADM_BPSEC_META_NAME_MID), AMP_TYPE_STR, 0, NULL, adm_print_string, adm_size_string);
	names_add_name("NAME", "The human-readable name of the ADM.", ADM_BPSEC, ADM_BPSEC_META_NAME_MID);
	adm_add_edd(mid_from_value(ADM_BPSEC_META_NAMESPACE_MID), AMP_TYPE_STR, 0, NULL, adm_print_string, adm_size_string);
	names_add_name("NAMESPACE", "The namespace of the ADM.", ADM_BPSEC, ADM_BPSEC_META_NAMESPACE_MID);
	adm_add_edd(mid_from_value(ADM_BPSEC_META_VERSION_MID), AMP_TYPE_STR, 0, NULL, adm_print_string, adm_size_string);
	names_add_name("VERSION", "The version of the ADM.", ADM_BPSEC, ADM_BPSEC_META_VERSION_MID);
	adm_add_edd(mid_from_value(ADM_BPSEC_META_ORGANIZATION_MID), AMP_TYPE_STR, 0, NULL, adm_print_string, adm_size_string);
	names_add_name("ORGANIZATION", "The name of the issuing organization of the ADM.", ADM_BPSEC, ADM_BPSEC_META_ORGANIZATION_MID);
}


void adm_bpsec_init_ops()
{
}


void adm_bpsec_init_reports()
{
	Lyst rpt = NULL;
	rpttpl_item_t *cur_item = NULL;
	uint32_t used = 0;

	rpt = lyst_create();
	lyst_insert_last(rpt,rpttpl_item_create(mid_from_value(ADM_BPSEC_EDD_NUM_GOOD_TX_BCB_BLK_MID), 0));
	lyst_insert_last(rpt,rpttpl_item_create(mid_from_value(ADM_BPSEC_EDD_NUM_BAD_TX_BCB_BLK_MID), 0));
	lyst_insert_last(rpt,rpttpl_item_create(mid_from_value(ADM_BPSEC_EDD_NUM_GOOD_RX_BCB_BLK_MID), 0));
	lyst_insert_last(rpt,rpttpl_item_create(mid_from_value(ADM_BPSEC_EDD_NUM_BAD_RX_BCB_BLK_MID), 0));
	lyst_insert_last(rpt,rpttpl_item_create(mid_from_value(ADM_BPSEC_EDD_NUM_MISSING_RX_BCB_BLKS_MID), 0));
	lyst_insert_last(rpt,rpttpl_item_create(mid_from_value(ADM_BPSEC_EDD_NUM_FWD_BCB_BLKS_MID), 0));
	lyst_insert_last(rpt,rpttpl_item_create(mid_from_value(ADM_BPSEC_EDD_NUM_GOOD_TX_BCB_BYTES_MID), 0));
	lyst_insert_last(rpt,rpttpl_item_create(mid_from_value(ADM_BPSEC_EDD_NUM_BAD_TX_BCB_BYTES_MID), 0));
	lyst_insert_last(rpt,rpttpl_item_create(mid_from_value(ADM_BPSEC_EDD_NUM_GOOD_RX_BCB_BYTES_MID), 0));
	lyst_insert_last(rpt,rpttpl_item_create(mid_from_value(ADM_BPSEC_EDD_NUM_BAD_RX_BCB_BYTES_MID), 0));
	lyst_insert_last(rpt,rpttpl_item_create(mid_from_value(ADM_BPSEC_EDD_NUM_MISSING_RX_BCB_BYTES_MID), 0));
	lyst_insert_last(rpt,rpttpl_item_create(mid_from_value(ADM_BPSEC_EDD_NUM_FWD_BCB_BYTES_MID), 0));
	lyst_insert_last(rpt,rpttpl_item_create(mid_from_value(ADM_BPSEC_EDD_NUM_GOOD_TX_BIB_BLKS_MID), 0));
	lyst_insert_last(rpt,rpttpl_item_create(mid_from_value(ADM_BPSEC_EDD_NUM_BAD_TX_BIB_BLKS_MID), 0));
	lyst_insert_last(rpt,rpttpl_item_create(mid_from_value(ADM_BPSEC_EDD_NUM_GOOD_RX_BIB_BLKS_MID), 0));
	lyst_insert_last(rpt,rpttpl_item_create(mid_from_value(ADM_BPSEC_EDD_NUM_BAD_RX_BIB_BLKS_MID), 0));
	lyst_insert_last(rpt,rpttpl_item_create(mid_from_value(ADM_BPSEC_EDD_NUM_MISS_RX_BIB_BLKS_MID), 0));
	lyst_insert_last(rpt,rpttpl_item_create(mid_from_value(ADM_BPSEC_EDD_NUM_FWD_BIB_BLKS_MID), 0));
	lyst_insert_last(rpt,rpttpl_item_create(mid_from_value(ADM_BPSEC_EDD_NUM_GOOD_TX_BIB_BYTES_MID), 0));
	lyst_insert_last(rpt,rpttpl_item_create(mid_from_value(ADM_BPSEC_EDD_NUM_BAD_TX_BIB_BYTES_MID), 0));
	lyst_insert_last(rpt,rpttpl_item_create(mid_from_value(ADM_BPSEC_EDD_NUM_GOOD_RX_BIB_BYTES_MID), 0));
	lyst_insert_last(rpt,rpttpl_item_create(mid_from_value(ADM_BPSEC_EDD_NUM_BAD_RX_BIB_BYTES_MID), 0));
	lyst_insert_last(rpt,rpttpl_item_create(mid_from_value(ADM_BPSEC_EDD_NUM_MISS_RX_BIB_BYTES_MID), 0));
	lyst_insert_last(rpt,rpttpl_item_create(mid_from_value(ADM_BPSEC_EDD_NUM_FWD_BIB_BYTES_MID), 0));
	lyst_insert_last(rpt,rpttpl_item_create(mid_from_value(ADM_BPSEC_EDD_LAST_UPDATE_MID), 0));
	lyst_insert_last(rpt,rpttpl_item_create(mid_from_value(ADM_BPSEC_EDD_NUM_KNOWN_KEYS_MID), 0));
	lyst_insert_last(rpt,rpttpl_item_create(mid_from_value(ADM_BPSEC_EDD_KEY_NAMES_MID), 0));
	lyst_insert_last(rpt,rpttpl_item_create(mid_from_value(ADM_BPSEC_EDD_CIPHERSUITE_NAMES_MID), 0));
	lyst_insert_last(rpt,rpttpl_item_create(mid_from_value(ADM_BPSEC_EDD_RULE_SOURCE_MID), 0));

	adm_add_rpttpl(mid_from_value(ADM_BPSEC_RPT_FULL_REPORT_MID), rpt);

	names_add_name("FULL_REPORT", "all known meta-data, externally defined data, and variables", ADM_BPSEC, ADM_BPSEC_RPT_FULL_REPORT_MID);



	rpt = lyst_create();

	cur_item = rpttpl_item_create(mid_from_value(ADM_BPSEC_EDD_NUM_GOOD_TX_BCB_BLKS_SRC_MID),1);
	rpttpl_item_add_parm_map(cur_item, 1, 1);
	lyst_insert_last(rpt,cur_item);

	cur_item = rpttpl_item_create(mid_from_value(ADM_BPSEC_EDD_NUM_BAD_TX_BCB_BLKS_SRC_MID),1);
	rpttpl_item_add_parm_map(cur_item, 1, 1);
	lyst_insert_last(rpt,cur_item);

	cur_item = rpttpl_item_create(mid_from_value(ADM_BPSEC_EDD_NUM_GOOD_RX_BCB_BLKS_SRC_MID),1);
	rpttpl_item_add_parm_map(cur_item, 1, 1);
	lyst_insert_last(rpt,cur_item);

	cur_item = rpttpl_item_create(mid_from_value(ADM_BPSEC_EDD_NUM_BAD_RX_BCB_BLKS_SRC_MID),1);
	rpttpl_item_add_parm_map(cur_item, 1, 1);
	lyst_insert_last(rpt,cur_item);

	cur_item = rpttpl_item_create(mid_from_value(ADM_BPSEC_EDD_NUM_MISSING_RX_BCB_BLKS_SRC_MID),1);
	rpttpl_item_add_parm_map(cur_item, 1, 1);
	lyst_insert_last(rpt,cur_item);

	cur_item = rpttpl_item_create(mid_from_value(ADM_BPSEC_EDD_NUM_FWD_BCB_BLKS_SRC_MID),1);
	rpttpl_item_add_parm_map(cur_item, 1, 1);
	lyst_insert_last(rpt,cur_item);

	cur_item = rpttpl_item_create(mid_from_value(ADM_BPSEC_EDD_NUM_GOOD_TX_BCB_BYTES_SRC_MID),1);
	rpttpl_item_add_parm_map(cur_item, 1, 1);
	lyst_insert_last(rpt,cur_item);

	cur_item = rpttpl_item_create(mid_from_value(ADM_BPSEC_EDD_NUM_BAD_TX_BCB_BYTES_SRC_MID),1);
	rpttpl_item_add_parm_map(cur_item, 1, 1);
	lyst_insert_last(rpt,cur_item);

	cur_item = rpttpl_item_create(mid_from_value(ADM_BPSEC_EDD_NUM_GOOD_RX_BCB_BYTES_SRC_MID),1);
	rpttpl_item_add_parm_map(cur_item, 1, 1);
	lyst_insert_last(rpt,cur_item);

	cur_item = rpttpl_item_create(mid_from_value(ADM_BPSEC_EDD_NUM_BAD_RX_BCB_BYTES_SRC_MID),1);
	rpttpl_item_add_parm_map(cur_item, 1, 1);
	lyst_insert_last(rpt,cur_item);

	cur_item = rpttpl_item_create(mid_from_value(ADM_BPSEC_EDD_NUM_MISSING_RX_BCB_BYTES_SRC_MID),1);
	rpttpl_item_add_parm_map(cur_item, 1, 1);
	lyst_insert_last(rpt,cur_item);

	cur_item = rpttpl_item_create(mid_from_value(ADM_BPSEC_EDD_NUM_FWD_BCB_BYTES_SRC_MID),1);
	rpttpl_item_add_parm_map(cur_item, 1, 1);
	lyst_insert_last(rpt,cur_item);

	cur_item = rpttpl_item_create(mid_from_value(ADM_BPSEC_EDD_NUM_GOOD_TX_BIB_BLKS_SRC_MID),1);
	rpttpl_item_add_parm_map(cur_item, 1, 1);
	lyst_insert_last(rpt,cur_item);

	cur_item = rpttpl_item_create(mid_from_value(ADM_BPSEC_EDD_NUM_BAD_TX_BIB_BLKS_SRC_MID),1);
	rpttpl_item_add_parm_map(cur_item, 1, 1);
	lyst_insert_last(rpt,cur_item);

	cur_item = rpttpl_item_create(mid_from_value(ADM_BPSEC_EDD_NUM_GOOD_RX_BIB_BLKS_SRC_MID),1);
	rpttpl_item_add_parm_map(cur_item, 1, 1);
	lyst_insert_last(rpt,cur_item);

	cur_item = rpttpl_item_create(mid_from_value(ADM_BPSEC_EDD_NUM_BAD_RX_BIB_BLKS_SRC_MID),1);
	rpttpl_item_add_parm_map(cur_item, 1, 1);
	lyst_insert_last(rpt,cur_item);

	cur_item = rpttpl_item_create(mid_from_value(ADM_BPSEC_EDD_NUM_MISS_RX_BIB_BLKS_SRC_MID),1);
	rpttpl_item_add_parm_map(cur_item, 1, 1);
	lyst_insert_last(rpt,cur_item);

	cur_item = rpttpl_item_create(mid_from_value(ADM_BPSEC_EDD_NUM_FWD_BIB_BLKS_SRC_MID),1);
	rpttpl_item_add_parm_map(cur_item, 1, 1);
	lyst_insert_last(rpt,cur_item);

	cur_item = rpttpl_item_create(mid_from_value(ADM_BPSEC_EDD_NUM_GOOD_TX_BIB_BYTES_SRC_MID),1);
	rpttpl_item_add_parm_map(cur_item, 1, 1);
	lyst_insert_last(rpt,cur_item);

	cur_item = rpttpl_item_create(mid_from_value(ADM_BPSEC_EDD_NUM_BAD_TX_BIB_BYTES_SRC_MID),1);
	rpttpl_item_add_parm_map(cur_item, 1, 1);
	lyst_insert_last(rpt,cur_item);

	cur_item = rpttpl_item_create(mid_from_value(ADM_BPSEC_EDD_NUM_GOOD_RX_BIB_BYTES_SRC_MID),1);
	rpttpl_item_add_parm_map(cur_item, 1, 1);
	lyst_insert_last(rpt,cur_item);

	cur_item = rpttpl_item_create(mid_from_value(ADM_BPSEC_EDD_NUM_BAD_RX_BIB_BYTES_SRC_MID),1);
	rpttpl_item_add_parm_map(cur_item, 1, 1);
	lyst_insert_last(rpt,cur_item);

	cur_item = rpttpl_item_create(mid_from_value(ADM_BPSEC_EDD_NUM_MISSING_RX_BIB_BYTES_SRC_MID),1);
	rpttpl_item_add_parm_map(cur_item, 1, 1);
	lyst_insert_last(rpt,cur_item);

	cur_item = rpttpl_item_create(mid_from_value(ADM_BPSEC_EDD_NUM_FWD_BIB_BYTES_SRC_MID),1);
	rpttpl_item_add_parm_map(cur_item, 1, 1);
	lyst_insert_last(rpt,cur_item);

	cur_item = rpttpl_item_create(mid_from_value(ADM_BPSEC_EDD_LAST_UPDATE_SRC_MID),1);
	rpttpl_item_add_parm_map(cur_item, 1, 1);
	lyst_insert_last(rpt,cur_item);

	cur_item = rpttpl_item_create(mid_from_value(ADM_BPSEC_EDD_LAST_RESET_MID),1);
	rpttpl_item_add_parm_map(cur_item, 1, 1);
	lyst_insert_last(rpt,cur_item);

	adm_add_rpttpl(mid_from_value(ADM_BPSEC_RPT_SOURCE_REPORT_MID), rpt);

	names_add_name("SOURCE_REPORT", "security info by source", ADM_BPSEC, ADM_BPSEC_RPT_SOURCE_REPORT_MID);
	UI_ADD_PARMSPEC_1(ADM_BPSEC_RPT_SOURCE_REPORT_MID, "source_name", AMP_TYPE_STR);

}

#endif // _HAVE_BPSEC_ADM_
