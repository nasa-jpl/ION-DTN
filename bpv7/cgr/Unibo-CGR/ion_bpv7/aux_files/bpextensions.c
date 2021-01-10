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
#if RGREB
#include "rgr.h"
#endif
#if CGRREB
#include "cgrr.h"
#endif
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
				0,
				bcbSerialize,
				{0,
				0,
				0,
				0,
				0},
				bcbRelease,
				bcbCopy,
				bcbAcquire,
				0,
				0,
				0,
				0,
                                bcbRecord,
				bcbClear
		},
		{ "bib", BlockIntegrityBlk,
				0,
				bibSerialize,
				{0,
				0,
				0,
				0,
				0},
				bibRelease,
				bibCopy,
				0,
				0,
				0,
				bibParse,
				0,
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
#if RGREB
		{ "rgr", RGRBlk,
				rgr_offer,
				0,
				{rgr_processOnFwd,
				 rgr_processOnAccept,
				 rgr_processOnEnqueue,
				 rgr_processOnDequeue,
				 0},
				 rgr_release,
				 rgr_copy,
				 rgr_acquire,
				 0,
				 0,
				 rgr_parse,
				 rgr_check,
				 rgr_record,
				 rgr_clear
		},
#endif
#if CGRREB
		{ "cgrr", CGRRBlk,
				cgrr_offer,
				0,
				{0,
				0,
				0,
				0,
				0},
				cgrr_release,
				cgrr_copy,
				cgrr_acquire,
				0,
				0,
				0,
				0,
				cgrr_record,
				cgrr_clear
		},
#endif
		{ "unknown",-1,0,0,{0,0,0,0,0},0,0,0,0,0,0,0,0,0 }
			};

static ExtensionSpec		extensionSpecs[] =
				{
					{ PreviousNodeBlk, 0, NoCRC },
					{ QualityOfServiceBlk, 0, NoCRC },
					{ BundleAgeBlk, 0, NoCRC },
					{ SnwPermitsBlk, 0, NoCRC },
					{ ImcDestinationsBlk, 0, NoCRC },
#if RGREB
					{ RGRBlk, 0, NoCRC },
#endif
#if CGRREB
					{ CGRRBlk, 0, NoCRC },
#endif
					{ UnknownBlk, 0, NoCRC }
				};
