# Atomic Operations

ION performs atomic read-modify-write operations in several hot paths:
BP and LTP tally counters,
inter-process reference counting on POSIX named semaphores,
daemon shutdown flags (`rtp.running`, `terminating`, `done`),
and sequence numbers used for cache invalidation
in the global semaphore table.
All of this goes through a single portable abstraction
in **`ici/include/ion_atomic.h`**.
Each source file that uses atomic types or macros
must `#include "ion_atomic.h"` directly —
it is not pulled in by `platform.h` or any other header.
The header is installed as a public header by `make install`,
and its tier-selection macro (`ION_HAVE_C11_ATOMICS`) is exported
via `pkg-config --cflags ion`
so external consumers automatically get the correct atomics configuration.
Application code, library code, and new daemons must not include `<stdatomic.h>`
or call `__atomic_*` / `__sync_*` built-ins directly —
use the ION wrappers described below.

> ⚠️ **Testing fallback tiers on a modern toolchain requires two levers, not one.**
> Defining `-DION_TEST_FORCE_FALLBACK` and/or `-DION_TEST_FORCE_SYNC_FALLBACK`
> alone only flips tier dispatch inside the header —
> the compiler stays in C18 mode
> (because `configure.ac`'s `-std=iso9899:2018` probe still succeeds
> and is baked into `AM_CFLAGS`).
> To exercise a genuine **C99 + `__atomic`** or **C99 + `__sync`** build —
> the compile environment a pre-GCC-4.7 flight toolchain would actually see —
> you must ALSO short-circuit the C18 probe with `ac_cv_c11=no`
> AND pass `-std=c99` in `CFLAGS`.
> Without both levers,
> the `_Atomic` keyword, `<stdatomic.h>` visibility,
> feature-test macro defaults, and library-header switches
> all differ from the C99 environment you think you are testing.
> Recipes are in
> **[Testing the Fallback Tiers](#testing-the-fallback-tiers)** below;
> a CI workflow that passes only the force macros is validating tier dispatch,
> not C99 language compatibility.

## Dual-Zone Architecture

`ion_atomic.h` defines **two opaque atomic types**,
distinguished by where the variable lives in memory:

| Type | Use for | Backing on C11 | Backing on C99 fallback |
|------|---------|----------------|-------------------------|
| `ion_atomic_t` | Process-local memory: heap, stack, static, `.bss` | Padded union over `_Atomic(vast)` | Padded union over `{ pthread_mutex_t; uvast }` |
| `ion_ipc_atomic_t` | Shared memory (SDR, SM global semaphore table, any `mmap`ed region crossing process boundaries) | `_Atomic(vast)` | `volatile vast` with compiler built-ins (see below) |

**The zone distinction is mandatory.**
`ion_atomic_t` embeds a POSIX mutex on the C99 fallback path.
A process-local mutex dropped into shared memory
causes deadlocks or segfaults when a second process touches it,
because `PTHREAD_PROCESS_PRIVATE` mutexes cannot be shared across address spaces.
`ion_ipc_atomic_t` is therefore strictly lock-free and async-signal-safe —
it never holds a mutex under any compilation path.

A common mistake is to declare a stats counter in `BpVdb`
(which is already in shared memory via the working-memory partition)
using `ion_atomic_t`.
**BpVdb lives in SM, so its atomic fields must be `ion_ipc_atomic_t`.**
When in doubt, ask:
"could a different process than the one that wrote this field ever read it?"
If yes, it's Zone 2.

## Three-Tier Fallback Chain

Both zones select their backing implementation at compile time
through feature macros:

```
+-------------------------------------------------------------+
| Tier 1: C11 stdatomic                                       |
|   when: __STDC_VERSION__ >= 201112L && !__STDC_NO_ATOMICS__ |
|   native: _Atomic(T), atomic_*_explicit, memory_order_*     |
+-------------------------------------------------------------+
               |  (fallback when C11 atomics unavailable)
               v
+-------------------------------------------------------------+
| Tier 2: GCC/Clang __atomic built-ins                        |
|   when: __clang__ || GCC >= 4.7                             |
|   uses: __atomic_load_n, __atomic_store_n, __atomic_*,      |
|         __ATOMIC_RELAXED ordering                           |
+-------------------------------------------------------------+
               |  (fallback when __atomic not available)
               v
+-------------------------------------------------------------+
| Tier 3: Legacy __sync built-ins                             |
|   when: pre-GCC-4.7 / non-GCC without __atomic              |
|   uses: __sync_fetch_and_add / _sub  (get, inc, dec),       |
|         __sync_val_compare_and_swap  (set, exchange via     |
|                                       CAS loop)             |
|         always full memory barrier (e.g., DMB ISH on ARM)   |
+-------------------------------------------------------------+
```

Tier 3 deliberately does **not** use `__sync_lock_test_and_set`
for `set` / `exchange`.
GCC documents that builtin as
(a) restricted on some targets to storing only the immediate constant 1, and
(b) an acquire-only barrier rather than a full barrier —
both of which would silently corrupt real callers that pass arbitrary values
(TallyDelta drains call `exchange(p, 0)`,
semaphore-flag resets call `set(p, 0)`).
The CAS loop over `__sync_val_compare_and_swap` accepts arbitrary values,
carries full-barrier semantics,
and is supported by every toolchain generation
that ships the rest of the `__sync_*` family.

Zone 1's fallback tiers are slightly different:
Zone 1 goes C11 → `pthread_mutex_t`-based
(no intermediate `__atomic` tier),
because the padded-union layout differs between C11 and the mutex fallback
and must match ABI across the process.
Zone 2 uses all three tiers;
its typedef is `volatile vast` for both Tier 2 and Tier 3,
so layouts are binary-compatible between the two fallback tiers.

### Consolidated Tier Summary

| Tier | Zone 1 (process-local) | Zone 2 (shared memory / IPC) | When selected |
|------|------------------------|------------------------------|---------------|
| C11 | `_Atomic(vast)` via `<stdatomic.h>` | `_Atomic(vast)` via `<stdatomic.h>` | C18/C11 compiler with `<stdatomic.h>` support |
| `__atomic` | `pthread_mutex_t`-backed union | `__atomic_*` built-ins (`__ATOMIC_RELAXED`) | C99 mode + GCC ≥ 4.7 or any Clang |
| `__sync` | `pthread_mutex_t`-backed union | `__sync_*` built-ins (full memory barrier) | C99 mode + pre-GCC-4.7 (e.g., RAD750, LEON) |

Modern Clang (all versions) and GCC ≥ 4.7 support all three tiers.
On these compilers `./configure` will auto-detect C18/`<stdatomic.h>`
and select Tier 1 (C11) —
you will not hit the `__sync` path unless you explicitly force it.
See **Testing the Fallback Tiers** below
for how to override the auto-detection.

**Why Tier 2 matters:** `__sync_*` built-ins are always sequentially-consistent
and emit full memory barriers.
On ARM/AArch64, `__sync_fetch_and_add(p, 0)`
(which `ion_ipc_atomic_get` falls back to in Tier 3)
compiles to a full LDAXR/STLXR + DMB ISH sequence —
a read-modify-write with a full barrier,
just to read a value.
`__atomic_load_n` with `__ATOMIC_RELAXED` compiles to a plain LDR
with no barrier.
For flight software deploying to ARM-based computers
(e.g., NVIDIA Orin, Xilinx Zynq, Ampere, Cortex-A),
Tier 2 avoids a significant per-op cost on every stats counter increment,
semaphore reference count operation, and daemon polling flag read.
Tier 3 is retained only for pre-GCC-4.7 toolchains on legacy flight processors
such as RAD750 and LEON3/4.

## Memory Ordering

All ION atomic operations currently use **relaxed** ordering
(`memory_order_relaxed` on C11; `__ATOMIC_RELAXED` on `__atomic`).
Relaxed ordering is correct for the current uses:

- **Stats counters** (BP/LTP tally deltas) — only the final sum matters;
  the readers aggregate over time and don't care about inter-counter ordering.
- **Reference counts** — the surrounding semaphore operations
  themselves provide the acquire/release synchronization needed
  to make the refcount update visible to other users of the protected resource.
- **Sequence numbers** (`gseq`) — monotonically increasing;
  readers only care that they see a later value than before.

**Shutdown flags are a borderline case.**
Flags like `rtp.running`, `gsem->ended`, and `gsem->pendingDelete`
are one-shot signals where a reader needs to observe all prior writes
by the setter.
Relaxed ordering is technically insufficient for these;
acquire/release would be more correct.
ION currently gets away with relaxed because
(a) readers poll in tight loops and will eventually observe the flag
on a later iteration, and
(b) the writes being ordered before the flag (e.g., SDR updates)
are themselves guarded by other synchronization
(SDR transaction locks, semaphore operations).
If you add a new polling flag that depends on observing prior non-atomic writes,
consult with the maintainers before defaulting to relaxed.

## False Sharing and Cache-Line Alignment

**False sharing** occurs when two logically independent atomic variables
live on the same 64-byte cache line
and are written by different CPUs concurrently:
even though the operations are non-overlapping,
each write forces the other CPU to invalidate and reload the line,
producing ping-pong cache traffic that can dominate the runtime
of an otherwise cheap atomic update.

False sharing is **only** a concern for *hot arrays of atomics* —
arrays where multiple threads simultaneously write to adjacent elements.
It is **not** a concern for:

- **Scattered single-field atomics inside larger structs**
  (daemon shutdown flags, per-SAP state, reference counters).
  The enclosing struct's other fields share the cache line anyway
  and are written holistically;
  padding just the atomic field is illusory protection.
- **File-scope static atomics** (initialization guards, shutdown signals).
  Their neighbors in `.bss` are typically unrelated read-mostly globals,
  not hot atomics.
- **Stack-local atomics**.
  Each thread has its own stack; no sharing possible.

The one ION pattern that *does* match "hot array of atomics"
is the BP/LTP tally delta arrays
(`BpVdb.{source,recv,discard,xmit,db}Deltas`, `VPlan.statsDeltas`,
`VInduct.statsDeltas`, `VEndpoint.statsDeltas`, `LtpVspan.statsDeltas`).
These hold adjacent counters that can be written concurrently
by different priority levels or different CLAs.
They are currently 16 bytes per pair (`ion_ipc_atomic_t` × 2),
so four pairs fit in a single cache line —
adjacent priorities share a line and can false-share under contention.
This is a deliberate memory-vs-contention trade-off:
ION prioritizes the SM-partition footprint on flight targets
over the marginal cache-line bouncing
that occurs under heavy concurrent tally updates,
which in practice is bounded by the bundle send/receive rate.

**Do not preemptively pad all atomics to 64 bytes.**
Every `ion_atomic_t` already pays one of two costs
(native `_Atomic(vast)` on C11 = 8 bytes,
mutex-backed union on C99 fallback = 64 bytes).
Padding scattered single fields provides no measurable benefit,
wastes memory universally,
and obscures the cases where padding actually matters.

**Do add `alignas(64)` surgically if a new hot array of atomics is introduced**
and profiling shows cache-line bouncing.
The idiom for per-element padding is:

```c
typedef struct {
    alignas(64) TallyDelta entry;
} PaddedTallyDelta;

/* Hot array — each element on its own cache line. */
PaddedTallyDelta sourceDeltas[3];
```

Use this surgically,
at the specific declaration where false sharing has been *measured*,
not preemptively across the codebase.
C11 and C++11 both support `alignas`,
and C++17 adds `std::hardware_destructive_interference_size`
for the hardware-specific cache-line size
(typically 64 bytes on x86_64 and most ARM64 cores).

### FIXME — Known candidates for future false-sharing optimization

These declarations are *plausible* false-sharing candidates
but have **not** been profiled under representative load.
Do not apply `alignas(64)` to them
until a benchmark on the target platform shows a measurable win.
Record the before/after numbers in the commit message
so the trade-off is auditable.

- **`TallyDelta` array elements in general** (`ici/include/ion.h`).
  Every struct that embeds `TallyDelta[]` is a candidate:
  `BpVdb.{source,recv,discard,xmit,db}Deltas`, `VPlan.statsDeltas`,
  `VInduct.statsDeltas`, `VEndpoint.statsDeltas`, `LtpVspan.statsDeltas`.
  A single `alignas(64)` on the `TallyDelta` type
  would address all of them simultaneously
  at the cost of ~5.8 KB extra SM-partition footprint for a typical node
  (120 TallyDeltas × 48 bytes of padding).
  This is the simplest and most comprehensive fix
  if the benchmark justifies it.

- **`LtpVspan.statsDeltas[LTP_SPAN_STATS]`** specifically
  is the strongest candidate among the list above.
  LTP spans are touched by up to five distinct threads per span
  (`ltpcli` receiver, `ltpclo` sender, `ltpmeter`, `ltpdeliv`, `ltpclock`),
  each writing a different `idx` value
  (receive vs transmit vs session-complete vs delivery).
  Adjacent elements are genuinely hit by different CPUs
  under realistic load.
  If budget constraints prevent padding *all* TallyDeltas,
  pad this one first.

- **`BpVdb.{source,recv,xmit}Deltas[3]`** (per-priority tally deltas)
  are a secondary candidate.
  In practice each priority tends to have one dominant writer
  (a specific application, a specific CLI, a specific CLO),
  so concurrent writes to adjacent priorities are less common
  than the LTP span case.

How to profile:

1. Pin two threads to different physical cores
   (`taskset -c 0,1` on Linux, or platform equivalent).
2. Have each thread call the target tally function in a tight loop
   (e.g., `bpSourceTally(i, 100)` with different `i` values)
   for a fixed wall-clock interval.
3. Measure total operations per second,
   compared against a padded variant built with `alignas(64)`
   applied to the struct under test.
4. Report the delta.
   A ≥2× difference on the hot path is a clear signal to pad;
   anything less is within noise and not worth the memory cost.

## API Summary

All six operations are defined for both zones.
The signatures are identical except for the type name:

```c
/* Zone 1 — process-local */
void  ion_atomic_init              (ion_atomic_t *p, vast v);
void  ion_atomic_set               (ion_atomic_t *p, vast v);
uvast ion_atomic_get               (ion_atomic_t *p);
uvast ion_atomic_get_and_increment (ion_atomic_t *p, vast d);
uvast ion_atomic_get_and_decrement (ion_atomic_t *p, vast d);
uvast ion_atomic_exchange          (ion_atomic_t *p, vast v);
void  ion_atomic_mutex_destroy     (ion_atomic_t *p);  /* no-op on C11 */

/* Zone 2 — shared memory */
#define ion_ipc_atomic_init(p,v)                /* ... */
#define ion_ipc_atomic_set(p,v)                 /* ... */
#define ion_ipc_atomic_get(p)                   /* ... */
#define ion_ipc_atomic_get_and_increment(p,d)   /* ... */
#define ion_ipc_atomic_get_and_decrement(p,d)   /* ... */
#define ion_ipc_atomic_exchange(p,v)            /* ... */
```

Notes on usage:

- **Always call `ion_atomic_init` before first use**
  of a Zone 1 atomic.
  On C11 this just stores the value;
  on the C99 mutex fallback it also calls `pthread_mutex_init`.
  Forgetting to initialize will cause an uninitialized-mutex fault
  on the fallback path that won't show up on C11 builds.
- **Never `memset` or `Zalloc` a struct containing an `ion_atomic_t`**
  and then use it without re-initializing.
  Zero-filling destroys the hidden mutex state on the C99 fallback.
  This is why `ResourceLock->initialized` in `platform_sm.c`
  is a plain `int` guarded by a meta-lock
  instead of an `ion_atomic_t`.
- **`ion_ipc_atomic_t` fields in shared memory
  must be initialized in the setup path that creates the shared region**,
  not in whatever code happens to see them first.
  See `_sembase` in `platform_sm.c` for the pattern:
  every `SmGlobalSem` in `gsemtable[]`
  has its atomics initialized in a single loop
  during `_sembase(IPC_ACTION_LOOKUP)`
  when the region is first created.
- **The return value of increment/decrement
  is the `vast` value *before* the operation**
  (`fetch_add` / `fetch_sub` semantics, not `add_fetch`).

## C++ Consumers

External C++ programs that link against the ION C library
are a supported use case.
Typical consumers include mission applications,
test harnesses,
and higher-level frameworks
that treat ION as a bundle-delivery service.
ION's public headers
(`platform.h`, `ion.h`, `bp.h`, `ltp.h`, `cfdp.h`, `ams.h`, …)
all carry `extern "C"` guards
so that function names, structs, and typedefs
are directly usable from a C++ translation unit.
Private implementation headers whose names end in `P.h`
(e.g., `bpP.h`, `ltpP.h`, `libbpP.c`'s private declarations)
are internal to their respective libraries
and are not part of the public API —
C++ consumers should never include them.

### The C++ compilation path in `ion_atomic.h`

Because C11 `_Atomic(T)`
and the GCC/Clang `__atomic_*` / `__sync_*` built-ins
are not valid C++,
`ion_atomic.h` selects a dedicated branch
when `__cplusplus` is defined.
Both atomic types are exposed as opaque,
size- and alignment-matched byte blobs:

```c
typedef struct {
    alignas(alignof(long long)) unsigned char opaque[64];
} ion_atomic_t;

typedef struct {
    alignas(alignof(long long)) unsigned char opaque[sizeof(long long)];
} ion_ipc_atomic_t;
```

The sizes (64 bytes / 8 bytes)
and 8-byte alignment match every C compilation tier
(C11 native, C99 + `__atomic`, C99 + `__sync`,
and the C99 mutex-backed Zone 1 fallback).
This ensures binary compatibility:
any ION struct that internally embeds `ion_atomic_t`
or `ion_ipc_atomic_t`
has a byte-identical layout
between C and C++ compilations,
so opaque pointers returned by the ION API
remain valid across the C/C++ boundary
even though the C++ consumer never sees the struct definitions.

C++ code does not receive the `ion_atomic_*` / `ion_ipc_atomic_*`
accessor macros or inline functions;
those are C-only.
The `ION_ATOMIC_INIT` macro is still defined
(expanding to `{{0}}`)
so static initializers in headers parse cleanly under C++,
but C++ code should not be creating `ion_atomic_t` instances itself —
initialization happens inside the C library via `ion_atomic_init()`.

### Rules for C++ consumers

1. **Never read or write `ion_atomic_t` / `ion_ipc_atomic_t` fields
   directly from C++.**
   The byte blob is deliberately opaque.
   There is no supported way to reinterpret it as `std::atomic<T>`:
   on the C99 mutex-backed Zone 1 path
   the blob actually contains a `pthread_mutex_t` plus a value,
   and on some targets `alignof(_Atomic long long)` in C
   may be stricter than `alignof(long long)` in C++,
   which would cause silent memory corruption under a `reinterpret_cast`.
   Always route atomic reads/writes through the ION C API —
   either by calling library functions
   that return pre-computed counter snapshots,
   or by calling C helper wrappers you provide in a small `.c` shim.

2. **Your own C++ atomics are completely independent of ION's atomics.**
   You can freely declare `std::atomic<T>` fields
   in your own C++ data structures
   and use them alongside ION.
   They occupy separate memory,
   use the C++ `<atomic>` implementation,
   and have no interaction with `ion_atomic_t` or `ion_ipc_atomic_t`:

    ```cpp
    #include <atomic>
    #include "bp.h"

    std::atomic<std::uint64_t> appBundleCount{0};  // your atomic
    Object                     newBundle;           // ION type

    void on_bundle_sent() {
        appBundleCount.fetch_add(1, std::memory_order_relaxed);
        // call ION C API...
    }
    ```

3. **Minimum C++ standard: C++11.**
   The opaque types use `alignas(alignof(long long))`,
   which requires C++11.
   C++11 is also the minimum for `std::atomic<T>`, `std::thread`,
   and other standard-library atomics/concurrency features
   you are likely to want on the C++ side anyway.

4. **Struct layout compatibility assumes
   both C and C++ compilations target the same architecture and ABI.**
   On 64-bit Linux, AArch64, and Windows,
   `long long` is 8 bytes aligned to 8 bytes in both C and C++,
   so `ion_atomic_t` is 64 bytes with 8-byte alignment
   and `ion_ipc_atomic_t` is 8 bytes with 8-byte alignment in both worlds.
   On exotic 32-bit ABIs where `alignof(_Atomic long long)`
   is stricter than `alignof(long long)`,
   a more defensive alignment may be needed — audit before porting.

### ION library compilation tier is independent of your C++ standard

The ION C library is compiled separately from your C++ program.
It may be built under C11/C18 or fall back to C99
depending on toolchain, platform, and `configure` options.
This compilation tier is independent
of the C++ standard you use for your own code —
a C++14 program can link against a C11 ION build
or a C99-fallback ION build with no source changes,
because the opaque types exposed to C++
are identical under every C tier.

That said, **prefer a C11/C18 ION build
whenever the target toolchain supports it**,
for reasons unrelated to C++:

- **Zone 1 performance.**
  The C11 path uses lock-free `_Atoaic(vast)`
  and compiles BP/LTP tally updates to a single atomic instruction.
  The C99 mutex fallback acquires and releases a `pthread_mutex_t`
  on every update,
  which is measurably slower under heavy bundle load.
- **Zone 2 performance on ARM / AArch64.**
  The C11 and C99-`__atomic` paths both use
  `memory_order_relaxed` / `__ATOMIC_RELAXED`,
  which compile to plain `LDR`/`STR` and `LDADD` instructions.
  The legacy `__sync` fallback wraps every operation
  in `DMB ISH` full barriers,
  an order of magnitude more expensive per op.

The C99 fallback path exists for legacy flight toolchains
(RAD750, LEON pre-GCC-4.7, older RTEMS/VxWorks)
where C11 atomics are unavailable.
It is not a recommended default for ground or Linux-class targets.

### Minimal C++ example

```cpp
// main.cpp — external C++ consumer of ION
#include <cstdio>
#include <atomic>
#include "bp.h"

static std::atomic<std::uint64_t> sentCount{0};

int main() {
    if (bp_attach() < 0) {
        std::fprintf(stderr, "bp_attach failed\n");
        return 1;
    }

    BpSAP sap = nullptr;
    if (bp_open((char*)"ipn:1.1", &sap) < 0) {
        std::fprintf(stderr, "bp_open failed\n");
        return 1;
    }

    // ... bundle send loop ...
    sentCount.fetch_add(1, std::memory_order_relaxed);

    bp_close(sap);
    bp_detach();
    return 0;
}
```

Build with:

```sh
g++ -std=c++11 -I/usr/local/include main.cpp -o app -lbp -lici -lm -lpthread
```

The `-lbp -lici` libraries may have been compiled
with either C11 or C99 fallback — either works.
Your C++ program's own `std::atomic<>` usage
is unaffected by ION's internal atomic implementation.

## Testing the Fallback Tiers

Three independent levers control the atomics path,
and the recipes below keep them distinct:

1. **Integrator directive** (highest priority) —
   define exactly one of `ION_ATOMIC_C11`,
   `ION_ATOMIC_BUILTIN`, or `ION_ATOMIC_SYNC`
   to command which tier the header compiles.
   Also reachable via `./configure --with-atomics={c11,builtin,sync}`.
   Overrides both auto-detection
   and the `ION_TEST_FORCE_*` test knobs,
   so this is the cleanest lever for pinning a build
   (or a single test binary) to a specific tier.
2. **Language standard** (in `configure.ac:64-92`) —
   a C18 probe runs first, falling back to C99.
   Pass `ac_cv_c11=no` on the `./configure` line
   to short-circuit the C18 probe and compile under `-std=c99`;
   this also flips the `HAVE_C11_ATOMICS` Automake conditional
   at `configure.ac:97`,
   causing `ici/library/ion_atomic.c`
   (the Zone 1 mutex-backed fallback TU) to be compiled.
3. **Atomics tier dispatch test knobs** —
   preprocessor macros that sit on top
   of whatever language mode configure picked
   (only consulted when no integrator directive from lever 1 is set):

```
-DION_TEST_FORCE_FALLBACK          # undefines ION_HAVE_C11_ATOMICS → Tier 2 (__atomic) or Tier 3 (__sync)
-DION_TEST_FORCE_SYNC_FALLBACK     # undefines ION_HAVE_GNU_ATOMIC → forces Tier 3 (__sync); pair WITH the above
```

Recipes for `./configure`:

```sh
# Exercise Tier 2 (__atomic) on a modern compiler — language standard remains C18
CFLAGS="-DION_TEST_FORCE_FALLBACK" ./configure

# Exercise Tier 3 (__sync) on a modern compiler — language standard remains C18
CFLAGS="-DION_TEST_FORCE_FALLBACK -DION_TEST_FORCE_SYNC_FALLBACK" ./configure

# Genuine C99-mode build; Tier 2 auto-selected on GCC ≥ 4.7 / any Clang
./configure ac_cv_c11=no CFLAGS="-std=c99"

# Genuine C99-mode build forced to Tier 3 (what a pre-GCC-4.7 flight toolchain would see)
./configure ac_cv_c11=no CFLAGS="-std=c99 -DION_TEST_FORCE_SYNC_FALLBACK"
```

The first two recipes only override tier dispatch —
the compiler stays in C18 mode,
so `ici/library/ion_atomic.c` is *not* compiled
(the `HAVE_C11_ATOMICS` Automake conditional is still true).
The last two recipes flip both levers
and exercise the full C99 path end-to-end.
Without any flag, `./configure` auto-detects C18/`<stdatomic.h>`
and selects the native C11 path (Tier 1)
on modern Clang and GCC (≥ 4.7).

Validation harness: `tests/atomics/dotest` builds
`tests/atomics/test_ipc_atomics.c` three times —
once each with `-DION_ATOMIC_C11`,
`-DION_ATOMIC_BUILTIN`, and `-DION_ATOMIC_SYNC` —
and runs every binary.
Each variant exercises `ion_ipc_atomic_t`
under 8 concurrent threads × 300 000 operations per thread
(gseq / refCount lock-free correctness)
and additionally asserts that `set` and `exchange`
preserve arbitrary non-1 values,
guarding against any future regression
to a builtin with the `__sync_lock_test_and_set` operand restriction.
The test source contains `#error` self-checks
so a misconfigured build cannot silently dispatch
to the wrong tier.
Add `-fsanitize=thread` to the C11 and `__atomic` variants
for TSan coverage when the host toolchain supports it.

## Files

| File | Purpose |
|------|---------|
| `ici/include/ion_atomic.h` | Zone definitions, tier dispatch, macros and inline wrappers |
| `ici/library/ion_atomic.c` | Zone 1 C99 mutex-fallback implementations (compiled only when `!ION_HAVE_C11_ATOMICS`) |
| `tests/atomics/test_ipc_atomics.c` | Validation harness for Zone 2 (concurrency + set/exchange value preservation) |
| `tests/atomics/dotest` | Per-tier build matrix driver (C11 / `__atomic` / `__sync`); entry point for `runtests` |

