# NAME

bssStreamingApp - Bundle Streaming Service transmission test program

# SYNOPSIS

**bssStreamingApp** _own\_endpoint\_ID_ _destination\_endpoint\_ID_ \[_class\_of\_service_\]

# DESCRIPTION

**bssStreamingApp** uses BSS to send streaming data over BP from
_own\_endpoint\_ID_ to bssrecv listening at _destination\_endpoint\_ID_.
_class\_of\_service_ is as specified for bptrace(1); if omitted, bundles
are sent at BP's standard priority (1).

The bundles issued by **bssStreamingApp** all have 65000-byte payloads, where
the ASCII representation of a positive integer (increasing monotonically
from 0, by 1, throughout the operation of the program) appears at the start
of each payload.  All bundles are sent with custody transfer requested, with
time-to-live set to 1 day.  The application meters output by sleeping for
12800 microseconds after issuing each bundle.

Use CTRL-C to terminate the program.

# FILES

No configuration files are needed.

# ENVIRONMENT

No environment variables apply.

# BUGS

Report bugs to <ion-dtn-support@lists.sourceforge.net>

# SEE ALSO

bssrecv(1), bss(3)
