/*
 *	bssfw.h:	definitions supporting the implementation
 *			of the forwarding infrastructure for endpoint
 *			ID schemes conforming to the Compressed Bundle
 *			Header Encoding conventions.
 *
 *	Copyright (c) 2005, California Institute of Technology.	
 *	Copyright (c) 2011, Space Internetworking Center,
 *	Democritus University of Thrace.
 *
 *	All rights reserved.						
 *	
 *	Authors: Scott Burleigh, Jet Propulsion Laboratory
 *		 Sotirios-Angelos Lenas, Space Internetworking Center (SPICE)
 *
 *	Modification History:
 *	Date	  Who	What
 *	08-08-11  SAL	Bundle Streaming Service extension.  
 */

#ifndef _BSSFW_H_
#define _BSSFW_H_

#include "bpP.h"
#include "cgr.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BSS_ALL_OTHERS	((unsigned long) -1)

/*  
 *  Each BSS stream is recorded in loggedStreams lyst. Stream structure    
 *  provides monitoring assistance by storing several crucial information for 
 *  each stream. Each entry in loggedStreams list has the following format:  
 *  +--------------------+-----------------------+-------------------------+  
 *  | source node number | source service number | destination node number |  
 *  +--------------------+-----------------------+-------------------------+  
 *  |     destination service number     |        higher logged time       |  
 *  +------------------------------------+---------------------------------+  
 */

typedef struct
{
	unsigned long	srcNodeNbr;
	unsigned long	srcServiceNbr;
	unsigned long	dstNodeNbr;
	unsigned long	dstServiceNbr;
	BpTimestamp	latestTimeLogged;
} stream;


/*   
 *  Each entry in the bssList has the following format:        	     
 *  +----------------------------+-------------------------+   
 *  | destination service number | destination node number |   
 *  +----------------------------+-------------------------+   
 */

typedef struct
{
	unsigned long	dstServiceNbr;
	unsigned long	dstNodeNbr;
} bssEntry;

typedef struct
{
	unsigned long	srcServiceNbr;
	unsigned long	srcNodeNbr;
	FwdDirective	directive;	/*	    default directive		*/
	FwdDirective	rtDirective;	/*	real-time mode directive	*/
	FwdDirective	pbDirective;	/*	playback mode directive		*/
} BssRule;

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
	FwdDirective	rtDirective;	
	FwdDirective	pbDirective;
	unsigned long	expectedRTT;
	Object		rules;			/*	SDR list	*/
} BssPlan;

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
	Object		bssList;  /* SDR list  - A catalogue that contains    *
				     the service numbers (or optionally pairs *
				     of node-service numbers) that BSS runs   * 
				     on. This list is mainly used for the     *
				     identification of bundles that belong    *
				     to BSS traffic.			      */
} BssDB;

typedef struct
{
	Object		outductElt;
	char		*destDuctName;
} DuctExpression;

extern int		ipnInit();
extern Object		getBssDbObject();
extern BssDB		*getBssConstants();

extern 	Object 		locateBssEntry(CbheEid dst, Object *nextEntry);
	
extern 	int 		bss_addBssEntry(long serviceNbr, long nodeNbr);	 		
extern 	int 		bss_removeBssEntry(long serviceNbr, long nodeNbr);

extern	void 		bss_monitorStream(Lyst loggedStreams, 
				Bundle bundle);

extern void		bss_findPlan(unsigned long nodeNbr, 
				Object *planAddr,
				Object *elt);

extern int		bss_addPlan(unsigned long nodeNbr,	
				DuctExpression *ductExpression,
				DuctExpression *udpExpression,
				DuctExpression *tcpExpression,
				unsigned long estimatedRTT);
extern int		bss_updatePlan(unsigned long nodeNbr,	
				DuctExpression *ductExpression,
				DuctExpression *udpExpression,
				DuctExpression *tcpExpression,
				int estimatedRTT);
extern int		bss_removePlan(unsigned long nodeNbr);

extern void		bss_findPlanRule(unsigned long nodeNbr,
				long sourceServiceNbr,
				long sourceNodeNbr, BssPlan *plan,
				Object *ruleAddr, Object *elt);

extern int		bss_addPlanRule(unsigned long nodeNbr,	
				long sourceServiceNbr,
				long sourceNodeNbr,
				DuctExpression *ductExpression,
				DuctExpression *udpExpression,
				DuctExpression *tcpExpression);
extern int		bss_updatePlanRule(unsigned long nodeNbr,
				long sourceServiceNbr,
				long sourceNodeNbr,
				DuctExpression *ductExpression,
				DuctExpression *udpExpression,
				DuctExpression *tcpExpression);
extern int		bss_removePlanRule(unsigned long nodeNbr,
				long sourceServiceNbr,
				long sourceNodeNbr);

extern int		bss_copyDirective(Bundle *bundle, 
				FwdDirective *directive,
				FwdDirective *defaultDirective,
		        	FwdDirective *rtDirective,
			  	FwdDirective *pbDirective,
			   	Lyst loggedStreams);
	
extern int		bss_lookupPlanDirective(unsigned long nodeNbr,
				unsigned long sourceServiceNbr,
				unsigned long sourceNodeNbr,
				Bundle *bundle,	FwdDirective *directive, 
				Lyst loggedStreams);

extern int 		bss_setCtDueTimer(Bundle bundle, 
				Object bundleAddr);

extern void		bss_findGroup(unsigned long firstNodeNbr,
				unsigned long lastNodeNbr,
				Object *groupAddr, Object *elt);

extern int		bss_addGroup(unsigned long firstNodeNbr,
				unsigned long lastNodeNbr, char *viaEid);
extern int		bss_updateGroup(unsigned long firstNodeNbr,
				unsigned long lastNodeNbr, char *viaEid);
extern int		bss_removeGroup(unsigned long firstNodeNbr,
				unsigned long lastNodeNbr);

extern void		bss_findGroupRule(unsigned long firstNodeNbr,
				unsigned long lastNodeNbr,
				long sourceServiceNbr,
				long sourceNodeNbr, IpnGroup *group,
				Object *ruleAddr, Object *elt);

extern int		bss_addGroupRule(unsigned long firstNodeNbr,
				unsigned long lastNodeNbr,
				long sourceServiceNbr,
				long sourceNodeNbr, char *viaEid);
extern int		bss_updateGroupRule(unsigned long firstNodeNbr,
				unsigned long lastNodeNbr,
				long sourceServiceNbr,
				long sourceNodeNbr, char *viaEid);
extern int		bss_removeGroupRule(unsigned long firstNodeNbr,
				unsigned long lastNodeNbr,
				long sourceServiceNbr,
				long sourceNodeNbr);

extern int		bss_lookupGroupDirective(unsigned long nodeNbr,
				unsigned long sourceServiceNbr,
				unsigned long sourceNodeNbr,
				Bundle *bundle,
				FwdDirective *directive,
				Lyst loggedStreams);

#ifdef __cplusplus
}
#endif

#endif  /* _BSSFW_H_ */
