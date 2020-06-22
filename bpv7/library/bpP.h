/*
 	bpP.h:	private definitions supporting the implementation
		of BP (Bundle Protocol) version 7 nodes in the ION
		(Interplanetary Overlay Network) stack, including
		scheme-specific forwarders and the convergence-layer
		adapters they rely on.

	Author: Scott Burleigh, JPL

	Modification History:
	Date      Who   What

	Copyright (c) 2019, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
 									*/
#ifndef _BPP_H_
#define _BPP_H_

#include "bp.h"
#include "rfx.h"
#include "cbor.h"
#include "crc.h"

#ifdef __cplusplus
extern "C" {
#endif

#define	BP_VERSION			7

#define	BpUdpDefaultPortNbr		4556
#define	BpTcpDefaultPortNbr		4556

/*	"Watch" switches for bundle protocol operation.			*/
#define	WATCH_a				(1)
#define	WATCH_b				(2)
#define	WATCH_c				(4)
#define	WATCH_m				(8)
#define	WATCH_w				(16)
#define	WATCH_x				(32)
#define	WATCH_y				(64)
#define	WATCH_z				(128)
#define	WATCH_abandon			(256)
#define	WATCH_expire			(512)
#define	WATCH_refusal			(1024)
#define	WATCH_clfail			(2048)
#define	WATCH_limbo			(4096)
#define	WATCH_delimbo			(8192)
#define	WATCH_timeout			(16384)

#define WATCH_BP	(WATCH_a | WATCH_b | WATCH_c | WATCH_y | WATCH_z \
| WATCH_abandon | WATCH_expire | WATCH_clfail | WATCH_limbo | WATCH_delimbo)

#define WATCH_BIBE	(WATCH_m | WATCH_w | WATCH_x | WATCH_refusal \
| WATCH_timeout)

#define SDR_LIST_ELT_OVERHEAD		(WORD_SIZE * 4)

/*	A note on coding technique: the checking of return codes
 *	can safely be foregone in some circumstances, to simplify
 *	the code and marginally improve performance.  The general
 *	rule is that while an SDR transaction is open it is not
 *	necessary to check the value returned by an SDR function
 *	UNLESS the result of that function must be non-failure
 *	in order for subsequent operations in the scope of the
 *	same transaction to be safe [e.g., to prevent segmentation
 *	faults].  The reason for this is that sdr_end_xn(), when
 *	it is invoked to conclude the transaction, will itself
 *	fail and back out the transaction if any SDR operation
 *	within the scope of the transaction failed; it is only
 *	(and ALWAYS) necessary to check the value returned by
 *	sdr_end_xn().
 *
 *	Note also that, if you do check the value returned by an
 *	SDR function while a transaction is open and it indicates
 *	failure, it is NOT necessary to abort the transaction by
 *	calling sdr_cancel_xn(); the transaction has already been
 *	aborted.  It is only necessary to return a failure indication
 *	to invoking code and let sdr_end_xn() at the top of the
 *	function stack clean up the transaction and return -1 to
 *	trigger the applicable failure handling.  Call sdr_cancel_xn()
 *	ONLY when a fatal error condition is detected elsewhere than
 *	in execution of an SDR function.				*/

#define MIN_PRIMARY_BLK_LENGTH		(23)
#define MAX_CL_PROTOCOL_NAME_LEN	(15)
#define MAX_CL_DUCT_NAME_LEN		(255)
#define	MAX_SCHEME_NAME_LEN		(15)
#define	MAX_NSS_LEN			(63)
#define	MAX_EID_LEN			(MAX_SCHEME_NAME_LEN + MAX_NSS_LEN + 2)

#ifndef	BP_MAX_BLOCK_SIZE
#define BP_MAX_BLOCK_SIZE		(2000)
#endif

#ifndef MAX_XMIT_COPIES
#define	MAX_XMIT_COPIES			(20)
#endif

#ifndef MIN_CONFIDENCE_IMPROVEMENT
#define	MIN_CONFIDENCE_IMPROVEMENT	(.05)
#endif

#ifndef MIN_NET_DELIVERY_CONFIDENCE
#define MIN_NET_DELIVERY_CONFIDENCE	(.80)
#endif

#ifndef	ION_DEFAULT_XMIT_RATE
#define	ION_DEFAULT_XMIT_RATE		(125000000)
#endif

#define	TYPICAL_STACK_OVERHEAD		(36)

#define	MAX_TIME			((unsigned int) ((1U << 31) - 1))

/*	An ION "node" is a set of cooperating state machines that
 *	together constitute a single functional point of presence,
 *	residing in a single SDR heap, in a DTN-based network.
 *
 *	A single node may be equipped to send and receive bundles
 *	to and from endpoints whose IDs are formed under one or
 *	more "schemes".  For each such scheme the node will have
 *	a single scheme-specific "forwarder", for which scheme-
 *	specific forwarding information may be recorded in the
 *	database.
 *
 *	To utilize the services of the node, an application instance
 *	calls bp_open() to reserve for its own use a specified DTN
 *	endpoint that is registered at this node, and then calls
 *	bp_receive() to extract inbound bundles from the queue of
 *	undelivered inbound bundles for that endpoint and/or calls
 *	bp_send() to issue bundles to some specified endpoint.
 *
 *	The blocks of data that DTN applications exchange among
 *	themselves are termed "application data units" (ADUs).  A
 *	BP forwarder packages ADUs within bundles and then uses
 *	services provided by underlying communication protocols (at
 *	the "convergence layer" and below) to transmit the bundles
 *	to other nodes.							*/

typedef struct
{
	uvast		nbr;
} NodeId;				//	Still needed?

typedef enum
{
	unknown = 0,
	dtn = 1,
	ipn = 2,
	imc = 3
} SchemeCodeNbr;

typedef enum
{
	EidNV = 0,			/*	Non-volatile.		*/
	EidV,				/*	Volatile.		*/
	EidS				/*	Null-terminated string.	*/
} EidMode;

typedef union
{
	Object		nv;		/*	Recorded in SDR heap.	*/
	PsmAddress	v;		/*	Recorded in wm.		*/
	char		*s;		/*	Temporary string in RAM.*/
} EndpointName;

typedef struct
{
	EndpointName	endpointName;
	int		nssLength;	/*	+ for nv, - for v.	*/
} DtnSSP;

typedef struct
{
	uvast		nodeNbr;
	unsigned int	serviceNbr;
} IpnSSP;

typedef struct
{
	uvast		groupNbr;
	unsigned int	serviceNbr;
} ImcSSP;

typedef union
{
	DtnSSP		dtn;
	IpnSSP		ipn;
	ImcSSP		imc;
} SSP;

typedef struct
{
	SchemeCodeNbr	schemeCodeNbr;
	SSP		ssp;
} EndpointId;

typedef struct
{
	char		*schemeName;
	int		schemeNameLength;
	SchemeCodeNbr	schemeCodeNbr;
	char		*colon;
	char		*nss;
	int		nssLength;
	uvast		elementNbr;	/*	Node nbr or group nbr.	*/
	unsigned int	serviceNbr;
	char		nullEndpoint;	/*	Boolean.		*/
} MetaEid;

typedef struct
{
	char		*protocolName;
	char		*proxNodeEid;
	size_t		xmitRate;
} DequeueContext;

/*	*	*	Bundle structures	*	*	*	*/

/*	For non-fragmentary bundles, and for the first fragmentary
 *	bundle of a fragmented source bundle, fragmentOffset is zero.
 *	For fragmentary bundles other than the first, fragmentOffset
 *	indicates the offset of the fragmentary bundle's payload
 *	from the beginning of the payload of the original source
 *	bundle.								*/

typedef struct
{
	EndpointId	source;		/*	Original sender.	*/
	BpTimestamp	creationTime;	/*	Time and count.		*/
	unsigned int	fragmentOffset;	/*	0 if not a fragment.	*/
} BundleId;

typedef struct
{
	vast		length;		/*	initial length of ZCO	*/
	Object		content;	/*	a ZCO reference in SDR	*/
	BpCrcType	crcType;
} Payload;

/*	Administrative record types	*/
#define	BP_STATUS_REPORT	(1)
#define	BP_MULTICAST_PETITION	(5)
#define	BP_SAGA_MESSAGE		(6)
#define	BP_BIBE_PDU		(7)
#define	BP_BIBE_SIGNAL		(8)	/*	Aggregate, in BIBE.	*/

typedef enum
{
	SrLifetimeExpired = 1,
	SrUnidirectionalLink,
	SrCanceled,
	SrDepletedStorage,
	SrDestinationUnintelligible,
	SrNoKnownRoute,
	SrNoTimelyContact,
	SrBlockUnintelligible,
	SrHopCountExceeded,
	SrTrafficPared
} BpSrReason;

typedef time_t		DtnTime;	/*	Epoch 2000.		*/

typedef struct
{
	BpTimestamp	creationTime;	/*	From bundle's ID.	*/
	unsigned int	fragmentOffset;	/*	From bundle's ID.	*/
	unsigned int	fragmentLength;
	EndpointId	sourceEid;	/*	From bundle's ID.	*/
	unsigned char	isFragment;	/*	Boolean.		*/
	unsigned char	flags;
	BpSrReason	reasonCode;
	DtnTime		receiptTime;
	DtnTime		forwardTime;
	DtnTime		deliveryTime;
	DtnTime		deletionTime;
} BpStatusRpt;

/*	The convergence-layer adapter uses the ClDossier structure to
 *	assert its own private knowledge about the bundle: authenticity
 *	and/or EID of sender.						*/

typedef struct
{
	int		authentic;	/*	Boolean.		*/
	EndpointId	senderEid;
	uvast		senderNodeNbr;	/*	If ipn endpoint.	*/
} ClDossier;

/*	Bundle processing flags						*/
#define BDL_IS_FRAGMENT		(1)	/* 0000 00000000 00000001	*/
#define BDL_IS_ADMIN		(2)	/* 0000 00000000 00000010	*/
#define BDL_DOES_NOT_FRAGMENT	(4)	/* 0000 00000000 00000100	*/
#define BDL_APP_ACK_REQUEST	(32)	/* 0000 00000000 00100000	*/
#define BDL_STATUS_TIME_REQ	(64)	/* 0000 00000000 01000000	*/
#define BDL_RECEIVED_RPT_REQ	(16384)	/* 0000 01000000 00000000	*/
#define BDL_FORWARDED_RPT_REQ	(65536)	/* 0001 00000000 00000000	*/
#define BDL_DELIVERED_RPT_REQ	(131072)/* 0010 00000000 00000000	*/
#define BDL_DELETED_RPT_REQ	(262144)/* 0100 00000000 00000000	*/

/*	Block processing flags						*/
#define BLK_MUST_BE_COPIED	(1)	/*	00000001		*/
#define BLK_REPORT_IF_NG	(2)	/*	00000010		*/
#define BLK_ABORT_IF_NG		(4)	/*	00000100		*/
#define BLK_REMOVE_IF_NG	(16)	/*	00010000		*/

typedef struct
{
	BundleId	id;		/*	Incl. source EID.	*/

	/*	Stuff in Primary block.					*/

	unsigned int	bundleProcFlags;/*	Incl. SR requests.	*/
	unsigned int	timeToLive;	/*	In seconds.		*/
	EndpointId	destination;	/*	...of bundle's ADU.	*/
		/*	source of bundle's ADU is in the id field.	*/
	EndpointId	reportTo;
	unsigned int	expirationTime;	/*	Time tag in seconds.	*/
		/*	creation time is in the id field.		*/
	unsigned int	totalAduLength;
		/*	fragment offset is in the id field.		*/

	/*	Stuff in QOS (nee ECOS) & Metadata extension blocks.	*/

	BpAncillaryData	ancillaryData;
	unsigned char	classOfService;	/*	From QoS block if any.	*/
	unsigned char	priority;	/*	Possibly an override.	*/
	unsigned char	ordinal;	/*	Possibly an override.	*/

	/*	Stuff in (or for) the Bundle Age extension block.	*/

	unsigned int	age;		/*	In microseconds.	*/
	struct timeval	arrivalTime;

	/*	Stuff in (or for) the Bundle Age extension block.	*/

	unsigned int	hopLimit;
	unsigned int	hopCount;

	/*	Stuff in Spray and Wait extension block.		*/

	unsigned char	permits;	/*	# SnW fwd permits left.	*/

	/*	Stuff in Payload block.					*/

	unsigned int	payloadBlockProcFlags;
	Payload		payload;

	/*	Stuff in extension blocks: an SDR list of ExtensionBlock
	 *	objects.  The extensionsLength field is the sum of the
	 *	"length" values of all ExtensionBlocks in the extensions
	 *	list.							*/

	Object		extensions;
	int		extensionsLength;	/*	Concatenated.	*/
	int		lastBlkNumber;

	/*	Internal housekeeping stuff.				*/

	char		detained;	/*	Boolean.		*/
	char		delivered;	/*	Boolean.		*/
	char		suspended;	/*	Boolean.		*/
	char		returnToSender;	/*	Boolean.		*/
	char		accepted;	/*	Boolean.		*/
	char		corrupt;	/*	Boolean.		*/
	char		altered;	/*	Boolean.		*/
	char		anonymous;	/*	Boolean.		*/
	char		fragmented;	/*	Boolean.		*/
	char		ovrdPending;	/*	Boolean.		*/
	int		dbOverhead;	/*	SDR bytes occupied.	*/
	ZcoAcct		acct;		/*	Inbound or Outbound.	*/
	BpStatusRpt	statusRpt;	/*	For response per SRRs.	*/
	ClDossier	clDossier;	/*	Processing hints.	*/
	Object		stations;	/*	Stack of EIDs (route).	*/

	/*	Stuff for opportunistic forwarding.  A "copy" is the
	 *	ID of a node to which CGR has decided to forward a
	 *	copy of the bundle even though our confidence that
	 *	the bundle will actually get delivered via the route
	 *	through that node is less than 100%.  The bundle's
	 *	dlvConfidence is the our net confidence that the
	 *	bundle will get delivered, one way or another, as
	 *	calculated from our confidence in all copies.		*/

	uvast		xmitCopies[MAX_XMIT_COPIES];
	int		xmitCopiesCount;
	float		dlvConfidence;	/*	0.0 to 1.0		*/

	/*	Database navigation stuff (back-references).		*/

	Object		hashEntry;	/*	Entry in bundles hash.	*/

	Object		timelineElt;	/*	TTL expire list ref.	*/
	Object		overdueElt;	/*	Xmit overdue ref.	*/
	Object		transitElt;	/*	Transit queue ref.	*/
	Object		fwdQueueElt;	/*	Scheme's queue ref.	*/
	Object		fragmentElt;	/*	Incomplete's list ref.	*/
	Object		dlvQueueElt;	/*	Endpoint's queue ref.	*/
	Object		trackingElts;	/*	List of app. list refs.	*/

	Object		incompleteElt;	/*	Ref. to Incomplete.	*/

	/*	Transmission queue (or limbo list) stuff.		*/

	Object		planXmitElt;	/*	Issuance queue ref.	*/
	Object		ductXmitElt;	/*	Transmission queue ref.	*/
	Object		proxNodeEid;	/*	An SDR string.		*/
	time_t		enqueueTime;	/*	When queued for xmit.	*/
} Bundle;

#define SRR_FLAGS(bundleProcFlags)	((bundleProcFlags >> 8) & 0xff)

typedef struct
{
	Object		fragments;	/*	SDR list		*/

	/*	Each element of the fragments list is a Bundle object;
	 *	the list is in ascending fragment offset order.  When
	 *	the list is complete -- the fragment offset of the
	 *	first bundle is zero, the fragment offset of each
	 *	subsequent bundle is less than or equal to the sum
	 *	of the fragment offset and payload length of the
	 *	prior bundle, and the sum of the fragment offset and
	 *	payload length of the last bundle is equal to the
	 *	total ADU length -- the payloads are concatenated
	 *	into the original ADU, the payload for a new 
	 *	aggregate bundle that is queued for delivery to
	 *	the application, and all fragments (including the
	 *	final one) are destroyed.				*/

	unsigned int	totalAduLength;
} IncompleteBundle;

/*	*	*	Endpoint structures	*	*	*	*/

typedef enum
{
	DiscardBundle = 0,
	EnqueueBundle
} BpRecvRule;

/*	The endpoint object characterizes an endpoint within which
 *	the node is registered.						*/

typedef struct
{
	char		nss[MAX_NSS_LEN + 1];
	BpRecvRule	recvRule;
	Object		recvScript;	/*	SDR string		*/
	Object		incompletes;	/*	SDR list of Incompletes	*/
	Object		deliveryQueue;	/*	SDR list of Bundles	*/
	Object		scheme;		/*	back-reference		*/
	Object		stats;		/*	EndpointStats address.	*/
	int		updateStats;	/*	Boolean.		*/
} Endpoint;

#define	BP_ENDPOINT_SOURCED		0
#define	BP_ENDPOINT_QUEUED		1
#define	BP_ENDPOINT_ABANDONED		2
#define	BP_ENDPOINT_DELIVERED		3
#define BP_ENDPOINT_STATS		4

typedef struct
{
	time_t		resetTime;
	Tally		tallies[BP_ENDPOINT_STATS];
} EndpointStats;

/*	The volatile endpoint object encapsulates the volatile state
 *	of the corresponding Endpoint.					*/

typedef struct
{
	Object		endpointElt;	/*	Reference to Endpoint.	*/
	Object		stats;		/*	EndpointStats address.	*/
	int		updateStats;	/*	Boolean.		*/
	char		nss[MAX_NSS_LEN + 1];
	int		appPid;		/*	Consumes dlv notices.	*/
	sm_SemId	semaphore;	/*	For dlv notices.	*/
} VEndpoint;

/*	*	*	Scheme structures	*	*	*	*/

/*	Scheme objects are used to encapsulate knowledge about how to
 *	forward bundles.  						*/

typedef struct
{
	char		name[MAX_SCHEME_NAME_LEN + 1];
	int		nameLength;
	SchemeCodeNbr	codeNumber;
	Object		fwdCmd; 	/*	For starting forwarder.	*/
	Object		admAppCmd; 	/*	For starting admin app.	*/
	Object		forwardQueue;	/*	SDR list of Bundles	*/
	Object		endpoints;	/*	SDR list of Endpoints	*/
	Object		bclas;		/*	SDR list of BIBE CLAs	*/
} Scheme;

typedef struct
{
	Object		schemeElt;	/*	Reference to scheme.	*/

	/*	Copied from Scheme.					*/

	char		name[MAX_SCHEME_NAME_LEN + 1];
	int		nameLength;
	SchemeCodeNbr	codeNumber;

	/*	Volatile administrative stuff.				*/

	char		adminEid[MAX_EID_LEN];
	int		adminNSSLength;
	int		fwdPid;		/*	For stopping forwarder.	*/
	int		admAppPid;	/*	For stopping admin app.	*/
	sm_SemId	semaphore;	/*	For dispatch notices.	*/
	PsmAddress	endpoints;	/*	SM list: VEndpoint.	*/
} VScheme;

/*	Definitions supporting the use of QOS-sensitive bandwidth
 *	management.
 *
 *	Outflow objects are non-persistent.  They are used only to
 *	do QOS-sensitive bandwidth management.  Since they are private
 *	to CLMs, they can reside in the private memory of the CLM
 *	(rather than in shared memory).
 *
 *	Each egress Plan has three Outflows:
 *
 *		Expedited = 2, Standard = 1, and Bulk = 0.
 *
 * 	Expedited traffic goes out before any other.  Standard traffic
 * 	(service factor = 2) goes out twice as fast as Bulk traffic
 * 	(service factor = 1).                 				*/

#define EXPEDITED_FLOW   2

typedef struct
{
	Object		outboundBundles;/*	SDR list.		*/
	int		totalBytesSent;
	int		svcFactor;
} Outflow;

/*	*	*	Egress Plan structures	*	*	*	*/

#define	BP_PLAN_ENQUEUED	0
#define	BP_PLAN_DEQUEUED	1
#define	BP_PLAN_STATS		2

typedef struct
{
	time_t		resetTime;
	Tally		tallies[BP_PLAN_STATS];
} PlanStats;

typedef struct
{
	Scalar		backlog;
	Object		lastForOrdinal;	/*	SDR list element.	*/
} OrdinalState;

typedef struct
{
	char		neighborEid[MAX_EID_LEN];

	/*	Note: neighborEid may be a wildcarded EID string.	*/

	uvast		neighborNodeNbr;/*	If neighborEid is ipn.	*/

	/*	If the plan for bundles destined for this neighbor is
	 *	to relay them via some other EID, then that "via"
	 *	EID is given here and the rest of the structure is
	 *	unused.							*/

	Object		viaEid;		/*	An sdrstring.		*/

	/*	Otherwise, viaEid is zero and the plan for bundles
	 *	destined for this neighbor is to transmit them using
	 *	one of the ducts in the list.				*/

	unsigned int	nominalRate;	/*	Bytes per second.	*/
	int		blocked;	/*	Boolean			*/
	Object		stats;		/*	PlanStats address.	*/
	int		updateStats;	/*	Boolean.		*/
	Object		bulkQueue;	/*	SDR list of Bundles	*/
	Scalar		bulkBacklog;	/*	Bulk bytes enqueued.	*/
	Object		stdQueue;	/*	SDR list of Bundles	*/
	Scalar		stdBacklog;	/*	Std bytes enqueued.	*/
	Object		urgentQueue;	/*	SDR list of Bundles	*/
	Scalar		urgentBacklog;	/*	Urgent bytes enqueued.	*/
	OrdinalState	ordinals[256];	/*	Orders urgent queue.	*/
	Object		ducts;		/*	SDR list: outduct elts.	*/
	Object		context;	/*	For duct selection.	*/
} BpPlan;

typedef struct
{
	Object		planElt;	/*	Reference to BpPlan.	*/
	Object		stats;		/*	PlanStats address.	*/
	int		updateStats;	/*	Boolean.		*/
	char		neighborEid[MAX_EID_LEN];
	uvast		neighborNodeNbr;/*	If neighborEid is ipn.	*/
	int		clmPid;		/*	For stopping the CLM.	*/
	sm_SemId	semaphore;	/*	Queue non-empty.	*/
	Throttle	xmitThrottle;	/*	For rate control.	*/
} VPlan;

/*	*	*	Induct structures	*	*	*	*/

typedef struct
{
	char		name[MAX_CL_DUCT_NAME_LEN + 1];
	Object		cliCmd;		/*	For starting the CLI.	*/
	Object		protocol;	/*	back-reference		*/
	Object		stats;		/*	InductStats address.	*/
	int		updateStats;	/*	Boolean.		*/
} Induct;

#define	BP_INDUCT_RECEIVED		0
#define	BP_INDUCT_MALFORMED		1
#define	BP_INDUCT_INAUTHENTIC		2
#define	BP_INDUCT_CONGESTIVE		3
#define	BP_INDUCT_STATS			4

typedef struct
{
	time_t		resetTime;
	Tally		tallies[BP_INDUCT_STATS];
} InductStats;

typedef struct
{
	Object		inductElt;	/*	Reference to Induct.	*/
	Object		stats;		/*	InductStats address.	*/
	int		updateStats;	/*	Boolean.		*/
	char		protocolName[MAX_CL_PROTOCOL_NAME_LEN + 1];
	char		ductName[MAX_CL_DUCT_NAME_LEN + 1];
	int		cliPid;		/*	For stopping the CLI.	*/
} VInduct;

/*	*	*	Outduct structures	*	*	*	*/

typedef struct
{
	Object		planDuctListElt;/*	Assigned BpPlan.	*/
	char		name[MAX_CL_DUCT_NAME_LEN + 1];
	Object		cloCmd;		/*	For starting the CLO.	*/

	/*	Outducts are created automatically in at least two
	 *	cases.  In neighbor discovery, the eureka library
	 *	will encode the host/port of the discovered neighbor
	 *	in name and post the outduct, and the appropriate
	 *	CLO task will be started and passed the new outduct's
	 *	name.  Upon acceptance of a TCPCL connection from a
	 *	neighboring node, tcpcli will encode the name of the
	 *	outduct as "#:<fd>", where <fd> is the number of the
	 *	file descriptor for the accepted socket, and a
	 *	transmission thread will be started and passed this
	 *	number.							*/

	unsigned int	maxPayloadLen;	/*	0 = no limit.		*/
	Object		xmitBuffer;	/*	SDR list of (1) ZCO.	*/
	Object		context;	/*	For duct selection.	*/
	Object		protocol;	/*	back-reference		*/
} Outduct;

typedef struct
{
	Object		outductElt;	/*	Reference to Outduct.	*/
	char		protocolName[MAX_CL_PROTOCOL_NAME_LEN + 1];
	char		ductName[MAX_CL_DUCT_NAME_LEN + 1];
	int		hasThread;	/*	Boolean.		*/
	pthread_t	cloThread;	/*	For stopping the CLO.	*/
	int		cloPid;		/*	For stopping the CLO.	*/
	sm_SemId	semaphore;	/*	Buffer non-empty.	*/
	time_t		timeOfLastXmit;
} VOutduct;

/*	*	*	Protocol structures	*	*	*	*/

#define	BP_PROTOCOL_ANY	(BP_BEST_EFFORT | BP_RELIABLE | BP_RELIABLE_STREAMING)

typedef struct
{
	char		name[MAX_CL_PROTOCOL_NAME_LEN + 1];
	int		payloadBytesPerFrame;
	int		overheadPerFrame;
	int		protocolClass;	/*	Contributes to QOS.	*/
} ClProtocol;

/*	*	*	BP Database structures	*	*	*	*/

#define	EPOCH_2000_SEC	946684800

/*	An Encounter is a record of a past discovered contact.		*/

typedef struct
{
	uvast		fromNode;	/*	CBHE node number	*/
	uvast		toNode;		/*	CBHE node number	*/
	time_t		fromTime;	/*	As from getCtime()	*/
	time_t		toTime;		/*	As from getCtime()	*/
	size_t		xmitRate;	/*	In bytes per second.	*/
} Encounter;

typedef enum
{
	expiredTTL = 1,
	xmitOverdue = 2,
	ctOverdue = 3
} BpEventType;

typedef struct
{
	BpEventType	type;
	time_t		time;		/*	as from time(2)		*/
	Object		ref;		/*	Bundle, etc.		*/
} BpEvent;

typedef struct
{
	Object		bundleObj;	/*	0 if count > 1.		*/
	unsigned int	count;
} BundleSet;

typedef struct
{
	Object		schemes;	/*	SDR list of Schemes	*/
	Object		plans;		/*	SDR list of BpPlans	*/
	Object		protocols;	/*	SDR list of ClProtocols	*/
	Object		inducts;	/*	SDR list of Inducts	*/
	Object		outducts;	/*	SDR list of Outducts	*/
	Object		saga[2];	/*	SDR lists of Encounters	*/
	Object		timeline;	/*	SDR list of BpEvents	*/
	Object		bundles;	/*	SDR hash of BundleSets	*/
	Object		inboundBundles;	/*	SDR list of ZCOs	*/

	/*	The Transit queue is a list of received in-transit
	 *	Bundles that are awaiting presentation to forwarder
	 *	daemons, so that they can be enqueued for transmission.	*/

	Object		transit;	/*	SDR list of Bundles	*/
	Object		limboQueue;	/*	SDR list of Bundles	*/
	Object		clockCmd; 	/*	For starting bpclock.	*/
	Object		transitCmd; 	/*	For starting bptransit.	*/
	unsigned int	maxAcqInHeap;	/*	Bytes of ZCO.		*/
	int		watching;	/*	Activity watch switch.	*/

	/*	For computation of BpTimestamp values.			*/

	time_t		creationTimeSec;/*	Epoch 2000.		*/
	unsigned int	bundleCounter;	/*	For value of count.	*/
	unsigned int	maxBundleCount;	/*	Limits value of count.	*/

	/*	Network management instrumentation			*/

	time_t		resetTime;	/*	Stats reset time.	*/
	time_t		startTime;	/*	Node restart time.	*/
	int		regCount;	/*	Since node restart.	*/
	int		updateStats;	/*	Boolean.		*/
	vast		currentBundlesFragmented;
	vast		totalBundlesFragmented;
	vast		currentFragmentsProduced;
	vast		totalFragmentsProduced;
	Object		sourceStats;	/*	BpCosStats address.	*/
	Object		recvStats;	/*	BpCosStats address.	*/
	Object		discardStats;	/*	BpCosStats address.	*/
	Object		xmitStats;	/*	BpCosStats address.	*/
	Object		delStats;	/*	BpDelStats address.	*/
	Object		dbStats;	/*	BpDbStats address.	*/
} BpDB;

#define BP_STATUS_RECEIVE	0
#define BP_STATUS_FORWARD	1
#define BP_STATUS_DELIVER	2
#define BP_STATUS_DELETE	3
#define BP_STATUS_STATS		4

#define BP_REASON_NONE		0
#define BP_REASON_EXPIRED	1
#define BP_REASON_FWD_UNIDIR	2
#define BP_REASON_CANCELED	3
#define BP_REASON_DEPLETION	4
#define BP_REASON_EID_MALFORMED	5
#define BP_REASON_NO_ROUTE	6
#define BP_REASON_NO_CONTACT	7
#define BP_REASON_BLK_MALFORMED	8
#define BP_REASON_TOO_MANY_HOPS	9
#define BP_REASON_TRAFFIC_PARED	10
#define BP_REASON_STATS		11

#define	BP_DB_QUEUED_FOR_FWD	0
#define	BP_DB_FWD_OKAY		1
#define	BP_DB_FWD_FAILED	2
#define	BP_DB_REQUEUED_FOR_FWD	3
#define	BP_DB_TO_LIMBO		4
#define	BP_DB_FROM_LIMBO	5
#define	BP_DB_EXPIRED		6
#define	BP_DB_ABANDON		7
#define	BP_DB_DISCARD		8
#define	BP_DB_STATS		9

typedef struct
{
	Tally		tallies[3];
} BpCosStats;		/*	Statistics by priority level.		*/

typedef struct
{
	unsigned int	totalDelByReason[BP_REASON_STATS];
	unsigned int	currentDelByReason[BP_REASON_STATS];
} BpDelStats;

typedef struct
{
	Tally		tallies[BP_DB_STATS];
} BpDbStats;

/*	Discoveries posted by IPND, the neighbor discovery protocol.
 *	These objects are used to remember the time of last contact
 *	with each discovered neighbor, to prevent unnecessary beacon
 *	transmission.							*/

typedef struct
{
	char		eid[MAX_EID_LEN];
	time_t		startOfContact;
	time_t		lastContactTime;
} Discovery;

/*	Volatile database encapsulates the volatile state of the
 *	database.							*/

typedef struct
{
	Object		sourceStats;	/*	BpCosStats address.	*/
	Object		recvStats;	/*	BpCosStats address.	*/
	Object		discardStats;	/*	BpCosStats address.	*/
	Object		xmitStats;	/*	BpCosStats address.	*/
	Object		delStats;	/*	BpDelStats address.	*/
	Object		dbStats;	/*	BpDbStats address.	*/
	int		updateStats;	/*	Boolean.		*/
	int		bundleCounter;
	int		clockPid;	/*	For stopping bpclock.	*/
	int		transitPid;	/*	For stopping bptransit.	*/
	sm_SemId	transitSemaphore;
	int		watching;	/*	Activity watch switch.	*/

	/*	For finding structures in database.			*/

	PsmAddress	schemes;	/*	SM list: VScheme.	*/
	PsmAddress	plans;		/*	SM list: VPlan.		*/
	PsmAddress	inducts;	/*	SM list: VInduct.	*/
	PsmAddress	outducts;	/*	SM list: VOutduct.	*/
	PsmAddress	discoveries;	/*	SM list: Discovery.	*/
	PsmAddress	timeline;	/*	SM RB tree: list xref.	*/
} BpVdb;

/*	*	*	Acquisition structures	*	*	*	*/

typedef enum
{
	AcqTBD = 0,
	AcqOK
} AcqDecision;

typedef struct
{
	VInduct		*vduct;

	/*	Per-bundle state variables.				*/

	Object		rawBundle;
	Bundle		bundle;
	int		headerLength;
	int		bundleLength;
	int		authentic;	/*	Boolean.		*/
	Lyst		extBlocks;	/*	(AcqExtBlock *)		*/
	AcqDecision	decision;
	int		malformed;
	int		congestive;	/*	Not enough ZCO space.	*/
	int		mustAbort;	/*	Unreadable block(s).	*/

	/*	Per-acquisition state variables.			*/

	int		allAuthentic;	/*	Boolean.		*/
	EndpointId	senderEid;
	Object		acqFileRef;
	Object		zco;		/*	Concatenated bundles.	*/
	Object		zcoElt;		/*	Retention in BpDB.	*/
	int		zcoLength;
	int		zcoBytesConsumed;
	ZcoReader	reader;
	int		zcoBytesReceived;
	int		bytesBuffered;
	char		buffer[BP_MAX_BLOCK_SIZE];
} AcqWorkArea;

/*	*	*	Function prototypes.	*	*	*	*/

extern int		bpSend(		MetaEid *sourceMetaEid,
					char *destEid,
					char *reportToEid,
					int lifespan,
					int priority,
					BpCustodySwitch custodySwitch,
					unsigned char srrFlags,
					int ackRequested,
					BpAncillaryData *ancillaryData,
					Object adu,
					Object *newBundle,
					int bundleIsAdmin);
			/*	This function creates a new bundle
			 *	and queues it for forwarding by the
			 *	applicable scheme-specific forwarder,
			 *	based on the scheme name of the
			 *	destination endpoint ID.
			 *
			 *	adu must be a zero-copy object
			 *	reference as returned by zco_create().
			 *	bundleIsAdmin is Boolean, must be
			 *	1 for status reports and other
			 *	BP administrative records but zero
			 *	otherwise.
			 *
			 *	Returns 1 on success, in which case
			 *	(and only in this case) the address
			 *	of the new bundle within the ION
			 *	database is returned in newBundle.
			 *	If destination is found to be NULL
			 *	(a transient error), returns zero.
			 *	Otherwise (permanent system failure)
			 *	returns -1.				*/

extern int		bpAbandon(	Object bundleObj,
					Bundle *bundle,
					int reason);
			/*	This is the common processing for any
			 *	bundle that a forwarder decides it
			 *	cannot accept for forwarding.  It 
			 *	sends any applicable status reports
			 *	and then deletes the bundle from
			 *	local storage.
			 *
			 *	Reason code s/b BP_REASON_DEPLETION
			 *	or BP_REASON_NO_ROUTE.
			 *
			 *	Call this function at most once per
			 *	bundle.	 Returns 0 on success, -1 on
			 *	any failure.				*/

extern int		bpClone(	Bundle *originalBundle,
					Bundle *newBundleBuffer,
					Object *newBundleObj,
					unsigned int offset,
					unsigned int length);
			/*	The function creates a copy of part
			 *	or all of originalBundle, writing
			 *	it to the location returned in
			 *	newBundleObj.  newBundleBuffer is
			 *	required: it is used as the work
			 *	area for constructing the new bundle.
			 *	If offset is zero and length is the
			 *	length of originalBundle's payload
			 *	(or zero, which is used to indicate
			 *	"length of original bundle's payload")
			 *	then the copy will have a copy of
			 *	the entire payload of the original
			 *	bundle.  Otherwise the function is
			 *	used to "fragment" the bundle: the
			 *	new bundle's payload will be the
			 *	indicated subset of the original
			 *	payload.  Returns 0 on success,
			 *	-1 on any failure.			*/

extern int		bpAccept(	Object bundleObj,
					Bundle *bundle);
			/*	This is the common processing for any
			 *	bundle that a forwarder decides it
			 *	can accept for forwarding, whether
			 *	the bundle was sourced locally or
			 *	was received from some other node.
			 *	It updates statistics that are used
			 *	to make future bundle acquisition
			 *	decisions and it sends any applicable
			 *	status reports.
			 *
			 *	This function might be called multiple
			 *	times per bundle but will take effect
			 *	only once.  Returns 0 on success, -1
			 *	on any failure.				*/

extern int		bpFragment(	Bundle *bundle, Object bundleObj,
					Object *queueElt, size_t fragmentLength,
					Bundle *bundle1, Object *bundle1Obj,
					Bundle *bundle2, Object *bundle2Obj);
			/*	This function creates two fragmentary
			 *	bundles from one original bundle and
			 *	destroys the original bundle.  Returns
			 *	0 on success, -1 on any failure.	*/

extern int		bpEnqueue(	VPlan *vplan,
					Bundle *bundle,
					Object bundleObj);
			/*	This function is invoked by a forwarder
			 *	to enqueue a bundle for transmission
			 *	according to a defined egress plan.
			 *	It appends the indicated bundle to the
			 *	appropriate transmission queue of the
			 *	egress plan identified by "vplan" based
			 *	on the bundle's priority.
			 *
			 *	If forwarding the bundle to multiple
			 *	nodes (flooding or multicast), call
			 *	this function once per planned
			 *	transmission.
			 *
			 *	Returns 0 on success, -1 on any
			 *	failure.				*/

extern int		bpDequeue(	VOutduct *vduct,
					Object *outboundZco,
					BpAncillaryData *ancillaryData,
					int stewardship);
			/*	This function is invoked by a
			 *	convergence-layer output adapter
			 *	(outduct) daemon to get a bundle that
			 *	it is to transmit to some remote
			 *	convergence-layer input adapter (induct)
			 *	daemon.
			 *
			 *	The function first pops the next (only)
			 *	outbound bundle from the queue of
			 *	outbound bundles for the indicated duct.
			 *	If no such bundle is currently waiting
			 *	for transmission, it blocks until one
			 *	is [or until the duct is closed, at
			 *	which time the function returns zero
			 *	without providing the address of an
			 *	outbound bundle ZCO].
			 *
			 *	On obtaining a bundle, bpDequeue
			 *	does DEQUEUE processing on the bundle's
			 *	extension blocks; if this processing
			 *	determines that the bundle is corrupt,
			 *	the function returns zero while
			 *	providing 1 (a nonsense address) in
			 *	*bundleZco as the address of the
			 *	outbound bundle ZCO.  The CLO should
			 *	handle this result by simply calling
			 *	bpDequeue again.
			 *
			 *	bpDequeue then catenates (serializes)
			 *	the BP header information (primary
			 *	block and all extension blocks) in
			 *	the bundle and prepends that serialized
			 *	header to the source data of the
			 *	bundle's payload ZCO.  Then it
			 *	returns the address of that ZCO in
			 *	*bundleZco for transmission at the
			 *	convergence layer (possibly entailing
			 *	segmentation that would be invisible
			 *	to BP).
			 *
			 *	Requested quality of service for the
			 *	bundle is provided in *ancillaryData
			 *	so that the requested QOS can be
			 *	mapped to the QOS features of the
			 *	convergence-layer protocol.  For
			 *	example, this is where a request for
			 *	custody transfer is communicated
			 *	to BIBE when the outduct daemon is
			 *	one that does BIBE transmission.
			 *
			 *	The stewardship argument controls
			 *	the disposition of the bundle
			 *	following transmission.  Any value
			 *	other than zero indicates that the
			 *	outduct daemon is one that performs
			 *	"stewardship" procedures.  An outduct
			 *	daemon that performs stewardship
			 *	procedures will disposition the bundle
			 *	as soon as the results of transmission
			 *	at the convergence layer are known,
			 *	by calling one of two functions:
			 *	either bpHandleXmitSuccess or else
			 *	bpHandleXmitFailure.  A value of
			 *	zero indicates that the outduct
			 *	daemon does not perform stewardship
			 *	procedures and will not disposition
			 *	the bundle following transmission;
			 *	instead, the bpDequeue function itself
			 *	will assume that transmission at the
			 *	convergence layer will be successful
			 *	and will disposition the bundle on
			 *	that basis.
			 *
			 *	Returns 0 on success, -1 on failure.	*/

extern int		bpHandleXmitSuccess(Object zco);
			/*	This function is invoked by a
			 *	convergence-layer output adapter (an
			 *	outduct) on detection of convergence-
			 *	layer protocol transmission success.
			 *	It causes the serialized (catenated)
			 *	outbound bundle in zco to be destroyed,
			 *	unless some constraint (such as local
			 *	delivery of a copy of the bundle)
			 *	requires that bundle destruction
			 *	be deferred.
			 *
			 *	Returns 1 if bundle success was
			 *	handled, 0 if bundle had already
			 *	been destroyed, -1 on system failure.	*/

extern int		bpHandleXmitFailure(Object zco);
			/*	This function is invoked by a
			 *	convergence-layer output adapter (an
			 *	outduct) on detection of a convergence-
			 *	layer protocol transmission error.
			 *	It causes the serialized (catenated)
			 *	outbound bundle in zco to be queued
			 *	up for re-forwarding.
			 *
			 *	Returns 1 if bundle failure was
			 *	handled, 0 if bundle had already
			 *	been destroyed, -1 on system failure.	*/

extern int		bpReforwardBundle(Object bundleToReforward);
			/*	bpReforwardBundle aborts the current
			 *	outduct queuing for the bundle and
			 *	queues it for re-forwarding, possibly
			 *	on a different route.  It is invoked
			 *	when transmission is determined to be
			 *	overdue, indicating an anomaly in the
			 *	originally selected route.
			 *
			 *	Returns 0 on success, -1 on failure.	*/

extern AcqWorkArea	*bpGetAcqArea(VInduct *vduct);
			/*	Allocates a bundle acquisition work
			 *	area for use in acquiring inbound
			 *	bundles via the indicated duct.
			 *
			 *	Returns NULL on any failure.		*/

extern void		bpReleaseAcqArea(AcqWorkArea *workArea);
			/*	Releases dynamically allocated
			 *	bundle acquisition work area.		*/

extern int		bpBeginAcq(	AcqWorkArea *workArea,
					int authentic,
					char *senderEid);
			/*	This function is invoked by a
			 *	convergence-layer input adapter
			 *	to initiate acquisition of a new
			 *	bundle via the indicated workArea.
			 *	It initializes deserialization of
			 *	an array of bytes constituting a
			 *	single transmitted bundle.
			 *
			 *	The "authentic" Boolean and "senderEid"
			 *	string are knowledge asserted by the
			 *	convergence-layer input adapter
			 *	invoking this function: an assertion
			 *	of authenticity of the data being
			 *	acquired (e.g., per knowledge that
			 *	the data were received via a
			 *	physically secure medium) and, if
			 *	non-NULL, an EID characterizing the
			 *	node that send this inbound bundle.
			 *
			 *	Returns 0 on success, -1 on any
			 *	failure.				*/

extern int		bpLoadAcq(	AcqWorkArea *workArea,
					Object zco);
			/*	This function continues acquisition
			 *	of a bundle as initiated by an
			 *	invocation of bpBeginAcq().  To
			 *	do so, it inserts the indicated
			 *	zero-copy object -- containing
			 *	the bundle in concatenated form --
			 *	into workArea.
			 *
			 *	bpLoadAcq is an alternative to
			 *	bpContinueAcq, intended for use
			 *	by convergence-layer adapters that
			 *	natively acquire concatenated
			 *	bundles into zero-copy objects.
			 *
			 *	Returns 0 on success, -1 on any
			 *	failure.				*/

extern int		bpContinueAcq(	AcqWorkArea *workArea,
					char *bytes,
					int length,
					ReqAttendant *attendant,
					unsigned char priority);
			/*	This function continues acquisition
			 *	of a bundle as initiated by an
			 *	invocation of bpBeginAcq().  To
			 *	do so, it appends the indicated array
			 *	of bytes, of the indicated length, to
			 *	the byte array that is encapsulated
			 *	in workArea.
			 *
			 *	bpContinueAcq is an alternative to
			 *	bpLoadAcq, intended for use by
			 *	convergence-layer adapters that
			 *	incrementally acquire portions of
			 *	concatenated bundles into byte-array
			 *	buffers.  The function transparently
			 *	creates a zero-copy object for
			 *	acquisition of the bundle, if one
			 *	does not already exist, and appends
			 *	"bytes" to the source data of that
			 *	ZCO.
			 *
			 * 	The behavior of bpContinueAcq when
			 *	currently available space for zero-
			 *	copy objects is insufficient to
			 *	contain this increment of bundle
			 *	source data depends on the value of
			 *	"attendant".  If "attendant" is NULL,
			 *	then bpContinueAcq will return 0 but
			 *	will flag the acquisition work area
			 *	for refusal of the bundle due to
			 *	resource exhaustion (congestion).
			 *	Otherwise, (i.e., "attendant" points
			 *	to a ReqAttendant structure, which 
			 *	MUST have already been initialized by
			 *	ionStartAttendant()), bpContinueAcq
			 *	will block until sufficient space
			 *	is available or the attendant is
			 *	paused or the function fails,
			 *	whichever occurs first.
			 *
			 *	"priority" is normally zero, but for
			 *	the TCPCL convergence-layer receiver
			 *	threads it is very high (255) because
			 *	any delay in allocating space to an
			 *	extent of TCPCL data delays the
			 *	processing of TCPCL control messages,
			 *	potentially killing TCPCL performance.
			 *	
			 *	Returns 0 on success (even if
			 *	"attendant" was paused or the
			 *	acquisition work area is flagged
			 *	for refusal due to congestion), -1
			 *	on any failure.				*/

extern void		bpCancelAcq(	AcqWorkArea *workArea);
			/*	Cancels acquisition of a new
			 *	bundle via the indicated workArea,
			 *	destroying the bundle acquisition
			 *	ZCO of workArea.			*/

extern int		bpEndAcq(	AcqWorkArea *workArea);
			/*	Concludes acquisition of a new
			 *	bundle via the indicated workArea.
			 *	This function is invoked after the
			 *	convergence-layer input adapter
			 *	has invoked either bpLoadAcq() or
			 *	bpContinueAcq() [perhaps invoking
			 *	the latter multiple times] such
			 *	that all bytes of the transmitted
			 *	bundle are now included in the
			 *	bundle acquisition ZCO of workArea.
			 *
			 *	Returns 1 on success, 0 on any failure
			 *	pertaining only to this bundle, -1 on
			 *	any other (i.e., system) failure.  If
			 *	1 is returned, then the bundle has been
			 *	fully acquired and dispatched (that is,
			 *	queued for delivery and/or forwarding).
			 *	In this case, the invoking convergence-
			 *	layer input adapter should simply
			 *	continue with the next cycle of
			 *	bundle acquisition, i.e., it should
			 *	call bpBeginAcq().
			 *
			 *	If 0 is returned then the failure
			 *	is transient, applying only to the
			 *	bundle that is currently being
			 *	acquired.  In this case, the current
			 *	bundle acquisition has failed but
			 *	BP itself can continue; the invoking
			 *	convergence-layer input adapter
			 *	should simply continue with the next
			 *	cycle of bundle acquisition just as
			 *	if the return code had been 1.		*/

extern int		bpDestroyBundle(Object bundleToDestroy,
					int expired);
			/*	bpDestroyBundle destroys the bundle,
			 *	provided all retention constraints
			 *	have been removed.  "expired" is
			 *	Boolean, set to 1 only by bpClock when
			 *	it destroys a bundle whose TTL has
			 *	expired or by bp_cancel on bundle
			 *	cancellation.  Returns 1 if bundle
			 *	is actually destroyed, 0 if bundle is
			 *	retained because not all constraints
			 *	have been removed, -1 on any error.	*/

extern int		bpInit();
extern void		bpDropVdb();
extern void		bpRaiseVdb();
extern int		bpStart();
extern void		bpStop();
extern int		bpAttach();
extern void		bpDetach();

extern Object		getBpDbObject();
extern BpDB		*getBpConstants();
extern BpVdb		*getBpVdb();

extern void		getCurrentDtnTime(DtnTime *dt);

extern Throttle		*applicableThrottle(VPlan *vplan);

extern unsigned int	guessBundleSize(Bundle *bundle);
extern unsigned int	computeECCC(unsigned int bundleSize);
extern void		computePriorClaims(BpPlan *plan, Bundle *bundle,
				Scalar *priorClaims, Scalar *backlog);

extern int		parseEidString(char *eidString, MetaEid *metaEid,
				VScheme **scheme, PsmAddress *schemeElt);
extern void		restoreEidString(MetaEid *metaEid);
extern int		recordEid(EndpointId *eid, MetaEid *meid, EidMode mode);

#define writeEid(eid, meid)	recordEid(eid, meid, EidNV)
#define noteEid(eid, meid)	recordEid(eid, meid, EidV)
#define jotEid(eid, meid)	recordEid(eid, meid, EidS)

extern void		eraseEid(EndpointId *eid);
extern int		readEid(EndpointId *eid, char **str);

extern int		acquireEid(EndpointId *eid,
				unsigned char **cursor,
				unsigned int *bytesRemaining);
extern uvast		computeBufferCrc(BpCrcType crcType,
				unsigned char *buffer, 
				int bytesToProcess,
				int endOfBlock,
				uvast aggregateCrc,
				uvast *extractedCrc);

extern int		computeZcoCrc(BpCrcType crcType,
				ZcoReader *reader,
				int bytesToProcess,
				uvast *computedCrc,
				uvast *extractedCrc);

extern int		startBpTask(Object cmd, Object cmdparms, int *pid);

extern void		noteStateStats(int stateIdx, Bundle *bundle);
extern void		clearAllStateStats();
extern void		reportAllStateStats();

extern void		findScheme(char *name, VScheme **vscheme,
				PsmAddress *elt);
extern int		addScheme(char *name, char *fwdCmd, char *admAppCmd);
extern int		updateScheme(char *name, char *fwdCmd, char *admAppCmd);
extern int		removeScheme(char *name);
extern int		bpStartScheme(char *name);
extern void		bpStopScheme(char *name);

extern void		findEndpoint(char *schemeName, char *nss,
				VScheme *vscheme, VEndpoint **vpoint,
				PsmAddress *elt);
/*	Note that adding an endpoint is also called "registering".	*/
extern int		addEndpoint(char *endpointName,
				BpRecvRule recvAction, char *recvScript);
extern int		updateEndpoint(char *endpointName,
				BpRecvRule recvAction, char *recvScript);
/*	Removing an endpoint is also called "unregistering".		*/
extern int		removeEndpoint(char *endpointName);

extern void		lookUpEidScheme(EndpointId *eid, VScheme **vscheme);
extern void		lookUpEndpoint(EndpointId *eid, VScheme *vscheme,
				VEndpoint **vpoint);

extern int		serializeEid(EndpointId *eid, unsigned char *buffer);

extern void		findPlan(char *eid, VPlan **vplan, PsmAddress *elt);

extern int		addPlan(char *eid, unsigned int nominalRate);
extern int		updatePlan(char *eid, unsigned int nominalRate);
extern int		removePlan(char *eid);
extern int		bpStartPlan(char *eid);
extern void		bpStopPlan(char *eid);
extern int		bpBlockPlan(char *eid);
extern int		bpUnblockPlan(char *eid);

extern int		setPlanViaEid(char *eid, char *viaEid);
extern int		attachPlanDuct(char *eid, Object outductElt);
extern int		detachPlanDuct(Object outductElt);
extern void		lookupPlan(char *eid, VPlan **vplan);

extern void	        removeBundleFromQueue(Bundle *bundle, BpPlan *plan);

extern void		fetchProtocol(char *name, ClProtocol *clp, Object *elt);
extern int		addProtocol(char *name, int payloadBytesPerFrame,
				int overheadPerFrame, int protocolClass);
extern int		removeProtocol(char *name);
extern int		bpStartProtocol(char *name);
extern void		bpStopProtocol(char *name);

extern void		findInduct(char *protocolName, char *name,
				VInduct **vduct, PsmAddress *elt);
extern int		addInduct(char *protocolName, char *name,
				char *cliCmd);
extern int		updateInduct(char *protocolName, char *name,
				char *cliCmd);
extern int		removeInduct(char *protocolName, char *name);
extern int		bpStartInduct(char *protocolName, char *ductName);
extern void		bpStopInduct(char *protocolName, char *ductName);

extern void		findOutduct(char *protocolName, char *name,
				VOutduct **vduct, PsmAddress *elt);

extern int		addOutduct(char *protocolName, char *name,
				char *cloCmd, unsigned int maxPayloadLength);
extern int		updateOutduct(char *protocolName, char *name,
				char *cloCmd, unsigned int maxPayloadLength);
extern int		removeOutduct(char *protocolName, char *name);
extern int		bpStartOutduct(char *protocolName, char *ductName);
extern void		bpStopOutduct(char *protocolName, char *ductName);

extern Object		insertBpTimelineEvent(BpEvent *newEvent);
extern void		destroyBpTimelineEvent(Object timelineElt);

extern int		decodeBundle(Sdr sdr, Object zco, unsigned char *buf,
				Bundle *image);
extern int		findBundle(char *sourceEid, BpTimestamp *creationTime,
				unsigned int fragmentOffset,
				unsigned int fragmentLength,
				Object *bundleAddr);
extern int		retrieveSerializedBundle(Object bundleZco, Object *obj);

extern int		deliverBundle(Object bundleObj, Bundle *bundle,
				VEndpoint *vpoint);
extern int		forwardBundle(Object bundleObj, Bundle *bundle,
				char *stationEid);

extern int		reverseEnqueue(Object xmitElt, BpPlan *plan,
				int sendToLimbo);

extern int		enqueueToLimbo(Bundle *bundle, Object bundleObj);
extern int		releaseFromLimbo(Object xmitElt, int resume);

extern int		sendStatusRpt(Bundle *bundle);
extern int		parseStatusRpt(BpStatusRpt *rpt, unsigned char *cursor,
				unsigned int unparsedBytes);

extern void		bpPlanTally(VPlan *vplan, unsigned int idx,
				unsigned int size);
extern void		bpXmitTally(unsigned int priority, unsigned int size);

typedef int		(*StatusRptCB)(BpDelivery *, unsigned char *,
				unsigned int);

typedef struct bpsap_st
{
	VEndpoint	*vpoint;
	MetaEid		endpointMetaEid;
	sm_SemId	recvSemaphore;
	int		detain;		/*	Boolean.		*/
} Sap;

extern int		_handleAdminBundles(char *adminEid,
				StatusRptCB handleStatusRpt);
extern int		endpointIsLocal(EndpointId eid);

#ifdef __cplusplus
}
#endif

#endif  /* _BPP_H_ */
