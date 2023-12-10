# NAME

bibeclo - BP convergence layer output task using bundle-in-bundle encapsulation

# SYNOPSIS

**bibeclo** _peer\_node\_eid_ _destination\_node\_eid_

# DESCRIPTION

**bibeclo** is a background "daemon" task that extracts bundles from the
queues of bundles ready for transmission to _destination\_node\_eid_ via
bundle-in-bundle encapsulation (BIBE), encapsulates them in BP administrative
records of (non-standard) record type 7 (BP\_ENCAPSULATED\_BUNDLE), and sends
those administrative records to the DTN node identified by _peer\_node\_eid_.
The receiving node is expected to process these received administrative
records by simply dispatching the encapsulated bundles as if they had been
received from neighboring nodes in the normal course of operations.

**bibeclo** is spawned automatically by **bpadmin** in response to the 's' (START)
command that starts operation of the Bundle Protocol, and it is terminated by
**bpadmin** in response to an 'x' (STOP) command.  **bibeclo** can also be
spawned and terminated in response to START and STOP commands that pertain
specifically to the BIBE convergence layer protocol.

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

- No such bibe duct.

    No BIBE outduct with duct name _destination\_node\_eid_ has been added to the BP
    database.  Use **bpadmin** to stop the BIBE convergence-layer protocol, add
    the outduct, and then restart the BIBE protocol.

- No such bcla.

    No BIBE convergence layer adapter named _peer\_node\_eid_ has been added to
    the BIBE database.  Use **bibeadmin** to add the BCLA.

- CLO task is already started for this duct.

    Redundant initiation of **bibeclo**.

- Can't prepend header; CLO stopping.

    This is a system error.  Check ION log, correct problem, and restart BIBE.

- Can't send encapsulated bundle; CLO stopping.

    This is a system error.  Check ION log, correct problem, and restart BIBE.

- \[!\] Encapsulated bundle not sent.

    Malformed bundle issuance request, which might be a software error.  Contact
    technical support.

# BUGS

Report bugs to <ion-dtn-support@lists.sourceforge.net>

# SEE ALSO

bibeadmin(1), bp(3), biberc(5)
