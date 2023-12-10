# NAME

tcacompile - Trusted Collective daemon task for compiling critical information bulletins

# SYNOPSIS

**tcacompile** _blocks\_group\_number_

# DESCRIPTION

**tcacompile** is a background "daemon" task that periodically generates new
proposed bulletins of recent critical information records and multicasts those
bulletins to all nodes in the collective authority for the TC application
identified by _blocks\_group\_number_.  It is spawned automatically by
**tcaadmin** in response to the 's' command that starts operation of the
TC authority function for this application on the local node, and it is
terminated by **tcaadmin** in response to an 'x' (STOP) command.

# EXIT STATUS

- "0"

    **tcacompile** terminated, for reasons noted in the **ion.log** file.  If this
    termination was not commanded, investigate and solve the problem identified
    in the log file and use **tcaadmin** to restart **tcacompile**.

- "1"

    **tcacompile** was unable to attach to TC authority operations, probably because
    **tcaadmin** has not yet been run.

# FILES

No configuration files are needed.

# ENVIRONMENT

No environment variables apply.

# DIAGNOSTICS

The following diagnostics may be issued to the **ion.log** log file:

- tcacompile can't attach to tca system.

    **tcaadmin** has not yet initialized the authority database for this TC
    application.

# BUGS

Report bugs to <ion-dtn-support@lists.sourceforge.net>

# SEE ALSO

tcaadmin(1), tc(3), tcarc(5)
