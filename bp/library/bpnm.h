/*
 	bpnm.h:	definitions supporting the BP instrumentation API.

	Copyright (c) 2011, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.

	Author: Larry Shackleford, GSFC
 									*/
#ifndef _BPNM_H_
#define _BPNM_H_

#ifdef __cplusplus
extern "C" {
#endif

#define BPNM_ENDPOINT_NAME_FMT_STRING           ("%s:%s")
#define BPNM_INDUCT_NAME_FMT_STRING             ("%s/%s")
#define BPNM_OUTDUCT_NAME_FMT_STRING            ("%s/%s")

#define BPNM_ENDPOINT_FILLPATT                  (0xE1)
#define BPNM_INDUCT_FILLPATT                    (0xE2)
#define BPNM_OUTDUCT_FILLPATT                   (0xE3)

#define BPNM_ENDPOINT_EIDSTRING_LEN             (32)
#define BPNM_INDUCT_NAME_LEN                    (32)
#define BPNM_OUTDUCT_NAME_LEN                   (32)


/*****************************************/
typedef struct
{
    char            eid[BPNM_ENDPOINT_EIDSTRING_LEN];

    unsigned long   currentQueuedBundlesCount;
    unsigned long   currentQueuedBundlesBytes;

    time_t          lastResetTime;

    unsigned long   bundleEnqueuedCount;
    unsigned long   bundleEnqueuedBytes;
    unsigned long   bundleDequeuedCount;
    unsigned long   bundleDequeuedBytes;
} NmbpEndpoint;

typedef struct
{
    char            inductName[BPNM_INDUCT_NAME_LEN];

    time_t          lastResetTime;

    unsigned long   bundleRecvCount;
    unsigned long   bundleRecvBytes;
    unsigned long   bundleMalformedCount;
    unsigned long   bundleMalformedBytes;
    unsigned long   bundleInauthenticCount;
    unsigned long   bundleInauthenticBytes;
    unsigned long   bundleOverflowCount;
    unsigned long   bundleOverflowBytes;
} NmbpInduct;

typedef struct
{
    char            outductName[BPNM_OUTDUCT_NAME_LEN];

    unsigned long   currentQueuedBundlesCount;
    unsigned long   currentQueuedBundlesBytes;

    time_t          lastResetTime;

    unsigned long   bundleEnqueuedCount;
    unsigned long   bundleEnqueuedBytes;
    unsigned long   bundleDequeuedCount;
    unsigned long   bundleDequeuedBytes;
} NmbpOutduct;

typedef struct
{
    unsigned long   currentResidentCount[3];
    unsigned long   currentResidentBytes[3];
    unsigned long   currentInLimbo;
    unsigned long   currentDispatchPending;
    unsigned long   currentForwardPending;
    unsigned long   currentReassemblyPending;
    unsigned long   currentInCustody;
    unsigned long   currentNotInCustody;

    time_t          lastResetTime;

    unsigned long   bundleSourceCount[3];
    unsigned long   bundleSourceBytes[3];
    unsigned long   bundleRecvCount[3];
    unsigned long   bundleRecvBytes[3];
    unsigned long   bundleDiscardCount[3];
    unsigned long   bundleDiscardBytes[3];
    unsigned long   bundleXmitCount[3];
    unsigned long   bundleXmitBytes[3];

    unsigned long   rptReceiveCount;
    unsigned long   rptAcceptCount;
    unsigned long   rptForwardCount;
    unsigned long   rptDeliverCount;
    unsigned long   rptDeleteCount;

    unsigned long   rptNoneCount;
    unsigned long   rptExpiredCount;
    unsigned long   rptFwdUnidirCount;
    unsigned long   rptCanceledCount;
    unsigned long   rptDepletionCount;
    unsigned long   rptEidMalformedCount;
    unsigned long   rptNoRouteCount;
    unsigned long   rptNoContactCount;
    unsigned long   rptBlkMalformedCount;

    unsigned long   custodyAcceptCount;
    unsigned long   custodyAcceptBytes;
    unsigned long   custodyReleasedCount;
    unsigned long   custodyReleasedBytes;
    unsigned long   custodyExpiredCount;
    unsigned long   custodyExpiredBytes;
    unsigned long   custodyRedundantCount;
    unsigned long   custodyRedundantBytes;
    unsigned long   custodyDepletionCount;
    unsigned long   custodyDepletionBytes;
    unsigned long   custodyEidMalformedCount;
    unsigned long   custodyEidMalformedBytes;
    unsigned long   custodyNoRouteCount;
    unsigned long   custodyNoRouteBytes;
    unsigned long   custodyNoContactCount;
    unsigned long   custodyNoContactBytes;
    unsigned long   custodyBlkMalformedCount;
    unsigned long   custodyBlkMalformedBytes;
   
    unsigned long   bundleQueuedForFwdCount;
    unsigned long   bundleQueuedForFwdBytes;
    unsigned long   bundleFwdOkayCount;
    unsigned long   bundleFwdOkayBytes;
    unsigned long   bundleFwdFailedCount;
    unsigned long   bundleFwdFailedBytes;
    unsigned long   bundleRequeuedForFwdCount;
    unsigned long   bundleRequeuedForFwdBytes;
    unsigned long   bundleExpiredCount;
    unsigned long   bundleExpiredBytes;
} NmbpDisposition;

extern void	bpnm_resources(double * occupancyCeiling, 
			double * maxForecastOccupancy, 
			double * currentOccupancy,
			double * maxHeapOccupancy,
			double * heapOccupancy,
			double * maxFileOccupancy,
			double * fileOccupancy);

extern void	bpnm_endpointNames_get(char * nameBuffer, char * nameArray [],
			int * numStrings);
extern void	bpnm_endpoint_get(char * name, NmbpEndpoint * buffer,
			int * success);
extern void	bpnm_endpoint_reset(char * name, int * success);

extern void	bpnm_inductNames_get(char * nameBuffer, char * nameArray [],
			int * numStrings);
extern void	bpnm_induct_get(char * name, NmbpInduct * buffer,
			int * success);
extern void	bpnm_induct_reset(char * name, int * success);

extern void	bpnm_outductNames_get(char * nameBuffer, char * nameArray [],
			int * numStrings);
extern void	bpnm_outduct_get(char * name, NmbpOutduct * buffer,
			int * success);
extern void	bpnm_outduct_reset(char * name, int * success);

extern void	bpnm_limbo_get(NmbpOutduct * state);

extern void	bpnm_disposition_get(NmbpDisposition * state);
extern void	bpnm_disposition_reset();

#ifdef __cplusplus
}
#endif

#endif  /* _BPNM_H_ */
