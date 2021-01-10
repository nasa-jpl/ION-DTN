/** \file cgrr.c
 *
 *	\brief  implementation of the extension definition
 *			functions for the CGR Route extension block.
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

/******************************************************************************
 *
 * \par Function Name: cgrr_attach
 *
 * \par Purpose: Compute and attach a CGRR block within the bundle.
 *
 * \return int
 *
 * \retval  -1     Error.
 * \retval  ">= 0" CGRRouteBlock Attached
 *
 * \param[out]     blk     The serialized CGRR extension block.
 * \param[out]     cgrrBlk The CGRRouteBlock to be serialized.
 *
 *
 * \par Notes:
 *	    1. The ExtesionBlock blk MUST be pre-allocated and of the correct
 *	       size to hold the CGRRouteBlock.
 *	    2. The passed-in CGRRouteBlock MUST be pre-initialized with at least
 *	       default values.
 *	    3. The CGRRouteBlock MUST be pre-allocated and initialized with a size,
 *	       and the object within the block MUST be allocated in the SDR.
 *
 * \par Modification History:
 *
 *  MM/DD/YY | AUTHOR        | DESCRIPTION
 *  -------- | ------------  | ---------------------------------------------
 *  28/10/18 | L. Mazzuca    | Initial Implementation
 *****************************************************************************/

int cgrr_attach(ExtensionBlock *blk, CGRRouteBlock *cgrrBlk) {

	int			result = 0;
	unsigned char	*serializedCgrr;
	uvast length;

	CHKERR(blk);
	CHKERR(cgrrBlk);

	/* Step 1 - Serialize the cgrr block into the cgrr Extension Block . */

	//cgrr_debugPrint("cgrr_attach: dataLength = %u", blk->dataLength);
	//cgrr_debugPrint("cgrr_attach: serializing CGRR block with custom function...");
		/* 1.1 - Create a serialized version of the cgrr block. */
		if((serializedCgrr = cgrr_serializeCGRR(&length, cgrrBlk)) == NULL)
		{
			blk->dataLength = length;
			cgrr_debugPrint("[x: cgrr.c/cgrr_attach] Unable to serialize CGRRouteBlock.  blk->dataLength = %d",
						  blk->dataLength);
			result = -1;
			//bundle->corrupt = 1;

			scratchExtensionBlock(blk);
			cgrr_debugPrint("[cgrr.c/cgrr_attach] result --> %d", result);
			return result;
		}

		blk->dataLength = length;
		//cgrr_debugPrint("[cgrr.c/cgrr_attach] done.");
		cgrr_debugPrint("[cgrr.c/cgrr_attach] dataLength = %u", blk->dataLength);
		//cgrr_debugPrint("[cgrr.c/cgrr_attach] serializing block with bei function...");
		/* Step 1.2 - Copy the serialized cgrr into the cgrr Extension Block. */

	//(result = serializeExtBlk(blk, NULL, (char *) serializedCgrr)) < 0 TODO
	if ((result = serializeExtBlk(blk, (char*) serializedCgrr)) < 0)
		{
			cgrr_debugPrint("[cgrr.c/cgrr_attach] a problem occurred serializing block...");
		}

		MRELEASE(serializedCgrr);

		//cgrr_debugPrint("- cgrr_attach --> %d", result);
		return result;
}

/******************************************************************************
 *
 * \par Function Name: cgrr_offer
 *
 * \par Purpose: This callback aims to ensure that the bundle contains
 * 		         a CGRR extension block to keep track of the source
 * 		         routing. If the bundle already contains such a CGRR
 * 		         block (inserted by an upstream node) then the function
 * 		         simply returns 0. Otherwise the function creates an empty
 * 		         CGRR block to be populated during the CGR execution.
 *
 * \return int
 *
 * \retval     0  The CGRR was successfully created, or not needed.
 * \retval    -1  There was a system error.
 *
 * \param[in,out]  blk    The block that might be added to the bundle.
 * \param[in]      bundle The bundle that would hold this new block.
 *
 * \par Notes:
 *      1. This code is inspired by the bpsec extension blocks
 *         designed by E. Birrane.
 *      2. All block memory is allocated using sdr_malloc.
 *
 * \par Modification History:
 *
 *  MM/DD/YY | AUTHOR       |  DESCRIPTION
 *  -------- | ------------ |  ---------------------------------------------
 *  20/10/18 | L. Mazzuca   |  Initial Implementation and documentation
 *****************************************************************************/

int	cgrr_offer(ExtensionBlock *blk, Bundle *bundle)
{
	Sdr	sdr = getIonsdr();
	CGRRouteBlock		cgrrBlk;
	int			result = 0;
	CGRRObject cgrrObj;
	/* Step 1 - Sanity Checks. */
	/* Step 1.1 - Make sure we have parameters...*/
	CHKERR(blk);
	CHKERR(bundle);

	// initialize cgrrObj
	memset(&cgrrObj, 0 ,sizeof(cgrrObj));


	//findExtensionBlock(bundle, EXTENSION_TYPE_CGRR, 0, 0, 0) TODO
	if (findExtensionBlock(bundle, CGRRBlk, 0))
	{
		/*	Don't create a CGRRouteBlock because it already exist.	*/
		cgrr_debugPrint("[x: cgrr.c/cgrr_offer] CGRR already exists.");

		blk->size = sizeof(CGRRObject);
		if(blk->object == 0)
		{
			blk->object = sdr_malloc(sdr, sizeof(CGRRObject));

			if(blk->object == 0)
			{
				return -1;
			}

			// initialize cgrrObj in SDR
			sdr_write(sdr, blk->object, (char *) &cgrrObj, blk->size);
		}

		result = 0;
		cgrr_debugPrint("[cgrr.c/cgrr_offer] result -> %d", result);
		return result;
	}


	/* Step 1.2 - Initialize ExtensionBlock param to default values. */

	//blk->blkProcFlags = BLK_MUST_BE_COPIED | BLK_FORWARDED_OPAQUE | BLK_REPORT_IF_NG;
	// BLK_FORWARDED_OPAQUE not more used in BPv7
	// TODO BLK_REPORT_IF_NG generate a bug in status report, so I deleted it.
	blk->blkProcFlags = BLK_MUST_BE_COPIED;
	blk->bytes = 0;
	blk->length = 0;
	blk->dataLength = 0;

	blk->size = sizeof(CGRRObject);
	blk->object = sdr_malloc(sdr, sizeof(CGRRObject));

	if(blk->object == 0)
	{
		return -1;
	}

	// initialize cgrrObj in SDR
	sdr_write(sdr, blk->object, (char *) &cgrrObj, blk->size);

	/**********Modified by F. Marchetti**************/
	/* Step 2 - Initialize cgrr structures. */
	cgrr_debugPrint("[cgrr.c/cgrr_offer] initializing cgrr structures...");
	memset(&cgrrBlk, 0, sizeof(CGRRouteBlock));
	/************************************************/

	/* Step 3 - Write the cgrr block into the Extension Block. */
	cgrr_debugPrint("[cgrr.c/cgrr_offer] attaching block...");
	if((result = cgrr_attach(blk, &cgrrBlk)) < 0)
	{
		cgrr_debugPrint("[cgrr.c/cgrr_offer] A problem occurred in attaching CGRRouteBlock.");
	}


	//cgrr_debugPrint("- cgrr_Offer -> %d", result);
	return result;

}

void cgrr_release(ExtensionBlock *blk)
{
	Sdr			sdr = getIonsdr();

//	cgrr_debugPrint("[cgrr.c/cgrr_release] Releasing CGRRouteBlock sdr memory...(%lu)(%u)", blk->object, blk->size);

	CHKVOID(blk);

	if (blk->object)
	{
		sdr_free(sdr, blk->object);
		blk->object = 0;
		blk->size = 0;
	}

	return;
}

int	cgrr_copy(ExtensionBlock *newBlk, ExtensionBlock *oldBlk)
{
	Sdr	sdr = getIonsdr();
	CGRRObject cgrrObj;

	newBlk->size = oldBlk->size;
	if (oldBlk->object == 0)
	{
		return 0;
	}

	if(newBlk->size == 0)
	{
		return 0;
	}

	newBlk->object = sdr_malloc(sdr, newBlk->size);
	if (newBlk->object == 0)
	{
		putErrmsg("Not enough heap space for CGRR block.", NULL);
		return -1;
	}

	sdr_read(sdr, (char *) &cgrrObj, oldBlk->object, oldBlk->size);

	if(!cgrrObj.cloneLevel)
	{
		cgrrObj.cloneLevel = 1;
	}

	sdr_write(sdr, newBlk->object, (char *) &cgrrObj, newBlk->size);

	return 0;
}

int	cgrr_acquire(AcqExtBlock *blk, AcqWorkArea *wk)
{
	int	result = 1;

	//cgrr_debugPrint("[+ : cgrr.c/cgrr_acquire] (0x%x, 0x%x)", (unsigned long) blk,
	//		(unsigned long) wk);

	CHKERR(blk);
	CHKERR(wk);

	result = cgrr_deserializeCGRR(blk, wk);
	//cgrr_debugPrint("i cgrr_acquire: Deserialize result %d", result);
	cgrr_debugPrint("i cgrr_acquire: blk->size %u", blk->size);
	//cgrr_debugPrint("- cgrr_acquire -> %d", result);

	return result;
}

int	cgrr_record(ExtensionBlock *sdrBlk, AcqExtBlock *ramBlk)
{
	Sdr sdr;
	CGRRObject cgrrObj;

	if(sdrBlk->object == 0)
	{
		sdr = getIonsdr();
		sdrBlk->object = sdr_malloc(sdr, sizeof(CGRRObject));

		if(sdrBlk->object == 0)
		{
			return -1;
		}

		sdrBlk->size = sizeof(CGRRObject);

		memset(&cgrrObj, 0, sizeof(cgrrObj));

		// initialize cgrrObj in SDR
		sdr_write(sdr, sdrBlk->object, (char *) &cgrrObj, sdrBlk->size);
	}

	return 0;
}

void cgrr_clear(AcqExtBlock *blk)
{
		CHKVOID(blk);
		if (blk->object)
		{
			MRELEASE(blk->object);
			blk->object = NULL;
			blk->size = 0;
		}

		return;
}
