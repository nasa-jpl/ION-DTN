Manager UI                              {#nmui}
========

The nm_mgr's is built by default with a simple text menu-based interface.

A shell-like interface, termed 'Automator Mode' is also available.  This can be activated from the main menu, or specifying '-A' when launching the manager.  This alternate interface is limited in functionality, but designed to aide scripting.  Type '?' from within the interface for usage information.  It is recommended to configure file logging or database output (if available) for parsing received reports.  

An enhanced NCURSES based UI is also available. This mode provides a more user friendly interface while preserving the same functionality available in the basic UI.  To use this interface, ensure the NCURSES library is available on your system and add "--with-ncurses" to your configure line.  For example, "configure --with-ncurses && make && sudo make install"

A REST API is now available if configured with "--enable-nmrest". This API should be considered experimental and is likely to evolve in future releases.  See below for details.

## REST API (Experimental)

The REST API is implemented using the [Civetweb](https://github.com/civetweb/civetweb) library. It's functionality is limited, and is primarily intended for integration with additional tools in the future.

Note: At this time, no attempt has been made to secure the REST API at this time.  The library can support SSL encryption and basic HTTP Authentication, or the user can wrap the server with security settings using an Apache server proxy.  Refer to the CivetWeb documentation and nm/mgr/nm_rest.c for details.

Limitations:
- JSON Report/Table output may not be complete at this time for all data types. In those cases, it will fallback to a string representation of the nested element.
- User is responsible for securing REST API access as required
- Control inputs are limited to HEX-Encoded CBOR strings of ARI Controls.  It is intended for other tools to integrate with the REST API (or DB interface) for higher level definitions, such as URI conversions or GUI applications.

The following APIs are available from the base URI of http://localhost:8089/nm/api, ie: http://localhost:8089/nm/api/version

The variable '$ADDR' below refers to the agent identifier. This can be
either "eid/$eid" where $eid is the EID of the node, or "idx/$idx",
where $idx is the index of a node from the all agents listing (as used
in the UI).  For example the path "/agents/$ADDR/json" could translate
to http://localhost:8089/nm/api/agents/idx/0/json or
http://localhost:8089/nm/api/agents/eid/ipn:2.1/json


 | Method | Path                        | Description                                                                                            |
 |--------|-----------------------------|--------------------------------------------------------------------------------------------------------|
 | GET    | /version                    | Return version information                                                                             |
 | GET    | /agents                     | Get a listing of registered agents                                                                     |
 | POST   | /agents                     | Register a new Agent at specified eid (in body of request)                                             |
 | PUT    | /agents/idx/$idx/hex        | Body is CBOR-encoded HEX ARI to send.  $idx is index of node from agents listing                       |
 | PUT    | /agents/eid/$eid/hex        | Body is CBOR-encoded HEX ARI to send.  $eid is the agent to query                                      |
 | PUT    | /agents/$ADDR/clear_reports | Clear all reports for given node                                                                       |
 | PUT    | /agents/$ADDR/clear_tables  | Clear all tables for given node                                                                        |
 | GET    | /agents/$ADDR/reports/hex   | Retrieve list of reports for node in HEX CBOR format                                                   |
 | GET    | /agents/$ADDR/reports       | Retrieve list of reports for node. Currently in HEX CBOR format, but may change in future              |
 | GET    | /agents/$ADDR/reports/text  | Retrieve list of reports for node in ASCII/text format                                                 |
 | GET    | /agents/$ADDR/reports/json  | Retrieve list of reports for node in JSON format                                                       |
 | GET    | /agents/$ADDR/reports/debug | Retrieve all reports for node. Debug information and/or multiple formats may be returned.              |
 | POST   | /agents/$node               | Same as by_idx, but $node is full node name (ie: "ipn:1.1"). May not be defined for v0                 |
 | GET    | /agents/$ADDR               | Retrieve node information, including name and # reports available                                      |
 | GET    | /agents/$ADDR/reports       | Added to above, get list of reports. See above for details                                             |
 | PUT    | /agents/$ADDR/reports/clear | Clear all cached reports                                                                               |
 | PUT    | /agents/$ADDR/hex           | Send a HEX-encoded ARI to defined node. Content should be provided as a HEX string in body of request. |
 |        |                             |                                                                                                        |

## Main Console UI
The following are some usage examples of the primary console UI.  

The listed numbers correspond to user inputs for the described items
at the time of writing.  If built with the NCURSES UI, top-level menu
items will remain the same, however input and navigation of child
forms will differ.

Be advised that menu options can change between ION releases.


### Example: Request ADM Full Report
@startuml
start

:2: List Agents;
:0: Select first agent;
:1: Build Control;
:Enter '0' for timestamp;
:1: Select ARI form list;
:1: amp_agent;
:6: gen_rpts;
:1 ARI in collection;
:1: Select ARI form list;
:1: amp_agent;
:7: RPTT;
:0: full_report;
:0 Elements in the TNVC;

if (Expect Report in Response?) then (yes)
    :wait for receipt;
    :4: Print agent reports;
endif


stop
@enduml
### Example: Send a raw ARI from saved HEX Encoding
@startuml
start
:2: List Agents;
:0: Select first agent;
:2: Send RAW Command;
:Input ARI as HEX;

if (Expect Report in Response?) then (yes)
    :wait for receipt;
    :4: Print agent reports;
endif

stop
@enduml
