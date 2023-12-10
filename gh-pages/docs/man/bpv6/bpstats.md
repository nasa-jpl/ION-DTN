# NAME

bpstats - Bundle Protocol (BP) processing statistics query utility

# SYNOPSIS

**bpstats**

# DESCRIPTION

**bpstats** simply logs messages containing the current values of all BP
processing statistics accumulators, then terminates.

# EXIT STATUS

- "0"

    **bpstats** has terminated.

# FILES

No configuration files are needed.

# ENVIRONMENT

No environment variables apply.

# DIAGNOSTICS

The following diagnostics may be issued to the **ion.log** log file:

- bpstats can't attach to BP.

    **bpadmin** has not yet initialized BP operations.

# BUGS

Report bugs to <ion-dtn-support@lists.sourceforge.net>

# SEE ALSO

ion(3)
