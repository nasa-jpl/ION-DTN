# NAME

sdatest - LTP service data aggreggation test program

# SYNOPSIS

**sdatest** \[_remoteEngineNbr_\]

# DESCRIPTION

**sdatest** tests LTP service data aggregation (SDA).

Each instance of the program can run either as a sender and receiver or as a
receiver only.  For sending, **sdatest** starts a background thread that offers
a command-line prompt and accepts strings of text for transmission.  It uses
SDA to aggregate the strings into LTP service data items for LTP client ID 2,
which is SDA.  The strings (SDA data units) are delimited by terminating NULLs,
and they are all identified by SDA client ID number 135.

For reception, **sdatest** listens for LTP traffic for client 2.  When it
receives an LTP Service data item (in an LTP block), it parses out all of
the SDA data units aggregated in that data item and prints them to stdout.

Use ^C to terminate **sdatest**.

# EXIT STATUS

- "0"

    **sdatest** has terminated.  Any problems encountered during operation
    will be noted in the **ion.log** log file.

- "1"

    **sdatest** was unable to start.  See the **ion.log** file for details.

# FILES

No files are used by sdatest.

# ENVIRONMENT

No environment variables apply.

# DIAGNOSTICS

Diagnostic messages produced by **sdatest** are written to the ION log
file _ion.log_.

# BUGS

Report bugs to <ion-dtn-support@lists.sourceforge.net>

# SEE ALSO

sda(3)
