# NAME

ipnrc - IPN scheme configuration commands file

# DESCRIPTION

IPN scheme configuration commands are passed to **ipnadmin** either in a file of
text lines or interactively at **ipnadmin**'s command prompt (:).  Commands
are interpreted line-by line, with exactly one command per line.

IPN scheme configuration commands (a) establish egress plans for direct
transmission to neighboring nodes that are members of endpoints identified
in the "ipn" URI scheme and (b) establish static default routing rules
for forwarding bundles to specified destination nodes.

The egress **plan** established for a given node associates a **duct expression**
with that node.  Each duct expression is a string of the form
"_protocol\_name_/_outduct\_name_" signifying that the bundle is to be
queued for transmission via the indicated convergence layer protocol outduct.

Note that egress plans **must** be established for all neighboring nodes,
regardless of whether or not contact graph routing is used for computing
dynamic routes to distant nodes.  This is by definition: if there isn't
an egress plan to a node, it can't be considered a neighbor.

Static default routes are declared as **exits** in the ipn-scheme routing
database.  An exit is a range of node numbers identifying a set of nodes
for which defined default routing behavior is established.  Whenever a
bundle is to be forwarded to a node whose number is in the exit's node
number range **and** it has not been possible to compute a dynamic route
to that node from the contact schedules that have been provided to the
local node **and** that node is not a neighbor to which the bundle can
be directly transmitted, BP will forward the bundle to the **gateway** node
associated with this exit.  The gateway node for any exit is identified
by an endpoint ID, which might or might not be an ipn-scheme EID; regardless,
directing a bundle to the gateway for an exit causes the bundle to be
re-forwarded to that intermediate destination endpoint.  Multiple exits
may encompass the same node number, in which case the gateway associated
with the most restrictive exit (the one with the smallest range) is
always selected.

**Note** that "exits" were termed "groups" in earlier versions of ION.  The
term "exit" has been adopted instead, to minimize any possible confusion
with multicast groups.  To protect backward compatibility, the keyword
"group" continues to be accepted by ipnadmin as an alias for the new keyword
"exit", but the older terminology is deprecated.

The formats and effects of the IPN scheme configuration commands are
described below.

# GENERAL COMMANDS

- **?**

    The **help** command.  This will display a listing of the commands and their
    formats.  It is the same as the **h** command.

- **#**

    Comment line.  Lines beginning with **#** are not interpreted.

- **e** { 1 | 0 }

    Echo control.  Setting echo to 1 causes all output printed by ipnadmin to be
    logged as well as sent to stdout.  Setting echo to 0 disables this behavior.

- **v** 

    Version number.  Prints out the version of ION currently installed.  HINT:
    combine with **e 1** command to log the version number at startup.

- **h**

    The **help** command.  This will display a listing of the commands and their
    formats.  It is the same as the **?** command.

# PLAN COMMANDS

- **a plan** _node\_nbr_ _duct\_expression_ \[_nominal\_data\_rate_\]

    The **add plan** command.  This command establishes an egress plan for
    the bundles that must be transmitted to the neighboring node identified
    by _node\_nbr_.  The _nominal\_data\_rate_ is the assumed rate of
    transmission to this node in the absence of contact plan information.
    A _nominal\_data\_rate_ of zero (the default) in the absence of contact
    plan information completely disables rate control.

    **Note that the plan commands consumed by ipnadmin are a simplified
    shortcut for submitting plan commands as consumed by bpadmin (see bprc(5)).
    The syntax of these commands is DIFFERENT from that of the more general
    and more powerful bpadmin commands.**

- **c plan** _node\_nbr_ _nominal\_data\_rate_

    The **change plan** command.  This command changes the nominal data rate
    for the indicated plan.

- **d plan** _node\_nbr_

    The **delete plan** command.  This command deletes the egress plan
    for the node identified by _node\_nbr_.

- **i plan** _node\_nbr_

    This command will print information about the egress plan for the node
    identified by _node\_nbr_.

- **l plan**

    This command lists all egress plans established in the IPN database for the
    local node.

# EXIT COMMANDS

- **a exit** _first\_node\_nbr_ _last\_node\_nbr_ _gateway\_endpoint\_ID_

    The **add exit** command.  This command establishes an "exit" for static 
    default routing as described above.

- **c exit** _first\_node\_nbr_ _last\_node\_nbr_ _gateway\_endpoint\_ID_

    The **change exit** command.  This command changes the gateway node
    number for the exit identified by _first\_node\_nbr_ and _last\_node\_nbr_ .

- **d exit** _first\_node\_nbr_ _last\_node\_nbr_

    The **delete exit** command.  This command deletes the exit identified
    by _first\_node\_nbr_ and _last\_node\_nbr_.

- **i exit** _first\_node\_nbr_ _last\_node\_nbr_

    This command will print information (the gateway endpoint ID) about the
    exit identified by _first\_node\_nbr_ and _last\_node\_nbr_.

- **l exit**

    This command lists all exits defined in the IPN database for the local node.

# OVERRIDE COMMANDS

- **a rtovrd** _data\_label_ _dest\_node\_nbr_ _source\_node\_nbr_ _neighbor_

    The **add rtovrd** command.  This command cause bundles characterized by
    _data\_label_, _dest\_node\_nbr_ ("all other destinations" if this node
    number is zero) and _source\_node\_nbr_ ("all other sources" if this node
    number is zero) to be forwarded to _neighbor_.  If _neighbor_ is zero,
    the override will be "learned" by ION: the neighbor selected for this
    bundle, by whatever means, becomes the override for all subsequent matching
    bundles.

- **c rtovrd** _data\_label_ _dest\_node\_nbr_ _source\_node\_nbr_ _neighbor_

    The **change rtovrd** command.  This command changes the override neighbor
    for the override identified by _data\_label_, _dest\_node\_nbr_, and
    _source\_node\_nbr_.  To cause ION to forget the override, use -1 as
    _neighbor_.

- **a cosovrd** _data\_label_ _dest\_node\_nbr_ _source\_node\_nbr_ _priority_ _ordinal_

    The **add cosovrd** command.  This command cause bundles characterized by
    _data\_label_, _dest\_node\_nbr_ ("all other destinations" if this node
    number is zero) and _source\_node\_nbr_ ("all other sources" if this node
    number is zero) to have their effective class of service (priority and
    ordinal) changed as noted.

- **c cosovrd** _data\_label_ _dest\_node\_nbr_ _source\_node\_nbr_ _priority_ _ordinal_

    The **change cosovrd** command.  This command changes the override priority
    and ordinal for the override identified by _data\_label_, _dest\_node\_nbr_,
    and _source\_node\_nbr_.  To cause ION to forget the override, use -1 as
    _priority_.

- **d ovrd** _data\_label_ _dest\_node\_nbr_ _source\_node\_nbr_

    The **delete override** command.  This command deletes all overrides identified
    by _data\_label_, _dest\_node\_nbr_, and _source\_node\_nbr_.

- **i ovrd** _data\_label_ _dest\_node\_nbr_ _source\_node\_nbr_

    This command will print information for all overrides identified
    by _data\_label_, _dest\_node\_nbr_, and _source\_node\_nbr_.

- **l ovrd**

    This command lists all overrides defined in the IPN database for the local node.

# EXAMPLES

- a plan 18 ltp/18

    Declares the egress plan to use for transmission from the local node to
    neighboring node 18.  By default, any bundle for which the computed "next
    hop" node is node 18 will be queued for transmission on LTP outduct 18.

- a exit 1 999 dtn://stargate

    Declares a default route for bundles destined for all nodes whose numbers
    are in the range 1 through 999 inclusive: absent any other routing decision,
    such bundles are to be forwarded to "dtn://stargate".

# SEE ALSO

ipnadmin(1)
