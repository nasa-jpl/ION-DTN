/*
	dtpcP.h:	Private definitions supporting the implementation
			of DTPC nodes in the ION (Interplanetary Overlay
			Network) stack.

	Authors: Giorgos Papastergiou, SPICE
		 Ioannis Alexiadis, SPICE

	Copyright (c) 2011, Space Internetworking Center,
	Democritus University of Thrace. ALL RIGHTS RESERVED.
*/

#include "dtpc.h"

#define DTPC_SEND_SVC_NBR	(128)
#define	DTPC_RECV_SVC_NBR	(129)
#define DTPC_MAX_SEQ_NBR	999999999
#define	EPOCH_2000_SEC		946684800
#define BUFMAXSIZE		(65536)

/*	"Watch" switches for DTPC protocol operation.			*/
#define WATCH_o			(1)
#define WATCH_newItem		(2)
#define WATCH_r			(4)
#define WATCH_complete		(8)
#define WATCH_send		(16)
#define	WATCH_l			(32)
#define	WATCH_m			(64)
#define WATCH_n			(128)
#define WATCH_i			(256)
#define WATCH_u			(512)
#define WATCH_v			(1024)
#define WATCH_discard		(2048)
#define WATCH_expire		(4096)
#define WATCH_reset		(8192)

typedef enum
{
	ResendAdu = 0,
	DeleteAdu,
	DeleteGap
} DtpcEventType;

typedef struct
{
        Object          text;           /*      Not NULL-terminated.    */
        unsigned int    textLength;
} BpString;

typedef struct
{
	Object		dstEid;		/*	SDR string		*/
	unsigned int	profileID;	
	Scalar		aduCounter;
	Object		outAdus;	/*	SDR list of outAdus	*/
	Object		queuedAdus;	/* SDR list of queued bundles	*/
	Object		inProgressAduElt;	/*	0 if no outADU
						 *	aggregation
						 *	in progress	*/
} OutAggregator;

typedef struct
{
	Scalar		seqNum;
	int		ageOfAdu;
	int		rtxCount;
	time_t		expirationTime;

	/*	Database navigation stuff	*/

	Object		aggregatedZCO;	/* ZCO Ref.			*/
	Object		bundleObj;	/* Bundle object		*/
	Object		outAggrElt;	/* Ref. to OutboundAggregator	*/
	Object		topics;		/* SDR list of Topics		*/ 
	Object		rtxEventElt;	/* Ref. to retransmission event	*/
	Object		delEventElt;	/* Ref. to deletion event	*/
} OutAdu;

typedef struct
{
	unsigned int	topicID;
	Object		payloadRecords;	/* SDR list of PayloadRecords	*/
	Object		outAduElt;	/* Ref. to OutAdu - not used	*/
} Topic;

typedef struct
{
	DtpcEventType	type;
	time_t		scheduledTime;
	Object		aduElt;
} DtpcEvent;
	
typedef struct
{
	Object		srcEid;		/* SDR string			*/
	unsigned int	profileID;
	Scalar		nextExpected;	/* Next expected seqNum		*/
	Scalar		resetSeqNum;	/* Last received seqNum in
					   reset range.			*/
	time_t		resetTimestamp;	/* Expiration time of
					   resetSeqNum			*/
	Object		inAdus;		/* SDR list of InAdus		*/	
} InAggregator;

typedef struct
{
	Scalar		seqNum;
        Object          aggregatedZCO;  /* ZCO Ref.			*/
        Object          inAggrElt;	/* Ref. to InboundAggretor	*/
	Object		gapEventElt;	/* Ref. to gap deletion event	*/
} InAdu;

typedef struct
{
	unsigned int	topicID;
	int		appPid;
	sm_SemId	semaphore;
	Object		dlvQueue;	/* SDR list of PayloadRecords */
} VSap;

typedef struct dtpcsap_st
{
	VSap		*vsap;
	DtpcElisionFn	elisionFn;	
	sm_SemId	semaphore;	
} Sap;


typedef struct
{
	Object		outAggregators;	/* SDR list OutboundAggregators	*/
	Object		inAggregators;	/* SDR list InboundAggregators	*/
	Object		events;		/* SDR list dtpcEvents		*/
	Object		profiles;	/* SDR list Profiles		*/
	Object		queues;		/* SDR list topic delivery queues 
					 * identified by list USER DATA	*/
	Object		outboundAdus;	/* SDR list: OutAdus		*/
} DtpcDB; 

typedef struct
{
	unsigned int	profileID;
	unsigned int	maxRtx;
	unsigned int    lifespan;
	unsigned int    aggrSizeLimit;
	unsigned int    aggrTimeLimit;
        BpAncillaryData	ancillaryData;
        int		srrFlags;
	BpCustodySwitch custodySwitch;
	Object		reportToEid;	/*	SDR String		*/
        int             classOfService;
} Profile;

typedef struct
{
	int		dtpcdPid;	/* For stopping dtpc daemon.	*/
	int		clockPid;	/* For stopping dtpcclock. 	*/
	int		watching;	/* Activity watch.		*/

	/* The aduSemaphore of the DTPC protocol is given whenever
	 * a new outAdu is inserted in the outbounAdus list.		*/ 

	sm_SemId	aduSemaphore;
	PsmAddress	vsaps;		/* SM list: VSaps		*/
	PsmAddress	profiles;	/* SM List: Profiles		*/
} DtpcVdb;

typedef struct
{
	Object		srcEid;		/*	SDR string		*/
	unsigned int	length;
	Object		content;
} DlvPayload;

extern int		dtpcInit();
#define dtpcStart()	_dtpcStart()
extern int		_dtpcStart();
#define dtpcStop()	_dtpcStop()
extern void		dtpcStop();
extern int		dtpcAttach();
extern unsigned int 	dtpcGetProfile(unsigned int maxRtx,
				unsigned int aggrSizeLimit,
				unsigned int aggrTimeLimit,
				unsigned int lifespan,
				BpAncillaryData *ancillaryData,
				unsigned char srrFlags,
				BpCustodySwitch custodySwitch,
				char *reportToEid,
				int classOfService);
extern int		raiseProfile(Sdr sdr, Object sdrElt, DtpcVdb *vdb);
extern int		raiseVSap(Sdr sdr, Object elt, DtpcVdb *vdb,
				unsigned int topicID);
extern int		initOutAdu(Profile *profile, Object outAggrAddr,
				Object outAggrElt, Object *outAduObj,
				Object *outAduElt);
extern int		insertRecord (DtpcSAP sap, char *dstEid,
				unsigned int profileID, unsigned int topicID,
				Object adu, int length);
extern int		createAdu(Profile *profile, Object outAduObj,
				Object outAduElt);
extern int		sendAdu(BpSAP sap);
extern void		deleteAdu(Sdr sdr, Object aduElt);
extern int		resendAdu(Sdr sdr, Object aduElt, time_t currentTime);
extern int		addProfile(unsigned int profileID, unsigned int maxRtx,
				unsigned int lifespan,
				unsigned int aggrSizeLimit,
				unsigned int aggrTimeLimit,
				char *svcClass, char *flags,
				char* reportToEid);
extern int		removeProfile(unsigned int profileID);
extern Object		getDtpcDbObject();
extern DtpcDB		*getDtpcConstants();
extern DtpcVdb		*getDtpcVdb();
extern int		handleInAdu(Sdr sdr, BpSAP txSap, BpDelivery *dlv,
				unsigned int profNum, Scalar seqNum);
extern int		handleAck(Sdr sdr, BpDelivery *dlv,
				unsigned int profNum, Scalar seqNum);
extern void		deletePlaceholder(Sdr sdr, Object aduElt);
extern int		parseInAdus(Sdr sdr);
extern int		sendAck(BpSAP sap, unsigned int profileID,
				Scalar seqNum, BpDelivery *dlv);
extern void		scalarToSdnv(Sdnv *sdnv, Scalar *scalar);
extern int		sdnvToScalar(Scalar *scalar, unsigned char *sdnvText);
extern int		compareScalars(Scalar *scalar1, Scalar *scalar2);

