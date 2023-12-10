# NAME

ltpsecadmin - LTP security policy administration interface

# SYNOPSIS

**ltpsecadmin** \[ _commands\_filename_ \]

# DESCRIPTION

**ltpsecadmin** configures and manages LTP security policy on the local computer.

It configures and manages LTP security policy on the local
computer in response to LTP configuration commands found in
_commands\_filename_, if provided; if not, **ltpsecadmin** prints
a simple prompt (:) so that the user may type commands directly
into standard input.

The format of commands for _commands\_filename_ can be queried from
**ltpsecadmin** by entering the command 'h' or '?' at the prompt.  The
commands are documented in ltpsecrc(5).

# EXIT STATUS

- "0"

    Successful completion of LTP security policy administration.

# EXAMPLES

- ltpsecadmin

    Enter interactive LTP security policy administration command entry mode.

- ltpsecadmin host1.ltpsecrc

    Execute all configuration commands in _host1.ltpsecrc_, then terminate
    immediately.

# FILES

Status and diagnostic messages from **ltpsecadmin** and from other software that
utilizes the ION node are nominally written to a log file in the current
working directory within which **ltpsecadmin** was run.  The log file is
typically named **ion.log**.

See also ltpsecrc(5).

# ENVIRONMENT

No environment variables apply.

# DIAGNOSTICS

**Note**: all ION administration utilities expect source file input to be
lines of ASCII text that are NL-delimited.  If you edit the ionrc file on
a Windows machine, be sure to **use dos2unix to convert it to Unix text format**
before presenting it to **ltpsecadmin**.  Otherwise **ltpsecadmin** will detect
syntax errors and will not function satisfactorily.

The following diagnostics may be issued to the log file:

- Can't open command file...

    The _commands\_filename_ specified in the command line doesn't exist.

Various errors that don't cause **ltpsecadmin** to fail but are noted in the
log file may be caused by improperly formatted commands given at the prompt
or in the _commands\_filename_.  Please see ltpsecrc(5) for details.

# BUGS

Report bugs to <ion-dtn-support@lists.sourceforge.net>

# SEE ALSO

ltpsecrc(5)
