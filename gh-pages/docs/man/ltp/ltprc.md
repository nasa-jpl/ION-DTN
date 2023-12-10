# NAME

ltprc - Licklider Transmission Protocol management commands file

# DESCRIPTION

LTP management commands are passed to **ltpadmin** either in a file of
text lines or interactively at **ltpadmin**'s command prompt (:).  Commands
are interpreted line-by line, with exactly one command per line.  The formats
and effects of the LTP management commands are described below.

# COMMANDS

- **?**

    The **help** command.  This will display a listing of the commands and their
    formats.  It is the same as the **h** command.

- **#**

    Comment line.  Lines beginning with **#** are not interpreted.

- **e** { 1 | 0 }

    Echo control.  Setting echo to 1 causes all output printed by ltpadmin to be
    logged as well as sent to stdout.  Setting echo to 0 disables this behavior.

- **v** 

    Version number.  Prints out the version of ION currently installed.  HINT:
    combine with **e 1** command to log the version number at startup.

- **1** _est\_max\_export\_sessions_

    The **initialize** command.  Until this command is executed, LTP is not
    in operation on the local ION node and most _ltpadmin_ commands will fail.

    The command uses _est\_max\_export\_sessions_ to configure the hashtable it
    will use to manage access to export transmission sessions that are currently
    in progress.  For optimum performance, _est\_max\_export\_sessions_ should
    normally equal or exceed the summation of _max\_export\_sessions_ over all
    spans as discussed below.

    Appropriate values for the parameters configuring
    each "span" of potential LTP data exchange between the local LTP and
    neighboring engines are non-trivial to determine.  See the ION LTP
    configuration spreadsheet and accompanying documentation for details.

- **a span** _peer\_engine\_nbr_ _max\_export\_sessions_ _max\_import\_sessions_ _max\_segment\_size_ _aggregation\_size\_threshold_ _aggregation\_time\_limit_ '_LSO\_command_' \[_queuing\_latency_\]

    The **add span** command.  This command declares that a _span_ of potential
    LTP data interchange exists between the local LTP engine and the indicated
    (neighboring) LTP engine.

    The _max\_segment\_size_ and _aggregation\_size\_threshold_
    are expressed as numbers of bytes of data.  _max\_segment\_size_
    limits the size of each of the segments into which each outbound data
    _block_ will be divided; typically this limit will be the maximum number
    of bytes that can be encapsulated within a single transmission frame of the
    underlying _link service_.

    _aggregation\_size\_threshold_ limits the number of LTP service data units
    (e.g., bundles) that can be aggregated into a single block: when
    the sum of the sizes of all service data units aggregated into a block
    exceeds this threshold, aggregation into this block must cease and the block
    must be segmented and transmitted.  

    _aggregation\_time\_limit_ alternatively limits the number of seconds that
    any single export session block for this span will await aggregation before
    it is segmented and transmitted regardless of size.  The aggregation time
    limit prevents undue delay before the transmission of data during periods
    of low activity.

    _max\_export\_sessions_ constitutes, in effect,
    the local LTP engine's retransmission "window" for this span.  The
    retransmission windows of the spans impose flow control on LTP transmission,
    reducing the chance of allocation of all available space in the ION node's data
    store to LTP transmission sessions.

    _max\_import\_sessions_ is simply the neighoring engine's own value for the
    corresponding export session parameter; it is the neighboring engine's
    retransmission window size for this span.  It reduces the chance of allocation
    of all available space in the ION node's data store to LTP reception sessions.

    _LSO\_command_ is script text that will be executed when LTP is started on
    this node, to initiate operation of a link service output task for this
    span.  Note that " _peer\_engine\_nbr_" will automatically be
    appended to _LSO\_command_ by **ltpadmin** before the command is executed,
    so only the link-service-specific portion of the command should be provided
    in the _LSO\_command_ string itself.

    _queuing\_latency_ is the estimated number of seconds that we expect to lapse
    between reception of a segment at this node and transmission of an
    acknowledging segment, due to processing delay in the node.  (See the
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

- **a seat** '_LSI\_command_' 

    The **add seat** command.  This command declares that the local LTP engine
    can receive LTP segments via the link service input daemon that begins
    running when _'LSI\_command'_ is executed.

- **c span** _peer\_engine\_nbr_ _max\_export\_sessions_ _max\_import\_sessions_ _max\_segment\_size_ _aggregation\_size\_threshold_ _aggregation\_time\_limit_ '_LSO\_command_' \[_queuing\_latency_\]

    The **change span** command.  This command sets the indicated span's 
    configuration parameters to the values provided as arguments.

- **d span** _peer\_engine\_nbr_

    The **delete span** command.  This command deletes the span identified
    by _peer\_engine\_nbr_.  The command will fail if any outbound segments
    for this span are pending transmission or any inbound blocks from the
    peer engine are incomplete.

- **d seat** '_LSI\_command_'

    The **delete seat** command.  This command deletes the seat identified
    by '_LSI\_command_'.  

- **i span** _peer\_engine\_nbr_

    This command will print information (all configuration parameters)
    about the span identified by _peer\_engine\_nbr_.

- **i seat** '_LSI\_command_'

    This command will print all information (i.e., process ID number) about
    the seat identified by '_LSI\_command_'.  

- **l span**

    This command lists all declared LTP data interchange spans.

- **l seat**

    This command lists all declared LTP data acquisition seats.

- **s** \['_LSI\_command_'\]

    The **start** command.  This command starts link service input tasks for
    all LTP seats and output tasks for all LTP spans (to remote engines) from
    the local LTP engine.  '_LSI\_command_' is deprecated but is supported for
    backward compatibility; if provided, the effect is the same as entering
    the command "a seat '_LSI\_command_'" prior to starting all daemon tasks.

- **m heapmax** _max\_database\_heap\_per\_block_

    The **manage heap for block acquisition** command.  This command declares
    the maximum number of bytes of SDR heap space that will be occupied by the
    acquisition of any single LTP block.  All data acquired in excess of this
    limit will be written to a temporary file pending extraction and dispatching
    of the acquired block.  Default is the minimum allowed value (560 bytes),
    which is the approximate size of a ZCO file reference object; this is the
    minimum SDR heap space occupancy in the event that all acquisition is into
    a file.

- **m screening** { y | n }

    The **manage screening** command.  

    The **manage screening** command.  This command disables or enables the
    screening of received LTP segments per the periods of scheduled reception
    in the node's contact graph.

    By default, screening is enabled - that is, LTP segments from a given
    remote LTP engine (ION node) will be silently discarded when they arrive
    during an interval when the contact graph says the data rate from that
    engine to the local LTP engine is zero.  The reason for this is that without
    a known nominal reception rate we cannot enforce reception rate control,
    which is needed in order to prevent resource exhaustion at the receiving node.

    Note, though, that the enabling of screening implies that the ranges declared
    in the contact graph must be accurate and clocks must be synchronized;
    otherwise, segments will be arriving at times other than the scheduled
    contact intervals and will be discarded.  

    For some research purposes this constraint may be difficult to satisfy.
    For such purposes ONLY, where resource exhaustion at the receiving node
    is not at issue, screening may be disabled.

- **m ownqtime** _own\_queuing\_latency_

    The **manage own queuing time** command.  This command sets the number of
    seconds of predicted additional latency attributable to processing delay
    within the local engine itself that should be included whenever LTP computes
    the nominal round-trip time for an exchange of data with any remote engine.
    The default value is 1.

- **m maxber** _max\_expected\_bit\_error\_rate_

    The **manage max bit error rate** command.  This command sets the expected
    maximum bit error rate that LTP should provide for in computing the maximum
    number of transmission efforts to initiate in the transmission of a given
    block.  (Note that this computation is also sensitive to data segment size
    and to the size of the block that is to be transmitted.)  The default value
    is .000001 (10^-6).

- **m maxbacklog** _max\_delivery\_backlog_

    The **manage max delivery backlog** command.  This command sets the limit
    on the number of blocks (service data units) that may be queued up for
    delivery to clients.  While the queue is at this limit, red segments
    are discarded as it is not possible to deliver the blocks to which they
    pertain.  The intent here is to prevent resource exhaustion by limiting
    the rate at which new blocks can be acquired and inserted into ZCO space.
    The default value is 10.

- **x**

    The **stop** command.  This command stops all link service input and output
    tasks for the local LTP engine.

- **w** { 0 | 1 | &lt;activity\_spec> }

    The **LTP watch** command.  This command enables and disables production of
    a continuous stream of user-selected LTP activity indication characters.  A
    watch parameter of "1" selects all LTP activity indication characters; "0"
    de-selects all LTP activity indication characters; any other _activity\_spec_
    such as "df{\]" selects all activity indication characters in the string,
    de-selecting all others.  LTP will print each selected activity indication
    character to **stdout** every time a processing event of the associated type
    occurs:

    **d**	bundle appended to block for next session

    **e**	segment of block is queued for transmission

    **f**	block has been fully segmented for transmission

    **g**	segment popped from transmission queue

    **h**	positive ACK received for block, session ended

    **s**	segment received

    **t**	block has been fully received

    **@**	negative ACK received for block, segments retransmitted

    **=**	unacknowledged checkpoint was retransmitted

    **+**	unacknowledged report segment was retransmitted

    **{**	export session canceled locally (by sender)

    **}**	import session canceled by remote sender

    **\[**	import session canceled locally (by receiver)

    **\]**	export session canceled by remote receiver

- **h**

    The **help** command.  This will display a listing of the commands and their
    formats.  It is the same as the **?** command.

# EXAMPLES

- a span 19 20 5 1024 32768 2 'udplso node19.ohio.edu:5001'

    Declares a data interchange span between the local LTP engine and the remote
    engine (ION node) numbered 19.  There can be at most 20 concurrent sessions
    of export activity to this node.  Conversely, node 19 can
    have at most 5 concurrent sessions of export activity to the local node.
    Maximum segment size for this span is set to 1024 bytes, aggregation size
    threshold is 32768 bytes, aggregation time limit is 2 seconds, and the link
    service output task that is initiated when LTP is started on the local ION node
    will execute the _udplso_ program as indicated.

- m screening n

    Disables strict enforcement of the contact schedule.

# SEE ALSO

ltpadmin(1), udplsi(1), udplso(1)
