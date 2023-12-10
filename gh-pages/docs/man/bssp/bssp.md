# NAME

bssp - Bundle Streaming Service Protocol (BSSP) communications library

# SYNOPSIS

    #include "bssp.h"

    typedef enum
    {
        BsspNoNotice = 0,
        BsspXmitSuccess,
        BsspXmitFailure,
        BsspRecvSuccess
    } BsspNoticeType;

    [see description for available functions]

# DESCRIPTION

The bssp library provides functions enabling application software to use BSSP
to send and receive streaming data in bundles.

BSSP is designed to forward streaming data in original transmission order
wherever possible but to retransmit data as necessary to ensure that the
entire stream is available for playback eventually.  To this end, BSSP uses
not one but two underlying "link service" channels: (a) an unreliable "best
efforts" channel, for data items that are successfully received upon initial
transmission over every extent of the end-to-end path, and (b) a "reliable"
channel, for data items that were lost at some point, had to be retransmitted,
and therefore are now out of order.  The BSS library at the destination node
supports immediate "real-time" display of all data received on the "best
efforts" channel in transmission order, together with database retention of
all data eventually received on the "reliable" channel.

The BSSP notion of **engine ID** corresponds closely to the Internet notion of
a host, and in ION engine IDs are normally indistinguishable from node numbers
including the node numbers in Bundle Protocol endpoint IDs conforming to
the "ipn" scheme.

The BSSP notion of **client ID** corresponds closely to the Internet notion of
"protocol number" as used in the Internet Protocol.  It enables data from
multiple applications -- clients -- to be multiplexed over a single reliable
link.  However, for ION operations we normally use BSSP exclusively for the
transmission of Bundle Protocol data, identified by client ID = 1.

- int bssp\_attach()

    Attaches the application to BSSP functionality on the lcoal computer.  Returns
    0 on success, -1 on any error.

- void bssp\_detach()

    Terminates all access to BSSP functionality on the local computer.

- int bssp\_engine\_is\_started()

    Returns 1 if the local BSSP engine has been started and not yet stopped,
    0 otherwise.

- int bssp\_send(uvast destinationEngineId, unsigned int clientId, Object clientServiceData, int inOrder, BsspSessionId \*sessionId)

    Sends a client service data unit to the application that is waiting for
    data tagged with the indicated _clientId_ as received at the remote BSSP
    engine identified by _destinationEngineId_.

    _clientServiceData_ must be a "zero-copy object" reference as returned
    by ionCreateZco().  Note that BSSP will privately make and destroy its own
    reference to the client service data object; the application is free to
    destroy its reference at any time.

    _inOrder_ is a Boolean value indicating whether or not the service data item
    that is being sent is "in order", i.e., was originally transmitted after all
    items that have previously been sent to this destination by this local BSSP
    engine: 0 if no (meaning that the item must be transmitted using the
    "reliable" channel), 1 if yes (meaning that the item must be transmitted
    using the "best-efforts" channel.

    On success, the function populates _\*sessionId_ with the source engine ID
    and the "session number" assigned to transmission of this client service
    data unit and returns zero.  The session number may be used to link future
    BSSP processing events to the affected client service data.  bssp\_send()
    returns -1 on any error.

- int bssp\_open(unsigned int clientId)

    Establishes the application's exclusive access to received service data
    units tagged with the indicated BSSP client service data ID.  At any time,
    only a single application task is permitted to receive service data units
    for any single client service data ID.

    Returns 0 on success, -1 on any error (e.g., the indicated client service
    is already being held open by some other application task).

- int bssp\_get\_notice(unsigned int clientId, BsspNoticeType \*type, BsspSessionId \*sessionId, unsigned char \*reasonCode, unsigned int \*dataLength, Object \*data)

    Receives notices of BSSP processing events pertaining to the flow of service
    data units tagged with the indicated client service ID.  The nature of each
    event is indicated by _\*type_.  Additional parameters characterizing the
    event are returned in _\*sessionId_, _\*reasonCode_, _\*dataLength_, and
    _\*data_ as relevant.

    The value returned in _\*data_ is always a zero-copy object; use the
    zco\_\* functions defined in "zco.h" to retrieve the content of that object.

    When the notice is an BsspRecvSuccess, the ZCO returned in _\*data_
    contains the content of a single BSSP block.

    The cancellation of an export session results in delivery of a 
    BsspXmitFailure notice.  In this case, the ZCO returned in \*data is a
    service data unit ZCO that had previously been passed to bssp\_send().

    bssp\_get\_notice() always blocks indefinitely until an BSSP processing event
    is delivered.

    Returns zero on success, -1 on any error.

- void bssp\_interrupt(unsigned int clientId)

    Interrupts an bssp\_get\_notice() invocation.  This function is designed to be
    called from a signal handler; for this purpose, _clientId_ may need to be
    obtained from a static variable.

- void bssp\_release\_data(Object data)

    Releases the resources allocated to hold _data_, which must be a **received**
    client service data unit ZCO.

- void bssp\_close(unsigned int clientId)

    Terminates the application's exclusive access to received service data
    units tagged with the indicated client service data ID.

# SEE ALSO

bsspadmin(1), bssprc(5), zco(3)
