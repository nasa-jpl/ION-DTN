/** \file routes.h
 *
 *  \brief  This file provides the definition of the Route type,
 *          with all the declarations of the functions to create
 *          and delete the routes and other utility functions.
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
 *	    Carlo Caini, carlo.caini@unibo.it
 */

#ifndef LIBRARY_MANAGEROUTES_H_
#define LIBRARY_MANAGEROUTES_H_

#include <sys/time.h>
#include "../contact_plan/nodes/nodes.h"
#include "../library/commonDefines.h"
#include "../library/list/list_type.h"
#include "../library_from_ion/scalar/scalar.h"

typedef struct cgrRoute
{
	/**************** Yen's k-th shortest path algorithm ****************/
	/**
	 * \brief Lawler's modification to Yen's algorithm: the hop from which this route was born
	 */
	ListElt *rootOfSpur;
	/**
	 * \brief boolean: 0 if the route hasn't a selectedChild, 1 otherwise
	 */
	int spursComputed;
	/**
	 * \brief The route that computed this route with the Yen's algorithm
	 *
	 * \details This is a citation to the element in father->children list where
	 *          is stored the citation to this route
	 */
	ListElt *citationToFather;
	/**
	 * \brief All the routes computed by applying the Yen's algorithm to this route
	 */
	List children;
	/**
	 * \brief The route that moved this route from the Yen's "list B" (knownRoutes)
	 * to the Yen's "list A" (selectedRoutes)
	 */
	struct cgrRoute *selectedFather;
	/**
	 * \brief The route moved from the Yen's "list B" (knownRoutes)
	 * to the Yen's "list A" by this route.
	 */
	struct cgrRoute *selectedChild;
	/*********************************************************************/

	/*************** Phase one values ***************/
	/**
	 * \brief Product of the confidence of all hops of this route
	 */
	float arrivalConfidence;
	/**
	 * \brief Best case arrival time at the destination
	 */
	time_t arrivalTime;
	/**
	 * \brief The time when this route has been computed
	 */
	time_t computedAtTime;

#if (LOG == 1)
	/**
	 * \brief Actually only for logs: Number of the route in the selectedRoutes list
	 */
	unsigned int num;
#endif
	/************************************************/

	/*************** Phase two values ***************/
	/**
	 * \brief RVL: SABR 3.2.6.8.10
	 */
	double routeVolumeLimit;
	/**
	 * \brief Earliest transmission opportunity to the neighbor.
	 */
	time_t eto;
	/**
	 * \brief Projected bundle arrival time to destination
	 */
	time_t pbat;
	/**
	 * \brief It's 0 if the route hasn't been checked by phase two, > 0 otherwise.
	 *
	 * \details It's also used to set the "loop probability" of the route.
	 */
	int checkValue;
	/************************************************/

	/**
	 * \brief Sum of the owlt of all hops of the route,
	 * used in the cost functions of phase one and phase three.
	 *
	 * \details Calculated in phase one, updated in phase two.
	 */
	unsigned int owltSum;
	/**
	 * \brief Back-reference to this element in knownRoutes or selectedRoutes.
	 */
	ListElt *referenceElt;
	/**
	 * \brief First contact's toNode field
	 */
	unsigned long long neighbor;
	/**
	 * \brief Best case transmission opportunity to the neighbor
	 */
	time_t fromTime;
	/**
	 * \brief Lower toTime field, referring to the contacts in the hops list
	 */
	time_t toTime;
	/**
	 * \brief Hops of the route, from the first contact to the last contact
	 */
	List hops;

	/************ Overbooking management ************/
	/**
	 * \brief Bytes overbooked (necessary reforwarding)
	 */
	CgrScalar overbooked;
	/**
	 * \brief Bytes not overbooked (no reforwarding)
	 */
	CgrScalar committed;
	/************************************************/
} Route;

#ifdef __cplusplus
extern "C"
{
#endif

extern Route* create_cgr_route();
extern void delete_cgr_route(void*);
extern void clear_routes_list(List routes);
extern void destroy_routes_list(List routes);
extern int move_route_from_known_to_selected(Route *route);

extern int insert_selected_route(RtgObject *rtgObj, Route *route);
extern int insert_known_route(RtgObject *rtgObj, Route *route);
extern Route * get_route_father(Route *son);

#ifdef __cplusplus
}
#endif

#endif /* LIBRARY_MANAGEROUTES_H_ */
