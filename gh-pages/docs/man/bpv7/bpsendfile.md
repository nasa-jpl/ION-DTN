# NAME

bpsendfile - Bundle Protocol (BP) file transmission utility

# SYNOPSIS

**bpsendfile** _own\_endpoint\_ID_ _destination\_endpoint\_ID_ _file\_name_ \[_class\_of\_service_ \[_time\_to\_live (seconds)_ \]\]

# DESCRIPTION

**bpsendfile** uses bp\_send() to issue a single bundle to a designated
destination endpoint, containing the contents of the file identified by
_file\_name_, then terminates.  The bundle is sent with no custody transfer
requested.  When _class\_of\_service_ is omitted, the bundle is sent at
standard priority; for details of the _class\_of\_service_ parameter,
see bptrace(1).  _time\_to\_live_, if not specified, defaults to 300
seconds (5 minutes).  **NOTE** that _time\_to\_live_ is specified **AFTER**
_class\_of\_service_, rather than before it as in bptrace(1).

# EXIT STATUS

- "0"

    **bpsendfile** has terminated.

# FILES

No configuration files are needed.

# ENVIRONMENT

No environment variables apply.

# DIAGNOSTICS

The following diagnostics may be issued to the **ion.log** log file:

- Can't attach to BP.

    **bpadmin** has not yet initialized BP operations.

- Can't open own endpoint.

    Another BP application task currently has _own\_endpoint\_ID_ open for
    bundle origination and reception.  Try again after that task has terminated.
    If no such task exists, it may have crashed while still holding the endpoint
    open; the easiest workaround is to select a different source endpoint.

- Can't stat the file

    Operating system error.  Check errtext, correct problem, and rerun.

- bpsendfile can't create file ref.

    Probably an unrecoverable database error, in which case the local ION
    node must be terminated and re-initialized.

- bpsendfile can't create ZCO.

    Probably an unrecoverable database error, in which case the local ION
    node must be terminated and re-initialized.

- bpsendfile can't send file in bundle.

    BP system error.  Check for earlier diagnostic messages describing the
    cause of the error; correct problem and rerun.

# BUGS

Report bugs to <ion-dtn-support@lists.sourceforge.net>

# SEE ALSO

bprecvfile(1), bp(3)
