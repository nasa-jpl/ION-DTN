# Known Issues

Here is a list of known issues that will updated on a regular basis to captures various lessons-learned relating to the configurations, testing, performance, and deployment of ION:

## Convergence Layer Adaptor

### UDP CLA ###

* When using UDP CLA for data delivery, one should be aware that:
   * UDP is inherently unreliable. Therefore the delivery of BP bundles may not be guaranteed, even withing a controlled, isolated network environment.
   * It is best to use iperf and other performance testing tools to properly character UDP performance before using UDP CLA. UDP loss may have high loss rate due to presence of other traffic or insufficient internal buffer.
   * When UDP CLA is used to deliver bundles larger than 64K, those bundle will be fragmented and reassembled at the destination. It has been observed on some platforms that UDP buffer overflow can cause a large number of 'cyclic' packet drops so that an unexpected large number of bundles are unable to be reassembled at the destination. These bundle fragments (which are themselves bundles) will take up storage and remain until either (a) the remaining fragments arrived or (b) the TTL expired.

### LTP CLA ###

* When using LTP over the UDP-based communication services (udpcli and udpclo daemons):
   * The network layer MTU and kernel buffer size should be properly configured
   * The use "WAN emulator" to add delay and probabilistic loss to data should be careful not to filter out UDP fragments that are needed to reconstruct the LTP segments or significantly delayed them such that the UDP segment reassembly will expire.

## Bundle Protocol ##

### CRC ###

* ION implementation currently default will apply CRC to Primary Block but not the Payload Block. To apply CRC to the Payload Block, a compiler flag needs to be set when building ION. There are currently no mechanism to dynamically turn on/turn off CRC without recompiling ION.

## Reporting Issues ##

* ION related issues can be reported to the public GitHub page for ION-DTN or ion-core.
* ION's SourceForge page is now deprecated but issues reported there will still be monitored.


