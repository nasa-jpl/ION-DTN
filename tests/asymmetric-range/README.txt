This test exercises CGR behavior with asymmetrically configured ranges in the
contact graph. Only one node (node 9) is instantiated for testing purposes.

TOPOLOGY:

The notional network has 3 nodes:
- Node 9: Local node (running)
- Node 5: Intermediate node (not running)
- Node 6: Destination node (not running)

ASYMMETRIC RANGES:

The contact graph includes asymmetric propagation delays:
- Node 9 -> Node 5: 1 second OWLT
- Node 5 -> Node 9: 5 seconds OWLT (asymmetric return)
- Node 5 -> Node 6: 20 seconds OWLT
- Node 6 -> Node 9: 1 second OWLT

CONTACT SCHEDULE:

- Contact 9<->5: Available from +0 to +60 seconds
- Contact 5<->6: Available from +0 to +3600 seconds
- Contact 6<->9: Available from +90 to +3600 seconds

TEST BEHAVIOR:

The test sends three bundles with different lifetimes to verify:
1. Bundle with 10-second lifetime: Put into limbo (no current route)
2. Bundle with 25-second lifetime: Forwarded but expires before retransmission
3. Bundle with 120-second lifetime: Forwarded, reforwarded upon timeout

This exercises CGR's ability to:
- Compute routes with asymmetric propagation delays
- Handle bundle reforwarding when acknowledgments don't arrive
- Properly expire bundles based on their lifetimes

CGR VISUALIZATION:

The dotest script automatically generates JSON files for CGR route visualization
using the cgrfetch utility. These files show how CGR computes routes considering
the asymmetric propagation delays:

  - cgr_to_node5.json  (node 9 -> node 5)
  - cgr_to_node6.json  (node 9 -> node 6)

To visualize these routes:
1. Run the test: ./dotest
2. Open contrib/cgr-viewer/cgr-viewer.html in a web browser
3. Click "Choose JSON" and select one of the generated JSON files
4. Explore the routes and observe the asymmetric OWLT values on the edges

The JSON files are preserved after test cleanup for visualization purposes.
The graphs will show the asymmetric propagation delays as edge labels, helping
visualize why CGR makes particular routing decisions.
