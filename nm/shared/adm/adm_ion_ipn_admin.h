/****************************************************************************
 **
 ** File Name: adm_ion_ipn_admin.h
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
 **  2018-01-06  AUTO             Auto-generated header file 
 **
 ****************************************************************************/


#ifndef ADM_ION_IPN_ADMIN_H_
#define ADM_ION_IPN_ADMIN_H_
#define _HAVE_ION_IPN_ADMIN_ADM_
#ifdef _HAVE_ION_IPN_ADMIN_ADM_

#include "lyst.h"
#include "../utils/nm_types.h"
#include "adm.h"

/*
 * +----------------------------------------------------------------------------------------------------------+
 * |			              ADM TEMPLATE DOCUMENTATION                                              +
 * +----------------------------------------------------------------------------------------------------------+
 *
 * ADM ROOT STRING:arn:DTN:ion_ipn_admin
 */

/*
 * +----------------------------------------------------------------------------------------------------------+
 * |				             AGENT NICKNAME DEFINITIONS                                       +
 * +----------------------------------------------------------------------------------------------------------+
 */
#define ION_IPN_ADMIN_ADM_META_NN_IDX 60
#define ION_IPN_ADMIN_ADM_META_NN_STR "60"

#define ION_IPN_ADMIN_ADM_EDD_NN_IDX 61
#define ION_IPN_ADMIN_ADM_EDD_NN_STR "61"

#define ION_IPN_ADMIN_ADM_VAR_NN_IDX 62
#define ION_IPN_ADMIN_ADM_VAR_NN_STR "62"

#define ION_IPN_ADMIN_ADM_RPT_NN_IDX 63
#define ION_IPN_ADMIN_ADM_RPT_NN_STR "63"

#define ION_IPN_ADMIN_ADM_CTRL_NN_IDX 64
#define ION_IPN_ADMIN_ADM_CTRL_NN_STR "64"

#define ION_IPN_ADMIN_ADM_CONST_NN_IDX 65
#define ION_IPN_ADMIN_ADM_CONST_NN_STR "65"

#define ION_IPN_ADMIN_ADM_MACRO_NN_IDX 66
#define ION_IPN_ADMIN_ADM_MACRO_NN_STR "66"

#define ION_IPN_ADMIN_ADM_OP_NN_IDX 67
#define ION_IPN_ADMIN_ADM_OP_NN_STR "67"

#define ION_IPN_ADMIN_ADM_TBL_NN_IDX 68
#define ION_IPN_ADMIN_ADM_TBL_NN_STR "68"

#define ION_IPN_ADMIN_ADM_ROOT_NN_IDX 69
#define ION_IPN_ADMIN_ADM_ROOT_NN_STR "69"


/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |		                    ION_IPN_ADMIN META-DATA DEFINITIONS                                                          
 * +-----------------------------------------------------------------------------------------------------------+
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |             NAME            |    MID     |              DESCRIPTION                         |     TYPE    |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |name                         |0x873c0100  |The human-readable name of the ADM.               |STR          |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |namespace                    |0x873c0101  |The namespace of the ADM                          |STR          |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |version                      |0x873c0102  |The version of the ADM                            |STR          |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |organization                 |0x873c0103  |The name of the issuing organization of the ADM   |STR          |
   +-----------------------------+------------+--------------------------------------------------+-------------+
 */
// "name"
#define ADM_ION_IPN_ADMIN_META_NAME_MID 0x873c0100
// "namespace"
#define ADM_ION_IPN_ADMIN_META_NAMESPACE_MID 0x873c0101
// "version"
#define ADM_ION_IPN_ADMIN_META_VERSION_MID 0x873c0102
// "organization"
#define ADM_ION_IPN_ADMIN_META_ORGANIZATION_MID 0x873c0103


/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |		                    ION_IPN_ADMIN EXTERNALLY DEFINED DATA DEFINITIONS                                               
 * +-----------------------------------------------------------------------------------------------------------+
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |             NAME            |    MID     |              DESCRIPTION                         |     TYPE    |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |ion_version                  |0x803d0100  |This is the version of ion is that currently insta|             |
   |                             |            |lled.                                             |STR          |
   +-----------------------------+------------+--------------------------------------------------+-------------+
 */
#define ADM_ION_IPN_ADMIN_EDD_ION_VERSION_MID 0x803d0100


/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |		                    ION_IPN_ADMIN VARIABLE DEFINITIONS                                                          
 * +-----------------------------------------------------------------------------------------------------------+
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |             NAME            |    MID     |              DESCRIPTION                         |     TYPE    |
   +-----------------------------+------------+--------------------------------------------------+-------------+
 */


/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |		                    ION_IPN_ADMIN REPORT DEFINITIONS                                                           
 * +-----------------------------------------------------------------------------------------------------------+
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |             NAME            |    MID     |              DESCRIPTION                         |     TYPE    |
   +-----------------------------+------------+--------------------------------------------------+-------------+
 */


/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |		                    ION_IPN_ADMIN CONTROL DEFINITIONS                                                         
 * +-----------------------------------------------------------------------------------------------------------+
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |             NAME            |    MID     |              DESCRIPTION                         |     TYPE    |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |exit_add                     |0x83400100  |This control establishes an "exit" for static defa|             |
   |                             |            |ult routing.                                      |             |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |exit_change                  |0x83400101  |This control changes the gateway node number for t|             |
   |                             |            |he exit identified by firstNodeNbr and lastNodeNbr|             |
   |                             |            |.                                                 |             |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |exit_del                     |0x83400102  |This control deletes the exit identified by firstN|             |
   |                             |            |odeNbr and lastNodeNbr.                           |             |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |exit_rule_add                |0x83400103  |This control adds an exit rule.                   |             |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |exit_rule_change             |0x83400104  |This control changes an exit rule.                |             |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |exit_rule_del                |0x83400105  |This control deletes an exit rule.                |             |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |plan_add                     |0x83400106  |This control establishes an egress plan for the bu|             |
   |                             |            |ndles that must be transmitted to the neighboring |             |
   |                             |            |node that is identified by it's nodeNbr.          |             |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |plan_change                  |0x83400107  |This control changes the duct expression for the i|             |
   |                             |            |ndicated plan.                                    |             |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |plan_del                     |0x83400108  |This control deletes the egress plan for the node |             |
   |                             |            |that is identified by it's nodeNbr.               |             |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |plan_rule_add                |0x83400109  |This control adds a plan rule.                    |             |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |plan_rule_change             |0x8340010a  |This control changes the plan rule.               |             |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |plan_rule_del                |0x8340010b  |This control deletes a plan rule.                 |             |
   +-----------------------------+------------+--------------------------------------------------+-------------+
 */
#define ADM_ION_IPN_ADMIN_CTRL_EXIT_ADD_MID 0x83400100
#define ADM_ION_IPN_ADMIN_CTRL_EXIT_CHANGE_MID 0x83400101
#define ADM_ION_IPN_ADMIN_CTRL_EXIT_DEL_MID 0x83400102
#define ADM_ION_IPN_ADMIN_CTRL_EXIT_RULE_ADD_MID 0x83400103
#define ADM_ION_IPN_ADMIN_CTRL_EXIT_RULE_CHANGE_MID 0x83400104
#define ADM_ION_IPN_ADMIN_CTRL_EXIT_RULE_DEL_MID 0x83400105
#define ADM_ION_IPN_ADMIN_CTRL_PLAN_ADD_MID 0x83400106
#define ADM_ION_IPN_ADMIN_CTRL_PLAN_CHANGE_MID 0x83400107
#define ADM_ION_IPN_ADMIN_CTRL_PLAN_DEL_MID 0x83400108
#define ADM_ION_IPN_ADMIN_CTRL_PLAN_RULE_ADD_MID 0x83400109
#define ADM_ION_IPN_ADMIN_CTRL_PLAN_RULE_CHANGE_MID 0x8340010a
#define ADM_ION_IPN_ADMIN_CTRL_PLAN_RULE_DEL_MID 0x8340010b


/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |		                    ION_IPN_ADMIN CONSTANT DEFINITIONS                                                         
 * +-----------------------------------------------------------------------------------------------------------+
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |             NAME            |    MID     |              DESCRIPTION                         |     TYPE    |
   +-----------------------------+------------+--------------------------------------------------+-------------+
 */


/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |		                    ION_IPN_ADMIN MACRO DEFINITIONS                                                            
 * +-----------------------------------------------------------------------------------------------------------+
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |             NAME            |    MID     |              DESCRIPTION                         |     TYPE    |
   +-----------------------------+------------+--------------------------------------------------+-------------+
 */


/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |		                    ION_IPN_ADMIN OPERATOR DEFINITIONS                                                          
 * +-----------------------------------------------------------------------------------------------------------+
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |             NAME            |    MID     |              DESCRIPTION                         |     TYPE    |
   +-----------------------------+------------+--------------------------------------------------+-------------+
 */

/* Initialization functions. */
void adm_ion_ipn_admin_init();
void adm_ion_ipn_admin_init_edd();
void adm_ion_ipn_admin_init_variables();
void adm_ion_ipn_admin_init_controls();
void adm_ion_ipn_admin_init_constants();
void adm_ion_ipn_admin_init_macros();
void adm_ion_ipn_admin_init_metadata();
void adm_ion_ipn_admin_init_ops();
void adm_ion_ipn_admin_init_reports();
#endif /* _HAVE_ION_IPN_ADMIN_ADM_ */
#endif //ADM_ION_IPN_ADMIN_H_