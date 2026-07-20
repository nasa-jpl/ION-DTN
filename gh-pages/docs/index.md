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
- New to DTN or ION? Start with the [ION Guide](./ION-Guide.md) and the [ION Design and API Overview](./ION-Design-and-API-Overview.md), then run the [Quick Start Guide](./ION-Quick-Start-Guide.md).

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
  [ION Design and API Overview](./ION-Design-and-API-Overview.md) and
  [ION Memory Protection](./ION-Memory-Protection.md).
- **Zero‑copy data path.** Bundle payloads move between processing stages
  as Zero‑Copy Objects (ZCO) in shared memory, minimizing copying on the
  critical path.
- **Modular daemons.** Each protocol and service runs as an independent,
  restartable daemon, so components can start, stop, and recover
  independently.
- **Portability and small footprint.** ION builds as a set of libraries
  suitable for static linking, and runs on Linux, RTEMS
  ([installation guide](./RTEMS6-Installation.md)), Solaris, FreeBSD, and
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
                                     · BSSP · SPP · BRS · file
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
[IPN Naming Scheme Transition](./IPN-Naming-Scheme-Transition.md)) as well
as the `dtn` scheme. Applications send and receive bundles through the
[BP Service API](./BP-Service-API.md); custom processing can be added
through the [Extension Block Interface](./Extension-Block-Interface.md).

### Routing

ION's default dynamic router is **Contact Graph Routing (CGR)**, which
computes routes over a *contact plan* — the schedule of future
transmission opportunities (contacts) and one‑way light times (ranges)
between nodes. The contact plan is also what drives transmission‑rate
control, maintained on a timeline by the `rfxclock` daemon (see
[Contact Graph Events and LTP Interaction](./Contact-Graph-Events-and-LTP.md)).
Alongside CGR, ION supports **static routes** ("exits") and **per‑bundle
routing overrides**. Routing is also **pluggable**: an externally‑developed
router can be dropped in as a shared object to make the per‑bundle next‑hop
decision in place of CGR, with an optional API for reading the contact
graph — see [Bundle Forwarding Architecture](./Bundle-Forwarding-Architecture.md)
and the [Writing a Custom Router](./Writing-a-Custom-Router.md) tutorial.

### Convergence‑layer adapters (transports)

Bundles are carried over links by convergence‑layer adapters. ION provides
a broad set:

- **LTP** — the Licklider Transmission Protocol (RFC 5326), ION's
  workhorse for reliable delivery over long‑delay, high‑error, one‑way
  space links, with contact‑plan‑driven rate control.
- **TCPCLv4** — the TCP Convergence Layer version 4 (RFC 9174), with
  optional **TLS 1.3** encryption, for reliable delivery over terrestrial/
  IP links. See the [TCPCLv4 Guide](./TCPCLv4-Guide.md).
- **TCP, STCP, UDP, DCCP** — simpler IP‑based convergence layers.
- **BRS** — Bundle Relay Service, for reaching nodes behind firewalls/NAT.
- **BSSP, SPP, EPP, and file** — the Bundle Streaming Service protocol,
  CCSDS Space Packet Protocol / Encapsulation Packet Protocol, and a
  file‑based CLA. See the
  [SPP/EPP/File CLA description](./SPP-EPP-File-CLA-Description.md) and the
  [CLA API](./CLA-API.md) for writing your own.

LTP itself can be adapted to arbitrary underlying links through a generic
link‑service template (see
[LTP Generic Link Service Template](./LTP-Generic-Link-Service-Template.md)).

### Security

ION implements **Bundle Protocol Security (BPSec, RFC 9172)** with the
default security contexts of RFC 9173 — the Block Integrity Block (BIB)
for integrity and the Block Confidentiality Block (BCB) for
confidentiality — configurable through a policy interface. See
[BpSec Configuration Examples](./BpSec-Configuration-Examples.md). ION can
also be built against the external **BPSec Library (BSL)** from NASA‑AMMOS
for an alternative security‑block implementation
([BSL Build & Install Guide](./BSL-Build-Install-Guide.md)).

### Reliable status signaling and custody

The **Compressed Bundle Status Reporting and Custody Transfer (CBR/CT)**
feature implements the CCSDS "Orange Book" conventions for efficient,
compact status signaling and reliable hop‑by‑hop custody transfer — useful
where full BPv7 status reports would be too costly. See the
[CBR/CT Tutorial](./CBR-CT-Tutorial.md).

### DTN application services

On top of the Bundle Protocol, ION ships complete application services:

- **CFDP** — a DTN‑based implementation of the CCSDS File Delivery
  Protocol for reliable file transfer across DTN paths
  ([CFDP Software Architecture](./CFDP-SW-Architecture.md)).
- **AMS** — the Asynchronous Message Service, a publish/subscribe and
  request/reply messaging system for distributed applications
  ([AMS Programmer's Guide](./AMS-Programmer-Guide.md)).
- **BSS** — the Bundle Streaming Service, for real‑time and playback
  streaming of continuous data (e.g. video) over DTN.
- **DTPC** — Delay‑Tolerant Payload Conditioning, providing
  application‑level end‑to‑end services (aggregation, ordering,
  retransmission) above BP.
- **IMC** — IPN Multicast, for one‑to‑many bundle delivery.

To build your own DTN applications, see
[Writing Applications for ION](./Writing-Applications-for-ION.md) and the
[ION Application Service Interface](./ION-Application-Service-Interface.md).

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

See [Use Cases](./Use-Cases.md) for worked scenarios.

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
   [Quick Start Guide](./ION-Quick-Start-Guide.md).
3. **Learn the configuration files** — the
   [Basic Configuration File Tutorial](./Basic-Configuration-File-Tutorial.md)
   and a set of ready‑to‑use
   [configuration templates](./ION-Config-File-Templates.md).
4. **Configuration tooling** — the
   [Configuration Tools Overview](./Configuration-Tools-Overview.md) and
   [Configure Multiple Network Interfaces](./Configure-Multiple-Network-Interfaces.md).

A pre‑built DTN Development/Deployment Kit — an Ubuntu virtual‑machine
image preconfigured with ION and demonstration network scenarios — is also
available (currently being updated for BPv7).

---

## Documentation map

**Learn and configure**

- [ION Guide](./ION-Guide.md) — the comprehensive manual.
- [ION Design and API Overview](./ION-Design-and-API-Overview.md) — architecture and the full API set.
- [ION Tutorials](./ION-Tutorials.md) · [Quick Start Guide](./ION-Quick-Start-Guide.md) · [Basic Configuration File Tutorial](./Basic-Configuration-File-Tutorial.md) · [Config File Templates](./ION-Config-File-Templates.md).

**Protocols and features**

- [TCPCLv4 Guide](./TCPCLv4-Guide.md) — the RFC 9174 TCP convergence layer with TLS 1.3.
- [Bundle Forwarding Architecture](./Bundle-Forwarding-Architecture.md) · [Writing a Custom Router](./Writing-a-Custom-Router.md) — routing internals and pluggable routers.
- [Contact Graph Events and LTP Interaction](./Contact-Graph-Events-and-LTP.md) — the contact plan and rate control.
- [CBR/CT Tutorial](./CBR-CT-Tutorial.md) — compressed status reporting and custody transfer.
- [BpSec Configuration Examples](./BpSec-Configuration-Examples.md) · [BSL Build & Install Guide](./BSL-Build-Install-Guide.md) — security.
- [CFDP Software Architecture](./CFDP-SW-Architecture.md) · [AMS Programmer's Guide](./AMS-Programmer-Guide.md) — file transfer and messaging services.
- [CLA API](./CLA-API.md) · [SPP/EPP/File CLA](./SPP-EPP-File-CLA-Description.md) · [LTP Generic Link Service Template](./LTP-Generic-Link-Service-Template.md) — convergence layers.

**Build applications and extend ION**

- [Writing Applications for ION](./Writing-Applications-for-ION.md) · [ION Application Service Interface](./ION-Application-Service-Interface.md) · [BP Service API](./BP-Service-API.md).
- [Public Administration API Guide](./Public-Admin-API-Guide.md) — configure ION, LTP, and BP from C.
- [ICI API](./ICI-API.md) · [Extension Block Interface](./Extension-Block-Interface.md) · [ION Coding Guide](./ION-Coding-Guide.md).

**Operate**

- [ION Utilities](./ION-Utilities.md) · [ION Launcher](./ION-Launcher.md) · [ionrun](./ionrun.md).
- [ION Monitoring Guide](./ION-Monitoring-Guide.md) · [ION Watch Characters](./ION-Watch-Characters.md).
- [ION Shutdown Guide](./ION-Shutdown-Guide.md) · [ION Memory Protection](./ION-Memory-Protection.md) · [SOP for ION](./SOP-for-ION.md).
- [ION Deployment Guide](./ION-Deployment-Guide.md) · [Known Issues](./Known-Issues.md).

---

## Performance

- **BP/LTP throughput study** — performance of BP/LTP across CPU/OS
  architectures is covered in the
  [ION Deployment Guide](./ION-Deployment-Guide.md).
- **ION TCPCL throughput assessment** — an Ohio University study of ION
  over the TCP convergence layer is available
  [online](https://etd.ohiolink.edu/acprod/odb_etd/etd/r/1501/10?clear=10&p10_accession_num=ohiou1619115602389023)
  and as a
  [PDF](https://etd.ohiolink.edu/acprod/odb_etd/ws/send_file/send?accession=ohiou1619115602389023&disposition=inline).

---

## Papers, license, and community

- **Publications:** see the [List of Papers](./List-of-Papers.md) for key
  DTN and ION references.
- **License:** ION is released under the MIT License; see the
  [LICENSE](./License.md).
- **Contributing:** see the [ION Coding Guide](./ION-Coding-Guide.md) for
  conventions when submitting changes.
