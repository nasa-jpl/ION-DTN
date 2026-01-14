/*
 *	bputa.h:	private definitions supporting the use of DTN
 *			Bundle Protocol at the UT layer.
 *
 *
 *	Copyright (c) 2019, California Institute of Technology.
 *	ALL RIGHTS RESERVED.  U.S. Government Sponsorship acknowledged.
 *
 *	Author: Scott Burleigh, JPL
 */

#ifndef BPUTA_H
#define BPUTA_H

#include "bp.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
	uvast		reportToFqnn;
	int		lifespan;
	int		classOfService;
	BpCustodySwitch	custodySwitch;
	unsigned int	ctInterval;
	unsigned char	srrFlags;
	int		ackRequested;
	BpAncillaryData	ancillaryData;
} BpUtParms;

#ifdef __cplusplus
}
#endif

#endif /* BPUTA_H */
