/****************************************************************************
 **
 ** File Name: adm_ionsec_admin.h
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
 **  2018-01-05  AUTO             Auto-generated header file 
 **
 ****************************************************************************/


#ifndef ADM_IONSEC_ADMIN_H_
#define ADM_IONSEC_ADMIN_H_
#define _HAVE_IONSEC_ADMIN_ADM_
#ifdef _HAVE_IONSEC_ADMIN_ADM_

#include "lyst.h"
#include "../utils/nm_types.h"
#include "adm.h"

/*
 * +----------------------------------------------------------------------------------------------------------+
 * |			              ADM TEMPLATE DOCUMENTATION                                              +
 * +----------------------------------------------------------------------------------------------------------+
 *
 * ADM ROOT STRING:arn:DTN:ionsec_admin
 */

/*
 * +----------------------------------------------------------------------------------------------------------+
 * |				             AGENT NICKNAME DEFINITIONS                                       +
 * +----------------------------------------------------------------------------------------------------------+
 */
#define IONSEC_ADMIN_ADM_META_NN_IDX 70
#define IONSEC_ADMIN_ADM_META_NN_STR "70"

#define IONSEC_ADMIN_ADM_EDD_NN_IDX 71
#define IONSEC_ADMIN_ADM_EDD_NN_STR "71"

#define IONSEC_ADMIN_ADM_VAR_NN_IDX 72
#define IONSEC_ADMIN_ADM_VAR_NN_STR "72"

#define IONSEC_ADMIN_ADM_RPT_NN_IDX 73
#define IONSEC_ADMIN_ADM_RPT_NN_STR "73"

#define IONSEC_ADMIN_ADM_CTRL_NN_IDX 74
#define IONSEC_ADMIN_ADM_CTRL_NN_STR "74"

#define IONSEC_ADMIN_ADM_CONST_NN_IDX 75
#define IONSEC_ADMIN_ADM_CONST_NN_STR "75"

#define IONSEC_ADMIN_ADM_MACRO_NN_IDX 76
#define IONSEC_ADMIN_ADM_MACRO_NN_STR "76"

#define IONSEC_ADMIN_ADM_OP_NN_IDX 77
#define IONSEC_ADMIN_ADM_OP_NN_STR "77"

#define IONSEC_ADMIN_ADM_TBL_NN_IDX 78
#define IONSEC_ADMIN_ADM_TBL_NN_STR "78"

#define IONSEC_ADMIN_ADM_ROOT_NN_IDX 79
#define IONSEC_ADMIN_ADM_ROOT_NN_STR "79"


/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |		                    IONSEC_ADMIN META-DATA DEFINITIONS                                                          
 * +-----------------------------------------------------------------------------------------------------------+
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |             NAME            |    MID     |              DESCRIPTION                         |     TYPE    |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |name                         |0x87460100  |The human-readable name of the ADM.               |STR          |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |namespace                    |0x87460101  |The namespace of the ADM.                         |STR          |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |version                      |0x87460102  |The version of the ADM.                           |STR          |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |organization                 |0x87460103  |The name of the issuing organization of the ADM.  |STR          |
   +-----------------------------+------------+--------------------------------------------------+-------------+
 */
// "name"
#define ADM_IONSEC_ADMIN_META_NAME_MID 0x87460100
// "namespace"
#define ADM_IONSEC_ADMIN_META_NAMESPACE_MID 0x87460101
// "version"
#define ADM_IONSEC_ADMIN_META_VERSION_MID 0x87460102
// "organization"
#define ADM_IONSEC_ADMIN_META_ORGANIZATION_MID 0x87460103


/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |		                    IONSEC_ADMIN EXTERNALLY DEFINED DATA DEFINITIONS                                               
 * +-----------------------------------------------------------------------------------------------------------+
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |             NAME            |    MID     |              DESCRIPTION                         |     TYPE    |
   +-----------------------------+------------+--------------------------------------------------+-------------+
 */


/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |		                    IONSEC_ADMIN VARIABLE DEFINITIONS                                                          
 * +-----------------------------------------------------------------------------------------------------------+
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |             NAME            |    MID     |              DESCRIPTION                         |     TYPE    |
   +-----------------------------+------------+--------------------------------------------------+-------------+
 */


/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |		                    IONSEC_ADMIN REPORT DEFINITIONS                                                           
 * +-----------------------------------------------------------------------------------------------------------+
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |             NAME            |    MID     |              DESCRIPTION                         |     TYPE    |
   +-----------------------------+------------+--------------------------------------------------+-------------+
 */


/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |		                    IONSEC_ADMIN CONTROL DEFINITIONS                                                         
 * +-----------------------------------------------------------------------------------------------------------+
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |             NAME            |    MID     |              DESCRIPTION                         |     TYPE    |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |key_add                      |0xc34a0100  |This control adds a named key value to the securit|             |
   |                             |            |y policy database. The content of file_name is tak|             |
   |                             |            |en as the value of the key. Named keys can be refe|             |
   |                             |            |renced by other elements of the security policy da|             |
   |                             |            |tabase.                                           |             |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |key_change                   |0xc34a0101  |This control changes the value of the named key, o|             |
   |                             |            |btaining the new key value from the content of fil|             |
   |                             |            |e_name.                                           |             |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |key_del                      |0xc34a0102  |This control deletes the key identified by name.  |             |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |ltp_rx_rule_add              |0xc34a0103  |This control adds a rule specifying the manner in |             |
   |                             |            |which LTP segment authentication will be applied t|             |
   |                             |            |o LTP segments recieved from the indicated LTP eng|             |
   |                             |            |ine. A segment from the indicated LTP engine will |             |
   |                             |            |only be deemed authentic if it contains an authent|             |
   |                             |            |ication extension computed via the ciphersuite ide|             |
   |                             |            |ntified by ciphersuite_nbr using the applicable ke|             |
   |                             |            |y value. If ciphersuite_nbr is 255 then the applic|             |
   |                             |            |able key value is a hard-coded constant and key_na|             |
   |                             |            |me must be omitted; otherwise key_name is required|             |
   |                             |            | and the applicable key value is the current value|             |
   |                             |            | of the key named key_name in the local security p|             |
   |                             |            |olicy database. Valid values of ciphersuite_nbr ar|             |
   |                             |            |e: 0: HMAC-SHA1-80 1: RSA-SHA256 255: NULL        |             |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |ltp_rx_rule_change           |0xc34a0104  |This control changes the parameters of the LTP seg|             |
   |                             |            |ment authentication rule for the indicated LTP eng|             |
   |                             |            |ine.                                              |             |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |ltp_rx_rule_del              |0xc34a0105  |This control deletes the LTP segment authenticatio|             |
   |                             |            |n rule for the indicated LTP engine.              |             |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |ltp_tx_rule_add              |0xc34a0106  |This control adds a rule specifying the manner in |             |
   |                             |            |which LTP segments transmitted to the indicated LT|             |
   |                             |            |P engine must be signed. Signing a segment destine|             |
   |                             |            |d for the indicated LTP engine entails computing a|             |
   |                             |            |n authentication extension via the ciphersuite ide|             |
   |                             |            |ntified by ciphersuite_nbr using the applicable ke|             |
   |                             |            |y value. If ciphersuite_nbr is 255 then the applic|             |
   |                             |            |able key value is a hard-coded constant and key_na|             |
   |                             |            |me must be omitted; otherwise key_name is required|             |
   |                             |            | and the applicable key value is the current value|             |
   |                             |            | of the key named key_name in the local security p|             |
   |                             |            |olicy database. Valid values of ciphersuite_nbr ar|             |
   |                             |            |e: 0:HMAC_SHA1-80 1: RSA_SHA256 255: NULL         |             |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |ltp_tx_rule_change           |0xc34a0107  |This control changes the parameters of the LTP seg|             |
   |                             |            |ment signing rule for the indicated LTP engine.   |             |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |ltp_tx_rule_del              |0xc34a0108  |This control deletes the LTP segment signing rule |             |
   |                             |            |for the indicated LTP engine.                     |             |
   +-----------------------------+------------+--------------------------------------------------+-------------+
 */
#define ADM_IONSEC_ADMIN_CTRL_KEY_ADD_MID 0xc34a0100
#define ADM_IONSEC_ADMIN_CTRL_KEY_CHANGE_MID 0xc34a0101
#define ADM_IONSEC_ADMIN_CTRL_KEY_DEL_MID 0xc34a0102
#define ADM_IONSEC_ADMIN_CTRL_LTP_RX_RULE_ADD_MID 0xc34a0103
#define ADM_IONSEC_ADMIN_CTRL_LTP_RX_RULE_CHANGE_MID 0xc34a0104
#define ADM_IONSEC_ADMIN_CTRL_LTP_RX_RULE_DEL_MID 0xc34a0105
#define ADM_IONSEC_ADMIN_CTRL_LTP_TX_RULE_ADD_MID 0xc34a0106
#define ADM_IONSEC_ADMIN_CTRL_LTP_TX_RULE_CHANGE_MID 0xc34a0107
#define ADM_IONSEC_ADMIN_CTRL_LTP_TX_RULE_DEL_MID 0xc34a0108


/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |		                    IONSEC_ADMIN CONSTANT DEFINITIONS                                                         
 * +-----------------------------------------------------------------------------------------------------------+
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |             NAME            |    MID     |              DESCRIPTION                         |     TYPE    |
   +-----------------------------+------------+--------------------------------------------------+-------------+
 */


/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |		                    IONSEC_ADMIN MACRO DEFINITIONS                                                            
 * +-----------------------------------------------------------------------------------------------------------+
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |             NAME            |    MID     |              DESCRIPTION                         |     TYPE    |
   +-----------------------------+------------+--------------------------------------------------+-------------+
 */


/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |		                    IONSEC_ADMIN OPERATOR DEFINITIONS                                                          
 * +-----------------------------------------------------------------------------------------------------------+
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |             NAME            |    MID     |              DESCRIPTION                         |     TYPE    |
   +-----------------------------+------------+--------------------------------------------------+-------------+
 */

/* Initialization functions. */
void adm_ionsec_admin_init();
void adm_ionsec_admin_init_edd();
void adm_ionsec_admin_init_variables();
void adm_ionsec_admin_init_controls();
void adm_ionsec_admin_init_constants();
void adm_ionsec_admin_init_macros();
void adm_ionsec_admin_init_metadata();
void adm_ionsec_admin_init_ops();
void adm_ionsec_admin_init_reports();
#endif /* _HAVE_IONSEC_ADMIN_ADM_ */
#endif //ADM_IONSEC_ADMIN_H_