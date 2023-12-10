# NAME

file2dgr - DGR transmission test program

# SYNOPSIS

**file2dgr** _remoteHostName_ _fileName_ \[_nbrOfCycles_\]

# DESCRIPTION

**file2dgr** uses DGR to send _nbrOfCycles_ copies of the text of the file
named _fileName_ to the **dgr2file** process running on the computer identified
by _remoteHostName_.  If not specified (or if less than 1), _nbrOfCycles_
defaults to 1.  After sending all lines of the file, **file2dgr** sends a
datagram containing an EOF string (the ASCII text "\*\*\* End of the file \*\*\*")
before reopening the file and starting transmission of the next copy.

When all copies of the file have been sent, **file2dgr** prints a performance
report:

        Bytes sent = I<byteCount>, usec elapsed = I<elapsedTime>.

        Sending I<dataRate> bits per second.

# EXIT STATUS

- "0"

    **file2dgr** has terminated.

# FILES

No configuration files are needed.

# ENVIRONMENT

No environment variables apply.

# DIAGNOSTICS

Diagnostic messages produced by **file2dgr** are written to the ION log
file _ion.log_.

- Can't open dgr service.

    ION system error.  Check for earlier diagnostic messages describing
    the cause of the error; correct problem and rerun.

- Can't open input file

    Operating system error.  Check errtext, correct problem, and rerun.

- Can't acquire DGR working memory.

    ION system error.  Check for earlier diagnostic messages describing
    the cause of the error; correct problem and rerun.

- Can't reopen input file

    Operating system error.  Check errtext, correct problem, and rerun.

- Can't read from input file

    Operating system error.  Check errtext, correct problem, and rerun.

- dgr\_send failed.

    ION system error.  Check for earlier diagnostic messages describing
    the cause of the error; correct problem and rerun.

# BUGS

Report bugs to <ion-dtn-support@lists.sourceforge.net>

# SEE ALSO

file2dgr(1), dgr(3)
