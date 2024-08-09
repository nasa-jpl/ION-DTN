# Known Issues & Patches

Here is a list of known issues that will be updated on a regular basis to captures various lessons-learned relating to the configurations, testing, performance, and deployment of ION:

## Permission Issues

When encountering any "permission denied" issues during installation, it is recommended that you:
  
1. Run `sudo make uninstall` and `make clean` to clear all previous ION build artifact and files, and
2. Review files and folders in the ION code's root directories (include subdirectories) that are owned by "root" and remove or change ownership. This occurs when ION was previously build and tested by the root user and was not properly removed from the system.

## UDP Convergence Layer Adaptor (CLA)

When using UDP CLA for data delivery, one should be aware that:

* UDP is inherently unreliable. Therefore the delivery of BP bundles may not be guaranteed, even within a controlled, isolated network environment.

* It is best to use `iperf`` and other performance testing tools to properly character UDP performance before using UDP CLA. UDP loss rates may flucturate due to presence of other competing, non-DTN traffic on the host, or due to insufficient kernel buffer space configured in the host.

* When UDP CLA is used to deliver bundles larger than 64 kilo-byte, those bundles will be fragmented and reassembled at the destination. It has been observed on some platforms that UDP buffer overflow can prevent a large number of bundles from being reassembled at the destination node. These bundle fragments (which are themselves bundles) will take up ION storage and remain until (a) remaining fragments arrived for reassemble or (b) the Time-to-Live (TTL) of the bundle expired.

## LTP CLA

When using LTP over the UDP-based communication services (udpcli and udpclo daemons), the network layer MTU and kernel buffer size should be properly configured

The use of "WAN emulator" to add delay and probabilistic loss to data should be careful not to filter out UDP fragments that are needed to reconstruct the LTP segments or significantly delayed them such that the UDP segment reassembly will expire.

## CRC

ION implementation versions, prior to and including version 4.1.2, apply CRC16 to Primary Block only. All other blocks are not protected by any CRC mechanism. In order To apply CRC to the Payload Block, a compiler flag needs to be set when building ION. There are currently no mechanism to dynamically turn on/turn off CRC options without recompiling ION. 

After ION 4.1.3, CRC16 will be turn on for primary block as well as all canonical blocks by default.

## Testing & Configuration

When developing and testing ION in a docker container with root permission while mounting to ION code residing in a user's directory on the host machine, file ownership may switch from user to `root`. 

This sometimes leads to build and test errors when one switches back to the host's native environment. Therefore, we recommend that you execute the `make clean` and `git stash` command to remove all build and testing artifacts from ION 's source directory before exiting a docker container.

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



