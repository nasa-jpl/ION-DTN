# NAME

bpclm - DTN bundle protocol convergence layer management daemon

# SYNOPSIS

**bpclm** _neighboring\_node\_ID_

# DESCRIPTION

**bpclm** is a background "daemon" task that manages the transmission of
bundles to a single designated neighboring node (as constrained by an
"egress plan" data structure for that node) by one or more convergence-layer
(CL) adapter output daemons (via buffer structures called "outducts").

**bpclm** is spawned automatically by **bpadmin** in response to the 's' (START)
command that starts operation of the Bundle Protocol, and it is terminated by
**bpadmin** in response to an 'x' (STOP) command.  **bpclm** can also be
spawned and terminated in response to commands that START and STOP the
corresponding node's egress plan.

# EXIT STATUS

- "0"

    **bpclm** terminated normally, for reasons noted in the **ion.log** file.  If
    this termination was not commanded, investigate and solve the problem identified
    in the log file and use **bpadmin** to restart the egress plan for this node.

- "1"

    **bpclm** terminated abnormally, for reasons noted in the **ion.log** file.
    Investigate and solve the problem identified in the log file, then use
    **bpadmin** to restart the egress plan for this node.

# FILES

No configuration files are needed.

# ENVIRONMENT

No environment variables apply.

# DIAGNOSTICS

The following diagnostics may be issued to the **ion.log** log file:

- bpclm can't attach to BP.

    **bpadmin** has not yet initialized Bundle Protocol operations.

- No egress plan for this node

    No egress plan for the node identified by _neighboring\_node\_ID_ has been
    added to the BP database.  Use **bpadmin** to add and start the plan.

- bpclm task is already started for this node

    Redundant initiation of **bpclm**.

# BUGS

Report bugs to <ion-dtn-support@lists.sourceforge.net>

# SEE ALSO

bpadmin(1), bprc(5)
