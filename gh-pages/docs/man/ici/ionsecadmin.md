# NAME

ionsecadmin - ION global security database management interface

# SYNOPSIS

**ionsecadmin** \[ _commands\_filename_ \]

# DESCRIPTION

**ionsecadmin** configures and manages the ION security database
on the local computer.

It configures and manages the ION security database on the local
computer in response to ION configuration commands found in
_commands\_filename_, if provided; if not, **ionsecadmin** prints
a simple prompt (:) so that the user may type commands directly
into standard input.

The format of commands for _commands\_filename_ can be queried from
**ionsecadmin** by entering the command 'h' or '?' at the prompt.  The
commands are documented in ionsecrc(5).

# EXIT STATUS

- "0"

    Successful completion of ION security database administration.

# EXAMPLES

- ionsecadmin

    Enter interactive ION security policy administration command entry mode.

- ionsecadmin host1.ionsecrc

    Execute all configuration commands in _host1.ionsecrc_, then terminate
    immediately.

# FILES

Status and diagnostic messages from **ionsecadmin** and from other software that
utilizes the ION node are nominally written to a log file in the current
working directory within which **ionsecadmin** was run.  The log file is
typically named **ion.log**.

See also ionsecrc(5).

# ENVIRONMENT

No environment variables apply.

# DIAGNOSTICS

**Note**: all ION administration utilities expect source file input to be
lines of ASCII text that are NL-delimited.  If you edit the ionrc file on
a Windows machine, be sure to **use dos2unix to convert it to Unix text format**
before presenting it to **ionsecadmin**.  Otherwise **ionsecadmin** will detect
syntax errors and will not function satisfactorily.

The following diagnostics may be issued to the log file:

- Can't open command file...

    The _commands\_filename_ specified in the command line doesn't exist.

Various errors that don't cause **ionsecadmin** to fail but are noted in the
log file may be caused by improperly formatted commands given at the prompt
or in the _commands\_filename_.  Please see ionsecrc(5) for details.

# BUGS

Report bugs to <ion-dtn-support@lists.sourceforge.net>

# SEE ALSO

ionsecrc(5)
