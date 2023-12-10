# NAME

rfxclock - ION daemon task for managing scheduled events

# SYNOPSIS

**rfxclock**

# DESCRIPTION

**rfxclock** is a background "daemon" task that periodically applies
scheduled changes in node connectivity and range to the ION node's database.
It is spawned automatically by **ionadmin** in response to the 's' command
that starts operation of the ION node infrastructure, and it is terminated
by **ionadmin** in response to an 'x' (STOP) command.

Once per second, **rfxclock** takes the following action:

> For each neighboring node that has been refusing custody of bundles sent to
> it to be forwarded to some destination node, to which no such bundle has
> been sent for at least N seconds (where N is twice the one-way light time
> from the local node to this neighbor), **rfxclock** turns on a _probeIsDue_
> flag authorizing transmission of the next such bundle in hopes of learning
> that this neighbor is now able to accept custody.
>
> Then **rfxclock** purges the database of all range and contact information
> that is no longer applicable, based on the stop times of the records.
>
> Finally, **rfxclock** applies to the database all range and contact information
> that is currently applicable, i.e., those records whose start times are
> before the current time and whose stop times are in the future.

# EXIT STATUS

- "0"

    **rfxclock** terminated, for reasons noted in the **ion.log** file.  If this
    termination was not commanded, investigate and solve the problem identified
    in the log file and use **ionadmin** to restart **rfxclock**.

- "1"

    **rfxclock** was unable to attach to the local ION node, probably because
    **ionadmin** has not yet been run.

# FILES

No configuration files are needed.

# ENVIRONMENT

No environment variables apply.

# DIAGNOSTICS

The following diagnostics may be issued to the **ion.log** log file:

- rfxclock can't attach to ION.

    **ionadmin** has not yet initialized the ION database.

- Can't apply ranges.

    An unrecoverable database error was encountered.  **rfxclock** terminates.

- Can't apply contacts.

    An unrecoverable database error was encountered.  **rfxclock** terminates.

- Can't purge ranges.

    An unrecoverable database error was encountered.  **rfxclock** terminates.

- Can't purge contacts.

    An unrecoverable database error was encountered.  **rfxclock** terminates.

# BUGS

Report bugs to <ion-dtn-support@lists.sourceforge.net>

# SEE ALSO

ionadmin(1)
