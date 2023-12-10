# NAME

sm2file - shared-memory linked list data extraction test program

# SYNOPSIS

**sm2file**

# DESCRIPTION

**sm2file** stress-tests shared-memory linked list data extraction by
retrieving and deleting all text file lines inserted into a shared-memory
linked list that is the root object of a PSM partition named "file2sm".

The operation of **sm2file** echoes the cyclical operation of **file2sm**:
the EOF lines inserted into the linked list by **file2sm** punctuate the
writing of files that are copies of **file2sm**'s original source text file.
The name of each file written by **sm2file** is file\_copy\__cycleNbr_,
where _cycleNbr_ is, in effect, the count of EOF lines encountered in the
linked list up to the point at which the writing of this file began.

**sm2file** may catch up with the data ingestion activity of **file2sm**,
in which case it blocks (taking the **file2sm** test semaphore) until
the linked list is no longer empty.

# EXIT STATUS

- "0"

    **sm2file** has terminated.

# FILES

No configuration files are needed.

# ENVIRONMENT

No environment variables apply.

# DIAGNOSTICS

- can't attach to shared memory

    Operating system error.  Check errtext, correct problem, and rerun.

- Can't manage shared memory.

    PSM error.  Check for earlier diagnostics describing the cause of the
    error; correct problem and rerun.

- Can't create shared memory list.

    PSM error.  Check for earlier diagnostics describing the cause of the
    error; correct problem and rerun.

- Can't create semaphore.

    ION system error.  Check for earlier diagnostics describing the cause of the
    error; correct problem and rerun.

- Can't open output file

    Operating system error.  Check errtext, correct problem, and rerun.

- can't write to output file

    Operating system error.  Check errtext, correct problem, and rerun.

# BUGS

Report bugs to <ion-dtn-support@lists.sourceforge.net>

# SEE ALSO

file2sm(1), smlist(3), psm(3)
