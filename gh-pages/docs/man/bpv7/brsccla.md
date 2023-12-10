# NAME

brsccla - BRSC-based BP convergence layer adapter (input and output) task

# SYNOPSIS

**brsccla** _server\_hostname_\[:_server\_port\_nbr_\]\__own\_node\_nbr_

# DESCRIPTION

BRSC is the "client" side of the Bundle Relay Service (BRS) convergence
layer protocol for BP.  It is complemented by BRSS, the "server" side of
the BRS convergence layer protocol for BP.  BRS clients send bundles directly
only to the server, regardless of their final destinations, and the server
forwards them to other clients as necessary.

**brsccla** is a background "daemon" task comprising three threads:
one that connects to the BRS server, spawns the other threads, and then
handles BRSC protocol output by transmitting bundles over the connected
socket to the BRS server; one that simply sends periodic "keepalive"
messages over the connected socket to the server (to assure that local
inactivity doesn't cause the connection to be lost); and one that
handles BRSC protocol input from the connected server.

The output thread connects to the server's TCP socket at _server\_hostname_
and _server\_port\_nbr_, sends over the connected socket the client's
_own\_node\_nbr_ (in SDNV representation) followed by a 32-bit time tag
and a 160-bit HMAC-SHA1 digest of that time tag, to authenticate
itself; checks the authenticity of the 160-bit countersign returned by
the server; spawns the keepalive and receiver threads; and then begins
extracting bundles from the queues of bundles ready for transmission
via BRSC and transmitting those bundles over the connected socket to
the server.  Each transmitted bundle is preceded by its length, a
32-bit unsigned integer in network byte order.  The default value for
_server\_port\_nbr_, if omitted, is 80.

The reception thread receives bundles over the connected socket and passes
them to the bundle protocol agent on the local ION node.  Each bundle
received on the connection is preceded by its length, a 32-bit unsigned
integer in network byte order.

The keepalive thread simply sends a "bundle length" value of zero (a 32-bit
unsigned integer in network byte order) to the server once every 15 seconds.

**brsccla** is spawned automatically by **bpadmin** in response to the 's'
(START) command that starts operation of the Bundle Protocol, and it is
terminated by **bpadmin** in response to an 'x' (STOP) command.  **brsccla**
can also be spawned and terminated in response to START and STOP commands
that pertain specifically to the BRSC convergence layer protocol.

# EXIT STATUS

- "0"

    **brsccla** terminated normally, for reasons noted in the **ion.log** file.  If
    this termination was not commanded, investigate and solve the problem identified
    in the log file and use **bpadmin** to restart the BRSC protocol.

- "1"

    **brsccla** terminated abnormally, for reasons noted in the **ion.log** file.
    Investigate and solve the problem identified in the log file, then use
    **bpadmin** to restart the BRSC protocol.

# FILES

No configuration files are needed.

# ENVIRONMENT

No environment variables apply.

# DIAGNOSTICS

The following diagnostics may be issued to the **ion.log** log file:

- brsccla can't attach to BP.

    **bpadmin** has not yet initialized Bundle Protocol operations.

- No such brsc induct.

    No BRSC induct with duct name matching _server\_hostname_, _own\_node\_nbr_,
    and _server\_port\_nbr_ has been added to the BP database.  Use **bpadmin**
    to stop the BRSC convergence-layer protocol, add the induct, and then
    restart the BRSC protocol.

- CLI task is already started for this duct.

    Redundant initiation of **brsccla**.

- No such brsc outduct.

    No BRSC outduct with duct name matching _server\_hostname_, _own\_node\_nbr_,
    and _server\_port\_nbr_ has been added to the BP database.  Use **bpadmin**
    to stop the BRSC convergence-layer protocol, add the outduct, and then
    restart the BRSC protocol.

- Can't connect to server.

    Operating system error.  Check errtext, correct problem, and restart BRSC.

- Can't register with server.

    Configuration error.  Authentication has failed, probably because (a) the
    client and server are using different HMAC/SHA1 keys or (b) the
    clocks of the client and server differ by more than 5 seconds.  Update
    security policy database(s), as necessary, and assure that the clocks are
    synchronized.

- brsccla can't create receiver thread

    Operating system error.  Check errtext, correct problem, and restart BRSC.

- brsccla can't create keepalive thread

    Operating system error.  Check errtext, correct problem, and restart BRSC.

# BUGS

Report bugs to <ion-dtn-support@lists.sourceforge.net>

# SEE ALSO

bpadmin(1), bprc(5), brsscla(1)
