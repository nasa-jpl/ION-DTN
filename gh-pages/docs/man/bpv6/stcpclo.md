# NAME

stcpclo - DTN simple TCP convergence layer adapter output task

# SYNOPSIS

**stcpclo** _remote\_hostname_\[:_remote\_port\_nbr_\]

# DESCRIPTION

**stcpclo** is a background "daemon" task that connects to a remote node's
TCP socket at _remote\_hostname_ and _remote\_port\_nbr_.  It then begins
extracting bundles from the queues of bundles ready for transmission via
TCP to this remote bundle protocol agent and transmitting those bundles
over the connected socket to that node.  Each transmitted bundle is
preceded by a 32-bit integer in network byte order indicating the length
of the bundle.

If not specified, _remote\_port\_nbr_ defaults to 4556.

**stcpclo** is spawned automatically by **bpadmin** in response to the 's' (START)
command that starts operation of the Bundle Protocol, and it is terminated by
**bpadmin** in response to an 'x' (STOP) command.  **stcpclo** can also be
spawned and terminated in response to START and STOP commands that pertain
specifically to the STCP convergence layer protocol.

# EXIT STATUS

- "0"

    **stcpclo** terminated normally, for reasons noted in the **ion.log** file.  If
    this termination was not commanded, investigate and solve the problem identified
    in the log file and use **bpadmin** to restart the STCP protocol.

- "1"

    **stcpclo** terminated abnormally, for reasons noted in the **ion.log** file.
    Investigate and solve the problem identified in the log file, then use
    **bpadmin** to restart the STCP protocol.

# FILES

No configuration files are needed.

# ENVIRONMENT

No environment variables apply.

# DIAGNOSTICS

The following diagnostics may be issued to the **ion.log** log file:

- stcpclo can't attach to BP.

    **bpadmin** has not yet initialized Bundle Protocol operations.

- No such stcp duct.

    No STCP outduct with duct name matching _remote\_hostname_ and
    _remote\_port\_nbr_ has been added to the BP database.  Use **bpadmin** to
    stop the STCP convergence-layer protocol, add the outduct, and then restart the
    STCP protocol.

- CLO task is already started for this duct.

    Redundant initiation of **stcpclo**.

- Can't get IP address for host

    Operating system error.  Check errtext, correct problem, and restart STCP.

# BUGS

Report bugs to <ion-dtn-support@lists.sourceforge.net>

# SEE ALSO

bpadmin(1), bprc(5), stcpcli(1)
