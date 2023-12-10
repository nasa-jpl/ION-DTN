# NAME

bsspadmin - Bundle Streaming Service Protocol (BSSP) administration interface

# SYNOPSIS

**bsspadmin** \[ _commands\_filename_ | . \]

# DESCRIPTION

**bsspadmin** configures, starts, manages, and stops BSSP operations for
the local ION node.

It operates in response to BSSP configuration commands found in the file
_commands\_filename_, if provided; if not, **bsspadmin** prints
a simple prompt (:) so that the user may type commands
directly into standard input.  If _commands\_filename_ is a period (.), the
effect is the same as if a command file containing the single command 'x'
were passed to **bsspadmin** -- that is, the ION node's _bsspclock_ task
and link service adapter tasks are stopped.

The format of commands for _commands\_filename_ can be queried from **bsspadmin**
with the 'h' or '?' commands at the prompt.  The commands are documented in
bssprc(5).

# EXIT STATUS

- `0`

    Successful completion of BSSP administration.

# EXAMPLES

- bsspadmin

    Enter interactive BSSP configuration command entry mode.

- bsspadmin host1.bssp

    Execute all configuration commands in _host1.bssp_, then terminate immediately.

- bsspadmin .

    Stop all BSSP operations on the local node.

# FILES

See bssprc(5) for details of the BSSP configuration commands.

# ENVIRONMENT

No environment variables apply.

# DIAGNOSTICS

**Note**: all ION administration utilities expect source file input to be
lines of ASCII text that are NL-delimited.  If you edit the bssprc file on
a Windows machine, be sure to **use dos2unix to convert it to Unix text format**
before presenting it to **bsspadmin**.  Otherwise **bsspadmin** will detect syntax
errors and will not function satisfactorily.

The following diagnostics may be issued to the logfile ion.log:

- bsspadmin can't attach to ION.

    There is no SDR data store for _bsspadmin_ to use.  You should run ionadmin(1)
    first, to set up an SDR data store for ION.

- Can't open command file...

    The _commands\_filename_ specified in the command line doesn't exist.

Various errors that don't cause **bsspadmin** to fail but are noted in the
**ion.log** log file may be caused by improperly formatted commands
given at the prompt or in the _commands\_filename_ file.
Please see bssprc(5) for details.

# BUGS

Report bugs to <ion-dtn-support@lists.sourceforge.net>

# SEE ALSO

bssprc(5)
