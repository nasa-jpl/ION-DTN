# NAME

ipnfw - bundle route computation task for the IPN scheme

# SYNOPSIS

**ipnfw**

# DESCRIPTION

**ipnfw** is a background "daemon" task that pops bundles from the queue of
bundle destined for IPN-scheme endpoints, computes proximate destinations
for those bundles, and appends those bundles to the appropriate queues of
bundles pending transmission to those computed proximate destinations.

For each possible proximate destination (that is, neighboring node) there is
a separate queue for each possible level of bundle priority: 0, 1, 2.  Each
outbound bundle is appended to the queue matching the bundle's designated
priority.

Proximate destination computation is affected by static and default routes
as configured by ipnadmin(1) and by contact graphs as managed by ionadmin(1)
and rfxclock(1).

**ipnfw** is spawned automatically by **bpadmin** in response to the
's' (START) command that starts operation of Bundle Protocol on the local
ION node, and it is terminated by **bpadmin** in response to an 'x' (STOP)
command.  **ipnfw** can also be spawned and terminated in response to
START and STOP commands that pertain specifically to the IPN scheme.

# EXIT STATUS

- "0"

    **ipnfw** terminated, for reasons noted in the **ion.log** log file.  If this
    termination was not commanded, investigate and solve the problem identified
    in the log file and use **bpadmin** to restart **ipnfw**.

- "1"

    **ipnfw** could not commence operations, for reasons noted in the **ion.log**
    log file.  Investigate and solve the problem identified in the log file, then
    use **bpadmin** to restart **ipnfw**.

# FILES

No configuration files are needed.

# ENVIRONMENT

No environment variables apply.

# DIAGNOSTICS

The following diagnostics may be issued to the **ion.log** log file:

- ipnfw can't attach to BP.

    **bpadmin** has not yet initialized BP operations.

- ipnfw can't load routing database.

    **ipnadmin** has not yet initialized the IPN scheme.

- Can't create lists for route computation.

    An unrecoverable database error was encountered.  **ipnfw** terminates.

- 'ipn' scheme is unknown.

    The IPN scheme was not added when **bpadmin** initialized BP operations.  Use
    **bpadmin** to add and start the scheme.

- Can't take forwarder semaphore.

    ION system error.  **ipnfw** terminates.

- Can't exclude sender from routes.

    An unrecoverable database error was encountered.  **ipnfw** terminates.

- Can't enqueue bundle.

    An unrecoverable database error was encountered.  **ipnfw** terminates.

# BUGS

Report bugs to <ion-dtn-support@lists.sourceforge.net>

# SEE ALSO

bpadmin(1), ipnadmin(1), bprc(5), ipnrc(5)
