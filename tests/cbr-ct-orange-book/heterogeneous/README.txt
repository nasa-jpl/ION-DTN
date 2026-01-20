CBR Phase 6 - Heterogeneous Network Test
=========================================

PURPOSE:
Tests custody transfer through a network with mixed CBR-supporting
and non-supporting intermediate nodes.

TOPOLOGY:
  Node 1 (source) --> Node 2 (relay) --> Node 3 (destination)
       CBR              no CBR              CBR

TEST SCENARIOS:
1. Basic bundle transfer through heterogeneous path
2. Custody bundle through non-supporting relay
3. CTEB block transparent forwarding verification
4. CCS signal return path through relay
5. Multiple bundles through heterogeneous path

KEY VERIFICATION POINTS:
- CTEB block is preserved through Node 2 (transparent forwarding)
- Node 2 does not process or modify CTEB block
- Bundles with custody request reach destination
- CCS signals flow back through the same path
- Custody is properly released at source

BLOCK PROCESSING FLAGS:
The CTEB block uses CBR_BLOCK_FLAGS (0x01) which sets:
- "Block must be replicated in every fragment" = 0
This ensures non-supporting nodes transparently forward the block.

RUNNING THE TEST:
  cd tests/cbr/phase6-integration/heterogeneous
  ./dotest

EXPECTED RESULTS:
All 5 tests should pass, verifying that custody transfer works
correctly even when intermediate nodes don't support CBR/CT.
