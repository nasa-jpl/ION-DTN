# NAME

sdrmend - SDR corruption repair utility

# SYNOPSIS

**sdrmend** _sdr\_name_ _config\_flags_ _heap\_words_ _heap\_key_ _path\_name_ \[_restartCmd_ _restartLatency_\]

# DESCRIPTION

The **sdrmend** program simply invokes the sdr\_reload\_profile() function (see
sdr(3)) to effect necessary repairs in a potentially corrupt SDR, e.g., due to
the demise of a program that had an SDR transaction in progress at the moment
it crashed.

Note that **sdrmend** need not be run to repair ION's data store in the event
of a hardware reboot: restarting ION will automatically reload the data
store's profile.  **sdrmend** is needed only when it is desired to repair
the data store without requiring all ION software to terminate and restart.

# EXIT STATUS

- "0"

    **sdrmend** has terminated successfully.

- "1"

    **sdrmend** has terminated unsuccessfully.  See diagnostic messages in the
    **ion.log** log file for details.

# FILES

No configuration files are needed.

# ENVIRONMENT

No environment variables apply.

# DIAGNOSTICS

The following diagnostics may be issued to the **ion.log** log file:

- Can't initialize the SDR system.

    Probable operations error: ION appears not to be initialized, in which case
    there is no point in running **sdrmend**.

- Can't reload profile for SDR.

    ION system error.  See earlier diagnostic messages posted to **ion.log**
    for details.  In this event it is unlikely that **sdrmend** can be run
    successfully, and it is also unlikely that it would have any effect if it
    did run successfully.

# BUGS

Report bugs to <ion-dtn-support@lists.sourceforge.net>

# SEE ALSO

ionunlock(1), sdr(3), ionadmin(1)
