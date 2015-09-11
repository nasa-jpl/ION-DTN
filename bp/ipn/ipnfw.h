/*
 	ipnfw.h:	definitions supporting the implementation
			of the forwarding infrastructure for endpoint
			ID schemes conforming to the Compressed Bundle
			Header Encoding conventions.

	Author: Scott Burleigh, JPL

	Modification History:
	Date      Who   What

	Copyright (c) 2005, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
 									*/
#ifndef _IPNFW_H_
#define _IPNFW_H_

#include "bpP.h"
#include "cgr.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IPN_ALL_OTHER_NODES	((uvast) -1)
#define IPN_ALL_OTHER_SERVICES	((unsigned int) -1)

typedef struct
{
	unsigned int	srcServiceNbr;
	uvast		srcNodeNbr;
	FwdDirective	directive;
} IpnRule;

/*	Cbhe-style egress rules are managed in the database's lists
 *	of IpnPlan and IpnExit objects that associate egress
 *	directives with, respectively, the node numbers of neighboring
 *	nodes and ranges of node numbers of non-neighboring nodes.
 *
 *	Each IpnPlan and each IpnExit may have an associated list
 *	of IpnRules indicating the egress directives for bundles
 *	that are characterized by a specific source service number
 *	and/or source node.  Each plan and each exit also has a
 *	default directive, which applies to all bundles for which
 *	no source service/node-specific FwdRules apply.
 *
 *	All directives established for IpnPlans must be Transmission
 *	directives.
 *
 *	Nodes for which no plans or contact schedules are known by
 *	the local forwarder may be forwarded to another forwarder
 *	that is expected to have the requisite knowledge; this
 *	mechanism enables IPN to scale up to relatively large numbers
 *	of nodes.  For this purpose, one or more "exits" -- each
 *	exit identified by a range of node numbers for which the
 *	designated "via" node takes forwarding responsibility
 *	-- may be defined.  Exits may nest, but otherwise the
 *	node ranges of exits may never overlap.  Any bundle that is
 *	to be sent to a node for which no local forwarding knowledge
 *	is available will be forwarded to the designated "via" node
 *	cited by the smallest exit between whose first and last node
 *	numbers (inclusive) the destination node's number lies, if any.
 *
 *	All directives established for IpnExits must be Forwarding
 *	directives.  Note that the First and Last node numbers
 *	specified for a exit may be the same number, identifying
 *	a static route to a specified node.				*/

typedef struct
{
	uvast		nodeNbr;
	FwdDirective	defaultDirective;
	Object		rules;			/*	SDR list	*/
} IpnPlan;

typedef struct
{
	uvast		firstNodeNbr;		/*	in range	*/
	uvast		lastNodeNbr;		/*	in range	*/
	FwdDirective	defaultDirective;
	Object		rules;			/*	SDR list	*/
} IpnExit;

typedef struct
{
	Object		plans;			/*	SDR list	*/
	Object		exits;			/*	SDR list	*/
} IpnDB;

typedef struct
{
	Object		outductElt;
	char		*destDuctName;
} DuctExpression;

extern int		ipnInit();
extern Object		getIpnDbObject();
extern IpnDB		*getIpnConstants();

extern void		ipn_findPlan(uvast nodeNbr, Object *planAddr,
				Object *elt);

extern int		ipn_addPlan(uvast nodeNbr,
				DuctExpression *ductExpression);
extern int		ipn_updatePlan(uvast nodeNbr, 
				DuctExpression *ductExpression);
extern int		ipn_removePlan(uvast nodeNbr);

extern void		ipn_findPlanRule(uvast nodeNbr,
				int sourceServiceNbr,
				vast sourceNodeNbr, IpnPlan *plan,
				Object *ruleAddr, Object *elt);

extern uvast		ipn_planNodeNbr(DuctExpression *ductExpression);

extern int		ipn_addPlanRule(uvast nodeNbr,
				int sourceServiceNbr,
				vast sourceNodeNbr,
				DuctExpression *ductExpression);
extern int		ipn_updatePlanRule(uvast nodeNbr,
				int sourceServiceNbr,
				vast sourceNodeNbr,
				DuctExpression *ductExpression);
extern int		ipn_removePlanRule(uvast nodeNbr,
				int sourceServiceNbr,
				vast sourceNodeNbr);

extern int		ipn_lookupPlanDirective(uvast nodeNbr, 
				unsigned int sourceServiceNbr,
				uvast sourceNodeNbr,
				Bundle *bundle,
				FwdDirective *directive);

extern void		ipn_findExit(uvast firstNodeNbr,
				uvast lastNodeNbr,
				Object *exitAddr, Object *elt);
extern int		ipn_addExit(uvast firstNodeNbr,
				uvast lastNodeNbr, char *viaEid);
extern int		ipn_updateExit(uvast firstNodeNbr,
				uvast lastNodeNbr, char *viaEid);
extern int		ipn_removeExit(uvast firstNodeNbr,
				uvast lastNodeNbr);

extern void		ipn_findExitRule(uvast firstNodeNbr,
				uvast lastNodeNbr,
				int sourceServiceNbr,
				vast sourceNodeNbr, IpnExit *exit,
				Object *ruleAddr, Object *elt);
extern int		ipn_addExitRule(uvast firstNodeNbr,
				uvast lastNodeNbr,
				int sourceServiceNbr,
				vast sourceNodeNbr, char *viaEid);
extern int		ipn_updateExitRule(uvast firstNodeNbr,
				uvast lastNodeNbr,
				int sourceServiceNbr,
				vast sourceNodeNbr, char *viaEid);
extern int		ipn_removeExitRule(uvast firstNodeNbr,
				uvast lastNodeNbr,
				int sourceServiceNbr,
				vast sourceNodeNbr);
extern int		ipn_lookupExitDirective(uvast nodeNbr, 
				unsigned int sourceServiceNbr,
				uvast sourceNodeNbr,
				FwdDirective *directive);

extern void		ipn_forgetOutduct(Object ductElt);
#ifdef __cplusplus
}
#endif

#endif  /* _IPNFW_H_ */
