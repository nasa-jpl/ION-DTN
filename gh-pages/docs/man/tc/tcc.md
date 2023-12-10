# NAME

tcc - Trusted Collective client daemon task for handling bulletins from a collective authority

# SYNOPSIS

**tcc** _blocks\_group\_number_

# DESCRIPTION

**tcc** is a background "daemon" task that receives code blocks multicast
by the authority nodes of the collective authority for the TC application
identified by _blocks\_group\_number_.  It reassembles bulletins from
compatible code blocks and delivers those bulletins to the application's
user function on the local node.

# EXIT STATUS

- "0"

    **tcc** terminated, for reasons noted in the **ion.log** file.

- "1"

    **tcc** was unable to attach to TC client operations, possibly because
    the TC client database for this application has not yet been initialized by
    **tcaadmin**.

# FILES

No configuration files are needed.

# ENVIRONMENT

No environment variables apply.

# DIAGNOSTICS

The following diagnostics may be issued to the **ion.log** log file:

- tcc can't attach to tcc system.

    **tcaadmin** has not yet initialized the TC client database for this application.

# BUGS

Report bugs to <ion-dtn-support@lists.sourceforge.net>

# SEE ALSO

tcaadmin(1), tcarc(5)
