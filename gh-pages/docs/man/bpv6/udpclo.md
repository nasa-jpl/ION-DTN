# NAME

udpclo - UDP-based BP convergence layer output task

# SYNOPSIS

**udpclo** _round\_trip\_time_ _remote\_hostname_\[:_remote\_port\_nbr_\]

# DESCRIPTION

**udpclo** is a background "daemon" task that extracts bundles from the
queues of bundles ready for transmission via UDP to a remote node's UDP
socket at _remote\_hostname_ and _remote\_port\_nbr_, encapsulates those
bundles in UDP datagrams, and sends those datagrams to that remote UDP socket.

Because UDP is not itself a "reliable" transmission protocol (i.e., it
performs no retransmission of lost data), it may be used in conjunction
with BP custodial retransmission.  BP custodial retransmission is triggered
only by expiration of a timer whose interval is nominally the round-trip
time between the sending BP node and the next node in the bundle's end-to-end
path that is expected to take custody of the bundle; notionally, if no custody
signal citing the transmitted bundle has been received before the end of this
interval it can be assumed that either the bundle or the custody signal was
lost in transmission and therefore the bundle should be retransmitted.  The
value of the custodial retransmission timer interval (the expected round-trip
time between the sending node and the anticipated next custodian) must be
provided as a run-time argument to **udpclo**.  If the value of this parameter
is zero, custodial retransmission is disabled.

**udpclo** is spawned automatically by **bpadmin** in response to the 's' (START)
command that starts operation of the Bundle Protocol, and it is terminated by
**bpadmin** in response to an 'x' (STOP) command.  **udpclo** can also be
spawned and terminated in response to START and STOP commands that pertain
specifically to the UDP convergence layer protocol.

# EXIT STATUS

- "0"

    **udpclo** terminated normally, for reasons noted in the **ion.log** file.  If
    this termination was not commanded, investigate and solve the problem identified
    in the log file and use **bpadmin** to restart **udpclo**.

- "1"

    **udpclo** terminated abnormally, for reasons noted in the **ion.log** file.
    Investigate and solve the problem identified in the log file, then use
    **bpadmin** to restart **udpclo**.

# FILES

No configuration files are needed.

# ENVIRONMENT

No environment variables apply.

# DIAGNOSTICS

The following diagnostics may be issued to the **ion.log** log file:

- udpclo can't attach to BP.

    **bpadmin** has not yet initialized Bundle Protocol operations.

- No memory for UDP buffer in udpclo.

    ION system error.  Check errtext, correct problem, and restart UDP.

- No such udp duct.

    No UDP outduct with duct name _remote\_hostname_\[:&lt;remote\_port\_nbr>\] has been
    added to the BP database.  Use **bpadmin** to stop the UDP convergence-layer
    protocol, add the outduct, and then restart the UDP protocol.

- CLO task is already started for this engine.

    Redundant initiation of **udpclo**.

- CLO can't open UDP socket

    Operating system error.  Check errtext, correct problem, and restart **udpclo**.

- CLO write() error on socket

    Operating system error.  Check errtext, correct problem, and restart **udpclo**.

- Bundle is too big for UDP CLA.

    Configuration error: bundles that are too large for UDP transmission (i.e.,
    larger than 65535 bytes) are being enqueued for **udpclo**.  Change routing.

# BUGS

Report bugs to <ion-dtn-support@lists.sourceforge.net>

# SEE ALSO

bpadmin(1), bprc(5), udpcli(1)
