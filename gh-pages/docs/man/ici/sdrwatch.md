# NAME

sdrwatch - SDR non-volatile data store activity monitor

# SYNOPSIS

**sdrwatch** _sdr\_name_ \[ -t | -s | -r | -z \] \[_interval_ \[_count_ \[ verbose \]\]\]

# DESCRIPTION

For _count_ interations (defaulting to 1), **sdrwatch** sleeps _interval_
seconds and then performs the SDR operation indicated by the specified
mode: 's' to print statistics, 'r' to reset statistics, 'z' to print ZCO
space utilization, 't' (the default) to call the sdr\_print\_trace() function
(see sdr(3)) to report on SDR data storage management activity in the SDR
data store identified by _sdr\_name_ during that interval.  If the optional
**verbose** parameter is specified, the printed SDR activity trace will be
verbose as described in sdr(3).

If _interval_ is zero, **sdrwatch** just performs the indicated operation
once (for 't', it merely prints a current usage summary for the indicated
data store) and terminates.

**sdrwatch** is helpful for detecting and diagnosing storage space leaks.  For
debugging the ION protocol stack, _sdr\_name_ is normally "ion" but might be
overridden by the value of sdrName in the .ionconfig file used to configure
the node under study.

# EXIT STATUS

- "0"

    **sdrwatch** has terminated.

# FILES

No configuration files are needed.

# ENVIRONMENT

No environment variables apply.

# DIAGNOSTICS

The following diagnostics may be issued to the **ion.log** log file:

- Can't attach to sdr.

    ION system error.  One possible cause is that ION has not yet been
    initialized on the local computer; run ionadmin(1) to correct this.

- Can't start trace.

    Insufficient ION working memory to contain trace information.  Reinitialize
    ION with more memory.

# BUGS

Report bugs to <ion-dtn-support@lists.sourceforge.net>

# SEE ALSO

sdr(3), psmwatch(1)
