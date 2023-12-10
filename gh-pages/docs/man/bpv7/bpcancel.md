# NAME

bpcancel - Bundle Protocol (BP) bundle cancellation utility

# SYNOPSIS

**bpcancel** _source\_EID_ _creation\_seconds_ \[_creation\_count_ \[_fragment\_offset_ \[_fragment\_length_\]\]\]

# DESCRIPTION

**bpcancel** attempts to locate the bundle identified by the command-line
parameter values and cancel transmission of this bundle.  Bundles for which
multiple copies have been queued for transmission can't be canceled, because
one or more of those copies might already have been transmitted.  Transmission
of a bundle that has never been cloned and that is still in local bundle
storage is cancelled by simulation of an immediate time-to-live expiration.

# EXIT STATUS

- "0"

    **bpcancel** has terminated.

# FILES

No configuration files are needed.

# ENVIRONMENT

No environment variables apply.

# DIAGNOSTICS

The following diagnostics may be issued to the **ion.log** log file:

- Can't attach to BP.

    **bpadmin** has not yet initialized BP operations.

- bpcancel failed finding bundle.

    The attempt to locate the subject bundle failed due to some serious system
    error.  It will probably be necessary to terminate and re-initialize the
    local ION node.

- bpcancel failed destroying bundle.

    Probably an unrecoverable database error, in which case the local ION
    node must be terminated and re-initialized.

- bpcancel failed.

    Probably an unrecoverable database error, in which case the local ION
    node must be terminated and re-initialized.

# BUGS

Report bugs to <ion-dtn-support@lists.sourceforge.net>

# SEE ALSO

bplist(1)
