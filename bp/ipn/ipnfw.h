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

#ifdef __cplusplus
extern "C" {
#endif

#define IPN_ALL_OTHERS	((unsigned long) -1)

typedef struct
{
	unsigned long	srcServiceNbr;
	unsigned long	srcNodeNbr;
	FwdDirective	directive;
} IpnRule;

/*	Cbhe-style egress rules are managed in the database's lists
 *	of IpnPlan and IpnGroup objects that associate egress
 *	directives with, respectively, the node numbers of neighboring
 *	nodes and ranges of node numbers of non-neighboring nodes.
 *
 *	Each IpnPlan and each IpnGroup may have an associated list
 *	of IpnRules indicating the egress directives for bundles
 *	that are characterized by a specific source service number
 *	and/or source node.  Each plan and each group also has a
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
 *	of nodes.  For this purpose, one or more "groups" of nodes
 *	-- each group identified by the range of node numbers for
 *	which the designated "via" node takes forwarding responsibility
 *	-- may be defined.  Groups may nest, but otherwise the
 *	memberships of groups may never overlap.  Any bundle that is
 *	to be sent to a node for which no local forwarding knowledge
 *	is available will be forwarded to the designated "via" node
 *	cited by the smallest group between whose first and last node
 *	numbers (inclusive) the destination node's number lies, if any.
 *
 *	All directives established for IpnGroups must be Forwarding
 *	directives.  Note that the First and Last node numbers
 *	specified for a group may be the same number, identifying
 *	a static route to a specified node.				*/

typedef struct
{
	unsigned long	nodeNbr;
	FwdDirective	defaultDirective;
	Object		rules;			/*	SDR list	*/
} IpnPlan;

typedef struct
{
	unsigned long	firstNodeNbr;		/*	in range	*/
	unsigned long	lastNodeNbr;		/*	in range	*/
	FwdDirective	defaultDirective;
	Object		rules;			/*	SDR list	*/
} IpnGroup;

typedef struct
{
	Object		plans;			/*	SDR list	*/
	Object		groups;			/*	SDR list	*/
} IpnDB;

typedef struct
{
	Object		outductElt;
	char		*destDuctName;
} DuctExpression;

extern int		ipnInit();
extern Object		getIpnDbObject();
extern IpnDB		*getIpnConstants();

extern void		ipn_findPlan(unsigned long nodeNbr, Object *planAddr,
				Object *elt);

extern int		ipn_addPlan(unsigned long nodeNbr,
				DuctExpression *ductExpression);
extern int		ipn_updatePlan(unsigned long nodeNbr, 
				DuctExpression *ductExpression);
extern int		ipn_removePlan(unsigned long nodeNbr);

extern void		ipn_findPlanRule(unsigned long nodeNbr,
				long sourceServiceNbr,
				long sourceNodeNbr, IpnPlan *plan,
				Object *ruleAddr, Object *elt);

extern int		ipn_addPlanRule(unsigned long nodeNbr,
				long sourceServiceNbr,
				long sourceNodeNbr,
				DuctExpression *ductExpression);
extern int		ipn_updatePlanRule(unsigned long nodeNbr,
				long sourceServiceNbr,
				long sourceNodeNbr,
				DuctExpression *ductExpression);
extern int		ipn_removePlanRule(unsigned long nodeNbr,
				long sourceServiceNbr,
				long sourceNodeNbr);

extern int		ipn_lookupPlanDirective(unsigned long nodeNbr, 
				unsigned long sourceServiceNbr,
				unsigned long sourceNodeNbr,
				FwdDirective *directive);

extern void		ipn_findGroup(unsigned long firstNodeNbr,
				unsigned long lastNodeNbr,
				Object *groupAddr, Object *elt);
extern int		ipn_addGroup(unsigned long firstNodeNbr,
				unsigned long lastNodeNbr, char *viaEid);
extern int		ipn_updateGroup(unsigned long firstNodeNbr,
				unsigned long lastNodeNbr, char *viaEid);
extern int		ipn_removeGroup(unsigned long firstNodeNbr,
				unsigned long lastNodeNbr);

extern void		ipn_findGroupRule(unsigned long firstNodeNbr,
				unsigned long lastNodeNbr,
				long sourceServiceNbr,
				long sourceNodeNbr, IpnGroup *group,
				Object *ruleAddr, Object *elt);
extern int		ipn_addGroupRule(unsigned long firstNodeNbr,
				unsigned long lastNodeNbr,
				long sourceServiceNbr,
				long sourceNodeNbr, char *viaEid);
extern int		ipn_updateGroupRule(unsigned long firstNodeNbr,
				unsigned long lastNodeNbr,
				long sourceServiceNbr,
				long sourceNodeNbr, char *viaEid);
extern int		ipn_removeGroupRule(unsigned long firstNodeNbr,
				unsigned long lastNodeNbr,
				long sourceServiceNbr,
				long sourceNodeNbr);

extern int		ipn_lookupGroupDirective(unsigned long nodeNbr, 
				unsigned long sourceServiceNbr,
				unsigned long sourceNodeNbr,
				FwdDirective *directive);

extern int		cgr_forward(Bundle *bundle, Object bundleObj,
				unsigned long stationNodeNbr, Object plans);
#ifdef __cplusplus
}
#endif

#endif  /* _IPNFW_H_ */
