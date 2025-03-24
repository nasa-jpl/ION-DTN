/** \file rgr.c
 *
 *  \brief  implementation of the extension definition
 *			functions for the Register Geo Route extension block.
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
#include "../rgr/rgr.h"



/******************************************************************************
 *
 * \par Function Name: rgr_attach
 *
 * \par Purpose: Compute and attach a Register block within the bundle.
 *
 * \return int
 *
 * \retval  -1  Error.
 * \retval   0  Register Attached
 *
 * \param[in,out]  bundle  The bundle to which a Route might be attached.
 * \param[out]     blk     The serialized Route extension block.
 * \param[out]     rgrBlk  The Route to be serialized.
 *
 *
 * \par Modification History:
 *
 *  MM/DD/YY | AUTHOR         | DESCRIPTION
 *  -------- | ------------   | ---------------------------------------------
 *  28/11/18 | L. Mazzuca     | Initial Implementation
 *****************************************************************************/
 int rgr_attach(Bundle *bundle, ExtensionBlock *blk, GeoRoute *rgrBlk) {

	Sdnv	lengthSdnv;
	int		result = 0;
	CHKERR(bundle);
	CHKERR(blk);
	CHKERR(rgrBlk);

	unsigned int dataBufferSize = 3 + rgrBlk->length;

	char	dataBuffer[dataBufferSize+1];

	blk->dataLength = 0;

	if (rgrBlk->length)
	{
		//rgr_debugPrint("i rgr_attach rgr length: %u\nnodes: %s", rgrBlk->length, rgrBlk->nodes);

		encodeSdnv(&lengthSdnv, rgrBlk->length);
		blk->dataLength += lengthSdnv.length;

		/*	Note that lengthSdnv.length can't exceed 2 because
		 *	rgrBlk->nodesCount is an unsigned char
		 *	and therefore has a maximum value of 255, which can
		 *	always be encoded in a 2-byte SDNV.			*/
		//rgr_debugPrint("i rgr_attach encoded length in SDNV. New blk->dataLength: %u", blk->dataLength);

		memcpy(dataBuffer, lengthSdnv.text, lengthSdnv.length);
		//rgr_debugPrint("i rgr_attach memcopied length in databuffer.");

		blk->dataLength += rgrBlk->length;
		//rgr_debugPrint("i rgr_attach added GeoRoute length to blk->dataLength. "
		//		"New blk->dataLength: %u", blk->dataLength);

		//rgr_debugPrint("i rgr_attach databuffer size: %u, strlen: %u",
		//			sizeof(dataBuffer), strlen(dataBuffer));
		memcpy(dataBuffer + lengthSdnv.length, (char *) rgrBlk->nodes, rgrBlk->length);
		//rgr_debugPrint("i rgr_attach added GeoRoute nodes to dataBuffer.");
	}

	//rgr_debugPrint("i rgr_attach databuffer size: %u\n"
	//			"blk->size: %u\nblk->dataLength: %u\n", sizeof(dataBuffer), blk->size, blk->dataLength);
	dataBuffer[dataBufferSize] = '\0';
	result = serializeExtBlk(blk, NULL, dataBuffer);
	MRELEASE(dataBuffer);

	return result;
}

 /******************************************************************************
  *
  * \par Function Name: rgr_offer
  *
  * \par Purpose: This callback aims to ensure that the bundle contains
  * 		         a RGR extension block keep track of the source
  * 		         routing. If the bundle already contains such a RR
  * 		         block (inserted by an upstream node) then the function
  * 		         simply returns 0. Otherwise the function creates an empty
  * 		         RR block to be populated during the ION execution.
  *
  * \return int
  *
  * \retval  0  The RGR was successfully created, or not needed.
  * \retval -1  There was a system error.
  *
  * \param[in,out]  blk    The block that might be added to the bundle.
  * \param[in]      bundle The bundle that would hold this new block.
  *
  * \par Notes:
  *      1. Code was written with inspiration to the bpsec extension blocks
  *         developed by E. Birrane.
  *      2. All block memory is allocated using sdr_malloc.
  *
  * \par Modification History:
  *
  *  MM/DD/YY | AUTHOR       |  DESCRIPTION
  *  -------- | ------------ |  ---------------------------------------------
  *  20/11/18 | L. Mazzuca   |  Initial Implementation and documentation
  *****************************************************************************/

int	rgr_offer(ExtensionBlock *blk, Bundle *bundle)
{
	GeoRoute		rgrBlk;
	int				result = 0;
	/* Step 1 - Sanity Checks. */

	/* Step 1.1 - Make sure we have parameters. s*/
	CHKERR(blk);
	CHKERR(bundle);

	if(findExtensionBlock(bundle, EXTENSION_TYPE_RGR, 0, 0, 0))
	{
		/*	Don't create a Route because it already exist.	*/
		//rgr_debugPrint("x rgr_offer - RGR already exists.");
		blk->size = 0;
		blk->object = 0;
		result = 0;
		//rgr_debugPrint("- rr_offer -> %d", result);
		return result;
	}

	/* Step 1.2 - Initialize ExtensionBlock param to default values. */

	blk->blkProcFlags = BLK_MUST_BE_COPIED;
	blk->bytes = 0;
	blk->length = 0;
	blk->object = 0;
	/* Step 2 - Initialize rgr structures. */
	rgr_debugPrint("[rgr.c/rgr_offer] Initializing rgr structures...");
	/* Step 2.1 - Populate the rgr block. */
	rgrBlk.length = 0;
	rgrBlk.nodes = NULL;

	//rgr_debugPrint("rgr_offer: initialized recomputed routes Check NL: %u", rgrBlk.length);

	rgr_debugPrint("[rgr.c/rgr_offer] Attaching block...");
	blk->size = 1; //to keep alive
	if((result = rgr_attach(bundle, blk, &rgrBlk)) < 0)
	{
		rgr_debugPrint("[rgr.c/rgr_offer] A problem occurred in attaching Route.");
	}

	//rgr_debugPrint("- rgr_offer -> %d", result);
	return result;

}

void rgr_release(ExtensionBlock *blk)
{
	Sdr	sdr;

	//rgr_debugPrint("rgr_release: Releasing Register Route sdr memory...(%lu)(%u)", blk->object, blk->size);

	CHKVOID(blk);
	if (blk->object)
	{
		sdr = getIonsdr();
		sdr_free(sdr, blk->object);
		blk->object = 0;
	}
	return;
}

int	rgr_copy(ExtensionBlock *newBlk, ExtensionBlock *oldBlk)
{
	return 0;
}

/******************************************************************************
 *
 * \par Function Name: rgr_processOnDequeue
 *
 * \par Purpose: This callback adds the information about the node transmitting
 * 				 and the timestamp at which the bundle is being trasmitted at.
 *
 * \return int
 *
 * \retval  0  The RGR was successfully populated with information.
 * \retval -1  There was a system error.
 *
 * \param[in,out]  blk    The block that's contained in the bundle.
 * \param[in]      bundle The bundle that would hold this block.
 * \param[in]      ctxt	  The context, passed byt the caller of this function.
 *
 * \par Notes:
 *      1. All block memory is allocated using MTAKE and release at the end.
 *
 * \par Modification History:
 *
 *  MM/DD/YY | AUTHOR         |  DESCRIPTION
 *  -------- | ------------   |  ---------------------------------------------
 *  01/12/18 | L. Mazzuca     |  Initial Implementation
 *  24/10/19 | G.M. De Cola	  | Changed the old processOnTransmit to processOnDequeue
 *  		 |				  | This was necessary due to an apparent ION bug for
 *  		 |				  | which the processOnTransmit doesn't actually attach
 *  		 |				  | the extension to the outgoing bundle
 *****************************************************************************/
int rgr_processOnDequeue(ExtensionBlock *blk, Bundle *bundle, void *ctxt){
		Sdr 			sdr = getIonsdr();
		GeoRoute		rgrBlk;
		char			*eidToAdd = NULL;
		unsigned char	*cursor = NULL;
		unsigned char	*blkBuffer = NULL;
		int				unparsedBytes = 0;
		unsigned int 	temp = 0;
		int				result = 0;

		rgrBlk.length = 0;
		rgrBlk.nodes = NULL;

		/*
		 * Step 1 - Parse current EID.
		 * */
		eidToAdd = parseAdminEID(sdr, bundle);


		/*
		 * Step 2 - Extract previously added routes, if there are any.
		 * */
		if (blk->dataLength > 0)
		{
			blkBuffer = MTAKE(blk->length);
			if (blkBuffer == NULL)
			{
				rgr_debugPrint("[rgr.c/rgr_processOnDequeue] No space for block buffer %u.", blk->length);
				return -1;
			}

			CHKERR(sdr_begin_xn(sdr));
			sdr_read(sdr, (char *) blkBuffer, blk->bytes, blk->length);
			sdr_end_xn(sdr);

			unparsedBytes = blk->dataLength;
			cursor = (blkBuffer) + (blk->length - blk->dataLength);
			//rgr_debugPrint("rgr_processOnDequeue: Unpased bytes: %d, data length: %u", unparsedBytes, blk->dataLength);

			extractSmallSdnv(&temp, &cursor, &unparsedBytes);
			//rgr_debugPrint("rgr_processOnDequeue: Retrieved GeoRoute block length of %u", temp);
			rgrBlk.length = temp;
			//rgr_debugPrint("rgr_processOnDequeue: GeoRoute block length = %u", rgrBlk.length);
		}

		/*
		 * Step 3 - Initialize with correct values rgrBlk.
		 * */
		rgrBlk.nodes = (char*) MTAKE(rgrBlk.length + sizeof(eidToAdd) + TIMESTAMPBUFSZ + 1);

		if (rgrBlk.nodes == NULL)
		{
				rgr_debugPrint("[rgr.c/rgr_processOnDequeue] Cannot instantiate memory for oldRgrBlk.nodes.");
				return -1;
		}

		if (rgrBlk.length > 0)
		{
			memcpy(rgrBlk.nodes, cursor, rgrBlk.length);
			//rgr_debugPrint("rgr_processOnDequeue: Extracted %s", rgrBlk.nodes);
		}
		else
		{
			/* Modified by G.M. De Cola
			 * In this case rgrBlk.length must be 0 in order to override "temporaryString" otherwise "temporaryString" is stored in the
			 * extension and carried out on the other nodes as part of the extension string
			 * In my tests there was a situation like this: temporaryStringipn:1.0,timestamp; etc...
			 */
			sprintf(rgrBlk.nodes, "temporaryString");
			rgrBlk.length = 0;
		}

		sprintf(rgrBlk.nodes + rgrBlk.length, "%s,%ld;", eidToAdd, getCtime() - ionReferenceTime(NULL));
		rgr_debugPrint("[rgr.c/rgr_processOnDequeue] Updated to %s", rgrBlk.nodes);

		rgrBlk.length = strlen(rgrBlk.nodes);
		//rgr_debugPrint("rgr_processOnDequeue: GeoRoute block length updated to: %u", rgrBlk.length);

		CHKERR(sdr_begin_xn(sdr));

		blk->size = sizeof(rgrBlk.length) + rgrBlk.length;
		if ( (blk->object = sdr_malloc(sdr, blk->size)) <= 0)
		{
			rgr_debugPrint("[rgr.c/rgr_processOnDequeue] Not enough space in sdr to save new block of size %u.", rgrBlk.length);
			blk->tag1 = 1;
			blk->size = 0;
			sdr_end_xn(sdr);
			result = -1;
			return result;
		}

		sdr_write(sdr, blk->object, (char *) &rgrBlk, blk->size);
		sdr_end_xn(sdr);

		if((result = rgr_attach(bundle, blk, &rgrBlk)) < 0)
		{
			rgr_debugPrint("[rgr.c/rgr_processOnDequeue] A problem occurred in attaching Route.");
			CHKERR(sdr_begin_xn(sdr));
			sdr_free(sdr, blk->object);
			sdr_end_xn(sdr);

			blk->object = 0;
			blk->size = 0;
		}

		MRELEASE(blkBuffer);
		MRELEASE(rgrBlk.nodes);

		return result;

}

int	rgr_acquire(AcqExtBlock *blk, AcqWorkArea *wk)
{
	GeoRoute		rgrBlk;
	int				result = 1;
	unsigned char 	*cursor;
	int				unparsedBytes = blk->length;

	//rgr_debugPrint("+ rgr_acquire(0x%x, 0x%x)", (unsigned long) blk,
	//		(unsigned long) wk);
	//rgr_debugPrint("i rgr_acquire: bytes %u length %u dataLength %u", blk->bytes, blk->length, blk->dataLength);

	CHKERR(blk);
	CHKERR(wk);

	cursor = ((unsigned char *)(blk->bytes))
				+ (blk->length - blk->dataLength);

	extractSmallSdnv(&(rgrBlk.length), &cursor, &unparsedBytes);
	rgrBlk.nodes = (char*) MTAKE(rgrBlk.length + 1);

	if (rgrBlk.nodes == NULL)
	{
		//rgr_debugPrint("rgr_acquire: cannot instantiate memory for rgrBlk.nodes.");
		return -1;
	}
	memcpy(rgrBlk.nodes, cursor, rgrBlk.length);

	rgr_debugPrint("[rgr.c/rgr_acquire] RGR length: %d\n nodes: %s", rgrBlk.length, rgrBlk.nodes);

/*	blk->size = sizeof(rgrBlk.length) + rgrBlk.length + 1;
	rgr_debugPrint("i rgr_acquire size of block set to %u", blk->size);
	blk->object = MTAKE(blk->size);
	if (blk->object == NULL)
	{
		rgr_debugPrint("x rgr_acquire: No space for GeoRoute scratchpad", NULL);
		return -1;
	}

	memcpy((char *) (blk->object), (char *) &rgrBlk, blk->size);

	result = 1;
	rgr_debugPrint("- rgr_acquire -> %d", result);*/
	MRELEASE(rgrBlk.nodes);
	rgrBlk.nodes = NULL;

	return result;
}

int rgr_parse(AcqExtBlock *blk, AcqWorkArea *wk)
{
	return 1;
}

int	rgr_check(AcqExtBlock *blk, AcqWorkArea *wk)
{
	return 1;
}

int	rgr_record(ExtensionBlock *sdrBlk, AcqExtBlock *ramBlk)
{
	return 0;
}

void rgr_clear(AcqExtBlock *blk)
{
	//rgr_debugPrint("+ rgr_clear(%x)", (unsigned long) blk);

	CHKVOID(blk);
	if (blk->object)
	{
		MRELEASE(blk->object);
		blk->object = NULL;
		blk->size = 0;
	}

	//rgr_debugPrint("- rgr_clear", NULL);

	return;
}

