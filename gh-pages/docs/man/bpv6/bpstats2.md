# NAME

bpstats2 - Bundle Protocol (BP) processing statistics query utility via bundles

# SYNOPSIS

**bpstats2** _sourceEID_ \[_default destEID_\] \[ct\]

# DESCRIPTION

**bpstats2** creates bundles containing the current values of all BP processing
statistics accumulators.  It creates these bundles when:

- an interrogation bundle is delivered to _sourceEID_: the contents of the
bundle are discarded, a new statistics bundle is generated and sent to the
source of the interrogation bundle.  The format of the interrogation bundle
is irrelevant.
- a SIGUSR1 signal is delivered to the **bpstats2** application: a new
statistics bundle is generated and sent to _default destEID_.

# EXIT STATUS

- "0"

    **bpstats2** has terminated. Any problems encountered during operation will be
    noted in the **ion.log** log file.

- "1"

    **bpstats2** failed to start up or receive bundles.  Diagnose the issue 
    reported in the **ion.log** file and try again.

# OPTIONS

- \[ct\]

    If the string "ct" is appended as the last argument, then statistics bundles
    will be sent with custody transfer requested.

# FILES

No configuration files are needed.

# ENVIRONMENT

No environment variables apply.

# DIAGNOSTICS

The following diagnostics may be issued to the **ion.log** log file:

- bpstats2 can't bp\_attach().

    **bpadmin** has not yet initialized BP operations.

- bpstats2 can't open own endpoint.

    Another BP application has opened that endpoint; close it and try again.

- No space for ZCO extent.

    ION system error.  Check for earlier diagnostic messages describing
    the cause of the error; correct problem and rerun.

- Can't create ZCO extent.

    ION system error.  Check for earlier diagnostic messages describing
    the cause of the error; correct problem and rerun.

- bpstats2 can't send stats bundle.

    Bundle Protocol service to the remote endpoint has been stopped.

- Can't send stats: bad dest EID (dest EID)

    The destination EID printed is an invalid destination EID.  The destination
    EID may be specified in _default destEID_ or the source EID of the
    interrogation bundle.  Ensure that _default destEID_ is an EID that 
    is valid for ION, and that the interrogator is a source EID that is also
    a valid destination EID.  Note that "dtn:none" is not a valid destination
    EID, but is a valid source EID.

# NOTES

A very simple interrogator is **bpchat** which can repeatedly interrogate
**bpstats2** by just striking the enter key.

# BUGS

Report bugs to <ion-dtn-support@lists.sourceforge.net>

# SEE ALSO

bpstats(1), bpchat(1)
