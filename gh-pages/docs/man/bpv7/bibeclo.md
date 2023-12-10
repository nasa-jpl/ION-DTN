# NAME

bibeclo - BP convergence layer output task using bundle-in-bundle encapsulation

# SYNOPSIS

**bibeclo** _peer\_EID_ _destination\_EID_

# DESCRIPTION

**bibeclo** is a background "daemon" task that extracts bundles from the
queues of bundles destined for _destination\_EID_ that are ready for
transmission via bundle-in-bundle encapsulation (BIBE) to _peer\_EID_,
encapsulates them in BP administrative records of (non-standard) record type
7 (BP\_BIBE\_PDU), and sends those administrative records in encapsulating
bundles destined for _peer\_EID_.  The forwarding of encapsulated bundles
for which custodial acknowledgment is requested causes **bibeclo** to post
custodial re-forwarding timers to the node's timeline.  Parameters governing
the forwarding of BIBE PDUs to _peer\_EID_ are stipulated in the corresponding
BIBE convergence-layer adapter (**bcla**) structure residing in the BIBE
database, as managed by **bibeadmin**.

The receiving node is expected to process received BIBE PDUs by simply
dispatching the encapsulated bundles - whose destination is the node
identified by _destination\_EID_ - as if they had been received from
neighboring nodes in the normal course of operations; BIBE PDUs for which
custodial acknowledgment was requested cause the received bundles to be
noted in custody signals that are being aggregated by the receiving node.

**bibeclo** additionally sends aggregated custody signals in BP administrative
records of (non-standard) record type 8 (BP\_BIBE\_SIGNAL) as the deadlines
for custody signal transmission arrive.

Note that the reception and processing of both encapsulated bundles and
custody signals is performed by the scheme-specific administration endpoint
daemon(s) at the receiving nodes.  Reception of a custody signal terminates
the custodial re-forwarding timers for all bundles acknowledged in that signal;
the re-forwarding of bundles upon custodial re-forwarding timer expiration is
initiated by the **bpclock** daemon.

**bibeclo** is spawned automatically by **bpadmin** in response to the 's'
(START) command that starts operation of the Bundle Protocol, and it is
terminated by **bpadmin** in response to an 'x' (STOP) command.  **bibeclo**
can also be spawned and terminated in response to START and STOP commands
that pertain specifically to the BIBE convergence layer protocol.

# EXIT STATUS

- "0"

    **bibeclo** terminated normally, for reasons noted in the **ion.log** file.  If
    this termination was not commanded, investigate and solve the problem identified
    in the log file and use **bpadmin** to restart **bibeclo**.

- "1"

    **bibeclo** terminated abnormally, for reasons noted in the **ion.log** file.
    Investigate and solve the problem identified in the log file, then use
    **bpadmin** to restart **bibeclo**.

# FILES

No configuration files are needed.

# ENVIRONMENT

No environment variables apply.

# DIAGNOSTICS

The following diagnostics may be issued to the **ion.log** log file:

- bibeclo can't attach to BP.

    **bpadmin** has not yet initialized Bundle Protocol operations.

- No such bibe outduct.

    No BIBE outduct with duct name _destination\_EID_ exists.  Use **bpadmin**
    to stop the BIBE convergence-layer protocol, add the outduct, and then
    restart the BIBE protocol.

- No such bcla.

    No bcla structure for the node identified by _peer\_EID_ has been added to
    the BP database.  Use **bpadmin** to stop the BIBE convergence-layer protocol,
    use **bibeadmin** to add the bcla, and then use **bpadmin** to restart the
    BIBE protocol.

- CLO task is already started for this duct.

    Redundant initiation of **bibeclo**.

- Can't dequeue bundle.

    BIBE outduct closed deleted or other system error.  Check ION log; correct
    the problem and restart BIBE.

- \[i\] bibeclo outduct closed.

    Nominal shutdown message.

- Can't prepend header; CLO stopping.

    System error.  Check ION log; correct the problem and restart BIBE.

- Can't destroy old ZCO; CLO stopping.

    System error.  Check ION log; correct the problem and restart BIBE.

- Can't get outbound space for BPDU.

    System error.  Check ION log; correct the problem and restart BIBE.

- Can't send encapsulated bundle.

    System error.  Check ION log; correct the problem and restart BIBE.

- Can't track bundle.

    System error.  Check ION log; correct the problem and restart BIBE.

- \[!\] Encapsulated bundle not sent.

    Malformed bundle issuance request, which might be a software error.  Contact
    technical support.

- Can't release ZCO; CLO stopping.

    System error.  Check ION log; correct the problem and restart BIBE.

- \[i\] bibeclo duct has ended.

    Nominal shutdown message.

# BUGS

Report bugs to <ion-dtn-support@lists.sourceforge.net>

# SEE ALSO

biberc(5), bibeadmin(1)
