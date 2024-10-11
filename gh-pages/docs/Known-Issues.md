# Knowledge Base, Issues & Patches

This is a short list of information regarding ION operation, known issues, and patches. 

Most of these information are likely to be found in other longer documents but it is presented here in a summarized form for easier search. Another useful document is the [ION Deployment Guide](./ION-Deployment-Guide.md) which contains recommendations on configuring and running ION and performance data.

## Permission Issues

When encountering any "permission denied" issues during installation, it is recommended that you:
  
1. Run `sudo make uninstall` and `make clean` to clear all previous ION build artifact and files, and
2. Review files and folders in the ION code's root directories (include subdirectories) that are owned by "root" and remove or change ownership. This occurs when ION was previously build and tested by the root user and was not properly removed from the system.

## UDP Convergence Layer Adaptor (CLA)

When using UDP CLA for data delivery, one should be aware that:

* UDP is inherently unreliable. Therefore the delivery of BP bundles may not be guaranteed, even within a controlled, isolated network environment.

* It is best to use `iperf` and other performance testing tools to properly character UDP performance before using UDP CLA. UDP loss rates may flucturate due to presence of other competing, non-DTN traffic on the host, or due to insufficient kernel buffer space configured in the host.

* When UDP CLA is used to deliver bundles larger than 64 kilo-byte, those bundles will be fragmented and reassembled at the destination. It has been observed on some platforms that UDP buffer overflow can prevent a large number of bundles from being reassembled at the destination node. These bundle fragments (which are themselves bundles) will take up ION storage and remain until (a) remaining fragments arrived for reassemble or (b) the Time-to-Live (TTL) of the bundle expired.

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

When developing and testing ION in a docker container with root permission while mounting to ION code residing in a user's directory on the host machine, file ownership may switch from user to `root`. 

## ionconfig

### Memory/Storage Allocation

* To set the `heapWord` parameter, it is recommended that you consider the worst case buffering need for a node, and use at least 5 times more for heap. For example, if you expect that your DTN node will need to buffer as much as 100M bytes of data during operation, you should allocate at least 500M bytes (or more) to the heap. Each heap `word` is determined by the size of the operation system. For a 64 bit system, each word is 8 bytes long. For in our example, the `heapWord` should be 62.5 mega or 62500000 words.
* Based on testing results and assuming using the default `maxHeap` parameter in `ionrc`, we recommend the following minimum setting for the ION working memory `wmSize` in bytes: 

```
wmSize = 3 x heapWords x 8 x 0.4 / 10
```
where 3 is the margin we recommend, 8 is the number of octets per word, 0.4 accounts for the fact that inbound and outbound heap space is only 40% of the total heapWord, and 10 accounts for the empirically estimated 10:1 ratio between the heap and working memory footprints per bundle stored.

## SDR 

## Shutdown ION

When writing a customized ION shutdown script, it is recommended that you stop the various daemons, if present, by running the various daemon administration programs, with a single period '.' as argument, in the following general order: 

* `cfdpadmin .` 
* If running BPv6, `acsadmin .` and `imcadmin .`
* `bpadmin .`
* `ltpadmin .`
* `bsspadmin .`
* `ipnadmin .`
* `ionadmin .`

The key here is that `ionadmin .` should be the last command to run in the shutdown process because most ION daemons are attached to the data structure and shared memory initialized by the `ionadmin` program. When these daemons execute a nominal shut down, they will try to _detach_ from ION first. Therefore, it is important to keep the ION's SDR and various Interprocess Communication (IPC) infrastructure in place until the very end, after all other processes have terminated.

If `ionadmin .` is not run last, it is possible some daemon can get stuck and will require manaul termination using `kill -9` command.

After ION shutdown completed, it is also recommended that you remove any file-based SDR and SDR log in the `/tmp` directory (or a customized directory you specified in the `.ionconfig` file). 

In the working directory where ION was launched, there may also be temporary files in the form of `bpacq.*` and `ltpacq.*`. These files are remnants of bundle and ltp segment acquisition processes that did not terminate nominally. Although doese files do not interfere with subsequent instances of ION operations, they can accumulate and take up storage space. So it is recommended that they be removed manually or via an automated script.

## SDR transaction reversal

When SDR transaction is canceled due to anomaly, ION will attempt automatically try the following:

1. Reverse transaction - if it is configured - to revert modifications to the SDR's heap space which contains both user and protocol data units. This action rolls back a series of operations on the SDR's data of the cancelled the transaction.
2. Once the SDR's heap space has been restored, the "volatile" state of the protocols must be restored because they might be modified by the transaction as well. This is performed by the `ionrestart` utility.
3. After the volatiles are reloaded, the 3rd step of restoring ION operation will need to be triggered by the users. During the anomously event that caused the transaction cancellation, some of ION's various daemons may have stopped. They can be restored by simply issuing the start ('s') command through `ionadmin` and `bpadmin`.

## Using ionrestart

When SDR transaction was reversed (when enabled) or cancelled, it is likely some degree of data corruption remains in ION. The `ionrestart` utility program will be triggered to reload the volatile state of the protocol stack and restarts ION's various daemons to ensure that ION can return to a consistent operational state.

During the reloading of the volatile state, the bundle protocol schemes, inducts, and outducts are stopped by terminating the associated daemons. Then ionrestart process will wait for the daemon's to be terminated before restarting them again.

To understand how ionrestart operates, you can look examples in the `reversibility check` test #1 and #2 under the `tests` folder.

## 'Init' Process PID 1

When running ION inside a docker container, the `init` process (PID 1) should be properly configured to reap all zombie processes because the restart process cannot proceed if a terminated daemon remains a zombie. Typically to ensure the proper `init` process, one should use the `--init` option for `docker run` command.

## Permission Issue with named semaphore

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
| TBD | 33 | bpv7 extension block CRC failure | TBD | 4.1.3 | 4.1.4 |



