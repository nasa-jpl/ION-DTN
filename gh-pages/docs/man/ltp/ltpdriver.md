# NAME

ltpdriver - LTP transmission test program

# SYNOPSIS

**ltpdriver** _remoteEngineNbr_ _clientId_ _nbrOfCycles_ _greenLength_ \[_ totalLength_\]

# DESCRIPTION

**ltpdriver** uses LTP to send _nbrOfCycles_ service data units of length
indicated by _totalLength_, of which the trailing _greenLength_ bytes are
sent unreliably, to the **ltpcounter** client service process for
client service number _clientId_ attached to the remote LTP engine
identified by _remoteEngineNbr_.  If omitted, _length_ defaults to
60000\.  If _length_ is 1, the sizes of the transmitted service data units
will be randomly selected multiples of 1024 in the range 1024 to 62464.

Whenever the size of the transmitted service data unit is less than or equal
to _greenLength_, the entire SDU is sent unreliably.

When all copies of the file have been sent, **ltpdriver** prints a performance
report.

# EXIT STATUS

- "0"

    **ltpdriver** has terminated.  Any problems encountered during operation
    will be noted in the **ion.log** log file.

- "1"

    **ltpdriver** was unable to start, because it could not attach to the LTP
    protocol on the local node.  Run **ltpadmin** to start LTP, then try again.

# FILES

The service data units transmitted by **ltpdriver** are sequences of text
obtained from a file in the current working directory named "ltpdriverAduFile",
which **ltpdriver** creates automatically.

# ENVIRONMENT

No environment variables apply.

# DIAGNOSTICS

Diagnostic messages produced by **ltpdriver** are written to the ION log
file _ion.log_.

- ltpdriver can't initialize LTP.

    **ltpadmin** has not yet initialized LTP protocol operations.

- Can't create ADU file

    Operating system error.  Check errtext, correct problem, and rerun.

- Error writing to ADU file

    Operating system error.  Check errtext, correct problem, and rerun.

- ltpdriver can't create file ref.

    ION system error.  Check for earlier diagnostic messages describing
    the cause of the error; correct problem and rerun.

- ltpdriver can't create ZCO.

    ION system error.  Check for earlier diagnostic messages describing
    the cause of the error; correct problem and rerun.

- ltpdriver can't send message.

    LTP span to the remote engine has been stopped.

- ltp\_send failed.

    LTP system error.  Check for earlier diagnostic messages describing
    the cause of the error; correct problem and rerun.

# BUGS

Report bugs to <ion-dtn-support@lists.sourceforge.net>

# SEE ALSO

ltpadmin(1), ltpcounter(1), ltp(3)
