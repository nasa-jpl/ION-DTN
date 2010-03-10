/*
 	dtn2fw.h:	definitions supporting the implementation
			of the forwarding infrastructure for
			DTN2-style endpoint IDs.

	Author: Scott Burleigh, JPL

	Modification History:
	Date      Who   What

	Copyright (c) 2006, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
 									*/
#ifndef _DTN2FW_H_
#define _DTN2FW_H_

#include "bpP.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
	Object		demux;			/*	SDR string	*/
	FwdDirective	directive;
} Dtn2Rule;

/*	DTN2 forwarding rules are managed in the non-volatile linked
 *	list of Dtn2Plan objects that associate egress directives
 *	with node name patterns.  Each plan may have an associated
 *	list of Dtn2Rules indicating the egress directives for bundles
 *	that are characterized by a specific demux token.  Each plan
 *	also has a default directive, which applies to all bundles
 *	for which no demux-specific Dtn2Rules apply.			*/

typedef struct
{
	Object		nodeName;		/*	SDR string	*/
	FwdDirective	defaultDirective;
	Object		rules;			/*	(Dtn2Rule *)	*/
} Dtn2Plan;

typedef struct
{
	Object		plans;
} DtnDB;

extern int		dtn2Init();
extern Object		getDtnDbObject();
extern DtnDB		*getDtnConstants();

extern void		dtn2_destroyDirective(FwdDirective *directive);

extern int		dtn2_lookupDirective(char *nodeName, char *demux,
				FwdDirective *directive);

extern void		dtn2_findPlan(char *nodeName, Object *planAddr,
				Object *elt);
extern int		dtn2_addPlan(char *nodeName,
				FwdDirective *defaultDirective);
extern int		dtn2_updatePlan(char *nodeName,
				FwdDirective *defaultDirective);
extern int		dtn2_removePlan(char *nodeName);

extern void		dtn2_findRule(char *nodeName, char *demux,
				Dtn2Plan *plan, Object *ruleAddr, Object *elt);
extern int		dtn2_addRule(char *nodeName, char *demux,
				FwdDirective *directive);
extern int		dtn2_updateRule(char *nodeName, char *demux,
				FwdDirective *directive);
extern int		dtn2_removeRule(char *nodeName, char *demux);

#ifdef __cplusplus
}
#endif

#endif  /* _DTN2FW_H_ */
