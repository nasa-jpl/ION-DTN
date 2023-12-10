# NAME

psmwatch - PSM memory partition activity monitor

# SYNOPSIS

**psmwatch** _shared\_memory\_key_ _memory\_size_ _partition\_name_ _interval_ _count_ \[ verbose \]

# DESCRIPTION

For _count_ interations, **psmwatch** sleeps _interval_ seconds and then
invokes the psm\_print\_trace() function (see psm(3)) to report on PSM dynamic
memory management activity in the PSM-managed shared memory partition
identified by _shared\_memory\_key_ during that interval.  If the optional
**verbose** parameter is specified, the printed PSM activity trace will be
verbose as described in psm(3).

To prevent confusion, the specified _memory\_size_ and _partition\_name_ are
compared to those declared when this shared memory partition was initially
managed; if they don't match, **psmwatch** immediately terminates.

If _interval_ is zero, **psmwatch** merely prints a current usage summary
for the indicated shared-memory partition and terminates.

**psmwatch** is helpful for detecting and diagnosing memory leaks.  For
debugging the ION protocol stack:

- _shared\_memory\_key_

    Normally "65281", but might be overridden by the value of wmKey in the
    .ionconfig file used to configure the node under study.

- _memory\_size_

    As given by the value of wmKey in the .ionconfig file used to configure the
    node under study.  If this value is not stated in the .ionconfig file, the
    default value is "5000000".

- _partition\_name_

    Always "ionwm".

# EXIT STATUS

- "0"

    **psmwatch** has terminated.

# FILES

No configuration files are needed.

# ENVIRONMENT

No environment variables apply.

# DIAGNOSTICS

The following diagnostics may be issued to the **ion.log** log file:

- Can't attach to psm.

    ION system error.  One possible cause is that ION has not yet been
    initialized on the local computer; run ionadmin(1) to correct this.

- Can't start trace.

    Insufficient ION working memory to contain trace information.  Reinitialize
    ION with more memory.

# BUGS

Report bugs to <ion-dtn-support@lists.sourceforge.net>

# SEE ALSO

psm(3), sdrwatch(1)
