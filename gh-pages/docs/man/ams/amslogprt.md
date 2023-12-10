# NAME

amslogprt - UNIX utility program for printing AMS log messages from amslog

# SYNOPSIS

**amslogprt**

# DESCRIPTION

**amslogprt** simply reads AMS activity log messages from standard input
(nominally written by **amslog** and prints them.  When the content of a
logged message is judged not to be an ASCII text string, the content is
printed in hexadecimal.

**amslogprt** terminates at the end of input.

# EXIT STATUS

- "0"

    **amslogprt** terminated normally.

# FILES

No files are needed by amslogprt.

# ENVIRONMENT

No environment variables apply.

# DIAGNOSTICS

None.

# BUGS

Report bugs to <ion-dtn-support@lists.sourceforge.net>

# SEE ALSO

amsrc(5)
