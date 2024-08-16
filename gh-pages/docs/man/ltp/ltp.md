# NAME

ltp - Licklider Transmission Protocol (LTP) communications library

# SYNOPSIS

    #include "ltp.h"

    typedef enum
    {
        LtpNoNotice = 0,
        LtpExportSessionStart,
        LtpXmitComplete,
        LtpExportSessionCanceled,
        LtpExportSessionComplete,
        LtpRecvGreenSegment,
        LtpRecvRedPart,
        LtpImportSessionCanceled
    } LtpNoticeType;

    [see description for available functions]

# DESCRIPTION

The ltp library provides functions enabling application software to use LTP
to send and receive information reliably over a long-latency link.  It
conforms to the LTP specification as documented by the Delay-Tolerant
Networking Research Group of the Internet Research Task Force.

The LTP notion of **engine ID** corresponds closely to the Internet notion of
a host, and in ION engine IDs are normally indistinguishable from node numbers
including the node numbers in Bundle Protocol endpoint IDs conforming to
the "ipn" scheme.

The LTP notion of **client ID** corresponds closely to the Internet notion of
"protocol number" as used in the Internet Protocol.  It enables data from
multiple applications -- clients -- to be multiplexed over a single reliable
link.  However, for ION operations we normally use LTP exclusively for the
transmission of Bundle Protocol data, identified by client ID = 1.

- int ltp\_attach()

    Attaches the application to LTP functionality on the lcoal computer.  Returns
    0 on success, -1 on any error.

- void ltp\_detach()

    Terminates all access to LTP functionality on the local computer.

- int ltp\_engine\_is\_started()

    Returns 1 if the local LTP engine has been started and not yet stopped,
    0 otherwise.

- int ltp\_send(uvast destinationEngineId, unsigned int clientId, Object clientServiceData, unsigned int redLength, LtpSessionId \*sessionId)

    Sends a client service data unit to the application that is waiting for
    data tagged with the indicated _clientId_ as received at the remote LTP
    engine identified by _destinationEngineId_.

    _clientServiceData_ must be a "zero-copy object" reference as returned
    by ionCreateZco().  Note that LTP will privately make and destroy its own
    reference to the client service data object; the application is free to
    destroy its reference at any time.

    _redLength_ indicates the number of leading bytes of data in
    _clientServiceData_ that are to be sent reliably, i.e., with selective
    retransmission in response to explicit or implicit negative acknowledgment
    as necessary.  All remaining bytes of data in _clientServiceData_ will be
    sent as "green" data, i.e., unreliably.  If _redLength_ is zero, the entire
    client service data unit will be sent unreliably.  If the entire client
    service data unit is to be sent reliably, _redLength_ may be simply be set
    to LTP\_ALL\_RED (i.e., -1).

    On success, the function populates _\*sessionId_ with the source engine ID
    and the "session number" assigned to transmission of this client service
    data unit and returns zero.  The session number may be used to link future
    LTP processing events, such as transmission cancellation, to the affected
    client service data.  ltp\_send() returns -1 on any error.

- int ltp\_open(unsigned int clientId)

    Establishes the application's exclusive access to received service data
    units tagged with the indicated client service data ID.  At any time, only
    a single application task is permitted to receive service data units for
    any single client service data ID.

    Returns 0 on success, -1 on any error (e.g., the indicated client service
    is already being held open by some other application task).

- int ltp\_get\_notice(unsigned int clientId, LtpNoticeType \*type, LtpSessionId \*sessionId, unsigned char \*reasonCode, unsigned char \*endOfBlock, unsigned int \*dataOffset, unsigned int \*dataLength, Object \*data)

    Receives notices of LTP processing events pertaining to the flow of service
    data units tagged with the indicated client service ID.  The nature of each
    event is indicated by _\*type_.  Additional parameters characterizing the
    event are returned in _\*sessionId_, _\*reasonCode_, _\*endOfBlock_,
    _\*dataOffset_, _\*dataLength_, and _\*data_ as relevant.

    The value returned in _\*data_ is always a zero-copy object; use the
    zco\_\* functions defined in "zco.h" to retrieve the content of that object.

    When the notice is an LtpRecvGreenSegment, the ZCO returned in _\*data_
    contains the content of a single LTP green segment.  Reassembly of the
    green part of some block from these segments is the responsibility of
    the application.

    When the notice is an LtpRecvRedPart, the ZCO returned in _\*data_
    contains the red part of a possibly aggregated block.  The ZCO's content
    may therefore comprise multiple service data objects.  Extraction of
    individual service data objects from the aggregated block is the
    responsibility of the application.  A simple way to do this is to
    prepend the length of the service data object to the object itself
    (using zco\_prepend\_header) before calling ltp\_send, so that the
    receiving application can alternate extraction of object lengths and
    objects from the delivered block's red part.

    The cancellation of an export session may result in delivery of multiple
    LtpExportSessionCanceled notices, one for each service data unit in the
    export session's (potentially) aggregated block.  The ZCO returned in
    _\*data_ for each such notice is a service data unit ZCO that had previously
    been passed to ltp\_send().

    ltp\_get\_notice() always blocks indefinitely until an LTP processing event
    is delivered.

    Returns zero on success, -1 on any error.

- void ltp\_interrupt(unsigned int clientId)

    Interrupts an ltp\_get\_notice() invocation.  This function is designed to be
    called from a signal handler; for this purpose, _clientId_ may need to be
    obtained from a static variable.

- void ltp\_release\_data(Object data)

    Releases the resources allocated to hold _data_, which must be a **received**
    client service data ZCO.

- void ltp\_close(unsigned int clientId)

    Terminates the application's exclusive access to received service data
    units tagged with the indicated client service data ID.

# SEE ALSO

ltpadmin(1), ltprc(5), zco(3)
