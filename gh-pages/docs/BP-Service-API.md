# BP Service API

This tutorial goes over the basic user APIs for developing application software to take advantage of the Bundle Protocol (BP) services provided by ION.

## Pre-requisite

For a user-developed software to utilize BP services provided by ION, there are two pre-requisite conditions: (1) ION services must be properly configured and already running in a state ready to provide BP services, and (2) BP libraries and header files must be installed and linked to the user application.

## Check ION Installation & BP Version

A simple way to check if ION is installed in the host is to determine the installation location of ION, one can execute `which ionadmin` and it will show the directory where ionadmin is currently located:

```bash
$ which ionadmin
/usr/local/bin/ionadmin
```

If ionadmin is not found, it means either ION was not built or not properly installed in the execution path. If ionadmin is found, run it and provide the command `v` at the `:` prompt to determine the installed ION software version. For example:

```bash
$ ionadmin
: v
ION-OPEN-SOURCE-4.1.2
```

Quit ionadmin by command `q`. Quitting `ionadmin` will not terminate any ION and BP services, it simply ends ION's user interface for configuration query and management. If you see warning messages such as those shown below when quitting `ionadmin`, then it confirms that ION is actually not running at the moment. There are no software errors:

```bash
: q
at line 427 of ici/library/platform_sm.c, Can't get shared memory segment: Invalid argument (0)
at line 312 of ici/library/memmgr.c, Can't open memory region.
at line 367 of ici/sdr/sdrxn.c, Can't open SDR working memory.
at line 513 of ici/sdr/sdrxn.c, Can't open SDR working memory.
at line 963 of ici/library/ion.c, Can't initialize the SDR system.
Stopping ionadmin.
```

## Determine BP Service State

Once it is determined that ION has been installed, a user may want to determine whether BP service is running by checking for the presence of various BP daemons and shared memory/semaphores, which are created by ION for interprocess communications among the various the BP service daemons.

To check if BP service is running, you can list current running processes using `ps -aux` command and inspect if you see the following BP service daemons:

* `rfxclock` - background daemon that schedules ION network configuration events
* `bpclm` - background daemon that selects and meters data transmission to outbound convergence layers
* `bpclock` - background daemon that schedule BP level events
* `bptransit` - daemon to move inbound traffic to outbound queue for forwarding
* `ipnfw` - BP IPN scheme-specific bundle routing daemon
* `ipnadminep` - IPN scheme-specific administrative end-point daemon

You can find more details about these daemons in the manual pages. You may also see  daemons related to other activate modules. For example, if the LTP engine is active in the system, you will see the following daemons:

* `ltpclock`
* `ltpdeliv`
* `ltpmeter`
* `udplso`
* `udplsi`
* `ltpcli`
* `ltpclo`

To further verify that BP service is running, you can check for presence of ION shared memory and semaphores:

```bash
------ Shared Memory Segments --------
key        shmid      owner         perms      bytes      nattch     status
0x0000ee02 47         userIon       666        641024     13
0x0000ff00 48         userIon       666        50000000   13
0x93de0005 49         userIon       666        1200002544 13
0x0000ff01 50         userIon       666        500000000  13

------ Semaphore Arrays --------
key        semid      owner         perms      nsems
0x0000ee01 23         userIon       666        1
0x18020001 24         userIon       666        250
```

In this example, the shared memory and semaphore keys for the SDR heap space (shmid 49) and the semaphorebase (semid 24) are created using a random key generated from the process ID of `ionadmin` and they will vary each time ION is instantiated. This is specific to SVR4 semaphore, which is the default for ION 4.1.2. However, starting with ION 4.1.3, the default semaphore will switch to POSIX semaphore and the output will be different. The other memory and semaphore keys listed in this example are typical default values, but they too, can be changed through ionconfig files.

### If ION is installed but not running

If ION is installed but not running (either you see no shared memory or don't see the BP service daemons) you can restart ION. Before restarting ION, run `ionstop` to clear all remaining orphaned processes/shared memory allocations in case the previous instance of BP service was not properly shutdown or has suffered a crash. Here is an example of the output you may see:

```bash
$ ionstop
IONSTOP will now stop ion and clean up the node for you...
cfdpadmin .
Stopping cfdpadmin.
bpv7
bpadmin .
Stopping bpadmin.
ltpadmin .
Stopping ltpadmin.
ionadmin .
Stopping ionadmin.
bsspadmin .
BSSP not initialized yet.
Stopping bsspadmin.
This is a single-ION instance configuration. Run killm.
killm
Sending TERM to acsadmin lt-acsadmin    acslist lt-acslist      aoslsi lt-aoslsi        aoslso lt-aoslso        bibeadmin lt-bibeadmin  bibeclo lt-bibeclo     bpadmin lt-bpadmin      bpcancel lt-bpcancel    bpchat lt-bpchat        bpclm lt-bpclm  bpclock lt-bpclock      bpcounter lt-bpcounter         bpdriver lt-bpdriver    bpecho lt-bpecho        bping lt-bping  bplist lt-bplist        bpnmtest lt-bpnmtest    bprecvfile lt-bprecvfile       bpsecadmin lt-bpsecadmin        bpsendfile lt-bpsendfile        bpsink lt-bpsink        bpsource lt-bpsource    bpstats lt-bpstats     bpstats2 lt-bpstats2    bptrace lt-bptrace      bptransit lt-bptransit  brsccla lt-brsccla      brsscla lt-brsscla      bsscounter lt-bsscounter       bssdriver lt-bssdriver  bsspadmin lt-bsspadmin  bsspcli lt-bsspcli      bsspclo lt-bsspclo      bsspclock lt-bsspclock  bssrecv lt-bssrecv     bssStreamingApp lt-bssStreamingApp      cgrfetch lt-cgrfetch    cpsd lt-cpsd    dgr2file lt-dgr2file    dgrcli lt-dgrcli        dgrclo lt-dgrclo        dtka lt-dtka    dtkaadmin lt-dtkaadmin         dtn2admin lt-dtn2admin  dtn2adminep lt-dtn2adminep      dtn2fw lt-dtn2fw        dtpcadmin lt-dtpcadmin  dtpcclock lt-dtpcclock         dtpcd lt-dtpcd  dtpcreceive lt-dtpcreceive      dtpcsend lt-dtpcsend    file2dgr lt-file2dgr    file2sdr lt-file2sdr    file2sm lt-file2sm     file2tcp lt-file2tcp    file2udp lt-file2udp    hmackeys lt-hmackeys    imcadmin lt-imcadmin    imcadminep lt-imcadminep      imcfw lt-imcfw   ionadmin lt-ionadmin    ionexit lt-ionexit      ionrestart lt-ionrestart        ionsecadmin lt-ionsecadmin      ionunlock lt-ionunlock         ionwarn lt-ionwarn      ipnadmin lt-ipnadmin    ipnadminep lt-ipnadminep        ipnd lt-ipnd    ipnfw lt-ipnfw  lgagent lt-lgagent     lgsend lt-lgsend        ltpadmin lt-ltpadmin    ltpcli lt-ltpcli        ltpclo lt-ltpclo        ltpclock lt-ltpclock    ltpcounter lt-ltpcounter       ltpdeliv lt-ltpdeliv    ltpdriver lt-ltpdriver  ltpmeter lt-ltpmeter    ltpsecadmin lt-ltpsecadmin      nm_agent lt-nm_agent  nm_mgr lt-nm_mgr         owltsim lt-owltsim      owlttb lt-owlttb        psmshell lt-psmshell    psmwatch lt-psmwatch    ramsgate lt-ramsgate  rfxclock lt-rfxclock     sdatest lt-sdatest      sdr2file lt-sdr2file    sdrmend lt-sdrmend      sdrwatch lt-sdrwatch    sm2file lt-sm2file    smlistsh lt-smlistsh     smrbtsh lt-smrbtsh      stcpcli lt-stcpcli      stcpclo lt-stcpclo      tcaadmin lt-tcaadmin    tcaboot lt-tcaboot    tcacompile lt-tcacompile tcapublish lt-tcapublish        tcarecv lt-tcarecv      tcc lt-tcc      tccadmin lt-tccadmin    tcp2file lt-tcp2file  tcpbsi lt-tcpbsi         tcpbso lt-tcpbso        tcpcli lt-tcpcli        tcpclo lt-tcpclo        udp2file lt-udp2file    udpbsi lt-udpbsi      udpbso lt-udpbso         udpcli lt-udpcli        udpclo lt-udpclo        udplsi lt-udplsi        udplso lt-udplso                amsbenchr lt-amsbenchr         amsbenchs lt-amsbenchs  amsd lt-amsd    amshello lt-amshello    amslog lt-amslog        amslogprt lt-amslogprt  amsshell lt-amsshell   amsstop lt-amsstop      bputa lt-bputa  cfdpadmin lt-cfdpadmin  cfdpclock lt-cfdpclock  cfdptest lt-cfdptest    bpcp lt-bpcp    bpcpd lt-bpcpd ...
Sending KILL to the processes...
Checking if all processes ended...
Deleting shared memory to remove SDR...
Killm completed.
ION node ended. Log file: ion.log
```

At this point run `ps -aux` or `ipcs` to verify that ION has terminated completely.

### Simple ION Installation Test

When ION is not running, you can perform a simple unit test to verify ION is built properly.

Navigate to the root directory of the ION source code, `cd` into the `tests` folder and then execute a `bping` test using the command:

```bash
./runtest bping
```

You can watch the terminal output ION restarting itself and executing a loopback ping. When successful it will indicate at the end of the test:

```bash
TEST PASSED!

passed: 1
    bping

failed: 0

skipped: 0

excluded by OS type: 0

excluded by BP version: 0

obsolete tests: 0
```

## Locate ION Libraries and Header Files

The standard `./configure; make; sudo make install; sudo ldconfig` process should automatically install the BP libraries under `/usr/local/lib` and the relevant header files under `/usr/local/include` unless ION is specifically configured to a different customized install location through the `./configure` script during the compilation process. Here is a list of libraries and header files you should find there:

```bash
$ cd /usr/local/lib
$ ls
libamp.a                 libbp.so.0.0.0    libcgr.a          libdtpc.la        libmbedcrypto.so.7   libtc.so.0.0.0      libudpcla.a
libamp.la                libbss.a          libcgr.la         libdtpc.so        libmbedtls.a         libtcaP.a           libudpcla.la
libamp.so                libbss.la         libcgr.so         libdtpc.so.0      libmbedtls.so        libtcaP.la          libudpcla.so
libamp.so.0              libbss.so         libcgr.so.0       libdtpc.so.0.0.0  libmbedtls.so.14     libtcaP.so          libudpcla.so.0
libamp.so.0.0.0          libbss.so.0       libcgr.so.0.0.0   libici.a          libmbedx509.a        libtcaP.so.0        libudpcla.so.0.0.0
libampAgentADM.a         libbss.so.0.0.0   libdgr.a          libici.la         libmbedx509.so       libtcaP.so.0.0.0    libudplsa.a
libampAgentADM.la        libbssp.a         libdgr.la         libici.so         libmbedx509.so.1     libtcc.a            libudplsa.la
libampAgentADM.so        libbssp.la        libdgr.so         libici.so.0       libstcpcla.a         libtcc.la           libudplsa.so
libampAgentADM.so.0      libbssp.so        libdgr.so.0       libici.so.0.0.0   libstcpcla.la        libtcc.so           libudplsa.so.0
libampAgentADM.so.0.0.0  libbssp.so.0      libdgr.so.0.0.0   libltp.a          libstcpcla.so        libtcc.so.0         libudplsa.so.0.0.0
libams.a                 libbssp.so.0.0.0  libdtka.a         libltp.la         libstcpcla.so.0      libtcc.so.0.0.0     libzfec.a
libams.la                libcfdp.a         libdtka.la        libltp.so         libstcpcla.so.0.0.0  libtcpbsa.a         libzfec.la
libbp.a                  libcfdp.la        libdtka.so        libltp.so.0       libtc.a              libtcpbsa.la        libzfec.so
libbp.la                 libcfdp.so        libdtka.so.0      libltp.so.0.0.0   libtc.la             libtcpbsa.so        libzfec.so.0
libbp.so                 libcfdp.so.0      libdtka.so.0.0.0  libmbedcrypto.a   libtc.so             libtcpbsa.so.0      libzfec.so.0.0.0
libbp.so.0               libcfdp.so.0.0.0  libdtpc.a         libmbedcrypto.so  libtc.so.0           libtcpbsa.so.0.0.0


$ cd /usr/local/include
$ ls
ams.h               bpsec_instr.h  cfdpops.h  eureka.h       llcv.h    platform.h     rfc9173_utils.h  sdrhash.h    sdrxn.h    tcaP.h
bcb_aes_gcm_sc.h    bpsec_util.h   crypto.h   icinm.h        ltp.h     platform_sm.h  rfx.h            sdrlist.h    smlist.h   tcc.h
bib_hmac_sha2_sc.h  bss.h          dgr.h      ion.h          lyst.h    psa            sci.h            sdrmgt.h     smrbt.h    tccP.h
bp.h                bssp.h         dtka.h     ion_test_sc.h  mbedtls   psm.h          sda.h            sdrstring.h  sptrace.h  zco.h
bpsec_asb.h         cfdp.h         dtpc.h     ionsec.h       memmgr.h  radix.h        sdr.h            sdrtable.h   tc.h
```

In this document, we assume that ION was built and installed via the `./configure` installation process using the full open-source codebase and with the standard set of options.

The location and content of the library and header directories shown above included non-BP modules and may not match exactly with what you have especially if you have built ION with features options enabled via `./configure` or used a manual/custom Makefile, or built ION from the `ion-core` package instead.

## Launch ION & BP Services

Once you are confident that ION has been properly built and installed in the system, you can start BP service by launching ION. To do this, please consult the various tutorials under _Configuration_.

After launching ION, you can [verify BP service status](#determine-bp-service-state) in the same manner as described in previous section.

## BP Service API Reference

### Header

```c
#include "bp.h"
```

### Critical Resource Management Warnings

**SDR Heap Management**

ION's SDR (Simple Data Recorder) heap is a fixed-size, pre-allocated storage pool shared by all bundles, ZCOs (zero-copy objects), and metadata in the system. Exhausting this heap will cause bp_send() to fail with "No space for ZCO" errors and can potentially destabilize the BP agent, preventing both sending and receiving of bundles.

To prevent SDR heap exhaustion:

* **Monitor SDR usage** - Use `bpadmin` and `ionadmin` tools to regularly check SDR occupancy
* **Use file-based ZCOs for large payloads** - For payloads larger than a few kilobytes, consider using file-based ZCOs instead of SDR-based ZCOs to avoid consuming heap space
* **Release detained bundles promptly** - If you open endpoints with detention enabled (`detain=1`), always call `bp_release()` on bundles when you're done tracking them. Detained bundles remain in the SDR until explicitly released or their TTL expires
* **Configure adequate SDR size** - Set appropriate SDR heap size during ION initialization based on your expected traffic patterns and bundle sizes
* **Always call bp_release_delivery()** - After every `bp_receive()` call, ensure you call `bp_release_delivery()` to free resources, even if no payload was received

Failure to follow these practices can lead to resource exhaustion, which will manifest as:
- "No space for ZCO" errors when calling bp_send()
- Inability to receive bundles due to lack of storage
- BP agent instability or crashes

**Endpoint Ownership**

Each BP endpoint can only be opened by one application at a time. Attempting to call `bp_open()` or `bp_open_source()` on an endpoint that is already open by another process will fail with "Endpoint is already open" error.

An endpoint becomes available for opening when:
- The owning application calls `bp_close()`
- The owning application terminates

Note: Opening an endpoint with `bp_open_source()` grants the ability to send from that endpoint with detention capability. To receive bundles at an endpoint, use `bp_open()`.

**ION Shutdown Order and Application Graceful Termination**

When ION shuts down via `ionstop` or `bpadmin .`, the shutdown process calls `sm_SemEnd()` on all endpoint semaphores to signal applications to terminate gracefully. Well-written BP applications that use `bp_receive()` will automatically detect this condition and exit cleanly.

**How Applications Detect Shutdown:**

When `bpadmin .` executes, it calls `sm_SemEnd()` on endpoint semaphores. Applications blocked in `bp_receive()` will wake up and receive `BpEndpointStopped` in the delivery result. Properly written applications should check for this condition and exit gracefully:

```c
while (running)
{
    if (bp_receive(sap, &dlv, BP_BLOCKING) < 0)
    {
        putErrmsg("bp_receive failed.", NULL);
        running = 0;
        continue;
    }

    switch (dlv.result)
    {
    case BpEndpointStopped:
        writeMemo("Endpoint stopped - shutting down.");
        running = 0;  // Exit gracefully
        break;

    case BpPayloadPresent:
        // Process bundle
        process_bundle(&dlv);
        break;

    default:
        break;
    }

    bp_release_delivery(&dlv, 1);
}
```

**Best Practices for Application Shutdown Handling:**

1. **Always check for BpEndpointStopped** - After every `bp_receive()` call, check if `dlv.result == BpEndpointStopped` and exit your main loop
2. **Use BP_BLOCKING mode** - Applications using `BP_BLOCKING` will wake up immediately when the endpoint is stopped
3. **Clean up resources** - Call `bp_close()` and `bp_detach()` before exiting
4. **Handle SIGTERM gracefully** - Implement signal handlers to allow operators to terminate your application manually
5. **Avoid long operations between bp_receive() calls** - If your application performs lengthy processing between receive calls, it may delay shutdown

**When Applications Don't Detect Shutdown Properly:**

If BP applications do not check for `BpEndpointStopped` or are blocked in operations outside of `bp_receive()`, they may not respond promptly to ION shutdown. In such cases, `bpadmin .` can hang indefinitely waiting for processes to terminate.

**Recommended Safe Shutdown Sequence:**

For environments where the ION operator and application users are different people, or when applications may not be shutdown-aware:

1. **Terminate all BP applications first** - Send SIGTERM to applications like bprecvfile, bpsendfile, and custom BP applications
2. **Run `bpadmin .`** - After all applications have exited (or after a reasonable timeout)
3. **Run `ionadmin .`** - To complete ION shutdown
4. **If processes hang, use `killm`** - To forcibly cleanup all ION processes and IPC resources

**For Application Developers:**

Design your applications to be "ION shutdown aware" by always checking for `BpEndpointStopped`. This allows ION operators to shut down the system cleanly without needing to coordinate with application users. Well-designed applications will automatically detect and respond to ION shutdown, making the system more reliable and easier to operate.

**Example: Shutdown-Aware Application Pattern**

```c
#include "bp.h"
#include <signal.h>

static int running = 1;

void sigterm_handler(int sig)
{
    writeMemo("Received SIGTERM - will exit after current operation.");
    running = 0;
}

int main(int argc, char **argv)
{
    BpSAP sap;
    BpDelivery dlv;

    // Set up signal handler
    signal(SIGTERM, sigterm_handler);

    if (bp_attach() < 0)
        return -1;

    if (bp_open("ipn:1.1", &sap) < 0)
    {
        bp_detach();
        return -1;
    }

    while (running)
    {
        if (bp_receive(sap, &dlv, BP_BLOCKING) < 0)
        {
            running = 0;
            continue;
        }

        // Check for ION shutdown
        if (dlv.result == BpEndpointStopped)
        {
            writeMemo("ION is shutting down - exiting gracefully.");
            running = 0;
        }
        else if (dlv.result == BpPayloadPresent)
        {
            // Process bundle here
        }

        bp_release_delivery(&dlv, 1);
    }

    // Clean shutdown
    bp_close(sap);
    bp_detach();
    writeMemo("Application terminated cleanly.");
    return 0;
}
```

This pattern ensures your application will:
- Respond to ION shutdown automatically via `BpEndpointStopped`
- Respond to operator SIGTERM signals
- Clean up resources properly before exiting
- Allow ION to shut down without hanging

---

### bp_attach

Function Prototype

```c
int bp_attach( )
```

Parameters

* None.

Return Value

* On success: 0
* Any error: -1

Example Call

```c
if (bp_attach() < 0)
{
        printf("Can't attach to BP.\n");
        /* user inser error handling code */
}
```

Description

Typically the `bp_attach()` call is made at the beginning of a user's application to attach to BP Service provided by ION in the host machine. This code checks for a negative return value.

`bp_attach()` automatically calls the ICI API `ion_attach()` when necessary, so there is no need to call them separately. In addition to gaining access to ION's SDR, which is what `ion_attach()` provides, `bp_attach()` also gains access to the Bundle Protocol's state information and database. For user application that interacts with the Bundle Protocol, `bp_attach()` is the entry point to ION.

---

### Sdr bp_get_sdr( )

Function Prototype

```c
Sdr bp_get_sdr()
```

Parameters

* None.

Return Value

* Handle for the SDR data store: success
* `NULL`: any error

Example Call

```c
/* declare SDR handle */
Sdr sdr;

/* get SDR handle */
sdr = bp_get_sdr();

/* user check sdr for NULL
 * and handle error */
```

Description

Returns handle for the SDR data store used for BP, to enable creation and interrogation of bundle payloads (application data units). Since the SDR handle is needed by many APIs, this function is typically executed early in the user's application in order to access other BP services.

---

### bp_detach

Function Prototype

```c
void bp_detach(void)
```

Parameters

* None.

Return Value

* None (void function)

Description

Terminates all access to BP functionality for the invoking process. This function does not return a value.

---

### bp_agent_is_started

Function Prototype

```c
int bp_agent_is_started(void)
```

Parameters

* None.

Return Value

* 1: BP agent is running
* 0: BP agent is not running

Description

Returns 1 if the local BP agent has been started and not yet stopped, 0 otherwise. This is a useful diagnostic function to verify that the BP service is running before attempting to send or receive bundles.

Example Usage

```c
if (!bp_agent_is_started())
{
    putErrmsg("BP agent not running.", NULL);
    return -1;
}
```

---

### bp_open

Function Prototype

```c
int bp_open(char *eid, BpSAP *ionsapPtr)
```

Parameters

* `*eid`: name of the endpoint
* `*ionsapPtr`: pointer to variable in which address of BP service access point will be returned

Return Value

* 0: success
* -1: any error

Example Call

```c
if (bp_open(ownEid, &sap) < 0)
{
        putErrmsg("bptrace can't open own endpoint.", ownEid);

        /* user's error handling function here */
}
```

Description

Opens the application's access to the BP endpoint identified by the string at `eid`, so that the application can take delivery of bundles destined for the indicated endpoint. This SAP can also be used for sending bundles whose source is the indicated endpoint.

Please note that all bundles sent via this SAP will be subject to immediate destruction upon transmission, i.e., no bundle addresses will be returned by `bp_send` for use in tracking, suspending/resuming, or cancelling transmission of these bundles.

On success, places a value in *ionsapPtr that can be supplied to future bp function invocations.

__NOTE:__ To allow for bp_send to return a bundle address for tracking purpose, please use `bp_open_source` instead.

---

### bp_open_source

Function Prototype

```c
int bp_open_source(char *eid, BpSAP *ionsapPtr, int detain)
```

Parameters

* `*eid`: name of the endpoint
* `*ionsapPtr`: pointer to variable in which address of BP service access point will be returned
* `detain`: indicator as to whether or not bundles sourced using this BpSAP should be detained in storage until explicitly released

Return Value

* 0: success
* -1: Any error

Example Call

```c
if (bp_open_source(ownEid, &txSap, 1) < 0)
{
        putErrmsg("can't open own 'send' endpoint.", ownEid);

        /* user error handling routine here */
}
```

Description

Opens the application's access to the BP endpoint identified by eid, so that the application can send bundles whose source is the indicated endpoint. If and only if the value of detain is non-zero, citing this SAP in an invocation of bp_send() will cause the address of the newly issued bundle to be returned for use in tracking, suspending/resuming, or cancelling transmission of this bundle.

__USE THIS FEATURE WITH GREAT CARE__: such a bundle will continue to occupy storage resources until it is explicitly released by an invocation of bp_release() or until its time to live expires, so bundle detention increases the risk of resource exhaustion. (If the value of detain is zero, all bundles sent via this SAP will be subject to immediate destruction upon transmission.)

On success, places a value in *ionsapPtr that can be supplied to future bp function invocations and returns 0. Returns -1 on any error.

---

### bp_send

Function Prototype

```c
int bp_send(BpSAP sap, char *destEid, char *reportToEid, int lifespan,
                int classOfService, BpCustodySwitch custodySwitch,
                unsigned char srrFlags, int ackRequested,
                BpAncillaryData *ancillaryData, SdrObject adu,
                SdrObject *newBundle)
```

Parameters

* `sap`: the source endpoint for the bundle, provided by the `bp_open` call.
* `*destEid`: identifies the destination endpoint for the bundle.
* `reportToEid`: identifies the endpoint to which any status reports pertaining to this bundle will be sent; if NULL, defaults to the source endpoint.
* `lifespan`: is the maximum number of seconds that the bundle can remain in-transit (undelivered) in the network prior to automatic deletion.
* `classOfService`: is simply priority for now: BP_BULK_PRIORITY, BP_STD_PRIORITY, or BP_EXPEDITED_PRIORITY. If class-of-service flags are defined in a future version of Bundle Protocol, those flags would be OR'd with priority.
* `custodySwitch`: indicates whether or not custody transfer is requested for this bundle and, if so, whether or not the source node itself is required to be the initial custodian. The valid values are SourceCustodyRequired, SourceCustodyOptional, NoCustodyRequired.

  **Important - BPv7 Custody Transfer Changes:** In Bundle Protocol version 7, custody transfer behavior has changed significantly from BPv6. The custodySwitch parameter now indicates a request to use BIBE (Bundle-in-Bundle Encapsulation) for reliable convergence-layer transmission of the bundle. Traditional end-to-end custody transfer with custody acceptance signals is no longer part of the core BPv7 specification; instead, reliable transmission is handled through BIBE encapsulation when available.

  Note that custody transfer is possible only for bundles that are uniquely identified, so it cannot be requested for bundles for which BP_MINIMUM_LATENCY is requested, since BP_MINIMUM_LATENCY may result in the production of multiple identical copies of the same bundle. Similarly, custody transfer should never be requested for a "loopback" bundle, i.e., one whose destination node is the same as the source node: the received bundle will be identical to the source bundle, both residing in the same node, so no custody acceptance signal can be applied to the source bundle and the source bundle will remain in storage until its TTL expires.

* `srrFlags`: if non-zero, is the logical OR of the status reporting behaviors requested for this bundle: BP_RECEIVED_RPT, BP_CUSTODY_RPT, BP_FORWARDED_RPT, BP_DELIVERED_RPT, BP_DELETED_RPT. **Note:** BP_CUSTODY_RPT no longer has any effect in BPv7, as custody transfer semantics have changed.
* `ackRequested`: is a Boolean parameter indicating whether or not the recipient application should be notified that the source application requests some sort of application-specific end-to-end acknowledgment upon receipt of the bundle.
* `ancillaryData`: if not NULL, is used to populate the Quality of Service (QoS) extension block for this bundle. **Note:** The QoS block (type 254) is an **ION-specific, non-standard extension** patterned after the IETF draft-burleigh-dtn-ecos-00 Extended Class of Service (ECOS) specification, which was removed from BPv7. ION's wire format encodes classOfService and ordinal as separate CBOR integers in a 4-element array, whereas the IETF draft specified a 5-element array with priority and ordinal bit-packed together. This block will not interoperate with implementations following the IETF ECOS draft.

The block's ordinal value is used to provide fine-grained ordering within "expedited" traffic: ordinal values from 0 (the default) to 254 (used to designate the most urgent traffic) are valid, with 255 reserved for custody signals. The value of the block's flags is the logical OR of the applicable extended class-of-service flags:

```
BP_MINIMUM_LATENCY designates the bundle as "critical" for the
purposes of Contact Graph Routing.

BP_BEST_EFFORT signifies that non-reliable convergence-layer protocols, as
available, may be used to transmit the bundle.  Notably, the bundle may be
sent as "green" data rather than "red" data when issued via LTP.

BP_DATA_LABEL_PRESENT signifies whether or not the value of _dataLabel_
in _ancillaryData_ must be encoded into the ECOS block when the bundle is
transmitted.
```

__NOTE:__ For Bundle Protocol v7, no Extended Class of Service, or equivalent, has been standardized yet. This capability, however, has been retained from BPv6 and is available to BPv7 implementation in ION.

* `adu`: is the "application data unit" that will be conveyed as the payload of the new bundle. adu must be a "zero-copy object" (ZCO). To ensure orderly access to transmission buffer space for all applications, adu must be created by invoking ionCreateZco(), which may be configured either to block so long as insufficient ZCO storage space is available for creation of the requested ZCO or to fail immediately if insufficient ZCO storage space is available.

Return Value

* 1: success
* 0: user error
* -1: any system error

Example Call

```c
if (bp_send(sap, destEid, reportToEid, ttl, priority,
	custodySwitch, srrFlags, 0, &ancillaryData,
	traceZco, &newBundle) <= 0)
{
        putErrmsg("bptrace can't send file in bundle.",
                        fileName);

        /* user error handling code goes here */
}
```

Description

Sends a bundle to the endpoint identified by destEid, from the source endpoint as provided to the bp_open() call that returned sap.

When sap is NULL, the transmitted bundle is anonymous, i.e., the source of the bundle is not identified. This is legal, but anonymous bundles cannot be uniquely identified; custody transfer and status reporting therefore cannot be requested for an anonymous bundle.

The function returns 1 on success, 0 on user error, -1 on any system error.

If 0 is returned, then an invalid argument value was passed to bp_send(); a message to this effect will have been written to the log file.

If 1 is returned, then either the destination of the bundle was "dtn:none" (the bit bucket) or the ADU has been accepted and queued for transmission in a bundle. In the latter case, if and only if sap was a reference to a BpSAP returned by an invocation of bp_open_source() that had a non-zero value in the detain parameter, then newBundle must be non-NULL and the address of the newly created bundle within the ION database is placed in newBundle. This address can be used to track, suspend/resume, or cancel transmission of the bundle.

---

### bp_track

Function Prototype

```c
int bp_track(SdrObject bundle, SdrObject trackingElt)
```

Parameters

* `bundle`: the bundle object data structure
* `trackingElt`: an sdrlist element managed by the user's application

Return Value

* 0: success
* -1: any error

Example Call

```c
/* a lyst of bundles in SDR */
SdrObject bundleList;

/* a bundle object in SDR */
SdrObject bundleObject;

bundleElt = sdr_list_insert_last(sdr, bundleList,
                bundleObject);
if (bp_track(outAdu.bundleObj, bundleElt) < 0)
{
        sdr_cancel_xn(sdr);
        putErrmsg("Can't track bundle.", NULL);

        /* user error handling code goes here */
}
```

The bundleList is managed via the [sdr_list](./ICI-API.md#sdr-list-management-apis) library of APIs.

Description

Adds `trackingElt` to the list of "tracking" references in bundle. `trackingElt` must be the address of an SDR list element -- whose data is the address of this same bundle -- within some list of bundles that is privately managed by the application. Upon destruction of the bundle this list element will automatically be deleted, thus removing the bundle from the application's privately managed list of bundles. This enables the application to keep track of bundles that it is operating on without risk of inadvertently de-referencing the address of a nonexistent bundle.

---

### bp_untrack

Function Prototype

```c
void bp_untrack(SdrObject bundle, SdrObject trackingElt)
```

Parameters

* `bundle`: the bundle object data structure
* `trackingElt`: an sdrlist element managed by the user's application

Return Value

* 0: success
* -1: Any error

Description

Removes `trackingElt` from the list of "tracking" references in bundle, if it is in that list. Does not delete `trackingElt` itself.

---

### bp_suspend

Function Prototype

```c
int bp_suspend(SdrObject bundle)
```

Parameters

* `bundle`: a bundle object in the SDR

Return Value

* 0: success
* -1: Any error

Description

Suspends transmission of bundle. Has no effect if bundle is "critical" (i.e., has got extended class of service BP_MINIMUM_LATENCY flag set) or if the bundle is already suspended. Otherwise, reverses the enqueuing of the bundle to its selected transmission outduct and places it in the "limbo" queue until the suspension is lifted by calling bp_resume. Returns 0 on success, -1 on any error.

---

### bp_resume

Function Prototype

```c
int bp_resume(SdrObject bundle)
```

Parameters

* `bundle`: a bundle object in the SDR

Return Value

* 0: success
* -1: Any error

Description

Terminates suspension of transmission of bundle. Has no effect if bundle is "critical" (i.e., has got extended class of service BP_MINIMUM_LATENCY flag set) or is not suspended. Otherwise, removes the bundle from the "limbo" queue and queues it for route re-computation and re-queuing. Returns 0 on success, -1 on any error.

---

### bp_cancel

Function Prototype

```c
int bp_cancel(SdrObject bundle)
```

Parameters

* `bundle`: a bundle object in the SDR

Return Value

* 0: success
* -1: Any error

Description

Cancels transmission of bundle. If the indicated bundle is currently queued for forwarding, transmission, or retransmission, it is removed from the relevant queue and destroyed exactly as if its Time To Live had expired. Returns 0 on success, -1 on any error.

---

### bp_release

Function Prototype

```c
int bp_release(SdrObject bundle)
```

Parameters

* `bundle`: a bundle object in the SDR

Return Value

* 0: success
* -1: Any error

Description

Releases a detained bundle for destruction when all retention constraints have been removed. After a detained bundle has been released, the application can no longer track, suspend/resume, or cancel its transmission. Returns 0 on success, -1 on any error.

**NOTE**: for bundles sent through an bundle protocol end-point which is opened via `bp_open_source` with `detain` set to non-zero value, they will not be destroyed, even after successful transmissions, until time-to-live has expired or explicitly released via `bp_release`.

---

### bp_receive

Function Prototype

```c
int bp_receive(BpSAP sap, BpDelivery *dlvBuffer, int timeoutSeconds)
```

Parameters

* `sap`: the source endpoint for the bundle, provided by the `bp_open` call
* `*dlvBuffer`: a pointer to a `BpDelivery` structure used to return the received bundle and/or outcome of reception
* `timoutSeconds`: a reception timer in seconds

Return Value

* 0: success
* -1: any error

Example Call

```c
if (bp_receive(state.sap, &dlv, BP_BLOCKING) < 0)
{
        putErrmsg("bpsink bundle reception failed.", NULL);

        /* user code to handle error or timeout*/
}
```

In this example, BP_BLOCKING is set to -1, that means that the call will block *forever* until a bundle is received, unless interrupted `bp_interrupt`.

Description

Receives a bundle, or reports on some failure of bundle reception activity.

The "result" field of the dlvBuffer structure will be used to indicate the outcome of the data reception activity.

If at least one bundle destined for the endpoint for which this SAP is opened has not yet been delivered to the SAP, then the payload of the oldest such bundle will be returned in `dlvBuffer->adu` and `dlvBuffer->result` will be set to BpPayloadPresent. If there is no such bundle, bp_receive() blocks for up to timeoutSeconds while waiting for one to arrive.

If timeoutSeconds is BP_POLL (i.e., zero) and no bundle is awaiting delivery, or if timeoutSeconds is greater than zero but no bundle arrives before timeoutSeconds have elapsed, then `dlvBuffer->result` will be set to BpReceptionTimedOut. If timeoutSeconds is BP_BLOCKING (i.e., -1) then bp_receive() blocks until either a bundle arrives or the function is interrupted by an invocation of `bp_interrupt`().

`dlvBuffer->result` will be set to BpReceptionInterrupted in the event that the calling process received and handled some signal other than SIGALRM while waiting for a bundle.

`dlvBuffer->result` will be set to BpEndpointStopped in the event that the operation of the indicated endpoint has been terminated.

The application data unit delivered in the data delivery structure, if any, will be a "zero-copy object" reference. Use zco reception functions (see zco(3)) to read the content of the application data unit.

**BpDelivery Structure Fields**

When bp_receive() successfully receives a bundle, the BpDelivery structure provides detailed information about the received bundle:

* `result`: Indicates the outcome of the reception operation. Possible values:
  - `BpPayloadPresent`: A bundle was successfully received
  - `BpReceptionTimedOut`: No bundle arrived within the timeout period
  - `BpReceptionInterrupted`: Reception was interrupted by a signal
  - `BpEndpointStopped`: The endpoint has been terminated

* `bundleSourceEid`: A null-terminated string containing the source endpoint ID of the received bundle. This allows you to identify where the bundle originated.

* `bundleCreationTime`: A BpTimestamp structure indicating when the bundle was created at the source. The timestamp contains:
  - `msec`: Milliseconds since epoch 2000-01-01 00:00:00 UTC
  - `count`: Sequence counter for bundles created in the same millisecond

* `timeToLive`: The original time-to-live value (in seconds) specified when the bundle was sent. Note this is not the remaining TTL, but the original value.

* `ackRequested`: Boolean value (0 or 1) indicating whether the sender requested application-level acknowledgment. If non-zero, the receiving application should send some form of acknowledgment back to the source.

* `adminRecord`: Boolean value indicating whether this bundle contains administrative data (1) or application data (0). Administrative bundles are used internally by BP for status reports and other control messages.

* `adu`: The application data unit as a zero-copy object (ZCO) reference. Use ZCO functions like `zco_source_data_length()` and `zco_receive_source()` to access the payload data.

* `metadataType`: The type code for the RFC 6258 metadata block, if present.

* `metadataLen`: Length of the metadata content (0-255 bytes).

* `metadata`: Array containing the actual metadata content from the RFC 6258 metadata extension block, if present.

Be sure to call `bp_release_delivery`() after every successful invocation of `bp_receive`().

The function returns 0 on success, -1 on any error.

---

### bp_interrupt

Function Prototype

```c
void bp_interrupt(BpSAP sap)
```

Parameters

* `sap`: the BpSAP returned by bp_open() or bp_open_source()

Return Value

* None (void function)

Description

Interrupts a `bp_receive()` invocation that is currently blocked. This function is designed to be called from a signal handler; for this purpose, sap may need to be obtained from a static variable. After interruption, bp_receive() will return with `dlvBuffer->result` set to BpReceptionInterrupted.

---

### bp_release_delivery

Function Prototype

```c
void bp_release_delivery(BpDelivery *dlvBuffer, int releaseAdu)
```

Parameters

* `*dlvBuffer`: a pointer to a `BpDelivery` structure used to return the received bundle and/or outcome of reception
* `releaseAdu`: a Boolean parameter: if non-zero, the ADU ZCO reference in dlvBuffer (if any) is destroyed.

Return Value

* none

Description

Releases resources allocated to the indicated delivery by `dlvBuffer`, which is returned by bp_receive. `releaseAdu` is a Boolean parameter: if non-zero, the ADU ZCO reference in `dlvBuffer` (if any) is destroyed, causing the ZCO itself to be destroyed if no other references to it remain.

---

### bp_close

Function Prototype

```c
void bp_close(BpSAP sap)
```

Parameters

* `sap`: the source endpoint for the bundle, provided by the `bp_open` or `bp_open_source` call

Return Value

* none

Description

Terminates the application's access to the BP endpoint identified by the eid cited by the indicated service access point. The application relinquishes its ability to take delivery of bundles destined for the indicated endpoint and to send bundles whose source is the indicated endpoint.

---

## Thread Safety Considerations

When developing multi-threaded applications that use the BP service API, understanding the thread safety characteristics of bp_send and bp_receive is important for building reliable software.

### Thread Safety Overview

Both bp_send and bp_receive are thread-safe APIs. You can safely call these functions from multiple threads within your application. However, there are important implementation details and best practices you should be aware of to ensure correct operation in multi-threaded environments.

### How Thread Safety is Achieved

ION achieves thread safety for bp_send and bp_receive through several internal mechanisms:

**SDR Transaction System**

Both functions rely on ION's SDR (Simple Data Recorder) transaction system, which provides synchronization through a series of transaction control functions:

* `sdr_begin_xn()` - Begins a transaction and acquires the necessary locks
* `sdr_end_xn()` - Commits the transaction and releases locks
* `sdr_exit_xn()` - Exits a transaction without committing changes
* `sdr_cancel_xn()` - Cancels a transaction and rolls back any changes

These transaction primitives ensure that SDR operations are properly synchronized across multiple threads accessing the shared data store.

**Semaphore-Based Synchronization**

The bp_receive function uses semaphores (sm_SemTake and sm_SemGive) to coordinate access to endpoint delivery queues. The endpoint semaphore ensures that multiple threads waiting on the same endpoint are properly synchronized and that bundle deliveries are correctly serialized.

**Process and Thread Ownership Verification**

Both functions verify that the calling thread or process owns the endpoint before proceeding with operations. For example, bp_receive checks ownership with:

```c
if (vpoint->appPid != sm_TaskIdSelf())
```

This verification helps prevent unauthorized or incorrect access to endpoints from different processes or threads.

### Important Considerations for Multi-Threaded Applications

While the bp_send and bp_receive APIs themselves handle internal synchronization, applications using multiple threads must follow certain guidelines when accessing SDR data, particularly payload data.

**Application-Level Mutex Required for SDR Access**

When your application uses multiple threads that issue their own SDR transactions — for example, a sender thread that builds a payload ZCO and a receiver thread that reads the delivered payload ZCO — you must serialize those application-issued SDR transactions with your own mutex. The ION APIs handle their internal synchronization, but the SDR transactions your application opens around payload data need additional protection so that a sender thread and a receiver thread don't try to hold the SDR transaction lock at the same time.

**Do not hold this mutex across `bp_receive`.** `bp_receive` with `BP_BLOCKING` can block indefinitely waiting for a bundle, and it manages its own SDR transactions and endpoint semaphore internally. Locking around it would prevent any other thread from issuing SDR operations (including `bp_send`) until a bundle arrives, defeating the concurrency you wanted in the first place. Lock only around the application-issued SDR transaction that reads or manipulates the delivered payload, and release the lock before calling `bp_release_delivery`.

Here's the pattern used by `bpchat` (see `bpv7/test/bpchat.c`) and `bping` (see `bpv7/test/bping.c`):

```c
pthread_mutex_t sdrmutex;   // shared with the sender thread

// Receiver thread
while (running)
{
    BpDelivery dlv;

    if (bp_receive(sap, &dlv, BP_BLOCKING) < 0)
    {
        // handle error
        break;
    }

    if (dlv.result == BpReceptionInterrupted || dlv.adu == 0)
    {
        bp_release_delivery(&dlv, 1);
        continue;
    }
    if (dlv.result == BpEndpointStopped)
    {
        bp_release_delivery(&dlv, 1);
        break;
    }

    // Application-issued SDR transaction: serialize with sender thread.
    if (pthread_mutex_lock(&sdrmutex) != 0)
    {
        // handle error
        break;
    }
    oK(sdr_begin_xn(sdr));

    // Read the payload ZCO (dlv.adu) here.
    int len = zco_source_data_length(sdr, dlv.adu);
    // ... zco_start_receiving / zco_receive_source ...

    if (sdr_end_xn(sdr) < 0)
    {
        // handle error
    }
    pthread_mutex_unlock(&sdrmutex);

    bp_release_delivery(&dlv, 1);
}
```

Note that `bp_receive` itself is called with no application mutex held, and `bp_release_delivery` is called after the mutex is dropped — `bp_release_delivery` performs its own SDR transaction internally and does not need (and must not be wrapped in) the application's transaction.

**Key Points to Remember**

* The bp_send and bp_receive APIs are thread-safe for concurrent calls from multiple threads; they manage their own SDR transactions internally.
* Do not hold an application mutex across `bp_receive` — a blocking receive would stall every other thread that also needs the SDR.
* Use your application mutex to serialize application-issued SDR transactions (e.g. reading the delivered payload ZCO in the receiver thread vs. building a payload ZCO in the sender thread).
* Always pair the lock with `sdr_begin_xn` / `sdr_end_xn` (or `sdr_cancel_xn` on error), and release the mutex before calling `bp_release_delivery`.
* Always call `bp_release_delivery` after a successful `bp_receive`, even in multi-threaded scenarios.
* Ensure proper mutex ordering to avoid deadlocks if your application uses multiple locks.

By following these guidelines, you can safely build multi-threaded applications that leverage ION's Bundle Protocol services while maintaining data integrity and avoiding race conditions.

---

## Error Handling

Proper error handling is essential for building robust BP applications. ION provides a comprehensive error reporting mechanism that allows you to track and debug issues effectively.

### Error Reporting Functions

ION uses a set of error logging functions from the ICI layer:

* **putErrmsg(char *msg, char *arg)** - Logs an error message with an optional argument. This is the primary function for recording errors in your application.

* **getErrmsg(char *buffer)** - Retrieves the most recent error message into the provided buffer.

* **writeErrmsgMemos()** - Flushes all accumulated error messages to the ION log file. This is useful for ensuring errors are persisted before the application exits.

### Error Handling Patterns

**Pattern 1: Basic Error Checking**

```c
if (bp_attach() < 0)
{
    putErrmsg("Failed to attach to BP.", NULL);
    return -1;
}
```

**Pattern 2: Error Checking with Context**

```c
if (bp_open(ownEid, &sap) < 0)
{
    putErrmsg("Cannot open endpoint.", ownEid);
    writeErrmsgMemos();
    return -1;
}
```

**Pattern 3: Checking bp_send() Return Values**

The bp_send() function has three possible return values that must be handled differently:

```c
int result = bp_send(sap, destEid, NULL, 300, BP_STD_PRIORITY,
                     NoCustodyRequested, 0, 0, NULL, zco, &bundleObj);

if (result <= 0)
{
    if (result == 0)
    {
        putErrmsg("Invalid argument to bp_send.", NULL);
        // User error - check your parameters
    }
    else  // result == -1
    {
        putErrmsg("bp_send system error.", NULL);
        // System error - may need to restart or check BP agent status
    }
    return -1;
}
// result == 1: success
```

**Pattern 4: Handling bp_receive() Results**

```c
BpDelivery dlv;

if (bp_receive(sap, &dlv, BP_BLOCKING) < 0)
{
    putErrmsg("bp_receive failed.", NULL);
    return -1;
}

switch (dlv.result)
{
case BpPayloadPresent:
    // Process the bundle
    process_bundle(&dlv);
    break;

case BpReceptionTimedOut:
    // Timeout is not an error, just no data available
    break;

case BpReceptionInterrupted:
    putErrmsg("Bundle reception interrupted.", NULL);
    running = 0;  // Typically means shutdown requested
    break;

case BpEndpointStopped:
    putErrmsg("Endpoint stopped.", NULL);
    running = 0;
    break;

default:
    putErrmsg("Unexpected delivery result.", NULL);
    break;
}

bp_release_delivery(&dlv, 1);  // Always call this
```

**Pattern 5: SDR Transaction Error Handling**

When working with SDR transactions for ZCO creation:

```c
Sdr sdr = bp_get_sdr();
SdrObject extent;

CHKERR(sdr_begin_xn(sdr));  // Macro that checks and handles errors

extent = sdr_malloc(sdr, dataLength);
if (extent == 0)
{
    putErrmsg("No SDR space for extent.", NULL);
    sdr_cancel_xn(sdr);  // Cancel transaction on error
    return -1;
}

sdr_write(sdr, extent, data, dataLength);

if (sdr_end_xn(sdr) < 0)
{
    putErrmsg("SDR transaction failed.", NULL);
    return -1;
}
```

### Common Error Messages and Their Meanings

* **"No space for ZCO"** - SDR heap is full. See the SDR Heap Management warning in the API Reference section.

* **"Endpoint is already open"** - Another process has already opened this endpoint. Only one process can open an endpoint at a time.

* **"Can't get SDR"** - BP agent may not be running, or bp_attach() was not called.

* **"Bundle is too big"** - The bundle exceeds the maximum bundle size configured in the BP agent.

* **"Invalid argument"** - One of the parameters passed to the function is invalid (NULL pointer, invalid endpoint ID format, etc.).

* **"BP agent not started"** - The BP service daemon is not running. Use `bp_agent_is_started()` to check before calling BP functions.

### Best Practices

1. **Always check return values** - Never ignore the return value of BP API functions, even void functions that might indicate errors through other mechanisms.

2. **Use putErrmsg consistently** - Log errors immediately when they occur to build a clear audit trail.

3. **Provide context in error messages** - Include relevant identifiers (EIDs, bundle IDs, etc.) in error messages to aid debugging.

4. **Clean up on errors** - If an error occurs partway through an operation, ensure you clean up any allocated resources (SAPs, ZCOs, transactions, etc.).

5. **Check BP agent status on startup** - Use `bp_agent_is_started()` before attempting BP operations to provide clear error messages.

6. **Flush error logs before exit** - Call `writeErrmsgMemos()` before your application exits to ensure all errors are recorded.

---

## Walk Through of `bpsource.c`

* For this example, it is assumed that the user is already familiar with the [ICI APIs](./ICI-API.md).

**TO BE UPDATED.**

## Compiling and Linking

To compile and link applications that use the BP API, you need to include the appropriate header files and link against the ION libraries.

### Required Headers

```c
#include "bp.h"      // Bundle Protocol API
#include "ion.h"     // ION infrastructure (for ionCreateZco, ionAttach, etc.)
```

### Required Libraries

Your application must link against the following ION libraries (in order):

* `-lbp` - Bundle Protocol library
* `-lici` - ION Common Infrastructure library
* `-lpthread` - POSIX threads library (required by ION)

### Compilation Example

**Basic compilation:**

```bash
gcc -o myapp myapp.c -I/usr/local/include -L/usr/local/lib -lbp -lici -lpthread
```

**With debugging symbols:**

```bash
gcc -g -o myapp myapp.c -I/usr/local/include -L/usr/local/lib -lbp -lici -lpthread
```

**With optimization:**

```bash
gcc -O2 -o myapp myapp.c -I/usr/local/include -L/usr/local/lib -lbp -lici -lpthread
```

### Using pkg-config (Recommended)

If ION was installed with pkg-config support, you can use it to automatically get the correct compiler and linker flags:

```bash
gcc -o myapp myapp.c `pkg-config --cflags --libs ion`
```

> **WARNING**: Using pkg-config is strongly recommended to ensure ABI compatibility. ION's public structures contain members whose sizes depend on platform-specific preprocessor definitions. Compiling without the correct flags (e.g., `-Dlinux`) will cause struct size mismatches, leading to buffer overruns and segmentation faults.

For more information about using pkg-config with ION, see the [Writing Applications for ION](./Writing-Applications-for-ION.md) guide.

### Makefile Example

```makefile
CC = gcc
CFLAGS = -g -Wall -I/usr/local/include
LDFLAGS = -L/usr/local/lib
LIBS = -lbp -lici -lpthread

myapp: myapp.o
	$(CC) $(LDFLAGS) -o $@ $< $(LIBS)

myapp.o: myapp.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f myapp myapp.o
```

### Platform-Specific Considerations

**Linux:**
- Standard installation typically uses `/usr/local/include` and `/usr/local/lib`
- May need to add `/usr/local/lib` to `LD_LIBRARY_PATH` if libraries are not found at runtime

**macOS:**
- Same paths as Linux typically work
- May need to use `DYLD_LIBRARY_PATH` instead of `LD_LIBRARY_PATH`

**Runtime Library Path:**

If you encounter "library not found" errors at runtime, you may need to set the library path:

```bash
export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH
```

Or compile with rpath:

```bash
gcc -o myapp myapp.c -I/usr/local/include -L/usr/local/lib \
    -Wl,-rpath,/usr/local/lib -lbp -lici -lpthread
```

## Zero-copy Object (ZCO) Creation and Management

Application data is handed to the Bundle Protocol via zero-copy objects (ZCOs). Understanding how to properly create and manage ZCOs is essential for using the BP API effectively.

### ZCO Types

There are two main types of ZCO relevant to application developers:

1. **SDR-based ZCO** - Data is stored in the ION SDR heap
2. **File-based ZCO** - Data is read from a file in the host filesystem

A third type, ZCO-of-ZCOs, is used internally by ION for bundle concatenation but is not typically used directly by applications.

### Creating SDR-based ZCOs

SDR-based ZCOs store the payload data in ION's SDR heap. This is appropriate for small to medium-sized payloads (typically under 10-100 KB, depending on your SDR configuration).

**Complete Example:**

```c
#include "bp.h"

int send_sdr_bundle(char *destEid, char *data, int dataLen)
{
    Sdr sdr;
    SdrObject extent, bundleZco;
    SdrObject bundleObj = 0;
    ReqAttendant attendant;
    int result;

    // Attach to BP
    if (bp_attach() < 0)
    {
        putErrmsg("Can't attach to BP.", NULL);
        return -1;
    }

    // Get SDR handle
    sdr = bp_get_sdr();

    // Initialize attendant for ZCO creation
    // This controls blocking behavior if SDR space is limited
    ionStartAttendant(&attendant);

    // Begin SDR transaction to allocate space for payload
    CHKERR(sdr_begin_xn(sdr));

    // Allocate SDR space for the data
    extent = sdr_malloc(sdr, dataLen);
    if (extent == 0)
    {
        putErrmsg("No SDR space for payload.", NULL);
        sdr_cancel_xn(sdr);
        ionStopAttendant(&attendant);
        bp_detach();
        return -1;
    }

    // Write data to SDR
    sdr_write(sdr, extent, data, dataLen);

    // End transaction
    if (sdr_end_xn(sdr) < 0)
    {
        putErrmsg("SDR transaction failed.", NULL);
        ionStopAttendant(&attendant);
        bp_detach();
        return -1;
    }

    // Create ZCO from SDR extent
    bundleZco = ionCreateZco(ZcoSdrSource, extent, 0, dataLen,
                             BP_STD_PRIORITY, 0, ZcoOutbound, &attendant);

    if (bundleZco == 0 || bundleZco == (SdrObject) ERROR)
    {
        putErrmsg("Can't create ZCO.", NULL);
        ionStopAttendant(&attendant);
        bp_detach();
        return -1;
    }

    // Send bundle (anonymous - no detention)
    result = bp_send(NULL, destEid, NULL, 300, BP_STD_PRIORITY,
                     NoCustodyRequested, 0, 0, NULL, bundleZco, &bundleObj);

    ionStopAttendant(&attendant);

    if (result <= 0)
    {
        putErrmsg("bp_send failed.", NULL);
        bp_detach();
        return -1;
    }

    bp_detach();
    return 0;
}
```

**Key Points for SDR ZCOs:**

* You must manage SDR transactions when creating the payload extent
* Use `ionStartAttendant()` and `ionStopAttendant()` to manage the attendant
* The ZCO is automatically consumed by `bp_send()` - do not reuse it
* SDR space is freed when the bundle is transmitted (or destroyed)

### Creating File-based ZCOs

File-based ZCOs read data directly from a file in the filesystem, avoiding SDR heap consumption. This is ideal for large payloads.

**Complete Example:**

```c
#include "bp.h"
#include <sys/stat.h>

int send_file_bundle(char *destEid, char *filename)
{
    struct stat statbuf;
    int fileRef;
    SdrObject bundleZco;
    SdrObject bundleObj = 0;
    ReqAttendant attendant;
    int result;

    // Check file exists and get size
    if (stat(filename, &statbuf) < 0)
    {
        putErrmsg("Can't stat file.", filename);
        return -1;
    }

    if (bp_attach() < 0)
    {
        putErrmsg("Can't attach to BP.", NULL);
        return -1;
    }

    // Open file (zco_create_file_ref opens and tracks the file)
    fileRef = zco_create_file_ref(bp_get_sdr(), filename, "", ZcoOutbound);
    if (fileRef < 0)
    {
        putErrmsg("Can't create file ref.", filename);
        bp_detach();
        return -1;
    }

    ionStartAttendant(&attendant);

    // Create ZCO from file
    bundleZco = ionCreateZco(ZcoFileSource, fileRef, 0, statbuf.st_size,
                             BP_STD_PRIORITY, 0, ZcoOutbound, &attendant);

    if (bundleZco == 0 || bundleZco == (SdrObject) ERROR)
    {
        putErrmsg("Can't create file ZCO.", NULL);
        ionStopAttendant(&attendant);
        bp_detach();
        return -1;
    }

    // Send bundle
    result = bp_send(NULL, destEid, NULL, 300, BP_STD_PRIORITY,
                     NoCustodyRequested, 0, 0, NULL, bundleZco, &bundleObj);

    ionStopAttendant(&attendant);

    if (result <= 0)
    {
        putErrmsg("bp_send failed.", NULL);
        bp_detach();
        return -1;
    }

    bp_detach();
    return 0;
}
```

**Important Notes for File ZCOs:**

* The file must remain accessible until the bundle is fully transmitted
* ION does not make a copy - it reads directly from the file
* The file should not be modified while the bundle is in transit
* Use `zco_create_file_ref()` to properly register the file with ION
* Good for large payloads to avoid SDR heap exhaustion

### Receiving and Processing ZCO Payloads

When you receive a bundle, the payload is delivered as a ZCO. You must use ZCO functions to extract the data.

**Complete Receive Example:**

```c
#include "bp.h"

void receive_and_process(BpSAP sap)
{
    BpDelivery dlv;
    Sdr sdr;
    ZcoReader reader;
    vast bytesRemaining;
    char buffer[1024];
    int bytesToRead;

    sdr = bp_get_sdr();

    while (running)
    {
        if (bp_receive(sap, &dlv, BP_BLOCKING) < 0)
            break;

        if (dlv.result == BpPayloadPresent)
        {
            // Get ZCO length
            CHKERR(sdr_begin_xn(sdr));
            zco_source_data_length(sdr, dlv.adu, &bytesRemaining);
            sdr_exit_xn(sdr);

            // Initialize ZCO reader
            zco_start_receiving(dlv.adu, &reader);

            // Read data in chunks
            while (bytesRemaining > 0)
            {
                bytesToRead = (bytesRemaining < sizeof(buffer))
                              ? bytesRemaining : sizeof(buffer);

                CHKERR(sdr_begin_xn(sdr));
                zco_receive_source(sdr, &reader, bytesToRead, buffer);
                sdr_exit_xn(sdr);

                // Process the data chunk
                process_data(buffer, bytesToRead);

                bytesRemaining -= bytesToRead;
            }

            zco_stop_receiving(&reader);
        }
        else if (dlv.result == BpEndpointStopped)
        {
            running = 0;
        }

        // Always release delivery
        bp_release_delivery(&dlv, 1);
    }
}
```

**Key ZCO Reception Functions:**

* `zco_source_data_length(sdr, zco, &length)` - Get total payload size
* `zco_start_receiving(zco, &reader)` - Initialize reader for the ZCO
* `zco_receive_source(sdr, &reader, length, buffer)` - Read data from ZCO
* `zco_stop_receiving(&reader)` - Clean up reader when done

### ZCO Best Practices

1. **Choose the right ZCO type**
   - SDR ZCO: Small payloads (< 10-100 KB)
   - File ZCO: Large payloads to avoid SDR exhaustion

2. **Always use SDR transactions** when creating SDR extents or reading ZCO data

3. **Don't reuse ZCOs** - After passing a ZCO to bp_send(), it's consumed and cannot be reused

4. **Manage the attendant properly** - Call ionStartAttendant() before creating ZCOs and ionStopAttendant() when done

5. **Release deliveries** - Always call bp_release_delivery() after bp_receive(), even if no payload was received

6. **Read ZCOs in chunks** - For large payloads, read the ZCO data in manageable chunks rather than allocating huge buffers

For more complete examples, see:
* `bpsource.c` - Demonstrates SDR-based ZCO creation and sending
* `bpsink.c` - Demonstrates receiving and processing ZCO payloads
* `bpdriver.c` - Demonstrates file-based ZCO creation

## Common Pitfalls and How to Avoid Them

This section documents common mistakes developers make when using the BP API and how to avoid them.

### 1. Forgetting to Call bp_release_delivery()

**Symptom:** Memory leaks, resource exhaustion, eventually "No space for ZCO" errors

**Problem:**
```c
// BAD - resource leak
while (running)
{
    if (bp_receive(sap, &dlv, BP_BLOCKING) < 0)
        break;

    if (dlv.result == BpPayloadPresent)
    {
        process_bundle(&dlv);
    }
    // Missing bp_release_delivery() - resources leaked!
}
```

**Solution:**
```c
// GOOD - always release
while (running)
{
    if (bp_receive(sap, &dlv, BP_BLOCKING) < 0)
        break;

    if (dlv.result == BpPayloadPresent)
    {
        process_bundle(&dlv);
    }

    bp_release_delivery(&dlv, 1);  // Always call this!
}
```

**Key Point:** Call `bp_release_delivery()` after EVERY `bp_receive()`, even if `dlv.result` is not `BpPayloadPresent`.

### 2. Not Releasing Detained Bundles

**Symptom:** SDR heap fills up, "No space for ZCO" errors, bundles not being destroyed even after transmission

**Problem:**
```c
// BAD - bundles never released
bp_open_source(ownEid, &sap, 1);  // detain=1

for (int i = 0; i < 1000; i++)
{
    SdrObject bundleObj;
    bp_send(sap, destEid, NULL, 300, BP_STD_PRIORITY,
            NoCustodyRequested, 0, 0, NULL, zco, &bundleObj);

    // Bundle tracking code here...

    // Missing bp_release() - bundles pile up in SDR!
}
```

**Solution:**
```c
// GOOD - release when done tracking
bp_open_source(ownEid, &sap, 1);  // detain=1

for (int i = 0; i < 1000; i++)
{
    SdrObject bundleObj;
    bp_send(sap, destEid, NULL, 300, BP_STD_PRIORITY,
            NoCustodyRequested, 0, 0, NULL, zco, &bundleObj);

    // Bundle tracking code here...

    // Release when done tracking
    bp_release(bundleObj);
}
```

**Key Point:** Detained bundles remain in SDR until explicitly released or TTL expires. Always call `bp_release()` when done tracking.

### 3. Incorrect bp_send() Return Value Checking

**Symptom:** User errors (invalid arguments) are not caught, leading to silent failures

**Problem:**
```c
// BAD - only checks for system errors
if (bp_send(sap, destEid, NULL, 300, BP_STD_PRIORITY,
            NoCustodyRequested, 0, 0, NULL, zco, &bundleObj) < 0)
{
    putErrmsg("bp_send failed.", NULL);
    return -1;
}
// This misses the case where bp_send() returns 0 (user error)!
```

**Solution:**
```c
// GOOD - checks for both user and system errors
int result = bp_send(sap, destEid, NULL, 300, BP_STD_PRIORITY,
                     NoCustodyRequested, 0, 0, NULL, zco, &bundleObj);

if (result <= 0)  // Catches both 0 (user error) and -1 (system error)
{
    if (result == 0)
        putErrmsg("bp_send: invalid argument.", NULL);
    else
        putErrmsg("bp_send: system error.", NULL);
    return -1;
}
```

**Key Point:** `bp_send()` returns 1 on success, 0 on user error, -1 on system error. Check for `<= 0`, not just `< 0`.

### 4. Reusing ZCOs After bp_send()

**Symptom:** Crashes, corruption, or "ZCO destroyed" errors

**Problem:**
```c
// BAD - trying to reuse ZCO
SdrObject zco = ionCreateZco(...);

bp_send(sap, "ipn:1.1", NULL, 300, BP_STD_PRIORITY,
        NoCustodyRequested, 0, 0, NULL, zco, NULL);

// ZCO has been consumed by bp_send()!
bp_send(sap, "ipn:2.1", NULL, 300, BP_STD_PRIORITY,
        NoCustodyRequested, 0, 0, NULL, zco, NULL);  // WRONG - ZCO is gone!
```

**Solution:**
```c
// GOOD - create new ZCO for each bundle
SdrObject zco1 = ionCreateZco(...);
bp_send(sap, "ipn:1.1", NULL, 300, BP_STD_PRIORITY,
        NoCustodyRequested, 0, 0, NULL, zco1, NULL);

SdrObject zco2 = ionCreateZco(...);  // Create new ZCO
bp_send(sap, "ipn:2.1", NULL, 300, BP_STD_PRIORITY,
        NoCustodyRequested, 0, 0, NULL, zco2, NULL);
```

**Key Point:** ZCOs are consumed by `bp_send()`. Create a new ZCO for each bundle.

### 5. Missing SDR Transactions for ZCO Data Access

**Symptom:** Data corruption, crashes, inconsistent reads

**Problem:**
```c
// BAD - reading ZCO without transaction
vast length;
zco_source_data_length(sdr, dlv.adu, &length);  // No transaction!
```

**Solution:**
```c
// GOOD - use transaction
vast length;
CHKERR(sdr_begin_xn(sdr));
zco_source_data_length(sdr, dlv.adu, &length);
sdr_exit_xn(sdr);
```

**Key Point:** Always use SDR transactions when reading or writing SDR data, including ZCO operations.

### 6. Not Checking if BP Agent is Running

**Symptom:** Cryptic errors like "Can't get SDR" when BP agent is not started

**Problem:**
```c
// BAD - no check
int main()
{
    bp_attach();
    // ... rest of code
}
```

**Solution:**
```c
// GOOD - check agent status
int main()
{
    if (!bp_agent_is_started())
    {
        fprintf(stderr, "Error: BP agent is not running.\n");
        fprintf(stderr, "Please start ION before running this application.\n");
        return 1;
    }

    if (bp_attach() < 0)
    {
        putErrmsg("Can't attach to BP.", NULL);
        return 1;
    }
    // ... rest of code
}
```

**Key Point:** Check `bp_agent_is_started()` before calling BP functions to provide clear error messages.

### 7. Requesting Status Reports for Anonymous Bundles

**Symptom:** Status reports are never received

**Problem:**
```c
// BAD - requesting reports for anonymous bundle
bp_send(NULL,  // Anonymous - no source endpoint!
        destEid, NULL, 300, BP_STD_PRIORITY,
        NoCustodyRequested,
        BP_RECEIVED_RPT | BP_DELIVERED_RPT,  // Reports requested
        0, NULL, zco, NULL);
// Reports can't be delivered because there's no source endpoint!
```

**Solution:**
```c
// GOOD - use proper source endpoint for reports
BpSAP sap;
bp_open_source("ipn:1.1", &sap, 0);

bp_send(sap,  // Proper source endpoint
        destEid, NULL, 300, BP_STD_PRIORITY,
        NoCustodyRequested,
        BP_RECEIVED_RPT | BP_DELIVERED_RPT,  // Now reports can be delivered
        0, NULL, zco, NULL);
```

**Key Point:** You cannot receive status reports for anonymous bundles (sap=NULL). Use a proper source endpoint.

### 8. Using Large SDR ZCOs Without Monitoring SDR Space

**Symptom:** Sudden "No space for ZCO" errors as SDR fills up

**Problem:**
```c
// BAD - using SDR for large files
char bigData[10 * 1024 * 1024];  // 10 MB
// ... load data ...
SdrObject extent = sdr_malloc(sdr, sizeof(bigData));  // Consumes SDR heap!
```

**Solution:**
```c
// GOOD - use file ZCO for large data
// For data > ~100KB, use file-based ZCO instead
int fileRef = zco_create_file_ref(sdr, "large_file.dat", "", ZcoOutbound);
SdrObject zco = ionCreateZco(ZcoFileSource, fileRef, 0, fileSize,
                          BP_STD_PRIORITY, 0, ZcoOutbound, &attendant);
```

**Key Point:** Use file-based ZCOs for large payloads to avoid exhausting the SDR heap.

### 9. Incorrect Endpoint ID Format

**Symptom:** "Invalid EID" or "Can't open endpoint" errors

**Problem:**
```c
// BAD - malformed EIDs
bp_open("ipn:1:1", &sap);        // Wrong: uses : instead of .
bp_open("ipn:1.1.1", &sap);      // Wrong: too many parts
bp_open("1.1", &sap);            // Wrong: missing scheme
```

**Solution:**
```c
// GOOD - proper EID formats
bp_open("ipn:1.1", &sap);        // Correct IPN format
bp_open("dtn://node1/service", &sap);  // Correct DTN format
```

**Key Point:** IPN EIDs use format `ipn:node.service` (with a dot). Ensure proper scheme and format.

### 10. Forgetting to Stop Attendant

**Symptom:** Resource leaks, potential blocking issues

**Problem:**
```c
// BAD - attendant never stopped
ReqAttendant attendant;
ionStartAttendant(&attendant);
SdrObject zco = ionCreateZco(..., &attendant);
bp_send(..., zco, ...);
// Missing ionStopAttendant()!
```

**Solution:**
```c
// GOOD - always stop attendant
ReqAttendant attendant;
ionStartAttendant(&attendant);
SdrObject zco = ionCreateZco(..., &attendant);
bp_send(..., zco, ...);
ionStopAttendant(&attendant);  // Clean up!
```

**Key Point:** Always call `ionStopAttendant()` after you're done creating ZCOs to properly release resources.

## Python API

ION provides a Python API that is a wrapper around the C API, called [PYION](https://github.com/nasa-jpl/pyion), which is available on GitHub.
