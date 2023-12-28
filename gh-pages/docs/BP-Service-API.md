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

Once you are confidant that a good ION build/install is in the system, you can start BP service. To do this, please consult the various tutorials under _Configuration_ section to launch ION.

After launching ION, you can [verify BP service status](#determine-bp-service-state) in the manner described in previous section.

## BP Service API Reference

### Header

* bp.h

-----------

### bp_attach

Function Prototype

```c
extern int bp_attach();
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

------------
### Sdr bp_get_sdr( )

Function Prototype

```c
extern Sdr bp_get_sdr();
```

Parameters

* None.

Return Value

* On success: Handle for the SDR data store
* Any error: `NULL`

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

Returns handle for the SDR data store used for BP, to enable creation and interrogation of bundle payloads (application data units). Since the SDR handle is needed by other , this function is typically executed early in the user's application in order to access other BP services.

---------------

### TBD

Function Prototype

```c

```

Parameters

* None.

Return Value

* 0: success
* -1: Any error

Example Call


Description

---------------

### TBD

Function Prototype

```c

```

Parameters

* None.

Return Value

* 0: success
* -1: Any error

Example Call


Description

---------------

### TBD

Function Prototype

```c

```

Parameters

* None.

Return Value

* 0: success
* -1: Any error

Example Call


Description

---------------

### TBD

Function Prototype

```c

```

Parameters

* None.

Return Value

* 0: success
* -1: Any error

Example Call


Description

---------------

### TBD

Function Prototype

```c

```

Parameters

* None.

Return Value

* 0: success
* -1: Any error

Example Call

```c

```

Description

---------------

### TBD

Function Prototype

```c

```

Parameters

* None.

Return Value

* 0: success
* -1: Any error

Example Call


Description

---------------

### TBD

Function Prototype

```c

```

Parameters

* None.

Return Value

* 0: success
* -1: Any error

Example Call

```c

```

Description

---------------






## Code Example


```c++
#include <stdio.h>
#include "bp.h"
#include "sdr.h"
#include "ion.h"

int main(int argc, char *argv[])
{
        unsigned int choice = 0;
        int ret_value;

        do
        {
                printf( "0 - Exit\n" );
                printf( "1 - BP Attach\n" );
                printf( "2 - BP Detach\n" );
                printf( "3 - SDR Shutdown\n" );
                printf( "4 - ION Detach\n" );
                printf( "Option:  " );
                scanf( "%u", &choice );

                switch( choice )
                {
                        case 0:
                                break;
                        case 1:
                                ret_value = bp_attach();

                                if( ret_value == 0 )
                                        printf( "Attached\n" );
                                else
                                {
                                        printf( "Error %d returned from bp_attach().\n", ret_value );
                                        return 0;
                                }
                                break;
                        case 2:
                                bp_detach();
                                break;
                        case 3:
                                sdr_shutdown();
                                break;
                        case 4:
                                ionDetach();
                                break;
                        default:
                                printf( "That's not a choice you should make...\n" );
                                break;
                };
        } while( choice );

        return 0;
}

```


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