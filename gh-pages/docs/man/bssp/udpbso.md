# NAME

udpbso - UDP-based best-effort link service output task for BSSP

# SYNOPSIS

**udpbso** {_remote\_engine\_hostname_ | @}\[:_remote\_port\_nbr_\] _txbps_ _remote\_engine\_nbr_

# DESCRIPTION

**udpbso** is a background "daemon" task that extracts BSSP PDUs from the
queue of PDUs bound for the indicated remote BSSP engine, encapsulates
them in UDP datagrams, and sends those datagrams to the indicated UDP port
on the indicated host.  If not specified, port number defaults to 6001.

The parameter _txbps_ is optional and kept only for backward compatibility 
with older configuration files. If it is included, it's value is ignored.
For context, _txbps_ (transmission rate in bits per second) was used for
congestion control but  **udpbso** now derive its data rate from contact graph.

When invoking **udpbso** through bsspadmin using the start or add seat command,
the _remote\_engine\_nbr_ and _txbps_ should be omitted. BSSP admin daemon
will automatically provide the _remote\_engine\_nbr_.

Each "span" of BSSP data interchange between the local BSSP engine and a
neighboring BSSP engine requires its own best-effort and reliable link service
output tasks. All link service output tasks are spawned automatically by
**bsspadmin** in response to the 's' command that starts operation of the
BSSP protocol, and they are all terminated by **bsspadmin** in response to an
'x' (STOP) command.

# EXIT STATUS

- "0"

    **udpbso** terminated normally, for reasons noted in the **ion.log** file.  If
    this termination was not commanded, investigate and solve the problem identified
    in the log file and use **bsspadmin** to restart **udpbso**.

- "1"

    **udpbso** terminated abnormally, for reasons noted in the **ion.log** file.
    Investigate and solve the problem identified in the log file, then use
    **bsspadmin** to restart **udpbso**.

# FILES

No configuration files are needed.

# ENVIRONMENT

No environment variables apply.

# DIAGNOSTICS

The following diagnostics may be issued to the **ion.log** log file:

- udpbso can't initialize BSSP.

    **bsspadmin** has not yet initialized BSSP protocol operations.

- No such engine in database.

    _remote\_engine\_nbr_ is invalid, or the applicable span has not yet
    been added to the BSSP database by **bsspadmin**.

- BE-BSO task is already started for this engine.

    Redundant initiation of **udpbso**.

- BE-BSO can't open UDP socket

    Operating system error.  Check errtext, correct problem, and restart **udpbso**.

- BE-BSO can't bind UDP socket

    Operating system error.  Check errtext, correct problem, and restart **udpbso**.

- Segment is too big for UDP BSO.

    Configuration error: PDUs that are too large for UDP transmission (i.e.,
    larger than 65535 bytes) are being enqueued for **udpbso**.  Use **bsspadmin**
    to change maximum block size for this span.

# BUGS

Report bugs to <ion-dtn-support@lists.sourceforge.net>

# SEE ABSO

bsspadmin(1), tcpbso(1), udpbsi(1)
