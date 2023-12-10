# NAME

dccpcli - DCCP-based BP convergence layer input task

# SYNOPSIS

**dccpcli** _local\_hostname_\[:_local\_port\_nbr_\]

# DESCRIPTION

**dccpcli** is a background "daemon" task that receives DCCP datagrams via a
DCCP socket bound to _local\_hostname_ and _local\_port\_nbr_, extracts
bundles from those datagrams, and passes them to the bundle protocol agent
on the local ION node.

If not specified, port number defaults to 4556.

Note that **dccpcli** has no fragmentation support at all. Therefore, the
largest bundle that can be sent via this convergence layer is limited to
just under the link's MTU (typically 1500 bytes).

The convergence layer input task is spawned automatically by **bpadmin** in
response to the 's' (START) command that starts operation of the Bundle
Protocol; the text of the command that is used to spawn the task must be
provided at the time the "dccp" convergence layer protocol is added to the BP
database.  The convergence layer input task is terminated by **bpadmin**
in response to an 'x' (STOP) command.  **dccpcli** can also be spawned and
terminated in response to START and STOP commands that pertain specifically
to the DCCP convergence layer protocol.

# EXIT STATUS

- "0"

    **dccpcli** terminated normally, for reasons noted in the **ion.log** file.  If
    this termination was not commanded, investigate and solve the problem identified
    in the log file and use **bpadmin** to restart **dccpcli**.

- "1"

    **dccpcli** terminated abnormally, for reasons noted in the **ion.log** file.
    Investigate and solve the problem identified in the log file, then use
    **bpadmin** to restart **dccpcli**.

# FILES

No configuration files are needed.

# ENVIRONMENT

No environment variables apply.

# DIAGNOSTICS

The following diagnostics may be issued to the **ion.log** log file:

- dccpcli can't attach to BP.

    **bpadmin** has not yet initialized Bundle Protocol operations.

- No such dccp duct.

    No DCCP induct matching _local\_hostname_ and _local\_port\_nbr_ has been added
    to the BP database.  Use **bpadmin** to stop the DCCP convergence-layer
    protocol, add the induct, and then restart the DCCP protocol.

- CLI task is already started for this duct.

    Redundant initiation of **dccpcli**.

- dccpcli can't get IP address for host.

    Operating system error.  Check errtext, correct problem, and restart **dccpcli**.

- CLI can't open DCCP socket. This probably means DCCP is not supported on your system.

    Operating system error. This probably means that you are not using an
    operating system that supports DCCP. Make sure that you are using a current
    Linux kernel and that the DCCP modules are being compiled. Check errtext, 
    correct problem, and restart **dccpcli**.

- CLI can't initialize socket.

    Operating system error.  Check errtext, correct problem, and restart **dccpcli**.

- dccpcli can't get acquisition work area.

    ION system error.  Check errtext, correct problem, and restart **dccpcli**.

- dccpcli can't create new thread.

    Operating system error.  Check errtext, correct problem, and restart **dccpcli**.

# BUGS

Report bugs to <ion-dtn-support@lists.sourceforge.net>

# SEE ALSO

bpadmin(1), bprc(5), dccpclo(1)
