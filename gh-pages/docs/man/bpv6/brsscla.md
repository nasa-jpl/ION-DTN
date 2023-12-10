# NAME

brsscla - BRSS-based BP convergence layer adapter (input and output) task

# SYNOPSIS

**brsscla** _local\_hostname_\[:_local\_port\_nbr_\]

# DESCRIPTION

BRSS is the "server" side of the Bundle Relay Service (BRS) convergence
layer protocol for BP.  It is complemented by BRSC, the "client" side of
the BRS convergence layer protocol for BP.

**brsscla** is a background "daemon" task that spawns 2\*N threads: one that
handles BRSS client connections and spawns sockets for continued data
interchange with connected clients; one that handles BRSS protocol output
by transmitting over those spawned sockets to the associated clients; and
two thread for each spawned socket, an input thread to handle BRSS protocol
input from the associated connected client and an output thread to forward
BRSS protocol output to the associated connected client.

The connection thread simply accepts connections on a TCP socket bound to
_local\_hostname_ and _local\_port\_nbr_ and spawns reception threads.  The
default value for _local\_port\_nbr_, if omitted, is 80.

Each reception thread receives over the socket connection the node number
of the connecting client (in SDNV representation), followed by a 32-bit time
tag and a 160-bit HMAC-SHA1 digest of that time tag.  The receiving thread
checks the time tag, requiring that it differ from the current time by no
more than BRSTERM (default value 5) seconds.  It then recomputes the digest
value using the HMAC-SHA1 key named "_node\_number_.brs" as recorded in the
ION security database (see ionsecrc(5)), requiring that the supplied and
computed digests be identical.  If all registration conditions are met, the
receiving thread sends the client a countersign -- a similarly computed
HMAC-SHA1 digest, for the time tag that is 1 second later than the provided
time tag -- to assure the client of its own authenticity, then commences
receiving bundles over the connected socket.  Each bundle received on the
connection is preceded by its length, a 32-bit unsigned integer in network
byte order.  The received bundles are passed to the bundle protocol agent
on the local ION node.

Each output thread extracts bundles from the queues of bundles ready for
transmission via BRSS to the corresponding connected client and transmits the
bundles over the socket to that client.  Each transmitted bundle is
preceded by its length, a 32-bit unsigned integer in network byte order.

**brsscla** is spawned automatically by **bpadmin** in response to the 's' (START)
command that starts operation of the Bundle Protocol, and it is terminated by
**bpadmin** in response to an 'x' (STOP) command.  **brsscla** can also be
spawned and terminated in response to START and STOP commands that pertain
specifically to the BRSS convergence layer protocol.

# EXIT STATUS

- "0"

    **brsscla** terminated normally, for reasons noted in the **ion.log** file.  If
    this termination was not commanded, investigate and solve the problem identified
    in the log file and use **bpadmin** to restart the BRSS protocol.

- "1"

    **brsscla** terminated abnormally, for reasons noted in the **ion.log** file.
    Investigate and solve the problem identified in the log file, then use
    **bpadmin** to restart the BRSS protocol.

# FILES

No configuration files are needed.

# ENVIRONMENT

No environment variables apply.

# DIAGNOSTICS

The following diagnostics may be issued to the **ion.log** log file:

- brsscla can't attach to BP.

    **bpadmin** has not yet initialized Bundle Protocol operations.

- No such brss induct.

    No BRSS induct with duct name matching _local\_hostname_ and _local\_port\_nbr_
    has been added to the BP database.  Use **bpadmin** to stop the BRSS
    convergence-layer protocol, add the induct, and then restart the BRSS protocol.

- CLI task is already started for this duct.

    Redundant initiation of **brsscla**.

- Can't get IP address for host

    Operating system error.  Check errtext, correct problem, and restart BRSS.

- Can't open TCP socket

    Operating system error -- unable to open TCP socket for accepting connections.
    Check errtext, correct problem, and restart BRSS.

- Can't initialize socket (note: must be root for port 80)

    Operating system error.  Check errtext, correct problem, and restart BRSS.

- brsscla can't create sender thread

    Operating system error.  Check errtext, correct problem, and restart BRSS.

- brsscla can't create access thread

    Operating system error.  Check errtext, correct problem, and restart BRSS.

# BUGS

Report bugs to <ion-dtn-support@lists.sourceforge.net>

# SEE ALSO

bpadmin(1), bprc(5), brsccla(1)
