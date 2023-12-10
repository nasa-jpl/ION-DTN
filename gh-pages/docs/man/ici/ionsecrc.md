# NAME

ionsecrc - ION security database management commands file

# DESCRIPTION

ION security database management commands are passed to **ionsecadmin**
either in a file of text lines or interactively at **ionsecadmin**'s command
prompt (:).  Commands are interpreted line-by line, with exactly one command per
line.  The formats and effects of the ION security database management
commands are described below.

# COMMANDS

- **?**

    The **help** command.  This will display a listing of the commands and their
    formats.  It is the same as the **h** command.

- **#**

    Comment line.  Lines beginning with **#** are not interpreted.

- **e** { 1 | 0 }

    Echo control.  Setting echo to 1 causes all output printed by ionsecadmin to
    be logged as well as sent to stdout.  Setting echo to 0 disables this behavior.

- **v** 

    Version number.  Prints out the version of ION currently installed.  HINT:
    combine with **e 1** command to log the version number at startup.

- **1**

    The **initialize** command.  Until this command is executed, the local ION
    node has no security database and most _ionsecadmin_ commands will fail.

- **a key** _key\_name_ _file\_name_

    The **add key** command.  This command adds a named key value to the
    security database.  The content of _file\_name_ is taken as the
    value of the key.  Named keys can be referenced by other elements of the
    security database.

- **c key** _key\_name_ _file\_name_

    The **change key** command.  This command changes the value of the named
    key, obtaining the new key value from the content of _file\_name_.

- **d key** _key\_name_

    The **delete key** command.  This command deletes the key identified by _name_.

- **i key** _key\_name_

    This command will print information about the named key, i.e., the length of
    its current value.

- **l key**

    This command lists all keys in the security database.

- **h**

    The **help** command.  This will display a listing of the commands and their
    formats.  It is the same as the **?** command.

# EXAMPLES

- a key BABKEY ./babkey.txt

    Adds a new key named "BABKEY" whose value is the content of the file
    "./babkey.txt".

# SEE ALSO

ionsecadmin(1)
