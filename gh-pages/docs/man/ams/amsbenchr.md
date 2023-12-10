# NAME

amsbenchr - Asynchronous Message Service (AMS) benchmarking meter

# SYNOPSIS

**amsbenchr**

# DESCRIPTION

**amsbenchr** is a test program that simply subscribes to subject "bench"
and receives messages published by **amsbenchs** until all messages in the
test - as indicated by the count of remaining messages, in in the first
four bytes of each message - have been received.  Then it stops receiving
messages, calculates and prints performance statistics, and terminates.

**amsbenchr** will register as an application module in the root unit of
the venture identified by application name "amsdemo" and authority name
"test".  A configuration server for the local continuum and a registrar
for the root unit of that venture (which may both be instantiated in a
single **amsd** daemon task) must be running in order for **amsbenchr** to
commence operations.

# EXIT STATUS

- -1

    **amsbenchr** failed, for reasons noted in the ion.log file.

- "0"

    **amsbenchr** terminated normally.

# FILES

A MIB initialization file with the applicable default name (see amsrc(5))
must be present.

# ENVIRONMENT

No environment variables apply.

# DIAGNOSTICS

The following diagnostics may be issued to the **ion.log** log file:

- amsbenchr can't register.

    **amsbenchr** failed to register, for reasons noted in the ion.log file.

- amsbenchr: subject 'bench' is unknown.

    **amsbenchr** can't subscribe to test messages; probably an error in the MIB
    initialization file.

- amsbenchr can't subscribe.

    **amsbenchr** failed to subscribe, for reasons noted in the ion.log file.

- amsbenchr can't get event.

    **amsbenchr** failed to receive a message, for reasons noted in the ion.log file.

# BUGS

Report bugs to <ion-dtn-support@lists.sourceforge.net>

# SEE ALSO

amsrc(5)
