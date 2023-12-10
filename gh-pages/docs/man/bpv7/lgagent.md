# NAME

lgagent - ION Load/Go remote agent program

# SYNOPSIS

**lgagent** _own\_endpoint\_ID_

# DESCRIPTION

ION Load/Go is a system for management of an ION-based network, enabling the
execution of ION administrative programs at remote nodes.  The system
comprises two programs, **lgsend** and **lgagent**.

The **lgagent** task on a given node opens the indicated ION endpoint
for bundle reception, receives the extracted payloads of Load/Go bundles
sent to it by **lgsend** as run on one or more remote nodes, and processes
those payloads, which are the text of Load/Go source files.

Load/Go source file content is limited to newline-terminated lines of ASCII
characters.  More specifically, the text of any Load/Go source file is a
sequence of _line sets_ of two types: _file capsules_ and _directives_.
Any Load/Go source file may contain any number of file capsules and any
number of directives, freely intermingled in any order, but the typical
structure of a Load/Go source file is simply a single file capsule
followed by a single directive.

When **lgagent** identifies a file capsule, it copies all of the capsule's
text lines to a new file that it creates in the current working directory.
When **lgagent** identifies a directive, it executes the directive by
passing the text of the directive to the pseudoshell() function
(see platform(3)).  **lgagent** processes the line sets of a Load/Go source
file in the order in which they appear in the file, so the text
of a directive may reference a file that was created as the result of
processing a prior file capsule in the same source file.

# EXIT STATUS

- "0"

    Load/Go remote agent processing has terminated.

# FILES

**lgfile** contains the Load/Go file capsules and directives that are to be
processed.

# ENVIRONMENT

No environment variables apply.

# DIAGNOSTICS

The following diagnostics may be issued to the **ion.log** log file:

- lgagent: can't attach to BP.

    Bundle Protocol is not running on this computer.  Run bpadmin(1) to start BP.

- lgagent: can't open own endpoint.

    _own\_endpoint\_ID_ is not a declared endpoint on the local ION node.  Run
    bpadmin(1) to add it.

- lgagent: bundle reception failed.

    ION system problem.  Investigate and correct before restarting.

- lgagent cannot continue.

    lgagent processing problem.  See earlier diagnostic messages for details.
    Investigate and correct before restarting.

- lgagent: no space for bundle content.

    ION system problem: have exhausted available SDR data store reserves.

- lgagent: can't receive bundle content.

    ION system problem: have exhausted available SDR data store reserves.

- lgagent: can't handle bundle delivery.

    ION system problem.  Investigate and correct before restarting.

- lgagent: pseudoshell failed.

    Error in directive line, usually an attempt to execute a non-existent
    administration program (e.g., a misspelled program name).  Terminates
    processing of source file content.

A variety of other diagnostics noting source file parsing problems may also
be reported.  These errors are non-fatal but they terminate the processing
of the source file content from the most recently received bundle.

# BUGS

Report bugs to <ion-dtn-support@lists.sourceforge.net>

# SEE ALSO

lgsend(1), lgfile(5)
