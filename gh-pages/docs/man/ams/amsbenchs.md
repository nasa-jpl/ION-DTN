# NAME

amsbenchs - Asynchronous Message Service (AMS) benchmarking driver

# SYNOPSIS

**amsbenchs** _count_ _size_

# DESCRIPTION

**amsbenchs** is a test program that simply publishes _count_ messages of
_size_ bytes each on subject "bench", then waits while all published
messages are transmitted, terminating when the user uses ^C to interrupt
the program.  The remaining number of messages to be published in the test
is written into the first four octets of each message.

**amsbenchs** will register as an application module in the root unit of
the venture identified by application name "amsdemo" and authority name
"test".  A configuration server for the local continuum and a registrar
for the root unit of that venture (which may both be instantiated in a
single **amsd** daemon task) must be running in order for **amsbenchs** to
commence operations.

# EXIT STATUS

- -1

    **amsbenchs** failed, for reasons noted in the ion.log file.

- "0"

    **amsbenchs** terminated normally.

# FILES

A MIB initialization file with the applicable default name (see amsrc(5))
must be present.

# ENVIRONMENT

No environment variables apply.

# DIAGNOSTICS

The following diagnostics may be issued to the **ion.log** log file:

- No memory for amsbenchs.

    Insufficient available memory for a message content buffer of the
    indicated size.

- amsbenchs can't register.

    **amsbenchs** failed to register, for reasons noted in the ion.log file.

- amsbenchs can't set event manager.

    **amsbenchs** failed to start its background event management thread, for
    reasons noted in the ion.log file.

- amsbenchs: subject 'bench' is unknown.

    **amsbenchs** can't publish test messages; probably an error in the MIB
    initialization file.

- amsbenchs can't publish message.

    **amsbenchs** failed to publish, for reasons noted in the ion.log file.

# BUGS

Report bugs to <ion-dtn-support@lists.sourceforge.net>

# SEE ALSO

amsrc(5)
