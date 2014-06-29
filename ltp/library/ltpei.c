/*
 *  ltpei.c: This is the C module that implements functions used to invoke
 *           extension callbacks, as well as common utility functions used
 *           by extension code.
 *
 *	Copyright (c) 2014, California Institute of Technology.
 *	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
 *	acknowledged.
 *									
 *  Author: TCSASSEMBLER, TopCoder
 *
 *  Modification History:
 *  Date       Who     What
 *  02-04-14    TC      - Added this file
 *  02-19-14    TC      - Modified signature of function
 *                        getIndexOfExtensionField and getExtensionField
 */
#include "ion.h"
#include "ltpP.h"
#include "ltpei.h"
#include "ltpextensions.c"

/* The number of LTP extensions. */
static int	extensionsCount = sizeof extensions / sizeof(ExtensionDef);

ExtensionDef	*findLtpExtensionDef(char tag)
{
	int		count = extensionsCount;
	int		i;
	ExtensionDef	*def;

	for (i = 0, def = extensions; i < count; i++, def++)
	{
		if (def->tag == tag)
		{
			return def;
		}
	}

	return NULL;
}

int	ltpei_add_xmit_header_extension(LtpXmitSeg *segment, char tag,
		int valueLength, char *value)
{
	Sdr 			sdr = getIonsdr();
	Sdnv			sdnv;
	LtpExtensionOutbound	extension;
	Object			addr;

	CHKERR(segment);
	CHKERR(ionLocked());
	encodeSdnv(&sdnv, valueLength);
	if (segment->pdu.headerExtensions == 0)
	{
		if ((segment->pdu.headerExtensions = sdr_list_create(sdr)) == 0)
		{
			return -1;	/*	No space in SDR heap.	*/
		}
	}

	extension.tag = tag;
	extension.length = valueLength;
	if (valueLength == 0)
	{
		extension.value = 0;
	}
	else
	{
		CHKERR(value);
		extension.value = sdr_insert(sdr, value, valueLength);
		if (extension.value == 0)
		{
			return -1;	/*	No space in SDR heap.	*/
		}
	}

	if ((addr = sdr_insert(sdr, (char *) &extension,
			sizeof(LtpExtensionOutbound))) == 0)
	{
		return -1;		/*	No space in SDR heap.	*/
	}

	if (sdr_list_insert_last(sdr, segment->pdu.headerExtensions, addr) == 0)
	{
		return -1;		/*	No space in SDR heap.	*/
	}

	segment->pdu.headerExtensionsCount++;
	segment->pdu.headerLength += (1 + sdnv.length + valueLength);
	return 0;
}

int	ltpei_add_xmit_trailer_extension(LtpXmitSeg *segment, char tag,
		int valueLength, char *value)
{
	Sdr 			sdr = getIonsdr();
	Sdnv			sdnv;
	LtpExtensionOutbound	extension;
	Object			addr;

	CHKERR(segment);
	CHKERR(ionLocked());
	encodeSdnv(&sdnv, valueLength);
	if (segment->pdu.trailerExtensions == 0)
	{
		if ((segment->pdu.trailerExtensions = sdr_list_create(sdr))
				== 0)
		{
			return -1;	/*	No space in SDR heap.	*/
		}
	}

	extension.tag = tag;
	extension.length = valueLength;
	if (valueLength == 0)
	{
		extension.value = 0;
	}
	else
	{
		CHKERR(value);
		extension.value = sdr_insert(sdr, value, valueLength);
		if (extension.value == 0)
		{
			return -1;	/*	No space in SDR heap.	*/
		}
	}

	if ((addr = sdr_insert(sdr, (char *) &extension,
			sizeof(LtpExtensionOutbound))) == 0)
	{
		return -1;		/*	No space in SDR heap.	*/
	}

	if (sdr_list_insert_last(sdr, segment->pdu.trailerExtensions, addr)
			== 0)
	{
		return -1;		/*	No space in SDR heap.	*/
	}

	segment->pdu.trailerExtensionsCount++;
	segment->pdu.trailerLength += (1 + sdnv.length + valueLength);
	return 0;
}

int	ltpei_parse_extension(char **cursor, int *bytesRemaining, Lyst exts,
		unsigned int *extensionOffset)
{
	char			*initialCursor;
	LtpExtensionInbound	*extField;
	unsigned int		valueLength;

	CHKERR(cursor);
	CHKERR(*cursor);
	CHKERR(bytesRemaining);
	CHKERR(exts);
	if ((*bytesRemaining) < 1)
	{
		return 0;	/*	Corrupt.			*/
	}

	extField = MTAKE(sizeof(LtpExtensionInbound));
	if (extField == NULL)
	{
		return -1;	/*	Give up.			*/
	}

	initialCursor = *cursor;
	extField->offset = *extensionOffset;
	extField->tag = **cursor;
	(*cursor)++;
	(*bytesRemaining)--;
	extractSmallSdnv(&valueLength, cursor, bytesRemaining);
	if (valueLength == 0 || *bytesRemaining < valueLength)
	{
		return 0;	/*	Corrupt.			*/
	}

	extField->length = valueLength;
	extField->value = MTAKE(valueLength);
	if (extField->value == 0)
	{
		MRELEASE(extField);
		return -1;	/*	Give up.			*/
	}

	memcpy(extField->value, *cursor, valueLength);
	(*cursor) += valueLength;
	(*bytesRemaining) -= valueLength;
	if (lyst_insert_last(exts, extField) == NULL)
	{
		MRELEASE(extField->value);
		MRELEASE(extField);
		return -1;	/*	Give up.			*/
	}

	*extensionOffset += ((*cursor) - initialCursor);
	return 1;
}

void	ltpei_discard_extensions(Lyst extensions)
{
	LystElt			elt;
	LtpExtensionInbound	*ext;

	CHKVOID(extensions);
	while ((elt = lyst_first(extensions)) != NULL)
	{
		ext = (LtpExtensionInbound *) lyst_data(elt);
		if (ext->value)
		{
			MRELEASE(ext->value);
		}

		MRELEASE(ext);
		lyst_delete(elt);
	}

	lyst_destroy(extensions);
}

void	ltpei_destroy_extension(Sdr sdr, Object elt, void *arg)
{
	Object			addr;
	LtpExtensionOutbound	ext;

	addr = sdr_list_data(sdr, elt);
	sdr_read(sdr, (char *) &ext, addr, sizeof(LtpExtensionOutbound));
	if (ext.value)
	{
		sdr_free(sdr, ext.value);
	}

	sdr_free(sdr, addr);
}

int	invokeInboundBeforeContentProcessingCallbacks(LtpRecvSeg* segment,
		Lyst headerExtensions, Lyst trailerExtensions,
		char *segmentRawData, LtpVspan* vspan)
{
	int		count = extensionsCount;
	int		 i;
	ExtensionDef	*def;
	int		result;

	for (i = 0, def = extensions; i < count; i++, def++)
	{
		if (def->inboundBeforeContentProcessing != NULL)
		{
			result = def->inboundBeforeContentProcessing(segment,
					headerExtensions, trailerExtensions,
					segmentRawData, vspan);
			if (result < 1)
			{
				return result;
			}
		}
	}

	return 1;
}

int	invokeOutboundOnHeaderExtensionGenerationCallbacks(LtpXmitSeg *segment)
{
	int		count = extensionsCount;
	int		i;
	ExtensionDef	*def;

	for (i = 0, def = extensions; i < count; i++, def++)
	{
		if (def->outboundOnHeaderExtensionGeneration != NULL)
		{
			if (def->outboundOnHeaderExtensionGeneration(segment)
					< 0)
			{
				return -1;
			}
		}
	}

	return 0;
}

int	invokeOutboundOnTrailerExtensionGenerationCallbacks(LtpXmitSeg *segment)
{
	int		count = extensionsCount;
	int		 i;
	ExtensionDef	*def;

	for (i = 0, def = extensions; i < count; i++, def++)
	{
		if (def->outboundOnTrailerExtensionGeneration != NULL)
		{
			if (def->outboundOnTrailerExtensionGeneration(segment)
					< 0)
			{
				return -1;
			}
		}
	}

	return 0;
}
