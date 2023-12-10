# NAME

tcpbso - TCP-based reliable link service output task for BSSP

# SYNOPSIS

**tcpbso** {_remote\_engine\_hostname_ | @}\[:_remote\_port\_nbr_\] _remote\_engine\_nbr_

# DESCRIPTION

**tcpbso** is a background "daemon" task that extracts BSSP blocks from the
queue of blocks bound for the indicated remote BSSP engine and uses a TCP
socket to send them to the indicated TCP port on the indicated host.  If not
specified, port number defaults to 4556.

Each "span" of BSSP data interchange between the local BSSP engine and a
neighboring BSSP engine requires its own best-effort and reliable link service
output tasks. All link service output tasks are spawned automatically by
**bsspadmin** in response to the 's' command that starts operation of the
BSSP protocol, and they are all terminated by **bsspadmin** in response to an
'x' (STOP) command.

# EXIT STATUS

- "0"

    **tcpbso** terminated normally, for reasons noted in the **ion.log** file.  If
    this termination was not commanded, investigate and solve the problem identified
    in the log file and use **bsspadmin** to restart **tcpbso**.

- "1"

    **tcpbso** terminated abnormally, for reasons noted in the **ion.log** file.
    Investigate and solve the problem identified in the log file, then use
    **bsspadmin** to restart **tcpbso**.

# FILES

No configuration files are needed.

# ENVIRONMENT

No environment variables apply.

# DIAGNOSTICS

The following diagnostics may be issued to the **ion.log** log file:

- tcpbso can't initialize BSSP.

    **bsspadmin** has not yet initialized BSSP protocol operations.

- No such engine in database.

    _remote\_engine\_nbr_ is invalid, or the applicable span has not yet
    been added to the BSSP database by **bsspadmin**.

- RL-BSO task is already started for this engine.

    Redundant initiation of **tcpbso**.

- RL-BSO can't open TCP socket

    Operating system error.  Check errtext, correct problem, and restart **tcpbso**.

- RL-BSO can't bind TCP socket

    Operating system error.  Check errtext, correct problem, and restart **tcpbso**.

# BUGS

Report bugs to <ion-dtn-support@lists.sourceforge.net>

# SEE ABSO

bsspadmin(1), tcpbsi(1), udpbso(1)
