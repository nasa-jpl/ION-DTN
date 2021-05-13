/** \file nodes.c
 *
 *  \brief This file provides the implementation of the functions
 *         to manage the node tree.
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

#include "nodes.h"

#include <stdlib.h>

#include "../contacts/contacts.h"
#include "../../library/list/list.h"
#include "../../library_from_ion/rbt/rbt.h"
#include "../../routes/routes.h"

/**
 * \brief USed to keep in one place the data used for the neighbors management.
 *
 * \par Notes:
 *             1. A neighbor here is intended as a node (N) for which there is a direct contact from the local node to this node (N).
 */
typedef struct {
	/**
	 * \brief The list of all neighbors of the local node.
	 */
	List local_node_neighbors;
	/**
	 * \brief The last contact to a neighbor expires at this time.
	 */
	time_t timeNeighborToRemove;
	/**
	 * \brief 1 if has been performed the search of the local node's neighbors.
	 */
	int neighbors_list_builded;
} NeighborsSAP;

static int compare_nodes(void*, void*);
static void free_node(void*);
static void erase_node(Node*);
static void erase_rtg_object(RtgObject *rtgObj);
static void free_neighbor(void*);
static void remove_citation(void*);


/******************************************************************************
 *
 * \par Function Name:
 *      get_node_graph
 *
 * \brief Get the node graph
 *
 *
 * \par Date Written:
 *      02/07/20
 *
 * \return Rbt*
 *
 * \retval  Rbt*   The node graph
 *
 * \param[in] *newNodes   Set to NULL if you just want to get the node graph.
 *                        Otherwise, set != NULL and the node graph will be overwritten.
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  02/07/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
static Rbt *get_node_graph(Rbt *newNodes) {
	static Rbt* nodes;

	if(newNodes != NULL) {
		nodes = newNodes;
	}

	return nodes;
}

/******************************************************************************
 *
 * \par Function Name:
 *      get_neighbors_sap
 *
 * \brief Get the reference to NeighborsSAP struct
 *
 *
 * \par Date Written:
 *      02/07/20
 *
 * \return NeighborsSAP*
 *
 * \retval  NeighborsSAP*   The reference to NeighborsSAP struct
 *
 * \param[in] *newSap     Set to NULL if you just want to get the reference to NeighborsSAP.
 *                        Otherwise, set != NULL and the NeighborsSAP will be overwritten.
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  02/07/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
static NeighborsSAP *get_neighbors_sap(NeighborsSAP *newSap) {
	static NeighborsSAP sap;

	if(newSap != NULL) {
		sap = *newSap;
	}

	return &sap;
}

/******************************************************************************
 *
 * \par Function Name:
 *      compare_nodes
 *
 * \brief Compare function for Node type
 *
 *
 * \par Date Written:
 *      15/01/20
 *
 * \return int
 *
 * \retval   0   The two nodes have the same ipn node number
 * \retval  -1   The first node has a less ipn node number than the second node
 * \retval   1   The first node has a greater ipn node number than the second node
 *
 * \param[in]  *first    The first node
 * \param[in]  *second   The second node
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  15/01/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
static int compare_nodes(void *first, void *second)
{
	int result = 0;
	Node *a, *b;
	if (first == second) //if they point to the same address
	{
		result = 0;
	}
	else if (first != NULL && second != NULL)
	{
		a = (Node*) first;
		b = (Node*) second;

		if (a->nodeNbr > b->nodeNbr)
		{
			result = 1;
		}
		else if (a->nodeNbr < b->nodeNbr)
		{
			result = -1;
		}
		else
		{
			result = 0;
		}
	}
	return result;
}

/******************************************************************************
 *
 * \par Function Name:
 *      free_rtg_object
 *
 * \brief Deallocate memory for a RtgObject type
 *
 *
 * \par Date Written:
 *      15/01/20
 *
 * \return void
 *
 * \param[in]  *data   The RtgObject that we want to deallocate
 *
 * \par Notes:
 *             1.  All routes in selectedRoutes and in knownRoutes will be deallocated.
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  15/01/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
static void free_rtg_object(void *data)
{
	RtgObject *rtgObj;
	if (data != NULL)
	{
		rtgObj = (RtgObject*) data;
		destroy_routes_list(rtgObj->knownRoutes);
		rtgObj->knownRoutes = NULL;
		destroy_routes_list(rtgObj->selectedRoutes);
		rtgObj->selectedRoutes = NULL;
		rtgObj->nodeAddr = NULL;
		free_list(rtgObj->citations);
		erase_rtg_object(rtgObj);
		MDEPOSIT(rtgObj);

	}
}

/******************************************************************************
 *
 * \par Function Name:
 *      erase_rtg_object
 *
 * \brief Reset all the RtgObject's fields
 *
 *
 * \par Date Written:
 *      15/01/20
 *
 * \return void
 *
 * \param[in]  *rtgObj   The RtgObject for which we want to reset all the fields
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  15/01/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
static void erase_rtg_object(RtgObject *rtgObj)
{
	memset(rtgObj,0,sizeof(RtgObject));
}

/******************************************************************************
 *
 * \par Function Name:
 *      erase_node
 *
 * \brief  Reset all the Node's fields
 *
 *
 * \par Date Written:
 *      15/01/20
 *
 * \return void
 *
 * \param[in]  *node   The Node for which we want to reset all the fields
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  15/01/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
static void erase_node(Node *node)
{
	node->nodeNbr = 0;
	node->routingObject = NULL;

	return;
}

/******************************************************************************
 *
 * \par Function Name:
 *      free_node
 *
 * \brief  Deallocate memory for a Node type
 *
 *
 * \par Date Written:
 *      15/01/20
 *
 * \return void
 *
 * \param[in]  *node   The Node that we want to deallocate
 *
 * \par Notes:
 *             1.  All routes computed to reach this ipn node will be deallocated.
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  15/01/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
static void free_node(void *node)
{
	Node *temp;
	if (node != NULL)
	{
		temp = (Node*) node;
		free_rtg_object(temp->routingObject);
		erase_node(temp);
		MDEPOSIT(temp);
	}

	return;
}
/******************************************************************************
 *
 * \par Function Name:
 *      create_NodesTree
 *
 * \brief  Allocate memory for the nodes tree (rbt)
 *
 *
 * \par Date Written:
 *      15/01/20
 *
 * \return int
 *
 * \retval   1   Success case: The tree now exists
 * \retval  -2   MWITHDRAW error
 *
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  15/01/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
int create_NodesTree()
{
	int result = 1;
	Rbt *nodes = get_node_graph(NULL);
	NeighborsSAP *sap = get_neighbors_sap(NULL);

	if (nodes == NULL)
	{
		nodes = rbt_create(free_node, compare_nodes);
		sap->local_node_neighbors = list_create(NULL, NULL, NULL, free_neighbor);

		if (nodes != NULL && sap->local_node_neighbors != NULL)
		{
			result = 1;
			get_node_graph(nodes); // save
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
 *      discardRoute
 *
 * \brief  Delete a route but doesn't touch other references to contacts or other routes
 *
 *
 * \par Date Written:
 *      15/01/20
 *
 * \return void
 *
 * \param[in]  *data  The route that we want to delete
 *
 * \warning Use this function ONLY when you are deleting all the routes and all the references from the contacts graph.
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  15/01/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
static void discardRoute(void *data)
{
	Route *route = (Route*) data;
	free_list(route->hops); 	//hops list has NULL as delete function
	free_list(route->children); //children list has NULL as delete function
	MDEPOSIT(route);
	return;
}

/******************************************************************************
 *
 * \par Function Name:
 *      discardAllRoutesFromNodesTree
 *
 * \brief Delete a route but doesn't touch other references to contacts or other routes
 *
 *
 * \par Date Written:
 *      15/01/20
 *
 * \return void
 *
 * \param[in]  *data   The route that we want to delete
 *
 * \warning Use this function ONLY when you are deleting all the routes and all the references from the contacts graph (for efficiency).
 *
 * \par Notes:
 *              1.  If you call this function you must call even the correspective function for the contacts graph.
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  15/01/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
void discardAllRoutesFromNodesTree()
{
	Node *node;
	RtgObject *rtgObj;
	RbtNode *elt;
	delete_function deleteFn;
	Rbt *nodes = get_node_graph(NULL);

	for (elt = rbt_first(nodes); elt != NULL; elt = rbt_next(elt))
	{
		node = (Node*) elt->data;
		rtgObj = node->routingObject;

		CLEAR_FLAGS(rtgObj->flags);

		deleteFn = rtgObj->knownRoutes->delete_data_elt;
		rtgObj->knownRoutes->delete_data_elt = discardRoute;
		free_list_elts(rtgObj->knownRoutes);
		rtgObj->knownRoutes->delete_data_elt = deleteFn;

		deleteFn = rtgObj->selectedRoutes->delete_data_elt;
		rtgObj->selectedRoutes->delete_data_elt = discardRoute;
		free_list_elts(rtgObj->selectedRoutes);
		rtgObj->selectedRoutes->delete_data_elt = deleteFn;
	}
}

/******************************************************************************
 *
 * \par Function Name:
 *      reset_NodesTree
 *
 * \brief  Delete all the nodes from the nodes tree, but not the tree itself
 *
 *
 * \par Date Written:
 *      15/01/20
 *
 * \return void
 *
 *
 * \par Notes:
 *             1.  All routes will be discarded.
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  15/01/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
void reset_NodesTree()
{
	Rbt *nodes = get_node_graph(NULL);
	NeighborsSAP *sap = get_neighbors_sap(NULL);
	free_list_elts(sap->local_node_neighbors);
	rbt_clear(nodes);
	sap->neighbors_list_builded = 0;
	sap->timeNeighborToRemove = MAX_POSIX_TIME;
}

/******************************************************************************
 *
 * \par Function Name:
 *      destroy_NodesTree
 *
 * \brief  Delete all the nodes from the nodes tree, and the tree itself
 *
 *
 * \par Date Written:
 *      15/01/20
 *
 * \return void
 *
 *
 * \par Notes:
 *             1.  All routes will be discarded.
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  15/01/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
void destroy_NodesTree()
{
	Rbt *nodes = get_node_graph(NULL);
	NeighborsSAP *sap = get_neighbors_sap(NULL);
	free_list(sap->local_node_neighbors);
	rbt_destroy(nodes);
	sap->neighbors_list_builded = 0;
	sap->timeNeighborToRemove = MAX_POSIX_TIME;
	sap->local_node_neighbors = NULL;
	nodes = NULL;
	get_node_graph(nodes); // save
}

/******************************************************************************
 *
 * \par Function Name:
 *      create_rtg_object
 *
 * \brief Allocate memory for a RtgObject type
 *
 *
 * \par Date Written:
 *      15/01/20
 *
 * \return RtgObject*
 *
 * \retval RtgObject*  The pointer to the new allocated RtgObject
 * \retval NULL        MWITHDRAW error
 *
 * \param[in]  *node   The node to which the new allocated RtgObject will refers.
 *
 * \par Notes:
 *             1.  You must check that the return value of this function is not NULL.
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  15/01/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
static RtgObject* create_rtg_object(Node *node)
{
	RtgObject *rtgObj = NULL;

	if (node != NULL)
	{
		rtgObj = (RtgObject*) MWITHDRAW(sizeof(RtgObject));

		if (rtgObj != NULL)
		{
			rtgObj->nodeAddr = node;
			rtgObj->knownRoutes = list_create(rtgObj, NULL, NULL, delete_cgr_route);
			rtgObj->selectedRoutes = list_create(rtgObj, NULL, NULL, delete_cgr_route);
			rtgObj->citations = list_create(rtgObj, NULL, NULL, remove_citation);
			CLEAR_FLAGS(rtgObj->flags);

			if (rtgObj->knownRoutes == NULL || rtgObj->selectedRoutes == NULL || rtgObj->citations == NULL)
			{
				free_rtg_object(rtgObj);
				rtgObj = NULL;
			}
		}

	}

	return rtgObj;
}

/******************************************************************************
 *
 * \par Function Name:
 *      create_node
 *
 * \brief  Allocate memory for a Node type
 *
 *
 * \par Date Written:
 *      15/01/20
 *
 * \return Node*
 *
 * \retval Node*  The new allocated Node 
 * \retval NULL   MWITHDRAW error
 *
 * \param[in]  nodeNbr  The ipn node number of the new Node
 *
 * \par Notes:
 *             1.  You must check that the return value of this function is not NULL.
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  15/01/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
static Node* create_node(unsigned long long nodeNbr)
{
	Node *result = (Node*) MWITHDRAW(sizeof(Node));

	if (result != NULL)
	{
		result->nodeNbr = nodeNbr;
		result->routingObject = create_rtg_object(result);
		if (result->routingObject == NULL)
		{
			free_node(result);
			result = NULL;
		}
	}

	return result;
}

/******************************************************************************
 *
 * \par Function Name:
 *      add_node
 *
 * \brief Add the ipn node to the nodes tree only if it isn't already inside.
 *
 *
 * \par Date Written:
 *      15/01/20
 *
 * \return Node*
 *
 * \retval Node*   The Node found or inserted 
 * \retval NULL    MWITHDRAW error
 *
 * \param[in]  nodeNbr   The ipn node number of the Node to add
 *
 * \par Notes:
 *             1.  You must check that the return value of this function is not NULL.
 *             2.  This function could be used to search a Node, if the Node isn't already inside
 *                 it will be inserted.
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  15/01/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
Node* add_node(unsigned long long nodeNbr)
{
	RbtNode *elt;
	Node *node;
	Rbt *nodes;

	node = get_node(nodeNbr);

	if (node == NULL)
	{
		node = create_node(nodeNbr);
		if (node != NULL)
		{
			nodes = get_node_graph(NULL);
			elt = rbt_insert(nodes, node);
			if (elt == NULL)
			{
				free_node(node);
				node = NULL;
			}
		}
	}

	return node;
}

/******************************************************************************
 *
 * \par Function Name:
 *      remove_node_from_graph
 *
 * \brief Deallocate the Node with the nodeNbr (ipn node number) passed as argument
 *
 *
 * \par Date Written:
 *      15/01/20
 *
 * \return void
 *
 * \param[in]  nodeNbr  The ipn node number of the Node to deallocate.
 *
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  15/01/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
void remove_node_from_graph(unsigned long long nodeNbr)
{
	Node node;
	Rbt *nodes = get_node_graph(NULL);

	node.nodeNbr = nodeNbr;
	node.routingObject = NULL;
	rbt_delete(nodes, &node);
	return;
}

/******************************************************************************
 *
 * \par Function Name:
 *      add_node_to_Graph
 *
 * \brief  Insert a Node inside the nodes tree
 *
 *
 * \par Date Written:
 *      15/01/20
 *
 * \return int
 *
 * \retval   1  Success case: the node now is inside the nodes tree
 * \retval  -2  MWITHDRAW error
 *
 * \param[in]  nodeNbr  The ipn node number of the new Node
 *
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  15/01/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
int add_node_to_graph(unsigned long long nodeNbr)
{
	int result;
	Node *node;

	node = add_node(nodeNbr);

	if (node != NULL)
	{
		result = 1;
	}
	else
	{
		result = -2;
	}

	return result;
}

/*
 * Functions to search a Node
 */

/******************************************************************************
 *
 * \par Function Name:
 *      get_node
 *
 * \brief  Get the node with the nodeNbr field (ipn node number) passed as argument
 *
 *
 * \par Date Written:
 *      15/01/20
 *
 * \return Node*
 *
 * \retval Node*  The Node found
 * \retval NULL   The Node isn't in the nodes tree
 *
 * \param[in]  nodeNbr  The ipn node number of the new Node
 *
 * \par Notes:
 *             1.  You must check that the return value of this function is not NULL.
 *
 * \par Revision History:
 *
 *  DD/MM/YY  AUTHOR            DESCRIPTION
 *  --------  ---------------  -----------------------------------------------
 *  15/01/20  L. Persampieri    Initial Implementation and documentation.
 *****************************************************************************/
Node* get_node(unsigned long long nodeNbr)
{
	Node arg;
	Node *result = NULL;
	RbtNode *currentNode = NULL;
	Rbt *nodes = get_node_graph(NULL);

	if (nodeNbr > 0)
	{
		erase_node(&arg);
		arg.nodeNbr = nodeNbr;
		currentNode = rbt_search(nodes, &arg, NULL);
		if (currentNode != NULL)
		{
			result = (Node*) currentNode->data;
		}
	}
	return result;
}

/******************************************************************************
 *
 * \par Function Name:
 *      remove_citation
 *
 * \brief  Remove the citation between neighbor and node's routing object
 *
 *
 * \par Date Written:
 *      28/04/20
 *
 * \return void
 *
 *
 * \param[in]  data   The citation to remove
 *
 * \par Revision History:
 *
 *  DD/MM/YY  AUTHOR            DESCRIPTION
 *  --------  ---------------  -----------------------------------------------
 *  28/04/20  L. Persampieri    Initial Implementation and documentation.
 *****************************************************************************/
static void remove_citation(void *data)
{
	ListElt *elt;
	List elt_list;
	delete_function temp;

	if(data != NULL)
	{
		elt = (ListElt *) data;
		elt_list = elt->list;

		if(elt_list != NULL)
		{
			temp = elt_list->delete_data_elt;
			elt_list->delete_data_elt = NULL; // no chain events

			list_remove_elt(elt);

			elt_list->delete_data_elt = temp;
		}
	}
}

/******************************************************************************
 *
 * \par Function Name:
 *      create_neighbor
 *
 * \brief  Allocate dynamic memory to Neighbor type
 *
 *
 * \par Date Written:
 *      28/04/20
 *
 * \return Neighbor*
 *
 * \retval Neighbor*  The neighbor allocated
 * \retval NULL       MWITHDRAW or arguments error
 *
 * \param[in]  node_number  The neighbor's ipn number
 * \param[in]  to_time      The time when expires the last contact from the local node
 *                          to this neighbor
 *
 * \par Revision History:
 *
 *  DD/MM/YY  AUTHOR            DESCRIPTION
 *  --------  ---------------  -----------------------------------------------
 *  28/04/20  L. Persampieri    Initial Implementation and documentation.
 *****************************************************************************/
static Neighbor* create_neighbor(unsigned long long node_number, time_t to_time)
{
	Neighbor *result = NULL;

	if(node_number > 0 && to_time >= 0)
	{
		result = (Neighbor*) MWITHDRAW(sizeof(Neighbor));

		if(result != NULL)
		{
			result->ipn_number = node_number;
			result->toTime = to_time;
			CLEAR_FLAGS(result->flags);
			result->citations = list_create(result, NULL, NULL, remove_citation);
			if(result->citations == NULL)
			{
				MDEPOSIT(result);
				result = NULL;
			}
		}
	}

	return result;
}

/******************************************************************************
 *
 * \par Function Name:
 *      free_neighbor
 *
 * \brief  Deallocate memory for Neighbor type
 *
 *
 * \par Date Written:
 *      28/04/20
 *
 * \return void
 *
 *
 * \param[in]  data   The neighbor to deallocate
 *
 * \par Revision History:
 *
 *  DD/MM/YY  AUTHOR            DESCRIPTION
 *  --------  ---------------  -----------------------------------------------
 *  28/04/20  L. Persampieri    Initial Implementation and documentation.
 *****************************************************************************/
static void free_neighbor(void *data)
{
	Neighbor *neighbor;

	if(data != NULL)
	{
		neighbor = (Neighbor *) data;

		free_list(neighbor->citations);

		memset(neighbor, 0 ,sizeof(Neighbor));
		MDEPOSIT(neighbor);
	}

	return;
}

/******************************************************************************
 *
 * \par Function Name:
 *      get_neighbor
 *
 * \brief  Get the local node's Neighbor with this same ipn node number
 *
 *
 * \par Date Written:
 *      28/04/20
 *
 * \return Neighbor*
 *
 * \retval  Neighbor*  The Neighbor found
 * \retval  NULL       Neighbor not found
 *
 *
 * \param[in]  node_number   The ipn number of the neighbor
 *
 * \par Revision History:
 *
 *  DD/MM/YY  AUTHOR            DESCRIPTION
 *  --------  ---------------  -----------------------------------------------
 *  28/04/20  L. Persampieri    Initial Implementation and documentation.
 *****************************************************************************/
Neighbor * get_neighbor(unsigned long long node_number)
{
	ListElt *elt;
	Neighbor *result = NULL;
	Neighbor *current;
	NeighborsSAP *sap = get_neighbors_sap(NULL);

	if(sap->local_node_neighbors!= NULL)
	{
		for(elt = sap->local_node_neighbors->first; elt != NULL && result == NULL; elt = elt->next)
		{
			if(elt->data != NULL)
			{
				current = (Neighbor *) elt->data;
				if(current->ipn_number == node_number)
				{
					result = current;
				}
			}
		}
	}

	return result;
}

/******************************************************************************
 *
 * \par Function Name:
 *      insert_citation_to_neighbor
 *
 * \brief  Insert the citation between destination's routing object and neighbor.
 *         Cross-reference citation (ListElt to ListElt)
 *
 *
 * \par Date Written:
 *      28/04/20
 *
 * \return int
 *
 * \retval   0   Success case: Citations correctly managed
 * \retval  -1   Neighbor not found
 * \retval  -2   MWITHDRAW error
 * \retval  -3   Arguments error
 *
 *
 * \param[in]  rtgObj               The destination's routing object where we store the citation
 *                                  to the neighbors that have a possible route to reach destination
 * \param[in]  neighbor_ipn_number  The neighbor's ipn node number
 *
 * \par Revision History:
 *
 *  DD/MM/YY  AUTHOR            DESCRIPTION
 *  --------  ---------------  -----------------------------------------------
 *  28/04/20  L. Persampieri    Initial Implementation and documentation.
 *****************************************************************************/
static int insert_citation_to_neighbor(RtgObject *rtgObj, unsigned long long neighbor_ipn_number)
{
	int result = -3;
	Neighbor *neighbor;
	ListElt *elt_destination, *elt_neighbor;

	if(rtgObj != NULL)
	{
		result = -1;
		neighbor = get_neighbor(neighbor_ipn_number);

		if(neighbor != NULL)
		{
			result = 0;
			elt_neighbor = list_insert_last(neighbor->citations, rtgObj);
			elt_destination = list_insert_last(rtgObj->citations, neighbor);
			if(elt_neighbor == NULL)
			{
				list_remove_elt(elt_destination);
				result = -2;
				verbose_debug_printf("MWITHDRAW error");
			}
			if(elt_destination == NULL)
			{
				list_remove_elt(elt_neighbor);
				result = -2;
				verbose_debug_printf("MWITHDRAW error");
			}

			if(result == 0)
			{
				// Cross reference
				elt_neighbor->data = elt_destination;
				elt_destination->data = elt_neighbor;
			}
		}
		else
		{
			verbose_debug_printf("Neighbor %llu not found...", neighbor_ipn_number);
		}
	}

	return result;
}

/******************************************************************************
 *
 * \par Function Name:
 *      is_node_in_destination_neighbors_list
 *
 * \brief  Check for the presence of the node into the neighbors list to reach destination
 *
 * \details  If the neighbors list to reach destination hasn't been builded we assume
 *           that it's the same of the neighbors list of the local node
 *
 *
 * \par Date Written:
 *      28/04/20
 *
 * \return int
 *
 * \retval   1   The node is into the neighbors list
 * \retval   0   The node isn't into the neighbors list
 *
 *
 * \param[in]  destination    The destination node
 * \param[in]         node    The node to search into the neighbors list
 *
 * \par Revision History:
 *
 *  DD/MM/YY  AUTHOR            DESCRIPTION
 *  --------  ---------------  -----------------------------------------------
 *  28/04/20  L. Persampieri    Initial Implementation and documentation.
 *****************************************************************************/
int is_node_in_destination_neighbors_list(Node *destination, unsigned long long node)
{
	int result = 0;
	ListElt *elt, *current;
	Neighbor *neighbor;
	List neighbors;
	RtgObject *rtgObj;

	if(destination != NULL && destination->routingObject != NULL &&
			destination->routingObject->citations != NULL)
	{
		rtgObj = destination->routingObject;
		neighbors = destination->routingObject->citations;

		if(NEIGHBORS_DISCOVERED(rtgObj))
		{
			for(elt = neighbors->first; elt != NULL && result == 0; elt = elt->next)
			{
				if(elt->data != NULL)
				{
					current = (ListElt *) elt->data;

					if(current->list != NULL && current->list->userData != NULL)
					{
						neighbor = (Neighbor *) current->list->userData;

						if(neighbor->ipn_number == node)
						{
							result = 1;
						}
					}
				}
			}
		}
		else if(get_neighbor(node) != NULL)
		{
			result = 1;
		}
	}

	return result;
}

/******************************************************************************
 *
 * \par Function Name:
 *      insert_neighbors_to_reach_destination
 *
 * \brief  Insert into the destination's neighbors list the references to the neighbors
 *         to reach this destination node.
 *
 *
 * \par Date Written:
 *      28/04/20
 *
 * \return int
 *
 * \retval  ">= 0"  The neighbors inserted into the list
 * \retval     -2   MWITHDRAW error
 * \retval     -3   Arguments error
 * \retval     -5   Local node's neighbors list not initialized
 *
 *
 * \param[in]  neighbors    The neighbors list to build the destination's neighbors list.
 *                          Each data in this list is an unsigned long long (the ipn number of the neighbor)
 * \param[in]  destination  The destination node where we store the citations to neighbors
 *                          usable to reach this node
 *
 * \par  Notes:
 *           1.  If we already discovered the neighbors for the destination
 *               we change the neighbors list only it it has a different length
 *               from the new neighbors list.
 *           2.  If this function is going to change the destination's neighbor list
 *               the first thing done is to clear the previous list
 *
 * \par Revision History:
 *
 *  DD/MM/YY  AUTHOR            DESCRIPTION
 *  --------  ---------------  -----------------------------------------------
 *  28/04/20  L. Persampieri    Initial Implementation and documentation.
 *****************************************************************************/
int insert_neighbors_to_reach_destination(List neighbors, Node *destination)
{
	int result = -3, stop = 0;
	int temp_result;
	ListElt *elt;
	unsigned long long *current;
	RtgObject *rtgObj;
	NeighborsSAP *sap = get_neighbors_sap(NULL);

	if(sap->local_node_neighbors == NULL)
	{
		result = -5;
	}
	else if(neighbors != NULL && destination != NULL && destination->routingObject != NULL
			&& destination->routingObject->citations != NULL)
	{
		rtgObj = destination->routingObject;
		if(NEIGHBORS_DISCOVERED(rtgObj) && (neighbors->length == rtgObj->citations->length))
		{
			// Same length, nothing to do
			result = 0;
		}
		else
		{
			debug_printf("Discovered new total neighbors number (%lu) to reach destination %llu. Previous total number (%lu)", neighbors->length, destination->nodeNbr, rtgObj->citations->length);
			free_list_elts(rtgObj->citations); //remove previous citations
			result = 0;
			for(elt = neighbors->first; elt != NULL && !stop; elt = elt->next)
			{
				current = (unsigned long long *) elt->data;
				if(current != NULL)
				{
					temp_result = insert_citation_to_neighbor(destination->routingObject, *current);
					if(temp_result == -2)
					{
						verbose_debug_printf("MWITHDRAW error");
						stop = 1;
						result = -2;
					}
					else if(temp_result < 0)
					{
						// go on
						// Neighbor not found
						verbose_debug_printf("Error...");
					}
					else
					{
						result++;
					}
				}
			}

			if(result >= 0)
			{
				SET_NEIGHBORS_DISCOVERED(rtgObj);
			}
		}
	}

	return result;
}

/******************************************************************************
 *
 * \par Function Name:
 *      add_neighbor
 *
 * \brief  Insert the neighbor into the local node's neighbors list
 *
 *
 * \par Date Written:
 *      28/04/20
 *
 * \return int
 *
 * \retval   0   Success case: Citations correctly managed
 * \retval  -1   Neighbor not found
 * \retval  -2   MWITHDRAW error
 * \retval  -3   Arguments error
 *
 *
 * \param[in]  node_number   The ipn number of the neighbor
 * \param[in]      to_time   The time when the last contact from the local node
 *                           to this neighbor expires.
 *
 * \par Revision History:
 *
 *  DD/MM/YY  AUTHOR            DESCRIPTION
 *  --------  ---------------  -----------------------------------------------
 *  28/04/20  L. Persampieri    Initial Implementation and documentation.
 *****************************************************************************/
static int add_neighbor(unsigned long long node_number, time_t to_time)
{
	int result = -1;
	Neighbor *neighbor;
	NeighborsSAP *sap = get_neighbors_sap(NULL);

	if(node_number <= 0 || to_time < 0 || sap->local_node_neighbors == NULL)
	{
		return -3;
	}

	neighbor = get_neighbor(node_number);
	if (neighbor != NULL) {
	    if (neighbor->toTime < to_time) {
	        neighbor->toTime = to_time;
	    }

	    result = 0;
	}
	else
	{
		//if neighbor not found
		neighbor = create_neighbor(node_number, to_time);
		if(neighbor != NULL)
		{
			if(list_insert_last(sap->local_node_neighbors, neighbor) != NULL)
			{
				result = 0;
				if(to_time < sap->timeNeighborToRemove)
				{
					sap->timeNeighborToRemove = to_time;
				}
			}
			else
			{
				result = -2;
				verbose_debug_printf("MWITHDRAW error");
			}
		}
		else
		{
			verbose_debug_printf("Can't create neighbor %llu (toTime: %ld)", node_number, (long int) to_time);
		}
	}

	return result;
}

/******************************************************************************
 *
 * \par Function Name:
 *      get_local_node_neighbors_count
 *
 * \brief  Get the number of the still *alive* local node's neighbors
 *
 *
 * \par Date Written:
 *      28/04/20
 *
 * \return long unsigned int
 *
 * \retval   ">= 0"  The number of the still *alive* local node's neighbors
 *
 *
 * \par Revision History:
 *
 *  DD/MM/YY  AUTHOR            DESCRIPTION
 *  --------  ---------------  -----------------------------------------------
 *  28/04/20  L. Persampieri    Initial Implementation and documentation.
 *****************************************************************************/
long unsigned int get_local_node_neighbors_count()
{
	unsigned long int result = 0;
	NeighborsSAP *sap = get_neighbors_sap(NULL);

	if(sap->local_node_neighbors != NULL)
	{
		result = sap->local_node_neighbors->length;
	}

	return result;
}

/******************************************************************************
 *
 * \par Function Name:
 *      reset_neighbors_temporary_fields
 *
 * \brief   Reset the neighbors fields used during the CGR's call
 *
 *
 * \par Date Written:
 *      28/04/20
 *
 * \return void
 *
 *
 * \par Revision History:
 *
 *  DD/MM/YY  AUTHOR            DESCRIPTION
 *  --------  ---------------  -----------------------------------------------
 *  28/04/20  L. Persampieri    Initial Implementation and documentation.
 *****************************************************************************/
void reset_neighbors_temporary_fields()
{
	ListElt *elt;
	Neighbor *current;
	NeighborsSAP *sap = get_neighbors_sap(NULL);

	if(sap->local_node_neighbors != NULL)
	{
		for(elt = sap->local_node_neighbors->first; elt != NULL; elt = elt->next)
		{
			if(elt->data != NULL)
			{
				current = (Neighbor *) elt->data;
				CLEAR_FLAGS(current->flags);
			}
		}
	}
}

/******************************************************************************
 *
 * \par Function Name:
 *      removeOldNeighbors
 *
 * \brief  Remove the *expired* local node's neighbors
 *
 *
 * \par Date Written:
 *      28/04/20
 *
 * \return void
 *
 *
 * \param[in]  current_time   The current time, used to know who are the neighbors expired
 *
 * \par Revision History:
 *
 *  DD/MM/YY  AUTHOR            DESCRIPTION
 *  --------  ---------------  -----------------------------------------------
 *  28/04/20  L. Persampieri    Initial Implementation and documentation.
 *****************************************************************************/
void removeOldNeighbors(time_t current_time)
{
	ListElt *elt, *next;
	Neighbor *current;
	NeighborsSAP *sap = get_neighbors_sap(NULL);

	if(current_time >= sap->timeNeighborToRemove && sap->local_node_neighbors != NULL)
	{
		sap->timeNeighborToRemove = MAX_POSIX_TIME;
		elt = sap->local_node_neighbors->first;
		while(elt != NULL)
		{
			current = (Neighbor*) elt->data;
			next = elt->next;
			if(current->toTime <= current_time)
			{
				debug_printf("Deleted neighbor %llu...", current->ipn_number);
				list_remove_elt(elt); //remove the citations to destination node
			}
			else if(current->toTime < sap->timeNeighborToRemove)
			{
				sap->timeNeighborToRemove = current->toTime;
			}

			elt = next;
		}
	}
}

/******************************************************************************
 *
 * \par Function Name:
 *      build_local_node_neighbors_list
 *
 * \brief  Build from the contacts graph the local node's neighbors list
 *
 *
 * \par Date Written:
 *      28/04/20
 *
 * \return int
 *
 * \retval   1  Success case: neighbors list builded correctly
 * \retval   0  The neighbors list has been already builded
 * \retval  -2  MWITHDRAW error
 *
 *
 * \param[in]  localNode   The local node
 *
 * \par Revision History:
 *
 *  DD/MM/YY  AUTHOR            DESCRIPTION
 *  --------  ---------------  -----------------------------------------------
 *  28/04/20  L. Persampieri    Initial Implementation and documentation.
 *****************************************************************************/
int build_local_node_neighbors_list(unsigned long long localNode)
{
	int result = 0, stop = 0;
	Contact *contact, *prevContact = NULL;
	RbtNode *node;
	NeighborsSAP *sap = get_neighbors_sap(NULL);

	if(!sap->neighbors_list_builded)
	{
		debug_printf("Building local node's neighbors list...");

		sap->neighbors_list_builded = 1;
		free_list(sap->local_node_neighbors);
		sap->local_node_neighbors = list_create(NULL, NULL, NULL, free_neighbor);
		if(sap->local_node_neighbors != NULL)
		{

		    List regions = get_known_regions();

		    if (regions == NULL) {
		        result = -2;
                verbose_debug_printf("MWITHDRAW error.");
                return result;
		    }

		    ListElt * elt;

		    for ( elt = list_get_first_elt(regions) ; elt != NULL ; elt = list_get_next_elt(elt))
		    {
		        unsigned long * currentRegion =  elt->data;

		        if ( currentRegion == NULL) {
		            continue;
		        }

		        prevContact = NULL;
		        stop = 0;

                for(contact = get_first_contact_from_node(*currentRegion, localNode, &node); contact != NULL && !stop; contact = get_next_contact(&node))
                {
                    if(prevContact != NULL)
                    {
                        if(contact->regionNbr != *currentRegion || contact->fromNode != localNode || contact->toNode != prevContact->toNode)
                        {
                            if(contact->regionNbr != *currentRegion || contact->fromNode != localNode)
                            {
                                // Found all neighbors in this region.
                                stop = 1;
                            }

                            if(add_neighbor(prevContact->toNode, prevContact->toTime) == -2)
                            {
                                // MWITHDRAW error
                                stop = 1;
                                result = -2;
                                verbose_debug_printf("MWITHDRAW error.");

                            }
                        }
                    }

                    prevContact = contact;
                }

                if(!stop && prevContact != NULL && prevContact->fromNode == localNode)
                {
                    //Last contact in the graph
                    if(add_neighbor(prevContact->toNode, prevContact->toTime) == -2)
                    {
                        verbose_debug_printf("MWITHDRAW error.");
                        result = -2;
                    }
                }

                if (result == -2) { /* unrecoverable error */
                    free_list(regions);
                    return result;
                }

		    }

            free_list(regions);

			debug_printf("Found %lu neighbors.", sap->local_node_neighbors->length);
		}
		else
		{
			result = -2;
			verbose_debug_printf("MWITHDRAW error.");
		}

	}

	return result;
}
