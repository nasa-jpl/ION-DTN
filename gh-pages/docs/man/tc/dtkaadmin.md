# NAME

dtkaadmin - DTKA user function administration interface

# SYNOPSIS

**dtkaadmin** \[ _commands\_filename_ \]

# DESCRIPTION

**dtkaadmin** configures and manages the DTKA administration database for
the local ION node, enabling the node to utilize the services of the
Trusted Collective for Delay-Tolerant Key Administration.

It configures and manages that database in response to DTKA configuration
commands found in _commands\_filename_, if provided; if not, **dtkaadmin**
prints a simple prompt (:) so that the user may type commands directly
into standard input.

The format of commands for _commands\_filename_ can be queried from
**dtkaadmin** by entering the command 'h' or '?' at the prompt.  The
commands are documented in dtkarc(5).

# EXIT STATUS

- "0"

    Successful completion of DTKA configuration.

# EXAMPLES

- dtkaadmin

    Enter interactive DTKA configuration command entry mode.

- dtkaadmin host1.karc

    Execute all configuration commands in _host1.karc_, then terminate
    immediately.

# FILES

Status and diagnostic messages from **dtkaadmin** and from other software that
utilizes the ION node are nominally written to a log file in the current
working directory within which **dtkaadmin** was run.  The log file is
typically named **ion.log**.

See also dtkarc(5).

# ENVIRONMENT

No environment variables apply.

# DIAGNOSTICS

**Note**: all ION administration utilities expect source file input to be
lines of ASCII text that are NL-delimited.  If you edit the dtkarc file on
a Windows machine, be sure to **use dos2unix to convert it to Unix text format**
before presenting it to **dtkaadmin**.  Otherwise **dtkaadmin** will detect
syntax errors and will not function satisfactorily.

The following diagnostics may be issued to the log file:

- Can't open command file...

    The _commands\_filename_ specified in the command line doesn't exist.

Various errors that don't cause **dtkaadmin** to fail but are noted in the
log file may be caused by improperly formatted commands given at the prompt
or in the _commands\_filename_.  Please see dtkarc(5) for details.

# BUGS

Report bugs to <ion-dtn-support@lists.sourceforge.net>

# SEE ALSO

dtka(1), dtkarc(5)
