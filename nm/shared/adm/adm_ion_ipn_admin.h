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
 **  2018-11-18  AUTO             Auto-generated header file 
 **
 ****************************************************************************/


#ifndef ADM_ION_IPN_ADMIN_H_
#define ADM_ION_IPN_ADMIN_H_
#define _HAVE_DTN_ION_IPNADMIN_ADM_
#ifdef _HAVE_DTN_ION_IPNADMIN_ADM_

#include "../utils/nm_types.h"
#include "adm.h"

extern vec_idx_t g_dtn_ion_ipnadmin_idx[11];


/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |                                        ADM TEMPLATE DOCUMENTATION                                        +
 * +-----------------------------------------------------------------------------------------------------------+
 *
 * ADM ROOT STRING:DTN/ION/ipnadmin
 */
#define ADM_ENUM_DTN_ION_IPNADMIN 6
/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |                                        AGENT NICKNAME DEFINITIONS                                        +
 * +-----------------------------------------------------------------------------------------------------------+
 */

/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |                                  DTN_ION_IPNADMIN META-DATA DEFINITIONS                                  +
 * +-----------------------------------------------------------------------------------------------------------+
 * |        NAME         |     ARI      |             DESCRIPTION              | TYPE  |         VALUE          |
 * +---------------------+--------------+--------------------------------------+-------+------------------------+
 * |name                 |4480188200    |The human-readable name of the ADM.   |STR    |ion_ipn_admin           |
 * +---------------------+--------------+--------------------------------------+-------+------------------------+
 * |namespace            |4480188201    |The namespace of the ADM              |STR    |DTN/ION/ipnadmin        |
 * +---------------------+--------------+--------------------------------------+-------+------------------------+
 * |version              |4480188202    |The version of the ADM                |STR    |v0.0                    |
 * +---------------------+--------------+--------------------------------------+-------+------------------------+
 * |organization         |4480188203    |The name of the issuing organization o|       |                        |
 * |                     |              |f the ADM                             |STR    |JHUAPL                  |
 * +---------------------+--------------+--------------------------------------+-------+------------------------+
 */
// "name"
#define DTN_ION_IPNADMIN_META_NAME 0x00
// "namespace"
#define DTN_ION_IPNADMIN_META_NAMESPACE 0x01
// "version"
#define DTN_ION_IPNADMIN_META_VERSION 0x02
// "organization"
#define DTN_ION_IPNADMIN_META_ORGANIZATION 0x03


/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |                           DTN_ION_IPNADMIN EXTERNALLY DEFINED DATA DEFINITIONS                           +
 * +-----------------------------------------------------------------------------------------------------------+
 * |        NAME         |     ARI      |             DESCRIPTION              | TYPE  |
 * +---------------------+--------------+--------------------------------------+-------+
 * |ion_version          |4482187a00    |This is the version of ion is that cur|       |
 * |                     |              |rently installed.                     |STR    |
 * +---------------------+--------------+--------------------------------------+-------+
 */
#define DTN_ION_IPNADMIN_EDD_ION_VERSION 0x00


/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |                                   DTN_ION_IPNADMIN VARIABLE DEFINITIONS                                   +
 * +-----------------------------------------------------------------------------------------------------------+
 * |        NAME         |     ARI      |             DESCRIPTION              | TYPE  |
 * +---------------------+--------------+--------------------------------------+-------+
 */


/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |                                    DTN_ION_IPNADMIN REPORT DEFINITIONS                                    +
 * +-----------------------------------------------------------------------------------------------------------+
 * |        NAME         |     ARI      |             DESCRIPTION              | TYPE  |
 * +---------------------+--------------+--------------------------------------+-------+
 */


/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |                                    DTN_ION_IPNADMIN TABLE DEFINITIONS                                    +
 * +-----------------------------------------------------------------------------------------------------------+
 * |        NAME         |     ARI      |             DESCRIPTION              | TYPE  |
 * +---------------------+--------------+--------------------------------------+-------+
 * |exits                |448a187f00    |This table lists all of the exits that|       |
 * |                     |              | are defined in the IPN database for t|       |
 * |                     |              |he local node.                        |       |
 * +---------------------+--------------+--------------------------------------+-------+
 * |plans                |448a187f01    |This table lists all of the egress pla|       |
 * |                     |              |ns that are established in the IPN dat|       |
 * |                     |              |abase for the local node.             |       |
 * +---------------------+--------------+--------------------------------------+-------+
 */
#define DTN_ION_IPNADMIN_TBLT_EXITS 0x00
#define DTN_ION_IPNADMIN_TBLT_PLANS 0x01


/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |                                   DTN_ION_IPNADMIN CONTROL DEFINITIONS                                   +
 * +-----------------------------------------------------------------------------------------------------------+
 * |        NAME         |     ARI      |             DESCRIPTION              | TYPE  |
 * +---------------------+--------------+--------------------------------------+-------+
 * |exit_add             |44c1187900    |This control establishes an "exit" for|       |
 * |                     |              | static default routing.              |       |
 * +---------------------+--------------+--------------------------------------+-------+
 * |exit_change          |44c1187901    |This control changes the gateway node |       |
 * |                     |              |number for the exit identified by firs|       |
 * |                     |              |tNodeNbr and lastNodeNbr.             |       |
 * +---------------------+--------------+--------------------------------------+-------+
 * |exit_del             |44c1187902    |This control deletes the exit identifi|       |
 * |                     |              |ed by firstNodeNbr and lastNodeNbr.   |       |
 * +---------------------+--------------+--------------------------------------+-------+
 * |plan_add             |44c1187903    |This control establishes an egress pla|       |
 * |                     |              |n for the bundles that must be transmi|       |
 * |                     |              |tted to the neighboring node that is i|       |
 * |                     |              |dentified by it's nodeNbr.            |       |
 * +---------------------+--------------+--------------------------------------+-------+
 * |plan_change          |44c1187904    |This control changes the duct expressi|       |
 * |                     |              |on for the indicated plan.            |       |
 * +---------------------+--------------+--------------------------------------+-------+
 * |plan_del             |44c1187905    |This control deletes the egress plan f|       |
 * |                     |              |or the node that is identified by it's|       |
 * |                     |              | nodeNbr.                             |       |
 * +---------------------+--------------+--------------------------------------+-------+
 */
#define DTN_ION_IPNADMIN_CTRL_EXIT_ADD 0x00
#define DTN_ION_IPNADMIN_CTRL_EXIT_CHANGE 0x01
#define DTN_ION_IPNADMIN_CTRL_EXIT_DEL 0x02
#define DTN_ION_IPNADMIN_CTRL_PLAN_ADD 0x03
#define DTN_ION_IPNADMIN_CTRL_PLAN_CHANGE 0x04
#define DTN_ION_IPNADMIN_CTRL_PLAN_DEL 0x05


/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |                                   DTN_ION_IPNADMIN CONSTANT DEFINITIONS                                   +
 * +-----------------------------------------------------------------------------------------------------------+
 * |        NAME         |     ARI      |             DESCRIPTION              | TYPE  |         VALUE          |
 * +---------------------+--------------+--------------------------------------+-------+------------------------+
 */


/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |                                    DTN_ION_IPNADMIN MACRO DEFINITIONS                                    +
 * +-----------------------------------------------------------------------------------------------------------+
 * |        NAME         |     ARI      |             DESCRIPTION              | TYPE  |
 * +---------------------+--------------+--------------------------------------+-------+
 */


/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |                                   DTN_ION_IPNADMIN OPERATOR DEFINITIONS                                   +
 * +-----------------------------------------------------------------------------------------------------------+
 * |        NAME         |     ARI      |             DESCRIPTION              | TYPE  |
 * +---------------------+--------------+--------------------------------------+-------+
 */

/* Initialization functions. */
void dtn_ion_ipnadmin_init();
void dtn_ion_ipnadmin_init_meta();
void dtn_ion_ipnadmin_init_cnst();
void dtn_ion_ipnadmin_init_edd();
void dtn_ion_ipnadmin_init_op();
void dtn_ion_ipnadmin_init_var();
void dtn_ion_ipnadmin_init_ctrl();
void dtn_ion_ipnadmin_init_mac();
void dtn_ion_ipnadmin_init_rpttpl();
void dtn_ion_ipnadmin_init_tblt();
#endif /* _HAVE_DTN_ION_IPNADMIN_ADM_ */
#endif //ADM_ION_IPN_ADMIN_H_