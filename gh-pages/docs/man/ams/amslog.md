# NAME

amslog - Asynchronous Message Service (AMS) test message receiver

# SYNOPSIS

**amslog** _unit\_name_ _role\_name_ _application\_name_ _authority\_name_ _\[{ s | i }\]_

# DESCRIPTION

**amslog** is a message reception program designed to test AMS functionality.

When **amslog** is started, it registers as an application module in the
unit identified by _unit\_name_ of the venture identified by
_application\_name_ and _authority\_name_; the role in which it registers
must be indicated in _role\_name_.  A configuration server for the local
continuum and a registrar for the indicated unit of the indicated venture
(which may both be instantiated in a single **amsd** daemon task) must be
running in order for **amslog** to run.

**amslog** runs as two threads: a background thread that receives AMS messages
and logs them to standard output, together with a foreground thread that
acquires operating parameters in lines of console input to control the
flow of messages to the background thread.

When the first character of a line of input from stdin to the **amslog**
foreground thread is '.' (period), **amslog** immediately terminates.
Otherwise, the first character of each line of input from stdin must
be either '+' indicating assertion of interest in a message subject or
'-' indicating cessation of interest in a subject.  In each case, the
name of the subject in question must begin in the second character of
the input line.  Note that "everything" is a valid subject name.

By default, **amslog** runs in "subscribe" mode: when interest in a message
subject is asserted, **amslog** subscribes to that subject; when interest
in a message subject is rescinded, **amslog** unsubscribes to that subject.
This behavior can be overridden by providing a third command-line argument
to **amslog** - a "mode" indicator.  When mode is 'i', **amslog** runs in
"invite" mode.  In "invite" mode, when interest in a message subject is
asserted, **amslog** invites messages on that subject; when interest
in a message subject is rescinded, **amslog** cancels its invitation for
messages on that subject.

The "domain" of a subscription or invitation can optionally be specified
immediately after the subject name, on the same line of console input:

> Domain continuum name may be specified, or the place-holder domain
> continuum name "\_" may be specified to indicate "all continua".
>
> If domain continuum name ("\_" or otherwise) is specified, then domain unit
> name may be specified or the place-holder domain unit name "\_" may be
> specified to indicate "the root unit" (i.e., the entire venture).
>
> If domain unit name ("\_" or otherwise) is specified, then domain role
> name may be specified.

When **amslog** runs in VxWorks or RTEMS, the subject and content of each
message are simply written to standard output in a text line for display
on the console.  When **amslog** runs in a UNIX environment, the subject
name length (a binary integer), subject name (ASCII text), content length
(a binary integer), and content (ASCII text) are written to standard output
for redirection either to a file or to a pipe to **amslogprt**.

Whenever a received message is flagged as a Query, **amslog** returns a
reply message whose content is the string "Got " followed by the first
128 bytes of the content of the Query message, enclosed in single quote
marks and followed by a period.

# EXIT STATUS

- -1

    **amslog** terminated with an error as noted in the ion.log file.

- "0"

    **amslog** terminated normally.

# FILES

A MIB initialization file with the applicable default name (see amsrc(5))
must be present.

# ENVIRONMENT

No environment variables apply.

# DIAGNOSTICS

- amslog can't register.

    **amslog** failed to register, for reasons noted in the ion.log file.

- amslog can't set event manager.

    **amslog** failed to start its background thread, for reasons noted in
    the ion.log file.

- amslog can't read from stdin

    **amslog** foreground thread failed to read console input, for reasons
    noted in the ion.log file.

# BUGS

Report bugs to <ion-dtn-support@lists.sourceforge.net>

# SEE ALSO

amsshell(1), amslogprt(1), amsrc(5)
