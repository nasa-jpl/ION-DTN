# NAME

bpcounter - Bundle Protocol reception test program

# SYNOPSIS

**bpcounter** _ownEndpointId_ \[_maxCount_\]

# DESCRIPTION

**bpcounter** uses Bundle Protocol to receive application data units
from a remote **bpdriver** application task.  When the total number of
application data units it has received exceeds _maxCount_, it terminates
and prints its reception count.  If _maxCount_ is omitted, the default
limit is 2 billion application data units.

# EXIT STATUS

- "0"

    **bpcounter** has terminated.  Any problems encountered during operation
    will be noted in the **ion.log** log file.

# FILES

No configuration files are needed.

# ENVIRONMENT

No environment variables apply.

# DIAGNOSTICS

Diagnostic messages produced by **bpcounter** are written to the ION log
file _ion.log_.

- Can't attach to BP.

    **bpadmin** has not yet initialized Bundle Protocol operations.

- Can't open own endpoint.

    Another application has already opened _ownEndpointId_.  Terminate that
    application and rerun.

- bpcounter bundle reception failed.

    BP system error.  Check for earlier diagnostic messages describing
    the cause of the error; correct problem and rerun.

# BUGS

Report bugs to <ion-dtn-support@lists.sourceforge.net>

# SEE ALSO

bpadmin(1), bpdriver(1), bpecho(1), bp(3)
