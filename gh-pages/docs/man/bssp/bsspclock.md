# NAME

bsspclock - BSSP daemon task for managing scheduled events

# SYNOPSIS

**bsspclock**

# DESCRIPTION

**bsspclock** is a background "daemon" task that periodically performs
scheduled BSSP activities.  It is spawned automatically by **bsspadmin** in
response to the 's' command that starts operation of the BSSP protocol, and
it is terminated by **bsspadmin** in response to an 'x' (STOP) command.

Once per second, **bsspclock** takes the following action:

> First it manages the current state of all links ("spans").  Specifically,
> it infers link state changes ("link cues") from data rate changes as noted
> in the RFX database by **rfxclock**:
>
> > If the rate of transmission to a neighbor was zero but is now non-zero, then
> > transmission to that neighbor resumes.  The applicable "buffer empty"
> > semaphore is given (enabling start of a new transmission session) and
> > the best-efforts and/or reliable "PDUs ready" semaphores are given if the
> > corresponding outbound PDU queues are non-empty (enabling transmission of
> > PDUs by the link service output task).
> >
> > If the rate of transmission to a neighbor was non-zero but is now zero, then
> > transmission to that neighbor is suspended -- i.e., the semaphores triggering
> > transmission will no longer be given.
> >
> > If the imputed rate of transmission from a neighbor was non-zero but is now
> > zero, then all best-efforts transmission acknowledgment timers affecting
> > transmission to that neighbor are suspended.  This has the effect of extending
> > the interval of each affected timer by the length of time that the timers
> > remain suspended.
> >
> > If the imputed rate of transmission from a neighbor was zero but is now
> > non-zero, then all best-efforts transmission acknowledgment timers affecting
> > transmission to that neighbor are resumed.
>
> Then **bsspclock** enqueues for reliable transmission all blocks for which
> the best-efforts transmission acknowledgment timeout interval has now expired
> but no acknowledgment has yet been received.

# EXIT STATUS

- "0"

    **bsspclock** terminated, for reasons noted in the **ion.log** file.  If this
    termination was not commanded, investigate and solve the problem identified
    in the log file and use **bsspadmin** to restart **bsspclock**.

- "1"

    **bsspclock** was unable to attach to BSSP protocol operations, probably because
    **bsspadmin** has not yet been run.

# FILES

No configuration files are needed.

# ENVIRONMENT

No environment variables apply.

# DIAGNOSTICS

The following diagnostics may be issued to the **ion.log** log file:

- bsspclock can't initialize BSSP.

    **bsspadmin** has not yet initialized BSSP protocol operations.

- Can't dispatch events.

    An unrecoverable database error was encountered.  **bsspclock** terminates.

- Can't manage links.

    An unrecoverable database error was encountered.  **bsspclock** terminates.

# BUGS

Report bugs to <ion-dtn-support@lists.sourceforge.net>

# SEE ALSO

bsspadmin(1), rfxclock(1)
