# NAME

bpcpd - ION Delay Tolerant Networking remote file copy daemon

# SYNOPSIS

**bpcpd** \[-d | -v\]

# DESCRIPTION

**bpcpd** is the daemon for **bpcp**. Together these programs copy files between
hosts utilizing NASA JPL's Interplanetary Overlay Network (ION) to provide a
delay tolerant network.

The options are permitted as follows:

> ** -d**	Debug output. Repeat for increased verbosity.
>
> ** -v**	Display version information.

**bpcpd** must be running in order to copy files from this host to another host
(i.e. remote to local). Copies in the other direction (local to remote) do not
require **bpcpd**. Further, **bpcpd** should NOT be run simultaneously with **bpcp**
or **cfdptest**.

# EXIT STATUS

- "0"

    **bpcpd** terminated normally.

- "1"

    **bpcpd** terminated abnormally. Check console and the **ion.log** file for error messages.

# FILES

No configuration files are needed.

# ENVIRONMENT

No environment variables apply.

# BUGS

Report bugs to <ion-dtn-support@lists.sourceforge.net>

# SEE ALSO

bpcp(1), ion(3), cfdptest(1)
