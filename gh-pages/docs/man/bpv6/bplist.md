# NAME

bplist - Bundle Protocol (BP) utility for listing queued bundles

# SYNOPSIS

**bplist** \[{count | detail} \[_destination\_EID_\[/_priority_\]\]\]

# DESCRIPTION

**bplist** is a utility program that reports on bundles that currently
reside in the local node, as identified by entries in the local bundle
agent's "timeline" list.

Either a count of bundles or a detailed list of bundles (noting primary block
information together with hex and ASCII dumps of the payload and all
extension blocks, in expiration-time sequence) may be requested.

Either all bundles or just a subset of bundles - restricted to bundles
for a single destination endpoint, or to bundles of a given level of priority
that are all destined for some specified endpoint - may be included in the
report.

By default, **bplist** prints a detailed list of all bundles residing in
the local node.

# EXIT STATUS

- "0"

    **bplist** terminated, for reasons noted in the **ion.log** file.

- "1"

    **bplist** was unable to attach to Bundle Protocol operations, probably because
    **bpadmin** has not yet been run.

# FILES

No configuration files are needed.

# ENVIRONMENT

No environment variables apply.

# DIAGNOSTICS

The following diagnostics may be issued to the **ion.log** log file:

- Can't attach to BP.

    **bpadmin** has not yet initialized BP operations.

# BUGS

Report bugs to <ion-dtn-support@lists.sourceforge.net>

# SEE ALSO

bpclock(1)
