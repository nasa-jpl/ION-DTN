# NAME

biberc - BIBE configuration commands file

# DESCRIPTION

BIBE configuration commands are passed to **bibeadmin** either in a file of
text lines or interactively at **bibeadmin**'s command prompt (:).  Commands
are interpreted line-by line, with exactly one command per line.

BIBE configuration commands establish the parameters governing transmission
of BIBE PDUs to specified **peer** nodes: anticipated delivery latency in the
forward direction, anticipated delivery latency in the return direction,
TTL for BIBE PDUs, priority for BIBE PDUs, ordinal priority for BIBE PDUs
in the event that priority is Expedited, and (optionally) data label for
BIBE PDUs.  As such, they configure BIBE convergence-layer adapter (**bcla**)
structures.

The formats and effects of the BIBE configuration commands are described below.

NOTE: in order to cause bundles to be transmitted via BIBE:

- **Plan**

    Remember that BIBE is a convergence-layer protocol; as such, it operates
    between two nodes that are topologically adjacent in a BP network (but in
    this case the BP network within which the nodes are topologically adjacent
    is an overlay on top of the real BP network).  Since the sending and
    receiving nodes are topologically adjacent they are neighbors: the sending
    node MUST have an egress plan for transmission to the receiving (that is,
    **peer**) node, and there MUST be a BIBE outduct attached to that plan.

- **Routing**

    In order to compel bundles bound for some destination node to be forwarded
    via the BIBE peer node rather than over some other route computed by CGR,
    you have to override CGR routing for that bundle.  The way to do this is
    to (a) tag the bundle with data label X (in ancillary data) and (b) use
    ipnadmin to establish at the sending node a _routing override_ that
    coerces all bundles with data label X to be sent directly to the peer node.

    If the peer node happens to be a true BP neighbor as well - that is,
    there is also a non-BIBE outduct attached to the egress plan for
    transmission to that node - then you additionally need to tell the
    egress plan management daemon (bpclm) for that node which bundles
    need to be forwarded using the BIBE outduct rather than the non-BIBE
    outduct.  The way to do this is to use ipnadmin to establish at the
    sending node a _class-of-service override_ that locally and temporarily
    OR's the BP\_BIBE\_REQUESTED flag (32) to the quality-of-service flags
    of any bundle tagged with data label X.

- **Quality of Service**

    If you want custody transfer to be invoked for each BIBE transmission of a
    bundle from the sending node to the peer node, you must additionally
    use ipnadmin to establish at the sending node a _class-of-service override_
    that locally and temporarily OR's the BP\_CT\_REQUESTED flag (64) to the
    quality-of-service flags of any bundle tagged with data label X.

    If you need to establish a class-of-service override to set the
    BP\_BIBE\_REQUESTED flag (as described above) as well, then use the
    OR of BP\_BIBE\_REQUESTED and BP\_CT\_REQUESTED - that is, 96 - as the
    quality-of-service flags argument for that override.

    **NOTE** that an alternative method of setting both the BP\_BIBE\_REQUESTED
    and BP\_CT\_REQUESTED flags for a given bundle is simply to request custody
    transfer when the bundle is sourced; this will OR that bundle's own
    quality-of-service flags (in ancillary data) with 96.  But be careful:
    in this case the bundle will be permanently tagged with these flag values,
    meaning that it will be forwarded via BIBE with custody transfer over
    every "hop" of the end-to-end path to its destination, and if BIBE is
    unavailable at any forwarding node on the path then the bundle can
    never reach the destination node.

# GENERAL COMMANDS

- **?**

    The **help** command.  This will display a listing of the commands and their
    formats.  It is the same as the **h** command.

- **#**

    Comment line.  Lines beginning with **#** are not interpreted.

- **e** { 1 | 0 }

    Echo control.  Setting echo to 1 causes all output printed by bibeadmin to be
    logged as well as sent to stdout.  Setting echo to 0 disables this behavior.

- **w** { 0 | 1 | _activity\_spec_ }

    The **watch** command.  This command enables and disables production of a
    continuous stream of user-selected Bundle-in-Bundle Encapsulation
    custody transfer activity indication characters.  A watch parameter of "1"
    selects all BIBE-CT activity indication characters; "0" de-selects all
    BIBE-CT activity indication characters; any other _activity\_spec_ such
    as "mw" selects all activity indicators in the string, de-selecting all
    others.  BIBE will print each selected activity indication character to
    **stdout** every time a processing event of the associated type occurs:

    **w**	custody of bundle is accepted

    **m**	custody acceptance is received for one bundle

    **x**	custody of bundle is refused

    **&**	custody refusal is received for one bundle

    **$**	bundle retransmitted due to expired custodial retransmission interval

- **h**

    The **help** command.  This will display a listing of the commands and their
    formats.  It is the same as the **?** command.

# BCLA COMMANDS

- **a** bcla _peer\_EID_ _fwd\_latency_ _rtn\_latency_ _time\_to\_live_ _priority_ _ordinal_ \[_data label_\]

    The **add bcla** command.  This command adds the neighboring node identified
    by _peer\_EID_ as a BIBE destination of the local node.

- **c** bcla _peer\_EID_ _fwd\_latency_ _rtn\_latency_ _time\_to\_live_ _priority_ _ordinal_ \[_data label_\]

    The **change bcla** command.  This command changes the transmission parameters
    governing BIBE PDU transmission to the indicated peer node.

- **d** bcla _peer\_EID_

    The **delete bcla** command.  This command deletes the **bcla** identified by
    _peer\_EID_.

- **i** bcla _peer\_EID_

    This command will print information (the transmission parameters) for the
    BIBE peer node identified by _peer\_EID_.

- **l**

    This command lists all of the local node's BIBE peers.

# EXAMPLES

- a bcla ipn:3.2 10 10 60 1 0 16

    Declares that ipn:3.2 is a BIBE destination and that BIBE PDUs destined for
    this node are to be sent with TTL 60 seconds, priority 1 (standard), and data
    label 16; it is expected that BIBE PDUs sent to this node will arrive within
    10 seconds and that BIBE PDUs sent from this node will arrive within 10 seconds.

# SEE ALSO

bibeadmin(1), bibeclo(1)
