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

void	dtn2_findPlan(char *nodeName, Object *planAddr, Object *eltp)
{
	Sdr		sdr = getIonsdr();
	char		eid[MAX_EID_LEN + 1];
	VPlan		*vplan;
	PsmAddress	vplanElt;

	/*	This function finds the BpPlan for the specified
	 *	node, if any.						*/

	CHKVOID(ionLocked());
	CHKVOID(nodeName && planAddr && eltp);
	*eltp = 0;			/*	Default.		*/
	isprintf(eid, sizeof eid, "dtn://%s/", nodeName);
	findPlan(eid, &vplan, &vplanElt);
	if (vplanElt == 0)
	{
		return;
	}

	*planAddr = sdr_list_data(sdr, vplan->planElt);
	*eltp = vplan->planElt;
}

int	dtn2_addPlan(char *nodeName, unsigned int nominalRate)
{
	int	result;
	char	eid[MAX_EID_LEN + 1];

	CHKERR(nodeName);
	isprintf(eid, sizeof eid, "dtn://%s/", nodeName);
	result = addPlan(eid, nominalRate);
	if (result == 1)
	{
		result = bpStartPlan(eid);
	}

	return result;
}

int	dtn2_addPlanDuct(char *nodeName, char *ductExpression)
{
	char		eid[MAX_EID_LEN + 1];
	char		*cursor;
	VOutduct	*vduct;
	PsmAddress	vductElt;

	CHKERR(nodeName);
	CHKERR(ductExpression);
	isprintf(eid, sizeof eid, "dtn://%s/", nodeName);
	cursor = strchr(ductExpression, '/');
	if (cursor == NULL)
	{
		writeMemoNote("[?] Duct expression lacks duct name",
				ductExpression);
		writeMemoNote("[?]   Attaching duct to plan", eid);
		return -1;
	}

	*cursor = '\0';
	findOutduct(ductExpression, cursor + 1, &vduct, &vductElt);
	*cursor = '/';
	if (vductElt == 0)
	{
		writeMemoNote("[?] Unknown duct", ductExpression);
		writeMemoNote("[?]   Attaching duct to plan", eid);
		return -1;
	}

	return attachPlanDuct(eid, vduct->outductElt);
}

int	dtn2_updatePlan(char *nodeName, unsigned int nominalRate)
{
	char	eid[MAX_EID_LEN + 1];

	CHKERR(nodeName);
	isprintf(eid, sizeof eid, "dtn://%s/", nodeName);
	return updatePlan(eid, nominalRate);
}

int	dtn2_setPlanViaEid(char *nodeName, char *viaEid)
{
	char	eid[MAX_EID_LEN + 1];

	CHKERR(nodeName);
	CHKERR(viaEid);
	isprintf(eid, sizeof eid, "dtn://%s/", nodeName);
	return setPlanViaEid(eid, viaEid);
}

int	dtn2_removePlanDuct(char *nodeName, char *ductExpression)
{
	char		eid[MAX_EID_LEN + 1];
	char		*cursor;
	VOutduct	*vduct;
	PsmAddress	vductElt;

	CHKERR(nodeName);
	CHKERR(ductExpression);
	isprintf(eid, sizeof eid, "dtn://%s/", nodeName);
	cursor = strchr(ductExpression, '/');
	if (cursor == NULL)
	{
		writeMemoNote("[?] Duct expression lacks duct name",
				ductExpression);
		writeMemoNote("[?] (Detaching duct from plan", eid);
		return -1;
	}

	*cursor = '\0';
	findOutduct(ductExpression, cursor + 1, &vduct, &vductElt);
	*cursor = '/';
	if (vductElt == 0)
	{
		writeMemoNote("[?] Unknown duct", ductExpression);
		writeMemoNote("[?] (Detaching duct from plan", eid);
		return -1;
	}

	return detachPlanDuct(vduct->outductElt);
}

int	dtn2_removePlan(char *nodeName)
{
	char	eid[MAX_EID_LEN + 1];

	isprintf(eid, sizeof eid, "dtn://%s/", nodeName);
	return removePlan(eid);
}
