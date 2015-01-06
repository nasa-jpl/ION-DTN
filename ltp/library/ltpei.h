/*
 *  ltpei.h: Private header file used to encapsulate structures,
 *           constants, and function prototypes that deal with LTP
 *           extension blocks.
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

#ifndef _LTPEI_H_
#define _LTPEI_H_

/*****************************************************************************
 *                           CALLBACK TYPE DEFINITIONS                       *
 *****************************************************************************/
/*
This is the definition of callback function which will be called before the
segment content of an inbound segment is processed.

Parameters:
- segment : the inbound segment
- segmentRawData : the raw data of the inbound segment
- session : the session
- span : the Ltp span
- vspan : the ltp vspan

Returns:
- 0 if the callback does not need to call off the segment processing
- -1 if the callback needs to call off the segment processing
*/
typedef	int	(*InboundBeforeContentProcessingCallback)
			(LtpRecvSeg *segment, Lyst headerExtensions,
			 Lyst trailerExtensions, char *segmentRawData,
			 LtpVspan *vspan);
/*
This is the definition of callback function which will be called to add LTP
header extensions to an outbound segment.

Parameters:
- segment : the segment to transmit
- session : the export session (for data segment)
- span : the span

Returns:
- 0 if the callback does not need to call off the segment processing
- -1 if the callback needs to call off the segment processing
*/
typedef int	(*OutboundOnHeaderExtensionGenerationCallback)
			(LtpXmitSeg *segment);
/*
This is the definition of callback function which will be called to add LTP
trailer extensions to an outbound segment.

Parameters:
- segment : the segment to transmit
- session : the export session (for data segment)
- span : the span

Returns:
- 0 if the callback does not need to call off the segment processing
- -1 if the callback needs to call off the segment processing
*/
typedef int	(*OutboundOnTrailerExtensionGenerationCallback)
			(LtpXmitSeg *segment);
/*
This is the definition of callback function which will be called to serialize
LTP header extensions to an outbound segment.

Parameters:
- fieldObj : the extension field
- segment : the segment to transmit
- cursor : the outbound cursor

Returns:
- 0 if the callback does not need to call off the segment processing
- -1 if the callback needs to call off the segment processing
*/
typedef int	(*OutboundOnHeaderExtensionSerializationCallback)
			(Object fieldObj, LtpXmitSeg *segment, char **cursor);
/*
This is the definition of callback function which will be called to serialize
LTP trailer extensions to an outbound segment.

Parameters:
- fieldObj : the extension field
- segment : the segment to transmit
- cursor : the outbound cursor

Returns:
- 0 if the callback does not need to call off the segment processing
- -1 if the callback needs to call off the segment processing
*/
typedef int	(*OutboundOnTrailerExtensionSerializationCallback)
			(Object fieldObj, LtpXmitSeg *segment, char **cursor);

/*****************************************************************************
 *                                DATA STRUCTURES                            *
 *****************************************************************************/
/*
 *  \struct LtpExtensionField
 *  \tag     Represents the extension tag, for instance 0x00 for RFC-5327 LTP
 *           Authentication Extension.
 *  \length  Represents the length of the extension field value, it will be a
 *           positive integer.
 *  \value   Represents the extension field value. It will be the address of an
 *           SDR Object. The actual extension value depends on the extension
 *           type, and concrete structures will be defined in concrete
 *           extension handling code.
 *
 * This struct defines generic LTP extension fields (header or trailer).
 */
typedef struct
{
	unsigned int	offset;
	char		tag;
	unsigned int	length;
	char		*value;
} LtpExtensionInbound;

typedef struct
{
	char		tag;
	unsigned int	length;
	Object		value;
} LtpExtensionOutbound;

/*
 *  \struct ExtensionDef
 *  \name    Represents the name.
 *  \tag     Represents the extension tag, for instance 0x00 for RFC-5327 LTP
 *           Authentication Extension.
 *  \inboundBeforeContentProcessing  Represents the callback function for
 *           InboundBeforeContentProcessingCallback. Can be null.
 *  \outboundOnHeaderExtensionGeneration  Represents the callback function for
 *           OutboundOnHeaderExtensionGenerationCallback. Can be null.
 *  \outboundOnTrailerExtensionGeneration  Represents the callback function for
 *           OutboundOnTrailerExtensionGenerationCallback. Can be null.
 *  \outboundOnHeaderExtensionSerialization  Represents the callback function
 *           for OutboundOnHeaderExtensionSerializationCallback. Can be null.
 *  \outboundOnTrailerExtensionSerialization  Represents the callback function
 *           for OutboundOnTrailerExtensionSerializationCallback. Can be null.
 *
 * This struct defines the callbacks used to process extension fields.
 */
typedef struct
{
	char	name[32];
	char	tag;
	InboundBeforeContentProcessingCallback
		inboundBeforeContentProcessing;
	OutboundOnHeaderExtensionGenerationCallback
		outboundOnHeaderExtensionGeneration;
	OutboundOnTrailerExtensionGenerationCallback
		outboundOnTrailerExtensionGeneration;
	OutboundOnHeaderExtensionSerializationCallback
		outboundOnHeaderExtensionSerialization;
	OutboundOnTrailerExtensionSerializationCallback
		outboundOnTrailerExtensionSerialization;
} ExtensionDef;

/*****************************************************************************
 *                     UTILITY FUNCTION PROTOTYPES                           *
 *****************************************************************************/

extern int	ltpei_add_xmit_header_extension(LtpXmitSeg *segment,
			char tag, int valueLength, char *value);
extern int	ltpei_add_xmit_trailer_extension(LtpXmitSeg *segment,
			char tag, int valueLength, char *value);

extern int	ltpei_parse_extension(char **cursor, int *bytesRemaining,
			Lyst extensionsList, unsigned int *offset);
extern int	ltpei_record_extensions(LtpPdu *pdu, Lyst headerExtensions,
			Lyst trailerExtensions);
extern void	ltpei_discard_extensions(Lyst extensions);

extern void	ltpei_destroy_extension(Sdr, Object, void *);

/*
This function is used to find the ExtensionDef based on the tag.

Parameters:
- tag : the tag

Returns:
the ExtensionDef
*/
extern ExtensionDef	*findLtpExtensionDef(char tag);

/*****************************************************************************
 *                           FUNCTION PROTOTYPES                             *
 *****************************************************************************/
/*
Invoke all InboundBeforeContentProcessingCallback functions.

Parameters:
- segment : the inbound segment
- segmentRawData : the raw data of the inbound segment
- session : the session
- span : the Ltp span
- vspan : the Ltp vspan

Returns:
- 0 if none of the callbacks needs to call off the segment processing
- -1 if any callback needs to call off the segment processing
*/
extern int	invokeInboundBeforeContentProcessingCallbacks
			(LtpRecvSeg *segment, Lyst headerExtensions,
			 Lyst trailerExtensions, char *segmentRawData,
			 LtpVspan *vspan);
/*
Invoke all OutboundOnHeaderExtensionGenerationCallback functions.

Parameters:
- segment : the segment to transmit
- session : the export session (for data segment)
- span : the span

Returns:
- 0 if the callback does not need to call off the segment processing
- -1 if the callback needs to call off the segment processing
*/
extern int	invokeOutboundOnHeaderExtensionGenerationCallbacks
			(LtpXmitSeg *segment);
/*
Invoke all OutboundOnTrailerExtensionGenerationCallback functions.

Parameters:
- segment : the segment to transmit
- session : the export session (for data segment)
- span : the span

Returns:
- 0 if the callback does not need to call off the segment processing
- -1 if the callback needs to call off the segment processing
*/
extern int	invokeOutboundOnTrailerExtensionGenerationCallbacks
			(LtpXmitSeg *segment);

#endif	/* _LTPEI_H_ */
