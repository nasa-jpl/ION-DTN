# NAME

imcrc - IMC scheme configuration commands file

# DESCRIPTION

IMC scheme configuration commands are passed to **imcadmin** either in a file of
text lines or interactively at **imcadmin**'s command prompt (:).  Commands
are interpreted line-by line, with exactly one command per line.

IMC scheme configuration commands simply establish which nodes are the
local node's parents and children within a single IMC multicast tree.  This
single spanning tree, an overlay on a single BP-based network, is used to
convey all multicast group membership assertions and cancellations in the
network, for all groups.  Each node privately tracks which of its immediate
"relatives" in the tree are members of which multicast groups and on this
basis selectively forwards -- directly, to all (and only) interested
relatives -- the bundles destined for the members of each group.

Note that all of a node's immediate relatives in the multicast tree **must**
be among its immediate neighbors in the underlying network.  This is because
multicast bundles can only be correctly forwarded within the tree if each
forwarding node knows the identity of the relative that passed the bundle
to it, so that the bundle is not passed back to that relative creating a
routing loop.  The identity of that prior forwarding node can only be known
if the forwarding node was a neighbor, because no prior forwarding node
(aside from the source) other than the immediate proximate (neighboring)
sender of a received bundle is ever known.

IMC group IDs are unsigned integers, just as IPN node IDs are unsigned
integers.  The members of a group are nodes identified by node number,
and the multicast tree parent and children of a node are neighboring
nodes identified by node number.

The formats and effects of the IMC scheme configuration commands are
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

- **h**

    The **help** command.  This will display a listing of the commands and their
    formats.  It is the same as the **?** command.

# KINSHIP COMMANDS

- **a** _node\_nbr_ { 1 | 0 }

    The **add kin** command.  This command adds the neighboring node identified
    by _node\_nbr_ as an immediate relative of the local node.  The Boolean
    value that follows the node number indicates whether or not this node is the
    local node's parent within the tree.

- **c** _node\_nbr_ { 1 | 0 }

    The **change kin** command.  This command changes the parentage status of the
    indicated relative according to Boolean value that follows the node number,
    as noted for the **add kin** command.

- **d** _node\_nbr_

    The **delete kin** command.  This command deletes the immediate multicast
    tree relative identified by _node\_nbr_.  That node still exists but it
    is no longer a parent or child of the local node.

- **i** _node\_nbr_

    This command will print information (the parentage switch) for the
    multicast tree relative identified by _node\_nbr_.

- **l**

    This command lists all of the local node's multicast tree relatives,
    indicating which one is its parent in the tree.

# EXAMPLES

- a 983 1

    Declares that 983 is the local node's parent in the network's multicast tree.

# SEE ALSO

imcadmin(1)
