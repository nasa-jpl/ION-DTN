# NAME

amsshell - Asynchronous Message Service (AMS) test message sender (UNIX)

# SYNOPSIS

**amsshell** _unit\_name_ _role\_name_ _application\_name_ _authority\_name_ _\[{ p | s | q | a }\]_

# DESCRIPTION

**amsshell** is a message issuance program designed to test AMS functionality.

When **amsshell** is started, it registers as an application module in the
unit identified by _unit\_name_ of the venture identified by
_application\_name_ and _authority\_name_; the role in which it registers
must be indicated in _role\_name_.  A configuration server for the local
continuum and a registrar for the indicated unit of the indicated venture
(which may both be instantiated in a single **amsd** daemon task) must be
running in order for **amsshell** to run.

**amsshell** runs as two threads: a background thread that receives watches
for AMS configuration events (including shutdown), together with a foreground
thread that acquires operating parameters and message content in lines of
console input to control the issuance of messages.

The first character of each line of input from stdin to the **amsshell**
indicates the significance of that line:

- **=**

    Sets the name of the subject on which all messages are to be issued, until
    superseded by another "=" line.  The subject name must begin at the
    second character of this line.  Optionally, subject name may be followed
    by a single ' ' (space) character and then the text of the first message
    to be issued on this subject, which is to be issued immediately.

- **r**

    Sets the number of the role constraining the domain of message issuance.
    The role number must begin at the second character of this line.

- **c**

    Sets the number of the continuum constraining the domain of message issuance.
    The continuum number must begin at the second character of this line.

- **u**

    Sets the number of the unit constraining the domain of message issuance.
    The unit number must begin at the second character of this line.

- **m**

    Sets the number of the module to which subsequent messages are to be issued.
    The module number must begin at the second character of this line.

- **.**

    Terminates **amsshell**.

When the first character of a line of input from stdin is none of the
above, the entire line is taken to be the text of a message that is
to be issued immediately, on the previously specified subject, to the
previously specified module (if applicable), and subject to the previously
specified domain (if applicable).

By default, **amsshell** runs in "publish" mode: when a message is to be
issued, it is simply published.  This behavior can be overridden by
providing a fifth command-line argument to **amsshell** - a "mode"
indicator.  The supported modes are as follows:

- **p**

    This is "publish" mode.  Every message is published.

- **s**

    This is "send" mode.  Every message is sent privately to the application
    module identified by the specified module, unit, and continuum numbers.

- **q**

    This is "query" mode.  Every message is sent privately to the application
    module identified by the specified module, unit, and continuum numbers,
    and **amsshell** then waits for a reply message before continuing.

- **a**

    This is "announce" mode.  Every message is announced to all modules in
    the domain established by the previously specified role, unit, and
    continuum numbers.

# EXIT STATUS

- -1

    **amsshell** terminated with an error as noted in the ion.log file.

- "0"

    **amsshell** terminated normally.

# FILES

A MIB initialization file with the applicable default name (see amsrc(5))
must be present.

# ENVIRONMENT

No environment variables apply.

# DIAGNOSTICS

- amsshell can't register.

    **amsshell** failed to register, for reasons noted in the ion.log file.

- amsshell can't set event manager.

    **amsshell** failed to start its background thread, for reasons noted in
    the ion.log file.

# BUGS

Report bugs to <ion-dtn-support@lists.sourceforge.net>

# SEE ALSO

amslog(1), amsrc(5)
