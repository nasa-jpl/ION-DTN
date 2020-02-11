/*
	ipnadminep.c:	Administrative endpoint application process
			for "ipn" scheme.

	Author: Scott Burleigh, JPL

	Copyright (c) 2006, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
									*/
#include "ipnfw.h"

static int	handleStatusRpt(BpDelivery *dlv, unsigned char *cursor,
			unsigned int unparsedBytes)
{
	BpStatusRpt	rpt;
	char		*sourceEid;
	char		memobuf[1024];
	unsigned int	statusTime = 0;
	char		*reasonString;

	if (parseStatusRpt(&rpt, cursor, unparsedBytes) < 1)
	{
		return 0;
	}

	if (readEid(&rpt.sourceEid, &sourceEid) < 0)
	{
		eraseEid(&rpt.sourceEid);
		return -1;
	}

	if (rpt.flags & BP_DELETED_RPT)
	{
		statusTime = rpt.deletionTime;
		switch (rpt.reasonCode)
		{
		case SrLifetimeExpired:
			reasonString = "TTL expired";
			break;

		case SrUnidirectionalLink:
			reasonString = "one-way link";
			break;

		case SrCanceled:
			reasonString = "canceled";
			break;

		case SrDepletedStorage:
			reasonString = "out of space";
			break;

		case SrDestinationUnintelligible:
			reasonString = "bad destination";
			break;

		case SrNoKnownRoute:
			reasonString = "no route to destination";
			break;

		case SrNoTimelyContact:
			reasonString = "would expire before contact";
			break;

		case SrBlockUnintelligible:
			reasonString = "bad block";
			break;

		default:
			reasonString = "(unknown)";
		}
	}
	else
	{
		reasonString = "okay";
		if (rpt.flags & BP_RECEIVED_RPT)
		{
			statusTime = rpt.receiptTime;
		}

		if (rpt.flags & BP_FORWARDED_RPT)
		{
			statusTime = rpt.forwardTime;
		}

		if (rpt.flags & BP_DELIVERED_RPT)
		{
			statusTime = rpt.deliveryTime;
		}
	}

	isprintf(memobuf, sizeof memobuf, "[s] (%s)/%u:%u/%u status %d at \
%lu on %s, '%s'.", sourceEid, rpt.creationTime.seconds,
		rpt.creationTime.count, rpt.fragmentOffset, rpt.flags,
		statusTime, dlv->bundleSourceEid, reasonString);
	writeMemo(memobuf);
	MRELEASE(sourceEid);
	eraseEid(&rpt.sourceEid);
	return 0;
}

#if defined (ION_LWT)
int	ipnadminep(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
#else
int	main(int argc, char *argv[])
{
#endif
	VScheme		*vscheme;
	PsmAddress	vschemeElt;

	if (bpAttach() < 0)
	{
		putErrmsg("ipnadminep can't attach to BP.", NULL);
		return 1;
	}

	if (ipnInit() < 0)
	{
		putErrmsg("ipnadminep can't load routing database.", NULL);
		return 1;
	}

	findScheme("ipn", &vscheme, &vschemeElt);
	if (vschemeElt == 0)
	{
		putErrmsg("ipnadminep can't get admin EID.", NULL);
		return 1;
	}

	writeMemo("[i] ipnadminep is running.");
	if (_handleAdminBundles(vscheme->adminEid, handleStatusRpt) < 0)
	{
		putErrmsg("ipnadminep crashed.", NULL);
	}

	writeErrmsgMemos();
	writeMemo("[i] ipnadminep has ended.");
	ionDetach();
	return 0;
}
