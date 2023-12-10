# NAME

acsadmin - ION Aggregate Custody Signal (ACS) administration interface

# SYNOPSIS

**acsadmin** \[ _commands\_filename_ \]

# DESCRIPTION

**acsadmin** configures aggregate custody signal behavior for the local ION
node.

It operates in response to ACS configuration commands found in the file
_commands\_filename_, if provided; if not, **acsadmin** prints
a simple prompt (:) so that the user may type commands
directly into standard input.

The format of commands for _commands\_filename_ can be queried from **acsadmin**
with the 'h' or '?' commands at the prompt.  The commands are documented in
acsrc(5).

# EXIT STATUS

- "0"
Successful completion of ACS administration.

# EXAMPLES

- acsadmin

    Enter interactive ACS configuration command entry mode.

- acsadmin host1.acs

    Execute all configuration commands in _host1.acs_, then terminate immediately.

# FILES

See acsrc(5) for details of the ACS configuration commands.

# ENVIRONMENT

No environment variables apply.

# DIAGNOSTICS

**Note**: all ION administration utilities expect source file input to be
lines of ASCII text that are NL-delimited.  If you edit the acsrc file on
a Windows machine, be sure to **use dos2unix to convert it to Unix text format**
before presenting it to **acsadmin**.  Otherwise **acsadmin** will detect syntax
errors and will not function satisfactorily.

The following diagnostics may be issued to the logfile ion.log:

- acsadmin can't attach to ION.

    There is no SDR data store for _acsadmin_ to use.  You should run ionadmin(1)
    first, to set up an SDR data store for ION.

- Can't open command file...

    The _commands\_filename_ specified in the command line doesn't exist.

Various errors that don't cause **acsadmin** to fail but are noted in the
**ion.log** log file may be caused by improperly formatted commands
given at the prompt or in the _commands\_filename_ file.
Please see acsrc(5) for details.

# BUGS

Report bugs to <ion-dtn-support@lists.sourceforge.net>

# SEE ALSO

ionadmin(1), bpadmin(1), acsrc(5)
