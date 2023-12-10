# NAME

tcpbsi - TCP-based reliable link service input task for BSSP

# SYNOPSIS

**tcpbsi** {_local\_hostname_ | @}\[:_local\_port\_nbr_\]

# DESCRIPTION

**tcpbsi** is a background "daemon" task that receives TCP stream data via a
TCP socket bound to _local\_hostname_ and _local\_port\_nbr_, extracts BSSP
blocks from that stream, and passes them to the local BSSP engine.
Host name "@" signifies that the host name returned by hostname(1) is to
be used as the socket's host name.  If not specified, port number defaults
to 4556.

The link service input task is spawned automatically by **bsspadmin** in
response to the 's' command that starts operation of the BSSP protocol;
the text of the command that is used to spawn the task must be provided
as a parameter to the 's' command.  The link service input task is
terminated by **bsspadmin** in response to an 'x' (STOP) command.

# EXIT STATUS

- "0"

    **tcpbsi** terminated normally, for reasons noted in the **ion.log** file.  If
    this termination was not commanded, investigate and solve the problem identified
    in the log file and use **bsspadmin** to restart **tcpbsi**.

- "1"

    **tcpbsi** terminated abnormally, for reasons noted in the **ion.log** file.
    Investigate and solve the problem identified in the log file, then use
    **bsspadmin** to restart **tcpbsi**.

# FILES

No configuration files are needed.

# ENVIRONMENT

No environment variables apply.

# DIAGNOSTICS

The following diagnostics may be issued to the **ion.log** log file:

- tcpbsi can't initialize BSSP.

    **bsspadmin** has not yet initialized BSSP protocol operations.

- RL-BSI task is already started.

    Redundant initiation of **tcpbsi**.

- RL-BSI can't open TCP socket

    Operating system error.  Check errtext, correct problem, and restart **tcpbsi**.

- RL-BSI can't initialize socket

    Operating system error.  Check errtext, correct problem, and restart **tcpbsi**.

- tcpbsi can't create receiver thread

    Operating system error.  Check errtext, correct problem, and restart **tcpbsi**.

# BUGS

Report bugs to <ion-dtn-support@lists.sourceforge.net>

# SEE ALSO

bsspadmin(1), tcpbso(1), udpbsi(1)
