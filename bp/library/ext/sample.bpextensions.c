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
#include "ionbsp.h"

/*	... and here.							*/

static ExtensionDef	extensions[] =
			{
				{ "bsp_bab_pre", BSP_BAB_TYPE, 0,
					bsp_babOffer,
					bsp_babRelease,
					bsp_babPreAcquire,
					bsp_babPreCheck,
					0,
					bsp_babClear,
					0,
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
				{ "bsp_bab_post", BSP_BAB_TYPE, 1,
					bsp_babOffer,
					bsp_babRelease,
					bsp_babAcquire,
					bsp_babPostCheck,
					0,
					bsp_babClear,
					0,
					{0,
					0,
					0,
					bsp_babPostProcessOnDequeue,
					bsp_babPostProcessOnTransmit}
				},
				{ "unknown",0,0,0,0,0,0,0,0,0,{0,0,0,0,0} }
			};

static int		extensionsCt = sizeof extensions / sizeof(ExtensionDef);

/*	NOTE: the order of appearance of extension definitions in the
 *	extensions array determines the order in which pre-payload
 *	extension blocks will be inserted into locally sourced bundles
 *	prior to the payload block and the order in which post-payload
 *	extension blocks will be inserted into locally sourced bundles
 *	after the payload block.					*/
