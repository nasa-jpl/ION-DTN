/*
 *	libdtn2fw.c:	functions enabling the implementation of
 *			a regional forwarder for the DTN endpoint
 *			ID scheme.
 *
 *	Copyright (c) 2006, California Institute of Technology.
 *	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
 *	acknowledged.
 *
 *	Author: Scott Burleigh, JPL
 *
 *	Modification History:
 *	Date	  Who	What
 *	02-06-06  SCB	Original development.
 */

#include "dtn2fw.h"

/*	*	*	Routing information mgt functions	*	*/

int	dtn2Init()
{
	return 0;	/*	No additional database needed.		*/
}

void	dtn2_findPlan(char *eid, Object *planAddr, Object *eltp)
{
	Sdr		sdr = getIonsdr();
	VPlan		*vplan;
	PsmAddress	vplanElt;

	/*	This function finds the BpPlan for the specified
	 *	node, if any.						*/

	CHKVOID(ionLocked());
	CHKVOID(eid && planAddr && eltp);
	*eltp = 0;
	findPlan(eid, &vplan, &vplanElt);
	if (vplanElt == 0)
	{
		return;
	}

	*planAddr = sdr_list_data(sdr, vplan->planElt);
	*eltp = vplan->planElt;
}

int	dtn2_addPlan(char *eid, unsigned int nominalRate)
{
	if (nominalRate == 0)
	{
		nominalRate = 125000000;	/*	Default 1Gbps.	*/
	}

	return addPlan(eid, nominalRate);
}

int	dtn2_addPlanDuct(char *eid, char *spec)
{
	return addPlanDuct(eid, spec);
}

int	dtn2_updatePlan(char *eid, unsigned int nominalRate)
{
	if (nominalRate == 0)
	{
		nominalRate = 125000000;	/*	Default 1Gbps.	*/
	}

	return updatePlan(eid, nominalRate);
}

int	dtn2_setPlanViaEid(char *eid, char *viaEid)
{
	return setPlanViaEid(eid, viaEid);
}

int	dtn2_removePlanDuct(char *eid, char *spec)
{
	return removePlanDuct(eid, spec);
}

int	dtn2_removePlan(char *eid)
{
	return removePlan(eid);
}

void	dtn2_lookupPlan(char *eid, VPlan **vplan)
{
	lookupPlan(eid, vplan);
}
