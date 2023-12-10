# NAME

ltpclo - LTP-based BP convergence layer adapter output task

# SYNOPSIS

**ltpclo** _remote\_node\_nbr_

# DESCRIPTION

**ltpclo** is a background "daemon" task that extracts bundles from the
queues of segments ready for transmission via LTP to the remote bundle
protocol agent identified by _remote\_node\_nbr_ and passes them to the
local LTP engine for aggregation, segmentation, and transmission to the
remote node.

**ltpclo** is spawned automatically by **bpadmin** in response to the 's' (START)
command that starts operation of the Bundle Protocol, and it is terminated by
**bpadmin** in response to an 'x' (STOP) command.  **ltpclo** can also be
spawned and terminated in response to START and STOP commands that pertain
specifically to the LTP convergence layer protocol.

# EXIT STATUS

- "0"

    **ltpclo** terminated normally, for reasons noted in the **ion.log** file.  If
    this termination was not commanded, investigate and solve the problem identified
    in the log file and use **bpadmin** to restart the BRSC protocol.

- "1"

    **ltpclo** terminated abnormally, for reasons noted in the **ion.log** file.
    Investigate and solve the problem identified in the log file, then use
    **bpadmin** to restart the BRSC protocol.

# FILES

No configuration files are needed.

# ENVIRONMENT

No environment variables apply.

# DIAGNOSTICS

The following diagnostics may be issued to the **ion.log** log file:

- ltpclo can't attach to BP.

    **bpadmin** has not yet initialized Bundle Protocol operations.

- No such ltp duct.

    No LTP outduct with duct name matching _remote\_node\_nbr_ has been added
    to the BP database.  Use **bpadmin** to stop the LTP convergence-layer
    protocol, add the outduct, and then restart the LTP protocol.

- CLO task is already started for this duct.

    Redundant initiation of **ltpclo**.

- ltpclo can't initialize LTP.

    **ltpadmin** has not yet initialized LTP operations.

# BUGS

Report bugs to <ion-dtn-support@lists.sourceforge.net>

# SEE ALSO

bpadmin(1), bprc(5), ltpadmin(1), ltprc(5), ltpcli(1)
