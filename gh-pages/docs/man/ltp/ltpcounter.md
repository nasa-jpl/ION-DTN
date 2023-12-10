# NAME

ltpcounter - LTP reception test program

# SYNOPSIS

**ltpcounter** _client\_ID_ \[_max\_nbr\_of\_bytes_\]

# DESCRIPTION

**ltpcounter** uses LTP to receive service data units flagged with client
service number _client\_ID_ from a remote **ltpdriver** client service
process.  When the total number of bytes of client service data it has
received exceeds _max\_nbr\_of\_bytes_, it terminates and prints reception
and cancellation statistics.  If _max\_nbr\_of\_bytes_ is omitted, the default
limit is 2 billion bytes.

While receiving data, **ltpcounter** prints a 'v' character every 5 seconds
to indicate that it is still alive.

# EXIT STATUS

- "0"

    **ltpcounter** has terminated.  Any problems encountered during operation
    will be noted in the **ion.log** log file.

- "1"

    **ltpcounter** was unable to start, because it could not attach to the LTP
    protocol on the local node or could not open access to client service
    _clientId_.

    In the former case, run **ltpadmin** to start LTP and try again.

    In the latter case, some other client service task has already opened
    access to client service _clientId_.  If no such task is currently running
    (e.g., it crashed while holding the client service open), use **ltpadmin** to
    stop and restart the LTP protocol.

# FILES

No configuration files are needed.

# ENVIRONMENT

No environment variables apply.

# DIAGNOSTICS

Diagnostic messages produced by **ltpcounter** are written to the ION log
file _ion.log_.

- ltpcounter can't initialize LTP.

    **ltpadmin** has not yet initialized LTP protocol operations.

- ltpcounter can't open client access.

    Another task has opened access to service client _clientId_ and has not yet
    relinquished it.

- Can't get LTP notice.

    LTP system error.  Check for earlier diagnostic messages describing
    the cause of the error; correct problem and rerun.

# BUGS

Report bugs to <ion-dtn-support@lists.sourceforge.net>

# SEE ALSO

ltpadmin(1), ltpdriver(1), ltp(3)
