# NAME

tcarecv - Trusted Collective daemon task for receiving newly generated records of critical information

# SYNOPSIS

**tcarecv** _blocks\_group\_number_

# DESCRIPTION

**tcarecv** is a background "daemon" task that receives new critical
information records multicast by user nodes of the TC application identified
by _blocks\_group\_number_.  It records those assertions in a database for
future processing by **tcacompile**.  It is spawned automatically by
**tcaadmin** in response to the 's' command that starts operation of
the TC authority function for this application on the local node, and
it is terminated by **tcaadmin** in response to an 'x' (STOP) command.

# EXIT STATUS

- "0"

    **tcarecv** terminated, for reasons noted in the **ion.log** file.  If this
    termination was not commanded, investigate and solve the problem identified
    in the log file and use **tcaadmin** to restart **tcarecv**.

- "1"

    **tcarecv** was unable to attach to DTKA operations, probably because
    **tcaadmin** has not yet been run.

# FILES

No configuration files are needed.

# ENVIRONMENT

No environment variables apply.

# DIAGNOSTICS

The following diagnostics may be issued to the **ion.log** log file:

- tcarecv can't attach to DTKA.

    **tcaadmin** has not yet initialized the authority database for this TC
    application.

# BUGS

Report bugs to <ion-dtn-support@lists.sourceforge.net>

# SEE ALSO

tcaadmin(1), tc(3), tcarc(5)
