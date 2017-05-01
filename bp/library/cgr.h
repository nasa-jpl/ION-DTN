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

// A CGR tracepoint
typedef enum
{
	// CgrBuildRoutes(uvast stationNode, unsigned int payloadLength,
	//                unsigned int atTime)
	CgrBuildRoutes,
	// CgrInvalidTerminusNode(void)
	CgrInvalidTerminusNode,

	// CgrBeginRoute(int payloadClass)
	CgrBeginRoute,
	// CgrConsiderRoot(uvast fromNode, uvast toNode)
	CgrConsiderRoot,
	// CgrConsiderContact(uvast fromNode, uvast toNode)
	CgrConsiderContact,
	// CgrIgnoreContact(CgrReason reason)
	CgrIgnoreContact,

	// CgrCost(unsigned int transmitTime, unsigned int owlt,
	//		unsigned int arrivalTime)
	CgrCost,
	// CgrHop(uvast fromNode, uvast toNode)
	CgrHop,

	// CgrAcceptRoute(uvast firstHop, unsigned int fromTime,
	//		unsigned int deliveryTime, uvast maxCapacity,
	//		int payloadClass)
	CgrAcceptRoute,
	// CgrDiscardRoute(void)
	CgrDiscardRoute,

	// CgrIdentifyProximateNodes(unsigned int deadline)
	CgrIdentifyProximateNodes,
	// CgrCheckRoute(int payloadClass, uvast firstHop,
	// 		unsigned int fromTime, unsigned int deliveryTime)
	CgrCheckRoute,
	// CgrRecomputeRoute(void)
	CgrRecomputeRoute,
	// CgrIgnoreRoute(CgrReason reason)
	CgrIgnoreRoute,
	// CgrAddProximateNode(void)
	CgrAddProximateNode,
	// CgrUpdateProximateNode(CgrReason reason)
	CgrUpdateProximateNode,

	// CgrSelectProximateNodes(void)
	CgrSelectProximateNodes,
	// CgrUseAllProximateNodes(void)
	CgrUseAllProximateNodes,
	// CgrConsiderProximateNode(uvast proxNode)
	CgrConsiderProximateNode,
	// CgrSelectProximateNode(void)
	CgrSelectProximateNode,
	// CgrIgnoreProximateNode(CgrReason reason)
	CgrIgnoreProximateNode,
	// CgrUseProximateNode(uvast proxNode)
	CgrUseProximateNode,
	// CgrNoProximateNode(void)
	CgrNoProximateNode,
	// CgrFullOverbooking(double overbooking)
	CgrFullOverbooking,
	// CgrPartialOverbooking(double overbooking)
	CgrPartialOverbooking,

	// End of valid trace types
	CgrTraceTypeMax,
} CgrTraceType;

// Describes the reason CGR made a certain decision
typedef enum
{
	// Reasons to ignore a contact (CgrIgnoreContact)
	CgrContactEndsEarly,
	CgrSuppressed,
	CgrVisited,
	CgrCapacityTooSmall,
	CgrNoRange,

	// Reasons to ignore a route (CgrIgnoreRoute)
	CgrRouteViaSelf,
	CgrRouteCapacityTooSmall,
	CgrInitialContactExcluded,
	CgrRouteTooSlow,
	CgrNoPlan,
	CgrBlockedPlan,
	CgrMaxPayloadTooSmall,
	CgrNoResidualCapacity,
	CgrResidualCapacityTooSmall,

	// Reasons to ignore a proximate node (CgrIgnoreRoute,
	// CgrIgnoreProximateNode) or reasons a previously-selected proximate
	// node was ignored (CgrUpdateProximateNode)
	CgrMoreHops,
	CgrIdentical,
	CgrNoHelp,
	CgrLowerCapacity,
	CgrLaterArrivalTime,
	CgrLargerNodeNbr,

	CgrReasonMax,
} CgrReason;

typedef void		(*CgrTraceFn)(void *data, unsigned int lineNbr,
				CgrTraceType traceType, ...);

typedef struct
{
	CgrTraceFn	fn;	/*	Function to call at tracepoint.	*/
	void		*data;	/*	Data to pass to the function.	*/
} CgrTrace;

extern int		cgr_predict_contacts();

extern void		cgr_start();
extern int		cgr_forward(Bundle *bundle, Object bundleObj,
				uvast terminusNode, CgrTrace *trace);
extern int		cgr_preview_forward(Bundle *bundle, Object bundleObj,
				uvast terminusNode, time_t atTime,
				CgrTrace *trace);
extern float		cgr_prospect(uvast terminusNode, unsigned int deadline);
extern const char	*cgr_tracepoint_text(CgrTraceType traceType);
extern const char	*cgr_reason_text(CgrReason reason);
extern void		cgr_stop();
#ifdef __cplusplus
}
#endif

#endif  /* _CGR_H_ */
