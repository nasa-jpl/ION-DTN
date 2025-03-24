/** \file rgr_utils.c
 *
 *	\brief       This file provides utility functions necessary for a
 *				 full implementation of Real Route Extension Block.
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
 *	\author Laura Mazzuca, laura.mazzuca@studio.unibo.it
 *
 *	\par Supervisor
 *	     Carlo Caini, carlo.caini@unibo.it
 */

#include "../rgr/rgr_utils.h"

///Added by A. Stramiglio 26/11/19
#include <string.h>
//Added by A.Stramiglio 28/11/19
#include <unistd.h>
//Added by A.Stramiglio 3/12/19
#include <lyst.h>

//Added by L. Persampieri 26/03/20
#include <ctype.h>

/******************************************************************************
 * \par Function Name: rgr_read
 *
 * \par Purpose: This function converts an ExtensionBlock into a GeoRoute Struct.
 * 				 if the ExtensionBlock is empty nothing changes. The GeoRoute Struct
 * 				 passed must be initialized. The function allocates an rgrBlk in psm Memory
 * 				 with MTAKE.The caller needs to do the MRELEASE.
 *
 * \return int
 *
 * \retval  0  The GeoRoute was successfully filled.
 * \retval -1  There was a system error.
 * \retval -2  Empty GeoRoute (rgrBlk)
 * \retval -3  Empty ExtensionBlock (blk)
 *
 * \param[in,out]  blk    The ExtensionBlock read by the sdrMemory
 * \param[in,out]  rgrBlk The GeoRoute filled with ExtensionBlock values.
 *
 * \par Notes:
 *      1. All block memory are allocated using MTAKE.
 *
 * \par Modification History:
 *
 *  MM/DD/YY | AUTHOR         | DESCRIPTION
 *  -------- | ------------   | ---------------------------------------------
 *  21/11/19 | A. Stramiglio  |   Initial Implementation and documentation
 *  22/02/20 | L. Persampieri |   Bug fix: nested if
 *****************************************************************************/
extern int rgr_read(ExtensionBlock *blk, GeoRoute *rgrBlk)
{

	Sdr sdr = getIonsdr();
	unsigned char *cursor = NULL;
	unsigned char *blkBuffer = NULL;
	int unparsedBytes = 0;
	unsigned int temp = 0;

	/*
	 * Step 1 - Extract previously added nodes, if there are any.
	 */

	if (blk->dataLength > 0)
	{
		blkBuffer = MTAKE(blk->length);
		if (blkBuffer == NULL)
		{
			rgr_debugPrint("[rgr.c/rgr_processOnDequeue] No space for block buffer %u.", blk->length);
			return -1;
		}

		CHKERR(sdr_begin_xn(sdr));
		sdr_read(sdr, (char*) blkBuffer, blk->bytes, blk->length);
		sdr_end_xn(sdr);

		unparsedBytes = blk->dataLength;
		cursor = (blkBuffer) + (blk->length - blk->dataLength);
		//rgr_debugPrint("rgr_processOnDequeue: Unpased bytes: %d, data length: %u", unparsedBytes, blk->dataLength);

		extractSmallSdnv(&temp, &cursor, &unparsedBytes);
		//rgr_debugPrint("rgr_processOnDequeue: Retrieved GeoRoute block length of %u", temp);
		rgrBlk->length = temp;
		//rgr_debugPrint("rgr_processOnDequeue: GeoRoute block length = %u", rgrBlk.length);

		/*
		 * Step 2 - Initialize rgrBlk with correct values.
		 * */

		if (rgrBlk->length != 0) //nested by L. Persampieri
		{

			rgrBlk->nodes = (char*) MTAKE(rgrBlk->length + 1);

			if (rgrBlk->nodes == NULL)
			{
				MRELEASE(blkBuffer); //Added by L. Persampieri
				rgr_debugPrint(
						"[rgr.c/rgr_processOnDequeue] Cannot instantiate memory for oldRgrBlk.nodes.");
				return -1;
			}

			if (rgrBlk->length > 0)
			{
				memcpy(rgrBlk->nodes, cursor, rgrBlk->length);
				//rgr_debugPrint("rgr_processOnDequeue: Extracted %s", rgrBlk.nodes);
			}

			MRELEASE(blkBuffer);
		}
		else
		{
			MRELEASE(blkBuffer);
			return -2;
		}
	}
	else
	{
		return -3;
	}

	return 0;
}

/******************************************************************************
 *
 * \par Function Name: findLoopEntryNode
 *
 * \par Purpose: This function finds if a loop occurred looking the GeoRoute.
 * 			     If my own node number is found in the route, the following node is considered as
 * 			     loopEntryNode.
 *
 * \return uvast
 *
 * \retval "> 0" Loop occurred, returning loopNode.
 * \retval    0  No loop or system error.
 *
 * \param[in] route				The GeoRoute containing information of the bundle route.
 * \param[in] nodeNum			Own node number of the bundle
 *
 * \par Notes:
 *
 * \par Modification History:
 *
 *  MM/DD/YY |  AUTHOR       |  DESCRIPTION
 *  -------- | ------------  | ---------------------------------------------
 *  3/12/19  | A. Stramiglio |    Initial Implementation and documentation
 *****************************************************************************/
extern uvast findLoopEntryNode(GeoRoute *route, uvast nodeNum)
{
	char *temp;
//	char *next;
	char stringNodeNum[sizeof(uvast) + 5];
//	char subString[256];
	uvast failedNode = 0;

	if (route == NULL || route->nodes == NULL || nodeNum == 0)
	{
		return 0;
	}

	//Step 1 -> Converts nodeNum into a string
	sprintf(stringNodeNum, "ipn:"UVAST_FIELDSPEC, nodeNum);

	//Step 2a -> not found my node, return 0
	temp = strstr(route->nodes, stringNodeNum);
	if (temp == NULL)
	{
		rgr_debugPrint("[rgr_utils.c/findLoopEntryNode] NOT found the node: %s in the route: %s",
				stringNodeNum, route->nodes);
		return 0;

	}
	else
	{
		//Step 2b -> found my node

		//if there is an occurrence
		rgr_debugPrint(
				"[rgr_utils.c/findLoopEntryNode] LOOP occurred! Found the node: %s in the route: %s",
				stringNodeNum, route->nodes);
		//make substring
		//		strcpy(subString, temp + strlen(stringNodeNum));
		temp = strstr(temp + 4, "ipn:");
		if (temp != NULL)
		{
			rgr_debugPrint("[rgr_utils.c/findLoopEntryNode] SubString: %s", temp);

			//Step 4 -> take the successive node as failedNode
			temp = temp + 4;
			if (*temp != '\0')
			{
				//node number always in format ipn:nodeNum
				failedNode = (uvast) strtouvast(temp);
				rgr_debugPrint("[rgr_utils.c/findLoopEntryNode] The failed node is: " UVAST_FIELDSPEC,
						failedNode);

			}
			else
			{
				rgr_debugPrint("[rgr_utils.c/findLoopEntryNode] Error string ends in ipn:");
				return 0;
			}
		}
		else
		{
			rgr_debugPrint("[rgr_utils.c/findLoopEntryNode] Error in strstr, not found the failedNode");
			return 0;
		}

	}

	return failedNode;

}

/******************************************************************************
 *
 * \par Function Name:
 *      get_geo_route_lyst
 *
 * \brief Convert the GeoRoute into a lyst of ipn nodes.
 *
 *
 * \par Date Written:
 *      26/03/20
 *
 * \return int
 *
 * \retval ">= 0"  Success case: length of the geographic route
 * \retval    -1   Arguments error
 * \retval    -2   Memory allocation error
 *
 * \param[in]       *route        The GeoRoute get by rgr_read, allocated by the caller.
 * \param[in,out]   resultLyst    The lyst of ipn nodes, allocated by the caller.
 *
 * \warning The route->nodes string must be well formed and has to follow
 *          the pattern: ipn:xx...ipn:yy... etc.
 *          Where: ... could be any character, xx is the node number and ; is the separator.
 *
 * \par Revision History:
 *
 *  MM/DD/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  26/03/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
int get_geo_route_lyst(GeoRoute *route, Lyst resultLyst)
{
	int result = -1, end = 0;
	char *temp;
	uvast ipn_node, *new_elt;

	if (route != NULL && resultLyst != NULL)
	{
		result = 0;
		end = 0;
		temp = route->nodes;
		while (!end)
		{
			temp = strstr(temp, "ipn:");
			if (temp != NULL && isdigit(*(temp + 4)))
			{
				temp = temp + 4;
				ipn_node = strtouvast(temp);
				new_elt = MTAKE(sizeof(uvast));
				if (new_elt != NULL)
				{
					*new_elt = ipn_node;
					if (lyst_insert_last(resultLyst, new_elt) == NULL)
					{
						MRELEASE(new_elt);
						end = 1;
						result = -2;
					}
					else
					{
						result++;
					}
				}
				else
				{
					end = 1;
					result = -2;
				}
			}
			else
			{
				end = 1;
			}
		}
	}

	return result;
}

void printRoute(GeoRoute *route)
{

}

void copyRoute(GeoRoute *dest, GeoRoute *src)
{

}

int parseAndSaveRoute(GeoRoute *route)
{
	return 1;
}

char* parseAdminEID(Sdr sdr, Bundle *bundle)
{
	MetaEid metaEid;
	VScheme *vscheme;
	PsmAddress vschemeElt;
	char proxNodeEid[SDRSTRING_BUFSZ];

	// as per libbpP func bpDequeue
	if (bundle->proxNodeEid)
	{
		sdr_begin_xn(sdr);
		sdr_string_read(sdr, proxNodeEid, bundle->proxNodeEid);
		sdr_end_xn(sdr);
	}

	/*
	 * Look at scheme we are delivering to, as that will be the
	 * scheme of our local admin EID, as we don't cross schemes
	 * in transmit.
	 */
	if (parseEidString(proxNodeEid, &metaEid, &vscheme, &vschemeElt) == 0)
	{
		/*	Can't know which admin EID to use.		*/
		return NULL;
	}

	restoreEidString(&metaEid);

	return vscheme->adminEid;
}
