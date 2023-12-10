# NAME

bpclock - Bundle Protocol (BP) daemon task for managing scheduled events

# SYNOPSIS

**bpclock**

# DESCRIPTION

**bpclock** is a background "daemon" task that periodically performs
scheduled Bundle Protocol activities.  It is spawned automatically by
**bpadmin** in response to the 's' command that starts operation of Bundle
Protocol on the local ION node, and it is terminated by **bpadmin** in
response to an 'x' (STOP) command.

Once per second, **bpclock** takes the following action:

> First it (a) destroys all bundles whose TTLs have expired, (b) enqueues
> for re-forwarding all bundles that were expected to have been transmitted
> (by convergence-layer output tasks) by now but are still stuck in their
> assigned transmission queues, and (c) enqueues for re-forwarding all
> bundles for which custody has not yet been taken that were expected to
> have been received and acknowledged by now (as noted by invocation of
> the bpMemo() function by some convergence-layer adapter that had CL-specific
> insight into the appropriate interval to wait for custody acceptance).
>
> Then **bpclock** adjusts the transmission and reception "throttles" that
> control rates of LTP transmission to and reception from neighboring nodes,
> in response to data rate changes as noted in the RFX database by **rfxclock**.
>
> **bpclock** then checks for bundle origination activity that has been blocked
> due to insufficient allocated space for BP traffic in the ION data store: if
> space for bundle origination is now available, **bpclock** gives the bundle
> production throttle semaphore to unblock that activity.
>
> Finally, **bpclock** applies rate control to all convergence-layer protocol
> inducts and outducts:
>
> > For each induct, **bpclock** increases the current capacity of the duct
> > by the applicable nominal data reception rate.  If the revised current
> > capacity is greater than zero, **bpclock** gives the throttle's semaphore
> > to unblock data acquisition (which correspondingly reduces the current
> > capacity of the duct) by the associated convergence layer input task.
> >
> > For each outduct, **bpclock** increases the current capacity of the duct
> > by the applicable nominal data transmission rate.  If the revised current
> > capacity is greater than zero, **bpclock** gives the throttle's semaphore
> > to unblock data transmission (which correspondingly reduces the current
> > capacity of the duct) by the associated convergence layer output task.

# EXIT STATUS

- "0"

    **bpclock** terminated, for reasons noted in the **ion.log** file.  If this
    termination was not commanded, investigate and solve the problem identified
    in the log file and use **bpadmin** to restart **bpclock**.

- "1"

    **bpclock** was unable to attach to Bundle Protocol operations, probably because
    **bpadmin** has not yet been run.

# FILES

No configuration files are needed.

# ENVIRONMENT

No environment variables apply.

# DIAGNOSTICS

The following diagnostics may be issued to the **ion.log** log file:

- bpclock can't attach to BP.

    **bpadmin** has not yet initialized BP operations.

- Can't dispatch events.

    An unrecoverable database error was encountered.  **bpclock** terminates.

- Can't adjust throttles.

    An unrecoverable database error was encountered.  **bpclock** terminates.

# BUGS

Report bugs to <ion-dtn-support@lists.sourceforge.net>

# SEE ALSO

bpadmin(1), rfxclock(1)
