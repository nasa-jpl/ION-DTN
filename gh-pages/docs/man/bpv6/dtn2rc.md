# NAME

dtn2rc - "dtn" scheme configuration commands file

# DESCRIPTION

"dtn" scheme configuration commands are passed to **dtn2admin** either in a
file of text lines or interactively at **dtn2admin**'s command prompt (:).
Commands are interpreted line-by line, with exactly one command per line.

"dtn" scheme configuration commands establish static routing rules
for forwarding bundles to "dtn"-scheme destination endpoints, identified by
node ID.  (Each node ID is simply a BP endpoint ID.)

Static routes are expressed as **plan**s in the "dtn"-scheme routing database.
A plan that is established for a given node name associates a routing
**directive** with the named node.  Each directive is a string of one of
two possible forms:

> f _endpoint\_ID_

...or...

> x _protocol\_name_/_outduct\_name_

The former form signifies that the bundle is to be forwarded to the indicated
endpoint, requiring that it be re-queued for processing by the forwarder
for that endpoint (which might, but need not, be identified by another
"dtn"-scheme endpoint ID).  The latter form signifies that the bundle is
to be queued for transmission via the indicated convergence layer protocol
outduct.

The node IDs cited in dtn2rc plans may be "wild-carded".  That is, when
the last character of a node ID is either '\*' or '~' (these two wild-card
characters are equivalent for this purpose), the plan applies to all nodes
whose IDs are identical to the wild-carded node name up to the wild-card
character.  For example, a bundle whose destination EID name is "dtn://foghorn"
would be routed by plans citing the following node IDs: "dtn://foghorn",
"dtn://fogh\*", "dtn://fog~", "//\*".  When multiple plans are all applicable
to the same destination EID, the one citing the longest (i.e., most narrowly
targeted) node ID will be applied.

The formats and effects of the DTN scheme configuration commands are
described below.

# GENERAL COMMANDS

- **?**

    The **help** command.  This will display a listing of the commands and their
    formats.  It is the same as the **h** command.

- **#**

    Comment line.  Lines beginning with **#** are not interpreted.

- **e** { 1 | 0 }

    Echo control.  Setting echo to 1 causes all output printed by dtn2admin to be
    logged as well as sent to stdout.  Setting echo to 0 disables this behavior.

- **v** 

    Version number.  Prints out the version of ION currently installed.  HINT:
    combine with **e 1** command to log the version number at startup.

- **h**

    The **help** command.  This will display a listing of the commands and their
    formats.  It is the same as the **?** command.

# PLAN COMMANDS

- **a plan** _node\_ID_ _directive_ \[_nominal\_xmit\_rate_\]

    The **add plan** command.  This command establishes a static route for
    the bundles destined for the node(s) identified by _node\_ID_.  The
    _nominal\_xmit\_rate_ is the assumed rate of transmission to this node
    in the absence of contact plan information.  A _nominal\_data\_rate_ of
    zero (the default) in the absence of contact plan information completely
    disables rate control.

    **Note that the plan commands consumed by dtn2admin are a simplified
    shortcut for submitting plan commands as consumed by bpadmin (see bprc(5)).
    The syntax of these commands is DIFFERENT from that of the more general
    and more powerful bpadmin commands.**

- **c plan** _node\_ID_ \[_directive_\] \[_nominal\_xmit\_rate_\]

    The **change plan** command.  This command revises the _directive_ and/or
    _nominal\_data\_rate_ of the static route for the node(s) identified by
    _node\_ID_.

- **d plan** _node\_ID_

    The **delete plan** command.  This command deletes the static route for
    the node(s) identified by _node\_ID_.

- **i plan** _node\_ID_

    This command will print information about the static route for the node(s)
    identified by _node\_ID_.

- **l plan**

    This command lists all static routes established in the DTN database for
    the local node.

# EXAMPLES

- a plan dtn://bbn2 f ipn:8.41

    Declares a static route from the local node to node "//bbn2".  Any bundle
    destined for any endpoint whose node name is "//bbn2" will be forwarded
    to endpoint "ipn:8.41".

- a plan dtn://mitre1\* x ltp/6

    Declares a static route from the local node to node "//mitre1".  Any bundle
    destined for any endpoint whose node ID begins with "mitre1" will
    be queued for transmission on LTP outduct 6.

# SEE ALSO

dtn2admin(1)
