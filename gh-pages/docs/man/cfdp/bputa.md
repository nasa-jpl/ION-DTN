# NAME

bputa - BP-based CFDP UT-layer adapter

# SYNOPSIS

**bputa**

# DESCRIPTION

**bputa** is a background "daemon" task that sends and receives CFDP PDUs
encapsulated in DTN bundles.

The task is spawned automatically by **cfdpadmin** in response to the 's'
command that starts operation of the CFDP protocol; the text of the
command that is used to spawn the task must be provided
as a parameter to the 's' command.  The UT-layer daemon is
terminated by **cfdpadmin** in response to an 'x' (STOP) command.

# EXIT STATUS

- "0"

    **bputa** terminated normally, for reasons noted in the **ion.log** file.  If
    this termination was not commanded, investigate and solve the problem identified
    in the log file and use **cfdpadmin** to restart **bputa**.

- "1"

    **bputa** terminated abnormally, for reasons noted in the **ion.log** file.
    Investigate and solve the problem identified in the log file, then use
    **cfdpadmin** to restart **bputa**.

# FILES

No configuration files are needed.

# ENVIRONMENT

No environment variables apply.

# DIAGNOSTICS

The following diagnostics may be issued to the **ion.log** log file:

- CFDP can't attach to BP.

    **bpadmin** has not yet initialized BP protocol operations.

- CFDP can't open own endpoint.

    Most likely another bputa task is already running.  Use **cfdpadmin** to
    stop CFDP and restart.

- CFDP can't get Bundle Protocol SAP.

    Most likely a BP configuration problem.  Use **bpadmin** to stop BP and restart.

- bputa can't attach to CFDP.

    **cfdpadmin** has not yet initialized CFDP protocol operations.

- bputa can't dequeue outbound CFDP PDU; terminating.

    Possible system error.  Check ion.log for additional diagnostic messages.

- bputa can't send PDU in bundle; terminating.

    Possible system error.  Check ion.log for additional diagnostic messages.

- bputa can't track PDU; terminating.

    Possible system error.  Check ion.log for additional diagnostic messages.

- bputa bundle reception failed.

    Possible system error; reception thread terminates.  Check ion.log for
    additional diagnostic messages.

- bputa can't receive bundle ADU.

    Possible system error; reception thread terminates.  Check ion.log for
    additional diagnostic messages.

- bputa can't handle bundle delivery.

    Possible system error; reception thread terminates.  Check ion.log for
    additional diagnostic messages.

- bputa can't handle inbound PDU.

    Possible system error; reception thread terminates.  Check ion.log for
    additional diagnostic messages.

# BUGS

Report bugs to <ion-dtn-support@lists.sourceforge.net>

# SEE ALSO

cfdpadmin(1), bpadmin(1)
