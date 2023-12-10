# NAME

dccplso - DCCP-based LTP link service output task

# SYNOPSIS

**dccplso** {_remote\_engine\_hostname_ | @}\[:_remote\_port\_nbr_\] _remote\_engine\_nbr_

# DESCRIPTION

**dccplso** is a background "daemon" task that extracts LTP segments from the
queue of segments bound for the indicated remote LTP engine, encapsulates
them in DCCP datagrams, and sends those datagrams to the indicated DCCP port
on the indicated host.  If not specified, port number defaults to 1113.

Each "span" of LTP data interchange between the local LTP engine and a
neighboring LTP engine requires its own link service output task, such
as **dccplso**.  All link service output tasks are spawned automatically by
**ltpadmin** in response to the 's' command that starts operation of the
LTP protocol, and they are all terminated by **ltpadmin** in response to an
'x' (STOP) command.

# EXIT STATUS

- "0"

    **dccplso** terminated normally, for reasons noted in the **ion.log** file.  If
    this termination was not commanded, investigate and solve the problem identified
    in the log file and use **ltpadmin** to restart **dccplso**.

- "1"

    **dccplso** terminated abnormally, for reasons noted in the **ion.log** file.
    Investigate and solve the problem identified in the log file, then use
    **ltpadmin** to restart **dccplso**.

# FILES

No configuration files are needed.

# ENVIRONMENT

No environment variables apply.

# DIAGNOSTICS

The following diagnostics may be issued to the **ion.log** log file:

- dccplso can't initialize LTP.

    **ltpadmin** has not yet initialized LTP protocol operations.

- No such engine in database.

    _remote\_engine\_nbr_ is invalid, or the applicable span has not yet
    been added to the LTP database by **ltpadmin**.

- LSO task is already started for this engine.

    Redundant initiation of **dccplso**.

- LSO can't create idle thread.

    Operating system error.  Check errtext, correct problem, and restart **dccplso**.

- LSO can't open DCCP socket. This probably means DCCP is not supported on your system.

    Operating system error. This probably means that you are not using an
    operating system that supports DCCP. Make sure that you are using a current
    Linux kernel and that the DCCP modules are being compiled. Check errtext,
    correct problem, and restart **dccplso**.

- LSO can't connect DCCP socket.

    Remote host's **dccplsi** isn't listening or has terminated. Restart **dccplsi**
    on the remote host and then restart **dccplso**.

- Segment is too big for DCCP LSO.

    Configuration error: segments that are too large for DCCP transmission (i.e.,
    larger than 65535 bytes) are being enqueued for **dccplso**.  Use **ltpadmin**
    to change maximum segment size for this span.

# BUGS

Report bugs to <ion-dtn-support@lists.sourceforge.net>

# SEE ALSO

ltpadmin(1), ltpmeter(1), dccplsi(1), owltsim(1)
