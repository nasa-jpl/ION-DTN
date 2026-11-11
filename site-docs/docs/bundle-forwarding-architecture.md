# Bundle Forwarding Architecture: Routing vs. the Forwarding Pipeline

## Overview

A common source of confusion in ION is the relationship between **routing** (deciding
where a bundle should go) and the **forwarding pipeline** that actually transmits it. They
are frequently conflated — "CGR sends the bundle" — but in ION they are distinct layers
with a deliberately narrow interface between them. Understanding that interface clarifies
how Contact Graph Routing (CGR), egress plans, rate control, and the convergence layer fit
together, and it explains why routing can be reasoned about — or replaced — largely
independently of everything beneath it.

The single most important idea in this document:

> **Routing in ION is next-hop selection.** A router's job is to choose the *proximate
> node* — the neighbor a bundle should be handed to next. Everything below that decision
> (which egress plan, which duct, at what rate, over which convergence-layer protocol) is a
> separate pipeline that operates the same way regardless of how the next hop was chosen.

## The forwarding pipeline

When a bundle is ready to be forwarded, it flows through these stages:

```
   scheme forward queue
        │
        ▼
   ┌─────────────────────────────┐
   │  ROUTING: choose next hop    │   ← CGR (or an override, static route, etc.)
   │  "send toward neighbor N"    │     produces a single proximate node number
   └──────────────┬──────────────┘
                  │  next-hop node N
                  ▼
   ┌─────────────────────────────┐
   │  EGRESS PLAN                 │   findPlan("ipn:N.0") → the plan for neighbor N
   │  priority queues (bulk/std/  │   bpEnqueue(): bundle joins one of three queues
   │  urgent), one per neighbor   │
   └──────────────┬──────────────┘
                  │
                  ▼
   ┌─────────────────────────────┐
   │  RATE CONTROL                │   per-neighbor Throttle, replenished each second
   │  (bpclm meters outflow)      │   from the contact plan's transmission rate
   └──────────────┬──────────────┘
                  │
                  ▼
   ┌─────────────────────────────┐
   │  CONVERGENCE LAYER           │   LTP / TCP / STCP / … actually transmits
   └─────────────────────────────┘
```

The routing stage emits one thing the rest of the pipeline consumes: **a next-hop node
number.** The egress plan, priority queues, rate throttle, and convergence layer neither
know nor care whether that neighbor was chosen by CGR, by a per-bundle override, or by a
static route.

For the `ipn` scheme this is implemented in `ipnfw.c`'s `enqueueBundle()`, which tries a
cascade of routing sources and stops at the first that succeeds in enqueuing the bundle to
a neighbor's plan:

1. **Per-bundle override** — a rule keyed on `(dataLabel, destination, source)` that pins a
   specific neighbor or duct. This already *is* a per-bundle next-hop chooser.
2. **Contact Graph Routing (CGR)** — dynamic routing over the contact plan.
3. **Direct neighbor** — if the destination is itself an adjacent neighbor.
4. **Static routes ("exits")** — a designated intermediate node for a range of
   destinations.
5. **Limbo** — hold the bundle until a future contact makes a route available.

Every one of these produces the same result: the bundle is enqueued to some neighbor's
egress plan. That uniformity is the seam between routing and the pipeline.

## "A route" vs. "the next hop"

CGR computes a **route**: an ordered path of scheduled contacts from the local node to the
destination, with a projected arrival time and a delivery confidence. It is natural to
assume the whole route drives forwarding. It does not.

**The bundle is enqueued using only the first hop of the route** — the immediate neighbor.
The remainder of the path is *not* used to steer transmission; each downstream node makes
its own independent next-hop decision when the bundle arrives. ION routing is therefore
fundamentally **hop-by-hop**: like IP, each node chooses only the next step, re-deciding
with locally current information rather than committing to a source-computed path.

So what is the rest of the route for? After CGR selects a route and the bundle is enqueued
to the first-hop neighbor, ION uses the full path for three *auxiliary* purposes — none of
which change which neighbor the bundle is handed to:

1. **End-to-end delivery confidence.** The per-contact confidence values along the path are
   multiplied into an estimated probability of delivery. If that estimate is too low, ION
   forwards additional copies along other routes for reliability.
2. **Transmission-volume reservation (MTV).** ION decrements each contact's residual
   *mission traffic volume* along the path, so that a single node does not commit more
   bytes to a future contact than that contact can carry. This is admission control, and it
   is inherently a whole-path operation.
3. **Proactive fragmentation.** If the route's tightest contact cannot carry the whole
   bundle, ION fragments it to fit.

These are all artifacts of **scheduled** DTN operation, where future contacts and their
capacities are known in advance. They enrich the forwarding decision but are separable from
it: the routing decision itself is still just "hand the bundle to neighbor N."

## Why rate control is not part of routing

A frequent misconception is that CGR controls transmission rate because both CGR and rate
control read the contact plan. They do — but through different data and for different
purposes, and they are independent.

The contact plan's transmission rate (`xmitRate`, in bytes/second) does **not** reach the
convergence layer through routes. It reaches it through **neighbor state** that the
`rfxclock` daemon maintains on a timeline of contact start/stop events:

```
   contact plan ──► rfxclock (timeline events) ──► IonNeighbor.{xmitRate, fireRate, owlt}
                                                         │
                                        ┌────────────────┴─────────────────┐
                                        ▼                                  ▼
                        LTP (ltpclock) reads it →           BP (bpclock) reads it →
                        LtpVspan transmission rate          per-neighbor Throttle → bpclm metering
```

CGR reads contacts to build *routes*; `rfxclock` reads the same contacts to maintain
*current neighbor rates*, which the convergence layers consume for pacing. These are two
independent consumers of one data source. The routing decision selects *which* neighbor;
the rate machinery, driven entirely by `rfxclock`, governs *how fast* transmission to that
neighbor proceeds. Neither depends on the other's output.

The mechanics of the rate side — the timeline events, clock-error compensation, and how LTP
in particular derives its send rate and screens/purges segments — are documented in detail
in [Contact Graph Events and LTP Interaction](./contact-graph-events-and-ltp.md). This page
is concerned only with establishing that rate control sits *below* the routing seam and is
independent of it.

## What happens under backpressure

Because rate control is open-loop from the contact plan above,
the pipeline is not slowed when a link runs slower than its planned rate.
The response depends on why the duct stops draining.

### Network (TCP) backpressure

The only signal given by a congested or slow link
is the convergence layer's own blocking socket write.
When the kernel send buffer fills,
the TCPCL sending thread is stalled inside `send()`
and bundles are no longer dequeued from the outduct.
That stall is not propagated upward immediately.
New bundles continue to be metered into the outduct
by `bpclm` at the contact-plan rate,
and injecting applications are given no signal to slow down.
Bundles accumulate in the outduct's queue in the SDR.

### Relay (inbound) backpressure

When a node cannot forward as fast as it receives,
its inbound ZCO heap is built up.
For a reliable induct (LTP or TCP),
acknowledgment of data is stopped by that buildup,
and the upstream sender is paced down.
A UDP-based induct has no such feedback,
so excess traffic is dropped on arrival.

### Stopping a duct or blocking a plan

Operator-driven stops are handled differently from network stalls:

*   When an outduct is stopped, its sending thread is ended,
    so bundles are no longer dequeued,
    but bundles already queued to that duct
    are retained in its queue until the duct is restarted.

*   When an egress plan is blocked,
    the neighbor is treated by the BP agent as if it did not exist.
    Bundles queued for that neighbor are reverse-enqueued back to the agent
    and are either re-routed over another path
    or held in [limbo](./topics/bundle-forwarding-disposition.md)
    until a future contact or route appears.

### The ZCO space-admission backstop

Creating a new payload requests zero-copy-object (ZCO) space.
Once the applicable ceiling is reached ---
the outbound heap account or the overall heap, whichever is hit first ---
the request is blocked on its attendant semaphore and is not completed,
so data can no longer be injected by the application.
This is the second, higher level of flow control,
and the SDR is protected from overflow.
The block is not propagated all the way up from the duct.
It is engaged only once storage is genuinely threatened,
which bounds a sustained buildup from any of the cases above.

## Why the layering matters

Because the interface between routing and the pipeline is a single next-hop node number,
several things follow:

- **Routing behavior can be understood in isolation.** To reason about where bundles go,
  you only need to reason about which neighbor each node selects — not about queues, ducts,
  throttles, or convergence-layer protocols.
- **Rate control keeps working regardless of the routing source.** Overrides, static
  routes, and CGR all feed the same pipeline; the `rfxclock`-driven throttle applies
  identically to whichever neighbor is chosen.
- **Alternative routers are feasible without disturbing the rest of ION.** Any mechanism
  that can answer "which neighbor next?" — a per-bundle override, a static exit, a
  third-party route computer such as Unibo-CGR, or an externally-developed policy router —
  plugs in at the routing stage and reuses the entire pipeline beneath it unchanged. The
  narrowness of the seam is what makes the routing layer replaceable.

## Summary

- ION forwarding is layered: **routing (next-hop selection)** sits above an **egress-plan →
  priority-queue → rate-control → convergence-layer** pipeline.
- The interface between the two is a single **next-hop node number**. The pipeline is
  agnostic to how that neighbor was chosen.
- CGR computes full routes, but **forwarding enqueues on the first hop only**; ION routing
  is hop-by-hop. The remainder of a route feeds auxiliary scheduled-DTN functions
  (delivery confidence, volume reservation, proactive fragmentation), not the choice of
  neighbor.
- **Rate control is not part of routing.** Transmission rate flows from the contact plan
  through `rfxclock` into per-neighbor state that the convergence layers consume, entirely
  independently of route computation.

## See also

- [Contact Graph Events and LTP Interaction](./contact-graph-events-and-ltp.md) — the rate
  side: timeline events, clock-error compensation, and LTP rate derivation.
- [ION Design and API Overview](./design-and-api-overview.md) — ION's modular
  architecture and the daemons involved.
- [CLA API](./cla-api.md) — the convergence-layer interface at the bottom of the pipeline.
- [Bundle Forwarding Disposition](./topics/bundle-forwarding-disposition.md) ---
  the limbo and abandonment lifecycle
  that backpressure and plan blocking feed into.
