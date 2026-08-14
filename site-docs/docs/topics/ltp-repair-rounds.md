# Bounding LTP Session Repair

How long an LTP session may keep trying to repair a block, why that is now
something you configure directly, and what it replaced.

## The problem

LTP sends a block of data as many segments. Some get lost, so the receiver
reports gaps and the sender retransmits — a **repair round**. LTP won't repair
forever; each session gets a budget, and when it runs out the session is
cancelled and the whole block is thrown away (BP re-sends it later).

The old code didn't ask you how much repair to allow. It *derived* it: you told
ION the expected loss rate, and it ran a little statistical model that stopped
when "no more loss expected."

Two things were wrong with that:

1. **The stopping rule was arbitrary.** It stopped when the expected number of
   still-missing segments dropped below 1.0 — which actually means "accept up to
   a ~60% chance of needing another round." Not "no more loss expected."

2. **The result jumped around for no good reason.** Because the model produced a
   small whole number through repeated rounding, the budget stepped at arbitrary
   block sizes. Two nodes with *identical* configuration on *identical* links
   could differ **50×** in how often sessions got cancelled — purely because one
   happened to bundle data into 60-segment blocks and the other into 40.
   Measured: a 49-segment block got budget 2, a 51-segment block got budget 3.
   Nobody could see this, and turning the loss-rate knob often did nothing at all
   until it crossed a hidden threshold, then changed everything at once.

## The fix

**Stop deriving the budget. Just ask for it.**

New setting: `m maxrepairrounds` — how many repair rounds a session may use.
Default 8. Set it globally, or per span with
`m span <engine ID> maxrepairrounds`.

This works because a round is the thing that actually *costs* something: each one
is a full round-trip. So `rounds × round-trip-time` = worst-case time before LTP
gives up. That's a number an operator can reason about — 16 seconds on a
low-Earth-orbit link, ~5 hours at Mars distance.

**Why 8?** We fixed a design assumption: a space link losing more than **5%** of
segments is too broken to be worth repairing — better to give up and let the
layer above re-send. At 5% loss, 8 rounds leaves less than a 3-in-a-million
chance of failure even for a 100 MiB block.

**Why rounds and not "number of repair messages"?** Those two grow for completely
different reasons. A big block needs *many* report messages just to list all its
gaps — a 100 MiB block might need ~950 reports, but only across ~11 rounds.
Capping reports would punish a large healthy transfer while barely restraining a
small failing one. Reports are cheap; rounds cost time. So reports are still
computed automatically from block size and loss rate — that's just arithmetic
about how many gaps fit in a message — while rounds are what you configure.

**The 50× swing is gone.** Budget is now flat across block sizes where it used to
step.

`m maxseglossrate` is unchanged and still needed: it is what sizes the reports
within each round.

## Related changes

- **A separate bug fix:** per-span settings were stored only in memory and
  silently vanished on restart. Now they're saved properly. This mattered more
  than it sounds — a safety timer that limits how long a stalled session can hang
  around was being silently switched off by every restart. Because settings now
  persist, `m span <engine ID> clearoverride` is how you remove one.
- **Visibility:** `l span` now shows the resolved settings and the resulting
  budget, so you can compare two nodes directly instead of guessing.
- **A diagnostic:** the two ends of a link each compute their own budget and never
  negotiate, so a session can die because of the *other* node's configuration
  while yours looks fine. There's now a log message that says so, instead of
  leaving you staring at an unexplained cancellation.

## Upgrading

The LTP database layout changed. Start a **new database** after upgrading —
`ionstop` then `ionstart`. An `ionrestart` is *not* sufficient: it rebuilds the
working state from the existing database rather than creating a new one. A node
with a file-backed SDR must also remove the SDR file, which `ionstop` does not
delete.

Both ends of a span should be upgraded together. A receiving engine allows itself
extra rounds so that the *sending* engine's setting governs, which means you only
have to configure each node for the traffic it sends — but that only works when
both ends implement the setting.

## The one-line version

Instead of asking "what's your loss rate?" and guessing how much repair effort
that deserves, ION now asks "how long may a session keep trying?" — which is both
the thing that costs money and the thing an operator can actually answer.

## See also

- [LTP Performance](LTP-Performance.md#bounding-session-repair) — the same
  parameter in tuning context, alongside throughput measurements.
- `ltprc(5)` — the `m maxrepairrounds`, `m span … maxrepairrounds` and
  `m span … clearoverride` command reference.
