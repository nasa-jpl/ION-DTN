# NAME

dgrcli - DGR-based BP convergence layer reception task

# SYNOPSIS

**dgrcli** _local\_hostname_\[:_local\_port\_nbr_\]

# DESCRIPTION

**dgrcli** is a background "daemon" task that handles DGR convergence layer
protocol input.

The daemon receives DGR messages via a UDP socket bound to
_local\_hostname_ and _local\_port\_nbr_, extracts bundles from those
messages, and passes them to the bundle protocol agent on the local ION node.
(_local\_port\_nbr_ defaults to 1113 if not specified.)

**dgrcli** is spawned automatically by **bpadmin** in response to the 's' (START)
command that starts operation of the Bundle Protocol, and it is terminated by
**bpadmin** in response to an 'x' (STOP) command.  **dgrcli** can also be
spawned and terminated in response to START and STOP commands that pertain
specifically to the DGR convergence layer protocol.

# EXIT STATUS

- "0"

    **dgrcli** terminated normally, for reasons noted in the **ion.log** file.  If
    this termination was not commanded, investigate and solve the problem identified
    in the log file and use **bpadmin** to restart **dgrcli**.

- "1"

    **dgrcli** terminated abnormally, for reasons noted in the **ion.log** file.
    Investigate and solve the problem identified in the log file, then use
    **bpadmin** to restart **dgrcli**.

# FILES

No configuration files are needed.

# ENVIRONMENT

No environment variables apply.

# DIAGNOSTICS

The following diagnostics may be issued to the **ion.log** log file:

- dgrcli can't attach to BP.

    **bpadmin** has not yet initialized Bundle Protocol operations.

- No such dgr induct.

    No DGR induct with duct name matching _local\_hostname_ and _local\_port\_nbr_
    has been added to the BP database.  Use **bpadmin** to stop the DGR
    convergence-layer protocol, add the induct, and then restart the DGR protocol.

- CLI task is already started for this engine.

    Redundant initiation of **dgrcli**.

- Can't get IP address for host

    Operating system error.  Check errtext, correct problem, and restart DGR.

- dgrcli can't open DGR service access point.

    DGR system error.  Check prior messages in **ion.log** log file, correct
    problem, and then stop and restart the DGR protocol.

- dgrcli can't create receiver thread

    Operating system error.  Check errtext, correct problem, and restart DGR.

# BUGS

Report bugs to <ion-dtn-support@lists.sourceforge.net>

# SEE ALSO

bpadmin(1), bprc(5)
