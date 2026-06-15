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
#ifdef ENABLE_IMC
#include "imc.h"
#endif
#include "creb.h"
#include "cteb.h"
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
				0,  /* Offer Function           - N/A. BCBs offered directly by BPA. */
				0,  /* Serialize Function       - N/A. BCBs serializied by BPA.      */
				{0, /* Process on Forward       - N/A. */
				0,  /* Process on Custody       - N/A. */
				0,  /* Process on Enqueue       - N/A. */
				0,  /* Process on Dequeue       - N/A. */
				0}, /* Process on Transmit      - N/A. */
				bpsec_util_outboundBlkRelease,   /* Outbound Release function. */
				bpsec_asb_outboundAsbCopy,       /* Outbound Copy Function.    */
#if USING_BSL
				0,  /* BSL parses ASB directly; skip ION's native deserialize.   */
#else
				bpsec_asb_inboundAsbDeserialize, /* Inbound Acquire Function.  */
#endif
				0,  /* Inbound Review function  - N/A. */
				0,  /* Inbound Decrypt function - N/A. */
				0,  /* Inbound Parse function   - N/A. */
				0,  /* Inbound Check function   - N/A. */
				bpsec_asb_inboundAsbRecord,      /* Inbound record function. */
				bpsec_util_inboundBlkClear       /* Inbound clear function.  */
		},
		{ "bib", BlockIntegrityBlk,
				0,  /* Offer Function           - N/A. BIBs offered directly by BPA. */
				0,  /* Serialize Function       - N/A. BIBs serializied by BPA.      */
				{0, /* Process on Forward       - N/A. */
				0,  /* Process on Custody       - N/A. */
				0,  /* Process on Enqueue       - N/A. */
				0,  /* Process on Dequeue       - N/A. */
				0}, /* Process on Transmit      - N/A. */
				bpsec_util_outboundBlkRelease,  /* Outbound Release function. */
				bpsec_asb_outboundAsbCopy,      /* Outbound Copy Function.    */
				0,  /* Inbound Acquire Function - N/A  */
				0,  /* Inbound Review function  - N/A. */
				0,  /* Inbound Decrypt function - N/A. */
#if USING_BSL
				0,  /* BSL parses ASB directly; skip ION's native deserialize.   */
#else
				bpsec_asb_inboundAsbDeserialize, /* Inbound Parse function - BIBs are deserialized in the PARSE callback. */
#endif
				0,  /* Inbound Check function - N/A.   */
				bpsec_asb_inboundAsbRecord,      /* Inbound record function. */
				bpsec_util_inboundBlkClear       /* Inbound clear function.  */
		},
		{ "bpq", QualityOfServiceBlk,
				qos_offer,
				qos_serialize,
				{0,
				0,
				0,
				qos_processOnDequeue,
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
				meb_processOnDequeue,
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
				bae_review,
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
#ifdef ENABLE_IMC
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
#endif
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
		{ "creb", CompressedReportingBlk,
				creb_offer,
				creb_serialize,
				{creb_processOnFwd,
				creb_processOnAccept,
				creb_processOnEnqueue,
				creb_processOnDequeue,
				0},
				creb_release,
				creb_copy,
				0,
				0,
				0,
				creb_parse,
				creb_check,
				creb_record,
				creb_clear
		},
		{ "cteb", CustodyTransferBlk,
				cteb_offer,
				cteb_serialize,
				{cteb_processOnFwd,
				cteb_processOnAccept,
				cteb_processOnEnqueue,
				cteb_processOnDequeue,
				0},
				cteb_release,
				cteb_copy,
				0,
				0,
				0,
				cteb_parse,
				cteb_check,
				cteb_record,
				cteb_clear
		},
		{ "unknown",-1,0,0,{0,0,0,0,0},0,0,0,0,0,0,0,0,0 }
			};

static ExtensionSpec		extensionSpecs[] =
				{
#ifdef ION_CORE_BUILD
	/* ION-CORE: Compile-time extension block selection */
	#ifdef PNB_EXT
					{ PreviousNodeBlk, 0, NoCRC },
	#endif
	#ifdef BPQ_EXT
					{ QualityOfServiceBlk, 0, NoCRC },
	#endif
	#ifdef BAE_EXT
					{ BundleAgeBlk, 0, NoCRC },
	#endif
	#ifdef SNW_EXT
					{ SnwPermitsBlk, 0, NoCRC },
	#endif
	#ifdef IMC_EXT
					{ ImcDestinationsBlk, 0, NoCRC },
	#endif
#else
	/* Standard ION-DTN: All extension blocks enabled */
					{ PreviousNodeBlk, 0, NoCRC },
					{ QualityOfServiceBlk, 0, NoCRC },
					{ BundleAgeBlk, 0, NoCRC },
					{ SnwPermitsBlk, 0, NoCRC },
					{ ImcDestinationsBlk, 0, NoCRC },
#endif
#if RGREB
					{ RGRBlk, 0, NoCRC },
#endif
#if CGRREB
					{ CGRRBlk, 0, NoCRC },
#endif
	/*	CBR/CT extension blocks for compressed reporting
	 *	and custody transfer.  Block type 13 (CTEB) and
	 *	block type 14 (CREB).				*/
					{ CompressedReportingBlk, 0, NoCRC },
					{ CustodyTransferBlk, 0, NoCRC },
					{ UnknownBlk, 0, NoCRC }
				};
