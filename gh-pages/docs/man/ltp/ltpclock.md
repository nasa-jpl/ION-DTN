# NAME

ltpclock - LTP daemon task for managing scheduled events

# SYNOPSIS

**ltpclock**

# DESCRIPTION

**ltpclock** is a background "daemon" task that periodically performs
scheduled LTP activities.  It is spawned automatically by **ltpadmin** in
response to the 's' command that starts operation of the LTP protocol, and
it is terminated by **ltpadmin** in response to an 'x' (STOP) command.

Once per second, **ltpclock** takes the following action:

> First it manages the current state of all links ("spans").  In
> particular, it checks the age of the currently buffered session block
> for each span and, if that age exceeds the span's configured aggregation
> time limit, gives the "buffer full" semaphore for that span to initiate
> block segmentation and transmission by **ltpmeter**.
>
> In so doing, it also infers link state changes ("link cues") from data rate
> changes as noted in the RFX database by **rfxclock**:
>
> > If the rate of transmission to a neighbor was zero but is now non-zero, then
> > transmission to that neighbor is unblocked.  The applicable "buffer empty"
> > semaphore is given if no outbound block is being constructed (enabling
> > start of a new transmission session) and the "segments ready" semaphore is
> > given if the outbound segment queue is non-empty (enabling transmission of
> > segments by the link service output task).
> >
> > If the rate of transmission to a neighbor was non-zero but is now zero, then
> > transmission to that neighbor is blocked -- i.e., the semaphores triggering
> > transmission will no longer be given.
> >
> > If the imputed rate of transmission from a neighbor was non-zero but is now
> > zero, then all timers affecting segment retransmission to that neighbor are
> > suspended.  This has the effect of extending the interval of each affected
> > timer by the length of time that the timers remain suspended.
> >
> > If the imputed rate of transmission from a neighbor was zero but is now
> > non-zero, then all timers affecting segment retransmission to that neighbor
> > are resumed.
>
> Then **ltpclock** retransmits all unacknowledged checkpoint segments,
> report segments, and cancellation segments whose computed timeout
> intervals have expired.

# EXIT STATUS

- "0"

    **ltpclock** terminated, for reasons noted in the **ion.log** file.  If this
    termination was not commanded, investigate and solve the problem identified
    in the log file and use **ltpadmin** to restart **ltpclock**.

- "1"

    **ltpclock** was unable to attach to LTP protocol operations, probably because
    **ltpadmin** has not yet been run.

# FILES

No configuration files are needed.

# ENVIRONMENT

No environment variables apply.

# DIAGNOSTICS

The following diagnostics may be issued to the **ion.log** log file:

- ltpclock can't initialize LTP.

    **ltpadmin** has not yet initialized LTP protocol operations.

- Can't dispatch events.

    An unrecoverable database error was encountered.  **ltpclock** terminates.

- Can't manage links.

    An unrecoverable database error was encountered.  **ltpclock** terminates.

# BUGS

Report bugs to <ion-dtn-support@lists.sourceforge.net>

# SEE ALSO

ltpadmin(1), ltpmeter(1), rfxclock(1)
