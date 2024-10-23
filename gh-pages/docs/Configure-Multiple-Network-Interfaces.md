# Configure ION for Multiple Network Interfaces

Lab testing of ION-based DTN network often uses only a single network. However, during deployment or operational testing, ION network must operate over multiple networks. To clarify how to configure ION, we consider the following hypothetical network configuration.

The basic topology is illustrated here:

```text
    +-------+  protocol a     protocol b  +-------+
    |       |                             |       |
    |  SC1  +-----+                  +--->+  MOC1 |
    |  21   |     |                  |    |  24   |
    +-------+     |     +-------+    |    +-------+
                  +---->+       +----+
          rfnet         |  GS   |          gsnet
                  +---->+  23   +----+
    +-------+     |     +-------+    |    +-------+
    |       |     |                  |    |       |
    |  SC2  +-----+                  +--->+  MOC2 |
    |  22   |                             |  25   |
    +-------+                             +-------+

subnet: 192.168.100.0/24      subnet:192.168.200.0/24
```

* There are five ION nodes deployed across two networks, `rfnet` (192.168.100.0/24) and `gsnet` (192.168.200.0/24).
* The GS node is connected to both networks and acts as a DTN router between ION nodes on rfnet and gsnet.
* We assume the link convergence layer in `rfnet` is protocol A and in `gsnet`, protocol B.
* We present a series of ION configurations with the following protocol (A,B) combinations:
  * (LTP, TCP)
  * (LTP, STCP)
  * (TCP, TCP)
  * (STCP, STCP)
  * (UDP, UDP)
  * (LTP, LTP)

## Induct and Outduct Relationship

ION associates each neighbor with an convergence layer protocol and an outduct. With the exception of UDP convergence layer, each outduct is associated with an induct as well.

When there are multiple neighbors using the same convergence layer protocol, only one induct is used to 'pair' with multiple outducts of the same protocol.

If neighbors using the same protocol are all within the same network, then the induct is associated with the IP address of the ION node on that particular network.

If the neighbors using the same protocol are from multiple networks, then the induct will need to be associated with INADDR_ANY, 0.0.0.0:port defined for protocols such as TCP, UDP, and STCP.

For LTP, however, multiple inducts can be defined for each network using the IP address of each network separately, and the induct for a network is called `seat` (see manual page for ltprc).

## ION Configurations

### LTP/TCP Example

In this case, SC1 and SC2 communicates with GS using LTP, while MOC1 and MOC2 communicate with GS using TCP. The port we used is 4556.

For GS, it defines TCP in this manner in the `.bprc` file:

```text
a protocol tcp

a induct tcp 192.168.200.23:4556 tcpcli

a outduct tcp 192.168.200.24:4556 tcpclo
a outduct tcp 192.168.200.25:4556 tcpclo

a plan ipn:24.0
a planduct ipn:24.0 tcp 192.168.200.24:4556

a plan ipn:25.0
a planduct ipn:25.0 tcp 192.168.200.25:4556
```

There is only induct for the two outducts. Since node 23, 24, and 25 are in the 192.168.200.0/24 subnet, the induct for 23 can simply use its statically assigned IP address of 192.168.200.23:4556.

For MOC1, TCP is specified in this manner in the `.bprc` file:

```text
a protocol tcp

a induct tcp 192.168.200.24:4556 tcpcli

a outduct tcp 192.168.200.23:4556 tcpclo

a plan ipn:23.0

a planduct ipn:23.0 tcp 192.168.200.23:4556
```

Since MOC1 has only 1 neighbor and uses TCP, the induct/outduct and egress plans are very much the standard configuration we see typically in a single network configuration.

Similar configuration can be written for MOC2.

For LTP, the configuration for GS is:

```text
# in bprc file
a protocol ltp

a induct ltp 23 ltpcli

a outduct ltp 21 ltpclo
a outduct ltp 22 ltpclo

a plan ipn:21.0 
a plan ipn:22.0

a planduct ipn:21.0 ltp 21
a planduct ipn:22.0 ltp 22

# in .ltprc file
a span 21 100 100 1482 100000 1 'udplso 192.168.100.21:1113'
a span 22 100 100 1482 100000 1 'udplso 192.168.100.22:1113'
a seat 'udplsi 192.168.100.23:1113'

s
```

For ltp, a single induct is specified for the 192.168.100.0/24 subnet using the `a seat` (add seat) command. The older syntax is `s 'udplsi 192.168.100.23:1113'`, which works only for the case of a single network and port combination. However, when extending LTP to multiple seats (inducts) when there are multiple networks __or when there are multiple ports__, the `seat` command offers the flexibility to support more complex configurations.

## LTP/STCP

The syntax for LTP/STCP is identical, except replacing `tcp` with `stcp`, `tcpcli` and `tcpclo` with `stcpcli` and `stcpclo` in the configuration files.

## TCP and STCP across multiple networks

When runing TCP or STCP over both networks, the only change is that for the GS node, the induct definitions in .bprc are replaced by:

`a induct tcp 0.0.0.0:4556 tcpcli` and `a induct stcp 0.0.0.0:4556 tcpcli`

## LTP over multiple networks

When running LTP over both networks, the only key difference is that in the `.ltprc` file for the GS node, two seats are defined:

```text
a span 21 100 100 1482 100000 1 'udplso 192.168.100.21:1113'
a span 22 100 100 1482 100000 1 'udplso 192.168.100.22:1113'
a span 24 100 100 1482 100000 1 'udplso 192.168.200.24:1113'
a span 25 100 100 1482 100000 1 'udplso 192.168.200.25:1113'
a seat 'udplsi 192.168.100.23:1113'
a seat 'udplsi 192.168.200.23:1113'

s
```

Of course the bprc file must also be updated to add reflect the additional LTP neighbors, but that extension is straightforward so we will not be listing them here.

## Use of Contact Graph

For ION, the use contact graph is optional when there is one hop. In that case, the data rate, normally defined in the contact graph, is provided through commands `plan` command in `.bprc` file.

When contact graph is present, the information in the contact graph supersedes the data rate specified in the `plan` command.

If there are no contact graph and the data rate is either 0 or omitted in the `plan` command, then there is no bundle level throttling of data.

## Use of `exit` command

When no contact graph is provided, only immediate neighbor can exchange data. If relay operation is stil desired, an `exit` command can be used. In the case of the topology presented earlier, the node GS can be a gateway between the rfnet and gsnet. So GS can be added as an `exit` node for identified pair of source-destination.
