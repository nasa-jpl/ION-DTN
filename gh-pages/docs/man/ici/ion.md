# NAME

ion - Interplanetary Overlay Network common definitions and functions

# SYNOPSIS

    #include "ion.h"

    [see description for available functions]

# DESCRIPTION

The Interplanetary Overlay Network (ION) software distribution is an
implementation of Delay-Tolerant Networking (DTN) architecture as described
in Internet RFC 4838.  It is designed to enable inexpensive insertion of
DTN functionality into embedded systems such as robotic spacecraft.  The
intent of ION deployment in space flight mission systems is to reduce
cost and risk in mission communications by simplifying the construction
and operation of automated digital data communication networks spanning
space links, planetary surface links, and terrestrial links.

The ION distribution comprises the following software packages:

> _ici_ (Interplanetary Communication Infrastructure), a set of general-purpose
> libraries providing common functionality to the other packages.
>
> _ltp_ (Licklider Transmission Protocol), a core DTN protocol that provides
> transmission reliability based on delay-tolerant acknowledgments, timeouts,
> and retransmissions.
>
> _dgr_ (Datagram Retransmission), a library that enables data to be
> transmitted via UDP with reliability comparable to that provided by TCP.  DGR
> is an alternative implementation of LTP, designed for use within an internet.
>
> _bssp_ (Bundle Streaming Service Protocol), a protocol that supports
> delay-tolerant data streaming.  BSSP delivers data in transmission order
> with minimum latency but possibly with omissions, for immediate display,
> and at the same time it delivers the same data reliably in background so
> that the streamed data can be "rewound" for possibly improved presentation.
>
> _bp_ (Bundle Protocol), a core DTN protocol that provides delay-tolerant
> forwarding of data through a network in which continuous end-to-end
> connectivity is never assured, including support for delay-tolerant
> dynamic routing.  The Bundle Protocol (BP) specification is defined
> in Internet RFC 5050.
>
> _ams_ (Asynchronous Message Service), _cfdp_ (CCSDS File Delivery
> Protocol), _dtpc_ (Delay-Tolerant Payload Conditioning), and _bss_
> (Bundle Streaming Service), application-layer services that are not part
> of the DTN architecture but utilize underlying DTN protocols.

Taken together, the packages included in the ION software distribution
constitute a communication capability characterized by the following
operational features:

> Reliable conveyance of data over a DTN, i.e., a network in which it might
> never be possible for any node to have reliable information about the
> detailed current state of any other node.
>
> Built on this capability, reliable distribution of short messages to multiple
> recipients (subscribers) residing in such a network.
>
> Management of traffic through such a network.
>
> Facilities for monitoring the performance of the network.
>
> Robustness against node failure.
>
> Portability across heterogeneous computing platforms.
>
> High speed with low overhead.
>
> Easy integration with heterogeneous underlying communication infrastructure,
> ranging from Internet to dedicated spacecraft communication links.

While most of the ici package consists of libraries providing functionality
that may be of general utility in any complex embedded software system,
the functions and macros described below are specifically designed to support
operations of ION's delay-tolerant networking protocol stack.

- TIMESTAMPBUFSZ

    This macro returns the recommended size of a buffer that is intended to
    contain a timestamp in ION-standard format:

    >     yyyy/mm/dd-hh:mm:ss

- int ionAttach()

    Attaches the invoking task to ION infrastructure as previously established
    by running the _ionadmin_ utility program.  Returns zero on success, -1 on
    any error.

- void ionDetach()

    Detaches the invoking task from ION infrastructure.  In particular, releases
    handle allocated for access to ION's non-volatile database.  **NOTE**, though,
    that ionDetach() has no effect when the invoking task is running in a
    non-memory-protected environment, such as VxWorks, where all ION resource
    access variables are shared by all tasks: no single task could detach
    without crashing all other ION tasks.

- void ionProd(uvast fromNode, uvast toNode, unsigned int xmitRate, unsigned int owlt)

    This function is designed to be called from an operating environment command
    or a fault protection routine, to enable operation of a node to resume when
    all of its scheduled contacts are in the past (making it impossible to use
    a DTN communication contact to assert additional future communication
    contacts).  The function asserts a single new unidirectional contact
    conforming to the arguments provided, including the applicable one-way light
    time, with start time equal to the current time (at the moment of execution
    of the function) and end time equal to the start time plus 2 hours.  The
    result of executing the function is written to the ION log using standard
    ION status message logging functions.

    **NOTE** that the ionProd() function must be invoked twice in order
    to establish bidirectional communication.

- void ionTerminate()

    Shuts down the entire ION node, terminating all daemons.  The state of 
    the node is retained in the node's SDR heap.

- int ionStartAttendant(ReqAttendant \*attendant)

    Initializes the semaphore in _attendant_ so that it can be used for blocking
    ZCO space requisitions by ionRequestZcoSpace().  Returns 0 on success,
    \-1 on any error.

- void ionPauseAttendant(ReqAttendant \*attendant)

    "Ends" the semaphore in _attendant_ so that the task that is blocked on
    taking it is interrupted and may respond to an error or shutdown condition.

- void ionResumeAttendant(ReqAttendant \*attendant)

    Reinitializes the semaphore in _attendant_ so that it can again be used
    for blocking ZCO space requisitions.

- void ionStopAttendant(ReqAttendant \*attendant)

    Destroys the semaphore in _attendant_, preventing a potential resource leak.

- int ionRequestZcoSpace(ZcoAcct acct, vast fileSpaceNeeded, vast bulkSpaceNeeded, vast heapSpaceNeeded, unsigned char coarsePriority, unsigned char finePriority, ReqAttendant \*attendant, ReqTicket \*ticket)

    Lodges a request for ZCO space in the pool identified by _acct_.  If the
    requested space can be provided immediately, it is reserved for use by the
    calling task.  Otherwise, if _attendant_ is non-NULL then the request is
    queued for service when space becomes available.  In any case, _\*ticket_
    is set to the address of a "ticket" referencing this request.  The status
    of the request can be interrogated by calling ionSpaceAwarded().  If this
    function returns 1 (True) then ZCO space may be consumed immediately and
    the ticket must then be destroyed by a call to ionShred().  Otherwise, if
    an attendant was provided, then the calling task should pend on the semaphore
    in _attendant_ and upon successfully taking the semaphore it must
    consume the requested ZCO space and then ionShred() the ticket.  Otherwise
    the request for ZCO space has been definitively denied and, as always, the
    ticket must by destroyed by an invocation of ionShred().  Returns 0 on success,
    \-1 on any failure.

- int ionSpaceAwarded(ReqTicket \*ticket)

    Returns 1 if _ticket_ is for a ZCO space request that has been serviced
    (ZCO space has been reserved per this request), 0 otherwise.

- void ionShred(ReqTicket \*ticket)

    Dismisses the reservation of ZCO space (if any) requested by the call to
    ionRequestZcoSpace() that returned _ticket_.  Calling ionShred() indicates
    either that the requested space was reserved (i.e., the request was "serviced")
    and has been claimed (consumed by the appending of a ZCO extent) or that the
    request has been canceled.  Note that failure to promptly (within 3 seconds of
    reception) ionShred() the ticket for a service request will be interpreted as
    refusal of the reserved ZCO space, resulting in that space being made
    available for use by other tasks.

- Object ionCreateZco(ZcoMedium source, Object location, vast offset, vast length, unsigned char coarsePriority, unsigned char finePriority, ZcoAcct acct, ReqAttendant \*attendant)

    This function provides a "blocking" implementation of admission control in
    ION.  Like zco\_create(), it constructs a zero-copy object (see zco(3)) that
    contains a single extent of source data residing at _location_ in _source_,
    of which the first _offset_ bytes are omitted and the next _length_ bytes
    are included.  But unlike zco\_create(), ionCreateZco() can be configured to
    block (rather than return an immediate error indication) so long as the total
    amount of space in _source_ that is available for new ZCO formation is less
    than _length_.  ionCreateZco() operates by calling ionRequestZcoSpace(),
    then pending on the semaphore in _attendant_ as necessary before creating
    the ZCO.  ionCreateZco() returns when either (a) space has become
    available and the ZCO has been created, in which case the location of
    the ZCO is returned, or (b) the function has failed (in which case
    ((Object) -1) is returned), or (c) either _attendant_ was null and sufficient
    space for the first extent of the ZCO was not immediately available or else
    the function was interrupted by ionPauseAttendant() before space for the
    ZCO became available (in which case 0 is returned).

- vast ionAppendZcoExtent(Object zco, ZcoMedium source, Object location, vast offset, vast length, unsigned char coarsePriority, unsigned char finePriority, ReqAttendant \*attendant)

    Similar to ionCreateZco() except that instead of creating a new ZCO it appends
    an additional extent to an existing ZCO.  Returns -1 on failure, 0 on
    interruption by ionPauseAttendant() or if _attendant_ was NULL and sufficient
    space for the extent was not immediately available, _length_ on success.

- char \*getIonVersionNbr()

    Returns the name of the ION version installed on the local machine.

- Sdr getIonsdr()

    Returns a pointer to the SDR management object, previously acquired by calling
    ionAttach(), or zero on any error.

- PsmPartition getIonwm()

    Returns a pointer to the ION working memory partition, previously acquired
    by calling ionAttach(), or zero on any error.

- int getIonMemoryMgr()

    Returns the memory manager ID for operations on ION's working memory partition,
    previously acquired by calling ionAttach(), or -1 on any error.

- int ionLocked();

    Returns 1 if the calling task is the owner of the current SDR transaction.
    Assuring that ION is locked while related critical operations are performed
    is essential to the avoidance of race conditions.

- uvast getOwnNodeNbr()

    Returns the Bundle Protocol node number identifying this node, as
    declared when ION was initialized by _ionadmin_.

- time\_t getCtime()

    Returns the current calendar (i.e., Unix epoch) time, as computed from local
    clock time and the computer's current offset from UTC (due to clock drift,
    **not** due to time zone difference; the **utcdelta**) as managed from
    _ionadmin_.

- int ionClockIsSynchronized()

    Returns 1 if the computer on which the local ION node is running has a
    synchronized clock , i.e., a clock that reports the current calendar (i.e.,
    Unix epoch) time as a value that differs from the correct calendar time by
    an interval approximately equal to the currently asserted offset from UTC
    due to clock drift; returns zero otherwise.

    If the machine's clock is synchronized then its reported values (as returned
    by getCtime()) can safely be used as the creation times of new bundles and
    the expiration time of such a bundle can accurately be computed as the sum
    of the bundle's creation time and time to live.  If not, then the creation
    timestamp time of new bundles sourced at the local ION node must be zero
    and the creation timestamp sequence numbers must increase monotonically
    forever, never rolling over to zero.

- void writeTimestampLocal(time\_t timestamp, char \*timestampBuffer)

    Expresses the time value in _timestamp_ as a local timestamp string in
    ION-standard format, as noted above, in _timestampBuffer_.

- void writeTimestampUTC(time\_t timestamp, char \*timestampBuffer)

    Expresses the time value in _timestamp_ as a UTC timestamp string in
    ION-standard format, as noted above, in _timestampBuffer_.

- time\_t readTimestampLocal(char \*timestampBuffer, time\_t referenceTime)

    Parses the local timestamp string in _timestampBuffer_ and returns the
    corresponding calendar (i.e., Unix epoch) time value (as would be returned by
    time(2)), or zero if the timestamp string cannot be parsed successfully.  The
    timestamp string is normally expected to be an absolute expression of local
    time in ION-standard format as noted above.  However, a relative time
    expression variant is also supported: if the first character of
    _timestampBuffer_ is '+' then the remainder of the string is interpreted
    as a count of seconds; the sum of this value and the time value in
    _referenceTime_ is returned.

- time\_t readTimestampUTC(char \*timestampBuffer, time\_t referenceTime)

    Same as readTimestampLocal() except that if _timestampBuffer_ is not a
    relative time expression then it is interpreted as an absolute expression
    of UTC time in ION-standard format as noted above.

# STATUS MESSAGES

ION uses writeMemo(), putErrmsg(), and putSysErrmsg() to log several different
types of standardized status messages.

- Informational messages

    These messages are generated to inform the user of the occurrence of events
    that are nominal but significant, such as the controlled termination of a
    daemon or the production of a congestion forecast.  Each informational
    message has the following format:

    >     {_yyyy/mm/dd hh:mm:ss_} \[i\] _text_

- Warning messages

    These messages are generated to inform the user of the occurrence of events
    that are off-nominal but are likely caused by configuration or operational
    errors rather than software failure.  Each warning message has the following
    format:

    >     {_yyyy/mm/dd hh:mm:ss_} \[?\] _text_

- Diagnostic messages

    These messages are produced by calling putErrmsg() or putSysErrmsg().  They
    are generated to inform the user of the occurrence of events that are
    off-nominal and might be due to errors in software.  The location within
    the ION software at which the off-nominal condition was detected is
    indicated in the message:

    >     {_yyyy/mm/dd hh:mm:ss_} at line _nnn_ of _sourcefilename_, _text_ (_argument_)

    Note that the _argument_ portion of the message (including its enclosing
    parentheses) will be provided only when an argument value seems potentially
    helpful in fault analysis.

- Bundle Status Report (BSR) messages

    A BSR message informs the user of the arrival of a BSR, a Bundle Protocol
    report on the status of some bundle.  BSRs are issued in the course of
    processing bundles for which one or more status report request flags are set,
    and they are also issued when bundles for which custody transfer is requested
    are destroyed prior to delivery to their destination endpoints.  A BSR message
    is generated by **ipnadminep** upon reception of a BSR.  The time and place
    (node) at which the BSR was issued are indicated in the message:

    >     {_yyyy/mm/dd hh:mm:ss_} \[s\] (_sourceEID_)/_creationTimeSeconds_:_counter_/_fragmentOffset_ status _flagsByte_ at _time_ on _endpointID_, '_reasonString_'.

- Communication statistics messages

    A network performance report is a set of eight communication statistics
    messages, one for each of eight different types of network activity.  A report
    is issued every time contact transmission or reception starts or stops,
    except when there is no activity of any kind on the local node since the prior
    report.  When a report is issued, statistic messages are generated to summarize
    all network activity detected since the prior report, after which all network
    activity counters and accumulators are reset to zero.

    **NOTE** also that the **bpstats** utility program can be invoked to issue an
    interim network performance report at any time.  Issuance of interim status
    reports does **not** cause network activity counters and accumulators to be
    reset to zero.

    Statistics messages have the following format:

    >     {_yyyy/mm/dd hh:mm:ss_} \[x\] _xxx_ from _tttttttt_ to _TTTTTTTT_: (0) _aaaa_ _bbbbbbbbbb_ (1) _cccc_ _dddddddddd_ (2) _eeee_ _ffffffffff_ (+) _gggg_ _hhhhhhhhhh_

    _xxx_ indicates the type of network activity that the message is reporting
    on.  Statistics for eight different types of network activity are reported:

    - **src**

        This message reports on the bundles sourced at the local node during the
        indicated interval.

    - **fwd**

        This message is about routing; it reports on the number of bundles queued
        for forwarding to neighboring nodes as selected by the routing procedure.
        When a bundle must be re-forwarded due to convergence-layer transmission
        failure it is counted a second time here.

    - **xmt**

        This message reports on the bundles passed to the convergence layer protocol(s)
        for transmission from this node.  Again, a re-forwarded bundle that is then
        re-transmitted at the convergence layer is counted a second time here.

    - **rcv**

        This message reports on the bundles from other nodes that were received at
        the local node.

    - **dlv**

        This message reports on the bundles delivered to applications via endpoints
        on the local node.

    - **ctr**

        This message reports on the custody refusal signals received at the local node.

    - **rfw**

        This message reports on bundles for which convergence-layer transmission
        failed at this node, causing the bundles to be re-forwarded.

    - **exp**

        This message reports on the bundles destroyed at this node due to TTL
        expiration.

    _tttttttt_ and _TTTTTTTT_ indicate the start and end times of the interval
    for which statistics are being reported, expressed in _yyyy/mm/dd-hh:mm:ss_
    format.  _TTTTTTTT_ is the current time and _tttttttt_ is the time of the
    prior report.

    Each of the four value pairs following the colon (:) reports on the number
    of bundles counted for the indicated type of network activity, for the
    indicated traffic flow, followed by the sum of the sizes of the payloads
    of all those bundles.  The four traffic flows for which statistics are
    reported are "(0)" the priority-0 or "bulk" traffic, "(1)" the priority-1
    "standard" traffic, "(2)" the priority-2 "expedited" traffic, and "(+)"
    the total for all classes of service.

- Free-form messages

    Other status messages are free-form, except that date and time are always noted
    just as for the documented status message types.

# SEE ALSO

ionadmin(1), rfxclock(1), bpstats(1), llcv(3), lyst(3), memmgr(3), platform(3), psm(3), sdr(3), zco(3), ltp(3), bp(3), cfdp(3), ams(3), bss(3)
