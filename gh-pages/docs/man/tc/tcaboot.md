# NAME

tcaboot - Trusted Collective (TC) authority initialization utility

# SYNOPSIS

**tcaboot** _multicast\_group\_number\_for\_TC\_bulletins_ _multicast\_group\_number\_for\_TC\_records_ _number\_of\_authorities\_in\_collective_ _K_ _R_ \[ _delay_ \]

# DESCRIPTION

**tcaboot** writes a TC authority administration command file that initializes
a TC authority database.  The file, named "boot.tcarc", is written to the
current working directory.  It simply contains two authority configuration
commands that initialize the TC authority database for the TC application
and then set the initial bulletin compilation time for this authority to
the current ctime plus _delay_ seconds.  If omitted, _delay_ defaults
to 5.  The other command-line arguments for **tcaboot** are discussed in
the descriptions of application initialization commands for **tcaadmin**;
see the tcarc(5) manual page for details.

# EXIT STATUS

- "0"

    Successful generation of TC authority initialization file.

# EXAMPLES

- tcaboot 210 209 6 50 .2

    Writes a boot.tcarc file that initializes the local node's TC authority
    database as indicated and sets the next bulletin compilation time to the
    current time plus 5 seconds.

- tcaboot 210 209 6 50 .2 90

    Writes a boot.tcarc file that initializes the local node's TC authority
    database as indicated and sets the next bulletin compilation time to the
    current time plus 90 seconds.

# FILES

No files apply.

# ENVIRONMENT

No environment variables apply.

# DIAGNOSTICS

The following diagnostics may be issued to the log file:

- Can't open cmd file

    **tcaboot** is unable to create a file named boot.tcarc for the indicated reason,
    a system error.

- Can't write to cmd file

    **tcaboot** is unable to write to boot.tcarc for the indicated reason,
    a system error.

# BUGS

Report bugs to <ion-dtn-support@lists.sourceforge.net>

# SEE ALSO

tcaadmin(1), tcarc(5)
