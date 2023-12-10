# NAME

tcputa - TCP/IP-based CFDP UT-layer adapter

# SYNOPSIS

**tcputa**

# DESCRIPTION

**tcputa** is a background "daemon" task that sends and receives CFDP PDUs
via a TCP/IP socket.

The task is spawned automatically by **cfdpadmin** in response to the 's'
command that starts operation of the CFDP protocol; the text of the
command that is used to spawn the task must be provided
as a parameter to the 's' command.  The UT-layer daemon is
terminated by **cfdpadmin** in response to an 'x' (STOP) command.

# EXIT STATUS

- "0"

    **tcputa** terminated normally, for reasons noted in the **ion.log** file.  If
    this termination was not commanded, investigate and solve the problem identified
    in the log file and use **cfdpadmin** to restart **tcputa**.

- "1"

    **tcputa** terminated abnormally, for reasons noted in the **ion.log** file.
    Investigate and solve the problem identified in the log file, then use
    **cfdpadmin** to restart **tcputa**.

# FILES

No configuration files are needed.

# ENVIRONMENT

No environment variables apply.

# DIAGNOSTICS

The following diagnostics may be issued to the **ion.log** log file:

- tcputa can't attach to CFDP.

    **cfdpadmin** has not yet initialized CFDP protocol operations.

- tcputa can't dequeue outbound CFDP PDU; terminating.

    Possible system error.  Check ion.log for additional diagnostic messages.

- tcputa can't track PDU; terminating.

    Possible system error.  Check ion.log for additional diagnostic messages.

- tcputa can't handle inbound PDU.

    Possible system error; reception thread terminates.  Check ion.log for
    additional diagnostic messages.

# BUGS

Report bugs to <ion-dtn-support@lists.sourceforge.net>

# SEE ALSO

cfdpadmin(1), bpadmin(1)
