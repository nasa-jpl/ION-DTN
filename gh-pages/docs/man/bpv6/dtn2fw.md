# NAME

dtn2fw - bundle route computation task for the "dtn" scheme

# SYNOPSIS

**dtn2fw**

# DESCRIPTION

**dtn2fw** is a background "daemon" task that pops bundles from the queue of
bundle destined for "dtn"-scheme endpoints, computes proximate destinations
for those bundles, and appends those bundles to the appropriate queues of
bundles pending transmission to those computed proximate destinations.

For each possible proximate destination (that is, neighboring node) there is
a separate queue for each possible level of bundle priority: 0, 1, 2.  Each
outbound bundle is appended to the queue matching the bundle's designated
priority.

Proximate destination computation is affected by static routes as configured
by dtn2admin(1).

**dtn2fw** is spawned automatically by **bpadmin** in response to the
's' (START) command that starts operation of Bundle Protocol on the local
ION node, and it is terminated by **bpadmin** in response to an 'x' (STOP)
command.  **dtn2fw** can also be spawned and terminated in response to
START and STOP commands that pertain specifically to the "dtn" scheme.

# EXIT STATUS

- "0"

    **dtn2fw** terminated, for reasons noted in the **ion.log** log file.  If this
    termination was not commanded, investigate and solve the problem identified
    in the log file and use **bpadmin** to restart **dtn2fw**.

- "1"

    **dtn2fw** could not commence operations, for reasons noted in the **ion.log**
    log file.  Investigate and solve the problem identified in the log file, then
    use **bpadmin** to restart **dtn2fw**.

# FILES

No configuration files are needed.

# ENVIRONMENT

No environment variables apply.

# DIAGNOSTICS

The following diagnostics may be issued to the **ion.log** log file:

- dtn2fw can't attach to BP.

    **bpadmin** has not yet initialized BP operations.

- dtn2fw can't load routing database.

    **dtn2admin** has not yet initialized the "dtn" scheme.

- Can't create lists for route computation.

    An unrecoverable database error was encountered.  **dtn2fw** terminates.

- 'dtn' scheme is unknown.

    The "dtn" scheme was not added when **bpadmin** initialized BP operations.  Use
    **bpadmin** to add and start the scheme.

- Can't take forwarder semaphore.

    ION system error.  **dtn2fw** terminates.

- Can't enqueue bundle.

    An unrecoverable database error was encountered.  **dtn2fw** terminates.

# BUGS

Report bugs to <ion-dtn-support@lists.sourceforge.net>

# SEE ALSO

bpadmin(1), dtn2admin(1), bprc(5), dtn2rc(5).
