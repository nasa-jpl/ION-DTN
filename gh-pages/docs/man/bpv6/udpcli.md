# NAME

udpcli - UDP-based BP convergence layer input task

# SYNOPSIS

**udpcli** _local\_hostname_\[:_local\_port\_nbr_\]

# DESCRIPTION

**udpcli** is a background "daemon" task that receives UDP datagrams via a
UDP socket bound to _local\_hostname_ and _local\_port\_nbr_, extracts
bundles from those datagrams, and passes them to the bundle protocol agent
on the local ION node.

If not specified, port number defaults to 4556.

The convergence layer input task is spawned automatically by **bpadmin** in
response to the 's' (START) command that starts operation of the Bundle
Protocol; the text of the command that is used to spawn the task must be
provided at the time the "udp" convergence layer protocol is added to the BP
database.  The convergence layer input task is terminated by **bpadmin**
in response to an 'x' (STOP) command.  **udpcli** can also be spawned and
terminated in response to START and STOP commands that pertain specifically
to the UDP convergence layer protocol.

# EXIT STATUS

- "0"

    **udpcli** terminated normally, for reasons noted in the **ion.log** file.  If
    this termination was not commanded, investigate and solve the problem identified
    in the log file and use **bpadmin** to restart **udpcli**.

- "1"

    **udpcli** terminated abnormally, for reasons noted in the **ion.log** file.
    Investigate and solve the problem identified in the log file, then use
    **bpadmin** to restart **udpcli**.

# FILES

No configuration files are needed.

# ENVIRONMENT

No environment variables apply.

# DIAGNOSTICS

The following diagnostics may be issued to the **ion.log** log file:

- udpcli can't attach to BP.

    **bpadmin** has not yet initialized Bundle Protocol operations.

- No such udp duct.

    No UDP induct matching _local\_hostname_ and _local\_port\_nbr_ has been added
    to the BP database.  Use **bpadmin** to stop the UDP convergence-layer
    protocol, add the induct, and then restart the UDP protocol.

- CLI task is already started for this duct.

    Redundant initiation of **udpcli**.

- Can't get IP address for host

    Operating system error.  Check errtext, correct problem, and restart UDP.

- Can't open UDP socket

    Operating system error.  Check errtext, correct problem, and restart UDP.

- Can't initialize socket

    Operating system error.  Check errtext, correct problem, and restart UDP.

- udpcli can't create receiver thread

    Operating system error.  Check errtext, correct problem, and restart UDP.

# BUGS

Report bugs to <ion-dtn-support@lists.sourceforge.net>

# SEE ALSO

bpadmin(1), bprc(5), udpclo(1)
