/** \file phase_one.c
 *
 *  \brief  This file provides the implementation of the CGR's phase one:
 *          in this phase we compute the routes that can be used to reach an ipn node.
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
#include <stdio.h>
#include <string.h>
#include <limits.h>

#include "cgr_phases.h"
#include "../contact_plan/contacts/contacts.h"
#include "../contact_plan/nodes/nodes.h"
#include "../contact_plan/ranges/ranges.h"
#include "../library/list/list.h"
#include "../routes/routes.h"
#include "../time_analysis/time.h"

/**
 * \brief Used to keep in one place all the data used by phase one.
 */
typedef struct {
	/**
	 * \brief List of unsigned long long, for each neighbor in this list we already have a computed
	 *        route (selectedRoutes)
	 */
	List excludedNeighbors;
	/**
	 * \brief A trick to exclude only one time the "neighbors" for each CGR's call.
	 *
	 * \details 1 if we already excluded all the neighbors for this call, 0 otherwise.
	 */
	int alreadyExcluded;
	/**
	 * \brief Boolean used to know if the graph has been already cleand with ClearTotally rule
	 *        during the current call.
	 *
	 * \details 1 if we already called the clear_work_areas with clearTotally rule, 0 otherwise.
	 */
	int graphCleaned;
	/**
	 * \brief A trick to update only one time the cost values for
	 * the route in Yen's "list B" (knownRoutes) for each CGR's call.
	 *
	 * \details 1 if we already updated the cost values for this call, 0 otherwise.
	 */
	int knownRoutesUpdated;
	/**
	 * \brief The contacts graph's root.
	 */
	Contact graphRoot;
	/**
	 * \brief The dijkstra's note for the contacts graph's root.
	 */
	ContactNote graphRootWork;
	/**
	 * \brief The destination of the current bundle
	 */
	unsigned long long destination;
    /**
     * \brief The region in which resides the bundle's destination
     */
	unsigned long regionNbr;
} PhaseOneSAP;

/**
 * \brief Not ordered queue used during Dijkstra's second loop.
 */
typedef struct {
	/**
	 * \brief The first contact in the queue
	 */
	Contact *firstContact;
	/**
	 * \brief The last contact in the queue
	 */
	Contact *lastContact;
} DijkstraQueue;

typedef enum
{
	ClearTotally = 1, // Clear all the graph to known values
	ClearPartially = 2, // Keep in mind the previous range found or not found
	ClearYen = 3 // ClearPartially and keep suppress the contact's with suppressed field equals to 2
} ClearRule;

typedef enum
{
	// You can remember
	// the range's value found for a contact during the current call
	// because in phase one we search the range
	// always at the same time (contact's start time)
	RangeNotFound = -1, // Range searched but not found
	RangeNotSearched = 0, // Range not searched
	RangeFound = 1 // Range searched and found
} RangeFlag;

typedef enum
{
	DijkstraNotSuppressed = 0, // The contact is in the graph
	DijkstraSuppressed = 1, // The contact is excluded from the graph
	SuppressedFromNodeForYenLoop = 2, // The contact is excluded from the graph due a loop
	                                  // caused by the fromNode field
	SuppressedToNodeForYenLoop = 3    // The contact is excluded from the graph due a loop
                                      // caused by the toNode field
} SuppressedFlag;

static int computeOneRoutePerNeighbor(Node *terminusNode, long unsigned int missingNeighbors);


/******************************************************************************
 *
 * \par Function Name:
 * 		get_dijkstra_queue
 *
 * \brief Get the struct DijkstraQueue, used to extract contacts for Dijkstra's second loop
 *
 *
 * \par Date Written:
 * 	    02/07/20
 *
 * \return DijkstraQueue*
 *
 * \retval  DijkstraQueue*   The queue
 *
 *
 * \par Revision History:
 *
 *  DD/MM/YY | AUTHOR          |  DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  02/07/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
static DijkstraQueue *get_dijkstra_queue() {
	static DijkstraQueue dq;

	return &dq;
}


/******************************************************************************
 *
 * \par Function Name:
 * 		get_first_contact_from_queue
 *
 * \brief Get the first contact from the DijkstraQueue
 *
 *
 * \par Date Written:
 * 	    02/07/20
 *
 * \return Contact*
 *
 * \retval  Contact*   The first contact in the queue
 * \retval  NULL       Empty queue
 *
 *
 * \par Revision History:
 *
 *  DD/MM/YY | AUTHOR          |  DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  02/07/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
static Contact *get_first_contact_from_queue() {
	DijkstraQueue *dq = get_dijkstra_queue();

	return dq->firstContact;
}

/******************************************************************************
 *
 * \par Function Name:
 * 		reset_dijkstra_queue
 *
 * \brief Reset the dijkstra queue (post-condition: empty queue)
 *
 *
 * \par Date Written:
 * 	    02/07/20
 *
 * \return void
 *
 *
 * \par Revision History:
 *
 *  DD/MM/YY | AUTHOR          |  DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  02/07/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
static void reset_dijkstra_queue() {
	DijkstraQueue *dq = get_dijkstra_queue();
	dq->firstContact = NULL;
	dq->lastContact = NULL;
}

/******************************************************************************
 *
 * \par Function Name:
 * 		add_contact_in_queue
 *
 * \brief Add a contact into DijkstraQueue
 *
 *
 * \par Date Written:
 * 	    02/07/20
 *
 * \return void
 *
 * \par Revision History:
 *
 *  DD/MM/YY | AUTHOR          |  DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  02/07/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
static void add_contact_in_queue(Contact *last) {
	DijkstraQueue *dq;

	// if not NULL (safety check) and if is not already in queue
	if (last != NULL &&
			last->routingObject->nextContactInDijkstraQueue == NULL)
	{
		dq = get_dijkstra_queue();

		if (dq->lastContact == NULL) //empty queue
		{
			dq->firstContact = last;
			dq->lastContact = last;
		}
		else if (last != dq->lastContact) // is not in queue
		{
			// previous last contact
			dq->lastContact->routingObject->nextContactInDijkstraQueue = last;
			// new last contact
			dq->lastContact = last;
		}
	}

	return;

}

/******************************************************************************
 *
 * \par Function Name:
 * 		get_phase_one_sap
 *
 * \brief Get the PhaseOneSAP with all the values stored by phase one for the current call
 *
 *
 * \par Date Written:
 * 	    02/07/20
 *
 * \return PhaseOneSAP*
 *
 * \retval  PhaseOneSAP*   The reference to the struct with all values used by phase one for the current call.
 *
 *
 * \par Revision History:
 *
 *  DD/MM/YY | AUTHOR          |  DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  02/07/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
static PhaseOneSAP *get_phase_one_sap(PhaseOneSAP * newSap) {
	static PhaseOneSAP sap;

	if(newSap != NULL) {
		sap = *newSap;
	}

	return &sap;
}

/******************************************************************************
 *
 * \par Function Name:
 * 		initialize_phase_one
 *
 * \brief Initialize all the data used by the phase one
 *
 *
 * \par Date Written:  30/01/20
 *
 * \return int
 *
 * \retval   1   Success case: initialized
 * \retval  -2   MWITHDRAW error
 *
 * \param[in]	ownNode   The local node
 *
 * \par Notes:
 * 			1.	Initialize:
 * 				-	The contacts graph's root
 * 				-	The excludedNeighbors list
 *
 *
 * \par Revision History:
 *
 *  DD/MM/YY | AUTHOR          |   DESCRIPTION
 *  -------- | --------------- |  -----------------------------------------------
 *  30/01/20 | L. Persampieri  |   Initial Implementation and documentation.
 *****************************************************************************/
int initialize_phase_one(unsigned long long ownNode)
{
	int result = 1;
	PhaseOneSAP *sap = get_phase_one_sap(NULL);

	sap->alreadyExcluded = 0;
	sap->knownRoutesUpdated = 0;
	sap->graphCleaned = 0;

	if (sap->excludedNeighbors == NULL)
	{
		//don't set delete_data_elt in this list !
		// We get the pointer to the neighbor with &(route->neighbor)
		sap->excludedNeighbors = list_create(NULL, NULL, NULL, NULL);
	}
	if (sap->excludedNeighbors == NULL)
	{
		result = -2;
	}
	else
	{
		free_list_elts(sap->excludedNeighbors);
		memset(&(sap->graphRoot), 0, sizeof(Contact));
		sap->graphRoot.fromNode = ownNode;
		sap->graphRoot.toNode = ownNode;
		sap->graphRoot.type = TypeScheduled;
		sap->graphRoot.toTime = MAX_POSIX_TIME;
		memset(&(sap->graphRootWork), 0, sizeof(ContactNote));
		sap->graphRoot.routingObject = &(sap->graphRootWork);
		sap->graphRoot.routingObject->arrivalConfidence = 1.0F;
	}

	return result;
}

/******************************************************************************
 *
 * \par Function Name: destroy_phase_one
 *
 * \brief Destroy all the data used by the phase one (memory areas will be deallocated)
 *
 * \details Destroy the excludedNeighbors list.
 *
 * \par Date Written:
 * 		30/01/20
 *
 * \return void
 *
 *
 * \par Revision History:
 *
 *  DD/MM/YY | AUTHOR          |   DESCRIPTION
 *  -------- | --------------- |  -----------------------------------------------
 *  30/01/20 | L. Persampieri  |   Initial Implementation and documentation.
 *****************************************************************************/
void destroy_phase_one()
{
	PhaseOneSAP *sap  = get_phase_one_sap(NULL);
	free_list(sap->excludedNeighbors);
	sap->excludedNeighbors = NULL;
	sap->alreadyExcluded = 0;
	sap->knownRoutesUpdated = 0;
	sap->graphCleaned = 0;
	sap->destination = 0;
	sap->regionNbr = 0;

	return;
}

/******************************************************************************
 *
 * \par Function Name:
 * 		reset_phase_one
 *
 * \brief Clear all the data used by the phase one
 *
 * \details Delete the elements of the excludedNeighbors list.
 *
 * \par Date Written:
 *  	30/01/20
 *
 * \return void
 *
 *
 * \par Notes:
 * 			1. 	Only the elements (ListElt) of the list will be deallocated, not
 * 				the list itself.
 *
 *
 * \par Revision History:
 *
 *  DD/MM/YY | AUTHOR          |   DESCRIPTION
 *  -------- | --------------- |  -----------------------------------------------
 *  30/01/20 | L. Persampieri  |   Initial Implementation and documentation.
 *****************************************************************************/
void reset_phase_one()
{
	PhaseOneSAP *sap = get_phase_one_sap(NULL);
	free_list_elts(sap->excludedNeighbors);
	sap->alreadyExcluded = 0;
	sap->knownRoutesUpdated = 0;
	sap->graphCleaned = 0;
	sap->destination = 0;
	sap->regionNbr = 0;

	return;
}

/******************************************************************************
 *
 * \par Function Name: clear_work_areas
 *
 * \brief Reset the ContactNote(s) used by the latest Dijkstra's search
 *
 *
 * \par Date Written:
 * 		30/01/20
 *
 * \return void
 *
 * \param[in] rule   It MUST be: ClearTotally OR ClearPartially OR CleanYen
 *
 * \par Notes:
 *          1. Set rule to ClearTotally to clean all contact notes
 *          2. Set rule to ClearPartially to clean all contact notes but
 *             not the owlt found for the contacts
 *          3. Set rule to ClearYen to get the same behavior of ClearPartially
 *             but without re-include in the graph the contacts with suppressed == 2
 *             (to avoid a challenging loop during Yen's algorithm)
 *
 *
 * \par Revision History:
 *
 *  DD/MM/YY | AUTHOR          |   DESCRIPTION
 *  -------- | --------------- |  -----------------------------------------------
 *  30/01/20 | L. Persampieri  |   Initial Implementation and documentation.
 *****************************************************************************/
static void clear_work_areas(ClearRule rule)
{
	Contact *current;
	RbtNode *node;
	ContactNote *work;
	PhaseOneSAP *sap;

	current = get_first_contact(&node);
	while (current != NULL)
	{
		work = current->routingObject;
		work->predecessor = NULL;
		if(rule == ClearTotally)
		{
			work->suppressed = 0;
			work->rangeFlag = 0;
			work->owlt = 0;
		}
		else if(rule == ClearPartially || work->suppressed == DijkstraSuppressed)
		{
			work->suppressed = 0;
		}

		work->visited = 0;
		work->owltSum = 0;
		work->arrivalTime = MAX_POSIX_TIME;
		work->hopCount = 0;
		work->arrivalConfidence = 1.0F;

		work->nextContactInDijkstraQueue = NULL;

		current = get_next_contact(&node);
	}

	if(rule == ClearTotally)
	{
		sap = get_phase_one_sap(NULL);
		sap->graphCleaned = 1;
	}

	reset_dijkstra_queue(); //initialize queue

	return;
}

/******************************************************************************
 *
 * \par Function Name:
 * 		neighbor_is_excluded
 *
 * \brief Check if the neighbor is in the excludedNeighbors list
 *
 *
 * \par Date Written:
 * 		30/01/20
 *
 * \return int
 *
 * \retval 	1	The neighbor is included in the excludedNeighbors list
 * \retval 	0	The neighbor isn't included in the excludedNeighbors list
 *
 * \param[in]	neighbor  The ipn node number of the neighbor
 *
 * \warning excludedNeighbors doesn't have to be NULL.
 *
 *
 * \par Revision History:
 *
 *  DD/MM/YY | AUTHOR          |   DESCRIPTION
 *  -------- | --------------- |  -----------------------------------------------
 *  30/01/20 | L. Persampieri  |   Initial Implementation and documentation.
 *****************************************************************************/
static int neighbor_is_excluded(unsigned long long neighbor)
{
	int result = 0;
	ListElt *elt;
	unsigned long long *current;
	PhaseOneSAP *sap = get_phase_one_sap(NULL);

	for (elt = sap->excludedNeighbors->first; elt != NULL && result == 0; elt = elt->next)
	{
		if (elt->data != NULL)
		{
			current = (unsigned long long*) elt->data;
			if (*current == neighbor)
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
 * 		exclude_current_neighbor
 *
 * \brief Add the route's neighbor in the excludedNeighbors list.
 *
 * \details We add only the pointer to the neighbor, Route MUST not be deallocated during the call.
 *
 *
 * \par Date Written:
 * 		30/01/20
 *
 * \return int
 *
 * \retval   0	The neighbor now is in the excludedNeighbors list
 * \retval  -1	Arguments error
 * \retval  -2	MWITHDRAW error
 *
 * \param[in]	*route	-	The Route for which we want to include the neighbor
 *                          in the excludedNeighbors list
 *
 * \warning route doesn't have to be NULL.
 *
 * \par Revision History:
 *
 *  DD/MM/YY | AUTHOR          |   DESCRIPTION
 *  -------- | --------------- |  -----------------------------------------------
 *  30/01/20 | L. Persampieri  |   Initial Implementation and documentation.
 *****************************************************************************/
static int exclude_current_neighbor(Route *route)
{
	int result = -1;
	PhaseOneSAP *sap;

	if(route != NULL)
	{
		sap = get_phase_one_sap(NULL);
		if (list_insert_first(sap->excludedNeighbors, &(route->neighbor)) != NULL)
		{
			result = 0;
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
 * 		exclude_all_neighbors_from_computed_routes
 *
 * \brief Add all neighbors in computed routes to the excludedNeighbors list
 *
 * \details We add only the pointer to the neighbor, Route MUST not be deallocated during the call.
 *
 *
 * \par Date Written:
 * 		30/01/20
 *
 * \return int
 *
 * \retval  ">= 0"	The number of neighbors added to the excludedNeighbors list
 * \retval     -2   MWITHDRAW error
 *
 * \param[in]	computedRoutes - The list of computed routes
 *
 * \warning computedRoutes doesn't have to be NULL.
 * \warning each route in computedRoutes doesn't have to be NULL.
 *
 * \par Revision History:
 *
 *  DD/MM/YY | AUTHOR          |   DESCRIPTION
 *  -------- | --------------- |  -----------------------------------------------
 *  30/01/20 | L. Persampieri  |   Initial Implementation and documentation.
 *****************************************************************************/
static int exclude_all_neighbors_from_computed_routes(List computedRoutes)
{
	int stop = 0, result = 0;
	Route *route;
	ListElt *elt;

	for (elt = computedRoutes->first; elt != NULL && !stop; elt = elt->next)
	{
		result++; //count the routes added by the add_route
		route = (Route*) elt->data;
		if (!neighbor_is_excluded(route->neighbor))
		{
			if (exclude_current_neighbor(route) < 0)
			{
				result = -2;
				stop = 1;
			}
		}
	}

	return result;
}

/******************************************************************************
 *
 * \par Function Name:
 * 		populate_route
 *
 * \brief Build the hops list of the route
 *
 *
 * \par Date Written:
 * 		30/01/20
 *
 * \return int
 *
 * \retval   0	Success case
 * \retval  -2	MWITHDRAW error
 *
 * \param[in]        sap                The current state of phase one
 * \param[in]        current_time       The interal time of Unibo-CGR
 * \param[in]		*finalContact		The last contact of the route (last hop)
 * \param[in]		*rootContact		The contact from which this route branches off
 * \param[in,out]	*resultRoute		The Route for which we want to build the hops list
 *
 * \warning finalContact doesn't have to be NULL.
 * \warning resultRoute doesn't have to be NULL.
 *
 * \par Notes:
 * 			1.	The following resultRoute's fields will be setted:
 * 				-	hops
 * 				-	arrivalTime
 * 				-	arrivalConfidence
 * 				-	owltSum
 * 				-	neighbor
 * 				-	fromTime
 * 				-	toTime
 * 				-	computedAtTime
 * 				-	rootOfSpur
 *
 *
 * \par Revision History:
 *
 *  DD/MM/YY | AUTHOR          |   DESCRIPTION
 *  -------- | --------------- |  -----------------------------------------------
 *  30/01/20 | L. Persampieri  |   Initial Implementation and documentation.
 *****************************************************************************/
static int populate_route(PhaseOneSAP *sap, time_t current_time, Contact *finalContact, Contact *rootContact, Route *resultRoute)
{
	int result = 0;
	time_t earliestEndTime;
	Contact *contact, *firstContact = NULL;
	ContactNote *current_work;
	ListElt *elt;

	resultRoute->arrivalTime = finalContact->routingObject->arrivalTime;
	resultRoute->arrivalConfidence = finalContact->routingObject->arrivalConfidence;
	resultRoute->owltSum = finalContact->routingObject->owltSum;
	resultRoute->computedAtTime = current_time;

	earliestEndTime = MAX_POSIX_TIME;
	contact = finalContact;

	while (contact != &(sap->graphRoot))
	{
		current_work = contact->routingObject;
		if (contact->toTime < earliestEndTime)
		{
			earliestEndTime = contact->toTime;
		}

		elt = list_insert_first(resultRoute->hops, contact);

		if (elt == NULL)
		{
			result = -2;
			contact = NULL; //I leave the loop
		}
		else
		{
			if (contact == rootContact)
			{
				//Spur route
				resultRoute->rootOfSpur = elt;
			}

			elt = list_insert_last(contact->citations, elt);

			if (elt == NULL)
			{
				result = -2;
				contact = NULL; //I leave the loop
			}
			else
			{
				firstContact = contact;
				contact = current_work->predecessor;
			}
		}
	}

	if (result == 0)
	{
		resultRoute->neighbor = firstContact->toNode;
		resultRoute->fromTime = firstContact->fromTime;
		resultRoute->toTime = earliestEndTime;
	}

	return result;
}

/******************************************************************************
 *
 * \par Function Name:
 * 		compare_dijkstra_edges
 *
 * \brief Compare two dijkstra's edges (ContactNotes)
 *
 *
 * \par Date Written:
 * 		30/01/20
 *
 * \return int
 *
 * \retval  -1	The	first edge has a less cost than the second edge
 * \retval   0	The two edges have the same cost
 * \retval   1	The first edge has a greater cost than the second edge
 *
 * \param[in]		*first		The first edge
 * \param[in]		*second		The second edge
 *
 * \warning first doesn't have to be NULL.
 * \warning second doesn't have to be NULL.
 *
 *
 * \par Revision History:
 *
 *  DD/MM/YY | AUTHOR          |   DESCRIPTION
 *  -------- | --------------- |  -----------------------------------------------
 *  30/01/20 | L. Persampieri  |   Initial Implementation and documentation.
 *****************************************************************************/
static int compare_dijkstra_edges(ContactNote *first, ContactNote *second)
{
	int result = 1;

	if (first->arrivalTime < second->arrivalTime)
	{
		result = -1;
	}
	else if (first->arrivalTime == second->arrivalTime)
	{
		if (first->hopCount < second->hopCount)
		{
			result = -1;
		}
		else if (first->hopCount == second->hopCount)
		{
			if (first->owltSum < second->owltSum)
			{
				result = -1;
			}
			else if (first->owltSum == second->owltSum)
			{
#if (NEGLECT_CONFIDENCE == 0)
				if (first->arrivalConfidence > second->arrivalConfidence)
				{
					result = -1;
				}
				else if (first->arrivalConfidence < second->arrivalConfidence)
				{
					result = 1;
				}
				else
				{
					result = 0;
				}
#else
				result = 0;
#endif
			}
		}
	}

	return result;
}

/******************************************************************************
 *
 * \par Function Name:
 * 		compute_new_distances
 *
 * \brief Dijkstra's algorithm first loop
 *
 * \details For the current contact
 *          we compute the distances between this contact and all of its (unvisited) neighbors,
 *          then we compare the distance computed with the previous computed distance for
 *          the neighbor contact and assign the smaller one.
 *
 *
 * \par Date Written:
 * 		30/01/20
 *
 * \return void
 *
 * \param[in]  *sap           The current state of phase one
 * \param[in]  current_time   The internal time of Unibo-CGR
 * \param[in]  *current       The current node, we consider the toNode field of this contact
 *                            to know who are the neighbours and at the end we add this
 *                            node to the visited set
 *
 * \par Notes:
 * 			1.	This is a modified Dijkstra's algorithm, so we have even the
 * 				excluded set and not excluded set (ContactNote's suppressed field)
 * 			2.	A neighbor for the current contact is each contact that has as
 * 				fromNode field equal to the toNode field of the current contact.
 *
 *
 *
 * \par Revision History:
 *
 *  DD/MM/YY | AUTHOR          |   DESCRIPTION
 *  -------- | --------------- |  -----------------------------------------------
 *  30/01/20 | L. Persampieri  |   Initial Implementation and documentation.
 *****************************************************************************/
static void compute_new_distances(PhaseOneSAP *sap, time_t current_time, Contact *current)
{
	int go_to_next = 0;
	Contact *contact;
	RbtNode *rbtNode;
	unsigned int owlt;
	unsigned int owltMargin;
	time_t earliestTransmissionTime;
	ContactNote *work, *currentWork, tempWork;

	currentWork = current->routingObject;

	for (contact = get_first_contact_from_node(sap->regionNbr, current->toNode, &rbtNode); contact != NULL;
			contact = get_next_contact(&rbtNode))
	{
		if (contact->fromNode != current->toNode || contact->regionNbr != sap->regionNbr)
		{
			rbtNode = NULL; //I leave the loop
		}
		else if ((contact->toNode != current->fromNode && contact->fromNode != contact->toNode)
				|| (current == &(sap->graphRoot)))
		{
			//don't route back and permits loopback
			//only for the local node (SABR)
			work = contact->routingObject;

			if(work->suppressed == SuppressedFromNodeForYenLoop)
			{
				// The work is suppressed due to a Yen loop caused by the fromNode,
				// All the contacts in this loop will have the same fromNode so
				// all contacts will be suppressed due to a Yen loop caused by the fromNode,
				// stop the loop and remember this for the currentWork in the next iterations
				// of the Yen's algotithm
				currentWork->suppressed = SuppressedToNodeForYenLoop;
				rbtNode = NULL;
			}
			else if (!work->suppressed && !work->visited)
			{
				earliestTransmissionTime = contact->fromTime;
				if (current == &(sap->graphRoot))
				{
					if (contact->fromTime < current_time) //SABR 3.2.4.1.1
					{
						earliestTransmissionTime = current_time;
					}
					if (neighbor_is_excluded(contact->toNode))
					{
						//helpful for "one route per neighbor"
						work->suppressed = DijkstraSuppressed;
						go_to_next = 1;
					}
#if (NEGLECT_CONFIDENCE == 0 && REVISABLE_CONFIDENCE == 0)
					/* TODO Removed check due to the insertion of opportunistic contacts
					else if (contact->confidence < 1.0F)
					{
						// first hop must be certain
						work->suppressed = Suppressed;
						go_to_next = 1;
					}
					 */
#endif
				}
				else if (contact->fromTime < currentWork->arrivalTime) //SABR 3.2.4.1.1
				{
					earliestTransmissionTime = currentWork->arrivalTime;
				}

				if (go_to_next)
				{
					go_to_next = 0; //reset for the next iteration
				}
				else if (contact->toTime > earliestTransmissionTime)
				{
					// in phase one we search the range ALWAYS at the start time of the contact
					// this depends on the destination's neighbors management.

					owlt = work->owlt; //initialize to work value

					if (work->rangeFlag == RangeNotFound
							|| (work->rangeFlag == RangeNotSearched
									&& get_applicable_range(contact->fromNode, contact->toNode, contact->fromTime, &owlt) < 0))
					{
						work->rangeFlag = RangeNotFound; //range not found at start time, this contact cannot be used to compute a route
						work->suppressed = DijkstraSuppressed;
					}
					//if rangeFlag == RangeFound, we use the work->owlt
					else
					{
						// Ok, range found at contact's start time
						work->rangeFlag = RangeFound;
						work->owlt = owlt;

						owltMargin = ((MAX_SPEED_MPH / 3600) * owlt) / 186282;
						owlt += owltMargin;

						tempWork.arrivalTime = earliestTransmissionTime + owlt;

						tempWork.owltSum = owlt + currentWork->owltSum;
						tempWork.hopCount = currentWork->hopCount;
						tempWork.arrivalConfidence = contact->confidence
								* currentWork->arrivalConfidence;

						if (contact->toNode != contact->fromNode)
						{
							tempWork.hopCount += 1;
						}

						if (compare_dijkstra_edges(&tempWork, work) < 0)
						{
							//found a new lower distance
							work->arrivalTime = tempWork.arrivalTime;
							work->hopCount = tempWork.hopCount;
							work->owltSum = tempWork.owltSum;
							work->predecessor = current;
							work->arrivalConfidence = tempWork.arrivalConfidence;

							// insert in queue (if not present)
							add_contact_in_queue(contact);
						}
					}
				}
			}
		}
	}

	currentWork->visited = 1;

	return;
}

/******************************************************************************
 *
 * \par Function Name:
 * 		find_best_contact
 *
 * \brief Dijkstra's algorithm second loop
 *
 * \details For all the contacts that are
 *          in the unvisited set with a finite distance we choose the
 *          contact with the smallest distance.
 *
 *
 * \par Date Written:  30/01/20
 *
 * \return Contact*
 *
 * \retval Contact*  The contact with the smallest distance
 * \retval NULL      There aren't contacts with a finite distance in the unvisited set
 *
 * \param[in]  toNode     The destination ipn node
 * \param[in]  localNode  The own node
 *
 * \warning current doesn't have to be NULL.
 * \warning lastLoopNode doesn't have to be NULL.
 *
 * \par Notes:
 *             1. This is a modified Dijkstra's algorithm, so we have even the
 *                excluded set and not excluded set (ContactNote's suppressed field)
 *
 *
 *
 * \par Revision History:
 *
 *  DD/MM/YY | AUTHOR          |   DESCRIPTION
 *  -------- | --------------- |  -----------------------------------------------
 *  30/01/20 | L. Persampieri  |   Initial Implementation and documentation.
 *  20/06/20 | L. Persampieri  |   Added queue (code optimization)
 *****************************************************************************/
static Contact* find_best_contact(unsigned long long toNode, unsigned long long localNode)
{
	Contact *contact;
//	RbtNode *rbtNode = NULL;
	ContactNote *work, tempWork;

	tempWork.arrivalTime = MAX_POSIX_TIME;
	tempWork.hopCount = UINT_MAX;
	tempWork.owltSum = UINT_MAX;
	tempWork.predecessor = NULL;
	tempWork.arrivalConfidence = 0.0F;

	// currently the queue is unordered, so we have to compare all contacts
	contact = get_first_contact_from_queue();

	while(contact != NULL)
	{
		work = contact->routingObject;

		if (!(work->suppressed) && !(work->visited) && work->arrivalTime != MAX_POSIX_TIME)
		{
			if (work->hopCount != 0 || toNode == localNode) //loopback only for the local node
			{
				if (compare_dijkstra_edges(work, &tempWork) < 0)
				{
					tempWork.predecessor = contact;
					tempWork.arrivalTime = work->arrivalTime;
					tempWork.hopCount = work->hopCount;
					tempWork.owltSum = work->owltSum;
					tempWork.arrivalConfidence = work->arrivalConfidence;
				}
			}
		}

		contact = work->nextContactInDijkstraQueue;
	}

	return tempWork.predecessor; //best contact found
}

/******************************************************************************
 *
 * \par Function Name:
 * 		dijkstra_search
 *
 * \brief Implementation of the Dijkstra's algorithm (SPF)
 *
 *
 * \par Date Written:
 * 		30/01/20
 *
 * \return int
 *
 * \retval   0	Success case: found route to the destination ipn node
 * \retval  -1	Error case:	route not found
 * \retval  -2	MWITHDRAW error
 *
 * \param[in]    *rootContact  The contact that will be used as contacts graph's root
 * \param[in]    toNode        The destination ipn node
 * \param[out]   *resultRoute  In success case all phase one fields of this Route
 *                             will be setted (see populate_route notes).
 *
 * \warning rootContact doesn't have to be NULL.
 * \warning resultRoute doesn't have to be NULL.
 *
 * \par Revision History:
 *
 *  DD/MM/YY | AUTHOR          |   DESCRIPTION
 *  -------- | --------------- |  -----------------------------------------------
 *  30/01/20 | L. Persampieri  |   Initial Implementation and documentation.
 *****************************************************************************/
static int dijkstra_search(Contact *rootContact, unsigned long long toNode, Route *resultRoute)
{
	int result = 0, stop = 0;
	Contact *current;
	Contact *finalContact = NULL;
	PhaseOneSAP *sap = get_phase_one_sap(NULL);
	time_t current_time = get_current_time();
	unsigned long long localNode = get_local_node();

	current = rootContact;

	while (!stop)
	{
		compute_new_distances(sap, current_time, current);

		current = find_best_contact(toNode, localNode);

		if (current != NULL)
		{
			if (current->toNode == toNode) //route found
			{
				finalContact = current;
				stop = 1; //I leave the loop
			}
		}
		else //route not found
		{
			stop = 1; //I leave the loop
		}
	}

	if (finalContact != NULL) //route found
	{
		result = populate_route(sap, current_time, finalContact, rootContact, resultRoute);
	}
	else
	{
		result = -1; //route not found
	}

	return result;
}

/******************************************************************************
 *
 * \par Function Name:
 * 		update_cost_values
 *
 * \brief Update the Route's values previously computed.
 *
 *
 * \par Date Written:
 * 		30/01/20
 *
 * \return int
 *
 * \retval   0	Success case: values updated
 * \retval  -1	Range not found, route can't be used
 * \retval  -2  The best-case delivery time is greater than the
 *              end time of a contact, route can't be used
 *
 * \param[in]  current_time  The internal time of Unibo-CGR
 * \param[in]  *route        The route for which we want to update the values
 *
 * \warning route doesn't have to be NULL.
 *
 * \par Revision History:
 *
 *  DD/MM/YY | AUTHOR          |   DESCRIPTION
 *  -------- | --------------- |  -----------------------------------------------
 *  30/01/20 | L. Persampieri  |   Initial Implementation and documentation.
 *****************************************************************************/
static int update_cost_values(time_t current_time, Route *route)
{
	ListElt *elt, *first;
	Contact *contact;
	time_t arrivalTime, earliestTransmissionTime;
	unsigned int owlt, owltMargin, owltSum;
	int result = 0;

	first = route->hops->first;
	contact = (Contact*) first->data;
	owlt = 1;
	owltSum = 0;
	arrivalTime = current_time;

	/*
	 * This function could be called before the clear_work_areas, so don't assume
	 * anything about previous range found
	 */

	for (elt = first; elt != NULL && result == 0; elt = elt->next)
	{
		contact = elt->data;
		owlt = 1;
		earliestTransmissionTime =
				(contact->fromTime > arrivalTime) ? contact->fromTime : arrivalTime;
		if (earliestTransmissionTime > contact->toTime)
		{
			//The best-case delivery time for the contact is greater than
			//the end time of the contact
			arrivalTime = MAX_POSIX_TIME;
			owltSum = UINT_MAX;
			result = -2;
			verbose_debug_printf("Can't update route's values.");
		}
		// in phase one we search the range ALWAYS at the start time of the contact
		// this depends on the destination's neighbors management.
		else if (get_applicable_range(contact->fromNode, contact->toNode,
				contact->fromTime, &owlt) < 0)
		{
			//Range not found
			contact->routingObject->rangeFlag = RangeNotFound;

			result = -1;
			arrivalTime = MAX_POSIX_TIME;
			owltSum = UINT_MAX;
			verbose_debug_printf("Can't update route's values.");
		}
		else
		{
			contact->routingObject->rangeFlag = RangeFound;
			contact->routingObject->owlt = owlt;

			owltMargin = ((MAX_SPEED_MPH / 3600) * owlt) / 186282;
			owlt += owltMargin;
			owltSum += owlt;

			arrivalTime = earliestTransmissionTime + owlt;
		}

	}

	if (result == 0)
	{
		route->arrivalTime = arrivalTime;
		route->owltSum = owltSum;
	}

	return result;
}

/******************************************************************************
 *
 * \par Function Name:
 * 		get_best_known_route
 *
 * \brief Choose the best Route from the Yen's "list B" (knownRoutes).
 *
 *
 * \par Date Written:
 * 		30/01/20
 *
 * \return Route*
 *
 * \retval Route*	The best route found.
 * \retval NULL		Route not found.
 *
 * \param[in]  *rtgObj    The RtgObject from which we get the knownRoutes list
 * \param[in]  neighbor   The neighbor's field of the routes that will be considered
 *                        in knownRoutes
 *
 * \warning rtgObj doesn't have to be NULL.
 *
 * \par Revision History:
 *
 *  DD/MM/YY | AUTHOR          |   DESCRIPTION
 *  -------- | --------------- |  -----------------------------------------------
 *  30/01/20 | L. Persampieri  |   Initial Implementation and documentation.
 *****************************************************************************/
static Route* get_best_known_route(RtgObject *rtgObj, unsigned long long neighbor)
{
	Route *bestRoute = NULL, *current;
	ListElt *elt;

	for (elt = rtgObj->knownRoutes->first; elt != NULL; elt = elt->next)
	{
		if (elt->data != NULL)
		{
			current = (Route*) elt->data;

#if (MAX_DIJKSTRA_ROUTES != 1)
			if (current->neighbor == neighbor)
			{
#endif
				if (bestRoute == NULL)
				{
					bestRoute = current;
				}
				else if (bestRoute->arrivalTime > current->arrivalTime)
				{
					bestRoute = current;
				}
				else if (bestRoute->arrivalTime == current->arrivalTime)
				{
					if (bestRoute->hops->length > current->hops->length)
					{
						bestRoute = current;
					}
					else if (bestRoute->hops->length == current->hops->length)
					{
						if (bestRoute->owltSum > current->owltSum)
						{
							bestRoute = current;
						}
						else if (bestRoute->owltSum == current->owltSum)
						{
							if (bestRoute->arrivalConfidence < current->arrivalConfidence)
							{
								bestRoute = current;
							}
#if (MAX_DIJKSTRA_ROUTES == 1)
							else if (bestRoute->arrivalConfidence == current->arrivalConfidence)
							{
								if (bestRoute->neighbor > current->neighbor)
								{
									bestRoute = current;
								}
							}
#endif
						}
					}
				}
#if (MAX_DIJKSTRA_ROUTES != 1)
			}
#endif
		}
	}

	if (bestRoute != NULL && bestRoute->arrivalTime == MAX_POSIX_TIME
			&& bestRoute->owltSum == UINT_MAX)
	{
		//Route for which we know that there isn't a range for some contact
		//or the arrivalTime of some contact is greater than the end time of the contact.
		//This route will be discarded in phase two.
		bestRoute = NULL;
		verbose_debug_printf(
				"Best known route found but its values hasn't been updated, discard it.");
	}

	return bestRoute;
}

/******************************************************************************
 *
 * \par Function Name:
 * 		suppress_root_path_ipn_node
 *
 * \brief Suppress all opportunity of transmission for the fromNode,
 *        in this way we avoid loop during Dijkstra's search.
 *
 * \details This isn't spur route dependent. You have to call this function
 *          just to avoid a loop in some computed route.
 *
 *
 * \par Date Written:
 * 		06/05/20
 *
 * \return void
 *
 * \param[in]  fromNode   The fromNode of the contacts that we are excluding from the graph
 *
 * \par Notes:
 *          1.  A root path ipn node is each contact->fromNode belonging to the root path
 *
 *
 * \par Revision History:
 *
 *  DD/MM/YY | AUTHOR          |   DESCRIPTION
 *  -------- | --------------- |  -----------------------------------------------
 *  04/05/20 | L. Persampieri  |   Initial Implementation and documentation.
 *****************************************************************************/
static void suppress_root_path_ipn_node(unsigned long long fromNode)
{
	int stop = 0;
	Contact *contact;
	RbtNode *rbtNode;
	PhaseOneSAP *sap = get_phase_one_sap(NULL);

	for (contact = get_first_contact_from_node(sap->regionNbr, fromNode, &rbtNode);
			contact != NULL && !stop; contact = get_next_contact(&rbtNode))
	{
		if (contact->fromNode != fromNode)
		{
			stop = 1;
		}
		else
		{
			//We want to exclude this contact even for the successive iteration
			//of Yen's algorithm on the current route
			//for this reason we set a distinguishable suppressed flag
			contact->routingObject->suppressed = SuppressedFromNodeForYenLoop;
		}
	}

	return;
}

/******************************************************************************
 *
 * \par Function Name:
 * 		initialize_root_path
 *
 * \brief Initialize all the values of the edges in the Yen's root path,
 *        and adds to the excluded set the spur nodes except
 *        the node pointed by rootOfSpur->data
 *
 *
 * \par Date Written:
 * 		30/01/20
 *
 * \return int
 *
 * \retval   0	Root path values initialized correctly
 * \retval  -1	Range not found for a contact
 * \retval  -2  The best-case delivery time (arrivalTime) for at least one
 *              contact is greater than the end time of the contact.
 *
 * \param[in]  current_time       The internal time of Unibo-CGR
 * \param[in]  *rootOfSpur        The last hop of the Yen's root path
 * \param[in]  isFirstSpurRoute   Used to know if it is the first spur route that we
 *                                are computing during the current iteration of the Yen's algorithm.
 *                                This argument is used to suppress the root path nodes that
 *                                will cause a loop during the Dijkstra's search.
 *
 * \warning rootOfSpur doesn't have to be NULL.
 *
 * \par Revision History:
 *
 *  DD/MM/YY | AUTHOR          |   DESCRIPTION
 *  -------- | --------------- |  -----------------------------------------------
 *  30/01/20 | L. Persampieri  |   Initial Implementation and documentation.
 *****************************************************************************/
static int initialize_root_path(time_t current_time, ListElt *rootOfSpur, int isFirstSpurRoute)
{
	Contact *contact, *rootOfSpurContact, *prevContact;
	ListElt *elt, *first;
	ContactNote *work = NULL;
	unsigned int owlt = 0, owltSum = 0;
	float arrivalConfidence = 1.0F;
	time_t transmitTime;
	unsigned int hop = 0;
	int result = 0;
	PhaseOneSAP *sap = get_phase_one_sap(NULL);

	rootOfSpurContact = (Contact*) rootOfSpur->data;
	prevContact = &(sap->graphRoot);
	first = rootOfSpur->list->first;

	elt = first;
	while (elt != NULL)
	{
		contact = (Contact*) elt->data;

		if (elt == first)
		{
			transmitTime = (contact->fromTime > current_time) ? contact->fromTime : current_time;
		}
		else
		{
			//work is referred to the previous contact
			transmitTime =
					(contact->fromTime > work->arrivalTime) ? contact->fromTime : work->arrivalTime;
		}

		work = contact->routingObject;
		owlt = work->owlt; //initialize with the work->owlt

		/*
		 * if(transmitTime > contact->toTime) == true ???
		 *
		 * My current choice: use the return value of this function,
		 * stop the Yen's algorithm and choose a route from the Yen's "list B".
		 *
		 */
		if (transmitTime > contact->toTime)
		{
			result = -2;
			elt = NULL;
			verbose_debug_printf("The transmit time is greater than the arrivalTime\n");
		}
		// in phase one we search the range ALWAYS at the start time of the contact
		// this depends on the destination's neighbors management.
		// Search the range only if you haven't searched it previously.
		else if (work->rangeFlag == RangeNotFound ||
				(work->rangeFlag == RangeNotSearched && get_applicable_range(contact->fromNode, contact->toNode, contact->fromTime, &owlt) < 0))
		{
			work->rangeFlag = RangeNotFound;
			result = -1;
			elt = NULL;
			verbose_debug_printf("Range not found.\n");
		}
		else
		{
			//Ok, range found
			work->owlt = owlt;
			work->rangeFlag = RangeFound;

			owlt += ((MAX_SPEED_MPH / 3600) * owlt) / 186282;
			work->arrivalTime = transmitTime + owlt;
			owltSum += owlt;
			work->owltSum = owltSum;
			arrivalConfidence *= contact->confidence;
			work->arrivalConfidence = arrivalConfidence;
			hop += 1;
			work->hopCount = hop;
			work->predecessor = prevContact;
			prevContact = contact;

			//This contacts will be suppressed also for the next spur routes
			work->suppressed = SuppressedFromNodeForYenLoop;

			if (contact != rootOfSpurContact) //Important check
			{
				elt = elt->next;
				if(isFirstSpurRoute)
				{
					// Only for the first spur route, for the other spur routes
					// the root path ipn nodes are already excluded (except the root vertex
					// that we exclude in the "else" condition)
					suppress_root_path_ipn_node(contact->fromNode);
				}
			}
			else
			{
				elt = NULL;
				// suppress the root vertex node
				suppress_root_path_ipn_node(contact->fromNode);
			}
		}

	}

	if (result < 0)
	{
		//don't compute a route from this root path
		rootOfSpurContact->routingObject->arrivalTime = MAX_POSIX_TIME;
	}

	return result;
}

/******************************************************************************
 *
 * \par Function Name:
 * 		avoid_duplicate_routes
 *
 * \brief For all routes in the Yen's "list A" (selectedRoutes) that share a root path
 *        equal to the current root path we add to the excluded set the next edge.
 *        Just to search an alternative route.
 *
 * \details This is spur route dependent.
 *
 *
 * \par Date Written:
 * 		30/01/20
 *
 * \return void
 *
 * \param[in]   *terminusNode  The Node from which we get the Yen's "list A" (selectedRoutes)
 * \param[in]   *rootOfSpur    The last hop of the Yen's root path
 *
 * \warning terminusNode doesn't have to be NULL.
 * \warning rootOfSpur doesn't have to be NULL.
 *
 * \par Revision History:
 *
 *  DD/MM/YY | AUTHOR          |   DESCRIPTION
 *  -------- | --------------- |  -----------------------------------------------
 *  30/01/20 | L. Persampieri  |   Initial Implementation and documentation.
 *****************************************************************************/
static void avoid_duplicate_routes(Node *terminusNode, ListElt *rootOfSpur)
{
	int stop = 0;
	RtgObject *rtgObj = terminusNode->routingObject;
	ListElt *elt, *temp, *rootPathContactElt;
	Contact *suppressMe;
	Route *route;
	List hops;

	for (elt = list_get_first_elt(rtgObj->selectedRoutes); elt != NULL; elt = elt->next)
	{
		route = (Route*) elt->data;

		if (rootOfSpur == NULL)
		{
			temp = list_get_first_elt(route->hops);
			suppressMe = (Contact*) temp->data;
			if(suppressMe->routingObject->suppressed == DijkstraNotSuppressed) //just for safety
			{
				suppressMe->routingObject->suppressed = DijkstraSuppressed;
			}
		}
		else
		{
			hops = rootOfSpur->list;
			stop = 0;
			rootPathContactElt = list_get_first_elt(hops);
			for (temp = list_get_first_elt(route->hops); temp != NULL && !stop; temp = temp->next)
			{
				if (temp->data == rootPathContactElt->data) //same contact
				{
					if (rootPathContactElt == rootOfSpur) //same root path
					{
						if (temp->next != NULL)
						{
							suppressMe = (Contact*) temp->next->data; //suppress next contact
							if(suppressMe->routingObject->suppressed == DijkstraNotSuppressed) //just for safety
							{
								suppressMe->routingObject->suppressed = DijkstraSuppressed;
							}
						}
						stop = 1;
					}
					else //same partial root path
					{
						rootPathContactElt = rootPathContactElt->next;
					}
				}
				else //different root path
				{
					stop = 1;
				}
			}
		}

	}

	return;
}

/******************************************************************************
 *
 * \par Function Name:
 * 		remove_neighbor_from_known_routes
 *
 * \brief Remove each route to this neighbor from knownRoute (Yen's list B).
 *
 * \details This function help us to avoid duplicate in computed routes (selectedRoutes) in
 *          some challenging case. You should call this function every time you find a route
 *          from a new neighbor in selectedRoutes.
 *
 *
 * \par Date Written:
 * 		06/05/20
 *
 * \return void
 *
 * \param[in]   knownRoutes   The Yen's "list B"
 * \param[in]   neighbor      The neighbor of the routes that we will remove
 *
 * \par Revision History:
 *
 *  DD/MM/YY | AUTHOR          |   DESCRIPTION
 *  -------- | --------------- |  -----------------------------------------------
 *  06/05/20 | L. Persampieri  |   Initial Implementation and documentation.
 *****************************************************************************/
void remove_neighbor_from_known_routes(List knownRoutes, unsigned long long neighbor)
{
	Route *route;
	ListElt *elt, *next;

	elt = knownRoutes->first;
	while(elt != NULL)
	{
		next = elt->next;
		if(elt->data != NULL)
		{
			route = (Route *) elt->data;
			if(route->neighbor == neighbor)
			{
				delete_cgr_route(route);
//				verbose_debug_printf("Remove from knownRoutes (neighbor: %llu)...", neighbor);
			}
		}

		elt = next;
	}

	return;
}

/******************************************************************************
 *
 * \par Function Name:
 * 		compute_spur_route
 *
 * \brief Yen's algorithm: compute a route that branches off from
 * 		  a particular hop of a previous computed route
 *
 *
 * \par Date Written:
 * 		30/01/20
 *
 * \return int
 *
 * \retval   0	Success case: Route computed
 * \retval  -1	Error case:	Route not found
 * \retval  -2	MWITHDRAW error
 * \retval  -3  The root path can't be used
 *
 * \param[in]    current_time      The internal time of Unibo-CGR
 * \param[in]    *terminusNode     The Node for which we want to compute a Route
 * \param[in]    isFirstSpurRoute  Set to 1 if it's the first spur route computed during
 *                                 the current Yen's algorithm on fromRoute.
 *                                 Set to 0 otherwise.
 * \param[in]    *rootOfSpur       The hop of the fromRoute from which we compute the new Route
 * \param[in]    *fromRoute        The Route that we consider as "father" of the new Route
 * \param[out]   *resultRoute      The Route computed
 *
 * \warning fromRoute doesn't have to be NULL.
 * \warning terminusNode doesn't have to be NULL.
 * \warning resultRoute doesn't have to be NULL.
 *
 * \par Notes:
 *             1. The relationship father-child will be managed
 *
 *
 *
 * \par Revision History:
 *
 *  DD/MM/YY | AUTHOR          |   DESCRIPTION
 *  -------- | --------------- |  -----------------------------------------------
 *  30/01/20 | L. Persampieri  |   Initial Implementation and documentation.
 *****************************************************************************/
static int compute_spur_route(time_t current_time, Route *fromRoute, int isFirstSpurRoute, ListElt *rootOfSpur, Node *terminusNode,
		Route *resultRoute)
{
	int result = 0;
	Contact *rootOfSpurContact;
	PhaseOneSAP *sap = get_phase_one_sap(NULL);

	if(isFirstSpurRoute)
	{
		if(!sap->graphCleaned)
		{
			// first time in the current call
			clear_work_areas(ClearTotally);
		}
		else
		{
			// first time in the current Yen's algorithm
			clear_work_areas(ClearPartially);
		}
	}
	else
	{
		clear_work_areas(ClearYen);
	}

	if (rootOfSpur == NULL)
	{
		rootOfSpurContact = &(sap->graphRoot);
	}
	else
	{
		rootOfSpurContact = (Contact*) rootOfSpur->data;

		if (initialize_root_path(current_time, rootOfSpur, isFirstSpurRoute) < 0)
		{
			result = -3; //the root path can't be used
		}
	}

	if (result == 0)
	{
		avoid_duplicate_routes(terminusNode, rootOfSpur);

		result = dijkstra_search(rootOfSpurContact, terminusNode->nodeNbr, resultRoute);

		if (result == 0)
		{
#if (MAX_DIJKSTRA_ROUTES != 1)
			if (resultRoute->neighbor == fromRoute->neighbor)
			{
#endif
				resultRoute->citationToFather = list_insert_last(fromRoute->children, resultRoute);
				if (resultRoute->citationToFather == NULL)
				{
					result = -2;
				}

#if (MAX_DIJKSTRA_ROUTES != 1)
			}
#endif

		}
	}

	return result;

}

/******************************************************************************
 *
 * \par Function Name:
 * 		compute_all_spurs
 *
 * \brief Yen's algorithm: compute the routes that branch off
 *        from each hop of a previous computed route
 *
 *
 * \par Date Written:
 * 		30/01/20
 *
 * \return int
 *
 * \retval  ">= 0"  The number of routes computed
 * \retval     -2   MWITHDRAW error
 *
 * \param[in]   *fromRoute          The Route that we consider as "father" of the new Route
 * \param[in]   *terminusNode       The Node for which we want to compute a Route
 * \param[in]   upperBound          Set to != NULL if you want to stop Yen's algorithm
 *                                  when you reach the upperBound as "next root of spur"
 * \param[out]  *allNeighborsFound  Boolean: 1 if we have a computed route for each neighbor,
 *                                  0 otherwise
 *
 * \warning fromRoute          doesn't have to be NULL.
 * \warning terminusNode       doesn't have to be NULL.
 * \warning allNeighborsFound  doens't have to be NULL.
 *
 * \par Notes:
 *               1.  Every time we find a route that has a neighbor not equal
 *                   to the fromRoute's neighbor, we add this route directly
 *                   in Yen's "list A" (selectedRoutes).
 *                   This because we consider as "children" only the routes that
 *                   have the same neighbor.
 *                   If we find a Route from another neighbor that means
 *                   there are no routes from that neighbor in selectedRoutes and
 *                   in knownRoutes so it's safe to add this route directly in selectedRoutes.
 *
 * \par Revision History:
 *
 *  DD/MM/YY | AUTHOR          |   DESCRIPTION
 *  -------- | --------------- |  -----------------------------------------------
 *  30/01/20 | L. Persampieri  |   Initial Implementation and documentation.
 *****************************************************************************/
static int compute_all_spurs(Route *fromRoute, Node *terminusNode, ListElt *upperBound, int *allNeighborsFound)
{

	int result = 0, stop = 0;
	int ok, created = 0;
	int isFirstSpurRoute;
	ListElt *rootOfNextSpur, *rootOfSpur, *elt = NULL;
	Route *last_computed_route = NULL;
	RtgObject *rtgObj = NULL;
	unsigned long long *current = NULL;
	ListElt *foundElt = NULL;
	int otherNeighbor = 0;
	PhaseOneSAP *sap = get_phase_one_sap(NULL);
	time_t current_time = get_current_time();

	stop = 0;
	*allNeighborsFound = 0;

	rootOfSpur = fromRoute->rootOfSpur;

	if (fromRoute->rootOfSpur == NULL)
	{
		// Only for a route computed from the graph root (so rootOfSpur is NULL)
		for (elt = sap->excludedNeighbors->first; elt != NULL && !stop; elt = elt->next)
		{
			current = (unsigned long long*) elt->data;
			if(current != NULL && *current == fromRoute->neighbor)
			{
				//remove temporarily the neighbor from the excluded list
				foundElt = elt;
				elt->data = NULL;
				stop = 1;
			}
		}

		rootOfNextSpur = list_get_first_elt(fromRoute->hops);
	}
	else
	{
		rootOfNextSpur = rootOfSpur->next;
	}

	rtgObj = terminusNode->routingObject;
	created = 0;
	stop = 0;
	result = 0;
	isFirstSpurRoute = 1;

	while (!stop)
	{
		if (!created)
		{
			last_computed_route = create_cgr_route();
			created = 1;
		}
		if (last_computed_route == NULL)
		{
			result = -2;
			stop = 1;
		}
		else
		{
			otherNeighbor = 0;

			ok = compute_spur_route(current_time, fromRoute, isFirstSpurRoute, rootOfSpur, terminusNode, last_computed_route);
			isFirstSpurRoute = 0;

			if (ok == 0)
			{
				result++;
#if (MAX_DIJKSTRA_ROUTES == 1)
				if (insert_known_route(rtgObj, last_computed_route) < 0)
				{
					result = -2;
					stop = 1;
				}

#else
				if (last_computed_route->neighbor == fromRoute->neighbor)
				{
					if (insert_known_route(rtgObj, last_computed_route) < 0)
					{
						result = -2;
						stop = 1;
					}
				}
				else
				{
					// --------- DISCOVERED ROUTE FROM NEW NEIGHBOR ---------
					//New neighbor, add route directly in Yen's "list A"
					//Always insert as first in selectedRoutes
					if (insert_selected_route(rtgObj, last_computed_route) == 0)
					{
						debug_printf("Discovered route from new neighbor (%llu).", last_computed_route->neighbor);
						otherNeighbor = 1;
						if (exclude_current_neighbor(last_computed_route) < 0)
						{
							result = -2;
							stop = 1;
						}
						// just to avoid future duplicate routes
						remove_neighbor_from_known_routes(rtgObj->knownRoutes, last_computed_route->neighbor);
					}
					else
					{
						result = -2;
						stop = 1;
					}
				}
#endif

				if (result != -2) //Success case
				{
					created = 0;
				}
			}
			else if (ok == -2) //MWITHDRAW error
			{
				result = -2;
				stop = 1;
			}
			else if (ok == -3) //Root path can't be used
			{
				stop = 1;
				verbose_debug_printf("Yen's algorithm must be stopped for the current route.");
			}
			else if(rootOfSpur == NULL)
			{
				//We have a route for all neighbors
				//Here we done a search from the graph's root and
				//we didn't find a new route, so we are sure
				//that there aren't route for other neighbors
				*allNeighborsFound = 1;
			}

			if (!otherNeighbor)
			{
				rootOfSpur = rootOfNextSpur;
				if (rootOfSpur != fromRoute->hops->last && rootOfSpur != upperBound)
				{
					rootOfNextSpur = rootOfNextSpur->next;
				}
				else
				{
					stop = 1;
					//reached destination or upper bound
				}

			}

		}
	}
	if (created)
	{
		delete_cgr_route(last_computed_route);
	}

	if (foundElt != NULL)
	{
		//re-include the neighbor in the excluded list
		foundElt->data = current;
	}

	return result;
}

/******************************************************************************
 *
 * \par Function Name:
 * 		compute_another_route
 *
 * \brief Implementation of the Yen's k-th shortest path algorithm.
 *
 * \details Compute the routes that branch off from a previous computed route,
 *          put these computed routes in the Yen's "list B", choose the best route
 *          from the Yen's "list B" and move that route to the Yen's "list A".
 *
 *
 * \par Date Written:
 * 		30/01/20
 *
 * \return int
 *
 * \retval   0	Success case: added route in Yen's "list A"
 * \retval  -1	Error case:	can't compute other routes from this route
 * \retval  -2	MWITHDRAW error
 * \retval  -3  Error case: Doesn't found routes in Yen's "list B"
 * \retval  -4  Error case: Cannot perform again Yen's algorithm on this route
 *
 * \param[in]  *fromRoute           The Route that we consider as "father" of the new Route
 * \param[in]  *terminusNode        The Node for which we want to compute a Route
 * \param[out]  *allNeighborsFound  Boolean: 1 if we have a computed route for each neighbor,
 *                                  0 otherwise
 *
 * \warning fromRoute           doesn't have to be NULL.
 * \warning terminusNode        doesn't have to be NULL.
 * \warning allNeighborsFound   doesn't have to be NULL.
 *
 * \par Notes:
 * 			1.	The relationship selectedFather-selectedChild will be managed
 *
 * \par Revision History:
 *
 *  DD/MM/YY | AUTHOR          |   DESCRIPTION
 *  -------- | --------------- |  -----------------------------------------------
 *  30/01/20 | L. Persampieri  |   Initial Implementation and documentation.
 *****************************************************************************/
static int compute_another_route(Route *fromRoute, Node *terminusNode, int *allNeighborsFound)
{
	int result = -4, totComputed = 0, computedNow = 0;
	Route *route;
	RtgObject *rtgObj = terminusNode->routingObject;
	ListElt *tempRootOfSpur;

	*allNeighborsFound = 0;

	if (fromRoute->spursComputed == 0)
	{
		if (fromRoute->children->length == 0)
		{

			//Only if this route hasn't children alive
			totComputed = compute_all_spurs(fromRoute, terminusNode, NULL, allNeighborsFound);
			computedNow = 1;
		}

		if (totComputed >= 0)
		{
			fromRoute->spursComputed = 1;
			result = 0;

			route = get_best_known_route(rtgObj, fromRoute->neighbor);

			if(route == NULL && fromRoute->rootOfSpur != NULL)
			{
				// 0 route found to the neighbor but we started from rootOfSpur
				// This must be done due to the use of the Lawler's modification to the Yen's algorithm
				// Remember that this is time-dependent graph
				tempRootOfSpur = fromRoute->rootOfSpur;
				fromRoute->rootOfSpur = NULL;
				result = compute_all_spurs(fromRoute, terminusNode, tempRootOfSpur, allNeighborsFound);
				fromRoute->rootOfSpur = tempRootOfSpur;

				if(result > 0)
				{
					route = get_best_known_route(rtgObj, fromRoute->neighbor);
				}
			}

			if(result != -2)
			{
				result = move_route_from_known_to_selected(route);
			}

			if (result == 0)
			{
				route->selectedFather = fromRoute;
				fromRoute->selectedChild = route;
			}
			else if(result == -1 && computedNow == 0)
			{
				result = -3;
			}
		}
		else
		{
			result = totComputed;
		}

	}
	return result;
}

/******************************************************************************
 *
 * \par Function Name:
 * 		computeOtherRoutes
 *
 * \brief For each route in Yen's "list A" we run the Yen's algorithm
 *        to compute other routes. Only one time per neighbor
 *
 *
 * \par Date Written:
 * 		30/01/20
 *
 * \return int
 *
 * \retval ">= 0"	Number of routes moved into the Yen's "list A" (selectedRoutes)
 * \retval    -2	MWITHDRAW error
 *
 * \param[in]  *terminusNode         The Node for which we want to compute the Routes
 * \param[in]  subsetComputedRoutes  The "fromRoute(s)" used by the Yen's algorithm to
 *                                   compute other routes
 * \param[in]  missingNeighbors      The number of missing neighbor. For each missing neighbor
 *                                   the phase two search a viable route.
 *
 * \warning terminusNode doesn't have to be NULL.
 *
 * \par Notes:
 *             1.  This function will updated the values of the previously
 *                 computed routes in knownRoutes. Previously == computed time < current time
 *
 *
 *
 * \par Revision History:
 *
 *  DD/MM/YY | AUTHOR          |   DESCRIPTION
 *  -------- | --------------- |  -----------------------------------------------
 *  30/01/20 | L. Persampieri  |   Initial Implementation and documentation.
 *****************************************************************************/
static int computeOtherRoutes(Node *terminusNode, List subsetComputedRoutes, long unsigned int missingNeighbors)
{
	int result = 0, computed = 0, stop = 0;
	int yenPerformedCorrectly = 0, discoveredAllNeighbors = 0, updateNeighbors = 0;
	long unsigned temp;
	RtgObject *rtgObj = terminusNode->routingObject;
	ListElt *elt, *next;
	Route *currentRoute;
	PhaseOneSAP *sap = get_phase_one_sap(NULL);
	time_t current_time = get_current_time();

	if (!sap->knownRoutesUpdated && subsetComputedRoutes != NULL && subsetComputedRoutes->length > 0)
	{
		//Only one time for CGR call and only if I'm effectively calling the Yen's algorithm
		//For all previously computed known routes
		elt = rtgObj->knownRoutes->first;
		while (elt != NULL)
		{
			next = elt->next;
			currentRoute = (Route*) elt->data;
			if (currentRoute->computedAtTime != current_time)
			{
				update_cost_values(current_time, currentRoute);
			}

			elt = next;
		}

		sap->knownRoutesUpdated = 1;
	}

#if (MAX_DIJKSTRA_ROUTES != 1)
	if (!sap->alreadyExcluded)
	{
		if(exclude_all_neighbors_from_computed_routes(rtgObj->selectedRoutes) < 0)
		{
			result = -2;
		}

		sap->alreadyExcluded = 1;
	}
#endif

	yenPerformedCorrectly = 0;
	updateNeighbors = 0;
	if (result != -2 && subsetComputedRoutes != NULL && subsetComputedRoutes->length > 0)
	{
		temp = rtgObj->selectedRoutes->length;
		stop = 0;

		debug_printf("Try Yen's algorithm on %lu routes", subsetComputedRoutes->length);

		for(elt = subsetComputedRoutes->first; elt != NULL && !stop; elt = elt->next)
		{
			currentRoute = (Route*) elt->data;
			if (currentRoute != NULL)
			{
				computed = compute_another_route(currentRoute, terminusNode, &discoveredAllNeighbors);

				if (computed == -2)
				{
					stop = 1;
					result = -2;
				}
				else if(computed != -3 && computed != -4)
				{
					//at least one time
					yenPerformedCorrectly = 1;
				}

				if(discoveredAllNeighbors == 1)
				{
					updateNeighbors = 1;
				}
			}
		}

		if (result != -2)
		{
			//set result to the new computed routes number
			result = (int) (rtgObj->selectedRoutes->length - temp);

			if(updateNeighbors)
			{
				exclude_all_neighbors_from_computed_routes(rtgObj->selectedRoutes);
			}
		}

	}

	if(result != -2 && missingNeighbors > 0 && !yenPerformedCorrectly)
	{
		// Yen doesn't performed and missing neighbors

		//phase two want a route for each "missing" neighbor
		//get at most one route for each "missing" neighbor
		debug_printf("Try Dijkstra's algorithm to find a route for %lu neighbors", missingNeighbors);
		result = computeOneRoutePerNeighbor(terminusNode, missingNeighbors);
	}

	return result;
}

#if (ADD_COMPUTED_ROUTE_TO_INTERMEDIATE_NODES == 1)
/******************************************************************************
 *
 * \par Function Name:
 * 		add_route
 *
 * \brief Add a route to a node only if there isn't already a route
 *        for this node from the same neighbor of the route that
 *        we want to add.
 *
 *
 * \par Date Written:
 * 		30/01/20
 *
 * \return int
 *
 * \retval   0	Success case: route added
 * \retval  -1	There is already a route with the same neighbor
 * \retval  -2	MWITHDRAW error
 *
 * \param[in]   *node           The Node to which we want to add the Route
 * \param[in]   *lastHopTonode  The last hop of the hops list that has
 *                              as receiver node the "node"
 * \param[in]   neighbor        The route's neighbor
 *
 * \warning node doesn't have to be NULL.
 * \warning lastHopToNode doesn't have to be NULL:
 *
 * \par Revision History:
 *
 *  DD/MM/YY | AUTHOR          |   DESCRIPTION
 *  -------- | --------------- |  -----------------------------------------------
 *  30/01/20 | L. Persampieri  |   Initial Implementation and documentation.
 *****************************************************************************/
static int add_route(Node *node, ListElt *lastHopToNode, unsigned long long neighbor)
{
	int result = -1, found;
	Contact *finalContact;
	Route *route;
	RtgObject *rtgObj = node->routingObject;
	ListElt *elt;

	found = 0;
	for (elt = rtgObj->selectedRoutes->first; elt != NULL && !found; elt = elt->next)
	{
		route = (Route*) elt->data;

		if (route->neighbor == neighbor)
		{
			found = 1;
		}
	}

	if (!found)
	{
		route = create_cgr_route();

		if (route != NULL)
		{
			finalContact = (Contact*) lastHopToNode->data;
			result = populate_route(finalContact, &graphRoot, route);

			if (result == 0)
			{
				//Always insert as first in selectedRoutes!!!
				if(insert_selected_route(rtgObj, route) < 0)
				{
					result = -2;
				}
			}

			if (result != 0)
			{
				delete_cgr_route(route);
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
 * 		add_computed_route_to_intermediate_nodes
 *
 * \brief Add the route to all receiver nodes in the hops list of the route
 *
 *
 * \par Date Written:
 * 		30/01/20
 *
 * \return int
 *
 * \retval  ">= 0"  Success case: number of routes added
 * \retval     -2   MWITHDRAW error
 *
 * \param[in]   *route   The route from which we get the hops list
 *                       to add a route for each hop receiver node
 *
 * \warning route doesn't have to be NULL.
 *
 * \par Notes:
 *             1.  This function should be used only after than
 *                 a "pure" Dijkstra's search (no Yen's algorithm).
 *                 If we compute the shortest path for a node we have computed
 *                 the shortest path for all the nodes in the hops list.
 *
 * \par Revision History:
 *
 *  DD/MM/YY | AUTHOR          |   DESCRIPTION
 *  -------- | --------------- |  -----------------------------------------------
 *  30/01/20 | L. Persampieri  |   Initial Implementation and documentation.
 *****************************************************************************/
static int add_computed_route_to_intermediate_nodes(Route *route)
{
	ListElt *elt, *last;
	Contact *current;
	Node *currentNode;
	int result = 0, count = 0;
	unsigned long long neighbor;

	last = route->hops->last;
	current = (Contact*) route->hops->first->data;
	neighbor = current->toNode;

	for (elt = route->hops->first; elt != last && result != -2; elt = elt->next)
	{
		current = (Contact*) elt->data;
		currentNode = add_node(current->toNode);
		if (currentNode != NULL)
		{
			result = add_route(currentNode, elt, neighbor);
			if (result == 0)
			{
				count++;
			}
		}
		else
		{
			result = -2;
		}
	}

	if (result != -2)
	{
		result = count;
	}

	return result;
}
#endif

/******************************************************************************
 *
 * \par Function Name:
 * 		computeOneRoutePerNeighbor
 *
 * \brief Compute one route for each neighbor that has a path to reach the destination.
 *
 *
 * \par Date Written:
 * 		30/01/20
 *
 * \return int
 *
 * \retval   ">= 0"	Number of routes computed and added to selectedRoutes
 * \retval      -2	MWITHDRAW error
 *
 * \param[in]  *terminusNode     The Node (destination) for which we are computing the routes
 * \param[in]  missingNeighbors  The number of routes to compute (one route for each missing neighbor)
 *                               MUST be greater than 0.
 * \warning terminusNode doesn't have to be NULL.
 *
 * \par Revision History:
 *
 *  DD/MM/YY | AUTHOR          |   DESCRIPTION
 *  -------- | --------------- |  -----------------------------------------------
 *  30/01/20 | L. Persampieri  |   Initial Implementation and documentation.
 *  27/04/20 | L. Persampieri  |   Refactoring
 *****************************************************************************/
static int computeOneRoutePerNeighbor(Node *terminusNode, long unsigned int missingNeighbors)
{
	int result, stop = 0;
	int ok;
	Route *route;
	RtgObject *rtgObj;
	ClearRule rule;
	PhaseOneSAP *sap = get_phase_one_sap(NULL);

	/*
	 * Every time we compute a route we add the neighbor to the excluded list,
	 * so the next routes branches off from another neighbor. At the first
	 * dijkstra's failed search we have computed the routes for all neighbors
	 * that have a path to destination.
	 */

	result = 0;
	rtgObj = terminusNode->routingObject;

	// Only for the first time that we compute routes for the destination
	// Note: the computeOtherRoutes function excluded all the neighbors
	// for which we already have a route and set alreadyExcluded to 1
	if(!(sap->alreadyExcluded))
	{
		result = exclude_all_neighbors_from_computed_routes(rtgObj->selectedRoutes);

		if(result < 0)
		{
			stop = 1;
		}
		else
		{
			result = 0;
		}

		sap->alreadyExcluded = 1;

	}
	/*
	 * We have excluded all neighbors for whom we already have at least a route
	 *
	 * For each new route found (necessarily from a new neighbor) we exclude the new neighbor
	 *
	 */

	ok = 0;
	if(!(sap->graphCleaned))
	{
		// first time during the current call
		rule = ClearTotally;
	}
	else
	{
		rule = ClearPartially;
	}

	if(missingNeighbors > 0)
	{
		while (!stop)
		{
			route = create_cgr_route();
			if (route != NULL)
			{
				clear_work_areas(rule);

				ok = dijkstra_search(&(sap->graphRoot), terminusNode->nodeNbr, route);

				rule = ClearPartially; //for each following Dijkstra's search

				if (ok == 0)
				{
					route->rootOfSpur = NULL;
					//Always insert as first element in selectedRoutes
					if (insert_selected_route(rtgObj, route) == 0)
					{
						result++;
						remove_neighbor_from_known_routes(rtgObj->knownRoutes, route->neighbor);

						if (result >= missingNeighbors)
						{
							stop = 1;
						}
						// Necessarily a new neighbor
						if (exclude_current_neighbor(route) < 0)
						{
							result = -2;
							stop = 1; //I leave the loop
						}
#if (ADD_COMPUTED_ROUTE_TO_INTERMEDIATE_NODES == 1)
						if(add_computed_route_to_intermediate_nodes(route) < 0)
						{
							result = -2;
							stop = 1; //I leave the loop
						}
#endif
					}
					else
					{
						delete_cgr_route(route);
						stop = 1;
						result = -2;
					}
				}
				else if (ok == -1) //no more routes
				{
					delete_cgr_route(route);
					stop = 1;
				}
				else //MWITHDRAW error
				{
					delete_cgr_route(route);
					stop = 1;
					result = -2;
				}
			}
			else //MWITHDRAW error
			{
				result = -2;
				stop = 1;
			}
		}
	}
	else
	{
		verbose_debug_printf("0 missing neighbors...");
	}

#if (MAX_DIJKSTRA_ROUTES == 1)
	free_list_elts(sap->excludedNeighbors);
#endif

	return result;
}

/******************************************************************************
 *
 * \par Function Name:
 * 		computeRoutes
 *
 * \brief Phase one entry point: get the computed routes list
 *
 *
 * \par Date Written:
 * 		30/01/20
 *
 * \return int
 *
 * \retval  ">= 0"  Success case: number of routes computed
 * \retval     -1   There are no routes to reach the destination
 * \retval     -2   MWITHDRAW error
 * \retval     -3   Arguments error
 *
 * \param[in]  *terminusNode         The Node for which we want to compute the routes
 * \param[in]  subsetComputedRoutes  Phase two recommends Yen's algorithm for each route
 *                                   in this set
 * \param[in]  missingNeighbors      The number of routes to find (from new neighbors)
 *
 *
 * \par Revision History:
 *
 *  DD/MM/YY | AUTHOR          |   DESCRIPTION
 *  -------- | --------------- |  -----------------------------------------------
 *  30/01/20 | L. Persampieri  |   Initial Implementation and documentation.
 *****************************************************************************/
int computeRoutes(unsigned long regionNbr, Node *terminusNode, List subsetComputedRoutes, long unsigned int missingNeighbors)
{

	int result = -1;
	RtgObject *rtgObj = NULL;
	PhaseOneSAP *sap = get_phase_one_sap(NULL);

	record_phases_start_time(phaseOne);

	debug_printf("Entry point phase one.");

	if (missingNeighbors > 0 && terminusNode != NULL)
	{
		// Assumption: terminusNode is correctly initialized

		sap->regionNbr = regionNbr;
		sap->destination = terminusNode->nodeNbr;

		rtgObj = terminusNode->routingObject;

			if (rtgObj->selectedRoutes->length == 0)
			{
				sap = get_phase_one_sap(NULL);
				sap->knownRoutesUpdated = 1;

				//reset knownRoutes, all the selectedRoutes are expired
				//I can't know who are the shortest path looking only
				//in knownRoutes
				clear_routes_list(rtgObj->knownRoutes); //reset the list

				result = computeOneRoutePerNeighbor(terminusNode, missingNeighbors);

			}
			else //Compute the next shortest path for each route in the subset
			{
				result = computeOtherRoutes(terminusNode, subsetComputedRoutes, missingNeighbors);
			}

		if (result != -2 && rtgObj->selectedRoutes->length == 0)
		{
			result = -1; // no routes to reach destination
		}
	}
	else
	{
		result = -3;
	}

	if(result == -1)
	{
		debug_printf("No routes to destination");
	}
	else if(result >= 0)
	{
		debug_printf("New routes computed: %d", result);
	}
	else
	{
		debug_printf("Result -> %d", result);
	}

	record_phases_stop_time(phaseOne);

	return result;
}

#if (LOG == 1)
/******************************************************************************
 *
 * \par Function Name:
 * 		print_phase_one_route
 *
 * \brief Print the route in the "call" file in phase one format
 *
 *
 * \par Date Written:
 * 		30/01/20
 *
 * \return void
 *
 * \param[in]   file    The file where we want to print the route
 * \param[in]   *route  The Route that we want to print.
 * \param[in]   num     The number of the route in the list
 *
 * \par Notes:
 *             1. For phase one computed routes we print only the values computed during that phase.
 *
 * \par Revision History:
 *
 *  DD/MM/YY | AUTHOR          |   DESCRIPTION
 *  -------- | --------------- |  -----------------------------------------------
 *  30/01/20 | L. Persampieri  |   Initial Implementation and documentation.
 *****************************************************************************/
static void print_phase_one_route(FILE *file, Route *route, unsigned int num)
{
	ListElt *elt;
	Contact *contact;
	char temp[20];
	Route *father;

	/* If you have to work with very large numbers change all fields in %-20 */

	if (file != NULL && route != NULL)
	{
		route->num = num;
		father = get_route_father(route);

		temp[0] = '\0';
		if (father != NULL)
		{
			sprintf(temp, "%u)", father->num);
		}
		fprintf(file,
				"\n%u) ComputedAtTime: %ld\n%-15s %-15s %-15s %-15s %-15s %-15s %-15s %-15s %s\n",
				route->num, (long int) route->computedAtTime, "Neighbor", "FromTime", "ToTime",
				"ArrivalTime", "OwltSum", "Confidence", "Children", "Father", "Hops");
		fprintf(file, "%-15llu %-15ld %-15ld %-15ld %-15u %-15.2f %-15lu %-15s ", route->neighbor,
				(long int) route->fromTime, (long int) route->toTime, (long int) route->arrivalTime,
				route->owltSum, route->arrivalConfidence, route->children->length,
				(father != NULL) ? temp : "no");

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
 * 		print_phase_one_routes
 *
 * \brief Print the routes in the file in phase one format
 *
 *
 * \par Date Written:
 * 		30/01/20
 *
 * \return void
 *
 * \param[in]   file               The file where we want to print the routes
 * \param[in]   computedRoutes   The routes that we want to print
 *
 *
 * \par Revision History:
 *
 *  DD/MM/YY | AUTHOR          |   DESCRIPTION
 *  -------- | --------------- |  -----------------------------------------------
 *  30/01/20 | L. Persampieri  |   Initial Implementation and documentation.
 *****************************************************************************/
void print_phase_one_routes(FILE *file, List computedRoutes)
{
	ListElt *elt;
	unsigned int count = 1;

	if(file != NULL)
	{

		fprintf(file,
				"\n--------------------------------------------------------- PHASE ONE: COMPUTED ROUTES ----------------------------------------------------------\n");
		if (computedRoutes != NULL && computedRoutes->length > 0)
		{
			for (elt = computedRoutes->last; elt != NULL; elt = elt->prev)
			{
				print_phase_one_route(file, (Route*) elt->data, count);
				count++;
			}
		}
		else
		{
			fprintf(file, "\n0 computed routes.\n");
		}
		fprintf(file,
				"\n-----------------------------------------------------------------------------------------------------------------------------------------------\n");

		debug_fflush(file);
	}
}

#endif

