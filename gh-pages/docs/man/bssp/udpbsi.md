# NAME

udpbsi - UDP-based best-effort link service input task for BSSP

# SYNOPSIS

**udpbsi** {_local\_hostname_ | @}\[:_local\_port\_nbr_\]

# DESCRIPTION

**udpbsi** is a background "daemon" task that receives UDP datagrams via a
UDP socket bound to _local\_hostname_ and _local\_port\_nbr_, extracts BSSP
PDUs from those datagrams, and passes them to the local BSSP engine.
Host name "@" signifies that the host name returned by hostname(1) is to
be used as the socket's host name.  If not specified, port number defaults
to 6001.

The link service input task is spawned automatically by **bsspadmin** in
response to the 's' command that starts operation of the BSSP protocol;
the text of the command that is used to spawn the task must be provided
as a parameter to the 's' command.  The link service input task is
terminated by **bsspadmin** in response to an 'x' (STOP) command.

# EXIT STATUS

- "0"

    **udpbsi** terminated normally, for reasons noted in the **ion.log** file.  If
    this termination was not commanded, investigate and solve the problem identified
    in the log file and use **bsspadmin** to restart **udpbsi**.

- "1"

    **udpbsi** terminated abnormally, for reasons noted in the **ion.log** file.
    Investigate and solve the problem identified in the log file, then use
    **bsspadmin** to restart **udpbsi**.

# FILES

No configuration files are needed.

# ENVIRONMENT

No environment variables apply.

# DIAGNOSTICS

The following diagnostics may be issued to the **ion.log** log file:

- udpbsi can't initialize BSSP.

    **bsspadmin** has not yet initialized BSSP protocol operations.

- BE-BSI task is already started.

    Redundant initiation of **udpbsi**.

- BE-BSI can't open UDP socket

    Operating system error.  Check errtext, correct problem, and restart **udpbsi**.

- BE-BSI can't initialize socket

    Operating system error.  Check errtext, correct problem, and restart **udpbsi**.

- udpbsi can't create receiver thread

    Operating system error.  Check errtext, correct problem, and restart **udpbsi**.

# BUGS

Report bugs to <ion-dtn-support@lists.sourceforge.net>

# SEE ALSO

bsspadmin(1), tcpbsi(1), udpbso(1)
