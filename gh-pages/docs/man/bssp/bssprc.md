# NAME

bssprc - Bundle Streaming Service Protocol management commands file

# DESCRIPTION

BSSP management commands are passed to **bsspadmin** either in a file of
text lines or interactively at **bsspadmin**'s command prompt (:).  Commands
are interpreted line-by line, with exactly one command per line.  The formats
and effects of the BSSP management commands are described below.

# COMMANDS

- **?**

    The **help** command.  This will display a listing of the commands and their
    formats.  It is the same as the **h** command.

- **#**

    Comment line.  Lines beginning with **#** are not interpreted.

- **e** { 1 | 0 }

    Echo control.  Setting echo to 1 causes all output printed by bsspadmin to be
    logged as well as sent to stdout.  Setting echo to 0 disables this behavior.

- **v** 

    Version number.  Prints out the version of ION currently installed.  HINT:
    combine with **e 1** command to log the version number at startup.

- **1** _est\_max\_nbr\_of\_sessions_

    The **initialize** command.  Until this command is executed, BSSP is not
    in operation on the local ION node and most _bsspadmin_ commands will fail.

    The command uses _est\_max\_nbr\_of\_sessions_ to configure the hashtable it
    will use to manage access to transmission sessions that are currently
    in progress.  For optimum performance, _est\_max\_nbr\_of\_sessions_ should
    normally equal or exceed the summation of _max\_nbr\_of\_sessions_ over all
    spans as discussed below.

- **a span** _peer\_engine\_nbr_ _max\_nbr\_of\_sessions_ _max\_block\_size_ '_BE-BSO\_command_' '_RL-BSO\_command_ \[_queuing\_latency_\]

    The **add span** command.  This command declares that a _span_ of potential
    BSSP data interchange exists between the local BSSP engine and the indicated
    (neighboring) BSSP engine.

    The _max\_block\_size_ is expressed as a number of bytes of data.
    _max\_block\_size_ is used to configure transmission buffer sizes; as such, it
    limits client data item size. When exceeded, it causes bssp\_send() to fail, and
    bssp\_clo() to shutdown. To restart bssp\_clo() without rebooting ION immediately,
    say, due to on-going data transfer over other convergence layers, one can try to first 
    stop bpadmin with 'x', then stop bsspadmin with 'x', then run ionrestart to repair
    any issue in the volatile database, then relaunch bssp daemon and then bp
    daemon using corresponding configuration files. This procedure could temporarily 
    enable resumption of operation until it is safe to reboot ION.

    _max\_nbr\_of\_\_sessions_ constitutes, in effect, the local BSSP engine's
    retransmission "window" for this span.  The retransmission windows of the
    spans impose flow control on BSSP transmission, reducing the chance ofx
    allocation of all available space in the ION node's data store to BSSP
    transmission sessions.

    _BE-BSO\_command_ is script text that will be executed when BSSP is started on
    this node, to initiate operation of the best-efforts transmission channel task
    for this span.  Note that " _peer\_engine\_nbr_" will automatically be
    appended to _BE-BSO\_command_ by **bsspadmin** before the command is executed,
    so only the link-service-specific portion of the command should be provided
    in the _LSO\_command_ string itself.

    _RL-BSO\_command_ is script text that will be executed when BSSP is started on
    this node, to initiate operation of the reliable transmission channel task
    for this span.  Note that " _peer\_engine\_nbr_" will automatically be
    appended to _RL-BSO\_command_ by **bsspadmin** before the command is executed,
    so only the link-service-specific portion of the command should be provided
    in the _LSO\_command_ string itself.

    _queuing\_latency_ is the estimated number of seconds that we expect to lapse
    between reception of a block at this node and transmission of an
    acknowledging PDU, due to processing delay in the node.  (See the
    'm ownqtime' command below.)  The default value is 1.

    If _queuing latency_ a negative number, the absolute value of this number
    is used as the actual queuing latency and session purging is enabled;
    otherwise session purging is disabled.  If session purging is enabled
    for a span then at the end of any period of transmission over this span
    all of the span's export sessions that are currently in progress are
    automatically canceled.  Notionally this forces re-forwarding of the DTN
    bundles in each session's block, to avoid having to wait for the restart
    of transmission on this span before those bundles can be successfully
    transmitted.

- **a seat** '_BE-BSI\_command_' '_RL-BSI\_command_'

    The **add seat** command.  This command declares that the local BSSP engine
    can receive BSSP PDUs via the link service input daemons that begin
    running when '_BE-BSI\_command_' and '_RL-BSI\_command_' are executed.

- **c span** _peer\_engine\_nbr_ _max\_nbr\_of\_sessions_ _max\_block\_size_ '_BE-BSO\_command_' '_RL-BSO\_command_ \[_queuing\_latency_\]

    The **change span** command.  This command sets the indicated span's 
    configuration parameters to the values provided as arguments.

- **d span** _peer\_engine\_nbr_

    The **delete span** command.  This command deletes the span identified
    by _peer\_engine\_nbr_.  The command will fail if any outbound blocks
    for this span are pending transmission.

- **d seat** '_BE-BSI\_command_' '_RL-BSI\_command_'

    The **delete span** command.  This command deletes the seat identified
    by '_BE-BSI\_command_' and '_RL-BSI\_command_'.

- **i span** _peer\_engine\_nbr_

    This command will print information (all configuration parameters)
    about the span identified by _peer\_engine\_nbr_.

- **i seat** '_BE-BSI\_command_' '_RL-BSI\_command_'

    This command will print all information (i.e., process ID numbers)
    about the seat identified by '_BE-BSO\_command_' and '_RL-BSO\_command_'.

- **l span**

    This command lists all declared BSSP data interchange spans.

- **l seat**

    This command lists all declared BSSP data acquisition seats.

- **s** \['_BE-BSI\_command_' '_RL-BSI\_command_'\]

    The **start** command.  This command starts reliable and best-efforts link
    service output tasks for all BSSP spans (to remote engines) from the local
    BSSP engine, and it starts the reliable and best-efforts link service input
    tasks for the local engine.  '_BE-BSI\_command_' and '_RL-BSI\_command_'
    are deprecated but are supported for backward compatibility; if provided,
    the effect is the same as entering the command 
    "a seat '_BE-BSI\_command_' '_RL-BSI\_command_'" prior to starting all
    daemon tasks.

- **m ownqtime** _own\_queuing\_latency_

    The **manage own queuing time** command.  This command sets the number of
    seconds of predicted additional latency attributable to processing delay
    within the local engine itself that should be included whenever BSSP computes
    the nominal round-trip time for an exchange of data with any remote engine.
    The default value is 1.

- **x**

    The **stop** command.  This command stops all link service input and output
    tasks for the local BSSP engine.

- **w** { 0 | 1 | &lt;activity\_spec> }

    The **BSSP watch** command.  This command enables and disables production of
    a continuous stream of user-selected BSSP activity indication characters.  A
    watch parameter of "1" selects all BSSP activity indication characters; "0"
    de-selects all BSSP activity indication characters; any other _activity\_spec_
    such as "DF-" selects the activity indication characters in the string,
    de-selecting all others.  BSSP will print each selected activity indication
    character to **stdout** every time a processing event of the associated type
    occurs:

    **D**	bssp send completed

    **E**	bssp block constructed for issuance

    **F**	bssp block issued

    **G**	bssp block popped from best-efforts transmission queue

    **H**	positive ACK received for bssp block, session ended

    **S**	bssp block received

    **T**	bssp block popped from reliable transmission queue

    **-**	unacknowledged best-efforts block requeued for reliable transmission

    **\***	session canceled locally by sender

- **h**

    The **help** command.  This will display a listing of the commands and their
    formats.  It is the same as the **?** command.

# EXAMPLES

- a span 19 20 4096 'udpbso node19.ohio.edu:5001' 'tcpbso node19.ohio.edu:5001'

    Declares a data interchange span between the local BSSP engine and the remote
    engine (ION node) numbered 19.  There can be at most 20 concurrent sessions
    of BSSP transmission activity to this node.  Maximum block size for this span
    is set to 4096 bytes, and the best-efforts and reliable link service
    output tasks that are initiated when BSSP is started on the local ION node
    will execute the _udpbso_ and _tcpbso_ programs as indicated.

- m ownqtime 2

    Sets local queuing delay allowance to 2 seconds.

# SEE ALSO

bsspadmin(1), udpbsi(1), udpbso(1), tcpbsi(1), tcpbso(1)
