/** \file cgr_phases.h
 *	
 *  \brief  This file contains the declarations of the functions
 *          to call and manage each of the three logical CGR phases.
 *
 *
 *  \details You find the implementation of the functions declared here
 *           in the files phase_one.c, phase_two.c and phase_three.c .
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

#ifndef SOURCES_CGR_PHASES_H_
#define SOURCES_CGR_PHASES_H_

#include "../bundles/bundles.h"
#include "../contact_plan/nodes/nodes.h"
#include <sys/time.h>
#include "../library/commonDefines.h"
#include "../library/list/list_type.h"
#include "../library_from_ion/scalar/scalar.h"

#ifdef __cplusplus
extern "C"
{
#endif

/******************************************************/

/************** BUILT-IN IMPLEMENTATIONS **************/
/*
 * In this section you find some macro to enable or disable
 * a series of built-in implementations.
 * Keep in mind that they must be mutually exclusive.
 *
 * Built-in implementations:
 * - CGR_UNIBO_SUGGESTED_SETTINGS
 * - CCSDS_SABR_DEFAULTS
 * - CGR_ION_3_7_0
 *
 */
/******************************************************/

/**********************SHARED MACROS*******************/
// Macros used in multiple CGR logical phases.

/**
 * \brief   The maximum rate of change in distance between any two nodes in the
 *          network is 450,000 miles (720,000 km) per hour.
 *
 * \details From SABR CCSDS Blue book, 2.4.2
 */
#define	MAX_SPEED_MPH	(450000)

#if (CGR_AVOID_LOOP > 0)
#undef NO_LOOP
#undef POSSIBLE_LOOP
#undef CLOSING_LOOP
#undef FAILED_NEIGHBOR
/**
 * \brief   Used to flag candidate routes that the anti-loop mechanism consider as "No loop routes".
 *
 * \details For the reactive version it means that the route's neighbor isn't a failed neighbor.
 *          For the proactive version it means that there aren't matches between
 *          the candidate route and the geo route of the bundle.
 *          For the proactive and reactive version it means that the CGR of the local node
 *          doesn't see a risk of a routing loop using this candidate route.
 *          Don't change this value.
 */
#define NO_LOOP 1
/**
 * \brief   Used to flag routes considered as "Possible loop routes" by the anti-loop proactive version.
 *
 * \details It means that at least one node of the route (except the neighbor)
 *          matches with one node of the geographic route of the bundle.
 *          Don't change this value.
 */
#define POSSIBLE_LOOP 2
/**
 * \brief   Used to flag routes considered as "Closing loop routes" by the anti-loop proactive version.
 *
 * \details It means that the route's neighbor matches with one node of the bundle's geo route.
 *          Don't change this value.
 */
#define CLOSING_LOOP 3
/**
 * \brief   Used by the anti-loop reactive versione to flag routes with a failed neighbor.
 *
 * \details It means that the route's neighbor matches with one bundle's failed neighbor.
 *          Don't change this value.
 *
 * \par Notes:
 *           - The bundle's failed neighbors list is the list of neighbor to which the
 *             bundle was previously forwarded by the local node but due a loop
 *             it's come back to the local node.
 */
#define FAILED_NEIGHBOR 4
#endif


/******************************************************/

/*******************PHASE ONE MACROS*******************/
// Macros used in phase one only



/******************************************************/

/*******************PHASE TWO MACROS*******************/
// Macros used in phase two only

#if (CCSDS_SABR_DEFAULTS == 0)
#ifndef NEGLECT_CONFIDENCE
/**
 * \brief   Used to enable or disable the use of the confidence for the computation and the choose of the routes.
 *
 * \details ION implementation differs slightly from CCSDS SABR in the candidate and best route selection criteria
 *          as ION use the route confidence, not mentioned in CCSDS SABR.
 *          Set to 1 if you want CGR to strictly implements the CCSDS SABR criteria, thus to neglect confidence.
 *          Otherwise set to 0.
 *
 * \hideinitializer
 */
#define NEGLECT_CONFIDENCE 0
#endif
#endif

#ifndef MIN_CONFIDENCE_IMPROVEMENT
/**
 * \brief   Lower confidence for a route.
 *
 * \details It must be between 0.0 and 1.0 .
 *          This macro should assume the same value used by the bundle protocol.
 *          Keep in mind that ION 3.7.0 use 0.05 as default value.
 *
 * \warning Change this value only if you know what you are doing.
 */
#define	MIN_CONFIDENCE_IMPROVEMENT	(0.05)
#endif


/******************************************************/

/***************************** PHASE ONE *****************************/
extern int initialize_phase_one(unsigned long long localNode);
extern void reset_phase_one();
extern void destroy_phase_one();
extern int computeRoutes(unsigned long regionNbr, Node *terminusNode, List subsetComputedRoutes, long unsigned int missingNeighbors);
/*********************************************************************/

/***************************** PHASE TWO *****************************/
extern int initialize_phase_two();
extern void destroy_phase_two();
extern void reset_phase_two();
extern int checkRoute(time_t current_time, unsigned long long localNode, CgrBundle *bundle, List excludedNeighbors, Route *route);
extern int getCandidateRoutes(Node *terminusNode, CgrBundle *bundle, List excludedNeighbors, List computedRoutes,
		List *subsetComputedRoutes, long unsigned int *missingNeighbors, List *candidateRoutes);
extern int computeApplicableBacklog(unsigned long long neighbor, int priority, unsigned int ordinal, CgrScalar *applicableBacklog,
		CgrScalar *totalBacklog);
/*********************************************************************/

/**************************** PHASE THREE ****************************/
extern int chooseBestRoutes(CgrBundle *bundle, List candidateRoutes);
/*********************************************************************/

/**************************** SHARED *****************************/
extern time_t get_current_time();
extern unsigned long long get_local_node();
/*****************************************************************/

#if (LOG == 1)
extern void print_phase_one_routes(FILE *file, List computedRoutes);
extern void print_phase_two_routes(FILE *file, List candidateRoutes);
extern void print_phase_three_routes(FILE *file, List bestRoutes);
#else
#define print_phase_one_routes(file, computedRoutes) do { } while(0)
#define print_phase_two_routes(file, candidateRoutes) do { } while(0)
#define print_phase_three_routes(file, bestRoutes) do { } while(0)
#endif

/******************CHECK MACROS ERROR******************/

/**
 * \cond
 */
#if (NEGLECT_CONFIDENCE != 0 && NEGLECT_CONFIDENCE != 1)
fatal error
// Intentional compilation error
// NEGLECT_CONFIDENCE must be 0 or 1.
#endif


/**
 * \endcond
 */

/******************************************************/



#ifdef __cplusplus
}
#endif

#endif /* SOURCES_CGR_PHASES_H_ */
