# ION Regression & Demo Test Matrix

This matrix maps every regression test (`tests/`) and demonstration (`demos/`) in the ION source tree to the DTN function or behavior it exercises, the kind of test it is, and the relevant IETF/CCSDS specification. It is generated from the tests' own descriptions and scripts.

- **Category / Subcategory** — the DTN function area exercised (see the summary below).
- **Type** — *System-dataflow* (end-to-end multi-hop scenario), *Integration* (multi-node feature exercise), *Regression* (pins a specific bug/issue fix), *Unit* (single component/algorithm), *Utility* (exercises a CLI tool), *Benchmark* (throughput/performance), *Conformance* (explicit spec check).
- **Spec §** — the governing standard; a section number is shown only where it is confidently applicable. `ION-specific` means no ratified standard governs the behavior; `—` means internal platform infrastructure. See the [Specification References](#specification-references) key.

## Coverage summary

| Category | Tests |
| --- | ---: |
| Bundle Protocol v7 — Core | 23 |
| Custody Transfer & Status Signaling | 26 |
| Routing — Contact Graph Routing / SABR | 19 |
| Licklider Transmission Protocol (LTP) | 18 |
| Convergence-Layer Adapters (CLAs) | 18 |
| Security — BPSec & Key Management | 9 |
| CCSDS File Delivery Protocol (CFDP) | 8 |
| Application Services & Utilities | 17 |
| Core Infrastructure & Platform | 24 |
| Performance Benchmarks | 12 |
| **Total** | **174** |

## System / data-flow demonstrations

These tests demonstrate an end-to-end data flow across a topology rather than checking a single component — useful as worked examples of ION carrying data through a scenario:

- **`demos/bss-multicast`** — Bundle Streaming Service multicast streaming to group (BSS multicast stream node 1 to group 19 for 30s; secondary: BP-CORE/multicast)
- **`tests/file-transfer`** — File CLA store-and-forward file transfer (File CLA sneakernet scenario; separate send/receive files)
- **`tests/relay-restart-mixed-cla`** — Relay restart mid-transfer with mixed LTP/TCP CLAs (3 nodes, 200 bundles; relay resumes after restart; mixed LTP+TCP)

> Note: many *Integration* tests below also exercise multi-node data flows for a single feature (relays, BPSec over LTP, CFDP over LTP, custody chains); the list above is limited to tests whose primary purpose is demonstrating a full scenario.

## Matrix by DTN function

### Bundle Protocol v7 — Core  <span style="font-weight:normal">(BP-CORE, 23 tests)</span>

| Test | Subcategory | DTN function / behavior | Type | Spec § | Notes |
| --- | --- | --- | --- | --- | --- |
| `tests/bug-critical-dedup-diamond` | critical-bundles | Dedup critical bundle on diamond+tail topology | Regression | RFC 9171 §5 | cbdedup de-duplicates critical (BP_MINIMUM_LATENCY) bundle at relay; 5 nodes; UDP |
| `tests/issue-325-329-critical` | critical-bundles | Critical bundle transmission | Regression | RFC 9171 | Pins #325/#329; critical bundle replication over routes |
| `tests/extblk-relay` | extension-blocks | Relay updates Bundle Age, continues Previous Node block hop-by-hop | Integration | RFC 9171 §4.2.3 | Bundle Age + Previous Node blocks on relay; non-ION inbound bundle; CCSDS 734.2-O-1 |
| `tests/extblk-source-defaults` | extension-blocks | Source-node extension block generation defaults | Conformance | RFC 9171 §4.2.3 | No PNB; Bundle Age only when creation time zero; per CCSDS 734.2-O-1 BPv7 |
| `tests/issue-37-blknum-overflow` | extension-blocks | Extension block number 255 forwarding without selectBlkNumber overflow | Regression | RFC 9171 §4.2.3 | Issue #37; 1 node UDP + Python receiver |
| `tests/crc_relay` | forwarding-relay | CRC type preserved for primary/payload blocks through relay | Integration | RFC 9171 §4 | Block CRC handling preserved end-to-end through relay |
| `tests/relay-restart-mixed-cla` | forwarding-relay | Relay restart mid-transfer with mixed LTP/TCP CLAs | System-dataflow | RFC 9171 §5 | 3 nodes, 200 bundles; relay resumes after restart; mixed LTP+TCP |
| `tests/issue-325-329-fragmentation` | fragmentation-reassembly | Partial replication of fragmented bundles | Regression | RFC 9171 §5.8 | Pins #325/#329; fragmentation with critical replication |
| `tests/issue-265-bpdriver-ttl-option` | lifetime-expiration | bpdriver sends bundles with varying TTL values | Regression | RFC 9171 §5.4 | Pins #265; bpdriver TTL option |
| `tests/issue-344-bpsource-ttl` | lifetime-expiration | bpsource sends bundles with varying TTL values | Regression | RFC 9171 §5.4 | Pins #344; bpsource TTL option |
| `tests/req-0002-bundle-age` | lifetime-expiration | Bundle expiration with and without synchronized clocks | Integration | RFC 9171 §5.4 | Bundle Age block / expiration for clock-synced and unsynced nodes |
| `tests/cpsync` | multicast | Multicast (imc) bundle transmission | Integration | RFC 9171 | Contact plan sync / multicast delivery; imc scheme is ION-specific |
| `tests/req-0003-multicast` | multicast | Multicast transmission | Integration | RFC 9171 | imc/multicast group delivery |
| `tests/req-0003-multicast-v7` | multicast | BPv7 multicast transmission | Integration | RFC 9171 | imc/multicast group delivery under BPv7 |
| `tests/req-0003-multicast-v7.ipn2` | multicast | BPv7 multicast transmission (ipn2 variant) | Integration | RFC 9171 | imc/multicast group delivery; ipn2 addressing |
| `tests/priorities` | priority-qos | Bundle priority effect on transmission ordering | Integration | RFC 9171 | Class-of-service priority ordering |
| `tests/stewardship` | reliability-stewardship | Delay bundle deletion until CLA notifies transmission outcome | Regression | RFC 9171 | Non-custodial bundle not deleted before CL xmit-completion notice |
| `tests/status-rpts` | status-reports | Bundle status report generation and logging | Integration | RFC 9171 | Status reports generated and logged |
| `tests/1021.dynamic-ep-valgrind` | transmission | Send 5 bundles between BP endpoints over UDP | Integration | RFC 9171 | Dynamic endpoint; UDP CLA; valgrind profiling |
| `tests/conformance_test` | transmission | Several bundles handled per RFC 9171 conformance | Conformance | RFC 9171 | 2-node; explicit BPv7 conformance check |
| `tests/ipn-null-eid` | transmission | dtn:none null EID handling does not crash node | Regression | RFC 9171 | Null endpoint ID (dtn:none) robustness |
| `tests/issue-236-src-eid-trunc` | transmission | Source EID not truncated via dpdriver/bpecho | Regression | RFC 9171 | Source EID truncation fix |
| `tests/issue-313-overlapping-memcpy` | transmission | Multiple bundles per LTP segment separated without corruption | Regression | RFC 9171 | Pins #313 overlapping memcpy in acquisition; LTP transport |

### Custody Transfer & Status Signaling  <span style="font-weight:normal">(CUSTODY, 26 tests)</span>

| Test | Subcategory | DTN function / behavior | Type | Spec § | Notes |
| --- | --- | --- | --- | --- | --- |
| `tests/bibect` | bibe-custody | Bundle-in-Bundle Encapsulation transfer | Integration | draft-ietf-dtn-bibect | BIBE encapsulation |
| `tests/bibect.ipn2` | bibe-custody | Bundle-in-Bundle Encapsulation transfer | Integration | draft-ietf-dtn-bibect | ipn2 SANA-range variant |
| `tests/req-0019-bibe` | bibe-custody | Bundle-in-Bundle Encapsulation | Integration | draft-ietf-dtn-bibect | BIBE encapsulation of bundles |
| `tests/req-0019-bibe.ipn2` | bibe-custody | Bundle-in-Bundle Encapsulation (ipn2 variant) | Integration | draft-ietf-dtn-bibect | BIBE; ipn2 addressing |
| `tests/cbr-ct-orange-book/cbr-aggr-runtime` | cbr-ct-signaling | Runtime reconfiguration of CRS aggregation limits | Integration | CCSDS 734.6-O-1 §5.2 | G8: live m cbraggr flushes pending CRS; 2 nodes |
| `tests/cbr-ct-orange-book/cbr-counter-width` | cbr-ct-signaling | Configurable sequence-counter wraparound width | Regression | CCSDS 734.6-O-1 §3.2 | G5: m cbrcounterwidth 16/32/64; single node config |
| `tests/cbr-ct-orange-book/creb-eid-persist` | cbr-ct-signaling | CREB source EID preserved through relay re-serialization | Regression | CCSDS 734.6-O-1 §5.1 | creb_parse sourceEid persistence (arrayLen>=4) through relay; 3 nodes |
| `tests/cbr-ct-orange-book/crebreportto-redirect` | cbr-ct-signaling | Override CREB report-to EID via m crebreportto | Regression | CCSDS 734.6-O-1 §5.1 | G4: crebDefaultReportToEid redirects CRS; 3 nodes |
| `tests/cbr-ct-orange-book/crs-aggregation` | cbr-ct-signaling | Aggregate multiple bundles into fewer CRS signals | Integration | CCSDS 734.6-O-1 §5.2 | cbraggr CRS-limit aggregation (20 bundles -> ~4 CRS); 2 nodes |
| `tests/cbr-ct-orange-book/crs-deletion` | cbr-ct-signaling | Deletion CRS generated on no-route abandonment | Regression | CCSDS 734.6-O-1 §5.2 | Deletion CRS status=3 via SrNoKnownRoute; Node 3 unreachable |
| `tests/cbr-ct-orange-book/crs-history` | cbr-ct-signaling | CRS reception history log query and filtering | Regression | CCSDS 734.6-O-1 §5.2 | l crslog history, status/sender filtering, m crsmaxlog; 2 nodes |
| `tests/cbr-ct-orange-book/crs-individual-flags` | cbr-ct-signaling | CRS reception/forwarding/delivery status flags generated independently | Integration | CCSDS 734.6-O-1 §5.2 | 3-node; bptrace -flags rcv/fwd/dlv; secondary: status-reports |
| `tests/cbr-ct-orange-book/crs-simple` | cbr-ct-signaling | CREB block attach and basic CRS generation/return | Integration | CCSDS 734.6-O-1 §5.1–5.2 | 2-node; CREB block, rcv+dlv CRS returned to source |
| `tests/cbr-ct-orange-book/custody-bundle-query` | cbr-ct-signaling | bpadmin custodybundle query of pending custody entries | Utility | CCSDS 734.6-O-1 §4.3 | 2-node; admin query of custody bundles; cbrretx=none |
| `tests/cbr-ct-orange-book/custody-ccs-verify` | cbr-ct-signaling | CCS signal flow verified via cbrcustodytest tool | Utility | CCSDS 734.6-O-1 §4.2 | 3-node; cbrcustodytest verifies custody/CCS receive/release |
| `tests/cbr-ct-orange-book/custody-stress` | cbr-ct-signaling | CCS/CRS signal aggregation under rapid bundle load | Integration | CCSDS 734.6-O-1 §4.2 | aggregate limit 5; 20 bundles -> 4 aggregated CCS |
| `tests/cbr-ct-orange-book/seqid-gap` | cbr-ct-signaling | CRS CBOR range-array encoding for non-contiguous seqNums | Integration | CCSDS 734.6-O-1 §5.2 | 4-node; gap via no-route; range-array CBOR decode; secondary: ici-encoding (seq range arrays §3.3) |
| `tests/cbr-ct-orange-book/seqid-nonzero` | cbr-ct-signaling | Non-zero seqId uses global sequence counter for custody | Integration | CCSDS 734.6-O-1 §3.2 | 3-node; global vs destination-specific counters; custody release verified |
| `tests/cbr-ct-orange-book/creb-custody-flags` | custody-acceptance | CREB custody-event request flags and accept CRS | Regression | CCSDS 734.6-O-1 §5.1 | G3: CREB request flags; custody-accepted CRS status=4; 2 nodes |
| `tests/cbr-ct-orange-book/custody-accept-whitelist` | custody-acceptance | cbraccept custodian whitelist enforced (refuse then accept) | Regression | CCSDS 734.6-O-1 §4.3 | 2-node; cbraccept whitelist policy; CTEB accept/refuse |
| `tests/cbr-ct-orange-book/custody-auto-request` | custody-acceptance | custodyreq policy auto-promotes bundle to custody | Regression | CCSDS 734.6-O-1 §4.3 | 2-node; auto custody-request attaches CTEB; CCS acceptance/release |
| `tests/cbr-ct-orange-book/custody-simple` | custody-acceptance | End-to-end custody transfer with CCS release | Integration | CCSDS 734.6-O-1 §4.3 | 3-node (1->2->3); custody request, CCS release, tracking empty after |
| `tests/cbr-ct-orange-book/heterogeneous` | custody-acceptance | CTEB transparently forwarded through non-CBR intermediate node | Integration | CCSDS 734.6-O-1 §4.1 | 3-node; relay lacks CBR; CTEB flags preserved; CCS returns; secondary: forwarding-relay |
| `tests/cbr-ct-orange-book/ccs-retransmit-range-array` | custody-retransmission | CCS-triggered retransmit of batched refused bundles | Regression | CCSDS 734.6-O-1 §4.3 | G6: cbr_handleCcs fix; RETX_SIGNAL retransmits 3 batched refused bundles; 2 nodes |
| `tests/cbr-ct-orange-book/custody-multi` | custody-retransmission | Multiple custody bundles all released after CCS | Integration | CCSDS 734.6-O-1 §4.3 | 3-node; 5 bundles; all custody entries released after CCS |
| `tests/cbr-ct-orange-book/custody-retransmit-timer` | custody-retransmission | Retransmit timer reforwards bundle when CCS never arrives | Regression | CCSDS 734.6-O-1 §4.3 | 2-node; cbrretx timer 5s; bpclock cbr_processTimeouts -> reforward |

### Routing — Contact Graph Routing / SABR  <span style="font-weight:normal">(ROUTING, 19 tests)</span>

| Test | Subcategory | DTN function / behavior | Type | Spec § | Notes |
| --- | --- | --- | --- | --- | --- |
| `tests/issue-196-checkforcongestion-looping` | congestion-forecasting | checkforcongestion() does not loop excessively | Regression | ION-specific (no ratified standard) | Congestion forecast loop-bound fix |
| `tests/issue-306-congestion-forecasting` | congestion-forecasting | Congestion forecasting works across configurations | Regression | ION-specific (no ratified standard) | Pins #306; congestion forecast engine |
| `tests/issue-323-congestion-forecasting-overflow` | congestion-forecasting | Congestion forecasting overflow corrected | Regression | ION-specific (no ratified standard) | Pins #323; overflow-inducing config still delivers |
| `tests/bug-0001-cgr-loopback` | contact-graph-routing | CGR routes bundles over loopback interface | Regression | CCSDS 734.3-B-1 | CGR loopback routing |
| `tests/cgr-test` | contact-graph-routing | CGR routing over a very large contact graph | Integration | CCSDS 734.3-B-1 | large contact plan scale test of SABR/CGR |
| `tests/cgr-test.ipn2` | contact-graph-routing | CGR routing over a very large contact graph | Integration | CCSDS 734.3-B-1 | ipn2 variant; large contact plan scale test |
| `tests/cgr-zeroed-hop-recovery` | contact-graph-routing | Relay contact expiry must not crash ipnfw via zeroed route hop | Regression | CCSDS 734.3-B-1 | pins #1047; zeroed CGR route-hop guard on contact expiry |
| `tests/issue-950-cgr-semaphores` | contact-graph-routing | CGR semaphore-footprint regression | Regression | CCSDS 734.3-B-1 | Issue #950; CGR resource/semaphore usage |
| `tests/adjacent-contacts` | contact-plan-mgmt | Seamless bundle transfer across adjacent contacts | Integration | CCSDS 734.3-B-1 | 2-node; back-to-back contacts without overlap; 6000 bundles |
| `tests/asymmetric-range` | contact-plan-mgmt | Asymmetric contact-graph range: forwarding, reforwarding, expiration | Integration | CCSDS 734.3-B-1 | Asymmetric OWLT ranges; expects forwarding, reforwarding, expiration |
| `tests/bug-0007-ionadmin-duplicate-contacts` | contact-plan-mgmt | ionadmin rejects duplicate contact entries | Regression | — (implementation) | Contact-plan management admin behavior |
| `tests/contact-volume/ltp-loopback` | contact-plan-mgmt | Contact volume behavior over LTP loopback | Integration | CCSDS 734.3-B-1 | Contact volume/capacity; single-node LTP loopback |
| `tests/contact-volume/udp-loopback` | contact-plan-mgmt | Contact volume behavior over UDP loopback | Integration | CCSDS 734.3-B-1 | Contact volume/capacity; single-node UDP loopback |
| `tests/issue-245-contactrangewildcard` | contact-plan-mgmt | Wildcard contact/range deletion works | Regression | CCSDS 734.3-B-1 | ionadmin contact/range wildcard deletion fix |
| `tests/issue-276-loopback-range` | contact-plan-mgmt | Non-zero loopback one-way light time supported | Regression | ION-specific (no ratified standard) | Pins #276; range/OWLT on loopback |
| `tests/bug-0008-limbo-bpclock-use-after-free` | limbo-reforwarding | Delete limbo-queued bundles on lifetime expiration | Regression | RFC 9171 §5.4 | limbo queue use-after-free fix in bpclock; secondary: lifetime-expiration |
| `tests/limbo` | limbo-reforwarding | Bundle limbo via blocking/unblocking an outduct | Integration | ION-specific (no ratified standard) | Limbo/reforwarding on outduct block-unblock |
| `tests/limbo.ipn2` | limbo-reforwarding | Bundle limbo via blocking/unblocking an outduct | Integration | ION-specific (no ratified standard) | Limbo/reforwarding on outduct block-unblock; ipn2 variant |
| `tests/req-0033-prob-CGR` | opportunistic-prob-routing | Opportunistic/probabilistic forwarding | Integration | CCSDS 734.3-B-1 | Simple opportunistic forwarding test |

### Licklider Transmission Protocol (LTP)  <span style="font-weight:normal">(LTP, 18 tests)</span>

| Test | Subcategory | DTN function / behavior | Type | Spec § | Notes |
| --- | --- | --- | --- | --- | --- |
| `tests/issue-324-ltp-files` | acquisition-files | LTP acquisition files removed after receipt | Regression | RFC 5326 | Pins #324; cleanup of receiving-node acquisition files |
| `tests/issue-264-ltp-blksize` | block-size-aggregation | LTP no longer limits aggregated block size | Regression | RFC 5326 | Pins #264; CCSDS 734.1-B-1 equivalent |
| `tests/ltp-tune` | block-size-aggregation | LTP throughput tuning across aggregation/session params | Benchmark | RFC 5326 | 2-node LTP perf-tuning harness; prints throughput |
| `tests/issue-303-green-sessions` | green-unreliable | Green data flows when red session limit reached | Regression | RFC 5326 | Pins #303; LTP green vs red sessions |
| `tests/ltp-green` | green-unreliable | LTP green (unacknowledged) transmission | Integration | RFC 5326 | Green/unreliable LTP data; no retransmission |
| `tests/ltp-inactivity-timeout` | inactivity-stale-session | Stale import session cleanup via inactivity timeout | Regression | RFC 5326 | Sender crashes before checkpoint/EORP/EOB; receiver import session times out |
| `tests/1002.loopback-valgrind` | red-retransmission | LTP loopback bundle send under valgrind | Integration | RFC 5326 | Single-node LTP loopback; valgrind profiling; CCSDS 734.1-B-1 equivalent |
| `tests/1003.loopback-sdr` | red-retransmission | LTP loopback multi-bundle send, memory-leak check | Integration | RFC 5326 | Single-node loopback; SDR memory-leak regression origin |
| `tests/ltp-retransmission` | red-retransmission | LTP block reassembly with out-of-order retransmitted segments | Integration | RFC 5326 | 2-node LTP; red-part retransmission; CCSDS 734.1-B-1 equivalent |
| `tests/ltp-retransmission.ipn2` | red-retransmission | Red block reassembly with out-of-order segments from retransmission | Integration | RFC 5326 | Retransmission-induced out-of-order arrival; ipn2 variant |
| `tests/ltp-sda` | service-data-aggregation | LTP Service Data Aggregation with two client IDs | Integration | CCSDS 734.1-B-1 §7 | SDA client op per CCSDS LTP §7; distinct client IDs at different layers |
| `tests/ltp-cancel-ack-regression` | session-cancellation | Cancel Segment ack for already-completed sessions | Regression | RFC 5326 §6 | Cancel segs after session completion must be acked; prevents needless retransmission |
| `tests/ltp-purge` | session-cancellation | LTP Purge functionality | Integration | RFC 5326 | Issue #173; multi-node purge of LTP sessions/blocks |
| `tests/issue-920-ltp-removespan-closedimports` | span-configuration | ltp removeSpan succeeds with non-empty closedImports | Regression | RFC 5326 | Issue #920; span teardown correctness |
| `tests/ltp-config-incomplete-warnings` | span-configuration | Warnings and default application for incomplete LTP config | Integration | ION-specific (no ratified standard) | Single node; LTP admin default-value application |
| `tests/ltp-config-span` | span-configuration | Per-span LTP override configuration | Integration | RFC 5326 | Per-span retries/loss overrides vs global defaults |
| `tests/ltp-config-split` | span-configuration | Asymmetric split-mode LTP xmit/recv configuration | Integration | RFC 5326 | Different xmit/recv retries and loss per direction |
| `tests/ltp-config-unified` | span-configuration | Unified-mode LTP maxretries/maxseglossrate config | Integration | RFC 5326 | 'm maxretries'/'m maxseglossrate' with 10% loss via owltsim |

### Convergence-Layer Adapters (CLAs)  <span style="font-weight:normal">(CLA, 18 tests)</span>

| Test | Subcategory | DTN function / behavior | Type | Spec § | Notes |
| --- | --- | --- | --- | --- | --- |
| `tests/ipaddr-caching-udpclo` | address-caching | Dual-stack IPv4/IPv6 address caching/re-resolution in udpclo | Regression | ION-specific (no ratified standard) | UDP CLO DNS re-resolution; invalid-address fallback logging |
| `tests/ipaddr-caching-udplso` | address-caching | IP address caching/periodic re-resolution in udplso (LTP) | Regression | ION-specific (no ratified standard) | LTP UDP link service DNS re-resolution |
| `tests/bssp` | bssp | Exercise BSSP protocol and API | Integration | ION-specific (no ratified standard) | Bundle Streaming Service Protocol |
| `tests/loopback-bp-dccp` | dccp | BP over DCCP convergence layer loopback | Integration | ION-specific (no ratified standard) | DCCP CLA; single-node loopback |
| `tests/loopback-ltp-dccp` | dccp | LTP over DCCP loopback | Integration | ION-specific (no ratified standard) | DCCP as LTP link-service; LTP per RFC 5326 |
| `tests/file-transfer` | file-epp-spp | File CLA store-and-forward file transfer | System-dataflow | ION-specific (no ratified standard) | File CLA sneakernet scenario; separate send/receive files |
| `tests/loopback-epp` | file-epp-spp | EPP CLA loopback with stub provider library | Integration | ION-specific (no ratified standard) | EPP CLA; custom loopback stub provider |
| `tests/loopback-file` | file-epp-spp | File CLA loopback with simple file provider library | Integration | ION-specific (no ratified standard) | File CLA; custom loopback provider |
| `tests/loopback-spp` | file-epp-spp | SPP CLA loopback with stub provider library | Integration | ION-specific (no ratified standard) | SPP CLA; custom loopback stub provider |
| `tests/issue-132-udplso-tx-rate-limit` | rate-limiting | Transmission rate limit on UDP link service to LTP | Regression | ION-specific (no ratified standard) | udplso tx rate limiting; LTP over UDP |
| `tests/qos-besteffort-tcp-fallback` | rate-limiting | Best-effort bundles fall back to reliable TCP CLA | Integration | ION-specific (no ratified standard) | QoS best-effort uses TCP CLA when no unreliable CLA; CLA selection |
| `tests/bug-0009-tcpclo-fixes` | tcpcl | tcpclo handles neighbors unavailable at startup | Regression | RFC 9174 | TCPCLv4 CLO outduct startup handling |
| `tests/bug-0015-tcpclo-bpcp-sig-handling` | tcpcl | Signal-interrupted semaphore ops in tcpclo/bpcp handlers | Regression | — (implementation) | EINTR semaphore handling in tcpclo/bpcp/bpcpd |
| `tests/issue-253-tcpcl-keepalive` | tcpcl | TCPCL keepalive backoff timer triggers keepalive | Regression | RFC 9174 | TCPCLv4 keepalive timer; every 30s |
| `tests/issue-253-tcpcl-keepalive-v6` | tcpcl | TCPCL keepalive backoff timer triggers keepalive (IPv6) | Regression | RFC 9174 | TCPCLv4 keepalive timer; IPv6; every 30s |
| `tests/issue-347-tcpcl-reconnection` | tcpcl | tcpcli recovers from neighbor shutdown cascade | Regression | RFC 9174 | Pins #347; tcpcli reconnection after shutdown message |
| `tests/tcpcl-ack-resilience` | tcpcl | TCPCL recovers and completes file transfer after mid-stream block | Regression | RFC 9174 | TCPCLv4 resilience to interrupted connection; RFC 7242 also relevant |
| `tests/tcpcl-dos` | tcpcl | tcpcli hardened against denial-of-service bug | Regression | RFC 9174 | Security/robustness of TCPCL input daemon |

### Security — BPSec & Key Management  <span style="font-weight:normal">(SECURITY, 9 tests)</span>

| Test | Subcategory | DTN function / behavior | Type | Spec § | Notes |
| --- | --- | --- | --- | --- | --- |
| `tests/bpsec/bpsec-bcb-only.bsl` | bcb-confidentiality | BCB confidentiality blocks across 3 nodes over LTP | Integration | RFC 9172 | BSL; confidentiality only; RFC 9173 default contexts |
| `tests/bpsec/bpsec-bib-only.bsl` | bib-integrity | BIB integrity blocks across 3 nodes over LTP | Integration | RFC 9172 | BSL; integrity only; RFC 9173 default contexts |
| `tests/bpsec/bpsec-verifier.bsl` | bib-integrity | BIB/BCB verification at intermediate relay node | Integration | RFC 9172 | Verifier role (2->3->4) over LTP; BIB and BCB verification |
| `tests/tc-dtka` | key-distribution | Delay-tolerant key administration (DTKA) key distribution | Integration | draft-burleigh-dtnwg-dtka | DTKA; multi-node public-key distribution |
| `tests/tc-dtka.ipn2` | key-distribution | Delay-tolerant key administration (DTKA) key distribution | Integration | draft-burleigh-dtnwg-dtka | DTKA; ipn2 variant; multi-node key distribution |
| `tests/bpsec/bpsec-all-multinode-test.bsl` | security-policy | BIB+BCB applied across 3 nodes over LTP | Integration | RFC 9172 | BSL; 6 bundles integrity+confidentiality; 2->3 and 2->3->4; RFC 9173 contexts |
| `tests/bpsec/bpsec-policy-test` | security-policy | BPSec policy rules for BIB and BCB source/acceptor | Integration | RFC 9172 | 3-4 nodes over LTP; bpsecadmin policyrule/event_set; bptrace-driven |
| `tests/bpsec/bpsec-target-mult.bsl` | security-policy | Multiple security blocks per bundle over LTP relay | Integration | RFC 9172 | BSL target multiplicity (2->3); RFC 9173 contexts |
| `tests/bpsec/python_tests` | security-policy | Python-driven BPSec test suite runner | Integration | RFC 9172 | dotest.py --test_macro all; needs Python 3.7+ |

### CCSDS File Delivery Protocol (CFDP)  <span style="font-weight:normal">(CFDP, 8 tests)</span>

| Test | Subcategory | DTN function / behavior | Type | Spec § | Notes |
| --- | --- | --- | --- | --- | --- |
| `tests/issue-330-cfdpclock-FDU-removal` | fdu-management | cfdpclock no longer removes FDUs without file data | Regression | CCSDS 727.0-B | Pins #330; cfdpclock FDU lifecycle |
| `demos/bench-cfdp` | file-delivery | Benchmark/exercise CCSDS file delivery protocol revisions | Benchmark | CCSDS 727.0-B | CFDP throughput demo; secondary: BENCH |
| `tests/cfdpv1` | file-delivery | CFDP v1 file delivery protocol revisions | Integration | CCSDS 727.0-B | baseline CFDP v1 test |
| `tests/cfdpv1-4node-ltp` | file-delivery | CFDP over LTP with limbo via outduct block/unblock | Integration | CCSDS 727.0-B | 4-node over LTP; limbo/reforwarding; RFC 5326 LTP CLA |
| `tests/cfdpv1-4node-ltp.ipn2` | file-delivery | CFDP over LTP with limbo via outduct block/unblock | Integration | CCSDS 727.0-B | 4-node ipn2 over LTP; limbo/reforwarding; RFC 5326 LTP CLA |
| `tests/cfdpv1-tcp` | file-delivery | CFDP v1 file delivery over TCP CLA | Integration | CCSDS 727.0-B | TCPCL transport |
| `tests/cfdpv1-tcp.ipn2` | file-delivery | CFDP v1 file delivery over TCP CLA | Integration | CCSDS 727.0-B | ipn2 variant; TCPCL transport |
| `tests/issue-358-cfdp-inactivity` | inactivity-deadline | CFDP inactivity deadline option handled correctly | Regression | CCSDS 727.0-B | Pins #358; CFDP inactivity deadline config |

### Application Services & Utilities  <span style="font-weight:normal">(APP-SVC, 17 tests)</span>

| Test | Subcategory | DTN function / behavior | Type | Spec § | Notes |
| --- | --- | --- | --- | --- | --- |
| `tests/ams-sana` | ams | AMS with large Node/Continuum numbers (SANA range upgrade) | Integration | CCSDS 735.1-B | Verifies AMS message space with expanded SANA node/continuum ranges |
| `tests/ams-sana.ipn2` | ams | AMS with large Node/Continuum numbers (SANA range upgrade) | Integration | CCSDS 735.1-B | ipn2 variant; expanded SANA node/continuum ranges |
| `tests/issue-319-parseSocketSpec` | ams | AMS socket-spec parsing / basic AMS functionality | Regression | CCSDS 735.1-B | Pins #319 parseSocketSpec; AMS message space |
| `tests/bpchat` | bp-utilities | bpchat bidirectional bundle send/receive | Utility | RFC 9171 | Exercises bpchat CLI over BP |
| `tests/bpcp-dirlist-truncation` | bp-utilities | bpcp -r surfaces incomplete listings/unsafe filenames/symlinks | Regression | ION-specific (no ratified standard) | bpcp recursive-copy safety |
| `tests/bping` | bp-utilities | Basic bping echo utility functionality | Utility | ION-specific (no ratified standard) | BP echo/ping utility |
| `tests/bpinspect-ops` | bp-utilities | bpinspect cancel/suspend/resume/filter/dry-run operations | Utility | ION-specific (no ratified standard) | bpinspect bundle-management CLI |
| `tests/bpstats2` | bp-utilities | Exercise bpstats2 statistics utility | Utility | ION-specific (no ratified standard) | bpstats2 CLI basic functionality |
| `tests/bptrace_terminal_test` | bp-utilities | Verify terminal-mode bptrace utility behavior | Utility | ION-specific (no ratified standard) | bptrace CLI (terminal); status-report tracing |
| `tests/issue-352-bpcp` | bp-utilities | bpcp local/remote file copy | Utility | ION-specific (no ratified standard) | Pins #352; local/remote/remote-remote copies |
| `tests/issue-352-bpcp-ltp` | bp-utilities | bpcp local/remote file copy over LTP | Utility | ION-specific (no ratified standard) | Pins #352; bpcp over LTP CLA |
| `tests/issue-352-bpcp-stcp` | bp-utilities | bpcp local/remote file copy over STCP | Utility | ION-specific (no ratified standard) | Pins #352; bpcp over STCP CLA |
| `tests/smart-file-transfer` | bp-utilities | sendfile/recvfile transfer with optional encryption | Utility | ION-specific (no ratified standard) | 2-node sendfile/recvfile; encrypt/decrypt variant; secondary: BPSec |
| `demos/bss-multicast` | bss | Bundle Streaming Service multicast streaming to group | System-dataflow | ION-specific (no ratified standard) | BSS multicast stream node 1 to group 19 for 30s; secondary: BP-CORE/multicast |
| `tests/issue-364-dtpc` | dtpc | Delay-tolerant payload conditioning | Regression | ION-specific (no ratified standard) | Pins #364; DTPC |
| `tests/issue-364-dtpc.ipn2` | dtpc | Delay-tolerant payload conditioning (2-node) | Integration | ION-specific (no ratified standard) | Pins #364; DTPC; ipn2 topology |
| `tests/ipnd` | neighbor-discovery | IP neighbor discovery of BPAs via beacons, dynamic outduct creation | Integration | draft-irtf-dtnrg-ipnd | IPND beacons over TCP CLA; 2 active nodes |

### Core Infrastructure & Platform  <span style="font-weight:normal">(INFRA, 24 tests)</span>

| Test | Subcategory | DTN function / behavior | Type | Spec § | Notes |
| --- | --- | --- | --- | --- | --- |
| `tests/1001.sysctl-script` | admin-config | Verify OSX sysctl shared-memory limits sufficient for ION | Utility | — (implementation) | Platform precondition check on macOS; no data flow |
| `tests/1004.loopback-bpstats` | admin-config | Verify BP batched statistics tally counts over LTP loopback | Regression | ION-specific (no ratified standard) | ION batched-stats tally functions; LTP loopback |
| `tests/1005.relay-stats` | admin-config | Verify BP and LTP batched statistics across 3-node relay | Integration | ION-specific (no ratified standard) | 3-node source/relay/dest; secondary: forwarding-relay |
| `tests/admin_parsers` | admin-config | Reject malformed numeric inputs across admin CLIs | Regression | — (implementation) | platform_parse_* validation across ionadmin/ltpadmin/bpsecadmin |
| `tests/issue-285-286-segfaults` | admin-config | Admin command syntax errors no longer segfault | Regression | — (implementation) | Pins #285/#286; ionadmin parser robustness |
| `tests/issue-918-bpadmin-null-eid` | admin-config | bpadmin rejects null-EID endpoint registration cleanly | Regression | — (implementation) | Issue #918; dtn:none / ipn:0.0 register/delete |
| `tests/linking` | build-linking | Verify linking cleanliness of built executables | Utility | — (implementation) | Build/link integrity check |
| `tests/atomics` | ici-encoding | Validate IPC atomics header across C11/builtin/sync tiers | Unit | — (implementation) | Offline per-tier compile+run of ion_atomic; no ION node |
| `tests/cbor-oob-read` | ici-encoding | CBOR string decoder rejects over-declared length | Regression | — (implementation) | ICI CBOR decoder OOB-read/underflow guard (VDP-2945) |
| `tests/issue-302-fast-structures` | ici-encoding | ICI fast data structures functionality | Regression | — (implementation) | Pins #302; 2-node LTP; internal data structures |
| `demos/watchdog-daemon` | lifecycle | psmwatch/sdrwatch auto-launch, ionrestart, ionexit lifecycle | Integration | — (implementation) | Watchdog daemon auto-launch; restart and shutdown; two LTP nodes |
| `tests/ionexit_test` | lifecycle | ionexit cleans up ION processes and shared resources | Utility | — (implementation) | ionexit IPC/shm/sem cleanup |
| `tests/multinode-sdrwmkey` | lifecycle | Per-node sdrWmKey isolates node shutdown from peers | Integration | — (implementation) | Unique sdrWmKey makes SDR working memory per-node |
| `tests/ion-memory-usage/memprotect-settings` | psm-memory | Memory protection threshold, edge-triggered logging, breach detection | Regression | — (implementation) | PSM/SDR memory watermark protection |
| `tests/ion-memory-usage/monitoring` | psm-memory | sdrwatch high-water memory warning and recovery logging | Utility | — (implementation) | sdrwatch daemon monitoring |
| `tests/issue-234-235` | psm-memory | PSM/SDR mutex bugs eliminated | Regression | — (implementation) | Mutex bugs in PSM and SDR (issues 234, 235) |
| `tests/issue-260-teach-valgrind-mtake` | psm-memory | Valgrind detects leaked blocks (mtake instrumentation) | Regression | — (implementation) | Valgrind PSM mtake leak-detection tooling |
| `tests/psm-trace-restart` | psm-memory | PSM trace lifecycle and trace-restart leak fix | Regression | — (implementation) | traceCount episodes, sptrace_stop leak fix, trace restart |
| `tests/issue-965-file-backed-zco-abort` | sdr-transactions | Unreadable file-backed ZCO drops one bundle not whole instance | Regression | — (implementation) | Issue #965; ZCO file-backed extent error handling |
| `tests/req-0022-reversibility/reversibilityCorrectness` | sdr-transactions | reverseTransaction restores SDR byte-for-byte on cancel | Unit | — (implementation) | Cancelled modified transaction reverses; read-only skip path when unmodified |
| `tests/req-0022-reversibility/reversibilityRecovery` | sdr-transactions | SDR transaction reversibility recovery via ionrestart | Regression | — (implementation) | Cancel-with-mods enters reversal; ionrestart; post-recovery round-trip |
| `tests/robust-sdr-lock-recovery` | sdr-transactions | SDR lock robust-mutex recovery after mid-transaction death | Regression | — (implementation) | Robust-mutex EOWNERDEAD recovery |
| `tests/sdr-no-dram` | sdr-transactions | SDR heap file-only mode (configFlags=2, no RAM copy) | Integration | — (implementation) | 2 nodes, 30 bundles with SDR heap in files only |
| `tests/sdr-preservation` | sdr-transactions | SDR data preserved across ionexit shutdown/restart | Integration | — (implementation) | ionexit k preservation for FILE and DRAM modes |

### Performance Benchmarks  <span style="font-weight:normal">(BENCH, 12 tests)</span>

| Test | Subcategory | DTN function / behavior | Type | Spec § | Notes |
| --- | --- | --- | --- | --- | --- |
| `demos/bench-cgr` | cgr-benchmark | Contact graph routing (SABR) throughput/performance benchmark | Benchmark | CCSDS 734.3-B-1 | CGR/SABR perf over STCP on one host; secondary: ROUTING |
| `demos/bench-ams` | throughput-benchmark | AMS two-node message throughput benchmark | Benchmark | CCSDS 735.1-B | AMS benchtest, 1000 msgs of 1000 bytes; secondary: APP-SVC/ams |
| `demos/bench-bibect` | throughput-benchmark | BIBE custodial-retransmission throughput over UDP | Benchmark | draft-ietf-dtn-bibect | BIBE over UDP with custodial retransmission; secondary: CUSTODY/bibe-custody |
| `demos/bench-bssp` | throughput-benchmark | BSSP convergence-layer throughput benchmark | Benchmark | ION-specific (no ratified standard) | BSSP CLA same-host; secondary: CLA/bssp |
| `demos/bench-dgr` | throughput-benchmark | DGR convergence-layer throughput benchmark | Benchmark | ION-specific (no ratified standard) | DGR (Internet-compatible LTP variant); secondary: CLA/dgr |
| `demos/bench-ltp` | throughput-benchmark | LTP convergence-layer throughput benchmark | Benchmark | RFC 5326 | LTP CLA same-host; CCSDS 734.1-B-1 equivalent |
| `demos/bench-ltp-xlsa` | throughput-benchmark | LTP throughput benchmark over shared-memory xlsa link | Benchmark | RFC 5326 | LTP over ION-specific xlsa ring; delay/drop/rate sweeps |
| `demos/bench-stcp` | throughput-benchmark | STCP convergence-layer throughput benchmark | Benchmark | ION-specific (no ratified standard) | Simplified TCP CLA; secondary: CLA/stcp |
| `demos/bench-tcp` | throughput-benchmark | TCPCL convergence-layer throughput benchmark | Benchmark | RFC 9174 | TCP CLA same-host; secondary: CLA/tcpcl |
| `demos/bench-tcpv6` | throughput-benchmark | TCPCL-over-IPv6 convergence-layer throughput benchmark | Benchmark | RFC 9174 | IPv6 transport; secondary: CLA/tcpcl |
| `demos/bench-testutils-ltp` | throughput-benchmark | LTP throughput benchmark using test utilities | Benchmark | RFC 5326 | LTP CLA benchmark variant using testutils |
| `demos/bench-udp` | throughput-benchmark | UDP convergence-layer throughput benchmark | Benchmark | ION-specific (no ratified standard) | UDP CLA same-host; secondary: CLA/udp |

## Specification references

| Reference | Title / notes |
| --- | --- |
| [RFC 9171](https://www.rfc-editor.org/rfc/rfc9171) | Bundle Protocol Version 7 |
| [RFC 9172](https://www.rfc-editor.org/rfc/rfc9172) | Bundle Protocol Security (BPSec) |
| [RFC 9173](https://www.rfc-editor.org/rfc/rfc9173) | Default Security Contexts for BPSec |
| [RFC 9174](https://www.rfc-editor.org/rfc/rfc9174) | DTN TCP Convergence-Layer Protocol Version 4 (TCPCLv4) |
| [RFC 7242](https://www.rfc-editor.org/rfc/rfc7242) | DTN TCP Convergence-Layer Protocol (v3, historical) |
| [RFC 5326](https://www.rfc-editor.org/rfc/rfc5326) | Licklider Transmission Protocol (LTP) — Specification |
| [RFC 4838](https://www.rfc-editor.org/rfc/rfc4838) | Delay-Tolerant Networking Architecture |
| draft-ietf-dtn-bibect | Bundle-in-Bundle Encapsulation (BIBE) — IETF Internet-Draft |
| draft-irtf-dtnrg-ipnd | DTN IP Neighbor Discovery (IPND) — IRTF Internet-Draft |
| draft-burleigh-dtnwg-dtka | Delay-Tolerant Key Administration (DTKA) — Internet-Draft |
| [CCSDS 727.0-B](https://ccsds.org/) | CCSDS File Delivery Protocol (CFDP) |
| [CCSDS 734.1-B-1](https://ccsds.org/) | Licklider Transmission Protocol (LTP) for CCSDS (§7 = Service Data Aggregation) |
| [CCSDS 734.2-O-1](https://ccsds.org/) | Bundle Protocol Specification, BPv7 (Orange Book) |
| [CCSDS 734.3-B-1](https://ccsds.org/) | Schedule-Aware Bundle Routing (SABR) — the CGR routing standard |
| [CCSDS 734.6-O-1](https://ccsds.org/publications/orangebooks/entry/4868/) | Custody Transfer and Compressed Bundle Status Reporting (Orange Book, Issue 1, June 2026) |
| [CCSDS 735.1-B](https://ccsds.org/) | Asynchronous Message Service (AMS) |
| ION-specific (no ratified standard) | Behavior specific to the ION implementation; no ratified IETF/CCSDS standard |
| — (implementation) | Internal platform / infrastructure; not governed by a protocol standard |

> **Citation policy.** Section numbers are given only where confidently applicable. Where a test invokes CCSDS conventions the CCSDS document is noted alongside the IETF RFC. The `cbr-ct-orange-book/*` suite maps to **CCSDS 734.6-O-1** (*Custody Transfer and Compressed Bundle Status Reporting*, Issue 1, June 2026): §3.2 = Bundle Sequence Numbers/counters, §3.3 = Sequences & Collections (range arrays), §4.1 = Custody Transfer Extension Block (CTEB), §4.2 = Compressed Custody Signal (CCS), §4.3 = Custody Procedures, §5.1 = Compressed Reporting Extension Block (CREB), §5.2 = Compressed Reporting Signal (CRS).

_Generated from `tests/` and `demos/` descriptions; regenerate when tests are added or renamed._
