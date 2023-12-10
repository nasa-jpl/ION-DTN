# NAME

udplso - UDP-based LTP link service output task

# SYNOPSIS

**udplso** {_remote\_engine\_hostname_ | @}\[:_remote\_port\_nbr_\] _remote\_engine\_nbr_

# DESCRIPTION

**udplso** is a background "daemon" task that extracts LTP segments from the
queue of segments bound for the indicated remote LTP engine, encapsulates
them in UDP datagrams, and sends those datagrams to the indicated UDP port
on the indicated host.  If not specified, port number defaults to 1113.

Each "span" of LTP data interchange between the local LTP engine and a
neighboring LTP engine requires its own link service output task, such
as **udplso**.  All link service output tasks are spawned automatically by
**ltpadmin** in response to the 's' command that starts operation of the
LTP protocol, and they are all terminated by **ltpadmin** in response to an
'x' (STOP) command.

# EXIT STATUS

- "0"

    **udplso** terminated normally, for reasons noted in the **ion.log** file.  If
    this termination was not commanded, investigate and solve the problem identified
    in the log file and use **ltpadmin** to restart **udplso**.

- "1"

    **udplso** terminated abnormally, for reasons noted in the **ion.log** file.
    Investigate and solve the problem identified in the log file, then use
    **ltpadmin** to restart **udplso**.

# FILES

No configuration files are needed.

# ENVIRONMENT

No environment variables apply.

# DIAGNOSTICS

The following diagnostics may be issued to the **ion.log** log file:

- udplso can't initialize LTP.

    **ltpadmin** has not yet initialized LTP protocol operations.

- No such engine in database.

    _remote\_engine\_nbr_ is invalid, or the applicable span has not yet
    been added to the LTP database by **ltpadmin**.

- LSO task is already started for this engine.

    Redundant initiation of **udplso**.

- LSO can't open UDP socket

    Operating system error.  Check errtext, correct problem, and restart **udplso**.

- LSO can't connect UDP socket

    Operating system error.  Check errtext, correct problem, and restart **udplso**.

- Segment is too big for UDP LSO.

    Configuration error: segments that are too large for UDP transmission (i.e.,
    larger than 65535 bytes) are being enqueued for **udplso**.  Use **ltpadmin**
    to change maximum segment size for this span.

# BUGS

Report bugs to <ion-dtn-support@lists.sourceforge.net>

# SEE ALSO

ltpadmin(1), ltpmeter(1), udplsi(1), owltsim(1)
