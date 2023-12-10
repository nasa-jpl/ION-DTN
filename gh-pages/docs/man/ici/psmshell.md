# NAME

psmshell - PSM memory management test shell

# SYNOPSIS

**psmshell** _partition\_size_

# DESCRIPTION

**psmshell** allocates a region of _partition\_size_ bytes of system memory,
places it under PSM management, and offers the user an interactive "shell"
for testing various PSM management functions.

**psmshell** prints a prompt string (": ") to stdout, accepts a command from 
stdin, executes the command (possibly printing a diagnostic message), 
then prints another prompt string and so on.

The locations of objects allocated from the PSM-managed region of memory
are referred to as "cells" in psmshell operations.  That is, when an object
is to be allocated, a cell number in the range 0-99 must be specified as
the notional "handle" for that object, for use in future commands.

The following commands are supported:

- **h**

    The **help** command.  Causes **psmshell** to print a summary of available
    commands.  Same effect as the **?** command.

- **?**

    Another **help** command.  Causes **psmshell** to print a summary of available
    commands.  Same effect as the **h** command.

- **m** _cell\_nbr_ _size_

    The **malloc** command.  Allocates a large-pool object of the indicated size and
    associates that object with _cell\_nbr_.

- **z** _cell\_nbr_ _size_

    The **zalloc** command.  Allocates a small-pool object of the indicated size and
    associates that object with _cell\_nbr_.

- **p** _cell\_nbr_

    The **print** command.  Prints the address (i.e., the offset within the
    managed block of memory) of the object associated with _cell\_nbr_.

- **f** _cell\_nbr_

    The **free** command.  Frees the object associated with _cell\_nbr_, returning
    the space formerly occupied by that object to the appropriate free block list.

- **u**

    The **usage** command.  Prints a partition usage report, as per psm\_report(3).

- **q**

    The **quit** command.  Frees the allocated system memory in the managed
    block and terminates **psmshell**.

# EXIT STATUS

- "0"

    **psmshell** has terminated.

# FILES

No configuration files are needed.

# ENVIRONMENT

No environment variables apply.

# DIAGNOSTICS

- IPC initialization failed.

    ION system error.  Investigate, correct problem, and try again.

- psmshell: can't allocate space; quitting.

    Insufficient available system memory for selected partition size.

- psmshell: can't allocate test variables; quitting.

    Insufficient available system memory for selected partition size.

# BUGS

Report bugs to <ion-dtn-support@lists.sourceforge.net>

# SEE ALSO

psm(3)
