# NAME

amsstop - Asynchronous Message Service (AMS) message space shutdown utility

# SYNOPSIS

**amsstop** _application\_name_ _authority\_name_

# DESCRIPTION

**amsstop** is a utility program that terminates the operation of all
registrars and all application modules running in the message space which is
that portion of the indicated AMS venture that is operating in the local
continuum.  If one of the **amsd** tasks that are functioning as registrars
for this venture is also functioning as the configuration server for
the local continuum, then that configuration server is also terminated.

_application\_name_ and _authority\_name_ must identify an AMS venture that
is known to operate in the local continuum, as noted in the MIB for the
**amsstop** application module.

A message space can only be shut down by **amsstop** if the subject "amsstop"
is defined in the MIBs of all modules in the message spaces.

# EXIT STATUS

- "0"

    **amsstop** terminated normally.

- "1"

    An anomalous exit status, indicating that **amsstop** was unable to register
    and therefore failed to shut down its message space, for reasons noted in the
    ion.log file.

# FILES

A MIB initialization file with the applicable default name (see amsrc(5)
and amsxml(5)) must be present.

# ENVIRONMENT

No environment variables apply.

# DIAGNOSTICS

The following diagnostics may be issued to the **ion.log** log file:

- amsstop can't register.

    This message indicates that **amsstop** was unable to register, possibly
    because the "amsstop" role is not defined in the MIB initialization file.

- amsstop subject undefined.

    This message indicates that **amsstop** was unable to stop the message space
    because the "amsstop" subject is not defined in the MIB initialization file.

- amsstop can't publish 'amsstop' message.

    This message indicates that **amsstop** was unable to publish a message on
    subject 'amsstop' for reasons noted in the **ion.log** log file.

# BUGS

Report bugs to <ion-dtn-support@lists.sourceforge.net>

# SEE ALSO

amsrc(5)
