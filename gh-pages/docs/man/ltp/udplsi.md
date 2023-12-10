# NAME

udplsi - UDP-based LTP link service input task

# SYNOPSIS

**udplsi** {_local\_hostname_ | @}\[:_local\_port\_nbr_\]

# DESCRIPTION

**udplsi** is a background "daemon" task that receives UDP datagrams via a
UDP socket bound to _local\_hostname_ and _local\_port\_nbr_, extracts LTP
segments from those datagrams, and passes them to the local LTP engine.
Host name "@" signifies that the host name returned by hostname(1) is to
be used as the socket's host name.  If not specified, port number defaults
to 1113.

The link service input task is spawned automatically by **ltpadmin** in
response to the 's' command that starts operation of the LTP protocol;
the text of the command that is used to spawn the task must be provided
as a parameter to the 's' command.  The link service input task is
terminated by **ltpadmin** in response to an 'x' (STOP) command.

# EXIT STATUS

- "0"

    **udplsi** terminated normally, for reasons noted in the **ion.log** file.  If
    this termination was not commanded, investigate and solve the problem identified
    in the log file and use **ltpadmin** to restart **udplsi**.

- "1"

    **udplsi** terminated abnormally, for reasons noted in the **ion.log** file.
    Investigate and solve the problem identified in the log file, then use
    **ltpadmin** to restart **udplsi**.

# FILES

No configuration files are needed.

# ENVIRONMENT

No environment variables apply.

# DIAGNOSTICS

The following diagnostics may be issued to the **ion.log** log file:

- udplsi can't initialize LTP.

    **ltpadmin** has not yet initialized LTP protocol operations.

- LSI task is already started.

    Redundant initiation of **udplsi**.

- LSI can't open UDP socket

    Operating system error.  Check errtext, correct problem, and restart **udplsi**.

- LSI can't initialize socket

    Operating system error.  Check errtext, correct problem, and restart **udplsi**.

- LSI can't create receiver thread

    Operating system error.  Check errtext, correct problem, and restart **udplsi**.

# BUGS

Report bugs to <ion-dtn-support@lists.sourceforge.net>

# SEE ALSO

ltpadmin(1), udplso(1), owltsim(1)
