# NAME

ltpsecrc - LTP security policy management commands file

# DESCRIPTION

LTP security policy management commands are passed to **ltpsecadmin** either
in a file of text lines or interactively at **ltpsecadmin**'s command prompt
(:).  Commands are interpreted line-by line, with exactly one command per
line.  The formats and effects of the LTP security policy management commands
are described below.

# COMMANDS

- **?**

    The **help** command.  This will display a listing of the commands and their
    formats.  It is the same as the **h** command.

- **#**

    Comment line.  Lines beginning with **#** are not interpreted.

- **e** { 1 | 0 }

    Echo control.  Setting echo to 1 causes all output printed by ltpsecadmin to
    be logged as well as sent to stdout.  Setting echo to 0 disables this behavior.

- **v** 

    Version number.  Prints out the version of ION currently installed.  HINT:
    combine with **e 1** command to log the version number at startup.

- **a ltprecvauthrule** _ltp\_engine\_id_ _ciphersuite\_nbr_ _\[key\_name\]_

    The **add ltprecvauthrule** command.  This command adds a rule specifying the
    manner in which LTP segment authentication will be applied to LTP segments
    received from the indicated LTP engine.

    A segment from the indicated LTP engine will only be deemed authentic if it
    contains an authentication extension computed via the ciphersuite identified
    by _ciphersuite\_nbr_ using the applicable key value.  If _ciphersuite\_nbr_
    is 255 then the applicable key value is a hard-coded constant and _key\_name_
    must be omitted; otherwise _key\_name_ is required and the applicable key
    value is the current value of the key named _key\_name_ in the local security
    policy database.

    Valid values of _ciphersuite\_nbr_ are:

    >     0: HMAC-SHA1-80
    >     1: RSA-SHA256
    >     255: NULL

- **c ltprecvauthrule** _ltp\_engine\_id_ _ciphersuite\_nbr_ _\[key\_name\]_

    The **change ltprecvauthrule** command.  This command changes the parameters
    of the LTP segment authentication rule for the indicated LTP engine. 

- **d ltprecvauthrule** _ltp\_engine\_id_

    The **delete ltprecvauthrule** command.  This command deletes the LTP segment
    authentication rule for the indicated LTP engine.

- **i ltprecvauthrule** _ltp\_engine\_id_

    This command will print information (the LTP engine id, ciphersuite
    number, and key name) about the LTP segment authentication rule for the
    indicated LTP engine.

- **l ltprecvauthrule**

    This command lists all LTP segment authentication rules in the security policy
    database.

- **a ltpxmitauthrule** _ltp\_engine\_id_ _ciphersuite\_nbr_ _\[key\_name\]_

    The **add ltpxmitauthrule** command.  This command adds a rule specifying the
    manner in which LTP segments transmitted to the indicated LTP engine must be
    signed.

    Signing a segment destined for the indicated LTP engine entails computing an
    authentication extension via the ciphersuite identified by _ciphersuite\_nbr_
    using the applicable key value.  If _ciphersuite\_nbr_ is 255 then the
    applicable key value is a hard-coded constant and _key\_name_ must be
    omitted; otherwise _key\_name_ is required and the applicable key
    value is the current value of the key named _key\_name_ in the local security
    policy database.

    Valid values of _ciphersuite\_nbr_ are:

    >     0: HMAC\_SHA1-80
    >     1: RSA\_SHA256
    >     255: NULL

- **c ltpxmitauthrule** _ltp\_engine\_id_ _ciphersuite\_nbr_ _\[key\_name\]_

    The **change ltpxmitauthrule** command.  This command changes the parameters
    of the LTP segment signing rule for the indicated LTP engine. 

- **d ltpxmitauthrule** _ltp\_engine\_id_

    The **delete ltpxmitauthrule** command.  This command deletes the LTP segment
    signing rule for the indicated LTP engine.

- **i ltpxmitauthrule** _ltp\_engine\_id_

    This command will print information (the LTP engine id, ciphersuite
    number, and key name) about the LTP segment signing rule for the indicated
    LTP engine.

- **l ltpxmitauthrule**

    This command lists all LTP segment signing rules in the security policy
    database.

- **h**

    The **help** command.  This will display a listing of the commands and their
    formats.  It is the same as the **?** command.

# SEE ALSO

ltpsecadmin(1)
