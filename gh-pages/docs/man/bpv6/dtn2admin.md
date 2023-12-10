# NAME

dtn2admin - baseline "dtn" scheme administration interface

# SYNOPSIS

**dtn2admin** \[ _commands\_filename_ \]

# DESCRIPTION

**dtn2admin** configures the local ION node's routing of bundles to endpoints
whose IDs conform to the _dtn_ endpoint ID scheme.  _dtn_ is a
non-CBHE-conformant scheme.  The structure of _dtn_ endpoint IDs remains
somewhat in flux at the time of this writing, but endpoint IDs in the _dtn_
scheme historically have been strings of the form
"dtn://_node\_name_\[/_demux\_token_\]", where _node\_name_
normally identifies a computer somewhere on the network and _demux\_token_
normally identifies a specific application processing point.  Although
the _dtn_ endpoint ID scheme imposes more transmission overhead than the
_ipn_ scheme, ION provides support for _dtn_ endpoint IDs to enable
interoperation with other implementations of Bundle Protocol.

**dtn2admin** operates in response to "dtn" scheme configuration commands found
in the file _commands\_filename_, if provided; if not, **dtn2admin** prints
a simple prompt (:) so that the user may type commands
directly into standard input.

The format of commands for _commands\_filename_ can be queried from **dtn2admin**
with the 'h' or '?' commands at the prompt.  The commands are documented in
dtn2rc(5).

# EXIT STATUS

- "0"
Successful completion of "dtn" scheme administration.
- "1"
Unsuccessful completion of "dtn" scheme administration, due to inability to
attach to the Bundle Protocol system or to initialize the "dtn" scheme.

# EXAMPLES

- dtn2admin

    Enter interactive "dtn" scheme configuration command entry mode.

- dtn2admin host1.dtn2rc

    Execute all configuration commands in _host1.dtn2rc_, then terminate
    immediately.

# FILES

See dtn2rc(5) for details of the DTN scheme configuration commands.

# ENVIRONMENT

No environment variables apply.

# DIAGNOSTICS

**Note**: all ION administration utilities expect source file input to be
lines of ASCII text that are NL-delimited.  If you edit the dtn2rc file on
a Windows machine, be sure to **use dos2unix to convert it to Unix text format**
before presenting it to **dtn2admin**.  Otherwise **dtn2admin** will detect syntax
errors and will not function satisfactorily.

The following diagnostics may be issued to the logfile ion.log:

- dtn2admin can't attach to BP.

    Bundle Protocol has not been initialized on this computer.  You need to run
    bpadmin(1) first.

- dtn2admin can't initialize routing database.

    There is no SDR data store for _dtn2admin_ to use.  Please run ionadmin(1)
    to start the local ION node.

- Can't open command file...

    The _commands\_filename_ specified in the command line doesn't exist.

Various errors that don't cause **dtn2admin** to fail but are noted in the
**ion.log** log file may be caused by improperly formatted commands
given at the prompt or in the _commands\_filename_ file.
Please see dtn2rc(5) for details.

# BUGS

Report bugs to <ion-dtn-support@lists.sourceforge.net>

# SEE ALSO

dtn2rc(5)
