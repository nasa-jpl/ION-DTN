/** \file phase_two.c
 *
 *  \brief  This file provides the implementation of the CGR's phase two:
 *          in this phase we determine who are the viable
 *          routes for the forwarding of the bundle.
 *
 *  \details We check each computed route (phase one's output).
 *           The output of this phase is the candidate routes list.
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
 *       Carlo Caini, carlo.caini@unibo.it
 */

#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <float.h>

#include "cgr_phases.h"
#include "../contact_plan/contacts/contacts.h"
#include "../contact_plan/ranges/ranges.h"
#include "../library/commonDefines.h"
#include "../library/list/list.h"
#include "../library_from_ion/scalar/scalar.h"
#include "../routes/routes.h"
#include "../time_analysis/time.h"

/**
 * \brief Used to keep in one place all the data used by phase two.
 */
typedef struct {
	/**
	 * \brief The candidate routes list.
	 */
	List routes;
	/**
	 * \brief The subset of the computed routes. For each route in this list
	 *        we want that phase one compute other routes.
	 */
	List subset;
	/**
	 * \brief In this list we store the suppressed (i.e: excluded, failed ...) during
	 *        the current call. All the nodes in this list are neighbors with a
	 *        possible route to destination
	 */
	List suppressedNeighbors;
	/**
	 * \brief The number of neighbors found during the current call.
	 */
	long unsigned int neighborsFound;
	/**
	 * \brief The "max neighbors number" computed during the previous
	 *        iteration in phase two during the current call to CGR.
	 *        Resetted to 0 at each call to CGR.
	 */
	long unsigned int last_max_neighbors_number;
} PhaseTwoSAP;

typedef enum {
	DiscardedRoute = 0,
	CandidateRoute = 1
} PhaseTwoRouteFlag;


/******************************************************************************
 *
 * \par Function Name:
 * 		get_current_call_sap
 *
 * \brief Get the PhaseTwoSAP with all the values used by phase two during the current call
 *
 *
 * \par Date Written:
 * 	    02/07/20
 *
 * \return PhaseTwoSAP*
 *
 * \retval  PhaseTwoSAP*     The struct with all the values used by phase two during the current call
 *
 * \param[in]   *newSap      If you just want a reference to the SAP set NULL here;
 *                           otherwise set != NULL (the previous one will be overwritten).
 *
 *
 * \par Revision History:
 *
 *  DD/MM/YY | AUTHOR          |  DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  02/07/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
static PhaseTwoSAP *get_phase_two_sap(PhaseTwoSAP * newSap) {
	static PhaseTwoSAP sap;

	if(newSap != NULL) {
		sap = *newSap;
	}

	return &sap;
}

/******************************************************************************
 *
 * \par Function Name:
 * 		initialize_phase_two
 *
 * \brief Initialize the data used by the phase two.
 *
 *
 * \par Date Written:
 * 		06/02/20
 *
 * \return int
 *
 * \retval   1   Success case: phase two initialized
 * \retval  -2   MWITHDRAW error
 *
 *
 *
 * \par Revision History:
 *
 *  DD/MM/YY | AUTHOR          |  DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  06/02/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
int initialize_phase_two()
{
	int result = 1;
	PhaseTwoSAP *sap = get_phase_two_sap(NULL);
	if (sap->routes == NULL)
	{
		sap->routes = list_create(NULL, NULL, NULL, NULL);
	}
	if(sap->subset == NULL)
	{
		sap->subset = list_create(NULL, NULL, NULL, NULL);
	}
	if(sap->suppressedNeighbors == NULL)
	{
		sap->suppressedNeighbors = list_create(NULL, NULL, NULL, NULL);
	}

	sap->neighborsFound = 0;
	sap->last_max_neighbors_number = 0;

	if (sap->routes == NULL || sap->subset == NULL || sap->suppressedNeighbors == NULL)
	{
		result = -2;
		destroy_phase_two();
	}
	else
	{
		free_list_elts(sap->routes);
		free_list_elts(sap->subset);
		free_list_elts(sap->suppressedNeighbors);
	}

	return result;
}

/******************************************************************************
 *
 * \par Function Name:
 * 		destroy_phase_two
 *
 * \brief Deallocate all the memory allocated by the phase two.
 *
 *
 * \par Date Written:
 * 		06/02/20
 *
 * \return void
 *
 *
 *
 * \par Revision History:
 *
 *  DD/MM/YY | AUTHOR          |  DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  06/02/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
void destroy_phase_two()
{
	PhaseTwoSAP *sap = get_phase_two_sap(NULL);

	free_list(sap->routes);
	free_list(sap->subset);
	free_list(sap->suppressedNeighbors);
	sap->suppressedNeighbors = NULL;
	sap->routes = NULL;
	sap->subset = NULL;
	sap->neighborsFound = 0;
	sap->last_max_neighbors_number = 0;

	return;
}

/******************************************************************************
 *
 * \par Function Name:
 * 		reset_phase_two
 *
 * \brief Reset the routes list, so at the end it will be empty.
 *
 *
 * \par Date Written:
 * 		06/02/20
 *
 * \return void
 *
 *
 *
 * \par Revision History:
 *
 *  DD/MM/YY | AUTHOR          |  DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  06/02/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
void reset_phase_two()
{
	PhaseTwoSAP *sap = get_phase_two_sap(NULL);

	sap->neighborsFound = 0;
	sap->last_max_neighbors_number = 0;
	free_list_elts(sap->routes);
	free_list_elts(sap->subset);
	free_list_elts(sap->suppressedNeighbors);

	return;
}

/******************************************************************************
 *
 * \par Function Name:
 * 		isExcluded
 *
 * \brief Check if "target" is an excluded neighbor.
 *
 *
 * \par Date Written:
 * 		06/02/20
 *
 * \return int
 *
 * \retval   1   target is an excluded neighbor.
 * \retval   0   target isn't an excluded neighbor.
 *
 * \param[in]	target               Is this ipn node number included into the excludedNeighbors list?
 * \param[in]	excludedNeighbors    The list of excluded neighbors
 *
 * \warning This function assumes that excludedNeighbors is not NULL.
 *
 *
 * \par Revision History:
 *
 *  DD/MM/YY | AUTHOR          |  DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  06/02/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
static int isExcluded(unsigned long long target, List excludedNeighbors)
{
	unsigned long long *current;
	ListElt *elt;
	int result = 0;

	for (elt = excludedNeighbors->first; elt != NULL && result == 0; elt = elt->next)
	{
		if (elt->data != NULL)
		{
			current = (unsigned long long*) elt->data;

			if (*current == target)
			{
				result = 1;
			}
		}
	}

	return result;
}

/******************************************************************************
 *
 * \par Function Name:
 * 		reached_neighbors_limit
 *
 * \brief Check if we found a candidate route for at least N neighbors (or all). So
 *        if we reached the limit.
 *
 *
 * \par Date Written:
 * 		13/05/20
 *
 * \return int
 *
 * \retval   1   Limit reached
 * \retval   0   Limit not reached
 *
 * \param[in]    neighborsFound     The number of neighbors that currently are associated with at least one candidate route.
 * \param[in]    suppressedFound    The number of suppressed neighbors (e.g. excluded, or closing loop with a candidate route).
 * \param[in]    neighborsLimit     The total number of neighbors for which we want a candidate route,
 *                                  including those already found.
 * \param[in]    maxNeighborsNumber The total number of neighbors for which we could find a route to destination
 *
 *
 * \par Revision History:
 *
 *  DD/MM/YY | AUTHOR          |  DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  13/05/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
int reached_neighbors_limit(long unsigned int neighborsFound, long unsigned int suppressedFound, long unsigned int neighborsLimit, long unsigned int maxNeighborsNumber)
{
	int result = 0;

	if(neighborsFound >= neighborsLimit)
	{
		result = 1;
	}
	else if(maxNeighborsNumber <= neighborsFound + suppressedFound)
	{
		result = 1;
	}
	else
	{
		result = 0;
	}

	return result;
}

/******************************************************************************
 *
 * \par Function Name:
 * 		computeResidualBacklog
 *
 * \brief
 * 		Compute the residual backlog for the neighbor (identified from
 * 		the first hop destination node of the route) and some other fields
 * 		to manage the potential overbooking (route->protected, allotment and volume)
 *
 *
 * \par Date Written:
 * 		06/02/20
 *
 * \return int
 *
 * \retval          0	There is at least one contact from the local node to the neighbor
 * 						with a start time less or equal than the route's first contact start time.
 * \retval         -1	No contacts found from the local node to the neighbor with a start time
 * 				   		less or equal than the route's first contact, that case should never happens
 *
 * \param[in]   current_time               The time used during the current call
 * \param[in]   localNode                  The IPN number of the node on which Unibo-CGR is running
 * \param[in]        *route                The route, used fields: neighbor, protected.
 *                                         Initially route->protected contains the total backlog,
 *                                         at the end it will contains the amount of bytes that will not be
 *                                         reforwarded by the overbooking management
 * \param[out]       *allotment            Bytes that could be overbooked
 * \param[out]       *volume               Applicable contact volume of the first contact of the route,
 *                                         computed as the product of the xmitRate and the applicable duration
 * \param[in,out]    *residualBacklog      Initially it contains the applicable backlog, at the end
 *                                         it will contains the residual backlog
 *
 * \warning route doesn't have to be NULL
 * \warning allotment doesn't have to be NULL
 * \warning volume doesn't have to be NULL
 * \warning residualBacklog doesn't have to be NULL
 *
 * \par Notes:
 * 			1.  The terminology and how to compute the residual backlog are explained
 * 			    in the SABR algorithm at the point 3.2.6.2 ( from a) to g) )
 * 			2.  In this function the route->protected field will be updated (overbooking management).
 *
 *
 * \par Revision History:
 *
 *  DD/MM/YY | AUTHOR          |  DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  06/02/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
static int computeResidualBacklog(unsigned long regionNbr, time_t current_time, unsigned long long localNode, Route *route, CgrScalar *allotment, CgrScalar *volume,
		CgrScalar *residualBacklog)
{
	int result = -1;
	Contact *contact;
	RbtNode *rbt_node;
	unsigned long long neighbor = route->neighbor;
	time_t startTime, endTime, applicableDuration;
	CgrScalar applicableBacklogRelief;

	loadCgrScalar(allotment, 0);
	loadCgrScalar(volume, 0);
	loadCgrScalar(&applicableBacklogRelief, 0);

	for (contact = get_first_contact_from_node_to_node(regionNbr, localNode, neighbor, &rbt_node);
			contact != NULL; contact = get_next_contact(&rbt_node))
	{
		if (contact->fromNode != localNode || contact->toNode != neighbor
				|| contact->fromTime > route->fromTime)
		{
			rbt_node = NULL;
		}
		else
		{
			result = 0;

			if (current_time > contact->fromTime)
			{
				startTime = current_time;
			}
			else
			{
				startTime = contact->fromTime;
			}

			endTime = contact->toTime;
			applicableDuration = endTime - startTime; // SABR 3.2.6.2 d)
			loadCgrScalar(volume, applicableDuration);
			multiplyCgrScalar(volume, (long int) contact->xmitRate);

			//volume = applicable prior contact volume SABR 3.2.6.2 e)

			/*************** OVERBOOKING MANAGEMENT ***************/
			// Ported from ION 3.7.0
			copyCgrScalar(allotment, volume);
			subtractFromCgrScalar(allotment, &(route->committed));
			if (!CgrScalarIsValid(allotment))
			{
				copyCgrScalar(allotment, volume);
			}
			else
			{
				copyCgrScalar(allotment, &(route->committed));
			}
			subtractFromCgrScalar(&(route->committed), volume);
			if (!CgrScalarIsValid(&(route->committed)))
			{
				loadCgrScalar(&(route->committed), 0);
			}
			/******************************************************/

			if (contact->fromTime >= route->fromTime) // SABR 3.2.6.2 c)
			{
				//first contact of the route the "future" contacts
				//haven't to be considered, so I leave the loop
				rbt_node = NULL;
			}
			else
			{
				// SABR 3.2.6.2 f)
				addToCgrScalar(&applicableBacklogRelief, volume);
			}

		}
	}

	if (result == 0)
	{
		// SABR 3.2.6.2 g)
		subtractFromCgrScalar(residualBacklog, &applicableBacklogRelief);
		if (!CgrScalarIsValid(residualBacklog))
		{
			loadCgrScalar(residualBacklog, 0);
		}
	}

	return result;
}

/******************************************************************************
 *
 * \par Function Name:
 * 		computeEffectiveVolumeLimit
 *
 * \brief Compute the effective volume limit, algorithm explained at: SABR 3.2.6.8.9
 *
 *
 * \par Date Written:
 *		06/02/20
 *
 * \return int
 * \retval     0        Success case: The EVL has been computed
 * \retval    -1        The first byte transmit time for the contact is later than
 *                      the lower stop time between all contacts from this contact to the last contact
 *                      of the hops list
 *
 * \param[in]		*contact                A contact of the hops list.
 * \param[in]		priority                The priority level of the bundle
 * \param[in]		firstByteTransmitTime   The first byte transmit time for the sender node of the contact
 * \param[in]		*elt                    The ListElt for which the contact is the data field
 * \param[out]		*effectiveVolumeLimit   The effective volume limit computed, only in success case
 *
 * \warning contact doesn't have to be NULL
 * \warning elt doesn't have to be NULL
 * \warning effectiveVolumeLimit doesn't have to be NULL
 *
 * \par Notes:
 *                      1.      To understand the terminology and the algorithm
 *                              look at SABR from 3.2.6.8.5 to 3.2.6.8.9
 *
 *
 * \par Revision History:
 *
 *  DD/MM/YY | AUTHOR          |  DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  06/02/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
static int computeEffectiveVolumeLimit(Contact *contact, Priority priority,
		time_t firstByteTransmitTime, ListElt *elt, double *effectiveVolumeLimit)
{
	time_t effectiveStopTime, effectiveDuration;
	int result = 0;
	Contact *temp;

	effectiveStopTime = contact->toTime;
	elt = elt->next;

	while (elt != NULL)
	{
		temp = (Contact*) elt->data;
		if (temp->toTime < effectiveStopTime)
		{
			effectiveStopTime = temp->toTime;
		}

		elt = elt->next;
	}

	effectiveDuration = effectiveStopTime - firstByteTransmitTime;
	if (effectiveDuration <= 0)
	{
		result = -1;
	}
	else
	{
		*effectiveVolumeLimit =
				(double) ((long unsigned int) effectiveDuration * contact->xmitRate);
		if (contact->mtv[priority] < *effectiveVolumeLimit)
		{
			*effectiveVolumeLimit = contact->mtv[priority];
		}

	}

	return result;
}

/******************************************************************************
 *
 * \par Function Name:
 * 		computeExpectedBundleDeliveryTime
 *
 * \brief Check if the route is viable for the forwarding of the bundle,
 * 				 compute the ETO, compute the PBAT and compute the RVL.
 *
 *
 * \par Date Written:
 * 		06/02/20
 *
 * \return int
 *
 * \retval       0      Success case: The route is viable for the forwarding of the bundle
 *                      and the PBAT has been computed
 * \retval      -1      The route isn't viable for the forwarding of the bundle, PBAT hasn't been computed
 *
 * \param[in]   current_time           The time used during the current call
 * \param[in]   *bundle                The bundle that has to be forwarded
 * \param[in]   *route                 The route to check and to update the pbat, owltSum,
 *                                     routeVolumeLimit and arrivalTime fields.
 * \param[in]   *residualBacklog       The residual backlog for the neighbor identified
 *                                     by the first hop destination node of the route
 * \param[out]  *lastByteArrivalTime   (PBAT) The expected arrival time at the destionation node
 *                                     for the last byte of the bundle
 *
 * \warning bundle doesn't have to be NULL
 * \warning route doesn't have to be NULL
 * \warning residualBacklog doesn't have to be NULL
 * \warning effectiveVolumeLimit doesn't have to be NULL
 *
 * \par Notes:
 * 			1.	To understand the terminology and the algorithm
 * 				look at SABR from 3.2.6.2 to 3.2.6.8.11
 * 			2.	This function will update the route's fields computed in phase one:
 * 				-	owltSum
 * 				-	arrivalTime
 *
 *
 * \par Revision History:
 *
 *  DD/MM/YY | AUTHOR          |  DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  06/02/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
static int computeExpectedBundleDeliveryTime(time_t current_time, CgrBundle *bundle, Route *route,
		CgrScalar *residualBacklog, time_t *lastByteArrivalTime)
{
	int result = -1, viableRoute;
	unsigned int owlt, owltMargin, owltSum = 0;
	CgrScalar applicableRadiationLatency;
	time_t firstByteTransmitTime, lastByteTransmitTime, startTime, arrivalTime;
	double effectiveVolumeLimit;
	Contact *contact, *nextContact;
	ListElt *elt, *nextElt;
	Priority priority;
#if (QUEUE_DELAY == 1)
	double nominalContactVolume;
	time_t queueDelay;
#endif

	priority = bundle->priority_level;
	elt = route->hops->first;
	contact = (Contact*) route->hops->first->data;
	*lastByteArrivalTime = 0;

	if (contact->xmitRate > 0)
	{
		if (current_time > contact->fromTime)
		{
			startTime = current_time;
		}
		else
		{
			startTime = contact->fromTime;
		}

		//I update the values of the route computed in phase one: arrivalTime and owltSum

		arrivalTime = startTime;
		owltSum = 0;

		copyCgrScalar(&applicableRadiationLatency, residualBacklog);
		divideCgrScalar(&applicableRadiationLatency, (long int) contact->xmitRate);

		// here applicableRadiationLatency = backlog lien SABR 3.2.6.2 g)

		firstByteTransmitTime = startTime
				+ ((ONE_GIG * applicableRadiationLatency.gigs) + applicableRadiationLatency.units);

		route->eto = firstByteTransmitTime; // SABR 3.2.6.2 i) (3.2.6.3)

		loadCgrScalar(&applicableRadiationLatency, (long int) bundle->evc);
		divideCgrScalar(&applicableRadiationLatency, (long int) contact->xmitRate);
		lastByteTransmitTime = firstByteTransmitTime
				+ ((ONE_GIG * applicableRadiationLatency.gigs) + applicableRadiationLatency.units); // SABR 3.2.6.4

		route->routeVolumeLimit = DBL_MAX;

		viableRoute = 1;

		while (contact != NULL && viableRoute)
		{
			nextElt = elt->next;

			nextContact = (nextElt == NULL) ? NULL : (Contact*) nextElt->data;

			if(lastByteTransmitTime > contact->toTime) //TODO do not fragment
			{
				viableRoute = 0;
			}
			else if (get_applicable_range(contact->fromNode, contact->toNode, firstByteTransmitTime,
					&owlt) < 0)
			{
				viableRoute = 0;
			}
			else
			{
				owltMargin = ((MAX_SPEED_MPH / 3600) * owlt) / 186282;
				*lastByteArrivalTime = lastByteTransmitTime + owlt + owltMargin; //SABR 3.2.6.6

				//update phase one value: owltSum
				owltSum += owlt + owltMargin;

				//update phase one value: arrivalTime
				if (arrivalTime > contact->fromTime)
				{
					arrivalTime += owlt + owltMargin;
				}
				else
				{
					arrivalTime = contact->fromTime + owlt + owltMargin;
				}

				if (contact->mtv[priority] <= 0.0) // SABR 3.2.6.8.11
				{
					viableRoute = 0;
				}
				else
				{
					if (computeEffectiveVolumeLimit(contact, priority, firstByteTransmitTime, elt,
							&effectiveVolumeLimit) < 0) // SABR 3.2.6.8.9
					{
						viableRoute = 0;
					}
					else if (effectiveVolumeLimit < bundle->evc) //TODO do not fragment
					{
						viableRoute = 0;
					}
					else
					{
						//SABR 3.2.6.8.10: RVL
						route->routeVolumeLimit =
								(route->routeVolumeLimit < effectiveVolumeLimit) ?
										route->routeVolumeLimit : effectiveVolumeLimit;

						contact = nextContact;
						elt = nextElt;

						if (contact != NULL)
						{

#if (QUEUE_DELAY == 1)
							/* Added by G.M. De Cola
							 * Computing queueDelay from nominal contactVolume, actual mtv
							 * for the selected bundle priority and nominal xmit rate of the contact
							 */

							nominalContactVolume = (double) ((long unsigned int) (contact->toTime
									- contact->fromTime) * contact->xmitRate);
							queueDelay = (time_t) ((nominalContactVolume - contact->mtv[priority])
									/ ((double) contact->xmitRate));

							//ETO on all hops
							if (*lastByteArrivalTime > contact->fromTime + queueDelay)
							{
								firstByteTransmitTime = *lastByteArrivalTime;
							}
							else
							{
								firstByteTransmitTime = contact->fromTime + queueDelay;
							}
#else
							if (*lastByteArrivalTime > contact->fromTime)
							{
								firstByteTransmitTime = *lastByteArrivalTime;
							}
							else
							{
								firstByteTransmitTime = contact->fromTime;
							}
#endif

							loadCgrScalar(&applicableRadiationLatency, (long int) bundle->evc);
							divideCgrScalar(&applicableRadiationLatency, (long int) contact->xmitRate);
							lastByteTransmitTime = firstByteTransmitTime
									+ ((ONE_GIG * applicableRadiationLatency.gigs)
											+ applicableRadiationLatency.units);
						}
					}
				}
			}
		}
		//end while

		if (viableRoute)
		{
			result = 0;
			route->arrivalTime = arrivalTime;
			route->owltSum = owltSum;
		}
		else
		{
			*lastByteArrivalTime = 0;
		}
	}

	return result;
}

/******************************************************************************
 *
 * \par Function Name:
 * 		computePBAT
 *
 * \brief        Check if the route is viable for the forwarding of the bundle,
 *               compute the ETO, computed the PBAT and check for potential overbooking
 *
 *
 * \par Date Written:
 * 		06/02/20
 *
 * \return int
 *
 * \retval   0  Success case: The route is viable for the forwarding of the bundle
 *              and, in particular, the PBAT has been computed.
 * \retval  -1  Error case: The route isn't viable for the forwarding of the bundle,
 *              PBAT can't be computed
 * \retval  -2  Error case: The applicable backlog can't be computed
 * \retval  -3  Error case: The residual backlog can't be computed
 * \retval  -4  Error case: The PBAT has been computed but is later than the bundle's deadline
 *
 * \param[in]   current_time  The time used during the current call
 * \param[in]   localNode    The node on which Unibo-CGR is running
 * \param[in]   *route        The route to check
 * \param[in]   *bundle       The bundle that has to be forwarded
 *
 * \warning bundle doesn't have to be NULL
 * \warning route doesn't have to be NULL
 *
 * \par Notes:
 * 			1.	At the end, in success case, the following route's fields will be updated:
 * 				-	eto
 * 				-	pbat
 * 				-	routeVolumeLimit
 * 				-	owltSum
 * 				-	arrivalTime
 * 				-	bundleEVC
 * 				-	protected
 * 				-	overbooked
 *
 *
 * \par Revision History:
 *
 *  DD/MM/YY | AUTHOR          |  DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  06/02/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
static int computePBAT(time_t current_time, unsigned long long localNode, Route *route, CgrBundle *bundle)
{
	int result = -1;
	CgrScalar applicableBacklog, totalBacklog, allotment, volume, residualBacklog;
	time_t lastByteArrivalTime;

	if (computeApplicableBacklog(route->neighbor, bundle->priority_level, bundle->ordinal, &applicableBacklog, &totalBacklog) < 0)
	{
		result = -2;
	}
	else
	{
		copyCgrScalar(&(route->committed), &totalBacklog);
		copyCgrScalar(&residualBacklog, &applicableBacklog);

		if (computeResidualBacklog(bundle->regionNbr, current_time, localNode, route, &allotment, &volume, &residualBacklog) < 0)
		{
			result = -3;
		}
		else
		{
			//volume = route's first hop applicable volume

			/***************** OVERBOOKING MANAGEMENT *****************/
			// Ported from ION 3.7.0
			copyCgrScalar(&(route->overbooked), &allotment);
			increaseCgrScalar(&(route->overbooked), (long int) bundle->evc);
			subtractFromCgrScalar(&(route->overbooked), &volume);
			if (!CgrScalarIsValid(&(route->overbooked)))
			{
				loadCgrScalar(&(route->overbooked), 0);
			}
			/**********************************************************/

			result = computeExpectedBundleDeliveryTime(current_time, bundle, route, &residualBacklog,
					&lastByteArrivalTime);

			if (result == 0)
			{
				if (lastByteArrivalTime > bundle->expiration_time)
				{
					result = -4;
				}
				else
				{
					route->pbat = lastByteArrivalTime;
				}
			}
		}
	}

	return result;
}

/******************************************************************************
 *
 * \par Function Name:
 * 		insert_route_in_subset_computed_routes
 *
 * \brief Insert a computed route into subset of computed routes that phase two
 *        recommends to phase one for Yen's algorithm
 *
 *
 * \par Date Written:
 *  	30/04/20
 *
 * \return int
 *
 * \retval   1  Route added into the subset
 * \retval   0  Phase one will discard this route, so don't add it into the subset (OR neighbor not found)
 * \retval  -2  MWITHDRAW error
 *
 * \param[in]  neighbor         The route's neighbor
 * \param[in]  newSubsetRoute   The route to add in the subset
 *
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  30/04/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
static int insert_route_in_subset_computed_routes(Neighbor *neighbor, Route *newSubsetRoute)
{
	int result = 0;

	PhaseTwoSAP *sap = get_phase_two_sap(NULL);

	if(newSubsetRoute->spursComputed == 0)
	{
		//only if phase one can compute other routes from this route

		if(neighbor == NULL)
		{
			neighbor = get_neighbor(newSubsetRoute->neighbor);
		}
		if(neighbor != NULL && !(CANDIDATE_ROUTES_FOUND(neighbor)))
		{
			//only if we don't already have some candidate route with this neighbor
			if(list_insert_last(sap->subset, newSubsetRoute) == NULL)
			{
				//MWITHDRAW error
				result = -2;
			}
			else
			{
				//Ok, route added
				result = 1;
				SET_ROUTES_IN_SUBSET(neighbor);
			}
		}
		else
		{
			//neighbor not found or a route with this neighbor already found.
			result = 0;
		}
	}
	else
	{
		//Phase one will discard this route, so don't add it in the subset
		result = 0;
	}


	return result;
}

/******************************************************************************
 *
 * \par Function Name:
 * 		remove_neighbor_from_subset_computed_routes
 *
 * \brief Remove all routes (with the neighbor passed as argument) from the subset
 *
 *
 * \par Date Written:
 *  	30/04/20
 *
 * \return void
 *
 * \param[in]  newNeighbor   The neighbor of the routes to remove
 *
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  30/04/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
static void remove_neighbor_from_subset_computed_routes(unsigned long long newNeighbor)
{
	ListElt *elt, *next;
	Route *current;

	PhaseTwoSAP *sap = get_phase_two_sap(NULL);

	elt = sap->subset->first;

	while(elt != NULL)
	{
		next = elt->next;
		if(elt->data != NULL)
		{
			current = (Route *) elt->data;

			if(current->neighbor == newNeighbor)
			{
				list_remove_elt(elt);
			}

		}
		 elt = next;
	}

	return;
}

/******************************************************************************
 *
 * \par Function Name:
 * 		suppress_neighbor
 *
 * \brief Add the ipn node in the suppressedNeighbors list, only if it isn't already present
 *
 *
 * \par Date Written:
 *  	30/04/20
 *
 * \return int
 *
 * \retval   0   Neighbor added into the suppressedNeighbors list
 * \retval  -1   Neighbor already present into the suppressedNeighbors list
 * \retval  -2   MWITHDRAW error
 *
 * \param[in]  suppressedNeighbors  The list of suppressed neighbors where we add the new neighbor
 * \param[in]  neighbor             The neighbor to add into the suppressed neighbors list
 *
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  30/04/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
static int suppress_neighbor(List suppressedNeighbors, unsigned long long *neighbor)
{
	int result = -1;

	if(search_ipn_node(suppressedNeighbors, *neighbor) == -1) // if not found
	{
		result = 0;

		if(list_insert_last(suppressedNeighbors, neighbor) == NULL)
		{
			result = -2;
		}
	}

	return result;
}

/******************************************************************************
 *
 * \par Function Name:
 * 		suppress_destination_excluded_neighbors
 *
 * \brief Suppress only the neighbors to reach destination that are in the excludedNeighbors
 *
 *
 * \par Date Written:
 *  	30/04/20
 *
 * \return int
 *
 * \retval   0   Success case
 * \retval  -2   MWITHDRAW error
 *
 * \param[in]  destination         The destination from which we get the neighbors list
 * \param[out] suppressedNeighbors The list where we store the excluded neighbors, from now
 *                                 they become "suppressed"
 * \param[in]  excludedNeighbors   The excludedNeighbors list
 *
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  30/04/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
static int suppress_destination_excluded_neighbors(Node *destination, List suppressedNeighbors, List excludedNeighbors)
{
	ListElt *elt, *elt_suppr, *next, *next_suppr;
	unsigned long long *current, *current_suppr;
	int result = 0;

	// initially remove all previously excluded neighbors from list
	// this is necessary
	elt = excludedNeighbors->first;
	while(elt != NULL)
	{
		next = elt->next;

		if(elt->data != NULL)
		{
			current = (unsigned long long *) elt->data;

			elt_suppr = suppressedNeighbors->first;

			while(elt_suppr != NULL)
			{

				next_suppr = elt_suppr->next;

				if(elt_suppr->data != NULL)
				{
					current_suppr = (unsigned long long *) elt_suppr->data;

					if(*current == *current_suppr)
					{
						list_remove_elt(elt_suppr);
						elt_suppr = NULL; // exit condition from nested while
					}
				}

				if(elt_suppr != NULL)
				{
					elt_suppr = next_suppr;
				}
			}

		}

		elt = next;
	}

	// then refill the list with only the possible excluded neighbors for destination
	for(elt = excludedNeighbors->first; elt != NULL && result == 0; elt = elt->next)
	{
		if(elt->data != NULL)
		{
			current = (unsigned long long *) elt->data;

			if(is_node_in_destination_neighbors_list(destination, *current))
			{
				if(suppress_neighbor(suppressedNeighbors, current) == -2)
				{
					result = -2;
				}
			}
		}
	}

	return result;
}


/******************************************************************************
 *
 * \par Function Name:
 * 		update_neighbors_counter
 *
 * \brief  Update the neighborsFound counter used during the current call
 *
 * \par Date Written:
 *  	30/04/20
 *
 * \return int
 *
 * \retval   1  neighborsFound counter incremented by 1
 * \retval   0  neighborsFound counter not incremented
 * \retval  -1  Error case: neighbor not found
 * \retval  -2  MWITHDRAW error
 *
 * \param[in]  neighbor           The neighbor of the newCandidateRoute
 * \param[in]  newCandidateRoute  The new candidate route found
 *
 *
 * \par Notes:
 *         1.  The counter will NOT be update ONLY if:
 *            -  We don't find the neighbor
 *            -  The new candidate route has a failed/closing loop neighbor
 *         2.  In case of failed/closing loop neighbor we add it to the
 *             suppressed neighbor list
 *
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  30/04/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
static int update_neighbors_counter(Neighbor *neighbor, Route *newCandidateRoute)
{
	int result = -1;
#if (CGR_AVOID_LOOP > 0)
	int update = 1;
#endif
	PhaseTwoSAP *sap;

	sap = get_phase_two_sap(NULL);

	if(neighbor == NULL)
	{
		neighbor = get_neighbor(newCandidateRoute->neighbor);
	}
	if(neighbor != NULL) //if neighbor exists
	{
		result = 1;

#if (CGR_AVOID_LOOP > 0)
		if(newCandidateRoute->checkValue == FAILED_NEIGHBOR || newCandidateRoute->checkValue == CLOSING_LOOP)
		{
			// Ok, (failed/closing loop) neighbor.
			// We treat this neighbors as "suppressed" neighbors,
			// the intent is to find N "no loop" neighbors.
			// Anyway we're in this function
			// because we keep in mind these "less prioritary" routes
			update = 0;
			result = 0;
			if(!(CANDIDATE_ROUTES_FOUND(neighbor)))
			{
				if(suppress_neighbor(sap->suppressedNeighbors, &(newCandidateRoute->neighbor)) == -2)
				{
					result = -2;
				}
			}
		}
#endif

#if (CGR_AVOID_LOOP > 0)
		if(update)
		{
#endif
			//only if this is the first candidate route with this neighbor
			if(!(CANDIDATE_ROUTES_FOUND(neighbor)))
			{
				sap->neighborsFound += 1;
			}

#if (CGR_AVOID_LOOP > 0)
		}
#endif

		SET_CANDIDATE_ROUTES_FOUND(neighbor);

	}

	return result;
}


/******************************************************************************
 *
 * \par Function Name:
 * 		phase_one_conversation_management
 *
 * \brief Manage the feedback that phase two provides to phase one
 *
 * \par Date Written:
 *  	30/04/20
 *
 * \return int
 *
 * \retval   1   Success case: nothing to do, we have already found a candidate route for N neighbors
 * \retval   0   Success case
 * \retval  -1   route's neighbor not found
 * \retval  -2   MWITHDRAW error
 *
 * \param[in]  routeFlag                  Flag used to know if route is a candidate or discarded route
 * \param[in]  *route                     Generally a route, it could be a candidate route or not
 * \param[in]  neighbors_limit            We want a candidate route for N neighbors, where N is the limit.
 *                                        This is the initial value of missing neighbors (N), so
 *                                        the real number of missing neighbors is done by
 *                                        the result of "neighbors_limit - neighborsFound " (if greater than 0, obv).
 * \param[in]  max_neighbors_number       The total number of neighbors to reach destination
 *
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  30/04/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
static int phase_one_conversation_management(PhaseTwoRouteFlag routeFlag, Route *route, long unsigned int neighbors_limit, long unsigned int max_neighbors_number)
{
	Neighbor *neighbor;
	int result = 0;
	PhaseTwoSAP *sap = get_phase_two_sap(NULL);

	if(reached_neighbors_limit(sap->neighborsFound, sap->suppressedNeighbors->length, neighbors_limit, max_neighbors_number))
	{
		// we already reached the neighbors limit, we don't go back to phase one
		// so there isn't a conversation to manage.
//		debug_printf("Nothing to do (nF %lu, miss %lu, supp %lu, tot %lu)..." , neighborsFound, missing_neighbors, suppressedNeighbors->length, max_neighbors_number);
		result = 1;
	}
	else if((neighbor = get_neighbor(route->neighbor)) == NULL)
	{
		result = -1;
	}
	else
	{
		if(routeFlag == CandidateRoute && ROUTES_IN_SUBSET(neighbor))
		{
			// route is a candidate route
			// remove all the routes with the same neighbor from the subset

			remove_neighbor_from_subset_computed_routes(route->neighbor);
			UNSET_ROUTES_IN_SUBSET(neighbor); //no more routes in the subset with this neighbor
		}
		if(routeFlag == CandidateRoute && result != -2)
		{
			// route is a candidate route
			// update the counter of "viable" neighbors
			//only if the neighbors isn't "failed" or "closing loop";
			//then set neighbor->routesFound to 1
			if(update_neighbors_counter(neighbor, route) < 0)
			{
				result = -2;
			}
		}
		if(routeFlag == DiscardedRoute && !(CANDIDATE_ROUTES_FOUND(neighbor)) && result != -2)
		{
			//route isn't a candidate route
			//candidate route for this neighbor yet not found.
			//add this route into the subset
			if(insert_route_in_subset_computed_routes(neighbor, route) < 0)
			{
				result = -2;
			}
		}
	}

	return result;
}

#if (CGR_AVOID_LOOP > 0)

/******************************************************************************
 *
 * \par Function Name:
 * 		loopIdentified
 *
 * \brief Check if this route could cause at least a loop
 *
 *
 * \par Date Written:
 * 		27/03/20
 *
 * \return int
 *
 * \retval   4  Failed neighbor identified
 * \retval   3  Closing loop identified
 * \retval   2  Possible loop identified
 * \retval   0  The route doesn't cause a loop
 *
 * \param[in]   route         The route for which we want to know if it could cause a loop
 * \param[in]  	bundle        The bundle that contains the informations from which we learn if the
 *                            route could cause a loop
 *
 * \warning route doesn't have to be NULL
 * \warning route->hops doesn't have to be NULL and the list must have at least one contact
 * \warning bundle doesn't have to be NULL and all fields must be initialized
 *
 * \par Revision History:
 *
 *  DD/MM/YY | AUTHOR          |  DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  27/03/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
static int loopIdentified(Route *route, CgrBundle *bundle)
{
	int result = 0;
#if (CGR_AVOID_LOOP == 2 || CGR_AVOID_LOOP == 3)
	int found;
	ListElt *elt, *last_contact;
	Contact *current;
	unsigned int loop_level = 0;
#endif

	route->checkValue = NO_LOOP;

#if (CGR_AVOID_LOOP == 1 || CGR_AVOID_LOOP == 3)
	// Reactive
	if (search_ipn_node(bundle->failedNeighbors, route->neighbor) == 0)
	{
		result = 4;
		route->checkValue = FAILED_NEIGHBOR;
	}
#endif
#if (CGR_AVOID_LOOP == 2 || CGR_AVOID_LOOP == 3)
	// Proactive
	if (result == 0 && route->hops != NULL)
	{
		found = 0;
		last_contact = route->hops->last; //the last contact can't cause the loop

		for (elt = route->hops->first; elt != last_contact && elt != NULL && !found; elt =
				elt->next)
		{
			current = (Contact*) elt->data;
			if (current != NULL && search_ipn_node(bundle->geoRoute, current->toNode) == 0)
			{
				found = 1;
			}

			loop_level++;
		}

		if (found)
		{
			result = 2;

			route->checkValue = POSSIBLE_LOOP;
			if (loop_level == 1)
			{
				result = 3;
				route->checkValue = CLOSING_LOOP;
			}
		}
	}
#endif

	return result;
}

#endif

#if (NEGLECT_CONFIDENCE == 0)
/******************************************************************************
 *
 * \par Function Name:
 * 		lowDlvConfidence
 *
 * \brief   Check if the route arrival confidence add to the bundle's delivery confidence
 *          a confidence less than the MIN_CONFIDENCE_IMPROVEMENT
 *
 *
 * \par Date Written:
 * 		06/02/20
 *
 * \return int
 *
 * \retval   0   Confidence ok for the forwarding of the bundle
 * \retval   1   The route can't be used due to low confidence.
 *
 * \param[in]   bundleDlvConfidence     The current delivery confidence of the bundle
 * \param[in]   routeArrivalConfidence  The route's arrival confidence
 *
 *
 * \par Revision History:
 *
 *  DD/MM/YY | AUTHOR          |  DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  06/02/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
static int lowDlvConfidence(float bundleDlvConfidence, float routeArrivalConfidence)
{
	// Confidence improvement computation ported from ION 3.7.0
	int result = 0;
	float newDlvConfidence, dlvFailureConfidence, confidenceImprovement;
	if (bundleDlvConfidence > 0.0 && bundleDlvConfidence < 1.0)
	{
		dlvFailureConfidence = (1.0F - bundleDlvConfidence) * (1.0F - routeArrivalConfidence);
		newDlvConfidence = (1.0F - dlvFailureConfidence);
		confidenceImprovement = (newDlvConfidence / bundleDlvConfidence) - 1.0F;
		if (confidenceImprovement < MIN_CONFIDENCE_IMPROVEMENT)
		{
			result = 1;
		}
	}

	return result;
}
#endif

/******************************************************************************
 *
 * \par Function Name:
 *      checkRoute
 *
 * \brief Check if the route is viable for the forwarding of the bundle.
 *
 *
 * \par Date Written:
 *      06/02/20
 *
 * \return int
 *
 * \retval    0  Success case: The route is viable for the forwarding of the bundle
 * \retval   -1  The route was already checked
 * \retval   -2  MWITHDRAW error
 * \retval   -3  Route not viable: Terminated route SABR 2.3.2.1
 * \retval   -4  Route not viable: Best-case delivery time is after than the bundle's deadline SABR 3.2.6.9 a)
 * \retval   -5  Route not viable: (no SABR) The first contact has a confidence less than 1.0 (used in ION)
 * \retval   -6  Route not viable: (no SABR) The route adds to the bundle's delivery confidence
 *               a not considerable confidence (used in ION)
 * \retval   -7  Route not viable: The neighbor is the local node but the local node
 *               isn't the bundle's destination node SABR 3.2.6.9 c)
 * \retval   -8  Route not viable: The neighbor is in the excluded nodes list SABR 3.2.6.9 b)
 * \retval   -9  Route not viable: PBAT computation error
 *
 * \param[in]    current_time       The time used during the call
 * \param[in]    localNode          The own node
 * \param[in]    *bundle            The bundle that has to be forwarded
 * \param[in]    excludedNeighbors  The excluded nodes list SABR 3.2.5.2
 * \param[in]    *route             The route to check
 *
 * \warning bundle doesn't have to be NULL
 * \warning excludedNeighbors doesn't have to be NULL
 * \warning route doesn't have to be NULL
 *
 * \par Notes:
 *                1.   In success case the following route's fields will be updated:
 *                     -  eto
 *                     -  pbat
 *                     -  routeVolumeLimit
 *                     -  bundleEVC
 *                     -  owltSum
 *                     -  arrivalTime
 *                     -  checkValue
 *                2.   In success case: if CGR_AVOID_LOOP is enabled and the neighbor is in
 *                     the bundle's loopNodes list we add the neighbor to the phase one's
 *                     failedNeighbors list, in that case the route's checkValue field will be
 *                     updated to CLOSING_LOOP.
 *
 *
 *
 * \par Revision History:
 *
 *  DD/MM/YY | AUTHOR          |  DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  06/02/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
int checkRoute(time_t current_time, unsigned long long localNode, CgrBundle *bundle, List excludedNeighbors, Route *route)
{
	int result = -1;
#if (NEGLECT_CONFIDENCE == 0)
	Contact *firstContact;
#endif

	if (route->checkValue == 0)
	{
		route->checkValue = 1;

#if (NEGLECT_CONFIDENCE == 0)
		firstContact = (Contact*) route->hops->first->data;
#endif

		if (route->toTime <= current_time)
		{
			result = -3;
		}
		else if (route->arrivalTime > bundle->expiration_time) //SABR 3.2.6.9 a)
		{
			result = -4;
		}
#if (NEGLECT_CONFIDENCE == 0)
		else if (firstContact->confidence < 1.0F) //(no SABR) first contact must be certain (confidence == 1.0F)
		{
			result = -5;
		}
		else if (lowDlvConfidence(bundle->dlvConfidence, route->arrivalConfidence)) // (no SABR)
		{
			result = -6;
		}
#endif
		else if (route->neighbor == localNode && bundle->terminus_node != localNode) //SABR 3.2.6.9 c)
		{
			result = -7;
		}
		else if (isExcluded(route->neighbor, excludedNeighbors)) //SABR 3.2.6.9 b)
		{
			result = -8;
		}
		else if (computePBAT(current_time, localNode, route, bundle) < 0)
		{
			result = -9;
		}
		else //viable route
		{
#if (CGR_AVOID_LOOP > 0)
			result = loopIdentified(route, bundle);

			if (result > 0)
			{
				result = 0;
			}
#else
			result = 0;
#endif

		}
	}

	return result;
}

/******************************************************************************
 *
 * \par Function Name:
 *      getCandidateRoutes
 *
 * \brief Check all the computed routes and populate the candidate routes list
 *
 *
 * \par Date Written:
 *      06/02/20
 *
 * \return int
 *
 * \retval    0   Go back to phase one
 * \retval "> 0"  Number of routes in candidate routes list.
 * \retval   -1   Arguments error
 * \retval   -2   MWITHDRAW error, candidateRoutes points to NULL
 *
 * \param[in]   *terminusNode          The destination node
 * \param[in]   *bundle                The bundle that has to be forwarded
 * \param[in]    excludedNeighbors     The excluded nodes list SABR 3.2.5.2
 * \param[in]    computedRoutes        The routes to check
 * \param[out]  *subsetComputedRoutes  The set of routes that this phase recommend
 *                                     to phase one. Each route in this set should be used
 *                                     to compute other routes (Yen).
 * \param[out]  *missingNeighbors      The number of "missing" neighbors. We want a route
 *                                     for each of these neighbors.
 * \param[out]  *candidateRoutes       The candidate routes, NULL only if the return value
 *                                     is less than 1 and not equal to -10.
 *
 * \par Notes:
 *               1.  In success case candidateRoutes will be a subset of computedRoutes,
 *                   otherwise candidateRoutes will points to NULL
 *
 *
 *
 * \par Revision History:
 *
 *  DD/MM/YY | AUTHOR          |  DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  06/02/20 | L. Persampieri  |  Initial Implementation and documentation.
 *  27/04/20 | L. Persampieri  |  Refactoring
 *****************************************************************************/
int getCandidateRoutes(Node *terminusNode, CgrBundle *bundle, List excludedNeighbors, List computedRoutes,
		List *subsetComputedRoutes, long unsigned int *missingNeighbors, List *candidateRoutes)
{
	int result = -1, check, error = 0;
	ListElt *elt;
	Route *route;
	long unsigned int max_neighbors_number;
	time_t current_time = get_current_time();
	unsigned long long localNode = get_local_node();
	PhaseTwoSAP *sap = get_phase_two_sap(NULL);

	record_phases_start_time(phaseTwo);

	debug_printf("Entry point phase two.");
	free_list_elts(sap->subset); //clear the previous subset

	if (terminusNode != NULL && terminusNode->routingObject != NULL
			&& terminusNode->routingObject->citations != NULL && bundle != NULL
			&& excludedNeighbors != NULL && computedRoutes != NULL
			&& subsetComputedRoutes != NULL && missingNeighbors != NULL
			&& candidateRoutes != NULL)
	{

	    // Get the neighbors count (all neighbors)
	    *missingNeighbors = get_local_node_neighbors_count();

		max_neighbors_number = *missingNeighbors;

#if (MAX_DIJKSTRA_ROUTES > 0)
		if(!IS_CRITICAL(bundle) && *missingNeighbors > MAX_DIJKSTRA_ROUTES)
		{
			*missingNeighbors = MAX_DIJKSTRA_ROUTES;
		}
#endif
		result = 0;

		if(max_neighbors_number != sap->last_max_neighbors_number)
		{
//			debug_printf("Suppressing excluded nodes (last: %lu, current: %lu)...",last_max_neighbors_number, max_neighbors_number);
			if(suppress_destination_excluded_neighbors(terminusNode, sap->suppressedNeighbors, excludedNeighbors) < 0)
			{
				result = -2;
				error = 1;
			}

			sap->last_max_neighbors_number = max_neighbors_number;
		}

		for (elt = computedRoutes->first; elt != NULL && !error; elt = elt->next)
		{
			if (elt->data != NULL)
			{
				route = (Route*) elt->data;
				check = checkRoute(current_time, localNode, bundle, excludedNeighbors, route);

				if (check == 0)
				{
					// ok, candidate route
					result++;

					// if we have reached the "neighbors limit" we don't go back to phase one
					// so there isn't a "conversation" between phase one and phase two

					if(phase_one_conversation_management(CandidateRoute, route, *missingNeighbors, max_neighbors_number) < 0)
					{
						result = -2;
						error = 1;
					}
					else if (list_insert_last(sap->routes, route) == NULL)
					{
						result = -2;
						error = 1;
					}

				}
				else if(check == -3 ||
						check == -4 ||
						check == -5 ||
						check == -6 ||
						check == -9 ||
						(check == -8 && MAX_DIJKSTRA_ROUTES == 1))
				{
					//Ok, this route isn't viable
					//but the checkRoute() said that
					//this route could be used as "fromRoute" to compute other routes

					if(phase_one_conversation_management(DiscardedRoute, route, *missingNeighbors, max_neighbors_number) < 0)
					{
						result = -2;
						error = 1;
					}
				}
				else if(check == -2)
				{
					result = -2;
					error = 1;
				}
			}
		}

		debug_printf("New candidate routes: %d", result);

		if (result < 0)
		{
			//error case
			*candidateRoutes = NULL;
			*subsetComputedRoutes = NULL;
		}
		else
		{

			*candidateRoutes = (sap->routes->length > 0) ? sap->routes : NULL;

			if(reached_neighbors_limit(sap->neighborsFound, sap->suppressedNeighbors->length, *missingNeighbors, max_neighbors_number))
			{
				*missingNeighbors = 0;
				*subsetComputedRoutes = NULL;
				result = (int) sap->routes->length;
			}
			else
			{
				*missingNeighbors -= sap->neighborsFound;
				*subsetComputedRoutes = sap->subset;
				result = 0; // go back to phase one
			}

		}

		debug_printf("%lu neighbors found, %lu missing neighbors, %lu suppressed neighbors.", sap->neighborsFound, *missingNeighbors, sap->suppressedNeighbors->length);

	}

	record_phases_stop_time(phaseTwo);

	return result;
}

#if (LOG == 1)
/******************************************************************************
 *
 * \par Function Name:
 * 		print_phase_two_route
 *
 * \brief Print the route in the "call" file in phase two format
 *
 *
 * \par Date Written:
 * 	    06/02/20
 *
 * \return int
 *
 * \retval       0    Success case
 * \retval      -1    NULL file or NULL route
 *
 * \param[in]   file     The file where we want to print the route
 * \param[in]   *route   The Route that we want to print.
 *
 * \par Notes:
 * 			1. For phase two routes we print only the values computed during that phase.
 *
 * \par Revision History:
 *
 *  DD/MM/YY | AUTHOR          |  DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  06/02/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
static int print_phase_two_route(FILE *file, Route *route)
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
 * 		print_phase_two_routes
 *
 * \par Purpose:
 * 		Print the routes in the file in phase two format
 *
 *
 * \par Date Written:
 * 		06/02/20
 *
 * \return void
 *
 * \param[in]   file             The file where we want to print the routes
 * \param[in]   candidateRoutes  The routes that we want to print
 *
 *
 * \par Revision History:
 *
 *  DD/MM/YY | AUTHOR          |  DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  06/02/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
void print_phase_two_routes(FILE *file, List candidateRoutes)
{
	ListElt *elt;

	if (file != NULL)
	{
		fprintf(file,
				"\n--------------------------------------------------------- PHASE TWO: CANDIDATE ROUTES ---------------------------------------------------------\n");
		if (candidateRoutes != NULL && candidateRoutes->length > 0)
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
			for (elt = candidateRoutes->last; elt != NULL; elt = elt->prev)
			{
				print_phase_two_route(file, (Route*) elt->data);
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

#endif

