/*
 * ion_admin.h
 *
 * Public API for ION (Interplanetary Overlay Network) Administration
 *
 * This header provides public interfaces for managing ION configuration
 * parameters that are currently accessible only through the ionadmin CLI.
 *
 * Copyright (c) 2025, California Institute of Technology.
 * ALL RIGHTS RESERVED.  U.S. Government Sponsorship acknowledged.
 */

#ifndef ION_ADMIN_H
#define ION_ADMIN_H

#include "ion.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * =============================================================================
 * Production, Consumption, and Horizon Management APIs
 * =============================================================================
 *
 * These functions manage ION's congestion forecasting parameters:
 * - Production rate: Expected mean rate of continuous data origination by
 *   local BP applications (bytes/sec)
 * - Consumption rate: Expected mean rate of data delivery to local BP
 *   applications (bytes/sec)
 * - Horizon: End time for congestion forecasts (UTC timestamp)
 */

/*
 * Set rates in bytes per second.
 *   Use -1 to indicate rate unknown
 *   Use 0 for router-only nodes with no local data production.
 *
 * return 0 on success, -1 on error.
 */
extern int ion_set_production_rate(int rate_bytes_per_sec);
extern int ion_set_consumption_rate(int rate_bytes_per_sec);

/*
 * Set the congestion forecast horizon time.
 * horizon_time - End time for congestion forecasts as a Unix timestamp
 *   (seconds since epoch).
 *   Use 0 to disable congestion forecasting entirely.
 *
 * return 0 on success, -1 on error.
 *
 *   // Set horizon to 24 hours from now
 *   time_t now = time(NULL);
 *   time_t horizon = now + (24 * 3600);
 *
 *   // Set horizon to a specific future date (2025-12-31 23:59:59 UTC)
 *   struct tm target_time = {0};
 *   target_time.tm_year = 2025 - 1900;
 *   target_time.tm_mon = 11;  // December (0-based)
 *   target_time.tm_mday = 31;
 *   target_time.tm_hour = 23;
 *   target_time.tm_min = 59;
 *   target_time.tm_sec = 59;
 *   time_t horizon = mktime(&target_time);
 */
extern int ion_set_congestion_forecast_horizon(time_t horizon_time);

/**
 * Get the current settings for production, consumption, and horizon
 *
 * Return 0 on success, -1 on error.
 */
extern int ion_get_production_rate(int *rate_bytes_per_sec);
extern int ion_get_consumption_rate(int *rate_bytes_per_sec);
extern int ion_get_congestion_forecast_horizon(time_t *horizon_time);

/*
 * =============================================================================
 * Node Registration API
 * =============================================================================
 */

/**
 * Register the local node in a region.
 *
 * This should be called after ionInitialize() and ionAttach() to register
 * the local node in the specified region. This is equivalent to the
 * registration step performed by ionadmin's "1 <nodeNbr> <configFile>" command.
 *
 * This must be called before adding contacts, as contacts require nodes to be
 * registered in a region first.
 *
 * @param region_nbr  The region number to register in (typically 1)
 *
 * @return 0 on success, -1 on error
 *
 * Example:
 *   ionInitialize(&parms, 1);
 *   ionAttach();
 *   ion_register_node(1);  // Register node in region 1
 *   // Now you can add contacts
 */
extern int ion_register_node(uint32_t region_nbr);

/*
 * =============================================================================
 * Contact Management APIs
 * =============================================================================
 *
 * These functions manage ION contacts, which define transmission opportunities
 * between nodes in the network. Contacts specify when nodes can communicate
 * and at what data rate.
 */

/**
 * Add a contact to the ION contact plan.
 *
 * A contact defines a scheduled transmission opportunity between two nodes.
 *
 * @param from_time     Start time of the contact (Unix timestamp, seconds since epoch).
 *                      Special values:
 *                      - Use 0 for hypothetical contacts (future possible contacts)
 *                      - Use MAX_POSIX_TIME for registration contacts
 * @param to_time       End time of the contact (Unix timestamp).
 *                      Must be greater than from_time for scheduled contacts.
 * @param from_node     Source node number
 * @param to_node       Destination node number
 * @param xmit_rate     Transmission rate in bytes/second.
 *                      Set to 0 for registration or hypothetical contacts.
 * @param confidence    Confidence level (0.0 to 1.0) that contact will occur.
 *                      Use 1.0 for certain contacts, lower values for uncertain ones.
 *
 * @return 0 on success, -1 on error
 *
 * Example - Add a contact from node 1 to itself, starting 1 second from now,
 *           lasting 1 hour, at 100000 bytes/sec:
 *
 *   time_t now = time(NULL);
 *   ion_add_contact(now + 1, now + 3600, 1, 1, 100000, 1.0);
 */
extern int ion_add_contact(time_t from_time, time_t to_time,
                           uvast from_node, uvast to_node,
                           unsigned int xmit_rate, float confidence);

/**
 * Delete a contact from the ION contact plan.
 *
 * @param from_time     Start time of the contact to delete.
 *                      Special values:
 *                      - Use NULL to delete all contacts matching from_node/to_node
 *                      - Use 0 for hypothetical contacts
 *                      - Use MAX_POSIX_TIME for registration contacts
 * @param from_node     Source node number
 * @param to_node       Destination node number
 *
 * @return 0 on success, -1 on error
 *
 * Example - Delete a specific contact:
 *   time_t contact_time = now + 1;
 *   ion_delete_contact(&contact_time, 1, 1);
 *
 * Example - Delete all contacts from node 1 to node 2:
 *   ion_delete_contact(NULL, 1, 2);
 */
extern int ion_delete_contact(time_t *from_time, uvast from_node, uvast to_node);

/**
 * List all contacts in the ION contact plan.
 *
 * Prints all contacts to stdout in a human-readable format.
 *
 * @return 0 on success, -1 on error
 */
extern int ion_list_contacts(void);

/*
 * =============================================================================
 * Range Management APIs
 * =============================================================================
 *
 * These functions manage ION ranges, which define the One-Way Light Time (OWLT)
 * between nodes in the network. Ranges specify the communication delay between
 * nodes due to distance.
 */

/**
 * Add a range to the ION range database.
 *
 * A range defines the One-Way Light Time (OWLT) between two nodes, which is
 * the propagation delay in light seconds.
 *
 * @param from_time     Start time of the range (Unix timestamp, seconds since epoch).
 *                      Use 0 for a range that starts immediately or in the past.
 * @param to_time       End time of the range (Unix timestamp).
 *                      Must be greater than from_time.
 * @param from_node     Source node number
 * @param to_node       Destination node number
 * @param owlt          One-Way Light Time in seconds (range in light seconds).
 *                      This is the propagation delay from from_node to to_node.
 *
 * @return 0 on success, -1 on error
 *
 * Example - Add a range from node 1 to node 1, starting 1 second from now,
 *           lasting 1 hour, with 1 second OWLT:
 *
 *   time_t now = time(NULL);
 *   ion_add_range(now + 1, now + 3600, 1, 1, 1);
 */
extern int ion_add_range(time_t from_time, time_t to_time,
                         uvast from_node, uvast to_node,
                         unsigned int owlt);

/**
 * Delete a range from the ION range database.
 *
 * @param from_time     Start time of the range to delete.
 *                      Use NULL to delete all ranges matching from_node/to_node
 * @param from_node     Source node number
 * @param to_node       Destination node number
 *
 * @return 0 on success, -1 on error
 *
 * Example - Delete a specific range:
 *   time_t range_time = now + 1;
 *   ion_delete_range(&range_time, 1, 1);
 *
 * Example - Delete all ranges from node 1 to node 1:
 *   ion_delete_range(NULL, 1, 1);
 */
extern int ion_delete_range(time_t *from_time, uvast from_node, uvast to_node);

/**
 * List all ranges in the ION range database.
 *
 * Prints all ranges to stdout in a human-readable format.
 *
 * @return 0 on success, -1 on error
 */
extern int ion_list_ranges(void);

#ifdef __cplusplus
}
#endif

#endif /* ION_ADMIN_H */
