/*
 *	imc.c:		implementation of the extension definition
 *			functions for the IPN Multicast block.
 *
 *	Copyright (c) 2020, California Institute of Technology.
 *	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
 *	acknowledged.
 *
 *	Author: Scott Burleigh, JPL
 */

#include "bpP.h"
#include "bei.h"
#include "imc.h"

int	imc_offer(ExtensionBlock *blk, Bundle *bundle)
{
	/*	Block must be offered as a placeholder to enable
	 *	later extension block processing.			*/

	blk->dataLength = 0;	/*	Will know length at dequeue.	*/
	blk->length = 0;
	blk->size = 1;		/*	Just to keep block alive.	*/
	blk->object = 0;
	return 0;
}

int	imc_serialize(ExtensionBlock *blk, Bundle *bundle)
{
	Sdr		sdr = getIonsdr();
	int		nbrOfDestinationNodes;
	int		bufferLength;
	unsigned char	*dataBuffer;
	unsigned char	*cursor;
	uvast		uvtemp;
	Object		elt;
	int		result;

	if (bundle->destinations == 0)
	{
		return 0;	/*	IMC block is unnecessary.	*/
	}

	nbrOfDestinationNodes = sdr_list_length(sdr, bundle->destinations);
	if (nbrOfDestinationNodes == 0)
	{
		return 0;	/*	IMC block is unnecessary.	*/
	}

	/*	Insert the new list of destinations into the block.	*/

	bufferLength = 9 * (nbrOfDestinationNodes + 1);
	dataBuffer = MTAKE(bufferLength);
	if (dataBuffer == NULL)
	{
		putErrmsg("No space for constructing IMC block.", NULL);
		return -1;
	}

	cursor = dataBuffer;
	uvtemp = nbrOfDestinationNodes;
	oK(cbor_encode_array_open(uvtemp, &cursor));
	for (elt = sdr_list_first(sdr, bundle->destinations); elt;
			elt = sdr_list_next(sdr, elt))
	{
		uvtemp = (uvast) sdr_list_data(sdr, elt);
		oK(cbor_encode_integer(uvtemp, &cursor));
	}

	blk->dataLength = cursor - dataBuffer;
	result = serializeExtBlk(blk, (char *) dataBuffer);
	MRELEASE(dataBuffer);
	return result;
}

void	imc_release(ExtensionBlock *blk)
{
	Sdr	sdr = getIonsdr();

	if (blk->object)
	{
		sdr_free(sdr, blk->object);
	}

	return;
}

int	imc_record(ExtensionBlock *sdrBlk, AcqExtBlock *ramBlk)
{
	Sdr	sdr = getIonsdr();

//printf("...in imc_record, ramBlk->size is %u...\n", ramBlk->size);
	if (ramBlk->size == 0)
	{
		return 0;
	}

	sdrBlk->size = ramBlk->size;
	sdrBlk->object = sdr_malloc(sdr, sdrBlk->size);
	if (sdrBlk->object == 0)
	{
		putErrmsg("Not enough heap space for IMC block.", NULL);
		return -1;
	}

	sdr_write(sdr, sdrBlk->object, (char *) (ramBlk->object), sdrBlk->size);
	return 0;
}

int	imc_copy(ExtensionBlock *newBlk, ExtensionBlock *oldBlk)
{
	Sdr	sdr = getIonsdr();
	char	*buffer;

//puts("...in imc_copy...");
	newBlk->size = oldBlk->size;	/*	Possible bogus size.	*/
	if (oldBlk->object == 0)
	{
//puts("...oldBlk->object is 0...");
		return 0;
	}

	buffer = MTAKE(newBlk->size);
	if (buffer == NULL)
	{
		putErrmsg("Not enough memory for IMC block buffer.", NULL);
		return -1;
	}

	newBlk->object = sdr_malloc(sdr, newBlk->size);
	if (newBlk->object == 0)
	{
		MRELEASE(buffer);
		putErrmsg("Not enough heap space for IMC block.", NULL);
		return -1;
	}

	sdr_read(sdr, buffer, oldBlk->object, oldBlk->size);
//printf("Copying extension block object of size %u.\n", newBlk->size);
	sdr_write(sdr, newBlk->object, buffer, newBlk->size);
	MRELEASE(buffer);
	return 0;
}

int	imc_processOnDequeue(ExtensionBlock *blk, Bundle *bundle, void *ctxt)
{
	Sdr	sdr = getIonsdr();

	if (blk->object)
	{
		/*	No longer need the old list of destinations.	*/

		sdr_free(sdr, blk->object);
		blk->object = 0;
	}

	blk->size = 0;	/*	Zero out object size, bogus or not.	*/
	return imc_serialize(blk, bundle);
}

int	imc_parse(AcqExtBlock *blk, AcqWorkArea *wk)
{
	unsigned char	*cursor;
	unsigned int	unparsedBytes = blk->dataLength;
	uvast		nbrOfDestinationNodes = 0;
	uvast		*destinationNodesArray;
	uvast		*destinationNode;
	uvast		uvtemp;
	int		i;

	if (unparsedBytes < 1)
	{
		writeMemo("[?] Can't decode IMC block.");
		return 0;		/*	Malformed.		*/
	}

	cursor = blk->bytes + (blk->length - blk->dataLength);
	if (cbor_decode_array_open(&nbrOfDestinationNodes, &cursor,
				&unparsedBytes) < 1)
	{
		writeMemo("[?] Can't decode IMC block array.");
		return 0;
	}

	blk->size = nbrOfDestinationNodes * sizeof(uvast);
	blk->object = MTAKE(blk->size);
	if (blk->object == NULL)
	{
		putErrmsg("Can't acquire IMC block.", itoa(blk->size));
		return -1;
	}

	destinationNodesArray = blk->object;
	for (i = 0, destinationNode = destinationNodesArray;
			i < nbrOfDestinationNodes; i++, destinationNode++)
	{
		if (cbor_decode_integer(&uvtemp, CborAny, &cursor,
				&unparsedBytes) < 1)
		{
			writeMemo("[?] Can't decode destination node.");
			MRELEASE(blk->object);
			return 0;	/*	Malformed.		*/
		}

		*destinationNode = uvtemp;
//printf("Parsed destination is " UVAST_FIELDSPEC ".\n", uvtemp);
	}

	if (unparsedBytes != 0)
	{
		writeMemo("[?] Excess bytes at end of IMC block.");
		MRELEASE(blk->object);
		return 0;		/*	Malformed.		*/
	}

	return 1;
}

int	imc_check(AcqExtBlock *blk, AcqWorkArea *wk)
{
	return 1;
}

void	imc_clear(AcqExtBlock *blk)
{
	if (blk->object)
	{
		MRELEASE(blk->object);
	}

	return;
}
