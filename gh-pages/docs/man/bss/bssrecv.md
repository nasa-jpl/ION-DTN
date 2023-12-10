# NAME

bssrecv - Bundle Streaming Service reception test program

# SYNOPSIS

**bssrecv**

# DESCRIPTION

**bssrecv** uses BSS to acquire streaming data from **bssStreamingApp**.

**bssrecv** is a menu-driven interactive test program, run from the operating
system shell prompt.  The program enables the user to begin and end a session
of BSS data acquisition from **bssStreamingApp**, displaying the data as it
arrives in real time; to replay data acquired during the current session;
and to replay data acquired during a prior session.

The user must provide values for three parameters in order to initiate the
acquisition or replay of data from **bssStreamingApp**:

- BSS database name

    All data acquired by the BSS session thread will be written to a BSS "database"
    comprising three files: table, list, and data.  The name of the database
    is the root name that is common to the three files, e.g., _db3_.tbl,
    _db3_.lst, _db3_.dat would be the three files making up the _db3_ BSS
    database.

- path name

    All three files of the selected BSS database must reside in the same
    directory of the file system; the path name of that directory is required.

- endpoint ID

    In order to acquire streaming data issued by **bssStreamingApp**, the **bssrecv**
    session thread must open the BP endpoint to which that data is directed.  For
    this purpose, the ID of that endpoint is needed.

**bssrecv** offers the following menu options:

- 1. Open BSS Receiver in playback mode

    **bssrecv** prompts the user for the three parameter values noted above, then
    opens the indicated BSS database for replay of the data in that database.

- 2. Start BSS receiving thread

    **bssrecv** prompts the user for the three parameter values noted above, then
    starts a background session thread to acquire data into the indicated database.
    Each bundle that is acquired is passed to a display function that prints a
    single line consisting of N consecutive '\*' characters, where N is computed
    as the data number at the start of the bundle's payload data, modulo 150.
    Note that the database is **not** open for replay at this time.

- 3. Run BSS receiver thread

    **bssrecv** prompts the user for the three parameter values noted above, then
    starts a background session thread to acquire data into the indicated database
    (displaying the data as described for option 2 above) and also opens the
    database for replay.

- 4. Close current playback session

    **bssrecv** closes the indicated BSS database, terminating replay access.

- 5. Stop BSS receiving thread

    **bssrecv** terminates the current background session thread.  Replay access
    to the BSS database, if currently open, is **not** terminated.

- 6. Stop BSS Receiver

    **bssrecv** terminates the current background session thread.  Replay access
    to the BSS database, if currently open, is also terminated.

- 7. Replay session

    **bssrecv** prompts the user for the start and end times bounding the
    reception interval that is to be replayed, then displays all data within that
    interval in both forward and reverse time order.  The display function
    performed for this purpose is the same one that is exercised during
    real-time acquisition of streaming data.

- 8. Exit

    **bssrecv** terminates.

# EXIT STATUS

- "0"

    **bssrecv** has terminated.

# FILES

No configuration files are needed.

# ENVIRONMENT

No environment variables apply.

# BUGS

Report bugs to <ion-dtn-support@lists.sourceforge.net>

# SEE ALSO

bssStreamingApp(1), bss(3)
