# NAME

ionxnowner - report on which threads are initiating SDR transactions

# SYNOPSIS

**ionxnowner** \[_interval_ \[_count_ \[_echo_\]\]\]

# DESCRIPTION

For _count_ interations (defaulting to 1), **ionxnowner** prints the
process ID and thread ID of the thread that currently "owns" the 
local node's SDR data store (i.e., started the current transaction), then
sleeps _interval_ seconds (minimum 1).  If the optional _echo_ parameter
is set to 1, then the transaction owner message is logged as well as
printed to the console.

# EXIT STATUS

- "0"

    **ionxnowner** has terminated.

# FILES

No configuration files are needed.

# ENVIRONMENT

No environment variables apply.

# DIAGNOSTICS

The following diagnostics may be issued to the **ion.log** log file:

- Can't attach to ION.

    ION system error.  See earlier diagnostic messages posted to ion.log
    for details.

- Can't access SDR.

    ION system error.  See earlier diagnostic messages posted to ion.log
    for details.

# BUGS

Report bugs to <ion-dtn-support@lists.sourceforge.net>

# SEE ALSO

ionunlock(1), sdr(3), psmwatch(1)
