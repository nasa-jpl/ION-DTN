/** \file cgrr_msr_utils.h
 *
 *  \brief  This file provides the declaration of the functions to optimize the use of CGRR Extension Block
 *          for Moderate Source Routing.
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

#ifndef BPV7_LIBRARY_EXT_CGRR_CGRR_MSR_UTILS_H_
#define BPV7_LIBRARY_EXT_CGRR_CGRR_MSR_UTILS_H_

#include "cgrr.h"
#include <stdlib.h>


#ifdef __cplusplus
extern "C"
{
#endif

extern int storeMsrRoute(CGRRoute *cgrr_route, Bundle* bundle);
extern int updateLastCgrrRoute(Bundle *bundle);

#ifdef __cplusplus
}
#endif


#endif /* BPV7_LIBRARY_EXT_CGRR_CGRR_MSR_UTILS_H_ */
