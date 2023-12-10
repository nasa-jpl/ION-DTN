# NAME

cfdprc - CCSDS File Delivery Protocol management commands file

# DESCRIPTION

CFDP management commands are passed to **cfdpadmin** either in a file of
text lines or interactively at **cfdpadmin**'s command prompt (:).  Commands
are interpreted line-by line, with exactly one command per line.  The formats
and effects of the CFDP management commands are described below.

# COMMANDS

- **?**

    The **help** command.  This will display a listing of the commands and their
    formats.  It is the same as the **h** command.

- **#**

    Comment line.  Lines beginning with **#** are not interpreted.

- **e** { 1 | 0 }

    Echo control.  Setting echo to 1 causes all output printed by cfdpadmin to be
    logged as well as sent to stdout.  Setting echo to 0 disables this behavior.

- **v** 

    Version number.  Prints out the version of ION currently installed.  HINT:
    combine with **e 1** command to log the version number at startup.

- **1**

    The **initialize** command.  Until this command is executed, CFDP is not
    in operation on the local ION node and most _cfdpadmin_ commands will fail.

- **a entity**> &lt;entity nbr> <UT protocol name> <UT endpoint name> &lt;rtt> &lt;incstype> &lt;outcstype>

    The **add entity** command.  This command will add a new remote CFDP entity to
    the CFDP management information base.  Valid UT protocol names are bp and
    tcp.  Endpoint name is EID for bp, socket spec (_IP address_:_port number_)
    for tcp.  RTT is round-trip time, used to set acknowledgement timers. incstype
    is the type of checksum to use when validating data received from this entity;
    valid values are 0 (modular checksum), 2 (CRC32C), and 15 (the null checksum).
    outcstype is the type of checksum to use when computing the checksum for
    transmitting data to this entity.

- **c entity**> &lt;entity nbr> <UT protocol name> <UT endpoint name> &lt;rtt> &lt;incstype> &lt;outcstype>

    The **change entity** command.  This command will change information associated
    with an existing entity in the CFDP management information base.

- **d entity**> &lt;entity nbr>

    The **delete entity** command.  This command will delete an existing entity from
    the CFDP management information base.

- **i** \[&lt;entity nbr>\]

    The **info** command.  When **entity nbr** is provided, this command will print
    information about the indicated entity.  Otherwise this command will print
    information about the current state of the local CFDP entity, including the
    current settings of all parameters that can be managed as described below.

- **s** '_UTS command_'

    The **start** command.  This command starts the UT-layer service task
    for the local CFDP entity.

- **m discard** { 0 | 1 }

    The **manage discard** command.  This command enables or disables the
    discarding of partially received files upon cancellation of a file reception.
    The default value is 1;

- **m requirecrc** { 0 | 1 }

    The **manage CRC data integrity** command.  This command enables or disables the
    attachment of CRCs to all PDUs issued by the local CFDP entity.  The default
    value is 0;

- **m fillchar** _file\_fill\_character_

    The **manage fill character** command.  This command establishes the fill
    character to use for the portions of an incoming file that have not yet
    been received.  The fill character is normally expressed in hex, e.g., 
    the default value is 0xaa.

- **m ckperiod** _check\_cycle\_period_

    The **manage check interval** command.  This command establishes the number
    of seconds following reception of the EOF PDU -- or following expiration
    of a prior check cycle -- after which the local CFDP will check for 
    completion of a file that is being received.  Default value is 86400 (i.e.,
    one day).

- **m maxtimeouts** _check\_cycle\_limit_

    The **manage check limit** command.  This command establishes the number
    of check cycle expirations after which the local CFDP entity will invoke
    the check cycle expiration fault handler upon expiration of a check cycle.
    Default value is 7.

- **m maxevents** _event\_queue\_limit_

    The **manage event queue limit** command.  This command establishes the
    maximum number of unread service indications (CFDP "events") that may be
    queued up for delivery at any time.  When the events queue length exceeds
    this figure, events are simply deleted (in decreasing age order, oldest
    first) until the the limit is no longer exceeded.  Default value is 20.

- **m maxtrnbr** _max\_transaction\_number_

    The **manage transaction numbers** command.  This command establishes the
    largest possible transaction number used by the local CFDP entity for file
    transmission transactions.  After this number has been used, the 
    transaction number assigned to the next transaction will be 1.  The
    default value is 999999999.

- **m segsize** _max\_bytes\_per\_file\_data\_segment_

    The **manage segment size** command.  This command establishes the
    number of bytes of file data in each file data PDU transmitted by the
    local CFDP entity in the absence of an application-supplied reader
    function.  The default value is 65000.

- **m inactivity** _inactivity\_period_

    The **manage inactivity period** command. This command establishes the number
    of seconds that a CFDP file transfer is allowed to go idle before being
    canceled for inactivity. The default is one day.

- **x**

    The **stop** command.  This command stops the UT-layer service task for
    the local CFDP engine.

- **w** { 0 | 1  | &lt;activity\_spec> }

    The **CFDP watch** command.  This command enables and disables production of
    a continuous stream of user-selected CFDP activity indication characters.  A
    watch parameter of "1" selects all CFDP activity indication characters; "0"
    de-selects all CFDP activity indication characters; any other _activity\_spec_
    such as "p" selects all activity indication characters in the string,
    de-selecting all others.  CFDP will print each selected activity indication
    character to **stdout** every time a processing event of the associated type
    occurs:

    **p**	CFDP PDU transmitted

    **q**	CFDP PDU received

- **h**

    The **help** command.  This will display a listing of the commands and their
    formats.  It is the same as the **?** command.

# EXAMPLES

- m requirecrc 1

    Initiates attachment of CRCs to all subsequently issued CFDP PDUs.

# SEE ALSO

cfdpadmin(1), bputa(1)
