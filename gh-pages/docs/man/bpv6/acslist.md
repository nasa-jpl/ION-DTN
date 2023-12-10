# NAME

acslist - Aggregate Custody Signals (ACS) utility for checking custody IDs.

# SYNOPSIS

**acslist** \[_-s|--stdout_\]

# DESCRIPTION

**acslist** is a utility program that lists all mappings from bundle ID to
custody ID currently in the local bundle agent's ACS ID database, in no
specific order.  A bundle ID (defined in RFC5050) is the tuple of
(source EID, creation time, creation count, fragment offset, fragment length).
A custody ID (defined in draft-jenkins-aggregate-custody-signals) is an
integer that the local bundle agent will be able to map to a bundle ID for
the purposes of aggregating and compressing custody signals.

The format for mappings is:

(ipn:13.1,333823688,95,0,0)->(26)

While listing, **acslist** also checks the custody ID database for
self-consistency, and if it detects any errors it will print a line starting
with "Mismatch:" and describing the error.

_-s|--stdout_ tells **acslist** to print results to stdout, rather than to
the ION log.

# EXIT STATUS

- "0"

    **acslist** terminated after verifying the consistency of the custody ID database.

- "1"

    **acslist** was unable to attach to the ACS database, or it detected an
    inconsistency.

# FILES

No configuration files are needed.

# ENVIRONMENT

No environment variables apply.

# DIAGNOSTICS

The following diagnostics may be issued:

- Can't attach to ACS.

    **acsadmin** has not yet initialized ACS operations.

- Mismatch: (description of the mismatch)

    **acslist** detected an inconsistency in the database; this is a bug in ACS.

# BUGS

Report bugs to <ion-dtn-support@lists.sourceforge.net>

# SEE ALSO

acsadmin(1), bplist(1)
