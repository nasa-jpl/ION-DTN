/*
	bp.h:	definitions supporting applications built on the
		implementation of the Bundle Protocol in the ION
		(Interplanetary Overlay Network) stack.

	Author: Scott Burleigh, JPL

	Modification History:
	Date  Who What

	Copyright (c) 2007, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
									*/

#ifndef BP_H
#define BP_H

#include "platform.h"
#include "psm.h"
#include "sdr.h"
#include "zco.h"
#include "lyst.h"
#include "smlist.h"
#include "ion.h"

#ifdef __cplusplus
extern "C" {
#endif

#define	BP_VERSION		7

typedef struct
{
	uvast			msec;	/*	Since Epoch 2000.	*/
	unsigned int		count;
} BpTimestamp;

typedef enum
{
	NoCRC = 0,
	X25CRC16 = 1,
	CRC32C = 2
} BpCrcType;

/*	bp_receive timeout values					*/
#define	BP_POLL			(0)	/*	Return immediately.	*/
#define	BP_NONBLOCKING		(0)	/*	Return immediately.	*/
#define BP_BLOCKING		(-1)	/*	Wait forever.		*/

/*	bp_send class-of-service values					*/
#define	BP_BULK_PRIORITY	(0)	/*	Slower.			*/
#define	BP_STD_PRIORITY		(1)	/*	Faster.			*/
#define	BP_EXPEDITED_PRIORITY	(2)	/*	Precedes others.	*/

typedef enum
{
	NoCustodyRequested = 0,
	SourceCustodyOptional,
	SourceCustodyRequired
} BpCustodySwitch;

/*	Note: non-zero custody switch indicates request to use
 *	enable custody transfer in the event that BIBE is used
 *	for convergence-layer transmission of the bundle.		*/

/*	Status report request flag values				*/
#define BP_RECEIVED_RPT		(1)	/*	00000001		*/
#define BP_CUSTODY_RPT		(2)	/*	00000010		*/
#define BP_FORWARDED_RPT	(4)	/*	00000100		*/
#define BP_DELIVERED_RPT	(8)	/*	00001000		*/
#define BP_DELETED_RPT		(16)	/*	00010000		*/

/*	Note: BP_CUSTODY_RPT no longer has any effect.			*/

/*	*	*	Scheme structures	*	*	*	*/

#define MAX_SCHEME_NAME_LEN	(15)
#define	MAX_NSS_LEN		(63)
#define	MAX_EID_LEN		(MAX_SCHEME_NAME_LEN + MAX_NSS_LEN + 2)

typedef enum
{
	unknown = 0,
	dtn = 1,
	ipn = 2,
	imc = 3
} SchemeCodeNbr;

/*	EidFormat records the CBOR array cardinality in which an ipn or
 *	imc EID was received or configured, so that the EID can be
 *	re-serialized in the same form rather than transcoded.  RFC 9758
 *	permits both a 2-element ([fqnn, service]) and a 3-element
 *	([allocator, node, service]) encoding of the same EID; transcoding
 *	between them changes the CBOR bytes and breaks any covering BPSec
 *	integrity block.  EidFormatDefault (0) preserves legacy behavior:
 *	derive the encoding from the fully-qualified node number per the
 *	RFC 9758 recommendation (2-element when the allocator is 0, else
 *	3-element).							*/

typedef enum
{
	EidFormatDefault = 0,	/*	Derive from fqnn (legacy).	*/
	EidFormat2Element,	/*	[fqnn, service]			*/
	EidFormat3Element	/*	[allocator, node, service]	*/
} EidFormat;

/*	Scheme objects are used to encapsulate knowledge about how to
 *	forward bundles.  						*/

typedef struct
{
	char		name[MAX_SCHEME_NAME_LEN + 1];
	int		nameLength;
	SchemeCodeNbr	codeNumber;
	SdrObject	fwdCmd;	      /* For starting forwarder. */
	SdrObject	admAppCmd;    /* For starting admin app. */
	SdrObject	forwardQueue; /* SDR list of Bundles */
	SdrObject	endpoints;    /* SDR list of Endpoints */
	SdrObject	bclas;	      /* SDR list of BIBE CLAs */
} Scheme;

typedef struct
{
	SdrObject schemeElt; /* Reference to scheme. */

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

/*	*	*	Objects for managing endpoint IDs.		*/

typedef enum
{
	EidNV = 0,			/*	Non-volatile.		*/
	EidV,				/*	Volatile.		*/
	EidS				/*	Null-terminated string.	*/
} EidMode;

typedef union
{
	SdrObject	nv;		/*	Recorded in SDR heap.	*/
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
	uvast		fqnn;		/*	Fully-qualified nodeNbr	*/
	unsigned long	serviceNbr;
} IpnSSP;

typedef struct
{
	uvast		fqgn;		/*	Fully-qualified groupNo */
	unsigned long	serviceNbr;
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
	unsigned char	eidFormat;	/*	EidFormat; fills padding.*/
	SSP		ssp;
} EndpointId;

typedef struct
{
	char		*eidCopy;	/*	Allocated copy of EID.	*/
	char		*schemeName;
	int		schemeNameLength;
	SchemeCodeNbr	schemeCodeNbr;
	char		*colon;
	char		*nss;
	int		nssLength;
	char		*nodeName;
	char		*delimiter;
	char		*demux;
	uvast		elementNbr;	/*	FQNN or group nbr.	*/
	unsigned long	serviceNbr;
	char		nullEndpoint;	/*	Boolean.		*/
	unsigned char	eidFormat;	/*	EidFormat of the text.	*/
} MetaEid;

/*	*	*	Objects for managing endpoint ID patterns.	*/

typedef struct
{
	uvast		first;
	uvast		last;
} EidpIpnInterval;

typedef struct
{
	Lyst		intervals;	/*	(EidpIpnInterval *)	*/
} EidpIpnRange;

typedef enum
{
	NoValue = 0,
	AnyValue,
	NumValue,
	RangeValue
} EidpIpnComponentType;

typedef union
{
	uvast		number;
	EidpIpnRange	range;
} EidpIpnComponentValue;

typedef struct
{
	EidpIpnComponentType	type;
	EidpIpnComponentValue	value;
} EidpIpnComponent;

typedef struct
{
	EidpIpnComponent	components[3];
} EidpIpnSSP;

typedef struct
{
	char		*any;		/*	Always NULL.		*/
} EidpAnySSP;

typedef union
{
	EidpIpnSSP	ipnSSP;
	EidpAnySSP	anySSP;
} EidpSSP;

/*	Note: EID pattern items may characterize EIDs for schemes
 *	other than "ipn" in the future.  When this happens, the SSPs
 *	of such items will have different structure.
 *
 *	At this time the only supported scheme is "ipn", i.e., 2.
 *	All other schemes are unsupported.
 *
 *	The item EID pattern SSP for a scheme that is known
 *	(i.e., has got a registered scheme code number) but not
 *	supported will be "any SSP", which is encoded as an EID
 *	pattern SSP that is simply a NULL pointer, indicating
 *	that the item matches any EID formed in that known but
 *	unsupported scheme.
 *
 *	The item EID pattern SSP for all schemes that are unknown
 *	(i.e., have no registered scheme code numbers) will likewise
 *	be "any SSP" indicating that the item matches any EID
 *	formed in any unknown scheme.					*/

typedef struct
{
	SchemeCodeNbr	schemeCodeNbr;
	char		*schemeName;
	EidpSSP		ssp;
} EidpItem;

typedef struct
{
	Lyst		items;		/*	(EidpItem *)		*/
} EidPattern;

/*	*	*	Objects for managing blocks.			*/

#ifndef BP_MAX_METADATA_LEN
#define	BP_MAX_METADATA_LEN	(30)
#endif

typedef enum
{
	UnknownBlk = -1,
	PrimaryBlk = 0,
	PayloadBlk = 1,
	PreviousNodeBlk = 6,
	BundleAgeBlk = 7,
	MetadataBlk = 8,
	HopCountBlk = 10,
	BlockIntegrityBlk = 11,
	BlockConfidentialityBlk = 12,
	CustodyTransferBlk = 13,		/*	CTEB per Orange Book	*/
	CompressedReportingBlk = 14,		/*	CREB per Orange Book	*/
	DataLabelBlk = 192,
	QualityOfServiceBlk = 193,
	SnwPermitsBlk = 194,
	ImcDestinationsBlk = 195,
	RGRBlk = 196,
	CGRRBlk = 197
} BpBlockType;

/*	ExtensionSpec provides the specification for producing an
 *	outbound extension block: block definition (identified by
 *	block type number), a formulation tag whose semantics are
 *	block-type-specific, and applicable CRC type.			*/

typedef struct
{
	BpBlockType	type;		/*	Block type		*/
	unsigned char	tag;		/*	Extension-specific	*/
	BpCrcType	crcType;	/*	Type of CRC on block	*/
} ExtensionSpec;

typedef struct
{
	/*	From Quality of Service (QOS) block, formerly the
	 *	Extended Class of Service (ECOS) block.			*/

	unsigned int	dataLabel;	/*	Optional.		*/
	unsigned char	flags;		/*	QoS flags; see below.	*/
	unsigned char	ordinal;	/*	0 to 254 (most urgent). 255 reserved.
				 *	NOTE: ION uses full 8-bit range,
				 *	not 6-bit (0-63) as in IETF ECOS draft. */

	/*	Note that priority is likewise now carried in the QOS
	 *	block.  However, the ION application protocol interface
	 *	preserves separate specification of priority (termed
	 *	"class of service") rather than including it in the
	 *	BpAncillaryData structure.				*/

	/*	From Metadata block.					*/

	unsigned char	metadataType;	/*	See RFC 6258.		*/
	unsigned char	metadataLen;
	unsigned char	metadata[BP_MAX_METADATA_LEN];

	/*	The following elements of BpAncillaryData are provided
	 *	only at the time the bundle is originally sourced.
	 *	These values affect the construction of the bundle;
	 *	they are NOT carried in the bundle itself.  We are
	 *	just using the AncillaryData structure as a convenient
	 *	way to add these features to the API without requiring
	 *	modification of applications built for earlier versions
	 *	of ION.							*/

	/*	An optional array of additional extension blocks
	 *	(beyond the baseline, which is established at compile
	 *	time) that are to be inserted into this bundle.		*/

	ExtensionSpec	*extensions;	/*	Add'l ext. blocks req'd.*/
	int		extensionsCt;	/*	Count of extensions.	*/

	/*	The number identifying the region(s) within which an
	 *	IMC "dispatch" bundle is to be propagated.		*/

	uint32_t	imcRegionNbr;

	/*	A user-asserted "do not fragment" switch, which may
	 *	be useful for network performance analysis purposes.	*/

	unsigned char	doNotFragment;

	/*	Sequence identifier for CBR (Compressed Bundle Reporting).
	 *	Per Orange Book Section 3.2.4:
	 *	  - seqId = 0: destination-specific sequence counters
	 *	  - seqId > 0: global counter independent of destination
	 *	Default is 0 for backward compatibility.		*/

	uvast		cbrSeqId;
} BpAncillaryData;

/*	Quality-of-service flags.					*/
#define	BP_MINIMUM_LATENCY	(1)	/*	Forward on all routes.	*/
#define	BP_BEST_EFFORT		(2)	/*	Unreliable CL okay.	*/
#if 0
#define	BP_DATA_LABEL_PRESENT	(4)	/*	Ignore data label if 0.	*/
#endif
#define	BP_RELIABLE		(8)	/*	Reliable CL okay.	*/
#if 0
#define	BP_RELIABLE_STREAMING	(16)	/*	BSSP mandatory.		*/
#define	BP_BIBE_REQUESTED	(32)	/*	Forward via BIBE.	*/
#endif
#define	BP_CT_REQUESTED		(64)	/*	BIBE must be reliable.	*/

typedef struct bpsap_st		*BpSAP;

typedef enum
{
	BpPayloadPresent = 1,
	BpReceptionTimedOut,
	BpReceptionInterrupted,
	BpEndpointStopped
} BpIndResult;

typedef struct
{
	BpIndResult	result;
	char		*bundleSourceEid;
	BpTimestamp	bundleCreationTime;
	unsigned int	timeToLive;
	int		ackRequested;	/*	(By app.)  Boolean.	*/
	int		adminRecord;	/*	Boolean: 0 = non-admin.	*/
	SdrObject	adu;		/*	Zero-copy object.	*/

	unsigned char	metadataType;	/*	See RFC 6258.		*/
	unsigned char	metadataLen;
	unsigned char	metadata[BP_MAX_METADATA_LEN];
} BpDelivery;

/*	*	*	Public function prototypes	*	*	*/

extern int		bp_attach(void);
			/*	Note that all ION libraries and
			 *	applications draw memory dynamically,
			 *	as needed, from a shared pool of ION
			 *	working memory.  The size of the pool
			 *	is established when it is first
			 *	accessed by one of the ION
			 *	administrative programs, either
			 *	bpadmin or ltpadmin.
			 *
			 *	Returns 0 on success, -1 on any error.	*/

extern int		bp_agent_is_started(void);
			/*	Returns 1 if the local BP agent has
			 *	been started and not yet stopped, 0
			 *	otherwise.				*/

extern Sdr		bp_get_sdr(void);
			/*	Returns the SDR used for BP, to enable
			 *	creation and interrogation of bundle
			 *	payloads (application data units).	*/

extern void		bp_detach(void);
			/*	Terminates access to local BP agent.	*/

extern int		bp_open(	char *eid,
					BpSAP *ionsapPtr);
			/*	Arguments are:
			 *		name of the endpoint
			 *		pointer to variable in which
			 *			address of BP service
			 *			access point will be
			 *			returned
			 *
			 *	Initiates ability to take delivery
			 *	of bundles destined for the indicated
			 *	endpoint and to send bundles whose
			 *	source is the indicated endpoint.
			 *
			 *	Returns 0 on success, -1 on any error.	*/

extern int		bp_open_source(	char *eid,
					BpSAP *ionsapPtr,
					int detain);
			/*	Arguments are:
			 *		name of the endpoint
			 *		pointer to variable in which
			 *			address of BP service
			 *			access point will be
			 *			returned
			 *		indicator as to whether or
			 *			not bundles sourced
			 *			using this BpSAP
			 *			should be detained
			 *			in storage until
			 *			explicitly released
			 *
			 *	Initiates ability to send bundles whose
			 *	source is the indicated endpoint.  If
			 *	"detain" is non-zero then each bundle
			 *	address returned when this BpSAP is
			 *	passed to bp_send will remain valid and
			 *	usable (i.e., the bundle object will
			 *	continue to occupy storage resources)
			 *	until the bundle is explicitly released
			 *	by an invocation of bp_release OR
			 *	the bundle's time to live expires.
			 *
			 *	Returns 0 on success, -1 on any error.	*/

#define BP_PARSE_QUALITY_OF_SERVICE_USAGE            \
	"<custody-requested>[.<priority>[.<ordinal>" \
	"[.<unreliable>[.<critical>[.<data-label>]]]]]"

extern int		bp_parse_quality_of_service(const char *token,
					BpAncillaryData *ancillaryData,
					BpCustodySwitch *custodySwitch,
					int *classOfService);
			/*  Parses the token string specifying service
			 *  parameters into appropriate service-related
			 *  arguments that may be passed to bp_send(),
			 *  according to the format specified in
			 *  BP_QUALITY_OF_SERVICE_USAGE.
			 *
			 *  NOTE that custody request only serves to
			 *  indicate that custody transfer is requested
			 *  in the event that BIBE is used as the
			 *  convergence-layer transmission protocol.
			 *
			 *  Returns 1 on success or 0 on parsing failure.
			 *  On failure, no arguments have been modified.*/

extern int		parseEidString(char *eidString, MetaEid *metaEid,
				VScheme **scheme, PsmAddress *schemeElt);
extern void		clearMetaEid(MetaEid *metaEid);
extern int		recordEid(EndpointId *eid, MetaEid *meid, EidMode mode);

#define writeEid(eid, meid)	recordEid(eid, meid, EidNV)
#define noteEid(eid, meid)	recordEid(eid, meid, EidV)
#define jotEid(eid, meid)	recordEid(eid, meid, EidS)

extern void		eraseEid(EndpointId *eid);
extern void		readEid(EndpointId *eid, char **str);
extern char		*_nullEid(void);

/*	String <-> CBOR-structured EID wrappers; the structured form
 *	is the RFC 9171 / RFC 9758 [uri-code, SSP] used by primary-block
 *	EIDs and (per CCSDS 734.6-O-1 Annex E) by every EID
 *	field in CTEB, CREB, and Compressed Custody / Reporting Signal
 *	Bundle Sequences.  serializeEidString returns bytes written or
 *	-1; acquireEidString returns bytes consumed, 0 on malformed
 *	input, or -1 on internal error.					*/

extern int		serializeEidString(char *eidString,
				unsigned char *buffer);
extern int		acquireEidString(char *eidString,
				size_t eidStrLen,
				unsigned char **cursor,
				unsigned int *bytesRemaining);

extern EidPattern	*createEidPattern(void);
extern void		destroyEidPattern(EidPattern *eidp);
extern int		loadEidPattern(EidPattern *eidp, const char *text);
extern int		eidMatchesPattern(EidPattern *eidp, EndpointId *eid);

extern int		bp_send(	BpSAP sap,
					char *destEid,
					char *reportToEid,
					int lifespan,
					int classOfService,
					BpCustodySwitch custodySwitch,
					unsigned char srrFlags,
					int ackRequested,
					BpAncillaryData *ancillaryData,
					SdrObject adu,
					SdrObject *newBundle);
			/*	ancillaryData, if not *	NULL, is used
			 *	to populate the Quality of service block.
			 *	Flag values are OR'd together.  If this
			 *	argument is NULL, the default flags and
			 *	ordinal values are 0 and there is no data
			 *	label.
			 *
			 *	adu must be a "zero-copy object" as
			 *	returned by ionCreateZco().
			 *
			 *	Returns 1 on success, 0 on user error
			 *	(an invalid argument value), -1 on
			 *	system error.  If 1 is returned, then
			 *	the ADU has been accepted.  If the
			 *	destination EID is "dtn:none" then
			 *	the ADU has been notionally encap-
			 *	sulated in a bundle but the bundle
			 *	has simply been discarded.  Otherwise
			 *	the ADU has been encapsulated in a
			 *	bundle that has been queued for
			 *	forwarding and - if and only if
			 *	"sap" was returned from a call to
			 *	bp_open_source with the "detain"
			 *	flag set to a non-zero value - the
			 *	new bundle's address has been placed
			 *	in newBundle.				*/

extern int bp_track(SdrObject bundleObj, SdrObject trackingElt);
			/*	Adds trackingElt to the list of
			 *	"tracking" references in the bundle.
			 *	trackingElt must be the address of
			 *	an SDR list element -- whose data
			 *	object's content nominally has got
			 *	embedded within itself the address
			 *	of this same bundle -- within some
			 *	list that is privately managed by
			 *	the application.  Upon destruction
			 *	of the bundle that list element
			 *	will automatically be deleted,
			 *	thus removing the bundle from the
			 *	application's privately managed
			 *	list.  This device enables the
			 *	application to keep track of bundles
			 *	that it is operating on without risk
			 *	of inadvertently de-referencing the
			 *	address of a nonexistent bundle.	*/

extern void bp_untrack(SdrObject bundleObj, SdrObject trackingElt);
			/*	Removes trackingElt from the bundle's
			 *	list of "tracking" references, if it
			 *	is in that list.  Does not delete
			 *	trackingElt itself.			*/

extern int bp_memo(SdrObject bundleObj, unsigned int interval);
			/*	Inserts a "custody-acceptance due"
			 *	event into the timeline.  The event
			 *	causes the indicated bundle to be
			 *	re-forwarded if it is still in the
			 *	database (i.e., it has not yet been
			 *	accepted by another custodian) as of
			 *	the time computed by adding interval
			 *	to the current time.
			 *
			 *	NOTE that this function no longer
			 *	has any effect, as custody transfer
			 *	is now performed entirely within
			 *	BIBE.  NO EVENT IS INSERTED INTO
			 *	THE TIMELINE.				*/

extern int bp_suspend(SdrObject bundleObj);
			/*	Suspends transmission of this bundle.	*/

extern int bp_resume(SdrObject bundleObj);
			/*	Resumes transmission of this bundle.	*/

extern int bp_cancel(SdrObject bundleObj);
			/*	Cancels transmission of this bundle.	*/

extern int bp_release(SdrObject bundleObj);
			/*	Terminates detention of this bundle,
			 *	enabling it to be deleted from
			 *	storage when all other retention
			 *	constraints have been removed.		*/

extern int		bp_receive(	BpSAP sap,
					BpDelivery *dlvBuffer,
					int timeoutSeconds);
			/*	The "result" field of the dlvBuffer
			 *	structure will be used to indicate the
			 *	outcome of the data reception activity.
			 *
			 *	If at least one bundle destined for
			 *	the endpoint for which this SAP is
			 *	opened has not yet been delivered
			 *	to the SAP, then the payload of the
			 *	oldest such bundle will be returned in
			 *	dlvBuffer->adu and dlvBuffer->result
			 *	will be set to BpPayloadPresent.  If
			 *	there is no such bundle, bp_receive
			 *	blocks for up to timeoutSeconds while
			 *	waiting for one to arrive.
			 *
			 *	If timeoutSeconds is BP_POLL and no
			 *	bundle is awaiting delivery, or if
			 *	timeoutSeconds is greater than zero but
			 *	no bundle arrives before timeoutSeconds
			 *	have elapsed, then dlvBuffer->result
			 *	will be set to BpReceptionTimedOut.
			 *
			 *	dlvBuffer->result will be set to
			 *	BpReceptionInterrupted in the event
			 *	that the calling process received and
			 *	handled some signal other than SIGALRM
			 *	while waiting for a bundle.
			 *
			 *	The application data unit delivered
			 *	in the data delivery structure, if
			 *	any, will be a "zero-copy object";
			 *	use the zco_receive_XXX functions to
			 *	read the content of the application
			 *	data unit.
			 *
			 *	Be sure to call bp_release_delivery()
			 *	after every successful invocation of
			 *	bp_receive().
			 *
			 *	Returns 0 on success, -1 on any error.	*/

extern void		bp_interrupt(BpSAP);
			/*	Interrupts a bp_receive invocation
			 *	that is currently blocked.  Designed
			 *	to be called from a signal handler;
			 *	for this purpose, the BpSAP may need
			 *	to be retained in a static variable.	*/

extern void		bp_release_delivery(BpDelivery *dlvBuffer,
					int releaseAdu);
			/*	Releases resources allocated to the
			 *	indicated delivery.  releaseAdu
			 *	is a Boolean parameter: if non-zero,
			 *	the ADU ZCO reference in dlvBuffer
			 *	(if any) is destroyed, causing the
			 *	ZCO itself to be destroyed if no
			 *	other references to it remain.		*/

extern void		bp_close(BpSAP sap);
			/*	Terminates access to the bundles
			 *	enqueued for the endpoint cited by
			 *	the indicated service access point.	*/

#ifdef __cplusplus
}
#endif

#endif /* BP_H */
