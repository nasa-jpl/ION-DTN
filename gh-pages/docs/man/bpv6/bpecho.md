# NAME

bpecho - Bundle Protocol reception test program

# SYNOPSIS

**bpecho** _ownEndpointId_

# DESCRIPTION

**bpecho** uses Bundle Protocol to receive application data units
from a remote **bpdriver** application task.  In response to each
received application data unit it sends back an "echo" application data
unit of length 2, the NULL-terminated string "x".

**bpecho** terminates upon receiving the SIGQUIT signal, i.e., ^C from the
keyboard.

# EXIT STATUS

- "0"

    **bpecho** has terminated normally.  Any problems encountered during operation
    will be noted in the **ion.log** log file.

- "1"

    **bpecho** has terminated due to a BP reception failure.  Details should be
    noted in the **ion.log** log file.

# FILES

No configuration files are needed.

# ENVIRONMENT

No environment variables apply.

# DIAGNOSTICS

Diagnostic messages produced by **bpecho** are written to the ION log
file _ion.log_.

- Can't attach to BP.

    **bpadmin** has not yet initialized Bundle Protocol operations.

- Can't open own endpoint.

    Another application has already opened _ownEndpointId_.  Terminate that
    application and rerun.

- bpecho bundle reception failed.

    BP system error.  Check for earlier diagnostic messages describing
    the cause of the error; correct problem and rerun.

- No space for ZCO extent.

    ION system error.  Check for earlier diagnostic messages describing
    the cause of the error; correct problem and rerun.

- Can't create ZCO.

    ION system error.  Check for earlier diagnostic messages describing
    the cause of the error; correct problem and rerun.

- bpecho can't send echo bundle.

    BP system error.  Check for earlier diagnostic messages describing
    the cause of the error; correct problem and rerun.

# BUGS

Report bugs to <ion-dtn-support@lists.sourceforge.net>

# SEE ALSO

bpadmin(1), bpdriver(1), bpcounter(1), bp(3)
