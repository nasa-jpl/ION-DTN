# NAME

imcfw - bundle route computation task for the IMC scheme

# SYNOPSIS

**imcfw**

# DESCRIPTION

**imcfw** is a background "daemon" task that pops bundles from the queue of
bundle destined for IMC-scheme (Interplanetary Multicast) endpoints, determines
which "relatives" on the IMC multicast tree to forward the bundles to,
and appends those bundles to the appropriate queues of bundles pending
transmission to those proximate destinations.

For each possible proximate destination (that is, neighboring node) there is
a separate queue for each possible level of bundle priority: 0, 1, 2.  Each
outbound bundle is appended to the queue matching the bundle's designated
priority.

Proximate destination computation is determined by multicast group membership
as resulting from nodes' registration in multicast endpoints, governed by
multicast tree structure as configured by imcadmin(1).

**imcfw** is spawned automatically by **bpadmin** in response to the
's' (START) command that starts operation of Bundle Protocol on the local
ION node, and it is terminated by **bpadmin** in response to an 'x' (STOP)
command.  **imcfw** can also be spawned and terminated in response to
START and STOP commands that pertain specifically to the IMC scheme.

# EXIT STATUS

- "0"

    **imcfw** terminated, for reasons noted in the **ion.log** log file.  If this
    termination was not commanded, investigate and solve the problem identified
    in the log file and use **bpadmin** to restart **imcfw**.

- "1"

    **imcfw** could not commence operations, for reasons noted in the **ion.log**
    log file.  Investigate and solve the problem identified in the log file, then
    use **bpadmin** to restart **imcfw**.

# FILES

No configuration files are needed.

# ENVIRONMENT

No environment variables apply.

# DIAGNOSTICS

The following diagnostics may be issued to the **ion.log** log file:

- imcfw can't attach to BP.

    **bpadmin** has not yet initialized BP operations.

- imcfw can't load routing database.

    **ipnadmin** has not yet initialized the IPN scheme.

- Can't create lists for route computation.

    An unrecoverable database error was encountered.  **imcfw** terminates.

- 'imc' scheme is unknown.

    The IMC scheme was not added when **bpadmin** initialized BP operations.  Use
    **bpadmin** to add and start the scheme.

- Can't take forwarder semaphore.

    ION system error.  **imcfw** terminates.

- Can't exclude sender from routes.

    An unrecoverable database error was encountered.  **imcfw** terminates.

- Can't enqueue bundle.

    An unrecoverable database error was encountered.  **imcfw** terminates.

# BUGS

Report bugs to <ion-dtn-support@lists.sourceforge.net>

# SEE ALSO

bpadmin(1), imcadmin(1), bprc(5), imcrc(5)
