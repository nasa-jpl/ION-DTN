/*
 	irf.h:	definitions supporting the utilization of inter-
		regional forwarding procedures.

	Author: Scott Burleigh, IPNGROUP

	This hierarchical inter-regional forwarding system is
	built on research performed by Pablo Madoery and, later,
	Nicola Alessi as visiting researchers at the Jet
	Propulsion Laboratory, California Institute of Technology.

	Modification History:
	Date      Who   What

	Copyright (c) 2021, IPNGROUP.  ALL RIGHTS RESERVED.
 									*/
#ifndef _IRF_H_
#define _IRF_H_

#include "bpP.h"

/*	Administrative record types	*/
#define	BP_IPT_REPORT	(9)

#ifdef __cplusplus
extern "C" {
#endif

extern int	irf_initialize(IonNode *terminusNode);
extern int	irf_add_candidate(uvast candidateNodeNbr, IonNode *terminusNode,
			PsmAddress nextElt);
extern int	irf_load_passageways(Bundle *bundle, Object bundleObj);
extern int	irf_identify_passageways(IonNode *terminusNode, Bundle *bundle,
			Lyst nominees);
extern int	irf_send_msg(uvast fromNodeNbr, uvast toNodeNbr,
			int isReachable, Lyst passageways);
extern int	irf_source_msg(Bundle *bundle, int isReachable);
extern int	irf_issue_ipt_rpt(Bundle *bundle);
extern int	irf_print_ipt_rpt(BpDelivery *dlf, unsigned char *cursor,
			unsigned int unparsedBytes);

#ifdef __cplusplus
}
#endif

#endif  /* _IRF_H_ */
