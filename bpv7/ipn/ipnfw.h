/*
	ipnfw.h:	definitions supporting the implementation
			of the forwarding infrastructure for the "ipn"
			endpoint ID scheme as originally developed
			to support the Compressed Bundle Header
			Encoding conventions.

			Prior to RFC 9758 the scheme-specific part
			of an ipn-scheme endpoint ID was of the form

				"node_number.service_number"

			where node_number was constrained to be a
			64-bit unsigned integer.  RFC 9758 revised
			the scheme-specific part of the ipn scheme:
			the revised SSP is of the form

				"[allocator_number.]node_number.service_number"

			where both allocator_number and node_number
			are constrained to be 32-bit unsigned integers.
			The concatenation of the allocator_number and
			the node_number constitutes a 64-bit unsigned
			integer which is termed a "fully-qualified
			node number" or FQNN.  ION has been modified
			to support the external representations (both
			in text and on-the-wire) of ipn-scheme EIDs
			in this new form, but internally ION continues
			to represent every ipn-scheme EID as a tuple
			comprising its FQNN (formerly called "node
			number" in all ION code) and service number.

	Author: Scott Burleigh, JPL

	Modification History:
	Date      Who   What

	Copyright (c) 2005, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
									*/

#ifndef IPNFW_H
#define IPNFW_H

#include "bpP.h"
#include "cgr.h"

/* Include the public admin API */
#include "bp_admin.h"

#ifdef __cplusplus
extern "C" {
#endif

/*	Bundles for which the forwarder lacks forwarding knowledge
 *	may be forwarded to another forwarder that is expected to
 *	have the requisite knowledge; this "static routing" mechanism
 *	enables ipn-scheme routing to scale up to relatively large
 *	numbers of nodes.  For this purpose, one or more "exits"
 *	-- each exit identified by a range of node numbers for which
 *	the designated "via" node takes forwarding responsibility --
 *	may be defined.
 *
 *	Exits may nest, but otherwise the node ranges of exits may
 *	never overlap.  Any bundle with an ipn-scheme destination
 *	for which the ipn forwarder cannot otherwise compute a
 *	route will be forwarded to the designated "via" node cited
 *	by the smallest exit between whose first and last node
 *	numbers (inclusive) the destination node's number lies, if any.
 *
 *	Note that the First and Last node numbers specified for an
 *	exit may be the same number, identifying a static route to a
 *	specified node.							*/

typedef struct
{
	uvast		firstFqnn;		/*	in range	*/
	uvast		lastFqnn;		/*	in range	*/
	SdrObject	eid;			/*	Send via.	*/
} IpnExit;

/*	Overrides linked to data labels as provided in ECOS extension
 *	blocks may be used to override normal bundle prioritization
 *	and/or routing.							*/

typedef struct
{
	/*	dataLabel = -1 indicates "all others"			*/
	unsigned int	dataLabel;

	/*	destFqnn = -1 indicates "all others"			*/
	uvast		destFqnn;

	/*	sourceFqnn = -1 indicates "all others"		*/
	uvast		sourceFqnn;

	/*	Routing override stuff.					*/

	/*	neighbor = -1 indicates "no routing override"		*/
	uvast		neighborFqnn;
	SdrObject	ductExpression; /* sdrstring */

	/*	QoS override stuff.					*/

	/*	priority = -1 indicates "no QoS override"		*/
	unsigned char	priority;		/*	CoS.		*/
	unsigned char	ordinal;		/*	For priority 2.	*/
	unsigned char	qosFlags;
} IpnOverride;

typedef struct
{
	SdrObject exits;     /* SDR list */
	SdrObject overrides; /* SDR list */
} IpnDB;

extern int		ipnInit(void);
extern SdrObject	getIpnDbObject(void);
extern IpnDB		*getIpnConstants(void);

extern int		ipn_setOvrd(unsigned int dataLabel,
				uvast destFqnn,
				uvast sourceFqnn,
			/*	neighbor = -2 indicates
			 		"no change from current value"	*/
				uvast neighborFqnn,
				char *ovrdDuctExpression,
			/*	priority = -2 indicates
			 		"no change from current value"	*/
				unsigned char priority,
				unsigned char ordinal,
				unsigned char qosFlags);

extern int		ipn_lookupOvrd(unsigned int dataLabel,
				uvast destFqnn,
				uvast sourceFqnn,
				SdrObject *ovrdAddr);

extern void		ipn_findExit(uvast firstFqnn,
				uvast lastFqnn,
				SdrObject *exitAddr, SdrObject *elt);

extern int		ipn_addExit(uvast firstFqnn,
				uvast lastFqnn, char *viaEid);
extern int		ipn_updateExit(uvast firstFqnn,
				uvast lastFqnn, char *viaEid);
extern int		ipn_removeExit(uvast firstFqnn,
				uvast lastFqnn);

extern int		ipn_lookupExit(uvast fqnn, char *eid);

/*	Egress plans are actually non-scheme-specific and are defined
 *	in BP.  But convenience functions for the ipn scheme are
 *	provided here.							*/

extern void ipn_findPlan(uvast fqnn, SdrObject *planAddr, SdrObject *elt);

extern int		ipn_addPlan(uvast fqnn, unsigned int nominalRate);
extern int		ipn_addPlanDuct(uvast fqnn, char *ductExpression);
extern int		ipn_updatePlan(uvast fqnn, unsigned int nominalRate);
extern int		ipn_removePlanDuct(uvast fqnn, char *ductExpression);
extern int		ipn_removePlan(uvast fqnn);
#ifdef __cplusplus
}
#endif

#endif /* IPNFW_H */
