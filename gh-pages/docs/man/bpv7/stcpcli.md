# NAME

sstcpcli - DTN simple TCP convergence layer input task

# SYNOPSIS

**stcpcli** _local\_hostname_\[:_local\_port\_nbr_\]

# DESCRIPTION

**stcpcli** is a background "daemon" task comprising 1 + N threads: one that
handles TCP connections from remote **stcpclo** tasks, spawning sockets for
data reception from those tasks, plus one input thread for each spawned
socket to handle data reception over that socket.

The connection thread simply accepts connections on a TCP socket bound to
_local\_hostname_ and _local\_port\_nbr_ and spawns reception threads.  The
default value for _local\_port\_nbr_, if omitted, is 4556.

Each reception thread receives bundles over the associated connected socket.
Each bundle received on the connection is preceded by a 32-bit unsigned
integer in network byte order indicating the length of the bundle.  The
received bundles are passed to the bundle protocol agent on the local ION node.

**stcpcli** is spawned automatically by **bpadmin** in response to the 's'
(START) command that starts operation of the Bundle Protocol; the text
of the command that is used to spawn the task must be provided at the
time the "stcp" convergence layer protocol is added to the BP database.
The convergence layer input task is terminated by **bpadmin** in
response to an 'x' (STOP) command.  **stcpcli** can also be spawned and
terminated in response to START and STOP commands that pertain specifically
to the STCP convergence layer protocol.

# EXIT STATUS

- "0"

    **stcpcli** terminated normally, for reasons noted in the **ion.log** file.  If
    this termination was not commanded, investigate and solve the problem identified
    in the log file and use **bpadmin** to restart **stcpcli**.

- "1"

    **stcpcli** terminated abnormally, for reasons noted in the **ion.log** file.
    Investigate and solve the problem identified in the log file, then use
    **bpadmin** to restart **stcpcli**.

# FILES

No configuration files are needed.

# ENVIRONMENT

No environment variables apply.

# DIAGNOSTICS

The following diagnostics may be issued to the **ion.log** log file:

- stcpcli can't attach to BP.

    **bpadmin** has not yet initialized Bundle Protocol operations.

- No such stcp duct.

    No STCP induct matching _local\_hostname_ and _local\_port\_nbr_ has been added
    to the BP database.  Use **bpadmin** to stop the STCP convergence-layer
    protocol, add the induct, and then restart the STCP protocol.

- CLI task is already started for this duct.

    Redundant initiation of **stcpcli**.

- Can't get IP address for host

    Operating system error.  Check errtext, correct problem, and restart STCP.

- Can't open TCP socket

    Operating system error.  Check errtext, correct problem, and restart STCP.

- Can't initialize socket

    Operating system error.  Check errtext, correct problem, and restart STCP.

- stcpcli can't create access thread

    Operating system error.  Check errtext, correct problem, and restart STCP.

# BUGS

Report bugs to <ion-dtn-support@lists.sourceforge.net>

# SEE ALSO

bpadmin(1), bprc(5), stcpclo(1)
