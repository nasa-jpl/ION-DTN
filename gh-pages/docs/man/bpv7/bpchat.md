# NAME

bpchat - Bundle Protocol chat test program

# SYNOPSIS

**bpchat** _sourceEID_ _destEID_ \[ct\]

# DESCRIPTION

**bpchat** uses Bundle Protocol to send input text in bundles, and display the
payload of received bundles as output.  It is similar to the **talk** utility,
but operates over the Bundle Protocol.  It operates like a combination of the
**bpsource** and **bpsink** utilities in one program (unlike **bpsource**, 
**bpchat** emits bundles with a _sourceEID_).

If the _sourceEID_ and _destEID_ are both **bpchat** applications, then two
users can chat with each other over the Bundle Protocol: lines that one user
types on the keyboard will be transported over the network in bundles and
displayed on the screen of the other user (and the reverse).

**bpchat** terminates upon receiving the SIGQUIT signal, i.e., ^C from the
keyboard.

# EXIT STATUS

- "0"

    **bpchat** has terminated normally.  Any problems encountered during operation
    will be noted in the **ion.log** log file.

- "1"

    **bpchat** has terminated due to a BP transmit or reception failure.  Details
    should be noted in the **ion.log** log file.

# OPTIONS

- \[ct\]

    If the string "ct" is appended as the last argument, then bundles will be sent
    with custody transfer requested.

# FILES

No configuration files are needed.

# ENVIRONMENT

No environment variables apply.

# DIAGNOSTICS

Diagnostic messages produced by **bpchat** are written to the ION log
file _ion.log_.

- Can't attach to BP.

    **bpadmin** has not yet initialized Bundle Protocol operations.

- Can't open own endpoint.

    Another application has already opened _ownEndpointId_.  Terminate that
    application and rerun.

- bpchat bundle reception failed.

    BP system error.  Check for earlier diagnostic messages describing
    the cause of the error; correct problem and rerun.

- No space for ZCO extent.

    ION system error.  Check for earlier diagnostic messages describing
    the cause of the error; correct problem and rerun.

- Can't create ZCO.

    ION system error.  Check for earlier diagnostic messages describing
    the cause of the error; correct problem and rerun.

- bpchat can't send echo bundle.

    BP system error.  Check for earlier diagnostic messages describing
    the cause of the error; correct problem and rerun.

# BUGS

Report bugs to <ion-dtn-support@lists.sourceforge.net>

# SEE ALSO

bpecho(1), bpsource(1), bpsink(1), bp(3)
