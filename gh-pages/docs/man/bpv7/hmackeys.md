# NAME

hmackeys - utility program for generating good HMAC-SHA1 keys

# SYNOPSIS

**hmackeys** \[ _keynames\_filename_ \]

# DESCRIPTION

**hmackeys** writes files containing randomized 160-bit key values suitable
for use by HMAC-SHA1 in support of Bundle Authentication Block processing,
Bundle Relay Service connections, or other functions for which symmetric
hash computation is applicable.  One file is written for each key name
presented to _hmackeys_; the content of each file is 20 consecutive randomly
selected 8-bit integer values, and the name given to each file is simply
"_keyname_.hmk".

**hmackeys** operates in response to the key names found in the file
_keynames\_filename_, one name per file text line, if provided; if not,
**hmackeys** prints a simple prompt (:) so that the user may type key names
directly into standard input.

When the program is run in interactive mode, either enter 'q' or press ^C to
terminate.

# EXIT STATUS

- "0"
Completion of key generation.

# EXAMPLES

- hmackeys

    Enter interactive HMAC/SHA1 key generation mode.

- hmackeys host1.keynames

    Create a key file for each key name in _host1.keynames_, then terminate
    immediately.

# FILES

No other files are used in the operation of _hmackeys_.

# ENVIRONMENT

No environment variables apply.

# DIAGNOSTICS

The following diagnostics may be issued to the logfile ion.log:

- Can't open keynames file...

    The _keynames\_filename_ specified in the command line doesn't exist.

# BUGS

Report bugs to <ion-dtn-support@lists.sourceforge.net>

# SEE ALSO

brsscla(1), ionsecadmin(1)
