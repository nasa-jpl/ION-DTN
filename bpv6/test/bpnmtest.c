/*
	bpnmtest.c:	Testing the NM API.
									*/
/*									*/
/*	Copyright (c) 2013, California Institute of Technology.		*/
/*	All rights reserved.						*/
/*	Author: Scott Burleigh, Jet Propulsion Laboratory		*/
/*									*/

#include <bp.h>
#include <bpnm.h>

#if defined (ION_LWT)
int	bpnmtest(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
#else
int	main(int argc, char **argv)
{
#endif
	NmbpNode	nodeBuf;
	NmbpDisposition	dispBuf;

#ifndef mingw
	setlinebuf(stdout);
#endif
	if (bp_attach() < 0)
	{
		putErrmsg("Can't attach to BP.", NULL);
		return 0;
	}

	bpnm_node_get(&nodeBuf);
	printf("nodeId:				'%s'\n",
			nodeBuf.nodeID);
	printf("bpVersionNbr:			'%s'\n",
			nodeBuf.bpVersionNbr);
	printf("avblStorage:			" UVAST_FIELDSPEC "\n",
			nodeBuf.avblStorage);
	printf("lastRestartTime:		%d\n",
			(int) nodeBuf.lastRestartTime);
	printf("nbrOfRegistrations:		%d\n",
			nodeBuf.nbrOfRegistrations);
	bpnm_disposition_get(&dispBuf);
	printf("currentForwardPending:		" UVAST_FIELDSPEC "\n",
			dispBuf.currentForwardPending);
	printf("currentDispatchPending:		" UVAST_FIELDSPEC "\n",
			dispBuf.currentDispatchPending);
	printf("currentInCustody:		" UVAST_FIELDSPEC "\n",
			dispBuf.currentInCustody);
	printf("currentReassemblyPending:	" UVAST_FIELDSPEC "\n",
			dispBuf.currentReassemblyPending);
	printf("bundleSourceCount[0]:		" UVAST_FIELDSPEC "\n",
			dispBuf.bundleSourceCount[0]);
	printf("bundleSourceCount[1]:		" UVAST_FIELDSPEC "\n",
			dispBuf.bundleSourceCount[1]);
	printf("bundleSourceCount[2]:		" UVAST_FIELDSPEC "\n",
			dispBuf.bundleSourceCount[2]);
	printf("bundleSourceBytes[0]:		" UVAST_FIELDSPEC "\n",
			dispBuf.bundleSourceBytes[0]);
	printf("bundleSourceBytes[1]:		" UVAST_FIELDSPEC "\n",
			dispBuf.bundleSourceBytes[1]);
	printf("bundleSourceBytes[2]:		" UVAST_FIELDSPEC "\n",
			dispBuf.bundleSourceBytes[2]);
	printf("currentResidentCount[0]:	" UVAST_FIELDSPEC "\n",
			dispBuf.currentResidentCount[0]);
	printf("currentResidentCount[1]:	" UVAST_FIELDSPEC "\n",
			dispBuf.currentResidentCount[1]);
	printf("currentResidentCount[2]:	" UVAST_FIELDSPEC "\n",
			dispBuf.currentResidentCount[2]);
	printf("currentResidentBytes[0]:	" UVAST_FIELDSPEC "\n",
			dispBuf.currentResidentBytes[0]);
	printf("currentResidentBytes[1]:	" UVAST_FIELDSPEC "\n",
			dispBuf.currentResidentBytes[1]);
	printf("currentResidentBytes[2]:	" UVAST_FIELDSPEC "\n",
			dispBuf.currentResidentBytes[2]);
	printf("bundlesFragmented:		" UVAST_FIELDSPEC "\n",
			dispBuf.bundlesFragmented);
	printf("fragmentsProduced:		" UVAST_FIELDSPEC "\n",
			dispBuf.fragmentsProduced);
	printf("delNoneCount:			" UVAST_FIELDSPEC "\n",
			dispBuf.delNoneCount);
	printf("delExpiredCount:		" UVAST_FIELDSPEC "\n",
			dispBuf.delExpiredCount);
	printf("delFwdUnidirCount:		" UVAST_FIELDSPEC "\n",
			dispBuf.delFwdUnidirCount);
	printf("delCanceledCount:		" UVAST_FIELDSPEC "\n",
			dispBuf.delCanceledCount);
	printf("delDepletionCount:		" UVAST_FIELDSPEC "\n",
			dispBuf.delDepletionCount);
	printf("delEidMalformedCount:		" UVAST_FIELDSPEC "\n",
			dispBuf.delEidMalformedCount);
	printf("delNoRouteCount:		" UVAST_FIELDSPEC "\n",
			dispBuf.delNoRouteCount);
	printf("delNoContactCount:		" UVAST_FIELDSPEC "\n",
			dispBuf.delNoContactCount);
	printf("delBlkMalformedCount:		" UVAST_FIELDSPEC "\n",
			dispBuf.delBlkMalformedCount);
	printf("bytesDeletedToDate:		" UVAST_FIELDSPEC "\n",
			dispBuf.bytesDeletedToDate);
	printf("custodyRefusedCount:		" UVAST_FIELDSPEC "\n",
			dispBuf.custodyRefusedCount);
	printf("custodyRefusedBytes:		" UVAST_FIELDSPEC "\n",
			dispBuf.custodyRefusedBytes);
	printf("bundleFwdFailedCount:		" UVAST_FIELDSPEC "\n",
			dispBuf.bundleFwdFailedCount);
	printf("bundleFwdFailedBytes:		" UVAST_FIELDSPEC "\n",
			dispBuf.bundleFwdFailedBytes);
	printf("bundleAbandonCount:		" UVAST_FIELDSPEC "\n",
			dispBuf.bundleAbandonCount);
	printf("bundleAbandonBytes:		" UVAST_FIELDSPEC "\n",
			dispBuf.bundleAbandonBytes);
	printf("bundleDiscardCount:		" UVAST_FIELDSPEC "\n",
			dispBuf.bundleDiscardCount);
	printf("bundleDiscardBytes:		" UVAST_FIELDSPEC "\n",
			dispBuf.bundleDiscardBytes);
	writeErrmsgMemos();
	PUTS("Stopping bpnmtest.");
	bp_detach();
	return 0;
}
