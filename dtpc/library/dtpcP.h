/*
	dtpcP.h:	Private definitions supporting the implementation
			of DTPC nodes in the ION (Interplanetary Overlay
			Network) stack.

	Authors: Giorgos Papastergiou, SPICE
		 Ioannis Alexiadis, SPICE

	Copyright (c) 2011, Space Internetworking Center,
	Democritus University of Thrace. ALL RIGHTS RESERVED.
*/

#include "bp.h"
#include "dtpc.h"
#include "platform.h"
#include "psm.h"
#include "sdrxn.h"

#include <time.h>

#define DTPC_SEND_SVC_NBR	(128)
#define DTPC_RECV_SVC_NBR	(129)

/*	"Watch" switches for DTPC protocol operation.			*/
#define WATCH_o			(1)
#define WATCH_newItem		(2)
#define WATCH_r			(4)
#define WATCH_complete		(8)
#define WATCH_send		(16)
#define WATCH_l			(32)
#define WATCH_m			(64)
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
	SdrObject    text; /* Not NULL-terminated. */
	unsigned int textLength;
} BpString;

typedef struct
{
	SdrObject    dstEid;	       /* SDR string */
	unsigned int profileID;
	Scalar	     aduCounter;
	SdrObject    outAdus;	       /* SDR list of outAdus */
	SdrObject    queuedAdus;       /* SDR list of queued bundles */
	SdrObject    inProgressAduElt; /* 0 if no outADU aggregation in progress */
} OutAggregator;

typedef struct
{
	Scalar		seqNum;
	int		ageOfAdu;
	int		rtxCount;
	time_t		expirationTime;

	/*	Database navigation stuff	*/

	SdrObject aggregatedZCO; /* ZCO Ref. */
	SdrObject bundleObj;	 /* Bundle object */
	SdrObject outAggrElt;	 /* Ref. to OutboundAggregator */
	SdrObject topics;	 /* SDR list of Topics */
	SdrObject rtxEventElt;	 /* Ref. to retransmission event */
	SdrObject delEventElt;	 /* Ref. to deletion event */
	uvast	  aggrLength;	 /* Sum of payload-record lengths */
} OutAdu;

typedef struct
{
	unsigned int	topicID;
	SdrObject	payloadRecords; /* SDR list of PayloadRecords */
	SdrObject	outAduElt;	/* Ref. to OutAdu - not used */
} Topic;

typedef struct
{
	DtpcEventType	type;
	time_t		scheduledTime;
	SdrObject	aduElt;
} DtpcEvent;

typedef struct
{
	SdrObject	srcEid;		/* SDR string */
	unsigned int	profileID;
	Scalar		nextExpected;	/* Next expected seqNum		*/
	Scalar		resetSeqNum;	/* Last received seqNum in
					   reset range.			*/
	time_t		resetTimestamp;	/* Expiration time of
					   resetSeqNum			*/
	SdrObject	inAdus;		/* SDR list of InAdus */
} InAggregator;

typedef struct
{
	Scalar	  seqNum;
	SdrObject aggregatedZCO; /* ZCO Ref. */
	SdrObject inAggrElt;	 /* Ref. to InboundAggretor */
	SdrObject gapEventElt;	 /* Ref. to gap deletion event */
} InAdu;

typedef struct
{
	unsigned int	topicID;
	int		appPid;
	sm_SemId	semaphore;
	SdrObject	dlvQueue; /* SDR list of PayloadRecords */
} VSap;

typedef struct dtpcsap_st
{
	VSap		*vsap;
	DtpcElisionFn	elisionFn;
	sm_SemId	semaphore;
} Sap;


typedef struct
{
	SdrObject outAggregators; /* SDR list OutboundAggregators */
	SdrObject inAggregators;  /* SDR list InboundAggregators */
	SdrObject events;	  /* SDR list dtpcEvents */
	SdrObject profiles;	  /* SDR list Profiles */
	/* SDR list topic delivery queues identified by list USER DATA */
	SdrObject queues;
	SdrObject outboundAdus;	  /* SDR list: OutAdus */
} DtpcDB;

typedef struct
{
	unsigned int	profileID;
	unsigned int	maxRtx;
	unsigned int	lifespan;
	size_t		aggrSizeLimit;
	unsigned int	aggrTimeLimit;
	BpAncillaryData ancillaryData;
	int		srrFlags;
	BpCustodySwitch custodySwitch;
	SdrObject	reportToEid; /* SDR String */
	int		classOfService;
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
	SdrObject	srcEid;		/* SDR string */
	size_t		length;
	SdrObject	content;
} DlvPayload;

extern int		dtpcInit(void);
#define dtpcStart()	_dtpcStart()
extern int		_dtpcStart(void);
#define dtpcStop()	_dtpcStop()
extern void		_dtpcStop(void);
extern int		dtpcAttach(void);
extern unsigned int 	dtpcGetProfile(unsigned int maxRtx,
				size_t aggrSizeLimit,
				unsigned int aggrTimeLimit,
				unsigned int lifespan,
				BpAncillaryData *ancillaryData,
				unsigned char srrFlags,
				BpCustodySwitch custodySwitch,
				char *reportToEid,
				int classOfService);
extern int		raiseProfile(Sdr sdr, SdrObject sdrElt, DtpcVdb *vdb);
extern int		raiseVSap(Sdr sdr, SdrObject elt, DtpcVdb *vdb,
				unsigned int topicID);
extern int		initOutAdu(Profile *profile, SdrObject outAggrAddr,
				SdrObject outAggrElt, SdrObject *outAduObj,
				SdrObject *outAduElt);
extern int		insertRecord (DtpcSAP sap, char *dstEid,
				unsigned int profileID, unsigned int topicID,
				SdrObject adu, size_t length);
extern int		createAdu(Profile *profile, SdrObject outAduObj,
				SdrObject outAduElt);
extern int		sendAdu(BpSAP sap);
extern void		deleteAdu(Sdr sdr, SdrObject aduElt);
extern int		resendAdu(Sdr sdr, SdrObject aduElt, time_t currentTime);
extern int		addProfile(unsigned int profileID,
				unsigned int maxRtx,
				size_t aggrSizeLimit,
				unsigned int aggrTimeLimit,
				unsigned int lifespan,
				char *svcClass,
				char* reportToEid,
				char *flags);
extern int		removeProfile(unsigned int profileID);
extern SdrObject		getDtpcDbObject(void);
extern DtpcDB		*getDtpcConstants(void);
extern DtpcVdb		*getDtpcVdb(void);
extern int		handleInAdu(Sdr sdr, BpSAP txSap, BpDelivery *dlv,
				unsigned int profNum, Scalar seqNum);
extern int		handleAck(Sdr sdr, BpDelivery *dlv,
				unsigned int profNum, Scalar seqNum);
extern void		deletePlaceholder(Sdr sdr, SdrObject aduElt);
extern int		parseInAdus(Sdr sdr);
extern int		sendAck(BpSAP sap, unsigned int profileID,
				Scalar seqNum, BpDelivery *dlv);
extern int		compareScalars(Scalar *scalar1, Scalar *scalar2);
