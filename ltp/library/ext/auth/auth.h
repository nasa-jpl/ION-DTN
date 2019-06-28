/*
 *  auth.h:  Definitions of the LTP authentication code.
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
 *                        serializeAuthTrailerExtensionField
 */

#ifndef _LTPAUTH_H_
#define _LTPAUTH_H_
#include "ltpP.h"
#include "ltpsec.h"
#include "ltpei.h"

/*
This is the callback function implementation of
InboundBeforeContentProcessingCallback.

This implementation will parse and verify the LTP authentication extension
fields (both header and trailer, if any), and return -1 to indicate error if
there are authentication extension fields and none of them can be verified,
return 0 otherwise.

Parameters:
- segment : the inbound segment
- headerExtensions: linked list of inbound segment's header extensions
- trailerExtensions: linked list of inbound segment's trailer extensions
- segmentRawData : the raw data of the inbound segment
- vspan : the Ltp vspan

Returns:
- 0 if the callback does not need to call off the segment processing
- -1 if the callback needs to call off the segment processing
*/
extern int	verifyAuthExtensionField(LtpRecvSeg* segment,
			Lyst headerExtensions, Lyst trailerExtensions,
			char *segmentRawData, LtpVspan* vspan);

/*
This is the callback function implementation of
OutboundOnHeaderExtensionGenerationCallback.

This implementation will query the outbound authentication rules and add
authentication header extension fields to the segment if needed, and return -1
to indicate error, return 0 otherwise.

Parameters:
- segment : the segment to transmit

Returns:
- 0 if the callback does not need to call off the segment processing
- -1 if the callback needs to call off the segment processing
*/
extern int	addAuthHeaderExtensionField(LtpXmitSeg *segment);

/*
This is the callback function implementation of
OutboundOnTrailerExtensionGenerationCallback.

This implementation will only add authentication trailer extensions (but the
AuthVal will not be computed here), and return -1 to indicate error, return 0
otherwise.

Parameters:
- segment : the segment to transmit

Returns:
- 0 if the callback does not need to call off the segment processing
- -1 if the callback needs to call off the segment processing
*/
extern int	addAuthTrailerExtensionField(LtpXmitSeg *segment);

/*
This is the callback function implementation of
OutboundOnTrailerExtensionSerializationCallback.

This implementation will compute the AuthVal for each authentication trailer
extension field, and return -1 to indicate error, return 0 otherwise.

Parameters:
- fieldObj : the extension field
- segment : the segment to transmit
- cursor : the outbound cursor

Returns:
- 0 if the callback does not need to call off the segment processing
- -1 if the callback needs to call off the segment processing
*/
extern int	serializeAuthTrailerExtensionField(Object fieldObj,
			LtpXmitSeg *segment, char** cursor);

#endif
