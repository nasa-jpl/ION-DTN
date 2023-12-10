# NAME

dgrclo - DGR-based BP convergence layer transmission task

# SYNOPSIS

**dgrclo** _remote\_hostname_\[:_remote\_port\_nbr_\]

# DESCRIPTION

**dgrclo** is a background "daemon" task that spawns two threads, one that
handles DGR convergence layer protocol input (positive and negative
acknowledgments) and a second that handles DGR convergence layer protocol
output.

The output thread extracts bundles from the queues of bundles ready for
transmission via DGR to a remote bundle protocol agent, encapsulates them in
DGR messages, and uses a randomly configured local UDP socket to send those
messages to the remote UDP socket bound to _remote\_hostname_ and
_remote\_port\_nbr_.  (_local\_port\_nbr_ defaults to 1113 if not specified.)

The input thread receives DGR messages via the same local UDP socket and uses
them to manage DGR retransmission of transmitted datagrams.

**dgrclo** is spawned automatically by **bpadmin** in response to the 's' (START)
command that starts operation of the Bundle Protocol, and it is terminated by
**bpadmin** in response to an 'x' (STOP) command.  **dgrclo** can also be
spawned and terminated in response to START and STOP commands that pertain
specifically to the DGR convergence layer protocol.

# EXIT STATUS

- "0"

    **dgrclo** terminated normally, for reasons noted in the **ion.log** file.  If
    this termination was not commanded, investigate and solve the problem identified
    in the log file and use **bpadmin** to restart **dgrclo**.

- "1"

    **dgrclo** terminated abnormally, for reasons noted in the **ion.log** file.
    Investigate and solve the problem identified in the log file, then use
    **bpadmin** to restart **dgrclo**.

# FILES

No configuration files are needed.

# ENVIRONMENT

No environment variables apply.

# DIAGNOSTICS

The following diagnostics may be issued to the **ion.log** log file:

- dgrclo can't attach to BP.

    **bpadmin** has not yet initialized Bundle Protocol operations.

- CLI task is already started for this engine.

    Redundant initiation of **dgrclo**.

- No such dgr outduct.

    No DGR outduct with duct name matching _remote\_hostname_ and _remote\_port\_nbr_
    has been added to the BP database.  Use **bpadmin** to stop the DGR
    convergence-layer protocol, add the outduct, and then restart the DGR protocol.

- dgrclo can't open DGR service access point.

    DGR system error.  Check prior messages in **ion.log** log file, correct
    problem, and then stop and restart the DGR protocol.

- dgrclo can't create sender thread

    Operating system error.  Check errtext, correct problem, and restart DGR.

- dgrclo can't create receiver thread

    Operating system error.  Check errtext, correct problem, and restart DGR.

# BUGS

Report bugs to <ion-dtn-support@lists.sourceforge.net>

# SEE ALSO

bpadmin(1), bprc(5)
