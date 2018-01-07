/****************************************************************************
 **
 ** File Name: adm_ion_ltp_admin.h
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


#ifndef ADM_ION_LTP_ADMIN_H_
#define ADM_ION_LTP_ADMIN_H_
#define _HAVE_ION_LTP_ADMIN_ADM_
#ifdef _HAVE_ION_LTP_ADMIN_ADM_

#include "lyst.h"
#include "../utils/nm_types.h"
#include "adm.h"

/*
 * +----------------------------------------------------------------------------------------------------------+
 * |			              ADM TEMPLATE DOCUMENTATION                                              +
 * +----------------------------------------------------------------------------------------------------------+
 *
 * ADM ROOT STRING:arn:DTN:ion_ltp_admin
 */

/*
 * +----------------------------------------------------------------------------------------------------------+
 * |				             AGENT NICKNAME DEFINITIONS                                       +
 * +----------------------------------------------------------------------------------------------------------+
 */
#define ION_LTP_ADMIN_ADM_META_NN_IDX 90
#define ION_LTP_ADMIN_ADM_META_NN_STR "90"

#define ION_LTP_ADMIN_ADM_EDD_NN_IDX 91
#define ION_LTP_ADMIN_ADM_EDD_NN_STR "91"

#define ION_LTP_ADMIN_ADM_VAR_NN_IDX 92
#define ION_LTP_ADMIN_ADM_VAR_NN_STR "92"

#define ION_LTP_ADMIN_ADM_RPT_NN_IDX 93
#define ION_LTP_ADMIN_ADM_RPT_NN_STR "93"

#define ION_LTP_ADMIN_ADM_CTRL_NN_IDX 94
#define ION_LTP_ADMIN_ADM_CTRL_NN_STR "94"

#define ION_LTP_ADMIN_ADM_CONST_NN_IDX 95
#define ION_LTP_ADMIN_ADM_CONST_NN_STR "95"

#define ION_LTP_ADMIN_ADM_MACRO_NN_IDX 96
#define ION_LTP_ADMIN_ADM_MACRO_NN_STR "96"

#define ION_LTP_ADMIN_ADM_OP_NN_IDX 97
#define ION_LTP_ADMIN_ADM_OP_NN_STR "97"

#define ION_LTP_ADMIN_ADM_TBL_NN_IDX 98
#define ION_LTP_ADMIN_ADM_TBL_NN_STR "98"

#define ION_LTP_ADMIN_ADM_ROOT_NN_IDX 99
#define ION_LTP_ADMIN_ADM_ROOT_NN_STR "99"


/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |		                    ION_LTP_ADMIN META-DATA DEFINITIONS                                                          
 * +-----------------------------------------------------------------------------------------------------------+
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |             NAME            |    MID     |              DESCRIPTION                         |     TYPE    |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |name                         |0x875a0100  |The human-readable name of the ADM.               |STR          |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |namespace                    |0x875a0101  |The namespace of the ADM.                         |STR          |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |version                      |0x875a0102  |The version of the ADM.                           |STR          |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |organization                 |0x875a0103  |The name of the issuing organization of the ADM.  |STR          |
   +-----------------------------+------------+--------------------------------------------------+-------------+
 */
// "name"
#define ADM_ION_LTP_ADMIN_META_NAME_MID 0x875a0100
// "namespace"
#define ADM_ION_LTP_ADMIN_META_NAMESPACE_MID 0x875a0101
// "version"
#define ADM_ION_LTP_ADMIN_META_VERSION_MID 0x875a0102
// "organization"
#define ADM_ION_LTP_ADMIN_META_ORGANIZATION_MID 0x875a0103


/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |		                    ION_LTP_ADMIN EXTERNALLY DEFINED DATA DEFINITIONS                                               
 * +-----------------------------------------------------------------------------------------------------------+
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |             NAME            |    MID     |              DESCRIPTION                         |     TYPE    |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |version                      |0x805b0100  |This is the version of ION that is currently insta|             |
   |                             |            |lled.                                             |STR          |
   +-----------------------------+------------+--------------------------------------------------+-------------+
 */
#define ADM_ION_LTP_ADMIN_EDD_VERSION_MID 0x805b0100


/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |		                    ION_LTP_ADMIN VARIABLE DEFINITIONS                                                          
 * +-----------------------------------------------------------------------------------------------------------+
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |             NAME            |    MID     |              DESCRIPTION                         |     TYPE    |
   +-----------------------------+------------+--------------------------------------------------+-------------+
 */


/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |		                    ION_LTP_ADMIN REPORT DEFINITIONS                                                           
 * +-----------------------------------------------------------------------------------------------------------+
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |             NAME            |    MID     |              DESCRIPTION                         |     TYPE    |
   +-----------------------------+------------+--------------------------------------------------+-------------+
 */


/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |		                    ION_LTP_ADMIN CONTROL DEFINITIONS                                                         
 * +-----------------------------------------------------------------------------------------------------------+
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |             NAME            |    MID     |              DESCRIPTION                         |     TYPE    |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |init                         |0xc35e0100  |Until this control is executed, LTP is not in oper|             |
   |                             |            |ation on the local ION node and most ltpadmin cont|             |
   |                             |            |rols will fail. The control uses estMaxExportSessi|             |
   |                             |            |ons to configure the hashtable it will use to mana|             |
   |                             |            |ge access to export transmission sessions that are|             |
   |                             |            | currently in progress. For optimum performance, e|             |
   |                             |            |stMaxExportSessions should normally equal or excee|             |
   |                             |            |d the summation of maxExportSessions over all span|             |
   |                             |            |s as discussed below. Appropriate values for the p|             |
   |                             |            |arameters configuring each "span" of potential LTP|             |
   |                             |            | data exchange between the local LTP and neighbori|             |
   |                             |            |ng engines are non-trivial to determine.          |             |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |manageHeap                   |0xc35e0101  |This control declares the maximum number of bytes |             |
   |                             |            |of SDR heap space that will be occupied by the acq|             |
   |                             |            |uisition of any single LTP block.  All data acquir|             |
   |                             |            |ed in excess of this limit will be written to a te|             |
   |                             |            |mporary file pending extraction and dispatching of|             |
   |                             |            | the acquired block. Default is the minimum allowe|             |
   |                             |            |d value (560 bytes), which is the approximate size|             |
   |                             |            | of a ZCO file reference object; this is the minim|             |
   |                             |            |um SDR heap space occupancy in the event that all |             |
   |                             |            |acquisition is into a file.                       |             |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |manageMaxBER                 |0xc35e0102  |This control sets the expected maximum bit error r|             |
   |                             |            |ate(BER) that LTP should provide for in computing |             |
   |                             |            |the maximum number of transmission efforts to init|             |
   |                             |            |iate in the transmission of a given block.(Note th|             |
   |                             |            |at this computation is also sensitive to data segm|             |
   |                             |            |ent size and to the size of the block that is to b|             |
   |                             |            |e transmitted.) The default value is .0001 (10^-4)|             |
   |                             |            |.                                                 |             |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |manageOwnQueueTime           |0xc35e0103  |This control sets the number of seconds of predict|             |
   |                             |            |ed additional latency attributable to processing d|             |
   |                             |            |elay within the local engine itself that should be|             |
   |                             |            | included whenever LTP computes the nominal round-|             |
   |                             |            |trip time for an exchange of data with any remote |             |
   |                             |            |engine. The default value is 1.                   |             |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |manageScreening              |0xc35e0104  |This control enables or disables the screening of |             |
   |                             |            |received LTP segments per the periods of scheduled|             |
   |                             |            | reception in the node's contact graph.  By defaul|             |
   |                             |            |t, screening is disabled. When screening is enable|             |
   |                             |            |d, such segments are silently discarded.  Note tha|             |
   |                             |            |t when screening is enabled the ranges declared in|             |
   |                             |            | the contact graph must be accurate and clocks mus|             |
   |                             |            |t be synchronized; otherwise, segments will be arr|             |
   |                             |            |iving at times other than the scheduled contact in|             |
   |                             |            |tervals and will be discarded.                    |             |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |spanAdd                      |0xc35e0105  |This control declares that a span of potential LTP|             |
   |                             |            | data interchange exists between the local LTP eng|             |
   |                             |            |ine and the indicated (neighboring) LTP engine.   |             |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |spanChange                   |0xc35e0106  |This control sets the indicated span's configurati|             |
   |                             |            |on parameters to the values provided as arguments |             |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |spanDel                      |0xc35e0107  |This control deletes the span identified by peerEn|             |
   |                             |            |gineNumber. The control will fail if any outbound |             |
   |                             |            |segments for this span are pending transmission or|             |
   |                             |            | any inbound blocks from the peer engine are incom|             |
   |                             |            |plete.                                            |             |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |stop                         |0x835e0108  |This control stops all link service input and outp|             |
   |                             |            |ut tasks for the local LTP engine.                |             |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |watchSet                     |0xc35e0109  |This control enables and disables production of a |             |
   |                             |            |continuous stream of user- selected LTP activity i|             |
   |                             |            |ndication characters. Activity parameter of "1" se|             |
   |                             |            |lects all LTP activity indication characters; "0" |             |
   |                             |            |de-selects all LTP activity indication characters;|             |
   |                             |            | any other activitySpec such as "df{]" selects all|             |
   |                             |            | activity indication characters in the string, de-|             |
   |                             |            |selecting all others. LTP will print each selected|             |
   |                             |            | activity indication character to stdout every tim|             |
   |                             |            |e a processing event of the associated type occurs|             |
   |                             |            |:                                                 |             |
   |                             |            |d    bundle appended to block for next session    |             |
   |                             |            |e    segment of block is queued for transmission  |             |
   |                             |            |f    block has been fully segmented for transmissi|             |
   |                             |            |on                                                |             |
   |                             |            |g    segment popped from transmission queue       |             |
   |                             |            |h    positive ACK received for block, session ende|             |
   |                             |            |d                                                 |             |
   |                             |            |s    segment received                             |             |
   |                             |            |t    block has been fully received                |             |
   |                             |            |@    negative ACK received for block, segments ret|             |
   |                             |            |ransmitted                                        |             |
   |                             |            |=    unacknowledged checkpoint was retransmitted  |             |
   |                             |            |+    unacknowledged report segment was retransmitt|             |
   |                             |            |ed                                                |             |
   |                             |            |{    export session canceled locally (by sender)  |             |
   |                             |            |}    import session canceled by remote sender     |             |
   |                             |            |[    import session canceled locally (by receiver)|             |
   |                             |            |]    export session canceled by remote receiver   |             |
   +-----------------------------+------------+--------------------------------------------------+-------------+
 */
#define ADM_ION_LTP_ADMIN_CTRL_INIT_MID 0xc35e0100
#define ADM_ION_LTP_ADMIN_CTRL_MANAGEHEAP_MID 0xc35e0101
#define ADM_ION_LTP_ADMIN_CTRL_MANAGEMAXBER_MID 0xc35e0102
#define ADM_ION_LTP_ADMIN_CTRL_MANAGEOWNQUEUETIME_MID 0xc35e0103
#define ADM_ION_LTP_ADMIN_CTRL_MANAGESCREENING_MID 0xc35e0104
#define ADM_ION_LTP_ADMIN_CTRL_SPANADD_MID 0xc35e0105
#define ADM_ION_LTP_ADMIN_CTRL_SPANCHANGE_MID 0xc35e0106
#define ADM_ION_LTP_ADMIN_CTRL_SPANDEL_MID 0xc35e0107
#define ADM_ION_LTP_ADMIN_CTRL_STOP_MID 0x835e0108
#define ADM_ION_LTP_ADMIN_CTRL_WATCHSET_MID 0xc35e0109


/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |		                    ION_LTP_ADMIN CONSTANT DEFINITIONS                                                         
 * +-----------------------------------------------------------------------------------------------------------+
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |             NAME            |    MID     |              DESCRIPTION                         |     TYPE    |
   +-----------------------------+------------+--------------------------------------------------+-------------+
 */


/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |		                    ION_LTP_ADMIN MACRO DEFINITIONS                                                            
 * +-----------------------------------------------------------------------------------------------------------+
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |             NAME            |    MID     |              DESCRIPTION                         |     TYPE    |
   +-----------------------------+------------+--------------------------------------------------+-------------+
 */


/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |		                    ION_LTP_ADMIN OPERATOR DEFINITIONS                                                          
 * +-----------------------------------------------------------------------------------------------------------+
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |             NAME            |    MID     |              DESCRIPTION                         |     TYPE    |
   +-----------------------------+------------+--------------------------------------------------+-------------+
 */

/* Initialization functions. */
void adm_ion_ltp_admin_init();
void adm_ion_ltp_admin_init_edd();
void adm_ion_ltp_admin_init_variables();
void adm_ion_ltp_admin_init_controls();
void adm_ion_ltp_admin_init_constants();
void adm_ion_ltp_admin_init_macros();
void adm_ion_ltp_admin_init_metadata();
void adm_ion_ltp_admin_init_ops();
void adm_ion_ltp_admin_init_reports();
#endif /* _HAVE_ION_LTP_ADMIN_ADM_ */
#endif //ADM_ION_LTP_ADMIN_H_