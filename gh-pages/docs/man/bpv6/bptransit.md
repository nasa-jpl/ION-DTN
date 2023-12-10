# NAME

bptransit - Bundle Protocol (BP) daemon task for forwarding received bundles

# SYNOPSIS

**bptransit**

# DESCRIPTION

**bptransit** is a background "daemon" task that is responsible for presenting
to ION's forwarding daemons any bundles that were received from other nodes
(i.e., bundles whose payloads reside in Inbound ZCO space) and are destined
for yet other nodes.  In doing so, it migrates these bundles from Inbound
buffer space to Outbound buffer space on the same prioritized basis as the
insertion of locally sourced outbound bundles.

Management of the bptransit daemon is automatic.  It is spawned automatically
by **bpadmin** in response to the 's' command that starts operation of Bundle
Protocol on the local ION node, and it is terminated by **bpadmin** in
response to an 'x' (STOP) command.

Whenever a received bundle is determined to have a destination other than the
local node, a pointer to that bundle is appended to one of two queues of 
"in-transit" bundles, one for bundles whose forwarding is provisional
(depending on the availability of Outbound ZCO buffer space; bundles in
this queue are potentially subject to congestion loss) and one for bundles
whose forwarding is confirmed.  Bundles received via convergence-layer adapters
that can sustain flow control, such as STCP, are appended to the "confirmed"
queue, while those from CLAs that cannot sustain flow control (such as LTP)
are appended to the "provisional" queue.

**bptransit** comprises two threads, one for each in-transit queue.  The
confirmed in-transit thread dequeues bundles from the "confirmed" queue
and moves them from Inbound to Outbound ZCO buffer space, blocking (if
necessary) until space becomes available.  The provisional in-transit queue
dequeues bundles from the "provisional" queue and moves them from Inbound
to Outbound ZCO buffer space if Outbound space is available, discarding
("abandoning") them if it is not.

# EXIT STATUS

- "0"

    **bptransit** terminated, for reasons noted in the **ion.log** file.  If this
    termination was not commanded, investigate and solve the problem identified
    in the log file and use **bpadmin** to restart **bptransit**.

- "1"

    **bptransit** was unable to attach to Bundle Protocol operations, probably
    because **bpadmin** has not yet been run.

# FILES

No configuration files are needed.

# ENVIRONMENT

No environment variables apply.

# DIAGNOSTICS

The following diagnostics may be issued to the **ion.log** log file:

- bptransit can't attach to BP.

    **bpadmin** has not yet initialized BP operations.

# BUGS

Report bugs to <ion-dtn-support@lists.sourceforge.net>

# SEE ALSO

bpadmin(1)
