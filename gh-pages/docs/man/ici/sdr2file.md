# NAME

sdr2file - SDR data extraction test program

# SYNOPSIS

**sdr2file** _configFlags_

# DESCRIPTION

**sdr2file** stress-tests SDR data extraction by retrieving and deleting
all text file lines inserted into a test SDR data store named
"testsdr_configFlags_" by the complementary test program file2sdr(1).

The operation of **sdr2file** echoes the cyclical operation of **file2sdr**:
each linked list created by **file2sdr** is used to create in the current
working directory a copy of **file2sdr**'s original source text file.
The name of each file written by **sdr2file** is file\_copy\__cycleNbr_,
where _cycleNbr_ identifies the linked list from which the file's text
lines were obtained.

**sdr2file** may catch up with the data ingestion activity of **file2sdr**,
in which case it blocks (taking the **file2sdr** test semaphore) until
the linked list it is currently draining is no longer empty.

# EXIT STATUS

- "0"

    **sdr2file** has terminated.

# FILES

No configuration files are needed.

# ENVIRONMENT

No environment variables apply.

# DIAGNOSTICS

- Can't use sdr.

    ION system error.  Check for diagnostics in the ION log file _ion.log_.

- Can't create semaphore.

    ION system error.  Check for diagnostics in the ION log file _ion.log_.

- SDR transaction failed.

    ION system error.  Check for diagnostics in the ION log file _ion.log_.

- Can't open output file

    Operating system error.  Check errtext, correct problem, and rerun.

- can't write to output file

    Operating system error.  Check errtext, correct problem, and rerun.

# BUGS

Report bugs to <ion-dtn-support@lists.sourceforge.net>

# SEE ALSO

file2sdr(1), sdr(3)
