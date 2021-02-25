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
 *  11/04/20 | L. Persampieri | Adapted to use in BPv7 due CBOR encoding
 *****************************************************************************/

int rgr_attach(ExtensionBlock *blk, GeoRoute *rgrBlk)
{
	int result = 0;
	unsigned char *dataBuffer;

	CHKERR(blk);
	CHKERR(rgrBlk);

	blk->dataLength = encode_rgr(rgrBlk, &dataBuffer);

//	rgr_debugPrint("[rgr.c/rgr_attach] Attaching %u", blk->dataLength);

	if(blk->dataLength < 1)
	{
		writeMemo("[?] [rgr_attach] Error...");
		return -1;
	}

	result = serializeExtBlk(blk, (char *) dataBuffer);
	MRELEASE(dataBuffer);

//	rgr_debugPrint("[rgr.c/rgr_attach] result: %d", result);

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

int rgr_offer(ExtensionBlock *blk, Bundle *bundle)
{
	int result = 0;
	GeoRoute rgrBlk;
	/* Step 1 - Sanity Checks. */

	/* Step 1.1 - Make sure we have parameters. s*/
	CHKERR(blk);
	CHKERR(bundle);

	if (findExtensionBlock(bundle, RGRBlk, 0))
	{
		rgr_debugPrint("[rgr.c/rgr_offer] Offer already exists.");
		/*	Don't create a Route because it already exist.	*/
		//rgr_debugPrint("x rgr_offer - RGR already exists.");
		blk->size = 0;
		blk->object = 0;
		result = 0;
		//rgr_debugPrint("- rr_offer -> %d", result);
		return result;
	}

	/* Step 1.2 - Initialize ExtensionBlock param to default values. */
	// TODO BLK_REPORT_IF_NG generate a bug in status report, so I deleted it.
	blk->blkProcFlags = BLK_MUST_BE_COPIED;
	blk->bytes = 0;
	blk->length = 0;
	/* Step 2 - Initialize rgr structures. */
//	rgr_debugPrint("[rgr.c/rgr_offer] Initializing rgr structures...");
	/* Step 2.1 - Populate the rgr block. */
	rgrBlk.length = 0;
	rgrBlk.nodes = NULL;

	//rgr_debugPrint("rgr_offer: initialized recomputed routes Check NL: %u", rgrBlk.length);

	rgr_debugPrint("[rgr.c/rgr_offer] Attaching block...");
	blk->size = 0;

	if ((result = rgr_attach(blk, &rgrBlk)) < 0)
	{
		rgr_debugPrint("[rgr.c/rgr_offer] A problem occurred in attaching Route.");
	}

//	rgr_debugPrint("[rgr.c/rgr_offer] result: %d", result);

	//rgr_debugPrint("- rgr_offer -> %d", result);
	return result;

}

void rgr_release(ExtensionBlock *blk)
{
	return;
}

int rgr_copy(ExtensionBlock *newBlk, ExtensionBlock *oldBlk)
{
	CHKERR(newBlk);
	CHKERR(oldBlk);
	rgr_debugPrint("(Old,New) -> length(%u,%u), dataLength(%u,%u)", oldBlk->length, newBlk->length, oldBlk->dataLength, newBlk->dataLength);
	return 0;
}

int	rgr_processOnFwd(ExtensionBlock *blk, Bundle *bundle, void *ctxt)
{
	return 0;
}

int	rgr_processOnAccept(ExtensionBlock *blk, Bundle *bundle, void *ctxt)
{
	return 0;
}

int	rgr_processOnEnqueue(ExtensionBlock *blk, Bundle *bundle, void *ctxt)
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
 *  11/04/20 | L. Persampieri | Adapted to use in BPv7 due CBOR encoding
 *****************************************************************************/

int rgr_processOnDequeue(ExtensionBlock *blk, Bundle *bundle, void *ctxt)
{
	Sdr sdr = getIonsdr();
	GeoRoute rgrBlk;
	char *eidToAdd = NULL;
	unsigned char *cursor = NULL;
	unsigned char * blkBuffer = NULL;
	size_t length;
	uvast value;
	int result = 0, result_decode;
	MetaEid metaEid;
	VScheme *vscheme = NULL;
	PsmAddress vschemeElt = 0;
	char proxNodeEid[SDRSTRING_BUFSZ];

	CHKERR(blk);
	CHKERR(bundle);

	rgrBlk.length = 0;
	rgrBlk.nodes = NULL;

	/*
	 * Step 1 - Parse current EID.
	 * */
	proxNodeEid[0] = '\0';
	if (bundle->proxNodeEid)
	{
		sdr_string_read(sdr, proxNodeEid, bundle->proxNodeEid);
	}
	if (parseEidString(proxNodeEid, &metaEid, &vscheme, &vschemeElt) == 0)
	{
		/*	Can't know which admin EID to use.		*/
		writeMemo("[?] [rgr.c/rgr_processOnDequeue] Admin EID not parsed");
		return -1;
	}

	if(vscheme == NULL)
	{
		writeMemo("[?] [rgr.c/rgr_processOnDequeue] vscheme NULL");
		return -1;
	}

	restoreEidString(&metaEid);

	eidToAdd = vscheme->adminEid;

	rgr_debugPrint("[rgr.c/rgr_processOnDequeue] eidToAdd: %s, dataLength: %u.", eidToAdd, blk->dataLength);

	length = strlen(eidToAdd);

	value = blk->dataLength + length + 23; //23 is the space for ,time;\0 where time is an integer number and shouldn't be greater than 20 digits

	rgrBlk.nodes = MTAKE(value);

	if (rgrBlk.nodes == NULL)
	{
		rgr_debugPrint("[rgr.c/rgr_processOnDequeue] Cannot instantiate memory for rgrBlk.nodes.");
		return -1;
	}

	memset(rgrBlk.nodes, 0, value);

	/*
	 * Step 2 - Extract previously added routes, if there are any.
	 * */

	if (blk->dataLength > 1)
	{
	//	writeMemo("[i] [rgr.c/rgr_processOnDequeue] Decoding previous block...");
		blkBuffer = MTAKE(blk->length);
		if(blkBuffer == NULL)
		{
			rgr_debugPrint("[rgr.c/rgr_processOnDequeue] Cannot instantiate memory for blkBuffer.");
			return -1;
		}
		sdr_read(sdr, (char*) blkBuffer, blk->bytes, blk->length);

		cursor = (blkBuffer) + (blk->length - blk->dataLength);

		result_decode = decode_rgr(blk->dataLength, cursor, &rgrBlk, 0);

		value = rgrBlk.length;

		if(result_decode < 0)
		{
			writeMemo("[?] [rgr_processOnDequeue] Decoding error...");
			MRELEASE(blkBuffer);
			return -1;
		}

		MRELEASE(blkBuffer);
	}
	else
	{
//		writeMemo("[i] [rgr.c/rgr_processOnDequeue] Attach new block...");
		value = 0;
	}

	/*
	 * Step 3 - Initialize with correct values rgrBlk.
	 * */

//	rgr_debugPrint("[rgr.c/rgr_processOnDequeue] Current string: %s", rgrBlk.nodes);
	sprintf(rgrBlk.nodes + value, "%s,%ld;", eidToAdd, getCtime() - ionReferenceTime(NULL));
	rgr_debugPrint("[rgr.c/rgr_processOnDequeue] Attach %s", rgrBlk.nodes);

	rgrBlk.length = strlen(rgrBlk.nodes);
//	rgr_debugPrint("[rgr.c/rgr_processOnDequeue] New length: %u", rgrBlk.length);

	if ((result = rgr_attach(blk, &rgrBlk)) < 0)
	{
		rgr_debugPrint("[rgr.c/rgr_processOnDequeue] A problem occurred in attaching Route.");
		blk->object = 0;
		blk->size = 0;
	}

	MRELEASE(rgrBlk.nodes);

//	rgr_debugPrint("[rgr.c/rgr_processOnDequeue] Final result: %d", result);

	return result;

}

int rgr_acquire(AcqExtBlock *blk, AcqWorkArea *wk)
{
	GeoRoute rgrBlk;
	unsigned char *cursor;

	CHKERR(blk);
	CHKERR(wk);

//	rgr_debugPrint("i rgr_acquire: length %u dataLength %u", blk->length, blk->dataLength);

	cursor = (blk->bytes) + (blk->length - blk->dataLength);

	if(decode_rgr(blk->dataLength, cursor, &rgrBlk, 1) < 0)
	{
		writeMemo("[?] [rgr_acquire] Decoding error...");
		return -1;
	}

	rgr_debugPrint("[rgr.c/rgr_acquire] RGR length: %u\n nodes: %s", rgrBlk.length, rgrBlk.nodes);

	MRELEASE(rgrBlk.nodes);

	return 1;
}

int rgr_parse(AcqExtBlock *blk, AcqWorkArea *wk)
{
	return 1;
}

int rgr_check(AcqExtBlock *blk, AcqWorkArea *wk)
{
	return 1;
}

int rgr_record(ExtensionBlock *sdrBlk, AcqExtBlock *ramBlk)
{
	return 0;
}

void rgr_clear(AcqExtBlock *blk)
{
	return;
}

