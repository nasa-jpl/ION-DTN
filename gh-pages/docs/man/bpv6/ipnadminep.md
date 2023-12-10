# NAME

ipnadminep - administrative endpoint task for the IPN scheme

# SYNOPSIS

**ipnadminep**

# DESCRIPTION

**ipnadminep** is a background "daemon" task that receives and processes
administrative bundles (all custody signals and, nominally, all bundle
status reports) that are sent to the IPN-scheme administrative endpoint
on the local ION node, if and only if such an endpoint was established by
**bpadmin**.  It is spawned automatically by **bpadmin** in response to the
's' (START) command that starts operation of Bundle Protocol on the local
ION node, and it is terminated by **bpadmin** in response to an 'x' (STOP)
command.  **ipnadminep** can also be spawned and terminated in response to
START and STOP commands that pertain specifically to the IPN scheme.

**ipnadminep** responds to custody signals as specified in the Bundle
Protocol specification, RFC 5050.  It responds to bundle status reports
by logging ASCII text messages describing the reported activity.

# EXIT STATUS

- "0"

    **ipnadminep** terminated, for reasons noted in the **ion.log** file.  If this
    termination was not commanded, investigate and solve the problem identified
    in the log file and use **bpadmin** to restart **ipnadminep**.

- "1"

    **ipnadminep** was unable to attach to Bundle Protocol operations or was
    unable to load the IPN scheme database, probably because **bpadmin** has
    not yet been run.

# FILES

No configuration files are needed.

# ENVIRONMENT

No environment variables apply.

# DIAGNOSTICS

The following diagnostics may be issued to the **ion.log** log file:

- ipnadminep can't attach to BP.

    **bpadmin** has not yet initialized BP operations.

- ipnadminep can't load routing database.

    **ipnadmin** has not yet initialized the IPN scheme.

- ipnadminep crashed.

    An unrecoverable database error was encountered.  **ipnadminep** terminates.

# BUGS

Report bugs to <ion-dtn-support@lists.sourceforge.net>

# SEE ALSO

bpadmin(1), ipnadmin(1), bprc(5).
