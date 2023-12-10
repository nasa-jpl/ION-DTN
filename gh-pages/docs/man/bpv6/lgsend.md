# NAME

lgsend - ION Load/Go command program

# SYNOPSIS

**lgsend** _command\_file\_name_ _own\_endpoint\_ID_ _destination\_endpoint\_ID_

# DESCRIPTION

ION Load/Go is a system for management of an ION-based network, enabling the
execution of ION administrative programs at remote nodes.  The system
comprises two programs, **lgsend** and **lgagent**.

The **lgsend** program reads a Load/Go source file from a local file system,
encapsulates the text of that source file in a bundle, and sends the bundle
to an **lgagent** task that is waiting for data at a designated DTN endpoint
on the remote node.

To do so, it first reads all lines of the Load/Go source file identified
by _command\_file\_name_ into a temporary buffer in ION's SDR data store,
concatenating the lines of the file and retaining all newline characters.
Then it invokes the bp\_send() function to create and send a bundle whose
payload is this temporary buffer, whose destination is
_destination\_endpoint\_ID_, and whose source endpoint ID is
_own\_endpoint\_ID_.  Then it terminates.

# EXIT STATUS

- "0"

    Load/Go file transmission succeeded.

- "1"

    Load/Go file transmission failed.  Examine **ion.log** to determine the
    cause of the failure, then re-run.

# FILES

**lgfile** contains the Load/Go file capsules and directive that are to
be sent to the remote node.

# ENVIRONMENT

No environment variables apply.

# DIAGNOSTICS

The following diagnostics may be issued to the **ion.log** log file:

- lgsend: can't attach to BP.

    Bundle Protocol is not running on this computer.  Run bpadmin(1) to start BP.

- lgsend: can't open own endpoint.

    _own\_endpoint\_ID_ is not a declared endpoint on the local ION node.
    Run bpadmin(1) to add it.

- lgsend: can't open file of LG commands: _error description_

    _command\_file\_name_ doesn't identify a file that can be opened.  Correct
    spelling of file name or file's access permissions.

- lgsend: can't get size of LG command file: _error description_

    Operating system problem.  Investigate and correct before rerunning.

- lgsend: LG cmd file size > 64000.

    Load/Go command file is too large.  Split it into multiple files if possible.

- lgsend: no space for application data unit.

    ION system problem: have exhausted available SDR data store reserves.

- lgsend: fgets failed: _error description_

    Operating system problem.  Investigate and correct before rerunning.

- lgsend: can't create application data unit.

    ION system problem: have exhausted available SDR data store reserves.

- lgsend: can't send bundle.

    ION system problem.  Investigate and correct before rerunning.

# BUGS

Report bugs to <ion-dtn-support@lists.sourceforge.net>

# SEE ALSO

lgagent(1), lgfile(5)
