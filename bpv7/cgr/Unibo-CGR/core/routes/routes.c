/** \file routes.c
 *	
 *  \brief  This file provides the implementations of the functions to create
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
 *       Carlo Caini, carlo.caini@unibo.it
 */

#include "routes.h"

#include <stdlib.h>
#include "../contact_plan/contacts/contacts.h"
#include "../contact_plan/nodes/nodes.h"
#include "../library/list/list.h"

/******************************************************************************
 *
 * \par Function Name:
 *      erase_cgr_route
 *
 * \brief Remove from the ranges graph all ranges where Range's toTime field is less than
 *        time passed as argument.
 *
 *
 * \par Date Written:
 *      21/01/20
 *
 * \return void
 *
 * \param[in]	*route	The Route for which we want to reset all fields
 *
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  21/01/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
static void erase_cgr_route(Route *route)
{
	memset(route, 0, sizeof(Route));
}

/******************************************************************************
 *
 * \par Function Name:
 * 		remove_citation
 *
 * \brief Remove the citation "target" from the citations list of the contact
 *
 *
 * \par Date Written:
 * 		21/01/20
 *
 * \return void
 *
 * \param[in]	*contact		The Contact for which we want to delete the citation "target"
 * \param[in]	*target			The citation to remove from the contact
 *
 * \warning We have to remove only the citation from the contact: no chain events
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  21/01/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
static void remove_citation(Contact *contact, ListElt *target)
{
	ListElt *current;
	int deleted = 0;

	if (contact != NULL && target != NULL)
	{
		if (contact->citations != NULL)
		{
			current = contact->citations->first;
			while (current != NULL)
			{
				if (current->data == target) //if they point to the same address
				{
					list_remove_elt(current);
					current = NULL; //I leave the loop
					deleted = 1;
				}
				else
				{
					current = current->next;
				}
			}
		}
	}

	if (deleted == 0)
	{
		flush_verbose_debug_printf("Error!!!");
	}

	return;
}

/******************************************************************************
 *
 * \par Function Name:
 * 		remove_citation
 *
 * \brief Remove the son's citation to the father
 *
 * \details Set to NULL the son's citation to the father
 *
 *
 * \par Date Written:
 * 		13/05/20
 *
 * \return void
 *
 * \param[in]	data   The son
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  13/05/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
static void remove_reference_from_son(void *data)
{
	Route *son;
	if(data != NULL)
	{
		son = data;
		son->citationToFather = NULL;
	}

	return;
}

/******************************************************************************
 *
 * \par Function Name:
 * 		update_references
 *
 * \brief Manage all the references between this route and other related
 * 		  routes (father, children, selectedFather, selectedChild)
 *
 *
 * \par Date Written:
 * 		21/01/20
 *
 * \return void
 *
 * \param[in]	*route		The Route that we want to be "forgotten" by all its children, father
 * 							selectedFather and selectedChild
 *
 * \par Notes:
 * 			1. The children of this route at the end will have the father field setted to NULL.
 * 			2. The father of this route at the end will not see anymore this route in the children list.
 * 			3. The selectedFather of this route at the end will acquire as selectedChild the selectedChild of this route.
 *			4. The selectedChild of this route at the end will acquire as selectedFather the selectedFather of this route.
 *			5. If this route has a selectedChild field setted to NULL we set to 0 the spursComputed field
 *			   of the selectedFather.
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  21/01/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
static void update_references(Route *route)
{
	//destroy children list, all son->father will be setted to NULL by remove_reference_from_son()
	free_list(route->children);
	route->children = NULL;

	// manage the selectedFather-selectedChild to optimize Yen's algorithm
	if (route->selectedFather != NULL)
	{
		route->selectedFather->selectedChild = route->selectedChild;
		if (route->selectedFather->selectedChild == NULL)
		{
			route->selectedFather->spursComputed = 0;
		}
	}
	if (route->selectedChild != NULL)
	{
		route->selectedChild->selectedFather = route->selectedFather;
	}

	// remove the citation to this son from the father's children list
	// the route->citationFather is setted to NULL by the remove_reference_from_son()
	if (route->citationToFather != NULL)
	{
		list_remove_elt(route->citationToFather);
	}

	return;
}

/******************************************************************************
 *
 * \par Function Name:
 * 		delete_cgr_route
 *
 * \brief Delete the route passed as argument and manage all its references with other
 * 		  routes, with all the contacts in the hops list and with the list
 * 		  where it was putted in (referenceElt).
 *
 *
 * \par Date Written:
 *		21/01/20
 *
 * \return void
 *
 * \param[in]	*data		The Route that we want to delete
 *
 * \par Notes:
 * 			1. All contacts in the hops list at the end will not have anymore any reference
 * 			   with this route.
 * 			2. The route will be removed also from the list where it was putted in (referenceElt field)
 * 			3. For the other references with other routes see the update_references function
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  21/01/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
void delete_cgr_route(void *data)
{
	ListElt *elt, *nextElt;
	Contact *contact;
	Route *route;
	delete_function temp;
	List list;
	if (data != NULL)
	{
		route = (Route*) data;

		if (route->hops != NULL)
		{
			elt = route->hops->first;
			while (elt != NULL)
			{
				nextElt = elt->next;
				contact = elt->data;
				remove_citation(contact, elt); //delete citation from the contact
				MDEPOSIT(elt);
				elt = nextElt;
			}

			MDEPOSIT(route->hops);
		}

		update_references(route); //manage the references with other routes

		if (route->referenceElt != NULL)
		{
			list = route->referenceElt->list;
			if (list != NULL)
			{
				temp = list->delete_data_elt;
				list->delete_data_elt = NULL; //It was setted with this function, so I set
											  //it to NULL to avoid chain events

				list_remove_elt(route->referenceElt); //delete the route from the list where it was putted in

				list->delete_data_elt = temp; //Reset the delete_function properly at the default value
			}

		}
		erase_cgr_route(route);
		MDEPOSIT(route);
	}
}

/******************************************************************************
 *
 * \par Function Name:
 * 		clear_routes_list
 *
 * \brief Delete all the routes in a list
 *
 *
 * \par Date Written:
 * 		03/04/20
 *
 * \return void
 *
 * \param[in]	*routes  The list of routes that we want to clear
 *
 * \par Notes:
 * 			1. The free_list_elts can't be used to delete a list of Route, due to the
 * 			   delete_cgr_route setted as delete_function that remove the element from the list.
 * 			2. All references will be managed by the delete_cgr_route called by this function
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  03/04/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
void clear_routes_list(List routes)
{
	ListElt *elt, *next;

	if (routes != NULL)
	{
		elt = routes->first;
		while (elt != NULL)
		{
			next = elt->next;
			if (elt->data != NULL)
			{
				delete_cgr_route(elt->data);
			}
			else
			{
				flush_verbose_debug_printf("Error: route NULL!!!");
				list_remove_elt(elt);
			}
			elt = next;
		}

		routes->first = NULL;
		routes->last = NULL;
		routes->length = 0;
	}

	return;
}
/******************************************************************************
 *
 * \par Function Name:
 * 		destroy_routes_list
 *
 * \brief Delete all the routes in a list and the list itself
 *
 *
 * \par Date Written:
 * 		21/01/20
 *
 * \return void
 *
 * \param[in]	*routes  The list of routes that we want to delete
 *
 * \par Notes:
 * 			1. The free_list can't be used to delete a list of Route, due to the
 * 			   delete_cgr_route setted as delete_function that remove the element from the list.
 * 			2. All references will be managed by the delete_cgr_route called by this function
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  21/01/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
void destroy_routes_list(List routes)
{
	clear_routes_list(routes);
	MDEPOSIT(routes);

	return;
}

/******************************************************************************
 *
 * \par Function Name:
 * 		create_cgr_route
 *
 * \brief Allocate memory for a Route type
 *
 *
 * \par Date Written:
 * 		21/01/20
 *
 * \return Route*
 *
 * \retval Route*  The new allocated Route
 * \retval NULL    MWITHDRAW error
 *
 *
 * \par Notes:
 * 			1. You must check that the return value of this function is not NULL.
 * 			2. The hops list will be allocated
 * 			3. The children list will be allocated
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  21/01/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
Route* create_cgr_route()
{
	Route *result;
	result = (Route*) MWITHDRAW(sizeof(Route));
	if (result != NULL)
	{
		erase_cgr_route(result);
		result->hops = list_create(result, NULL, NULL, NULL);
		result->children = list_create(result, NULL, NULL, remove_reference_from_son);

		if (result->hops == NULL || result->children == NULL)
		{
			MDEPOSIT(result->hops);
			MDEPOSIT(result->children);
			MDEPOSIT(result);
			result = NULL;
		}
	}

	return result;
}

/******************************************************************************
 *
 * \par Function Name:
 * 		move_route_from_known_to_selected
 *
 * \brief Move the route from the knownRoutes to the selectedRoutes as first element.
 *
 *
 * \par Date Written:
 * 		21/01/20
 *
 * \return int
 *
 * \retval   0		Success case: the Route now is in the selectedRoutes
 * \retval  -1		Error case: Route is not in knownRoutes
 *
 * \param[in]	*route	The route that we want to move from the knownRoutes to the selectedRoutes
 *
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  21/01/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
int move_route_from_known_to_selected(Route *route)
{

	int result = -1;
	RtgObject *rtgObj;
	ListElt *elt_in_known;
	List knownRoutes;

	if (route != NULL)
	{
		elt_in_known = route->referenceElt;
		if (elt_in_known != NULL)
		{
			knownRoutes = elt_in_known->list;
			if (knownRoutes != NULL)
			{
				rtgObj = (RtgObject*) knownRoutes->userData;
				if (rtgObj != NULL)
				{
					if (knownRoutes == rtgObj->knownRoutes)
					{
						result = move_elt_to_other_list(route->referenceElt,
								rtgObj->selectedRoutes);
					}
				}
			}
		}
	}

	return result;
}

/******************************************************************************
 *
 * \par Function Name:
 * 		insert_selected_route
 *
 * \brief Insert the route in the selectedRoutes list as first element
 *
 *
 * \par Date Written:
 * 		21/01/20
 *
 * \return int
 *
 * \retval   0		Success case: the Route now is in the selectedRoutes as first element
 * \retval  -1		Error case: rtgObj NULL or route NULL
 * \retval  -2		MWITHDRAW error
 *
 * \param[in]	*rtgObj		The RtgObject that contains the selectedRoutes where we want to put the route
 * \param[in]	*route		The route that we want to put into the selectedRoutes as first element
 *
 * \par Notes:
 * 			1.	The Route's referenceElt will be setted
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  21/01/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
int insert_selected_route(RtgObject *rtgObj, Route *route)
{
	int result = -1;
	ListElt *elt;
	if (rtgObj != NULL && route != NULL)
	{
		elt = list_insert_first(rtgObj->selectedRoutes, route);
		if (elt != NULL)
		{
			route->referenceElt = elt;
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
 * 		insert_known_route
 *
 * \brief Insert the route in the knownRoutes list as first element
 *
 *
 * \par Date Written:
 * 		21/01/20
 *
 * \return int
 *
 * \retval   0		Success case: the Route now is in the knownRoutes as first element
 * \retval  -1		Error case: rtgObj NULL or route NULL
 * \retval  -2		MWITHDRAW error
 *
 * \param[in]	*rtgObj		The RtgObject that contains the knownRoutes where we want to put the route
 * \param[in]	*route		The route that we want to put into the knownRoutes as first element
 *
 * \par Notes:
 * 			1.	The Route's referenceElt will be setted
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  21/01/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
int insert_known_route(RtgObject *rtgObj, Route *route)
{
	int result = -1;
	ListElt *elt;
	if (rtgObj != NULL && route != NULL)
	{
		elt = list_insert_first(rtgObj->knownRoutes, route);
		if (elt != NULL)
		{
			route->referenceElt = elt;
			result = 0;
		}
		else
		{
			result = -2;
		}
	}

	return result;
}

Route * get_route_father(Route *son)
{
	Route *father = NULL;
	if(son != NULL && son->citationToFather != NULL && son->citationToFather->list != NULL)
	{
		father = son->citationToFather->list->userData;
	}

	return father;
}
