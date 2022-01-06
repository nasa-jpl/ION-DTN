/*
 	irr.h:	definitions supporting the utilization of Contact
		Graph Routing in forwarding infrastructure.

	Author: Scott Burleigh, IPNGROUP

	Modification History:
	Date      Who   What

	Copyright (c) 2021, IPNGROUP.  ALL RIGHTS RESERVED.
 									*/
#ifndef _IRR_H_
#define _IRR_H_

#include "bpP.h"

#ifdef __cplusplus
extern "C" {
#endif

extern int	irr_initialize(IonNode *terminusNode);
extern int	irr_add_candidate(uvast candidateNodeNbr, IonNode *terminusNode,
			PsmAddress nextElt);
extern int	irr_load_passageways(Bundle *bundle, Object bundleObj);
extern int	irr_identify_passageways(IonNode *terminusNode, Bundle *bundle,
			Lyst nominees);
extern int	irr_send_msg(uvast fromNodeNbr, uvast toNodeNbr,
			int isReachable, Lyst passageways);
extern int	irr_source_msg(Bundle *bundle, int isReachable);

#ifdef __cplusplus
}
#endif

#endif  /* _IRR_H_ */
