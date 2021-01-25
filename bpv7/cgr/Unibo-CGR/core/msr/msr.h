/** \file msr.h
 *
 *  \brief  This file provides the declaration of the function used
 *          to start, call and stop the Moderate Source Routing.
 *          You find also the macro to enable MSR in this CGR implementation,
 *          and other macros to change MSR behavior.
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

#ifndef CGR_UNIBO_MSR_MSR_H_
#define CGR_UNIBO_MSR_MSR_H_

#include <stdlib.h>

#include "../library/commonDefines.h"
#include "../library/list/list.h"
#include "../bundles/bundles.h"
#include "../routes/routes.h"


#if (MSR == 1)

extern int tryMSR(time_t current_time, CgrBundle *bundle, List excludedNeighbors, FILE *file_call, List *bestRoutes);
extern int initialize_msr();
extern void destroy_msr();

#endif

/************ CHECK MACROS ERROR *************/

#if (MSR != 0 && MSR != 1)
fatal error
// Intentional compilation error
// MSR must be 0 or 1
#endif

#if (MSR == 1)

#if (WISE_NODE != 0 && WISE_NODE != 1)
fatal error
// Intentional compilation error
// WISE_NODE must be 0 or 1
#endif

#if (MSR_TIME_TOLERANCE < 0)
fatal error
// Intentional compilation error
// MSR_TIME_TOLERANCE must be greater or equal to 0.
#endif

#if (WISE_NODE == 0 && MSR_HOPS_LOWER_BOUND < 1)
fatal error
// Intentional compilation error
// MSR_HOPS_TOLERANCE must be greater or equal to 1.
#endif

#endif

/*********************************************/

#endif /* CGR_UNIBO_MSR_MSR_H_ */
