# NAME

cfdpadmin - ION's CCSDS File Delivery Protocol (CFDP) administration interface

# SYNOPSIS

**cfdpadmin** \[ _commands\_filename_ | . | ! \]

# DESCRIPTION

**cfdpadmin** configures, starts, manages, and stops CFDP operations for
the local ION node.

It operates in response to CFDP configuration commands found in the file
_commands\_filename_, if provided; if not, **cfdpadmin** prints
a simple prompt (:) so that the user may type commands
directly into standard input.  If _commands\_filename_ is a period (.), the
effect is the same as if a command file containing the single command 'x'
were passed to **cfdpadmin** -- that is, the ION node's _cfdpclock_ task 
and UT layer service task (nominally _bputa_) are stopped.
If _commands\_filename_ is an exclamation point (!), that effect is
reversed: the ION node's _cfdpclock_ task and UT layer service task
(nominally _bputa_) are restarted.

The format of commands for _commands\_filename_ can be queried from **cfdpadmin**
with the 'h' or '?' commands at the prompt.  The commands are documented in
cfdprc(5).

# EXIT STATUS

- "0"

    Successful completion of CFDP administration.

# EXAMPLES

- cfdpadmin

    Enter interactive CFDP configuration command entry mode.

- cfdpadmin host1.cfdprc

    Execute all configuration commands in _host1.cfdprc_, then terminate
    immediately.

- cfdpadmin .

    Stop all CFDP operations on the local node.

# FILES

See cfdprc(5) for details of the CFDP configuration commands.

# ENVIRONMENT

No environment variables apply.

# DIAGNOSTICS

**Note**: all ION administration utilities expect source file input to be
lines of ASCII text that are NL-delimited.  If you edit the cfdprc file on
a Windows machine, be sure to **use dos2unix to convert it to Unix text format**
before presenting it to **cfdpadmin**.  Otherwise **cfdpadmin** will detect syntax
errors and will not function satisfactorily.

The following diagnostics may be issued to the logfile ion.log:

- cfdpadmin can't attach to ION.

    There is no SDR data store for _cfdpadmin_ to use.  You should run ionadmin(1)
    first, to set up an SDR data store for ION.

- Can't open command file...

    The _commands\_filename_ specified in the command line doesn't exist.

Various errors that don't cause **cfdpadmin** to fail but are noted in the
**ion.log** log file may be caused by improperly formatted commands
given at the prompt or in the _commands\_filename_ file.
Please see cfdprc(5) for details.

# BUGS

Report bugs to <ion-dtn-support@lists.sourceforge.net>

# SEE ALSO

cfdprc(5)
