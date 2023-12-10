# NAME

ionlog - utility for redirecting stdin to the ION log file

# SYNOPSIS

**ionlog**

# DESCRIPTION

The **ionlog** program simply reads lines of text from stdin and uses
writeMemo to copy them into the ion.log file.  It terminates when it
reaches EOF in stdin.

# EXIT STATUS

- "0"

    **ionlog** has terminated successfully.

# FILES

No configuration files are needed.

# ENVIRONMENT

No environment variables apply.

# DIAGNOSTICS

The following diagnostics may be issued to the **ion.log** log file:

- ionlog unable to attach to ION.

    Probable operations error: ION appears not to be initialized, in which case
    there is no point in running **ionlog**.

# BUGS

Report bugs to <ion-dtn-support@lists.sourceforge.net>

# SEE ALSO

amslogprt(1)
