CBR/CT Aggregation Stress Test
===============================

Purpose:
--------
This test verifies that Compressed Custody Signals (CCS) and Compressed
Reporting Signals (CRS) are properly aggregated when multiple bundles are
sent rapidly.

Configuration:
--------------
- 2-node setup: Node 1 (source) -> Node 2 (destination/custodian)
- CCS aggregate limit: 5 bundles
- CRS aggregate limit: 5 bundles
- Aggregate timeout: 10 seconds
- Sends 20 bundles rapidly to trigger multiple aggregated signals

Expected Behavior:
------------------
1. Node 1 sends 20 bundles with custody transfer requested
2. Node 2 receives bundles and queues CCS acknowledgments
3. CCS signals should be aggregated (4 signals of 5 bundles each)
4. Aggregated CCS sent back to Node 1 to release custody
5. All bundles should be delivered and custody released

Test Verification:
------------------
- All 20 bundles received at destination
- Custody tracking shows 0 bundles after CCS processing
- Log analysis shows aggregated signals (bundleCount > 1)

Files:
------
1.node/  - Source node configuration
2.node/  - Destination/custodian node configuration
dotest   - Test execution script
cleanup  - Cleanup script (removes artifacts but preserves configs)
