/** \file config.h
 *
 *  \brief  This file contains the most important macros to configure Unibo-CGR.
 *
 *
 *  \details You can configure dynamic memory, logging, debugging and other features of Unibo-CGR
 *           (i.e. SABR enhancements or Moderate Source Routing).
 *
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

#ifndef BPV7_CGR_UNIBO_CGR_CORE_CONFIG_H_
#define BPV7_CGR_UNIBO_CGR_CORE_CONFIG_H_

#include <stdlib.h>

#define UNIBO_CGR_VERSION (1.0)

/************************************************************************************/
/************************************************************************************/
/***************************** BP AND SYSTEM CONFIGURATION  *************************/
/************************************************************************************/
/************************************************************************************/

/*
 * In this section you find all macros that are BP(or system)-dependent
 */

#ifndef CGR_BUILD_FOR_ION
/**
 * \brief Enable (1) to build Unibo-CGR for ION. Disable (0) otherwise.
 *
 * \hideinitializer
 */
#define CGR_BUILD_FOR_ION 1
#endif


#if (CGR_BUILD_FOR_ION)
#include "ion.h"
/**
 * \brief Dynamic memory allocation (use this as malloc()).
 *
 * \hideinitializer
 */
#define MWITHDRAW(size) allocFromIonMemory(__FILE__, __LINE__, size)
/**
 * \brief Dynamic memory deallocation (use this as free()).
 *
 * \hideinitializer
 */
#define MDEPOSIT(addr) releaseToIonMemory(__FILE__, __LINE__, addr)
#else
#define MWITHDRAW(size) malloc(size)
#define MDEPOSIT(addr) free(addr)
#endif

/************************************************************************************/
/************************************************************************************/
/************************************************************************************/
/************************************************************************************/
/************************************************************************************/





/************************************************************************************/
/************************************************************************************/
/******************************** LOGGING AND DEBUGGING *****************************/
/************************************************************************************/
/************************************************************************************/

/*
 * In this section you find some macro to manage the logging and debug print of Unibo-CGR.
 */

#ifndef LOG
/**
 * \brief Boolean: Set to 1 if you want to print various log files, otherwise set to 0.
 *        These files will be created in the ./cgr_log directory.
 *
 * \hideinitializer
 */
#define LOG 0
#endif

#ifndef DEBUG_CGR
/**
 * \brief Boolean: Set to 1 if you want some utility functions for debugging, 0 otherwhise
 *
 * \hideinitializer
 */
#define DEBUG_CGR 0
#endif

#ifndef CGR_DEBUG_FLUSH
/**
 * \brief Boolean: Set to 1 if you want to flush all debug print and log when you are
 *        in a debugging session, set to 0 otherwise.
 *
 * \par Notes:
 *          1. Remember that flushes every print is a bad practice, use it only for (hard) debug.
 *
 * \hideinitializer
 */
#define CGR_DEBUG_FLUSH 0
#endif

/************************************************************************************/
/************************************************************************************/
/************************************************************************************/
/************************************************************************************/
/************************************************************************************/





/************************************************************************************/
/************************************************************************************/
/***************************** COMPUTATIONAL LOAD ANALYSIS **************************/
/************************************************************************************/
/************************************************************************************/

/*
 * Enabling one (or more) of the following macros you will find
 * the ./total_<local_node>.csv
 * file where are reported all the times specified by the following macros.
 *
 * If you want to test how long does Unibo-CGR take
 * be sure that in your machine the Linux kernel is running;
 * otherwise do not enable the following macros.
 *
 * \par Notes:
 *      - Note that log and debug prints add considerable time.
 *        Disable them if you want to calculate more precise times.
 *      - To obtain precise results, activate only one of the
 *        following macros at a time, otherwise keep in mind
 *        the overhead added by calculating the innermost times
 *        to the outermost ones.
 */

#ifndef COMPUTE_TOTAL_CORE_TIME
/**
 * \brief Boolean: Set to 1 if you want to know how much time requires
 *        Unibo-CGR's core routing algorithm. O otherwise.
 *
 * \hideinitializer
 */
#define COMPUTE_TOTAL_CORE_TIME 0
#endif

#ifndef COMPUTE_PHASES_TIME
/**
 * \brief Boolean: Set to 1 if you want to know how much time requires
 *        each Unibo-CGR's phase. O otherwise.
 *
 * \hideinitializer
 */
#define COMPUTE_PHASES_TIME 0
#endif

#ifndef COMPUTE_TOTAL_INTERFACE_TIME
/**
 * \brief Exactly like COMPUTE_TOTAL_CORE_TIME, but with this macro you will consider
 *        also the interface's overhead.
 *
 * \hideinitializer
 */
#define COMPUTE_TOTAL_INTERFACE_TIME 0
#endif

#if (COMPUTE_TOTAL_CORE_TIME || COMPUTE_PHASES_TIME || COMPUTE_TOTAL_INTERFACE_TIME)
#undef TIME_ANALYSIS_ENABLED
#define TIME_ANALYSIS_ENABLED 1
#else
#undef TIME_ANALYSIS_ENABLED
#define TIME_ANALYSIS_ENABLED 0
#endif

/************************************************************************************/
/************************************************************************************/
/************************************************************************************/
/************************************************************************************/
/************************************************************************************/





/************************************************************************************/
/************************************************************************************/
/****************************** BUILT-IN CONFIGURATIONS  ****************************/
/************************************************************************************/
/************************************************************************************/

/*
 * In this section you find some macro to enable a preconfigured set of features.
 *
 * Notes:
 *      - The macros in this section are mutually exclusive.
 */

#ifndef UNIBO_CGR_SUGGESTED_SETTINGS
/**
 * \brief Anti-loop mechanism (proactive and reactive), one-route-per-neighbor and
 *        per-hop-queue-delay enabled. Boolean (1 enable, 0 disable).
 *
 * \par Notes:
 *           - RGR Ext. Block is required.
 *
 * \hideinitializer
 */
#define UNIBO_CGR_SUGGESTED_SETTINGS 0
#endif

#if (UNIBO_CGR_SUGGESTED_SETTINGS)
#define RGR 1
#define MAX_DIJKSTRA_ROUTES 0
#define CGR_AVOID_LOOP 3
#define QUEUE_DELAY 1
#define ADD_COMPUTED_ROUTE_TO_INTERMEDIATE_NODES 0
#define MSR 0
#endif

#ifndef MSR_PRECONF
/**
 * \brief Enable (1) this macro and this "wise" node will run Moderate Source Routing.
 *        Boolean (1 enable, 0 disable).
 *
 * \hideinitializer
 */
#define MSR_PRECONF 0
#endif

#if (MSR_PRECONF)
#define CGRR 1
#define WISE_NODE 1
#define MSR 1
#define MSR_TIME_TOLERANCE 2
#endif

#ifndef UNIBO_CGR_ENH_WITHOUT_EXP_EXTENSIONS
/**
 * \brief   Enable (1) this macro if you want to run Unibo-CGR with just
 *          one-route-per neighbor and per-hop-queue-delay enhancements.
 *          Boolean (1 enable, 0 disable).
 *
 * \details Enabling this macro is recommended if BP lacks of CGRR and RGR Ext. Block.
 *
 * \hideinitializer
 */
#define UNIBO_CGR_ENH_WITHOUT_EXP_EXTENSIONS 0
#endif

#if UNIBO_CGR_ENH_WITHOUT_EXP_EXTENSIONS
#define RGR 0
#define CGRR 0
#define MAX_DIJKSTRA_ROUTES 0
#define CGR_AVOID_LOOP 0
#define QUEUE_DELAY 1
#define MSR 0
#endif

#ifndef CCSDS_SABR_DEFAULTS
/**
 * \brief   Enable to get Unibo-CGR that follows all and only the criteria listed by CCSDS SABR.
 *          Boolean (1 enable, 0 disable).
 *
 * \details Set to 1 to enforce a CCSDS SABR like behavior
 *          (it will disable all Unibo-CGR enhancements when in contrast to CCSDS SABR).
 *          Otherwise set to 0.
 *
 * \hideinitializer
 */
#define CCSDS_SABR_DEFAULTS 0
#endif

#if (CCSDS_SABR_DEFAULTS == 1)
#define CGR_AVOID_LOOP 0
#define QUEUE_DELAY 0
#define MAX_DIJKSTRA_ROUTES 1
#define ADD_COMPUTED_ROUTE_TO_INTERMEDIATE_NODES 0
#define MSR 0
#define PERC_CONVERGENCE_LAYER_OVERHEAD (3.0)
#define MIN_CONVERGENCE_LAYER_OVERHEAD 100
#endif


/************************************************************************************/
/************************************************************************************/
/************************************************************************************/
/************************************************************************************/
/************************************************************************************/





/************************************************************************************/
/************************************************************************************/
/*********************** EXPERIMENTAL EXTENSION BLOCKS KNOWN ************************/
/************************************************************************************/
/************************************************************************************/

/*
 * In this section you can configure if Unibo-CGR should use some
 * experimental Extension Block (external to Unibo-CGR) or not.
 */

#ifndef CGRR
/**
 * \brief CGR Route Extension Block. Boolean (1 enable, 0 disable).
 *
 * \details Enabling this macro implies that the BP includes the source code of CGRR Ext. Block.
 *
 * \par Notes:
 *          - This extension is required for the purpose of Moderate Source Routing.
 *
 * \hideinitializer
 */
#define CGRR 0
#endif

#ifndef WISE_NODE
/**
 * \brief Boolean: set to 1 (enable) if this node has the complete knowledge of the contact plan, otherwise set to 0.
 *        Only a "wise node" can update the CGRR extension block.
 *
 * \par Notes:
 *           - For a "wise node" the MSR must find all the contacts proposed in the MSR route.
 *
 * \hideinitializer
 */
#define WISE_NODE 1
#endif

#ifndef RGR
/**
 * \brief Register Geographical Route Extension Block. Boolean (1 enable, 0 disable).
 *
 * \details Enabling this macro implies that the BP the BP includes the source code of RGR Ext. Block.
 *
 * \par Notes:
 *           - This extension is required for the purpose of anti-loop experimental enhancement.
 *
 * \hideinitializer
 */
#define RGR 0
#endif


/************************************************************************************/
/************************************************************************************/
/************************************************************************************/
/************************************************************************************/
/************************************************************************************/





/************************************************************************************/
/************************************************************************************/
/************************ EXPERIMENTAL ENHANCEMENTS FOR SABR ************************/
/************************************************************************************/
/************************************************************************************/

/*
 * In this section you can enable some feature of Unibo-CGR in order to get better performance from SABR.
 */

// TODO LP: modificare nome macro
#ifndef MAX_DIJKSTRA_ROUTES
/**
 * \brief   This macro limits the number of Dijkstra's routes calculated by the Unibo_CGR "one-route-per-neighbor" enhancement.
 *
 * \details CCSDS SABR leaves to the implementation the number of routes computed by Dijkstra at each call of Phase 1; in ION only one route is computed.
 *          Better perfomance can be achived by computing additional routes, in particular by forcing one route for each neighbor, if possible ("one-route-
 *          per-neighbor").
 *          To cope with environments with stringent computational constraints, it is possible to limit the number of routes to the best N neighbors.
 *          - Set to 0 to disable the limit (i.e. leaving one-route-per-neigbhor unmodified).
 *          - Set to N (N > 0) to limit the number of routes; in particular, set to 1 to enforce ION behavior.
 *
 * \hideinitializer
 */
#define MAX_DIJKSTRA_ROUTES 1
#endif

#ifndef CGR_AVOID_LOOP
/**
 * \brief    Unibo_CGR provides the user with both a reactive and a proactive mechanism to counteract loops.
 *
 * \details This macro is used in all logical phases.
 *          - Set to 0 to disable all anti-loop mechanisms (as in CCSDS and ION SABR).
 *          - Set to 1 to enable the reactive version only.
 *          - Set to 2 to enable the proactive version only.
 *          - Set to 3 to enable both.
 *
 * \par Notes:
 *           - This enhancement needs the RGR Extension Block enabled.
 *
 * \hideinitializer
 */
#define CGR_AVOID_LOOP 0
#endif

#ifndef QUEUE_DELAY
/**
 * \brief   ETO on all hops
 *
 * \details This is a possible enhancement provided by Unibo_CGR. In both CCSDS SABR and ION only the local queue delay is computed ("ETO on first hop")
 *          However, by exploiting the Expected Volume Consumptions of the route contacts, it is possible to have a rough but conservative estimation of
 *          queuing delay on next hops.
 *          - Set to 1 to consider ETO (expected queue delays) on all hops of the route.
 *          - Set to 0 to disable this enhancement.
 *
 * \hideinitializer
 */
#define QUEUE_DELAY 0
#endif

#ifndef ADD_COMPUTED_ROUTE_TO_INTERMEDIATE_NODES
/**
 * \brief Enable to compute routes in advance to some nodes.
 *
 * \details This is a possible enhancement provided by Unibo_CGR. Dijkstra is called in phase 1 to compute a CGR route
 *          (sequence of contacts) to destination D. At computation time ("t0") this route, however, because of Dikstra's optimality,
 *          it is also the best route to get not only to D, but to all nodes along the geo route subjected at the CGR route (e.g. B-C-F-D).
 *          Set to 1 if you want to add the computed route to the computed routes list of each node of the GEO route (e.g. B,C,F and D).
 *          Set to 0 if you want the computed route to be added only to D, thus disabling this optimization.
 *          Note that this optimization is intended to reduce computational load, should a following bundle be destined (at "t1") to an intermediate node
 *          (e.g. B,C or F). On the other hand, a route computed at "t0" may be no more optimal at "t1", with a possible impact on routing accuracy.
 *          - Set to 1 to possibly reduce computational load.
 *          - Set to 0 to to preserve standard behavior (to disable the enhancement).
 *
 * \hideinitializer
 */
#define ADD_COMPUTED_ROUTE_TO_INTERMEDIATE_NODES 0
#endif

/************************************************************************************/
/************************************************************************************/
/************************************************************************************/
/************************************************************************************/
/************************************************************************************/





/************************************************************************************/
/************************************************************************************/
/****************************** MODERATE SOURCE ROUTING *****************************/
/************************************************************************************/
/************************************************************************************/

/*
 * In this section you can configure the Moderate Source Routing.
 */

#ifndef MSR
/**
 * \brief Used to enable or disable Moderate Source Routing: set to 1 to enable MSR, otherwise set to 0.
 *
 * \par Notes:
 *           - For the purpose of MSR you have to enable also CGRR.
 *
 * \hideinitializer
 */
#define MSR 0
#endif

#ifndef MSR_TIME_TOLERANCE
/**
 * \brief Tolerance for the contact's fromTime field.
 *        The fromTime of the CGRR contact can differs of MSR_TIME_TOLERANCE from the fromTime of the
 *        contact stored in the contact graph of this node.
 */
#define MSR_TIME_TOLERANCE 2
#endif

#if (MSR && WISE_NODE == 0)
#ifndef MSR_HOPS_LOWER_BOUND
/**
 * \brief Used only with WISE_NODE disabled: set to N if you want that this node find (in the contact graph)
 *        at least the first N contacts (hops) of the MSR route.
 *
 * \hideinitializer
 */
#define MSR_HOPS_LOWER_BOUND 1
#endif
#endif



/************************************************************************************/
/************************************************************************************/
/************************************************************************************/
/************************************************************************************/
/************************************************************************************/




/************************************************************************************/
/************************************************************************************/
/******************************* CCSDS SABR CONSTANTS  ******************************/
/************************************************************************************/
/************************************************************************************/

/*
 * In this section you find some constants defined by SABR.
 *
 */

#ifndef PERC_CONVERGENCE_LAYER_OVERHEAD
/**
 * \brief   Value used to compute the estimated convergence-layer overhead.
 *
 * \details It must be greater or equal to 0.0 .
 *          Keep in mind that ION 3.7.0 use 6.25% as default value.
 *          That is defined 3.00% in CCSDS SABR.
 */
#define PERC_CONVERGENCE_LAYER_OVERHEAD (6.25)
#endif

#ifndef MIN_CONVERGENCE_LAYER_OVERHEAD
/**
 * \brief   Minimum value for the estimated convergence-layer overhead.
 *
 * \details It assumes the same meaning of the macro TIPYCAL_STACK_OVERHEAD in ION's bpP.h.
 *
 * \par Notes:
 *           - CCSDS SABR: 100
 *           - ION 3.7.0:   36 (default value in ION but it can be modified)
 *
 */
#define MIN_CONVERGENCE_LAYER_OVERHEAD 36
#endif

/************************************************************************************/
/************************************************************************************/
/************************************************************************************/
/************************************************************************************/
/************************************************************************************/




/************************************************************************************/
/************************************************************************************/
/******************************* FATAL ERROR SECTION  *******************************/
/************************************************************************************/
/************************************************************************************/
/*
 * This section is used to find errors at compile-time.
 */
/**
 * \cond
 */

#if (CGR_BUILD_FOR_ION != 0 && CGR_BUILD_FOR_ION != 1)
fatal error
// Intentional compilation error
// CGR_BUILD_FOR_ION must be 0 or 1
#endif

#if (LOG != 0 && LOG != 1)
fatal error
// Intentional compilation error
// LOG must be 0 or 1
#endif

#if (DEBUG_CGR != 0 && DEBUG_CGR != 1)
fatal error
// Intentional compilation error
// DEBUG_CGR must be 0 or 1
#endif

#if (CGR_DEBUG_FLUSH != 0 && CGR_DEBUG_FLUSH != 1)
fatal error
// Intentional compilation error
// CGR_DEBUG_FLUSH must be 0 or 1
#endif

#if (MAX_DIJKSTRA_ROUTES < 0)
fatal error
// Intentional compilation error
// MAX_DIJKSTRA_ROUTES must be >= 0.
#endif

#if (ADD_COMPUTED_ROUTE_TO_INTERMEDIATE_NODES != 0 && ADD_COMPUTED_ROUTE_TO_INTERMEDIATE_NODES != 1)
fatal error
// Intention compilation error
// ADD_COMPUTED_ROUTE_TO_INTERMEDIATE_NODES must be 0 or 1
#endif

#if (QUEUE_DELAY != 0 && QUEUE_DELAY != 1)
fatal error
// Intentional compilation error
// QUEUE_DELAY must be 0 or 1.
#endif

#if (CGR_AVOID_LOOP != 0 && CGR_AVOID_LOOP != 1 && CGR_AVOID_LOOP != 2 && CGR_AVOID_LOOP != 3)
fatal error
// Intentional compilation error
// CGR_AVOID_LOOP must be 0 or 1 or 2 or 3.
#endif

#if (CGRR != 0 && CGRR != 1)
fatal error
// Intentional compilation error
// CGRR must be 0 or 1.
#endif

#if (WISE_NODE != 0 && WISE_NODE != 1)
fatal error
// Intentional compilation error
// WISE_NODE must be 0 or 1.
#endif

#if (RGR != 0 && RGR != 1)
fatal error
// Intentional compilation error
// RGR must be 0 or 1.
#endif

#if (CGR_AVOID_LOOP && !RGR)
fatal error
// Intentional compilation error
// Anti-loop mechanism needs RGR enabled.
#endif

#if (MSR != 0 && MSR != 1)
fatal error
// Intentional compilation error
// MSR must be 0 or 1.
#endif

#if (MSR && !CGRR)
fatal error
// Intentional compilation error
// MSR needs CGRR enabled.
#endif

#if (MSR && !WISE_NODE && MSR_HOPS_LOWER_BOUND <= 0)
fatal error
// Intentional compilation error
// MSR_HOPS_LOWER_BOUND must be greater than 0.
#endif

#if (UNIBO_CGR_SUGGESTED_SETTINGS != 0 && UNIBO_CGR_SUGGESTED_SETTINGS != 1)
// Intentional compilation error
// CGR_UNIBO_SUGGESTED_SETTINGS must be 0 or 1.
fatal error
#endif

#if (CCSDS_SABR_DEFAULTS != 0 && CCSDS_SABR_DEFAULTS != 1)
// Intentional compilation error
// CCSDS_SABR_DEFAULTS must be 0 or 1.
fatal error
#endif

#if( (CCSDS_SABR_DEFAULTS && (UNIBO_CGR_SUGGESTED_SETTINGS || MSR_CONFIG || UNIBO_CGR_ENH_WITHOUT_EXP_EXTENSIONS)) \
	|| (UNIBO_CGR_SUGGESTED_SETTINGS && (CCSDS_SABR_DEFAULTS || MSR_CONFIG || UNIBO_CGR_ENH_WITHOUT_EXP_EXTENSIONS)) \
	|| (UNIBO_CGR_ENH_WITHOUT_EXP_EXTENSIONS && (UNIBO_CGR_SUGGESTED_SETTINGS || MSR_CONFIG || CCSDS_SABR_DEFAULTS)) \
	|| (MSR_PRECONF && (UNIBO_CGR_SUGGESTED_SETTINGS || CCSDS_SABR_DEFAULTS || UNIBO_CGR_ENH_WITHOUT_EXP_EXTENSIONS)))
fatal error
// Intentional compilation error
// CGR_UNIBO_SUGGESTED_SETTINGS, CCSDS_SABR_DEFAULTS and CGR_ION_3_7_0 must be mutually exclusive.
#endif

#if (MIN_CONVERGENCE_LAYER_OVERHEAD < 0)
// Intentional compilation error
// MIN_CONVERGENCE_LAYER_OVERHEAD cannot be negative.
fatal error
#endif
/**
 * \endcond
 */

/************************************************************************************/
/************************************************************************************/
/************************************************************************************/
/************************************************************************************/
/************************************************************************************/

#endif /* BPV7_CGR_UNIBO_CGR_CORE_CONFIG_H_ */
