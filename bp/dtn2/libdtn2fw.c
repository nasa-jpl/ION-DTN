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
	int	result;

	result = addPlan(eid, nominalRate);
	if (result == 1)
	{
		result = bpStartPlan(eid);
	}

	return result;
}

int	dtn2_addPlanDuct(char *eid, char *spec)
{
	char		*cursor;
	VOutduct	*vduct;
	PsmAddress	vductElt;

	cursor = strchr(spec, '/');
	if (cursor == NULL)
	{
		writeMemoNote("[?] Duct expression lacks duct name",
				spec);
		writeMemoNote("[?]   Attaching duct to plan", eid);
		return -1;
	}

	*cursor = '\0';
	findOutduct(spec, cursor + 1, &vduct, &vductElt);
	*cursor = '/';
	if (vductElt == 0)
	{
		writeMemoNote("[?] Unknown duct", spec);
		writeMemoNote("[?]   Attaching duct to plan", eid);
		return -1;
	}

	return attachPlanDuct(eid, vduct->outductElt);
}

int	dtn2_updatePlan(char *eid, unsigned int nominalRate)
{
	return updatePlan(eid, nominalRate);
}

int	dtn2_setPlanViaEid(char *eid, char *viaEid)
{
	return setPlanViaEid(eid, viaEid);
}

int	dtn2_removePlanDuct(char *eid)
{
	Sdr		sdr = getIonsdr();
	VPlan		*vplan;
	PsmAddress	vplanElt;
			OBJ_POINTER(BpPlan, plan);
	Object		elt;
	Object		outductElt;

	CHKERR(eid);
	findPlan(eid, &vplan, &vplanElt);
	if (vplanElt == 0)
	{
		writeMemoNote("[?] No such plan", eid);
		return -1;
	}

	GET_OBJ_POINTER(sdr, BpPlan, plan, sdr_list_data(sdr, vplan->planElt));
	CHKERR(plan);
	elt = sdr_list_first(sdr, plan->ducts);
	outductElt = sdr_list_data(sdr, elt);
	return detachPlanDuct(outductElt);
}

int	dtn2_removePlan(char *eid)
{
	return removePlan(eid);
}
