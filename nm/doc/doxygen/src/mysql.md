Manager Database Support          {#mysql}
=================

To build with MySQL support, specify "--with-mysql" when running
configure.  

The database schema and associated documentation can be found in the
contrib folder under amp-sql.

NOTE: This is the initial release of the MySQL support and is not yet
feature complete for handling of all data types.

## Connection

To connect to the database automatically, specify the connection
parameters at the command-line.  Run "nm_mgr --help" for a full
listing of supported options.  For example:

  ./nm_mgr ipn:2.65 --sql-user amp --sql-pass amp --sql-db amp_core --sql-host db

Database connection can also be adjusted from the main menu of the UI.


## Design Notes

The manager currently maintains a pool of MySQL connections, with one
connection assigned to each thread.

The control thread will poll the database once per second for outgoing
message sets in the ready state.  A message set can be targeted at one or more NM
agents.  The state for each message set will be updated to 'success'
or 'error' as appropriate.

The RX thread, which handles receipt of reports from agents, performs
all SQL operations within the context of a transaction. This ensures
that other DB clients will not see a message set until it is full
processed.  

At this time, transactions are always committed after
parsing of each received message set.  If an error occurred, the message set
state will be set to 'error' in the database, and the raw Hex CBOR
encoded message will be inserted into the log table.  For this entry,
'source' is the agent string EID and 'line' is overridden as the
message set id.

If the db_log_always flag is set, the raw HEX CBOR encoding will be
logged for all messages, regardless of status.  

NOTE: For this initial release, db_log_always is enabled by
default.  Future releases will disable this unless explicitly enabled
via the UI or command-line flag.
