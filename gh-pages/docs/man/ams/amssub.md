# NAME

amssub - Asynchronous Message Service (AMS) test message receiver for VxWorks

# SYNOPSIS

**amssub** "_application\_name_", "_authority\_name_", "_subject\_name_"

# DESCRIPTION

**amssub** is a message reception program designed to test AMS functionality
in a VxWorks environment.  When an **amssub** task is started, it registers
as an application module in the root unit of the venture identified by
_application\_name_ and _authority\_name_, looks up the subject number for
_subject\_name_, subscribes to that subject, and begins receiving and
printing messages on that subject until terminated by **amsstop**.

A configuration server for the local continuum and a registrar for the root
unit of the indicated venture (which may both be instantiated in a single
**amsd** daemon task) must be running in order for **amssub** to run.

# EXIT STATUS

- -1

    **amssub** terminated with an error as noted in the ion.log file.

- "0"

    **amssub** terminated normally.

# FILES

The **amssub** source code is in the amspubsub.c source file.

A MIB initialization file with the applicable default name (see amsrc(5))
must be present.

# ENVIRONMENT

No environment variables apply.

# DIAGNOSTICS

- amssub can't register.

    **amssub** failed to register, for reasons noted in the ion.log file.

- amssub: subject is unknown

    **amssub** can't subscribe to messages on the specified subject; possibly
    an error in the MIB initialization file.

- amssub can't subscribe.

    **amssub** failed to subscribe, for reasons noted in the ion.log file.

- amssub can't get event.

    **amssub** failed to receive message, for reasons noted in the ion.log file.

# BUGS

Report bugs to <ion-dtn-support@lists.sourceforge.net>

# SEE ALSO

amspub(1), amsrc(5)
