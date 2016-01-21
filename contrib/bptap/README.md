BPTAP 20150420
--------------
Philip Tsao <ptsao@jpl.nasa.gov> 
Sam Nguyen <phn19@yahoo.com>

BPTAP is a virtual Ethernet driver which runs atop the ION
(Interplanetary Overlay Network) implementation of the DTN (Delay/Disruption 
Tolerant Network) protocols. This has been used to stream video and play 
Starcraft over Bundle Protocol. ION may be obtained from 
<http://ion-dtn.sourceforge.net>. BPTAP also requires TUN/TAP. This program
has been tested on Linux but should work on most UNIX-like environments and
possibly even Windows.

To build:

1. As necessary, edit the $IONPATH variable in the Makefile.
2. Type 'make'

*Other platforms may require adjustments to headers and libs.

Usage:

    # ./bptap <device> <own eid> <dest eid1> [dest eid2] ...

As BPTAP creates a TAP device, it will likely require root-like permissions.
For example, if you have three machines with EID's ipn:1.x, ipn:2.x, ipn:3.x

    # ./bptap dtn0 ipn:1.1 ipn:2.1 ipn:3.1 

Don't forget to bind an address to the device (in this case "dtn0") on each
machine. 

