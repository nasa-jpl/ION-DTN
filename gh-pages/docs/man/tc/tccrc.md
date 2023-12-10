# NAME

tccrc - Trusted Collective client configuration commands file

# DESCRIPTION

TC client configuration commands are passed to **tccadmin** either
in a file of text lines or interactively at **tccadmin**'s command prompt
(:).  Commands are interpreted line-by line, with exactly one command per
line.  The formats and effects of the TC client configuration commands
are described below.

# COMMANDS

- **?**

    The **help** command.  This will display a listing of the commands and their
    formats.  It is the same as the **h** command.

- **#**

    Comment line.  Lines beginning with **#** are not interpreted.

- **e** { 1 | 0 }

    Echo control.  Setting echo to 1 causes all output printed by tccadmin to be
    logged as well as sent to stdout.  Setting echo to 0 disables this behavior.

- **v**

    Version number.  Prints out the version of ION currently installed.  HINT:
    combine with**e 1** command to log the version number at startup.

- **h**

    The **help** command.  This will display a listing of the commands and their
    formats.  It is the same as the **h** command.

- **1** _number\_of\_authorities\_in\_collective_ \[ _K_ \[ _R_ \]\]

    The **initialize** command.  Until this command is executed, the client
    daemon for the selected TC application is not in operation on the local
    ION node and most _tccadmin_ commands will fail.

    _K_ is the mandated **diffusion** for the selected TC application, i.e.,
    the number of blocks into which each bulletin of published TC information
    is divided for transmission.

    _R_ is the mandated **redundancy** for the selected TC application, i.e.,
    the percentage of  blocks issued per bulletin that will be parity blocks
    rather than extents of the bulletin itself.

- **i**

    The **info** This command will print information about the current state
    of the client daemon for the selected TC application on the local node,
    i.e., the identities of the TC authority nodes that the client daemon
    recognizes.

- **s**

    The **start** command.  This command starts the client daemon (**tcc**)
    for the selected TC application.

- **m authority** _authority\_array\_index_ _node\_number_

    This command asserts that the Nth member of the collective authority for
    the selected TC application, where N is the _authority\_array\_index_ value,
    is the node identified by _node\_number_.

- **x**

    The **stop** command.  This command stops the client daemon (**tcc**) for
    the selected TC application.

- **h**

    The **help** command.  This will display a listing of the commands and their
    formats.  It is the same as the **?** command.

# EXAMPLES

- m authority 3 6913

    Asserts that node 6913 is member 3 of the collective authority for the
    selected application.

# SEE ALSO

tccadmin(1)
