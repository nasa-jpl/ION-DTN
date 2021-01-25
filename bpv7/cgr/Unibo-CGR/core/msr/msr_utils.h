/** \file msr_utils.h
 *
 *  \brief  This file provides the declaration of some utility function to manage routes
 *          get from CGRR Extension Block and attach them to CgrBundle struct.
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

#ifndef CGR_UNIBO_MSR_MSR_UTILS_H_
#define CGR_UNIBO_MSR_MSR_UTILS_H_

#include <stdlib.h>

#include "../library/commonDefines.h"
#include "../library/list/list.h"
#include "../contact_plan/contacts/contacts.h"
#include "../contact_plan/ranges/ranges.h"
#include "../bundles/bundles.h"
#include "../cgr/cgr_phases.h"
#include "msr.h"
#include "../routes/routes.h"

extern void delete_msr_route(Route *route);

#if (CGRR == 1 && MSR == 1)
#include "cgrr.h"
//You need to include a folder with cgrr.h in the path (options -I in gcc)
// From cgrr.h we get: CGRRouteBlock, CGRRoute, CGRRHop
/*
typedef struct
{
	uvast	fromNode;
	uvast	toNode;
	time_t	fromTime;
} CGRRHop;

//Note: uvast could be replaced with: unsigned int, unsigned long int, unsigned long long int

typedef struct
{
	unsigned int  hopCount; //Number of hops (contacts)
	CGRRHop		 *hopList; //Hop (contact): identified by [from, to, fromTime]
} CGRRoute;

//Note: hopList must be an array of hopCount size

typedef struct
{
	unsigned int recRoutesLength; //number of recomputedRoutes
	CGRRoute originalRoute; //computed by the source
	CGRRoute *recomputedRoutes; //computed by following nodes
} CGRRouteBlock;
*/
// Another implementation that defines these three struct type
// Can use the following functions:

extern int set_msr_route(time_t current_time, CGRRouteBlock *cgrrBlk, CgrBundle *bundle);

#endif




#endif /* CGR_UNIBO_MSR_MSR_UTILS_H_ */
