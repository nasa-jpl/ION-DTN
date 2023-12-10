# NAME

imcadmin - Interplanetary Multicast (IMC) scheme administration interface

# SYNOPSIS

**imcadmin** \[ _commands\_filename_ \]

# DESCRIPTION

**imcadmin** configures the local ION node's routing of bundles to endpoints
whose IDs conform to the _imc_ endpoint ID scheme.  _imc_ is a CBHE-conformant
scheme; that is, every endpoint ID in the _imc_ scheme is a string of the
form "imc:_group\_number_._service\_number_" where _group\_number_ (an IMC
multicast group number) serves as a CBHE "node number" and _service\_number_
identifies a specific application processing point.

**imcadmin** operates in response to IMC scheme configuration commands found
in the file _commands\_filename_, if provided; if not, **imcadmin** prints
a simple prompt (:) so that the user may type commands
directly into standard input.

The format of commands for _commands\_filename_ can be queried from **imcadmin**
with the 'h' or '?' commands at the prompt.  The commands are documented in
imcrc(5).

# EXIT STATUS

- "0"
Successful completion of IMC scheme administration.
- "1"
Unsuccessful completion of IMC scheme administration, due to inability to
attach to the Bundle Protocol system or to initialize the IMC scheme.

# EXAMPLES

- imcadmin

    Enter interactive IMC scheme configuration command entry mode.

- imcadmin host1.imcrc

    Execute all configuration commands in _host1.imcrc_, then terminate
    immediately.

# FILES

See imcrc(5) for details of the IMC scheme configuration commands.

# ENVIRONMENT

No environment variables apply.

# DIAGNOSTICS

**Note**: all ION administration utilities expect source file input to be
lines of ASCII text that are NL-delimited.  If you edit the imcrc file on
a Windows machine, be sure to **use dos2unix to convert it to Unix text format**
before presenting it to **imcadmin**.  Otherwise **imcadmin** will detect syntax
errors and will not function satisfactorily.

The following diagnostics may be issued to the logfile ion.log:

- imcadmin can't attach to BP.

    Bundle Protocol has not been initialized on this computer.  You need to run
    bpadmin(1) first.

- imcadmin can't initialize routing database.

    There is no SDR data store for _imcadmin_ to use.  Please run ionadmin(1) to
    start the local ION node.

- Can't open command file...

    The _commands\_filename_ specified in the command line doesn't exist.

Various errors that don't cause **imcadmin** to fail but are noted in the
**ion.log** log file may be caused by improperly formatted commands
given at the prompt or in the _commands\_filename_ file.
Please see imcrc(5) for details.

# BUGS

Report bugs to <ion-dtn-support@lists.sourceforge.net>

# SEE ALSO

imcrc(5)
