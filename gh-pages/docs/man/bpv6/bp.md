# NAME

bp - Bundle Protocol communications library

# SYNOPSIS

    #include "bp.h"

    [see description for available functions]

# DESCRIPTION

The bp library provides functions enabling application software to use
Bundle Protocol to send and receive information over a delay-tolerant
network.  It conforms to the Bundle Protocol specification as documented
in Internet RFC 5050.

- int bp\_attach( )

    Attaches the application to BP functionality on the local computer.  Returns
    0 on success, -1 on any error.

    Note that all ION libraries and applications draw memory dynamically, as
    needed, from a shared pool of ION working memory.  The size of the pool is
    established when ION node functionality is initialized by ionadmin(1).  This
    is a precondition for initializing BP functionality by running bpadmin(1),
    which in turn is required in order for bp\_attach() to succeed.

- Sdr bp\_get\_sdr( )

    Returns handle for the SDR data store used for BP, to enable creation and
    interrogation of bundle payloads (application data units).

- void bp\_detach( )

    Terminates all access to BP functionality on the local computer.

- int bp\_open(char \*eid, BpSAP \*ionsapPtr)

    Opens the application's access to the BP endpoint identified by _eid_,
    so that the application can take delivery of bundles destined for the
    indicated endpoint.  This SAP can also be used for sending bundles whose
    source is the indicated endpoint; all bundles sent via this SAP will be
    subject to immediate destruction upon transmission, i.e., no bundle
    addresses will be returned by bp\_send() for use in tracking,
    suspending/resuming, or cancelling transmission of these bundles.  On
    success, places a value in _\*ionsapPtr_ that can be supplied to future
    bp function invocations and returns 0.  Returns -1 on any error.

- int bp\_open\_source(char \*eid, BpSAP \*ionsapPtr, detain)

    Opens the application's access to the BP endpoint identified by _eid_,
    so that the application can send bundles whose source is the indicated
    endpoint.  If and only if the value of _detain_ is non-zero, citing this
    SAP in an invocation of bp\_send() will cause the address of the newly
    issued bundle to be returned for use in tracking, suspending/resuming, or
    cancelling transmission of this bundle.  **USE THIS FEATURE WITH GREAT CARE:**
    such a bundle will continue to occupy storage resources until it is
    explicitly released by an invocation of bp\_release() or until its time to
    live expires, so bundle detention increases the risk of resource exhaustion.
    (If the value of _detain_ is zero, all bundles sent via this SAP will be
    subject to immediate destruction upon transmission.)  On success, places a
    value in _\*ionsapPtr_ that can be supplied to future bp function invocations
    and returns 0.  Returns -1 on any error.

- int bp\_send(BpSAP sap, char \*destEid, char \*reportToEid, int lifespan, int classOfService, BpCustodySwitch custodySwitch, unsigned char srrFlags, int ackRequested, BpAncillaryData \*ancillaryData, Object adu, Object \*newBundle)

    Sends a bundle to the endpoint identified by _destEid_, from the
    source endpoint as provided to the bp\_open() call that returned _sap_.
    When _sap_ is NULL, the transmitted bundle is anonymous, i.e., the source
    of the bundle is not identified.  This is legal, but anonymous bundles cannot
    be uniquely identified; custody transfer and status reporting therefore cannot
    be requested for an anonymous bundle.

    _reportToEid_ identifies the endpoint to which any status reports
    pertaining to this bundle will be sent; if NULL, defaults to the
    source endpoint.

    _lifespan_ is the maximum number of seconds that the bundle can remain
    in-transit (undelivered) in the network prior to automatic deletion.

    _classOfService_ is simply priority for now: BP\_BULK\_PRIORITY,
    BP\_STD\_PRIORITY, or BP\_EXPEDITED\_PRIORITY.  If class-of-service flags
    are defined in a future version of Bundle Protocol, those flags would be
    OR'd with priority.

    _custodySwitch_ indicates whether or not custody transfer is requested for
    this bundle and, if so, whether or not the source node itself is required
    to be the initial custodian.  The valid values are SourceCustodyRequired,
    SourceCustodyOptional, NoCustodyRequired.  Note that custody transfer is
    possible only for bundles that are uniquely identified, so it cannot be
    requested for bundles for which BP\_MINIMUM\_LATENCY is requested, since
    BP\_MINIMUM\_LATENCY may result in the production of multiple identical
    copies of the same bundle.  Similarly, custody transfer should never be
    requested for a "loopback" bundle, i.e., one whose destination node is
    the same as the source node: the received bundle will be identical to the
    source bundle, both residing in the same node, so no custody acceptance
    signal can be applied to the source bundle and the source bundle will
    remain in storage until its TTL expires.

    _srrFlags_, if non-zero, is the logical OR of the status reporting behaviors
    requested for this bundle: BP\_RECEIVED\_RPT, BP\_CUSTODY\_RPT, BP\_FORWARDED\_RPT,
    BP\_DELIVERED\_RPT, BP\_DELETED\_RPT.

    _ackRequested_ is a Boolean parameter indicating whether or not the recipient
    application should be notified that the source application requests some sort
    of application-specific end-to-end acknowledgment upon receipt of the bundle.

    _ancillaryData_, if not NULL, is used to populate the Extended Class Of
    Service block for this bundle.  The block's _ordinal_ value is used to
    provide fine-grained ordering within "expedited" traffic: ordinal values
    from 0 (the default) to 254 (used to designate the most urgent traffic)
    are valid, with 255 reserved for custody signals.  The value of the block's
    _flags_ is the logical OR of the applicable extended class-of-service flags:

    >     BP\_MINIMUM\_LATENCY designates the bundle as "critical" for the
    >     purposes of Contact Graph Routing.
    >
    >     BP\_BEST\_EFFORT signifies that non-reliable convergence-layer protocols, as
    >     available, may be used to transmit the bundle.  Notably, the bundle may be
    >     sent as "green" data rather than "red" data when issued via LTP.
    >
    >     BP\_DATA\_LABEL\_PRESENT signifies whether or not the value of _dataLabel_
    >     in _ancillaryData_ must be encoded into the ECOS block when the bundle is
    >     transmitted.

    _adu_ is the "application data unit" that will be conveyed as the payload
    of the new bundle.  _adu_ must be a "zero-copy object" (ZCO).  To ensure
    orderly access to transmission buffer space for all applications, _adu_
    must be created by invoking ionCreateZco(), which may be configured either
    to block so long as insufficient ZCO storage space is available for creation
    of the requested ZCO or to fail immediately if insufficient ZCO storage space
    is available.

    The function returns 1 on success, 0 on user error, -1 on any system
    error.  If 0 is returned, then an invalid argument value was passed to
    bp\_send(); a message to this effect will have been written to the log file.
    If 1 is returned, then either the destination of the bundle was
    "dtn:none" (the bit bucket) or the ADU has been accepted and queued for
    transmission in a bundle.  In the latter case, if and only if _sap_ was
    a reference to a BpSAP returned by an invocation of bp\_open\_source() that
    had a non-zero value in the _detain_ parameter, then _newBundle_ must be
    non-NULL and the address of the newly created bundle within the ION database
    is placed in _newBundle_.  This address can be used to track, suspend/resume,
    or cancel transmission of the bundle.

- int bp\_track(Object bundle, Object trackingElt)

    Adds _trackingElt_ to the list of "tracking" references in _bundle_.
    _trackingElt_ must be the address of an SDR list element -- whose data is
    the address of this same bundle -- within some list of bundles that is
    privately managed by the application.  Upon destruction of the bundle this
    list element will automatically be deleted, thus removing the bundle from
    the application's privately managed list of bundles.  This enables the
    application to keep track of bundles that it is operating on without risk
    of inadvertently de-referencing the address of a nonexistent bundle. 

- void bp\_untrack(Object bundle, Object trackingElt)

    Removes _trackingElt_ from the list of "tracking" references in _bundle_,
    if it is in that list.  Does not delete _trackingElt_ itself.

- int bp\_memo(Object bundle, unsigned int interval)

    Implements custodial retransmission.  This function inserts a
    "custody-acceptance due" event into the timeline.  The event causes the
    indicated bundle to be re-forwarded if it is still in the database (i.e.,
    it has not yet been accepted by another custodian) as of the time computed
    by adding the indicated interval to the current time.

- int bp\_suspend(Object bundle)

    Suspends transmission of _bundle_.  Has no effect if bundle is "critical"
    (i.e., has got extended class of service BP\_MINIMUM\_LATENCY flag set) or
    if the bundle is already suspended.  Otherwise, reverses the enqueuing of
    the bundle to its selected transmission outduct and places it in the
    "limbo" queue until the suspension is lifted by calling bp\_resume.  Returns
    0 on success, -1 on any error.

- int bp\_resume(Object bundle)

    Terminates suspension of transmission of _bundle_.  Has no effect if
    bundle is "critical" (i.e., has got extended class of service
    BP\_MINIMUM\_LATENCY flag set) or is not suspended.  Otherwise, removes
    the bundle from the "limbo" queue and queues it for route re-computation
    and re-queuing.  Returns 0 on success, -1 on any error.

- int bp\_cancel(Object bundle)

    Cancels transmission of _bundle_.  If the indicated bundle is currently
    queued for forwarding, transmission, or retransmission, it is removed
    from the relevant queue and destroyed exactly as if its Time To Live had
    expired.  Returns 0 on success, -1 on any error.

- int bp\_release(Object bundle)

    Releases a detained bundle for destruction when all retention constraints
    have been removed.  After a detained bundle has been released, the application
    can no longer track, suspend/resume, or cancel its transmission.  Returns 0
    on success, -1 on any error.

- int bp\_receive(BpSAP sap, BpDelivery \*dlvBuffer, int timeoutSeconds)

    Receives a bundle, or reports on some failure of bundle reception activity.

    The "result" field of the dlvBuffer structure will be used to indicate the
    outcome of the data reception activity.

    If at least one bundle destined for the endpoint for which this SAP is
    opened has not yet been delivered to the SAP, then the payload of the
    oldest such bundle will be returned in _dlvBuffer_->_adu_ and
    _dlvBuffer_->_result_ will be set to BpPayloadPresent.  If there is
    no such bundle, bp\_receive() blocks for up to _timeoutSeconds_ while
    waiting for one to arrive.

    If _timeoutSeconds_ is BP\_POLL (i.e., zero) and no bundle is awaiting
    delivery, or if _timeoutSeconds_ is greater than zero but no bundle
    arrives before _timeoutSeconds_ have elapsed, then _dlvBuffer_->_result_
    will be set to BpReceptionTimedOut.  If _timeoutSeconds_ is BP\_BLOCKING
    (i.e., -1) then bp\_receive() blocks until either a bundle arrives or the
    function is interrupted by an invocation of bp\_interrupt().

    _dlvBuffer_->_result_ will be set to BpReceptionInterrupted in the event
    that the calling process received and handled some signal other than SIGALRM
    while waiting for a bundle.

    _dlvBuffer_->_result_ will be set to BpEndpointStopped in the event
    that the operation of the indicated endpoint has been terminated.

    The application data unit delivered in the data delivery structure, if
    any, will be a "zero-copy object" reference.  Use zco reception functions
    (see zco(3)) to read the content of the application data unit.

    Be sure to call bp\_release\_delivery() after every successful invocation of
    bp\_receive().

    The function returns 0 on success, -1 on any error.

- void bp\_interrupt(BpSAP sap)

    Interrupts a bp\_receive() invocation that is currently blocked.  This
    function is designed to be called from a signal handler; for this purpose,
    _sap_ may need to be obtained from a static variable.

- void bp\_release\_delivery(BpDelivery \*dlvBuffer, int releaseAdu)

    Releases resources allocated to the indicated delivery.  _releaseAdu_ is a
    Boolean parameter: if non-zero, the ADU ZCO reference in _dlvBuffer_ (if
    any) is destroyed, causing the ZCO itself to be destroyed if no other
    references to it remain.

- void bp\_close(BpSAP sap)

    Terminates the application's access to the BP endpoint identified by
    the _eid_ cited by the indicated service access point.  The application
    relinquishes its ability to take delivery of bundles destined for the
    indicated endpoint and to send bundles whose source is the indicated
    endpoint.

# SEE ALSO

bpadmin(1), lgsend(1), lgagent(1), bpextensions(3), bprc(5), lgfile(5)
