# NAME

cfdpclock - CFDP daemon task for managing scheduled events

# SYNOPSIS

**cfdpclock**

# DESCRIPTION

**cfdpclock** is a background "daemon" task that periodically performs
scheduled CFDP activities.  It is spawned automatically by **cfdpadmin** in
response to the 's' command that starts operation of the CFDP protocol, and
it is terminated by **cfdpadmin** in response to an 'x' (STOP) command.

Once per second, **cfdpclock** takes the following action:

> First it scans all inbound file delivery units (FDUs).  For each one
> whose check timeout deadline has passed, it increments the check timeout
> count and resets the check timeout deadline.  For each one whose check
> timeout count exceeds the limit configured for this node, it invokes
> the Check Limit Reached fault handling procedure.
>
> Then it scans all outbound FDUs.  For each one that has been Canceled, it
> cancels all extant PDU bundles and sets transmission progress to the size
> of the file, simulating the completion of transmission.  It destroys each
> outbound FDU whose transmission is completed.

# EXIT STATUS

- "0"

    **cfdpclock** terminated, for reasons noted in the **ion.log** file.  If this
    termination was not commanded, investigate and solve the problem identified
    in the log file and use **cfdpadmin** to restart **cfdpclock**.

- "1"

    **cfdpclock** was unable to attach to CFDP protocol operations, probably because
    **cfdpadmin** has not yet been run.

# FILES

No configuration files are needed.

# ENVIRONMENT

No environment variables apply.

# DIAGNOSTICS

The following diagnostics may be issued to the **ion.log** log file:

- cfdpclock can't initialize CFDP.

    **cfdpadmin** has not yet initialized CFDP protocol operations.

- Can't dispatch events.

    An unrecoverable database error was encountered.  **cfdpclock** terminates.

- Can't manage links.

    An unrecoverable database error was encountered.  **cfdpclock** terminates.

# BUGS

Report bugs to <ion-dtn-support@lists.sourceforge.net>

# SEE ALSO

cfdpadmin(1)
