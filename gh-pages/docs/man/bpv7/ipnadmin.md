# NAME

ipnadmin - Interplanetary Internet (IPN) scheme administration interface

# SYNOPSIS

**ipnadmin** \[ _commands\_filename_ \]

# DESCRIPTION

**ipnadmin** configures the local ION node's routing of bundles to endpoints
whose IDs conform to the _ipn_ endpoint ID scheme.  Every endpoint ID in the
_ipn_ scheme is a string of the form "ipn:_node\_number_._service\_number_"
where _node\_number_ is a CBHE "node number" and _service\_number_ identifies
a specific application processing point.  When _service\_number_ is zero,
the endpoint ID constitutes a node ID.  All endpoint IDs formed in the
_ipn_ scheme identify singleton endpoints.

**ipnadmin** operates in response to IPN scheme configuration commands found
in the file _commands\_filename_, if provided; if not, **ipnadmin** prints
a simple prompt (:) so that the user may type commands
directly into standard input.

The format of commands for _commands\_filename_ can be queried from **ipnadmin**
with the 'h' or '?' commands at the prompt.  The commands are documented in
ipnrc(5).

# EXIT STATUS

- "0"
Successful completion of IPN scheme administration.
- "1"
Unsuccessful completion of IPN scheme administration, due to inability to
attach to the Bundle Protocol system or to initialize the IPN scheme.

# EXAMPLES

- ipnadmin

    Enter interactive IPN scheme configuration command entry mode.

- ipnadmin host1.ipnrc

    Execute all configuration commands in _host1.ipnrc_, then terminate
    immediately.

# FILES

See ipnrc(5) for details of the IPN scheme configuration commands.

# ENVIRONMENT

No environment variables apply.

# DIAGNOSTICS

**Note**: all ION administration utilities expect source file input to be
lines of ASCII text that are NL-delimited.  If you edit the ipnrc file on
a Windows machine, be sure to **use dos2unix to convert it to Unix text format**
before presenting it to **ipnadmin**.  Otherwise **ipnadmin** will detect syntax
errors and will not function satisfactorily.

The following diagnostics may be issued to the logfile ion.log:

- ipnadmin can't attach to BP.

    Bundle Protocol has not been initialized on this computer.  You need to run
    bpadmin(1) first.

- ipnadmin can't initialize routing database.

    There is no SDR data store for _ipnadmin_ to use.  Please run ionadmin(1) to
    start the local ION node.

- Can't open command file...

    The _commands\_filename_ specified in the command line doesn't exist.

Various errors that don't cause **ipnadmin** to fail but are noted in the
**ion.log** log file may be caused by improperly formatted commands
given at the prompt or in the _commands\_filename_ file.
Please see ipnrc(5) for details.

# BUGS

Report bugs to <ion-dtn-support@lists.sourceforge.net>

# SEE ALSO

ipnrc(5)
