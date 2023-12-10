# NAME

ionrc - ION node management commands file

# DESCRIPTION

ION node management commands are passed to **ionadmin** either in a file of
text lines or interactively at **ionadmin**'s command prompt (:).  Commands
are interpreted line-by line, with exactly one command per line.  The formats
and effects of the ION node management commands are described below.

# TIME REPRESENTATION

For many ION node management commands, time values must be passed as
arguments.  Every time value may be represented in either of two formats.
Absolute time is expressed as: 

> _yyyy_/_mm_/_dd_-_hh_:_mm_:_ss_

Relative time (a number of seconds following the current _reference time_,
which defaults to the current time at the moment _ionadmin_ began execution
but which can be overridden by the **at** command described below) is expressed
as:

> \+_ss_

# COMMANDS

- **?**

    The **help** command.  This will display a listing of the commands and their
    formats.  It is the same as the **h** command.

- **#**

    Comment line.  Lines beginning with **#** are not interpreted.

- **e** { 1 | 0 }

    Echo control.  Setting echo to 1 causes all output printed by ionadmin to
    be logged as well as sent to stdout.  Setting echo to 0 disables this behavior.

- **v** 

    Version number.  Prints out the version of ION currently installed.  HINT:
    combine with **e 1** command to log the version number at startup.

- **1** _node\_number_ \[ { _ion\_config\_filename_ | '.' | '' } \]

    The **initialize** command.  Until this command is executed, the local ION
    node does not exist and most _ionadmin_ commands will fail.

    The command configures the local node to be identified by _node\_number_, a
    CBHE node number which uniquely identifies the node in the delay-tolerant
    network.  It also configures ION's data space (SDR) and shared working-memory
    region.  For this purpose it uses a set of default settings if no argument
    follows _node\_number_ or if the argument following _node\_number_ is '';
    otherwise it uses the configuration settings found in a configuration
    file.  If configuration file name '.' is provided, then the configuration
    file's name is implicitly "_hostname_.ionconfig"; otherwise,
    _ion\_config\_filename_ is taken to be the explicit configuration file name.
    Please see ionconfig(5) for details of the configuration settings.

    For example:

            1 19 ''

    would initialize ION on the local computer, assigning the local ION node the
    node number 19 and using default values to configure the data space and
    shared working-memory region.

- **@** _time_

    The **at** command.  This is used to set the reference time that will be
    used for interpreting relative time values from now until the next revision
    of reference time.  Note that the new reference time can be a relative time,
    i.e., an offset beyond the current reference time.

- **^** _region\_number_

    The **region** command.  This is used to select the region to which
    all ensuing "contact" operations (until this execution of ionadmin
    terminates, or until the next **region** command is processed) pertain.
    A **region** is an arbitarily managed set of nodes that customarily are able
    to use contact graph routing to compute forwarding routes among themselves,
    and which consequently share a common contact plan.  As such, there is a
    one-to-one correspondence between regions and contact plans, so in
    effect the **region** command is used to switch between contact plans.
    Regions are notionally quite small sets (on the order of 16-32 nodes)
    because contact graph routing is computationally intensive.

    Information regarding up to two (2) regions may be managed at any single node.

    By default, region number 1 (the "universal" region) is selected.

- **!** \[0 | 1\]

    "Announce" control.  Setting the announce flag to 1 causes contact plan
    updates (contact add/change/delete, range add/delete) to be multicast to
    all other nodes in the region in addition to being processed at the local
    node.  Setting the announce flag to 0 disables this behavior.

- **a contact** _start\_time_ _stop\_time_ _source\_node_ _dest\_node_ _xmit\_data\_rate_ \[_confidence_\]

    The **add contact** command.  This command schedules a period of data
    transmission from _source\_node_ to _dest\_node_.  The period of
    transmission will begin at _start\_time_ and end at _stop\_time_,
    and the rate of data transmission will be _xmit\_data\_rate_ bytes/second.
    Our confidence in the contact defaults to 1.0, indicating that the contact
    is scheduled - not that non-occurrence of the contact is impossible, just
    that occurrence of the contact is planned and scheduled rather than merely
    imputed from past node behavior.  In the latter case, _confidence_
    indicates our estimation of the likelihood of this potential contact.

    The period of time between the start and stop times of a contact is termed
    the contact's "interval".  The intervals of scheduled contacts are not
    allowed to overlap.

    Commands pertaining to three different types of contact can be intermixed
    within an .ionrc file that defines a contact plan.

    - 1  Registration

        When _start\_time_ is "-1", the contact signifies the "registration" of a
        node in the region corresponding to the contact plan of which this contact is
        a part.  In this case, _source\_node_ and _dest\_node_ must be identical and
        non-zero.  A registration contact simply affirms the source node's permanent
        membership in this region, persisting even during periods when the node
        is able neither to send nor to receive data.  When inserted into the
        contact plan, the contact's start and stop times are both automatically
        set to the maximum POSIX time, its data rate is set to zero, and its
        confidence value is set to 1.0.

    - 2  Hypothetical

        When _stop\_time_ is "0", the contact is "hypothetical".  A hypothetical
        contact is an anticipated opportunity for the local node to transmit data
        to, or receive data from, some potentially neighboring node in the same
        region.  The nature of that contact is completely unknown; if and when
        the contact occurs, the hypothetical contact will be transformed into
        a "discovered" contact for the duration of the opportunity, after which
        it will revert to being hypothetical.  _source\_node_ and _dest\_node_ must
        **NOT** be identical, and one or the other must identify the local node.  When
        inserted into the contact plan, the contact's start time is automatically
        set to zero, its stop time is set to the maximum POSIX time, its data rate
        is set to zero, and its confidence value is set to 0.0.

    - 3  Scheduled

        Otherwise, the contact is "scheduled".  A scheduled contact is a managed
        opportunity to transmit data between nodes, as inferred (for example)
        from a spacecraft or ground station operating plan.  _start\_time_ must
        be less than _stop\_time_ and _data\_rate_ and _confidence_ must both
        be greater than zero.

- **c contact** _start\_time_ _source\_node_ _dest\_node_ _xmit\_data\_rate_ \[_confidence_\]

    The **change contact** command.  This command changes the data transmission
    rate and possibly our level of confidence in the scheduled period of data
    transmission from _source\_node_ to _dest\_node_ starting at _start\_time_.
    Registration and hypothetical contacts cannot be changed.

- **d contact** _start\_time_ _source\_node_ _dest\_node_

    The **delete contact** command.  This command deletes the contact from
    _source\_node_ to _dest\_node_ starting at _start\_time_.  To delete
    all scheduled contacts between some pair of nodes, use '\*' as _start\_time_.
    To delete a registration contact, use "-1" as _start\_time_.  To delete
    a hypothetical contact, use "0" as _start\_time_.

- **i contact** _start\_time_ _source\_node_ _dest\_node_

    This command will print information (stop time, data rate, confidence) about
    the scheduled period of transmission from _source\_node_ to _dest\_node_
    that starts at _start\_time_.

- **l contact**

    This command lists all contacts in the contact plan for the selected region.

- **b contact**

    The **brief contacts** command.  This command writes a file of commands
    that will recreate the current list of contacts, for the selected region,
    in the node's ION database.  The name of the file will be
    "contacts._region\_number_.ionrc".

- **a range** _start\_time_ _stop\_time_ _one\_node_ _the\_other\_node_ _distance_

    The **add range** command.  This command predicts a period of time during
    which the distance from _one\_node_ to _the\_other\_node_ will be constant
    to within one light second.  The period will begin at _start\_time_ and
    end at _stop\_time_, and the distance between the nodes during that time
    will be _distance_ light seconds.

    **NOTE** that the ranges declared by these commands are directional.  ION
    does not automatically assume that the distance from node A to node B is
    the same as the distance from node B to node A.  While this symmetry is
    certainly true of geographic distance, the range that concerns ION is the
    latency in propagating a signal from one node to the other; this latency may
    be different in different directions because (for example) the signal from
    B to A might need to be forwarded along a different convergence-layer network
    path from the one used for the signal from A to B.

    For this reason, the range identification syntax for this command is
    asymmetrical: ION interprets an **add range** command in which the node number
    of the first cited node is numerically less than that of the second cited node
    as implicitly declaring the same distance in the reverse direction (the
    normal case)  **UNLESS** a second range command is present that cites the
    same two nodes in the opposite order, which overrides the implicit
    declaration.  A range command in which the node number of the first
    cited node is numerically greater than that of the second cited node
    implies **ABSOLUTELY NOTHING** about the distance in the reverse direction.

- **d range** _start\_time_ _one\_node_ _the\_other\_node_

    The **delete range** command.  This command deletes the predicted period of
    constant distance between _one\_node_ and _the\_other\_node_ starting
    at _start\_time_.  To delete all ranges between some pair of nodes,
    use '\*' as _start\_time_.

    **NOTE** that the range identification syntax for this command is
    asymmetrical, much as described for the **add range** command described
    above.  ION interprets a **delete range** command in which the node number of
    the first cited node is numerically less than that of the second cited node
    as implicitly requesting deletion of the range in the opposite direction
    as well.  A **delete range** command in which the node number of the first
    cited node is numerically greater than that of the second cited node
    deletes only the range in that direction; the asserted range in the
    opposite direction is unaffected.

- **i range** _start\_time_ _one\_node_ _the\_other\_node_

    This command will print information (the stop time and range) about the
    predicted period of constant distance between _one\_node_ and _the\_other\_node_
    that starts at _start\_time_.

- **l range**

    This command lists all predicted periods of constant distance.

- **b range**

    The **brief ranges** command.  This command writes a file of commands that
    will recreate the current list of ranges in the node's ION database.  The
    file's name will be "ranges.ionrc".

- **m utcdelta** _local\_time\_sec\_after\_UTC_

    This management command sets ION's understanding of the current difference
    between correct UTC time and the localtime equivalent of the current calendar
    (i.e., Unix epoch) time as reported by the clock for the local ION node's
    computer.  This delta is automatically applied to locally
    obtained time values whenever ION needs to know the current time.  For
    machines that are synchronized by NTP, the value of this delta should be 0,
    the default.

    Note that the purpose of the UTC delta is not to correct for time zone
    differences (which operating systems often do natively) but rather to
    compensate for error (drift) in clocks, particularly spacecraft clocks.
    The hardware clock on a spacecraft might gain or lose a few seconds every
    month, to the point at which its understanding of the current time - as
    reported out by the operating system and converted to UTC - might
    differ significantly from the actual value of UTC as reported by authoritative
    clocks on Earth.  To compensate for this difference without correcting the
    clock itself (which can be difficult and dangerous), ION simply adds the UTC
    delta to the calendar time reported by the operating system.

    Note that this means that setting the UTC delta is not a one-time node
    configuration activity but rather an ongoing node administration chore,
    because a drifting clock typically keeps on drifting.

- **m clockerr** _known\_maximum\_clock\_error_

    This management command sets ION's understanding of the accuracy of the
    scheduled start and stop times of planned contacts, in seconds.  The
    default value is 1.  When revising local data transmission and reception
    rates, _ionadmin_ will adjust contact start and stop times by this
    interval to be sure not to send bundles that arrive before the neighbor
    expects data arrival or to discard bundles that arrive slightly before
    they were expected.

- **m clocksync** \[ { 1 | 0 } \]

    This management command reports whether or not the computer on which
    the local ION node is running has a synchronized clock, as discussed in
    the description of the ionClockIsSynchronized() function (ion(3)).

    If a Boolean argument is provided when the command is executed, the
    characterization of the machine's clock is revised to conform with
    the asserted value.  The default value is 1.

- **m production** _planned\_data\_production\_rate_

    This management command sets ION's expectation of the mean rate of continuous
    data origination by local BP applications throughout the period of time
    over which congestion forecasts are computed, in bytes per second.  For
    nodes that function only as routers this variable will normally be zero.  A
    value of -1, which is the default, indicates that the rate of local data
    production is unknown; in that case local data production is not considered
    in the computation of congestion forecasts.

- **m consumption** _planned\_data\_consumption\_rate_

    This management command sets ION's expectation of the mean rate of continuous
    data delivery to local BP applications throughout the period of time
    over which congestion forecasts are computed, in bytes per second.  For
    nodes that function only as routers this variable will normally be zero.  A
    value of -1, which is the default, indicates that the rate of local data
    consumption is unknown; in that case local data consumption is not considered
    in the computation of congestion forecasts.

- **m inbound** _heap\_occupancy\_limit_ \[_file\_system\_occupancy\_limit_\]

    This management command sets the maximum number of megabytes of storage space
    in ION's SDR non-volatile heap, and/or in the local file system, that can be
    used for the storage of inbound zero-copy objects.  A value of -1 for either
    limit signifies "leave unchanged".  The default heap limit is 30% of the SDR
    data space's total heap size.  The default file system limit is 1 Terabyte.

- **m outbound** _heap\_occupancy\_limit_ \[_file\_system\_occupancy\_limit_\]

    This management command sets the maximum number of megabytes of storage space
    in ION's SDR non-volatile heap, and/or in the local file system, that can be
    used for the storage of outbound zero-copy objects.  A value of -1 for either
    limit signifies "leave unchanged".  The default heap limit is 30% of the SDR
    data space's total heap size.  The default file system limit is 1 Terabyte.

- **m search** _max\_free\_blocks\_to\_search\_through_

    This management command sets the limit on the number of free blocks
    the heap space allocation function will search through in the nominal
    free space bucket, looking for a sufficiently large free block, before
    giving up and switching to the next higher non-empty free space bucket.
    The default value is 0, which yields the highest memory management speed
    but may leave heap space under-utilized: data objects may be stored in
    unnecessarily large heap space blocks.  Increasing the value of the heap
    space search limit will manage space more efficiently - with less waste -
    but more slowly.

- **m horizon** { 0 | _end\_time\_for\_congestion\_forecasts_ }

    This management command sets the end time for computed congestion
    forecasts.  Setting congestion forecast horizon to zero sets the congestion
    forecast end time to infinite time in the future: if there is any predicted
    net growth in bundle storage space occupancy at all, following the end of
    the last scheduled contact, then eventual congestion will be predicted.  The
    default value is zero, i.e., no end time.

- **m alarm** '_congestion\_alarm\_command_'

    This management command establishes a command which will automatically be
    executed whenever _ionadmin_ predicts that the node will become congested
    at some future time.  By default, there is no alarm command.

- **m usage**

    This management command simply prints ION's current data space occupancy
    (the number of megabytes of space in the SDR non-volatile heap and file system
    that are occupied by inbound and outbound zero-copy objects), the total
    zero-copy-object space occupancy ceiling, and the maximum level
    of occupancy predicted by the most recent _ionadmin_ congestion forecast
    computation.

- **r** '_command\_text_'

    The **run** command.  This command will execute _command\_text_ as if it
    had been typed at a console prompt.  It is used to, for example, run
    another administrative program.

- **s**

    The **start** command.  This command starts the _rfxclock_ task on the local
    ION node.

- **x**

    The **stop** command.  This command stops the _rfxclock_ task on the local
    ION node.

- **h**

    The **help** command.  This will display a listing of the commands and their
    formats.  It is the same as the **?** command.

# EXAMPLES

- @ 2008/10/05-11:30:00

    Sets the reference time to 1130 (UTC) on 5 October 2008.

- a range +1 2009/01/01-00:00:00 1 2 12

    Predicts that the distance between nodes 1 and 2 (endpoint IDs
    ipn:1.0 and ipn:2.0) will remain constant at 12 light seconds over the
    interval that begins 1 second after the reference time and ends at the
    end of calendar year 2009.

- a contact +60 +7260 1 2 10000

    Schedules a period of transmission at 10,000 bytes/second from node 1 to
    node 2, starting 60 seconds after the reference time and ending exactly
    two hours (7200 seconds) after it starts.

# SEE ALSO

ionadmin(1), rfxclock(1), ion(3)
