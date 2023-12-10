# NAME

dtkarc - DTKA user configuration commands file

# DESCRIPTION

DTKA user configuration commands are passed to **dtkaadmin** either
in a file of text lines or interactively at **dtkaadmin**'s command prompt
(:).  Commands are interpreted line-by line, with exactly one command per
line.  The formats and effects of the DTKA user function administration
commands are described below.

# COMMANDS

- **?**

    The **help** command.  This will display a listing of the commands and their
    formats.  It is the same as the **h** command.

- **#**

    Comment line.  Lines beginning with **#** are not interpreted.

- **e** { 1 | 0 }

    Echo control.  Setting echo to 1 causes all output printed by dtkaadmin to be
    logged as well as sent to stdout.  Setting echo to 0 disables this behavior.

- **1**

    The **initialize** command.  Until this command is executed, the DTKA user
    function is not in operation on the local ION node and most _dtkaadmin_
    commands will fail.

- **i**

    The **info** This command will print information about the current state
    of the local node's DTKA user function, including the current settings of
    all parameters that can be managed as described below.

- **m keygentime** _time_

    The **manage key generation time** command.  This command sets the time at
    which the node will next generate a public/private key pair and multicast
    the public key.  The command is not needed in normal operations, because
    future key generation times are computed automatically as key pairs are
    generated. _time_ must be in yyyy/mm/dd-hh:mm:ss format.

- **m interval** _key\_pair\_generation\_interval_

    The **manage key pair generation interval** command.  This interval,
    expressed as a number of seconds, controls the period on which the DTKA
    user function will generate new public/private key pairs.  The default
    value is 604800 (one week).

- **m leadtime** _key\_pair\_effectivenes\_lead\_time_

    The **manage key pair effectivenes lead time** command.  This interval,
    expressed as a number of seconds, controls the length of time after the
    time of key pair generation at which the key pair will become effective.
    The default value is 345600 (four days).

- **h**

    The **help** command.  This will display a listing of the commands and their
    formats.  It is the same as the **?** command.

# EXAMPLES

- m interval 3600

    Asserts that the DTKA function will generate a new key pair every 3600 seconds
    (one hour).

# SEE ALSO

dtkaadmin(1)
