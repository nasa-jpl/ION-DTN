/*
	dtpc.h:	Definitions supporting applications built on the
		implementation of the Delay Tolerant Payload Conditioning
		in the ION (Interplanetary Overlay Network) stack.

	Authors: Giorgos Papastergiou, SPICE
		 Ioannis Alexiadis, SPICE

	Copyright (c) 2011, Space Internetworking Center,
	Democritus University of Thrace. ALL RIGHTS RESERVED.
										*/

#include "bp.h"


#define DTPC_POLL                 (0)     /*      Return immediately.     */
#define DTPC_NONBLOCKING          (0)     /*      Return immediately.     */
#define DTPC_BLOCKING             (-1)    /*      Wait forever.           */

typedef struct
{
        Object          payload;
        Sdnv            length;
	Object          topicElt;       /* Ref. to Topic - not used     */
} PayloadRecord;


typedef int     (*DtpcElisionFn) (Object recordsList, PayloadRecord *newRecord);

typedef struct dtpcsap_st        *DtpcSAP;

typedef enum
{
        PayloadPresent = 1,
        ReceptionTimedOut,
        ReceptionInterrupted,
        DtpcServiceStopped
} DtpcIndResult;

typedef struct
{
        DtpcIndResult   result;
	char		*srcEid;
	unsigned int	length;
        Object          adu;   	 /* ZCO ref  */

} DtpcDelivery;

/*      *       *       DTPC initilization       *       *       *       */


extern int      dtpc_init();

extern int      dtpc_entity_is_started();


/*      *       *       DTPC local services      *       *       *       */

extern int      dtpc_send(	unsigned int profileID,
				DtpcSAP sap,
				char *dstEid,
				unsigned int maxRtx,
				unsigned int aggrSizeLimit,
				unsigned int aggrTimeLimit,
                                int lifespan,
				BpExtendedCOS *extendedCOS,
				unsigned char srrFlags,
				BpCustodySwitch custodySwitch,
				char *reportToEid,
                                int classOfService,
                                Object adu,
				unsigned int length);

extern void     dtpc_interrupt(DtpcSAP sap);

extern int      dtpc_open (unsigned int topicID, DtpcElisionFn elisionFn, DtpcSAP *dtpcsapPtr);

extern void     dtpc_close(DtpcSAP sap);

extern int      dtpc_receive(DtpcSAP sap, DtpcDelivery *dlv, int timeoutSeconds);

extern void	dtpc_interrupt(DtpcSAP sap);

extern void	dtpc_release_delivery(DtpcDelivery *dlvBuffer);
