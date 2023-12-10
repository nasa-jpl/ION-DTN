# NAME

bpsource - Bundle Protocol transmission test shell

# SYNOPSIS

**bpsource** _destinationEndpointId_ \["_text_"\] \[-t_TTL_\]

# DESCRIPTION

When _text_ is supplied, **bpsource** simply uses Bundle Protocol
to send _text_ to a counterpart **bpsink** application task that
has opened the BP endpoint identified by _destinationEndpointId_, then
terminates.

Otherwise, **bpsource** offers the user an interactive "shell" for testing
Bundle Protocol data transmission.  **bpsource** prints a prompt string (": ")
to stdout, accepts a string of text from stdin, uses Bundle Protocol
to send the string to a counterpart **bpsink** application task that
has opened the BP endpoint identified by _destinationEndpointId_, then
prints another prompt string and so on.  To terminate the program, enter
a string consisting of a single exclamation point (!) character.

_TTL_ indicates the number of seconds the bundles may remain in the
network, undelivered, before they are automatically destroyed. If omitted, _TTL_
defaults to 300 seconds.

The source endpoint ID for each bundle sent by **bpsource** is the null
endpoint ID, i.e., the bundles are anonymous.  All bundles are sent standard
priority with no custody transfer and no status reports requested.

# EXIT STATUS

- "0"

    **bpsource** has terminated.  Any problems encountered during operation
    will be noted in the **ion.log** log file.

# FILES

The service data units transmitted by **bpsource** are sequences of text
obtained from a file in the current working directory named "bpsourceAduFile",
which **bpsource** creates automatically.

# ENVIRONMENT

No environment variables apply.

# DIAGNOSTICS

Diagnostic messages produced by **bpsource** are written to the ION log
file _ion.log_.

- Can't attach to BP.

    **bpadmin** has not yet initialized Bundle Protocol operations.

- bpsource fgets failed

    Operating system error.  Check errtext, correct problem, and rerun.

- No space for ZCO extent.

    ION system error.  Check for earlier diagnostic messages describing
    the cause of the error; correct problem and rerun.

- Can't create ZCO extent.

    ION system error.  Check for earlier diagnostic messages describing
    the cause of the error; correct problem and rerun.

- bpsource can't send ADU

    Bundle Protocol service to the remote endpoint has been stopped.

# BUGS

Report bugs to <ion-dtn-support@lists.sourceforge.net>

# SEE ALSO

bpadmin(1), bpsink(1), bp(3)
