# NAME

dccpclo - DCCP-based BP convergence layer output task

# SYNOPSIS

**dccpclo** _remote\_hostname_\[:_remote\_port\_nbr_\]

# DESCRIPTION

**dccpclo** is a background "daemon" task that connects to a remote node's
DCCP socket at _remote\_hostname_ and _remote\_port\_nbr_. It then begins
extracting bundles from the queues of bundles ready for transmission via
DCCP to this remote bundle protocol agent and transmitting those bundles
as DCCP datagrams to the remote host.

If not specified, _remote\_port\_nbr_ defaults to 4556.

Note that **dccpclo** has no fragmentation support at all. Therefore, the
largest bundle that can be sent via this convergence layer is limited to
just under the link's MTU (typically 1500 bytes).

**dccpclo** is spawned automatically by **bpadmin** in response to the 's' (START)
command that starts operation of the Bundle Protocol, and it is terminated by
**bpadmin** in response to an 'x' (STOP) command.  **dccpclo** can also be
spawned and terminated in response to START and STOP commands that pertain
specifically to the DCCP convergence layer protocol.

# EXIT STATUS

- "0"

    **dccpclo** terminated normally, for reasons noted in the **ion.log** file.  If
    this termination was not commanded, investigate and solve the problem identified
    in the log file and use **bpadmin** to restart **dccpclo**.

- "1"

    **dccpclo** terminated abnormally, for reasons noted in the **ion.log** file.
    Investigate and solve the problem identified in the log file, then use
    **bpadmin** to restart **dccpclo**.

# FILES

No configuration files are needed.

# ENVIRONMENT

No environment variables apply.

# DIAGNOSTICS

The following diagnostics may be issued to the **ion.log** log file:

- dccpclo can't attach to BP.

    **bpadmin** has not yet initialized Bundle Protocol operations.

- No memory for DCCP buffer in dccpclo.

    ION system error.  Check errtext, correct problem, and restart **dccpclo**.

- No such dccp duct.

    No DCCP outduct matching _local\_hostname_ and _local\_port\_nbr_ has been
    added to the BP database.  Use **bpadmin** to stop the DCCP convergence-layer
    protocol, add the outduct, and then restart **dccpclo**.

- CLO task is already started for this duct.

    Redundant initiation of **dccpclo**.

- dccpclo can't get IP address for host.

    Operating system error.  Check errtext, correct problem, and restart **dccpclo**.

- dccpclo can't create thread.

    Operating system error.  Check errtext, correct problem, and restart **dccpclo**.

- CLO can't open DCCP socket. This probably means DCCP is not supported on your system.

    Operating system error. This probably means that you are not using an
    operating system that supports DCCP. Make sure that you are using a current
    Linux kernel and that the DCCP modules are being compiled. Check errtext, 
    correct problem, and restart **dccpclo**.

- CLO can't initialize socket.

    Operating system error.  Check errtext, correct problem, and restart **dccpclo**.

- CLO send() error on socket.

    Operating system error.  Check errtext, correct problem, and restart **dccpclo**.

- Bundle is too big for DCCP CLO.

    Configuration error: bundles that are too large for DCCP transmission (i.e.,
    larger than the MTU of the link or 65535 bytes--whichever is smaller) are being
    enqueued for **dccpclo**.  Change routing or use smaller bundles.

# BUGS

Report bugs to <ion-dtn-support@lists.sourceforge.net>

# SEE ALSO

bpadmin(1), bprc(5), dccpcli(1)
