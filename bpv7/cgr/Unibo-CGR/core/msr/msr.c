/** \file msr.c
 *
 *  \brief    Implementation of Moderate Source Routing (MSR). In this file you find
 *            all the functions to start, call and stop MSR.
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
 *       Carlo Caini, carlo.caini@unibo.it
 */

#include "msr.h"

#include "../cgr/cgr_phases.h"
#include "../contact_plan/contacts/contacts.h"
#include "../contact_plan/ranges/ranges.h"

#if (LOG == 1)
static void print_msr_proposed_routes(FILE *file_call, CgrBundle *bundle);
static void print_msr_candidate_routes(FILE *file_call);
static void print_msr_best_routes(FILE *file_call);
#else
#define print_msr_proposed_routes(file_call, bundle) do {  } while(0)
#define print_msr_candidate_routes(file_call) do {  } while(0)
#define print_msr_best_routes(file_call) do {  } while(0)
#endif

/******************************************************************************
 *
 * \par Function Name:
 * 		get_routes
 *
 * \brief Get the routes list. In this list are stored the routes choosed by MSR.
 *
 *
 * \par Date Written:
 * 	    02/07/20
 *
 * \return List
 *
 * \retval  List   The list where are stored the routes choosed by MSR.
 *
 * \param[in] newRoutes  Set to NULL if you just want to get the routes list;
 *                       otherwise set != NULL to overwrite the previous list
 *
 *
 * \par Revision History:
 *
 *  DD/MM/YY | AUTHOR          |  DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  02/07/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
static List get_routes(List newRoutes) {
	static List routes;

	if(newRoutes != NULL) {
		routes = newRoutes;
	}

	return routes;
}

/******************************************************************************
 *
 * \par Function Name:
 * 		initialized_msr
 *
 * \brief Initialize the data used by MSR.
 *
 * \par Date Written:
 * 		23/04/20
 *
 * \return int
 *
 * \retval  1   MSR initialized correctly.
 * \retval -2   MWITHDRAW error
 *
 *
 * \par Revision History:
 *
 *  DD/MM/YY | AUTHOR          |   DESCRIPTION
 *  -------- | --------------- |  -----------------------------------------------
 *  23/04/20 | L. Persampieri  |   Initial Implementation and documentation.
 *****************************************************************************/
int initialize_msr()
{
	int result = 1;
	List routes = list_create(NULL,NULL,NULL,NULL);
	if(routes == NULL)
	{
		result = -2;
	}
	else
	{
		result = 1;
		get_routes(routes); // save
	}

	return result;
}


/******************************************************************************
 *
 * \par Function Name:
 * 		detroy_msr
 *
 * \brief Deallocate all memory previously allocated by MSR.
 *
 * \par Date Written:
 * 		23/04/20
 *
 * \return void
 *
 *
 * \par Revision History:
 *
 *  DD/MM/YY | AUTHOR          |   DESCRIPTION
 *  -------- | --------------- |  -----------------------------------------------
 *  23/04/20 | L. Persampieri  |   Initial Implementation and documentation.
 *****************************************************************************/
void destroy_msr()
{
	List routes = get_routes(NULL);
	free_list(routes);
	routes = NULL;
	get_routes(routes); // save
}

/******************************************************************************
 *
 * \par Function Name:
 * 		tryMSR
 *
 * \brief Moderate Source Routing: this function checks if the route stored in bundle->msrRoute is viable
 *        for the forwarding of the bundle. In affermative case then we propose this route as "best route".
 *
 * \par Date Written:
 * 		23/04/20
 *
 * \return int
 *
 * \retval   ">=  0"   The number of "best routes" found.
 * \retval       -1    The MSR route stored in the bundle isn't viable.
 * \retval       -2    MWITHDRAW error
 * \retval       -3    Arguments error
 * \retval       -4    Critical bundle, MSR is not optimal for a critical bundle so we
 *                     don't perform the MSR algorithm
 *
 * \param[in]   current_time       The current (differential from interface) time from the start of Unibo-CGR
 * \param[in]   *bundle            The bundle to forward
 * \param[in]   excludedNeighbors  The excluded nodes list SABR 3.2.5.2
 * \param[in]   file_call          The file where we print some informations about
 *                                 what has been done during the MSR algorithm.
 * \param[out]  *bestRoutes        The MSR's output, the list of "best routes" if there
 *                                 are any.
 *
 *
 * \par Revision History:
 *
 *  DD/MM/YY | AUTHOR          |   DESCRIPTION
 *  -------- | --------------- |  -----------------------------------------------
 *  23/04/20 | L. Persampieri  |   Initial Implementation and documentation.
 *****************************************************************************/
int tryMSR(time_t current_time, CgrBundle *bundle, List excludedNeighbors, FILE* file_call, List *bestRoutes)
{
	int result = -3;
	List routes = get_routes(NULL);

	debug_printf("Entry point.");

	if(bundle != NULL && excludedNeighbors != NULL && bestRoutes != NULL)
	{
		free_list_elts(routes);

		if(IS_CRITICAL(bundle))
		{
			result = -4;
		}
		else if(bundle->msrRoute == NULL)
		{
			print_msr_proposed_routes(file_call, bundle);
			debug_printf("MSR route not found.");
			result = 0;
		}
		else
		{
			result = checkRoute(current_time, get_local_node(NULL), bundle,excludedNeighbors, bundle->msrRoute);
			if(result == -2)
			{
				writeLog("Check route: MWITHDRAW error.");
			}
			else if(result != 0)
			{
				print_msr_proposed_routes(file_call, bundle);
				print_msr_candidate_routes(file_call);
				debug_printf("MSR route isn't viable.");
				result = -1;
			}
			else
			{
				print_msr_proposed_routes(file_call, bundle);
				debug_printf("MSR route is viable.");
				if(list_insert_last(routes, bundle->msrRoute) != NULL)
				{
					print_msr_candidate_routes(file_call);
					// go directly to phase three
					result = chooseBestRoutes(bundle, routes);
					if(result > 0)
					{
						*bestRoutes = routes;
					}
					print_msr_best_routes(file_call);
				}
				else
				{
					result = -2;
				}
			}
		}
	}

	return result;
}

#if (LOG == 1)
/******************************************************************************
 *
 * \par Function Name:
 * 		print_msr_proposed_route
 *
 * \brief Print the route in the "call" file in MSR "proposed" format
 *
 *
 * \par Date Written:
 * 		23/04/20
 *
 * \return void
 *
 * \param[in]   file    The file where we want to print the route
 * \param[in]   *route  The Route that we want to print.
 * \param[in]   num     The number of the route in the list
 *
 *
 * \par Revision History:
 *
 *  DD/MM/YY | AUTHOR          |   DESCRIPTION
 *  -------- | --------------- |  -----------------------------------------------
 *  23/04/20 | L. Persampieri  |   Initial Implementation and documentation.
 *****************************************************************************/
static void print_msr_proposed_route(FILE *file, Route *route, unsigned int num)
{
	ListElt *elt;
	Contact *contact;

	/* If you have to work with very large numbers change all fields in %-20 */

	if (file != NULL && route != NULL)
	{
		route->num = num;

		fprintf(file,
				"\n%u)\n%-15s %-15s %-15s %-15s %-15s %-15s %s\n",
				route->num, "Neighbor", "FromTime", "ToTime", "ArrivalTime", "OwltSum", "Confidence", "Hops");
		fprintf(file, "%-15llu %-15ld %-15ld %-15ld %-15u %-15.2f ", route->neighbor,
				(long int) route->fromTime, (long int) route->toTime, (long int) route->arrivalTime,
				route->owltSum, route->arrivalConfidence);

		if (route->hops == NULL)
		{
			fprintf(file, "NULL\n");
		}
		else if (route->hops != NULL)
		{
			fprintf(file, "%lu\n%-15s %-15s %-15s %-15s %-15s %-15s %-15s %-15s %s\n",
					route->hops->length, "FromNode", "ToNode", "FromTime", "ToTime", "XmitRate",
					"Confidence", "MTV[Bulk]", "MTV[Normal]", "MTV[Expedited]");
			for (elt = route->hops->first; elt != NULL; elt = elt->next)
			{
				if (elt->data != NULL)
				{
					contact = (Contact*) elt->data;
					fprintf(file,
							"%-15llu %-15llu %-15ld %-15ld %-15lu %-10.2f%-5s %-15g %-15g %g\n",
							contact->fromNode, contact->toNode, (long int) contact->fromTime,
							(long int) contact->toTime, contact->xmitRate, contact->confidence,
							(elt == route->rootOfSpur) ? " x" : "", contact->mtv[0],
							contact->mtv[1], contact->mtv[2]);
				}
			}
		}

	}
}

/******************************************************************************
 *
 * \par Function Name:
 * 		print_msr_candidate_route
 *
 * \brief Print the route in the "call" file in MSR "candidate" format
 *
 *
 * \par Date Written:
 * 		23/04/20
 *
 * \return void
 *
 * \param[in]   file    The file where we want to print the route
 * \param[in]   *route  The Route that we want to print.
 *
 *
 * \par Revision History:
 *
 *  DD/MM/YY | AUTHOR          |   DESCRIPTION
 *  -------- | --------------- |  -----------------------------------------------
 *  23/04/20 | L. Persampieri  |   Initial Implementation and documentation.
 *****************************************************************************/
static int print_msr_candidate_route(FILE *file, Route *route)
{
	int result = -1;
	char num[20];
#if (CGR_AVOID_LOOP > 0)
	char *temp;
#endif

	if (file != NULL && route != NULL)
	{
#if (CGR_AVOID_LOOP > 0)
		if (route->checkValue == NO_LOOP)
		{
			temp = "No loop";
		}
#if (CGR_AVOID_LOOP == 2 || CGR_AVOID_LOOP == 3)
		else if (route->checkValue == POSSIBLE_LOOP)
		{
			temp = "Possible loop";
		}
		else if (route->checkValue == CLOSING_LOOP)
		{
			temp = "Closing loop";
		}
#endif
#if (CGR_AVOID_LOOP == 1 || CGR_AVOID_LOOP == 3)
		else if (route->checkValue == FAILED_NEIGHBOR)
		{
			temp = "Failed neighbor";
		}
#endif
		else
		{
			temp = "";
		}
#endif

		result = 0;
		num[0] = '\0';
		sprintf(num, "%u)", route->num);

#if (CGR_AVOID_LOOP > 0)
		fprintf(file, "%-15s %-15ld %-15ld %-15g %-15s %-15ld %-15ld %-15ld %ld\n", num,
				(long int) route->eto, (long int) route->pbat, route->routeVolumeLimit, temp,
				route->overbooked.gigs, route->overbooked.units, route->committed.gigs,
				route->committed.units);
#else
		fprintf(file, "%-15s %-15ld %-15ld %-15g %-15ld %-15ld %-15ld %ld\n", num,
				(long int) route->eto, (long int) route->pbat, route->routeVolumeLimit, route->overbooked.gigs,
				route->overbooked.units, route->committed.gigs, route->committed.units);
#endif

	}

	return result;
}

/******************************************************************************
 *
 * \par Function Name:
 * 		print_msr_best_route
 *
 * \brief Print the route in the "call" file in MSR "best route" format
 *
 *
 * \par Date Written:
 * 		23/04/20
 *
 * \return int
 *
 * \retval      0   Success case
 * \retval     -1   Arguments error
 *
 * \param[in]   file    The file where we want to print the route
 * \param[in]   *route  The Route that we want to print.
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  13/02/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
static int print_msr_best_route(FILE *file, Route *route)
{
	int len = -1, result = -1;
	char num[20];

	if (file != NULL && route != NULL)
	{
		result = 0;
		num[0] = '\0';
		len = sprintf(num, "%u)", route->num);
		if (len >= 0)
		{
			fprintf(file, "%-15s %llu\n", num, route->neighbor);
		}

	}

	return result;
}


/******************************************************************************
 *
 * \par Function Name:
 * 		print_msr_proposed_routes
 *
 * \brief Print the routes in the file in MSR "proposed" format
 *
 *
 * \par Date Written:
 * 		23/04/20
 *
 * \return void
 *
 * \param[in]   *file        The file where we want to print the routes
 * \param[in]   *bundle      The bundle that contains the MSR proposed routes
 *
 * \par Notes:
 *          1. In this first version there is at most 1 proposed route.
 *
 * \par Revision History:
 *
 *  DD/MM/YY | AUTHOR          |   DESCRIPTION
 *  -------- | --------------- |  -----------------------------------------------
 *  23/04/20 | L. Persampieri  |   Initial Implementation and documentation.
 *****************************************************************************/
static void print_msr_proposed_routes(FILE *file, CgrBundle *bundle)
{
	unsigned int count = 1;

	if(file != NULL)
	{

		fprintf(file,
				"\n------------------------------------------------------------ MSR: PROPOSED ROUTES -------------------------------------------------------------\n");
		if(bundle->msrRoute != NULL)
		{
			print_msr_proposed_route(file, bundle->msrRoute, count);
		}
		else
		{
			fprintf(file, "\n0 proposed routes.\n");
		}
		fprintf(file,
				"\n-----------------------------------------------------------------------------------------------------------------------------------------------\n");

		debug_fflush(file);
	}
}

/******************************************************************************
 *
 * \par Function Name:
 * 		print_msr_candidate_routes
 *
 * \brief Print the routes in the file in MSR "candidate" format
 *
 *
 * \par Date Written:
 * 		23/04/20
 *
 * \return void
 *
 * \param[in]   file               The file where we want to print the routes
 *
 *
 * \par Revision History:
 *
 *  DD/MM/YY | AUTHOR          |   DESCRIPTION
 *  -------- | --------------- |  -----------------------------------------------
 *  23/04/20 | L. Persampieri  |   Initial Implementation and documentation.
 *****************************************************************************/
static void print_msr_candidate_routes(FILE *file)
{
	ListElt *elt;
	List routes = get_routes(NULL);

	if (file != NULL)
	{
		fprintf(file,
				"\n------------------------------------------------------------ MSR: CANDIDATE ROUTES ------------------------------------------------------------\n");
		if (routes != NULL && routes->length > 0)
		{

#if (CGR_AVOID_LOOP > 0)
			fprintf(file, "\n%-15s %-15s %-15s %-15s %-15s %-15s %-15s %-15s %s\n", "Route n.",
					"ETO", "PBAT", "RVL", "Type", "Overbooked (G)", "Overbooked (U)",
					"Protected (G)", "Protected (U)");
#else
			fprintf(file, "\n%-15s %-15s %-15s %-15s %-15s %-15s %-15s %s\n",
					"Route n.", "ETO", "PBAT", "RVL", "Overbooked (G)", "Overbooked (U)",
					"Protected (G)", "Protected (U)");
#endif
			for (elt = routes->last; elt != NULL; elt = elt->prev)
			{
				print_msr_candidate_route(file, (Route*) elt->data);
			}
		}
		else
		{
			fprintf(file, "\n0 candidate routes.\n");
		}

		fprintf(file,
				"\n-----------------------------------------------------------------------------------------------------------------------------------------------\n");

		debug_fflush(file);

	}
}


/******************************************************************************
 *
 * \par Function Name:
 * 		print_msr_best_routes
 *
 * \brief Print the routes in the file in MSR "best routes" format
 *
 *
 * \par Date Written:
 * 		23/04/20
 *
 * \return void
 *
 * \param[in]   file         The file where we want to print the routes
 *
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  23/04/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
static void print_msr_best_routes(FILE *file)
{
	ListElt *elt;
	List routes = get_routes(NULL);

	if (file != NULL)
	{
		fprintf(file, "\n---------------- MSR: BEST ROUTES ----------------\n");

		if (routes != NULL && routes->length > 0)
		{
			fprintf(file, "\n%-15s %s\n", "Route n.", "Neighbor");
			for (elt = routes->first; elt != NULL; elt = elt->next)
			{
				print_msr_best_route(file, (Route*) elt->data);
			}
		}
		else
		{
			fprintf(file, "\n0 best routes.\n");
		}
		fprintf(file, "\n--------------------------------------------------\n");

		debug_fflush(file);
	}
}



#endif

