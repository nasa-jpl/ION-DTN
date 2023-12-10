# NAME

tccadmin - DTKA node administration interface

# SYNOPSIS

**tccadmin** _blocks\_group\_number_ \[ _commands\_filename_ \]

# DESCRIPTION

**tccadmin** configures and manages the Trusted Collecive client databases
for TC applications on the local ION node, enabling the node to utilize the
services of one or more collective authorities.

The first command-line argument to **tccadmin** is _blocksGroupNumber_,
which identifies the specific TC application to which all commands
submitted to this instance of **tccadmin** will apply.  A TC
application is uniquely identified by the group number of the Bundle
Protocol multicast group comprising all nodes hosting TC clients that
subscribe to TC "blocks" published for that application.

**tccadmin** configures and manages a TC client database in response to client
configuration commands found in _commands\_filename_, if provided; if not,
**tccadmin** prints a simple prompt (:) so that the user may type commands
directly into standard input.

The format of commands for _commands\_filename_ can be queried from
**tccadmin** by entering the command 'h' or '?' at the prompt.  The
commands are documented in tccrc(5).

# EXIT STATUS

- "0"

    Successful completion of TC client administration.

# EXAMPLES

- tccadmin

    Enter interactive TC client administration command entry mode.

- tccadmin host1.karc

    Execute all configuration commands in _host1.karc_, then terminate
    immediately.

# FILES

Status and diagnostic messages from **tccadmin** and from other software that
utilizes the ION node are nominally written to a log file in the current
working directory within which **tccadmin** was run.  The log file is
typically named **ion.log**.

See also tccrc(5).

# ENVIRONMENT

No environment variables apply.

# DIAGNOSTICS

**Note**: all ION administration utilities expect source file input to be
lines of ASCII text that are NL-delimited.  If you edit the tccrc file on
a Windows machine, be sure to **use dos2unix to convert it to Unix text format**
before presenting it to **tccadmin**.  Otherwise **tccadmin** will detect
syntax errors and will not function satisfactorily.

The following diagnostics may be issued to the log file:

- Can't open command file...

    The _commands\_filename_ specified in the command line doesn't exist.

Various errors that don't cause **tccadmin** to fail but are noted in the
log file may be caused by improperly formatted commands given at the prompt
or in the _commands\_filename_.  Please see tccrc(5) for details.

# BUGS

Report bugs to <ion-dtn-support@lists.sourceforge.net>

# SEE ALSO

tcc(1), tccrc(5)
