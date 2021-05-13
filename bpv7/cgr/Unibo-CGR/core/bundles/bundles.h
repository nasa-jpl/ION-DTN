/** \file bundles.h
 *
 *  \brief  This file provides the definition of the CgrBundle type
 *          and of the Priority type.
 *
 *
 ** \copyright Copyright (c) 2020, Alma Mater Studiorum, University of Bologna, All rights reserved.
 **
 ** \par License
 **
 **    This file is part of Unibo-CGR.                                            <br>
 **                                                                               <br>
 **    Unibo-CGR is free software: you can redistribute it and/or modify
 **    it under the terms of the GNU General Public License as published by
 **    the Free Software Foundation, either version 3 of the License, or
 **    (at your option) any later version.                                        <br>
 **    Unibo-CGR is distributed in the hope that it will be useful,
 **    but WITHOUT ANY WARRANTY; without even the implied warranty of
 **    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 **    GNU General Public License for more details.                               <br>
 **                                                                               <br>
 **    You should have received a copy of the GNU General Public License
 **    along with Unibo-CGR.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  \author Lorenzo Persampieri, lorenzo.persampieri@studio.unibo.it
 *
 *  \par Supervisor
 *          Carlo Caini, carlo.caini@unibo.it
 */

#ifndef LIBRARY_MANAGEBUNDLES_H_
#define LIBRARY_MANAGEBUNDLES_H_

#include <sys/time.h>

#include "../library/list/list_type.h"
#include "../library/commonDefines.h"
#include "../routes/routes.h"

typedef enum
{
	Bulk = 0, Normal, Expedited
} Priority;

typedef struct
{
	unsigned long long source_node;
	unsigned long long creation_timestamp;
	unsigned int sequence_number;
	unsigned int fragment_length;
	unsigned int fragment_offset;
} CgrBundleID;

typedef struct
{
	/**
	 * \brief Bundle's ID
	 */
	CgrBundleID id;
	/**
	 * \brief Ipn node number of the Node that sends to me (the own node) the bundle.
	 *
	 * \details Previous hop.
	 */
	unsigned long long sender_node;
	/**
	 * \brief Region in which the destination resides.
	 */
	unsigned long regionNbr;
	/**
	 * \brief Ipn node number of the destination node for the bundle.
	 */
	unsigned long long terminus_node;
	/**
	 * \brief Bulk, Normal or Expedited.
	 */
	Priority priority_level;
	/**
	 * \brief This is a bit mask
	 * - Critical bit (SET_CRITICAL, UNSET_CRITICAL and IS_CRITICAL macros)
	 * - Probe bit (SET_PROBE, UNSET_PROBE and IS_PROBE macros)
	 * - Fragmentable bit (SET_FRAGMENTABLE,UNSET_FRAGMENTABLE and IS_FRAGMENTABLE macros)
	 * - Backward propagation bit (SET_BACKWARD_PROPAGATION,UNSET_BACKWARD_PROPAGATION and RETURN_TO_SENDER macros)
	 * - You can clear this mask with CLEAR_FLAGS macro
	 */
	unsigned char flags;
	/**
	 * \brief From 0 to 255, only for Expedited priority_level.
	 */
	unsigned int ordinal;
	/**
	 * \brief Sum of payload + header.
	 */
	long unsigned int size;
	/**
	 *  \brief Estimated volume consumption SABR 2.4.3.
	 */
	long unsigned int evc;
	/**
	 * \brief The time when the bundle's lifetime expires (deadline).
	 */
	time_t expiration_time;
	/**
	 * \brief From 0.0 to 1.0.
	 */
	float dlvConfidence;
	/**
	 * \brief The geographic route of the bundle, used by the CGR to avoid loops;
	 *        get from RGR extension block.
	 */
	List geoRoute;
	/**
	 * \brief The list of neighbors that caused a loop for this bundle.
	 */
	List failedNeighbors;
	/**
	 * \brief The MSR route get from CGRR extension block.
	 */
	Route *msrRoute;
} CgrBundle;

#ifdef __cplusplus
extern "C"
{
#endif

#define CRITICAL               (1) /* (00000001) */
#define PROBE                  (2) /* (00000010) */
#define FRAGMENTABLE           (4) /* (00000100) */
#define BACKWARD_PROPAGATION   (8) /* (00001000) */

#define SET_CRITICAL(bundle) (((bundle)->flags) |= CRITICAL)
#define UNSET_CRITICAL(bundle) (((bundle)->flags) &= ~CRITICAL)

#define SET_PROBE(bundle) (((bundle)->flags) |= PROBE)
#define UNSET_PROBE(bundle) (((bundle)->flags) &= ~PROBE)

#define SET_FRAGMENTABLE(bundle) (((bundle)->flags) |= FRAGMENTABLE)
#define UNSET_FRAGMENTABLE(bundle) (((bundle)->flags) &= ~FRAGMENTABLE)

#define SET_BACKWARD_PROPAGATION(bundle) (((bundle)->flags) |= BACKWARD_PROPAGATION)
#define UNSET_BACKWARD_PROPAGATION(bundle) (((bundle)->flags) &= ~BACKWARD_PROPAGATION)

#define IS_CRITICAL(bundle) (((bundle)->flags) & CRITICAL)
#define IS_PROBE(bundle) (((bundle)->flags) & PROBE)
#define IS_FRAGMENTABLE(bundle) (((bundle)->flags) & FRAGMENTABLE)
#define RETURN_TO_SENDER(bundle) (((bundle)->flags) & BACKWARD_PROPAGATION)

extern int add_ipn_node_to_list(List nodes, unsigned long long ipnNode);
extern int search_ipn_node(List nodes, unsigned long long target);
extern int set_failed_neighbors_list(CgrBundle *bundle, unsigned long long ownNode);
extern int set_geo_route_list(char *geoRouteString, CgrBundle *bundle);
extern int check_bundle(CgrBundle *bundle);
extern long unsigned int computeBundleEVC(long unsigned int size);
extern void bundle_destroy(CgrBundle *bundle);
extern CgrBundle* bundle_create();
extern void reset_bundle(CgrBundle *bundle);
extern int initialize_bundle(int backward_propagation, int critical, float dlvConfidence,
		time_t expiration_time, Priority priority, unsigned int ordinal, int probe, int fragmentable, long unsigned int size,
		unsigned long long sender_node, unsigned long long terminus_node, CgrBundle *bundle);

#if (LOG == 1)
/**
 * \brief Print the bundle's ID in the main log file.
 *
 * \hideinitializer
 */
#define print_log_bundle_id(source, timestamp, seq_number, fragm_length, fragm_offset) \
	writeLog("Bundle ID { source: %llu, creation timestamp (msec): %llu, sequence number: %u, fragm. length: %u, fragm. offset: %u }.", \
(source), (timestamp), (seq_number), (fragm_length), (fragm_offset))

extern void print_bundle(FILE *file_call, CgrBundle *bundle, List excludedNodes,
		time_t currentTime);
#else
#define print_log_bundle_id(source, timestamp, seq_number, fragm_length, fragm_offset) do {  } while(0)
#define print_bundle(file_call,bundle,excludedNodes, current_time) do { } while(0)
#endif

#ifdef __cplusplus
}
#endif

#endif /* LIBRARY_MANAGEBUNDLES_H_ */

