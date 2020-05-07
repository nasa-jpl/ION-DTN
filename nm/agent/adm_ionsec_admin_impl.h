/****************************************************************************
 **
 ** File Name: adm_ionsec_admin_impl.h
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

#ifndef ADM_IONSEC_ADMIN_IMPL_H_
#define ADM_IONSEC_ADMIN_IMPL_H_

/*   START CUSTOM INCLUDES HERE  */
#define	EPOCH_2000_SEC	946684800
/*		LTP authentication ciphersuite numbers			*/
#define LTP_AUTH_HMAC_SHA1_80	0
#define LTP_AUTH_RSA_SHA256	1
#define LTP_AUTH_NULL		255

/*   STOP CUSTOM INCLUDES HERE  */


#include "shared/utils/utils.h"
#include "shared/primitives/ctrl.h"
#include "shared/primitives/table.h"
#include "shared/primitives/tnv.h"

/*   START typeENUM */

/*   STOP typeENUM  */

void name_adm_init_agent();



/*
 * +---------------------------------------------------------------------------------------------+
 * |                                     Retrieval Functions                                     +
 * +---------------------------------------------------------------------------------------------+
 */
/*   START CUSTOM FUNCTIONS HERE */
//static void	printBspBabRule(Object ruleAddr);
//static void	printBspBibRule(Object ruleAddr);
//static void printBspBcbRule(Object ruleAddr);
//static void	printLtpRecvAuthRule(Object ruleAddr);
//static void	printLtpXmitAuthRule(Object ruleAddr);

/*	*	Functions for managing security information.		*/

extern void	sec_findKey(char *keyName, Object *keyAddr, Object *eltp);
extern int	sec_addKey(char *keyName, char *fileName);
extern int	sec_updateKey(char *keyName, char *fileName);
extern int	sec_removeKey(char *keyName);
extern int	sec_activeKey(char *keyName);
extern int	sec_addKeyValue(char *keyName, char *keyVal, uint32_t keyLen);
/*   STOP CUSTOM FUNCTIONS HERE  */

void dtn_ion_ionsecadmin_setup();
void dtn_ion_ionsecadmin_cleanup();


/* Metadata Functions */
tnv_t *dtn_ion_ionsecadmin_meta_name(tnvc_t *parms);
tnv_t *dtn_ion_ionsecadmin_meta_namespace(tnvc_t *parms);
tnv_t *dtn_ion_ionsecadmin_meta_version(tnvc_t *parms);
tnv_t *dtn_ion_ionsecadmin_meta_organization(tnvc_t *parms);

/* Constant Functions */

/* Collect Functions */


/* Control Functions */
tnv_t *dtn_ion_ionsecadmin_ctrl_key_add(eid_t *def_mgr, tnvc_t *parms, int8_t *status);
tnv_t *dtn_ion_ionsecadmin_ctrl_key_change(eid_t *def_mgr, tnvc_t *parms, int8_t *status);
tnv_t *dtn_ion_ionsecadmin_ctrl_key_del(eid_t *def_mgr, tnvc_t *parms, int8_t *status);
tnv_t *dtn_ion_ionsecadmin_ctrl_ltp_rx_rule_add(eid_t *def_mgr, tnvc_t *parms, int8_t *status);
tnv_t *dtn_ion_ionsecadmin_ctrl_ltp_rx_rule_change(eid_t *def_mgr, tnvc_t *parms, int8_t *status);
tnv_t *dtn_ion_ionsecadmin_ctrl_ltp_rx_rule_del(eid_t *def_mgr, tnvc_t *parms, int8_t *status);
tnv_t *dtn_ion_ionsecadmin_ctrl_ltp_tx_rule_add(eid_t *def_mgr, tnvc_t *parms, int8_t *status);
tnv_t *dtn_ion_ionsecadmin_ctrl_ltp_tx_rule_change(eid_t *def_mgr, tnvc_t *parms, int8_t *status);
tnv_t *dtn_ion_ionsecadmin_ctrl_ltp_tx_rule_del(eid_t *def_mgr, tnvc_t *parms, int8_t *status);
tnv_t *dtn_ion_ionsecadmin_ctrl_list_keys(eid_t *def_mgr, tnvc_t *parms, int8_t *status);
tnv_t *dtn_ion_ionsecadmin_ctrl_list_ltp_rx_rules(eid_t *def_mgr, tnvc_t *parms, int8_t *status);
tnv_t *dtn_ion_ionsecadmin_ctrl_list_ltp_tx_rules(eid_t *def_mgr, tnvc_t *parms, int8_t *status);


/* OP Functions */


/* Table Build Functions */
tbl_t *dtn_ion_ionsecadmin_tblt_ltp_rx_rules(ari_t *id);
tbl_t *dtn_ion_ionsecadmin_tblt_ltp_tx_rules(ari_t *id);

#endif //ADM_IONSEC_ADMIN_IMPL_H_
