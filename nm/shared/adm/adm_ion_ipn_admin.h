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
 **  2020-04-13  AUTO             Auto-generated header file 
 **
 ****************************************************************************/


#ifndef ADM_ION_IPN_ADMIN_H_
#define ADM_ION_IPN_ADMIN_H_
#define _HAVE_DTN_ION_IPNADMIN_ADM_
#ifdef _HAVE_DTN_ION_IPNADMIN_ADM_

#include "shared/utils/nm_types.h"
#include "shared/adm/adm.h"


/*
 * +---------------------------------------------------------------------------------------------+
 * |                                 ADM TEMPLATE DOCUMENTATION                                  +
 * +---------------------------------------------------------------------------------------------+
 *
 * ADM ROOT STRING:DTN/ION/ipnadmin
 */
#define ADM_ENUM_DTN_ION_IPNADMIN 6
/*
 * +---------------------------------------------------------------------------------------------+
 * |                                 AGENT NICKNAME DEFINITIONS                                  +
 * +---------------------------------------------------------------------------------------------+
 */

/*
 * +---------------------------------------------------------------------------------------------+
 * |                           DTN_ION_IPNADMIN META-DATA DEFINITIONS                            +
 * +---------------------------------------------------------------------------------------------+
 * |        NAME         |             DESCRIPTION              | TYPE  |         VALUE          |
 * +---------------------+--------------------------------------+-------+------------------------+
 * |name                 |The human-readable name of the ADM.   |STR    |ion_ipn_admin           |
 * +---------------------+--------------------------------------+-------+------------------------+
 * |namespace            |The namespace of the ADM              |STR    |DTN/ION/ipnadmin        |
 * +---------------------+--------------------------------------+-------+------------------------+
 * |version              |The version of the ADM                |STR    |v0.0                    |
 * +---------------------+--------------------------------------+-------+------------------------+
 * |organization         |The name of the issuing organization o|       |                        |
 * |                     |f the ADM                             |STR    |JHUAPL                  |
 * +---------------------+--------------------------------------+-------+------------------------+
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
 * +---------------------------------------------------------------------------------------------+
 * |                    DTN_ION_IPNADMIN EXTERNALLY DEFINED DATA DEFINITIONS                     +
 * +---------------------------------------------------------------------------------------------+
 * |        NAME         |             DESCRIPTION              | TYPE  |
 * +---------------------+--------------------------------------+-------+
 * |ion_version          |This is the version of ion is that cur|       |
 * |                     |rently installed.                     |STR    |
 * +---------------------+--------------------------------------+-------+
 */
#define DTN_ION_IPNADMIN_EDD_ION_VERSION 0x00


/*
 * +---------------------------------------------------------------------------------------------+
 * |                            DTN_ION_IPNADMIN VARIABLE DEFINITIONS                            +
 * +---------------------------------------------------------------------------------------------+
 * |        NAME         |             DESCRIPTION              | TYPE  |
 * +---------------------+--------------------------------------+-------+
 */


/*
 * +---------------------------------------------------------------------------------------------+
 * |                             DTN_ION_IPNADMIN REPORT DEFINITIONS                             +
 * +---------------------------------------------------------------------------------------------+
 * |        NAME         |             DESCRIPTION              | TYPE  |
 * +---------------------+--------------------------------------+-------+
 */


/*
 * +---------------------------------------------------------------------------------------------+
 * |                             DTN_ION_IPNADMIN TABLE DEFINITIONS                              +
 * +---------------------------------------------------------------------------------------------+
 * |        NAME         |             DESCRIPTION              | TYPE  |
 * +---------------------+--------------------------------------+-------+
 * |exits                |This table lists all of the exits that|       |
 * |                     | are defined in the IPN database for t|       |
 * |                     |he local node.                        |       |
 * +---------------------+--------------------------------------+-------+
 * |plans                |This table lists all of the egress pla|       |
 * |                     |ns that are established in the IPN dat|       |
 * |                     |abase for the local node.             |       |
 * +---------------------+--------------------------------------+-------+
 */
#define DTN_ION_IPNADMIN_TBLT_EXITS 0x00
#define DTN_ION_IPNADMIN_TBLT_PLANS 0x01


/*
 * +---------------------------------------------------------------------------------------------+
 * |                            DTN_ION_IPNADMIN CONTROL DEFINITIONS                             +
 * +---------------------------------------------------------------------------------------------+
 * |        NAME         |             DESCRIPTION              | TYPE  |
 * +---------------------+--------------------------------------+-------+
 * |exit_add             |This control establishes an exit for s|       |
 * |                     |tatic default routing.                |       |
 * +---------------------+--------------------------------------+-------+
 * |exit_change          |This control changes the gateway node |       |
 * |                     |number for the exit identified by firs|       |
 * |                     |tNodeNbr and lastNodeNbr.             |       |
 * +---------------------+--------------------------------------+-------+
 * |exit_del             |This control deletes the exit identifi|       |
 * |                     |ed by firstNodeNbr and lastNodeNbr.   |       |
 * +---------------------+--------------------------------------+-------+
 * |plan_add             |This control establishes an egress pla|       |
 * |                     |n for the bundles that must be transmi|       |
 * |                     |tted to the neighboring node that is i|       |
 * |                     |dentified by it's nodeNbr.            |       |
 * +---------------------+--------------------------------------+-------+
 * |plan_change          |This control changes the duct expressi|       |
 * |                     |on for the indicated plan.            |       |
 * +---------------------+--------------------------------------+-------+
 * |plan_del             |This control deletes the egress plan f|       |
 * |                     |or the node that is identified by it's|       |
 * |                     | nodeNbr.                             |       |
 * +---------------------+--------------------------------------+-------+
 */
#define DTN_ION_IPNADMIN_CTRL_EXIT_ADD 0x00
#define DTN_ION_IPNADMIN_CTRL_EXIT_CHANGE 0x01
#define DTN_ION_IPNADMIN_CTRL_EXIT_DEL 0x02
#define DTN_ION_IPNADMIN_CTRL_PLAN_ADD 0x03
#define DTN_ION_IPNADMIN_CTRL_PLAN_CHANGE 0x04
#define DTN_ION_IPNADMIN_CTRL_PLAN_DEL 0x05


/*
 * +---------------------------------------------------------------------------------------------+
 * |                            DTN_ION_IPNADMIN CONSTANT DEFINITIONS                            +
 * +---------------------------------------------------------------------------------------------+
 * |        NAME         |             DESCRIPTION              | TYPE  |         VALUE          |
 * +---------------------+--------------------------------------+-------+------------------------+
 */


/*
 * +---------------------------------------------------------------------------------------------+
 * |                             DTN_ION_IPNADMIN MACRO DEFINITIONS                              +
 * +---------------------------------------------------------------------------------------------+
 * |        NAME         |             DESCRIPTION              | TYPE  |
 * +---------------------+--------------------------------------+-------+
 */


/*
 * +---------------------------------------------------------------------------------------------+
 * |                            DTN_ION_IPNADMIN OPERATOR DEFINITIONS                            +
 * +---------------------------------------------------------------------------------------------+
 * |        NAME         |             DESCRIPTION              | TYPE  |
 * +---------------------+--------------------------------------+-------+
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