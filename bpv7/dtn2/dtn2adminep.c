/*
	dtn2adminep.c:	Administrative endpoint application process
			for "dtn2" scheme.

	Author: Scott Burleigh, JPL

	Copyright (c) 2006, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
									*/
#include "dtn2fw.h"

static int	handleStatusRpt(BpDelivery *dlv, unsigned char *cursor,
			unsigned int unparsedBytes)
{
	BpStatusRpt	rpt;
	char		*sourceEid;
	char		memobuf[1024];

	if (parseStatusRpt(&rpt, cursor, unparsedBytes) < 1)
	{
		return 0;
	}

	if (readEid(&rpt.sourceEid, &sourceEid) < 0)
	{
		eraseEid(&rpt.sourceEid);
		return -1;
	}

	isprintf(memobuf, sizeof memobuf, "[s] bundle (%s), %u:%u, %u \
status is %d", sourceEid, rpt.creationTime.seconds,
		rpt.creationTime.count, rpt.fragmentOffset, rpt.reasonCode);
	writeMemo(memobuf);
	MRELEASE(sourceEid);
	eraseEid(&rpt.sourceEid);
	return 0;
}

#if defined (ION_LWT)
int	dtn2adminep(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
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
		putErrmsg("dtn2adminep can't attach to BP.", NULL);
		return 1;
	}

	findScheme("dtn", &vscheme, &vschemeElt);
	if (vschemeElt == 0)
	{
		putErrmsg("dtn2adminep can't get admin EID.", NULL);
		return 1;
	}

	writeMemo("[i] dtn2adminep is running.");
	if (_handleAdminBundles(vscheme->adminEid, handleStatusRpt) < 0)
	{
		putErrmsg("dtn2adminep crashed.", NULL);
	}

	writeErrmsgMemos();
	writeMemo("[i] dtn2adminep has ended.");
	ionDetach();
	return 0;
}
