# NAME

file2sdr - SDR data ingestion test program

# SYNOPSIS

**file2sdr** _configFlags_ _fileName_

# DESCRIPTION

**file2sdr** stress-tests SDR data ingestion by repeatedly writing all text
lines of the file named _fileName_ to one of a series of non-volatile
linked lists created in a test SDR data store named "testsdr_configFlags_".
By incorporating the data store configuration into the name
(e.g., "testsdr14") we make it relatively easy to perform comparative
testing on SDR data stores that are identical aside from their configuration
settings.

The operation of **file2sdr** is cyclical: a new linked list is created each
time the program finishes copying the file's text lines and starts over
again.  If you use ^C to terminate **file2sdr** and then restart it, the
program resumes operation at the point where it left off.

After writing each line to the current linked list, **file2sdr** gives a
semaphore to indicate that the list is now non-empty.  This is mainly for
the benefit of the complementary test program sdr2file(1).

At the end of each cycle **file2sdr** appends a final EOF line to the current
linked list, containing the text "\*\*\* End of the file \*\*\*", and prints a
brief performance report:

        Processing I<lineCount> lines per second.

# EXIT STATUS

- "0"

    **file2sdr** has terminated.

# FILES

No configuration files are needed.

# ENVIRONMENT

No environment variables apply.

# DIAGNOSTICS

Diagnostic messages produced by **file2sdr** are written to the ION log
file _ion.log_.

- Can't use sdr.

    ION system error.  Check for earlier diagnostic messages describing
    the cause of the error; correct problem and rerun.

- Can't create semaphore.

    ION system error.  Check for earlier diagnostic messages describing
    the cause of the error; correct problem and rerun.

- SDR transaction failed.

    ION system error.  Check for earlier diagnostic messages describing
    the cause of the error; correct problem and rerun.

- Can't open input file

    Operating system error.  Check errtext, correct problem, and rerun.

- Can't reopen input file

    Operating system error.  Check errtext, correct problem, and rerun.

- Can't read from input file

    Operating system error.  Check errtext, correct problem, and rerun.

# BUGS

Report bugs to <ion-dtn-support@lists.sourceforge.net>

# SEE ALSO

sdr2file(1), sdr(3)
