/** \file nodes.h
 *
 * \brief  This file provides the definition of the Node type and
 *         of the RtgObject type, with all the declarations
 *         of the functions to manage the node tree.
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

#ifndef SOURCES_CONTACTS_PLAN_NODES_NODES_H_
#define SOURCES_CONTACTS_PLAN_NODES_NODES_H_

#include "../../library/commonDefines.h"
#include "../../library/list/list_type.h"

typedef struct node Node;

typedef struct
{
	/**
	 * \brief Back-reference.
	 */
	Node *nodeAddr;
	/**
	 * \brief Computed routes list (and Yen's "list A")
	 */
	List selectedRoutes;
	/**
	 * \brief Yen's "list B"
	 */
	List knownRoutes;
	/**
	 * \brief Each element in this list is a pointer to the ListElt of the relative neighbor
	 *        in the local node's neighbor list.
	 *
	 * \details  In this list we store the citations to only the neighbors that could
	 *           be used to reach the Node (intended as destination node)
	 *
	 * \par Notes:
	 *          1.  Cross-reference between relative citations elements in RtgObject and Neighbor
	 */
	List citations;
	/**
	 * \brief This is a bit mask
	 * - Routes computed bit (SET_COMPUTED, UNSET_COMPUTED and ALREADY_COMPUTED macros)
	 * - Routes not found bit (SET_ROUTES_NOT_FOUND, UNSET_ROUTES_NOT_FUND and ROUTES_NOT_FOUND macros)
	 * - You can clear this mask with CLEAR_FLAGS macro
	 */
	unsigned char flags;
} RtgObject;

struct node
{
	/**
	 * \brief Ipn node number
	 */
	unsigned long long nodeNbr;
	/**
	 * \brief Where we store the routes computed to reach this node
	 */
	RtgObject *routingObject;
};

/**************** NODE & ROUTING OBJECT FLAGS MACROS ****************/

// #define COMPUTED        (1) /* (00000001) */
// #define NO_ROUTES       (2) /* (00000010) */
#define NEIGHBORS       (4) /* (00000100) */

/*
#define SET_COMPUTED(rtgObj) (((rtgObj)->flags) |= COMPUTED)
#define UNSET_COMPUTED(rtgObj) (((rtgObj)->flags) &= ~COMPUTED)
#define ALREADY_COMPUTED(rtgObj) (((rtgObj)->flags) & COMPUTED)

#define SET_ROUTES_NOT_FOUND(rtgObj) (((rtgObj)->flags) |= NO_ROUTES)
#define UNSET_ROUTES_NOT_FOUND(rtgObj) (((rtgObj)->flags) &= ~NO_ROUTES)
#define ROUTES_NOT_FOUND(rtgObj) (((rtgObj)->flags) & NO_ROUTES)
*/
#define SET_NEIGHBORS_DISCOVERED(rtgObj) (((rtgObj)->flags) |= NEIGHBORS)
#define UNSET_NEIGHBORS_DISCOVERED(rtgObj) (((rtgObj)->flags) &= ~NEIGHBORS)
#define NEIGHBORS_DISCOVERED(rtgObj) (((rtgObj)->flags) & NEIGHBORS)

/********************************************************************/

typedef struct
{
	/**
	 * \brief The ipn number of the local node's neighbor
	 */
	unsigned long long ipn_number;
	/**
	 * \brief The time when the last contact to this neighbor expires
	 */
	time_t toTime;
	/**
	 * \brief This is a bit mask
	 * - Candidate routes already found for this neighbor bit (SET_CANDIDATE_ROUTES_FOUND, UNSET_CANDIDATE_ROUTE_FOUND and CANDIDATE_ROUTES_FOUND macros)
	 * - There are already routes in subset bit (SET_ROUTES_IN_SUBSET, UNSET_ROUTES_IN_SUBSET and ROUTES_IN_SUBSET macros)
	 * - You can clear this mask with CLEAR_FLAGS macro
	 */
	unsigned char flags;
	/**
	 * \brief Each element in this list is the ListElt element of the relative
	 *        citation in RtgObject.
	 *
	 * \par Notes:
	 *          1.  Cross-reference between relative citations elements in RtgObject and Neighbor
	 */
	List citations;
} Neighbor;

/*********************** NEIGHBOR FLAGS MACROS ***********************/

#define CANDIDATE_ROUTES (1) /* 00000001 */
#define SUBSET_ROUTES    (2) /* 00000010 */

#define SET_CANDIDATE_ROUTES_FOUND(neighbor) (((neighbor)->flags) |= CANDIDATE_ROUTES)
#define UNSET_CANDIDATE_ROUTES_FOUND(neighbor) (((neighbor)->flags) &= ~CANDIDATE_ROUTES)
#define CANDIDATE_ROUTES_FOUND(neighbor) (((neighbor)->flags) & CANDIDATE_ROUTES)

#define SET_ROUTES_IN_SUBSET(neighbor) (((neighbor)->flags) |= SUBSET_ROUTES)
#define UNSET_ROUTES_IN_SUBSET(neighbor) (((neighbor)->flags) &= ~SUBSET_ROUTES)
#define ROUTES_IN_SUBSET(neighbor) (((neighbor)->flags) & SUBSET_ROUTES)

/*********************************************************************/

#ifdef __cplusplus
extern "C"
{
#endif


extern int create_NodesTree();

extern void discardAllRoutesFromNodesTree();

extern int add_node_to_graph(unsigned long long nodeNbrToAdd);
extern Node* add_node(unsigned long long nodeNbr);

extern void remove_node_from_graph(unsigned long long nodeNbrToRemove);

extern Node* get_node(unsigned long long nodeNbr);

extern void reset_NodesTree();
extern void destroy_NodesTree();

extern Neighbor * get_neighbor(unsigned long long node_number);
extern long unsigned int get_local_node_neighbors_count();
extern void reset_neighbors_temporary_fields();
extern int insert_neighbors_to_reach_destination(List neighbors, Node *destination);
extern void removeOldNeighbors(time_t current_time);
extern int build_local_node_neighbors_list(unsigned long long localNode);
extern int is_node_in_destination_neighbors_list(Node *destination, unsigned long long node);

#ifdef __cplusplus
}
#endif

#endif /* SOURCES_CONTACTS_PLAN_NODES_NODES_H_ */
