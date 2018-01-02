/*****************************************************************************
 **
 ** File Name: ./agent/adm_IonAdmin_impl.c
 **
 ** Description: TODO
 **
 ** Notes: TODO
 **
 ** Assumptions: TODO
 **
 ** Modification History:
 **  YYYY-MM-DD  AUTHOR         DESCRIPTION
 **  ----------  ------------   ---------------------------------------------
 **  2017-11-11  AUTO           Auto generated c file 
 *****************************************************************************/

/*   START CUSTOM INCLUDES HERE  */
#include "rfx.h"
/*   STOP CUSTOM INCLUDES HERE   */

#include "adm_IonAdmin_impl.h"

/*   START CUSTOM FUNCTIONS HERE */
/*   STOP CUSTOM FUNCTIONS HERE  */


/* Metadata Functions */

value_t adm_IonAdmin_meta_name(tdc_t params)
{
	return val_from_string("adm_IonAdmin");
}

value_t adm_IonAdmin_meta_namespace(tdc_t params)
{
	return val_from_string("arn:DTN:IonAdmin");
}


value_t adm_IonAdmin_meta_version(tdc_t params)
{
	return val_from_string("V0.0");
}


value_t adm_IonAdmin_meta_organization(tdc_t params)
{
	return val_from_string("JHUAPL");
}



/* Collect Functions */
// This is how accurate the ION Agent's clock is.
value_t adm_IonAdmin_get_clockError(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_IonAdmin_get_clockError BODY
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
	 * |STOP CUSTOM FUNCTION adm_IonAdmin_get_clockError BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// This is whether or not the the computer on which the local ION node is running has a synchronized clock.
value_t adm_IonAdmin_get_clockSync(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_IonAdmin_get_clockSync BODY
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
	 * |STOP CUSTOM FUNCTION adm_IonAdmin_get_clockSync BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// This is whether or not the node has a control that will set off alarm if it will become congested at some future time.
value_t adm_IonAdmin_get_congestionAlarmControl(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_IonAdmin_get_congestionAlarmControl BODY
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
	 * |STOP CUSTOM FUNCTION adm_IonAdmin_get_congestionAlarmControl BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// This is when the node will be predicted to be no longer congested.
value_t adm_IonAdmin_get_congestionEndTimeForecasts(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_IonAdmin_get_congestionEndTimeForecasts BODY
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
	 * |STOP CUSTOM FUNCTION adm_IonAdmin_get_congestionEndTimeForecasts BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// This is the mean rate of continuous data delivery to local BP applications.
value_t adm_IonAdmin_get_consumptionRate(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_IonAdmin_get_consumptionRate BODY
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
	 * |STOP CUSTOM FUNCTION adm_IonAdmin_get_consumptionRate BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// This is the maximum number of megabytes of storage space in ION's local file system that can be used for the storage of inbound zero-copy objects. The default heap limit is 1 Terabyte.
value_t adm_IonAdmin_get_inboundFileSystemOccupancyLimit(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_IonAdmin_get_inboundFileSystemOccupancyLimit BODY
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
	 * |STOP CUSTOM FUNCTION adm_IonAdmin_get_inboundFileSystemOccupancyLimit BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// This is the maximum number of megabytes of storage space in ION's SDR non-volatile heap that can be used for the storage of inbound zero-copy objects. The default heap limit is 30% of the SDR data space's total heap size.
value_t adm_IonAdmin_get_inboundHeapOccupancyLimit(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_IonAdmin_get_inboundHeapOccupancyLimit BODY
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
	 * |STOP CUSTOM FUNCTION adm_IonAdmin_get_inboundHeapOccupancyLimit BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// This is a CBHE node number which uniquely identifies the node in the delay-tolerant network.
value_t adm_IonAdmin_get_number(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_IonAdmin_get_number BODY
	 * +-------------------------------------------------------------------------+
	 */

	
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_IonAdmin_get_number BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// This is the maximum number of megabytes of storage space in ION's local file system that can be used for the storage of outbound zero-copy objects. The default heap limit is 1 Terabyte.
value_t adm_IonAdmin_get_outboundFileSystemOccupancyLimit(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_IonAdmin_get_outboundFileSystemOccupancyLimit BODY
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
	 * |STOP CUSTOM FUNCTION adm_IonAdmin_get_outboundFileSystemOccupancyLimit BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// This is the maximum number of megabytes of storage space in ION's SDR non-volatile heap that can be used for the storage of outbound zero-copy objects. The default heap limit is 30% of the SDR data space's total heap size.
value_t adm_IonAdmin_get_outboundHeapOccupancyLimit(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_IonAdmin_get_outboundHeapOccupancyLimit BODY
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
	 * |STOP CUSTOM FUNCTION adm_IonAdmin_get_outboundHeapOccupancyLimit BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// This is the rate of local data production.
value_t adm_IonAdmin_get_productionRate(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_IonAdmin_get_productionRate BODY
	 * +-------------------------------------------------------------------------+
	 */

	
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_IonAdmin_get_productionRate BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// This is the reference time that will be used for interpreting relative time values from now until the next revision of reference time.
value_t adm_IonAdmin_get_refTime(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_IonAdmin_get_refTime BODY
	 * +-------------------------------------------------------------------------+
	 */

	
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_IonAdmin_get_refTime BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// The UTC delta is used to compensate for error (drift) in clocks, particularly spacecraft clocks. The hardware clock on a spacecraft might gain or lose a few seconds every month, to the point at which its understanding of the current time - as reported out by the operating system - might differ significantly from the actual value of UTC as reported by authoritative clocks on Earth. To compensate for this difference without correcting the clock itself (which can be difficult and dangerous), ION simply adds the UTC delta to the UTC reported by the operating system.
value_t adm_IonAdmin_get_utcDelta(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_IonAdmin_get_utcDelta BODY
	 * +-------------------------------------------------------------------------+
	 */

	
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_IonAdmin_get_utcDelta BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// This is the version of ION that is currently installed.
value_t adm_IonAdmin_get_version(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_IonAdmin_get_version BODY
	 * +-------------------------------------------------------------------------+
	 */

	
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_IonAdmin_get_version BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

/* Table Functions */

// This table shows all scheduled periods of data transmission.
// TS:startTime&TS:stopTime&UINT:srcNode&STR:destNode&REAL32:xmitRate&REAL32:prob
table_t *adm_IonAdmin_table_contacts()
{
	table_t  *table = NULL;

	if((table = table_create(NULL, NULL)) == NULL)
	{
		return NULL;
	}

	if((table_add_col(table, "startTime", AMP_TYPE_TS) == ERROR) ||
	   (table_add_col(table, "stopTime", AMP_TYPE_TS) == ERROR) ||
	   (table_add_col(table, "srcNode", AMP_TYPE_UINT) == ERROR) ||
	   (table_add_col(table, "destNode", AMP_TYPE_STRING) == ERROR) ||
	   (table_add_col(table, "xmitRate", AMP_TYPE_REAL32) == ERROR) ||
	   (table_add_col(table, "prob", AMP_TYPE_REAL32) == ERROR))
	{
		table_destroy(table, 1);
		return NULL;
	}

	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_IonBpAdmin_table_endpoints BODY
	 * +-------------------------------------------------------------------------+
	 */



	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_IonBpAdmin_table_endpoints BODY
	 * +-------------------------------------------------------------------------+
	 */

	return table;
}

// This table shows ION's current data space occupancy (the number of megabytes of space in the SDR non-volatile heap and file system that are occupied by inbound and outbound zero-copy objects), the total zero-copy-object space occupancy ceiling, and the maximum level of occupancy predicted by the most recent ionadmin congestion forecast computation.
// UINT:curDataSpaceOcc&UINT:totZcoSpaceOcc&UINT:maxLvlOcc
table_t *adm_IonAdmin_table_usage()
{
	table_t  *table = NULL;

	if((table = table_create(NULL, NULL)) == NULL)
	{
		return NULL;
	}

	if((table_add_col(table, "curDataSpaceOcc", AMP_TYPE_UINT) == ERROR) ||
	   (table_add_col(table, "totZcoSpaceOcc", AMP_TYPE_UINT) == ERROR) ||
	   (table_add_col(table, "maxLvlOcc", AMP_TYPE_UINT) == ERROR))
	{
		table_destroy(table, 1);
		return NULL;
	}

	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_IonBpAdmin_table_usage BODY
	 * +-------------------------------------------------------------------------+
	 */



	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_IonBpAdmin_table_usage BODY
	 * +-------------------------------------------------------------------------+
	 */

	return table;

}

// This table shows all predicted periods of constant distance between nodes.
// TS:start&TS:stop&UINT:node&UINT:otherNode&UINT:distance
table_t *adm_IonAdmin_table_ranges()
{
	table_t  *table = NULL;

	if((table = table_create(NULL, NULL)) == NULL)
	{
		return NULL;
	}

	if((table_add_col(table, "start", AMP_TYPE_TS) == ERROR) ||
	   (table_add_col(table, "stop", AMP_TYPE_TS) == ERROR) ||
	   (table_add_col(table, "node", AMP_TYPE_UINT) == ERROR) ||
	   (table_add_col(table, "otherNode", AMP_TYPE_UINT) == ERROR) ||
	   (table_add_col(table, "distance", AMP_TYPE_UINT) == ERROR))
	{
		table_destroy(table, 1);
		return NULL;
	}

	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_IonBpAdmin_table_ranges BODY
	 * +-------------------------------------------------------------------------+
	 */



	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_IonBpAdmin_table_ranges BODY
	 * +-------------------------------------------------------------------------+
	 */

	return table;


}

/* Control Functions */
// Until this control is executed, the local ION node does not exist and most ionadmin controls will fail. The control configures the local node to be identified by nodeNumber, a CBHE node number which uniquely identifies the node in the delay-tolerant network.  It also configures ION's data space (SDR) and shared working-memory region.  For this purpose it uses a set of default settings if no argument follows nodeNumber or if the argument following nodeNumber is ''; otherwise it uses the configuration settings found in a configuration file.  If configuration file name '.' is provided, then the configuration file's name is implicitly 'hostname.ionconfig'; otherwise, ionConfigFileName is taken to be the explicit configuration file name.
tdc_t *adm_IonAdmin_ctrl_nodeInit(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_IonAdmin_ctrl_nodeInit BODY
	 * +-------------------------------------------------------------------------+
	 */

	
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_IonAdmin_ctrl_nodeInit BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// This management control sets ION's understanding of the accuracy of the scheduled start and stop times of planned contacts, in seconds.  The default value is 1.
tdc_t *adm_IonAdmin_ctrl_nodeClockErrorSet(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_IonAdmin_ctrl_nodeClockErrorSet BODY
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
	 * |STOP CUSTOM FUNCTION adm_IonAdmin_ctrl_nodeClockErrorSet BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// This management control reports whether or not the computer on which the local ION node is running has a synchronized clock.
tdc_t *adm_IonAdmin_ctrl_nodeClockSyncSet(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_IonAdmin_ctrl_nodeClockSyncSet BODY
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
	 * |STOP CUSTOM FUNCTION adm_IonAdmin_ctrl_nodeClockSyncSet BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// This management control establishes a control which will automatically be executed whenever ionadmin predicts that the node will become congested at some future time.
tdc_t *adm_IonAdmin_ctrl_nodeCongestionAlarmControlSet(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_IonAdmin_ctrl_nodeCongestionAlarmControlSet BODY
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
	 * |STOP CUSTOM FUNCTION adm_IonAdmin_ctrl_nodeCongestionAlarmControlSet BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// This management control sets the end time for computed congestion forecasts. Setting congestion forecast horizon to zero sets the congestion forecast end time to infinite time in the future: if there is any predicted net growth in bundle storage space occupancy at all, following the end of the last scheduled contact, then eventual congestion will be predicted. The default value is zero, i.e., no end time.
tdc_t *adm_IonAdmin_ctrl_nodeCongestionEndTimeForecastsSet(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_IonAdmin_ctrl_nodeCongestionEndTimeForecastsSet BODY
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
	 * |STOP CUSTOM FUNCTION adm_IonAdmin_ctrl_nodeCongestionEndTimeForecastsSet BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// This management control sets ION's expectation of the mean rate of continuous data delivery to local BP applications throughout the period of time over which congestion forecasts are computed. For nodes that function only as routers this variable will normally be zero. A value of -1, which is the default, indicates that the rate of local data consumption is unknown; in that case local data consumption is not considered in the computation of congestion forecasts.
tdc_t *adm_IonAdmin_ctrl_nodeConsumptionRateSet(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_IonAdmin_ctrl_nodeConsumptionRateSet BODY
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
	 * |STOP CUSTOM FUNCTION adm_IonAdmin_ctrl_nodeConsumptionRateSet BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// This control schedules a period of data transmission from sourceNode to destNode. The period of transmission will begin at startTime and end at stopTime, and the rate of data transmission will be xmitDataRate bytes/second. Our confidence in the contact defaults to 1.0, indicating that the contact is scheduled - not that non-occurrence of the contact is impossible, just that occurrence of the contact is planned and scheduled rather than merely imputed from past node behavior. In the latter case, confidence indicates our estimation of the likelihood of this potential contact.
// "TS:start&TS:stop&UINT:nodeId&STR:dest&UINT:dataRate&FLOAT32:prob",
tdc_t *adm_IonAdmin_ctrl_nodeContactAdd(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_IonAdmin_ctrl_nodeContactAdd BODY
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
	 * |STOP CUSTOM FUNCTION adm_IonAdmin_ctrl_nodeContactAdd BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// This control deletes the scheduled period of data transmission from sourceNode to destNode starting at startTime. To delete all contacts between some pair of nodes, use '*' as startTime.
tdc_t *adm_IonAdmin_ctrl_nodeContactDel(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_IonAdmin_ctrl_nodeContactDel BODY
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
	 * |STOP CUSTOM FUNCTION adm_IonAdmin_ctrl_nodeContactDel BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// This management control sets the maximum number of megabytes of storage space in ION's SDR non-volatile heap that can be used for the storage of inbound zero-copy objects. A value of -1 for either limit signifies 'leave unchanged'. The default heap limit is 30% of the SDR data space's total heap size.
tdc_t *adm_IonAdmin_ctrl_nodeInboundHeapOccupancyLimitSet(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_IonAdmin_ctrl_nodeInboundHeapOccupancyLimitSet BODY
	 * +-------------------------------------------------------------------------+
	 */

	
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_IonAdmin_ctrl_nodeInboundHeapOccupancyLimitSet BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// This management control sets the maximum number of megabytes of storage space in ION's SDR non-volatile heap that can be used for the storage of outbound zero-copy objects.  A value of -1 for either limit signifies 'leave unchanged'. The default heap limit is 30% of the SDR data space's total heap size.
tdc_t *adm_IonAdmin_ctrl_nodeOutboundHeapOccupancyLimitSet(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_IonAdmin_ctrl_nodeOutboundHeapOccupancyLimitSet BODY
	 * +-------------------------------------------------------------------------+
	 */

	
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_IonAdmin_ctrl_nodeOutboundHeapOccupancyLimitSet BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// This management control sets ION's expectation of the mean rate of continuous data origination by local BP applications throughout the period of time over which congestion forecasts are computed. For nodes that function only as routers this variable will normally be zero. A value of -1, which is the default, indicates that the rate of local data production is unknown; in that case local data production is not considered in the computation of congestion forecasts.
tdc_t *adm_IonAdmin_ctrl_nodeProductionRateSet(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_IonAdmin_ctrl_nodeProductionRateSet BODY
	 * +-------------------------------------------------------------------------+
	 */

	
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_IonAdmin_ctrl_nodeProductionRateSet BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// This control predicts a period of time during which the distance from node to otherNode will be constant to within one light second. The period will begin at startTime and end at stopTime, and the distance between the nodes during that time will be distance light seconds.
tdc_t *adm_IonAdmin_ctrl_nodeRangeAdd(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_IonAdmin_ctrl_nodeRangeAdd BODY
	 * +-------------------------------------------------------------------------+
	 */

	
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_IonAdmin_ctrl_nodeRangeAdd BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// This control deletes the predicted period of constant distance between node and otherNode starting at startTime. To delete all ranges between some pair of nodes, use '*' as startTime.
tdc_t *adm_IonAdmin_ctrl_nodeRangeDel(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_IonAdmin_ctrl_nodeRangeDel BODY
	 * +-------------------------------------------------------------------------+
	 */

	
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_IonAdmin_ctrl_nodeRangeDel BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// This is used to set the reference time that will be used for interpreting relative time values from now until the next revision of reference time. sNote that the new reference time can be a relative time, i.e., an offset beyond the current reference time.
tdc_t *adm_IonAdmin_ctrl_nodeRefTimeSet(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_IonAdmin_ctrl_nodeRefTimeSet BODY
	 * +-------------------------------------------------------------------------+
	 */

	
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_IonAdmin_ctrl_nodeRefTimeSet BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// This management control sets ION's understanding of the current difference between correct UTC time and the time values reported by the clock for the local ION node's computer. This delta is automatically applied to locally obtained time values whenever ION needs to know the current time.
tdc_t *adm_IonAdmin_ctrl_nodeUTCDeltaSet(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_IonAdmin_ctrl_nodeUTCDeltaSet BODY
	 * +-------------------------------------------------------------------------+
	 */

	
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_IonAdmin_ctrl_nodeUTCDeltaSet BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}



/* OP Functions */
