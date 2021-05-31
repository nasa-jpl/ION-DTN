/** \file interface_cgr_ion.c
 *  
 *  \brief    This file provides the implementation of the functions
 *            that make this CGR's implementation compatible with ION.
 *	
 *  \details  In particular the functions that import the contact plan and the BpPlans.
 *            We provide in output the best routes list.
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
#include "interface_cgr_ion.h"

#if UNIBO_CGR

#include <sys/time.h>

//include from ion
#include "ion.h"
#include "platform.h"
#include "sdrstring.h"
#include "cgr.h"
#include "bpP.h"
#include "bp.h"

#include <stdio.h>
#include <stdlib.h>
#include "../../core/bundles/bundles.h"
#include "../../core/contact_plan/contacts/contacts.h"
#include "../../core/cgr/cgr.h"
#include "../../core/cgr/cgr_phases.h"
#include "../../core/library_from_ion/scalar/scalar.h"

#include "../../core/contact_plan/contactPlan.h"
#include "../../core/contact_plan/contacts/contacts.h"
#include "../../core/contact_plan/ranges/ranges.h"
#include "../../core/msr/msr_utils.h"
#include "utility_functions_from_ion/general_functions_ported_from_ion.h"
#include "../../core/library/commonDefines.h"
#include "../../core/library/list/list.h"
#include "../../core/msr/msr.h"
#include "../../core/time_analysis/time.h"

#if(CGRR)
#include "cgrr.h"
#include "cgrr_msr_utils.h"
#include "cgrr_help.h"
#endif
#if (RGR)
#include "rgr.h"
#endif

/**
 * \brief Get the absolute value of "a"
 *
 * \param[in]   a   The real number for which we want to know the absolute value
 *
 * \hideinitializer
 */
#define absolute(a) (((a) < 0) ? (-(a)) : (a))

#ifndef GET_MTV_FROM_SDR
/**
 * \brief Set to 1 if you want to get the MTVs from ION's contact MTVs stored into SDR.
 *        Otherwise set to 0 (they will be computed as xmitRate*(toTime - fromTime)).
 *
 * \hideinitializer
 */
#define GET_MTV_FROM_SDR 1
#endif

#ifndef DEBUG_ION_INTERFACE
/**
 * \brief Boolean value used to enable some debug's print for this interface.
 * 
 * \details Set to 1 if you want to enable the debug's print, otherwise set to 0.
 *
 * \hideinitializer
 */
#define DEBUG_ION_INTERFACE 0
#endif

#define NOMINAL_PRIMARY_BLKSIZE	29 // from ION 4.0.0: bpv7/library/libbpP.c

typedef struct {
	/**
	 * \brief ION's reference time, used to convert times from Unibo-CGR (differential time) to ION
	 */
	time_t reference_time;
	/**
	 * \brief 1 if Unibo-CGR's interface for ION has been initialized. 0 otherwise.
	 */
	int initialized;
} InterfaceUniboCgrSAP;

/**
 * \brief Struct used to mantain in one place all the temporary data used by the interface
 *        in each call. These data make sense only during a call to Unibo-CGR.
 */
typedef struct {
	/**
	 * \brief Bundle sent from ION. We keep the reference to it, in this way
	 *        Unibo-CGR can talk with CL queue.
	 */
	Bundle *ionBundle; /* sent from ION */
	/**
	 * \brief Bundle sent to Unibo-CGR to find viable route(s) for it.
	 */
	CgrBundle *uniboCgrBundle; /* sent to Unibo-CGR */
	/**
	 * \brief List of excluded neighbors sent to Unibo-CGR. The bundle cannot be sent
	 *        to these neighbors.
	 */
	List excludedNeighbors; /* sent to Unibo-CGR */
	/**
	 * \brief Reference to all CgrRoute object created.
	 */
	Lyst routes;
} CurrentCallSAP;


/******************************************************************************
 *
 * \par Function Name:
 * 		get_current_call_sap
 *
 * \brief Get the CurrentCallSAP with all the values memorized by interface for the current call
 *        to Unibo-CGR.
 *
 *
 * \par Date Written:
 * 	    02/07/20
 *
 * \return CurrentCallSAP*
 *
 * \retval  CurrentCallSAP*  The struct with all the values memorized by interface
 *                           for the current call.
 *
 * \param[in]   *newSap      If you just want a reference to the SAP set NULL here;
 *                           otherwise set a new CurrentCallSAP (the previous one will be overwritten).
 *
 *
 * \par Revision History:
 *
 *  DD/MM/YY | AUTHOR          |  DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  02/07/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
static CurrentCallSAP *get_current_call_sap(CurrentCallSAP *newSap) {
	static CurrentCallSAP	sap;

	if (newSap != NULL)
	{
		sap = *newSap;
	}

	return &sap;
}

static void destroyRoute(LystElt elt, void *arg)
{
	PsmPartition ionwm = getIonwm();
	CgrRoute *route = lyst_data(elt);

	if (route)
	{
		sm_list_destroy(ionwm, route->hops, NULL, NULL);
		MRELEASE(route);
	}
}


/******************************************************************************
 *
 * \par Function Name:
 * 		destroy_current_call_sap
 *
 * \brief Deallocate all memory referenced by the CurrentCallSAP.
 *
 *
 * \par Date Written:
 * 	    02/07/20
 *
 * \return void
 *
 * \par Revision History:
 *
 *  DD/MM/YY | AUTHOR          |  DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  02/07/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
static void destroy_current_call_sap()
{

	CurrentCallSAP *sap = get_current_call_sap(NULL);

	bundle_destroy(sap->uniboCgrBundle);
	free_list(sap->excludedNeighbors);
	lyst_delete_set(sap->routes, destroyRoute, NULL);
	lyst_destroy(sap->routes);
	sap->ionBundle = NULL;
	sap->uniboCgrBundle = NULL;
	sap->excludedNeighbors = NULL;
	sap->routes = NULL;

	return;
}

/******************************************************************************
 *
 * \par Function Name:
 * 		initialize_current_call_sap
 *
 * \brief Allocate all memory (list etc.) that will be used by the CurrentCallSAP.
 *        Initialized all CurrentCallSAP's field to a known state.
 *
 *
 * \par Date Written:
 * 	    02/07/20
 *
 * \return int
 *
 * \retval  1    Success case, CurrentCallSAP initialized
 * \retval -2    MWITHDRAW error
 *
 *
 * \par Revision History:
 *
 *  DD/MM/YY | AUTHOR          |  DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  02/07/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
static int initialize_current_call_sap() {
	int result = 1;
	CurrentCallSAP *sap = get_current_call_sap(NULL);
	int ionMemIdx;

	sap->ionBundle = NULL; // init, no bundle
	sap->uniboCgrBundle = bundle_create(); //init Unibo-CGR's bundle
	sap->excludedNeighbors = list_create(NULL, NULL, NULL, MDEPOSIT_wrapper);
	ionMemIdx = getIonMemoryMgr();
	sap->routes = lyst_create_using(ionMemIdx);

	if (sap->routes == NULL || sap->uniboCgrBundle == NULL || sap->excludedNeighbors == NULL) {
		result = -2;
		bundle_destroy(sap->uniboCgrBundle);
		free_list(sap->excludedNeighbors);
		lyst_destroy(sap->routes);
	}

	return result;
}


/******************************************************************************
 *
 * \par Function Name:
 * 		initialize_cgr_sap
 *
 * \brief Initialize the InterfaceUniboCgrSAP, received as argument, to a known state.
 *        InterfaceUniboCgrSAP must be allocated by the caller.
 *
 *
 * \par Date Written:
 * 	    02/07/20
 *
 * \return int
 *
 * \retval  1    Success case, InterfaceUniboCgrSAP (sap) initialized
 * \retval -1    Error case: arguments error
 *
 * \param[in]      reference_time  The reference_time used by interface to convert all times
 *                                 in differential time, so this is the time "0".
 *                                 It will be saved into CgrSAP.
 * \param[in,out]  sap             The InterfaceUniboCgrSAP that the caller want to initialize.
 *
 *
 * \par Revision History:
 *
 *  DD/MM/YY | AUTHOR          |  DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  02/07/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
static int initialize_cgr_sap(time_t reference_time, InterfaceUniboCgrSAP *sap) {
	int result = 1;

	if(reference_time < 0 || sap == NULL) {
		result = -1;
	}
	else {
		sap->reference_time = reference_time;
		sap->initialized = 1;
	}

	return result;
}



#if(DEBUG_ION_INTERFACE == 1)
/******************************************************************************
 *
 * \par Function Name:
 * 		printDebugIonRoute
 *
 * \brief Print the "ION"'s route to standard output
 *
 *
 * \par Date Written:
 * 	    04/04/20
 *
 * \return void
 *
 * \param[in]   *route   The route that we want to print.
 *
 * \par Notes:
 *            1. This function print only the value of the route that
 *               will effectively used by ION.
 *            2. All the times are differential times from the reference time.
 *
 *
 * \par Revision History:
 *
 *  DD/MM/YY | AUTHOR          |  DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  04/04/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
static void printDebugIonRoute(PsmPartition ionwm, CgrRoute *route)
{
	int stop = 0;
	Sdr sdr = getIonsdr();
	PsmAddress addr, addrContact;
	SdrObject contactObj;
	IonCXref *contact;
	IonContact contactBuf;
	time_t ref = reference_time;
	if (route != NULL)
	{
		fprintf(stdout, "\nPRINT ION ROUTE\n%-15s %-15s %-15s %-15s %-15s %-15s %s\n", "ToNodeNbr",
				"FromTime", "ToTime", "ETO", "PBAT", "MaxVolumeAvbl", "BundleECCC");
		fprintf(stdout, "%-15llu %-15ld %-15ld %-15ld %-15ld %-15g %lu\n",
				(unsigned long long) route->toNodeNbr, (long int) route->fromTime - ref,
				(long int) route->toTime - ref, (long int) route->eto - ref,
				(long int) route->pbat - ref, route->maxVolumeAvbl, (long unsigned int) route->bundleECCC);
		fprintf(stdout, "%-15s %-15s %-15s %-15s %-15s %s\n", "Confidence", "Hops", "Overbooked (G)",
				"Overbooked (U)", "Protected (G)", "Protected (U)");
		fprintf(stdout, "%-15.2f %-15ld %-15d %-15d %-15d %d\n", route->arrivalConfidence,
				(long int) sm_list_length(ionwm, route->hops), route->overbooked.gigs,
				route->overbooked.units, route->protected.gigs, route->protected.units);
		fprintf(stdout, "%-15s %-15s %-15s %-15s %-15s %-15s %-15s %-15s %s\n", "FromNode",
				"ToNode", "FromTime", "ToTime", "XmitRate", "Confidence", "MTV[Bulk]",
				"MTV[Normal]", "MTV[Expedited]");
		for (addr = sm_list_first(ionwm, route->hops); addr != 0 && !stop;
				addr = sm_list_next(ionwm, addr))
		{
			addrContact = sm_list_data(ionwm, addr);
			stop = 1;
			if (addrContact != 0)
			{
				contact = psp(ionwm, addrContact);
				if (contact != NULL)
				{
					stop = 0;
					contactObj = sdr_list_data(sdr, contact->contactElt);
					sdr_read(sdr, (char*) &contactBuf, contactObj, sizeof(IonContact));
					fprintf(stdout,
							"%-15llu %-15llu %-15ld %-15ld %-15lu %-15.2f %-15g %-15g %g\n",
							(unsigned long long) contact->fromNode,
							(unsigned long long) contact->toNode,
							(long int) contact->fromTime - ref, (long int) contact->toTime - ref,
							(long unsigned int) contact->xmitRate, contact->confidence, contactBuf.mtv[0],
							contactBuf.mtv[1], contactBuf.mtv[2]);
				}
				else
				{
					fprintf(stdout, "Contact: NULL.\n");
				}

			}
			else
			{
				fprintf(stdout, "PsmAddress: 0.\n");
			}
		}

		debug_fflush(stdout);

	}

	return;
}
#else
#define printDebugIonRoute(ionwm, route) do {  } while(0)
#endif


/******************************************************************************
 *
 * \par Function Name:
 *      convert_CtRegistration_from_ion_to_cgr
 *
 * \brief  Convert a Registration contact from ION to this CGR's implementation
 *
 *
 * \par Date Written:
 *     19/02/20
 *
 * \return int
 *
 * \retval   0   Success case
 * \retval  -1   Arguments error
 *
 * \param[in]    *IonContact   The Registration contact in ION.
 * \param[out]   *CgrContact   The Registration contact in this CGR's implementation.
 *
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  19/02/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
int convert_CtRegistration_from_ion_to_cgr(IonCXref *IonContact, Contact *CgrContact)
{
	int result = -1;

	if (IonContact != NULL && CgrContact != NULL)
	{
        CgrContact->regionNbr = IonContact->regionNbr;
		CgrContact->fromNode = IonContact->fromNode;
		CgrContact->toNode = IonContact->toNode;
		CgrContact->fromTime = MAX_POSIX_TIME;
		CgrContact->toTime = MAX_POSIX_TIME;
		CgrContact->type = TypeRegistration;
		CgrContact->xmitRate = 0;
		CgrContact->confidence = 1.0F;

		result = 0;
	}

	return result;
}

/******************************************************************************
 *
 * \par Function Name:
 *      convert_CtScheduled_from_ion_to_cgr
 *
 * \brief  Convert a Scheduled contact from ION to this CGR's implementation
 *
 *
 * \par Date Written:
 *      19/02/20
 *
 * \return int
 *
 * \retval  0   Success case
 * \retval -1   Arguments error
 *
 * \param[in]    *IonContact    The Scheduled contact in ION.
 * \param[out]   *CgrContact    The Scheduled contact in this CGR's implementation.
 * \param[in]    reference_time The reference time used to convert POSIX time in differential time from it.
 *
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  19/02/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
static int convert_CtScheduled_from_ion_to_cgr(IonCXref *IonContact, Contact *CgrContact, time_t reference_time)
{
	int result = -1;

	if (IonContact != NULL && CgrContact != NULL)
	{
	    CgrContact->regionNbr = IonContact->regionNbr;
		CgrContact->fromNode = IonContact->fromNode;
		CgrContact->toNode = IonContact->toNode;
		CgrContact->fromTime = IonContact->fromTime - reference_time;
		CgrContact->toTime = IonContact->toTime - reference_time;
		CgrContact->type = TypeScheduled;
		CgrContact->xmitRate = IonContact->xmitRate;
		CgrContact->confidence = IonContact->confidence;
		// TODO Consider to add MTVs

		result = 0;
	}

	return result;
}

/******************************************************************************
 *
 * \par Function Name:
 *      convert_CtRegistration_from_cgr_to_ion
 *
 * \brief Convert a Registration contact from this CGR's implementation to ION
 *
 *
 * \par Date Written:
 *      19/02/20
 *
 * \return int
 *
 * \retval    0  Success case
 * \retval   -1  Arguments error
 *
 * \param[in]   *CgrContact   The Registration contact in this CGR's implementation.
 * \param[out]  *IonContact   The Registration contact in ION.
 *
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  19/02/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
int convert_CtRegistration_from_cgr_to_ion(Contact *CgrContact, IonCXref *IonContact)
{
	int result = -1;

	if (IonContact != NULL && CgrContact != NULL)
	{
        IonContact->regionNbr = CgrContact->regionNbr;
		IonContact->fromNode = CgrContact->fromNode;
		IonContact->toNode = CgrContact->toNode;
		IonContact->fromTime = MAX_POSIX_TIME;
		IonContact->toTime = MAX_POSIX_TIME;
		IonContact->type = CtRegistration;
		IonContact->xmitRate = 0;
		IonContact->confidence = 1.0F;

		result = 0;
	}

	return result;
}

/******************************************************************************
 *
 * \par Function Name:
 *      convert_CtScheduled_from_cgr_to_ion
 *
 * \brief  Convert a Scheduled contact from this CGR's implementation to ION
 *
 *
 * \par Date Written:
 *      19/02/20
 *
 * \return int
 *
 * \retval   0	Success case
 * \retval  -1  Arguments error
 *
 * \param[in]    *CgrContact    The Scheduled contact in this CGR's implementation.
 * \param[out]   *IonContact    The Scheduled contact in ION.
 * \param[in]    reference_time The reference time used to convert POSIX time in differential time from it.
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  19/02/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
static int convert_CtScheduled_from_cgr_to_ion(Contact *CgrContact, IonCXref *IonContact, time_t reference_time)
{
	int result = -1;

	if (IonContact != NULL && CgrContact != NULL)
	{
        IonContact->regionNbr = CgrContact->regionNbr;
		IonContact->fromNode = CgrContact->fromNode;
		IonContact->toNode = CgrContact->toNode;
		IonContact->fromTime = CgrContact->fromTime + reference_time;
		IonContact->toTime = CgrContact->toTime + reference_time;
		IonContact->type = CtScheduled;
		IonContact->xmitRate = CgrContact->xmitRate;
		IonContact->confidence = CgrContact->confidence;

		result = 0;
	}

	return result;
}

/******************************************************************************
 *
 * \par Function Name:
 *      convert_contact_from_ion_to_cgr
 *
 * \brief Convert a contact from ION to this CGR's implementation
 *
 *
 * \par Date Written:
 *      19/02/20
 *
 * \return int
 *
 * \retval   0  Success case
 * \retval  -1  Arguments error
 * \retval  -2  Unknown contact's type
 *
 * \param[in]   *IonContact    The contact in ION.
 * \param[out]  *CgrContact    The contact in this CGR's implementation.
 * \param[in]   reference_time The reference time used to convert POSIX time in differential time from it.
 *
 * \par	Notes:
 *             1.  Only Scheduled contacts are allowed.
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  19/02/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
static int convert_contact_from_ion_to_cgr(IonCXref *IonContact, Contact *CgrContact, time_t reference_time)
{
	int result = -1;
	if (IonContact != NULL && CgrContact != NULL)
	{
	    if (IonContact->type == CtScheduled)
		{
			result = convert_CtScheduled_from_ion_to_cgr(IonContact, CgrContact, reference_time);
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
 *      convert_contact_from_cgr_to_ion
 *
 * \brief Convert a contact from this CGR's implementation to ION
 *
 *
 * \par Date Written:
 *      19/02/20
 *
 * \return int
 *
 * \retval  0  Success case
 * \retval -1  Arguments error
 * \retval  -2  Unknown contact's type
 *
 * \param[in]    *CgrContact    The contact in this CGR's implementation.
 * \param[out]   *IonContact    The contact in ION.
 * \param[in]    reference_time The reference time used to convert POSIX time in differential time from it.
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  19/02/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
static int convert_contact_from_cgr_to_ion(Contact *CgrContact, IonCXref *IonContact, time_t reference_time)
{
	int result = -1;
	if (IonContact != NULL && CgrContact != NULL)
	{
	    if (CgrContact->type == TypeScheduled)
		{
			result = convert_CtScheduled_from_cgr_to_ion(CgrContact, IonContact, reference_time);
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
 *      convert_range_from_ion_to_cgr
 *
 * \brief  Convert a range from ION to this CGR's implementation
 *
 *
 * \par Date Written:
 *      19/02/20
 *
 * \return int
 *
 * \retval   0   Success case
 * \retval  -1   Arguments error
 *
 * \param[in]    *IonRange  The range in ION.
 * \param[out]   *CgrRange  The range in this CGR's implementation.
 * \param[in]    reference_time The reference time used to convert POSIX time in differential time from it.
 *
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  19/02/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
static int convert_range_from_ion_to_cgr(IonRXref *IonRange, Range *CgrRange, time_t reference_time)
{
	int result = -1;

	if (IonRange != NULL && CgrRange != NULL)
	{
		CgrRange->fromNode = IonRange->fromNode;
		CgrRange->toNode = IonRange->toNode;
		CgrRange->fromTime = IonRange->fromTime - reference_time;
		CgrRange->toTime = IonRange->toTime - reference_time;
		CgrRange->owlt = IonRange->owlt;

		result = 0;
	}

	return result;
}

/******************************************************************************
 *
 * \par Function Name:
 *      convert_range_from_cgr_to_ion
 *
 * \brief  Convert a range from this CGR's implementation to ION
 *
 *
 * \par Date Written:
 *      19/02/20
 *
 * \return int
 *
 * \retval  0  Success case
 * \retval -1  Arguments error
 *
 * \param[in]   *CgrRange       The range in this CGR's implementation.
 * \param[out]  *IonRange       The range in ION.
 * \param[in]   reference_time  The reference time used to convert POSIX time in differential time from it.
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  19/02/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
static int convert_range_from_cgr_to_ion(Range *CgrRange, IonRXref *IonRange, time_t reference_time)
{
	int result = -1;

	if (IonRange != NULL && CgrRange != NULL)
	{
		IonRange->fromNode = CgrRange->fromNode;
		IonRange->toNode = CgrRange->toNode;
		IonRange->fromTime = CgrRange->fromTime + reference_time;
		IonRange->toTime = CgrRange->toTime + reference_time;
		IonRange->owlt = CgrRange->owlt;

		result = 0;
	}

	return result;
}

#if (CGR_AVOID_LOOP > 0)
/******************************************************************************
 *
 * \par Function Name:
 *      get_rgr_ext_block
 *
 * \brief  Get the GeoRoute stored into RGR Extension Block
 *
 *
 * \par Date Written:
 *      23/04/20
 *
 * \return int
 *
 * \retval  0  Success case: GeoRoute found
 * \retval -1  GeoRoute not found
 * \retval -2  System error
 *
 * \param[in]   *bundle    The bundle that contains the RGR Extension Block
 * \param[out]  *resultBlk The GeoRoute extracted from the RGR Extension Block, only in success case.
 *                         The resultBLk must be allocated by the caller.
 *
 * \warning bundle    doesn't have to be NULL
 * \warning resultBlk doesn't have to be NULL
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  23/04/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
static int get_rgr_ext_block(Bundle *bundle, GeoRoute *resultBlk)
{
	Sdr sdr = getIonsdr();
	int result = 0;
	Object extBlockElt;
	Address extBlkAddr;

	OBJ_POINTER(ExtensionBlock, blk);

	/* Step 1 - Check for the presence of RGR extension*/

	if (!(extBlockElt = findExtensionBlock(bundle, EXTENSION_TYPE_RGR, 0, 0, 0)))
	{
		result = -1;
	}
	else
	{
		/* Step 2 - Get deserialized version of RGR extension block*/

		extBlkAddr = sdr_list_data(sdr, extBlockElt);

		GET_OBJ_POINTER(sdr, ExtensionBlock, blk, extBlkAddr);

		result = rgr_read(blk, resultBlk);

		if(result == -1)
		{
			result = -2; // system error
		}
		else if(result < -1)
		{
			result = -1; // geo route not found
		}
		else
		{
			result = 0; // geo route found
		}
	}

	return result;
}
#endif

#if (CGRR && CGRREB)

// path to macro:
// CGRR: Unibo-CGR/core/config.h
// MSR: Unibo-CGR/core/config.h
// WISE_NODE: Unibo-CGR/core/config.h
// CGRREB (from ION's root directory): bpv*/library/ext/cgrr/cgrr.h

#if (WISE_NODE)

/******************************************************************************
 *
 * \par Function Name:
 *      refill_mtv_into_ion
 *
 * \brief Refill contact's MTV of some size passed as argument.
 *
 * \par Date Written:
 * 		11/12/20
 *
 * \return int
 *
 * \retval  0   Contact MTVs updated
 * \retval -1   Arguments error
 * \retval -2   Contact not found
 *
 * \param[in]   regionNbr      Contact plan region
 * \param[in]      fromNode       The sender node of the contact
 * \param[in]      toNode         The receiver node of the contact
 * \param[in]      fromTime       The start time of the contact
 * \param[in]      tolerance      Time tolerance
 * \param[in]      refillSize     The size to add into MTV
 * \param[in]      priority       The upper-bound priority. The refillSize will be added
 *                                into all MTV that refers to (all) less or equal priority.
 * \param[in]      reference_time ION's start time
 *
 * \par Revision History:
 *
 *  DD/MM/YY | AUTHOR          |   DESCRIPTION
 *  -------- | --------------- |  -----------------------------------------------
 *  11/12/20 | L. Persampieri  |   Initial Implementation and documentation.
 *****************************************************************************/
static int refill_mtv_into_ion(uint32_t regionNbr, uvast fromNode, uvast toNode, time_t fromTime, unsigned int tolerance, uvast refillSize, int priority, time_t reference_time)
{
	Sdr sdr = getIonsdr();
	IonVdb *ionvdb = getIonVdb();
	PsmPartition ionwm = getIonwm();
	IonCXref *contact, *contactFound = NULL;
	IonCXref arg;
	IonContact contactBuf;
	Object contactObj;
	PsmAddress elt = 0, contactAddr;
	int result = 0, i, stop;
	unsigned int difference;
	time_t startTime;

	if (priority < 0 || priority > 2)
	{
		// arguments error
		result = -1;
	}
	else if (refillSize == 0)
	{
		result = 0;
	}
	else
	{
		memset(&arg, 0, sizeof(arg));
		arg.regionNbr = regionNbr;
		arg.fromNode = fromNode;
		arg.toNode = toNode;
		stop = 0;

		// search the contact
		for(oK(sm_rbt_search(ionwm, ionvdb->contactIndex, rfx_order_contacts,
						&arg, &elt)); elt && !stop; elt = sm_rbt_next(ionwm, elt))
		{
			contactAddr = sm_rbt_data(ionwm, elt);
			contact = (IonCXref *) psp(ionwm, contactAddr);

			if(contact != NULL && contact->regionNbr == regionNbr && contact->fromNode == fromNode && contact->toNode == toNode)
			{
				startTime = contact->fromTime - reference_time;
				// difference in absolute value
				difference = absolute(startTime - fromTime);

				if(difference <= tolerance)
				{
					contactFound = contact;
					stop = 1;
				}
				else if(startTime > fromTime + tolerance)
				{
					stop = 1;
				}
			}
			else
			{
				stop = 1;
			}

		}

		if (contactFound == NULL)
		{
			// contact not found
			result = -2;
		}
		else
		{
			result = 0;
			contactObj = sdr_list_data(sdr, contactFound->contactElt);

			sdr_stage(sdr, (char *) &contactBuf, contactObj, sizeof(IonContact));

			for (i = priority; i >= 0; i--)
			{
				contactBuf.mtv[i] += refillSize;
			}

			sdr_write(sdr, contactObj, (char *) &contactBuf, sizeof(IonContact));
		}
	}

	return result;

}

/******************************************************************************
 *
 * \par Function Name:
 *      update_mtv_before_reforwarding
 *
 * \brief  This function has effect only if this bundle has to be reforwarded for some reason.
 *
 * \details We refill the contacts' MTVs of the previous route computed for this bundle.
 *          CGRR Ext. Block is required.
 *
 *
 * \par Date Written:
 *      11/12/20
 *
 * \return int
 *
 * \retval  0  Success case: MTV updated
 * \retval -1  Some error occurred
 *
 * \param[in]   *bundle        The bundle to forward
 * \param[in]   *cgrrExtBlk    The ExtensionBlock type of CGRR
 * \param[in]   *resultBlk     The CGRRouteBlock extracted from the CGRR Extension Block.
 * \param[in]   reference_time The reference time used to convert POSIX time in differential time from it.
 * \param[in]   regionNbr      Contact plan region
 *
 * \warning bundle    doesn't have to be NULL
 * \warning cgrrExtBlk doesn't have to be NULL
 * \warning cgrrBlk   doesn't have to be NULL
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  11/12/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
static int update_mtv_before_reforwarding(Bundle *bundle, ExtensionBlock *cgrrExtBlk, CGRRouteBlock *cgrrBlk, time_t reference_time, uint32_t regionNbr)
{
	int result = -1;
	uvast size;
	int temp;
	CGRRoute *lastRoute = NULL;
	CGRRHop *currentHop;
	int i;
	unsigned int timeTolerance;
	int priority;

	temp = -1;

	if(!(bundle->ancillaryData.flags & BP_MINIMUM_LATENCY))
	{
		// bundle isn't critical
		temp = cgrr_getUsedEvc(bundle, cgrrExtBlk, &size);
	}
	if (temp >= 0 && temp != 2)
	{
			if (temp == 1)
			{
				size = bundle->payload.length;
			}

			if (size == 0)
			{
				return 0;
			}

			// now get the last route from CGRRouteBlock

			if(cgrrBlk->recRoutesLength == 0)
			{
				lastRoute = &(cgrrBlk->originalRoute);
			}
			else
			{
				lastRoute = &(cgrrBlk->recomputedRoutes[cgrrBlk->recRoutesLength - 1]);
			}

			result = -1;

#if (MSR == 1)
			timeTolerance = MSR_TIME_TOLERANCE;
#else
			timeTolerance = 0;
#endif

			priority = bundle->priority;

			if(lastRoute != NULL && lastRoute->hopCount > 0)
			{
				result = 0;

				for(i = 0; i < lastRoute->hopCount; i++)
				{
					currentHop = &(lastRoute->hopList[i]);

					// refill MTV into Unibo-CGR's contact graph
					refill_mtv((unsigned long) regionNbr, (unsigned long long) currentHop->fromNode,
							(unsigned long long) currentHop->toNode,
							currentHop->fromTime, timeTolerance, (unsigned int) size,
							priority);

					// refill MTV into ION's contact graph
					refill_mtv_into_ion(regionNbr, currentHop->fromNode, currentHop->toNode,
							currentHop->fromTime, timeTolerance, size,
							priority, reference_time);
				}
			}
	}

	return result;
}

#endif

#if (WISE_NODE || MSR)

/******************************************************************************
 *
 * \par Function Name:
 *      get_cgrr_ext_block
 *
 * \brief  Get the CGRRouteBlock stored into CGRR Extension Block
 *
 *
 * \par Date Written:
 *      23/04/20
 *
 * \return int
 *
 * \retval  0  Success case: CGRRouteBlock found
 * \retval -1  CGRRouteBlock not found
 * \retval -2  System error
 *
 * \param[in]   *bundle        The bundle that contains the CGRR Extension Block
 * \param[in]   reference_time The reference time used to convert POSIX time in differential time from it.
 * \param[out]  *extBlk        The ExtensionBlock type of CGRR
 * \param[out] **resultBlk     The CGRRouteBlock extracted from the CGRR Extension Block, only in success case.
 *                             The resultBLk will be allocated by this function.
 *
 * \warning bundle    doesn't have to be NULL
 * \warning resultBlk doesn't have to be NULL
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  23/04/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
static int get_cgrr_ext_block(Bundle *bundle, time_t reference_time, ExtensionBlock *extBlk, CGRRouteBlock **resultBlk)
{
	Sdr sdr = getIonsdr();
	int result = 0, i, j;
	Object extBlockElt;
	Address extBlkAddr;
	CGRRouteBlock* cgrrBlk;
	CGRRoute *route;

	OBJ_POINTER(ExtensionBlock, blk);

	/* Step 1 - Check for the presence of CGRR extension*/

	if (!(extBlockElt = findExtensionBlock(bundle, EXTENSION_TYPE_CGRR, 0, 0, 0)))
	{
		result = -1;
	}
	else
	{

		/* Step 2 - Get deserialized version of CGRR extension block*/

		extBlkAddr = sdr_list_data(sdr, extBlockElt);

		GET_OBJ_POINTER(sdr, ExtensionBlock, blk, extBlkAddr);

		cgrrBlk = (CGRRouteBlock*) MTAKE(sizeof(CGRRouteBlock));

		if (cgrrBlk == NULL)
		{
			result = -2;
		}
		else
		{
			if (cgrr_getCGRRFromExtensionBlock(blk, cgrrBlk) < 0)
			{
				result = -2;
				MRELEASE(cgrrBlk);
			}
			else
			{
				result = 0;
				*resultBlk = cgrrBlk;
				memcpy(extBlk, blk, sizeof(ExtensionBlock));

			//	printCGRRouteBlock(cgrrBlk);

				route = &(cgrrBlk->originalRoute);

				for(i = 0; i < route->hopCount; i++)
				{
					route->hopList[i].fromTime -= reference_time;
				}

				for(j = 0; j < cgrrBlk->recRoutesLength; j++)
				{
					route = &(cgrrBlk->recomputedRoutes[j]);

					for(i = 0; i < route->hopCount; i++)
					{
						route->hopList[i].fromTime -= reference_time;
					}
				}
			}
		}
	}


	return result;
}

#endif

/******************************************************************************
 *
 * \par Function Name:
 *      CGRR_management
 *
 * \brief  Update CGRR after routing.
 *
 *
 * \par Date Written:
 *      25/09/20
 *
 * \return int
 *
 * \retval  0  Success case
 * \retval -1  Some error
 * \retval -2  System error
 * \retval -3  Nothing to do
 *
 * \param[in]   ionwm          ION Working Memory
 * \param[in]   bestRoutes     Routes selected from routing.
 * \param[in]   *bundle        The bundle that contains the CGRR Ext. Block
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  25/09/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
static int CGRR_management(PsmPartition ionwm, Lyst bestRoutes, Bundle *bundle)
{
#if(WISE_NODE)
	LystElt lyst_elt;
	CGRRoute cgrr_route;
	CgrRoute *route;
	Sdr sdr;
	CurrentCallSAP *currSap;
	Object extBlockElt;
	Address extBlkAddr;
#endif
	int result = -3, temp;

	if (lyst_length(bestRoutes) == 0)
	{
		return -3;
	}

#if(MSR)
	result = -1;

    if(bundle->ancillaryData.flags & BP_MINIMUM_LATENCY) {
    	// Critical bundle (MSR discouraged)
    	return -1;
    }

	if(get_last_call_routing_algorithm() == msr)
	{
		// update the MSR route in CGRR extension block

		temp = updateLastCgrrRoute(bundle);

		if(temp == -2)
		{
			// System error
			result = -2;
		}
		else if(temp == -1 || temp == -3)
		{
			// some error
			result = -1;
		}
		else
		{
			result = 0;
		}

	}
#if(WISE_NODE)
	else
	{
		// replace the MSR route in CGRR extension block

		lyst_elt = lyst_first(bestRoutes);
		if (lyst_elt)
		{
			route = (CgrRoute *) lyst_data(lyst_elt);
		}
		else
		{
			return -1;
		}

		temp = getCGRRoute(route, &cgrr_route);


		if(temp == -1)
		{
			result = -1;
		}

		else if(temp == -2)
		{
			result = -2;
		}
		else
		{
			temp = storeMsrRoute(&cgrr_route, bundle);

			if(temp == -2)
			{
				// System error
				result = -2;
			}
			else if(temp == -1)
			{
				// some error
				result = -1;
			}
			else
			{
				result = 0;
			}


			MRELEASE(cgrr_route.hopList);
		}
	}
#endif
#elif (WISE_NODE)
	int stop = 0;
	result = 0;
	for (lyst_elt = lyst_first(bestRoutes); lyst_elt && !stop; lyst_elt = lyst_next(lyst_elt))
	{
		route = (CgrRoute*) lyst_data(lyst_elt);

		temp = getCGRRoute(route, &cgrr_route);

		if(temp == -2)
		{
			result = -2;
			stop = 1;
		}
		else if(temp == 0 && cgrr_route.hopCount > 0)
		{
			temp = saveRouteToExtBlock(cgrr_route.hopCount, cgrr_route.hopList,
					bundle);

			if (temp == -2)
			{
				// some error
				result = -1;
				stop = 1;
			}
			else if (temp == -1)
			{
				// System error
				result = -2;
				stop = 1;
			}

			MRELEASE(cgrr_route.hopList);
		}
	}

#endif

#if (WISE_NODE)
	if (result == 0)
	{
		sdr = getIonsdr();
		currSap = get_current_call_sap(NULL);

		OBJ_POINTER(ExtensionBlock, blk);

		if ((extBlockElt = findExtensionBlock(bundle, EXTENSION_TYPE_CGRR, 0, 0, 0)))
		{
			extBlkAddr = sdr_list_data(sdr, extBlockElt);

			GET_OBJ_POINTER(sdr, ExtensionBlock, blk, extBlkAddr);

			cgrr_setUsedEvc(bundle, blk, currSap->uniboCgrBundle->evc);
		}
	}
#endif

	return result;

}
#endif

/******************************************************************************
 *
 * \par Function Name:
 *      convert_bundle_from_ion_to_cgr
 *
 * \brief Convert the characteristics of the bundle from ION to
 *        this CGR's implementation and initialize all the
 *        bundle fields used by the CGR.
 *
 *
 * \par Date Written:
 *      19/02/20
 *
 * \return int
 *
 * \retval  0  Success case
 * \retval -1  Arguments error
 * \retval -2  MWITHDRAW error
 *
 * \param[in]    toNode             The destination ipn node for the bundle
 * \param[in]    current_time       The current time, in differential time from reference_time
 * \param[in]    reference_time     The reference time used to convert POSIX time in differential time from it.
 * \param[in]    *IonBundle         The bundle in ION
 * \param[out]   *CgrBundle         The bundle in this CGR's implementation.
 *
 * \par	Notes:
 *             1. This function prints the ID of the bundle in the main log file.
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  19/02/20 | L. Persampieri  |  Initial Implementation and documentation.
 *  24/03/20 | L. Persampieri  |  Added geoRouteString
 *****************************************************************************/
static int convert_bundle_from_ion_to_cgr(unsigned long long toNode, time_t current_time, time_t reference_time, Bundle *IonBundle, CgrBundle *CgrBundle)
{
	int result = -1;
	time_t offset;
#if (CGRR && CGRREB && (WISE_NODE || MSR))
	CGRRouteBlock *cgrrBlk;
	ExtensionBlock cgrrExtBlk;
#endif
#if (CGR_AVOID_LOOP > 0)
	GeoRoute geoRoute;
#endif

	if (IonBundle != NULL && CgrBundle != NULL)
	{
        uint32_t regionNbr = 0;
        oK(ionRegionOf(getOwnNodeNbr(), (uvast) toNode, &regionNbr));
        CgrBundle->regionNbr = (unsigned long) regionNbr;

		CgrBundle->terminus_node = toNode;

#if (CGRR && CGRREB && (WISE_NODE || MSR))
		CgrBundle->msrRoute = NULL;
		result = get_cgrr_ext_block(IonBundle, reference_time, &cgrrExtBlk, &cgrrBlk);

		if(result == 0)
		{
#if (WISE_NODE)
			update_mtv_before_reforwarding(IonBundle, &cgrrExtBlk, cgrrBlk, reference_time, regionNbr);
#endif
#if (MSR)
			result = set_msr_route(current_time, cgrrBlk, CgrBundle);
#endif
			releaseCgrrBlkMemory(cgrrBlk);
		}
#endif
#if (CGR_AVOID_LOOP > 0)
		if(result != -2)
		{
			result = get_rgr_ext_block(IonBundle, &geoRoute);
			if(result == 0)
			{
				result = set_geo_route_list(geoRoute.nodes, CgrBundle);
				MRELEASE(geoRoute.nodes);
			}
		}
#endif
		if (result != -2)
		{
			CLEAR_FLAGS(CgrBundle->flags); //reset previous mask

            if(IonBundle->ancillaryData.flags & BP_MINIMUM_LATENCY)
            {
            	SET_CRITICAL(CgrBundle);
            }

			if (!(IS_CRITICAL(CgrBundle)) && IonBundle->returnToSender)
			{
				SET_BACKWARD_PROPAGATION(CgrBundle);
			}
			if(!(IonBundle->bundleProcFlags & BDL_DOES_NOT_FRAGMENT))
			{
				SET_FRAGMENTABLE(CgrBundle);
			}

			//TODO search probe field in ION's bundle...

			CgrBundle->ordinal = (unsigned int) IonBundle->ordinal;

			//size computation ported by ION 3.7.0
			CgrBundle->size = NOMINAL_PRIMARY_BLKSIZE + IonBundle->dictionaryLength
					+ IonBundle->extensionsLength[PRE_PAYLOAD] + IonBundle->payload.length
					+ IonBundle->extensionsLength[POST_PAYLOAD];

			CgrBundle->evc = computeBundleEVC(CgrBundle->size); // SABR 2.4.3

			offset = IonBundle->id.creationTime.seconds + EPOCH_2000_SEC - reference_time;

			CgrBundle->expiration_time = IonBundle->expirationTime
					- IonBundle->id.creationTime.seconds + offset;
			CgrBundle->sender_node = IonBundle->clDossier.senderNodeNbr;
			CgrBundle->priority_level = IonBundle->priority;

			CgrBundle->dlvConfidence = IonBundle->dlvConfidence;

			if(IonBundle->bundleProcFlags & BDL_IS_FRAGMENT)
			{
				CgrBundle->id.fragment_length = IonBundle->payload.length;
			}
			else
			{
				CgrBundle->id.fragment_length = 0;
			}

			CgrBundle->id.source_node = IonBundle->id.source.c.nodeNbr;
			CgrBundle->id.creation_timestamp = IonBundle->id.creationTime.seconds;
			CgrBundle->id.sequence_number = IonBundle->id.creationTime.count;
			CgrBundle->id.fragment_offset = IonBundle->id.fragmentOffset;

			print_log_bundle_id(CgrBundle->id.source_node,
					CgrBundle->id.creation_timestamp, CgrBundle->id.sequence_number,
					CgrBundle->id.fragment_length, CgrBundle->id.fragment_offset);

			if(IonBundle->bundleProcFlags & BDL_IS_FRAGMENT)
			{
				// this bundle is a fragment. we print the payload of the original bundle.
				writeLog("Payload length: %u.", IonBundle->totalAduLength);
			}
			else
			{
				writeLog("Payload length: %u.", IonBundle->payload.length);
			}

			result = 0;
		}
	}

	return result;
}

/******************************************************************************
 *
 * \par Function Name:
 *      convert_scalar_from_ion_to_cgr
 *
 * \brief  Convert a scalar type from ION to this CGR's implementation.
 *
 *
 * \par Date Written:
 *      19/02/20
 *
 * \return int
 *
 * \retval   0  Success case
 * \retval  -1  Arguments error
 *
 * \param[in]   *ion_scalar   The scalar type in ION.
 * \param[out]  *cgr_scalar   The scalar type in this CGR's implementation
 *
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  19/02/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
static int convert_scalar_from_ion_to_cgr(Scalar *ion_scalar, CgrScalar *cgr_scalar)
{
	int result = -1;

	if (ion_scalar != NULL && cgr_scalar != NULL)
	{
		cgr_scalar->gigs = ion_scalar->gigs;
		cgr_scalar->units = ion_scalar->units;
		result = 0;
	}

	return result;
}

/******************************************************************************
 *
 * \par Function Name:
 *      convert_scalar_from_cgr_to_ion
 *
 * \brief  Convert a scalar type from this CGR's implementation to ION.
 *
 *
 * \par Date Written:
 *      19/02/20
 *
 * \return int
 *
 * \retval   0  Success case
 * \retval  -1  Arguments error
 *
 * \param[in]   *cgr_scalar   The scalar type in this CGR's implementation.
 * \param[out]  *ion_scalar   The scalar type in ION.
 *
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  19/02/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
static int convert_scalar_from_cgr_to_ion(CgrScalar *cgr_scalar, Scalar *ion_scalar)
{
	int result = -1;

	if (ion_scalar != NULL && cgr_scalar != NULL)
	{
		ion_scalar->gigs = cgr_scalar->gigs;
		ion_scalar->units = cgr_scalar->units;
		result = 0;
	}

	return result;
}

/******************************************************************************
 *
 * \par Function Name:
 *      convert_hops_list_from_cgr_to_ion
 *
 * \brief  Convert a list of contacts from this CGR's implementation to ION.
 * 
 * \details  This function is thinked to convert the hops list of a Route
 *           and only for this scope.
 *
 *
 * \par Date Written:
 *      19/02/20
 *
 * \return  int
 * 
 * \retval  ">= 0"  Number of contacts converted
 * \retval     -1   CGR's contact NULL
 * \retval     -2   Memory allocation error
 * \retval     -3   Contact not found in the ION's contacts graph
 *
 * \param[in]   reference_time  The reference time used to convert POSIX time in differential time from it.
 * \param[in]   ionwm           The partition of the ION's contacts graph
 * \param[in]   *ionvdb         The ion's volatile database
 * \param[in]   CgrHops         The list of contacts of this CGR's implementation
 * \param[out]  IonHops         The list of contact in ION's format.
 *
 * \warning ionvdb doesn't have to be NULL
 * \warning CgrHops doesn't have to be NULL
 * \warning IonHops doesn't have to be 0
 *
 * \par	Notes:
 *                1.    All the contacts will be searched in the ION's contacts graph, and
 *                      then the contact found will be added in the list.
 *                2.    The ION's list mantains the same order of the CGR's list.
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  19/02/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
static int convert_hops_list_from_cgr_to_ion(time_t reference_time, PsmPartition ionwm, IonVdb *ionvdb, List CgrHops,
		PsmAddress IonHops)
{
	ListElt *elt;
	IonCXref IonContact, *IonTreeContact;
	PsmAddress tree_node, citation, contactAddr;
	int result = 0;

	for (elt = CgrHops->first; elt != NULL && result >= 0; elt = elt->next)
	{
		if (convert_contact_from_cgr_to_ion((Contact*) elt->data, &IonContact, reference_time) == 0)
		{
			tree_node = sm_rbt_search(ionwm, ionvdb->contactIndex, rfx_order_contacts, &IonContact,
					0);
			if (tree_node != 0)
			{
				contactAddr = sm_rbt_data(ionwm, tree_node);
				IonTreeContact = (IonCXref*) psp(ionwm, contactAddr);
				if (IonTreeContact != NULL)
				{
					citation = sm_list_insert_last(ionwm, IonHops, contactAddr);

					if (citation == 0)
					{
						result = -2;
					}
					else
					{
						result++;
					}
				}
				else
				{
					result = -3;
				}
			}
			else
			{
				result = -3;
			}
		}
		else
		{
			result = -1;
		}
	}

	return result;
}

/******************************************************************************
 *
 * \par Function Name:
 *      convert_routes_from_cgr_to_ion
 *
 * \brief Convert a list of routes from CGR's format to ION's format
 *
 *
 * \par Date Written: 
 *      19/02/20
 *
 * \return int 
 *
 * \retval   0  All routes converted
 * \retval  -1  CGR's contact points to NULL
 * \retval  -2  Memory allocation error
 *
 * \param[in]    reference_time   The reference time used to convert POSIX time in differential time from it.
 * \param[in]     ionwm           The partition of the ION's contacts graph
 * \param[in]     *ionvdb         The ION's volatile database
 * \param[in]     *terminusNode   The node for which the routes have been computed
 * \param[in]     evc             Bundle's estimated volume consumption
 * \param[in]     cgrRoutes       The list of routes in CGR's format
 * \param[out]    IonRoutes       The list converted (only if return value is 0)
 *
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  19/02/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
static int convert_routes_from_cgr_to_ion(time_t reference_time, PsmPartition ionwm, IonVdb *ionvdb, IonNode *terminusNode,
		long unsigned int evc, List cgrRoutes, Lyst IonRoutes)
{
	ListElt *elt;
	PsmAddress hops;
	CgrRoute *IonRoute = NULL;
	Route *current;
	int tmp;
	CurrentCallSAP *currSap = get_current_call_sap(NULL);
	CgrRoute *newRoute;
	LystElt nextRouteElt;

	int result = 0;

	nextRouteElt = lyst_first(currSap->routes);

	for (elt = cgrRoutes->first; elt != NULL && result >= 0; elt = elt->next)
	{
		if (elt->data != NULL)
		{
			current = (Route*) elt->data;

			if (nextRouteElt == NULL)
			{
				newRoute = MTAKE(sizeof(CgrRoute));

				if(newRoute == NULL)
				{
					return -2;
				}

				memset((char*) newRoute, 0, sizeof(CgrRoute));

				newRoute->hops = sm_list_create(ionwm);
				nextRouteElt = lyst_insert_last(currSap->routes, newRoute);

				if (nextRouteElt == NULL || newRoute->hops == 0)
				{
					return -2;
				}

			}
			IonRoute = lyst_data(nextRouteElt);
			hops = IonRoute->hops;
			nextRouteElt = lyst_next(nextRouteElt);

			if (IonRoute != NULL && hops != 0)
			{
				sm_list_clear(ionwm, hops, NULL, NULL);
				IonRoute->toNodeNbr = current->neighbor;
				IonRoute->fromTime = current->fromTime + reference_time;
				IonRoute->toTime = current->toTime + reference_time;
				IonRoute->arrivalConfidence = current->arrivalConfidence;
				IonRoute->arrivalTime = current->arrivalTime + reference_time;
				IonRoute->maxVolumeAvbl = current->routeVolumeLimit;
				IonRoute->bundleECCC = evc;
				IonRoute->eto = current->eto + reference_time;
				IonRoute->pbat = current->pbat + reference_time;
				convert_scalar_from_cgr_to_ion(&(current->committed),
						&(IonRoute->protected));
				convert_scalar_from_cgr_to_ion(&(current->overbooked),
						&(IonRoute->overbooked));

				tmp = convert_hops_list_from_cgr_to_ion(reference_time, ionwm,
						ionvdb, current->hops, hops);
				if (tmp >= 0)
				{
					IonRoute->hops = hops;
					printDebugIonRoute(ionwm, IonRoute);
					if (lyst_insert_last(IonRoutes, (void*) IonRoute) == NULL)
					{
						result = -2;
					}
				}
				else if (tmp == -3)
				{
					writeLog(
							"(Interface) Skipped route to neighbor %llu, conversion failed.",
							current->neighbor);
				}
				else
				{
					result = -2;
				}
			}
			else
			{
				result = -2;
			}
		}
		else
		{
			result = -1;
		}
	}

	return result;
}

/******************************************************************************
 *
 * \par Function Name:
 *      convert_PsmAddress_to_IonCXref
 *
 * \brief  Convert a PsmAddress to ION's contact type
 *
 *
 * \par Date Written:
 *      19/02/20
 *
 * \return IonCXref*
 *
 * \retval  IonCXref*	The contact converted
 * \retval  NULL        If the address is a NULL pointer
 *
 * \param[in]  ionwm     The partition of the ION's contacts graph
 * \param[in]  address   The address of the contact in the partition
 *
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  19/02/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
static IonCXref* convert_PsmAddress_to_IonCXref(PsmPartition ionwm, PsmAddress address)
{
	IonCXref *contact = NULL;

	if (address != 0)
	{
		contact = psp(ionwm, address);
	}

	return contact;
}

/******************************************************************************
 *
 * \par Function Name:
 *      convert_PsmAddress_to_IonRXref
 *
 * \brief Convert a PsmAddress to ION's range type
 *
 *
 * \par Date Written:
 *      19/02/20
 *
 * \return IonRXref*
 *
 * \retval  IonRXref*  The contact converted
 * \retval  NULL       If the address is a NULL pointer
 *
 * \param[in]   ionwm     The partition of the ION's ranges graph
 * \param[in]   address   The address of the range in the partition
 *
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  19/02/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
static IonRXref* convert_PsmAddress_to_IonRXref(PsmPartition ionwm, PsmAddress address)
{
	IonRXref *range = NULL;

	if (address != 0)
	{
		range = psp(ionwm, address);
	}

	return range;
}

/******************************************************************************
 *
 * \par Function Name:
 *      add_contact
 *
 * \brief  Add a contact to the contacts graph of this CGR's implementation.
 *
 *
 * \par Date Written:
 *      19/02/20
 *
 * \return int
 *
 * \retval   1   Contact added
 * \retval   0   Contact's arguments error
 * \retval  -1   The contact overlaps with other contacts
 * \retval  -2   MWITHDRAW error
 * \retval  -3   ION's contact points to NULL
 *
 * \param[in]   *ContactInION     The contact in ION's format
 * \param[in]    reference_time   The reference time used to convert POSIX time in differential time from it.
 *
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  19/02/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
static int add_contact(IonCXref *ContactInION, time_t reference_time)
{
	Contact CgrContact;
	int result;
	double mtv[3];
#if (GET_MTV_FROM_SDR)
	Sdr sdr = getIonsdr();
	Object contactObj;
	IonContact contactBuf;
#endif

	result = convert_contact_from_ion_to_cgr(ContactInION, &CgrContact, reference_time);

	if (result == 0)
	{

#if (GET_MTV_FROM_SDR)
		// TODO CONSIDER TO MOVE MTVs INTO CONVERT CONTACT FUNCTION
		sdr = getIonsdr();
		contactObj = sdr_list_data(sdr, ContactInION->contactElt);
		sdr_read(sdr, (char *) &contactBuf, contactObj, sizeof(IonContact));
		mtv[0] = contactBuf.mtv[0];
		mtv[1] = contactBuf.mtv[1];
		mtv[2] = contactBuf.mtv[2];

		// Try to add contact
		// Use the MTV passed as argument
		result = addContact(CgrContact.regionNbr, CgrContact.fromNode, CgrContact.toNode, CgrContact.fromTime,
				CgrContact.toTime, CgrContact.xmitRate, CgrContact.confidence, COPY_MTV, mtv);

		if(result >= 1)
		{
			// result == 2 : xmitRate revised, consider it as "new contact"
			result = 1;
		}
#else
		// Initialize to known value...
		mtv[0] = 0;
		mtv[1] = 0;
		mtv[2] = 0;

		// Try to add contact
		// Compute MTV as [xmitRate * (toTime - fromTime)]
		result = addContact(CgrContact.regionNbr, CgrContact.fromNode, CgrContact.toNode, CgrContact.fromTime,
				CgrContact.toTime, CgrContact.xmitRate, CgrContact.confidence, DO_NOT_COPY_MTV, mtv);
		if(result >= 1)
		{
			// result == 2 : xmitRate revised, consider it as "new contact"
			result = 1;
		}
#endif

	}
	else
	{
		result = -3;
	}

	return result;
}

/**
 *
 *
 * TODO consider to add a revise contact function, to change contact's confidence and xmitRate
 *
 *
 */

/******************************************************************************
 *
 * \par Function Name:
 *      add_new_contacts
 *
 * \brief  Add all new contacts of the ION's contacts graph to the
 *         contacts graph of thic CGR's implementation.
 *
 * \details Only for Registration and Scheduled contacts.
 *
 *
 * \par Date Written:
 *      19/02/20
 *
 * \return int
 *
 * \retval  ">= 0"  Number of contacts added to the contacts graph
 * \retval     -2   MWITHDRAW error
 *
 * \param[in]   ionwm          The ION's contacts graph partition
 * \param[in]   *ionvdb        The ION's volatile database
 * \param[in]   reference_time The reference time used to convert POSIX time in differential time from it.
 *
 * \warning ionvdb doesn't have to be NULL.
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  19/02/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
static int add_new_contacts(PsmPartition ionwm, IonVdb *ionvdb, time_t reference_time)
{
	int result = 0, totAdded = 0, stop = 0;
	PsmAddress nodeAddr;
	IonCXref *currentIonContact = NULL;

	for (nodeAddr = sm_rbt_first(ionwm, ionvdb->contactIndex); nodeAddr != 0 && !stop; nodeAddr =
			sm_rbt_next(ionwm, nodeAddr))
	{
		currentIonContact = convert_PsmAddress_to_IonCXref(ionwm, sm_rbt_data(ionwm, nodeAddr));

		if (currentIonContact->type == CtRegistration || currentIonContact->type == CtScheduled)
		{
			result = add_contact(currentIonContact, reference_time);

			if (result == 1)
			{
				totAdded++;
			}
			else if (result == -2) //MWITHDRAW error
			{
				stop = 1;
			}
		}
	}

	if (!stop)
	{
		result = totAdded;
	}

	return result;
}

/******************************************************************************
 *
 * \par Function Name:
 *      remove_deleted_contacts
 *
 * \brief  Remove all the contacts that are not anymore
 *         in the ION's contacts graph from the contacts graph
 *         of this CGR's implementation.
 *
 *
 * \par Date Written:
 *      19/02/20
 *
 * \return int
 *
 * \retval  ">= 0"  Number of contacts deleted from the contacts graph
 *
 * \param[in]  ionwm          The ION's contacts graph partition
 * \param[in]  *ionvdb        The ION's volatile database
 * \param[in]  reference_time The reference time used to convert POSIX time in differential time from it.
 *
 * \warning ionvdb doesn't have to be NULL.
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  19/02/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
static int remove_deleted_contacts(PsmPartition ionwm, IonVdb *ionvdb, time_t reference_time)
{
	int result = 0;
	Contact *CgrContact, *nextCgrContact;
	RbtNode *node;
	IonCXref IonContact;

	CgrContact = get_first_contact(&node);
	while (CgrContact != NULL)
	{
		nextCgrContact = get_next_contact(&node);
		convert_contact_from_cgr_to_ion(CgrContact, &IonContact, reference_time);

		if (sm_rbt_search(ionwm, ionvdb->contactIndex, rfx_order_contacts, &IonContact, 0) == 0)
		{
			remove_contact_elt_from_graph(CgrContact);
			result++;
		}
		CgrContact = nextCgrContact;
	}

	return result;
}
/******************************************************************************
 *
 * \par Function Name:
 *      add_range
 *
 * \brief  Add a range to the ranges graph of this CGR's implementation.
 *
 *
 * \par Date Written:
 *      19/02/20
 *
 * \return int
 *
 * \retval   1   Range added
 * \retval   0   Range's arguments error
 * \retval  -1   The range overlaps with other ranges
 * \retval  -2   MWITHDRAW error
 * \retval  -3   ION's range points to NULL
 *
 * \param[in]  *IonRange      The range in ION's format
 * \param[in]  reference_time The reference time used to convert POSIX time in differential time from it.
 *
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  19/02/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
static int add_range(IonRXref *IonRange, time_t reference_time)
{
	Range CgrRange;
	int result;

	result = convert_range_from_ion_to_cgr(IonRange, &CgrRange, reference_time);

	if (result == 0)
	{
		result = addRange(CgrRange.fromNode, CgrRange.toNode, CgrRange.fromTime, CgrRange.toTime,
				CgrRange.owlt);
		if(result >= 1)
		{
			// result == 2 : owlt revised, consider it as "new range"
			result = 1;
		}
	}
	else
	{
		result = -3;
	}

	return result;
}

/******************************************************************************
 *
 * \par Function Name:
 *      add_new_rangess
 *
 * \brief  Add all new ranges of the ION's ranges graph to the
 *         ranges graph of thic CGR's implementation.
 *
 *
 * \par Date Written:
 *      19/02/20
 *
 * \return int
 * 
 * \retval  ">= 0"   Number of ranges added to the ranges graph
 * \retval     -2    MWITHDRAW error
 *
 * \param[in]  ionwm          The ION's ranges graph partition
 * \param[in]  *ionvdb        The ION's volatile database
 * \param[in]  reference_time The reference time used to convert POSIX time in differential time from it.
 *
 * \warning ionvdb doesn't have to be NULL.
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  19/02/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
static int add_new_ranges(PsmPartition ionwm, IonVdb *ionvdb, time_t reference_time)
{
	int result = 0, totAdded = 0, stop = 0;
	PsmAddress nodeAddr;
	IonRXref *currentIonRange = NULL;

	for (nodeAddr = sm_rbt_first(ionwm, ionvdb->rangeIndex); nodeAddr != 0 && !stop; nodeAddr =
			sm_rbt_next(ionwm, nodeAddr))
	{
		currentIonRange = convert_PsmAddress_to_IonRXref(ionwm, sm_rbt_data(ionwm, nodeAddr));
		result = add_range(currentIonRange, reference_time);

		if (result >= 0)
		{
			totAdded++;
		}
		else if (result == -2) //MWITHDRAW error
		{
			stop = 1;
		}
	}

	if (!stop)
	{
		result = totAdded;
	}

	return result;
}

/******************************************************************************
 *
 * \par Function Name:
 *      remove_deleted_contacts
 *
 * \brief  Remove all the ranges that are not anymore
 *         in the ION's ranges graph from the ranges graph
 *         of this CGR's implementation.
 *
 *
 * \par Date Written:
 *      19/02/20
 *
 * \return int
 *
 * \retval  ">= 0"  Number of ranges deleted from the ranges graph
 *
 * \param[in]   ionwm          The ION's ranges graph partition
 * \param[in]   *ionvdb        The ION's volatile database
 * \param[in]   reference_time The reference time used to convert POSIX time in differential time from it.
 *
 * \warning ionvdb doesn't have to be NULL.
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  19/02/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
static int remove_deleted_ranges(PsmPartition ionwm, IonVdb *ionvdb, time_t reference_time)
{
	int result = 0;
	Range *CgrRange, *nextCgrRange;
	RbtNode *node;
	IonRXref IonRange;

	CgrRange = get_first_range(&node);
	while (CgrRange != NULL)
	{
		nextCgrRange = get_next_range(&node);
		convert_range_from_cgr_to_ion(CgrRange, &IonRange, reference_time);

		if (sm_rbt_search(ionwm, ionvdb->rangeIndex, rfx_order_ranges, &IonRange, 0) == 0)
		{
			remove_range_elt_from_graph(CgrRange);
			result++;
		}
		CgrRange = nextCgrRange;
	}

	return result;
}

/******************************************************************************
 *
 * \par Function Name:
 *      update_contact_plan
 *
 * \brief  Check if the ION's contact plan changed and in affermative case
 *         update the CGR's contact plan.
 *
 *
 * \par Date Written:
 *      19/02/20
 *
 * \return int
 *
 * \retval   0  Contact plan has been changed and updated
 * \retval  -1  Contact plan isn't changed
 * \retval  -2  MWITHDRAW error
 *
 * \param[in]   ionwm           The ION's contact plan partition
 * \param[in]   *ionvdb         The ION's volatile database
 * \param[in]   reference_time  The reference time used to convert POSIX time in differential time from it.
 *
 * \warning ionvdb doesn't have to be NULL.
 * \warning ionwm  doesn't have to be NULL.
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  19/02/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
static int update_contact_plan(PsmPartition ionwm, IonVdb *ionvdb, time_t reference_time)
{
	int result = -1;
	int result_contacts, result_ranges;
	ContactPlanSAP cpSap = get_contact_plan_sap(NULL);

	if (ionvdb->lastEditTime.tv_sec > cpSap.contactPlanEditTime.tv_sec
			|| (ionvdb->lastEditTime.tv_sec == cpSap.contactPlanEditTime.tv_sec
					&& ionvdb->lastEditTime.tv_usec > cpSap.contactPlanEditTime.tv_usec))
	{

		writeLog("#### Contact plan modified ####");

		// TODO consider to remove all contacts and ranges and then copy the ION's RBTs
		// TODO maybe it is more efficient
		// TODO (so discard all routes and nodes here)

		result_contacts = remove_deleted_contacts(ionwm, ionvdb, reference_time);
		result_ranges = remove_deleted_ranges(ionwm, ionvdb, reference_time);
#if (LOG == 1)
		if (result_contacts > 0)
		{
			writeLog("Deleted %d contacts.", result_contacts);
		}
		if (result_ranges > 0)
		{
			writeLog("Deleted %d ranges.", result_ranges);
		}
#endif
		result_contacts = add_new_contacts(ionwm, ionvdb, reference_time);
		result_ranges = add_new_ranges(ionwm, ionvdb, reference_time);
#if (LOG == 1)
		if (result_contacts > 0)
		{
			writeLog("Added %d contacts.", result_contacts);
		}
		if (result_ranges > 0)
		{
			writeLog("Added %d ranges.", result_ranges);
		}
#endif
		if (result_contacts == -2 || result_ranges == -2) //MWITHDRAW error
		{
			result = -2;
		}
		else
		{
			result = 0;
		}

		cpSap.contactPlanEditTime.tv_sec = ionvdb->lastEditTime.tv_sec;
		cpSap.contactPlanEditTime.tv_usec = ionvdb->lastEditTime.tv_usec;

		set_time_contact_plan_updated(ionvdb->lastEditTime.tv_sec, ionvdb->lastEditTime.tv_usec);

		writeLog("###############################");
		printCurrentState();

	}

	return result;

}

/******************************************************************************
 *
 * \par Function Name:
 *      exclude_neighbors
 *
 * \brief  Exclude all the neighbors that can't be used as first hop
 *         to reach the destination.
 *
 *
 * \par Date Written:
 *      19/02/20
 *
 * \return int
 *
 * \retval   0   Success case: List converted
 * \retval  -2   MWITHDRAW error
 *
 * \param[in]      excludedNodes   The list of the excluded neighbors
 * \param[in,out]  *sap            The CurrentCallSAP used by the interface. Here it is used
 *                                 to save the excluded neighbors into the sap->excludedNeighbors list.
 *                                 The excluded neighbors list must be previously allocated by the caller.
 *
 * \warning excludedNodes doesn't have to be NULL
 * \warning excludedNeighbors has to be previously initialized
 * \warning sap must be correctly initialized.
 *
 * \par Notes:
 *           1. This function, first, clear the previous excludedNeighbors list
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  19/02/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
static int exclude_neighbors(Lyst excludedNodes, CurrentCallSAP *sap)
{
	LystElt elt;
	uvast *node;
	unsigned long long *nodeToUniboCGR;
	int result = 0, error = 0;

	free_list_elts(sap->excludedNeighbors); //clear the previous list

	for (elt = lyst_first(excludedNodes); elt && !error; elt = lyst_next(elt))
	{
		//	node = (NodeId *) lyst_data(elt);
		node = (uvast *) lyst_data(elt);
		if (node != NULL && *node != 0)
		{
			nodeToUniboCGR = MWITHDRAW(sizeof(unsigned long long));
			if (nodeToUniboCGR != NULL)
			{
				*nodeToUniboCGR = *node;
				if (list_insert_last(sap->excludedNeighbors, nodeToUniboCGR) == NULL)
				{
					error = 1;
					MDEPOSIT(nodeToUniboCGR);
				}
			}
			else
			{
				error = 1;
			}
		}

	}

	if(error)
	{
		result = -2;
	}


	return result;
}

/******************************************************************************
 *
 * \par Function Name:
 *      cgr_identify_best_routes
 *
 * \brief  Entry point to call the CGR from ION, get the best routes to reach
 *         the destination for the bundle.
 *
 *
 * \par Date Written:
 *      19/02/20
 *
 * \return int
 *
 * \retval  ">= 0"   Number of routes computed by Unibo-CGR to reach destination
 * \retval     -1    System error
 *
 * \param[in]     *terminusNode     The destination node for the bundle
 * \param[in]     *bundle           The ION's bundle that has to be forwarded
 * \param[in]     excludedNodes     Nodes to which bundle must not be forwarded
 * \param[in]     time              The current time
 * \param[in]     sap               The service access point for this session
 * \param[in]     *trace            CGR calculation tracing control
 * \param[out]    IonRoutes         The lyst of best routes found
 *
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  19/02/20 | L. Persampieri  |  Initial Implementation and documentation.
 *  01/08/20 | L. Persampieri  |  Changed return values.
 *****************************************************************************/
int	cgr_identify_best_routes(IonNode *terminusNode, Bundle *bundle, Lyst excludedNodes, time_t time, CgrSAP sap,
		CgrTrace *trace, Lyst IonRoutes)
{
	PsmPartition	ionwm = getIonwm();
	IonVdb		*ionvdb = getIonVdb();
	CgrVdb		*cgrvdb = cgr_get_vdb();
	int		result = -5;
	List		cgrRoutes = NULL;
	CurrentCallSAP *currentCallSap;
	InterfaceUniboCgrSAP *int_ucgr_sap = sap;
#if (LOG || TIME_ANALYSIS_ENABLED)
	UniboCgrSAP uniboCgrSAP = get_unibo_cgr_sap(NULL);
#endif
#if TIME_ANALYSIS_ENABLED
	CgrBundleID id;
#endif

	record_total_interface_start_time();

	debug_printf("Entry point interface.");

	if (int_ucgr_sap != NULL && int_ucgr_sap->initialized && bundle != NULL && terminusNode != NULL && ionwm != NULL && ionvdb != NULL && cgrvdb != NULL)
	{
		start_call_log(time - int_ucgr_sap->reference_time, uniboCgrSAP.count_bundles);

		currentCallSap = get_current_call_sap(NULL);
		// INPUT CONVERSION: check if the contact plan has been changed, in affermative case update it
		result = update_contact_plan(ionwm, ionvdb, int_ucgr_sap->reference_time);
		if (result != -2)
		{
			result = create_ion_node_routing_object(terminusNode, ionwm, cgrvdb);
			if (result == 0)
			{
				// INPUT CONVERSION: learn the bundle's characteristics and store them into the CGR's bundle struct
				result = convert_bundle_from_ion_to_cgr(terminusNode->nodeNbr, time - int_ucgr_sap->reference_time, int_ucgr_sap->reference_time, bundle, currentCallSap->uniboCgrBundle);
				if (result == 0)
				{
					result = exclude_neighbors(excludedNodes, currentCallSap);
					if (result >= 0)
					{
						currentCallSap->ionBundle = bundle;
						debug_printf("Go to CGR.");
						// Call Unibo-CGR
						result = getBestRoutes(time - int_ucgr_sap->reference_time, currentCallSap->uniboCgrBundle, currentCallSap->excludedNeighbors,
								&cgrRoutes);

						if (result > 0 && cgrRoutes != NULL)
						{
							// OUTPUT CONVERSION: convert the best routes into ION's CgrRoute and
							// put them into ION's Lyst
							result = convert_routes_from_cgr_to_ion(int_ucgr_sap->reference_time, ionwm, ionvdb, terminusNode,
									currentCallSap->uniboCgrBundle->evc, cgrRoutes, IonRoutes);
							// ION's contacts MTVs are decreased by ipnfw

							if (result == -1)
							{
								result = -8;
							}
#if(CGRR && CGRREB)
							if(result == 0 && CGRR_management(ionwm, IonRoutes, bundle) == -2)
							{
								result = -2;
							}
#endif
						}
					}
				}
				else
				{
					result = -7;
				}

#if TIME_ANALYSIS_ENABLED
				memcpy(&id, &(currentCallSap->uniboCgrBundle->id), sizeof(CgrBundleID));
#endif
				reset_bundle(currentCallSap->uniboCgrBundle);
				currentCallSap->ionBundle = NULL;
			}
			else
			{
				result = -6;
			}
		}
	}

	/* cgr_identify_best_routes() internal error codes:
	 * -2   MWITHDRAW error
	 * -3   Phase one arguments error
	 * -4   CGR arguments error
	 * -5   callCGR arguments error
	 * -6   Can't create IonNode's routing object
	 * -7   Bundle's conversion error
	 * -8   NULL pointer during conversion to ION's contact
	 *
	 * Note: (-1, >= 0) are not errors
	 */
	debug_printf("internal result -> %d\n", result);

#if (LOG == 1)
	if (result < -1)
	{
		writeLog("Fatal error (cgr_identify_best_routes() internal error code): %d.", result);
	}
	end_call_log();
	// Log interactivity...
	log_fflush();
#endif

	/****** RESULT HANDLING ******/

	if(result == -1)
	{
		// Unibo-CGR says that there aren't route to reach destination
		result = 0;
	}
	else if(result >= 0)
	{
		// We return to ION the number of routes computed by phase one to reach destination
		// Note: the routes used to send the bundle are a subset of these computed routes,
		//       here we get the number of ALL computed routes.
		result = (int) get_computed_routes_number((unsigned long long) terminusNode->nodeNbr);
	}
	else
	{
		// Negative result
		// System error
		result = -1;
	}

	debug_printf("result to ION -> %d\n", result);

	record_total_interface_stop_time();
	print_time_results(time - int_ucgr_sap->reference_time, uniboCgrSAP.count_bundles, &id);

	return result;
}

/******************************************************************************
 *
 * \par Function Name:
 *      computeApplicableBacklog
 *
 * \brief  Compute the applicable backlog (SABR 3.2.6.2 b) ) and the total backlog for a neighbor.
 *
 *
 * \par Date Written:
 *      19/02/20
 *
 * \return int
 *
 * \retval   0   Applicable and total backlog computed
 * \retval  -1   Arguments error
 * \retval  -2   Plan not found
 *
 * \param[in]   neighbor                   The neighbor for which we want to compute
 *                                         the applicable and total backlog.
 * \param[in]   priority                   The bundle's cardinal priority
 * \param[in]   ordinal                    The bundle's ordinal priority (used with expedited cardinal priority)
 * \param[out]  *CgrApplicableBacklog      The applicable backlog computed
 * \param[out]  *CgrTotalBacklog           The total backlog computed
 *
 * \par Notes:
 *          1. This function talks with the CL queue, that should be done during
 *             phase two to get a more accurate applicable (and total) backlog
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  19/02/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
int computeApplicableBacklog(unsigned long long neighbor, int priority, unsigned int ordinal, CgrScalar *CgrApplicableBacklog,
		CgrScalar *CgrTotalBacklog)
{
	int result = -1;
	Sdr sdr;
	char eid[SDRSTRING_BUFSZ];
	VPlan *vplan;
	PsmAddress vplanElt;
	Object planObj;
	BpPlan plan;
	Scalar IonApplicableBacklog, IonTotalBacklog;
	CurrentCallSAP *sap = get_current_call_sap(NULL);

	if (CgrApplicableBacklog != NULL && CgrTotalBacklog != NULL && sap->ionBundle != NULL)
	{
		isprintf(eid, sizeof eid, "ipn:" UVAST_FIELDSPEC ".0", neighbor);
		sdr = getIonsdr();
		findPlan(eid, &vplan, &vplanElt);
		if (vplanElt != 0)
		{

			planObj = sdr_list_data(sdr, vplan->planElt);
			sdr_read(sdr, (char*) &plan, planObj, sizeof(BpPlan));
			if (!plan.blocked)
			{
				computePriorClaims(&plan, sap->ionBundle, &IonApplicableBacklog, &IonTotalBacklog);

				convert_scalar_from_ion_to_cgr(&IonApplicableBacklog, CgrApplicableBacklog);
				convert_scalar_from_ion_to_cgr(&IonTotalBacklog, CgrTotalBacklog);

				result = 0;
			}
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
 *      cgr_stop_SAP
 *
 * \brief  Deallocate all the memory used by the CGR session.
 *
 *
 * \par Date Written:
 *      19/02/20
 *
 * \return	void
 *
 * \param[in]	sap	The service access point for this CGR session.
 *
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  19/02/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
void cgr_stop_SAP(CgrSAP sap)
{
	InterfaceUniboCgrSAP *ucgr_sap;

	if(sap == NULL)
	{
		return;
	}

	ucgr_sap = sap;

	destroy_cgr((getCtime()) - ucgr_sap->reference_time);

	destroy_current_call_sap();
	memset(ucgr_sap, 0, sizeof(InterfaceUniboCgrSAP));
	MDEPOSIT(ucgr_sap);

	return;
}

/******************************************************************************
 *
 * \par Function Name:
 *      cgr_start_SAP
 *
 * \brief  Initialize the CGR session.
 *
 *
 * \par Date Written:
 *      19/02/20
 *
 * \return int
 *
 * \retval   1   Success case: CGR initialized
 * \retval  -1   Error case: ownNode can't be 0
 * \retval  -2   MWITHDRAW error
 * \retval  -3   Error case: log directory can't be opened
 * \retval  -4   Error case: log file can't be opened
 * \retval  -5   Arguments error
 *
 * \param[in]		ownNode	The node from which CGR will compute all routes
 * \param[in]		time	The reference time (time 0 for the CGR session)
 * \param[in,out]	sap	    The service access point for this session.
 *                          It must be allocated by the caller.
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  19/02/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
int	cgr_start_SAP(uvast ownNode, time_t time, CgrSAP *sap)
{
	Sdr		sdr = getIonsdr();
	int		result = 1;
	PsmPartition	ionwm;
	IonVdb		*ionvdb;
	InterfaceUniboCgrSAP * ucgr_sap;

	if (ownNode == 0 || sap == NULL || sdr == NULL)
	{
		putErrmsg("Cannot initialize Unibo-CGR.", NULL);
		return -5;
	}

	*sap = MWITHDRAW(sizeof(InterfaceUniboCgrSAP));

	if(*sap == NULL)
	{
		putErrmsg("Cannot initialize Unibo-CGR.", NULL);
		return -2;
	}

	ucgr_sap = *sap;

	ionwm = getIonwm();
	ionvdb = getIonVdb();
	CHKERR(sdr_begin_xn(sdr));

	result = initialize_cgr(0, ownNode);

	if (result == 1) {
		writeLog("Reference time (Unix time): %ld s.", (long int) time);

		if(initialize_cgr_sap(time, ucgr_sap) != 1) {
			result = -2;
		}
		else if(initialize_current_call_sap() != 1) {
			result = -2;
		}
		else if (update_contact_plan(ionwm, ionvdb, ucgr_sap->reference_time) == -2) {
			writeLog("Cannot update contact plan");
			//printf("Cannot update contact plan in Unibo-CGR");
			result = -2;
		}
		else {
			result = 1;
		}
	}

	if(result != 1) {
		// init error
		putErrmsg("Can't initialize Unibo-CGR.", NULL);
		MDEPOSIT(*sap);
		*sap = NULL;
	}

	sdr_exit_xn(sdr);
	return result;
}

#else
#include <stdlib.h>
#include "../../core/cgr/cgr_phases.h"
#include "../../core/library_from_ion/scalar/scalar.h"

int computeApplicableBacklog(unsigned long long neighbor, int priority, unsigned int ordinal, CgrScalar *CgrApplicableBacklog,
		CgrScalar *CgrTotalBacklog) {
	loadCgrScalar(CgrApplicableBacklog, 0);
	loadCgrScalar(CgrTotalBacklog, 0);
	return 0;		
}
#endif
