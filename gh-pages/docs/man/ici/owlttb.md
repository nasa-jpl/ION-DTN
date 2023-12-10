# NAME

owlttb - one-way light time transmission delay simulator

# SYNOPSIS

**owlttb** _own\_uplink\_port#_ _own\_downlink\_port#_ _dest\_uplink\_IP\_address_ _dest\_uplink\_port#_ _dest\_downlink\_IP\_address_ _dest\_downlink\_port#_ _owlt\_sec._ \[-v\]

# DESCRIPTION

**owlttb** delays delivery of data between an NTTI and a NetAcquire box (or
two, one for uplink and one for downlink) by a specified length of time,
simulating the signal propagation delay imposed by distance between the nodes.

Its operation is configured by the command-line parameters, except that the
delay interval itself may be changed while the program is running.  **owlttb**
offers a command prompt (:), and when a new value of one-way light time is
entered at this prompt the new delay interval takes effect immediately.

- _own\_uplink\_port#_ identifies the port on **owlttb** accepts the NTTI's TCP connection for uplink traffic (i.e., data destined for the NetAcquire box).
- _own\_downlink\_port#_ identifies the port on **owlttb** accepts the NTTI's TCP connection for downlink traffic (i.e., data issued by the NetAcquire box).
- _dest\_uplink\_IP\_address_ is the IP address (a dotted string) identifying the NetAcquire box to which **owlttb** will transmit uplink traffic.
- _dest\_uplink\_port#_ identifies the TCP port to which **owlttb** will connect in order to transmit uplink traffic to NetAcquire.
- _dest\_downlink\_IP\_address_ is the IP address (a dotted string) identifying the NetAcquire box from which **owlttb** will receive downlink traffic.
- _dest\_downlink\_port#_ identifies the TCP port to which **owlttb** will connect in order to receive downlink traffic from NetAcquire.
- _owlt_ specifies the number of seconds to wait before forwarding each received segment of TCP traffic.

The optional **-v** ("verbose") parameter causes **owlttb** to print a
message whenever it receives, sends, or discards (due to absence of a
connected downlink client) a segment of TCP traffic.

**owlttb** is designed to run indefinitely.  To terminate the program, just
use control-C to kill it or enter "q" at the prompt.

# EXIT STATUS

- "0"
Nominal termination.
- "1"
Termination due to an error condition, as noted in printed messages.

# EXAMPLES

Here is a sample owlttb command:

- owlttb 2901 2902 137.7.8.19 10001 137.7.8.19 10002 75

This command indicates that **owlttb** will accept an uplink traffic connection
on port 2901, forwarding the received uplink traffic to port 10001 on the
NetAcquire box at 137.7.8.19, and it will accept a downlink traffic connection
on port 2902, delivering over that connection all downlink traffic that it
receives from connecting to port 10002 on the NetAcquire box at 137.7.8.19.
75 seconds of delay (simulating a distance of 75 light seconds) will be
imposed on this transmission activity.

# FILES

Not applicable.

# ENVIRONMENT

No environment variables apply.

# DIAGNOSTICS

The following diagnostics may be printed to stdout:

- owlttb can't spawn uplink thread

    The program terminates.

- owlttb can't spawn uplink sender thread

    The program terminates.

- owlttb can't spawn downlink thread

    The program terminates.

- owlttb can't spawn downlink receiver thread

    The program terminates.

- owlttb can't spawn downlink sender thread

    The program terminates.

- owlttb fgets failed

    The program terminates.

- owlttb out of memory.

    The program terminates.

- owlttb lost uplink client.

    This is an informational message.  The NTTI may reconnect at any time.

- owlttb lost downlink client

    This is an informational message.  The NTTI may reconnect at any time.

- owlttb can't open TCP socket to NetAcquire

    The program terminates.

- owlttb can't connect TCP socket to NetAcquire

    The program terminates.

- owlttb write() error on socket

    The program terminates if it was writing to NetAcquire; otherwise it
    simply recognizes that the client NTTI has disconnected.

- owlttb read() error on socket

    The program terminates.

- owlttb can't open uplink dialup socket

    The program terminates.

- owlttb can't initialize uplink dialup socket

    The program terminates.

- owlttb can't open downlink dialup socket

    The program terminates.

- owlttb can't initialize downlink dialup socket

    The program terminates.

- owlttb accept() failed

    The program terminates.

# BUGS

Report bugs to <ion-dtn-support@lists.sourceforge.net>
