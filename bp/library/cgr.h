/*
 	cgr.h:	definitions supporting the utilization of Contact
		Graph Routing in forwarding infrastructure.

	Author: Scott Burleigh, JPL

	Modification History:
	Date      Who   What

	Copyright (c) 2011, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
 									*/
#ifndef _CGR_H_
#define _CGR_H_

#include "bpP.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef int		(*CgrLookupFn)(uvast nodeNbr, Object plans,
				Bundle *bundle, FwdDirective *directive);
extern void		cgr_start();
extern int		cgr_forward(Bundle *bundle, Object bundleObj,
				uvast stationNodeNbr, Object plans,
				CgrLookupFn callback);
extern void		cgr_stop();
#ifdef __cplusplus
}
#endif

#endif  /* _CGR_H_ */
