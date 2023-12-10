# NAME

bpsecrc - BP security policy management commands file

# DESCRIPTION

BP security policy management commands are passed to **bpsecadmin** either
in a file of text lines or interactively at **bpsecadmin**'s command prompt
(:).  Commands are interpreted line-by line, with exactly one command per
line.  The formats and effects of the BP security policy management commands
are described below.

A parameter identifed as an _eid\_expr_ is an "endpoint ID expression."  For
all commands, whenever the last character of an endpoint ID expression is
the wild-card character '\*', an applicable endpoint ID "matches" this EID
expression if all characters of the endpoint ID expression prior to the last
one are equal to the corresponding characters of that endpoint ID.  Otherwise
an applicable endpoint ID "matches" the EID expression only when all characters
of the EID and EID expression are identical.

ION supports the proposed "streamlined" Bundle Security Protocol (currently
posted as CCSDS Red Book 734.5-R-1) in place of the standard Bundle Security
Protocol (RFC 6257).  Since SBSP is not yet a published standard, ION's
Bundle Protocol security mechanisms will not necessarily
interoperate with those of other BP implementations.  This is unfortunate but
(we hope) temporary, as SBSP represents a major improvement in bundle security.
It is possible that the SBSP specification will change somewhat between now
and the time SBSP is published as a CCSDS standard and eventually an RFC,
and ION will be revised as necessary to conform to those changes, but in
the meantime we believe that the advantages of SBSP make it more suitable
than RFC 6257 as a foundation for the development and deployment of secure
DTN applications.

# COMMANDS

- **?**

    The **help** command.  This will display a listing of the commands and their
    formats.  It is the same as the **h** command.

- **#**

    Comment line.  Lines beginning with **#** are not interpreted.

- **e** { 1 | 0 }

    Echo control.  Setting echo to 1 causes all output printed by bpsecadmin to
    be logged as well as sent to stdout.  Setting echo to 0 disables this behavior.

- **v** 

    Version number.  Prints out the version of ION currently installed.  HINT:
    combine with **e 1** command to log the version number at startup.

- **a bspbibrule** _source\_eid\_expr_ _destination\_eid\_expr_ _block\_type\_number_ _{ '' | ciphersuite\_name key\_name }_

    The **add bspbibrule** command.  This command adds a rule specifying the
    manner in which Block Integrity Block (BIB) validation will be applied
    to blocks of type _block\_type\_number_ for all bundles sourced at any node
    whose administrative endpoint ID matches _source\_eid\_expr_ and destined for
    any node whose administrative endpoint ID ID matches _destination\_eid\_expr_.

    If a zero-length string ('') is indicated instead of a _ciphersuite\_name_
    then BIB validation is disabled for this source/destination EID expression
    pair: blocks of the type indicated by _block\_type\_number_ in all
    bundles sourced at nodes with matching administrative endpoint IDs and
    destined for nodes with matching administrative endpoint IDs will be
    immediately deemed valid.  Otherwise, a block of the indicated type that
    is attached to a bundle sourced at a node with matching administrative
    endpoint ID and destined for a node with matching administrative endpoint
    ID will only be deemed valid if the bundle contains a corresponding BIB
    computed via the ciphersuite named by _ciphersuite\_name_ using a key
    value that is identical to the current value of the key named _key\_name_
    in the local security policy database.

- **c bspbibrule** _source\_eid\_expr_ _destination\_eid\_expr_ _block\_type\_number_ _{ '' | ciphersuite\_name key\_name }_

    The **change bspbibrule** command.  This command changes the ciphersuite
    name and/or key name for the BIB rule pertaining to the source/destination EID
    expression pair identified by _source\_eid\_expr_ and _destination\_eid\_expr_
    and the block identified by _block\_type\_number_.
    Note that the _eid\_expr_s must exactly match those of the rule that is to
    be modified, including any terminating wild-card character.  

- **d bspbibrule** _source\_eid\_expr_ _destination\_eid\_expr_ _block\_type\_number_

    The **delete bspbibrule** command.  This command deletes the BIB rule
    pertaining to the source/destination EID expression pair identified by
    _sender\_eid\_expr_ and _receiver\_eid\_expr_ and the block identified by
    _block\_type\_number_.  Note that the _eid\_expr_s
    must exactly match those of the rule that is to be deleted, including any
    terminating wild-card character.

- **i bspbibrule** _source\_eid\_expr_ _destination\_eid\_expr_ _block\_type\_number_

    This command will print information (the ciphersuite and key names) about the
    BIB rule pertaining to _source\_eid\_expr_, _destination\_eid\_expr_, and
    _block\_type\_number_.

- **l bspbibrule**

    This command lists all BIB rules in the security policy database.

- **a bspbcbrule** _source\_eid\_expr_ _destination\_eid\_expr_ _block\_type\_number_ _{ '' | ciphersuite\_name key\_name }_

    The **add bspbcbrule** command.  This command adds a rule specifying the
    manner in which Block Confidentiality Block (BCB) encryption will be applied
    to blocks of type _block\_type\_number_ for all bundles sourced at any node
    whose administrative endpoint ID matches _source\_eid\_expr_ and destined for
    any node whose administrative endpoint ID ID matches _destination\_eid\_expr_.

    If a zero-length string ('') is indicated instead of a _ciphersuite\_name_
    then BCB encryption is disabled for this source/destination EID expression
    pair: blocks of the type indicated by _block\_type\_number_ in all
    bundles sourced at nodes with matching administrative endpoint IDs and
    destined for nodes with matching administrative endpoint IDs will be
    sent in plain text.  Otherwise, a block of the indicated type that
    is attached to a bundle sourced at a node with matching administrative
    endpoint ID and destined for a node with matching administrative endpoint
    ID can only be deemed decrypted if the bundle contains a corresponding BCB
    computed via the ciphersuite named by _ciphersuite\_name_ using a key
    value that is identical to the current value of the key named _key\_name_
    in the local security policy database.

- **c bspbcbrule** _source\_eid\_expr_ _destination\_eid\_expr_ _block\_type\_number_ _{ '' | ciphersuite\_name key\_name }_

    The **change bspbcbrule** command.  This command changes the ciphersuite
    name and/or key name for the BCB rule pertaining to the source/destination EID
    expression pair identified by _source\_eid\_expr_ and _destination\_eid\_expr_
    and the block identified by _block\_type\_number_.
    Note that the _eid\_expr_s must exactly match those of the rule that is to
    be modified, including any terminating wild-card character.  

- **d bspbcbrule** _source\_eid\_expr_ _destination\_eid\_expr_ _block\_type\_number_

    The **delete bspbcbrule** command.  This command deletes the BCB rule
    pertaining to the source/destination EID expression pair identified by
    _sender\_eid\_expr_ and _receiver\_eid\_expr_ and the block identified by
    _block\_type\_number_.  Note that the _eid\_expr_s
    must exactly match those of the rule that is to be deleted, including any
    terminating wild-card character.

- **i bspbcbrule** _source\_eid\_expr_ _destination\_eid\_expr_ _block\_type\_number_

    This command will print information (the ciphersuite and key names) about the
    BCB rule pertaining to _source\_eid\_expr_, _destination\_eid\_expr_, and
    _block\_type\_number_.

- **l bspbcbrule**

    This command lists all BCB rules in the security policy database.

- **x** _\[ { ~ | sender\_eid\_expr } \[ { ~ | receiver\_eid\_expr} \[ { ~ | bib | bcb } \] \] \]_

    This command will clear all rules for the indicated type of bundle security
    block between the indicated security source and security destination.  If
    block type is omitted it defaults to **~** signifying "all SBSP blocks".  If
    both block type and security destination are omitted, security destination
    defaults to **~** signifying "all SBSP security destinations".  If all three
    command-line parameters are omitted, then security source defaults to **~**
    signifying "all SBSP security sources".

- **h**

    The **help** command.  This will display a listing of the commands and their
    formats.  It is the same as the **?** command.

# SEE ALSO

bpsecadmin(1)
