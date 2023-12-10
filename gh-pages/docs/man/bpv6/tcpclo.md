# NAME

tcpclo - DTN TCPCL-compliant convergence layer adapter output task

# SYNOPSIS

**tcpclo** _remote\_hostname_\[:_remote\_port\_nbr_\]

# DESCRIPTION

**tcpclo** is a background "daemon" task that connects to a remote node's
TCP socket at _remote\_hostname_ and _remote\_port\_nbr_.  It sends
a contact header, and it records the acknowledgement flag, reactive
fragmentation flag and negative acknowledgements flag in the contact
header it receives from its peer **tcpcli** task.  It then begins
extracting bundles from the queues of bundles ready for transmission via
TCP to this remote bundle protocol agent and transmitting those bundles
over the connected socket to that node.  Each transmitted bundle is
preceded by message type, segmentation flags, and an SDNV indicating the
size of the bundle (in bytes).

If not specified, _remote\_port\_nbr_ defaults to 4556.

**tcpclo** is spawned automatically by **bpadmin** in response to the 's' (START)
command that starts operation of the Bundle Protocol, and it is terminated by
**bpadmin** in response to an 'x' (STOP) command.  **tcpclo** can also be
spawned and terminated in response to START and STOP commands that pertain
specifically to the TCP convergence layer protocol.

# EXIT STATUS

- "0"

    **tcpclo** terminated normally, for reasons noted in the **ion.log** file.  If
    this termination was not commanded, investigate and solve the problem identified
    in the log file and use **bpadmin** to restart the TCPCL protocol.

- "1"

    **tcpclo** terminated abnormally, for reasons noted in the **ion.log** file.
    Investigate and solve the problem identified in the log file, then use
    **bpadmin** to restart the TCPCL protocol.

# FILES

No configuration files are needed.

# ENVIRONMENT

No environment variables apply.

# DIAGNOSTICS

The following diagnostics may be issued to the **ion.log** log file:

- tcpclo can't attach to BP.

    **bpadmin** has not yet initialized Bundle Protocol operations.

- No such tcp duct.

    No TCP outduct with duct name matching _remote\_hostname_ and
    _remote\_port\_nbr_ has been added to the BP database.  Use **bpadmin** to
    stop the TCP convergence-layer protocol, add the outduct, and then restart the
    TCP protocol.

- CLO task is already started for this duct.

    Redundant initiation of **tcpclo**.

- Can't get IP address for host

    Operating system error.  Check errtext, correct problem, and restart TCP.

# BUGS

Report bugs to <ion-dtn-support@lists.sourceforge.net>

# SEE ALSO

bpadmin(1), bprc(5), tcpcli(1)
