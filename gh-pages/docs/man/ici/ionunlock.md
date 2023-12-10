# NAME

ionunlock - utility for unlocking a locked ION node

# SYNOPSIS

**ionunlock** \[_sdr\_name_\]

# DESCRIPTION

The **ionunlock** program is designed to be run when some ION thread has
terminated while it is the owner of the node's system mutex, i.e., while
in the midst of an SDR transaction.  IT MUST NEVER BE RUN AT ANY OTHER TIME
as it will totally corrupt a node that is not locked up. The program simply
declares itself to be the owner of the incomplete transaction and cancels it,
enabling the rest of the system to resume operations.

If omitted, _sdr\_name_ defaults to "ion".

# EXIT STATUS

- "0"

    **ionunlock** has terminated successfully.

- "1"

    **ionunlock** has terminated unsuccessfully.  See diagnostic messages in the
    **ion.log** log file for details.

# FILES

No configuration files are needed.

# ENVIRONMENT

No environment variables apply.

# DIAGNOSTICS

The following diagnostics may be issued to the **ion.log** log file:

- Can't initialize the SDR system.

    Probable operations error: ION appears not to be initialized, in which case
    there is no point in running **ionunlock**.

- Can't start using SDR.

    ION system error.  See earlier diagnostic messages posted to **ion.log**
    for details.  In this event it is unlikely that **ionunlock** can be run
    successfully, and it is also unlikely that it would have any effect if it
    did run successfully.

- ionunlock unnecessary; exiting.

    Either the indicated SDR is not initialized or it is not currently stuck in
    a transaction.

# BUGS

Report bugs to <ion-dtn-support@lists.sourceforge.net>

# SEE ALSO

ionxnowner(1), sdrmend(1), sdr(3)
