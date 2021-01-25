/** \file cgr.h
 *  
 *  \brief  This files provides the declarations of the functions
 *          to start, call and stop the CGR; included in cgr.c .
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

#ifndef SOURCES_CGR_CGR_H_
#define SOURCES_CGR_CGR_H_

#include <sys/time.h>

#include "../library/commonDefines.h"
#include "../library/list/list_type.h"
#include "../bundles/bundles.h"
#include "../routes/routes.h"

typedef struct {
	/**
	 * \brief Purpose: count the calls to Unibo-CGR. (Currently used just for logging).
	 */
	unsigned int count_bundles;
	/**
	 * \brief The own ipn node.
	 */
	unsigned long long localNode;

	/**
	 * \brief The last time when the CGR discarded all routes.
	 */
	struct timeval cgrEditTime;



} UniboCgrSAP;

typedef enum {
	unknown_algorithm = 0,
	cgr = 1,
	msr = 2
} RoutingAlgorithm;

#ifdef __cplusplus
extern "C"
{
#endif

extern int getBestRoutes(time_t time, CgrBundle *bundle, List excludedNeighbors, List *routes);
extern int initialize_cgr(time_t time, unsigned long long ownNode);
extern void destroy_cgr(time_t time);
extern UniboCgrSAP get_unibo_cgr_sap(UniboCgrSAP *newSap);
extern long unsigned int get_computed_routes_number(unsigned long long destination);
extern RoutingAlgorithm get_last_call_routing_algorithm();

#if (LOG == 1)
/**
 * \brief Set the time for the log of the current call and print the call number in the main log file.
 */
#define start_call_log(time, count_bundles) do { \
		setLogTime(time); \
		writeLog("###### CGR: call n. %u ######", count_bundles); \
} while(0)

/**
 * \brief Print the sequence of characters that identify the end of the call.
 */
#define end_call_log() writeLog("###############################")

#else
#define start_call_log(time, count_bundles) do {  } while(0)
#define end_call_log() do {  } while(0)
#endif

#ifdef __cplusplus
}
#endif

#endif /* SOURCES_CGR_CGR_H_ */
