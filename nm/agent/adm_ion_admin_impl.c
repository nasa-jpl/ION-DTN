/****************************************************************************
 **
 ** File Name: adm_ion_admin_impl.c
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
#include "rfx.h"
/*   STOP CUSTOM INCLUDES HERE  */


#include "shared/adm/adm.h"
#include "adm_ion_admin_impl.h"

/*   START CUSTOM FUNCTIONS HERE */
/*   STOP CUSTOM FUNCTIONS HERE  */

void dtn_ion_ionadmin_setup()
{

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

void dtn_ion_ionadmin_cleanup()
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


tnv_t *dtn_ion_ionadmin_meta_name(tnvc_t *parms)
{
	return tnv_from_str("ion_admin");
}


tnv_t *dtn_ion_ionadmin_meta_namespace(tnvc_t *parms)
{
	return tnv_from_str("DTN/ION/ionadmin");
}


tnv_t *dtn_ion_ionadmin_meta_version(tnvc_t *parms)
{
	return tnv_from_str("v0.0");
}


tnv_t *dtn_ion_ionadmin_meta_organization(tnvc_t *parms)
{
	return tnv_from_str("JHUAPL");
}


/* Constant Functions */
/* Table Functions */


/*
 * This table shows all scheduled periods of data transmission.
 */
tbl_t *dtn_ion_ionadmin_tblt_contacts(ari_t *id)
{
	tbl_t *table = NULL;
	if((table = tbl_create(id)) == NULL)
	{
		return NULL;
	}

	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION tblt_contacts BODY
	 * +-------------------------------------------------------------------------+
	 */
	Sdr             sdr = getIonsdr();
	PsmPartition    ionwm = getIonwm();
	IonVdb          *vdb = getIonVdb();
	PsmAddress      elt;
	PsmAddress      addr;
	IonCXref        *contact;
	tnvc_t *cur_row = NULL;

	CHKNULL(sdr_begin_xn(sdr));
	for (elt = sm_rbt_first(ionwm, vdb->contactIndex); elt;
			elt = sm_rbt_next(ionwm, elt))
	{
		addr = sm_rbt_data(ionwm, elt);
		CHKNULL(addr);

		if((contact = (IonCXref *) psp(getIonwm(), addr)) == NULL)
		{
			AMP_DEBUG_WARN("dtn_ion_ionadmin_tblt_contacts","NULL contact encountered. Skipping.", NULL);
			continue;
		}

		 /* Table is: (TV)Start, (TV)Stop, (UINT)Src Node, (UINT)Dest Node, (UVAST)Xmit, (UVAST)Confidence */
		if((cur_row = tnvc_create(6)) != NULL)
		{
			tnvc_insert(cur_row, tnv_from_tv((uvast)(contact->fromTime)));
 			tnvc_insert(cur_row, tnv_from_tv((uvast)(contact->toTime)));
			tnvc_insert(cur_row, tnv_from_uint(contact->fromNode));
			tnvc_insert(cur_row, tnv_from_uint(contact->toNode));
			tnvc_insert(cur_row, tnv_from_uvast(contact->xmitRate));
			tnvc_insert(cur_row, tnv_from_uvast(contact->confidence));

			tbl_add_row(table, cur_row);
		}
		else
		{
			AMP_DEBUG_WARN("dtn_ion_ionadmin_tblt_contacts", "Can't allocate row. Skipping.", NULL);
		}
	}

	sdr_exit_xn(sdr);


	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION tblt_contacts BODY
	 * +-------------------------------------------------------------------------+
	 */
	return table;
}


/*
 * This table shows all predicted periods of constant distance between nodes.
 */
tbl_t *dtn_ion_ionadmin_tblt_ranges(ari_t *id)
{
	tbl_t *table = NULL;
	if((table = tbl_create(id)) == NULL)
	{
		return NULL;
	}

	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION tblt_ranges BODY
	 * +-------------------------------------------------------------------------+
	 */

	Sdr             sdr = getIonsdr();
	PsmPartition    ionwm = getIonwm();
	IonVdb          *vdb = getIonVdb();
	PsmAddress      elt;
	PsmAddress      addr;
	IonRXref        *range = NULL;
	tnvc_t *cur_row = NULL;


	CHKNULL(sdr_begin_xn(sdr));
	for (elt = sm_rbt_first(ionwm, vdb->rangeIndex); elt;
			elt = sm_rbt_next(ionwm, elt))
	{
		addr = sm_rbt_data(ionwm, elt);
		CHKNULL(addr);

        if((range = (IonRXref *) psp(getIonwm(), addr)) == NULL)
        {
			AMP_DEBUG_WARN("dtn_ion_ionadmin_tblt_ranges","NULL contact encountered. Skipping.", NULL);
			continue;
		}

		 /* Table is: (TV)Start, (TV)Stop, (UINT) Node, (UINT)Other Node, (UINT) Dist */
		if((cur_row = tnvc_create(5)) != NULL)
		{
			tnvc_insert(cur_row, tnv_from_uvast(range->fromTime));
			tnvc_insert(cur_row, tnv_from_uvast(range->toTime));
			tnvc_insert(cur_row, tnv_from_uint(range->fromNode));
			tnvc_insert(cur_row, tnv_from_uint(range->toNode));
			tnvc_insert(cur_row, tnv_from_uint(range->owlt));

			tbl_add_row(table, cur_row);
		}
		else
		{
			AMP_DEBUG_WARN("dtn_ion_ionadmin_tblt_contacts", "Can't allocate row. Skipping.", NULL);
		}
	}

	  sdr_exit_xn(sdr);

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION tblt_ranges BODY
	 * +-------------------------------------------------------------------------+
	 */
	return table;
}


/* Collect Functions */
/*
 * This is how accurate the ION Agent's clock is described as number of seconds, an absolute value.
 */
tnv_t *dtn_ion_ionadmin_get_clock_error(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_clock_error BODY
	 * +-------------------------------------------------------------------------+
	 */

	 Sdr     sdr = getIonsdr();
	 Object  iondbObj = getIonDbObject();
	 IonDB   iondb;

	 CHKNULL(sdr_begin_xn(sdr));
	 sdr_stage(sdr, (char *) &iondb, iondbObj, sizeof(IonDB));
	 result = tnv_from_int(iondb.maxClockError);
	 sdr_end_xn(sdr);

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_clock_error BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * This is whether or not the the computer on which the local ION node is running has a synchronized cl
 * ock.
 */
tnv_t *dtn_ion_ionadmin_get_clock_sync(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_clock_sync BODY
	 * +-------------------------------------------------------------------------+
	 */
	Sdr     sdr;
	Object  iondbObj;
	IonDB   iondb;

	sdr = getIonsdr();
	iondbObj = getIonDbObject();
	CHKNULL(sdr_begin_xn(sdr));
	sdr_stage(sdr, (char *) &iondb, iondbObj, sizeof(IonDB));
	result = tnv_from_int(iondb.clockIsSynchronized);
	sdr_end_xn(sdr);

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_clock_sync BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * This is whether or not the node has a control that will set off alarm if it will become congested at
 *  some future time.
 */
tnv_t *dtn_ion_ionadmin_get_congestion_alarm_control(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_congestion_alarm_control BODY
	 * +-------------------------------------------------------------------------+
	 */

	Sdr     sdr = getIonsdr();
	Object  iondbObj = getIonDbObject();
	IonDB   iondb;
	char    alarmBuffer[40 + TIMESTAMPBUFSZ]; // Pulled from ION

	CHKNULL(sdr_begin_xn(sdr));
	sdr_stage(sdr, (char *) &iondb, iondbObj, sizeof(IonDB));


	if (iondb.alarmScript)
	{
		memset(alarmBuffer,0,sizeof(alarmBuffer));
		sdr_string_read(sdr, alarmBuffer, iondb.alarmScript);
		result = tnv_from_str(alarmBuffer);
	}

	sdr_end_xn(sdr);

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_congestion_alarm_control BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * This is the time horizon beyond which we don't attempt to forecast congestion
 */
tnv_t *dtn_ion_ionadmin_get_congestion_end_time_forecasts(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_congestion_end_time_forecasts BODY
	 * +-------------------------------------------------------------------------+
	 */

	Sdr     sdr = getIonsdr();
	Object  iondbObj = getIonDbObject();
	IonDB   iondb;

	CHKNULL(sdr_begin_xn(sdr));
	sdr_stage(sdr, (char *) &iondb, iondbObj, sizeof(IonDB));
	result = tnv_from_uint(iondb.horizon);
	sdr_end_xn(sdr);
	
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_congestion_end_time_forecasts BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * This is the mean rate of continuous data delivery to local BP applications.
 */
tnv_t *dtn_ion_ionadmin_get_consumption_rate(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_consumption_rate BODY
	 * +-------------------------------------------------------------------------+
	 */
	
	Sdr     sdr = getIonsdr();
	Object  iondbObj = getIonDbObject();
	IonDB   iondb;

	CHKNULL(sdr_begin_xn(sdr));
	sdr_stage(sdr, (char *) &iondb, iondbObj, sizeof(IonDB));
	result = tnv_from_uint(iondb.consumptionRate);
	sdr_end_xn(sdr);

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_consumption_rate BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * This is the maximum number of megabytes of storage space in ION's local file system that can be used
 *  for the storage of inbound zero-copy objects. The default heap limit is 1 Terabyte.
 */
tnv_t *dtn_ion_ionadmin_get_inbound_file_system_occupancy_limit(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_inbound_file_system_occupancy_limit BODY
	 * +-------------------------------------------------------------------------+
	 */

	Sdr     sdr = getIonsdr();
	Object  iondbObj = getIonDbObject();
	IonDB   iondb;
	vast    fileLimit;

	CHKNULL(sdr_begin_xn(sdr));
	fileLimit = zco_get_max_file_occupancy(sdr, ZcoInbound);
	result = tnv_from_vast(fileLimit);
	sdr_end_xn(sdr);

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_inbound_file_system_occupancy_limit BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * This is the maximum number of megabytes of storage space in ION's SDR non-volatile heap that can be 
 * used for the storage of inbound zero-copy objects. The default heap limit is 20% of the SDR data spa
 * ce's total heap size.
 */
tnv_t *dtn_ion_ionadmin_get_inbound_heap_occupancy_limit(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_inbound_heap_occupancy_limit BODY
	 * +-------------------------------------------------------------------------+
	 */

	Sdr     sdr = getIonsdr();
	Object  iondbObj = getIonDbObject();
	IonDB   iondb;
	vast    heapLimit;

	CHKNULL(sdr_begin_xn(sdr));
	heapLimit = zco_get_max_heap_occupancy(sdr, ZcoInbound);
	result = tnv_from_vast(heapLimit);
	sdr_end_xn(sdr);

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_inbound_heap_occupancy_limit BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * This is a CBHE node number which uniquely identifies the node in the delay-tolerant network.
 */
tnv_t *dtn_ion_ionadmin_get_number(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_number BODY
	 * +-------------------------------------------------------------------------+
	 */

	Sdr     sdr = getIonsdr();
	Object  iondbObj = getIonDbObject();
	IonDB   iondb;
	
	CHKNULL(sdr_begin_xn(sdr));
	sdr_stage(sdr, (char *) &iondb, iondbObj, sizeof(IonDB));
	result = tnv_from_uvast(iondb.ownNodeNbr);
	sdr_end_xn(sdr);
	  
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_number BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * This is the maximum number of megabytes of storage space in ION's local file system that can be used
 *  for the storage of outbound zero-copy objects. The default heap limit is 1 Terabyte.
 */
tnv_t *dtn_ion_ionadmin_get_outbound_file_system_occupancy_limit(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_outbound_file_system_occupancy_limit BODY
	 * +-------------------------------------------------------------------------+
	 */

	Sdr     sdr = getIonsdr();
	Object  iondbObj = getIonDbObject();
	IonDB   iondb;
	vast    fileLimit;

	CHKNULL(sdr_begin_xn(sdr));
	fileLimit = zco_get_max_file_occupancy(sdr, ZcoOutbound);
	result = tnv_from_vast(fileLimit);
	sdr_end_xn(sdr);
	
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_outbound_file_system_occupancy_limit BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * This is the maximum number of megabytes of storage space in ION's SDR non-volatile heap that can be 
 * used for the storage of outbound zero-copy objects. The default heap limit is 20% of the SDR data sp
 * ace's total heap size.
 */
tnv_t *dtn_ion_ionadmin_get_outbound_heap_occupancy_limit(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_outbound_heap_occupancy_limit BODY
	 * +-------------------------------------------------------------------------+
	 */

	Sdr     sdr = getIonsdr();
	Object  iondbObj = getIonDbObject();
	IonDB   iondb;
	vast    heapLimit;

	CHKNULL(sdr_begin_xn(sdr));
	heapLimit = zco_get_max_heap_occupancy(sdr, ZcoOutbound);
	result = tnv_from_vast(heapLimit);
	sdr_end_xn(sdr);
	
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_outbound_heap_occupancy_limit BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * This is the rate of local data production.
 */
tnv_t *dtn_ion_ionadmin_get_production_rate(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_production_rate BODY
	 * +-------------------------------------------------------------------------+
	 */

	Sdr sdr = getIonsdr();
	Object iondbObj = getIonDbObject();
	IonDB iondb;
	  
	CHKNULL(sdr_begin_xn(sdr));
	sdr_stage(sdr, (char *) &iondb, iondbObj, sizeof(IonDB));
	result = tnv_from_int(iondb.productionRate);
	sdr_end_xn(sdr);
	
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_production_rate BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * This is the reference time that will be used for interpreting relative time values from now until th
 * e next revision of reference time.
 */
tnv_t *dtn_ion_ionadmin_get_ref_time(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_ref_time BODY
	 * +-------------------------------------------------------------------------+
	 */

	
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_ref_time BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * The time delta is used to compensate for error (drift) in clocks, particularly spacecraft clocks. Th
 * e hardware clock on a spacecraft might gain or lose a few seconds every month, to the point at which
 *  its understanding of the current time - as reported out by the operating system - might differ sign
 * ificantly from the actual value of Unix Epoch time as reported by authoritative clocks on Earth. To 
 * compensate for this difference without correcting the clock itself (which can be difficult and dange
 * rous), ION simply adds the time delta to the Epoch time reported by the operating system.
 */
tnv_t *dtn_ion_ionadmin_get_time_delta(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_time_delta BODY
	 * +-------------------------------------------------------------------------+
	 */
		
	Sdr sdr = getIonsdr();
	Object iondbObj = getIonDbObject();
	IonDB iondb;
	  
	CHKNULL(sdr_begin_xn(sdr));
	sdr_stage(sdr, (char *) &iondb, iondbObj, sizeof(IonDB));
	result = tnv_from_int(iondb.deltaFromUTC);
	sdr_end_xn(sdr);
	
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_time_delta BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * This is the version of ION that is currently installed.
 */
tnv_t *dtn_ion_ionadmin_get_version(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_version BODY
	 * +-------------------------------------------------------------------------+
	 */

	char buffer[80]; // pulled from ionadmin.c
	isprintf(buffer, sizeof(buffer), "%s", IONVERSIONNUMBER);
	result = tnv_from_str(buffer);
	
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_version BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}



/* Control Functions */

/*
 * Until this control is executed, the local ION node does not exist and most ionadmin controls will fa
 * il. The control configures the local node to be identified by node_number, a CBHE node number which 
 * uniquely identifies the node in the delay-tolerant network.  It also configures ION's data space (SD
 * R) and shared working-memory region.  For this purpose it uses a set of default settings if no argum
 * ent follows node_number or if the argument following node_number is ''; otherwise it uses the config
 * uration settings found in a configuration file.  If configuration file name is provided, then the co
 * nfiguration file's name is implicitly 'hostname.ionconfig'; otherwise, ion_config_filename is taken 
 * to be the explicit configuration file name.
 */
tnv_t *dtn_ion_ionadmin_ctrl_node_init(eid_t *def_mgr, tnvc_t *parms, int8_t *status)
{
	tnv_t *result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_node_init BODY
	 * +-------------------------------------------------------------------------+
	 */

	
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_node_init BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * This management control sets ION's understanding of the accuracy of the scheduled start and stop tim
 * es of planned contacts, in seconds.  The default value is 1.
 */
tnv_t *dtn_ion_ionadmin_ctrl_node_clock_error_set(eid_t *def_mgr, tnvc_t *parms, int8_t *status)
{
	tnv_t *result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_node_clock_error_set BODY
	 * +-------------------------------------------------------------------------+
	 */
	 Sdr      sdr = getIonsdr();
	 Object   iondbObj = getIonDbObject();
	 IonDB    iondb;
	 uint32_t newMaxClockError;
	 int      success = 0;


	 newMaxClockError = adm_get_parm_uint(parms,0,&success);

	 if ((success) && (newMaxClockError >= 60) && (newMaxClockError <= 60))
	 {
		 CHKNULL(sdr_begin_xn(sdr));
		 sdr_stage(sdr, (char *) &iondb, iondbObj, sizeof(IonDB));
		 iondb.maxClockError = newMaxClockError;
		 sdr_write(sdr, iondbObj, (char *) &iondb, sizeof(IonDB));
		 sdr_end_xn(sdr);
		 *status = CTRL_SUCCESS;
	 }

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_node_clock_error_set BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * This management control reports whether or not the computer on which the local ION node is running h
 * as a synchronized clock.
 */
tnv_t *dtn_ion_ionadmin_ctrl_node_clock_sync_set(eid_t *def_mgr, tnvc_t *parms, int8_t *status)
{
	tnv_t *result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_node_clock_sync_set BODY
	 * +-------------------------------------------------------------------------+
	 */
	 Sdr     sdr;
	 Object  iondbObj;
	 IonDB   iondb;
	 int     newSyncVal;
	 int     success = 0;

	 newSyncVal = adm_get_parm_uint(parms,0,&success);

	 if(success)
	 {
		 sdr = getIonsdr();
		 iondbObj = getIonDbObject();
		 CHKNULL(sdr_begin_xn(sdr));
		 sdr_stage(sdr, (char *) &iondb, iondbObj, sizeof(IonDB));
		 iondb.clockIsSynchronized = (!(newSyncVal == 0));
		 sdr_write(sdr, iondbObj, (char *) &iondb, sizeof(IonDB));
		 sdr_end_xn(sdr);
		 *status = CTRL_SUCCESS;
	 }
	
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_node_clock_sync_set BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * This management control establishes a control which will automatically be executed whenever ionadmin
 *  predicts that the node will become congested at some future time.
 */
tnv_t *dtn_ion_ionadmin_ctrl_node_congestion_alarm_control_set(eid_t *def_mgr, tnvc_t *parms, int8_t *status)
{
	tnv_t *result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_node_congestion_alarm_control_set BODY
	 * +-------------------------------------------------------------------------+
	 */

	Sdr     sdr = getIonsdr();
	Object  iondbObj = getIonDbObject();
	IonDB   iondb;
	char    *newAlarmScript;

	newAlarmScript = adm_get_parm_obj(parms, 0, AMP_TYPE_STR);

	if(strlen(newAlarmScript) <= 255)
	{

		CHKNULL(sdr_begin_xn(sdr));
		sdr_stage(sdr, (char *) &iondb, iondbObj, sizeof(IonDB));
        if (iondb.alarmScript != 0)
        {
        	sdr_free(sdr, iondb.alarmScript);
        }

        iondb.alarmScript = sdr_string_create(sdr, newAlarmScript);
        sdr_write(sdr, iondbObj, (char *) &iondb, sizeof(IonDB));
        sdr_end_xn(sdr);
        *status = CTRL_SUCCESS;
	}

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_node_congestion_alarm_control_set BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * This management control sets the end time for computed congestion forecasts. Setting congestion fore
 * cast horizon to zero sets the congestion forecast end time to infinite time in the future: if there 
 * is any predicted net growth in bundle storage space occupancy at all, following the end of the last 
 * scheduled contact, then eventual congestion will be predicted. The default value is zero, i.e., no e
 * nd time.
 */
tnv_t *dtn_ion_ionadmin_ctrl_node_congestion_end_time_forecasts_set(eid_t *def_mgr, tnvc_t *parms, int8_t *status)
{
	tnv_t *result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_node_congestion_end_time_forecasts_set BODY
	 * +-------------------------------------------------------------------------+
	 */

	Sdr     sdr = getIonsdr();
	Object  iondbObj = getIonDbObject();
	char    *horizonString;
	time_t  refTime;
	time_t  horizon;
	IonDB   iondb;

	horizonString = adm_get_parm_obj(parms, 0, AMP_TYPE_STR);

	if (*horizonString == '0' && *(horizonString + 1) == 0)
	{
		horizon = 0;    /*      Remove horizon from database.   */
	}
	else
	{
//		refTime = _referenceTime(NULL); TODO: Figure out how to get this
		//horizon = readTimestampUTC(horizonString, refTime);
		horizon = readTimestampUTC(horizonString, 0);
	}

	CHKNULL(sdr_begin_xn(sdr));
	sdr_stage(sdr, (char *) &iondb, iondbObj, sizeof(IonDB));
	iondb.horizon = horizon;
	sdr_write(sdr, iondbObj, (char *) &iondb, sizeof(IonDB));
	sdr_end_xn(sdr);
	*status = CTRL_SUCCESS;

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_node_congestion_end_time_forecasts_set BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * This management control sets ION's expectation of the mean rate of continuous data delivery to local
 *  BP applications throughout the period of time over which congestion forecasts are computed. For nod
 * es that function only as routers this variable will normally be zero. A value of -1, which is the de
 * fault, indicates that the rate of local data consumption is unknown; in that case local data consump
 * tion is not considered in the computation of congestion forecasts.
 */
tnv_t *dtn_ion_ionadmin_ctrl_node_consumption_rate_set(eid_t *def_mgr, tnvc_t *parms, int8_t *status)
{
	tnv_t *result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_node_consumption_rate_set BODY
	 * +-------------------------------------------------------------------------+
	 */

	Sdr     sdr = getIonsdr();
	Object  iondbObj = getIonDbObject();
	IonDB   iondb;
	int     newRate;
	int 	success = 0;

	newRate = adm_get_parm_uint(parms, 0, &success);

	if(success)
	{
		if (newRate < 0)
		{
			newRate = -1;                   /*      Not metered.    */
		}

		CHKNULL(sdr_begin_xn(sdr));
		sdr_stage(sdr, (char *) &iondb, iondbObj, sizeof(IonDB));
		iondb.consumptionRate = newRate;
		sdr_write(sdr, iondbObj, (char *) &iondb, sizeof(IonDB));
		sdr_end_xn(sdr);
		*status = CTRL_SUCCESS;
	}

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_node_consumption_rate_set BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * This control schedules a period of data transmission from source_node to dest_node. The period of tr
 * ansmission will begin at start_time and end at stop_time, and the rate of data transmission will be 
 * xmit_data_rate bytes/second. Our confidence in the contact defaults to 1.0, indicating that the cont
 * act is scheduled - not that non-occurrence of the contact is impossible, just that occurrence of the
 *  contact is planned and scheduled rather than merely imputed from past node behavior. In the latter 
 * case, confidence indicates our estimation of the likelihood of this potential contact.
 */
tnv_t *dtn_ion_ionadmin_ctrl_node_contact_add(eid_t *def_mgr, tnvc_t *parms, int8_t *status)
{
	tnv_t *result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_node_contact_add BODY
	 * +-------------------------------------------------------------------------+
	 */

	time_t      fromTime = 0;
	time_t      toTime = 0;
	uvast       fromNodeNbr = 0;
	uvast       toNodeNbr = 0;
	int         regionIdx;
	PsmAddress  xaddr;
	uvast    	xmitRate;
	uvast       confidence;
	int     	success = 0;

	fromTime = adm_get_parm_uint(parms, 0, &success);

	if(success)
	{
		toTime = adm_get_parm_uint(parms, 1, &success);
	}

	if (toTime <= fromTime)
	{
		return NULL;
	}

	if(success)
	{
		fromNodeNbr = adm_get_parm_uvast(parms, 2, &success);
	}

	if(success)
	{
		toNodeNbr = adm_get_parm_uvast(parms, 3, &success);
	}

	regionIdx = ionRegionOf(fromNodeNbr, toNodeNbr);
	if(success)
	{
		xmitRate = adm_get_parm_uvast(parms, 4, &success);
	}

	if(success)
	{
		confidence = adm_get_parm_uvast(parms, 5, &success);
	}

	if(success)
	{
		if(rfx_insert_contact(regionIdx, fromTime, toTime, fromNodeNbr,
				toNodeNbr, xmitRate, confidence, &xaddr) == 0)
		{
			*status = CTRL_SUCCESS;
		}
	}


	
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_node_contact_add BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * This control deletes the scheduled period of data transmission from source_node to dest_node startin
 * g at start_time. To delete all contacts between some pair of nodes, use '*' as start_time.
 */
tnv_t *dtn_ion_ionadmin_ctrl_node_contact_del(eid_t *def_mgr, tnvc_t *parms, int8_t *status)
{
	tnv_t *result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_node_contact_del BODY
	 * +-------------------------------------------------------------------------+
	 */

	time_t  timestamp = 0;
	uvast   fromNodeNbr = 0;
	uvast   toNodeNbr = 0;
	int 	success = 0;

	timestamp = adm_get_parm_uint(parms, 0, &success);

	if(success)
	{
		fromNodeNbr = adm_get_parm_uint(parms, 1, &success);
	}
	if(success)
	{
		toNodeNbr = adm_get_parm_uint(parms, 2, &success);
	}

	if(success)
	{
		// TODO refTime = _referenceTime(NULL);
	    // TODO  timestamp = readTimestampUTC(tokens[2], refTime);

		if (timestamp == 0)
		{
			return NULL;
		}
		if(rfx_remove_contact(&timestamp, fromNodeNbr, toNodeNbr) == 0)
		{
			// TODO _forecastNeeded(1);
			*status = CTRL_SUCCESS;
		}
	}

	
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_node_contact_del BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * This management control sets the maximum number of megabytes of storage space in ION's SDR non-volat
 * ile heap that can be used for the storage of inbound zero-copy objects. A value of -1 for either lim
 * it signifies 'leave unchanged'. The default heap limit is 30% of the SDR data space's total heap siz
 * e.
 */
tnv_t *dtn_ion_ionadmin_ctrl_node_inbound_heap_occupancy_limit_set(eid_t *def_mgr, tnvc_t *parms, int8_t *status)
{
	tnv_t *result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_node_inbound_heap_occupancy_limit_set BODY
	 * +-------------------------------------------------------------------------+
	 */

	
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_node_inbound_heap_occupancy_limit_set BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * This management control sets the maximum number of megabytes of storage space in ION's SDR non-volat
 * ile heap that can be used for the storage of outbound zero-copy objects.  A value of  -1 for either 
 * limit signifies 'leave unchanged'. The default heap  limit is 30% of the SDR data space's total heap
 *  size.
 */
tnv_t *dtn_ion_ionadmin_ctrl_node_outbound_heap_occupancy_limit_set(eid_t *def_mgr, tnvc_t *parms, int8_t *status)
{
	tnv_t *result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_node_outbound_heap_occupancy_limit_set BODY
	 * +-------------------------------------------------------------------------+
	 */

	
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_node_outbound_heap_occupancy_limit_set BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * This management control sets ION's expectation of the mean rate of continuous data origination by lo
 * cal BP applications throughout the period of time over which congestion forecasts are computed. For 
 * nodes that function only as routers this variable will normally be zero. A value of -1, which is the
 *  default, indicates that the rate of local data production is unknown; in that case local data produ
 * ction is not considered in the computation of congestion forecasts.
 */
tnv_t *dtn_ion_ionadmin_ctrl_node_production_rate_set(eid_t *def_mgr, tnvc_t *parms, int8_t *status)
{
	tnv_t *result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_node_production_rate_set BODY
	 * +-------------------------------------------------------------------------+
	 */

	
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_node_production_rate_set BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * This control predicts a period of time during which the distance from node to other_node will be con
 * stant to within one light second. The period will begin at start_time and end at stop_time, and the 
 * distance between the nodes during that time will be distance light seconds.
 */
tnv_t *dtn_ion_ionadmin_ctrl_node_range_add(eid_t *def_mgr, tnvc_t *parms, int8_t *status)
{
	tnv_t *result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_node_range_add BODY
	 * +-------------------------------------------------------------------------+
	 */

	time_t  start = 0;
	time_t  stop  = 0;
	uint32_t    from_node = 0;
	uint32_t    to_node   = 0;
	uint32_t    distance  = 0;
	int 	success   = 0;
	PsmAddress xaddr;

	start = adm_get_parm_uint(parms, 0, &success);

	if(success)
	{
	  stop = adm_get_parm_uint(parms, 1, &success);
	}
	if(success)
	{
	  from_node = adm_get_parm_uint(parms, 2, &success);
	}
	if(success)
	{
	  to_node = adm_get_parm_uint(parms, 3, &success);
	}
	if(success)
	{
	  distance = adm_get_parm_uint(parms, 4, &success);
	}   

	if(success)
	{
	  if(stop <= start) 
	  {
	    return NULL;
	  }
	  
	  if(rfx_insert_range(start, stop, from_node, to_node, distance, &xaddr) >= 0 && xaddr != 0)
	  {
	    *status = CTRL_SUCCESS;
	  }
	}
	
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_node_range_add BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * This control deletes the predicted period of constant distance between node and other_node starting 
 * at start_time. To delete all ranges between some pair of nodes, use '*' as start_time.
 */
tnv_t *dtn_ion_ionadmin_ctrl_node_range_del(eid_t *def_mgr, tnvc_t *parms, int8_t *status)
{
	tnv_t *result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_node_range_del BODY
	 * +-------------------------------------------------------------------------+
	 */
	
	time_t  start = 0;
	uint32_t    from_node = 0;
	uint32_t    to_node   = 0;
	int 	success   = 0;
	PsmAddress xaddr;

	start = adm_get_parm_uint(parms, 0, &success);

	if(success)
	{
	  from_node = adm_get_parm_uint(parms, 1, &success);
	}
	if(success)
	{
	  to_node = adm_get_parm_uint(parms, 2, &success);
	}

	if(success)
	{
	  if(rfx_remove_range(&start, from_node, to_node) >= 0)
	  {
	    *status = CTRL_SUCCESS;
	  }
	}
	
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_node_range_del BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * This is used to set the reference time that will be used for interpreting relative time values from 
 * now until the next revision of reference time. Note that the new reference time can be a relative ti
 * me, i.e., an offset beyond the current reference time.
 */
tnv_t *dtn_ion_ionadmin_ctrl_node_ref_time_set(eid_t *def_mgr, tnvc_t *parms, int8_t *status)
{
	tnv_t *result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_node_ref_time_set BODY
	 * +-------------------------------------------------------------------------+
	 */

	
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_node_ref_time_set BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * This management control sets ION's understanding of the current difference between correct time and 
 * the Unix Epoch time values reported by the clock for the local ION node's computer. This delta is au
 * tomatically applied to locally obtained time values whenever ION needs to know the current time.
 */
tnv_t *dtn_ion_ionadmin_ctrl_node_time_delta_set(eid_t *def_mgr, tnvc_t *parms, int8_t *status)
{
	tnv_t *result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_node_time_delta_set BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_node_time_delta_set BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}



/* OP Functions */
