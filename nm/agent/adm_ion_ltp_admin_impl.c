/****************************************************************************
 **
 ** File Name: adm_ion_ltp_admin_impl.c
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

/*   START CUSTOM INCLUDES HERE  */
/*   STOP CUSTOM INCLUDES HERE  */

#include "adm_ion_ltp_admin_impl.h"

/*   START CUSTOM FUNCTIONS HERE */
/*   STOP CUSTOM FUNCTIONS HERE  */

void adm_ion_ltp_admin_setup(){

	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION setup BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION setup BODY
	 * +-------------------------------------------------------------------------+
	 */
}

void adm_ion_ltp_admin_cleanup(){

	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION cleanup BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION cleanup BODY
	 * +-------------------------------------------------------------------------+
	 */
}


/* Metadata Functions */


value_t adm_ion_ltp_admin_meta_name(tdc_t params)
{
	return val_from_string("adm_ion_ltp_admin");
}


value_t adm_ion_ltp_admin_meta_namespace(tdc_t params)
{
	return val_from_string("arn:DTN:ion_ltp_admin");
}


value_t adm_ion_ltp_admin_meta_version(tdc_t params)
{
	return val_from_string("V0.0");
}


value_t adm_ion_ltp_admin_meta_organization(tdc_t params)
{
	return val_from_string("JHUAPL");
}


/* Table Functions */


/*
 * This table lists all spans of potential LTP data interchange that exists between the local LTP engin
 * e and the indicated (neighboring) LTP engine.
 */

table_t* adm_ion_ltp_admin_tbl_spans()
{
	table_t *table = NULL;
	if((table = table_create(NULL,NULL)) == NULL)
	{
		return NULL;
	}

	if(
		(table_add_col(table, "peer_engine_nbr", AMP_TYPE_UINT) == ERROR) ||
		(table_add_col(table, "max_export_sessions", AMP_TYPE_UINT) == ERROR) ||
		(table_add_col(table, "max_import_sessions", AMP_TYPE_UINT) == ERROR) ||
		(table_add_col(table, "max_segment_size", AMP_TYPE_UINT) == ERROR) ||
		(table_add_col(table, "aggregation_size_limit", AMP_TYPE_UINT) == ERROR) ||
		(table_add_col(table, "aggregation_time_limit", AMP_TYPE_UINT) == ERROR) ||
		(table_add_col(table, "lso_control", AMP_TYPE_STR) == ERROR) ||
		(table_add_col(table, "queueing_latency", AMP_TYPE_UINT) == ERROR))
	{
		table_destroy(table, 1);
		return NULL;
	}

	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION tbl_spans BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION tbl_spans BODY
	 * +-------------------------------------------------------------------------+
	 */
	return table;
}


/* Collect Functions */
/*
 * This is the version of ION that is currently installed.
 */
value_t adm_ion_ltp_admin_get_version(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_version BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_version BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}



/* Control Functions */

/*
 * Until this control is executed, LTP is not in operation on the local ION node and most ltpadmin cont
 * rols will fail. The control uses estMaxExportSessions to configure the hashtable it will use to mana
 * ge access to export transmission sessions that are currently in progress. For optimum performance, e
 * stMaxExportSessions should normally equal or exceed the summation of maxExportSessions over all span
 * s as discussed below. Appropriate values for the parameters configuring each "span" of potential LTP
 *  data exchange between the local LTP and neighboring engines are non-trivial to determine.
 */
tdc_t* adm_ion_ltp_admin_ctrl_init(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_init BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_init BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * This control declares the maximum number of bytes of SDR heap space that will be occupied by the acq
 * uisition of any single LTP block.  All data acquired in excess of this limit will be written to a te
 * mporary file pending extraction and dispatching of the acquired block. Default is the minimum allowe
 * d value (560 bytes), which is the approximate size of a ZCO file reference object; this is the minim
 * um SDR heap space occupancy in the event that all acquisition is into a file.
 */
tdc_t* adm_ion_ltp_admin_ctrl_manage_heap(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_manage_heap BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_manage_heap BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * This control sets the expected maximum bit error rate(BER) that LTP should provide for in computing 
 * the maximum number of transmission efforts to initiate in the transmission of a given block.(Note th
 * at this computation is also sensitive to data segment size and to the size of the block that is to b
 * e transmitted.) The default value is .0001 (10^-4).
 */
tdc_t* adm_ion_ltp_admin_ctrl_manage_max_ber(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_manage_max_ber BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_manage_max_ber BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * This control sets the number of seconds of predicted additional latency attributable to processing d
 * elay within the local engine itself that should be included whenever LTP computes the nominal round-
 * trip time for an exchange of data with any remote engine. The default value is 1.
 */
tdc_t* adm_ion_ltp_admin_ctrl_manage_own_queue_time(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_manage_own_queue_time BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_manage_own_queue_time BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * This control enables or disables the screening of received LTP segments per the periods of scheduled
 *  reception in the node's contact graph.  By default, screening is disabled. When screening is enable
 * d, such segments are silently discarded.  Note that when screening is enabled the ranges declared in
 *  the contact graph must be accurate and clocks must be synchronized; otherwise, segments will be arr
 * iving at times other than the scheduled contact intervals and will be discarded.
 */
tdc_t* adm_ion_ltp_admin_ctrl_manage_screening(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_manage_screening BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_manage_screening BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * This control declares that a span of potential LTP data interchange exists between the local LTP eng
 * ine and the indicated (neighboring) LTP engine.
 */
tdc_t* adm_ion_ltp_admin_ctrl_span_add(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_span_add BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_span_add BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * This control sets the indicated span's configuration parameters to the values provided as arguments
 */
tdc_t* adm_ion_ltp_admin_ctrl_span_change(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_span_change BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_span_change BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * This control deletes the span identified by peerEngineNumber. The control will fail if any outbound 
 * segments for this span are pending transmission or any inbound blocks from the peer engine are incom
 * plete.
 */
tdc_t* adm_ion_ltp_admin_ctrl_span_del(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_span_del BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_span_del BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * This control stops all link service input and output tasks for the local LTP engine.
 */
tdc_t* adm_ion_ltp_admin_ctrl_stop(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_stop BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_stop BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * This control enables and disables production of a continuous stream of user- selected LTP activity i
 * ndication characters. Activity parameter of "1" selects all LTP activity indication characters; "0" 
 * de-selects all LTP activity indication characters; any other activitySpec such as "df{]" selects all
 *  activity indication characters in the string, de-selecting all others. LTP will print each selected
 *  activity indication character to stdout every time a processing event of the associated type occurs
 * :
 *  * d    bundle appended to block for next session
 *  * e    segment of block is queued for transmission
 *  * f    block has been fully segmented for transmission
 *  * g    segment popped from transmission queue
 *  * h    positive ACK received for block, session ended
 *  * s    segment received
 *  * t    block has been fully received
 *  * @    negative ACK received for block, segments retransmitted
 *  * =    unacknowledged checkpoint was retransmitted
 *  * +    unacknowledged report segment was retransmitted
 *  * {    export session canceled locally (by sender)
 *  * }    import session canceled by remote sender
 *  * [    import session canceled locally (by receiver)
 *  * ]    export session canceled by remote receiver
 */
tdc_t* adm_ion_ltp_admin_ctrl_watch_set(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_watch_set BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_watch_set BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}



/* OP Functions */
