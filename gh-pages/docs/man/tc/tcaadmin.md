# NAME

tcaadmin - Trusted Collective (TC) authority administration interface

# SYNOPSIS

**tcaadmin** _blocks\_group\_number_ \[ _commands\_filename_ \]

# DESCRIPTION

**tcaadmin** configures and manages the Trusted Collective authority
databases for TC applications on the local ION node, enabling the node
to function as a member of one or more collective authorities.

The first command-line argument to **tcaadmin** is _blocksGroupNumber_,
which identifies the specific TC application to which all commands
submitted to this instance of **tcaadmin** will apply.  A TC
application is uniquely identified by the group number of the Bundle
Protocol multicast group comprising all nodes hosting TC clients that
subscribe to TC "blocks" published for that application.

**tcaadmin** configures and manages a TC authority database in response to
authority configuration commands found in _commands\_filename_, if provided;
if not, **tcaadmin** prints a simple prompt (:) so that the user may type
commands directly into standard input.

The format of commands for _commands\_filename_ can be queried from
**tcaadmin** by entering the command 'h' or '?' at the prompt.  The
commands are documented in tcarc(5).

# EXIT STATUS

- "0"

    Successful completion of TC authority administration.

# EXAMPLES

- tcaadmin 203

    Enter interactive TC authority administration command entry mode for
    application 203.

- tcaadmin 203 host1.tcarc

    Apply the application-203 configuration commands in _host1.tcarc_, then
    terminate immediately.

# FILES

Status and diagnostic messages from **tcaadmin** and from other software that
utilizes the ION node are nominally written to a log file in the current
working directory within which **tcaadmin** was run.  The log file is
typically named **ion.log**.

See also tcarc(5).

# ENVIRONMENT

No environment variables apply.

# DIAGNOSTICS

**Note**: all ION administration utilities expect source file input to be
lines of ASCII text that are NL-delimited.  If you edit the tcarc file on
a Windows machine, be sure to **use dos2unix to convert it to Unix text format**
before presenting it to **tcaadmin**.  Otherwise **tcaadmin** will detect
syntax errors and will not function satisfactorily.

The following diagnostics may be issued to the log file:

- Can't open command file...

    The _commands\_filename_ specified in the command line doesn't exist.

Various errors that don't cause **tcaadmin** to fail but are noted in the
log file may be caused by improperly formatted commands given at the prompt
or in the _commands\_filename_.  Please see tcarc(5) for details.

# BUGS

Report bugs to <ion-dtn-support@lists.sourceforge.net>

# SEE ALSO

tcacompile(1), tcapublish(1), tcarecv(1), tcarc(5)
