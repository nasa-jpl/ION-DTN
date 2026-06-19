# Bundle Forwarding Disposition: Prospect, Limbo, and Abandonment

## Overview

When ION's `ipn`-scheme forwarder (`ipnfw`) cannot immediately enqueue a
bundle to a next hop, it must decide between two very different fates:

- **Limbo** — hold the bundle and wait, in the hope that a route becomes
  usable before the bundle's lifetime (TTL) runs out.
- **Abandonment** — discard the bundle right now, because there is no
  realistic chance of ever delivering it in time.

These show up in the [watch characters](../ION-Watch-Characters.md) as
`j` (placed in limbo), `k` (released from limbo for re-forwarding), and
`~` (abandoned on attempt to forward). A common point of confusion is
why a bundle to an unreachable node is sometimes abandoned (`a~a~a~`)
rather than parked in limbo until its TTL expires (`j … !`). The answer
is the **prospect** test.

This page explains what a "prospect" is, the exact criteria that route a
bundle to limbo versus abandonment, and why merely *naming* a
destination in the contact plan is **not** enough to keep its bundles
alive.

## The forwarding decision

For each bundle, `enqueueBundle()` in `bpv7/ipn/ipnfw.c` tries, in order:

1. **CGR (dynamic routing)** — if the destination ("terminus") node is
   in a region the local node also belongs to, consult the contact plan
   to compute a route and enqueue to the first hop.
2. **Direct neighbor** — if the terminus is a directly reachable
   neighbor (egress plan), enqueue for direct transmission.
3. **Static route ("exit")** — forward via the configured "via" endpoint
   for the narrowest node-number range that contains the terminus.

If none of these enqueues the bundle, the forwarder reaches the
limbo-versus-abandon decision:

```c
if (cgr_prospect(fqnn, bundle->expirationTime) > 0 || hasCustody)
{
        /* There is hope, or we hold custody: keep the bundle. */
        enqueueToLimbo(bundle, bundleObj);     /* watch: j */
}
...
return bpAbandon(bundleObj, bundle, BP_REASON_NO_ROUTE);   /* watch: ~ */
```

So a bundle is parked in limbo only when **either**:

- `cgr_prospect()` reports that a usable route might still materialize, **or**
- the bundle carries custody (an Orange Book CTEB), in which case the
  source must retain it regardless.

Otherwise it is abandoned immediately with reason `BP_REASON_NO_ROUTE`.

## What "prospect" means

`cgr_prospect()` (in `bpv7/cgr/libcgr.c`) answers a single question:

> *Is there at least one computed route to this terminus node that has
> not yet expired and that could still deliver the bundle before its TTL
> deadline?*

CGR stores the routes it computes for a terminus node in that node's
`routingObject->selectedRoutes`. Each route carries:

- **`arrivalTime`** — the projected time the bundle would *arrive at the
  terminus* via that route (accumulated contact start times plus one-way
  light time along the path).
- **`toTime`** — when the route *stops being viable* (the earliest end
  time among the contacts that make up the path).

`cgr_prospect()` returns **1** (a prospect exists → limbo) only if every
one of these is true:

1. The terminus is a known node in the contact database.
2. It has a routing object (CGR has run for it).
3. Its `selectedRoutes` list is non-empty.
4. At least one route passes **both** gates:
   - **not expired**: `toTime > currentTime` (expired routes are pruned
     and skipped), and
   - **time-feasible**: `arrivalTime <= deadline`, where `deadline` is
     the bundle's `expirationTime`.

If no route clears both gates, `cgr_prospect()` returns **0** → the
bundle is abandoned.

### Why a route can exist yet not be used "right now"

If a route is good enough to be a prospect, why didn't step 1 (CGR)
already enqueue the bundle on it? Because `tryCGR()` enqueues only when
the route's **first hop is usable at this instant** — the first contact
is open now and the neighbor has capacity. A route whose first contact
starts in the *future* is valid and time-feasible but not yet
*actionable*. That is precisely the "hope" that limbo exists for: the
bundle waits until the contact opens, at which point a contact-start
event releases it from limbo (`k`) and it is re-forwarded.

So, stated plainly:

> **Prospect = a non-expired, time-feasible route exists, but it cannot
> be acted on yet** (typically its first-hop contact has not started).

### A common misconception, corrected

It is tempting to think "if a route exists but is too slow to beat the
TTL, there's still hope, so the bundle goes to limbo." The code does the
opposite. A route whose projected `arrivalTime` is *after* the deadline
is explicitly **rejected** as "not a plausible route" and skipped. Only
routes that could arrive *in time* count as prospects. A route that is
both unusable now *and* too slow offers no hope, so the bundle is
abandoned.

## Being "mentioned" in the contact plan is not enough

A destination appearing somewhere in the contact graph does **not**
guarantee its bundles will be held in limbo. Each of the following puts a
named destination straight onto the abandonment path (`~`):

1. **Terminus region unknown to the local node.** CGR is consulted only
   when the terminus shares a region with the local node. A terminus in
   an unknown or foreign region skips CGR entirely, so no routes are
   built and `cgr_prospect()` returns 0. (ION retains the region data
   model, but inter-regional/cross-region routing via passageways is not
   implemented; cross-region destinations must be reachable through a
   directly configured plan.)
2. **CGR found no route.** The node is known and CGR ran, but the contact
   graph is disconnected from the local node in the usable time window,
   so `selectedRoutes` is empty. *Mentioned ≠ reachable.*
3. **All routes already expired.** Every path toward the node is built
   from contacts whose `toTime` is already in the past. Such routes are
   pruned, leaving nothing — a node described only by *elapsed* contacts
   is abandoned.
4. **All routes too slow for the TTL.** Routes exist and are not expired,
   but every one would arrive *after* the bundle's deadline (e.g. the
   soonest contact toward the node starts further out than the bundle's
   lifetime, or the one-way light time pushes arrival past TTL).

In other words, a "chance" is not created by naming a node in a contact.
A chance requires a CGR-computed route that is **non-expired** *and*
**fast enough to beat the bundle's TTL**. Everything else is discarded up
front, because parking a doomed bundle in limbo would only consume
storage until it expired anyway.

## Worked example: `bping` to an unreachable node

`bping` sends echo-request bundles with a default lifetime of **3600
seconds** (`-t` to change it). Ping a node for which the local contact
plan has no usable route and you will see, per ping:

```
(…)a~      a: queued for forwarding;  ~: abandoned (no route)
```

This is correct behavior, not a defect. ION has no link-layer "host
unreachable" signal; it simply finds no prospect and discards each
request, while `bping` keeps issuing pings until its count/duration
elapses. No echo reply ever returns.

To instead observe the limbo lifecycle (`j … !`) you need to make the
destination a genuine prospect: add a contact (and range) toward that
node that *starts within the bundle's TTL window*, so CGR builds a
time-feasible route whose first hop is not yet open. For example, in
`ionrc`:

```
a contact +0 +7200 <localNode> <destNode> 100000
a range   +0 +7200 <localNode> <destNode> 1
```

With a prospect present, each `bping` bundle goes to limbo (`j`) instead
of being abandoned. If a real receiver never appears, the bundle rides
limbo until its lifetime expires and is then destroyed (`!`).

> **Note on the limbo lifecycle.** A bundle in limbo normally shows a
> single `j` and then waits; it is not a continuous `jkjk…` stream. The
> `k` (release from limbo) fires only on a triggering event — a contact
> starting, or a blocked outduct/plan becoming unblocked. A steady
> `jkjk` pattern therefore implies something is repeatedly releasing and
> re-failing the bundle (for example a plan flapping between blocked and
> unblocked), not an idle wait.

## See also

- [ION Watch Characters](../ION-Watch-Characters.md) — the meaning of
  `a`, `b`, `j`, `k`, `~`, and `!`.
- [Contact Graph Events and LTP Interaction](../Contact-Graph-Events-and-LTP.md)
  — how contact timing drives forwarding events.
- [ION Design & Operation Manual](../ION-Guide.md) — the broader route
  computation and limbo description.
