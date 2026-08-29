# Interplanetary Overlay Network (ION)

**Interplanetary Overlay Network (ION)** is NASA/JPL's implementation of
Delay/Disruption-Tolerant Networking (DTN) — a store‑carry‑forward
network architecture designed to move data reliably across links that are
long‑delay, intermittently connected, and asymmetric, where the
end‑to‑end connectivity assumed by TCP/IP simply does not exist. ION
implements the DTN architecture of Internet RFC 4838 together with Bundle
Protocol version 7 (BPv7, RFC 9171) and version 6 (BPv6, RFC 5050), and
surrounds them with a full stack of transport protocols, security,
routing, and application services.

ION is engineered for **both flight and ground systems**: it runs within
a fixed memory budget, checkpoints protocol state and in‑transit data to
persistent storage so a node survives a reset, and ports across operating
systems from real‑time flight executives to Linux servers. It is a
flight‑proven, operational implementation — used as the baseline for
science‑instrument data handling on the International Space Station and in
gateway/relay roles for command, telemetry, and science downlink.

- Videos and presentation materials: [DTN / ION courses](https://www.nasa.gov/directorates/heo/scan/engineering/technology/disruption_tolerant_networking_software_options_ion).
- New to DTN or ION? Start with the [ION Guide](./design-operations-guide.md) and the [ION Design and API Overview](./design-and-api-overview.md), then run the [Quick Start Guide](./quick-start-guide.md).

---

## Why DTN, and why ION

Space and other challenged networks break the assumptions of the
terrestrial Internet: light‑time delays of seconds to minutes, links that
open and close on a schedule, one‑way or rate‑asymmetric channels, and
frequent disruption. DTN answers this with **bundles** — self‑contained
units of application data that each node stores and forwards when a
transmission opportunity arises, rather than requiring a continuous
end‑to‑end path. ION applies specific engineering principles to make this
work in the most constrained environments:

- **Bounded, self‑managed memory.** ION manages dynamic allocation inside
  a host‑specified budget via its own memory managers (PSM/SDR), avoiding
  unbounded heap growth on flight hardware.
- **Survivability over raw speed.** Protocol state and queued bundles are
  continuously checkpointed to the SDR (which can be backed by
  non‑volatile storage), so a node recovers queued data, custody
  commitments, and protocol state after an unplanned reset instead of
  losing everything in flight. See the
  [ION Design and API Overview](./design-and-api-overview.md) and
  [ION Memory Protection](./memory-protection.md).
- **Zero‑copy data path.** Bundle payloads move between processing stages
  as Zero‑Copy Objects (ZCO) in shared memory, minimizing copying on the
  critical path.
- **Modular daemons.** Each protocol and service runs as an independent,
  restartable daemon, so components can start, stop, and recover
  independently.
- **Portability and small footprint.** ION builds as a set of libraries
  suitable for static linking, and runs on Linux, RTEMS
  ([installation guide](./rtems6-installation.md)), Solaris, FreeBSD, and
  (best‑effort) macOS.

---

## Capabilities at a glance

ION is organized as layers: applications and DTN services sit on the
Bundle Protocol agent, which routes bundles and hands them to convergence‑
layer adapters that carry them over the underlying links.

```
   Applications  ·  CFDP file transfer  ·  AMS messaging  ·  BSS streaming  ·  DTPC
   ─────────────────────────────────────────────────────────────────────────────
                     Bundle Protocol agent  (BPv7 / BPv6)
        store‑carry‑forward · custody · fragmentation · status reports · CoS
        BPSec security  ·  extension blocks  ·  IPN multicast (IMC)
   ─────────────────────────────────────────────────────────────────────────────
        Routing:  Contact Graph Routing (CGR)  ·  static routes / overrides
                  ·  pluggable external routers
   ─────────────────────────────────────────────────────────────────────────────
        Convergence layers (CLAs):  LTP · TCPCLv4 (+TLS) · STCP · TCP · UDP
                                     · SPP · BRS · file
   ─────────────────────────────────────────────────────────────────────────────
        Contact plan & rate control (rfxclock)  ·  link service  ·  SDR / PSM / ZCO
```

### Bundle Protocol (BPv7 and BPv6)

The core of ION is a Bundle Protocol agent implementing **BPv7 (RFC 9171)**
and **BPv6 (RFC 5050)**: store‑carry‑forward relaying, custody transfer,
fragmentation and reassembly, bundle status reports, class‑of‑service and
priority (bulk / standard / expedited with ordinals), time‑to‑live
expiration, and an extensible block architecture. ION uses the **`ipn`
naming scheme** with fully‑qualified node numbers (FQNN, RFC 9758; see the
[IPN Naming Scheme Transition](./ipn-naming-scheme-transition.md)) as well
as the `dtn` scheme. Applications send and receive bundles through the
[BP Service API](./bp-service-api.md); custom processing can be added
through the [Extension Block Interface](./extension-block-interface.md).

### Routing

ION's default dynamic router is **Contact Graph Routing (CGR)**, which
computes routes over a *contact plan* — the schedule of future
transmission opportunities (contacts) and one‑way light times (ranges)
between nodes. The contact plan is also what drives transmission‑rate
control, maintained on a timeline by the `rfxclock` daemon (see
[Contact Graph Events and LTP Interaction](./contact-graph-events-and-ltp.md)).
Alongside CGR, ION supports **static routes** ("exits") and **per‑bundle
routing overrides**.

### Convergence‑layer adapters (transports)

Bundles are carried over links by convergence‑layer adapters. ION provides
a broad set:

- **LTP** — the Licklider Transmission Protocol (RFC 5326), ION's
  workhorse for reliable delivery over long‑delay, high‑error, one‑way
  space links, with contact‑plan‑driven rate control.
- **TCP, STCP, UDP, DCCP** — simpler IP‑based convergence layers.
- **BRS** — Bundle Relay Service, for reaching nodes behind firewalls/NAT.
- **SPP, EPP, and file** — the CCSDS Space Packet Protocol /
  Encapsulation Packet Protocol and a file‑based CLA. See the
  [SPP/EPP/File CLA description](./spp-epp-file-cla-description.md) and the
  [CLA API](./cla-api.md) for writing your own.

LTP itself can be adapted to arbitrary underlying links through a generic
link‑service template (see
[LTP Generic Link Service Template](./ltp-generic-link-service-template.md)).

### Security

ION implements **Bundle Protocol Security (BPSec, RFC 9172)** with the
default security contexts of RFC 9173 — the Block Integrity Block (BIB)
for integrity and the Block Confidentiality Block (BCB) for
confidentiality — configurable through a policy interface. See
[BpSec Configuration Examples](./bpsec-configuration-examples.md). ION can
also be built against the external **BPSec Library (BSL)** from NASA‑AMMOS
for an alternative security‑block implementation
([BSL Build & Install Guide](./bsl-build-install-guide.md)).

### Reliable status signaling and custody

The **Compressed Bundle Status Reporting and Custody Transfer (CBR/CT)**
feature implements the CCSDS "Orange Book" conventions for efficient,
compact status signaling and reliable hop‑by‑hop custody transfer — useful
where full BPv7 status reports would be too costly. See the
[CBR/CT Tutorial](./cbr-ct-tutorial.md).

### DTN application services

On top of the Bundle Protocol, ION ships complete application services:

- **CFDP** — a DTN‑based implementation of the CCSDS File Delivery
  Protocol for reliable file transfer across DTN paths
  ([CFDP Software Architecture](./cfdp-sw-architecture.md)).
- **AMS** — the Asynchronous Message Service, a publish/subscribe and
  request/reply messaging system for distributed applications
  ([AMS Programmer's Guide](./ams-programmer-guide.md)).
- **BSS** — the Bundle Streaming Service, for real‑time and playback
  streaming of continuous data (e.g. video) over DTN.
- **DTPC** — Delay‑Tolerant Payload Conditioning, providing
  application‑level end‑to‑end services (aggregation, ordering,
  retransmission) above BP.
- **IMC** — IPN Multicast, for one‑to‑many bundle delivery.

To build your own DTN applications, see
[Writing Applications for ION](./writing-applications-for-ion.md) and the
[ION Application Service Interface](./application-services-api.md).

---

## Application domains

- **Deep‑space and planetary missions** — command/control of and data
  return from robotic explorers across interplanetary distances.
- **Satellite constellations and spacecraft fleets** — coordinated,
  resource‑efficient communication among many spacecraft.
- **Space‑station and platform data handling** — reliable transfer between
  ground and onboard science payloads and instruments.
- **The emerging Interplanetary Internet** — an internetwork that extends
  beyond Earth across multiple bodies and relays.
- **Terrestrial challenged networks** — tactical, mobile, sensor, and
  disaster‑response networks with intermittent connectivity.

See [Use Cases](./use-cases.md) for worked scenarios.

---

## Getting started

1. **Get the source code.**
   - *Release archive (no git required):* on the
     [ION‑DTN releases page](https://github.com/nasa-jpl/ION-DTN/releases),
     pick a release, expand "Assets", and download the ZIP or tar.gz.
   - *Clone the repository:*
     ```bash
     git clone https://github.com/nasa-jpl/ION-DTN.git
     ```
2. **Build, install, and run a two‑node example** — follow the
   [Quick Start Guide](./quick-start-guide.md).
3. **Learn the configuration files** — the
   [Basic Configuration File Tutorial](./basic-configuration-file-tutorial.md)
   and a set of ready‑to‑use
   [configuration templates](./config-file-templates.md).
4. **Configuration tooling** — the
   [Configuration Tools Overview](./configuration-tools-overview.md) and
   [Configure Multiple Network Interfaces](./configure-multiple-network-interfaces.md).

A pre‑built DTN Development/Deployment Kit — an Ubuntu virtual‑machine
image preconfigured with ION and demonstration network scenarios — is also
available (currently being updated for BPv7).

---

## Documentation map

**Learn and configure**

- [ION Guide](./design-operations-guide.md) — the comprehensive manual.
- [ION Design and API Overview](./design-and-api-overview.md) — architecture and the full API set.
- [ION Tutorials](./tutorials.md) · [Quick Start Guide](./quick-start-guide.md) · [Basic Configuration File Tutorial](./basic-configuration-file-tutorial.md) · [Config File Templates](./config-file-templates.md).

**Protocols and features**

- [Bundle Forwarding Architecture](./bundle-forwarding-architecture.md) — routing internals.
- [Contact Graph Events and LTP Interaction](./contact-graph-events-and-ltp.md) — the contact plan and rate control.
- [CBR/CT Tutorial](./cbr-ct-tutorial.md) — compressed status reporting and custody transfer.
- [BpSec Configuration Examples](./bpsec-configuration-examples.md) · [BSL Build & Install Guide](./bsl-build-install-guide.md) — security.
- [CFDP Software Architecture](./cfdp-sw-architecture.md) · [AMS Programmer's Guide](./ams-programmer-guide.md) — file transfer and messaging services.
- [CLA API](./cla-api.md) · [SPP/EPP/File CLA](./spp-epp-file-cla-description.md) · [LTP Generic Link Service Template](./ltp-generic-link-service-template.md) — convergence layers.

**Build applications and extend ION**

- [Writing Applications for ION](./writing-applications-for-ion.md) · [ION Application Service Interface](./application-services-api.md) · [BP Service API](./bp-service-api.md).
- [Public Administration API Guide](./public-admin-api-guide.md) — configure ION, LTP, and BP from C.
- [ICI API](./ici-api.md) · [Extension Block Interface](./extension-block-interface.md) · [ION Coding Guide](./coding-guide.md).

**Operate**

- [ION Utilities](./utilities.md) · [ION Launcher](./ion-launcher.md) · [ionrun](./ionrun.md).
- [ION Monitoring Guide](./monitoring-guide.md) · [ION Watch Characters](./watch-characters.md).
- [ION Shutdown Guide](./shutdown-guide.md) · [ION Memory Protection](./memory-protection.md) · [SOP for ION](./standard-operating-procedure.md).
- [ION Deployment Guide](./deployment-guide.md) · [Known Issues](./known-issues.md).

---

## Performance

- **BP/LTP throughput study** — performance of BP/LTP across CPU/OS
  architectures is covered in the
  [ION Deployment Guide](./deployment-guide.md).
- **ION TCPCL throughput assessment** — an Ohio University study of ION
  over the TCP convergence layer is available
  [online](https://etd.ohiolink.edu/acprod/odb_etd/etd/r/1501/10?clear=10&p10_accession_num=ohiou1619115602389023)
  and as a
  [PDF](https://etd.ohiolink.edu/acprod/odb_etd/ws/send_file/send?accession=ohiou1619115602389023&disposition=inline).

---

## Papers, license, and community

- **Publications:** see the [List of Papers](./list-of-papers.md) for key
  DTN and ION references.
- **License:** ION is released under the MIT License; see the
  [LICENSE](./license.md).
- **Contributing:** see the [ION Coding Guide](./coding-guide.md) for
  conventions when submitting changes.
