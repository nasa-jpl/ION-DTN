# NAME

bping - Send and receive Bundle Protocol echo bundles.

# SYNOPSIS

**bping** \[**-c** _count_\] \[**-i** _interval_\] \[**-p** _priority_\] \[**-q** _wait_\] \[**-r** _flags_\] \[**-t** _ttl_\] _srcEID_ _destEID_ \[_reporttoEID_\]

# DESCRIPTION

**bping** sends bundles from _srcEID_ to _destEID_.  If the _destEID_ echoes
the bundles back (for instance, it is a **bpecho** endpoint), **bping** will
print the round-trip time.  When complete, bping will print statistics before
exiting.  It is very similar to **ping**, except it works with the bundle
protocol.

**bping** terminates when one of the following happens: it receives the SIGINT
signal (Ctrl+C), it receives responses to all of the bundles it sent, or it has
sent all _count_ of its bundles and waited _wait_ seconds.

When **bping** is executed in a VxWorks or RTEMS environment, its runtime
arguments are presented positionally rather than by keyword, in this order:
count, interval, priority, wait, flags, TTL, verbosity (a Boolean, defaulting
to zero), source EID, destination EID, report-to EID.

Source EID and destination EID are always required.

# EXIT STATUS

These exit statuses are taken from **ping**.

- "0"

    **bping** has terminated normally, and received responses to all the packets it
    sent.

- "1"

    **bping** has terminated normally, but it did not receive responses to all the
    packets it sent.

- "2"

    **bping** has terminated due to an error.  Details should be noted in the
    **ion.log** log file.

# FILES

No configuration files are needed.

# ENVIRONMENT

No environment variables apply.

# DIAGNOSTICS

Diagnostic messages produced by **bping** are written to the ION log file
_ion.log_ and printed to standard error.  Diagnostic messages that don't cause
**bping** to terminate indicate a failure parsing an echo response bundle.  This
means that _destEID_ isn't an echo endpoint: it's responding with some other
bundle message of an unexpected format.

- Can't attach to BP.

    **bpadmin** has not yet initialized Bundle Protocol operations.

- Can't open own endpoint.

    Another application has already opened _ownEndpointId_.  Terminate that
    application and rerun.

- bping bundle reception failed.

    BP system error.  Check for earlier diagnostic messages describing
    the cause of the error; correct problem and rerun.

- No space for ZCO extent.

    ION system error.  Check for earlier diagnostic messages describing
    the cause of the error; correct problem and rerun.

- Can't create ZCO.

    ION system error.  Check for earlier diagnostic messages describing
    the cause of the error; correct problem and rerun.

- bping can't send echo bundle.

    BP system error.  Check for earlier diagnostic messages describing
    the cause of the error; correct problem and rerun.

# BUGS

Report bugs to <ion-dtn-support@lists.sourceforge.net>

# SEE ALSO

bpecho(1), bptrace(1), bpadmin(1), bp(3), ping(8)
