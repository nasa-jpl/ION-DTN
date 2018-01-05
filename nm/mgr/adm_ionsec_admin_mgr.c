/****************************************************************************
 **
 ** File Name: adm_ionsec_admin_mgr.c
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
 **  2018-01-05  AUTO             Auto-generated c file 
 **
 ****************************************************************************/


#include "ion.h"
#include "lyst.h"
#include "platform.h"
#include "../shared/adm/adm_ionsec_admin.h"
#include "../shared/utils/utils.h"
#include "../shared/primitives/def.h"
#include "../shared/primitives/nn.h"
#include "../shared/primitives/report.h"
#include "../shared/primitives/blob.h"
#include "nm_mgr_names.h"
#include "nm_mgr_ui.h"

#define _HAVE_IONSEC_ADMIN_ADM_
#ifdef _HAVE_IONSEC_ADMIN_ADM_

void adm_ionsec_admin_init()
{
	adm_ionsec_admin_init_edd();
	adm_ionsec_admin_init_variables();
	adm_ionsec_admin_init_controls();
	adm_ionsec_admin_init_constants();
	adm_ionsec_admin_init_macros();
	adm_ionsec_admin_init_metadata();
	adm_ionsec_admin_init_ops();
	adm_ionsec_admin_init_reports();
}


void adm_ionsec_admin_init_edd()
{
}


void adm_ionsec_admin_init_variables()
{
}


void adm_ionsec_admin_init_controls()
{
	adm_add_ctrl(ADM_IONSEC_ADMIN_CTRL_KEY_ADD_MID, NULL);
	names_add_name("KEY_ADD", "This control adds a named key value to the security policy database. The content of file_name is taken as the value of the key. Named keys can be referenced by other elements of the security policy database.", ADM_IONSEC_ADMIN, ADM_IONSEC_ADMIN_CTRL_KEY_ADD_MID);
	UI_ADD_PARMSPEC_2(ADM_IONSEC_ADMIN_CTRL_KEY_ADD_MID, "key_name", AMP_TYPE_STR, "file_name", AMP_TYPE_BLOB);

	adm_add_ctrl(ADM_IONSEC_ADMIN_CTRL_KEY_CHANGE_MID, NULL);
	names_add_name("KEY_CHANGE", "This control changes the value of the named key, obtaining the new key value from the content of file_name.", ADM_IONSEC_ADMIN, ADM_IONSEC_ADMIN_CTRL_KEY_CHANGE_MID);
	UI_ADD_PARMSPEC_2(ADM_IONSEC_ADMIN_CTRL_KEY_CHANGE_MID, "key_name", AMP_TYPE_STR, "file_name", AMP_TYPE_BLOB);

	adm_add_ctrl(ADM_IONSEC_ADMIN_CTRL_KEY_DEL_MID, NULL);
	names_add_name("KEY_DEL", "This control deletes the key identified by name.", ADM_IONSEC_ADMIN, ADM_IONSEC_ADMIN_CTRL_KEY_DEL_MID);
	UI_ADD_PARMSPEC_1(ADM_IONSEC_ADMIN_CTRL_KEY_DEL_MID, "key_name", AMP_TYPE_STR);

	adm_add_ctrl(ADM_IONSEC_ADMIN_CTRL_LTP_RX_RULE_ADD_MID, NULL);
	names_add_name("LTP_RX_RULE_ADD", "This control adds a rule specifying the manner in which LTP segment authentication will be applied to LTP segments recieved from the indicated LTP engine. A segment from the indicated LTP engine will only be deemed authentic if it contains an authentication extension computed via the ciphersuite identified by ciphersuite_nbr using the applicable key value. If ciphersuite_nbr is 255 then the applicable key value is a hard-coded constant and key_name must be omitted; otherwise key_name is required and the applicable key value is the current value of the key named key_name in the local security policy database. Valid values of ciphersuite_nbr are: 0: HMAC-SHA1-80 1: RSA-SHA256 255: NULL", ADM_IONSEC_ADMIN, ADM_IONSEC_ADMIN_CTRL_LTP_RX_RULE_ADD_MID);
	UI_ADD_PARMSPEC_3(ADM_IONSEC_ADMIN_CTRL_LTP_RX_RULE_ADD_MID, "ltp_engine_id", AMP_TYPE_UINT, "ciphersuite_nbr", AMP_TYPE_UINT, "key_name", AMP_TYPE_STR);

	adm_add_ctrl(ADM_IONSEC_ADMIN_CTRL_LTP_RX_RULE_CHANGE_MID, NULL);
	names_add_name("LTP_RX_RULE_CHANGE", "This control changes the parameters of the LTP segment authentication rule for the indicated LTP engine.", ADM_IONSEC_ADMIN, ADM_IONSEC_ADMIN_CTRL_LTP_RX_RULE_CHANGE_MID);
	UI_ADD_PARMSPEC_3(ADM_IONSEC_ADMIN_CTRL_LTP_RX_RULE_CHANGE_MID, "ltp_engine_id", AMP_TYPE_UINT, "ciphersuite_nbr", AMP_TYPE_UINT, "key_name", AMP_TYPE_STR);

	adm_add_ctrl(ADM_IONSEC_ADMIN_CTRL_LTP_RX_RULE_DEL_MID, NULL);
	names_add_name("LTP_RX_RULE_DEL", "This control deletes the LTP segment authentication rule for the indicated LTP engine.", ADM_IONSEC_ADMIN, ADM_IONSEC_ADMIN_CTRL_LTP_RX_RULE_DEL_MID);
	UI_ADD_PARMSPEC_1(ADM_IONSEC_ADMIN_CTRL_LTP_RX_RULE_DEL_MID, "ltp_engine_id", AMP_TYPE_UINT);

	adm_add_ctrl(ADM_IONSEC_ADMIN_CTRL_LTP_TX_RULE_ADD_MID, NULL);
	names_add_name("LTP_TX_RULE_ADD", "This control adds a rule specifying the manner in which LTP segments transmitted to the indicated LTP engine must be signed. Signing a segment destined for the indicated LTP engine entails computing an authentication extension via the ciphersuite identified by ciphersuite_nbr using the applicable key value. If ciphersuite_nbr is 255 then the applicable key value is a hard-coded constant and key_name must be omitted; otherwise key_name is required and the applicable key value is the current value of the key named key_name in the local security policy database. Valid values of ciphersuite_nbr are: 0:HMAC_SHA1-80 1: RSA_SHA256 255: NULL", ADM_IONSEC_ADMIN, ADM_IONSEC_ADMIN_CTRL_LTP_TX_RULE_ADD_MID);
	UI_ADD_PARMSPEC_3(ADM_IONSEC_ADMIN_CTRL_LTP_TX_RULE_ADD_MID, "ltp_engine_id", AMP_TYPE_UINT, "ciphersuite_nbr", AMP_TYPE_UINT, "key_name", AMP_TYPE_STR);

	adm_add_ctrl(ADM_IONSEC_ADMIN_CTRL_LTP_TX_RULE_CHANGE_MID, NULL);
	names_add_name("LTP_TX_RULE_CHANGE", "This control changes the parameters of the LTP segment signing rule for the indicated LTP engine.", ADM_IONSEC_ADMIN, ADM_IONSEC_ADMIN_CTRL_LTP_TX_RULE_CHANGE_MID);
	UI_ADD_PARMSPEC_3(ADM_IONSEC_ADMIN_CTRL_LTP_TX_RULE_CHANGE_MID, "ltp_engine_id", AMP_TYPE_UINT, "ciphersuite_nbr", AMP_TYPE_UINT, "key_name", AMP_TYPE_STR);

	adm_add_ctrl(ADM_IONSEC_ADMIN_CTRL_LTP_TX_RULE_DEL_MID, NULL);
	names_add_name("LTP_TX_RULE_DEL", "This control deletes the LTP segment signing rule for the indicated LTP engine.", ADM_IONSEC_ADMIN, ADM_IONSEC_ADMIN_CTRL_LTP_TX_RULE_DEL_MID);
	UI_ADD_PARMSPEC_1(ADM_IONSEC_ADMIN_CTRL_LTP_TX_RULE_DEL_MID, "ltp_engine_id", AMP_TYPE_UINT);

}


void adm_ionsec_admin_init_constants()
{
}


void adm_ionsec_admin_init_macros()
{
}


void adm_ionsec_admin_init_metadata()
{
	/* Step 1: Register Nicknames */
	oid_nn_add_parm(IONSEC_ADMIN_ADM_META_NN_IDX, IONSEC_ADMIN_ADM_META_NN_STR, "IONSEC_ADMIN", "2017-08-17");
	oid_nn_add_parm(IONSEC_ADMIN_ADM_EDD_NN_IDX, IONSEC_ADMIN_ADM_EDD_NN_STR, "IONSEC_ADMIN", "2017-08-17");
	oid_nn_add_parm(IONSEC_ADMIN_ADM_VAR_NN_IDX, IONSEC_ADMIN_ADM_VAR_NN_STR, "IONSEC_ADMIN", "2017-08-17");
	oid_nn_add_parm(IONSEC_ADMIN_ADM_RPT_NN_IDX, IONSEC_ADMIN_ADM_RPT_NN_STR, "IONSEC_ADMIN", "2017-08-17");
	oid_nn_add_parm(IONSEC_ADMIN_ADM_CTRL_NN_IDX, IONSEC_ADMIN_ADM_CTRL_NN_STR, "IONSEC_ADMIN", "2017-08-17");
	oid_nn_add_parm(IONSEC_ADMIN_ADM_CONST_NN_IDX, IONSEC_ADMIN_ADM_CONST_NN_STR, "IONSEC_ADMIN", "2017-08-17");
	oid_nn_add_parm(IONSEC_ADMIN_ADM_MACRO_NN_IDX, IONSEC_ADMIN_ADM_MACRO_NN_STR, "IONSEC_ADMIN", "2017-08-17");
	oid_nn_add_parm(IONSEC_ADMIN_ADM_OP_NN_IDX, IONSEC_ADMIN_ADM_OP_NN_STR, "IONSEC_ADMIN", "2017-08-17");
	oid_nn_add_parm(IONSEC_ADMIN_ADM_ROOT_NN_IDX, IONSEC_ADMIN_ADM_ROOT_NN_STR, "IONSEC_ADMIN", "2017-08-17");

	/* Step 2: Register Metadata Information. */
	adm_add_edd(ADM_IONSEC_ADMIN_META_NAME_MID, AMP_TYPE_STR, 0, NULL, adm_print_string, adm_size_string);
	names_add_name("NAME", "The human-readable name of the ADM.", ADM_IONSEC_ADMIN, ADM_IONSEC_ADMIN_META_NAME_MID);
	adm_add_edd(ADM_IONSEC_ADMIN_META_NAMESPACE_MID, AMP_TYPE_STR, 0, NULL, adm_print_string, adm_size_string);
	names_add_name("NAMESPACE", "The namespace of the ADM.", ADM_IONSEC_ADMIN, ADM_IONSEC_ADMIN_META_NAMESPACE_MID);
	adm_add_edd(ADM_IONSEC_ADMIN_META_VERSION_MID, AMP_TYPE_STR, 0, NULL, adm_print_string, adm_size_string);
	names_add_name("VERSION", "The version of the ADM.", ADM_IONSEC_ADMIN, ADM_IONSEC_ADMIN_META_VERSION_MID);
	adm_add_edd(ADM_IONSEC_ADMIN_META_ORGANIZATION_MID, AMP_TYPE_STR, 0, NULL, adm_print_string, adm_size_string);
	names_add_name("ORGANIZATION", "The name of the issuing organization of the ADM.", ADM_IONSEC_ADMIN, ADM_IONSEC_ADMIN_META_ORGANIZATION_MID);
}


void adm_ionsec_admin_init_ops()
{
}


void adm_ionsec_admin_init_reports()
{
	uint32_t used= 0;
	Lyst rpt = NULL;
}

#endif // _HAVE_IONSEC_ADMIN_ADM_
