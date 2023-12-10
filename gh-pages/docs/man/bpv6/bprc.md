# NAME

bprc - Bundle Protocol management commands file

# DESCRIPTION

Bundle Protocol management commands are passed to **bpadmin** either in a file of
text lines or interactively at **bpadmin**'s command prompt (:).  Commands
are interpreted line-by line, with exactly one command per line.  The formats
and effects of the Bundle Protocol management commands are described below.

# GENERAL COMMANDS

- **?**

    The **help** command.  This will display a listing of the commands and their
    formats.  It is the same as the **h** command.

- **#**

    Comment line.  Lines beginning with **#** are not interpreted.

- **e** { 1 | 0 }

    Echo control.  Setting echo to 1 causes all output printed by bpadmin to be
    logged as well as sent to stdout.  Setting echo to 0 disables this behavior.

- **v** 

    Version number.  Prints out the version of ION currently installed and the
    crypto suite BP was compiled with.  HINT: combine with **e 1** command to log
    the version number at startup.

- **1**

    The **initialize** command.  Until this command is executed, Bundle Protocol
    is not in operation on the local ION node and most _bpadmin_ commands will
    fail.

- **r** '_command\_text_'

    The **run** command.  This command will execute _command\_text_ as if it
    had been typed at a console prompt.  It is used to, for example, run
    another administrative program.

- **s**

    The **start** command.  This command starts all schemes and all protocols
    on the local node.

- **m heapmax** _max\_database\_heap\_per\_acquisition_

    The **manage heap for bundle acquisition** command.  This command declares
    the maximum number of bytes of SDR heap space that will be occupied by any
    single bundle acquisition activity (nominally the acquisition of a single
    bundle, but this is at the discretion of the convergence-layer input task).
    All data acquired in excess of this limit will be written to a temporary file
    pending extraction and dispatching of the acquired bundle or bundles.  Default
    is the minimum allowed value (560 bytes), which is the approximate size of a
    ZCO file reference object; this is the minimum SDR heap space occupancy in the
    event that all acquisition is into a file.

- **m maxcount** _max\_value\_of\_bundle\_ID\_sequence\_nbr_

    The **manage maximum bundle ID sequence number** command.  This command sets
    the maximum value that will be assigned as the sequence number in a bundle ID
    for any bundle sourced at a node that lacks a synchronized clock (such that
    the creation time in the ID of every locally sourced bundle is always zero).
    The default value is -1, i.e., unlimited.

- **x**

    The **stop** command.  This command stops all schemes and all protocols
    on the local node.

- **w** { 0 | 1 | _activity\_spec_ }

    The **BP watch** command.  This command enables and disables production of
    a continuous stream of user-selected Bundle Protocol activity indication
    characters.  A watch parameter of "1" selects
    all BP activity indication characters; "0" de-selects all BP activity
    indication characters; any other _activity\_spec_ such as "acz~" selects
    all activity indication characters in the string, de-selecting all
    others.  BP will print each selected activity indication character to
    **stdout** every time a processing event of the associated type occurs:

    **a**	new bundle is queued for forwarding

    **b**	bundle is queued for transmission

    **c**	bundle is popped from its transmission queue

    **m**	custody acceptance signal is received

    **w**	custody of bundle is accepted

    **x**	custody of bundle is refused

    **y**	bundle is accepted upon arrival

    **z**	bundle is queued for delivery to an application

    **~**	bundle is abandoned (discarded) on attempt to forward it

    **!**	bundle is destroyed due to TTL expiration

    **&**	custody refusal signal is received

    **#**	bundle is queued for re-forwarding due to CL protocol failure

    **j**	bundle is placed in "limbo" for possible future re-forwarding

    **k**	bundle is removed from "limbo" and queued for re-forwarding

    **$**	bundle's custodial retransmission timeout interval expired

    Note that a slightly amended interpretation should be applied to watch
    characters printed in the course of multicast transmission.  The '~'
    character, meaning Abandoned (node did not forward this bundle), is printed
    by a node that is a leaf of the multicast tree, i.e., a node with no children;
    it cannot forward the bundle because it's got nobody to forward it to.  The
    '!' character, meaning Destroyed (node destroyed a physical copy of a bundle),
    is printed by a node that has forwarded copies of the bundle to all of its
    children and no longer needs to retain the original - so it does an immediate
    permanent bundle destruction just as if the bundle's time to live had expired.
    Neither condition is anomalous.

- **h**

    The **help** command.  This will display a listing of the commands and their
    formats.  It is the same as the **?** command.

# SCHEME COMMANDS

- **a scheme** _scheme\_name_ '_forwarder\_command_' '_admin\_app\_command_'

    The **add scheme** command.  This command declares an endpoint naming
    "scheme" for use in endpoint IDs, which are structured as URIs:
    _scheme\_name_:_scheme-specific\_part_.  _forwarder\_command_ will be
    executed when the scheme is started on this node, to initiate operation
    of a forwarding daemon for this scheme.  _admin\_app\_command_ will also
    be executed when the scheme is started on this node, to initiate
    operation of a daemon that opens a custodian endpoint identified within
    this scheme so that it can receive and process custody signals and bundle
    status reports.

- **c scheme** _scheme\_name_ '_forwarder\_command_' '_admin\_app\_command_'

    The **change scheme** command.  This command sets the indicated scheme's 
    _forwarder\_command_ and _admin\_app\_command_ to the strings provided
    as arguments.

- **d scheme** _scheme\_name_

    The **delete scheme** command.  This command deletes the scheme identified
    by _scheme\_name_.  The command will fail if any bundles identified in
    this scheme are pending forwarding, transmission, or delivery.

- **i scheme** _scheme\_name_

    This command will print information (number and commands) about
    the endpoint naming scheme identified by _scheme\_name_.

- **l scheme**

    This command lists all declared endpoint naming schemes.

- **s scheme** _scheme\_name_

    The **start scheme** command.  This command starts the forwarder and
    administrative endpoint tasks for the indicated scheme task on the local node.

- **x scheme** _scheme\_name_

    The **stop scheme** command.  This command stops the forwarder and
    administrative endpoint tasks for the indicated scheme task on the local node.

# ENDPOINT COMMANDS

- **a endpoint** _endpoint\_ID_ { q | x } \['_recv\_script_'\]

    The **add endpoint** command.  This command establishes a DTN endpoint named
    _endpoint\_ID_ on the local node.  The remaining parameters indicate
    what is to be done when bundles destined for this endpoint arrive at a time
    when no application has got the endpoint open for bundle reception.  If 'x',
    then such bundles are to be discarded silently and immediately.  If 'q',
    then such bundles are to be enqueued for later delivery and, if _recv\_script_
    is provided, _recv\_script_ is to be executed.

- **c endpoint** _endpoint\_ID_ { q | x } \['_recv\_script_'\]

    The **change endpoint** command.  This command changes the action that is
    to be taken when bundles destined for this endpoint arrive at a time
    when no application has got the endpoint open for bundle reception, as
    described above.

- **d endpoint** _endpoint\_ID_

    The **delete endpoint** command.  This command deletes the endpoint identified
    by _endpoint\_ID_.  The command will fail if any bundles are currently
    pending delivery to this endpoint.

- **i endpoint** _endpoint\_ID_

    This command will print information (disposition and script) about
    the endpoint identified by _endpoint\_ID_.

- **l endpoint**

    This command lists all local endpoints, regardless of scheme name.

# PROTOCOL COMMANDS

- **a protocol** _protocol\_name_ _payload\_bytes\_per\_frame_ _overhead\_bytes\_per\_frame_ \[_protocol\_class_\]

    The **add protocol** command.  This command establishes access to the named
    convergence layer protocol at the local node.  The _payload\_bytes\_per\_frame_
    and _overhead\_bytes\_per\_frame_ arguments are used in calculating the
    estimated transmission capacity consumption of each bundle, to aid in
    route computation and congestion forecasting.

    The optional _protocol\_class_ argument indicates the reliability of the
    protocol.  The value 1 indicates that the protocol natively supports bundle
    streaming; currently the only protocol in class 1 is BSSP.  The value 2
    indicates that the protocol performs no retransmission; an example is UDP.
    The value 8 (which is the default) indicates that the protocol detects data
    loss and automatically retransmits data accordingly; an example is TCP.
    Protocol class need not be specified when _protocol\_name_ is bssp, udp,
    tcp, stcp, brss, brsc, or ltp, as the protocol classes for these well-known
    protocols are hard-coded in ION.

- **d protocol** _protocol\_name_

    The **delete protocol** command.  This command deletes the convergence layer
    protocol identified by _protocol\_name_.  The command will fail if any ducts
    are still locally declared for this protocol.

- **i protocol** _protocol\_name_

    This command will print information about the convergence layer protocol
    identified by _protocol\_name_.

- **l protocol**

    This command lists all convergence layer protocols that can currently
    be utilized at the local node.

- **s protocol** _protocol\_name_

    The **start protocol** command.  This command starts all induct and outduct
    tasks for inducts and outducts that have been defined for the indicated
    CL protocol on the local node.

- **x protocol** _protocol\_name_

    The **stop protocol** command.  This command stops all induct and outduct
    tasks for inducts and outducts that have been defined for the indicated
    CL protocol on the local node.

# INDUCT COMMANDS

- **a induct** _protocol\_name_ _duct\_name_ '_CLI\_command_'

    The **add induct** command.  This command establishes a "duct" for reception
    of bundles via the indicated CL protocol.  The duct's data acquisition
    structure is used and populated by the "induct" task whose operation is
    initiated by _CLI\_command_ at the time the duct is started.

- **c induct** _protocol\_name_ _duct\_name_ '_CLI\_command_'

    The **change induct** command.  This command changes the command that is
    used to initiate operation of the induct task for the indicated duct.

- **d induct** _protocol\_name_ _duct\_name_

    The **delete induct** command.  This command deletes the induct identified
    by _protocol\_name_ and _duct\_name_.  The command will fail if any bundles
    are currently pending acquisition via this induct.

- **i induct** _protocol\_name_ _duct\_name_

    This command will print information (the CLI command) about
    the induct identified by _protocol\_name_ and _duct\_name_.

- **l induct** \[_protocol\_name_\]

    If _protocol\_name_ is specified, this command lists all inducts
    established locally for the indicated CL protocol.  Otherwise it lists
    all locally established inducts, regardless of protocol.

- **s induct** _protocol\_name_ _duct\_name_

    The **start induct** command.  This command starts the indicated induct 
    task as defined for the indicated CL protocol on the local node.

- **x induct** _protocol\_name_ _duct\_name_

    The **stop induct** command.  This command stops the indicated induct 
    task as defined for the indicated CL protocol on the local node.

# OUTDUCT COMMANDS

- **a outduct** _protocol\_name_ _duct\_name_ '_CLO\_command_' \[_max\_payload\_length_\]

    The **add outduct** command.  This command establishes a "duct" for transmission
    of bundles via the indicated CL protocol.  The duct's data transmission
    structure is serviced by the "outduct" task whose operation is
    initiated by _CLO\_command_ at the time the duct is started.  A value of
    zero for _max\_payload\_length_ indicates that bundles of any size can be
    accommodated; this is the default.

- **c outduct** _protocol\_name_ _duct\_name_ '_CLO\_command_' \[_max\_payload\_length_\]

    The **change outduct** command.  This command sets new values for the indicated
    duct's payload size limit and the command that is used to initiate operation of
    the outduct task for this duct.

- **d outduct** _protocol\_name_ _duct\_name_

    The **delete outduct** command.  This command deletes the outduct identified
    by _protocol\_name_ and _duct\_name_.  The command will fail if any bundles
    are currently pending transmission via this outduct.

- **i outduct** _protocol\_name_ _duct\_name_

    This command will print information (the CLO command) about
    the outduct identified by _protocol\_name_ and _duct\_name_.

- **l outduct** \[_protocol\_name_\]

    If _protocol\_name_ is specified, this command lists all outducts
    established locally for the indicated CL protocol.  Otherwise it lists
    all locally established outducts, regardless of protocol.

- **s outduct** _protocol\_name_ _duct\_name_

    The **start outduct** command.  This command starts the indicated outduct 
    task as defined for the indicated CL protocol on the local node.

- **x outduct** _protocol\_name_ _duct\_name_

    The **stop outduct** command.  This command stops the indicated outduct 
    task as defined for the indicated CL protocol on the local node.

# EGRESS PLAN COMMANDS

- **a plan** _endpoint\_name_ \[_transmission\_rate_\]

    The **add plan** command.  This command establishes an egress plan governing
    transmission to the neighboring node\[s\] identified by _endpoint\_name_.  The
    plan is functionally enacted by a bpclm(1) daemon dedicated to managing
    bundles queued for transmission to the indicated neighboring node\[s\].

    NOTE that these "plan" commands supersede and generalize the egress plan
    commands documented in the ipnrc(5) and dtn2rc(5) man pages, which are
    retained for backward compatibility.  **The syntax of the egress plan commands
    consumed by bpadmin is DIFFERENT from that of the commands consumed by
    ipnadmin and dtn2admin.**  The _endpoint\_name_ identifying
    an egress plan is normally the node ID for a single node but may instead
    be "wild-carded".  That is, when the last character of an endpoint name
    ID is either '\*' or '~' (these two wild-card characters are equivalent
    for this purpose), the plan applies to all nodes whose IDs are identical
    to the wild-carded node name up to the wild-card character.  For example,
    a bundle whose destination EID name is "dtn://foghorn" would be routed
    by plans citing the following node IDs: "dtn://foghorn", "dtn://fogh\*",
    "dtn://fog~", "//\*".  When multiple plans are all applicable to the same
    destination EID, the one citing the longest (i.e., most narrowly targeted)
    node ID will be applied.

    An egress plan may direct that bundles queued for transmission to the
    node\[s\] matching _endpoint\_name_ be transmitted using one of the
    convergence-layer protocol "outducts" that have been attached to the
    plan, or instead that those bundles be routed to some other "gateway"
    endpoint (resulting in transmission according to some other egress
    plan).  In the event that both a gateway endpoint and one or more
    outducts have been declared for a given plan, the gateway declaration prevails.

    A _transmission\_rate_ may be asserted for an egress plan; this rate is
    used as the basis for transmission rate control in the absence of applicable
    contacts (in the node's contact plan, as per ionrc(5)).  A transmission
    rate of zero (absent applicable contacts) disables rate control completely;
    this is the default.

- **c plan** _endpoint\_name_ \[_transmission\_rate_\]

    The **change plan** command.  This command sets a new value for the indicated
    plan's transmission rate.

- **d plan** _endpoint\_name_

    The **delete plan** command.  This command deletes the outduct identified
    by _endpoint\_name_.  The command will fail if any bundles are currently
    pending transmission per this plan.

- **i plan** _endpoint\_name_

    This command will print information (the transmission rate) about
    the plan identified by _endpoint\_name_.

- **l plan**

    This command lists all locally established egress plans.

- **s plan** _endpoint\_name_

    The **start plan** command.  This command starts the bpclm(1) task for
    the indicated egress plan.

- **x plan** _endpoint\_name_

    The **stop plan** command.  This command stops the bpclm(1) task for
    the indicated egress plan. 

- **b plan** _endpoint\_name_

    The **block plan** command.  This command disables transmission of bundles
    queued for transmission to the indicated node and reforwards all non-critical
    bundles currently queued for transmission to this node.  This may result in
    some or all of these bundles being enqueued for transmission (actually just
    retention) to the pseudo-node "limbo".

- **u plan** _endpoint\_name_

    The **unblock plan** command.  This command re-enables transmission of
    bundles to the indicated node and reforwards all bundles in "limbo"
    in the hope that the unblocking of this egress plan will enable some of them
    to be transmitted.

- **g plan** _endpoint\_name_ _gateway\_endpoint\_name_

    The **declare gateway** command.  This command declares the name of the
    endpoint to which bundles queued for transmission to the node\[s\]
    identified by _endpoint\_name_ must be re-routed.  Declaring
    _gateway\_endpoint\_name_ to be the zero-length string "''" disables
    re-routing: bundles will instead be transmitted using the plan's attached
    convergence-layer protocol outduct\[s\].

- **a planduct** _endpoint\_name_ _protocol\_name_ _duct\_name_

    The **attach outduct** command.  This command declares that the indicated
    convergence-layer protocol outduct is now a viable device for transmitting
    bundles to the node\[s\] identified by _endpoint\_name_.

- **d planduct** _protocol\_name_ _duct\_name_

    The **detach outduct** command.  This command declares that the indicated
    convergence-layer protocol outduct is no longer a viable device for
    transmitting bundles to the node\[s\] to which it is currently assigned.

# EXAMPLES

- a scheme ipn 'ipnfw' 'ipnadminep'

    Declares the "ipn" scheme on the local node.

- a protocol udp 1400 100 16384

    Establishes access to the "udp" convergence layer protocol on the local
    node, estimating the number of payload bytes per ultimate (lowest-layer)
    frame to be 1400 with 100 bytes of total overhead (BP, UDP, IP, AOS) per
    lowest-layer frame, and setting the default nominal data rate to be 16384
    bytes per second.

- r 'ipnadmin flyby.ipnrc'

    Runs the administrative program _ipnadmin_ from within _bpadmin_.

# SEE ALSO

bpadmin(1), ipnadmin(1), dtn2admin(1)
