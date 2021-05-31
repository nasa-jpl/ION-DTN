/** \file contactPlan.h
 *
 *  \brief   This file provides the declarations of the functions
 *           to manage the contact plan. The functions are implemented
 *           in contactPlan.c
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

#ifndef LIBRARY_CONTACTSPLAN_H_
#define LIBRARY_CONTACTSPLAN_H_

#include <sys/time.h>

#include "../library/commonDefines.h"

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * \brief Used to get some information about the state of the contact plan.
 */
typedef struct {
	/**
	 * \brief Boolean: 0 if the contacts graph hasn't been initialized, 1 otherwise.
	 */
	int contactsGraph;
	/**
	 * \brief Boolean: 0 if the ranges graph hasn't been initialized, 1 otherwise.
	 */
	int rangesGraph;
	/**
	 * \brief Boolean: 0 if the nodes tree hasn't been initialized, 1 otherwise.
	 */
	int nodes;
	/**
	 * \brief Boolean: 0 if all the main structures hasn't been initialized, 1 otherwise.
	 */
	int initialized;
	/**
	 * \brief The last time when we add/delete contacts/ranges.
	 */
	struct timeval contactPlanEditTime;
} ContactPlanSAP;

extern ContactPlanSAP get_contact_plan_sap(ContactPlanSAP *newSap);

extern void set_time_contact_plan_updated(__time_t seconds, __suseconds_t micro_seconds);

extern int initialize_contact_plan();

extern void removeExpired(time_t time);

extern int addContact(unsigned long regionNbr, unsigned long long fromNode, unsigned long long toNode, time_t fromTime,
		time_t toTime, long unsigned int xmitRate, float confidence, int copyMTV, double mtv[]);
extern int removeContact(unsigned long regionNbr, unsigned long long fromNode, unsigned long long toNode,
		time_t *fromTime);

extern int addRange(unsigned long long fromNode, unsigned long long toNode, time_t fromTime,
		time_t toTime, unsigned int owlt);
extern int removeRange(unsigned long long fromNode, unsigned long long toNode, time_t *fromTime);

extern void reset_contact_plan();
extern void destroy_contact_plan();

#ifdef __cplusplus
}
#endif

#endif /* LIBRARY_CONTACTSPLAN_H_ */
