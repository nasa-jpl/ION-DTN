NM Manager                              {#nmmgr}
========

The manager (nm_mgr) provides the server-side implementation of the
Asynchronous Management Protocol (AMP).

The following pages contain additional information on the Manager:
- \subpage nmui
- \subpage mysql

## Command Line Options

 | Parameter | Name                     | Description                                                               |
 |-----------|--------------------------|---------------------------------------------------------------------------|
 | -A        | Automator Mode           | Startup directly in the alternative Automator UI mode.                    |
 | -l        | Enable Logging           | If set, logging will be enabled on startup. Else it must be set in the UI |
 | -d        | Log to Agent Directories | If set, log each agent to its own subdirectory.                           |
 | -L #      | Max entries per file     | Controls log file rotation foor reportss                                  |
 | -D DIR    | Log Directory            | Change the default path for NM log files                                  |
 | -r        | Log Reports in text Mode | Log received reports to file in text format (as shown in UI)              |
 | -t        | Log Tables in text Mode  | Log received tables to file in text format (as shown in UI)               |
 | -R        | Log Reports as HEX       | Log all received messages in ASCII-encoded CBOR HEX Strings               |



## Threads
[nm_mgr](nm_mgr.c) starts a minimum of two (POSIX) threads.

- [mgr_rx_thread](@ref mgr_rx_thread) - Process all inbound messages from registered agents.
- [ui_thread](@ref ui_thread) - Runs the console-based user interface
- [db_mgt_daemon](@ref db_mgt_daemon) - If SQL support is enabled, this thread handles all database access.
- [civetweb](https://github.com/civetweb/civetweb) - If REST support is enabled, this thread handles requests.

## RX Thread
This thread processes all received messages as described below.

This thread will run until a fatal error occurs, such as message receipt failure.  This will subsequently trigger nm_mgr to exit all remaining threads.

The following activity diagram represents a single iteration of the receive loop:

@startuml

start

:Wait for receipt of next message (iif_receive);

:Deserialize the received CBOR-encoded message group (msg_grp_deserialize);

while (messages in group)

:Retrieve msg from vector;

if (MSG_TYPE_RPT_SET) then (yes)
    :msg_rpt_deserialize;
    :rx_data_rpt;
elseif (MSG_TYPE_TBL_SET) then (yes)
    :msg_tbl_deserialize;
    :rx_data_tbl;
elseif (MSG_TYPE_REG_AGENT) then (yes)
    :msg_agent_deserialize;
    :rx_agent_reg;
    :agent_add;
    :msg_agent_release;
else (default)
    :Log Error;
endif

endwhile (No more msgs in group)

stop

@enduml
