/** \file time.h
 *
 *  \brief   This file contains the definition of the functions
 *           to perform a Unibo-CGR computational load analysis
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

#ifndef CORE_TIME_ANALYSIS_TIME_H_
#define CORE_TIME_ANALYSIS_TIME_H_

#include "../config.h"
#include "../library/commonDefines.h"
#include "../bundles/bundles.h"

typedef enum {
	phaseOne = 1,
	phaseTwo = 2,
	phaseThree = 3
} UniboCgrPhase;

#ifdef __cplusplus
extern "C"
{
#endif

#if (TIME_ANALYSIS_ENABLED)

extern void initialize_time_analysis();
extern void destroy_time_analysis();
extern void print_time_results(time_t currentTime, unsigned int callNumber, CgrBundleID *id);

#else

#define initialize_time_analysis() do {  } while(0)
#define destroy_time_analysis() do {  } while(0)
#define print_time_results(currentTime, callNumber, id) do {  } while(0)

#endif


#if (COMPUTE_TOTAL_CORE_TIME)

extern void record_total_core_start_time();
extern void record_total_core_stop_time();

#else

#define record_total_core_start_time() do {  } while(0)
#define record_total_core_stop_time() do {  } while(0)

#endif

#if (COMPUTE_TOTAL_INTERFACE_TIME)

extern void record_total_interface_start_time();
extern void record_total_interface_stop_time();

#else

#define record_total_interface_start_time() do {  } while(0)
#define record_total_interface_stop_time() do {  } while(0)

#endif

#if (COMPUTE_PHASES_TIME)

extern void record_phases_start_time(UniboCgrPhase phase);
extern void record_phases_stop_time(UniboCgrPhase phase);

#else

#define record_phases_start_time(phase) do {  } while(0)
#define record_phases_stop_time(phase) do {  } while(0)

#endif

#ifdef __cplusplus
}
#endif


#endif /* CORE_TIME_ANALYSIS_TIME_H_ */
