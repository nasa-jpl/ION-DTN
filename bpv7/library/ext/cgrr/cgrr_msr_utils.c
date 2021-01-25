/** \file cgrr_msr_utils.c
 *
 *  \brief  This file provides the implementations of the functions to optimize the use of CGRR Extension Block
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

#include "cgrr_msr_utils.h"

/******************************************************************************
 *
 * \par Function Name:
 * 		encode_unique_cgrr_route
 *
 * \brief Encode unique route into CGRRouteBlock and save it into bundle.
 *
 * \details Other CGRR routes will be discarded.
 *
 *
 * \par Date Written:
 * 		25/09/20
 *
 * \return int
 *
 * \retval   0     Success case
 * \retval   -2    System error
 *
 * \param[in]	blk            The extension block (in volatile memory)
 * \param[in]	extBlkAddr     The extension block (in SDR)
 * \param[in]   bundle         The bundle (in volatile memory)
 * \param[in]   cgrr_route     The route to save into CGRR
 *
 * \warning This function assumes that all arguments are initialized.
 *
 *
 * \par Revision History:
 *
 *  DD/MM/YY | AUTHOR          |  DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  25/09/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
static int encode_unique_cgrr_route(ExtensionBlock *blk, Object extBlkAddr, Bundle *bundle, CGRRoute *cgrr_route) {
	unsigned int oldLength;
	unsigned char * tempBuffer;
	CGRRouteBlock tempBlk;
	uvast tempLength;
	int result;

	cgrr_debugPrint("[cgrr_msr_utils.c/encode_unique_cgrr_route] Encoding route...");

	oldLength = blk->length;

	// initialize CGRRouteBlock to 0
	memset(&tempBlk, 0 ,sizeof(tempBlk));

	memcpy(&(tempBlk.originalRoute), cgrr_route, sizeof(CGRRoute));
	tempBuffer = cgrr_serializeCGRR(&tempLength, &tempBlk);

	if(tempBuffer == NULL) {
		return -2;
	}

	blk->dataLength = tempLength;

	result = serializeExtBlk(blk, (char *) tempBuffer);
	MRELEASE(tempBuffer);
	if (result >= 0)
	{
		processModifiedExtensionBlock(bundle, extBlkAddr, blk, oldLength, blk->size);
		result = 0;

		cgrr_debugPrint("[cgrr_msr_utils.c/encode_unique_cgrr_route] Route encoded!");
	}
	else
	{
		return -2;
	}

	return result;


}

/******************************************************************************
 *
 * \par Function Name:
 * 		storeMsrRoute
 *
 * \brief Save into bundle a CGRR ext. Block with only one route.
 *
 * \details Other CGRR routes (found into previous CGRR block) will be discarded.
 *
 * \par Date Written:
 * 		25/09/20
 *
 * \return int
 *
 * \retval   0     Success case
 * \retval   -1    CGRR block not found
 * \retval   -2    System error
 *
 * \param[in]   cgrr_route     The route to save into CGRR
 * \param[in]   bundle         The bundle (in volatile memory)
 *
 *
 * \par Revision History:
 *
 *  DD/MM/YY | AUTHOR          |  DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  25/09/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
int storeMsrRoute(CGRRoute *cgrr_route, Bundle* bundle) {
	Sdr 			sdr = getIonsdr();
	Object 		elt;
	Object		extBlkAddr;

	CHKERR(cgrr_route != NULL);
	CHKERR(bundle != NULL);
    CHKERR(cgrr_route->hopList != NULL);
    CHKERR(cgrr_route->hopCount);

    cgrr_debugPrint("[cgrr_msr_utils.c/storeMsrRoute] Entry point.");

	if ((elt = findExtensionBlock(bundle, CGRRBlk, 0)) <= 0 )
	{
		cgrr_debugPrint("[cgrr_msr_utils.c/storeMsrRoute] No CGRR Extension Block found in bundle.");
		return -1;
	}

	extBlkAddr = sdr_list_data(sdr, elt);
	CHKERR(extBlkAddr != 0);

	OBJ_POINTER(ExtensionBlock, blk);

	GET_OBJ_POINTER(sdr, ExtensionBlock, blk, extBlkAddr);

	return encode_unique_cgrr_route(blk, extBlkAddr, bundle, cgrr_route);
}

/******************************************************************************
 *
 * \par Function Name:
 * 		updateLastCgrrRoute
 *
 * \brief This function takes the last route encoded into CGRR and remove all hops until the first hop
 *        of the CGRR route is the contact with this local node as "sender node"; then this reduced route
 *        will be the only route encoded into CGRR (so other routes will be discarded).
 *
 * \details Helper function to decrease progressively CGRR's overhead size for Moderate Source Routing.
 *
 * \par Date Written:
 * 		25/09/20
 *
 * \return int
 *
 * \retval   0     Success case
 * \retval   -1    CGRR block not found
 * \retval   -2    System error
 * \retval   -3    CGRR hasn't been updated
 *
 * \param[in]   bundle         The bundle (in volatile memory)
 *
 *
 * \par Revision History:
 *
 *  DD/MM/YY | AUTHOR          |  DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  25/09/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
int updateLastCgrrRoute(Bundle *bundle) {
	Sdr 			sdr = getIonsdr();
	int 			result = 1;
	Object			extBlkAddr, extBlkElt;
	CGRRouteBlock *cgrrBlk;
	CGRRoute *route, *aux_route;
	int count;
	uvast localNode;

	cgrr_debugPrint("[cgrr_msr_utils.c/updateLastCgrrRoute] Entry point.");

	if ((extBlkElt = findExtensionBlock(bundle, CGRRBlk, 0)) <= 0 )
	{
		cgrr_debugPrint("[cgrr_msr_utils.c/updateLastCgrrRoute] No CGRR Extension Block found in bundle.");
		return -1;
	}

	extBlkAddr = sdr_list_data(sdr, extBlkElt);
	CHKERR(extBlkAddr != 0); //Added by F. Marchetti

	cgrrBlk = MTAKE(sizeof(CGRRouteBlock));

	if(cgrrBlk == NULL) {
		return -2;
	}

	memset(cgrrBlk, 0, sizeof(CGRRouteBlock));

	OBJ_POINTER(ExtensionBlock, blk);

	GET_OBJ_POINTER(sdr, ExtensionBlock, blk, extBlkAddr);

	result = cgrr_getCGRRFromExtensionBlock(blk, cgrrBlk);

	if( result < 0)
	{
		cgrr_debugPrint("[cgrr_msr_utils.c/updateLastCgrrRoute] Cannot extract CGRR from Ext. Block.");
		releaseCgrrBlkMemory(cgrrBlk);
		return -2;
	}
	else if (result == 0)
	{
		cgrr_debugPrint("[cgrr_msr_utils.c/updateLastCgrrRoute] CGRR didn't pass sanity checks.");
		releaseCgrrBlkMemory(cgrrBlk);
		return -1;
	}

	if(cgrrBlk->recRoutesLength > 0)
	{
		aux_route = &(cgrrBlk->recomputedRoutes[cgrrBlk->recRoutesLength - 1]);
	}
	else
	{
		aux_route = &(cgrrBlk->originalRoute);
	}

	localNode = getOwnNodeNbr();

	for(count = 0; count < aux_route->hopCount; count++) {
		if(aux_route->hopList[count].fromNode == localNode) {
			break;
		}
	}

	//we don't touch the extension block if the first contact's sender node of the CGRR Route
	//is the local node or if we don't find the local node in the set of sender nodes of the route.
	if(count != 0 && count != aux_route->hopCount) {
		route = MTAKE(sizeof(CGRRoute));

		if(route == NULL) {
			releaseCgrrBlkMemory(cgrrBlk);
			return -2;
		}

		route->hopCount = aux_route->hopCount - count;
		route->hopList = MTAKE(sizeof(CGRRHop) * route->hopCount);

		if(route->hopList == NULL) {
			releaseCgrrBlkMemory(cgrrBlk);
			MRELEASE(route);
			return -2;
		}

		memcpy(route->hopList, &(aux_route->hopList[count]), sizeof(CGRRHop) * route->hopCount);

		result = encode_unique_cgrr_route(blk, extBlkAddr, bundle, route);

		MRELEASE(route->hopList);
		MRELEASE(route);
		releaseCgrrBlkMemory(cgrrBlk);

	}
	else {
		cgrr_debugPrint("[cgrr_msr_utils.c/updateLastCgrrRoute] No modifications needed for CGRR Ext. Block.");
		releaseCgrrBlkMemory(cgrrBlk);
		return -3;
	}


	return result;

}



