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
 **  2020-04-13  AUTO             Auto-generated header file 
 **
 ****************************************************************************/


#ifndef ADM_ION_LTP_ADMIN_H_
#define ADM_ION_LTP_ADMIN_H_
#define _HAVE_DTN_ION_LTPADMIN_ADM_
#ifdef _HAVE_DTN_ION_LTPADMIN_ADM_

#include "shared/utils/nm_types.h"
#include "shared/adm/adm.h"


/*
 * +---------------------------------------------------------------------------------------------+
 * |                                 ADM TEMPLATE DOCUMENTATION                                  +
 * +---------------------------------------------------------------------------------------------+
 *
 * ADM ROOT STRING:DTN/ION/ltpadmin
 */
#define ADM_ENUM_DTN_ION_LTPADMIN 9
/*
 * +---------------------------------------------------------------------------------------------+
 * |                                 AGENT NICKNAME DEFINITIONS                                  +
 * +---------------------------------------------------------------------------------------------+
 */

/*
 * +---------------------------------------------------------------------------------------------+
 * |                           DTN_ION_LTPADMIN META-DATA DEFINITIONS                            +
 * +---------------------------------------------------------------------------------------------+
 * |        NAME         |             DESCRIPTION              | TYPE  |         VALUE          |
 * +---------------------+--------------------------------------+-------+------------------------+
 * |name                 |The human-readable name of the ADM.   |STR    |ion_ltp_admin           |
 * +---------------------+--------------------------------------+-------+------------------------+
 * |namespace            |The namespace of the ADM.             |STR    |DTN/ION/ltpadmin        |
 * +---------------------+--------------------------------------+-------+------------------------+
 * |version              |The version of the ADM.               |STR    |v0.0                    |
 * +---------------------+--------------------------------------+-------+------------------------+
 * |organization         |The name of the issuing organization o|       |                        |
 * |                     |f the ADM.                            |STR    |JHUAPL                  |
 * +---------------------+--------------------------------------+-------+------------------------+
 */
// "name"
#define DTN_ION_LTPADMIN_META_NAME 0x00
// "namespace"
#define DTN_ION_LTPADMIN_META_NAMESPACE 0x01
// "version"
#define DTN_ION_LTPADMIN_META_VERSION 0x02
// "organization"
#define DTN_ION_LTPADMIN_META_ORGANIZATION 0x03


/*
 * +---------------------------------------------------------------------------------------------+
 * |                    DTN_ION_LTPADMIN EXTERNALLY DEFINED DATA DEFINITIONS                     +
 * +---------------------------------------------------------------------------------------------+
 * |        NAME         |             DESCRIPTION              | TYPE  |
 * +---------------------+--------------------------------------+-------+
 * |ion_version          |This is the version of ION that is cur|       |
 * |                     |rently installed.                     |STR    |
 * +---------------------+--------------------------------------+-------+
 */
#define DTN_ION_LTPADMIN_EDD_ION_VERSION 0x00


/*
 * +---------------------------------------------------------------------------------------------+
 * |                            DTN_ION_LTPADMIN VARIABLE DEFINITIONS                            +
 * +---------------------------------------------------------------------------------------------+
 * |        NAME         |             DESCRIPTION              | TYPE  |
 * +---------------------+--------------------------------------+-------+
 */


/*
 * +---------------------------------------------------------------------------------------------+
 * |                             DTN_ION_LTPADMIN REPORT DEFINITIONS                             +
 * +---------------------------------------------------------------------------------------------+
 * |        NAME         |             DESCRIPTION              | TYPE  |
 * +---------------------+--------------------------------------+-------+
 */


/*
 * +---------------------------------------------------------------------------------------------+
 * |                             DTN_ION_LTPADMIN TABLE DEFINITIONS                              +
 * +---------------------------------------------------------------------------------------------+
 * |        NAME         |             DESCRIPTION              | TYPE  |
 * +---------------------+--------------------------------------+-------+
 * |spans                |This table lists all spans of potentia|       |
 * |                     |l LTP data interchange that exists bet|       |
 * |                     |ween the local LTP engine and the indi|       |
 * |                     |cated (neighboring) LTP engine.       |       |
 * +---------------------+--------------------------------------+-------+
 */
#define DTN_ION_LTPADMIN_TBLT_SPANS 0x00


/*
 * +---------------------------------------------------------------------------------------------+
 * |                            DTN_ION_LTPADMIN CONTROL DEFINITIONS                             +
 * +---------------------------------------------------------------------------------------------+
 * |        NAME         |             DESCRIPTION              | TYPE  |
 * +---------------------+--------------------------------------+-------+
 * |manage_heap          |This control declares the maximum numb|       |
 * |                     |er of bytes of SDR heap space that wil|       |
 * |                     |l be occupied by the acquisition of an|       |
 * |                     |y single LTP block. All data acquired |       |
 * |                     |in excess of this limit will be writte|       |
 * |                     |n to a temporary file pending extracti|       |
 * |                     |on and dispatching of the acquired blo|       |
 * |                     |ck. Default is the minimum allowed val|       |
 * |                     |ue (560 bytes), which is the approxima|       |
 * |                     |te size of a ZCO file reference object|       |
 * |                     |; this is the minimum SDR heap space o|       |
 * |                     |ccupancy in the event that all acquisi|       |
 * |                     |tion is into a file.                  |       |
 * +---------------------+--------------------------------------+-------+
 * |manage_max_ber       |This control sets the expected maximum|       |
 * |                     | bit error rate(BER) that LTP should p|       |
 * |                     |rovide for in computing the maximum nu|       |
 * |                     |mber of transmission efforts to initia|       |
 * |                     |te in the transmission of a given bloc|       |
 * |                     |k.(Note that this computation is also |       |
 * |                     |sensitive to data segment size and to |       |
 * |                     |the size of the block that is to be tr|       |
 * |                     |ansmitted.) The default value is .0001|       |
 * |                     | (10^-4).                             |       |
 * +---------------------+--------------------------------------+-------+
 * |manage_own_queue_time|This control sets the number of second|       |
 * |                     |s of predicted additional latency attr|       |
 * |                     |ibutable to processing delay within th|       |
 * |                     |e local engine itself that should be i|       |
 * |                     |ncluded whenever LTP computes the nomi|       |
 * |                     |nal round-trip time for an exchange of|       |
 * |                     | data with any remote engine.The defau|       |
 * |                     |lt value is 1.                        |       |
 * +---------------------+--------------------------------------+-------+
 * |manage_screening     |This control enables or disables the s|       |
 * |                     |creening of received LTP segments per |       |
 * |                     |the periods of scheduled reception in |       |
 * |                     |the node's contact graph. By default, |       |
 * |                     |screening is disabled. When screening |       |
 * |                     |is enabled, such segments are silently|       |
 * |                     | discarded. Note that when screening i|       |
 * |                     |s enabled the ranges declared in the c|       |
 * |                     |ontact graph must be accurate and cloc|       |
 * |                     |ks must be synchronized; otherwise, se|       |
 * |                     |gments will be arriving at times other|       |
 * |                     | than the scheduled contact intervals |       |
 * |                     |and will be discarded.                |       |
 * +---------------------+--------------------------------------+-------+
 * |span_add             |This control declares that a span of p|       |
 * |                     |otential LTP data interchange exists b|       |
 * |                     |etween the local LTP engine and the in|       |
 * |                     |dicated (neighboring) LTP engine.     |       |
 * +---------------------+--------------------------------------+-------+
 * |span_change          |This control sets the indicated span's|       |
 * |                     | configuration parameters to the value|       |
 * |                     |s provided as arguments               |       |
 * +---------------------+--------------------------------------+-------+
 * |span_del             |This control deletes the span identifi|       |
 * |                     |ed by peerEngineNumber. The control wi|       |
 * |                     |ll fail if any outbound segments for t|       |
 * |                     |his span are pending transmission or a|       |
 * |                     |ny inbound blocks from the peer engine|       |
 * |                     | are incomplete.                      |       |
 * +---------------------+--------------------------------------+-------+
 * |stop                 |This control stops all link service in|       |
 * |                     |put and output tasks for the local LTP|       |
 * |                     | engine.                              |       |
 * +---------------------+--------------------------------------+-------+
 * |watch_set            |This control enables and disables prod|       |
 * |                     |uction of a continuous stream of user-|       |
 * |                     | selected LTP activity indication char|       |
 * |                     |acters. Activity parameter of 1 select|       |
 * |                     |s all LTP activity indication characte|       |
 * |                     |rs; 0 de-selects all LTP activity indi|       |
 * |                     |cation characters; any other activityS|       |
 * |                     |pec such as df{] selects all activity |       |
 * |                     |indication characters in the string, d|       |
 * |                     |e-selecting all others. LTP will print|       |
 * |                     | each selected activity indication cha|       |
 * |                     |racter to stdout every time a processi|       |
 * |                     |ng event of the associated type occurs|       |
 * |                     |: d bundle appended to block for next |       |
 * |                     |session, e segment of block is queued |       |
 * |                     |for transmission, f block has been ful|       |
 * |                     |ly segmented for transmission, g segme|       |
 * |                     |nt popped from transmission queue, h p|       |
 * |                     |ositive ACK received for block and ses|       |
 * |                     |sion ended, s segment received, t bloc|       |
 * |                     |k has been fully received, @ negative |       |
 * |                     |ACK received for block and segments re|       |
 * |                     |transmitted, = unacknowledged checkpoi|       |
 * |                     |nt was retransmitted, + unacknowledged|       |
 * |                     | report segment was retransmitted, { e|       |
 * |                     |xport session canceled locally (by sen|       |
 * |                     |der), } import session canceled by rem|       |
 * |                     |ote sender, [ import session canceled |       |
 * |                     |locally (by receiver), ] export sessio|       |
 * |                     |n canceled by remote receiver         |       |
 * +---------------------+--------------------------------------+-------+
 */
#define DTN_ION_LTPADMIN_CTRL_MANAGE_HEAP 0x00
#define DTN_ION_LTPADMIN_CTRL_MANAGE_MAX_BER 0x01
#define DTN_ION_LTPADMIN_CTRL_MANAGE_OWN_QUEUE_TIME 0x02
#define DTN_ION_LTPADMIN_CTRL_MANAGE_SCREENING 0x03
#define DTN_ION_LTPADMIN_CTRL_SPAN_ADD 0x04
#define DTN_ION_LTPADMIN_CTRL_SPAN_CHANGE 0x05
#define DTN_ION_LTPADMIN_CTRL_SPAN_DEL 0x06
#define DTN_ION_LTPADMIN_CTRL_STOP 0x07
#define DTN_ION_LTPADMIN_CTRL_WATCH_SET 0x08


/*
 * +---------------------------------------------------------------------------------------------+
 * |                            DTN_ION_LTPADMIN CONSTANT DEFINITIONS                            +
 * +---------------------------------------------------------------------------------------------+
 * |        NAME         |             DESCRIPTION              | TYPE  |         VALUE          |
 * +---------------------+--------------------------------------+-------+------------------------+
 */


/*
 * +---------------------------------------------------------------------------------------------+
 * |                             DTN_ION_LTPADMIN MACRO DEFINITIONS                              +
 * +---------------------------------------------------------------------------------------------+
 * |        NAME         |             DESCRIPTION              | TYPE  |
 * +---------------------+--------------------------------------+-------+
 */


/*
 * +---------------------------------------------------------------------------------------------+
 * |                            DTN_ION_LTPADMIN OPERATOR DEFINITIONS                            +
 * +---------------------------------------------------------------------------------------------+
 * |        NAME         |             DESCRIPTION              | TYPE  |
 * +---------------------+--------------------------------------+-------+
 */

/* Initialization functions. */
void dtn_ion_ltpadmin_init();
void dtn_ion_ltpadmin_init_meta();
void dtn_ion_ltpadmin_init_cnst();
void dtn_ion_ltpadmin_init_edd();
void dtn_ion_ltpadmin_init_op();
void dtn_ion_ltpadmin_init_var();
void dtn_ion_ltpadmin_init_ctrl();
void dtn_ion_ltpadmin_init_mac();
void dtn_ion_ltpadmin_init_rpttpl();
void dtn_ion_ltpadmin_init_tblt();
#endif /* _HAVE_DTN_ION_LTPADMIN_ADM_ */
#endif //ADM_ION_LTP_ADMIN_H_