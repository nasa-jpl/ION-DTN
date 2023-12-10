# NAME

bibeadmin - bundle-in-bundle encapsulation database administration interface

# SYNOPSIS

**bibeadmin** \[ _commands\_filename_ \]

# DESCRIPTION

**bibeadmin** configures the local ION node's database of parameters governing
the forwarding of BIBE PDUs to specified remote nodes.

**bibeadmin** operates in response to BIBE configuration commands found
in the file _commands\_filename_, if provided; if not, **bibeadmin** prints
a simple prompt (:) so that the user may type commands
directly into standard input.

The format of commands for _commands\_filename_ can be queried from **bibeadmin**
with the 'h' or '?' commands at the prompt.  The commands are documented in
biberc(5).

# EXIT STATUS

- "0"
Successful completion of BIBE administration.
- "1"
Unsuccessful completion of BIBE administration, due to inability to
attach to the Bundle Protocol system or to initialize the BIBE database.

# EXAMPLES

- bibeadmin

    Enter interactive BIBE configuration command entry mode.

- bibeadmin host1.biberc

    Execute all configuration commands in _host1.biberc_, then terminate
    immediately.

# FILES

See biberc(5) for details of the BIBE configuration commands.

# ENVIRONMENT

No environment variables apply.

# DIAGNOSTICS

**Note**: all ION administration utilities expect source file input to be
lines of ASCII text that are NL-delimited.  If you edit the biberc file on
a Windows machine, be sure to **use dos2unix to convert it to Unix text format**
before presenting it to **bibeadmin**.  Otherwise **bibeadmin** will detect syntax
errors and will not function satisfactorily.

The following diagnostics may be issued to the logfile ion.log:

- bibeadmin can't attach to BP.

    Bundle Protocol has not been initialized on this computer.  You need to run
    bpadmin(1) first.

- bibeadmin can't initialize routing database.

    There is no SDR data store for _bibeadmin_ to use.  Please run ionadmin(1) to
    start the local ION node.

- Can't open command file...

    The _commands\_filename_ specified in the command line doesn't exist.

Various errors that don't cause **bibeadmin** to fail but are noted in the
**ion.log** log file may be caused by improperly formatted commands
given at the prompt or in the _commands\_filename_ file.
Please see biberc(5) for details.

# BUGS

Report bugs to <ion-dtn-support@lists.sourceforge.net>

# SEE ALSO

bibeclo(1), biberc(5)
