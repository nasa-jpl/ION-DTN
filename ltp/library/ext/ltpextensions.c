/*
 *  ltpextensions.c:  Licklider Transmission Protocol
 *          extension definition module, implementing
 *          LTP Authentication Block support.
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
 */
#include "ltpei.h"
#ifdef LTPAUTH
#include "auth.h"
#endif

/** Represents the LTP extension definitions. */
static ExtensionDef	extensions[] =
{
#ifdef LTPAUTH
	{
		"Authentication",
		0x00,
		verifyAuthExtensionField,
		addAuthHeaderExtensionField,
		addAuthTrailerExtensionField,
		NULL,
		serializeAuthTrailerExtensionField
	},
#endif
	{ "unknown",0,0,0,0,0,0 }
};

/* Note that the extension definitions order matters, for
 * instance, the LTP Authentication extension definition
 * must be at the last since it is used to verify the entire
 * LTP segment.*/
