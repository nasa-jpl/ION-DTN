/** \file interface_unibocgr_dtn2.cc
 *  
 *  \brief    This file provides the implementation of the functions
 *            that make UniboCGR's implementation compatible with DTN2.
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
 *  \author Giacomo Gori, giacomo.gori3@studio.unibo.it
 *
 *  \par Supervisor
 *       Carlo Caini, carlo.caini@unibo.it
 */
#ifdef HAVE_CONFIG_H
#  include <dtn-config.h>
#endif
#include "interface_unibocgr_dtn2.h"
//include from dtn2
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>



#include "../../core/library/commonDefines.h"
#include "../../core/cgr/cgr.h"

#include "../../core/bundles/bundles.h"
#include "../../core/contact_plan/contactPlan.h"

#include "../../core/contact_plan/contacts/contacts.h"
#include "../../core/contact_plan/ranges/ranges.h"
#include "../../core/library/list/list.h"
#include "../../core/library_from_ion/scalar/scalar.h"

#include "../../core/cgr/cgr_phases.h"
#include "../../core/msr/msr_utils.h"
#include "../../core/time_analysis/time.h"


#define NOMINAL_PRIMARY_BLKSIZE	29 // from ION 4.0.0: bpv7/library/libbpP.c
#define EPOCH_2000_SEC 946684800


/**
 * \brief This is the name of the contact file from which unibocgr get contacts and ranges
 */
#define CONTACT_FILE "./contact-plan.txt"
/**
 * \brief This time is used by the CGR as time 0.
 */
static time_t reference_time = -1;
/**
 * \brief Boolean used to know if the CGR has been initialized or not.
 */
static int initialized = 0;
/**
 * \brief The list of excluded neighbors for the current bundle.
 */
static List excludedNeighbors = NULL;
/**
 * \brief The local node
 */

static unsigned long long localIpn;
/**
 * \brief The istance of the UniboCGR router used by dtn2
 */
static dtn::UniboCGRBundleRouter* uniborouter;
//}
/**
 * \brief CgrBundle used during the current call.
 */
static CgrBundle *cgrBundle = NULL;

/**
 * \brief Default region number.
 */
static const unsigned long defaultRegionNbr = 1;

#define printDebugIonRoute(ionwm, route) do {  } while(0)

//ported from ION bpv6 for CGRR and RGR extension
typedef struct
{
	unsigned long long	fromNode;
	unsigned long long	toNode;
	time_t	fromTime;
} CGRRHop;

typedef struct
{
	unsigned int  hopCount; //Number of hops (contacts)
	CGRRHop		 *hopList; //Hop (contact): identified by [from, to, fromTime]
} CGRRoute;


typedef struct
{
	unsigned int recRoutesLength; //number of recomputedRoutes
	CGRRoute originalRoute; //computed by the source
	CGRRoute *recomputedRoutes; //computed by following nodes
} CGRRouteBlock;

typedef struct
{
	unsigned int	length;
	char			*nodes;
} GeoRoute;


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
 *
 *
 * \return int
 *
 * \retval  0  Success case: GeoRoute found
 * \retval -1  GeoRoute not found
 * \retval -2  System error
 *
 * \param[in]   *bundle    The DTN2 bundle that contains the RGR Extension Block
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
 *  16/09/20 | G. Gori         |  Definition and documentation.
 *****************************************************************************/
static int get_rgr_ext_block(dtn::Bundle *bundle, GeoRoute *resultBlk)
{
	int result =0;
	//TODO: implement RGR extension block processor, take data and popolate resultBlk.
	//pay attention, resultBlk NEEDS to be allocated ( malloc() ) and then call free() outside this function in convert_bundle_...()
	return result;
}
#endif

#if (MSR == 1)
/******************************************************************************
 *
 * \par Function Name:
 *      get_cgrr_ext_block
 *
 * \brief  Get the CGRRouteBlock stored into CGRR Extension Block
 *
 *
 * \par Date Written:
 *
 *
 * \return int
 *
 * \retval  0  Success case: CGRRouteBlock found
 * \retval -1  CGRRouteBlock not found
 * \retval -2  System error
 *
 * \param[in]   *bundle    The DTN2 bundle that contains the RGR Extension Block
 * \param[out] **resultBlk The CGRRouteBlock extracted from the CGRR Extension Block, only in success case.
 *                         The resultBLk will be allocated by this function.
 *
 * \warning bundle    doesn't have to be NULL
 * \warning resultBlk doesn't have to be NULL
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  16/09/20 | G. Gori		   |  Definition and documentation.
 *****************************************************************************/
static int get_cgrr_ext_block(dtn::Bundle *bundle, CGRRouteBlock **resultBlk)
{
	int result = 0, i, j;
	//TODO: implements CGRR extension block processor, take data and popolate resultBlk
	//pay attention, resultBlk NEEDS to be allocated ( malloc() ) and then call free() outside this function in convert_bundle_...()

	return result;
}
#endif
/******************************************************************************
 *
 * \par Function Name:
 *      convert_bundle_from_dtn2_to_cgr
 *
 * \brief Convert the characteristics of the bundle from DTN2 to
 *        this CGR's implementation and initialize all the
 *        bundle fields used by the Unibo CGR.
 *
 *
 * \par Date Written:
 *      05/07/20
 *
 * \return int
 *
 * \retval  0  Success case
 * \retval -1  Arguments error
 * \retval -2 Bundle have a DTN name, not IPN. Can't use CGR.
 *
 * \param[in]    current_time       The current time, in differential time from reference_time
 * \param[in]    *Dtn2Bundle        The bundle in DTN2
 * \param[out]   *CgrBundle         The bundle in this CGR's implementation.
 *
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  05/07/20 | G. Gori		    |  Initial Implementation and documentation.
 *****************************************************************************/
static int convert_bundle_from_dtn2_to_cgr(time_t current_time, dtn::Bundle *Dtn2Bundle, CgrBundle *CgrBundle)
{
	//TODO Consider to take count of extensions
	int result = -1;
	time_t offset;
#if (MSR == 1)
	CGRRouteBlock *cgrrBlk;
#endif
#if (CGR_AVOID_LOOP > 0)
	GeoRoute geoRoute;
#endif

	if (Dtn2Bundle != NULL && CgrBundle != NULL)
	{
		std::string ipnName = Dtn2Bundle->dest().str();
		if (ipnName.rfind("dtn", 0) == 0) {
		  // it's a dtn name, can't use CGR
		  result = -2;
		}
		else
		{
			std::string delimiter1 = ":";
			std::string delimiter2 = ".";
			std::string s = ipnName.substr(ipnName.find(delimiter1) + 1, ipnName.find(delimiter2) - 1);
			std::stringstream convert;
			unsigned long long destNode;
			convert << s;
			convert >> destNode;
			CgrBundle->terminus_node = destNode;
			CgrBundle->regionNbr = defaultRegionNbr;

			#if (MSR == 1)
					CgrBundle->msrRoute = NULL;
					result = get_cgrr_ext_block(Dtn2Bundle, &cgrrBlk);

					if(result == 0)
					{
						result = set_msr_route(current_time, cgrrBlk, CgrBundle);
						free(cgrrBlk);
					}
			#endif
			#if (CGR_AVOID_LOOP > 0)
					if(result != -2)
					{
						result = get_rgr_ext_block(Dtn2Bundle, &geoRoute);
						if(result == 0)
						{
							result = set_geo_route_list(geoRoute.nodes, CgrBundle);
							free(geoRoute.nodes);
						}
					}
			#endif
			if (result != -2)
			{
				CLEAR_FLAGS(CgrBundle->flags); //reset previous mask
					#ifdef ECOS_ENABLED
				if(Dtn2Bundle->ecos_critical())
				{
					SET_CRITICAL(CgrBundle);
				}
					#endif
				/*Non ho trovato info sul backward propagation
				if (!(IS_CRITICAL(CgrBundle)) && IonBundle->returnToSender)
				{
					SET_BACKWARD_PROPAGATION(CgrBundle);
				}*/
				if(!(Dtn2Bundle->do_not_fragment()))
				{
					SET_FRAGMENTABLE(CgrBundle);
				}
				#ifdef ECOS_ENABLED
				CgrBundle->ordinal = (unsigned int) Dtn2Bundle->ecos_ordinal();
				#endif

				 // TODO CgrBundle->id.source_node = Dtn2Bundle->source();
				CgrBundle->id.source_node = 0; // TODO TEMP
				CgrBundle->id.creation_timestamp = Dtn2Bundle->creation_ts().seconds_;
				CgrBundle->id.sequence_number = Dtn2Bundle->creation_ts().seqno_;
				if( Dtn2Bundle->is_fragment())
				{
					CgrBundle->id.fragment_length = Dtn2Bundle->frag_length();
					CgrBundle->id.fragment_offset = Dtn2Bundle->frag_offset();
				}
				else
				{
					CgrBundle->id.fragment_length = 0;
					CgrBundle->id.fragment_offset = 0;
				}

				CgrBundle->size = NOMINAL_PRIMARY_BLKSIZE + Dtn2Bundle->durable_size();

				CgrBundle->evc = computeBundleEVC(CgrBundle->size); // SABR 2.4.3

				offset = Dtn2Bundle->creation_ts().seconds_ + EPOCH_2000_SEC - reference_time;
				//offset è la differenza tra la creazione del bundle e il momento di partenza del demone dtnd
				//CgrBundle->expiration_time = IonBundle->expirationTime
				//		- IonBundle->id.creationTime.seconds + offset;

				CgrBundle->expiration_time = Dtn2Bundle->expiration_millis() + offset;
				//Read PreviousHop Extension
				std::string ipnName2 = Dtn2Bundle->prevhop().str();
				if (ipnName2.rfind("dtn", 0) == 0) {
				  // it's a dtn name, can't use CGR
				  //try to use it anyway without previous hop
				  ipnName2 = "ipn:0.0";
				  //result = -2;
				}
				if(ipnName2 != "")
				{
					std::size_t da = ipnName2.find(delimiter1);
					std::size_t a = ipnName2.find(delimiter2);
					std::string s2 = ipnName2.substr(da +1,a -1);
					std::stringstream converts;
					unsigned long long sendNode;
					converts << s2;
					converts >> sendNode;
					CgrBundle->sender_node = sendNode;
				}
				else
				{
					CgrBundle->sender_node = 0;
				}
				u_int8_t pr = Dtn2Bundle->priority();
				if(pr==0) {CgrBundle->priority_level = Bulk;}
				if(pr==1) {CgrBundle->priority_level = Normal;}
				if(pr==2) {CgrBundle->priority_level = Expedited;}
				if(pr<0 || pr>2) {CgrBundle->priority_level = Normal; writeLog("Error: priority of bundle is not a valid value: %zu. Using default: normal", pr);}
				CgrBundle->dlvConfidence = 0;
					result = 0;
			}
		}
	}

	return result;
}



/******************************************************************************
 *
 * \par Function Name:
 *      convert_routes_from_cgr_to_dtn2
 *
 * \brief Convert a list of routes from CGR's format to DTN2's format
 *
 *
 * \par Date Written: 
 *      16/07/20
 *
 * \return int 
 *
 * \retval   0  All routes converted
 * \retval  -1  CGR's contact points to NULL
 *
 * \param[in]     evc             Bundle's estimated volume consumption
 * \param[in]     cgrRoutes       The list of routes in CGR's format
 * \param[out]    *res        	 All the next hops that CGR found, separated by a single space
 *
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  16/07/20 | G. Gori         |  Initial Implementation and documentation.
 *****************************************************************************/
static int convert_routes_from_cgr_to_dtn2(long unsigned int evc, List cgrRoutes, std::string *res)
{
	ListElt *elt;
	Route *current;
	size_t count = 0;
	int result = 0;
	int numRes = 0;
	for (elt = cgrRoutes->first; elt != NULL && result >= 0; elt = elt->next)
	{
		if (elt->data != NULL)
		{
			if(numRes>0)
			{
				//spazio che separa i risultati
				res->append(" ");
			}
			current = (Route*) elt->data;
			std::string toNode = "ipn:";
			std::stringstream streamNode;
			streamNode << toNode << current->neighbor;
			res->append(streamNode.str());
			// Always using 0 as demux token
			res->append(".0");
			numRes++;
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
 *      add_contact
 *
 * \brief  Add a contact to the contacts graph of this CGR's implementation.
 *
 *
 * \par Date Written:
 *      02/07/20
 *
 * \return int
 *
 * \retval   1   Contact added
 * \retval   0   Contact's arguments error
 * \retval  -1   The contact overlaps with other contacts
 * \retval  -2   Can't read all info of the contact
 * \retval  -3   Can't read all info of the contact
 * \retval  -4   Can't read fromTime or toTime
 *
 * \param[in]   fileline	The line read from the file
 *
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  02/07/20 | G. Gori		    |  Initial Implementation and documentation.
 *****************************************************************************/
static int add_contact(char * fileline)
{
	Contact CgrContact;
	int count = 10, n=0, result = -2;
	long long fn = 0;
	long unsigned int xn = 0;
	CgrContact.type = TypeScheduled;
	//fromTime
	if(fileline[count] == '+')
	{
		count++;
		while(fileline[count] >= 48 && fileline[count] <= 57) {
			n = n * 10 + fileline[count] - '0';
			count++;
		}
		count++; 
		CgrContact.fromTime = n;
		n=0;
	}
	else result = -4;
	//toTime
	if(fileline[count] == '+')
	{
		count++;
		while(fileline[count] >= 48 && fileline[count] <= 57) {
			n = n * 10 + fileline[count] - '0';
			count++;
		}
		count++;
		CgrContact.toTime = n;
		n=0;
	}
	else result = -4;
	//fromNode
	while(fileline[count] >= 48 && fileline[count] <= 57) {
			fn = fn * 10 + fileline[count] - '0';
			count++;
		}
	count++;
	CgrContact.fromNode = fn ;
	fn=0;
	//toNode
	while(fileline[count] >= 48 && fileline[count] <= 57) {
			fn = fn * 10 + fileline[count] - '0';
			count++;
		}
	count++;
	CgrContact.toNode = fn ;
	fn=0;
	//txrate
	while(fileline[count] >= 48 && fileline[count] <= 57) {
			xn = xn * 10 + fileline[count] - '0';
			count++;
			result=0;
		}
	count++;
	CgrContact.xmitRate = xn ;
	CgrContact.confidence = 1;
	if (result == 0)
	{
		// Try to add contact
		result = addContact(defaultRegionNbr, CgrContact.fromNode, CgrContact.toNode, CgrContact.fromTime,
				CgrContact.toTime, CgrContact.xmitRate, CgrContact.confidence, 0, NULL);
		if(result >= 1)
		{
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
 *      add_range
 *
 * \brief  Add a range to the ranges graph of this CGR's implementation.
 *
 *
 * \par Date Written:
 *      02/07/20
 *
 * \return int
 *
 * \retval   1   Range added
 * \retval   0   Range's arguments error
 * \retval  -1   fileline is not properly formatted
 * \retval  -2   fileline doesn't start with "+"
 *
 * \param[in]  char*		The line that contains the range
 *
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  02/07/20 | G. Gori		    |  Initial Implementation and documentation.
 *****************************************************************************/
static int add_range(char* fileline)
{
	Range CgrRange;
	int result = -1;
	int n = 0;
	int count = 8;
	unsigned int ow = 0;
	long long fn = 0;
	//fromTime
	if(fileline[count] == '+')
	{
		count++;
		while(fileline[count] >= 48 && fileline[count] <= 57) {
			n = n * 10 + fileline[count] - '0';
			count++;
		}
		count++; 
		CgrRange.fromTime = n;
		n=0;
	}
	else result = -2;
	//toTime
	if(fileline[count] == '+')
	{
		count++;
		while(fileline[count] >= 48 && fileline[count] <= 57) {
			n = n * 10 + fileline[count] - '0';
			count++;
		}
		count++; 
		CgrRange.toTime = n;
		n=0;
	}
	else result = -2;
	//fromNode
	while(fileline[count] >= 48 && fileline[count] <= 57) {
			fn = fn * 10 + fileline[count] - '0';
			count++;
		}
	count++;
	CgrRange.fromNode = fn ;
	fn=0;
	//toNode
	while(fileline[count] >= 48 && fileline[count] <= 57) {
			fn = fn * 10 + fileline[count] - '0';
			count++;
		}
	count++;
	CgrRange.toNode = fn ;
	//owlt
	while(fileline[count] >= 48 && fileline[count] <= 57) {
			ow = ow * 10 + fileline[count] - '0';
			count++;
			result = 0;
		}
	count++;
	CgrRange.owlt = ow ;
	if (result == 0)
	{
		result = addRange(CgrRange.fromNode, CgrRange.toNode, CgrRange.fromTime, CgrRange.toTime,
				CgrRange.owlt);
		if(result >= 1)
		{
			result = 1;
		}
	}
	else
	{
		result = -1;
	}
	return result;
}


/******************************************************************************
 *
 * \par Function Name:
 *      read_file_contactranges
 *
 * \brief  Read and add all new contacts or ranges of the file with filename
 * 		  specified in the first parameter to the
 *         contacts graph of thic CGR's implementation.
 *
 * \details Only for Registration and Scheduled contacts.
 *
 *
 * \par Date Written:
 *      02/07/20
 *
 * \return int
 *
 * \retval  ">= 0"  Number of contacts added to the contacts graph
 * \retval     -1   open file error
 *
 * \param[in]   char*	The name of the file to read
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  02/07/20 | G. Gori         |  Initial Implementation and documentation.
 *****************************************************************************/
static int read_file_contactranges(char * filename)
{
	int result = 0, totAdded = 0, stop = 0;
	int skip = 0, count = 1;
	int result_contacts = 0, result_ranges = 0;
	int fd = open(filename, O_RDONLY);
	char car;
	if(fd < 0)
	{
		result = -1;
	}
	else
	{
		while(read(fd,&car,sizeof(char)))
		{
			if(car == '#') skip = 1;
			if(car == '\n')
			{
				if(skip==0)
				{
					lseek(fd,-count, 1);
					char * res;
					res = (char*) malloc(sizeof(char)*count);

					read(fd,res,sizeof(char)*count);
					//Recognize if it's enough lenght and if it is a contact or range
					if(count > 18 && res[0] == 'a')
					{
						if(res[2] == 'c' && res[3] == 'o' && res[4] == 'n' && res[5] == 't' && res[6] == 'a' && res[7] == 'c' && res[8] == 't')
						{
							result = add_contact(res);
							if(result == 1)
							{
								result_contacts++;
							}
						}
						else if(res[2] == 'r' && res[3] == 'a' && res[4] == 'n' && res[5] == 'g' && res[6] == 'e')
						{
							result = add_range(res);
							if(result == 1)
							{
								result_ranges++;
							}
						}
					}
					count = 0;
					free(res);
				}
				else if(skip==1)
				{
					count=0;
					skip=0;
				}
			}
			count++;
		}
		close(fd);
#if (LOG == 1)
		if (result_contacts > 0)
		{
			writeLog("Added %d contacts.", result_contacts);
		}
		if (result_ranges > 0)
		{
			//This piece of code add bidirectional range if there are only unidirectional in the contact plan
			RbtNode *node = NULL;
			Range* primo; RbtNode *node2 = NULL;
			primo = get_first_range(&node);
			if(primo != NULL)
			{
				Range *trovato = NULL;
				do
				{
					if((*primo).fromNode < (*primo).toNode)
					{
						trovato = get_first_range_from_node_to_node((*primo).toNode, (*primo).fromNode, &node2);
						if(trovato == NULL)
						{
							//I add the opposite range
							result = addRange((*primo).toNode, (*primo).fromNode, (*primo).fromTime, (*primo).toTime, (*primo).owlt);
							if(result >= 1)
							{
								result_ranges++;
							}
						}
					}
					primo = get_next_range(&node);
				}
				while(primo != NULL);
			}
			else
			{
				writeLog("No ranges found in graph");
			}
			writeLog("Added %d ranges.", result_ranges);
		}
#endif
	}
	result = result_contacts + result_ranges;
	return result;
}





/******************************************************************************
 *
 * \par Function Name:
 *      update_contact_plan
 *
 * \brief  
 *
 *
 * \par Date Written:
 *      01/07/20
 *
 * \return int
 *
 * \retval   0  Contact plan has been changed and updated
 * \retval  -1  Contact plan isn't changed
 * \retval  -2  File error
 *
 * \param[in]   filename     The name of the file from which we read contacts
 * \param[in]   update       Set true to update contact plan, false to not update
 *
 * \warning file should exist
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  01/07/20 | G. Gori         |  Initial Implementation and documentation.
 *****************************************************************************/
static int update_contact_plan(char * filename, bool update)
{
	int result = -2;
	int result_read;
	ContactPlanSAP cpSap = get_contact_plan_sap(NULL);
   //Currently contacts are read from file, so we don't have any update during execution
   //Set a condition in this if() if you need to check for update
	if (update)
	{
		writeLog("#### Contact plan modified ####");
		result_read = read_file_contactranges(filename);
		if (result_read < 0) //Error
		{
			result = -1;
		}
		else
		{
			result = 0;
		}
		struct timeval tv;
		gettimeofday(&tv, NULL);
		cpSap.contactPlanEditTime.tv_sec = tv.tv_sec;
		cpSap.contactPlanEditTime.tv_usec = tv.tv_usec;
		set_time_contact_plan_updated(cpSap.contactPlanEditTime.tv_sec,cpSap.contactPlanEditTime.tv_usec);

		writeLog("###############################");
		printCurrentState();

	}
	else result = -1;

	return result;

}

/******************************************************************************
 *
 * \par Function Name:
 *      exclude_neighbors
 *
 * \brief  Function that will exclude all the neighbors that can't be used as first hop
 *         to reach the destination. Currently it only clear the list.
 *
 *
 * \par Date Written:
 *      01/07/20
 *
 * \return int
 *
 * \retval   0   Success case: List converted
 *
 * \warning excludedNeighbors has to be previously initialized
 *
 * \par Notes:
 *           1. This function clear the previous excludedNeighbors list
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  01/07/20 | G. Gori         |  Initial Implementation and documentation.
 *****************************************************************************/
static int exclude_neighbors()
{
	int result = 0;
	free_list_elts(excludedNeighbors); //clear the previous list
	return result;
}

/******************************************************************************
 *
 * \par Function Name:
 *      callUniboCGR
 *
 * \brief  Entry point to call the CGR from DTN2, get the best routes to reach
 *         the destination for the bundle.
 *
 *
 * \par Date Written:
 *      05/07/20
 *
 * \return int
 *
 * \retval    0   Routes found and converted
 * \retval   -1   There aren't route to reach the destination
 * \retval   -3   Phase one arguments error
 * \retval   -4   CGR arguments error
 * \retval   -5   callCGR arguments error
 * \retval   -7   Bundle's conversion error
 * \retval   -8   Bundle's conversion error: bundle have DTN name instead of IPN
 *
 * \param[in]     time              The current time
 * \param[in]     *bundle           The DTN2's bundle that has to be forwarded
 * \param[out]    *res          	The list of best routes found written on a string separated by a space
 *
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  05/07/20 | G. Gori		    |  Initial Implementation and documentation.
 *****************************************************************************/
int callUniboCGR(time_t time, dtn::Bundle *bundle, std::string *res)
{

	int result = -5;
	List cgrRoutes = NULL;
	UniboCgrSAP sap = get_unibo_cgr_sap(NULL);

	record_total_interface_start_time();
	debug_printf("Entry point interface.");

	if (initialized && bundle != NULL)
	{
		// INPUT CONVERSION: check if the contact plan has been changed, in affermative case update it
		result = update_contact_plan("", false);
		if (result != -2)
		{
			// INPUT CONVERSION: learn the bundle's characteristics and store them into the CGR's bundle struct
			result = convert_bundle_from_dtn2_to_cgr(time - reference_time, bundle, cgrBundle);
			if (result == 0)
			{
				//Start log only after checking that bundle is ok
				start_call_log(time - reference_time,sap.count_bundles);
				print_log_bundle_id(cgrBundle->id.source_node,
						cgrBundle->id.creation_timestamp,
						cgrBundle->id.sequence_number,
						cgrBundle->id.fragment_length,
						cgrBundle->id.fragment_offset);
				writeLog("Payload length: %zu.", bundle->payload().length());
				debug_printf("Go to CGR.");
				// Call Unibo-CGR
				result = getBestRoutes(time - reference_time, cgrBundle, excludedNeighbors,
						&cgrRoutes);

				if (result > 0 && cgrRoutes != NULL)
				{
					// OUTPUT CONVERSION: convert the best routes into DTN2's CgrRoute and
					// put them into ION's Lyst
					result = convert_routes_from_cgr_to_dtn2(
							cgrBundle->evc, cgrRoutes, res);

					if (result == -1)
					{
						result = -8;
					}
				}
			}
			else
			{
				if(result == -2)
				{
					result = -8;
				}
				else result = -7;
			}
			reset_bundle(cgrBundle);
		}
	}

	debug_printf("result -> %d\n", result);

#if (LOG == 1)
	if (result < -1)
	{
		writeLog("Error (interface): %d.", result);
	}
	end_call_log();
	log_fflush();
#endif

	record_total_interface_stop_time();
	print_time_results(time - reference_time, sap.count_bundles, &(cgrBundle->id));
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
 *      20/07/20
 *
 * \return int
 *
 * \retval   0   Applicable and total backlog computed
 * \retval  -1   Arguments error
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
 *  20/07/20 | G. Gori         |  Initial Implementation and documentation.
 *  10/08/20 | G. Gori		   |  Fixed oasys::Lock bug
 *****************************************************************************/
int computeApplicableBacklog(unsigned long long neighbor, int priority, unsigned int ordinal, CgrScalar *CgrApplicableBacklog,
		CgrScalar *CgrTotalBacklog)
{
	int result = -1;
	long int byteTot; long int byteApp;
	if (CgrApplicableBacklog != NULL && CgrTotalBacklog != NULL)
	{
		byteTot=0; byteApp=0;
		result = uniborouter->getBacklogForNode(neighbor,priority,&byteApp,&byteTot);
		loadCgrScalar(CgrTotalBacklog, byteTot);
		loadCgrScalar(CgrApplicableBacklog, byteApp);
		if(result >= 0)
		{
			debugLog("found %d bundle in queue on neighbor %llu.", result, neighbor);
			result=0;
		}

	}
	else result = -1;
	return result;
}

/******************************************************************************
 *
 * \par Function Name:
 *      destroy_contact_graph_routing
 *
 * \brief  Deallocate all the memory used by the CGR.
 *
 *
 * \par Date Written:
 *      14/07/20
 *
 * \return  void
 *
 * \param[in]  time  The current time
 *
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  14/07/20 | G. Gori		    |  Initial Implementation and documentation.
 *****************************************************************************/
void destroy_contact_graph_routing(time_t time)
{
	free_list(excludedNeighbors);
	excludedNeighbors = NULL;
	bundle_destroy(cgrBundle);
	cgrBundle = NULL;
	destroy_cgr(time - reference_time);
	initialized = 0;
	reference_time = -1;
	return;
}

/******************************************************************************
 *
 * \par Function Name:
 *      initialize_contact_graph_routing
 *
 * \brief  Initialize all the data used by the CGR.
 *
 *
 * \par Date Written:
 *      01/07/20
 *
 * \return int
 *
 * \retval   1   Success case: CGR initialized
 * \retval  -1   Error case: ownNode can't be 0
 * \retval  -2   can't open or read file with contacts
 * \retval  -3   Error case: log directory can't be opened
 * \retval  -4   Error case: log file can't be opened
 * \retval  -5   Arguments error
 *
 * \param[in]   ownNode   The node that the CGR will consider as contacts graph's root
 * \param[in]   time      The reference unix time (time 0 for the CGR)
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  01/07/20 | G. Gori         |  Initial Implementation and documentation.
 *****************************************************************************/
int initialize_contact_graph_routing(unsigned long long ownNode, time_t time, dtn::UniboCGRBundleRouter *router)
{

	int result = 1;
	char fileName[100] = CONTACT_FILE;
	if(!(ownNode != 0 && time >= 0))
	{
		printf("Initialize CGR arguments error\n");
		result = -5;
	}
	if (initialized != 1)
	{
		excludedNeighbors = list_create(NULL, NULL, NULL, free);
		cgrBundle = bundle_create();

		if (excludedNeighbors != NULL && cgrBundle != NULL)
		{
			result = initialize_cgr(0, ownNode);

			if (result == 1)
			{
				initialized = 1;
				reference_time = time;
				localIpn = ownNode;
				writeLog("Reference time (Unix time): %ld s.", (long int) reference_time);

				if(update_contact_plan(fileName, true) < 0)
				{
					printf("Cannot update contact plan in Unibo-CGR: can't open file");
					result = -2;
				}
				uniborouter = router;
			}
			else
			{
				printf("CGR initialize error: %d\n", result);
			}
		}
		else
		{
			free_list(excludedNeighbors);
			bundle_destroy(cgrBundle);
			excludedNeighbors = NULL;
			cgrBundle = NULL;
			result = -2;
		}
	}

	return result;
}




















