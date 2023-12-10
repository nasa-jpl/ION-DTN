# NAME

ionadmin - ION node administration interface

# SYNOPSIS

**ionadmin** \[ _commands\_filename_ | . | ! \]

# DESCRIPTION

**ionadmin** configures, starts, manages, and stops the ION node on the local
computer.

It configures the node and sets (and reports on) global operational
settings for the DTN protocol stack on the local computer in response
to ION configuration commands found in _commands\_filename_, if provided;
if not, **ionadmin** prints a simple prompt (:) so that the user may type
commands directly into standard input.  If _commands\_filename_ is a
period (.), the effect is the same as if a command file containing
the single command 'x' were passed to **ionadmin** -- that is, the ION
node's _rfxclock_ task is stopped.  If _commands\_filename_ is an
exclamation point (!), that effect is reversed: the ION node's _rfxclock_
task is restarted.

The format of commands for _commands\_filename_ can be queried from **ionadmin**
by entering the command 'h' or '?' at the prompt.  The commands are documented
in ionrc(5).

Note that _ionadmin_ always computes a congestion forecast immediately
before exiting.  The result of this forecast -- maximum projected occupancy
of the DTN protocol traffic allocation in ION's SDR database -- is retained
for application flow control purposes: if maximum projected occupancy is the
entire protocol traffic allocation, then a message to this effect is logged
and no new bundle origination by any application will be accepted until
a subsequent forecast that predicts no congestion is computed.  (Congestion
forecasts are constrained by _horizon_ times, which can be established by
commands issued to _ionadmin_.  One way to re-enable data origination
temporarily while long-term traffic imbalances are being addressed is to
declare a congestion forecast horizon in the near future, before congestion
would occur if no adjustments were made.)

# EXIT STATUS

- "0"

    Successful completion of ION node administration.

# EXAMPLES

- ionadmin

    Enter interactive ION configuration command entry mode.

- ionadmin host1.ion

    Execute all configuration commands in _host1.ion_, then terminate immediately.

# FILES

Status and diagnostic messages from **ionadmin** and from other software that
utilizes the ION node are nominally written to a log file in the current
working directory within which **ionadmin** was run.  The log file is typically
named **ion.log**.

See also ionconfig(5) and ionrc(5).

# ENVIRONMENT

Environment variables ION\_NODE\_LIST\_DIR and ION\_NODE\_WDNAME can be used to
enable the operation of multiple ION nodes on a single workstation computer.
See section 2.1.3 of the ION Design and Operations Guide for details.

# DIAGNOSTICS

**Note**: all ION administration utilities expect source file input to be
lines of ASCII text that are NL-delimited.  If you edit the ionrc file on
a Windows machine, be sure to **use dos2unix to convert it to Unix text format**
before presenting it to **ionadmin**.  Otherwise **ionadmin** will detect syntax
errors and will not function satisfactorily.

The following diagnostics may be issued to the log file:

- Can't open command file...

    The _commands\_filename_ specified in the command line doesn't exist.

- ionadmin SDR definition failed.

    A node initialization command was executed, but an SDR database already
    exists for the indicated node.  It is likely that an ION node is already
    running on this computer or that destruction of a previously started the
    previous ION node was incomplete.  For most ION installations, incomplete
    node destruction can be repaired by (a) killing all ION processes that
    are still running and then (b) using **ipcrm** to remove all SVr4 IPC
    objects owned by ION.

- ionadmin can't get SDR parms.

    A node initialization command was executed, but the _ion\_config\_filename_
    passed to that command contains improperly formatted commands.  Please see
    ionconfig(5) for further details.

Various errors that don't cause **ionadmin** to fail but are noted in the
log file may be caused by improperly formatted commands given at the prompt
or in the _commands\_filename_.  Please see ionrc(5) for details.

# BUGS

If the _ion\_config\_filename_ parameter passed to a node initialization
command refers to a nonexistent filename, then **ionadmin** uses default
values are used rather than reporting an error in the command line argument.

Report bugs to <ion-dtn-support@lists.sourceforge.net>

# SEE ALSO

ionrc(5), ionconfig(5)
