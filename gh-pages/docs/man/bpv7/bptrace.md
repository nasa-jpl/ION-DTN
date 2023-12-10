# NAME

bptrace - Bundle Protocol (BP) network trace utility

# SYNOPSIS

**bptrace** _own\_endpoint\_ID_ _destination\_endpoint\_ID_ _report-to\_endpoint\_ID_ _TTL_ _class\_of\_service_ "_trace\_text_" \[_status\_report\_flags_\]

# DESCRIPTION

**bptrace** uses bp\_send() to issue a single bundle to a designated
destination endpoint, with status reporting options enabled as selected
by the user, then terminates.  The status reports returned as the bundle
makes its way through the network provide a view of the operation of the
network as currently configured.

_TTL_ indicates the number of seconds the trace bundle may remain in the
network, undelivered, before it is automatically destroyed.

_class\_of\_service_ is _custody-requested_._priority_\[._ordinal_\[._unreliable_._critical_\[._data-label_\]\]\],
where _custody-requested_ must be 0 or 1 (Boolean), _priority_ must be 0
(bulk) or 1 (standard) or 2 (expedited), _ordinal_ must be 0-254,
_unreliable_ must be 0 or 1 (Boolean), _critical_ must also be 0 or 1
(Boolean), and _data-label_ may be any unsigned integer.  _custody-requested_
is passed in with the bundle transmission request, but if set to 1 it serves
only to request the use of reliable convergence-layer protocols; this will
have the effect of enabling custody transfer whenever the applicable
convergence-layer protocol is bundle-in-bundle encapsulation (BIBE).
_ordinal_ is ignored if _priority_ is not 2.  Setting _class\_of\_service_ to
"0.2.254" or "1.2.254" gives a bundle the highest possible priority.  Setting
_unreliable_ to 1 causes BP to forego convergence-layer retransmission in
the event of data loss.  Setting _critical_ to 1 causes contact graph routing
to forward the bundle on all plausible routes rather than just the "best" route
it computes; this may result in multiple copies of the bundle arriving at the
destination endpoint, but when used in conjunction with priority 2.254 it
ensures that the bundle will be delivered as soon as physically possible.

_trace\_text_ can be any string of ASCII text; alternatively, if we want to send
a file, it can be "@" followed by the name of the file.

_status\_report\_flags_ must be a sequence of status report flags, separated
by commas, with no embedded whitespace.  Each status report flag must be one
of the following: rcv, fwd, dlv, del.

# EXIT STATUS

- "0"

    **bptrace** has terminated.

# FILES

No configuration files are needed.

# ENVIRONMENT

No environment variables apply.

# DIAGNOSTICS

The following diagnostics may be issued to the **ion.log** log file:

- bptrace can't attach to BP.

    **bpadmin** has not yet initialized BP operations.

- bptrace can't open own endpoint.

    Another BP application task currently has _own\_endpoint\_ID_ open for
    bundle origination and reception.  Try again after that task has terminated.
    If no such task exists, it may have crashed while still holding the endpoint
    open; the easiest workaround is to select a different source endpoint.

- No space for bptrace text.

    Probably an unrecoverable database error, in which case the local ION
    node must be terminated and re-initialized.

- bptrace can't create ZCO.

    Probably an unrecoverable database error, in which case the local ION
    node must be terminated and re-initialized.

- bptrace can't send message.

    BP system error.  Check for earlier diagnostic messages describing the
    cause of the error; correct problem and rerun.

# BUGS

Report bugs to <ion-dtn-support@lists.sourceforge.net>

# SEE ALSO

bp(3)
