# NAME

dtn2adminep - administrative endpoint task for the "dtn" scheme

# SYNOPSIS

**dtn2adminep**

# DESCRIPTION

**dtn2adminep** is a background "daemon" task that receives and processes
administrative bundles (all custody signals and, nominally, all bundle
status reports) that are sent to the "dtn"-scheme administrative endpoint
on the local ION node, if and only if such an endpoint was established by
**bpadmin**.  It is spawned automatically by **bpadmin** in response to the
's' (START) command that starts operation of Bundle Protocol on the local
ION node, and it is terminated by **bpadmin** in response to an 'x' (STOP)
command.  **dtn2adminep** can also be spawned and terminated in response to
START and STOP commands that pertain specifically to the "dtn" scheme.

**dtn2adminep** responds to custody signals as specified in the Bundle
Protocol specification, RFC 5050.  It responds to bundle status reports
by logging ASCII text messages describing the reported activity.

# EXIT STATUS

- "0"

    **dtn2adminep** terminated, for reasons noted in the **ion.log** file.  If this
    termination was not commanded, investigate and solve the problem identified
    in the log file and use **bpadmin** to restart **dtn2adminep**.

- "1"

    **dtn2adminep** was unable to attach to Bundle Protocol operations or was
    unable to load the "dtn" scheme database, probably because **bpadmin** has
    not yet been run.

# FILES

No configuration files are needed.

# ENVIRONMENT

No environment variables apply.

# DIAGNOSTICS

The following diagnostics may be issued to the **ion.log** log file:

- dtn2adminep can't attach to BP.

    **bpadmin** has not yet initialized BP operations.

- dtn2adminep can't load routing database.

    **dtn2admin** has not yet initialized the "dtn" scheme.

- dtn2adminep can't get admin EID.

    **dtn2admin** has not yet initialized the "dtn" scheme.

- dtn2adminep crashed.

    An unrecoverable database error was encountered.  **dtn2adminep** terminates.

# BUGS

Report bugs to <ion-dtn-support@lists.sourceforge.net>

# SEE ALSO

bpadmin(1), dtn2admin(1).
