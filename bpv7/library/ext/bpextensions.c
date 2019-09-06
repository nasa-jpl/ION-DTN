/*
 *	bpextensions.c:	Bundle Protocol extension definitions
 *			module.
 *
 *	Copyright (c) 2008, California Institute of Technology.
 *	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
 *	acknowledged.
 *
 *	Author: Scott Burleigh, JPL
 */

/*	Add external function declarations between here...		*/

#include "phn.h"
#include "qos.h"
#include "meb.h"
#include "bae.h"
#include "snw.h"
#if defined(BPSEC)
#include "bib.h"
#include "bcb.h"
#endif

/*	... and here.							*/

static ExtensionDef	extensionDefs[] =
			{
		{ "phn", EXTENSION_TYPE_PHN,
				phn_offer,
				{phn_processOnFwd,
				phn_processOnAccept,
				phn_processOnEnqueue,
				phn_processOnDequeue,
				0},
				phn_release,
				phn_copy,
				0,
				0,
				0,
				phn_parse,
				phn_check,
				phn_record,
				phn_clear
		},
#if defined(BPSEC)
		{ "bcb", BLOCK_TYPE_BCB,
				bcbOffer,
				{0,
				0,
				0,
				bcbProcessOnDequeue,
				0},
				bcbRelease,
				bcbCopy,
				bcbAcquire,
				bcbReview,
				bcbDecrypt,
				0,
				0,
                                0,
				bcbClear
		},
		{ "bib", BLOCK_TYPE_BIB,
				bibOffer,
				{0,
				0,
				0,
				0,
				0},
				bibRelease,
				bibCopy,
				0,
				bibReview,
				0,
				bibParse,
				bibCheck,
				0,
				bibClear
		},
#endif
		{ "qos", EXTENSION_TYPE_QOS,
				qos_offer,
				{qos_processOnFwd,
				qos_processOnAccept,
				qos_processOnEnqueue,
				qos_processOnDequeue,
				0},
				qos_release,
				qos_copy,
				0,
				0,
				0,
				qos_parse,
				qos_check,
				qos_record,
				qos_clear
		},
		{ "meb", EXTENSION_TYPE_MEB,
				meb_offer,
				{meb_processOnFwd,
				meb_processOnAccept,
				meb_processOnEnqueue,
				meb_processOnDequeue,
				0},
				meb_release,
				meb_copy,
				meb_acquire,
				0,
				0,
				0,
				meb_check,
				meb_record,
				meb_clear
		},
		{ "bae", EXTENSION_TYPE_BAE,
				bae_offer,
				{bae_processOnFwd,
				bae_processOnAccept,
				bae_processOnEnqueue,
				bae_processOnDequeue,
				0},
				bae_release,
				bae_copy,
				0,
				0,
				0,
				bae_parse,
				bae_check,
				bae_record,
				bae_clear
		},
		{ "snw", EXTENSION_TYPE_SNW,
				snw_offer,
				{snw_processOnFwd,
				snw_processOnAccept,
				snw_processOnEnqueue,
				snw_processOnDequeue,
				0},
				snw_release,
				snw_copy,
				0,
				0,
				0,
				snw_parse,
				snw_check,
				snw_record,
				snw_clear
		},
				{ "unknown",0,0,{0,0,0,0,0},0,0,0,0,0,0,0,0,0 }
			};

/*	NOTE: the order of appearance of extension definitions in the
 *	extensionSpecs array determines the order in which pre-payload
 *	extension blocks will be inserted into locally sourced bundles
 *	prior to the payload block and the order in which post-payload
 *	extension blocks will be inserted into locally sourced bundles
 *	after the payload block.					*/

static ExtensionSpec	extensionSpecs[] =
			{
				{ EXTENSION_TYPE_PHN, 0, 0, 0 },
				{ EXTENSION_TYPE_QOS, 0, 0, 0 },
				{ EXTENSION_TYPE_MEB, 0, 0, 0 },
				{ EXTENSION_TYPE_BAE, 0, 0, 0 },
				{ EXTENSION_TYPE_SNW, 0, 0, 0 },
#if defined(BPSEC)
				{ BLOCK_TYPE_BIB, 0, 0, 0 },
				{ BLOCK_TYPE_BCB, 1, 0, 0 },
#endif
				{ 0,0,0,0 }
			};
