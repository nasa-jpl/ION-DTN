/*
 * ion_admin.c
 *
 * Implementation of public API functions for ION administration.
 *
 * This implementation provides programmatic access to ION configuration
 * parameters that were previously only accessible through ionadmin CLI.
 *
 * Copyright (c) 2025, California Institute of Technology.
 * ALL RIGHTS RESERVED.  U.S. Government Sponsorship acknowledged.
 */

#include "ion.h"
#include "ion_admin.h"
#include "rfx.h"

/*
 * =============================================================================
 * Production Rate Management
 * =============================================================================
 */

int ion_set_production_rate(int rate_bytes_per_sec)
{
	Sdr	sdr;
	SdrObject iondbObj;
	IonDB	iondb;

	/* Validate that ION is attached */
	sdr = getIonsdr();
	if (sdr == NULL)
	{
		putErrmsg("ION not attached. Call ionAttach() first.", NULL);
		return -1;
	}

	iondbObj = getIonDbObject();
	if (iondbObj == 0)
	{
		putErrmsg("ION database object not found.", NULL);
		return -1;
	}

	/* Normalize negative values to -1 (not metered) */
	if (rate_bytes_per_sec < 0)
	{
		rate_bytes_per_sec = -1;
	}

	/* Begin SDR transaction to update production rate */
	if (sdr_begin_xn(sdr) < 0)
	{
		putErrmsg("Failed to begin SDR transaction.", NULL);
		return -1;
	}

	/* Stage the IonDB structure, modify it, and write it back */
	sdr_stage(sdr, (char *) &iondb, iondbObj, sizeof(IonDB));
	iondb.productionRate = rate_bytes_per_sec;
	sdr_write(sdr, iondbObj, (char *) &iondb, sizeof(IonDB));

	/* Commit transaction */
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Failed to update production rate.", NULL);
		return -1;
	}

	/* Trigger congestion forecast update */
	return pseudoshell("ionwarn");
}

int ion_get_production_rate(int *rate_bytes_per_sec)
{
	Sdr	sdr;
	SdrObject iondbObj;
	IonDB	iondb;

	if (rate_bytes_per_sec == NULL)
	{
		putErrmsg("Invalid parameter: rate_bytes_per_sec is NULL.", NULL);
		return -1;
	}

	/* Validate that ION is attached */
	sdr = getIonsdr();
	if (sdr == NULL)
	{
		putErrmsg("ION not attached. Call ionAttach() first.", NULL);
		return -1;
	}

	iondbObj = getIonDbObject();
	if (iondbObj == 0)
	{
		putErrmsg("ION database object not found.", NULL);
		return -1;
	}

	/* Read the current production rate */
	if (sdr_begin_xn(sdr) < 0)
	{
		putErrmsg("Failed to begin SDR transaction.", NULL);
		return -1;
	}

	sdr_read(sdr, (char *) &iondb, iondbObj, sizeof(IonDB));
	*rate_bytes_per_sec = iondb.productionRate;

	sdr_exit_xn(sdr);

	return 0;
}

/*
 * =============================================================================
 * Consumption Rate Management
 * =============================================================================
 */

int ion_set_consumption_rate(int rate_bytes_per_sec)
{
	Sdr	sdr;
	SdrObject iondbObj;
	IonDB	iondb;

	/* Validate that ION is attached */
	sdr = getIonsdr();
	if (sdr == NULL)
	{
		putErrmsg("ION not attached. Call ionAttach() first.", NULL);
		return -1;
	}

	iondbObj = getIonDbObject();
	if (iondbObj == 0)
	{
		putErrmsg("ION database object not found.", NULL);
		return -1;
	}

	/* Normalize negative values to -1 (not metered) */
	if (rate_bytes_per_sec < 0)
	{
		rate_bytes_per_sec = -1;
	}

	/* Begin SDR transaction to update consumption rate */
	if (sdr_begin_xn(sdr) < 0)
	{
		putErrmsg("Failed to begin SDR transaction.", NULL);
		return -1;
	}

	/* Stage the IonDB structure, modify it, and write it back */
	sdr_stage(sdr, (char *) &iondb, iondbObj, sizeof(IonDB));
	iondb.consumptionRate = rate_bytes_per_sec;
	sdr_write(sdr, iondbObj, (char *) &iondb, sizeof(IonDB));

	/* Commit transaction */
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Failed to update consumption rate.", NULL);
		return -1;
	}

	/* Trigger congestion forecast update */
	return pseudoshell("ionwarn");
}

int ion_get_consumption_rate(int *rate_bytes_per_sec)
{
	Sdr	sdr;
	SdrObject iondbObj;
	IonDB	iondb;

	if (rate_bytes_per_sec == NULL)
	{
		putErrmsg("Invalid parameter: rate_bytes_per_sec is NULL.", NULL);
		return -1;
	}

	/* Validate that ION is attached */
	sdr = getIonsdr();
	if (sdr == NULL)
	{
		putErrmsg("ION not attached. Call ionAttach() first.", NULL);
		return -1;
	}

	iondbObj = getIonDbObject();
	if (iondbObj == 0)
	{
		putErrmsg("ION database object not found.", NULL);
		return -1;
	}

	/* Read the current consumption rate */
	if (sdr_begin_xn(sdr) < 0)
	{
		putErrmsg("Failed to begin SDR transaction.", NULL);
		return -1;
	}

	sdr_read(sdr, (char *) &iondb, iondbObj, sizeof(IonDB));
	*rate_bytes_per_sec = iondb.consumptionRate;

	sdr_exit_xn(sdr);

	return 0;
}

/*
 * =============================================================================
 * Congestion Forecast Horizon Management
 * =============================================================================
 */

int ion_set_congestion_forecast_horizon(time_t horizon_time)
{
	Sdr	sdr;
	SdrObject iondbObj;
	IonDB	iondb;

	/* Validate that ION is attached */
	sdr = getIonsdr();
	if (sdr == NULL)
	{
		putErrmsg("ION not attached. Call ionAttach() first.", NULL);
		return -1;
	}

	iondbObj = getIonDbObject();
	if (iondbObj == 0)
	{
		putErrmsg("ION database object not found.", NULL);
		return -1;
	}

	/* Begin SDR transaction to update horizon */
	if (sdr_begin_xn(sdr) < 0)
	{
		putErrmsg("Failed to begin SDR transaction.", NULL);
		return -1;
	}

	/* Stage the IonDB structure, modify it, and write it back */
	sdr_stage(sdr, (char *) &iondb, iondbObj, sizeof(IonDB));
	iondb.horizon = horizon_time;
	sdr_write(sdr, iondbObj, (char *) &iondb, sizeof(IonDB));

	/* Commit transaction */
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Failed to update congestion forecast horizon.", NULL);
		return -1;
	}

	/* trigger forecast */
	return pseudoshell("ionwarn");
}

int ion_get_congestion_forecast_horizon(time_t *horizon_time)
{
	Sdr	sdr;
	SdrObject iondbObj;
	IonDB	iondb;

	if (horizon_time == NULL)
	{
		putErrmsg("Invalid parameter: horizon_time is NULL.", NULL);
		return -1;
	}

	/* Validate that ION is attached */
	sdr = getIonsdr();
	if (sdr == NULL)
	{
		putErrmsg("ION not attached. Call ionAttach() first.", NULL);
		return -1;
	}

	iondbObj = getIonDbObject();
	if (iondbObj == 0)
	{
		putErrmsg("ION database object not found.", NULL);
		return -1;
	}

	/* Read the current horizon time */
	if (sdr_begin_xn(sdr) < 0)
	{
		putErrmsg("Failed to begin SDR transaction.", NULL);
		return -1;
	}

	sdr_read(sdr, (char *) &iondb, iondbObj, sizeof(IonDB));
	*horizon_time = iondb.horizon;

	sdr_exit_xn(sdr);

	return 0;
}

/*
 * =============================================================================
 * Node Registration
 * =============================================================================
 */

int ion_register_node(uint32_t region_nbr)
{
	PsmAddress	xaddr;
	uvast		ownFqnn;
	int		announce = 0;	/* Assumption: Don't announce registration */

	/*
	 * NOTE: This public API wrapper makes the following assumptions:
	 * - announce = 0 (registration is private to local node, not announced)
	 *
	 * For advanced use cases requiring registration announcement,
	 * use rfx_insert_contact() directly.
	 */

	/* Validate that ION is attached */
	if (getIonsdr() == NULL)
	{
		putErrmsg("ION not attached. Call ionAttach() first.", NULL);
		return -1;
	}

	/* Validate region number */
	if (region_nbr == 0)
	{
		putErrmsg("Region number must be greater than zero.", NULL);
		return -1;
	}

	ownFqnn = getOwnFqnn();

	/*
	 * Register the local node in the specified region by inserting
	 * a registration contact. This is equivalent to what ionadmin's
	 * "1 <nodeNbr> <configFile>" command does.
	 *
	 * A registration contact has:
	 * - from_time = MAX_POSIX_TIME
	 * - to_time = MAX_POSIX_TIME
	 * - from_node = to_node = local node
	 * - xmit_rate = 0
	 * - confidence = 1.0
	 */
	if (rfx_insert_contact(region_nbr, MAX_POSIX_TIME, MAX_POSIX_TIME,
			ownFqnn, ownFqnn, 0, 1.0, &xaddr, announce) == 0)
	{
		return 0;
	}

	return -1;
}

/*
 * =============================================================================
 * Contact Management
 * =============================================================================
 */

int ion_add_contact(time_t from_time, time_t to_time,
                    uvast from_node, uvast to_node,
                    unsigned int xmit_rate, float confidence)
{
	PsmAddress	xaddr;
	uvast		ownFqnn;
	unsigned int	rate;
	uint32_t	regionNbr;
	int		announce = 0;	/* Assumption: Don't announce contact (private) */

	/*
	 * NOTE: This public API wrapper makes the following assumptions:
	 * - announce = 0 (contact is private to local node, not announced)
	 * - regionNbr is determined automatically using ionRegionOf()
	 *
	 * For advanced use cases requiring contact announcement,
	 * use rfx_insert_contact() directly.
	 */

	/* Validate that ION is attached */
	if (getIonsdr() == NULL)
	{
		putErrmsg("ION not attached. Call ionAttach() first.", NULL);
		return -1;
	}

	/* Validate node numbers */
	if (from_node <= 0)
	{
		putErrmsg("'From' node number must be greater than zero.", NULL);
		return -1;
	}

	if (to_node <= 0)
	{
		putErrmsg("'To' node number must be greater than zero.", NULL);
		return -1;
	}

	/* Validate confidence */
	if (confidence < 0.0 || confidence > 1.0)
	{
		putErrmsg("Confidence must be in the range 0.0 to 1.0.", NULL);
		return -1;
	}

	ownFqnn = getOwnFqnn();

	/*
	 * Use region 1 as the default region.
	 * This matches ionadmin's _regionNbr static variable default.
	 *
	 * Note: rfx_insert_contact() will automatically register
	 * the nodes in this region if they aren't already registered,
	 * so we don't need to do any manual registration.
	 */
	regionNbr = 1;

	/* Handle registration contact (from_time == MAX_POSIX_TIME) */
	if (from_time == MAX_POSIX_TIME)
	{
		if (from_node != to_node)
		{
			putErrmsg("For registration contact, from and to nodes "
					"must be identical.", NULL);
			return -1;
		}

		rate = 0;
		confidence = 1.0;
	}
	/* Handle hypothetical contact (from_time == 0) */
	else if (from_time == 0)
	{
		if (from_node == to_node
		|| (from_node != ownFqnn && to_node != ownFqnn))
		{
			putErrmsg("For hypothetical contact, either from or to "
					"node must be the local node and the other "
					"must not.", NULL);
			return -1;
		}

		rate = 0;
		confidence = 0.0;
	}
	/* Handle scheduled contact */
	else
	{
		if (to_time <= from_time)
		{
			putErrmsg("Interval end time must be later than start "
					"time and earlier than 19 January 2038.", NULL);
			return -1;
		}

		rate = xmit_rate;
	}

	/* Insert the contact */
	if (rfx_insert_contact(regionNbr, from_time, to_time,
			from_node, to_node, rate, confidence,
			&xaddr, announce) == 0)
	{
		/*
		 * NOTE: ionadmin sets a forecast flag here, which triggers
		 * congestion forecast recalculation later. The public API
		 * does not currently expose this forecast triggering mechanism.
		 * Applications should be aware that adding contacts may require
		 * additional steps to update congestion forecasts if needed.
		 */
		return 0;
	}

	return -1;
}

int ion_delete_contact(time_t *from_time, uvast from_node, uvast to_node)
{
	uint32_t	regionNbr = 1;	/* Default region */
	int		announce = 0;	/* Assumption: Don't announce deletion (private) */

	/*
	 * NOTE: This public API wrapper makes the following assumptions:
	 * - announce = 0 (deletion is private to local node, not announced)
	 * - regionNbr = 1 (default region)
	 *
	 * For advanced use cases requiring deletion announcement or
	 * different region numbers, use rfx_remove_contact() directly.
	 */

	/* Validate that ION is attached */
	if (getIonsdr() == NULL)
	{
		putErrmsg("ION not attached. Call ionAttach() first.", NULL);
		return -1;
	}

	/* Delete the contact */
	if (rfx_remove_contact(regionNbr, from_time, from_node,
			to_node, announce) < 0)
	{
		return -1;
	}

	/*
	 * NOTE: ionadmin sets a forecast flag here. See note in
	 * ion_add_contact() about forecast triggering.
	 */
	return 0;
}

int ion_list_contacts(void)
{
	Sdr		sdr;
	PsmPartition	ionwm;
	IonVdb		*vdb;
	PsmAddress	elt;
	PsmAddress	addr;
	char		buffer[RFX_NOTE_LEN];

	/* Validate that ION is attached */
	sdr = getIonsdr();
	if (sdr == NULL)
	{
		putErrmsg("ION not attached. Call ionAttach() first.", NULL);
		return -1;
	}

	ionwm = getIonwm();
	vdb = getIonVdb();
	if (vdb == NULL)
	{
		putErrmsg("ION volatile database not found.", NULL);
		return -1;
	}

	/* Begin transaction */
	CHKERR(sdr_begin_xn(sdr));

	/* Iterate through all contacts and print them */
	for (elt = sm_rbt_first(ionwm, vdb->contactIndex); elt;
			elt = sm_rbt_next(ionwm, elt))
	{
		addr = sm_rbt_data(ionwm, elt);
		rfx_print_contact(addr, buffer);
		writeMemo(buffer);
	}

	sdr_exit_xn(sdr);

	return 0;
}

/*
 * =============================================================================
 * Range Management
 * =============================================================================
 */

int ion_add_range(time_t from_time, time_t to_time,
                  uvast from_node, uvast to_node,
                  unsigned int owlt)
{
	PsmAddress	xaddr;
	int		announce = 0;	/* Assumption: Don't announce range (private) */

	/*
	 * NOTE: This public API wrapper makes the following assumptions:
	 * - announce = 0 (range is private to local node, not announced)
	 *
	 * For advanced use cases requiring range announcement,
	 * use rfx_insert_range() directly.
	 */

	/* Validate that ION is attached */
	if (getIonsdr() == NULL)
	{
		putErrmsg("ION not attached. Call ionAttach() first.", NULL);
		return -1;
	}

	/* Validate node numbers */
	if (from_node <= 0)
	{
		putErrmsg("'From' node number must be greater than zero.", NULL);
		return -1;
	}

	if (to_node <= 0)
	{
		putErrmsg("'To' node number must be greater than zero.", NULL);
		return -1;
	}

	/* Validate time range */
	if (to_time <= from_time)
	{
		putErrmsg("Interval end time must be later than start time "
			"and earlier than 19 January 2038.", NULL);
		return -1;
	}

	/* Insert the range */
	if (rfx_insert_range(from_time, to_time, from_node, to_node,
			owlt, &xaddr, announce) == 0)
	{
		return 0;
	}

	return -1;
}

int ion_delete_range(time_t *from_time, uvast from_node, uvast to_node)
{
	int	announce = 0;	/* Assumption: Don't announce deletion (private) */

	/*
	 * NOTE: This public API wrapper makes the following assumptions:
	 * - announce = 0 (deletion is private to local node, not announced)
	 *
	 * For advanced use cases requiring deletion announcement,
	 * use rfx_remove_range() directly.
	 */

	/* Validate that ION is attached */
	if (getIonsdr() == NULL)
	{
		putErrmsg("ION not attached. Call ionAttach() first.", NULL);
		return -1;
	}

	/* Delete the range */
	if (rfx_remove_range(from_time, from_node, to_node, announce) < 0)
	{
		return -1;
	}

	return 0;
}

int ion_list_ranges(void)
{
	Sdr		sdr;
	PsmPartition	ionwm;
	IonVdb		*vdb;
	PsmAddress	elt;
	PsmAddress	addr;
	char		buffer[RFX_NOTE_LEN];

	/* Validate that ION is attached */
	sdr = getIonsdr();
	if (sdr == NULL)
	{
		putErrmsg("ION not attached. Call ionAttach() first.", NULL);
		return -1;
	}

	ionwm = getIonwm();
	vdb = getIonVdb();
	if (vdb == NULL)
	{
		putErrmsg("ION volatile database not found.", NULL);
		return -1;
	}

	/* Begin transaction */
	CHKERR(sdr_begin_xn(sdr));

	/* Iterate through all ranges and print them */
	for (elt = sm_rbt_first(ionwm, vdb->rangeIndex); elt;
			elt = sm_rbt_next(ionwm, elt))
	{
		addr = sm_rbt_data(ionwm, elt);
		rfx_print_range(addr, buffer);
		writeMemo(buffer);
	}

	sdr_exit_xn(sdr);

	return 0;
}
