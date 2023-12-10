# NAME

cfdptest - CFDP test shell for ION

# SYNOPSIS

**cfdptest** \[ _commands\_filename_ \]

# DESCRIPTION

**cfdptest** provides a mechanism for testing CFDP file transmission.  It can
be used in either scripted or interactive mode.  All bundles containing CFDP
PDUs are sent with custody transfer requested and with all bundle status
reporting disabled.

When scripted with _commands\_filename_, **cfdptest** operates in response to
CFDP management commands contained in the provided commands file.  Each line
of text in the file is interpreted as a single command comprising several
tokens: a one-character command code and, in most cases, one or more command
arguments of one or more characters.  The commands configure and initiate
CFDP file transmission operations.

If no file is specified, **cfdptest** instead offers the user an interactive
"shell" for command entry.  **cfdptest** prints a prompt string (": ") to
stdout, accepts strings of text from stdin, and interprets each string as
a command.

The supported **cfdptest** commands (whether interactive or scripted) are as
follows:

- **?**

    The **help** command.  This will display a listing of the commands and their
    formats.  It is the same as the **h** command.

- **h**

    An alternate form of the **help** command.

- **z** \[&lt;number of seconds to pause>\]

    The **pause** command.  When **cfdptest** is running in interactive mode,
    this command causes the console input processing thread to pause the
    indicated number of seconds (defaulting to 1) before processing the next
    command.  This is provided for use in test scripting.

- **d** &lt;destination CFDP entity ID number>

    The **destination** command.  This command establishes the CFDP entity
    to which the next file transmission operation will be directed.  CFDP entity
    numbers in ION are, by convention, the same as BP node numbers.

- **f** &lt;source file path name>

    The **from** command.  This command identifies the file that will be
    transmitted when the next file transmission operation is commanded.

- **t** &lt;destination file path name>

    The **to** command.  This command provides the name for the file that will be
    created at the receiving entity when the next file transmission operation
    is commanded.

- **l** &lt;lifetime in seconds>

    The **time-to-live** command.  This command establishes the time-to-live for
    all subsequently issued bundles containing CFDP PDUs.  If not specified, the
    default value 86400 (1 day) is used.

- **p** &lt;priority>

    The **priority** command.  This command establishes the priority (class of
    service) for all subsequently issued bundles containing CFDP PDUs.  Valid
    values are 0, 1, and 2.  If not specified, priority is 1.

- **o** &lt;ordinal>

    The **ordinal** command.  This command establishes the "ordinal" (sub-priority
    within priority 2) for all subsequently issued bundles containing CFDP PDUs.
    Valid values are 0-254.  If not specified, ordinal is 0.

- **m** &lt;mode>

    The **mode** command.  This command establishes the transmission mode
    ("best-effort" or assured) for all subsequently issued bundles containing
    CFDP PDUs.  Valid values are 0 (assured, reliable, with reliability
    provided by a reliable DTN convergence layer protocol), 1 (best-effort,
    unreliable), and 2 (assured, reliable, but with reliability provided by BP
    custody transfer).  If not specified, transmission mode is 0.

- **a** &lt;latency in seconds>

    The **closure latency** command.  This command establishes the transaction
    closure latency for all subsequent file transmission operations.  When it is
    set to zero, the file transmission is "open loop" and the CFDP transaction
    at the sending entity finishes when the EOF is sent.  Otherwise, the
    receiving CFDP entity is being asked to send a "Finished" PDU back to the
    sending CFDP entity when the transaction finishes at the receiving entity.
    Normally the transaction finishes at the sending entity only when that
    Finished PDU is received.  However, when _closure latency_ seconds elapse
    following transmission of the EOF PDU prior to receipt of the Finished PDU,
    the transaction finishes immediately with a Check Timer fault.

- **n** { 0 | 1 }

    The **segment metadata** command.  This command controls the insertion of
    sample segment metadata -- a string representation of the current time --
    in every file data segment PDU.  A value of 1 enables segment metadata
    insertion, while a value of 0 disables it.

- **g** &lt;srrflags>

    The **srrflags** command.  This command establishes the BP status reporting
    that will be requested for all subsequently issued bundles containing
    CFDP PDUs.  _srrflags_ must be a status reporting flags string as defined
    for bptrace(1): a sequence of status report flags, separated by commas,
    with no embedded whitespace.  Each status report flag must be one of the
    following: rcv, ct, fwd, dlv, del.

- **c** &lt;criticality>

    The **criticality** command.  This command establishes the criticality
    for all subsequently issued bundles containing CFDP PDUs.  Valid values
    are 0 (not critical) and 1 (critical).  If not specified, criticality is 0.

- **r** &lt;action code nbr> &lt;first path name> &lt;second path name>

    The **filestore request** command.  This command adds a filestore request to
    the metadata that will be issued when the next file transmission operation
    is commanded.  Action code numbers are:

    - 0 = create file
    - 1 = delete file
    - 2 = rename file
    - 3 = append file
    - 4 = replace file
    - 5 = create directory
    - 6 = remove directory
    - 7 = deny file
    - 8 = deny directory

- **u** '&lt;message text>'

    The **user message** command.  This command adds a user message to the
    metadata that will be issued when the next file transmission operation
    is commanded.

- **&**

    The **send** command.  This command initiates file transmission as configured
    by the most recent preceding **d**, **f**, **t**, and **a** commands.

- **|**

    The **get** command.  This command causes a request for file transmission to
    the local node, subject to the parameters provided by the most recent preceding
    **f**, **t**, and **a** commands, to be sent to the entity identified by the
    most recent preceding **d** command.

    **NOTE** that 'get' in CFDP is implemented very differently from 'send'.  The
    'send' operation is a native element of the CFDP protocol. The 'get' operation
    is implemented by sending to the responding entity a standardized sequence of
    message-to-user messages in a Metadata PDU - the _user application_ at the
    responding entity receives those messages and initiates a 'send' to accomplish
    transmission of the file.  This means that 'send' can succeed even if no user
    application is running at the remote node, but 'get' cannot.

- **^**

    The **cancel** command.  This command cancels the most recently initiated
    file transmission.

- **%**

    The **suspend** command.  This command suspends the most recently initiated
    file transmission.

- **$**

    The **resume** command.  This command resumes the most recently initiated
    file transmission.

- **#**

    The **report** command.  This command reports on the most recently initiated
    file transmission.

- **q**

    The **quit** command.  Terminates the cfdptest program.

**cfdptest** in interactive mode also spawns a CFDP event handling thread.  The
event thread receives CFDP service indications and simply prints lines of
text to stdout to announce them.

**NOTE** that when **cfdptest** runs in scripted mode it does **not** spawn an
event handling thread, which makes it possible for the CFDP events queue to
grow indefinitely unless some other task consumes and reports on the events.
One simple solution is to run an interactive **cfdptest** task in background,
simply to keep the event queue cleared, while scripted non-interactive
**cfdptest** tasks are run in the foreground.

# EXIT STATUS

- "0"

    **cfdptest** has terminated.  Any problems encountered during operation
    will be noted in the **ion.log** log file.

# FILES

See above for details on valid _commands\_filename_ commands.

# ENVIRONMENT

No environment variables apply.

# DIAGNOSTICS

Diagnostic messages produced by **cfdptest** are written to the ION log
file _ion.log_.

- Can't open command file...

    The file identified by _commands\_filename_ doesn't exist.

- cfdptest can't initialize CFDP.

    **cfdpadmin** has not yet initialized CFDP operations.

- Can't put FDU.

    The attempt to initiate file transmission failed.  See the ION log for
    additional diagnostic messages from the CFDP library.

- Failed getting CFDP event.

    The attempt to retrieve a CFDP service indication failed.  See the ION log for
    additional diagnostic messages from the CFDP library.

# BUGS

Report bugs to <ion-dtn-support@lists.sourceforge.net>

# SEE ALSO

cfdpadmin(1), cfdp(3)
