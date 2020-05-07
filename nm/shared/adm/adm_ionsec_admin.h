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
 **  2020-04-13  AUTO             Auto-generated header file 
 **
 ****************************************************************************/


#ifndef ADM_IONSEC_ADMIN_H_
#define ADM_IONSEC_ADMIN_H_
#define _HAVE_DTN_ION_IONSECADMIN_ADM_
#ifdef _HAVE_DTN_ION_IONSECADMIN_ADM_

#include "shared/utils/nm_types.h"
#include "shared/adm/adm.h"


/*
 * +---------------------------------------------------------------------------------------------+
 * |                                 ADM TEMPLATE DOCUMENTATION                                  +
 * +---------------------------------------------------------------------------------------------+
 *
 * ADM ROOT STRING:DTN/ION/ionsecadmin
 */
#define ADM_ENUM_DTN_ION_IONSECADMIN 8
/*
 * +---------------------------------------------------------------------------------------------+
 * |                                 AGENT NICKNAME DEFINITIONS                                  +
 * +---------------------------------------------------------------------------------------------+
 */

/*
 * +---------------------------------------------------------------------------------------------+
 * |                          DTN_ION_IONSECADMIN META-DATA DEFINITIONS                          +
 * +---------------------------------------------------------------------------------------------+
 * |        NAME         |             DESCRIPTION              | TYPE  |         VALUE          |
 * +---------------------+--------------------------------------+-------+------------------------+
 * |name                 |The human-readable name of the ADM.   |STR    |ionsec_admin            |
 * +---------------------+--------------------------------------+-------+------------------------+
 * |namespace            |The namespace of the ADM.             |STR    |DTN/ION/ionsecadmin     |
 * +---------------------+--------------------------------------+-------+------------------------+
 * |version              |The version of the ADM.               |STR    |v0.0                    |
 * +---------------------+--------------------------------------+-------+------------------------+
 * |organization         |The name of the issuing organization o|       |                        |
 * |                     |f the ADM.                            |STR    |JHUAPL                  |
 * +---------------------+--------------------------------------+-------+------------------------+
 */
// "name"
#define DTN_ION_IONSECADMIN_META_NAME 0x00
// "namespace"
#define DTN_ION_IONSECADMIN_META_NAMESPACE 0x01
// "version"
#define DTN_ION_IONSECADMIN_META_VERSION 0x02
// "organization"
#define DTN_ION_IONSECADMIN_META_ORGANIZATION 0x03


/*
 * +---------------------------------------------------------------------------------------------+
 * |                   DTN_ION_IONSECADMIN EXTERNALLY DEFINED DATA DEFINITIONS                   +
 * +---------------------------------------------------------------------------------------------+
 * |        NAME         |             DESCRIPTION              | TYPE  |
 * +---------------------+--------------------------------------+-------+
 */


/*
 * +---------------------------------------------------------------------------------------------+
 * |                          DTN_ION_IONSECADMIN VARIABLE DEFINITIONS                           +
 * +---------------------------------------------------------------------------------------------+
 * |        NAME         |             DESCRIPTION              | TYPE  |
 * +---------------------+--------------------------------------+-------+
 */


/*
 * +---------------------------------------------------------------------------------------------+
 * |                           DTN_ION_IONSECADMIN REPORT DEFINITIONS                            +
 * +---------------------------------------------------------------------------------------------+
 * |        NAME         |             DESCRIPTION              | TYPE  |
 * +---------------------+--------------------------------------+-------+
 */


/*
 * +---------------------------------------------------------------------------------------------+
 * |                            DTN_ION_IONSECADMIN TABLE DEFINITIONS                            +
 * +---------------------------------------------------------------------------------------------+
 * |        NAME         |             DESCRIPTION              | TYPE  |
 * +---------------------+--------------------------------------+-------+
 * |ltp_rx_rules         |This table lists all LTP segment authe|       |
 * |                     |ntication rulesin the security policy |       |
 * |                     |database.                             |       |
 * +---------------------+--------------------------------------+-------+
 * |ltp_tx_rules         |This table lists all LTP segment signi|       |
 * |                     |ng rules in the security policy databa|       |
 * |                     |se.                                   |       |
 * +---------------------+--------------------------------------+-------+
 */
#define DTN_ION_IONSECADMIN_TBLT_LTP_RX_RULES 0x00
#define DTN_ION_IONSECADMIN_TBLT_LTP_TX_RULES 0x01


/*
 * +---------------------------------------------------------------------------------------------+
 * |                           DTN_ION_IONSECADMIN CONTROL DEFINITIONS                           +
 * +---------------------------------------------------------------------------------------------+
 * |        NAME         |             DESCRIPTION              | TYPE  |
 * +---------------------+--------------------------------------+-------+
 * |key_add              |This control adds a named key value to|       |
 * |                     | the security policy database. The con|       |
 * |                     |tent of file_name is taken as the valu|       |
 * |                     |e of the key.Named keys can be referen|       |
 * |                     |ced by other elements of thesecurity p|       |
 * |                     |olicy database.                       |       |
 * +---------------------+--------------------------------------+-------+
 * |key_change           |This control changes the value of the |       |
 * |                     |named key, obtaining the new key value|       |
 * |                     | from the content of file_name.       |       |
 * +---------------------+--------------------------------------+-------+
 * |key_del              |This control deletes the key identifie|       |
 * |                     |d by name.                            |       |
 * +---------------------+--------------------------------------+-------+
 * |ltp_rx_rule_add      |This control adds a rule specifying th|       |
 * |                     |e manner in which LTP segment authenti|       |
 * |                     |cation will be applied to LTP segments|       |
 * |                     |recieved from the indicated LTP engine|       |
 * |                     |. A segment from the indicated LTP eng|       |
 * |                     |ine will only be deemed authentic if i|       |
 * |                     |t contains an authentication extension|       |
 * |                     | computed via the ciphersuite identifi|       |
 * |                     |ed by ciphersuite_nbr using the applic|       |
 * |                     |able key value. If ciphersuite_nbr is |       |
 * |                     |255 then the applicable key value is a|       |
 * |                     | hard-coded constant and key_name must|       |
 * |                     | be omitted; otherwise key_nameis requ|       |
 * |                     |ired and the applicable key value is t|       |
 * |                     |he current value of the key named key_|       |
 * |                     |name in the local security policy data|       |
 * |                     |base. Valid values of ciphersuite_nbr |       |
 * |                     |are: 0: HMAC-SHA1-80 1: RSA-SHA256 255|       |
 * |                     |: NULL                                |       |
 * +---------------------+--------------------------------------+-------+
 * |ltp_rx_rule_change   |This control changes the parameters of|       |
 * |                     | the LTP segment authentication rule f|       |
 * |                     |or the indicated LTP engine.          |       |
 * +---------------------+--------------------------------------+-------+
 * |ltp_rx_rule_del      |This control deletes the LTP segment a|       |
 * |                     |uthentication rule for the indicated L|       |
 * |                     |TP engine.                            |       |
 * +---------------------+--------------------------------------+-------+
 * |ltp_tx_rule_add      |This control adds a rule specifying th|       |
 * |                     |e manner in which LTP segments transmi|       |
 * |                     |tted to the indicated LTP engine mustb|       |
 * |                     |e signed. Signing a segment destined f|       |
 * |                     |or the indicated LTP engineentails com|       |
 * |                     |puting an authentication extension via|       |
 * |                     | the ciphersuite identified by ciphers|       |
 * |                     |uite_nbr using the applicable key valu|       |
 * |                     |e. If ciphersuite_nbr is 255 then the |       |
 * |                     |applicable key value is a hard-coded c|       |
 * |                     |onstant and key_name must be omitted; |       |
 * |                     |otherwise key_nameis required and the |       |
 * |                     |applicable key value is the current va|       |
 * |                     |lue of the key named key_name in the l|       |
 * |                     |ocal security policy database.Valid va|       |
 * |                     |lues of ciphersuite_nbr are: 0:HMAC_SH|       |
 * |                     |A1-80 1: RSA_SHA256 255: NULL         |       |
 * +---------------------+--------------------------------------+-------+
 * |ltp_tx_rule_change   |This control changes the parameters of|       |
 * |                     | the LTP segment signing rule for the |       |
 * |                     |indicated LTP engine.                 |       |
 * +---------------------+--------------------------------------+-------+
 * |ltp_tx_rule_del      |This control deletes the LTP segment s|       |
 * |                     |igning rule forthe indicated LTP engin|       |
 * |                     |e.                                    |       |
 * +---------------------+--------------------------------------+-------+
 * |list_keys            |This control lists the names of keys a|       |
 * |                     |vailable in the key policy database.  |       |
 * +---------------------+--------------------------------------+-------+
 * |list_ltp_rx_rules    |This control lists all LTP segment aut|       |
 * |                     |hentication rules in the security poli|       |
 * |                     |cy database.                          |       |
 * +---------------------+--------------------------------------+-------+
 * |list_ltp_tx_rules    |This control lists all LTP segment sig|       |
 * |                     |ning rules in the security policy data|       |
 * |                     |base.                                 |       |
 * +---------------------+--------------------------------------+-------+
 */
#define DTN_ION_IONSECADMIN_CTRL_KEY_ADD 0x00
#define DTN_ION_IONSECADMIN_CTRL_KEY_CHANGE 0x01
#define DTN_ION_IONSECADMIN_CTRL_KEY_DEL 0x02
#define DTN_ION_IONSECADMIN_CTRL_LTP_RX_RULE_ADD 0x03
#define DTN_ION_IONSECADMIN_CTRL_LTP_RX_RULE_CHANGE 0x04
#define DTN_ION_IONSECADMIN_CTRL_LTP_RX_RULE_DEL 0x05
#define DTN_ION_IONSECADMIN_CTRL_LTP_TX_RULE_ADD 0x06
#define DTN_ION_IONSECADMIN_CTRL_LTP_TX_RULE_CHANGE 0x07
#define DTN_ION_IONSECADMIN_CTRL_LTP_TX_RULE_DEL 0x08
#define DTN_ION_IONSECADMIN_CTRL_LIST_KEYS 0x09
#define DTN_ION_IONSECADMIN_CTRL_LIST_LTP_RX_RULES 0x0a
#define DTN_ION_IONSECADMIN_CTRL_LIST_LTP_TX_RULES 0x0b


/*
 * +---------------------------------------------------------------------------------------------+
 * |                          DTN_ION_IONSECADMIN CONSTANT DEFINITIONS                           +
 * +---------------------------------------------------------------------------------------------+
 * |        NAME         |             DESCRIPTION              | TYPE  |         VALUE          |
 * +---------------------+--------------------------------------+-------+------------------------+
 */


/*
 * +---------------------------------------------------------------------------------------------+
 * |                            DTN_ION_IONSECADMIN MACRO DEFINITIONS                            +
 * +---------------------------------------------------------------------------------------------+
 * |        NAME         |             DESCRIPTION              | TYPE  |
 * +---------------------+--------------------------------------+-------+
 */


/*
 * +---------------------------------------------------------------------------------------------+
 * |                          DTN_ION_IONSECADMIN OPERATOR DEFINITIONS                           +
 * +---------------------------------------------------------------------------------------------+
 * |        NAME         |             DESCRIPTION              | TYPE  |
 * +---------------------+--------------------------------------+-------+
 */

/* Initialization functions. */
void dtn_ion_ionsecadmin_init();
void dtn_ion_ionsecadmin_init_meta();
void dtn_ion_ionsecadmin_init_cnst();
void dtn_ion_ionsecadmin_init_edd();
void dtn_ion_ionsecadmin_init_op();
void dtn_ion_ionsecadmin_init_var();
void dtn_ion_ionsecadmin_init_ctrl();
void dtn_ion_ionsecadmin_init_mac();
void dtn_ion_ionsecadmin_init_rpttpl();
void dtn_ion_ionsecadmin_init_tblt();
#endif /* _HAVE_DTN_ION_IONSECADMIN_ADM_ */
#endif //ADM_IONSEC_ADMIN_H_