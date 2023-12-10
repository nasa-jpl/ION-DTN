# NAME

dgr - Datagram Retransmission system library

# SYNOPSIS

    #include "dgr.h"

    [see description for available functions]

# DESCRIPTION

The DGR library is an alternative implementation of a subset of LTP, intended
for use over UDP/IP in the Internet; unlike ION's canonical LTP implementation
it includes a congestion control mechanism that interprets LTP block
transmission failure as an indication of network congestion (not data
corruption) and reduces data transmission rate in response.

As such, DGR differs from many reliable-UDP systems in two main ways:

        It uses adaptive timeout interval computation techniques
        borrowed from TCP to try to avoid introducing congestion
        into the network.

        It borrows the concurrent-session model of transmission
        from LTP (and ultimately from CFDP), rather than waiting
        for one datagram to be acknowledged before sending the next,
        to improve bandwidth utilization.

At this time DGR is interoperable with other implementations of LTP only when
each block it receives is transmitted in a single LTP data segment encapsulated
in a single UDP datagram.  More complex LTP behavior may be implemented in
the future.

- int dgr\_open(uvast ownEngineId, unsigned int clientSvcId, unsigned short ownPortNbr, unsigned int ownIpAddress, char \*memmgrName, Dgr \*dgr, DgrRC \*rc)

    Establishes the application's access to DGR communication service.

    _ownEngineId_ is the sending LTP engine ID that will characterize segments
    issued by this DGR service access point.  In order to prevent erroneous system
    behavior, never assign the same LTP engine ID to any two interoperating
    DGR SAPs.

    _clientSvcId_ identifies the LTP client service to which all LTP segments
    issued by this DGR service access point will be directed.

    _ownPortNbr_ is the port number to use for DGR service.  If zero, a
    system-assigned UDP port number is used.

    _ownIpAddress_ is the Internet address of the network interface to use for
    DGR service.  If zero, this argument defaults to the address of the interface
    identified by the local machine's host name.

    _memmgrName_ is the name of the memory manager (see memmgr(3)) to use for
    dynamic memory management in DGR.  If NULL, defaults to the standard
    system malloc() and free() functions.

    _dgr_ is the location in which to store the service access pointer that must
    be supplied on subsequent DGR function invocations.

    _rc_ is the location in which to store the DGR return code resulting from
    the attempt to open this service access point (always DgrOpened).

    On any failure, returns -1.  On success, returns zero.

- void dgr\_getsockname(Dgr dgr, unsigned short \*portNbr, unsigned int \*ipAddress)

    States the port number and IP address of the UDP socket used for this DGR
    service access point.

- void dgr\_close(Dgr dgr)

    Reverses dgr\_open(), releasing resources where possible.

- int dgr\_send(Dgr dgr, unsigned short toPortNbr, unsigned int toIpAddress, int notificationFlags, char \*content, int length, DgrRC \*rc)

    Sends the indicated content, of length as indicated, to the remote DGR
    service access point identified by _toPortNbr_ and _toIpAddress_.  The
    message will be retransmitted as necessary until either it is acknowledged or
    DGR determines that it cannot be delivered.

    _notificationFlags_, if non-zero, is the logical OR of the notification
    behaviors requested for this datagram.  Available behaviors are DGR\_NOTE\_FAILED
    (a notice of datagram delivery failure will issued if delivery of the
    datagram fails) and DGR\_NOTE\_ACKED (a notice of datagram delivery success
    will be issued if delivery of the datagram succeeds).  Notices are issued
    via dgr\_receive() that is, the thread that calls dgr\_receive() on this DGR
    service access point will receive these notices interspersed with inbound
    datagram contents.

    _length_ of content must be greater than zero and may be as great
    as 65535, but lengths greater than 8192 may not be supported by the local
    underlying UDP implementation; to minimize the chance of data loss when
    transmitting over the internet, length should not exceed 512.

    _rc_ is the location in which to store the DGR return code resulting from
    the attempt to send the content.

    On any failure, returns -1 and sets _\*rc_ to DgrFailed.  On success, returns
    zero.

- int dgr\_receive(Dgr dgr, unsigned short \*fromPortNbr, unsigned int \*fromIpAddress, char \*content, int \*length, int \*errnbr, int timeoutSeconds, DgrRC \*rc)

    Delivers the oldest undelivered DGR event queued for delivery.

    DGR events are of two type: (a) messages received from a remote DGR
    service access point and (b) notices of previously sent messages that
    DGR has determined either have been or cannot be delivered, as requested
    in the _notificationFlags_ parameters provided to the dgr\_send() calls
    that sent those messages.

    In the former case, dgr\_receive() will place the content of the inbound
    message in _content_, its length in _length_, and the IP address and port
    number of the sender in _fromIpAddress_ and _fromPortNbr_, and it will
    set _\*rc_ to DgrDatagramReceived and return zero.

    In the latter case, dgr\_receive() will place the content of the affected
    **outbound** message in _content_ and its length in _length_ and return
    zero.  If the event being reported is a delivery success, then
    DgrDatagramAcknowledged will be placed in _\*rc_.  Otherwise,
    DgrDatagramNotAcknowledged will be placed in _\*rc_ and
    the relevant errno (if any) will be placed in _\*errnbr_.

    The _content_ buffer should be at least 65535 bytes in length to enable
    delivery of the content of the received or delivered/undeliverable message.

    _timeoutSeconds_ controls blocking behavior.  If _timeoutSeconds_
    is DGR\_BLOCKING (i.e., -1), dgr\_receive() will not return until (a) there
    is either an inbound message to deliver or an outbound message delivery
    result to report, or (b) the function is interrupted by means of
    dgr\_interrupt().  If _timeoutSeconds_ is DGR\_POLL (i.e., zero),
    dgr\_receive() returns immediately; if there is currently no
    inbound message to deliver and no outbound message
    delivery result to report, the function sets _\*rc_ to DgrTimedOut and
    returns zero.
    For any other positive value of _timeoutSeconds_, dgr\_receive() returns
    after the indicated number of seconds have lapsed (in which case the
    returned value of _\*rc_ is DgrTimedOut), or when there is a message to deliver
    or a delivery result to report, or when the function is interrupted
    by means of dgr\_interrupt(), whichever occurs first.  When the function
    returns due to interruption by dgr\_interrupt(), the value placed in _\*rc_ is
    DgrInterrupted instead of DgrDatagramReceived.

    _rc_ is the location in which to store the DGR return code resulting from
    the attempt to receive content.

    On any I/O error or other unrecoverable system error, returns -1.  Otherwise
    always returns zero, placing DgrFailed in _\*rc_ and writing a failure message
    in the event of an operating error.

- void dgr\_interrupt(Dgr dgr)

    Interrupts a dgr\_receive() invocation that is currently blocked.  Designed 
    to be called from a signal handler; for this purpose, _dgr_ may need to
    be obtained from a static variable.

# SEE ALSO

ltp(3), file2dgr(1), dgr2file(1)
