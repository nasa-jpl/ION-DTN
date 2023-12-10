# NAME

imcadminep - administrative endpoint task for the IMC (multicast) scheme

# SYNOPSIS

**imcadminep**

# DESCRIPTION

**imcadminep** is a background "daemon" task that receives and processes
administrative bundles (multicast group petitions) that are sent to the
IMC-scheme administrative endpoint on the local ION node, if and only
if such an endpoint was established by **bpadmin**.  It is spawned
automatically by **bpadmin** in response to the 's' (START) command
that starts operation of Bundle Protocol on the local ION node, and
it is terminated by **bpadmin** in response to an 'x' (STOP) command.
**imcadminep** can also be spawned and terminated in response to
START and STOP commands that pertain specifically to the IMC scheme.

**imcadminep** responds to multicast group "join" and "leave" petitions
by managing entries in the node's database of multicast groups and
their members.

# EXIT STATUS

- "0"

    **imcadminep** terminated, for reasons noted in the **ion.log** file.  If this
    termination was not commanded, investigate and solve the problem identified
    in the log file and use **bpadmin** to restart **imcadminep**.

- "1"

    **imcadminep** was unable to attach to Bundle Protocol operations or was
    unable to load the IMC scheme database, probably because **bpadmin** has
    not yet been run.

# FILES

No configuration files are needed.

# ENVIRONMENT

No environment variables apply.

# DIAGNOSTICS

The following diagnostics may be issued to the **ion.log** log file:

- imcadminep can't attach to BP.

    **bpadmin** has not yet initialized BP operations.

- imcadminep can't load routing database.

    **imcadmin** has not yet initialized the IMC scheme.

- imcadminep crashed.

    An unrecoverable database error was encountered.  **imcadminep** terminates.

# BUGS

Report bugs to <ion-dtn-support@lists.sourceforge.net>

# SEE ALSO

bpadmin(1), bprc(5)
