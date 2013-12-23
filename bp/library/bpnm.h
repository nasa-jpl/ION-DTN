/*
 	bpnm.h:	definitions supporting the BP instrumentation API.

	Copyright (c) 2011, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.

	Author: Larry Shackleford, GSFC
	Modified by Scott Burleigh per changes in MIB definition
 									*/
#ifndef _BPNM_H_
#define _BPNM_H_

#ifdef __cplusplus
extern "C" {
#endif

#define BPNM_ENDPOINT_NAME_FMT_STRING           ("%s:%s")
#define BPNM_ENDPOINT_FILLPATT                  (0xE1)
#define BPNM_ENDPOINT_EIDSTRING_LEN             (32)


/*****************************************/

typedef struct
{
	char		nodeID[256];
	char		bpVersionNbr[16];
	uvast		avblStorage;
	time_t		lastRestartTime;
	unsigned int	nbrOfRegistrations;
} NmbpNode;

typedef struct
{
	uvast		currentForwardPending;
	uvast		currentDispatchPending;
	uvast		currentInCustody;
	uvast		currentReassemblyPending;

	uvast		bundleSourceCount[3];
	uvast		bundleSourceBytes[3];
	uvast		currentResidentCount[3];
	uvast		currentResidentBytes[3];

	uvast		bundlesFragmented;
	uvast		fragmentsProduced;

	uvast		delNoneCount;
	uvast		delExpiredCount;
	uvast		delFwdUnidirCount;
	uvast		delCanceledCount;
	uvast		delDepletionCount;
	uvast		delEidMalformedCount;
	uvast		delNoRouteCount;
	uvast		delNoContactCount;
	uvast		delBlkMalformedCount;

	uvast		bytesDeletedToDate;

	uvast		custodyRefusedCount;
	uvast		custodyRefusedBytes;
	uvast		bundleFwdFailedCount;
	uvast		bundleFwdFailedBytes;
	uvast		bundleAbandonCount;
	uvast		bundleAbandonBytes;
	uvast		bundleDiscardCount;
	uvast		bundleDiscardBytes;
} NmbpDisposition;

typedef struct
{
	char		eid[BPNM_ENDPOINT_EIDSTRING_LEN];
	int		active;			/*	Boolean		*/
	int		singleton;		/*	Boolean		*/
	int		abandonOnDelivFailure;	/*	Boolean		*/
} NmbpEndpoint;

extern void	bpnm_node_get(NmbpNode * buffer);

extern void	bpnm_extensions_get(char * nameBuffer, int bufLen,
			char * nameArray [], int * numStrings);

extern void	bpnm_disposition_get(NmbpDisposition * buffer);
extern void	bpnm_disposition_reset();

extern void	bpnm_endpointNames_get(char * nameBuffer, int bufLen,
			char * nameArray [], int * numStrings);
extern void	bpnm_endpoint_get(char * name, NmbpEndpoint * buffer,
			int * success);
#ifdef __cplusplus
}
#endif

#endif  /* _BPNM_H_ */
