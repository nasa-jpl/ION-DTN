/*
 	bpP.h:	private definitions supporting the implementation
		of BP (Bundle Protocol) nodes in the ION
		(Interplanetary Overlay Network) stack, including
		scheme-specific forwarders and the convergence-layer
		adapters they rely on.

	Author: Scott Burleigh, JPL

	Modification History:
	Date      Who   What

	Copyright (c) 2005, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
 									*/
#ifndef _BPP_H_
#define _BPP_H_

#include "rfx.h"
#include "ionsec.h"
#include "bp.h"

#ifdef __cplusplus
extern "C" {
#endif

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
 *	sdr_end_xn().  Note also that, if you do check the value
 *	returned by an SDR function while a transaction is open
 *	and it indicates failure, it is NOT necessary to abort
 *	the transaction by calling sdr_cancel_xn(); the transaction
 *	has already been aborted.  It is only necessary to return
 *	a failure indication to invoking code and let sdr_end_xn()
 *	at the top of the function stack clean up the transaction
 *	and return -1 to trigger the applicable failure handling.	*/

#define MIN_PRIMARY_BLK_LENGTH		(23)
#define MAX_CL_PROTOCOL_NAME_LEN	(15)
#define MAX_CL_DUCT_NAME_LEN		(255)
#define	MAX_SCHEME_NAME_LEN		(15)
#define	MAX_NSS_LEN			(63)
#define	MAX_EID_LEN			(MAX_SCHEME_NAME_LEN + MAX_NSS_LEN + 2)
#define MAX_CBHE_NODE_NBR		(16777215)
#define MAX_CBHE_SERVICE_NBR		(32767)

#ifndef	CBHE_SCHEME_NAME
#define CBHE_SCHEME_NAME		"ipn"
#endif

#ifndef	DTN2_SCHEME_NAME
#define DTN2_SCHEME_NAME		"dtn"
#endif

#ifndef	BP_MAX_BLOCK_SIZE
#define BP_MAX_BLOCK_SIZE		(2000)
#endif

/*	A BP "node" is a set of cooperating state machines that
 *	together constitute a single functional point of presence,
 *	residing in a single SDR database, in a DTN-based network.
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
	long		nominalRate;	/*	In bytes per second.	*/
	long		capacity;	/*	Bytes, current second.	*/
	sm_SemId	semaphore;
} Throttle;

typedef struct
{
	Object		text;		/*	Not NULL-terminated.	*/
	unsigned int	textLength;
} BpString;

typedef struct
{
	unsigned short	schemeNameOffset;	/*	In dictionary.	*/
	unsigned short	nssOffset;		/*	In dictionary.	*/
} DtnEid;

typedef struct
{
	unsigned long	nodeNbr;
	unsigned long	serviceNbr;
} CbheEid;

typedef struct
{
	DtnEid		d;
	CbheEid		c;
	char		cbhe;		/*	Boolean.		*/
} EndpointId;

typedef struct
{
	unsigned long	nodeNbr;
	unsigned long	serviceNbr;
#if BP_URI_RFC
	/*	Note that the scheme name and NSS identified here
	 *	are those of the embedded URI inside the EID string.
	 *	For external representation, the embedded URI is
	 *	preceded by the characters "dtn::" -- except that
	 *	the null endpoint is *always* represented as simply
	 *	"dtn:none".						*/
#endif
	char		*schemeName;
	int		schemeNameLength;
	char		*colon;
	char		*nss;
	int		nssLength;
	char		cbhe;		/*	Boolean.		*/
	char		nullEndpoint;	/*	Boolean.		*/
} MetaEid;

typedef struct
{
	Object		bundleXmitElt;
	Object		ductXmitElt;
	Object		bundleObj;
	Object		proxNodeEid;	/*	An SDR string		*/
	Object		destDuctName;	/*	An SDR string		*/
} XmitRef;

/*	For non-fragmentary bundles, and for the first fragmentary
 *	bundle of a fragmented source bundle, fragmentOffset is zero.
 *	For fragmentary bundles other than the first, fragmentOffset
 *	indicates the offset of the fragmentary bundle's payload
 *	from the beginning of the payload of the original source
 *	bundle.								*/

typedef struct
{
	EndpointId	source;		/*	Original sender.	*/
	BpTimestamp	creationTime;
	unsigned long	fragmentOffset;
} BundleId;

typedef struct
{
	unsigned long	length;		/*	initial length of ZCO	*/
	Object		content;	/*	a ZCO reference in SDR	*/
} Payload;

typedef struct
{
	unsigned char	rank;		/*	Order within def array.	*/
	unsigned char	type;		/*	Per definitions array.	*/
	unsigned short	blkProcFlags;	/*	Per BP spec.		*/
	unsigned int	dataLength;	/*	Block content.		*/
	unsigned int	length;		/*	Length of bytes array.	*/
	unsigned int	size;		/*	Size of scratchpad obj.	*/
	Object		object;		/*	Opaque scratchpad.	*/
	Object		eidReferences;	/*	SDR list (may be 0).	*/
	Object		bytes;		/*	Array in SDR heap.	*/
	int		suppressed;
} ExtensionBlock;

/*	Administrative record types	*/
#define	BP_STATUS_REPORT	(1)
#define	BP_CUSTODY_SIGNAL	(2)

/*	Administrative record flags	*/
#define BP_BDL_IS_A_FRAGMENT	(1)	/*	00000001		*/

typedef enum
{
	SrLifetimeExpired = 1,
	SrUnidirectionalLink,
	SrCanceled,
	SrDepletedStorage,
	SrDestinationUnintelligible,
	SrNoKnownRoute,
	SrNoTimelyContact,
	SrBlockUnintelligible
} BpSrReason;

typedef struct
{
	unsigned long	seconds;
	unsigned long	nanosec;
} DtnTime;

typedef struct
{
	BpTimestamp	creationTime;	/*	From bundle's ID.	*/
	unsigned long	fragmentOffset;	/*	From bundle's ID.	*/
	unsigned long	fragmentLength;
	char		*sourceEid;	/*	From bundle's ID.	*/
	unsigned char	isFragment;	/*	Boolean.		*/
	unsigned char	flags;
	BpSrReason	reasonCode;
	DtnTime		receiptTime;
	DtnTime		acceptanceTime;
	DtnTime		forwardTime;
	DtnTime		deliveryTime;
	DtnTime		deletionTime;
} BpStatusRpt;

typedef enum
{
	CtRedundantReception = 3,
	CtDepletedStorage,
	CtDestinationUnintelligible,
	CtNoKnownRoute,
	CtNoTimelyContact,
	CtBlockUnintelligible
} BpCtReason;

typedef struct
{
	BpTimestamp	creationTime;	/*	From bundle's ID.	*/
	unsigned long	fragmentOffset;	/*	From bundle's ID.	*/
	unsigned long	fragmentLength;
	char		*sourceEid;	/*	From bundle's ID.	*/
	unsigned char	isFragment;	/*	Boolean.		*/
	unsigned char	succeeded;	/*	Boolean.		*/
	BpCtReason	reasonCode;
	DtnTime		signalTime;
} BpCtSignal;

/*	The convergence-layer adapter uses the ClDossier structure to
 *	assert its own private knowledge about the bundle: authenticity
 *	and/or EID of sender.						*/

typedef struct
{
	int		authentic;	/*	Boolean.		*/
	BpString	senderEid;
	unsigned long	senderNodeNbr;	/*	If CBHE.		*/
} ClDossier;

/*	Bundle processing flags						*/
#define BDL_IS_FRAGMENT		(1)	/*	00000001		*/
#define BDL_IS_ADMIN		(2)	/*	00000010		*/
#define BDL_DOES_NOT_FRAGMENT	(4)	/*	00000100		*/
#define BDL_IS_CUSTODIAL	(8)	/*	00001000		*/
#define BDL_DEST_IS_SINGLETON	(16)	/*	00010000		*/
#define BDL_APP_ACK_REQUEST	(32)	/*	00100000		*/

/*	Block processing flags						*/
#define BLK_MUST_BE_COPIED	(1)	/*	00000001		*/
#define BLK_REPORT_IF_NG	(2)	/*	00000010		*/
#define BLK_ABORT_IF_NG		(4)	/*	00000100		*/
#define BLK_IS_LAST		(8)	/*	00001000		*/
#define BLK_REMOVE_IF_NG	(16)	/*	00010000		*/
#define BLK_FORWARDED_OPAQUE	(32)	/*	00100000		*/
#define BLK_HAS_EID_REFERENCES	(64)	/*	01000000		*/

/*	Extension locations						*/
#define PRE_PAYLOAD		(0)
#define POST_PAYLOAD		(1)

typedef struct
{
	BundleId	id;

	/*	Stuff in Primary block.					*/

	unsigned long	bundleProcFlags;	/*	Incl. CoS, SRR.	*/
	EndpointId	destination;	/*	...of bundle's ADU	*/
		/*	source of bundle's ADU is in the id field.	*/
	EndpointId	reportTo;
	EndpointId	custodian;
		/*	creation time is in the id field.		*/
	unsigned long	expirationTime;	/*	Time tag in seconds.	*/
	unsigned long	dictionaryLength;
	Object		dictionary;
		/*	fragment offset is in the id field.		*/
	unsigned long	totalAduLength;

	/*	Stuff in Extended COS extension block.			*/

	BpExtendedCOS	extendedCOS;

	/*	Stuff in Payload block.					*/

	unsigned long	payloadBlockProcFlags;
	Payload		payload;

	/*	Stuff in extension blocks, preceding and following the
	 *	payload block: SDR lists of ExtensionBlock objects.
	 *	Each extensionsLength field is the sum of the "length"
	 *	values of all ExtensionBlocks in the corresponding
	 *	extensions list.					*/

	Object		extensions[2];
	int		extensionsLength[2];	/*	Concatenated.	*/

	/*	Internal housekeeping stuff.				*/

	char		custodyTaken;	/*	Boolean.		*/
	char		suspended;	/*	Boolean.		*/
	char		catenated;	/*	Boolean.		*/
	char		returnToSender;	/*	Boolean.		*/
	int		dbOverhead;	/*	SDR bytes occupied.	*/
	int		dbTotal;	/*	Overhead + payload len.	*/
	BpStatusRpt	statusRpt;	/*	For response per CoS.	*/
	BpCtSignal	ctSignal;	/*	For acknowledgement.	*/
	ClDossier	clDossier;	/*	Processing hints.	*/
	Object		stations;	/*	Stack of EIDs (route).	*/

	/*	Database navigation stuff (back-references).		*/

	Object		timelineElt;	/*	TTL expire list ref.	*/
	Object		overdueElt;	/*	Xmit overdue ref.	*/
	Object		ctDueElt;	/*	CT deadline ref.	*/
	Object		fwdQueueElt;	/*	Scheme's queue ref.	*/
	Object		fragmentElt;	/*	Incomplete's list ref.	*/
	Object		dlvQueueElt;	/*	Endpoint's queue ref.	*/
	Object		trackingElts;	/*	List of app. list refs.	*/

	Object		incompleteElt;	/*	Ref. to Incomplete.	*/

	Object		xmitRefs;	/*	SDR list of XmitRefs	*/
	int		xmitsNeeded;
	time_t		enqueueTime;	/*	When queued for xmit.	*/

	Object		inTransitEntry;	/*	Hash table entry.	*/
} Bundle;

#define COS_FLAGS(bundleProcFlags)	((bundleProcFlags >> 7) & 0x7f)
#define SRR_FLAGS(bundleProcFlags)	((bundleProcFlags >> 14) & 0x7f)

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

	unsigned long	totalAduLength;
} IncompleteBundle;

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
} Endpoint;

/*	The volatile endpoint object encapsulates the volatile state
 *	of the corresponding Endpoint.					*/

typedef struct
{
	Object		endpointElt;	/*	Reference to Endpoint.	*/
	char		nss[MAX_NSS_LEN + 1];
	int		appPid;		/*	Consumes dlv notices.	*/
	sm_SemId	semaphore;	/*	For dlv notices.	*/
} VEndpoint;

/*	Scheme objects are used to encapsulate knowledge about how to
 *	forward bundles.  CBHE-conformant schemes are so noted.		*/

typedef struct
{
	char		name[MAX_SCHEME_NAME_LEN + 1];
	int		cbhe;		/*	Boolean.		*/
	Object		fwdCmd; 	/*	For starting forwarder.	*/
	Object		admAppCmd; 	/*	For starting admin app.	*/
	Object		forwardQueue;	/*	SDR list of Bundles	*/
	Object		endpoints;	/*	SDR list of Endpoints	*/
} Scheme;

typedef struct
{
	Object		schemeElt;	/*	Reference to scheme.	*/
	char		name[MAX_SCHEME_NAME_LEN + 1];
	char		custodianEidString[MAX_EID_LEN];
	int		custodianSchemeNameLength;
	int		custodianNssLength;
	int		cbhe;		/*	Copied from Scheme.	*/
	int		fwdPid;		/*	For stopping forwarder.	*/
	int		admAppPid;	/*	For stopping admin app.	*/
	sm_SemId	semaphore;	/*	For dispatch notices.	*/
	PsmAddress	endpoints;	/*	SM list: VEndpoint.	*/
} VScheme;

typedef struct
{
	char		name[MAX_CL_DUCT_NAME_LEN + 1];
	Object		cliCmd;		/*	For starting the CLI.	*/
	Object		protocol;	/*	back-reference		*/
} Induct;

typedef struct
{
	Object		inductElt;	/*	Reference to Induct.	*/
	char		protocolName[MAX_CL_PROTOCOL_NAME_LEN + 1];
	char		ductName[MAX_CL_DUCT_NAME_LEN + 1];
	int		cliPid;		/*	For stopping the CLI.	*/
	Throttle	acqThrottle;	/*	For congestion control.	*/
} VInduct;

typedef struct
{
	Scalar		backlog;
	Object		lastForOrdinal;	/*	SDR list element.	*/
} OrdinalState;

typedef struct
{
	char		name[MAX_CL_DUCT_NAME_LEN + 1];
	Object		cloCmd;		/*	For starting the CLO.	*/
	Object		bulkQueue;	/*	SDR list of XmitRefs	*/
	Scalar		bulkBacklog;	/*	Bulk bytes enqueued.	*/
	Object		stdQueue;	/*	SDR list of XmitRefs	*/
	Scalar		stdBacklog;	/*	Std bytes enqueued.	*/
	Object		urgentQueue;	/*	SDR list of XmitRefs	*/
	Scalar		urgentBacklog;	/*	Urgent bytes enqueued.	*/
	OrdinalState	ordinals[256];	/*	Orders urgent queue.	*/
	Object		protocol;	/*	back-reference		*/
	int		blocked;	/*	Boolean			*/
} Outduct;

typedef struct
{
	Object		outductElt;	/*	Reference to Outduct.	*/
	char		protocolName[MAX_CL_PROTOCOL_NAME_LEN + 1];
	char		ductName[MAX_CL_DUCT_NAME_LEN + 1];
	int		cloPid;		/*	For stopping the CLO.	*/
	sm_SemId	semaphore;	/*	For transmit notices.	*/
	Throttle	xmitThrottle;	/*	For rate control.	*/
} VOutduct;

typedef struct
{
	char		name[MAX_CL_PROTOCOL_NAME_LEN + 1];
	int		payloadBytesPerFrame;
	int		overheadPerFrame;
	long		nominalRate;	/*	Bytes per second.	*/
	Object		inducts;	/*	SDR list of Inducts	*/
	Object		outducts;	/*	SDR list of Outducts	*/
} ClProtocol;

typedef struct
{
	char		*protocolName;
	char		*proxNodeEid;
} DequeueContext;

#define	EPOCH_2000_SEC	946684800

typedef enum
{
	expiredTTL = 1,
	xmitOverdue = 2,
	ctDue = 3
} BpEventType;

typedef struct
{
	BpEventType	type;
	time_t		time;		/*	as from time(2)		*/
	Object		ref;		/*	Bundle, etc.		*/
} BpEvent;

typedef struct
{
	Object		schemes;	/*	SDR list of Schemes	*/
	Object		protocols;	/*	SDR list of ClProtocols	*/
	Object		timeline;	/*	SDR list of BpEvents	*/
	Object		inTransitHash;	/*	SDR hash of Bundles	*/
	Object		inboundBundles;	/*	SDR list of ZCOs	*/
	Object		limboQueue;	/*	SDR list of XmitRefs	*/
	Object		clockCmd; 	/*	For starting clock.	*/
	BpString	custodianEidString;
	int		maxAcqInHeap;
} BpDB;

/*	Volatile database encapsulates the volatile state of the
 *	database.							*/

typedef struct
{
	unsigned long	bytes;		/*	Of payload data.	*/
	unsigned int	bundles;	/*	Count.			*/
} BpClassStats;


typedef struct
{
	BpClassStats	stats[4];	/*	Indexed by priority.	*/
} BpStateStats;

#define	BPSTATS_SOURCE	(0)
#define	BPSTATS_FORWARD	(1)
#define	BPSTATS_XMIT	(2)
#define	BPSTATS_RECEIVE	(3)
#define	BPSTATS_DELIVER	(4)
#define	BPSTATS_REFUSE	(5)
#define	BPSTATS_TIMEOUT	(6)
#define	BPSTATS_EXPIRE	(7)

/*	"Watch" switches for bundle protocol operation.			*/
#define	WATCH_a			(1)
#define	WATCH_b			(2)
#define	WATCH_c			(4)
#define	WATCH_m			(8)
#define	WATCH_w			(16)
#define	WATCH_x			(32)
#define	WATCH_y			(64)
#define	WATCH_z			(128)
#define	WATCH_abandon		(256)
#define	WATCH_expire		(512)
#define	WATCH_refusal		(1024)
#define	WATCH_timeout		(2048)
#define	WATCH_limbo		(4096)
#define	WATCH_delimbo		(8192)

typedef struct
{
	unsigned long	creationTimeSec;
	int		bundleCounter;
	int		clockPid;	/*	For stopping clock.	*/
	int		watching;	/*	Activity watch switch.	*/

	/*	For congestion control.					*/

	Throttle	productionThrottle;

	/*	For finding structures in database.			*/

	PsmAddress	schemes;	/*	SM list: VScheme.	*/
	PsmAddress	cbheScheme;	/*	A single VScheme.	*/
	PsmAddress	inducts;	/*	SM list: VInduct.	*/
	PsmAddress	outducts;	/*	SM list: VOutduct.	*/

	/*	For monitoring network performance.			*/

	BpStateStats	stateStats[8];
	time_t		xmitStartTime;	/*	Transmitted.		*/
	time_t		recvStartTime;	/*	Received, delivered.	*/
	time_t		statsStartTime;	/*	Sourced, forwarded.	*/
} BpVdb;

typedef struct
{
	unsigned char	type;		/*	Per extensions array.	*/
	unsigned short	blkProcFlags;	/*	Per BP spec.		*/
	unsigned int	dataLength;	/*	Block content.		*/
	unsigned int	length;		/*	Length of bytes array.	*/
	unsigned int	size;		/*	Size of scratchpad obj.	*/
	void		*object;	/*	Opaque scratchpad.	*/
	Lyst		eidReferences;	/*	May be NULL.		*/
	unsigned char	bytes[1];
} AcqExtBlock;

typedef enum
{
	AcqTBD = 0,
	AcqOK,
	AcqNG
} AcqDecision;

typedef struct
{
	VInduct		*vduct;

	/*	Per-bundle state variables.				*/

	Bundle		bundle;
	int		headerLength;
	int		trailerLength;
	int		bundleLength;
	int		authentic;	/*	Boolean.		*/
	char		*dictionary;
	Lyst		extBlocks[2];	/*	(AcqExtBlock *)		*/
	int		currentExtBlocksList;	/*	0 or 1.		*/
	AcqDecision	decision;
	int		lastBlockParsed;
	int		malformed;
	int		mustAbort;	/*	Unreadable block(s).	*/

	/*	Per-acquisition state variables.			*/

	int		allAuthentic;	/*	Boolean.		*/
	char		*senderEid;
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

/*	Definitions supporting route computation.			*/

typedef enum
{
	fwd = 1,	/*	Forward via indicated EID.		*/
	xmit = 2	/*	Transmit in indicated CL duct.		*/
} FwdAction;

typedef struct
{
	FwdAction	action;
	Object		outductElt;	/*	sdrlist elt for xmit	*/
	Object		destDuctName;	/*	sdrstring for xmit	*/
	Object		eid;		/*	sdrstring for fwd	*/
} FwdDirective;

/*	Definitions supporting determination of sender endpoint ID.	*/

typedef int		(*BpEidLookupFn)(char *uriBuffer, char *neighborClId);

extern BpEidLookupFn	*senderEidLookupFunctions(BpEidLookupFn fn);
extern void		getSenderEid(char **eidBuffer, char *neighborClId);
extern int		clIdMatches(char *neighborClId, FwdDirective *dir);

/*	Definitions supporting the use of Bundle Protocol extensions.	*/

typedef int		(*BpExtBlkOfferFn)(ExtensionBlock *, Bundle *);
typedef void		(*BpExtBlkReleaseFn)(ExtensionBlock *);
typedef int		(*BpExtBlkRecordFn)(ExtensionBlock *, AcqExtBlock *);
typedef int		(*BpExtBlkCopyFn)(ExtensionBlock *, ExtensionBlock *);
typedef int		(*BpExtBlkProcessFn)(ExtensionBlock *, Bundle *, void*);
typedef int		(*BpAcqExtBlkAcquireFn)(AcqExtBlock *, AcqWorkArea *);
typedef int		(*BpAcqExtBlkCheckFn)(AcqExtBlock *, AcqWorkArea *);
typedef void		(*BpAcqExtBlkClearFn)(AcqExtBlock *);

extern void		getSenderEid(char **eidBuffer, char *neighborClId);

#define	PROCESS_ON_FORWARD	0
#define	PROCESS_ON_TAKE_CUSTODY	1
#define	PROCESS_ON_ENQUEUE	2
#define	PROCESS_ON_DEQUEUE	3
#define	PROCESS_ON_TRANSMIT	4

typedef struct
{
	char			name[32];
	unsigned char		type;
	unsigned char		listIdx;	/*	0 or 1		*/
	BpExtBlkOfferFn		offer;
	BpExtBlkReleaseFn	release;
	BpAcqExtBlkAcquireFn	acquire;
	BpAcqExtBlkCheckFn	check;
	BpExtBlkRecordFn	record;
	BpAcqExtBlkClearFn	clear;
	BpExtBlkCopyFn		copy;
	BpExtBlkProcessFn	process[5];
} ExtensionDef;

extern void		discardExtensionBlock(AcqExtBlock *blk);
extern int		serializeExtBlk(ExtensionBlock *blk,
					Lyst eidReferences,
					char *blockData);
extern void		scratchExtensionBlock(ExtensionBlock *blk);
extern void		suppressExtensionBlock(ExtensionBlock *blk);
extern void		restoreExtensionBlock(ExtensionBlock *blk);
extern Object		findExtensionBlock(Bundle *bundle, unsigned int type,
					unsigned int listIdx);

/*	Definitions supporting the use of QoS-sensitive bandwidth
 *	management.							*/

/*	Outflow objects are non-persistent.  They are used only to
 *	do QoS-sensitive bandwidth management.  Since they are private
 *	to CLOs, they can reside in the private memory of the CLO
 *	(rather than in shared memory).
 *
 *	Each Outduct, of whatever CL protocol, has three Outflows:
 *
 *		Expedited = 2, Standard = 1, and Bulk = 0.
 *
 * 	Expedited traffic goes out before any other.  Standard traffic
 * 	(service factor = 2) goes out twice as fast as Bulk traffic
 * 	(service factor = 1).                 				*/

#define EXPEDITED_FLOW   2

typedef struct
{
	Object		outboundBundles;	/*	SDR list.	*/
	int		totalBytesSent;
	int		svcFactor;
} Outflow;

/*	*	*	Function prototypes.	*	*	*	*/

extern int		bpSend(		MetaEid *sourceMetaEid,
					char *destEid,
					char *reportToEid,
					int lifespan,
					int classOfService,
					BpCustodySwitch custodySwitch,
					unsigned char srrFlags,
					int ackRequested,
					BpExtendedCOS *extendedCOS,
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
			 *	1 for status reports and custody
			 *	signals but zero otherwise.
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
					Bundle *bundle);
			/*	This is the common processing for any
			 *	bundle that a forwarder decides it
			 *	cannot accept for forwarding.  It 
			 *	sends any applicable status reports
			 *	and then deletes the bundle from
			 *	local storage.
			 *
			 *	Call this function at most once per
			 *	bundle.	 Returns 0 on success, -1 on
			 *	any failure.				*/

extern int		bpAccept(	Bundle *bundle);
			/*	This is the common processing for any
			 *	bundle that a forwarder decides it
			 *	can accept for forwarding, whether
			 *	the bundle was sourced locally or
			 *	was received from some other node.
			 *	It updates statistics that are used
			 *	to make future bundle acquisition
			 *	decisions; if custody transfer is
			 *	requested, it takes custody of the
			 *	bundle; and it sends any applicable
			 *	status reports.
			 *
			 *	Call this function at most once per
			 *	bundle.	 Returns 0 on success, -1 on
			 *	any failure.				*/

extern int		bpEnqueue(	FwdDirective *directive,
					Bundle *bundle,
					Object bundleObj,
					char *proxNodeEid);
			/*	This function is invoked by a forwarder
			 *	to enqueue a bundle for transmission
			 *	by a convergence-layer output adapter
			 *	to the proximate node identified by
			 *	proxNodeEid.  It appends the indicated
			 *	bundle to the appropriate transmission
			 *	queue of the duct indicated by
			 *	"directive" based on the bundle's
			 *	priority.  If the bundle is destined
			 *	for a specific location among all the
			 *	locations that are reachable via this
			 *	duct, then the directive' destDuctName
			 *	must be a string identifying that
			 *	location.
			 *
			 *	If forwarding the bundle to multiple
			 *	nodes (flooding or multicast), call
			 *	this function once per planned
			 *	transmission.
			 *
			 *	Returns 0 on success, -1 on any
			 *	failure.				*/

extern int		bpDequeue(	VOutduct *vduct,
					Outflow *outflows,
					Object *outboundZco,
					BpExtendedCOS *extendedCOS,
					char *destDuctName,
					int stewardshipAccepted);
			/*	This function is invoked by a
			 *	convergence-layer output adapter (an
			 *	outduct) to get a bundle that it is to
			 *	transmit to some remote convergence-
			 *	layer input adapter (induct).
			 *
			 *	The function first selects the next
			 *	outbound bundle from the set of outduct
			 *	bundle queues identified by outflows.
			 *	If no such bundle is currently waiting
			 *	for transmission, it blocks until one
			 *	is [or until a signal handler calls
			 *	bp_interrupt()].
			 *
			 *	Then, if the outduct is rate-controlled,
			 *	bpDequeue blocks until the capacity
			 *	of the outduct's xmitThrottle is non-
			 *	negative.  In this way BP imposes rate
			 *	control on outbound traffic, limiting
			 *	transmission rate to the nominal data
			 *	rate of the outduct.
			 *
			 *	Then bpDequeue catenates (serializes)
			 *	the BP block information in the bundle
			 *	and prepends that serialized block to
			 *	the source data of the bundle's
			 *	payload ZCO.  Then it returns the
			 *	address of that ZCO in *bundleZco
			 *	for transmission at the convergence
			 *	layer (possibly entailing segmentation
			 *	that would be invisible to BP).
			 *
			 *	The extended class of service for the
			 *	bundle is provided in *extendedCOS
			 *	so that the requested QOS can be
			 *	mapped to the QOS features of the
			 *	convergence-layer protocol.
			 *
			 *	If this bundle is destined for a
			 *	specific location among all the
			 *	locations that are reachable via
			 *	this duct, then a string identifying
			 *	that location is written into
			 *	destDuctName, which must be an array
			 *	of at least MAX_CL_DUCT_NAME_LEN + 1
			 *	bytes.
			 *
			 *	The stewardshipAccepted argument, a
			 *	Boolean, indicates whether or not
			 *	the calling function commits to
			 *	dispositioning the returned bundle
			 *	when the results of convergence-
			 *	layer transmission are known, by
			 *	calling one of two functions:
			 *	either bpHandleXmitSuccess or else
			 *	bpHandleXmitFailure.
			 *
			 *	Returns 0 on success, -1 on failure.	*/

extern int		bpIdentify(Object bundleZco, Object *bundleObj);
			/*	This function parses out the ID fields
			 *	of the catenated outbound bundle in
			 *	bundleZco, locates the bundle that
			 *	is identified by that bundle ID, and
			 *	passes that bundle's address back in
			 *	bundleObj to enable the insertion
			 *	of a custody timeout event via bpMemo.
			 *
			 *	bpIdentify allocates a temporary
			 *	buffer of size BP_MAX_BLOCK_SIZE
			 *	into which the initial block(s) of
			 *	the concatenated bundle identified
			 *	by bundleZco are read for parsing.
			 *	If that buffer is not long enough
			 *	to contain the entire primary block
			 *	of the bundle, bundle ID field
			 *	extraction will be incomplete and
			 *	the bundle will not be located.
			 *
			 *	If the bundle is not located, the
			 *	address returned in *bundleObj will
			 *	be zero.
			 *
			 *	Returns 0 on success (whether bundle
			 *	was located or not), -1 on system
			 *	failure.				*/

extern int		bpMemo(Object bundleObj, int interval); 
			/*	This function inserts a "custody-
			 *	acceptance due" event into the
			 *	timeline.  The event causes bpclock
			 *	to re-forward the indicated bundle if
			 *	it is still in the database (i.e., it
			 *	has not yet been accepted by another
			 *	custodian) as of the moment computed
			 *	by adding the indicated interval to
			 *	the current time.
			 *
			 *	Returns 0 on success, -1 on failure.	*/

extern int		bpHandleXmitSuccess(Object zco);
			/*	This function is invoked by a
			 *	convergence-layer output adapter (an
			 *	outduct) on detection of convergence-
			 *	layer protocol transmission success.
			 *	It causes the serialized (catenated)
			 *	outbound custodial bundle in zco to
			 *	be destroyed, unless some constraint
			 *	(such as acceptance of custody)
			 *	requires that bundle destruction be
			 *	deferred.
			 *
			 *	Returns 0 on success, -1 on failure.	*/

extern int		bpHandleXmitFailure(Object zco);
			/*	This function is invoked by a
			 *	convergence-layer output adapter (an
			 *	outduct) on detection of a convergence-
			 *	layer protocol transmission error.
			 *	It causes the serialized (catenated)
			 *	outbound custodial bundle in zco to
			 *	be queued up for re-forwarding.  In
			 *	effect, this function implements
			 *	custodial retransmission due to
			 *	"timeout" - that is, convergence-layer
			 *	protocol failure - rather than due
			 *	to explicit refusal of custody.
			 *
			 *	Returns 0 on success, -1 on failure.	*/

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
					int length);
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
			 *	Returns 0 on success, -1 on any
			 *	failure.				*/

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
			 *	cancellation.  Returns 1 if bundlex
			 *	is actually destroyed, 0 if bundle is
			 *	retained because not all constraints
			 *	have been removed, -1 on any error.	*/

extern int		bpConstructStatusRpt(BpStatusRpt *rpt,
					Object *payloadZco);
			/*	Catenates (serializes) rpt as the
			 *	source data of a new ZCO and passes
			 *	back the address of that new ZCO.
			 *
			 *	Returns 0 on success, -1 on any error.	*/

extern void		bpEraseStatusRpt(BpStatusRpt *rpt);
			/*	Frees any dynamic memory used in
			 *	expressing this status report.		*/

extern int		bpConstructCtSignal(BpCtSignal *signal,
					Object *payloadZco);
			/*	Catenates (serializes) signal as the
			 *	source data of a new ZCO and passes
			 *	back the address of that new ZCO.
			 *
			 *	Returns 0 on success, -1 on any error.	*/

extern void		bpEraseCtSignal(BpCtSignal *signal);
			/*	Frees any dynamic memory used in
			 *	expressing this custody transfer signal.*/

extern int		bpParseAdminRecord(int *adminRecordType,
					BpStatusRpt *rpt,
					BpCtSignal *signal,
					Object payload);
			/*	Populates the appropriate structure
			 *	from payload content and notes the
			 *	corresponding admin record type (0
			 *	if payload is not an admin record).
			 *
			 *	Returns 1 on success, 0 on parsing
			 *	failure, -1 on any other error.		*/

extern int		bpInit();
extern int		bpStart();
extern void		bpStop();
extern int		bpAttach();

extern Object		getBpDbObject();
extern BpDB		*getBpConstants();
extern BpVdb		*getBpVdb();

extern void		getCurrentDtnTime(DtnTime *dt);

extern int		guessBundleSize(Bundle *bundle);
extern int		computeECCC(int bundleSize, ClProtocol *protocol);
extern void		computeApplicableBacklog(Outduct *, Bundle *, Scalar *);

extern int		putBpString(BpString *bpString, char *string);
extern char		*getBpString(BpString *bpString);

extern char		*retrieveDictionary(Bundle *bundle);
extern void		releaseDictionary(char *dictionary);

extern int		parseEidString(char *eidString, MetaEid *metaEid,
				VScheme **scheme, PsmAddress *schemeElt);
extern void		restoreEidString(MetaEid *metaEid);
extern int		printEid(EndpointId *eid, char *dictionary, char **str);

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

extern void		fetchProtocol(char *name, ClProtocol *clp, Object *elt);
extern int		addProtocol(char *name, int payloadBytesPerFrame,
				int overheadPerFrame, long nominalRate);
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
				char *cloCmd);
extern int		updateOutduct(char *protocolName, char *name,
				char *cloCmd);
extern int		removeOutduct(char *protocolName, char *name);
extern int		bpStartOutduct(char *protocolName, char *ductName);
extern void		bpStopOutduct(char *protocolName, char *ductName);
extern int		bpBlockOutduct(char *protocolName, char *ductName);
extern int		bpUnblockOutduct(char *protocolName, char *ductName);

extern Object		insertBpTimelineEvent(BpEvent *newEvent);

extern int		findBundle(char *sourceEid, BpTimestamp *creationTime,
				unsigned long fragmentOffset,
				unsigned long fragmentLength,
				Object *bundleAddr, Object *timelineElt);
extern int		retrieveInTransitBundle(Object bundleZco, Object *obj);

extern int		forwardBundle(Object bundleObj, Bundle *bundle,
				char *stationEid);

extern int		reverseEnqueue(Object xmitElt, ClProtocol *protocol,
				Object outductObj, Outduct *outduct);

extern int		enqueueToLimbo(Bundle *bundle, Object bundleObj);
extern int		releaseFromLimbo(Object xmitElt, int resume);

extern int		sendStatusRpt(Bundle *bundle, char *dictionary);

typedef int		(*StatusRptCB)(BpDelivery *, BpStatusRpt *);
typedef int		(*CtSignalCB)(BpDelivery *, BpCtSignal *);

typedef struct bpsap_st
{
	VEndpoint	*vpoint;
	MetaEid		endpointMetaEid;
	sm_SemId	recvSemaphore;
} Sap;

extern int		_handleAdminBundles(char *adminEid,
				StatusRptCB handleStatusRpt,
				CtSignalCB handleCtSignal);

#ifdef __cplusplus
}
#endif

#endif  /* _BPP_H_ */
