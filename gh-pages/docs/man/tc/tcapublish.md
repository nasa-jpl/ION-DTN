# NAME

tcapublish - Trusted Collective authority task that publishes consensus critical information bulletins

# SYNOPSIS

**tcapublish** _blocks\_group\_number_

# DESCRIPTION

**tcapublish** is a background task that completes the processing of a single
iteration of the bulletin publication cycle for the collective authority
function of the TC application identified by _blocks\_group\_number_ on the
local node.  To do so, it receives proposed bulletins multicast by
**tcacompile** daemons, resolves differences among the received bulletins
to arrive at a consensus bulletin, computes a hash for the consensus bulletin,
erasure-codes the consensus bulletin, and multicasts that subset of the
resulting code blocks that is allocated to the local node according
to the local node's assigned position in the authority array of the
application's collective authority.  It is spawned automatically by the
local node's **tcacompile** daemon for the indicated application, at the time
that daemon publishes its own proposed bulletin for this iteration of the
bulletin compilation cycle; it terminates immediately after it has finished
publishing code blocks.

# EXIT STATUS

- "0"

    **tcapublish** terminated, for reasons noted in the **ion.log** file.

- "1"

    **tcapublish** was unable to attach to TC authority operations, probably because
    **tcaadmin** has not yet been run.

# FILES

No configuration files are needed.

# ENVIRONMENT

No environment variables apply.

# DIAGNOSTICS

The following diagnostics may be issued to the **ion.log** log file:

- tcapublish can't attach to DTKA.

    **tcaadmin** has not yet initialized the authority database for this TC
    application.

# BUGS

Report bugs to <ion-dtn-support@lists.sourceforge.net>

# SEE ALSO

tcaadmin(1), tc(3), tcauthrc(5)
