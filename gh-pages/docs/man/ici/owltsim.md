# NAME

owltsim - one-way light time transmission delay simulator

# SYNOPSIS

**owltsim** _config\_filename_ \[-v\]

# DESCRIPTION

**owltsim** delays delivery of data between pairs of ION nodes by specified
lengths of time, simulating the signal propagation delay imposed by distance
between the nodes.

Its operation is configured by delay simulation configuration lines in the
file identified by _config\_filename_.  A pair of threads is created for
each line in the file: one that receives UDP datagrams on a specified port
and queues them in a linked list, and a second that later removes queued
datagrams from the linked list and sends them on to a specified UDP port
on a specified network host.

Each configuration line must be of the following form:

> _to_ _from_ _my\_port#_ _dest\_host_ _dest\_port#_ _owlt_ _modulus_

- _to_ identifies the receiving node.

    This parameter is purely informational, intended to make **owltsim**'s
    printed messages more helpful to the user.

- _from_ identifies the sending node.

    A value of '\*' may be used to indicate "all nodes".  Again, this parameter
    is purely informational, intended to make **owltsim**'s printed messages
    more helpful to the user.

- _my\_port#_ identifies **owltsim**'s receiving port for this traffic.
- _dest\_host_ is a hostname identifying the computer to which **owltsim**
will transmit this traffic.
- _dest\_port#_ identifies the port to which **owltsim** will transmit
this traffic.
- _owlt_ specifies the number of seconds to wait before forwarding each
received datagram.
- _modulus_ controls the artificial random data loss imposed on this traffic by **owltsim**.

    A value of '0' specifies "no imposed data loss".  Any modulus value N > 0
    causes **owltsim** to randomly drop (i.e., not transmit upon expiration of the
    delay interval) one out of every N packets.  Any modulus value N < 0
    causes **owltsim** to deterministically drop every (0 - N)th packet.

The optional **-v** ("verbose") parameter causes **owltsim** to print a
message whenever it receives, sends, or drops (due to artificial random
data loss) a datagram.

Note that error conditions may cause one delay simulation (a pair of threads)
to terminate without terminating any others.

**owltsim** is designed to run indefinitely.  To terminate the program, just
use control-C to kill it.

# EXIT STATUS

- "0"
Nominal termination.
- "1"
Termination due to an error condition, as noted in printed messages.

# EXAMPLES

Here is a sample owltsim configuration file:

- 2 7 5502 ptl07.jpl.nasa.gov 5001 75 0
- 7 2 5507 ptl02.jpl.nasa.gov 5001 75 16

This file indicates that **owltsim** will receive on port 5502 the ION
traffic from node 2 that is destined for node 7, which will receive it at
port 5001 on the computer named ptl07.jpl.nasa.gov; 75 seconds of delay
(simulating a distance of 75 light seconds) will be imposed on this
transmission activity, and **owltsim** will not simulate any random data loss.

In the reverse direction, **owltsim** will receive on port 5507 the ION
traffic from node 7 that is destined for node 2, which will receive it at
port 5001 on the computer named ptl02.jpl.nasa.gov; 75 seconds of delay
will again be imposed on this transmission activity, and **owltsim** will
randomly discard (i.e., not transmit upon expiration of the transmission
delay interval) one datagram out of every 16 received at this port.

# FILES

Not applicable.

# ENVIRONMENT

No environment variables apply.

# DIAGNOSTICS

The following diagnostics may be printed to stdout:

- owltsim can't open configuration file

    The program terminates.

- owltsim failed on fscanf

    Failure on reading the configuration file.  The program terminates.

- owltsim stopped malformed config file line _line\_number_.

    Failure on parsing the configuration file.  The program terminates.

- owltsim can't spawn receiver thread

    The program terminates.

- owltsim out of memory.

    The program terminates.

- owltsim can't open reception socket

    The program terminates.

- owltsim can't initialize reception socket

    The program terminates.

- owltsim can't open transmission socket

    The program terminates.

- owltsim can't initialize transmission socket

    The program terminates.

- owltsim can't spawn timer thread

    The program terminates.

- owltsim can't acquire datagram

    Datagram transmission failed.  This causes the threads for the affected
    delay simulation to terminate, without terminating any other threads.

- owltsim failed on send

    Datagram transmission failed.  This causes the threads for the affected
    delay simulation to terminate, without terminating any other threads.

- at _time_ owltsim LOST a dg of length _length_ from _sending node_ destined for _receiving node_ due to ECONNREFUSED.

    This is an informational message.  Due to an apparent bug in Internet
    protocol implementation, transmission of a datagram on a connected UDP
    socket occasionally fails.  **owltsim** does not attempt to retransmit the
    affected datagram.

# BUGS

Report bugs to <ion-dtn-support@lists.sourceforge.net>

# SEE ALSO

udplsi(1), udplso(1)
