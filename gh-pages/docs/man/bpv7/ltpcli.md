# NAME

ltpcli - LTP-based BP convergence layer input task

# SYNOPSIS

**ltpcli** _local\_node\_nbr_

# DESCRIPTION

**ltpcli** is a background "daemon" task that receives LTP data transmission
blocks, extracts bundles from the received blocks, and passes them to the
bundle protocol agent on the local ION node.

**ltpcli** is spawned automatically by **bpadmin** in response to the 's'
(START) command that starts operation of the Bundle Protocol; the text
of the command that is used to spawn the task must be provided at the
time the "ltp" convergence layer protocol is added to the BP database.
The convergence layer input task is terminated by **bpadmin** in
response to an 'x' (STOP) command.  **ltpcli** can also be spawned and
terminated in response to START and STOP commands that pertain specifically
to the LTP convergence layer protocol.

# EXIT STATUS

- "0"

    **ltpcli** terminated normally, for reasons noted in the **ion.log** file.  If
    this termination was not commanded, investigate and solve the problem identified
    in the log file and use **bpadmin** to restart **ltpcli**.

- "1"

    **ltpcli** terminated abnormally, for reasons noted in the **ion.log** file.
    Investigate and solve the problem identified in the log file, then use
    **bpadmin** to restart **ltpcli**.

# FILES

No configuration files are needed.

# ENVIRONMENT

No environment variables apply.

# DIAGNOSTICS

The following diagnostics may be issued to the **ion.log** log file:

- ltpcli can't attach to BP.

    **bpadmin** has not yet initialized Bundle Protocol operations.

- No such ltp duct.

    No LTP induct matching _local\_node\_nbr_ has been added to the BP
    database.  Use **bpadmin** to stop the LTP convergence-layer protocol,
    add the induct, and then restart the LTP protocol.

- CLI task is already started for this duct.

    Redundant initiation of **ltpcli**.

- ltpcli can't initialize LTP.

    **ltpadmin** has not yet initialized LTP operations.

- ltpcli can't open client access.

    Another task has already opened the client service for BP over LTP.

- ltpcli can't create receiver thread

    Operating system error.  Check errtext, correct problem, and restart LTP.

# BUGS

Report bugs to <ion-dtn-support@lists.sourceforge.net>

# SEE ALSO

bpadmin(1), bprc(5), ltpadmin(1), ltprc(5), ltpclo(1)
