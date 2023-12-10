# NAME

acsrc - Aggregate Custody Signal management commands file

# DESCRIPTION

Aggregate Custody Signal management commands are passed to **acsadmin** either
in a file of text lines or interactively at **acsadmin**'s command prompt (:).
Commands are interpreted line-by line, with exactly one command per line.  The
formats and effects of the Aggregate Custody Signal management commands are
described below.

# GENERAL COMMANDS

- **?**

    The **help** command.  This will display a listing of the commands and their
    formats.  It is the same as the **h** command.

- **#**

    Comment line.  Lines beginning with **#** are not interpreted.

- **e** { 1 | 0 }

    Echo control.  Setting echo to 1 causes all output printed by acsadmin to be
    logged as well as sent to stdout.  Setting echo to 0 disables this behavior.

- **v** 

    Version number.  Prints out the version of ION currently installed.  HINT:
    combine with **e 1** command to log the version number at startup.

- **1** &lt;logLevel> \[&lt;heapWords>\]

    The **initialize** command.  Until this command is executed, Aggregate Custody
    Signals are not in operation on the local ION node and most _acsadmin_
    commands will fail.

    The _logLevel_ argument specifies at which log level the ACS appending and
    transmitting implementation should record its activity to the ION log file.
    This argument is the bitwise "OR" of the following log levels:

    - 0x01  ERROR

        Errors in ACS programming are logged.

    - 0x02  WARN

        Warnings like "out of memory" that don't cause ACS to fail but may change
        behavior are logged.

    - 0x04  INFO

        Informative information like "this custody signal is a duplicate" is logged.

    - 0x08  DEBUG

        Verbose information like the state of the pending ACS tree is logged.

    The optional _heapWords_ argument informs ACS to allocate that many heap words
    in its own DRAM SDR for constructing pending ACS.  If not supplied, the 
    default value 10000 is used.  Once all ACS SDR is allocated,
    any incoming custodial bundles that would trigger an ACS will trigger a normal,
    non-aggregate custody signal instead, until ACS SDR is freed.  If your node
    intermittently emits non-aggregate custody signals when it should emit ACS,
    you should increase _heapWords_.

    Since ACS uses SDR only for emitting Aggregate Custody Signals, ION can still
    receive ACS even if this command is not executed, or all ACS SDR memory is
    allocated.

- **h**

    The **help** command.  This will display a listing of the commands and their
    formats.  It is the same as the **?** command.

- **s** &lt;minimumCustodyId>

    This command sets the minimum custody ID that the local bundle agent may use
    in custody transfer enhancement blocks that it emits.  These custody IDs must
    be unique in the network (for the lifetime of the bundles to which they
    refer).

    The _minimumCustodyId_ provided is stored in SDR, and incremented every time a
    new custody ID is required.  So, this command should be used only when the
    local bundle agent has discarded its SDR and restarted.

- **t** &lt;acsBundleLifetime>

    This command sets the lifetime that will be asserted for every ACS bundle
    subsequently issued by the local bundle agent.  The new _acsBundleLifetime_
    is stored in SDR.

# CUSTODIAN COMMANDS

- **a** _custodianEid_ _acsSize_ \[_acsDelay_\]

    The **add custodian** command.  This command provides information about the ACS
    characteristics of a remote custodian.  _custodianEid_ is the custodian EID for
    which this command is providing information.  _acsSize_ is the preferred size
    of ACS bundles sent to _custodianEid_; ACS bundles this implementation sends
    to _custodianEid_ will aggregate until ACS are at most _acsSize_ bytes (if
    _acsSize_ is smaller than 19 bytes, some ACS containing only one signal will
    exceed _acsSize_ and be sent anyways; setting _acsSize_ to 0 causes
    "aggregates" of only 1 signal to be sent).

    _acsDelay_
    is the maximum amount of time to delay an ACS destined for this custodian
    before sending it, in seconds; if not specified, the default value 15 will be
    used.

# EXAMPLES

- a ipn:15.0 100 27

    Informs ACS on the local node that the local node should send ACS bundles
    destined for the custodian ipn:15.0 whenever they are 100 bytes in size or have
    been delayed for 27 seconds, whichever comes first.

# SEE ALSO

acsadmin(1)
