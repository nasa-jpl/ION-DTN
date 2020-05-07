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
#ifndef _BP_H_
#define _BP_H_

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

typedef struct
{
	time_t			seconds;	/*	Epoch 2000.	*/
	unsigned int		count;
} BpTimestamp;

/*	bp_receive timeout values					*/
#define	BP_POLL			(0)	/*	Return immediately.	*/
#define	BP_NONBLOCKING		(0)	/*	Return immediately.	*/
#define BP_BLOCKING		(-1)	/*	Wait forever.		*/

/*	bp_send priority values						*/
#define	BP_BULK_PRIORITY	(0)	/*	Slower.			*/
#define	BP_STD_PRIORITY		(1)	/*	Faster.			*/
#define	BP_EXPEDITED_PRIORITY	(2)	/*	Precedes others.	*/

typedef enum
{
	NoCustodyRequested = 0,
	SourceCustodyOptional,
	SourceCustodyRequired
} BpCustodySwitch;

/*	Status report request flag values				*/
#define BP_RECEIVED_RPT		(1)	/*	00000001		*/
#define BP_CUSTODY_RPT		(2)	/*	00000010		*/
#define BP_FORWARDED_RPT	(4)	/*	00000100		*/
#define BP_DELIVERED_RPT	(8)	/*	00001000		*/
#define BP_DELETED_RPT		(16)	/*	00010000		*/

#ifndef BP_MAX_METADATA_LEN
#define	BP_MAX_METADATA_LEN	(30)
#endif

typedef struct
{
	/*	From Extended Class of Service (ECOS) block.		*/

	unsigned int	dataLabel;	/*	Optional.		*/
	unsigned char	flags;		/*	See below.		*/
	unsigned char	ordinal;	/*	0 to 254 (most urgent).	*/

	/*	From Metadata block.					*/

	unsigned char	metadataType;	/*	See RFC 6258.		*/
	unsigned char	metadataLen;
	unsigned char	metadata[BP_MAX_METADATA_LEN];
#if 0
	/*	From Sequence Number block.				*/

	unsigned long	sequenceNumber;
	struct timeval	arrivalTime;
#endif
} BpAncillaryData;

/*	Extended class-of-service flags.				*/
#define	BP_MINIMUM_LATENCY	(1)	/*	Forward on all routes.	*/
#define	BP_BEST_EFFORT		(2)	/*	Unreliable CL needed.	*/
#define	BP_DATA_LABEL_PRESENT	(4)	/*	Ignore data label if 0.	*/
#define	BP_RELIABLE		(8)	/*	Reliable CL needed.	*/
#define	BP_RELIABLE_STREAMING	(BP_BEST_EFFORT | BP_RELIABLE)

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
	Object		adu;		/*	Zero-copy object.	*/

	unsigned char	metadataType;	/*	See RFC 6258.		*/
	unsigned char	metadataLen;
	unsigned char	metadata[BP_MAX_METADATA_LEN];
} BpDelivery;

extern int		bp_attach();
			/* 	Note that all ION libraries and
			 * 	applications draw memory dynamically,
			 * 	as needed, from a shared pool of ION
			 * 	working memory.  The size of the pool
			 * 	is established when it is first
			 * 	accessed by one of the ION
			 * 	administrative programs, either
			 * 	bpadmin or ltpadmin.
			 *
			 *	Returns 0 on success, -1 on any error.	*/

extern int		bp_agent_is_started();
			/*	Returns 1 if the local BP agent has
			 *	been started and not yet stopped, 0
			 *	otherwise.				*/

extern Sdr		bp_get_sdr();
			/*	Returns the SDR used for BP, to enable
			 *	creation and interrogation of bundle
			 *	payloads (application data units).	*/

extern void		bp_detach();
			/*	Terminates access to local BP agent.	*/

extern int		bp_open(	char *eid,
					BpSAP *ionsapPtr);
			/*	Arguments are:
		 	 *  		name of the endpoint 
			 *		pointer to variable in which
			 *			address of BP service
			 *			access point will be
			 *			returned
			 *
			 * 	Initiates ability to take delivery
			 *	of bundles destined for the indicated
			 *	endpoint and to send bundles whose
			 *	source is the indicated endpoint.
			 *
			 *	Returns 0 on success, -1 on any error.	*/

extern int		bp_open_source(	char *eid,
					BpSAP *ionsapPtr,
					int detain);
			/*	Arguments are:
		 	 *  		name of the endpoint 
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
			 * 	Initiates ability to send bundles whose
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

#define BP_PARSE_QUALITY_OF_SERVICE_USAGE				\
	"<custody-requested>.<priority>[.<ordinal>" 	\
	"[.<unreliable>.<critical>[.<data-label>]]]"

extern int		bp_parse_quality_of_service(	const char *token,
					BpAncillaryData *ancillaryData,
					BpCustodySwitch *custodySwitch,
					int *priority);
			/*  Parses the token string specifying service
			 *  parameters into appropriate service-related 
			 *  arguments to bp_send(), according to the format
			 *  specified in BP_CLASS_OF_SERVICE_USAGE.
			 *
			 *  Returns 1 on success or 0 on parsing failure.
			 *  On failure, no arguments have been modified.*/

extern int		bp_send(	BpSAP sap,
					char *destEid,
					char *reportToEid,
					int lifespan,
					int classOfService,
					BpCustodySwitch custodySwitch,
					unsigned char srrFlags,
					int ackRequested,
					BpAncillaryData *ancillaryData,
					Object adu,
					Object *newBundle);
			/*	Class of service is simply priority
			 *	for now.  If class-of-service flags
			 *	are defined in a future version of
			 *	Bundle Protocol, those flags would
			 *	be OR'd with priority.
			 *
			 *	Extended class of service, if not
			 *	NULL, is used to populate the extended
			 *	class of service block.  Flag values
			 *	are OR'd together.  If this argument
			 *	is NULL, the default flags and ordinal
			 *	values are 0 and there is no data
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

extern int		bp_track(	Object bundleObj,
					Object trackingElt);
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

extern void		bp_untrack(	Object bundleObj,
					Object trackingElt);
			/*	Removes trackingElt from the bundle's
			 *	list of "tracking" references, if it
			 *	is in that list.  Does not delete
			 *	trackingElt itself.			*/

extern int		bp_memo(	Object bundleObj,
					unsigned int interval); 
			/*	Inserts a "custody-acceptance due"
			 *	event into the timeline.  The event
			 *	causes the indicated bundle to be
			 *	re-forwarded if it is still in the
			 *	database (i.e., it has not yet been
			 *	accepted by another custodian) as of
			 *	the time computed by adding interval
			 *	to the current time.			*/

extern int		bp_suspend(	Object bundleObj);
			/*	Suspends transmission of this bundle.	*/

extern int		bp_resume(	Object bundleObj);
			/*	Resumes transmission of this bundle.	*/

extern int		bp_cancel(	Object bundleObj);
			/*	Cancels transmission of this bundle.	*/

extern int		bp_release(	Object bundleObj);
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

#endif	/* _BP_H */
