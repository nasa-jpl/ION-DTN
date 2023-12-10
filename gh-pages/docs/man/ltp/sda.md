# NAME

sda - LTP Service Data Aggregation (SDA) library

# SYNOPSIS

    #include "sda.h"

    typedef vast (*SdaDelimiterFn)(unsigned int clientId, unsigned char *buffer, vast bufferLength);

    typedef int (*SdaHandlerFn)(uvast sourceEngineId, unsigned int clientId, Object clientServiceData);

    [see description for available functions]

# DESCRIPTION

The **sda** library provides functions enabling application software to use LTP
more efficiently, by aggregating multiple small client service data units
into larger LTP client service data items.  This reduces overhead somewhat,
but more importantly it reduces the volume of positive and negative LTP
acknowledgment traffic by sending more data in fewer LTP blocks -- because
LTP acknowledgments are issued on a per-block basis.

The library relies on the application to detect the boundaries between
aggregated service data units in each received LTP client service data item;
the application must provide an SdaDelimiterFn function for this purpose.  An
SDA delimiter function inspects the client service data bytes in _buffer_ -
some portion of an LTP service data block, of length _bufferLength_ - to
determine the length of the client service data unit at the start of the
buffer; data unit client service ID is provided to aid in this determination.
The function returns that length if the determination was successful, zero
if there is no valid client service data item at the start of the buffer, -1
on any system failure.

The **sda** library similarly relies on the application to process the service
data units identified in this manner; the application must provide an
SdaHandlerFn function for this purpose.  An SDA handler function is provided
with the ID of the LTP engine that sent the service data unit, the client
ID characterizing the service data unit, and the service data unit itself;
the service data unit is presented as a Zero-Copy Object (ZCO).  The handler
function must return -1 on any system error, zero otherwise.

- int sda\_send(uvast destinationEngineId, unsigned int clientId, Object clientServiceData);

    Sends a client service data unit to an application, identified by _clientId_,
    at the LTP engine identified by _destinationEngineId_.  clientServiceData must
    be a "zero-copy object" reference as returned by ionCreateZco().  Note that SDA
    will privately make and destroy its own reference to the client service data;
    the application is free to destroy its reference at any time.   Note that the
    client service data unit will always be sent reliably (i.e., "red").

    Also note that sda\_run() must be executing in order for sda\_send to be
    performed.

    Returns 0 on success, -1 on any system failure.

- int sda\_run(SdaDelimiterFn delimiter, SdaHandlerFn handler);

    sda\_run() executes an infinite loop that receives LTP client service data items,
    calls _delimiter_ to determine the length of each client service data unit
    in each item, and passes those client service data units to the _handler_
    function.  To terminate the loop, call sda\_interrupt().  Note that sda\_send()
    can only be executed while the sda\_run() loop is still executing.

    Returns 0 on success, -1 on any system failure.

- void sda\_interrupt();

    Interrupts sda\_run().

# SEE ALSO

sdatest(1), zco(3)
