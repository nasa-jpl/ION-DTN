/** \file contactPlan.c
 *
 *  \brief This file provides the implementation of the functions
 *         to manage the contact plan
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

#include "contactPlan.h"

#include "contacts/contacts.h"
#include "nodes/nodes.h"
#include "ranges/ranges.h"


/******************************************************************************
 *
 * \par Function Name:
 *      get_contact_plan_sap
 *
 * \brief Get the values stored into ContactPlanSAP
 *
 *
 * \par Date Written:
 *      02/07/20
 *
 * \return ContactPlanSAP
 *
 * \retval  ContactPlanSAP   A copy of the ContactPlanSAP.
 *
 * \param[in] *newSap     Set to NULL if you just want a copy of the ContactPlanSAP.
 *                        Otherwise, set != NULL and the ContactPlanSAP will be overwritten,
 *                        but this is discouraged outside from contactPlan.c.
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  02/07/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
ContactPlanSAP get_contact_plan_sap(ContactPlanSAP *newSap) {
	static ContactPlanSAP sap;

	if(newSap != NULL) {
		sap = *newSap;
	}

	return sap;
}


/******************************************************************************
 *
 * \par Function Name:
 *      get_contact_plan_sap
 *
 * \brief Set the last time when contact plan has been modified
 *
 *
 * \par Date Written:
 *      02/07/20
 *
 * \return void
 *
 * \param[in]  seconds        The second when the contact plan has been modified.
 * \param[in]  micro_seconds  The microsecond (referred to second) when the contact plan has been modified
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  02/07/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
void set_time_contact_plan_updated(__time_t seconds, __suseconds_t micro_seconds) {
	ContactPlanSAP sap = get_contact_plan_sap(NULL);

	sap.contactPlanEditTime.tv_sec = seconds;
	sap.contactPlanEditTime.tv_usec = micro_seconds;

	get_contact_plan_sap(&sap);

}

/******************************************************************************
 *
 * \par Function Name:
 *      initialize_contact_plan
 *
 * \brief Initialize the structures to manage the contact plan
 *
 *
 * \par Date Written:
 *      23/01/20
 *
 * \return int
 *
 * \retval   1   Success case:	Contacts graph, ranges graph and nodes tree initialized correctly
 * \retval  -2   Error case:  MWITHDRAW error
 *
 *
 * \par Revision History:
 *
 *  DD/MM/YY | AUTHOR         |   DESCRIPTION
 *  -------- | ---------------|  -----------------------------------------------
 *  23/01/20 | L. Persampieri |   Initial Implementation and documentation.
 *****************************************************************************/
int initialize_contact_plan()
{
	int result = 1;
	ContactPlanSAP sap = get_contact_plan_sap(NULL);

	if (sap.contactsGraph == 0)
	{
		if (create_ContactsGraph() == 1)
		{
			sap.contactsGraph = 1;
		}
		else
		{
			result = -2;
		}
	}
	if (sap.rangesGraph == 0)
	{
		if (create_RangesGraph() == 1)
		{
			sap.rangesGraph = 1;
		}
		else
		{
			result = -2;
		}
	}
	if (sap.nodes == 0)
	{
		if (create_NodesTree() == 1)
		{
			sap.nodes = 1;
		}
		else
		{
			result = -2;
		}
	}

	if (result == 1)
	{
		sap.initialized = 1;
	}

	sap.contactPlanEditTime.tv_sec = -1;
	sap.contactPlanEditTime.tv_usec = -1;

	get_contact_plan_sap(&sap); // save

	return result;
}

/******************************************************************************
 *
 * \par Function Name:
 *      removeExpired
 *
 * \brief Remove the expired contacts ,ranges (toTime field less than time)
 *        and neighbors to whom whe haven't other contacts.
 *
 *
 * \par Date Written:
 *      23/01/20
 *
 * \return void
 *
 * \param[in]	time   The time used to know who are the expired contacts, ranges and "neighbors"
 *
 *
 * \par Revision History:
 *
 *  DD/MM/YY | AUTHOR         |   DESCRIPTION
 *  -------- | ---------------|  -----------------------------------------------
 *  23/01/20 | L. Persampieri |   Initial Implementation and documentation.
 *****************************************************************************/
void removeExpired(time_t time)
{
	ContactPlanSAP sap = get_contact_plan_sap(NULL);
	if (sap.initialized)
	{
		removeExpiredContacts(time);
		removeExpiredRanges(time);
		removeOldNeighbors(time);
	}
}

/******************************************************************************
 *
 * \par Function Name:
 *      addContact
 *
 * \brief Add a contact to the contacts graph
 *
 *
 * \par Date Written:
 *      23/01/20
 *
 * \return int
 *
 * \retval   2  Success case:   Revised contact's xmit rate (and MTVs)
 * \retval   1  Success case:	The contact now is inside the contacts graph
 * \retval   0  Arguments error case:	The contact cannot be inserted with this arguments
 * \retval  -1  The contact cannot be inserted because it overlaps some other contacts.
 * \retval  -2  MWITHDRAW error
 * \retval  -3  Precondition error, you must call initialize_contact_plan() before!
 *
 * \param[in]	fromNode    The contact's sender node
 * \param[in]	toNode      The contact's receiver node
 * \param[in]	fromTime    The contact's start time
 * \param[in]	toTime      The contact's end time
 * \param[in]	xmitRate    In bytes per second
 * \param[in]	confidence  The confidence that the contact will effectively materialize
 * \param[in]   copyMTV      Set to 1 if you want to copy the MTV from mtv input parameter.
 *                           Set to 0 otherwise
 * \param[in]   mtv          The contact's MTV: this must be an array of 3 elements.
 *
 *
 * \par Revision History:
 *
 *  DD/MM/YY | AUTHOR         |   DESCRIPTION
 *  -------- | ---------------|  -----------------------------------------------
 *  23/01/20 | L. Persampieri |   Initial Implementation and documentation.
 *****************************************************************************/
int addContact(unsigned long regionNbr, unsigned long long fromNode, unsigned long long toNode, time_t fromTime,
		time_t toTime, long unsigned int xmitRate, float confidence, int copyMTV, double mtv[])
{
	int result = 0;
	ContactPlanSAP sap = get_contact_plan_sap(NULL);

	if (sap.initialized)
	{
		result = add_contact_to_graph(regionNbr, fromNode, toNode, fromTime, toTime, xmitRate, confidence, copyMTV, mtv);
	}

	return result;
}

/******************************************************************************
 *
 * \par Function Name:
 *      removeContact
 *
 * \brief Remove a contact from the contacts graph
 *
 *
 * \par Date Written:
 *      23/01/20
 *
 * \return int
 *
 * \retval   1  If the contact plan is initialized
 * \retval   0  If the contact plan isn't initialized
 *
 * \param[in]   fromNode    The contact's sender node
 * \param[in]   toNode      The contact's receiver node
 * \param[in]   *fromTime   The contact's start time, if NULL all contacts with fields
 *                          {fromNode, toNode} will be deleted
 *
 * \par Notes:
 *          1. The free_contact will be called for the contact(s) removed (routes discarded).
 *
 * \par Revision History:
 *
 *  DD/MM/YY | AUTHOR         |   DESCRIPTION
 *  -------- | ---------------|  -----------------------------------------------
 *  23/01/20 | L. Persampieri |   Initial Implementation and documentation.
 *****************************************************************************/
int removeContact(unsigned long regionNbr, unsigned long long fromNode, unsigned long long toNode, time_t *fromTime)
{
	int result = 0;
	ContactPlanSAP sap = get_contact_plan_sap(NULL);

	if (sap.initialized)
	{
		remove_contact_from_graph(regionNbr, fromTime, fromNode, toNode);
		result = 1;
	}

	return result;
}

/******************************************************************************
 *
 * \par Function Name:
 *      addRange
 *
 * \brief Add a range to the ranges graph
 *
 *
 * \par Date Written:
 *      23/01/20
 *
 * \return int
 *
 * \retval   1   Success case:	The range now is inside the contacts graph
 * \retval   0   Arguments error case:	The range cannot be inserted with this arguments
 * \retval  -1   The range cannot be inserted because it overlaps some other ranges.
 * \retval  -2   MWITHDRAW error
 * \retval  -3   Precondition error, you must call initialize_contact_plan() before!
 *
 * \param[in]	fromNode   The contact's sender node
 * \param[in]	toNode     The contact's receiver node
 * \param[in]	fromTime   The contact's start time
 * \param[in]	toTime     The contact's end time
 * \param[in]	owlt       The distance from the sender node to the receiver node in light time
 *
 *
 * \par Revision History:
 *
 *  DD/MM/YY | AUTHOR         |   DESCRIPTION
 *  -------- | ---------------|  -----------------------------------------------
 *  23/01/20 | L. Persampieri |   Initial Implementation and documentation.
 *****************************************************************************/
int addRange(unsigned long long fromNode, unsigned long long toNode, time_t fromTime, time_t toTime,
		unsigned int owlt)
{
	int result = -3;
	ContactPlanSAP sap = get_contact_plan_sap(NULL);

	if (sap.initialized)
	{
		result = add_range_to_graph(fromNode, toNode, fromTime, toTime, owlt);

	}

	return result;
}

/******************************************************************************
 *
 * \par Function Name:
 *      removeRange
 *
 * \brief Remove a range from the ranges graph
 *
 *
 * \par Date Written:
 *      23/01/20
 *
 * \return int
 *
 * \retval   1   If the contact plan is initialized
 * \retval   0   If the contact plan isn't initialized
 *
 * \param[in]	fromNode   The range's sender node
 * \param[in]	toNode     The range's receiver node
 * \param[in]	*fromTime  The range's start time, if NULL all ranges with fields
 *                         {fromNode, toNode} will be deleted
 *
 * \par Notes:
 *           1. The free_range will be called for the range(s) removed.
 *
 * \par Revision History:
 *
 *  DD/MM/YY | AUTHOR         |   DESCRIPTION
 *  -------- | ---------------|  -----------------------------------------------
 *  23/01/20 | L. Persampieri |   Initial Implementation and documentation.
 *****************************************************************************/
int removeRange(unsigned long long fromNode, unsigned long long toNode, time_t *fromTime)
{
	int result = 0;
	ContactPlanSAP sap = get_contact_plan_sap(NULL);

	if (sap.initialized)
	{
		remove_range_from_graph(fromTime, fromNode, toNode);
		result = 1;
	}

	return result;
}

/******************************************************************************
 *
 * \par Function Name:
 *      reset_contact_plan
 *
 * \brief Delete all the contacts, ranges and nodes (but not the graphs)
 *
 *
 * \par Date Written:
 *      23/01/20
 *
 * \return void
 *
 *
 * \par Notes:
 *           1. All routes will be discarded
 *
 * \par Revision History:
 *
 *  DD/MM/YY | AUTHOR         |   DESCRIPTION
 *  -------- | ---------------|  -----------------------------------------------
 *  23/01/20 | L. Persampieri |   Initial Implementation and documentation.
 *****************************************************************************/
void reset_contact_plan()
{
	reset_NodesTree();
	reset_RangesGraph();
	reset_ContactsGraph();

	return;
}

/******************************************************************************
 *
 * \par Function Name:
 *      destroy_contact_plan
 *
 * \brief Delete all the contacts, ranges and nodes (and the graphs)
 *
 *
 * \par Date Written:
 *      23/01/20
 *
 * \return void
 *
 *
 * \par Notes:
 *           1. All routes will be discarded
 *
 * \par Revision History:
 *
 *  DD/MM/YY | AUTHOR         |   DESCRIPTION
 *  -------- | ---------------|  -----------------------------------------------
 *  23/01/20 | L. Persampieri |   Initial Implementation and documentation.
 *****************************************************************************/
void destroy_contact_plan()
{
	ContactPlanSAP sap = get_contact_plan_sap(NULL);

	destroy_NodesTree();
	destroy_RangesGraph();
	destroy_ContactsGraph();

	sap.initialized = 0;
	sap.contactsGraph = 0;
	sap.rangesGraph = 0;
	sap.nodes = 0;

	sap.contactPlanEditTime.tv_sec = -1;
	sap.contactPlanEditTime.tv_usec = -1;

	get_contact_plan_sap(&sap); // save

	return;
}

