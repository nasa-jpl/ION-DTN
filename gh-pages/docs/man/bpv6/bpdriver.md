# NAME

bpdriver - Bundle Protocol transmission test program

# SYNOPSIS

**bpdriver** _nbrOfCycles_ _ownEndpointId_ _destinationEndpointId_ \[_length_\] \[t_TTL_\]

# DESCRIPTION

**bpdriver** uses Bundle Protocol to send _nbrOfCycles_ application data
units of length indicated by _length_, to a counterpart application task
that has opened the BP endpoint identified by _destinationEndpointId_.

If omitted, _length_ defaults to 60000.  

_TTL_ indicates the number of seconds the bundles may remain in the
network, undelivered, before they are automatically destroyed. If omitted, _TTL_
defaults to 300 seconds.

**bpdriver** normally runs in "echo" mode: after sending each bundle it
waits for an acknowledgment bundle before sending the next one.  For this
purpose, the counterpart application task should be **bpecho**.

Alternatively **bpdriver** can run in "streaming" mode, i.e., without
expecting or receiving acknowledgments.  Streaming mode is enabled when
_length_ is specified as a negative number, in which case the additive
inverse of _length_ is used as the effective value of _length_.  For
this purpose, the counterpart application task should be **bpcounter**.

If the effective value of _length_ is 1, the sizes of the transmitted
service data units will be randomly selected multiples of 1024 in the
range 1024 to 62464.

**bpdriver** normally runs with custody transfer disabled.  To request
custody transfer for all bundles sent by **bpdriver**, specify _nbrOfCycles_
as a negative number; the additive inverse of _nbrOfCycles_ will be used
as its effective value in this case.

When all copies of the file have been sent, **bpdriver** prints a performance
report.

# EXIT STATUS

- "0"

    **bpdriver** has terminated.  Any problems encountered during operation
    will be noted in the **ion.log** log file.

# FILES

The service data units transmitted by **bpdriver** are sequences of text
obtained from a file in the current working directory named "bpdriverAduFile",
which **bpdriver** creates automatically.

# ENVIRONMENT

No environment variables apply.

# DIAGNOSTICS

Diagnostic messages produced by **bpdriver** are written to the ION log
file _ion.log_.

- Can't attach to BP.

    **bpadmin** has not yet initialized Bundle Protocol operations.

- Can't open own endpoint.

    Another application has already opened _ownEndpointId_.  Terminate that
    application and rerun.

- Can't create ADU file

    Operating system error.  Check errtext, correct problem, and rerun.

- Error writing to ADU file

    Operating system error.  Check errtext, correct problem, and rerun.

- bpdriver can't create file ref.

    ION system error.  Check for earlier diagnostic messages describing
    the cause of the error; correct problem and rerun.

- bpdriver can't create ZCO.

    ION system error.  Check for earlier diagnostic messages describing
    the cause of the error; correct problem and rerun.

- bpdriver can't send message

    Bundle Protocol service to the remote endpoint has been stopped.

- bpdriver reception failed

    **bpdriver** is in "echo" mode, and Bundle Protocol delivery service
    has been stopped.

# BUGS

Report bugs to <ion-dtn-support@lists.sourceforge.net>

# SEE ALSO

bpadmin(1), bpcounter(1), bpecho(1), bp(3)
