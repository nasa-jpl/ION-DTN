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

* ION implementation currently default will apply CRC16 to Primary Block but not the Payload Block. To apply CRC to the Payload Block, a compiler flag needs to be set when building ION. There are currently no mechanism to dynamically turn on/turn off CRC without recompiling ION.

## Testing & Configuration ##

* When developing and testing ION in a docker container with root permission while mounting to ION code residing in a user's directory on the host machine, file ownership may switch from user to `root`. This sometimes leads to build and test errors when one switches back to the host's development and testing environment. Therefore, we recommend that you execute the `make clean` and `git stash` command to remove all build and testing artifacts from ION 's source directory before exiting the container.

## SDR transaction ##

### SDR transaction reversal ### 

When SDR transaction is canceled due to anomaly, ION will attempt automatically try the following:

1. Reverse transaction - if it is configured - to revert modifications to the SDR's heap space which contains both user and protocol data units. This action rolls back a series of operations on the SDR's data of the cancelled the transaction. 
2. Once the SDR's heap space has been restored, the "volatile" state of the protocols must be restored because they might be modified by the transaction as well. This is performed by the `ionrestart` utility.
3. After the volatiles are reloaded, the 3rd step of restoring ION operation will need to be triggered by the users. During the anomously event that caused the transaction cancellation, some of ION's various daemons may have stopped. They can be restored by simply issuing the start ('s') command through `ionadmin` and `bpadmin`. 

### 'Init' Process PID 1 ###

The reloading of the volatile state and restarting of daemons is necessary to ensure the ION system is in a consistent state before resuming normal operations. 

During the reloading of the volatile state, the bundle protocol schemes, inducts, and outducts are stopped by terminating the associated daemons. The restart process will wait for the daemon's to be terminated before restarting them. When running ION inside a docker container, the `init` process (PID 1) should be properly configured to reap all zombie processes because the restart process cannot proceed if a terminated daemon remains a zombie. Typically to ensure the proper `init` process, one should use the `--init` option for `docker run` command.

## Reporting Issues ##

* ION related issues can be reported to the public GitHub page for ION-DTN or ion-core.
* ION's SourceForge page is now deprecated and issued reported there will not be monitored.


