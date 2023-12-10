# NAME

nm\_mgr - Network Management server implementing the Asynchronous Management Protocol (AMP)

# SYNOPSIS

**nm\_mgr** \[_options_\] _manager eid_

The following options may be specified to customize behavior.  Use "nm\_mgr --help" for full usage information:

- -A

    Startup directly in the alternative Automator UI mode. This mode is
    designed to provide a consistent line-based interface suitable for
    automated scripting.  Type ? when active for usage details.

- -l

    If specified, enable file-based logging of Manager Activity on startup.  This can be toggled at any time from the main menu of the UI.

    If logging is not enabled, the following arguments have no affect until enabled in UI.

- -d

    Log each agent to a different directory.

- -L #

    Specify maximum number of entries (reports+tables) per file before rotating.

- -D DIR

    NM logs will be placed in this directory.

- -r

    Log all received reports to file in text format (as shown in UI).

- -t

    Log all received tables to file in text format (as shown in UI).

- -T

    Log all transmitted message as ASCII-encoded CBOR HEX strings.

- -R

    Log all received messages as ASCII-encoded CBOR HEX strings.

# DESCRIPTION

Starts the **nm\_mgr** application listening on _mgr eid_ for messages from **nm\_agent** clients.  Specify "--help" for full usage information.

An agent will automatically attempt to register with it's configured manager on startup.  Agents may also be added manually through the managers UI.

The manager provides a text based UI as its primary interface.  The UI provides capabilities to list, register, or delete agents. It can view received reports and tables, and be used to send commands (ARIs) to registered agents.

An experimental REST API is available if built with the configuration option "--enable-rest".  The default configuration will be accessible at http://localhost:8089/nm/api.

# SEE ALSO

[Asynchronous Management Protocol](https://datatracker.ietf.org/doc/draft-birrane-dtn-amp/)

nm\_agent(1)
