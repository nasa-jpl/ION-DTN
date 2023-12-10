# NAME

tcpcli - DTN TCPCL-compliant convergence layer input task

# SYNOPSIS

**tcpcli** _local\_hostname_\[:_local\_port\_nbr_\]

# DESCRIPTION

**tcpcli** is a background "daemon" task comprising 3 + 2\*N threads: an
executive thread; a clock thread that periodically attempts to connect
to remote TCPCL entities as identified by the tcp outducts enumerated in
the bprc(5) file (each of which must specify the _hostname_\[:_port\_nbr_\]
to connect to); a thread that handles TCP connections from remote TCPCL
entities, spawning sockets for data reception from those tasks; plus one
input thread and one output thread for each connection, to handle data
reception and transmission over that socket.

The connection thread simply accepts connections on a TCP socket bound to
_local\_hostname_ and _local\_port\_nbr_ and spawns reception threads.  The
default value for _local\_port\_nbr_, if omitted, is 4556.

Each time a connection is established, the entities will first exchange
contact headers, because connection parameters need to be negotiated.
**tcpcli** records the acknowledgement flags, reactive fragmentation flag,
and negative acknowledgements flag in the contact header it receives from
its peer TCPCL entity.

Each reception thread receives bundles over the associated connected socket.
Each bundle received on the connection is preceded by message type,
fragmentation flags, and size represented as an SDNV.  The received bundles
are passed to the bundle protocol agent on the local ION node.

Similarly, each transmission thread obtains outbound bundles from the local
ION node, encapsulates them as noted above, and transmits them over the
associated connected socket.

**tcpcli** is spawned automatically by **bpadmin** in response to the 's'
(START) command that starts operation of the Bundle Protocol; the text
of the command that is used to spawn the task must be provided at the
time the "tcp" convergence layer protocol is added to the BP database.
The convergence layer input task is terminated by **bpadmin** in
response to an 'x' (STOP) command.  **tcpcli** can also be spawned and
terminated in response to START and STOP commands that pertain specifically
to the TCP convergence layer protocol.

# EXIT STATUS

- "0"

    **tcpcli** terminated normally, for reasons noted in the **ion.log** file.  If
    this termination was not commanded, investigate and solve the problem identified
    in the log file and use **bpadmin** to restart **tcpcli**.

- "1"

    **tcpcli** terminated abnormally, for reasons noted in the **ion.log** file.
    Investigate and solve the problem identified in the log file, then use
    **bpadmin** to restart **tcpcli**.

# FILES

No configuration files are needed.

# ENVIRONMENT

No environment variables apply.

# DIAGNOSTICS

The following diagnostics may be issued to the **ion.log** log file:

- tcpcli can't attach to BP.

    **bpadmin** has not yet initialized Bundle Protocol operations.

- No such tcp duct.

    No TCP induct matching _local\_hostname_ and _local\_port\_nbr_ has been added
    to the BP database.  Use **bpadmin** to stop the TCP convergence-layer
    protocol, add the induct, and then restart the TCP protocol.

- CLI task is already started for this duct.

    Redundant initiation of **tcpcli**.

- Can't get IP address for host

    Operating system error.  Check errtext, correct problem, and restart TCP.

- Can't open TCP socket

    Operating system error.  Check errtext, correct problem, and restart TCP.

- Can't initialize socket

    Operating system error.  Check errtext, correct problem, and restart TCP.

- tcpcli can't create access thread

    Operating system error.  Check errtext, correct problem, and restart TCP.

# BUGS

Report bugs to <ion-dtn-support@lists.sourceforge.net>

# SEE ALSO

bpadmin(1), bprc(5)
