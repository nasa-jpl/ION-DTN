/** \file bundles.c
 *
 *  \brief  This file provides the implementation of some functions
 *          to manage the CgrBundle's type and other utility functions.
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

#include "bundles.h"

#include <string.h>
#include <ctype.h>

#include "../library_from_ion/scalar/scalar.h"
#include "../cgr/cgr_phases.h"
#include "../library/list/list.h"
#include "../msr/msr_utils.h"

/******************************************************************************
 *
 * \par Function Name:
 *      add_ipn_node_to_list
 *
 * \brief Add the ipn node (as last element) in the nodes list
 *
 *
 * \par Date Written:
 *      16/03/20
 *
 * \return int
 *
 * \retval   0   Success case: Added ipn node in the list
 * \retval  -1   nodes: NULL
 * \retval  -2   MWITHDRAW error
 *
 * \param[in]       nodes          The nodes list
 * \param[in]       ipnNode        The ipn node to add in the list
 *
 * \par Notes:
 *           1. Function used to manage the CgrBundle's geoRoute and failedNeighbors lists
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |    DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  16/03/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
int add_ipn_node_to_list(List nodes, unsigned long long ipnNode)
{
	int result = 0;
	unsigned long long *new_element;

	if (nodes == NULL)
	{
		result = -1;
	}
	else
	{
		new_element = (unsigned long long*) MWITHDRAW(sizeof(unsigned long long));

		if (new_element != NULL)
		{
			result = 0;
			*new_element = ipnNode;
			if (list_insert_last(nodes, new_element) == NULL)
			{
				result = -2;
			}
		}
		else
		{
			result = -2;
		}
	}

	return result;
}

/******************************************************************************
 *
 * \par Function Name:
 * 		search_ipn_node
 *
 * \brief Search the "target" in the nodes list
 *
 *
 * \par Date Written:
 * 		16/03/20
 *
 * \return int
 *
 * \retval   0  Found target in the nodes list
 * \retval  -1  Target not found in the nodes list
 * \retval  -2  Arguments error (nodes NULL)
 *
 * \param[in]       nodes       The nodes list
 * \param[in]       target      The ipn node that we want to search in the nodes list
 *
 * \warning The nodes list must be a list of unsigned long long element
 *
 * \par Notes:
 *           1. Function used to manage the CgrBundle's geoRoute and failedNeighbors lists

 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |    DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  16/03/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
int search_ipn_node(List nodes, unsigned long long target)
{
	ListElt *elt;
	int result = -2;
	unsigned long long *current;

	if (nodes != NULL)
	{
		result = -1;
		for (elt = nodes->first; elt != NULL && result == -1; elt = elt->next)
		{
			if (elt->data != NULL)
			{
				current = (unsigned long long*) elt->data;

				if (*current == target)
				{
					result = 0;
				}
			}
		}

	}

	return result;
}

/******************************************************************************
 *
 * \par Function Name:
 * 		set_failed_neighbors_list
 *
 * \brief Build the bundle's failedNeighbors list from the bundle's geoRoute list
 *
 *
 * \par Date Written:
 * 		29/03/20
 *
 * \return int
 *
 * \retval  ">= 0" The total number of failed neighbor found, the real number of failed
 *                 neighbors could be less than this return value due to some nodes
 *                 duplicate or more.
 * \retval  -1     Arguments error
 * \retval  -2     MWITHDRAW error
 *
 * \param[in]      *bundle      The bundle to which we want to set the failed neighbors list.
 *                              The caller must initialize the geographic route of the bundle
 *                              (geoRoute).
 * \param[in]      ownNode      The own node (ipn node)
 *
 * \par Notes:
 *          1. A failed neighbor is a node that appears in the geographic route of the bundle
 *             immediately after the ownNode. So there was a loop and the ownNode previously
 *             send this bundle to that neighbor.
 *          2. If this function returns a value greater than 0 you can assume that
 *             there was a loop
 *          3. The bundle->failedNeighbors list initially will be resetted.
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |    DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  29/03/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
int set_failed_neighbors_list(CgrBundle *bundle, unsigned long long ownNode)
{
	ListElt *elt;
	int result = -1, stop, checkNext;
	unsigned long long *current;

	if (bundle != NULL && bundle->failedNeighbors != NULL && bundle->geoRoute != NULL
			&& ownNode != 0)
	{
		free_list_elts(bundle->failedNeighbors); //reset the list
		result = 0;
		stop = 0;
		checkNext = 0;
		for (elt = bundle->geoRoute->first; elt != NULL && !stop; elt = elt->next)
		{
			if (elt->data != NULL)
			{
				current = (unsigned long long*) elt->data;

				if (checkNext == 1)
				{
					//found failed neighbor -> LOOP
					//we add the node in the list only if it is not already inside
					result++;
					if (search_ipn_node(bundle->failedNeighbors, *current) != 0
							&& add_ipn_node_to_list(bundle->failedNeighbors, *current) != 0)
					{
						result = -2;
					}
					checkNext = 0;
				}
				else if (*current == ownNode)
				{
					checkNext = 1;
				}
				else
				{
					checkNext = 0;
				}
			}
		}

	}

	return result;
}

/******************************************************************************
 *
 * \par Function Name:
 *      set_geo_route_list
 *
 * \brief Convert the geographic route from a string to a list of ipn nodes number.
 *
 *
 * \par Date Written:
 *      29/03/20
 *
 * \return int
 *
 * \retval ">= 0"  Success case: length of the geographic route
 * \retval    -1   Arguments error
 * \retval    -2   Memory allocation error
 *
 * \param[in]       *geoRouteString     The geographic route in string format.
 * \param[in,out]   bundle              The bundle to which we set the geoRoute field.
 *                                      The geoRoute has to be initialized by the caller, this
 *                                      function initially reset the geoRoute.
 *
 * \warning The geoRouteString must be well formed and has to follow
 *          the pattern: ipn:xx...;ipn:yy... etc.
 *          Where: ... could be any character, xx (or yy etc.) is the node number and ; is the separator.
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  29/03/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
int set_geo_route_list(char *geoRouteString, CgrBundle *bundle)
{
	int result = -1, end = 0;
	char *temp = NULL;
	unsigned long long ipn_node, *new_elt, prev = 0;

	if (bundle != NULL && bundle->geoRoute != NULL)
	{
		free_list_elts(bundle->geoRoute);
		if (geoRouteString != NULL)
		{
			result = 0;
			end = 0;

			while (!end)
			{
				geoRouteString = strstr(geoRouteString, "ipn:");
				if (geoRouteString != NULL && isdigit(*(geoRouteString + 4)))
				{
					geoRouteString = geoRouteString + 4;
					temp = geoRouteString;
					ipn_node = strtoull(geoRouteString, &temp, 0);
					if (temp != geoRouteString && ipn_node != prev)
					{
						new_elt = MWITHDRAW(sizeof(unsigned long long));
						if (new_elt != NULL)
						{
							*new_elt = ipn_node;
							prev = ipn_node;
							if (list_insert_last(bundle->geoRoute, new_elt) == NULL)
							{
								MDEPOSIT(new_elt);
								end = 1;
								result = -2;
							}
							else
							{
								result++;
							}
						}
						else
						{
							end = 1;
							result = -2;
						}
					}
				}
				else
				{
					end = 1;
				}
			}
		}
	}

	return result;
}

/******************************************************************************
 *
 * \par Function Name:
 *      check_bundle
 *
 * \brief Check the validity of the bundle's fields
 *
 *
 * \par Date Written:
 *      31/03/20
 *
 * \return int
 *
 * \retval     0   Success case: all fields OK
 * \retval    -1   Bundle: NULL
 * \retval    -2   Bundle's fields are not valid
 * \retval    -3   Bundle's failedNeighbors/geoRoute: NULL
 *
 * \param[in]       *bundle   The CgrBundle for which we want to check the validity of the fields
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  31/03/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
int check_bundle(CgrBundle *bundle)
{
	int result = 0;

	if (bundle == NULL)
	{
		result = -1;
	}
	// The sender node is voluntarily not checked (0 could be used for loopback)
	else if (bundle->terminus_node == 0 || bundle->expiration_time < 0
			|| (bundle->dlvConfidence < 0.0F || bundle->dlvConfidence > 1.0F)
			|| (bundle->priority_level != Bulk && bundle->priority_level != Normal
					&& bundle->priority_level != Expedited) || bundle->ordinal > 255)
	{
		result = -2;
	}
#if (CGR_AVOID_LOOP > 0)
	else if (bundle->geoRoute == NULL)
	{
		result = -3;
	}
#endif
#if (CGR_AVOID_LOOP == 1 || CGR_AVOID_LOOP == 3)
	else if (bundle->failedNeighbors == NULL)
	{
		result = -3;
	}
#endif

	return result;

}

/******************************************************************************
 *
 * \par Function Name:
 * 		computeBundleEVC
 *
 * \brief Compute the estimated volume consumption (EVC)
 *
 *
 * \par Date Written:
 * 		06/02/20
 *
 * \return long unsigned int
 *
 * \retval ">= 0"    The estimated volume consumption
 *
 * \param[in]	size   The size of the bundle (sum of payload + header)
 *
 * \par Notes:
 * 			1.	The EVC refers to the SABR terminology, so it will be
 * 				computed as the sum of payload + header + convergence layer overhead
 *
 *
 * \par Revision History:
 *
 *  DD/MM/YY | AUTHOR          |  DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  06/02/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
long unsigned int computeBundleEVC(long unsigned int size)
{
	long unsigned int evc, estimated_convergence_layer_overhead;

	estimated_convergence_layer_overhead = (long unsigned int) (((double) size * PERC_CONVERGENCE_LAYER_OVERHEAD) / 100);

	if (estimated_convergence_layer_overhead < MIN_CONVERGENCE_LAYER_OVERHEAD)
	{
		estimated_convergence_layer_overhead = MIN_CONVERGENCE_LAYER_OVERHEAD;
	}

	evc = size + estimated_convergence_layer_overhead;

	return evc;
}

/******************************************************************************
 *
 * \par Function Name:
 *      bundle_destroy
 *
 * \brief Deallocate memory for CgrBundle type
 *
 *
 * \par Date Written:
 *      31/03/20
 *
 * \return void
 *
 * \param[in]       *bundle   The CgrBundle to deallocate
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  31/03/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
void bundle_destroy(CgrBundle *bundle)
{
	if (bundle != NULL)
	{
		free_list(bundle->geoRoute);
		free_list(bundle->failedNeighbors);
		delete_msr_route(bundle->msrRoute);
		memset(bundle, 0, sizeof(CgrBundle));
		MDEPOSIT(bundle);
	}

	return;
}

/******************************************************************************
 *
 * \par Function Name:
 *      bundle_create
 *
 * \brief Allocate memory for a CgrBundle type
 *
 *
 * \par Date Written:
 *      31/03/20
 *
 * \return CgrBundle*
 *
 * \retval     CgrBundle*  The bundle just allocated
 * \retval     NULL        MWITHDRAW error
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  31/03/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
CgrBundle* bundle_create()
{
	CgrBundle *bundle = NULL;

	bundle = (CgrBundle*) MWITHDRAW(sizeof(CgrBundle));
	if (bundle != NULL)
	{
		memset(bundle,0,sizeof(CgrBundle)); //initialize to known value
		bundle->geoRoute = list_create(bundle, NULL, NULL, MDEPOSIT_wrapper);
		bundle->failedNeighbors = list_create(bundle, NULL, NULL, MDEPOSIT_wrapper);

		if (bundle->failedNeighbors == NULL || bundle->geoRoute == NULL)
		{
			MDEPOSIT(bundle->geoRoute);
			MDEPOSIT(bundle->failedNeighbors);
			MDEPOSIT(bundle);
			bundle = NULL;
		}
	}

	return bundle;
}

/******************************************************************************
 *
 * \par Function Name:
 *      reset_bundle
 *
 * \brief Reset the bundle's fields to known values
 *
 *
 * \par Date Written:
 *      31/03/20
 *
 * \return void
 *
 * \param[in]  bundle   The bundle to reset
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  31/03/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
void reset_bundle(CgrBundle *bundle)
{
	bundle->dlvConfidence = 0.0F;
	bundle->evc = 0;
	bundle->expiration_time = 0;
	bundle->ordinal = 0;
	bundle->priority_level = Bulk;
	bundle->sender_node = 0;
	bundle->size = 0;
	bundle->terminus_node = 0;
	bundle->regionNbr = 0;
	CLEAR_FLAGS(bundle->flags);
	memset(&(bundle->id), 0, sizeof(CgrBundleID));
	free_list_elts(bundle->geoRoute);
	free_list_elts(bundle->failedNeighbors);
	delete_msr_route(bundle->msrRoute);
	bundle->msrRoute = NULL;
}

/******************************************************************************
 *
 * \par Function Name:
 *      initialize_bundle
 *
 * \brief Initialize the bundle's fields and check the validity
 *
 *
 * \par Date Written:
 *      31/03/20
 *
 * \return int
 *
 * \retval     0   Success case: all fields OK
 * \retval    -1   Bundle: NULL
 * \retval    -2   Bundle's fields are not valid
 * \retval    -3   Bundle's failedNeighbors/geoRoute: NULL
 *
 * \param[in,out]  *bundle                The bundle for which we want to set all the fields
 * \param[in]      backward_propagation   boolean: 0 if the sender_node has to be into the excludedNeighbors list, 1 otherwise.
 * \param[in]      critical               boolean: 1 the bundle is critical -> it will be send to all "viable" neighbors,
 *                                        0 otherwise -> it will be send to only one neighbor (best route).
 * \param[in]      dlvConfidence          The previous delivery confidence of the bundle (from 0.0 to 1.0)
 * \param[in]      expiration_time        The deadline
 * \param[in]      priority               The priority of the bundle: Bulk, Normal or Expedited
 * \param[in]      ordinal                Only for expedited priority, from 0 to 255
 * \param[in]      probe                  boolean: 0 if it's a probe bundle, 1 otherwise
 * \param[in]      fragmentable           boolean: 1 if this bundle is fragmentable, 0 otherwise
 * \param[in]      size                   The sum of payload + header
 * \param[in]      sender_node            The node that directly sent to us this bundle
 * \param[in]      terminus_node          The node to which this bundle has to be forwarded
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  31/03/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
int initialize_bundle(int backward_propagation, int critical, float dlvConfidence,
		time_t expiration_time, Priority priority, unsigned int ordinal, int probe, int fragmentable, long unsigned int size,
		unsigned long long sender_node, unsigned long long terminus_node, CgrBundle *bundle)
{
	int result = -1;

	if (bundle != NULL)
	{
		CLEAR_FLAGS(bundle->flags);
		if(backward_propagation != 0)
		{
			SET_BACKWARD_PROPAGATION(bundle);
		}
		if(critical != 0)
		{
			SET_CRITICAL(bundle);
		}
		if(probe != 0)
		{
			SET_PROBE(bundle);
		}
		if(fragmentable != 0)
		{
			SET_FRAGMENTABLE(bundle);
		}

		bundle->dlvConfidence = dlvConfidence;
		bundle->expiration_time = expiration_time;
		bundle->priority_level = priority;
		bundle->ordinal = ordinal;
		bundle->size = size;
		bundle->evc = computeBundleEVC(size);
		bundle->sender_node = sender_node;
		bundle->terminus_node = terminus_node;

		result = check_bundle(bundle);
	}

	return result;
}

#if (LOG == 1)
/******************************************************************************
 *
 * \par Function Name:
 *  	print_bundle
 *
 * \brief  Print the bundle's fields
 *
 *
 * \par Date Written:
 *  	15/02/20
 *
 * \return void
 *
 * \param[in]   file_call      The file where we want to print the bundle's fields
 * \param[in]   bundle         The bundle to print
 * \param[in]   excludedNodes  The excluded nodes for the forwarding of the bundle
 * \param[in]   currentTime    The internal time of Unibo-CGR
 *
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  15/02/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
void print_bundle(FILE *file_call, CgrBundle *bundle, List excludedNodes, time_t currentTime)
{
	char *priority;

	if (file_call != NULL)
	{
		if (bundle->priority_level == Bulk)
		{
			priority = "Bulk";
		}
		else if (bundle->priority_level == Normal)
		{
			priority = "Normal";
		}
		else
		{
			priority = "Expedited";
		}
		fprintf(file_call,
				"\ncurrent time: %ld\n\n------------------------------------------- BUNDLE -------------------------------------------\n",
				(long int) currentTime);
		fprintf(file_call, "\n%-15s %-15s %-15s %-15s %-15s %s\n%-15llu %-15llu %-15lu %-15ld %-15lu %.2f\n",
				"Destination", "SenderNode", "Size", "Deadline", "Bundle EVC", "DlvConfidence",
				bundle->terminus_node, bundle->sender_node, bundle->size, bundle->expiration_time,
				bundle->evc, bundle->dlvConfidence);
		fprintf(file_call, "%-15s %-15s %-15s %-15s %-15s %s\n%-15s %-15u %-15s %-15s %-15s %s\n",
				"PriorityLevel", "Ordinal", "Critical", "ReturnToSender", "Probe", "Fragmentable",
				priority, bundle->ordinal, (IS_CRITICAL(bundle) != 0) ? "yes" : "no",
				(RETURN_TO_SENDER(bundle) != 0) ? "yes" : "no", (IS_PROBE(bundle) != 0) ? "yes" : "no",
				(IS_FRAGMENTABLE(bundle) != 0 ? "yes" : "no"));

		print_ull_list(file_call, excludedNodes, "\nExcluded neighbors: ", ", ");

#if (CGR_AVOID_LOOP == 1 || CGR_AVOID_LOOP == 3)
		print_ull_list(file_call, bundle->failedNeighbors, "\nFailed neighbors: ", ", ");
#endif
#if (CGR_AVOID_LOOP == 2 || CGR_AVOID_LOOP == 3)
		print_ull_list(file_call, bundle->geoRoute, "\nGeo route: ", " -> ");
#endif

		fprintf(file_call,
				"\n----------------------------------------------------------------------------------------------\n");

		debug_fflush(file_call);

	}

}
#endif
