# NAME

bprecvfile2 - Bundle Protocol (BP) file reception utility

# SYNOPSIS

**bprecvfile** _own\_endpoint\_ID_ \[_filename_\]

# DESCRIPTION

**This is an updated version of the original bprecvfile utility**

**bprecvfile** is intended to serve as the counterpart to **bpsendfile**.  It
uses bp\_receive() to receive bundles containing file content.  The content
of each bundle is simply written to a file named "filename". If the filename
is not provided on the command line bundles are written to stdout. Use of
UNIX pipes is allowed. Note: If filename exists data will be appended to that
file. If filename does not exist it will be created.
Use ^C to terminate the program.

# EXIT STATUS

- "0"

    **bprecvfile** has terminated.

# FILES

No configuration files are needed.

# ENVIRONMENT

No environment variables apply.

# DIAGNOSTICS

The following diagnostics may be issued to the **ion.log** log file:

- Can't attach to BP.

    **bpadmin** has not yet initialized BP operations.

- Can't open own endpoint.

    Another BP application task currently has _own\_endpoint\_ID_ open for
    bundle origination and reception.  Try again after that task has terminated.
    If no such task exists, it may have crashed while still holding the endpoint
    open; the easiest workaround is to select a different source endpoint.

- bprecvfile bundle reception failed.

    BP system error.  Check for earlier diagnostic messages describing the
    cause of the error; correct problem and rerun.

- bprecvfile: can't open test file

    File system error.  **bprecvfile** terminates.

- bprecvfile: can't receive bundle content.

    BP system error.  Check for earlier diagnostic messages describing the
    cause of the error; correct problem and rerun.

- bprecvfile: can't write to test file

    File system error.  **bprecvfile** terminates.

- bprecvfile cannot continue.

    BP system error.  Check for earlier diagnostic messages describing the
    cause of the error; correct problem and rerun.

- bprecvfile: can't handle bundle delivery.

    BP system error.  Check for earlier diagnostic messages describing the
    cause of the error; correct problem and rerun.

# BUGS

Report bugs to <ion-dtn-support@lists.sourceforge.net>

# SEE ALSO

bpsendfile(1), bp(3)