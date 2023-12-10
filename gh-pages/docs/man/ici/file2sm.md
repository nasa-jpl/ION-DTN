# NAME

file2sm - shared-memory linked list data ingestion test program

# SYNOPSIS

**file2sm** _fileName_

# DESCRIPTION

**file2sm** stress-tests shared-memory linked list data ingestion by repeatedly
writing all text lines of the file named _fileName_ to a shared-memory linked
list that is the root object of a PSM partition named "file2sm".

After writing each line to the linked list, **file2sm** gives a semaphore to
indicate that the list is now non-empty.  This is mainly for the benefit of
the complementary test program sm2file(1).

The operation of **file2sm** is cyclical.  After copying all text lines of
the source file to the linked list, **file2sm** appends an EOF line to
the linked list, containing the text "\*\*\* End of the file \*\*\*", and prints a
brief performance report:

        Processing I<lineCount> lines per second.

Then it reopens the source file and starts appending the file's text lines
to the linked list again.

# EXIT STATUS

- "0"

    **file2sm** has terminated.

# FILES

No configuration files are needed.

# ENVIRONMENT

No environment variables apply.

# DIAGNOSTICS

- Can't attach to shared memory

    Operating system error.  Check errtext, correct problem, and rerun.

- Can't manage shared memory.

    PSM error.  Check for earlier diagnostic messages describing
    the cause of the error; correct problem and rerun.

- Can't create shared memory list.

    smlist error.  Check for earlier diagnostic messages describing
    the cause of the error; correct problem and rerun.

- Can't create semaphore.

    ION system error.  Check for earlier diagnostic messages describing
    the cause of the error; correct problem and rerun.

- Can't open input file

    Operating system error.  Check errtext, correct problem, and rerun.

- Can't reopen input file

    Operating system error.  Check errtext, correct problem, and rerun.

- Can't read from input file

    Operating system error.  Check errtext, correct problem, and rerun.

- Ran out of memory.

    Nominal behavior.  **sm2file** is not extracting data from the linked list
    quickly enough to prevent it from growing to consume all memory allocated
    to the test partition.

# BUGS

Report bugs to <ion-dtn-support@lists.sourceforge.net>

# SEE ALSO

sm2file(1), smlist(3), psm(3)
