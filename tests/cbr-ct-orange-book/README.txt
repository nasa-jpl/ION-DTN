CBR Phase 6 Integration Tests - Multi-hop Custody Transfer
==========================================================

PURPOSE
-------
This test validates end-to-end custody transfer across a 3-node DTN network
using Compressed Bundle Reporting (CBR) and Custody Transfer (CT) per the
CCSDS Orange Book Draft K specification.

TOPOLOGY
--------
    Node 1              Node 2              Node 3
    (source)   --->   (custodian)   --->   (destination)
    ipn:1.*            ipn:2.*             ipn:3.*

    UDP ports:
    - Node 1: 127.0.0.1:4551
    - Node 2: 127.0.0.1:4552
    - Node 3: 127.0.0.1:4553

TEST CASES
----------
1. Basic Bundle Transfer (no custody)
   - Verifies network connectivity
   - Bundle flows: Node 1 -> Node 2 -> Node 3

2. Bundle Transfer with Custody Request
   - Source requests custody transfer
   - CTEB block added to bundle
   - Each hop takes custody and sends CCS back

3. CTEB Block Processing
   - Verifies CTEB is processed at intermediate node
   - Custodian EID updated at each hop

4. CCS Signal Flow
   - Node 3 sends CCS to Node 2 (accepting custody)
   - Node 2 sends CCS to Node 1 (accepting custody)
   - Previous custodians release custody on CCS receipt

5. Custody Tracking Cleanup
   - Verifies no orphaned custody entries
   - All custody released after successful delivery

6. Multiple Bundle Custody Transfer
   - Sends multiple bundles with custody
   - Verifies all delivered correctly

RUNNING THE TEST
----------------
From this directory:
    ./dotest

Or using ION's test framework:
    cd <ion-root>/tests
    ./runtests cbr/phase6-integration

EXPECTED OUTPUT
---------------
All tests should pass with output similar to:
    Tests run:    6
    Tests passed: 6
    Tests failed: 0
    All Phase 6 integration tests PASSED

CLEANUP
-------
Run the cleanup script if the test is interrupted:
    ./cleanup

FILES
-----
- dotest         : Main test script
- cleanup        : Cleanup script
- global.ionrc   : Contact plan for all nodes
- 1.node/        : Node 1 (source) configuration
- 2.node/        : Node 2 (intermediate custodian) configuration
- 3.node/        : Node 3 (destination) configuration

DEPENDENCIES
------------
- ION must be built with CBR/CT support
- bpsource and bpsink utilities
- UDP CLA configured

TROUBLESHOOTING
---------------
1. If test fails to start nodes:
   - Check that no ION processes are running (killm)
   - Verify UDP ports 4551-4553 are available

2. If bundles don't arrive:
   - Check ion.log files in each node directory
   - Verify contact plan timing (contacts start at +5 seconds)

3. If custody signals fail:
   - Check that CTEB block type 13 is registered
   - Verify CCS admin record handler is installed
