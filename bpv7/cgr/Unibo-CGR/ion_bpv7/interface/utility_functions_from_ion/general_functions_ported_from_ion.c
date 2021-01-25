/***************************************************************
 * \file general_functions_ported_from_ion.c
 * 
 * \brief  In this file there are the implementations of some functions
 *         used by the CGR's interface for ION.
 * 
 * \par Ported from ION 3.7.0 by
 *      Lorenzo Persampieri, lorenzo.persampieri@studio.unibo.it
 * 
 * \par Supervisor
 *      Carlo Caini, carlo.caini@unibo.it
 *
 * \par Date
 *      16/02/20
 ***************************************************************/

#include "general_functions_ported_from_ion.h"

#include <stdlib.h>

/******************************************************************************
 *
 * \par Function Name:
 *      removeRoute
 *
 * \brief  Deallocate a route and remove all references to the route.
 *
 *
 * \par Date Written:
 *      XX/XX/XX
 *
 * \return 	void
 *
 * \param[in]		ionwm     The partition of the ION's contacts graph
 * \param[in]		routeElt  The address of the route to remove
 *
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  16/02/20 | L. Persampieri  |  Ported function from ION 3.7.0
 *****************************************************************************/
void removeRoute(PsmPartition ionwm, PsmAddress routeElt)
{
	/* from bp/cgr/libcgr.c */

	PsmAddress routeAddr;
	CgrRoute *route;
	PsmAddress citation;
	PsmAddress nextCitation;
	PsmAddress contactAddr;
	IonCXref *contact;
	PsmAddress citationElt;
	int found = 0;

	routeAddr = sm_list_data(ionwm, routeElt);
	route = (CgrRoute*) psp(ionwm, routeAddr);
	if (route->referenceElt)
	{
		sm_list_delete(ionwm, route->referenceElt, NULL, NULL);
	}

	if (routeElt != route->referenceElt)
	{
		sm_list_delete(ionwm, routeElt, NULL, NULL);
	}

	/*	Each member of the "hops" list of this route is one
	 *	among possibly many citations of some specific contact,
	 *	i.e., its content is the address of that contact.
	 *	For many-to-many cross-referencing, we append the
	 *	address of each citation - that is, the address of
	 *	the list element that cites some contact - to the
	 *	cited contact's list of citations; the content of
	 *	each member of a contact's citations list is the
	 *	address of one among possibly many citations of that
	 *	contact, each of which is a member of some route's
	 *	list of hops.
	 *
	 *	When a route is removed, we must detach the route
	 *	from every contact that is cited in one of that
	 *	route's hops.  That is, for each hop of the route,
	 *	we must go through the citations list of the contact
	 *	cited by that hop and remove from that list the list
	 *	member whose content is the address of this citation
	 *	- the address of this "hops" list element.		*/

	if (route->hops)
	{
		for (citation = sm_list_first(ionwm, route->hops); citation; citation = nextCitation)
		{
			nextCitation = sm_list_next(ionwm, citation);
			contactAddr = sm_list_data(ionwm, citation);
			if (contactAddr == 0)
			{
				/*	This contact has been deleted;
				 *	all references to it have been
				 *	zeroed out.			*/

				sm_list_delete(ionwm, citation, NULL, NULL);
			}
			else
			{
				contact = (IonCXref*) psp(ionwm, contactAddr);
				if (contact->citations)
				{
					found = 0;
					for (citationElt = sm_list_first(ionwm, contact->citations);
							citationElt && !found; citationElt = sm_list_next(ionwm, citationElt))
					{
						/*	Does this list element
						 *	point at this route's
						 *	citation of the contact?
						 *	If so, delete it.	*/

						if (sm_list_data(ionwm, citationElt) == citation)
						{
							sm_list_delete(ionwm, citationElt, NULL, NULL);
							found = 1;
						}
					}
				}

				sm_list_delete(ionwm, citation, NULL, NULL);
			}
		}

		sm_list_destroy(ionwm, route->hops, NULL, NULL);
	}

	psm_free(ionwm, routeAddr);
}

/******************************************************************************
 *
 * \par Function Name:
 *      create_ion_node_routing_object
 *
 * \brief Create a routing object for an IonNode tyoe
 *
 *
 * \par Date Written:
 *      XX/XX/XX
 *
 * \return int

 * \retval   0   Success case
 * \retval  -1   Error case
 *
 * \param[in]		*terminusNode   The IonNode for which we want to create the routing object
 * \param[in]		ionwm           The ION's memory partition
 * \param[in]		*cgrvdb         The ION's CGR virtual database
 *
 * \warning terminusNode doesn't have to be NULL.
 * \warning cgrvdb doesn't have to be NULL.
 * 
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  16/02/20 | L. Persampieri  |  Ported function from ION 3.7.0
 *****************************************************************************/
int create_ion_node_routing_object(IonNode *terminusNode, PsmPartition ionwm, CgrVdb *cgrvdb)
{
	int result = 0;
	CgrRtgObject *routingObj = (CgrRtgObject*) psp(ionwm, terminusNode->routingObject);
	CHKERR(routingObj);
	if (routingObj->selectedRoutes == 0)
	{
		/*	Must initialize routing object for CGR.		*/

		routingObj->selectedRoutes = sm_list_create(ionwm);
		if (routingObj->selectedRoutes == 0)
		{
			result = -1;
		}
		else
		{
			routingObj->knownRoutes = sm_list_create(ionwm);
			if (routingObj->knownRoutes == 0)
			{
				result = -1;
			}
			else if (sm_list_insert_last(ionwm, cgrvdb->routingObjects,
					terminusNode->routingObject) == 0)
			{
				return -1;
			}
		}
	}

	return result;

}

