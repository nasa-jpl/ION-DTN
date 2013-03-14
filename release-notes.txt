= Release Notes for ION 3.1.2 =

March 13, 2013

%%%%%%%%%%%
= GENERAL =
%%%%%%%%%%%

The ION (interplanetary overlay network) software is a suite of communication
protocol implementations designed to support mission operation communications
across an end-to-end interplanetary network, which might include on-board
(flight) subnets, in-situ planetary or lunar networks, proximity links, deep
space links, and terrestrial internets.  Included in the ION software
distribution are the following packages:

- ici (interplanetary communication infrastructure), a set of libraries
  that provide flight-software-compatible support for functions on which
  the other packages rely, such as dynamic memory management, non-volatile
  storage management, and inter-task communication via shared memory.
  The ici libraries are designed to make the porting of IPN software to
  multiple operating systems - Linux, VxWorks, Solaris, etc. - as easy as
  possible. Ici now includes zco (zero-copy object), a library that 
  minimizes the copying of application data as it is encapsulated in 
  multiple layers of protocol structure while traversing the protocol
  stack.

- bp (bundle protocol), an implementation of the Delay-Tolerant
  Networking (DTN) architecture's Bundle Protocol.

- ltp (licklider transmission protocol), a DTN convergence layer for
  reliable transmission over links characterized by long or highly
  variable delay.

- dgr (datagram retransmission), an alternative implementation of ltp
  that is designed for use over the Internet protocol stack.  dgr
  implements congestion control and is designed for relatively high
  performance.

- ams - an implementation of the CCSDS Asynchronous Message Service.

- cfdp - a class-1 (Unacknowledged) implementation of the CCSDS File
  Delivery Protocol.

- bss - a Bundle Streaming Service (BSS) for disruption-tolerant reliable
  data streaming.  BSS supports real-time streaming applications by
  passing the bundle payloads to the associated application for immediate
  display of the most recent data while storing all bundle payloads
  received into a database for user-directed playback.


Features included:

- cgr - Contract graph routing: a method of dynamic routing designed for
  space based applications of ION, but still usable for terrestrial
  applications. It computes routes using scheduled communication and deals
  with time-varying network topology.

- brs - Bundle relay service: provides interconnectivity between networks
  that do not allow servers (those behind NAT for example). For more
  information, check man brsscla and man brsccla.

%%%%%%%%%%%%%%%%%%%%%%%%%%
= NOTES ON RELEASE 3.1.2 =
%%%%%%%%%%%%%%%%%%%%%%%%%%

- Bug #13: Migrated admission control facilities into the ZCO subsystem so that
  they can be used outside of the BP subsystem.

- Bug #14: Fixed bug where CFDP headers could be stripped by the ZCO subsystem.

- Bug #15: Fixed various bugs that could affect ION's stability.
    - Fixed possible deadlock if the bpcp/bpcpd were terminated via a signal
      while in the middle of an SDR transaction.
    - Fixed bug in the outducts of several convergence layers and forwarder
      daemons that could cause them to not shut down properly due to an
      unintended interaction between the erasure of a taskVar semaphore with
      their internal looping constructs.

- Feature Request #1: Implemented "payload classes" to enable more
  sophisticated contact graph routing functionality.
  Extended CGR to compute routes on a per-payload-class basis that respects
  the maximum bundle size that can be sent along each route.
  The current payload classes are as follows:
    - Payloads up to 1024 bytes (1 KB)
    - Payloads between 1024 bytes (1 KB) and 1048576 bytes (1 MB)
    - Payloads between 1048576 bytes (1 MB) and 1073741824 bytes (1 GB)
    - Payloads larger than 1073741824 bytes (1 GB)

- Feature Request #4: Updated SDNV decoder and corresponding APIs to support
  up to 64-bit SDNV's on 32-bit systems, allowing for a wider space of possible
  IPN values.

- Added uClibc support for ION.

%%%%%%%%%%%%%%%%%%%%%%%%%%
= NOTES ON RELEASE 3.1.1 =
%%%%%%%%%%%%%%%%%%%%%%%%%%

- Bug #9:  Fixed several bugs in the TCP convergence layer.
    - TCPCLO would not reconnect to its neighbor if the neighbor was unreachable
      on startup.
      Fixed by instituting a default keepalive (currently 15 seconds).
    - Fixed a TCPCL interoperability bug when an external implementation uses
      full-duplex TCP connections where the tcpclo would fail to receive bundles
      from this implementation if the TCP connection had ever been previously
      lost.
    - Capped the maximum TCP reconnection timeout to 1 hour down from 24 hours.

- Bug #11: Fixed bug with transaction reversibility enabled where unpredictable
  behavior could occur when a transaction reversal was forced and the state of
  the SDR and working memory were out of sync.
  ION now reinitializes working memory from the SDR to ensure data consistency.

- Bug #12: Fixed a bug in the red-black tree data structure used in LTP by which
  sm_rbt_destroy() didn't release the tree's mutex to the OS which could
  ultimately lead to consumption of all semaphores on the system and crash the
  node.

%%%%%%%%%%%%%%%%%%%%%%%%%%
= NOTES ON RELEASE 3.1.0 =
%%%%%%%%%%%%%%%%%%%%%%%%%%

- Began transitioning to the SourceForge issue tracker for code management.
  SourceForge-originated issues are designated as "Bugs" or "Feature Requests".

- Bug #1: Fixed bug where the contact graph routing engine would prefer a
  multi-hop route to the source over the "no-hop" loopback connection.

- Bug #2: Fixed bug in contact graph routing engine where erroneous routes
  could be selected if route caching was enabled and any "downstream" contact on
  a route had an end time earlier than the end time of the first contact on the
  route.

- Bug #3: Updated code and configurations to always use the dotted-string
  representation of the sender's IP address rather than hostname.  This was done
  to mitigate problems that can arise due to aliasing and multihoming.

- Bug #7: Fixed bug where ionadmin would duplicate all currently-added contacts
  and ranges that were added in the ION configuration files before starting
  ION with the 's' command.

- Bug #8: Fixed bug where bundles that expired while in the limbo queue were not
  properly deallocated.

- Bug #10: Updated the ionstart/ionscript/killm scripts with support for the
  following ION administrative modules:
    - Bundle streaming service (BSS)
    - Interplanetary multicast (IMC)
    - Aggregate custody signals (ACS)

- Feature Request #2: Added support for bundle age extension blocks to help
  accomodate platforms that may not be able to provide a stable clock. 
  For more information on the bundle age extension, refer to internet draft
      draft-irtf-dtnrg-bundle-age-block-01

- Feature Request #3: Added support for bundle multicast.

- Issue #195: Finished replacement of pthreads-based semaphore "unwedge"
  function with simpler platform-specific equivalents, begun in ION 2.4.0.

- Issue #306: Implemented several improvements to congestion forecasting.
    - Improved the accuracy of the forecast of maximum storage occupancy
      by incorporating estimations based on the rate of data being
      transmitted in and out of the internal data heaps.

    - ION now raises an alarm if expected peak volume of data is larger than
      ION's allocated file storage size.

    - Added new utility called "ionwarn" that is used for computing congestion
      forecast at an ION node based on contact plan.

- Issue #311: Added a Bundle Streaming Service (BSS) regression test that
  does not require the use of "xterm".

- Issue #339: Fixed bug in aggregate custody signals implementation that
  prevented successful completion of regression tests on some platforms.

- Issue #349: Optimized the LTP engine to achieve faster data rates by by
  substituting red-black trees and hash tables for linked lists where
  appropriate.

%%%%%%%%%%%%%%%%%%%%%%%%%%
= NOTES ON RELEASE 3.0.2 =
%%%%%%%%%%%%%%%%%%%%%%%%%%

- Issue #280: Fixed a deadlock scenario when using "ionstart" to start two nodes
  nodes in parallel.

- Issue #349: Applied some optimizations to the LTP engine to help it operate
  at data rates that can exceed 80Mbps.

- Issue #357: Fixed compilation issues that prevented ION from building properly
  on some kfreebsd systems.

- Issue #358: Fixed a bug in computing the CFDP inactivity deadline.
  Updated the "cfdprc" man page to expose the parameters for configuring the
  CFDP inactivity timeout interval. 

- Issue #360: Updated the MinGW port of ION to supply definitions for various
  required macros that are no longer supplied by newer versions of MinGW.

- Issue #361: Fixed compilation issues that prevented ION from building properly
  on some debian-unstable systems.

- Issue #362: Fixed a bug where transplantation of man pages into ION.pdf could
  fail if an obsolete version of man was used, causing compilation to fail.  ION
  now detects if the required "-l" option for man is supported, and does not
  attempt to transplant the man pages if the option is not found.

- Issue #363: Fixed several memory leaks where SDR heap space was allocated but
  not freed.

%%%%%%%%%%%%%%%%%%%%%%%%%%
= NOTES ON RELEASE 3.0.1 =
%%%%%%%%%%%%%%%%%%%%%%%%%%

- *Issue #347: Included a test exercizing the fix for the TCPCL reconnection
  bug that was made available in ion-open-source v3.0.0.

- Issue #348:  Refreshed the MinGW and RTEMS ports of ION to build properly.

- Issue #352: BPCP, an rcp-like remote copy utility that transfers files via
  CFDP, is a new utility now included with ION.  It is built and installed by
  default and has an interface similar to rcp, with support for local to
  remote, remote to local, and remote to remote transfers in recursive and
  non-recursive modes.

- Issue #353:  Scrubbed various test configurations in the test suite that 
  produced superfluous messages regarding the bundle security database not
  being initialized properly.

- Issue #354:  Slightly changed the behavior of "make retest", a directive that
  reruns tests that failed on the previous test run.
  Previously "make retest" would re-run the entire test suite if issued
  immediately following a completely sucessful test run.
  "make retest" now returns SUCCESS immediately if issued following a
  completely successful test run, a behavior more amenable to automated
  testing.

- Issue #355:  Updated the ION build process to be compatible with the latest
  versions of autotools, particularly automake v1.12.

- Issue #356:  Enhanced the bptrace application with file-sending capabilities.

- ION has now been ported to the "bionic" libc implementation, the first
  step toward porting ION to the Android operating system.

- Starting from this release the ION manual "ION.pdf" will be distributed with
  an appendix containing up-to-date man pages for all ION applications.

%%%%%%%%%%%%%%%%%%%%%%%%%%
= NOTES ON RELEASE 3.0.0 =
%%%%%%%%%%%%%%%%%%%%%%%%%%

- Issue #242: Removed unnecessary recursion from the CGR implementation to help
  protect against overflows from incorrect configurations.

- Issue #262: ION is now wired with a rudimentary instrumentation framework for
  insight into run-time status, including bundle processing statistics and node
  state metrics.  This is in preparation for standardized network management. 

- Issue #302: Implemented red-black trees as an alternative data structure to
  linked lists for some performance-sensitive applications.  Migrated the BP
  timeline to use red-black trees.

- Issue #304: Optimized CGR route computation by developing a new algorithm that
  employs Dijkstra's Algorithm.

- Issue #308: Several updates to ION's Payload Integrity Blocks (PIB) and
  Payload Confidentiality Blocks (PCB):
    - Bundle Security Protocol (BSP) utilities now support PIB and PCB.
    - PIB and PCB extension block code has been added.
    - Roughly 40 new test cases for PIB, PCB, and BAB combinations have been
      introduced.

- Issue #311: Bundle Streaming Service (BSS) support has been added to ION.
  BSS passes "real-time" bundle payloads to an application callback while also
  storing the bundle payloads to a database for user-directed playback.

- Issue #314: Standardized spelling of the word "semaphore" throughout the ION
  codebase.

- Issue #322: Improved reliability for custodial retransmissions when the next
  custodian is a neighbor by making the following tweaks in libbpP.c:
    - Changed the bpDequeue "stewardship accepted" flag to a custodial timeout
      interval value (in seconds) that is computed by the CL output daemon,
      based on CL-specific knowledge, in the event that the CL protocol is not
      reliable and therefore can't accept stewardship of the bundle. 
    - Added the same custodial timeout interval value to bpHandleXmitSuccess, to
      trigger reforwarding of the bundle even when stewardship has been taken
      and CL transmission has been nominally successful and reliable, in the
      event that the CL transmission ultimately (unexpectedly) does not succeed.

- Issue #325: Fixed a bug that could cause ION to crash when sending bundles for
  which the destination endpoint is not a singleton.

- Issue #329: ION now supports sending (rather than just receiving) fragmented
  bundles.  (NOTE: this entailed several changes to the ZCO API.) 

- Issue #339: Aggregate Custody Signals (ACS) have been added to ION.
    - ACS is a technology that combines information from various separate
      custody signals into a single bundle, which has the potential to save
      "ack channel" bandwidth.
    - For more information on ACS, see the following document:
        <http://bioserve.colorado.edu/bp-acs/draft-kuzminsky-aggregate-custody-signals-02.txt>
  
- Issue #342: Changed several invocations of lyst_create() to
  lyst_create_using(), to ensure that ION’s private memory management is always
  used.

- Issue #343: Fixed bugs in LTP that could cause crashes at high data loss rates
    - Fixed mechanisms for preventing resurrection of closed Import sessions.
    - Fixed issue where the same report number could become assigned to two
      different reports, resulting in link service daemons crashing.

- Issue #344: Added a "-t <ttl>" command line parameter to set the time-to-live
  for bundles sent from the bpsource application.  Previously bundles were
  hard-coded with lifetimes of 300 seconds, which is now the default if the TTL
  is not explicitly specified.

- Issue #345: The ION Deployment Guide is now bundled with ION.  It provides
  FAQ-style documentation for helping new users familiarize themselves with ION.

- Issue #347: Fixed bug in re-establishing connectivity via TCPCL when a node
  shuts down and then restarts.

%%%%%%%%%%%%%%%%%%%%%%%%%%
= NOTES ON RELEASE 2.5.3 =
%%%%%%%%%%%%%%%%%%%%%%%%%%

- Issue #271: Various autotools changes to update how CFDP is built.

- Issue #288: Fixed bug in CFDP implementation where performing a proxy "puts"
  operation without supplying a flow label would trigger an assertion failure.

- Issue #290: Fixed bug in CFDP implementation where an internal SDR list was
  improperly referenced.

- Issue #316: Fixed several unresolved symbolic links in the "limbo" subsection
  of the test suite.

- Issue #318: Fixed bug in CFDP implementation where an assertion was triggered
  when a remote directory listing was performed for a directory that did not
  exist.

- Issue #323: Fixed bug in BP implementation where an integer overflow in the
  congestion forecasting code would cause BP to refuse bundles from local user
  applications on 32-bit machines with long, busy contacts.

- Issue #328: Fixed bug in CFDP implementation where internal calculations for
  its CFDP Directory Listing Response user messages were one character shorter
  than they should be.

- Issue #330: Fixed bug in CFDP implementation where cfdpclock would remove
  unsent FDUs that did not contain any actual file data.

- Issue #331: Fixed bug in CFDP implementation where user messages were not
  restricted to a maximum of 255 characters in length, as per the CFDP
  specification.

- Issue #333: Fixed bug in CFDP implementation where originating transaction ID
  messages were not wrapped as CFDP user messages as per the CFDP specification,
  resulting in messages that contained the wrong type codes.

- Issue #334: Fixed bug in CFDP implementation where the cfdp_put() and
  cfdp_get() operations returned "0" as transactionId rather than the actual
  CFDP transaction number.

- Issue #338: Modified test suite to use relative times instead of absolute
  times wherever possible.  This fixes several tests with configurations that
  became obsolete after the start of 2012 and ensures better resilience of the
  test suite to any kind of absolute time boundaries.

- Issue #340: Several minor updates have been applied to the DCCP implementation
  to detect and build DCCP for linux kernels that meet or exceed version 3.2.0,
  which puts several key DCCP fixes into the kernel baseline.

- Status reporting support has been added to cfdptest. 

%%%%%%%%%%%%%%%%%%%%%%%%%%
= NOTES ON RELEASE 2.5.2 =
%%%%%%%%%%%%%%%%%%%%%%%%%%
 
- Issue #297: The ION design guide "ION.pdf" now dynamically incorporates the
  ION manpages.
  This feature requires ghostscript, psutils, groff, and groff-base.
  If not all are present, ION will build but ION.pdf will not have the latest
  manpages appended.
  ION.pdf is built automatically via the "make" command.
  To explicitly build the ION design guide document, use the following command:
     make ION.pdf
 
- Issue #298+: Fixed build incompatibility between gcc v4.6.1 and later with 
  valgrind v3.6.1 and earlier.  If needed to compile with Valgrind support,
  ION will add -Wno-unused-but-set-variable to AM_CFLAGS.
  Valgrind inclusion can also be explicitly controlled via the following flags:
     ./configure --enable-valgrind:  Build with Valgrind support
     ./configure --disable-valgrind: Build without Valgrind support
 
- Issue #299: ION now prints a warning when given any BAB rule that is not for a
  wild-carded EID signifying "all endpoints at the indicated node".
 
- Issue #300: Fixed several bugs in ipnfw:
    - lookupRule() function of libipnfw.c incorrectly used srcServiceNbr
      instead of srcNodeNbr.
    - Several IPN function calls in ipnadmin.c have been wrapped in
      sdr_begin_xn/sdr_exit_xn blocks to protect against assertion failures.
    - Before putting a bundle into the Limbo list, ipnfw now searches for
      blocked outducts and discards the bundle if none are found.
 
- Issue #301: Fixed issue where Ctl+C no longer terminated the bpsink and
  bprecvfile utilities.
 
- Issue #303: Fixed issue where new all-green transmission sessions could be
  inhibited by the LTP limit on the total number of export sessions.
  Since LTP green is unreliable, LTP green sessions don't need to occupy
  resources at the sender for retransmissions, so the sessions need not be
  restricted.
 
- Issue #307: Fixed overflow bugs in the increaseScalar() and reduceScalar()
  functions.
  Added Scalar to SDNV conversion functions.
 
- Issue #313: Fixed issue with overlapping bundle source/destination memory
  addresses that could cause bundle corruption on 64-bit systems.

- Issue #319: Fixed issue with AMS where the parseSocketSpec() function could
  return INADDR_ANY, which isn't an acceptable IP address within AMS. 
  Now possible to enable  AMS debug output via the following configure flag:
     ./configure --enable-ams-debug:  Build with AMS debugging output

- Issue #324: Fixed issue where LTP could accumulate block acquisision files
  after the block acquisition had already been completed.  This was generally
  due to unnecessary retransmitted segments that arrived late.  Since the
  blocks had already been delivered, the ensuing files were never destroyed.

%%%%%%%%%%%%%%%%%%%%%%%%%%
= NOTES ON RELEASE 2.5.1 =
%%%%%%%%%%%%%%%%%%%%%%%%%%

- Issue #185: ION now supports binding to INADDR_ANY in UDP/TCP ports and link
  service adapters.

- Issue #298: Code scrub to remove unused variable warnings that prevent
  compilation of ION under the default settings of gcc 4.6.1.

- Issue #310: Fixed broken Windows support.  Fixed several minor issues.

%%%%%%%%%%%%%%%%%%%%%%%%%%
= NOTES ON RELEASE 2.5.0 =
%%%%%%%%%%%%%%%%%%%%%%%%%%

- Issue #189/#250/#252: ION shutdown utility added.

- Issue #196: The congestion checking subroutines have been implemented more
  efficiently.

- Issue #265: The "bpdriver" application now allows users to set the TTL value
  dynamically via a the "t<ttl>" command line parameter.

- Issue #276: Loopback range can now be set to a non-zero value.

- Issue #279: Fixed issue where calling bpMemo twice created timeline events
  that could not be removed.

- Issue #286: Fixed issue where erroneous input to several administrative
  applications could cause them to crash.

- Issue #292: Fixed issue where pid-checking routines can give false-negatives
  for vxworks processes.

- Issue #296: Corrected usage of internal _xxxConstants() functions to ensure
  they are only used with invariant values.

%%%%%%%%%%%%%%%%%%%%%%%%%%
= NOTES ON RELEASE 2.4.2 =
%%%%%%%%%%%%%%%%%%%%%%%%%%
 
- Issue #264: Removed checks for maximum block size in LTP, simplifying LTP
  configuration and fixing a bug.
  Related to the fix for issue #293, upgrading the LTP configuration worksheet.

- Issue #269: Clean up platform (portability) files, making it easier to add
  new ports.
  Removed support for some obsolete platforms (e.g., HP-UX).

- Issue #270: Fixed issue in cfdptest that prevented the "Custody Requested"
  switch from being set.

- Issue #275: ION now has support for code coverage analysis via lcov/gcov.
  To enable this feature, use "--configure --with-gcov".
  Building with code coverage support enables the following new directives:
     make cov: creates a "coverage" subfolder from the top of the ION
               directory where HTML coverage results are stored
     make cov clean: removes the trace files and the "coverage" subdirectory

- Issue #281: Fixed issue that terminated bputa when it was directed
  to write to a file without sufficient write permissions.

- Issue #282: Fixed issue where LTP segments received outside of a contact
  were mishandled.

- Issue #283: Fixed issue where the CFDP FDU didn't load the database before
  using it, resulting in erroneous data being fed into the CFDP event queue.

- Issue #284: Fixed issue where CFDP mishandled extent merges, resulting
  in checksum failures.

- Issue #285: Fixed issue in cfdpadmin where malformed commands could cause the
  application to crash.

- Issue #287: Fixed issue that prevented the reporting of user messages and
  filestore responses in cfdptest.

- Issue #293: Fixed issue in the ION LTP Worksheet where the spreadsheet
  doesn't account for a small aggregation size limit when calculating
  export blocks

- Issue #295: ION's internal contact graph routing routines have been
  parameterized to enable better support for deployment-specific variations
  of the algorithm.

- ION now has native Windows support via MingW.  
  Support for Cygwin and Interix has been removed.

- Added support for inactivity timer in CFDP.

- Fixed race conditions in support for multithreaded AMS.

- Fixed memory leaks in AMS shutdown.

- Added ability to update AMS modules’ management information bases while the
  modules are running, by using the new amsmib utility.  Modified amsstop to
  use a similar mechanism.

- Added AMS Programmer’s Guide to distribution.

- Various minor bug fixes.

%%%%%%%%%%%%%%%%%%%%%%%%%%
= NOTES ON RELEASE 2.4.1 =
%%%%%%%%%%%%%%%%%%%%%%%%%%

- *Issue #240: Added DCCP link service adapters to ION for the purpose of 
  providing congestion control without reliability guarantees, which is
  beneficial for LTP deployments over IP networks, particularly when traversing
  the Internet. The sender program is called "dccplso" and the receiver program
  is called "dccplsi".

- *Issue #241: Added DCCP convergence layer for BP. The sender program is called
  "dccpclo" and the receiver program is called "dccpcli".

- Issue #245: Added support for using the character "*" as a wildcard to remove
  multiple contacts and ranges at once.

- Issue #257: Fixed issue where bundles would remain in the limbo queue after
  an outduct was blocked rather than be reforwarded.

- Issue #263: Fixed issue with pthread_t.  pthread_t is now treated as an
  opaque type for the purpose of improving code correctness and portability.

- Issue #273: Added a ".hgignore" file that can be populated with regular
  expressions of files to suppress when the "hg status" command is run.

- Issue #277: Fixed 3-node-stcp-ltp configuration to remove an unintentional
  connection between nodes 1 and 3.


- Issue #278: Updated various test configuration files to have more recent
  contact times.

- Tests have been scrubbed to adopt the new security syntax enhancements
  introduced in Issue #247.

* Note: Due to an immature DCCP implementation in most native OS kernels at the
        time of release, formal DCCP testing on 64-bit systems has yet to be
        conducted.  Currently an experimental kernel containing several DCCP
        fixes must be built to achieve acceptable performance.  These fixes are
        expected to become standard in the native linux 3.1 kernel.

%%%%%%%%%%%%%%%%%%%%%%%%%%
= NOTES ON RELEASE 2.4.0 =
%%%%%%%%%%%%%%%%%%%%%%%%%%

- Issue 105: Add correlation blocks to the bundle protocol block extension
  interface.

- Issue 106: Extract bundle block-related functionality into separate source .c
  and .h files to allow for future modular development of block extensions.

- Issue 161: Corrected some redundant and/or circular library linking when
  using autotools.

- Issue 176: Upgrade ltp-cla to better conform to current ltpcla draft in the
  DTNRG.  Particularly the new "green" service handling.

- Issue 195: sm_SemUnwedge() in platform_sm.c uses a pthread to ensure a
  semaphore is not already taken.  Some platforms do not include pthreads and
  the function can be implemented using semaphore trywait semantics().

- Issue 233: Some additional LTP debugging hooks added.

- Issue 238: Fixed possible leak of custodianEid in sendCtSignal.

- Issue 247: Updating the syntax of ionsecadmin for flexibility and readability.

- Issue 248: Fixed a timeline ordering bug in LTP.

- Issue 249: Added support to ION's LTP for section 6.21 of RFC 5326, the LTP
  specification.  Specifically detecting "red" and "green" segments that
  overlap: an error-state.

- Issue 253: Fixed a tcpcl bug: When connection is lost to a node, a keepalive
  needs to be at the end of the backoff period. TCPCL waits backoff period +
  keepalive period to send the keepalive.

- Issue 254: Fixed LTP report serial number bug.

- Issue 255: Fixed bug where pseudoshell can cause duplicate parent processes.

- Issue 256: Fixed uninitialized memory read in reverseEnqueue().

- Issue 258: ZCO referenced file progress.

- Issue 260: Teach memcheck about MTAKE/MRELEASE for improved testing using
  valgrind.

- Various bugfixes and features for AMS package.

- Various bugfixes and features for CFDP package.

%%%%%%%%%%%%%%%%%%%%%%%%%%
= NOTES ON RELEASE 2.3.0 =
%%%%%%%%%%%%%%%%%%%%%%%%%%

- Modified administrative startup commands to block until startup is complete,
  rather than return immediately even if the node hasn't been fully initialized.

- Fixed a bug which truncated dtn-scheme bundle source EIDs.

- Modified some tests (1000.loopback) to run compiled objects instead of shell
  scripts.  This eliminates the need for shell scripting and allows automated
  tests to function on platforms without shell access.

- Combined similar test configurations to use the shared /configs directory.
  This will provide a standard set of updated configurations.

- Test suite has been modified to support multiple test sets.  Creating a
  text file in the tests directory containing a list of tests is all that
  is needed to create a test set.
  Examples: make test-all ; make test-branch ; make test-<anything>

- Added "limbo" queue to support suspension and resumption.  This allows the
  node to handle unexpected convergence layer failure as opposed to scheduled
  or static contact termination.

- Code scrub of DGR.

- Added bping, bpchat, and bpstats2 utilities.

- Custody transfer is now optional in both bptrace and bpsend.

- Added feature for "convergence-layer stewardship"; CL's can initiate
  reforwarding of bundles on transmission failure without explicit
  bundle-layer custodianship.

- Enhanced LTP's udplso with a transmission rate limiter.

- Added new AOS CLA for LTP.

- Bug fixes to: LTP, CGR, SDR mutex, bpcounter, ionscript, tcpcl, stcp,
  PSM mutex, SDR, bpsink, and others.

%%%%%%%%%%%%%%%%%%%%%%%%%%
= NOTES ON RELEASE 2.2.1 =
%%%%%%%%%%%%%%%%%%%%%%%%%%

- Corrected bug in LTP's data acquisition into file-system storage.  Didn't
  handle data loss and retransmission properly.

- Fixed bugs in LTP session cancellation that caused slow storage leaks.

- Upgraded the LTP configuration spreadsheet and its documentation, to
  provide better guidance when most data are sent from or acquired into
  file system storage.

- Updated the Design and Operations Guide to document new features and
  API changes.

- Various other bug fixes and tweaks.

%%%%%%%%%%%%%%%%%%%%%%%%%%
= NOTES ON RELEASE 2.2.0 =
%%%%%%%%%%%%%%%%%%%%%%%%%%

- Store bundles in ZCO.

- Added feature to store bundles directly to file-system memory, allowing
  much larger bundles to be handled by the node.

- Added feature to allow all stdout output to be redirected to log files.
  Feature is enabled by #defining FSWLOGGER.

- Code scrub BP, CFDP, ICI, LTP.

- Add support for asymmetric link-delay (range) in contacts.

- Add option to configure LTP spans for "purge" behavior on contact termination.

- Fixed handling of dtn:none eids.

- Optimize contact graph routing.

- Various bug fixes and tweaks.

%%%%%%%%%%%%%%%%%%%%%%%%%%
= NOTES ON RELEASE 2.1.0 =
%%%%%%%%%%%%%%%%%%%%%%%%%%

- Port to FreeBSD and RTEMS (via manual makefiles).

- Various fixes and tweaks.

- Added comprehensive automated testing suite.

- Fixed various compiler bugs

- Modified custodian EID generated from dtn://hostname to dtn://hostname.dtn

- Modified the loopback and ion-dtn2 configuration files for the new command
  syntax.

- Added BSP implementation using "stub" versions of hmac and sha1 for public
  release.

- Added CFDP implementation.

- Bug fixes in LTP, BRS, and some BP applications.

- Tweaks related to running on VxWorks.

- Added assertions.

- Bug fixes in the TCP convergence layer, particularly in the interest of
  interoperability with dtn2.

%%%%%%%%%%%%%%%%%%%%%%%%%%
= NOTES ON RELEASE 2.0.0 =
%%%%%%%%%%%%%%%%%%%%%%%%%%

- Based on JPL revision 255:6443ed8258e1

- Converted underlying version control system to Mercurial from Subversion.

- Add ionsecadmin for managing the security policy database.

- Added better support for cross-scheme routing.

- Add support for the previous-hop-node extension.

- Update route computation to current CBHE draft standard.

- Addresses memory leak problem identified by U. of Colorado in long-duration
  tests.

- LTP heavily modified to become more flexible and powerful.

- dtnadmin and associated utilities renamed to dtn2admin.

- Configuration file syntax changes slightly in ionadmin, bpadmin, ipnadmin,
  dtn2admin; heavily changes syntax of ltpadmin.  Included ION-LTP-Configuration
  document and spreadsheet to explain new syntax and calculate values.

- Bug fixes.

- Documentation updates.

- Various testing applications in earlier releases are split from the ion
  release.

%%%%%%%%%%%%%%%%%%%%%%%%%%
= NOTES ON RELEASE 1.1.0 =
%%%%%%%%%%%%%%%%%%%%%%%%%%

- Based on JPL revision 226.

- Experimental new tcp convergence layer compatible with the standard defined
  by draft-irtf-dtnrg-tcp-clayer-02.  This convergence layer is titled tcp,
  with other programs tcpcli tcpclo, and is suitable for connectivity with the
  DTN2 reference implementation.  Currently only unidirectional, unacknowledged
  communication is supported.  Some bugs still present.

- ION-specific tcp-based convergence layer is renamed stcp (stcpcli stcpclo)
  and still functions as expected.

- Adds user-contributed applications bping bping bpmon bpalive.

- some LTP related bugs are not yet squashed.

- Updates to various user-contrib programs; added a bpmon_query program to work
  with SNMP.

- Revise LTP to use randomly selected session numbers rather than recycling
  small session numbers - prevent data loss and corruption due to application of
  late-arriving segments to sessions that are re-using the session numbers of
  earlier sessions.

- added the bplive program.

- Updated for sbp_api.h and abp_api.c

- brought over basic working draft-tcpcl standards-compliant code with help of
  patch submitted by Andrew.Jenkins@colorado.edu

- manually applied patches from seb@highlab.com, adding sanity-checks and more
  consistent comment-line detection to the admin programs

- Add fixes to eliminate compiler warnings, per Ohio U.  Remove all ppc-vxworks
  makefile directories, replacing them with arch-vxworks5 directories because
  the VxWorks build varies with VxWorks version number and not with the hardware
  platform that you build for.  Add "expat", which is needed by AMS and is not
  provided with VxWorks.  Fix segfault bug in dtnfw reported by U. of Colorado.
  Fix bug in support for trackingElts list.

- applied the patch from Andrew.Jenkins@colorado.edu about dtnfw's rule/plan but

- added new ION.pdf to the in-development release

- overhauled ionstart, ionscript, ionstart.awk to account for the dtnadmin need
  for a node name as well as the bpadmin/dtnadmin/ipnadmin startup order bug
  found by Andrew.Jenkins@colorado.edu

- Revise routing to match applicable backlog (based on bundle's priority; not
  necessarily total backlog) against aggregate capacity when determining route
  viability.  (Per JIRA item DINET-107)  Add implementation of extension block
  for extended class of service, which includes an additional 256 "ordinal"
  priority sub-levels within priority 2.

- Upgrade AMS implementation to Red Book 2.

- Add interface for inserting BP extensions.

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
= NOTES ON RELEASE 1.0_r203 =
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

- Based on jpl r203.

- Adds LTP package.

- Supports contact graph routing.

- Compilation changed to autoconf and automake; compilation tested on:
  Ubuntu 7.04, 7.10, 8.04, Gentoo, Fedora 3, Fedora 7, OSX 10.5, and Solaris 10.

- Killm updated to work on OSX 10.4, due to an ipcs incompatibility; also
  updated to make sure all processes are killed.

- Removed multiple compiler warnings.

- Includes bundle relay programs brsscla and brsccla; brsscla acts as the
  server, brsccla acts as the client.

- Zco package rolled into ici.

- Ionscript added for configuration file management; ionstart and ionstop
  rewritten.

- Various bug fixes.


- 2008-11-11 Added the ION Design and Operation manual v1.6.

%%%%%%%%%%%%%%%%%%%%%%%%
= NOTES ON RELEASE 0.3 =
%%%%%%%%%%%%%%%%%%%%%%%%

- Based on jpl r105.

- Simplified and cleaned up SDR implementation in the ici package.

- ion package and utilities renamed to "bp" for "bundle protocol."

- Restructured congestion control to be based on rate control.

- Implements BP version 6.

- Revised the zco package.

- Added capability for dynamic routing based on network topology that changes
  with the passage of time.

- Fixed various bugs.

- All administrative and application errors are reported to ion.log

- Added ionstart and ionstop scripts.

%%%%%%%%%%%%%%%%%%%%%%%%
= NOTES ON RELEASE 0.2 =
%%%%%%%%%%%%%%%%%%%%%%%%

- The ion package, which implements the DTN Bundle Protocol (BP), has been
  upgraded to conform to BP version 5, submitted to IETF in December 2006.
  The changes are almost exclusively internal to the software and protocol:
  the format of bundles exchanged through the delay-tolerant network is altered,
  but the API for ION is mostly unchanged.  Note that this release of ION is
  not interoperable with implementations of BP version 4.

- The Remote AMS functionality of AMS has been upgraded to conform to the
  specification most recently posted to the public documents list of the CCSDS
  CWE site for AMS.  The API for AMS is unchanged.

- A variety of miscellaneous bugs have been fixed.

%%%%%%%%%%%%%%%%%%%%%%%%
= NOTES ON RELEASE 0.1 =
%%%%%%%%%%%%%%%%%%%%%%%%

- Initial code released for review.
