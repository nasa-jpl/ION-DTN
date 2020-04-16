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
 **  2020-04-13  AUTO             Auto-generated c file 
 **
 ****************************************************************************/

/*   START CUSTOM INCLUDES HERE  */
#include "ltpP.h"

/*   STOP CUSTOM INCLUDES HERE  */


#include "shared/adm/adm.h"
#include "adm_ion_ltp_admin_impl.h"

/*   START CUSTOM FUNCTIONS HERE */
/*   STOP CUSTOM FUNCTIONS HERE  */

void dtn_ion_ltpadmin_setup()
{

	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION setup BODY
	 * +-------------------------------------------------------------------------+
	 */

	ltpAttach();

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION setup BODY
	 * +-------------------------------------------------------------------------+
	 */
}

void dtn_ion_ltpadmin_cleanup()
{

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


tnv_t *dtn_ion_ltpadmin_meta_name(tnvc_t *parms)
{
	return tnv_from_str("ion_ltp_admin");
}


tnv_t *dtn_ion_ltpadmin_meta_namespace(tnvc_t *parms)
{
	return tnv_from_str("DTN/ION/ltpadmin");
}


tnv_t *dtn_ion_ltpadmin_meta_version(tnvc_t *parms)
{
	return tnv_from_str("v0.0");
}


tnv_t *dtn_ion_ltpadmin_meta_organization(tnvc_t *parms)
{
	return tnv_from_str("JHUAPL");
}


/* Constant Functions */
/* Table Functions */


/*
 * This table lists all spans of potential LTP data interchange that exists between the local LTP engin
 * e and the indicated (neighboring) LTP engine.
 */
tbl_t *dtn_ion_ltpadmin_tblt_spans(ari_t *id)
{
	tbl_t *table = NULL;
	if((table = tbl_create(id)) == NULL)
	{
		return NULL;
	}

	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION tblt_spans BODY
	 * +-------------------------------------------------------------------------+
	 */

	Sdr		sdr = getIonsdr();
	LtpVdb		*vdb = getLtpVdb();
	PsmPartition	ionwm = getIonwm();
	Object		ltpdbObj = getLtpDbObject();
	OBJ_POINTER(LtpDB, ltpdb);
	PsmAddress	elt;
	LtpVspan	*vspan;
	char	cmd[SDRSTRING_BUFSZ];
	OBJ_POINTER(LtpSpan, span);
	tnvc_t  *cur_row = NULL;


	CHKNULL(sdr_begin_xn(sdr));	/*	Just to lock memory.	*/
	GET_OBJ_POINTER(sdr, LtpDB, ltpdb, ltpdbObj);

	for (elt = sm_list_first(ionwm, vdb->spans); elt; elt = sm_list_next(ionwm, elt))
	{
		vspan = (LtpVspan *) psp(ionwm, sm_list_data(ionwm, elt));

		GET_OBJ_POINTER(sdr, LtpSpan, span, sdr_list_data(sdr, vspan->spanElt));
		sdr_string_read(sdr, cmd, span->lsoCmd);


		/* (UINT) peer_engine_nbr, (UINT) max_export_sess, (UINT) max_import_sess,
		 * (UINT) max-seg-size, (UINT) agg_size_limit, (UINT) agg_time_limit,
		 * (STR) lso_ctrl, (UINT) q_latency
		 */
		if((cur_row = tnvc_create(8)) != NULL)
		{
			tnvc_insert(cur_row, tnv_from_uvast(vspan->engineId));
			tnvc_insert(cur_row, tnv_from_uint(span->maxExportSessions));
			tnvc_insert(cur_row, tnv_from_uint(span->maxImportSessions));
			tnvc_insert(cur_row, tnv_from_uint(span->maxSegmentSize));
			tnvc_insert(cur_row, tnv_from_uint(span->aggrSizeLimit));
			tnvc_insert(cur_row, tnv_from_uint(span->aggrTimeLimit));
			tnvc_insert(cur_row, tnv_from_str(cmd));
			tnvc_insert(cur_row, tnv_from_uint(span->remoteQtime));

			tbl_add_row(table, cur_row);
		}
		else
		{
			AMP_DEBUG_WARN("dtn_ion_ltpadmin_tblt_spans", "Can't allocate row. Skipping.", NULL);
		}
	}

	sdr_exit_xn(sdr);

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION tblt_spans BODY
	 * +-------------------------------------------------------------------------+
	 */
	return table;
}


/* Collect Functions */
/*
 * This is the version of ION that is currently installed.
 */
tnv_t *dtn_ion_ltpadmin_get_ion_version(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_ion_version BODY
	 * +-------------------------------------------------------------------------+
	 */

	result = tnv_from_str(IONVERSIONNUMBER);

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_ion_version BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}



/* Control Functions */

/*
 * This control declares the maximum number of bytes of SDR heap space that will be occupied by the acq
 * uisition of any single LTP block. All data acquired in excess of this limit will be written to a tem
 * porary file pending extraction and dispatching of the acquired block. Default is the minimum allowed
 *  value (560 bytes), which is the approximate size of a ZCO file reference object; this is the minimu
 * m SDR heap space occupancy in the event that all acquisition is into a file.
 */
tnv_t *dtn_ion_ltpadmin_ctrl_manage_heap(eid_t *def_mgr, tnvc_t *parms, int8_t *status)
{
	tnv_t *result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_manage_heap BODY
	 * +-------------------------------------------------------------------------+
	 */

	Sdr		sdr = getIonsdr();
	Object		ltpdbObj = getLtpDbObject();
	LtpDB		ltpdb;
	int success;
	unsigned int heapmax = adm_get_parm_uint(parms, 0, &success);

	if (heapmax < 560)
	{
		AMP_DEBUG_ERR("dtn_ion_ltpadmin_ctrl_manage_heap", "Heapmax must be at least 560, not %lu", heapmax);
		return result;
	}

	CHKNULL(sdr_begin_xn(sdr));
	sdr_stage(sdr, (char *) &ltpdb, ltpdbObj, sizeof(LtpDB));
	ltpdb.maxAcqInHeap = heapmax;
	sdr_write(sdr, ltpdbObj, (char *) &ltpdb, sizeof(LtpDB));
	if (sdr_end_xn(sdr) < 0)
	{
		AMP_DEBUG_ERR("dtn_ion_ltpadmin_ctrl_manage_heap", "Can't change maxAcqInHeap.", NULL);
	}
	else
	{
		*status = CTRL_SUCCESS;
	}

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
tnv_t *dtn_ion_ltpadmin_ctrl_manage_max_ber(eid_t *def_mgr, tnvc_t *parms, int8_t *status)
{
	tnv_t *result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_manage_max_ber BODY
	 * +-------------------------------------------------------------------------+
	 */

	Sdr		sdr = getIonsdr();
	Object		ltpdbObj = getLtpDbObject();
	PsmPartition	ionwm = getIonwm();
	LtpVdb		*vdb = getLtpVdb();
	LtpDB		ltpdb;
	double		newMaxBER;
	PsmAddress	elt;
	LtpVspan	*vspan;
	int success;
	newMaxBER = adm_get_parm_real32(parms, 0, &success);

	if (newMaxBER < 0.0)
	{
		AMP_DEBUG_ERR("dtn_ion_ltpadmin_ctrl_manage_max_ber","Max BER invalid %f", newMaxBER);
		return NULL;
	}

	CHKNULL(sdr_begin_xn(sdr));
	sdr_stage(sdr, (char *) &ltpdb, ltpdbObj, sizeof(LtpDB));
	ltpdb.maxBER = newMaxBER;
	sdr_write(sdr, ltpdbObj, (char *) &ltpdb, sizeof(LtpDB));
	for (elt = sm_list_first(ionwm, vdb->spans); elt;
			elt = sm_list_next(ionwm, elt))
	{
		vspan = (LtpVspan *) psp(ionwm, sm_list_data(ionwm, elt));
		computeRetransmissionLimits(vspan);
	}

	if (sdr_end_xn(sdr) < 0)
	{
		AMP_DEBUG_ERR("dtn_ion_ltpadmin_ctrl_manage_max_ber","Can't change maximum bit error rate.", NULL);
	}
	else
	{
		*status = CTRL_SUCCESS;
	}


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
 * trip time for an exchange of data with any remote engine.The default value is 1.
 */
tnv_t *dtn_ion_ltpadmin_ctrl_manage_own_queue_time(eid_t *def_mgr, tnvc_t *parms, int8_t *status)
{
	tnv_t *result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_manage_own_queue_time BODY
	 * +-------------------------------------------------------------------------+
	 */

	Sdr	sdr = getIonsdr();
	Object	ltpdbObj = getLtpDbObject();
	LtpDB	ltpdb;
	int	newOwnQtime;
	int success;

	newOwnQtime = adm_get_parm_uint(parms, 0, &success);

	if (newOwnQtime < 0)
	{
		AMP_DEBUG_WARN("dtn_ion_ltpadmin_ctrl_manage_own_queue_time", "Own Q time invalid: %d", newOwnQtime);
		return NULL;
	}

	CHKNULL(sdr_begin_xn(sdr));
	sdr_stage(sdr, (char *) &ltpdb, ltpdbObj, sizeof(LtpDB));
	ltpdb.ownQtime = newOwnQtime;
	sdr_write(sdr, ltpdbObj, (char *) &ltpdb, sizeof(LtpDB));
	if (sdr_end_xn(sdr) < 0)
	{
		AMP_DEBUG_ERR("dtn_ion_ltpadmin_ctrl_manage_own_queue_time", "Can't change own LTP queuing time.", NULL);
	}
	else
	{
		*status = CTRL_SUCCESS;
	}

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_manage_own_queue_time BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * This control enables or disables the screening of received LTP segments per the periods of scheduled
 *  reception in the node's contact graph. By default, screening is disabled. When screening is enabled
 * , such segments are silently discarded. Note that when screening is enabled the ranges declared in t
 * he contact graph must be accurate and clocks must be synchronized; otherwise, segments will be arriv
 * ing at times other than the scheduled contact intervals and will be discarded.
 */
tnv_t *dtn_ion_ltpadmin_ctrl_manage_screening(eid_t *def_mgr, tnvc_t *parms, int8_t *status)
{
	tnv_t *result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_manage_screening BODY
	 * +-------------------------------------------------------------------------+
	 */

	AMP_DEBUG_WARN("dtn_ion_ltpadmin_ctrl_manage_screening","Note: LTP screening is now always on, cannot be disabled.", NULL);

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
tnv_t *dtn_ion_ltpadmin_ctrl_span_add(eid_t *def_mgr, tnvc_t *parms, int8_t *status)
{
	tnv_t *result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_span_add BODY
	 * +-------------------------------------------------------------------------+
	 */
	int success;
	uvast engineId = adm_get_parm_uvast(parms, 0, &success);
	unsigned int maxExportSessions = adm_get_parm_uint(parms, 1, &success);
	unsigned int maxImportSessions = adm_get_parm_uint(parms, 2, &success);
	unsigned int maxSegmentSize = adm_get_parm_uint(parms, 3, &success);
	unsigned int aggrSizeLimit = adm_get_parm_uint(parms, 4, &success);
	unsigned int aggrTimeLimit = adm_get_parm_uint(parms, 5, &success);
	char *lsoCmd = adm_get_parm_obj(parms, 6, AMP_TYPE_STR);
	unsigned int qTime = adm_get_parm_uint(parms, 7, &success);
	int purge = 0;

	if (qTime == 0)
	{
		purge = 1;
		qTime = 0 - qTime;
	}

	if(addSpan(engineId, maxExportSessions, maxImportSessions, maxSegmentSize, aggrSizeLimit, aggrTimeLimit, lsoCmd, qTime, purge) > 0)
	{
		*status = CTRL_SUCCESS;
	}

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
tnv_t *dtn_ion_ltpadmin_ctrl_span_change(eid_t *def_mgr, tnvc_t *parms, int8_t *status)
{
	tnv_t *result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_span_change BODY
	 * +-------------------------------------------------------------------------+
	 */

	int success;
	uvast engineId = adm_get_parm_uvast(parms, 0, &success);
	unsigned int maxExportSessions = adm_get_parm_uint(parms, 1, &success);
	unsigned int maxImportSessions = adm_get_parm_uint(parms, 2, &success);
	unsigned int maxSegmentSize = adm_get_parm_uint(parms, 3, &success);
	unsigned int aggrSizeLimit = adm_get_parm_uint(parms, 4, &success);
	unsigned int aggrTimeLimit = adm_get_parm_uint(parms, 5, &success);
	char *lsoCmd = adm_get_parm_obj(parms, 6, AMP_TYPE_STR);
	unsigned int qTime = adm_get_parm_uint(parms, 7, &success);
	int purge = 0;

	if (qTime == 0)
	{
		purge = 1;
		qTime = 0 - qTime;
	}

	if(updateSpan(engineId, maxExportSessions, maxImportSessions, maxSegmentSize, aggrSizeLimit, aggrTimeLimit, lsoCmd, qTime, purge) > 0)
	{
		*status = CTRL_SUCCESS;
	}

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
tnv_t *dtn_ion_ltpadmin_ctrl_span_del(eid_t *def_mgr, tnvc_t *parms, int8_t *status)
{
	tnv_t *result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_span_del BODY
	 * +-------------------------------------------------------------------------+
	 */

	int success;
	uvast engineId = adm_get_parm_uvast(parms, 0, &success);
	if(removeSpan(engineId) > 0)
	{
		*status = CTRL_SUCCESS;
	}

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
tnv_t *dtn_ion_ltpadmin_ctrl_stop(eid_t *def_mgr, tnvc_t *parms, int8_t *status)
{
	tnv_t *result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_stop BODY
	 * +-------------------------------------------------------------------------+
	 */

	ltpStop();
	*status = CTRL_SUCCESS;

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_stop BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * This control enables and disables production of a continuous stream of user- selected LTP activity i
 * ndication characters. Activity parameter of 1 selects all LTP activity indication characters; 0 de-s
 * elects all LTP activity indication characters; any other activitySpec such as df{] selects all activ
 * ity indication characters in the string, de-selecting all others. LTP will print each selected activ
 * ity indication character to stdout every time a processing event of the associated type occurs: d bu
 * ndle appended to block for next session, e segment of block is queued for transmission, f block has 
 * been fully segmented for transmission, g segment popped from transmission queue, h positive ACK rece
 * ived for block and session ended, s segment received, t block has been fully received, @ negative AC
 * K received for block and segments retransmitted, = unacknowledged checkpoint was retransmitted, + un
 * acknowledged report segment was retransmitted, { export session canceled locally (by sender), } impo
 * rt session canceled by remote sender, [ import session canceled locally (by receiver), ] export sess
 * ion canceled by remote receiver
 */
tnv_t *dtn_ion_ltpadmin_ctrl_watch_set(eid_t *def_mgr, tnvc_t *parms, int8_t *status)
{
	tnv_t *result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_watch_set BODY
	 * +-------------------------------------------------------------------------+
	 */

	int success;
	Sdr	sdr = getIonsdr();
	Object	dbObj = getLtpDbObject();
	LtpDB	db;
	char *name = adm_get_parm_obj(parms, 0, AMP_TYPE_STR);
	LtpVdb	*vdb = getLtpVdb();
	int i;

	*status = CTRL_SUCCESS;

	if (strcmp(name, "1") == 0)
	{
		vdb->watching = -1;
		return result;
	}

	vdb->watching = 0;
	if (strcmp(name, "0") == 0)
	{
		return result;
	}

	for(i = 0; i < strlen(name); i++)
	{
		switch(name[i])
		{
			case 'd': vdb->watching |= WATCH_d; break;
			case 'e': vdb->watching |= WATCH_e; break;
			case 'f': vdb->watching |= WATCH_f; break;
			case 'g': vdb->watching |= WATCH_g; break;
			case 'h': vdb->watching |= WATCH_h; break;
			case 's': vdb->watching |= WATCH_s; break;
			case 't': vdb->watching |= WATCH_t; break;
			case '@': vdb->watching |= WATCH_nak; break;
			case '{': vdb->watching |= WATCH_CS; break;
			case '}': vdb->watching |= WATCH_handleCS;  break;
			case '[': vdb->watching |= WATCH_CR; break;
			case ']': vdb->watching |= WATCH_handleCR; break;
			case '=': vdb->watching |= WATCH_resendCP;   break;
			case '+': vdb->watching |= WATCH_resendRS; break;
			default:
				AMP_DEBUG_WARN("dtn_ion_ltpadmin_ctrl_watch_set", "Invalid watch char %c.", name[i]);
				break;
		}
	}

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_watch_set BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}



/* OP Functions */
