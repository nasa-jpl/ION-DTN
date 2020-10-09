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

#include "pnb.h"
#include "bpq.h"
#include "meb.h"
#include "bae.h"
#include "hcb.h"
#include "snw.h"
#include "imc.h"
#include "bib.h"
#include "bcb.h"

/*	... and here.							*/

static ExtensionDef	extensionDefs[] =
			{
		{ "pnb", PreviousNodeBlk,
				pnb_offer,
				pnb_serialize,
				{0,
				0,
				0,
				pnb_processOnDequeue,
				0},
				0,
				0,
				0,
				0,
				0,
				pnb_parse,
				pnb_check,
				0,
				0
		},
		{ "bcb", BlockConfidentialityBlk,
				bcbOffer,
				bcbserialize,
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
                                bcbRecord,
				bcbClear
		},
		{ "bib", BlockIntegrityBlk,
				bibOffer,
				bibserialize,
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
				bibRecord,
				bibClear
		},
		{ "bpq", QualityOfServiceBlk,
				qos_offer,
				qos_serialize,
				{0,
				0,
				0,
				0,
				0},
				0,
				0,
				0,
				0,
				0,
				qos_parse,
				qos_check,
				0,
				0
		},
		{ "meb", MetadataBlk,
				meb_offer,
				meb_serialize,
				{0,
				0,
				0,
				0,
				0},
				0,
				0,
				0,
				0,
				0,
				meb_parse,
				meb_check,
				0,
				0
		},
		{ "bae", BundleAgeBlk,
				bae_offer,
				bae_serialize,
				{0,
				0,
				0,
				bae_processOnDequeue,
				0},
				0,
				0,
				0,
				0,
				0,
				bae_parse,
				bae_check,
				0,
				0
		},
		{ "hcb", HopCountBlk,
				hcb_offer,
				hcb_serialize,
				{0,
				0,
				0,
				hcb_processOnDequeue,
				0},
				0,
				0,
				0,
				0,
				0,
				hcb_parse,
				hcb_check,
				0,
				0
		},
		{ "snw", SnwPermitsBlk,
				snw_offer,
				snw_serialize,
				{0,
				0,
				0,
				snw_processOnDequeue,
				pnb_serialize,
				0},
				0,
				0,
				0,
				0,
				0,
				snw_parse,
				snw_check,
				0,
				0
		},
		{ "imc", ImcDestinationsBlk,
				imc_offer,
				imc_serialize,
				{0,
				0,
				0,
				imc_processOnDequeue,
				0},
				imc_release,
				imc_copy,
				0,
				0,
				0,
				imc_parse,
				imc_check,
				imc_record,
				imc_clear
		},
		{ "unknown",-1,0,0,{0,0,0,0,0},0,0,0,0,0,0,0,0,0 }
			};

/*	NOTE: see the comments in the bei.h header file for an
 *	explanation of the target scope mechanism that enables
 *	target multiplicity in the Bundle Security protocol.
 *
 *	The first target scope in the target scopes array MUST ALWAYS
 *	be a scope that includes the primary block, so that BIB
 *	protection of the primary block is always supported (even
 *	if operationally disabled at a given node).			*/

static ExtensionTargetSpec	defaultBibTargets[] =
				{
					{ PrimaryBlk, 0 }
				};

static ExtensionTargetSpec	defaultBcbTargets[] =
				{
					{ PayloadBlk, 0 }
				};

static ExtensionTargetScope	defaultBibScope =
				{
					sizeof defaultBibTargets
			       		/ sizeof(ExtensionTargetSpec), 
					defaultBibTargets
				};

static ExtensionTargetScope	defaultBcbScope =
				{
					sizeof defaultBcbTargets
					/ sizeof(ExtensionTargetSpec), 
					defaultBcbTargets
				};

static ExtensionTargetScope	targetScopes[] =
				{
					defaultBibScope,
					defaultBcbScope
				};

static int			targetScopesCount = sizeof targetScopes
					/ sizeof(ExtensionTargetScope);

static ExtensionSpec		extensionSpecs[] =
				{
					{ PreviousNodeBlk, 0, 0 },
					{ QualityOfServiceBlk, 0, 0 },
					{ BundleAgeBlk, 0, 0 },
					{ SnwPermitsBlk, 0, 0 },
					{ ImcDestinationsBlk, 0, 0 },
					{ BlockIntegrityBlk, 0, 0 },
					{ BlockConfidentialityBlk, 1, 0 },
					{ UnknownBlk, 0, 0 }
				};
