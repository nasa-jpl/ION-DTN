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
 **  2018-01-05  AUTO             Auto-generated c file 
 **
 ****************************************************************************/

/*   START CUSTOM INCLUDES HERE  */
#include "rfx.h"
/*   STOP CUSTOM INCLUDES HERE  */

#include "adm_ion_admin_impl.h"

/*   START CUSTOM FUNCTIONS HERE */
/*   STOP CUSTOM FUNCTIONS HERE  */

void adm_ion_admin_setup(){

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

void adm_ion_admin_cleanup(){

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


value_t adm_ion_admin_meta_name(tdc_t params)
{
	return val_from_string("adm_ion_admin");
}


value_t adm_ion_admin_meta_namespace(tdc_t params)
{
	return val_from_string("arn:DTN:ion_admin");
}


value_t adm_ion_admin_meta_version(tdc_t params)
{
	return val_from_string("V0.0");
}


value_t adm_ion_admin_meta_organization(tdc_t params)
{
	return val_from_string("JHUAPL");
}


/* Table Functions */


/*
 * This table shows all scheduled periods of data transmission.
 */

table_t* adm_ion_admin_tbl_contacts()
{
	table_t *table = NULL;
	if((table = table_create(NULL,NULL)) == NULL)
	{
		return NULL;
	}

	if(
		(table_add_col(table, "start_time", AMP_TYPE_TS) == ERROR) ||
		(table_add_col(table, "stop_time", AMP_TYPE_TS) == ERROR) ||
		(table_add_col(table, "source_node", AMP_TYPE_UINT) == ERROR) ||
		(table_add_col(table, "dest_node", AMP_TYPE_STR) == ERROR) ||
		(table_add_col(table, "xmit_data", AMP_TYPE_REAL32) == ERROR) ||
		(table_add_col(table, "prob", AMP_TYPE_REAL32) == ERROR))
	{
		table_destroy(table, 1);
		return NULL;
	}

	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION tbl_contacts BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION tbl_contacts BODY
	 * +-------------------------------------------------------------------------+
	 */
	return table;
}


/*
 * This table shows ION's current data space occupancy (the number of megabytes of space in the SDR non
 * -volatile heap and file system that are occupied by inbound and outbound zero-copy objects), the tot
 * al zero-copy-object space occupancy ceiling, and the maximum level of occupancy predicted by the mos
 * t recent ionadmin congestion forecast computation.
 */

table_t* adm_ion_admin_tbl_describe_usage()
{
	table_t *table = NULL;
	if((table = table_create(NULL,NULL)) == NULL)
	{
		return NULL;
	}

	if(
		(table_add_col(table, "current_data_space_occupancy", AMP_TYPE_UINT) == ERROR) ||
		(table_add_col(table, "total_zcp_space_occupancy", AMP_TYPE_UINT) == ERROR) ||
		(table_add_col(table, "max_lvl_occupancy", AMP_TYPE_UINT) == ERROR))
	{
		table_destroy(table, 1);
		return NULL;
	}

	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION tbl_describe_usage BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION tbl_describe_usage BODY
	 * +-------------------------------------------------------------------------+
	 */
	return table;
}


/*
 * This table shows all predicted periods of constant distance between nodes.
 */

table_t* adm_ion_admin_tbl_ranges()
{
	table_t *table = NULL;
	if((table = table_create(NULL,NULL)) == NULL)
	{
		return NULL;
	}

	if(
		(table_add_col(table, "start", AMP_TYPE_TS) == ERROR) ||
		(table_add_col(table, "stop", AMP_TYPE_TS) == ERROR) ||
		(table_add_col(table, "node", AMP_TYPE_UINT) == ERROR) ||
		(table_add_col(table, "other_node", AMP_TYPE_UINT) == ERROR) ||
		(table_add_col(table, "distance", AMP_TYPE_UINT) == ERROR))
	{
		table_destroy(table, 1);
		return NULL;
	}

	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION tbl_ranges BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION tbl_ranges BODY
	 * +-------------------------------------------------------------------------+
	 */
	return table;
}


/* Collect Functions */
/*
 * This is how accurate the ION Agent's clock is.
 */
value_t adm_ion_admin_get_clock_error(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_clock_error BODY
	 * +-------------------------------------------------------------------------+
	 */

	 Sdr     sdr = getIonsdr();
	 Object  iondbObj = getIonDbObject();
	 IonDB   iondb;

	 sdr_begin_xn(sdr);
	 sdr_stage(sdr, (char *) &iondb, iondbObj, sizeof(IonDB));
	 result = val_from_int(iondb.maxClockError);
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
value_t adm_ion_admin_get_clock_sync(tdc_t params)
{
	value_t result;
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
	sdr_begin_xn(sdr);
	sdr_stage(sdr, (char *) &iondb, iondbObj, sizeof(IonDB));
	result = val_from_int(iondb.clockIsSynchronized);
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
value_t adm_ion_admin_get_congestion_alarm_control(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_congestion_alarm_control BODY
	 * +-------------------------------------------------------------------------+
	 */

	Sdr     sdr = getIonsdr();
	Object  iondbObj = getIonDbObject();
	IonDB   iondb;
	char    alarmBuffer[40 + TIMESTAMPBUFSZ]; // Pulled from ION

	sdr_begin_xn(sdr);
	sdr_stage(sdr, (char *) &iondb, iondbObj, sizeof(IonDB));


	if (iondb.alarmScript)
	{
		memset(alarmBuffer,0,sizeof(alarmBuffer));
		sdr_string_read(sdr, alarmBuffer, iondb.alarmScript);
		result = val_from_string(alarmBuffer);
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
 * This is when the node will be predicted to be no longer congested.
 */
value_t adm_ion_admin_get_congestion_end_time_forecasts(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_congestion_end_time_forecasts BODY
	 * +-------------------------------------------------------------------------+
	 */

	Sdr     sdr = getIonsdr();
	Object  iondbObj = getIonDbObject();
	IonDB   iondb;

	sdr_begin_xn(sdr);
	sdr_stage(sdr, (char *) &iondb, iondbObj, sizeof(IonDB));
	result = val_from_uint(iondb.horizon);
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
value_t adm_ion_admin_get_consumption_rate(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_consumption_rate BODY
	 * +-------------------------------------------------------------------------+
	 */
	
	Sdr     sdr = getIonsdr();
	Object  iondbObj = getIonDbObject();
	IonDB   iondb;

	sdr_begin_xn(sdr);
	sdr_stage(sdr, (char *) &iondb, iondbObj, sizeof(IonDB));
	result = val_from_uint(iondb.consumptionRate);
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
value_t adm_ion_admin_get_inbound_file_system_occupancy_limit(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_inbound_file_system_occupancy_limit BODY
	 * +-------------------------------------------------------------------------+
	 */

	Sdr     sdr = getIonsdr();
	Object  iondbObj = getIonDbObject();
	IonDB   iondb;
	vast    fileLimit;

	sdr_begin_xn(sdr);
	fileLimit = zco_get_max_file_occupancy(sdr, ZcoInbound);
	result = val_from_vast(fileLimit);
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
 * used for the storage of inbound zero-copy objects. The default heap limit is 30% of the SDR data spa
 * ce's total heap size.
 */
value_t adm_ion_admin_get_inbound_heap_occupancy_limit(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_inbound_heap_occupancy_limit BODY
	 * +-------------------------------------------------------------------------+
	 */

	Sdr     sdr = getIonsdr();
	Object  iondbObj = getIonDbObject();
	IonDB   iondb;
	vast    heapLimit;

	sdr_begin_xn(sdr);
	heapLimit = zco_get_max_heap_occupancy(sdr, ZcoInbound);
	result = val_from_vast(heapLimit);
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
value_t adm_ion_admin_get_number(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_number BODY
	 * +-------------------------------------------------------------------------+
	 */

	Sdr     sdr = getIonsdr();
	Object  iondbObj = getIonDbObject();
	IonDB   iondb;
	
	sdr_begin_xn(sdr);
	sdr_stage(sdr, (char *) &iondb, iondbObj, sizeof(IonDB));
	result = val_from_uvast(iondb.ownNodeNbr);
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
value_t adm_ion_admin_get_outbound_file_system_occupancy_limit(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_outbound_file_system_occupancy_limit BODY
	 * +-------------------------------------------------------------------------+
	 */

	Sdr     sdr = getIonsdr();
	Object  iondbObj = getIonDbObject();
	IonDB   iondb;
	vast    fileLimit;

	sdr_begin_xn(sdr);
	fileLimit = zco_get_max_file_occupancy(sdr, ZcoOutbound);
	result = val_from_vast(fileLimit);
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
 * used for the storage of outbound zero-copy objects. The default heap limit is 30% of the SDR data sp
 * ace's total heap size.
 */
value_t adm_ion_admin_get_outbound_heap_occupancy_limit(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_outbound_heap_occupancy_limit BODY
	 * +-------------------------------------------------------------------------+
	 */

	Sdr     sdr = getIonsdr();
	Object  iondbObj = getIonDbObject();
	IonDB   iondb;
	vast    heapLimit;

	sdr_begin_xn(sdr);
	heapLimit = zco_get_max_heap_occupancy(sdr, ZcoOutbound);
	result = val_from_vast(heapLimit);
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
value_t adm_ion_admin_get_production_rate(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_production_rate BODY
	 * +-------------------------------------------------------------------------+
	 */

	Sdr sdr = getIonsdr();
	Object iondbObj = getIonDbObject();
	IonDB iondb;
	  
	sdr_begin_xn(sdr);
	sdr_stage(sdr, (char *) &iondb, iondbObj, sizeof(IonDB));
	result = val_from_int(iondb.productionRate);
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
value_t adm_ion_admin_get_ref_time(tdc_t params)
{
	value_t result;
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
 * The UTC delta is used to compensate for error (drift) in clocks, particularly spacecraft clocks. The
 *  hardware clock on a spacecraft might gain or lose a few seconds every month, to the point at which 
 * its understanding of the current time - as reported out by the operating system - might differ signi
 * ficantly from the actual value of UTC as reported by authoritative clocks on Earth. To compensate fo
 * r this difference without correcting the clock itself (which can be difficult and dangerous), ION si
 * mply adds the UTC delta to the UTC reported by the operating system.
 */
value_t adm_ion_admin_get_utc_delta(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_utc_delta BODY
	 * +-------------------------------------------------------------------------+
	 */
	
	Sdr sdr = getIonsdr();
	Object iondbObj = getIonDbObject();
	IonDB iondb;
	  
	sdr_begin_xn(sdr);
	sdr_stage(sdr, (char *) &iondb, iondbObj, sizeof(IonDB));
	result = val_from_int(iondb.deltaFromUTC);
	sdr_end_xn(sdr);
	
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_utc_delta BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * This is the version of ION that is currently installed.
 */
value_t adm_ion_admin_get_version(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_version BODY
	 * +-------------------------------------------------------------------------+
	 */

	char buffer[80]; // pulled from ionadmin.c
	isprintf(buffer, sizeof(buffer), "%s", IONVERSIONNUMBER);
	result = val_from_string(buffer);
	
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
tdc_t* adm_ion_admin_ctrl_node_init(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
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
tdc_t* adm_ion_admin_ctrl_node_clock_error_set(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
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
	 int8_t success = 0;


	 newMaxClockError = adm_extract_uint(params,0,&success);

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
tdc_t* adm_ion_admin_ctrl_node_clock_sync_set(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
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
	 int8_t success = 0;

	 newSyncVal = adm_extract_uint(params,0,&success);

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
tdc_t* adm_ion_admin_ctrl_node_congestion_alarm_control_set(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
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
	int8_t success = 0;

	newAlarmScript = adm_extract_string(params,0,&success);

	if((success) && (strlen(newAlarmScript) <= 255))
	{

		sdr_begin_xn(sdr);
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

	SRELEASE(newAlarmScript);



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
tdc_t* adm_ion_admin_ctrl_node_congestion_end_time_forecasts_set(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
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
	int8_t success = 0;

	horizonString = adm_extract_string(params,0,&success);

	if(success)
	{

		if (*horizonString == '0' && *(horizonString + 1) == 0)
		{
			horizon = 0;    /*      Remove horizon from database.   */
		}
		else
		{
//			refTime = _referenceTime(NULL); TODO: Figure out how to get this
			//horizon = readTimestampUTC(horizonString, refTime);
			horizon = readTimestampUTC(horizonString, 0);
		}

		sdr_begin_xn(sdr);
		sdr_stage(sdr, (char *) &iondb, iondbObj, sizeof(IonDB));
		iondb.horizon = horizon;
		sdr_write(sdr, iondbObj, (char *) &iondb, sizeof(IonDB));
		sdr_end_xn(sdr);
		*status = CTRL_SUCCESS;
	}

	SRELEASE(horizonString);

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
tdc_t* adm_ion_admin_ctrl_node_consumption_rate_set(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
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
	int8_t 	success = 0;

	newRate = adm_extract_uint(params,0,&success);

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
tdc_t* adm_ion_admin_ctrl_node_contact_add(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_node_contact_add BODY
	 * +-------------------------------------------------------------------------+
	 */

	time_t          fromTime = 0;
	time_t          toTime = 0;
	uvast           fromNodeNbr = 0;
	uvast           toNodeNbr = 0;
	PsmAddress      xaddr;
	unsigned int    xmitRate;
	float           confidence;
	int8_t 	success = 0;

	fromTime = adm_extract_uint(params,0,&success);

	if(success)
	{
		toTime = adm_extract_uint(params,1,&success);
	}

	if (toTime <= fromTime)
	{
		return NULL;
	}

	if(success)
	{
		fromNodeNbr = adm_extract_uvast(params,2,&success);
	}

	if(success)
	{
		toNodeNbr = adm_extract_uvast(params,3,&success);
	}

	if(success)
	{
		xmitRate = adm_extract_uint(params,4,&success);
	}

	if(success)
	{
		confidence = adm_extract_real32(params,5,&success);
	}

	if(success)
	{
		if(rfx_insert_contact(fromTime, toTime, fromNodeNbr,
				toNodeNbr, xmitRate, confidence, &xaddr) > 0)
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
tdc_t* adm_ion_admin_ctrl_node_contact_del(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_node_contact_del BODY
	 * +-------------------------------------------------------------------------+
	 */

	time_t  timestamp = 0;
	uvast   fromNodeNbr = 0;
	uvast   toNodeNbr = 0;
	int8_t 	success = 0;

	timestamp = adm_extract_uint(params,0,&success);

	if(success)
	{
		fromNodeNbr = adm_extract_uint(params,1,&success);
	}
	if(success)
	{
		toNodeNbr = adm_extract_uint(params,2,&success);
	}

	if(success)
	{
		// TODO refTime = _referenceTime(NULL);
	    // TODO  timestamp = readTimestampUTC(tokens[2], refTime);

		if (timestamp == 0)
		{
			return NULL;
		}
		if(rfx_remove_contact(timestamp, fromNodeNbr, toNodeNbr) > 0)
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
tdc_t* adm_ion_admin_ctrl_node_inbound_heap_occupancy_limit_set(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
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
 * ile heap that can be used for the storage of outbound zero-copy objects.  A value of -1 for either l
 * imit signifies 'leave unchanged'. The default heap limit is 30% of the SDR data space's total heap s
 * ize.
 */
tdc_t* adm_ion_admin_ctrl_node_outbound_heap_occupancy_limit_set(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
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
tdc_t* adm_ion_admin_ctrl_node_production_rate_set(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
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
tdc_t* adm_ion_admin_ctrl_node_range_add(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_node_range_add BODY
	 * +-------------------------------------------------------------------------+
	 */

	time_t  start = 0;
	time_t  stop  = 0;
	uint    from_node = 0;
	uint    to_node   = 0;
	uint    distance  = 0;
	int8_t 	success   = 0;
	PsmAddress xaddr;

	start = adm_extract_uint(params,0,&success);

	if(success)
	{
	  stop = adm_extract_uint(params,1,&success);
	}
	if(success)
	{
	  from_node = adm_extract_uint(params,2,&success);
	}
	if(success)
	{
	  to_node = adm_extract_uint(params, 3, &success);
	}
	if(success)
	{
	  distance = adm_extract_uint(params, 4, &success);
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
tdc_t* adm_ion_admin_ctrl_node_range_del(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_node_range_del BODY
	 * +-------------------------------------------------------------------------+
	 */
	
	time_t  start = 0;
	uint    from_node = 0;
	uint    to_node   = 0;
	int8_t 	success   = 0;
	PsmAddress xaddr;

	start = adm_extract_uint(params,0,&success);

	if(success)
	{
	  from_node = adm_extract_uint(params, 1, &success);
	}
	if(success)
	{
	  to_node = adm_extract_uint(params, 2, &success);
	}

	if(success)
	{
	  if(rfx_remove_range(start, from_node, to_node) >= 0)
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
tdc_t* adm_ion_admin_ctrl_node_ref_time_set(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
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
 * This management control sets ION's understanding of the current difference between correct UTC time 
 * and the time values reported by the clock for the local ION node's computer. This delta is automatic
 * ally applied to locally obtained time values whenever ION needs to know the current time.
 */
tdc_t* adm_ion_admin_ctrl_node_utc_delta_set(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_node_utc_delta_set BODY
	 * +-------------------------------------------------------------------------+
	 */

	
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_node_utc_delta_set BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}



/* OP Functions */
