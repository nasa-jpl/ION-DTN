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

Quit ionadmin by command `q`. Quitting `ionadmin` will not terminate any ION and BP services, it simply ends ION's user interface for configuration query and management. If you see warning messages such as those shown below when quitting `ionadmin`, then it confirms that ION is actually not running at the moment. There are no software error:

```bash
: q
at line 427 of ici/library/platform_sm.c, Can't get shared memory segment: Invalid argument (0)
at line 312 of ici/library/memmgr.c, Can't open memory region.
at line 367 of ici/sdr/sdrxn.c, Can't open SDR working memory.
at line 513 of ici/sdr/sdrxn.c, Can't open SDR working memory.
at line 963 of ici/library/ion.c, Can't initialize the SDR system.
Stopping ionadmin.
```

You can also run `bpversion` to determine the version of Bundle Protocol built in the host:

```bash
$ bpversion
bpv7
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

To further vary that BP service is running, you can check for presence of ION shared memory and semaphores:

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
Sending TERM to acsadmin lt-acsadmin    acslist lt-acslist      aoslsi lt-aoslsi        aoslso lt-aoslso        bibeadmin lt-bibeadmin  bibeclo lt-bibeclo     bpadmin lt-bpadmin      bpcancel lt-bpcancel    bpchat lt-bpchat        bpclm lt-bpclm  bpclock lt-bpclock      bpcounter lt-bpcounter         bpdriver lt-bpdriver    bpecho lt-bpecho        bping lt-bping  bplist lt-bplist        bpnmtest lt-bpnmtest    bprecvfile lt-bprecvfile       bpsecadmin lt-bpsecadmin        bpsendfile lt-bpsendfile        bpsink lt-bpsink        bpsource lt-bpsource    bpstats lt-bpstats     bpstats2 lt-bpstats2    bptrace lt-bptrace      bptransit lt-bptransit  brsccla lt-brsccla      brsscla lt-brsscla      bsscounter lt-bsscounter       bssdriver lt-bssdriver  bsspadmin lt-bsspadmin  bsspcli lt-bsspcli      bsspclo lt-bsspclo      bsspclock lt-bsspclock  bssrecv lt-bssrecv     bssStreamingApp lt-bssStreamingApp      cgrfetch lt-cgrfetch    cpsd lt-cpsd    dccpcli lt-dccpcli      dccpclo lt-dccpclo    dccplsi lt-dccplsi       dccplso lt-dccplso      dgr2file lt-dgr2file    dgrcli lt-dgrcli        dgrclo lt-dgrclo        dtka lt-dtka    dtkaadmin lt-dtkaadmin         dtn2admin lt-dtn2admin  dtn2adminep lt-dtn2adminep      dtn2fw lt-dtn2fw        dtpcadmin lt-dtpcadmin  dtpcclock lt-dtpcclock         dtpcd lt-dtpcd  dtpcreceive lt-dtpcreceive      dtpcsend lt-dtpcsend    file2dgr lt-file2dgr    file2sdr lt-file2sdr    file2sm lt-file2sm     file2tcp lt-file2tcp    file2udp lt-file2udp    hmackeys lt-hmackeys    imcadmin lt-imcadmin    imcadminep lt-imcadminep      imcfw lt-imcfw   ionadmin lt-ionadmin    ionexit lt-ionexit      ionrestart lt-ionrestart        ionsecadmin lt-ionsecadmin      ionunlock lt-ionunlock         ionwarn lt-ionwarn      ipnadmin lt-ipnadmin    ipnadminep lt-ipnadminep        ipnd lt-ipnd    ipnfw lt-ipnfw  lgagent lt-lgagent     lgsend lt-lgsend        ltpadmin lt-ltpadmin    ltpcli lt-ltpcli        ltpclo lt-ltpclo        ltpclock lt-ltpclock    ltpcounter lt-ltpcounter       ltpdeliv lt-ltpdeliv    ltpdriver lt-ltpdriver  ltpmeter lt-ltpmeter    ltpsecadmin lt-ltpsecadmin      nm_agent lt-nm_agent  nm_mgr lt-nm_mgr         owltsim lt-owltsim      owlttb lt-owlttb        psmshell lt-psmshell    psmwatch lt-psmwatch    ramsgate lt-ramsgate  rfxclock lt-rfxclock     sdatest lt-sdatest      sdr2file lt-sdr2file    sdrmend lt-sdrmend      sdrwatch lt-sdrwatch    sm2file lt-sm2file    smlistsh lt-smlistsh     smrbtsh lt-smrbtsh      stcpcli lt-stcpcli      stcpclo lt-stcpclo      tcaadmin lt-tcaadmin    tcaboot lt-tcaboot    tcacompile lt-tcacompile tcapublish lt-tcapublish        tcarecv lt-tcarecv      tcc lt-tcc      tccadmin lt-tccadmin    tcp2file lt-tcp2file  tcpbsi lt-tcpbsi         tcpbso lt-tcpbso        tcpcli lt-tcpcli        tcpclo lt-tcpclo        udp2file lt-udp2file    udpbsi lt-udpbsi      udpbso lt-udpbso         udpcli lt-udpcli        udpclo lt-udpclo        udplsi lt-udplsi        udplso lt-udplso                amsbenchr lt-amsbenchr         amsbenchs lt-amsbenchs  amsd lt-amsd    amshello lt-amshello    amslog lt-amslog        amslogprt lt-amslogprt  amsshell lt-amsshell   amsstop lt-amsstop      bputa lt-bputa  cfdpadmin lt-cfdpadmin  cfdpclock lt-cfdpclock  cfdptest lt-cfdptest    bpcp lt-bpcp    bpcpd lt-bpcpd ...
Sending KILL to the processes...
Checking if all processes ended...
Deleting shared memory to remove SDR...
Killm completed.
ION node ended. Log file: ion.log
```

At this point run `ps -aux` or `ipcs` to verify that ION has terminated completely.

### Simple ION Installation Test

When ION is not running, you can performance a simple unit test to verify ION is build properly.

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

In this document, we assume that ION was build and installed via the `./configure` installation process using the full open-source codebase and with the standard set of options.

The location and content of the library and header directories shown above included non-BP modules and may not match exactly with what you have especially if you have built ION with features options enabled via `./configure` or used a manual/custom Makefile, or built ION from the `ion-core` package instead.

## Launch ION & BP Services

Once you are confidant that ION has been properly built and installed in the system, you can start BP service by launching ION. To do this, please consult the various tutorials under _Configuration_.

After launching ION, you can [verify BP service status](#determine-bp-service-state) in the same manner as described in previous section.

## BP Service API Reference

### Header

```c
#include "bp.h"
```

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
void bp_detach( )

```

Parameters

* None.

Return Value

* 0: success
* -1: Any error

Description

Terminates all access to BP functionality for the invoking process.

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
int bp_send(BpSAP sap, char *destEid, char *reportToEid, 
             int lifespan, int classOfService, BpCustodySwitch custodySwitch, 
             unsigned char srrFlags, int ackRequested, 
             BpAncillaryData *ancillaryData, Object adu, Object *newBundle)
```

Parameters

* `sap`: the source endpoint for the bundle, provided by the `bp_open` call.
* `*destEid`: identifies the destination endpoint for the bundle.
* `reportToEid`: identifies the endpoint to which any status reports pertaining to this bundle will be sent; if NULL, defaults to the source endpoint.
* `lifespan`: is the maximum number of seconds that the bundle can remain in-transit (undelivered) in the network prior to automatic deletion.
* `classOfService`: is simply priority for now: BP_BULK_PRIORITY, BP_STD_PRIORITY, or BP_EXPEDITED_PRIORITY. If class-of-service flags are defined in a future version of Bundle Protocol, those flags would be OR'd with priority.
* `custodySwitch`: indicates whether or not custody transfer is requested for this bundle and, if so, whether or not the source node itself is required to be the initial custodian. The valid values are SourceCustodyRequired, SourceCustodyOptional, NoCustodyRequired. Note that custody transfer is possible only for bundles that are uniquely identified, so it cannot be requested for bundles for which BP_MINIMUM_LATENCY is requested, since BP_MINIMUM_LATENCY may result in the production of multiple identical copies of the same bundle. Similarly, custody transfer should never be requested for a "loopback" bundle, i.e., one whose destination node is the same as the source node: the received bundle will be identical to the source bundle, both residing in the same node, so no custody acceptance signal can be applied to the source bundle and the source bundle will remain in storage until its TTL expires.
* `srrFlags`: if non-zero, is the logical OR of the status reporting behaviors requested for this bundle: BP_RECEIVED_RPT, BP_CUSTODY_RPT, BP_FORWARDED_RPT, BP_DELIVERED_RPT, BP_DELETED_RPT.
* `ackRequested`: is a Boolean parameter indicating whether or not the recipient application should be notified that the source application requests some sort of application-specific end-to-end acknowledgment upon receipt of the bundle.
* `ancillaryData`: if not NULL, is used to populate the Extended Class Of Service block for this bundle. The block's ordinal value is used to provide fine-grained ordering within "expedited" traffic: ordinal values from 0 (the default) to 254 (used to designate the most urgent traffic) are valid, with 255 reserved for custody signals. The value of the block's flags is the logical OR of the applicable extended class-of-service flags:

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
int bp_track(Object bundle, Object trackingElt)
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
Object bundleList;

/* a bundle object in SDR */
Object bundleObject;

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
void bp_untrack(Object bundle, Object trackingElt)
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
int bp_suspend(Object bundle)
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
int bp_resume(Object bundle)
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
int bp_cancel(Object bundle)
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
int bp_release(Object bundle)
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

Be sure to call `bp_release_delivery`() after every successful invocation of `bp_receive`().

The function returns 0 on success, -1 on any error.

---

### bp_interrupt

Function Prototype

```c
void bp_interrupt(BpSAP sap)
```

Parameters

* `bundle`: a bundle object in the SDR

Return Value

* 0: success
* -1: Any error

Description

Interrupts a `bp_receive`() invocation that is currently blocked. This function is designed to be called from a signal handler; for this purpose, sap may need to be obtained from a static variable.

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

## Walk Through of `bpsource.c`

* For this example, it is assumed that the user is already familiar with the [ICI APIs](./ICI-API.md).

**TO BE UPDATED.**

## Compiling and Linking

## Zero-copy Object (ZCO) Types

We have shown that the way to hand user data from an application to BP is via a zero-copy object (ZCO).

In general there are two types of ZCO that is relevant to a user.

* Note A third type of ZCO is called a ZCO ZCO - one consists of multiple other ZCOs. This is used internally by ION to handle concatenation of data but it is not relevant in the context of this topic.

### SDR ZCO

* Used in our example
* Example in `bpsource.c`

### File ZCO

* Can be used if you wish not to make a copy of the data in the ION SDR (possibly to save SDR space)
* You allow ION to access the original data (file) in the host computer's file system. Need to check if this restriction applies.
* See example in `bpdriver.c`
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

Quit ionadmin by command `q`. Quitting `ionadmin` will not terminate any ION and BP services, it simply ends ION's user interface for configuration query and management. If you see warning messages such as those shown below when quitting `ionadmin`, then it confirms that ION is actually not running at the moment. There are no software error:

```bash
: q
at line 427 of ici/library/platform_sm.c, Can't get shared memory segment: Invalid argument (0)
at line 312 of ici/library/memmgr.c, Can't open memory region.
at line 367 of ici/sdr/sdrxn.c, Can't open SDR working memory.
at line 513 of ici/sdr/sdrxn.c, Can't open SDR working memory.
at line 963 of ici/library/ion.c, Can't initialize the SDR system.
Stopping ionadmin.
```

You can also run `bpversion` to determine the version of Bundle Protocol built in the host:

```bash
$ bpversion
bpv7
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

To further vary that BP service is running, you can check for presence of ION shared memory and semaphores:

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
Sending TERM to acsadmin lt-acsadmin    acslist lt-acslist      aoslsi lt-aoslsi        aoslso lt-aoslso        bibeadmin lt-bibeadmin  bibeclo lt-bibeclo     bpadmin lt-bpadmin      bpcancel lt-bpcancel    bpchat lt-bpchat        bpclm lt-bpclm  bpclock lt-bpclock      bpcounter lt-bpcounter         bpdriver lt-bpdriver    bpecho lt-bpecho        bping lt-bping  bplist lt-bplist        bpnmtest lt-bpnmtest    bprecvfile lt-bprecvfile       bpsecadmin lt-bpsecadmin        bpsendfile lt-bpsendfile        bpsink lt-bpsink        bpsource lt-bpsource    bpstats lt-bpstats     bpstats2 lt-bpstats2    bptrace lt-bptrace      bptransit lt-bptransit  brsccla lt-brsccla      brsscla lt-brsscla      bsscounter lt-bsscounter       bssdriver lt-bssdriver  bsspadmin lt-bsspadmin  bsspcli lt-bsspcli      bsspclo lt-bsspclo      bsspclock lt-bsspclock  bssrecv lt-bssrecv     bssStreamingApp lt-bssStreamingApp      cgrfetch lt-cgrfetch    cpsd lt-cpsd    dccpcli lt-dccpcli      dccpclo lt-dccpclo    dccplsi lt-dccplsi       dccplso lt-dccplso      dgr2file lt-dgr2file    dgrcli lt-dgrcli        dgrclo lt-dgrclo        dtka lt-dtka    dtkaadmin lt-dtkaadmin         dtn2admin lt-dtn2admin  dtn2adminep lt-dtn2adminep      dtn2fw lt-dtn2fw        dtpcadmin lt-dtpcadmin  dtpcclock lt-dtpcclock         dtpcd lt-dtpcd  dtpcreceive lt-dtpcreceive      dtpcsend lt-dtpcsend    file2dgr lt-file2dgr    file2sdr lt-file2sdr    file2sm lt-file2sm     file2tcp lt-file2tcp    file2udp lt-file2udp    hmackeys lt-hmackeys    imcadmin lt-imcadmin    imcadminep lt-imcadminep      imcfw lt-imcfw   ionadmin lt-ionadmin    ionexit lt-ionexit      ionrestart lt-ionrestart        ionsecadmin lt-ionsecadmin      ionunlock lt-ionunlock         ionwarn lt-ionwarn      ipnadmin lt-ipnadmin    ipnadminep lt-ipnadminep        ipnd lt-ipnd    ipnfw lt-ipnfw  lgagent lt-lgagent     lgsend lt-lgsend        ltpadmin lt-ltpadmin    ltpcli lt-ltpcli        ltpclo lt-ltpclo        ltpclock lt-ltpclock    ltpcounter lt-ltpcounter       ltpdeliv lt-ltpdeliv    ltpdriver lt-ltpdriver  ltpmeter lt-ltpmeter    ltpsecadmin lt-ltpsecadmin      nm_agent lt-nm_agent  nm_mgr lt-nm_mgr         owltsim lt-owltsim      owlttb lt-owlttb        psmshell lt-psmshell    psmwatch lt-psmwatch    ramsgate lt-ramsgate  rfxclock lt-rfxclock     sdatest lt-sdatest      sdr2file lt-sdr2file    sdrmend lt-sdrmend      sdrwatch lt-sdrwatch    sm2file lt-sm2file    smlistsh lt-smlistsh     smrbtsh lt-smrbtsh      stcpcli lt-stcpcli      stcpclo lt-stcpclo      tcaadmin lt-tcaadmin    tcaboot lt-tcaboot    tcacompile lt-tcacompile tcapublish lt-tcapublish        tcarecv lt-tcarecv      tcc lt-tcc      tccadmin lt-tccadmin    tcp2file lt-tcp2file  tcpbsi lt-tcpbsi         tcpbso lt-tcpbso        tcpcli lt-tcpcli        tcpclo lt-tcpclo        udp2file lt-udp2file    udpbsi lt-udpbsi      udpbso lt-udpbso         udpcli lt-udpcli        udpclo lt-udpclo        udplsi lt-udplsi        udplso lt-udplso                amsbenchr lt-amsbenchr         amsbenchs lt-amsbenchs  amsd lt-amsd    amshello lt-amshello    amslog lt-amslog        amslogprt lt-amslogprt  amsshell lt-amsshell   amsstop lt-amsstop      bputa lt-bputa  cfdpadmin lt-cfdpadmin  cfdpclock lt-cfdpclock  cfdptest lt-cfdptest    bpcp lt-bpcp    bpcpd lt-bpcpd ...
Sending KILL to the processes...
Checking if all processes ended...
Deleting shared memory to remove SDR...
Killm completed.
ION node ended. Log file: ion.log
```

At this point run `ps -aux` or `ipcs` to verify that ION has terminated completely.

### Simple ION Installation Test

When ION is not running, you can performance a simple unit test to verify ION is build properly.

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

In this document, we assume that ION was build and installed via the `./configure` installation process using the full open-source codebase and with the standard set of options.

The location and content of the library and header directories shown above included non-BP modules and may not match exactly with what you have especially if you have built ION with features options enabled via `./configure` or used a manual/custom Makefile, or built ION from the `ion-core` package instead.

## Launch ION & BP Services

Once you are confidant that ION has been properly built and installed in the system, you can start BP service by launching ION. To do this, please consult the various tutorials under _Configuration_.

After launching ION, you can [verify BP service status](#determine-bp-service-state) in the same manner as described in previous section.

## BP Service API Reference

### Header

```c
#include "bp.h"
```

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
void bp_detach( )

```

Parameters

* None.

Return Value

* 0: success
* -1: Any error

Description

Terminates all access to BP functionality for the invoking process.

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
int bp_send(BpSAP sap, char *destEid, char *reportToEid, 
             int lifespan, int classOfService, BpCustodySwitch custodySwitch, 
             unsigned char srrFlags, int ackRequested, 
             BpAncillaryData *ancillaryData, Object adu, Object *newBundle)
```

Parameters

* `sap`: the source endpoint for the bundle, provided by the `bp_open` call.
* `*destEid`: identifies the destination endpoint for the bundle.
* `reportToEid`: identifies the endpoint to which any status reports pertaining to this bundle will be sent; if NULL, defaults to the source endpoint.
* `lifespan`: is the maximum number of seconds that the bundle can remain in-transit (undelivered) in the network prior to automatic deletion.
* `classOfService`: is simply priority for now: BP_BULK_PRIORITY, BP_STD_PRIORITY, or BP_EXPEDITED_PRIORITY. If class-of-service flags are defined in a future version of Bundle Protocol, those flags would be OR'd with priority.
* `custodySwitch`: indicates whether or not custody transfer is requested for this bundle and, if so, whether or not the source node itself is required to be the initial custodian. The valid values are SourceCustodyRequired, SourceCustodyOptional, NoCustodyRequired. Note that custody transfer is possible only for bundles that are uniquely identified, so it cannot be requested for bundles for which BP_MINIMUM_LATENCY is requested, since BP_MINIMUM_LATENCY may result in the production of multiple identical copies of the same bundle. Similarly, custody transfer should never be requested for a "loopback" bundle, i.e., one whose destination node is the same as the source node: the received bundle will be identical to the source bundle, both residing in the same node, so no custody acceptance signal can be applied to the source bundle and the source bundle will remain in storage until its TTL expires.
* `srrFlags`: if non-zero, is the logical OR of the status reporting behaviors requested for this bundle: BP_RECEIVED_RPT, BP_CUSTODY_RPT, BP_FORWARDED_RPT, BP_DELIVERED_RPT, BP_DELETED_RPT.
* `ackRequested`: is a Boolean parameter indicating whether or not the recipient application should be notified that the source application requests some sort of application-specific end-to-end acknowledgment upon receipt of the bundle.
* `ancillaryData`: if not NULL, is used to populate the Extended Class Of Service block for this bundle. The block's ordinal value is used to provide fine-grained ordering within "expedited" traffic: ordinal values from 0 (the default) to 254 (used to designate the most urgent traffic) are valid, with 255 reserved for custody signals. The value of the block's flags is the logical OR of the applicable extended class-of-service flags:

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
int bp_track(Object bundle, Object trackingElt)
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
Object bundleList;

/* a bundle object in SDR */
Object bundleObject;

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
void bp_untrack(Object bundle, Object trackingElt)
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
int bp_suspend(Object bundle)
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
int bp_resume(Object bundle)
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
int bp_cancel(Object bundle)
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
int bp_release(Object bundle)
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

Be sure to call `bp_release_delivery`() after every successful invocation of `bp_receive`().

The function returns 0 on success, -1 on any error.

---

### bp_interrupt

Function Prototype

```c
void bp_interrupt(BpSAP sap)
```

Parameters

* `bundle`: a bundle object in the SDR

Return Value

* 0: success
* -1: Any error

Description

Interrupts a `bp_receive`() invocation that is currently blocked. This function is designed to be called from a signal handler; for this purpose, sap may need to be obtained from a static variable.

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

## Walk Through of `bpsource.c`

* For this example, it is assumed that the user is already familiar with the [ICI APIs](./ICI-API.md).

**TO BE UPDATED.**

## Compiling and Linking

## Zero-copy Object (ZCO) Types

We have shown that the way to hand user data from an application to BP is via a zero-copy object (ZCO).

In general there are two types of ZCO that is relevant to a user.

* Note A third type of ZCO is called a ZCO ZCO - one consists of multiple other ZCOs. This is used internally by ION to handle concatenation of data but it is not relevant in the context of this topic.

### SDR ZCO

* Used in our example
* Example in `bpsource.c`

### File ZCO

* Can be used if you wish not to make a copy of the data in the ION SDR (possibly to save SDR space)
* You allow ION to access the original data (file) in the host computer's file system. Need to check if this restriction applies.
* See example in `bpdriver.c`

## Python API

ION provides a Python API that is a wrapper around the C API, called [PYION](https://github.com/nasa-jpl/pyion), which is available on GitHub.