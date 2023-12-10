# NAME

smrbtsh - shared-memory red-black tree test shell

# SYNOPSIS

**smrbtsh** \[_command\_file\_name_\]

# DESCRIPTION

**smrbtsh** allocates a region of shared system memory, attaches to that
region, places it under PSM management, creates a temporary "test" red-black
tree in that memory region, and executes a series of shared-memory red-black
tree commands that exercise various tree access and management functions.

If _command\_file\_name_ is provided, then the commands in the indicated file
are executed and the program then terminates.  Upon termination, the shared
memory region allocated to **smrbtsh** is detached and destroyed.

Otherwise, **smrbtsh** offers the user an interactive "shell" for testing the
smrbt functions in a conversational manner: **smrbtsh** prints a prompt string
(": ") to stdout, accepts a command from stdin, executes the command (possibly
printing a diagnostic message), then prints another prompt string and so on.
Upon execution of the 'q' command, the program terminates.

The following commands are supported:

- **h**

    The **help** command.  Causes **smrbtsh** to print a summary of available
    commands.  Same effect as the **?** command.

- **?**

    Another **help** command.  Causes **smrbtsh** to print a summary of available
    commands.  Same effect as the **h** command.

- **s** \[_seed\_value_\]

    The **seed** command.  Seeds random data value generator, which is used to
    generate node values when the **r** command is used.  If _seed\_value_ is
    omitted, uses current time (as returned by time(1)) as seed value.

- **r** \[_count_\]

    The **random** command.  Inserts _count_ new nodes into the red-black tree,
    using randomly selected unsigned long integers as the data values of the
    nodes; _count_ defaults to 1 if omitted.

- **i** _data\_value_

    The **insert** command.  Inserts a single new node into the red-black tree,
    using _data\_value_ as the data value of the node.

- **f** _data\_value_

    The **find** command.  Finds the rbt node whose value is _data\_value_,
    within the red-black tree, and prints the address of that node.  If the 
    node is not found, prints address zero and prints the address of the
    successor node in the tree.

- **d** _data\_value_

    The **delete** command.  Deletes the rbt node whose data value is
    _data\_value_.

- **p**

    The **print** command.  Prints the red-black tree, using indentation to
    indicate descent along paths of the tree.

    Note: this function is supported only if the **smrbt** library was built
    with compilation flag -DSMRBT\_DEBUG=1.

- **k**

    The **check** command.  Examines the red-black tree, noting the first
    violation of red-black structure rules, if any.

    Note: this function is supported only if the **smrbt** library was built
    with compilation flag -DSMRBT\_DEBUG=1.

- **l**

    The **list** command.  Lists all nodes in the red-black tree in traversal
    order, noting any nodes whose data values are not in ascending numerical
    order.

- **q**

    The **quit** command.  Detaches **smrbtsh** from the region of shared
    memory it is currently using, destroys that shared memory region, and
    terminates **smrbtsh**.

# EXIT STATUS

- "0"

    **smrbtsh** has terminated.

# FILES

No configuration files are needed.

# ENVIRONMENT

No environment variables apply.

# DIAGNOSTICS

No diagnostics apply.

# BUGS

Report bugs to <ion-dtn-support@lists.sourceforge.net>

# SEE ALSO

smrbt(3)
