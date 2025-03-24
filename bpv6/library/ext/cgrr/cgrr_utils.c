/** \file cgrr_utils.c
 *
 *	\brief This file provides utility functions necessary for a
 *		   full implementation of CGR Route Extension Block.
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

#include "cgrr.h"
#include "cgrr_help.h"

/*	We hitchhike on the ZCO heap space management system to
 *	manage the space occupied by Bundle objects.  In effect,
 *	the Bundle overhead objects compete with ZCOs for available
 *	SDR heap space.  We don't want this practice to become
 *	widespread, which is why these functions are declared
 *	privately here rather than publicly in the zco.h header.	*/

extern void	zco_increase_heap_occupancy(Sdr sdr, vast delta, ZcoAcct acct);
extern void	zco_reduce_heap_occupancy(Sdr sdr, vast delta, ZcoAcct acct);

void printCGRRoute(CGRRoute *cgrRoute)
{
	int j;

	CHKVOID(cgrRoute);

	cgrr_debugPrint("[cgrr_utils.c/printCGRRoute] Hop Count= %u\n Printing Hop List...", cgrRoute->hopCount);

	for (j = 0; j < cgrRoute->hopCount; j++)
	{
		cgrr_debugPrint("hopList[%d].fromNode = " UVAST_FIELDSPEC ".",
							j, cgrRoute->hopList[j].fromNode);
		cgrr_debugPrint("hopList[%d].toNode = " UVAST_FIELDSPEC ".",
							j, cgrRoute->hopList[j].toNode);
		cgrr_debugPrint("hopList[%d].fromTime = %ld.",
							j, cgrRoute->hopList[j].fromTime - ionReferenceTime(NULL));
	}
	cgrr_debugPrint("[cgrr_utils.c/printCGRRoute] done.");
}

void printCGRRouteBlock(CGRRouteBlock *cgrrBlk)
{
	int i;

		CHKVOID(cgrrBlk);

		cgrr_debugPrint("[cgrr_utils.c/printCGRRouteBlock] Printing Original Route...");

		printCGRRoute(&(cgrrBlk->originalRoute));

		cgrr_debugPrint("[cgrr_utils.c/printCGRRouteBlock] done.");

		cgrr_debugPrint("[cgrr_utils.c/printCGRRouteBlock] Recomputed routes length: %u\n Printing Recomputed Routes...", cgrrBlk->recRoutesLength);

		for (i = 0; i < cgrrBlk->recRoutesLength; i++)
		{
			printCGRRoute(&(cgrrBlk->recomputedRoutes[i]));
		}

		cgrr_debugPrint("[cgrr_utils.c/printCGRRouteBlock] done.");
}

/******************************************************************************
 *
 * \par Function Name: cgrr_serializeCGRR
 *
 * \par Purpose: Serializes an outbound bundle cgrr block and returns the
 *               serialized representation.
 *
 * \par Date Written:  23/10/18
 *
 * \return unsigned char *
 *
 * \retval "unsigned char *" the serialized outbound bundle CGRR Block.
 *
 * \param[out] length The length of the serialized block.
 * \param[in]  cgrrBlk    The CGRRouteBlock to serialize.
 *
 * \par Notes:
 *      1. This function uses MTAKE to allocate space for the serialized CGRRouteBlock.
 *         This serialized CGRRouteBlock (if not NULL) must be freed using MRELEASE.
 *
 * \par Revision History:
 *
 *  MM/DD/YY | AUTHOR         | SPR#  |   DESCRIPTION
 *  -------- | ------------   | ----- | ------------------------------------------
 *  23/10/18 | L. Mazzuca     |       | Initial Implementation and documentation.
 *****************************************************************************/
unsigned char *cgrr_serializeCGRR(uvast *length, CGRRouteBlock *cgrrBlk)
{
	unsigned char	*serializedCgrr;
	unsigned char	*cursor;
	int			i, j, k, l = 0;

	int nHopCount = 2 + cgrrBlk->recRoutesLength;
	int nHops = cgrrBlk->originalRoute.hopCount;

	for (i = 0; i < cgrrBlk->recRoutesLength; i++)
		nHops += cgrrBlk->recomputedRoutes[i].hopCount;

	/* Step 0 - Create SDNV variables. */

	Sdnv		fromNode[nHops];
	Sdnv		toNode[nHops];
	Sdnv		fromTime[nHops];
	Sdnv		hopCount[nHopCount];
	Sdnv		recRoutesLength;


	/* Step 1 - Sanity Checks. */

	CHKNULL(length);
	CHKNULL(cgrrBlk);

	//cgrr_debugPrint("cgrr_serializeCGRR: Block dataLength = %u", *length);
	*length = 0;
	//cgrr_debugPrint("cgrr_serializeCGRR: Block dataLength = %u", *length);

	/* Step 2 - Encode CGRR Block parameters in SDNV values.
	 * 			This step is needed because we need to know
	 * 			how many bytes the values will take up.
	 * 			Also, encoding follows logic of insertion.		 */


	/* Step 2.1 - Encode original route. */

	encodeSdnv(&(hopCount[l]), cgrrBlk->originalRoute.hopCount);
	*length += hopCount[l].length;
	l++;

	for (i = 0; i < cgrrBlk->originalRoute.hopCount; i++) {
		encodeSdnv(&fromNode[i], cgrrBlk->originalRoute.hopList[i].fromNode);
		*length += fromNode[i].length;
		encodeSdnv(&toNode[i], cgrrBlk->originalRoute.hopList[i].toNode);
		*length += toNode[i].length;
		encodeSdnv(&fromTime[i], cgrrBlk->originalRoute.hopList[i].fromTime);
		*length += fromTime[i].length;
	}

	/* Step 2.2 - Encode recomputed routes. */

	encodeSdnv(&recRoutesLength, cgrrBlk->recRoutesLength);
	*length += recRoutesLength.length;

	for (j = 0; j < cgrrBlk->recRoutesLength; j++, l++)
	{
		encodeSdnv(&hopCount[l], cgrrBlk->recomputedRoutes[j].hopCount);
		*length += hopCount[l].length;

		for ( k = 0; k < cgrrBlk->recomputedRoutes[j].hopCount; k++, i++) {
				encodeSdnv(&fromNode[i], cgrrBlk->recomputedRoutes[j].hopList[k].fromNode);
				*length += fromNode[i].length;
				encodeSdnv(&toNode[i], cgrrBlk->recomputedRoutes[j].hopList[k].toNode);
				*length += toNode[i].length;
				encodeSdnv(&fromTime[i], cgrrBlk->recomputedRoutes[j].hopList[k].fromTime);
				*length += fromTime[i].length;
			}

	}

	/*********************************************************************
	 *            Serialize the CGRRB into the allocated buffer          *
	 *********************************************************************/
//*length o sizeof(*length)
	//Modified by G.M. De Cola
	if ((serializedCgrr = (unsigned char *)MTAKE(*length)) == NULL)
	{
		cgrr_debugPrint("[x: cgrr_utils/cgrr_serializeCGRR] %d bytes needed but unavailable .", *length);
		return NULL;
	}

	l = 0;

	cursor = serializedCgrr;

	/* Step 4.1 - Serialize original route. */
	cursor = cgrr_addSdnvToStream(cursor, &hopCount[l]);
	l++;
	for ( i = 0; i < cgrrBlk->originalRoute.hopCount; i++)
	{
		cursor = cgrr_addSdnvToStream(cursor, &fromNode[i]);
		cursor = cgrr_addSdnvToStream(cursor, &toNode[i]);
		cursor = cgrr_addSdnvToStream(cursor, &fromTime[i]);
	}

	/* Step 4.2 - Serialize recomputed routes. */

	cursor = cgrr_addSdnvToStream(cursor, &recRoutesLength);

	for (j = 0; j < cgrrBlk->recRoutesLength; j++, l++)
	{
		cursor = cgrr_addSdnvToStream(cursor, &hopCount[l]);

		for ( k = 0; k < cgrrBlk->recomputedRoutes[j].hopCount; k++, i++)
		{
			cursor = cgrr_addSdnvToStream(cursor, &fromNode[i]);
			cursor = cgrr_addSdnvToStream(cursor, &toNode[i]);
			cursor = cgrr_addSdnvToStream(cursor, &fromTime[i]);
		}
	}

	//cgrr_debugPrint("cgrr_serializeCGRR: Block dataLength = %u", *length);

	return serializedCgrr;

}



/******************************************************************************
 *
 * \par Function Name: cgrr_deserializeCGRR
 *
 * \par Purpose: This utility function accepts a serialized CGRRouteBlock
 *               from a bundle during acquisition and places it in a
 *               CGRRouteBlock structure stored in the Acquisition Extension
 *               Block's scratchpad area.
 *
 * \par Date Written:  02/11/18
 *
 * \return int
 *
 * \retval  1  A CGRRouteBlock was successfully deserialized into the scratchpad
 * \retval  0  The deserialized CGRRouteBlock did not pass its sanity check.
 * \retval -1  There was a system error.
 *
 * \param[in,out]  blk  A pointer to the acquisition block holding the
 *                      serialized abstract security block.
 * \param[in]      wk   Work area holding bundle information.
 *
 * \par Notes:
 *      1. This function allocates memory using the MTAKE method.  This
 *         scratchpad must be freed by the caller if the method does
 *         not return -1.  Any system error will release the memory.
 *
 * \par Revision History:
 *
 *  MM/DD/YY | AUTHOR         |  SPR#  |   DESCRIPTION
 *  -------- | ------------   | ------ | -----------------------------------------
 *  02/11/18 | L. Mazzuca     |        | Initial Implementation.
 *  13/04/19 | L. Mazzuca	  |	       | Cut part on saving in Object.
 *****************************************************************************/

int	cgrr_deserializeCGRR(AcqExtBlock *blk, AcqWorkArea *wk)
{
	int				result = 1;
	int				totalHopCount = 0;
	int				i, j;
	CGRRouteBlock	cgrrBlk;
	unsigned char	*cursor = NULL;
	int				unparsedBytes = 0;
	unsigned int 	itemp = 0;
	uvast 			temp;

	/* Step 0.1 - Sanity checks. */
	CHKERR(blk);
	CHKERR(wk);

	//cgrr_debugPrint("i cgrr_deserializeCGRR blk length %u, blk dataLength %u", blk->length, blk->dataLength);
	unparsedBytes = blk->length;

	/* Step 0.2 - Initialize CGRRouteBlock to host serialized block. */
	memset((char *) &cgrrBlk, 0, sizeof(CGRRouteBlock));

	//cgrr_debugPrint("i cgrr_deserializeCGRR memset of CGRRouteBlock done.");
	//cgrr_debugPrint("i cgrr_deserializeCGRR HC=%u", cgrrBlk.originalRoute.hopCount);

	/* Step 0.3 - Position cursor to start of block-type-specific data of the
	 * 			  extension block, by skipping over the extension block header. */

	cursor = ((unsigned char *)(blk->bytes))
			+ (blk->length - blk->dataLength);

	/*********************************************************************
	 *          Deserialize the CGRRB into the new CGRRouteBlock         *
	 *********************************************************************/

	/* Step 1 - The deserialization MUST, obviously, follow the order the CGRRouteBlock
	 * 			was serialized in. */

	/* Step 1.1 - Deserialize original route. */
	extractSmallSdnv(&itemp, &cursor, &unparsedBytes);
	cgrrBlk.originalRoute.hopCount = itemp;
	totalHopCount = cgrrBlk.originalRoute.hopCount;

	//cgrr_debugPrint("i cgrr_deserializeCGRR extracted original route HC of=%u", itemp);
	//cgrr_debugPrint("i cgrr_deserializeCGRR value in cgrrBlk=%u", cgrrBlk.originalRoute.hopCount);

	/* Step 1.2 - If the Hop Count is grater than 0, we allocate memory.
	 * 				If there is no hop count, it means that a orute was not computer at the source
	 * 				so the block is not needed and we can scratch it. */
	if (cgrrBlk.originalRoute.hopCount > 0)
	{
		//Modified by G.M. De Cola
		cgrrBlk.originalRoute.hopList = (CGRRHop*) MTAKE(sizeof(CGRRHop)*cgrrBlk.originalRoute.hopCount);
		if (cgrrBlk.originalRoute.hopList == NULL)
		{
			cgrr_debugPrint("[i: cgrr_utils.c/cgrr_deserializeCGRR] not enough space for original route Hop List. Need %d bytes.",
					(sizeof(CGRRHop)*cgrrBlk.originalRoute.hopCount));

		}

		memset((char *) cgrrBlk.originalRoute.hopList, 0, (sizeof(CGRRHop)*cgrrBlk.originalRoute.hopCount));
	}

	for ( i = 0; i < cgrrBlk.originalRoute.hopCount; i++) {
		extractSdnv(&temp, &cursor, &unparsedBytes);
		cgrrBlk.originalRoute.hopList[i].fromNode = temp;
		//cgrr_debugPrint("i cgrr_deserializeCGRR extracted original route H[%d].fromNode of= " UVAST_FIELDSPEC ".", i, temp);
		//cgrr_debugPrint("i cgrr_deserializeCGRR value in cgrrBlk= " UVAST_FIELDSPEC ".", cgrrBlk.originalRoute.hopList[i].fromNode);
		extractSdnv(&temp, &cursor, &unparsedBytes);
		cgrrBlk.originalRoute.hopList[i].toNode = temp;
		//cgrr_debugPrint("i cgrr_deserializeCGRR extracted original route H[%d].toNode of=" UVAST_FIELDSPEC ".", i, temp);
		//cgrr_debugPrint("i cgrr_deserializeCGRR value in cgrrBlk=" UVAST_FIELDSPEC ".", cgrrBlk.originalRoute.hopList[i].toNode);
		extractSdnv(&temp, &cursor, &unparsedBytes);
		cgrrBlk.originalRoute.hopList[i].fromTime = temp;
		//cgrr_debugPrint("i cgrr_deserializeCGRR extracted original route H[%d].fromTime of=" UVAST_FIELDSPEC ".", i, temp);
		//cgrr_debugPrint("i cgrr_deserializeCGRR value in cgrrBlk=%ld.", cgrrBlk.originalRoute.hopList[i].fromTime);
	}

	/* Step 1.3 - If the recomputed legth is greater than zero, we allocate memory to the recomputed routes.
	 * 				else it is not needed, so go on leaving the recomputedRoutes empty. */
	extractSmallSdnv(&itemp, &cursor, &unparsedBytes);
	cgrrBlk.recRoutesLength = itemp;
	cgrr_debugPrint("[i: cgrr_utils.c/cgrr_deserializeCGRR] extracted recomputed length of=%u", itemp);
	cgrr_debugPrint("[i: cgrr_utils.c/cgrr_deserializeCGRR] value in cgrrBlk=%u", cgrrBlk.recRoutesLength);


	if (cgrrBlk.recRoutesLength > 0)
	{
		//Modified by G.M. De Cola
		cgrrBlk.recomputedRoutes = (CGRRoute*) MTAKE(sizeof(CGRRoute)*cgrrBlk.recRoutesLength);
		if (cgrrBlk.recomputedRoutes == NULL)
		{
			cgrr_debugPrint("[i: cgrr_utils.c/cgrr_deserializeCGRR] not enough space for recomputed routes. Need %d bytes.",
					(sizeof(CGRRoute)*cgrrBlk.recRoutesLength));

		}
		memset((char *) cgrrBlk.recomputedRoutes, 0, (sizeof(CGRRoute)*cgrrBlk.recRoutesLength));
	}


	/* Step 1.4 - If the recomputed legth is greater than one we will be allocating memory for
	 * 				each ecomputed route's hopList, just like we did for the original route. */
	for (j = 0; j < cgrrBlk.recRoutesLength; j++)
	{
		extractSmallSdnv(&itemp, &cursor, &unparsedBytes);
		cgrrBlk.recomputedRoutes[j].hopCount = itemp;
		totalHopCount += cgrrBlk.recomputedRoutes[j].hopCount;
		//cgrr_debugPrint("i cgrr_deserializeCGRR extracted recomputed route %d HC of=%u", j, itemp);
		//cgrr_debugPrint("i cgrr_deserializeCGRR value in cgrrBlk=%u", cgrrBlk.recomputedRoutes[j].hopCount);

		//Modified by G.M. De Cola
		cgrrBlk.recomputedRoutes[j].hopList = (CGRRHop*) MTAKE(sizeof(CGRRHop)*cgrrBlk.recomputedRoutes[j].hopCount);
		if (cgrrBlk.recomputedRoutes[j].hopList == NULL)
		{
			cgrr_debugPrint("[i: cgrr_utils.c/cgrr_deserializeCGRR] not enough space for recomputed route %d Hop List. Need %d bytes.",
					j, (sizeof(CGRRHop)*cgrrBlk.recomputedRoutes[j].hopCount));

		}
		memset((char *) cgrrBlk.recomputedRoutes[j].hopList, 0, (sizeof(CGRRHop)*cgrrBlk.recomputedRoutes[j].hopCount));

		for ( i = 0; i < cgrrBlk.recomputedRoutes[j].hopCount; i++)
		{
			extractSdnv(&temp, &cursor, &unparsedBytes);
			cgrrBlk.recomputedRoutes[j].hopList[i].fromNode = temp;
			//cgrr_debugPrint("[i: cgrr_utils.c/cgrr_deserializeCGRR] extracted recomputed route %d H[%d].fromNode of=" UVAST_FIELDSPEC ".",j, i, temp);
			//cgrr_debugPrint("[i: cgrr_utils.c/cgrr_deserializeCGRR] value in cgrrBlk=" UVAST_FIELDSPEC ".", cgrrBlk.recomputedRoutes[j].hopList[i].fromNode);
			extractSdnv(&temp, &cursor, &unparsedBytes);
			cgrrBlk.recomputedRoutes[j].hopList[i].toNode = temp;
			//cgrr_debugPrint("[i: cgrr_utils.c/cgrr_deserializeCGRR] extracted recomputed route %d H[%d].toNode of=" UVAST_FIELDSPEC ".", j, i, temp);
			//cgrr_debugPrint("[i: cgrr_utils.c/cgrr_deserializeCGRR] value in cgrrBlk=" UVAST_FIELDSPEC ".", cgrrBlk.recomputedRoutes[j].hopList[i].toNode);
			extractSdnv(&temp, &cursor, &unparsedBytes);
			cgrrBlk.recomputedRoutes[j].hopList[i].fromTime = temp;
			//cgrr_debugPrint("[i: cgrr_utils.c/cgrr_deserializeCGRR] extracted recomputed route %d H[%d].fromTime of= %ld.", j, i, temp - ionReferenceTime(NULL));
			//cgrr_debugPrint("[i: cgrr_utils.c/cgrr_deserializeCGRR] value in cgrrBlk= %ld.", cgrrBlk.recomputedRoutes[j].hopList[i].fromTime - ionReferenceTime(NULL));
		}
	}

	/* Step 2 - The block has been deserialized. Now we need to tell to the Extension Block the CGRRouteBlock's size
	 * 			and allocate enough memory in the Object scratchpad to store it. */

	//Modified by G.M. De Cola
	//blk->size = sizeof(CGRRouteBlock) + (sizeof(CGRRoute) * cgrrBlk.recRoutesLength) + (sizeof(CGRRHop) * totalHopCount);
/*	blk->size = sizeof(unsigned int) + sizeof(CGRRoute) + (sizeof(CGRRoute) * cgrrBlk.recRoutesLength) + (sizeof(CGRRHop) * totalHopCount);
	//cgrr_debugPrint("[i: cgrr_utils.c/cgrr_deserializeCGRR] size of block set to %u", blk->size);
	
	blk->object = MTAKE(blk->size);
	if (blk->object == NULL)
	{
		cgrr_debugPrint("[x: cgrr_utils.c/cgrr_deserializeCGRR] No space for CGRRouteBlock scratchpad", NULL);
		return -1;
	}

	memcpy((char *) (blk->object), (char *) &cgrrBlk, blk->size);

	//cgrr_debugPrint("- cgrr_deserializeCGRR -> %d", result);
*/
	printCGRRouteBlock(&cgrrBlk);

	return result;
}

/******************************************************************************
 *
 * \par Function Name: cgrr_getCGRRFromExtensionBlock
 *
 * \par Purpose: This utility function accepts an sdr Extension Block and
 * 				 an empty but initialized CGRRouteBlock to fill with the
 * 				 deserialized information. We use this function because,
 * 				 since CGRRouteBlock is a dynamic structure, we cannot retrieve
 * 				 the information about the routes and the hop lists in a single
 * 				 sdr_read; we instead need to retrieve the information from the
 * 				 serialization that is stored in the Extension Block Object value
 * 				 "bytes".
 *
 * \par Date Written:  22/11/18
 *
 * \return int
 *
 * \retval  1  A CGRRouteBlock was successfully deserialized into the given
 * 			   structure.
 * \retval  0  The deserialized CGRRouteBlock did not pass its sanity check.
 * \retval -1  There was a system error.
 *
 * \param[in]      blk  	A pointer to the ExtensionBlock from which we need to
 * 							retrieve the CGRRouteBlock.
 * \param[in, out] cgrrBlk  Pointer to an initialized but empty CGRRouteBlock to be
 * 							filled with deserialized information.
 *
 * \par Notes:
 *      1. This function allocates memory using the MTAKE method.  The
 *         block must be freed by the caller if the method does
 *         not return -1.  Any system error will release the memory.
 *
 * \par Revision History:
 *
 *  MM/DD/YY | AUTHOR       | SPR# |   DESCRIPTION
 *  -------- | ------------ | -----| ------------------------------------------
 *  22/11/18 | L. Mazzuca   |      |  Initial Implementation.
 *  07/04/19 | L. Mazzuca	|	   |  Modified return values.
 *****************************************************************************/

int	cgrr_getCGRRFromExtensionBlock(ExtensionBlock *blk, CGRRouteBlock *cgrrBlk)
{
	Sdr 			sdr = getIonsdr();
	int				totalHopCount = 0;
	int				i, j;
	unsigned char	*cursor = NULL;
	int				unparsedBytes = 0;
	unsigned int 	itemp;
	uvast			temp;
	unsigned char	*blkBuffer;
	/* Step 0.1 - Sanity checks. */
	CHKERR(blk);
	CHKERR(cgrrBlk);

	//	cgrr_debugPrint("cgrr_record: beginning structures initialization...");
	//	sdrCgrrBlk.recRoutesLength = 0;
	//	sdrCgrrBlk.originalRoute.hopCount = 0;
	//	sdrCgrrBlk.originalRoute.hopList = NULL;
	//	sdrCgrrBlk.recomputedRoutes= NULL;
	//
	//	cgrr_debugPrint("cgrr_record: structures initialized");
	//
	//	appo = (CGRRouteBlock*)ramBlk->object;
	//	sdrCgrrBlk.recRoutesLength = appo->recRoutesLength;
	//	sdrCgrrBlk.originalRoute.hopCount = appo->originalRoute.hopCount;
	//	sdrCgrrBlk.originalRoute.hopList = (Hop*) malloc(sizeof(Hop) * appo->originalRoute.hopCount);
	//	memcpy((char*) sdrCgrrBlk.originalRoute.hopList, appo->originalRoute.hopList, sizeof(Hop) * appo->originalRoute.hopCount);
	//	sdrCgrrBlk.recomputedRoutes = (CGRRoute* ) malloc(sizeof(CGRRoute) * appo->recRoutesLength);
	//
	//	cgrr_debugPrint("cgrr_record: beginning data extraction...");
	//
	//	int myIndex;
	//
	//	for (myIndex = 0; myIndex < appo->recRoutesLength; ++myIndex)
	//	{
	//		sdrCgrrBlk.recomputedRoutes[myIndex].hopCount = appo->recomputedRoutes[myIndex].hopCount;
	//		sdrCgrrBlk.recomputedRoutes[myIndex].hopList = (Hop*) malloc(sizeof(Hop) * appo->recomputedRoutes[myIndex].hopCount);
	//		memcpy(sdrCgrrBlk.recomputedRoutes[myIndex].hopList,
	//				appo->recomputedRoutes[myIndex].hopList,
	//				sizeof(Hop) * appo->recomputedRoutes[myIndex].hopCount);
	//	}
	//	/***********************************************/
	//
	//	cgrr_debugPrint("cgrr_record: completed data extraction");
	//
	//
	//	cgrr_debugPrint("cgrr_record: printing retrieved CGRRouteBlock...");
	//	printCGRRouteBlock(&sdrCgrrBlk);



	//cgrr_debugPrint("i cgrr_getCGRRFromExtensionBlock blk length %u, blk dataLength %u", blk->length, blk->dataLength);
	unparsedBytes = blk->length;

	//Modified by G.M. De Cola
	blkBuffer = (unsigned char *) MTAKE(blk->length);
	if (blkBuffer == NULL)
	{
		cgrr_debugPrint("[cgrr_utils.c/cgrr_getCGRRFromExtensionBlock] No space for block buffer %u.", blk->length);
		return -1;
	}

	CHKERR(sdr_begin_xn(sdr));
	sdr_read(sdr, (char *) blkBuffer, blk->bytes, blk->length);
	sdr_end_xn(sdr);

	/* Step 0.3 - Position cursor to start of block-type-specific data of the
	 * 			  extension block, by skipping over the extension block header. */

	cursor = (blkBuffer) + (blk->length - blk->dataLength);

	/*********************************************************************
	 *          Deserialize the CGRRB into the new CGRRouteBlock             *
	 *********************************************************************/

	/* Step 1 - The deserialization MUST, obviously, follow the order the CGRRouteBlock
	 * 			was serialized in. */

	/* Step 1.1 - Deserialize original route. */
	extractSmallSdnv(&itemp, &cursor, &unparsedBytes);
	cgrrBlk->originalRoute.hopCount = itemp;
	totalHopCount = cgrrBlk->originalRoute.hopCount;

	cgrr_debugPrint("[i: cgrr_utils.c/cgrr_getCGRRFromExtensionBlock] extracted original route HC of=%u", itemp);
	cgrr_debugPrint("[i: cgrr_utils.c/cgrr_getCGRRFromExtensionBlock] value in cgrrBlk=%u", cgrrBlk->originalRoute.hopCount);

	/* Step 1.2 - If the Hop Count is greater than 0, we allocate memory.
	 * 				If there is no hop count, it means that a route was either not computed at the source
	 * 				or it's being processed at source node.
	 */
	if (cgrrBlk->originalRoute.hopCount > 0)
	{
		//Modified by G.M. De Cola
		cgrrBlk->originalRoute.hopList = (CGRRHop*) MTAKE(sizeof(CGRRHop)*cgrrBlk->originalRoute.hopCount);
		if (cgrrBlk->originalRoute.hopList == NULL)
		{
			cgrr_debugPrint("[i: cgrr_utils.c/cgrr_getCGRRFromExtensionBlock] not enough space for original route Hop List. Need %d bytes.",
					(sizeof(CGRRHop)*cgrrBlk->originalRoute.hopCount));
			//Added by G.M. De Cola
			MRELEASE(blkBuffer);
			return -1;

		}
		//Commented by G.M. De Cola
		//cgrrBlk->originalRoute.hopList = MTAKE(sizeof(CGRRHop)*cgrrBlk->originalRoute.hopCount);
	}

	for ( i = 0; i < cgrrBlk->originalRoute.hopCount; i++) {
		extractSdnv(&temp, &cursor, &unparsedBytes);
		cgrrBlk->originalRoute.hopList[i].fromNode = temp;
		//cgrr_debugPrint("i cgrr_getCGRRFromExtensionBlock extracted original route H[%d].fromNode of=" UVAST_FIELDSPEC ".", i, temp);
		//cgrr_debugPrint("i cgrr_getCGRRFromExtensionBlock value in cgrrBlk=" UVAST_FIELDSPEC ".", cgrrBlk->originalRoute.hopList[i].fromNode); //Modified by F. Marchetti
		extractSdnv(&temp, &cursor, &unparsedBytes);
		cgrrBlk->originalRoute.hopList[i].toNode = temp;
		//cgrr_debugPrint("i cgrr_getCGRRFromExtensionBlock extracted original route H[%d].toNode of=" UVAST_FIELDSPEC ".", i, temp);
		//cgrr_debugPrint("i cgrr_getCGRRFromExtensionBlock value in cgrrBlk=" UVAST_FIELDSPEC ".", cgrrBlk->originalRoute.hopList[i].toNode); //Modified by F. Marchetti
		extractSdnv(&temp, &cursor, &unparsedBytes);
		cgrrBlk->originalRoute.hopList[i].fromTime = temp;
		//cgrr_debugPrint("i cgrr_getCGRRFromExtensionBlock extracted original route H[%d].fromTime of=" UVAST_FIELDSPEC ".", i, temp);
		//cgrr_debugPrint("i cgrr_getCGRRFromExtensionBlock value in cgrrBlk= %ld.", cgrrBlk->originalRoute.hopList[i].fromTime); //Modified by F. Marchetti
	}

	/* Step 1.3 - If the recomputed length is greater than zero, we allocate memory to the recomputed routes.
	 * 				else it is not needed, so go on leaving the recomputedRoutes empty. */
	extractSmallSdnv(&itemp, &cursor, &unparsedBytes);
	cgrrBlk->recRoutesLength = itemp;
	//cgrr_debugPrint("i cgrr_getCGRRFromExtensionBlock extracted recomputed length of=%u", itemp);
	//cgrr_debugPrint("i cgrr_getCGRRFromExtensionBlock value in cgrrBlk=%u", cgrrBlk->recRoutesLength); //Modified by F. Marchetti


	/* Step 1.3.1 - In this case, if the recomputed routes length is not greater than zero we can fall through
	 * 				automatically skipping the for cycle, because this may be the second+ node using CGR so there
	 * 				is no recomputed route yet.
	 */
	if (cgrrBlk->recRoutesLength > 0)
	{
		//Modified by G.M. De Cola
		cgrrBlk->recomputedRoutes = (CGRRoute*) MTAKE(sizeof(CGRRoute)*cgrrBlk->recRoutesLength);
		if (cgrrBlk->recomputedRoutes == NULL)
		{
			cgrr_debugPrint("[i: cgrr_utils.c/cgrr_getCGRRFromExtensionBlock] not enough space for recomputed routes. Need %d bytes.",
					(sizeof(CGRRoute)*cgrrBlk->recRoutesLength));
			cgrr_debugPrint("[cgrr_utils.c/cgrr_getCGRRFromExtensionBlock] result -> %d", -1);
			//Added by G.M. De Cola
			MRELEASE(blkBuffer);
			return -1;
		}
	}

	/* Step 1.4 - If the reocmputed legth is greater than one we will be allocating memory for
	 * 				each ecomputed route's hopList, just like we did for the original route. */
	for (j = 0; j < cgrrBlk->recRoutesLength; j++)
	{
		extractSmallSdnv(&itemp, &cursor, &unparsedBytes);
		cgrrBlk->recomputedRoutes[j].hopCount = itemp;
		totalHopCount += cgrrBlk->recomputedRoutes[j].hopCount;
		//cgrr_debugPrint("i cgrr_getCGRRFromExtensionBlock extracted recomputed route %d HC of=%u", j, temp);
		//cgrr_debugPrint("i cgrr_getCGRRFromExtensionBlock value in cgrrBlk=%u", cgrrBlk->recomputedRoutes[j].hopCount);

		//Modified by G.M. De Cola
		cgrrBlk->recomputedRoutes[j].hopList = (CGRRHop*) MTAKE(sizeof(CGRRHop)*cgrrBlk->recomputedRoutes[j].hopCount); //Modified by F. Marchetti
		if (cgrrBlk->recomputedRoutes[j].hopList == NULL)
		{
			cgrr_debugPrint("[i: cgrr_utils.c/cgrr_getCGRRFromExtensionBlock] not enough space for recomputed route %d Hop List. Need %d bytes.",
					j, (sizeof(CGRRHop)*cgrrBlk->recomputedRoutes[j].hopCount));
			cgrr_debugPrint("[cgrr_utils.c/cgrr_getCGRRFromExtensionBlock] result -> %d", -1);
			//Added by G.M. De Cola
			MRELEASE(blkBuffer);
			return -1;
		}

		for ( i = 0; i < cgrrBlk->recomputedRoutes[j].hopCount; i++) {
				extractSdnv(&temp, &cursor, &unparsedBytes);
				cgrrBlk->recomputedRoutes[j].hopList[i].fromNode = temp;
				//cgrr_debugPrint("i cgrr_getCGRRFromExtensionBlock extracted recomputed route %d H[%d].fromNode of=" UVAST_FIELDSPEC ".",j, i, temp);
				//cgrr_debugPrint("i cgrr_getCGRRFromExtensionBlock value in cgrrBlk=" UVAST_FIELDSPEC ".", cgrrBlk->recomputedRoutes[j].hopList[i].fromNode);
				extractSdnv(&temp, &cursor, &unparsedBytes);
				cgrrBlk->recomputedRoutes[j].hopList[i].toNode = temp;
				//cgrr_debugPrint("i cgrr_getCGRRFromExtensionBlock extracted recomputed route %d H[%d].toNode of=" UVAST_FIELDSPEC ".", j, i, temp);
				//cgrr_debugPrint("i cgrr_getCGRRFromExtensionBlock value in cgrrBlk=" UVAST_FIELDSPEC ".", cgrrBlk->recomputedRoutes[j].hopList[i].toNode);
				extractSdnv(&temp, &cursor, &unparsedBytes);
				cgrrBlk->recomputedRoutes[j].hopList[i].fromTime = temp;
				//cgrr_debugPrint("i cgrr_getCGRRFromExtensionBlock extracted recomputed route %d H[%d].fromTime of=" UVAST_FIELDSPEC ".", j, i, temp);
				//cgrr_debugPrint("i cgrr_getCGRRFromExtensionBlock value in cgrrBlk=" UVAST_FIELDSPEC ".", cgrrBlk->recomputedRoutes[j].hopList[i].fromTime);
		}
	}

	/* Step 2 - The block has been deserialized. Now we need to tell to the Extension Block the CGRRouteBlock's size
	 * 			and allocate enough memory in the Object scratchpad to store it. */

	//printCGRRouteBlock(cgrrBlk);

	//cgrr_debugPrint("- cgrr_getCGRRFromExtensionBlock -> %d", 1);

	MRELEASE(blkBuffer);

	return 1;
}

unsigned char	*cgrr_addSdnvToStream(unsigned char *stream, Sdnv* value)
{
	//cgrr_debugPrint("+ cgrr_addSdnvToStream(%x, %x)",
	//		(unsigned long) stream, (unsigned long) value);

	if ((stream != NULL) && (value != NULL) && (value->length > 0))
	{
		memcpy(stream, value->text, value->length);
		stream += value->length;
	}

	//cgrr_debugPrint("- cgrr_addSdnvToStream --> %x", (unsigned long) stream);

	return stream;
}

/******************************************************************************
 *
 * \par Function Name: saveRouteToExtBlock
 *
 * \par Purpose: This utility function accepts a hopCount, a vector of Hop of hopCount
 * 				 length and a Bundle to which we shall attach the given Hop list if it
 * 				 contains a CGRR extension block. What we do here is create the CGRRoute
 * 				 that will be given to the addRoute() function, which is responsable of
 * 				 actually adding the route.
 *
 * \par Date Written:  22/11/18
 *
 * \return int
 *
 * \retval  1  A CGRRouteBlock was successfully saved in the EB.
 * \retval  0  The input pointers did not pass its sanity check.
 * \retval -1  There was a system error.
 * \retval -2  The CGRREB was not found inside the Bundle.
 *
 *
 * \param[in] 		hopCount  	An integer representing the length of the hopList.
 * \param[in] 		hopList  	Pointer to a list of Hop initialized and populated
 * 								by the enqueueToNeighbor() function (libcgr.c).
* \param[in, out] 	bundle  	Pointer to the Bundle struct that represents the bundle
* 								to which we shall add the given HopList.
 *
 * \par Notes:
 *      1. This function allocates memory using the MTAKE method.  The
 *         block is freed by this function. Any system error will release the memory.
 *
 * \par Revision History:
 *
 *  MM/DD/YY | AUTHOR       | SPR#  |  DESCRIPTION
 *  -------- | ------------ | ----- | ------------------------------------------
 *  22/11/18 | L. Mazzuca   |       | Initial Implementation.
 *  07/04/19 | L. Mazzuca	|	    | Added documentation.
 *****************************************************************************/

int saveRouteToExtBlock(int hopCount, CGRRHop* hopList, Bundle* bundle)
{
	Object 		elt;
	int 		result = 1, i;
	CGRRoute	*routeToAdd;


	CHKERR(hopList != NULL); //Added by F. Marchetti
	CHKERR(bundle != NULL); //Added by F. Marchetti

	/* Step 0: Check if applicable (i.e. an Extension Block of type CGRR(23)
	 * 		   has been defined. If not return N/A (i.e. 0). */

	if ((elt = findExtensionBlock(bundle, EXTENSION_TYPE_CGRR, 0, 0, 0)) <= 0 )
	{
		cgrr_debugPrint("[cgrr_utils.c/saveRouteToExtBlock] No CGRR Extesion Block found in bundle.");
		return -2;
	}

	/* Step 2: Create routeToAdd and add it to retrieved CGRRouteBlock in bundle. */

	/* Step 2.1: Populate routeToAdd. */
	cgrr_debugPrint("[cgrr_utils.c/saveRouteToExtBlock] populating routeToAdd...");

	routeToAdd = (CGRRoute*) MTAKE(sizeof(CGRRoute));
	if(routeToAdd == NULL) //Added by F. Marchetti
	{
		cgrr_debugPrint("[cgrr_utils.c/saveRouteToExtBlock] cannot instantiate memory for routeToAdd.");
		return -1;
	}

	routeToAdd->hopList = (CGRRHop *) MTAKE(sizeof(CGRRHop) * hopCount);
	if(routeToAdd->hopList == NULL) //Added by F. Marchetti
	{
		cgrr_debugPrint("[cgrr_utils.c/saveRouteToExtBlock] cannot instantiate memory for hopList.");
		MRELEASE(routeToAdd);
		return -1;
	}

	routeToAdd->hopCount = hopCount;

	for (i = 0; i < routeToAdd->hopCount; i++)
	{
		routeToAdd->hopList[i].fromNode = hopList[i].fromNode;
		cgrr_debugPrint("[cgrr_utils.c/saveRouteToExtBlock] Added routeToAdd.hopList[%d].fromNode = " UVAST_FIELDSPEC ".",
				i, routeToAdd->hopList[i].fromNode);
		routeToAdd->hopList[i].toNode = hopList[i].toNode;
		cgrr_debugPrint("[cgrr_utils.c/saveRouteToExtBlock] Added routeToAdd.hopList[%d].toNode = " UVAST_FIELDSPEC ".",
				i, routeToAdd->hopList[i].toNode);
		routeToAdd->hopList[i].fromTime = hopList[i].fromTime;
		cgrr_debugPrint("[cgrr_utils.c/saveRouteToExtBlock] Added routeToAdd.hopList[%d].fromTime = %ld.",
				i, routeToAdd->hopList[i].fromTime - ionReferenceTime(NULL));

	}

	/* Step 2.3: If there's no originalRoute, we must be at first node. Add route as
	 * 			 orginal. Else it is recomputed, so add it to recomputed routes. */

	cgrr_debugPrint("[cgrr_utils.c/saveRouteToExtBlock] going into addRoute...");
	if ((result = addRoute(bundle, elt, routeToAdd)) < 0)
	{
		cgrr_debugPrint("[cgrr_utils.c/saveRouteToExtBlock] an error occurred adding the Route to ExtB.");
	}

	MRELEASE(routeToAdd->hopList); //Added by F. Marchetti
	MRELEASE(routeToAdd);

	return result;
}

/******************************************************************************
 *
 * \par Function Name: addRoute
 *
 * \par Purpose: This utility function accepts a bundle, an elt address representing the
 * 				 CGRREB in sdr memory and a route to add to the extension block. This function
 * 				 is called by the saveRouteToExtBlock() and its purpose is to create
 * 				 a new CGRRouteBlock from the one already contained in the EB and add to it
 * 				 the given route. Then it calls the cgrr_attach() fucntion to save the
 * 				 modified information to the sdr Bundle.
 *
 * \par Date Written:  22/11/18
 *
 * \return int
 *
 * \retval  1   A CGRRouteBlock was successfully deserialized into the given
 * 		        structure.
 * \retval  0   The input pointers did not pass its sanity check.
 * \retval -1   There was a system error.
 * \retval -2   The CGRREB was not found inside the Bundle.
 *
 *
 * \param[in, out] 	bundle  		Pointer to the Bundle struct that represents the bundle
 * 									to which we shall add the given CGRRoute.
 * \param[in] 		extBlockElt  	Elt Address where we can find the Extension Block.
 * \param[in] 		routeToAdd  	CGRRoute we are going to add to the Extension Block.
 *
 * \par Notes:
 *      1. This function allocates memory using the MTAKE method.  The
 *         block is freed by this function. Any system error will release the memory.
 *
 * \par Revision History:
 *
 *  MM/DD/YY |  AUTHOR        | SPR#  |  DESCRIPTION
 *  -------- | ------------   |------ | -----------------------------------------
 *  22/11/18 | L. Mazzuca     |       | Initial Implementation.
 *  07/04/19 | L. Mazzuca	  |	      | Added documentation.
 *****************************************************************************/
int addRoute( Bundle *bundle, Object extBlockElt, CGRRoute *routeToAdd)
{
	Sdr 			sdr = getIonsdr();
	int 			result = 1;
	unsigned int	oldLength;
	int				i, newIdx;
	Object			extBlkAddr;
	CGRRouteBlock		*oldCgrrBlk = NULL;
	CGRRouteBlock		*newCgrrBlk = NULL;

	OBJ_POINTER(ExtensionBlock, blk);

	/* Step 0 - Sanity Checks. */

	CHKERR(bundle); //Modified by F. Marchetti
	CHKERR(routeToAdd); //Modified by F. Marchetti

	//cgrr_debugPrint("addRoute: sanity checks done.");

	/* Step 1: Retrieve ExtensionBlock and then CGRRouteBlock from address returned by
	 * 			 findExtensionBlock(). */

	extBlkAddr = sdr_list_data(sdr, extBlockElt);
	CHKERR(extBlkAddr != 0); //Added by F. Marchetti

	GET_OBJ_POINTER(sdr, ExtensionBlock, blk, extBlkAddr);
	//cgrr_debugPrint("addRoute: retrieved Extension Block of values:\n %u blk address: %lu, blk length %u, blk dataLength %u, blk size %u",
	//		blk->type, blk->object, blk->length, blk->dataLength, blk->size);

	/* Step 0.2 - Initialize CGRRouteBlock to host serialized block. */
	oldCgrrBlk =  (CGRRouteBlock*) MTAKE(sizeof(CGRRouteBlock));

	if (oldCgrrBlk == NULL)
	{
			cgrr_debugPrint("[cgrr_utils.c/addRoute] cannot instantiate memory for newCGRRouteBlock.");
			return -1;
	}

	if ( (result = cgrr_getCGRRFromExtensionBlock(blk, oldCgrrBlk)) < 1)
	{
		cgrr_debugPrint("addRoute: an error occured retrieving the CGRRouteBlock from the Extension Block.");
		releaseCgrrBlkMemory(oldCgrrBlk);
		return result;
	}

	oldLength = blk->length;

	//cgrr_debugPrint("addRoute: blk data length %u.", blk->dataLength);

	//cgrr_debugPrint("addRoute: retrieved CGRRouteBlock of values: OR_HC= %u, RR_Len = %u.",
	//		oldCgrrBlk->originalRoute.hopCount, oldCgrrBlk->recRoutesLength);

	/* Step 2: If there's no originalRoute, we must be at first node. Add route as
	 * 	 	   orginal. Else it is recomputed, so add it to recomputed routes. */

	if (oldCgrrBlk->originalRoute.hopCount == 0)
	{
		/* Step 2.1: Instantiate memory for original route hoplist. */

		oldCgrrBlk->originalRoute.hopList = (CGRRHop *) MTAKE(sizeof(CGRRHop) * routeToAdd->hopCount);

		if (oldCgrrBlk->originalRoute.hopList == NULL)
		{
			cgrr_debugPrint("[cgrr_utils.c/addRoute] cannot instantiate memory for original route hopeList.");
			releaseCgrrBlkMemory(oldCgrrBlk);
			return -1;
		}

		/* Step 2.2: . */

		copyCGRRoute(&(oldCgrrBlk->originalRoute), routeToAdd);

		if ((result = cgrr_attach(blk, oldCgrrBlk)) < 0)
		{
			cgrr_debugPrint("[cgrr_utils.c/addRoute] a problem occurred attaching CGRRouteBlock to ExtensionBlock.");
		}
	}
	else
	{
		/* Step 2.1: For the recomputed routes we need to create a new CGRRouteBlock to attach
		 * 			so we initialize the necessary memory checking step by step that we
		 * 			were successful in doing so. */
		newCgrrBlk = (CGRRouteBlock*) MTAKE(sizeof(CGRRouteBlock));
		if (newCgrrBlk == NULL)
		{
				cgrr_debugPrint("[cgrr_utils.c/addRoute] cannot instantiate memory for newCGRRouteBlock.");
				return -1;
		}

		newCgrrBlk->originalRoute.hopList = (CGRRHop *) MTAKE(sizeof(CGRRHop) * oldCgrrBlk->originalRoute.hopCount);
		newCgrrBlk->recomputedRoutes = (CGRRoute*) MTAKE(sizeof(CGRRoute) * (oldCgrrBlk->recRoutesLength + 1) );

		if (newCgrrBlk->originalRoute.hopList == NULL
			|| newCgrrBlk->recomputedRoutes == NULL)
		{
			cgrr_debugPrint("[cgrr_utils.c/addRoute] cannot instantiate memory for original route's hop list or recomputed routes.");
			releaseCgrrBlkMemory(newCgrrBlk);
			return -1;
		}

		for (i = 0; i < oldCgrrBlk->recRoutesLength; i++)
		{
			newCgrrBlk->recomputedRoutes[i].hopList = (CGRRHop *) MTAKE(sizeof(CGRRHop) * oldCgrrBlk->recomputedRoutes[i].hopCount);
			if (newCgrrBlk->recomputedRoutes[i].hopList == NULL)
			{
				cgrr_debugPrint("[cgrr_utils.c/addRoute] cannot instantiate memory for %dth recomputedRoute's hop list.", i);
				releaseCgrrBlkMemory(newCgrrBlk);
				return -1;
			}
		}

		newIdx = oldCgrrBlk->recRoutesLength;

		newCgrrBlk->recomputedRoutes[newIdx].hopList = (CGRRHop *) MTAKE(sizeof(CGRRHop) * routeToAdd->hopCount);
		if (newCgrrBlk->recomputedRoutes[newIdx].hopList == NULL)
		{
			cgrr_debugPrint("[cgrr_utils.c/addRoute] cannot instantiate memory for newCGRRouteBlock's recomputedRoute to add.");
			releaseCgrrBlkMemory(newCgrrBlk);
			return -1;
		}

		/* Step 2.2: Now that we know that we have enough memory we can check if the sdr can save the new
		 * 			 CGRRouteBlock once it will be populated. */

		/* Step 2.3: Now we go ahead and populate the new Extension Block with the retrieved values
		 * 			 and then we add the latest recomputed route and increment the recRoutesLength. */

		//cgrr_debugPrint("addRoute: copying old block into new block...");
		copyCGRRouteBlock(newCgrrBlk, oldCgrrBlk);
		//cgrr_debugPrint("addRoute: done.");
		//cgrr_debugPrint("addRoute: adding new recomputed route...");
		copyCGRRoute(&(newCgrrBlk->recomputedRoutes[newIdx]), routeToAdd);
		//cgrr_debugPrint("addRoute: done.");
		newCgrrBlk->recRoutesLength++;

		/* Step 2.4: Last but not least, we save and serialize our CGRRouteBlock. */

		if ((result = cgrr_attach(blk, newCgrrBlk)) < 0)
		{
			releaseCgrrBlkMemory(oldCgrrBlk);
			releaseCgrrBlkMemory(newCgrrBlk);
			cgrr_debugPrint("[cgrr_utils.c/addRoute] a problem occurred attaching CGRRouteBlock to ExtensionBlock.");

		}

	}

	/* Step 3: Save into sdr the modified Extension Block. */

	if (result >= 0)
	{
		cgrr_debugPrint("[cgrr_utils.c/addRoute] processing extblock...");
		processModifiedExtensionBlock(bundle, extBlkAddr, blk, oldLength, blk->size);
	}

	releaseCgrrBlkMemory(oldCgrrBlk);
	releaseCgrrBlkMemory(newCgrrBlk);
	return result;
}

static void	adjustDbOverhead(Bundle *bundle, unsigned int oldLength,
			unsigned int newLength, unsigned int oldSize,
			unsigned int newSize)
{
	bundle->dbOverhead -= oldLength;
	bundle->dbOverhead += newLength;
	bundle->dbOverhead -= oldSize;
	bundle->dbOverhead += newSize;
}

int processModifiedExtensionBlock(Bundle *bundle, Object blkAddr, ExtensionBlock *blk, unsigned int oldLength, unsigned int oldSize)
{
	Sdr					bpSdr = getIonsdr();
	int					i;
	int					oldDbOverhead;
	Object				elt;
	Object				nextElt;
	unsigned int		wasSuppressed;

	CHKERR(bundle);

	oldDbOverhead = bundle->dbOverhead;
	//cgrr_debugPrint("processModifiedExtensionBlock: oldLength %u, oldSize %u, oldDbOverhead %u",
	//					oldLength, oldSize, oldDbOverhead);

	CHKERR(sdr_begin_xn(bpSdr));

	for (i = 0; i < 2; i++)
	{
		if (bundle->extensions[i] == 0)
		{
			continue;
		}

		for (elt = sdr_list_first(bpSdr, bundle->extensions[i]); elt;
				elt = nextElt)
		{
			nextElt = sdr_list_next(bpSdr, elt);

			if (sdr_list_data(bpSdr, elt) == blkAddr)
			{
				wasSuppressed = blk->suppressed;

				//cgrr_debugPrint("processModifiedExtensionBlock: %u address: %lu, length %u, datalen %u, size %u",
				//		blk->type, blk->object, blk->length, blk->dataLength, blk->size);

				if (blk->length == 0)	/*	Scratched.	*/
				{
					bundle->extensionsLength[i] -= oldLength;
					adjustDbOverhead(bundle, oldLength, 0,
							oldSize, 0);
					deleteExtensionBlock(elt,
							&bundle->extensionsLength[i]);
					continue;
				}

				/*	Revise aggregate extensions length as
				 *	necessary.				*/

				if (wasSuppressed)
				{
					if (!blk->suppressed)	/*	restore	*/
					{
						bundle->extensionsLength[i]
								+= blk->length;
					}

					/*	Still suppressed: no change.	*/
				}
				else	/*	Wasn't suppressed before.	*/
				{
					if (!blk->suppressed)
					{
						/*	Still not suppressed,
						 *	but length may have
						 *	changed.  Subtract the
						 *	old length and add the
						 *	new length.		*/

						bundle->extensionsLength[i]
								-= oldLength;
						bundle->extensionsLength[i]
								+= blk->length;
					}
					else	/*	Newly suppressed.	*/
					{
						bundle->extensionsLength[i]
								-= oldLength;
					}
				}

				if (blk->length != oldLength || blk->size != oldSize)
				{
					//cgrr_debugPrint("processModifiedExtensionBlock: adjusting overhead...");
					adjustDbOverhead(bundle, oldLength, blk->length,
							oldSize, blk->size);
					//cgrr_debugPrint("processModifiedExtensionBlock: done.");
				}

				//cgrr_debugPrint("processModifiedExtensionBlock: %u address: %lu, length %u, datalen %u, size %u",
				//						blk->type, blk->object, blk->length, blk->dataLength, blk->size);

				//cgrr_debugPrint("addRoute: writing modified Extension Block to sdr...");
				//CHKERR(sdr_begin_xn(bpSdr));
				sdr_write(bpSdr, blkAddr, (char *) blk,
										sizeof(ExtensionBlock));
				//sdr_end_xn(bpSdr);
				break;
			}
		}
	}
	sdr_end_xn(bpSdr);

	if (bundle->dbOverhead != oldDbOverhead)
	{
		//cgrr_debugPrint("processModifiedExtensionBlock: adjusting overhead in ZCO...");
		zco_reduce_heap_occupancy(bpSdr, oldDbOverhead, bundle->acct);
		zco_increase_heap_occupancy(bpSdr, bundle->dbOverhead,
				bundle->acct);
		//cgrr_debugPrint("processModifiedExtensionBlock: done.");
	}

	return 0;
}

void copyCGRRouteBlock(CGRRouteBlock *dest, CGRRouteBlock *src)
{
	int i;

	CHKVOID(dest);
	CHKVOID(src);

	copyCGRRoute(&(dest->originalRoute), &(src->originalRoute));

	dest->recRoutesLength = src->recRoutesLength;
	for (i = 0; i < src->recRoutesLength; i++)
	{
		copyCGRRoute(&(dest->recomputedRoutes[i]), &(src->recomputedRoutes[i]));
	}
}

void copyCGRRoute(CGRRoute *dest, CGRRoute *src)
{
	int j;

	CHKVOID(dest);
	CHKVOID(src);

	//cgrr_debugPrint("copyCGRRoute: src HC= %u.", src->hopCount); //Modified by F. Marchetti
	//cgrr_debugPrint("copyCGRRoute: dest HC= %u.", dest->hopCount); //Modified by F. Marchetti

	dest->hopCount = src->hopCount;
	//cgrr_debugPrint("[cgrr_utils.c/copyCGRRoute] added dest HC=%u.", dest->hopCount);//Modified by F. Marchetti
	for (j = 0; j < src->hopCount; j++)
	{
		dest->hopList[j].fromNode = src->hopList[j].fromNode;
		//cgrr_debugPrint("[cgrr_utils.c/copyCGRRoute] Added dest->hopList[%d].fromNode = " UVAST_FIELDSPEC ".",
		//					j, dest->hopList[j].fromNode); //Modified by F. Marchetti
		dest->hopList[j].toNode = src->hopList[j].toNode;
		//cgrr_debugPrint("[cgrr_utils.c/copyCGRRoute] Added dest->hopList[%d].toNode = " UVAST_FIELDSPEC ".",
		//					j, dest->hopList[j].toNode); //Modified by F. Marchetti
		dest->hopList[j].fromTime = src->hopList[j].fromTime;
		//cgrr_debugPrint("copyCGRRoute: Added dest->hopList[%d].fromTime = %ld.",
		//					j, dest->hopList[j].fromTime); //Modified by F. Marchetti
	}

	return; //Added by F. Marchetti
}

/*
unsigned int writeCGRRouteBlockToSdr(Sdr sdr, Object destAddress, CGRRouteBlock *src)
{
	int i;
	unsigned int offset = 0;
	CHKERR(src);

	offset = writeCGRRouteToSdr(sdr, destAddress, &(src->originalRoute));

	CHKERR(sdr_begin_xn(sdr));
	sdr_write(sdr, destAddress + offset, (char*)&(src->recRoutesLength), sizeof (src->recRoutesLength));
	offset += sizeof (src->recRoutesLength);
	sdr_end_xn(sdr);

	for (i = 0; i < src->recRoutesLength; i++)
	{
		offset += writeCGRRouteToSdr(sdr, destAddress + offset, &(src->recomputedRoutes[i]));
	}

	return offset;
}

unsigned int writeCGRRouteToSdr(Sdr sdr, Object destAddress, CGRRoute *src)
{
	int j;
	unsigned int offset = 0;
	CHKERR(src);
	CHKERR(sdr_begin_xn(sdr));

	sdr_write(sdr, destAddress + offset, (char)&(src->hopCount), sizeof (src->hopCount));
	offset += src->hopCount;
	for (j = 0; j < src->hopCount; j++)
	{
		sdr_write(sdr, destAddress + offset, (char*)src->hopList[j], sizeof (Hop));
		offset += sizeof (Hop);
		sdr_write(sdr, destAddress + offset, (char*)&(src->hopList[j].toNode), sizeof (src->hopList[j].toNode));
		offset += sizeof (src->hopList[j].toNode);
		sdr_write(sdr, destAddress + offset, (char*)&(src->hopList[j].fromTime), sizeof (src->hopList[j].fromTime));
		offset += sizeof (src->hopList[j].fromTime);
	}

	return offset;
}
*/

/******************************************************************************
 *
 * \par Function Name: releaseCgrrBlkMemory
 *
 * \par Purpose: Frees all the memory previously allocated in a CGRRBlock
 *
 * \par Date Written:  02/10/19
 *
 * \return void
 *
 * \param[in]  *cgrrBlk    The CGRRouteBlock to free.
 *
 * \par Revision History:
 *
 *  MM/DD/YY |  AUTHOR      |  SPR#  |   DESCRIPTION
 *  -------- | ------------ | ------ | -----------------------------------------
 *  02/10/19 | G.M. De Cola	|	     | Initial implementation and documentation.
 *****************************************************************************/


void releaseCgrrBlkMemory(CGRRouteBlock *cgrrBlk){
	if(cgrrBlk == NULL)
		return;

	int i = 0;
	for(i = 0; i < cgrrBlk->recRoutesLength; i++){
		if(cgrrBlk->recomputedRoutes[i].hopList != NULL)
			MRELEASE(cgrrBlk->recomputedRoutes[i].hopList);
	}

	if(cgrrBlk->recomputedRoutes != NULL)
		MRELEASE(cgrrBlk->recomputedRoutes);

	if(cgrrBlk->originalRoute.hopList != NULL)
		MRELEASE(cgrrBlk->originalRoute.hopList);

	MRELEASE(cgrrBlk);
}

// Written by L. Persampieri
// -1 : 0 hops in out_route
// -2 : System error
//  0 : Success case
int getCGRRoute(CgrRoute * in_route, CGRRoute * out_route) {
	PsmPartition ionwm = getIonwm();
	PsmAddress elt, addr;
	IonCXref *contact;
	int cursor = 0;

	CHKERR(ionwm);
	CHKERR(in_route);
	CHKERR(out_route);


	out_route->hopCount = sm_list_length(ionwm,in_route->hops);

	if (out_route->hopCount == 0)
	{
		return -1;
	}

	out_route->hopList = MTAKE(sizeof(CGRRHop) * out_route->hopCount);

	if (out_route->hopList == NULL)
	{
		return -2;
	}

	for (elt = sm_list_first(ionwm,in_route->hops); elt;
			elt = sm_list_next(ionwm, elt))
	{
		addr = sm_list_data(ionwm, elt);
		contact = (IonCXref*) psp(ionwm, addr);

		out_route->hopList[cursor].fromNode = contact->fromNode;
		out_route->hopList[cursor].toNode = contact->toNode;
		out_route->hopList[cursor].fromTime = contact->fromTime;

		cursor++;

	}

	return 0;

}

/******************************************************************************
 *
 * \par Function Name:
 * 		cgrr_getUsedEvc
 *
 * \brief This function, in success case, returns the previous EVC associated to the
 *        last route inserted into CGRR by local node's CGR.
 *
 * \par Date Written:
 * 		09/12/20
 *
 * \return int
 *
 * \retval  2  We cannot read the data.
 * \retval  1  The bundle is a new fragment, previous information in CGRRObject about
 *             used MTV are not more helpful. I suggests to recompute the EVC starting from
 *             fragment's payload. Size now contains payload (in bytes).
 * \retval  0  Ok, the bundle has the same information stored into CGRRObject.
 *             size now contains the EVC previously stored.
 * \retval  -1 Some error occurred.
 *
 * \par Notes:
 *      This function help you to refill contact's MTV (of the discarded route)
 *      when a bundle is reforwarded, i.e. when the forfeit time is expired.
 *
 * \par Revision History:
 *
 *  DD/MM/YY | AUTHOR          |  DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  09/12/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
int cgrr_getUsedEvc(Bundle *bundle, ExtensionBlock *cgrrExtBlk, uvast *size)
{
	Sdr sdr = getIonsdr();
	CGRRObject cgrrObj;
	int result;

	CHKERR(bundle);
	CHKERR(cgrrExtBlk);
	CHKERR(size);
	CHKERR(cgrrExtBlk->object); // if 0 nothing to do here

	sdr_read(sdr, (char *) &cgrrObj, cgrrExtBlk->object, cgrrExtBlk->size);

	if(!cgrrObj.readLock || cgrrObj.cloneLevel == 2)
	{
		result = 2;
		*size = 0;
	}
	else if(cgrrObj.cloneLevel == 1 && (!(bundle->bundleProcFlags & BDL_IS_FRAGMENT)))
	{
		/*
		 *  Bundle has been cloned, now we risk that the clone forwarding
		 *  is interpreted as original bundle reforwarding with MTV reintegration.
		 *  So, from now, we don't consider this clone anymore.
		 */
		cgrrObj.cloneLevel = 2;
		cgrrObj.evc = 0;
		result = 2;
		*size = 0;
		cgrrObj.readLock = 0;	/*	disable next read	*/
		sdr_write(sdr,cgrrExtBlk->object, (char *) &cgrrObj, sizeof(cgrrObj)); // store the lock
	}
	else
	{
		if ((bundle->bundleProcFlags & BDL_IS_FRAGMENT)
				&& (bundle->id.fragmentOffset != cgrrObj.fragmOffset
				|| bundle->payload.length != cgrrObj.fragmLength))
		{
			result = 1;
			*size = bundle->payload.length; // fragment payload
			cgrr_debugPrint("[cgrr_utils.c/cgrr_getUsedEvc] new fragment.");
		}
		else
		{
			result = 0;
			*size = cgrrObj.evc; // previously used evc
			cgrr_debugPrint("[cgrr_utils.c/cgrr_getUsedEvc] match OK.");
		}

		cgrrObj.readLock = 0;	/*	disable next read	*/
		sdr_write(sdr,cgrrExtBlk->object, (char *) &cgrrObj, sizeof(cgrrObj)); // store the lock
	}

	return result;

}

/******************************************************************************
 *
 * \par Function Name:
 * 		cgrr_setUsedEvc
 *
 * \brief This function, in success case, stores into CGRR the EVC passed as argument.
 *
 * \par Date Written:
 * 		09/12/20
 *
 * \return int
 *
 * \retval  0  Success case
 * \retval -1  Some error occurred
 *
 * \par Notes:
 *      This function help you to refill contact's MTV (of the discarded route)
 *      when a bundle is reforwarded, i.e. when the forfeit time is expired.
 *
 * \par Revision History:
 *
 *  DD/MM/YY | AUTHOR          |  DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  09/12/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
int cgrr_setUsedEvc(Bundle *bundle, ExtensionBlock *cgrrExtBlk, uvast evc)
{
	Sdr sdr = getIonsdr();
	CGRRObject cgrrObj;

	CHKERR(bundle);
	CHKERR(cgrrExtBlk);
	CHKERR(cgrrExtBlk->object); // if 0 nothing to do here

	// read to get the cloneLevel field
	sdr_read(sdr, (char *) &cgrrObj, cgrrExtBlk->object, sizeof(cgrrObj));

	if(cgrrObj.cloneLevel == 2)
	{
		return -1;
	}

	cgrrObj.fragmOffset = bundle->id.fragmentOffset;
	if(bundle->bundleProcFlags & BDL_IS_FRAGMENT)
	{
		// if bundle is a fragment
		cgrrObj.fragmLength = bundle->payload.length;
	}
	else
	{
		cgrrObj.fragmLength = 0;
	}
	cgrrObj.evc = evc;
	cgrrObj.readLock = 1;	/*	enable next read	*/

	sdr_write(sdr,cgrrExtBlk->object, (char *) &cgrrObj, sizeof(cgrrObj)); // store values

	return 0;
}
