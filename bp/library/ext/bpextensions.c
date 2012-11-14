/*
 *	bpextensions.c:	Bundle Protocol extension definition
 *			module, implementing Bundle Authentication
 *			Block support in addition to the Extended
 *			Class of Service (ECOS) block.
 *
 *	Copyright (c) 2008, California Institute of Technology.
 *	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
 *	acknowledged.
 *
 *	Author: Scott Burleigh, JPL
 */

/*	Add external function declarations between here...		*/

#include "ecos.h"
#include "bae.h"
#include "extbspbab.h"
#include "extbsppcb.h"
#include "extbsppib.h"

#ifdef ENABLE_BPACS
#include "cteb.h"
#endif /* ENABLE_BPACS */

/*	... and here.							*/

static ExtensionDef	extensions[] =
{
		{ "bab", BSP_BAB_TYPE, 0,
		//	NK changed the name from "bsp_bab_pre"	
				bsp_babOffer,
				bsp_babRelease,
				bsp_babAcquire,
				bsp_babPreCheck,
				0,
				bsp_babClear,
				bsp_babCopy,
				{0,
				0,
				0,
				bsp_babPreProcessOnDequeue,
				0}
		},
		{ "ecos", EXTENSION_TYPE_ECOS, 0,
				ecos_offer,
				ecos_release,
				ecos_acquire,
				ecos_check,
				ecos_record,
				ecos_clear,
				ecos_copy,
				{ecos_processOnFwd,
				ecos_processOnAccept,
				ecos_processOnEnqueue,
				ecos_processOnDequeue,
				0}
		},
		{ "bae", EXTENSION_TYPE_BAE, 0,
				bae_offer,
				bae_release,
				bae_acquire,
				bae_check,
				bae_record,
				bae_clear,
				bae_copy,
				{bae_processOnFwd,
				bae_processOnAccept,
				bae_processOnEnqueue,
				bae_processOnDequeue,
				0}
		},
#ifdef ENABLE_BPACS
        	{ "cteb", EXTENSION_TYPE_CTEB, 0,
				cteb_offer,
				cteb_release,
				cteb_acquire,
				0,
				cteb_record,
				cteb_clear,
				cteb_copy,
				{0,
				0,
				0,
				cteb_processOnDequeue,
				0}
       		},
#endif /* ENABLE_BPACS */
		{ "pib", BSP_PIB_TYPE, 0,
				bsp_pibOffer,
				bsp_pibRelease,
				bsp_pibAcquire,
				bsp_pibCheck,
				0,
				bsp_pibClear,
				bsp_pibCopy,
				{0,
				0,
				0,
				bsp_pibProcessOnDequeue,
				0}
		},
		{ "pcb", BSP_PCB_TYPE, 0,
				bsp_pcbOffer,
				bsp_pcbRelease,
				bsp_pcbAcquire,
				bsp_pcbCheck,
                                0,
				bsp_pcbClear,
				bsp_pcbCopy,
				{0,
				0,
				0,
				bsp_pcbProcessOnDequeue,
				0}
		},
		{ "bsp_bab_post", BSP_BAB_TYPE, 1,
				bsp_babOffer,
				bsp_babRelease,
				bsp_babAcquire,
				bsp_babPostCheck,
				0,
				bsp_babClear,
				bsp_babCopy,
				{0,
				0,
				0,
				bsp_babPostProcessOnDequeue,
				bsp_babPostProcessOnTransmit}
		},
		{ "unknown",0,0,0,0,0,0,0,0,0,{0,0,0,0,0} }
};

/*	NOTE: the order of appearance of extension definitions in the
 *	extensions array determines the order in which pre-payload
 *	extension blocks will be inserted into locally sourced bundles
 *	prior to the payload block and the order in which post-payload
 *	extension blocks will be inserted into locally sourced bundles
 *	after the payload block.					*/
