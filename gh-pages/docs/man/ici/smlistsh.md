# NAME

smlistsh - shared-memory linked list test shell

# SYNOPSIS

**smlistsh** _partition\_size_

# DESCRIPTION

**smlistsh** attaches to a region of system memory (allocating it if
necessary, and placing it under PSM management as necessary) and offers
the user an interactive "shell" for testing various shared-memory linked
list management functions.

**smlistsh** prints a prompt string (": ") to stdout, accepts a command from 
stdin, executes the command (possibly printing a diagnostic message), 
then prints another prompt string and so on.

The following commands are supported:

- **h**

    The **help** command.  Causes **smlistsh** to print a summary of available
    commands.  Same effect as the **?** command.

- **?**

    Another **help** command.  Causes **smlistsh** to print a summary of available
    commands.  Same effect as the **h** command.

- **k**

    The **key** command.  Computes and prints an unused shared-memory key, for
    possible use in attaching to a shared-memory region.

- **+** _key\_value_ _size_

    The **attach** command.  Attaches **smlistsh** to a region of shared memory.
    _key\_value_ identifies an existing shared-memory region, in the event
    that you want to attach to an existing shared-memory region (possibly created
    by another **smlistsh** process running on the same computer).  To create and
    attach to a new shared-memory region that other processes can attach to,
    use a _key\_value_ as returned by the **key** command and supply the _size_
    of the new region.  If you want to create and attach to a new shared-memory
    region that is for strictly private use, use -1 as key and supply the _size_
    of the new region.

- **-**

    The **detach** command.  Detaches **smlistsh** from the region of shared
    memory it is currently using, but does not free any memory.

- **n**

    The **new** command.  Creates a new shared-memory list to operate on, within
    the currently attached shared-memory region.  Prints the address of the list.

- **s** _list\_address_

    The **share** command.  Selects an existing shared-memory list to operate on,
    within the currently attached shared-memory region.

- **a** _element\_value_

    The **append** command.  Appends a new list element, containing
    _element\_value_, to the list on which **smlistsh** is currently operating.

- **p** _element\_value_

    The **prepend** command.  Prepends a new list element, containing
    _element\_value_, to the list on which **smlistsh** is currently operating.

- **w**

    The **walk** command.  Prints the addresses and contents of all elements of
    the list on which **smlistsh** is currently operating.

- **f** _element\_value_

    The **find** command.  Finds the list element that contains _element\_value_,
    within the list on which **smlistsh** is currently operating, and prints
    the address of that list element.

- **d** _element\_address_

    The **delete** command.  Deletes the list element located at _element\_address_.

- **r**

    The **report** command.  Prints a partition usage report, as per psm\_report(3).

- **q**

    The **quit** command.  Detaches **smlistsh** from the region of shared
    memory it is currently using (without freeing any memory) and terminates
    **smlistsh**.

# EXIT STATUS

- "0"

    **smlistsh** has terminated.

# FILES

No configuration files are needed.

# ENVIRONMENT

No environment variables apply.

# DIAGNOSTICS

No diagnostics apply.

# BUGS

Report bugs to <ion-dtn-support@lists.sourceforge.net>

# SEE ALSO

smlist(3)
