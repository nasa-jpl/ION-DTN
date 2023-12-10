# NAME

tcarc - Trusted Collective authority configuration commands file

# DESCRIPTION

TC authority configuration commands are passed to **tcaadmin** either
in a file of text lines or interactively at **tcaadmin**'s command prompt
(:).  Commands are interpreted line-by line, with exactly one command per
line.  The formats and effects of the TC authority configuration
commands are described below.

# COMMANDS

- **?**

    The **help** command.  This will display a listing of the commands and their
    formats.  It is the same as the **h** command.

- **#**

    Comment line.  Lines beginning with **#** are not interpreted.

- **e** { 1 | 0 }

    Echo control.  Setting echo to 1 causes all output printed by tcaadmin to be
    logged as well as sent to stdout.  Setting echo to 0 disables this behavior.

- **v** 

    Version number.  Prints out the version of ION currently installed.  HINT:
    combine with **e 1** command to log the version number at startup.

- **h**

    The **help** command.  This will display a listing of the commands and their
    formats.  It is the same as the **?** command.

- **1** _multicast\_group\_number\_for\_TC\_bulletins_ _multicast\_group\_number\_for\_TC\_records_ _number\_of\_authorities\_in\_collective_ _K_ _R_

    The **initialize** command.  Until this command is executed, the authority
    function for the selected TC application is not in operation on the local
    ION node and most _tcaadmin_ commands will fail.

    _K_ is the mandated **diffusion** for the selected TC application, i.e.,
    the number of blocks into which each bulletin of published TC information
    is divided for transmission.

    _R_ is the mandated **redundancy** for the selected TC application, i.e.,
    the percentage of blocks issued per bulletin that will be parity blocks
    rather than extents of the bulletin itself.

- **i**

    The **info** This command will print information about the current state
    of the authority function for the selected TC application on the local
    node, including the current settings of all parameters that can be
    managed as described below.

- **s**

    The **start** command.  This command starts the **tcarecv** and **tcacompile**
    daemons of the authority function for the selected TC application on the
    local node.

- **m compiletime** _time_

    The **manage compile time** command.  This command sets the time at which
    the authority function for the selected TC application on this node will next
    compile a bulletin.  The command is not needed in normal operations, because
    future compile times are computed automatically as bulletins are compiled.
    _time_ must be in yyyy/mm/dd-hh:mm:ss format.

- **m interval** _bulletin\_compilation\_interval_

    The **manage bulletin compilation interval** command.  This interval,
    expressed as a number of seconds, controls the period on which the 
    authority function for the selected TC appliction on this node will
    compile new key information bulletins.  The default value is 3600 (one hour).

- **m grace** _bulletin\_consensus\_grace\_time_

    The **manage bulletin consensus grace time** command.  This interval,
    expressed as a number of seconds, controls the length of time the
    authority function for the selected TC application on this node  will
    wait after publishing a bulletin before computing a consensus bulletin;
    this parameter is intended to relax the degree to which the system
    clocks of all members of the collective authority for this TC
    application must be in agreement.  The default value is 60 (1 minute).

- **+** _authority\_array\_index_ _node\_number_

    This command asserts that the trusted Nth member of the collective authority
    for the selected TC application, where N is the _authority\_array\_index_
    value, is the node identified by _node\_number_.

- **-** _authority\_array\_index_

    This command asserts that the Nth member of the collective authority for
    the selected TC application, where N is the _authority\_array\_index_ value,
    is no longer trusted; bulletins received from this collective authority
    member must be discarded.

- **a** _node\_number_

    This command adds the node identified by _node\_number_ to the list of nodes
    hosting **authorized\_clients** for the selected TC application.  Once this
    list has been populated, TC records for this application that are received
    from clients residing on nodes that are not in the list are automatically
    discarded by the authority function residing on the local node.

- **d** _node\_number_

    This command deletes the node identified by _node\_number_ from the list of
    nodes hosting **authorized\_clients** for the selected TC application.

- **l**

    This command lists all nodes currently hosting **authorized\_clients** for
    the selected TC application.

- **x**

    The **stop** command.  This command stops the **tcarecv** and **tcacompile**
    daemons of the authority function for the selected TC application on the
    local node.

# EXAMPLES

- + 3 6913

    Asserts that node 6913 is now member 3 of the collective authority for the
    selected application.

# SEE ALSO

tcaadmin(1), dtka(3)
