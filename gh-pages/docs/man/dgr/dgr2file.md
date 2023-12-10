# NAME

dgr2file - DGR reception test program

# SYNOPSIS

**dgr2file**

# DESCRIPTION

**dgr2file** uses DGR to receive multiple copies of the text of a file
transmitted by **file2dgr**, writing each copy of the file to the current
working directory.  The name of each file written by **dgr2file** is
file\_copy\__cycleNbr_, where _cycleNbr_ is initially zero and is increased
by 1 every time **dgr2file** closes the file it is currently writing and
opens a new one.

Upon receiving a DGR datagram from **file2dgr**, **dgr2file** extracts the
content of the datagram (either a line of text from the file that is being
transmitted by **file2dgr** or else an EOF string indicating the end of that
file).  It appends each extracted line of text to the local copy of that
file that **dgr2file** is currently writing.  When the extracted datagram
content is an EOF string (the ASCII text "\*\*\* End of the file \*\*\*"),
**dgr2file** closes the file it is writing, increments _cycleNbr_, opens
a new copy of the file for writing, and prints the message "working on cycle
_cycleNbr_."

**dgr2file** always receives datagrams at port 2101.

# EXIT STATUS

- "0"

    **dgr2file** has terminated.

# FILES

No configuration files are needed.

# ENVIRONMENT

No environment variables apply.

# DIAGNOSTICS

- can't open dgr service

    Operating system error.  Check errtext, correct problem, and rerun.

- can't open output file

    Operating system error.  Check errtext, correct problem, and rerun.

- dgr\_receive failed

    Operating system error.  Check errtext, correct problem, and rerun.

- can't write to output file

    Operating system error.  Check errtext, correct problem, and rerun.

# BUGS

Report bugs to <ion-dtn-support@lists.sourceforge.net>

# SEE ALSO

file2dgr(1), dgr(3)
