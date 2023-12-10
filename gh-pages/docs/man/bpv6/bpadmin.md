# NAME

bpadmin - ION Bundle Protocol (BP) administration interface

# SYNOPSIS

**bpadmin** \[ _commands\_filename_ | . | ! \]

# DESCRIPTION

**bpadmin** configures, starts, manages, and stops bundle protocol operations
for the local ION node.

It operates in response to BP configuration commands found in the file
_commands\_filename_, if provided; if not, **bpadmin** prints
a simple prompt (:) so that the user may type commands
directly into standard input.  If _commands\_filename_ is a period (.), the
effect is the same as if a command file containing the single command 'x'
were passed to **bpadmin** -- that is, the ION node's _bpclock_ task, 
forwarder tasks, and convergence layer adapter tasks are stopped.
If _commands\_filename_ is an exclamation point (!), that effect is
reversed: the ION node's _bpclock_ task, forwarder tasks, and convergence
layer adapter tasks are restarted.

The format of commands for _commands\_filename_ can be queried from **bpadmin**
with the 'h' or '?' commands at the prompt.  The commands are documented in
bprc(5).

# EXIT STATUS

- "0"
Successful completion of BP administration.

# EXAMPLES

- bpadmin

    Enter interactive BP configuration command entry mode.

- bpadmin host1.bp

    Execute all configuration commands in _host1.bp_, then terminate immediately.

- bpadmin .

    Stop all bundle protocol operations on the local node.

# FILES

See bprc(5) for details of the BP configuration commands.

# ENVIRONMENT

No environment variables apply.

# DIAGNOSTICS

**Note**: all ION administration utilities expect source file input to be
lines of ASCII text that are NL-delimited.  If you edit the bprc file on
a Windows machine, be sure to **use dos2unix to convert it to Unix text format**
before presenting it to **bpadmin**.  Otherwise **bpadmin** will detect syntax
errors and will not function satisfactorily.

The following diagnostics may be issued to the logfile ion.log:

- ION can't set custodian EID information.

    The _custodial\_endpoint\_id_ specified in the BP initialization ('1')
    command is malformed.  Remember that the format for this argument is 
    ipn:_element\_number_.0 and that the final 0 is required, as 
    custodial/administration service is always service 0.  Additional detail
    for this error is provided if one of the following other errors is present:

    >     Malformed EID.
    >
    >     Malformed custodian EID.

- bpadmin can't attach to ION.

    There is no SDR data store for _bpadmin_ to use.  You should run ionadmin(1)
    first, to set up an SDR data store for ION.

- Can't open command file...

    The _commands\_filename_ specified in the command line doesn't exist.

Various errors that don't cause **bpadmin** to fail but are noted in the
**ion.log** log file may be caused by improperly formatted commands
given at the prompt or in the _commands\_filename_ file.
Please see bprc(5) for details.

# BUGS

Report bugs to <ion-dtn-support@lists.sourceforge.net>

# SEE ALSO

ionadmin(1), bprc(5), ipnadmin(1), ipnrc(5), dtnadmin(1), dtnrc(5)
