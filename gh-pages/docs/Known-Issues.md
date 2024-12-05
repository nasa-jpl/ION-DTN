# Knowledge Base, Issues & Patches

This is a short list of information regarding ION operation, known issues, and patches. 

Most of these information are likely to be found in other longer documents but it is presented here in a summarized form for easier search. Another useful document is the [ION Deployment Guide](./ION-Deployment-Guide.md) which contains recommendations on configuring and running ION and performance data.

## Convergence Layer Adaptor

### UDP CLA

* When using UDP CLA for data delivery, one should be aware that:
  * UDP is inherently unreliable. Therefore the delivery of BP bundles may not be guaranteed, even withing a controlled, isolated network environment.
  * It is best to use iperf and other performance testing tools to properly character UDP performance before using UDP CLA. UDP loss may have high loss rate due to presence of other traffic or insufficient internal buffer.
  * When UDP CLA is used to deliver bundles larger than 64K, those bundle will be fragmented and reassembled at the destination. It has been observed on some platforms that UDP buffer overflow can cause a large number of 'cyclic' packet drops so that an unexpected large number of bundles are unable to be reassembled at the destination. These bundle fragments (which are themselves bundles) will take up storage and remain until either (a) the remaining fragments arrived or (b) the TTL expired.

### LTP CLA

* When using LTP over the UDP-based communication services (udpcli and udpclo daemons):
  * The network layer MTU and kernel buffer size should be properly configured
  * The use "WAN emulator" to add delay and probabilistic loss to data should be careful not to filter out UDP fragments that are needed to reconstruct the LTP segments or significantly delayed them such that the UDP segment reassembly will expire.

## Bundle Protocol

### Routing

ION handles routing based on the following general hierarchy:

1. Routing Override in the `ipnrc`
2. Rerouting toward `gateway` instead of `destination`
3. Routing using CGR - for either `gateway` or `destination`
4. Routing to a neighbor if that neighbor happen to be either the `gateway` or `destination`
5. Routing to an `exit` node
6. Place bundle in `limbo` state awaiting either TTL expiration or rerouting

### CRC

* ION implementation currently default will apply CRC16 to Primary Block but not the Payload Block. To apply CRC to the Payload Block, a compiler flag needs to be set when building ION. There are currently no mechanism to dynamically turn on/turn off CRC without recompiling ION.

## Testing & Configuration

* When developing and testing ION in a docker container with root permission while mounting to ION code residing in a user's directory on the host machine, file ownership may switch from user to `root`. This sometimes leads to build and test errors when one switches back to the host's development and testing environment. Therefore, we recommend that you execute the `make clean` and `git stash` command to remove all build and testing artifacts from ION 's source directory before exiting the container.

## ionconfig

### Memory/Storage Allocation

* To set the `heapWord` parameter, it is recommended that you consider the worst case buffering need for a node, and use at least 5 times more for heap. For example, if you expect that your DTN node will need to buffer as much as 100M bytes of data during operation, you should allocate at least 500M bytes (or more) to the heap. Each heap `word` is determined by the size of the operation system. For a 64 bit system, each word is 8 bytes long. For in our example, the `heapWord` should be 62.5 mega or 62500000 words.
* Based on testing results and assuming using the default `maxHeap` parameter in `ionrc`, we recommend the following minimum setting for the ION working memory `wmSize` in bytes: 

```
wmSize = 3 x heapWords x 8 x 0.4 / 10
```
where 3 is the margin we recommend, 8 is the number of octets per word, 0.4 accounts for the fact that inbound and outbound heap space is only 40% of the total heapWord, and 10 accounts for the empirically estimated 10:1 ratio between the heap and working memory footprints per bundle stored.

## SDR 

### SDR transaction reversal

When SDR transaction is canceled due to anomaly, ION will attempt automatically try the following:

1. Reverse transaction - if it is configured - to revert modifications to the SDR's heap space which contains both user and protocol data units. This action rolls back a series of operations on the SDR's data of the cancelled the transaction.
2. Once the SDR's heap space has been restored, the "volatile" state of the protocols must be restored because they might be modified by the transaction as well. This is performed by the `ionrestart` utility.
3. After the volatiles are reloaded, the 3rd step of restoring ION operation will need to be triggered by the users. During the anomously event that caused the transaction cancellation, some of ION's various daemons may have stopped. They can be restored by simply issuing the start ('s') command through `ionadmin` and `bpadmin`.

### 'Init' Process PID 1

The reloading of the volatile state and restarting of daemons is necessary to ensure the ION system is in a consistent state before resuming normal operations.

During the reloading of the volatile state, the bundle protocol schemes, inducts, and outducts are stopped by terminating the associated daemons. The restart process will wait for the daemon's to be terminated before restarting them. When running ION inside a docker container, the `init` process (PID 1) should be properly configured to reap all zombie processes because the restart process cannot proceed if a terminated daemon remains a zombie. Typically to ensure the proper `init` process, one should use the `--init` option for `docker run` command.

### Permission Issue with named semaphore

If you encounter an error reported in ion.log file such as this:

```text
at line 3850 of ici/library/platform_sm.c, Can't initialize IPC semaphore: Permission denied (/ion:GLOBAL:ipcSem)
at line 3868 of ici/library/platform_sm.c, Can't initialize IPC.
at line 481 of ici/sdr/sdrxn.c, Can't initialize IPC system.
at line 695 of ici/library/ion.c, Can't initialize the SDR system.
at line 216 of ici/utils/ionadmin.c, ionadmin can't initialize ION.
```

It indicates that ION is unable to clean out previously left behind semaphore files. This typically occurs when the previous ION run was launched by a different user, and ION was not properly shutdown via a shutdown script - instead, the global `ionstop` or `killm` script was used. The semaphore files used by POSIX named semaphore typically only allows the owner to delete it. The work around is to clear these files out. ION-related semaphore files have the name pattern of `sem.ion:GLOBAL:<integer>`. For Ubuntu, it is usually found in the `/dev/shm` directory; for other Linux distribution, the location can be different.

## Reporting Issues

* ION related issues can be reported to the public GitHub page for ION-DTN or ion-core.
* ION's SourceForge page is now deprecated and issued reported there will not be monitored.

## Patches

In this section, we post patches issued between or ahead of major releases to fix bugs.

Each patch is described as follows:

- Issue Date: this is the date when the patch is made available.
- Issue No.: GitHub issue number, if any, related to the patch.
- Issue description
- Link to the patch
- The baseline version of ION from which the patch is issued.
- The target version of ION to which the patch will incorporate. 

| Issue Date | Issue # | Issue Description | Patch | From ION ver. | To ION ver. |
|------------|---------|-------------------|-------|-----|-----|
| 5/30/2024 | 33 | bpv7 extension block CRC failure | deferred to 4.1.3s | 4.1.3 | 4.1.3s |

## Security Advisories

The following security advisories are associated with ION:

- [CVE-2024-54130](https://github.com/nasa-jpl/ION-DTN/security/advisories/GHSA-7pj7-hfwv-q3v6) - fixed in 4.1.3s
- [CVE-2024-54129](https://github.com/nasa-jpl/ION-DTN/security/advisories/GHSA-393w-w6jh-pq3j) - fixed in 4.1.3s

