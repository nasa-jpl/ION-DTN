# NAME

amspub - Asynchronous Message Service (AMS) test driver for VxWorks

# SYNOPSIS

**amspub** "_application\_name_", "_authority\_name_", "_subject\_name_", "_message\_text_"

# DESCRIPTION

**amspub** is a message publication program designed to test AMS functionality
in a VxWorks environment.  When an **amspub** task is started, it registers
as an application module in the root unit of the venture identified by
_application\_name_ and _authority\_name_, looks up the subject number for
_subject\_name_, publishes a single message with content _message\_text_ on
that subject, unregisters, and terminates.

A configuration server for the local continuum and a registrar for the root
unit of the indicated venture (which may both be instantiated in a single
**amsd** daemon task) must be running in order for **amspub** to run.

# EXIT STATUS

- -1

    **amspub** terminated with an error as noted in the ion.log file.

- "0"

    **amspub** terminated normally.

# FILES

The **amspub** source code is in the amspubsub.c source file.

A MIB initialization file with the applicable default name (see amsrc(5))
must be present.

# ENVIRONMENT

No environment variables apply.

# DIAGNOSTICS

- amspub can't register.

    **amspub** failed to register, for reasons noted in the ion.log file.

- amspub: subject is unknown

    **amspub** can't publish test messages on the specified subject; possibly
    an error in the MIB initialization file.

- amspub can't publish message.

    **amspub** failed to publish, for reasons noted in the ion.log file.

# BUGS

Report bugs to <ion-dtn-support@lists.sourceforge.net>

# SEE ALSO

amssub(1), amsrc(5)
