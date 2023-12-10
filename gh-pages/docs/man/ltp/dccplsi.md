# NAME

dccplsi - DCCP-based LTP link service input task

# SYNOPSIS

**dccplsi** {_local\_hostname_ | @}\[:_local\_port\_nbr_\]

# DESCRIPTION

**dccplsi** is a background "daemon" task that receives DCCP datagrams via a
DCCP socket bound to _local\_hostname_ and _local\_port\_nbr_, extracts LTP
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

    **dccplsi** terminated normally, for reasons noted in the **ion.log** file.  If
    this termination was not commanded, investigate and solve the problem identified
    in the log file and use **ltpadmin** to restart **dccplsi**.

- "1"

    **dccplsi** terminated abnormally, for reasons noted in the **ion.log** file.
    Investigate and solve the problem identified in the log file, then use
    **ltpadmin** to restart **dccplsi**.

# FILES

No configuration files are needed.

# ENVIRONMENT

No environment variables apply.

# DIAGNOSTICS

The following diagnostics may be issued to the **ion.log** log file:

- dccplsi can't initialize LTP.

    **ltpadmin** has not yet initialized LTP protocol operations.

- LSI task is already started.

    Redundant initiation of **dccplsi**.

- LSI can't open DCCP socket. This probably means DCCP is not supported on your system.

    Operating system error. This probably means that you are not using an
    operating system that supports DCCP. Make sure that you are using a current
    Linux kernel and that the DCCP modules are being compiled. Check errtext,
    correct problem, and restart **dccplsi**.

- LSI can't initialize socket.

    Operating system error.  Check errtext, correct problem, and restart **dccplsi**.

- LSI can't create listener thread.

    Operating system error.  Check errtext, correct problem, and restart **dccplsi**.

# BUGS

Report bugs to <ion-dtn-support@lists.sourceforge.net>

# SEE ALSO

ltpadmin(1), dccplso(1), owltsim(1)
