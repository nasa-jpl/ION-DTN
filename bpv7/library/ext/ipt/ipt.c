/*
 *	ipt.c:		implementation of the extension definition
 *			functions for the IRF Passageway Trace block.
 *
 *	Copyright (c) 2021, IPNGROUP.  ALL RIGHTS RESERVED.
 *
 *	Author: Scott Burleigh, IPNGROUP
 */

#include "bpP.h"
#include "ipt.h"

int	ipt_offer(ExtensionBlock *blk, Bundle *bundle)
{
	/*	Block must be offered as a placeholder to enable
	 *	later extension block processing.			*/

	(void)bundle;

	blk->dataLength = 0;	/*	Will know length at dequeue.	*/
	blk->length = 0;
	blk->size = 1;		/*	Just to keep block alive.	*/
	blk->object = 0;
	return 0;
}

int	ipt_serialize(ExtensionBlock *blk, Bundle *bundle)
{
	Sdr		sdr = getIonsdr();
	int		nbrOfNodesInTrace;
	int		bufferLength;
	unsigned char	*dataBuffer;
	unsigned char	*cursor;
	uvast		uvtemp;
	Object		elt;
	int		result;

	if (bundle->passageways == 0)
	{
		return 0;	/*	IPT block is unnecessary.	*/
	}

	nbrOfNodesInTrace = sdr_list_length(sdr, bundle->passageways);
	if (nbrOfNodesInTrace == 0)
	{
		return 0;	/*	IPT block is unnecessary.	*/
	}

	/*	Insert the new list of node numbers into the block.	*/

	bufferLength = 9 * (nbrOfNodesInTrace + 1);
	dataBuffer = MTAKE(bufferLength);
	if (dataBuffer == NULL)
	{
		putErrmsg("No space for constructing IPT block.", NULL);
		return -1;
	}

	cursor = dataBuffer;
	uvtemp = nbrOfNodesInTrace;
	oK(cbor_encode_array_open(uvtemp, &cursor));
	for (elt = sdr_list_first(sdr, bundle->passageways); elt;
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

void	ipt_release(ExtensionBlock *blk)
{
	Sdr	sdr = getIonsdr();

	if (blk->object)
	{
		sdr_free(sdr, blk->object);
	}

	return;
}

int	ipt_record(ExtensionBlock *sdrBlk, AcqExtBlock *ramBlk)
{
	Sdr	sdr = getIonsdr();

	if (ramBlk->size == 0)
	{
		return 0;
	}

	sdrBlk->size = ramBlk->size;
	sdrBlk->object = sdr_malloc(sdr, sdrBlk->size);
	if (sdrBlk->object == 0)
	{
		putErrmsg("Not enough heap space for IPT block.", NULL);
		return -1;
	}

	sdr_write(sdr, sdrBlk->object, (char *) (ramBlk->object), sdrBlk->size);
	return 0;
}

int	ipt_copy(ExtensionBlock *newBlk, ExtensionBlock *oldBlk)
{
	Sdr	sdr = getIonsdr();
	char	*buffer;

	newBlk->size = oldBlk->size;	/*	Possible bogus size.	*/
	if (oldBlk->object == 0)
	{
		return 0;
	}

	buffer = MTAKE(newBlk->size);
	if (buffer == NULL)
	{
		putErrmsg("Not enough memory for IPT block buffer.", NULL);
		return -1;
	}

	newBlk->object = sdr_malloc(sdr, newBlk->size);
	if (newBlk->object == 0)
	{
		MRELEASE(buffer);
		putErrmsg("Not enough heap space for IPT block.", NULL);
		return -1;
	}

	sdr_read(sdr, buffer, oldBlk->object, oldBlk->size);
	sdr_write(sdr, newBlk->object, buffer, newBlk->size);
	MRELEASE(buffer);
	return 0;
}

int	ipt_processOnDequeue(ExtensionBlock *blk, Bundle *bundle, void *ctxt)
{
	Sdr	sdr = getIonsdr();

	(void)ctxt;

	if (blk->object)
	{
		/*	No longer need the old array of passageways.	*/

		sdr_free(sdr, blk->object);
		blk->object = 0;
	}

	blk->size = 0;	/*	Zero out object size, bogus or not.	*/
	return ipt_serialize(blk, bundle);
}

int	ipt_parse(AcqExtBlock *blk, AcqWorkArea *wk)
{
	unsigned char	*cursor;
	unsigned int	unparsedBytes = blk->dataLength;
	uvast		nbrOfNodesInTrace = 0;
	uvast		*nodeNbrsArray;
	uvast		*nodeNbr;
	uvast		uvtemp;
	uvast		i;

	(void)wk;

	if (unparsedBytes < 1)
	{
		writeMemo("[?] Can't decode IPT block.");
		return 0;		/*	Malformed.		*/
	}

	cursor = blk->bytes + (blk->length - blk->dataLength);
	if (cbor_decode_array_open(&nbrOfNodesInTrace, &cursor, &unparsedBytes)
			< 1)
	{
		writeMemo("[?] Can't decode IPT block array.");
		return 0;
	}

	blk->size = nbrOfNodesInTrace * sizeof(uvast);
	blk->object = MTAKE(blk->size);
	if (blk->object == NULL)
	{
		putErrmsg("Can't acquire IPT block.", itoa(blk->size));
		return -1;
	}

	nodeNbrsArray = blk->object;
	for (i = 0, nodeNbr = nodeNbrsArray; i < nbrOfNodesInTrace;
			i++, nodeNbr++)
	{
		if (cbor_decode_integer(&uvtemp, CborAny, &cursor,
				&unparsedBytes) < 1)
		{
			writeMemo("[?] Can't decode destination node.");
			MRELEASE(blk->object);
			return 0;	/*	Malformed.		*/
		}

		*nodeNbr = uvtemp;
	}

	if (unparsedBytes != 0)
	{
		writeMemo("[?] Excess bytes at end of IPT block.");
		MRELEASE(blk->object);
		return 0;		/*	Malformed.		*/
	}

	return 1;
}

int	ipt_check(AcqExtBlock *blk, AcqWorkArea *wk)
{
	(void)blk;
	(void)wk;

	return 1;
}

void	ipt_clear(AcqExtBlock *blk)
{
	if (blk->object)
	{
		MRELEASE(blk->object);
	}

	return;
}
