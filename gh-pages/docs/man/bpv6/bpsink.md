# NAME

bpsink - Bundle Protocol reception test program

# SYNOPSIS

**bpsink** _ownEndpointId_

# DESCRIPTION

**bpsink** uses Bundle Protocol to receive application data units from a
remote **bpsource** application task.  For each application data unit it
receives, it prints the ADU's length and -- if length is less than 80 -- its
text.

**bpsink** terminates upon receiving the SIGQUIT signal, i.e., ^C from the
keyboard.

# EXIT STATUS

- "0"

    **bpsink** has terminated.  Any problems encountered during operation
    will be noted in the **ion.log** log file.

# FILES

No configuration files are needed.

# ENVIRONMENT

No environment variables apply.

# DIAGNOSTICS

Diagnostic messages produced by **bpsink** are written to the ION log
file _ion.log_.

- Can't attach to BP.

    **bpadmin** has not yet initialized Bundle Protocol operations.

- Can't open own endpoint.

    Another application has already opened _ownEndpointId_.  Terminate that
    application and rerun.

- bpsink bundle reception failed.

    BP system error.  Check for earlier diagnostic messages describing
    the cause of the error; correct problem and rerun.

- Can't receive payload.

    BP system error.  Check for earlier diagnostic messages describing
    the cause of the error; correct problem and rerun.

- Can't handle delivery.

    BP system error.  Check for earlier diagnostic messages describing
    the cause of the error; correct problem and rerun.

# BUGS

Report bugs to <ion-dtn-support@lists.sourceforge.net>

# SEE ALSO

bpadmin(1), bpsource(1), bp(3)
