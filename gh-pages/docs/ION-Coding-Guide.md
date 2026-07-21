# ION Coding Guide

- [ION Coding Guide](#ion-coding-guide)
  - [Preface](#preface)
  - [C Language Standard](#c-language-standard)
    - [C11/C18 Features in Use](#c11c18-features-in-use)
    - [C99 Features Used Throughout](#c99-features-used-throughout)
    - [Per-Component Overrides](#per-component-overrides)
    - [Guidelines for Contributors](#guidelines-for-contributors)
    - [Operating System Support Matrix for Space Processors](#operating-system-support-matrix-for-space-processors)
  - [Atomic Operations](#atomic-operations)
    - [Dual-Zone Architecture](#dual-zone-architecture)
    - [Three-Tier Fallback Chain](#three-tier-fallback-chain)
      - [Consolidated Tier Summary](#consolidated-tier-summary)
    - [Memory Ordering](#memory-ordering)
    - [False Sharing and Cache-Line Alignment](#false-sharing-and-cache-line-alignment)
      - [FIXME — Known candidates for future false-sharing optimization](#fixme--known-candidates-for-future-false-sharing-optimization)
    - [API Summary](#api-summary)
    - [C++ Consumers](#c-consumers)
      - [The C++ compilation path in `ion_atomic.h`](#the-c-compilation-path-in-ion_atomich)
      - [Rules for C++ consumers](#rules-for-c-consumers)
      - [ION library compilation tier is independent of your C++ standard](#ion-library-compilation-tier-is-independent-of-your-c-standard)
      - [Minimal C++ example](#minimal-c-example)
    - [Testing the Fallback Tiers](#testing-the-fallback-tiers)
    - [Files](#files)
  - [Application Behavior](#application-behavior)
  - [Function Design Guidelines](#function-design-guidelines)
  - [Error Checking](#error-checking)
    - [CHK Macro Behavior and Fail-Fast Mode](#chk-macro-behavior-and-fail-fast-mode)
  - [Error and Status Reporting](#error-and-status-reporting)
  - [‘C’ Coding Style](#c-coding-style)
    - [Naming Conventions](#naming-conventions)
    - [Indentation, Bracketing, Whitespace](#indentation-bracketing-whitespace)
    - [Comment Formatting](#comment-formatting)
    - [Miscellaneous Rules](#miscellaneous-rules)
  - [SDR Transaction](#sdr-transaction)
    - [Transaction Lock and Robust-Mutex Recovery](#transaction-lock-and-robust-mutex-recovery)
  - [Static Analysis Notes](#static-analysis-notes)
    - [Null Pointer Dereference — Shared Memory Lookups](#null-pointer-dereference--shared-memory-lookups)
    - [Null Pointer Dereference — Callback Parameters](#null-pointer-dereference--callback-parameters)
    - [File System Race Condition (TOCTOU)](#file-system-race-condition-toctou)
    - [Double Lock / Double Unlock — Session Teardown](#double-lock--double-unlock--session-teardown)
    - [Unreasonable Size Argument](#unreasonable-size-argument)
    - [Coercion Alters Value](#coercion-alters-value)
    - [Cast Alters Value](#cast-alters-value)
    - [Ignored Return Value](#ignored-return-value)
    - [Useless Assignment — `sm_TaskVar()` Pattern](#useless-assignment--sm_taskvar-pattern)
    - [Unreachable Data Flow / Unreachable Computation / Unreachable Call](#unreachable-data-flow--unreachable-computation--unreachable-call)
    - [Redundant Condition](#redundant-condition)
    - [Empty if Statement](#empty-if-statement)
    - [Dangerous Function Cast](#dangerous-function-cast)
    - [Tainted Buffer Access](#tainted-buffer-access)

## Preface

The following coding guidelines apply to all software delivered as part of the Interplanetary Overlay Network (ION) distribution, except:

- Where the delivered software is legacy code rather than code developed specifically for ION.
- Where conformance to some other standard is clearly appropriate. For example, when using a framework library like Motif it may be appropriate to modify these guidelines so as to be consistent with the practices of the framework.
- Where, in the judgment of the programmer, deviating from the guidelines in a particular case results in manifestly clearer code. This is not a license to ignore the guidelines; it is intended to cover special circumstances.
Adherence to these guidelines is the responsibility of the individual programmer but will be considered during peer reviews of new ION code.

## C Language Standard

ION targets **C18 (ISO 9899:2018)** as its primary standard, with an automatic fallback to **C99 (ISO 9899:1999)** for older toolchains. C17/C18 is a bugfix revision of C11 with no new language features, so in practice ION is a **C11/C18 codebase**.

The build system in `configure.ac` establishes a preference hierarchy:

1. **C18** (`-std=iso9899:2018`) — validated by checking if `<stdatomic.h>` compiles.
2. **C99** (`-std=c99`) — validated by checking if `<stdint.h>` compiles.
3. The build fails if neither is supported.

The `-pedantic` flag is enabled to enforce strict standards compliance.

### C11/C18 Features in Use

- **Atomic operations** — used for lock-free reference counting (semaphore management), lock-free statistics counters (BP and LTP tally deltas), daemon shutdown flags, and inter-process semaphore table state in shared memory. See the **Atomic Operations** section below for the portable abstraction, the dual-zone architecture, and the three-tier fallback chain.

### C99 Features Used Throughout

- `<stdint.h>`, `<stdbool.h>`
- Designated initializers
- `inline` functions
- Mixed declarations and code
- `//` single-line comments

### Per-Component Overrides

Some subdirectories pin to a specific standard in their own Makefiles:

- **QCBOR, Unity, libbloom** — pinned to `-std=c99`
- **contrib/bptap** — uses `-std=gnu99` (GCC extension of C99)

### Guidelines for Contributors

- Avoid GNU extensions and non-standard constructs in core ION code.
- Use standards-compliant macro helper names.
- When adding new code that needs atomics,
  use the ION-specific opaque types `ion_atomic_t` (process-local)
  or `ion_ipc_atomic_t` (shared memory)
  together with the `ion_atomic_*` / `ion_ipc_atomic_*` accessor macros.
  Do not include `<stdatomic.h>` directly; include `ion_atomic.h` instead.
  See the **Atomic Operations** section below for selection rules.
- Prefer C99-compatible constructs for all other code; this maximizes portability to the C99 fallback path.
- A basic [`AGENTS.md`](AGENTS.md) is provided for use with LLMs.
  Copy it into the main folder to use.
  PRs and issues will not be accepted for this.
  To use it with Claude Code, create a symlink to it named `CLAUDE.md`.

### Operating System Support Matrix for Space Processors

The following table summarizes C standard support across operating systems commonly used on space-qualified hardware. ION's C99 fallback path ensures compatibility with all of these environments.

| Operating System | Supported Standard | Hardware Targets | Key Space Features |
|---|---|---|---|
| VxWorks 6.x/7 | C99, C11, C17, C++17 | RAD750, RAD5545, ARM | Determinism, safety-certifiable, container support. |
| RTEMS 4/5/6 | C99, C11, C18, Ada | LEON, SPARC, PowerPC | Open-source, POSIX API, SMP support. |
| Linux (Yocto) | C11, C17, C23 | ARM, NVIDIA Orin, Xilinx | High-throughput, extensive libraries, Space 2.0. |
| Zephyr RTOS | C11 | LEON, ARM, RISC-V | Lightweight, growing aerospace community. |
| Bare-Metal (BCC) | C99 | LEON, SPARC | Minimal overhead for simple controllers. |

## Atomic Operations

ION performs atomic read-modify-write operations in several hot paths:
BP and LTP tally counters,
inter-process reference counting on POSIX named semaphores,
daemon shutdown flags (`rtp.running`, `terminating`, `done`),
and sequence numbers used for cache invalidation
in the global semaphore table.
All of this goes through a single portable abstraction
in **`ici/include/ion_atomic.h`**.
Each source file that uses atomic types or macros must `#include "ion_atomic.h"` directly — it is not pulled in by `platform.h` or any other header.
The header is installed as a public header by `make install`, and its tier-selection macro (`ION_HAVE_C11_ATOMICS`) is exported via `pkg-config --cflags ion` so external consumers automatically get the correct atomics configuration.
Application code, library code, and new daemons must not include `<stdatomic.h>`
or call `__atomic_*` / `__sync_*` built-ins directly —
use the ION wrappers described below.

> ⚠️ **Testing fallback tiers on a modern toolchain requires two levers, not one.**
> Defining `-DION_TEST_FORCE_FALLBACK` and/or `-DION_TEST_FORCE_SYNC_FALLBACK` alone only flips tier dispatch inside the header — the compiler stays in C18 mode (because `configure.ac`'s `-std=iso9899:2018` probe still succeeds and is baked into `AM_CFLAGS`). To exercise a genuine **C99 + `__atomic`** or **C99 + `__sync`** build — the compile environment a pre-GCC-4.7 flight toolchain would actually see — you must ALSO short-circuit the C18 probe with `ac_cv_c11=no` AND pass `-std=c99` in `CFLAGS`. Without both levers, the `_Atomic` keyword, `<stdatomic.h>` visibility, feature-test macro defaults, and library-header switches all differ from the C99 environment you think you are testing. Recipes are in **[Testing the Fallback Tiers](#testing-the-fallback-tiers)** below; a CI workflow that passes only the force macros is validating tier dispatch, not C99 language compatibility.

### Dual-Zone Architecture

`ion_atomic.h` defines **two opaque atomic types**, distinguished by where the variable lives in memory:

| Type | Use for | Backing on C11 | Backing on C99 fallback |
|------|---------|----------------|-------------------------|
| `ion_atomic_t` | Process-local memory: heap, stack, static, `.bss` | Padded union over `_Atomic(vast)` | Padded union over `{ pthread_mutex_t; uvast }` |
| `ion_ipc_atomic_t` | Shared memory (SDR, SM global semaphore table, any `mmap`ed region crossing process boundaries) | `_Atomic(vast)` | `volatile vast` with compiler built-ins (see below) |

**The zone distinction is mandatory.** `ion_atomic_t` embeds a POSIX mutex on the C99 fallback path. A process-local mutex dropped into shared memory causes deadlocks or segfaults when a second process touches it, because `PTHREAD_PROCESS_PRIVATE` mutexes cannot be shared across address spaces. `ion_ipc_atomic_t` is therefore strictly lock-free and async-signal-safe — it never holds a mutex under any compilation path.

A common mistake is to declare a stats counter in `BpVdb` (which is already in shared memory via the working-memory partition) using `ion_atomic_t`. **BpVdb lives in SM, so its atomic fields must be `ion_ipc_atomic_t`.** When in doubt, ask: "could a different process than the one that wrote this field ever read it?" If yes, it's Zone 2.

### Three-Tier Fallback Chain

Both zones select their backing implementation at compile time through feature macros:

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

Tier 3 deliberately does **not** use `__sync_lock_test_and_set` for `set` / `exchange`. GCC documents that builtin as (a) restricted on some targets to storing only the immediate constant 1, and (b) an acquire-only barrier rather than a full barrier — both of which would silently corrupt real callers that pass arbitrary values (TallyDelta drains call `exchange(p, 0)`, semaphore-flag resets call `set(p, 0)`). The CAS loop over `__sync_val_compare_and_swap` accepts arbitrary values, carries full-barrier semantics, and is supported by every toolchain generation that ships the rest of the `__sync_*` family.

Zone 1's fallback tiers are slightly different: Zone 1 goes C11 → `pthread_mutex_t`-based (no intermediate `__atomic` tier), because the padded-union layout differs between C11 and the mutex fallback and must match ABI across the process. Zone 2 uses all three tiers; its typedef is `volatile vast` for both Tier 2 and Tier 3, so layouts are binary-compatible between the two fallback tiers.

#### Consolidated Tier Summary

| Tier | Zone 1 (process-local) | Zone 2 (shared memory / IPC) | When selected |
|------|------------------------|------------------------------|---------------|
| C11 | `_Atomic(vast)` via `<stdatomic.h>` | `_Atomic(vast)` via `<stdatomic.h>` | C18/C11 compiler with `<stdatomic.h>` support |
| `__atomic` | `pthread_mutex_t`-backed union | `__atomic_*` built-ins (`__ATOMIC_RELAXED`) | C99 mode + GCC ≥ 4.7 or any Clang |
| `__sync` | `pthread_mutex_t`-backed union | `__sync_*` built-ins (full memory barrier) | C99 mode + pre-GCC-4.7 (e.g., RAD750, LEON) |

Modern Clang (all versions) and GCC ≥ 4.7 support all three tiers. On these compilers `./configure` will auto-detect C18/`<stdatomic.h>` and select Tier 1 (C11) — you will not hit the `__sync` path unless you explicitly force it. See **Testing the Fallback Tiers** below for how to override the auto-detection.

**Why Tier 2 matters:** `__sync_*` built-ins are always sequentially-consistent and emit full memory barriers. On ARM/AArch64, `__sync_fetch_and_add(p, 0)` (which `ion_ipc_atomic_get` falls back to in Tier 3) compiles to a full LDAXR/STLXR + DMB ISH sequence — a read-modify-write with a full barrier, just to read a value. `__atomic_load_n` with `__ATOMIC_RELAXED` compiles to a plain LDR with no barrier. For flight software deploying to ARM-based computers (e.g., NVIDIA Orin, Xilinx Zynq, Ampere, Cortex-A), Tier 2 avoids a significant per-op cost on every stats counter increment, semaphore reference count operation, and daemon polling flag read. Tier 3 is retained only for pre-GCC-4.7 toolchains on legacy flight processors such as RAD750 and LEON3/4.

### Memory Ordering

All ION atomic operations currently use **relaxed** ordering (`memory_order_relaxed` on C11; `__ATOMIC_RELAXED` on `__atomic`). Relaxed ordering is correct for the current uses:

- **Stats counters** (BP/LTP tally deltas) — only the final sum matters; the readers aggregate over time and don't care about inter-counter ordering.
- **Reference counts** — the surrounding semaphore operations themselves provide the acquire/release synchronization needed to make the refcount update visible to other users of the protected resource.
- **Sequence numbers** (`gseq`) — monotonically increasing; readers only care that they see a later value than before.

**Shutdown flags are a borderline case.** Flags like `rtp.running`, `gsem->ended`, and `gsem->pendingDelete` are one-shot signals where a reader needs to observe all prior writes by the setter. Relaxed ordering is technically insufficient for these; acquire/release would be more correct. ION currently gets away with relaxed because (a) readers poll in tight loops and will eventually observe the flag on a later iteration, and (b) the writes being ordered before the flag (e.g., SDR updates) are themselves guarded by other synchronization (SDR transaction locks, semaphore operations). If you add a new polling flag that depends on observing prior non-atomic writes, consult with the maintainers before defaulting to relaxed.

### False Sharing and Cache-Line Alignment

**False sharing** occurs when two logically independent atomic variables live on the same 64-byte cache line and are written by different CPUs concurrently: even though the operations are non-overlapping, each write forces the other CPU to invalidate and reload the line, producing ping-pong cache traffic that can dominate the runtime of an otherwise cheap atomic update.

False sharing is **only** a concern for *hot arrays of atomics* — arrays where multiple threads simultaneously write to adjacent elements. It is **not** a concern for:

- **Scattered single-field atomics inside larger structs** (daemon shutdown flags, per-SAP state, reference counters). The enclosing struct's other fields share the cache line anyway and are written holistically; padding just the atomic field is illusory protection.
- **File-scope static atomics** (initialization guards, shutdown signals). Their neighbors in `.bss` are typically unrelated read-mostly globals, not hot atomics.
- **Stack-local atomics**. Each thread has its own stack; no sharing possible.

The one ION pattern that *does* match "hot array of atomics" is the BP/LTP tally delta arrays (`BpVdb.{source,recv,discard,xmit,db}Deltas`, `VPlan.statsDeltas`, `VInduct.statsDeltas`, `VEndpoint.statsDeltas`, `LtpVspan.statsDeltas`). These hold adjacent counters that can be written concurrently by different priority levels or different CLAs. They are currently 16 bytes per pair (`ion_ipc_atomic_t` × 2), so four pairs fit in a single cache line — adjacent priorities share a line and can false-share under contention. This is a deliberate memory-vs-contention trade-off: ION prioritizes the SM-partition footprint on flight targets over the marginal cache-line bouncing that occurs under heavy concurrent tally updates, which in practice is bounded by the bundle send/receive rate.

**Do not preemptively pad all atomics to 64 bytes.** Every `ion_atomic_t` already pays one of two costs (native `_Atomic(vast)` on C11 = 8 bytes, mutex-backed union on C99 fallback = 64 bytes). Padding scattered single fields provides no measurable benefit, wastes memory universally, and obscures the cases where padding actually matters.

**Do add `alignas(64)` surgically if a new hot array of atomics is introduced** and profiling shows cache-line bouncing. The idiom for per-element padding is:

```c
typedef struct {
    alignas(64) TallyDelta entry;
} PaddedTallyDelta;

/* Hot array — each element on its own cache line. */
PaddedTallyDelta sourceDeltas[3];
```

Use this surgically, at the specific declaration where false sharing has been *measured*, not preemptively across the codebase. C11 and C++11 both support `alignas`, and C++17 adds `std::hardware_destructive_interference_size` for the hardware-specific cache-line size (typically 64 bytes on x86_64 and most ARM64 cores).

#### FIXME — Known candidates for future false-sharing optimization

These declarations are *plausible* false-sharing candidates but have **not** been profiled under representative load. Do not apply `alignas(64)` to them until a benchmark on the target platform shows a measurable win. Record the before/after numbers in the commit message so the trade-off is auditable.

- **`TallyDelta` array elements in general** (`ici/include/ion.h`). Every struct that embeds `TallyDelta[]` is a candidate: `BpVdb.{source,recv,discard,xmit,db}Deltas`, `VPlan.statsDeltas`, `VInduct.statsDeltas`, `VEndpoint.statsDeltas`, `LtpVspan.statsDeltas`. A single `alignas(64)` on the `TallyDelta` type would address all of them simultaneously at the cost of ~5.8 KB extra SM-partition footprint for a typical node (120 TallyDeltas × 48 bytes of padding). This is the simplest and most comprehensive fix if the benchmark justifies it.

- **`LtpVspan.statsDeltas[LTP_SPAN_STATS]`** specifically is the strongest candidate among the list above. LTP spans are touched by up to five distinct threads per span (`ltpcli` receiver, `ltpclo` sender, `ltpmeter`, `ltpdeliv`, `ltpclock`), each writing a different `idx` value (receive vs transmit vs session-complete vs delivery). Adjacent elements are genuinely hit by different CPUs under realistic load. If budget constraints prevent padding *all* TallyDeltas, pad this one first.

- **`BpVdb.{source,recv,xmit}Deltas[3]`** (per-priority tally deltas) are a secondary candidate. In practice each priority tends to have one dominant writer (a specific application, a specific CLI, a specific CLO), so concurrent writes to adjacent priorities are less common than the LTP span case.

How to profile:

1. Pin two threads to different physical cores (`taskset -c 0,1` on Linux, or platform equivalent).
2. Have each thread call the target tally function in a tight loop (e.g., `bpSourceTally(i, 100)` with different `i` values) for a fixed wall-clock interval.
3. Measure total operations per second, compared against a padded variant built with `alignas(64)` applied to the struct under test.
4. Report the delta. A ≥2× difference on the hot path is a clear signal to pad; anything less is within noise and not worth the memory cost.

### API Summary

All six operations are defined for both zones. The signatures are identical except for the type name:

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

- **Always call `ion_atomic_init` before first use** of a Zone 1 atomic. On C11 this just stores the value; on the C99 mutex fallback it also calls `pthread_mutex_init`. Forgetting to initialize will cause an uninitialized-mutex fault on the fallback path that won't show up on C11 builds.
- **Never `memset` or `Zalloc` a struct containing an `ion_atomic_t`** and then use it without re-initializing. Zero-filling destroys the hidden mutex state on the C99 fallback. This is why `ResourceLock->initialized` in `platform_sm.c` is a plain `int` guarded by a meta-lock instead of an `ion_atomic_t`.
- **`ion_ipc_atomic_t` fields in shared memory must be initialized in the setup path that creates the shared region**, not in whatever code happens to see them first. See `_sembase` in `platform_sm.c` for the pattern: every `SmGlobalSem` in `gsemtable[]` has its atomics initialized in a single loop during `_sembase(IPC_ACTION_LOOKUP)` when the region is first created.
- **The return value of increment/decrement is the `vast` value *before* the operation** (`fetch_add` / `fetch_sub` semantics, not `add_fetch`).

### C++ Consumers

External C++ programs that link against the ION C library are a supported use case. Typical consumers include mission applications, test harnesses, and higher-level frameworks that treat ION as a bundle-delivery service. ION's public headers (`platform.h`, `ion.h`, `bp.h`, `ltp.h`, `cfdp.h`, `ams.h`, …) all carry `extern "C"` guards so that function names, structs, and typedefs are directly usable from a C++ translation unit. Private implementation headers whose names end in `P.h` (e.g., `bpP.h`, `ltpP.h`, `libbpP.c`'s private declarations) are internal to their respective libraries and are not part of the public API — C++ consumers should never include them.

#### The C++ compilation path in `ion_atomic.h`

Because C11 `_Atomic(T)` and the GCC/Clang `__atomic_*` / `__sync_*` built-ins are not valid C++, `ion_atomic.h` selects a dedicated branch when `__cplusplus` is defined. Both atomic types are exposed as opaque, size- and alignment-matched byte blobs:

```c
typedef struct {
    alignas(alignof(long long)) unsigned char opaque[64];
} ion_atomic_t;

typedef struct {
    alignas(alignof(long long)) unsigned char opaque[sizeof(long long)];
} ion_ipc_atomic_t;
```

The sizes (64 bytes / 8 bytes) and 8-byte alignment match every C compilation tier (C11 native, C99 + `__atomic`, C99 + `__sync`, and the C99 mutex-backed Zone 1 fallback). This ensures binary compatibility: any ION struct that internally embeds `ion_atomic_t` or `ion_ipc_atomic_t` has a byte-identical layout between C and C++ compilations, so opaque pointers returned by the ION API remain valid across the C/C++ boundary even though the C++ consumer never sees the struct definitions.

C++ code does not receive the `ion_atomic_*` / `ion_ipc_atomic_*` accessor macros or inline functions; those are C-only. The `ION_ATOMIC_INIT` macro is still defined (expanding to `{{0}}`) so static initializers in headers parse cleanly under C++, but C++ code should not be creating `ion_atomic_t` instances itself — initialization happens inside the C library via `ion_atomic_init()`.

#### Rules for C++ consumers

1. **Never read or write `ion_atomic_t` / `ion_ipc_atomic_t` fields directly from C++.** The byte blob is deliberately opaque. There is no supported way to reinterpret it as `std::atomic<T>`: on the C99 mutex-backed Zone 1 path the blob actually contains a `pthread_mutex_t` plus a value, and on some targets `alignof(_Atomic long long)` in C may be stricter than `alignof(long long)` in C++, which would cause silent memory corruption under a `reinterpret_cast`. Always route atomic reads/writes through the ION C API — either by calling library functions that return pre-computed counter snapshots, or by calling C helper wrappers you provide in a small `.c` shim.

2. **Your own C++ atomics are completely independent of ION's atomics.** You can freely declare `std::atomic<T>` fields in your own C++ data structures and use them alongside ION. They occupy separate memory, use the C++ `<atomic>` implementation, and have no interaction with `ion_atomic_t` or `ion_ipc_atomic_t`:

    ```cpp
    #include <atomic>
    #include "bp.h"

    std::atomic<std::uint64_t> appBundleCount{0};  // your atomic
    SdrObject                  newBundle;          // ION type

    void on_bundle_sent() {
        appBundleCount.fetch_add(1, std::memory_order_relaxed);
        // call ION C API...
    }
    ```

3. **Minimum C++ standard: C++11.** The opaque types use `alignas(alignof(long long))`, which requires C++11. C++11 is also the minimum for `std::atomic<T>`, `std::thread`, and other standard-library atomics/concurrency features you are likely to want on the C++ side anyway.

4. **Struct layout compatibility assumes both C and C++ compilations target the same architecture and ABI.** On 64-bit Linux, AArch64, and Windows, `long long` is 8 bytes aligned to 8 bytes in both C and C++, so `ion_atomic_t` is 64 bytes with 8-byte alignment and `ion_ipc_atomic_t` is 8 bytes with 8-byte alignment in both worlds. On exotic 32-bit ABIs where `alignof(_Atomic long long)` is stricter than `alignof(long long)`, a more defensive alignment may be needed — audit before porting.

#### ION library compilation tier is independent of your C++ standard

The ION C library is compiled separately from your C++ program. It may be built under C11/C18 or fall back to C99 depending on toolchain, platform, and `configure` options. This compilation tier is independent of the C++ standard you use for your own code — a C++14 program can link against a C11 ION build or a C99-fallback ION build with no source changes, because the opaque types exposed to C++ are identical under every C tier.

That said, **prefer a C11/C18 ION build whenever the target toolchain supports it**, for reasons unrelated to C++:

- **Zone 1 performance.** The C11 path uses lock-free `_Atomic(vast)` and compiles BP/LTP tally updates to a single atomic instruction. The C99 mutex fallback acquires and releases a `pthread_mutex_t` on every update, which is measurably slower under heavy bundle load.
- **Zone 2 performance on ARM / AArch64.** The C11 and C99-`__atomic` paths both use `memory_order_relaxed` / `__ATOMIC_RELAXED`, which compile to plain `LDR`/`STR` and `LDADD` instructions. The legacy `__sync` fallback wraps every operation in `DMB ISH` full barriers, an order of magnitude more expensive per op.

The C99 fallback path exists for legacy flight toolchains (RAD750, LEON pre-GCC-4.7, older RTEMS/VxWorks) where C11 atomics are unavailable. It is not a recommended default for ground or Linux-class targets.

#### Minimal C++ example

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

The `-lbp -lici` libraries may have been compiled with either C11 or C99 fallback — either works. Your C++ program's own `std::atomic<>` usage is unaffected by ION's internal atomic implementation.

### Testing the Fallback Tiers

Three independent levers control the atomics path, and the recipes below keep them distinct:

1. **Integrator directive** (highest priority) — define exactly one of `ION_ATOMIC_C11`, `ION_ATOMIC_BUILTIN`, or `ION_ATOMIC_SYNC` to command which tier the header compiles. Also reachable via `./configure --with-atomics={c11,builtin,sync}`. Overrides both auto-detection and the `ION_TEST_FORCE_*` test knobs, so this is the cleanest lever for pinning a build (or a single test binary) to a specific tier.
2. **Language standard** (in `configure.ac:64-92`) — a C18 probe runs first, falling back to C99. Pass `ac_cv_c11=no` on the `./configure` line to short-circuit the C18 probe and compile under `-std=c99`; this also flips the `HAVE_C11_ATOMICS` Automake conditional at `configure.ac:97`, causing `ici/library/ion_atomic.c` (the Zone 1 mutex-backed fallback TU) to be compiled.
3. **Atomics tier dispatch test knobs** — preprocessor macros that sit on top of whatever language mode configure picked (only consulted when no integrator directive from lever 1 is set):

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

The first two recipes only override tier dispatch — the compiler stays in C18 mode, so `ici/library/ion_atomic.c` is *not* compiled (the `HAVE_C11_ATOMICS` Automake conditional is still true). The last two recipes flip both levers and exercise the full C99 path end-to-end. Without any flag, `./configure` auto-detects C18/`<stdatomic.h>` and selects the native C11 path (Tier 1) on modern Clang and GCC (≥ 4.7).

Validation harness: `tests/atomics/dotest` builds `tests/atomics/test_ipc_atomics.c` three times — once each with `-DION_ATOMIC_C11`, `-DION_ATOMIC_BUILTIN`, and `-DION_ATOMIC_SYNC` — and runs every binary. Each variant exercises `ion_ipc_atomic_t` under 8 concurrent threads × 300 000 operations per thread (gseq / refCount lock-free correctness) and additionally asserts that `set` and `exchange` preserve arbitrary non-1 values, guarding against any future regression to a builtin with the `__sync_lock_test_and_set` operand restriction. The test source contains `#error` self-checks so a misconfigured build cannot silently dispatch to the wrong tier. Add `-fsanitize=thread` to the C11 and `__atomic` variants for TSan coverage when the host toolchain supports it.

### Files

| File | Purpose |
|------|---------|
| `ici/include/ion_atomic.h` | Zone definitions, tier dispatch, macros and inline wrappers |
| `ici/library/ion_atomic.c` | Zone 1 C99 mutex-fallback implementations (compiled only when `!ION_HAVE_C11_ATOMICS`) |
| `tests/atomics/test_ipc_atomics.c` | Validation harness for Zone 2 (concurrency + set/exchange value preservation) |
| `tests/atomics/dotest` | Per-tier build matrix driver (C11 / `__atomic` / `__sync`); entry point for `runtests` |

## Application Behavior

Every process should return an exit code on termination.

- On normal termination, the exit code should be 0.
- On abnormal or error termination, the exit code should be a non-zero number in the range 1-255.
  - In this case the code should be 1 unless specific codes are used to distinguish between different kinds of errors.

## Function Design Guidelines

All file I/O should be performed using POSIX functions rather than the buffered I/O functions `fopen`, `fread`, `fseek`, etc.  This is because buffered I/O entails the dynamic allocation of system memory, which some missions may prohibit in flight software.

The iputs function provided in `platform.c` should be used in place of `fputs`, and the `igets` function should be used in place of `fgets`.  Rather than `fscanf`, use `igets` and `sscanf`; rather than `fprintf`, use `isprintf` and `iputs`.

All varargs-based string composition should be performed using `isprintf` rather than `sprintf`, to minimize the chance of overrunning string composition buffers.  (`isprintf` is similar to `snprintf`.  Since VxWorks 5.4 does not support `snprintf`, `isnprintf` is included in `platform.c`.)

Similarly, all string copying should be performed using `istrcpy` rather than `strcpy`, `strncpy`, and `strcat`.

The `isignal` function should be used instead of `signal`; it ensures that reception of a signal will always interrupt system calls in SVR4 fashion even when running on a FreeBSD platform.

The `iblock` function provides a simple, portable means of preventing reception of the indicated signal by the calling thread.

Data objects larger than 1024 bytes should not be declared in stack space.  This is to

- Minimize complaints by Coverity, and
- Minimize the chance of overrunning allocated stack space when running on a VxWorks platform.

**Static variables that must be made globally accessible should be declared within external functions, rather than declared as external variables.**  This is per the JPL C Coding Standard, but it also has the useful property of providing an easy way to track all access to a global static variable in `gdb`: you just set a breakpoint at the start of the function in which the variable is declared.

## Error Checking

In the implementation of any ION library function or any ION task’s top-level driver function, any condition that prevents the function from continuing execution toward producing the effect it is designed to produce is considered an “error”.

Detection of an error should result in the printing of an error message and, normally, the immediate return of whatever return value is used to indicate the failure of the function in which the error was detected.

By convention this value is usually -1, but both zero and NULL are appropriate failure indications under some circumstances such as object  creation.

The `CHKERR`, `CHKZERO`, `CHKNULL`, and `CHKVOID` macros are used to implement this behavior in a standard and lexically terse manner.

### CHK Macro Behavior and Fail-Fast Mode

When a CHK macro's condition evaluates to false, the following actions occur:

1. An error message is posted with the file name, line number, and the failed assertion expression
2. All error memos are written to the log via `writeErrmsgMemos()`
3. A stack trace is printed via `printStackTrace()` (on Linux and Solaris platforms)
4. If fail-fast mode is enabled (`CORE_FILE_NEEDED=1`), `sm_Abort()` is called to terminate immediately with a core dump
5. Otherwise, the function returns the appropriate error value (-1, 0, NULL, or void)

**Fail-Fast Mode (CORE_FILE_NEEDED)**

The `CORE_FILE_NEEDED` parameter controls whether assertion failures cause immediate process termination:

- **Default value is 1 (enabled)**: Assertion failures will cause immediate termination with a core dump, providing maximum debugging information
- **To disable at compile time**: Use `-DCORE_FILE_NEEDED=0` when compiling
- **To control at runtime**: Call `_coreFileNeeded(int *ctrl)` with a pointer to 0 (disable) or 1 (enable)

```c
/* Disable fail-fast mode at runtime */
int off = 0;
oK(_coreFileNeeded(&off));

/* Re-enable fail-fast mode */
int on = 1;
oK(_coreFileNeeded(&on));
```

**Stack Trace Support**

The `printStackTrace()` function prints a symbolic stack trace when assertions fail. This is supported on:

- **Linux**: Requires `HAVE_EXECINFO_H` to be defined and linking with `-rdynamic`
- **Solaris**: Uses `printstack()` from `<ucontext.h>`
- **FreeBSD**: Uses `backtrace()` from `<execinfo.h>`
- **macOS**: Uses `backtrace()` from `<execinfo.h>`

On other platforms, a message indicating stack trace unavailability will be logged instead.

**SDR Transaction Assertions (XNCHK macros)**

For assertions within SDR transactions, use the `XNCHKERR`, `XNCHKZERO`, `XNCHKNULL`, and `XNCHKVOID` macros. These variants additionally cancel the current SDR transaction via `crashXn()` before the fail-fast check, ensuring proper transaction cleanup.

**Interaction with ionrestart and SDR Reversibility**

When SDR transaction reversibility is enabled, failed transactions trigger the `ionrestart` utility to recover the system. During recovery, `ionrestart` temporarily disables fail-fast mode before restarting daemons to prevent assertion failures from cascading during the restart process. Fail-fast mode is restored after all daemons have successfully restarted.

If you are writing tests that intentionally trigger assertion failures or crash recovery scenarios, you should disable fail-fast mode at the start of your test:

```c
int off = 0;
oK(_coreFileNeeded(&off));
/* ... test code that may trigger assertions ... */
```

In the absence of any error, the function returns a value that indicates nominal completion. By convention this value is usually zero, but under some circumstances other values (such as pointers or addresses) are appropriate indications of nominal completion. Any additional information produced by the function, such as an indication of “success”, is usually returned as the value of a reference argument.

However, database management functions and the SDR hash table management functions deviate from this rule: most return 0 to indicate nominal completion but functional failure (e.g., duplicate key or object not found) and return 1 to indicate functional success.

Whenever returning a value that indicates an error:

- If the failure is due to the failure of a system call or some other non-ION function, assu=me that errno has already been set by the function at the lowest layer of the call stack; use `putSysErrmsg` (or `postSysErrmsg` if in a hurry) as described below.
- Otherwise – i.e., the failure is due to a condition that was detected within ION –use `putErrmsg` (or `postErrmg` if pressed for time) as described below; this will aid in tracing the failure through the function stack in which the failure was detected.

When a failure in a called function is reported to “driver” code in an application program, before continuing or exiting use `writeErrmsgMemos()` to empty the message pool and print a simple stack trace identifying the failure.

Calling code may choose to ignore the error indication returned by a function (e.g., when an error returned by an sdr function is subsumed by a future check of the error code returned by `sdr_end_xn`).  To do so without incurring the wrath of a static analysis tool, pass the entire function call as the sole argument to the `oK` macro; the macro operates on the return code, casting it to `(void)` and thus placating static analysis.

## Error and Status Reporting

To write a simple status message, use `writeMemo`.  To write a status message and annotate that message with some other context-dependent string, use `writeMemoNote`.  (The `itoa` and `utoa` functions may be used to express signed and unsigned integer values, respectively, as strings for this purpose.)  Note that adhering to ION’s conventions for tagging status messages will simplify any automated status message processing that the messages might be delivered to, i.e., the first four characters of the status message should be as follows:

- [i] – informational
- [?] – warning
- [s] – reserved for bundle status reports
- [x] – reserved for communication statistics

To write a simple diagnostic message, use `putErrmsg`; the source file name and line number will automatically be inserted into the message text, and a context-dependent string may be provided.  (Again the `itoa` and `utoa` functions may be helpful here.)  The diagnostic message should normally begin with a capital letter and end with a period.

To write a diagnostic message in response to the failure of a system call or some other non-ION function that sets errno, use `putSysErrmsg` instead.  In this case, the diagnostic message should normally begin with a capital letter and not end with a period.

## ‘C’ Coding Style

This page contains guidelines for programming in the C language.

### Naming Conventions

Names of global variables, local variables, structure fields, and function arguments are in mixed upper and lower case, without embedded underscores, and beginning with a lowercase letter.

```c
int numItems;
```

Private function names are in mixed upper and lower case, without embedded underscores, and beginning with a lowercase letter.

```c
void computeSomething(int firstArg, int secondArg);
```

Public function names are in lower case with tokens separated by underscores.  The first token of each public function name is the name of the package whose “include” directory contains the .h file in which the function prototype is defined.

```c
 extern int ltp_open(unsigned long clientId);
```

Macro names are written in upper case with tokens separated by underscores.

```c
#define SYMBOLIC_CONSTANT 5
```

Unions are not used.

Typedef names are in mixed upper and lower case, with the first token capitalized.  Type names are never the same as the structure or enum tags for the structures and enums that they name.

```c
typedef struct gloplist_str
{
int thing1;
int thing2;
} GlopList;
```

### Indentation, Bracketing, Whitespace

No line of source text is ever more than 80 characters long.  When the length of a line of code exceeds 80 characters, the line of code is wrapped across two or more lines of text.  Whenever the point at which the text must be wrapped is within a literal, a newline character (\) is inserted at the wrap point and the continuation of the literal begins in the first column of the next text line.  Otherwise, each continuation line is normally indented two tab stops from the first text line of the long line of code; when indenting just one tab stop (rather than two) seems to make the code more readable, indenting one tab stop is okay.  When a single meaningful clause of a source code line must be wrapped across multiple lines of text, each text line after the first line in that clause is normally indented one additional tab stop.

In the declaration of a function or variable, the type name and function/variable name are normally separated by a single tab.  They may be separated by multiple tabs when this is necessary in order to have the variable names in multiple consecutive variable declarations line up, which is always preferred.

Functions are written with the return type, function name, and arguments as a single line of code, subject to the code line wrapping guidelines given above.  The opening brace of the function definition appears in the first column of the next line.

The opening brace of a structure definition likewise appears in the first column of the next line after the structure name.

A control statement (starting with `if`, `else`, `while`, or `switch`) begins a new line of code.  The opening brace for the control statement always appears on the next line, at the same indentation as the control statement keyword.

The first line of code appearing after an opening brace (whether for a structure definition, for a function definition, or in the scope of a control statement) always appears on the next line, indented one tab stop.  From that point on, every subsequent line of code is indented the same number of tabs as the preceding line of code, subject to the code line wrapping guidelines given above.

Every closing brace always appears in the same column as the corresponding opening brace.

Every closing brace is always followed by a single blank line, except when it is immediately followed either by another closing brace (which will be indented one less tab stop) or by an else (which will be indented by the same number of tab stops as the closing brace and, therefore, the corresponding if).

```c
static void  computeSomething(int numItems, Item *items)
{
 unsigned int x;
 int  i;

 while (x > 0)
 {
  x--;
 }

 for (i = 0; i < numItems; i++)
 {
  x += items[i].field1;
 }

 if (numItems == 0)
 {
  doThis();
 }
 else
 {
  doThat();
 }
}
```

The case labels in switch statements line up with the braces.  Every case (or default) label in the switch, after the first case, is preceded by a blank line.  Cases which do not include a break or return statement either contain no code at all  or else end with a comment along the lines of:

```c
/* Intentional fall-through to next case. */
For example:
switch (ch)
{
case 'A':
.
.
.
break;

case 'B':
case 'C':
.
.
.
break;

case 'D':
.
.
.
/* Intentionally falls through. */

case 'E':
.
.
.
break;

default:
.
.
.
break;
}
```

### Comment Formatting

Comments are so rare and valuable that we hesitate to risk discouraging them by overly constraining their format.  In general, comments should be inserted in such a way as to be as easy as possible to read in relevant context.  The multi-line comment formatting performed automatically by vim is particularly acceptable.

```c
/* Here is the beginning of an extremely long comment, so long
 *  that it has to wrap over two lines of source code text. */
```

### Miscellaneous Rules

Use – and write – thread-safe library functions where possible.  E.g., normally prefer `strtok_r()` to `strtok()`.

Avoid writing non-portable code, e.g., prefer POSIX library calls to OS-specific library calls.

Template for ".c" files

```c
 1 2 3 4 5 6 7
123456789012345678901234567890123456789012345678901234567890123456789012
/*
 platform_sm.c: platform-dependent implementation of common
   functions, to simplify porting.

 Author:  Alan Schlutsmeyer, JPL

 Copyright 1997, California Institute of Technology.
 ALL RIGHTS RESERVED.  U.S. Government sponsorship
 acknowledged.
                                         */
```

Each file should have a header comment like the one shown above.

```c
#include <stdio.h>
#include <locallib.h>
#include "appheader.h"
  .
  .
  .
```

.h files are included just after the header.  System-provided headers should be specified with angle brackets; ION-provided headers should be specified with double-quotes.

```c
#define SYMBOLIC_CONSTANT 5
```

Next, symbolic constants and macros (if any) are defined. They normally go first, because they might be used in the definitions of data types and static variables.  However, symbolic constants and macros may be inserted later in the source text if that will improve the readability of the file.

```c
typedef struct fb_str
{
 int field1;
 in field2;
} Foobar;
```

Data types are defined next because they might be used by static variables.

```c
static int numFoobars = 0;  /* Number of foobars in the program. */
  .
  .
  .
```

Global functions used only within the program should be declared static.
Public function prototypes should be in a header file; the definitions of those functions, with their headers, are included in the corresponding .c file.  Low-level functions, such as commonly-used utility functions, appear first in the .c file.  They are followed by the functions that call those functions directly, followed by higher-level-functions that call those functions, and so on.

Template for ".h" Files

```c
 1 2 3 4 5 6 7
123456789012345678901234567890123456789012345678901234567890123456789012
/*
 platform_sm.h: portable definitions of types and functions.

 Author:  Alan Schlutsmeyer, JPL

 Copyright 1997, California Institute of Technology.
 ALL RIGHTS RESERVED.  U.S. Government sponsorship
 acknowledged.
                                         */
```

Each header file begins with a standard header comment like the one shown above.

```c
#ifndef _PLATFORM_SM_H_
#define _PLATFORM_SM_H_
```

Each header file must have an "include" guard.

```c
#include "platform.h"
  .
  .
  .
```

Next come any includes required by the declarations in the header.

```c
#ifdef __cplusplus
extern "C" {
#endif
```

Next comes the beginning of the C++ guard. This allows the header to be included in a C++ program without error.

Next come declarations of various sorts, followed by the ends of the C++ and “include” guards.

```c
#ifdef __cplusplus
}
#endif

#endif
```

Nothing should go after the "#endif" of the include guard.

## SDR Transaction

All writing to an SDR heap must occur during a transaction that was initiated by the task issuing the write.  Transactions are single-threaded; if task B wants to start a transaction while a transaction begun by task A is still in progress, it must wait until A's transaction is either ended or cancelled.

A transaction is begun by calling `sdr_begin_xn()`, and the current transaction is normally ended by calling the `sdr_end_xn()` function, which returns an error code in the event that any serious SDR-related processing error was encountered in the course of the transaction Transactions may safely be nested, provided that every level of transaction activity that is begun
is properly ended.

Another way to terminate a transaction is using the `sdr_exit_xn()` call, as a way to implement a critical section within which SDR data is read while no data modifications occurred. Using the `sdr_exit_xn()` to end a transaction indicates that no SDR modification should have occurred during the critical section. Therefore the `sdr_exit_xn` function will check whether any SDR modifications were made during the transaction and if the current transaction is the outer most layer (depth = 1) then it will result in a unrecoverable SDR error.

Given that `sdr_end_xn()` is designed to handle transaction with SDR modification while `sdr_exit_xn()` does not, why would one want to use `sdr_exit_xn()` to end a transaction? This is useful as a way to detect SDR changes that is not supposed to occur but does occur - it catches both logical error and implementation error in the code. It is a useful "check" whether the SDR behavior is as expected. In case  `sdr_exit_xn()` detected a SDR modification as the outer most layer of transaction, an error message will be loggedin ion.log file to document a potential issue with the implementation logic. This is usedul for debugging and improving the overall SDR transaction management software.

The choice between `sdr_end_xn()` and `sdr_exit_xn()` depends on the intent of the transaction and the outcome of a transaction:

- Situation A: If a fault occurred that requires reverting SDR modifications (including those made by all nested layers) leading up to the fault event, then one should call sdr_cancel_xn to trigger SDR reversibility, if configured, and allow ion to reload the volatile protocol state to restore the SDR heap and working memory state.

- Situation B: If a fault occurred but the implementation has the proper procedure and logic to handle handling it such as:

  - The fault did not result in any SDR heap changes, or
  - The modifications that occurred before the fault and as part of fault handling afterward do not require reversal. For example, if the SDR modifications made before the fault may still be valid despite the occurrence of the fault, and the SDR modifications made after the fault, intentionally as part of fault handling procedure such as updating bundle error counters are successful, **AND**
  - In either case, the integrity of the protocol state is not affected or can be resotred. For example, an invalid bundle was detected and can be safety removed from ION by making appropriate updates to the heap and the working memory. Then one can still call `sdr_end_xn` since the failure was handled nominally.

- Situation C: All transactions and modifications are nominal. In this case, call `sdr_end_xn.`

The implementation must be able to discern between situations A and B. When one cannot make certain that the SDR will be in operation state due to a complex transaction failure case, or due to system errors, one one should default to A and issue transaction cancellation and rely on reversibility in order to avoid leaving the SDR in an inconsistent state.

### Transaction Lock and Robust-Mutex Recovery

The lock that serializes access to an SDR is, by default, a POSIX named semaphore. This carries a failure mode: POSIX named semaphores have no dead-owner recovery, so if a process is killed (`SIGKILL`, an OOM kill, a crash, a debugger) between `sdr_begin_xn()` and `sdr_end_xn()` / `sdr_exit_xn()`, the lock is orphaned. Every subsequent `sdr_begin_xn()` on that node then blocks forever and the node wedges silently — no assertion, no core dump.

On platforms that support it, ION instead implements the transaction lock as a process-shared **robust** `pthread_mutex_t` (`PTHREAD_MUTEX_ROBUST`). When the owning process dies, the next `pthread_mutex_lock()` returns `EOWNERDEAD` deterministically; ION then rolls back the dead owner's partial transaction using the reversibility log and marks the mutex consistent again, so the node recovers instead of wedging. If the dead transaction modified a *non-reversible* SDR it cannot be rolled back: the mutex is left `ENOTRECOVERABLE`, so `sdr_begin_xn()` fails loudly rather than wedging or building on a corrupt SDR.

**Configure option.** Robust-mutex support is detected automatically by `configure` and enabled wherever it is available; `config.h` then defines `ION_HAVE_ROBUST_MUTEX`. Pass `--disable-robust-sdr-lock` to force the legacy semaphore path even where the robust mutex is supported.

**Platform gating.** The robust mutex is used only on **Linux, FreeBSD and Solaris**: `configure` applies an explicit platform allowlist in addition to the toolchain probe. VxWorks and RTEMS keep the legacy semaphore path even though a recent RTEMS C library may implement `PTHREAD_MUTEX_ROBUST`; macOS has no robust-mutex support at all. See the **Operating System Support Matrix** above. A single ION build uses one path or the other for every process, since all processes on a node share the SDR.

**Relationship to the atomics zones and tiers.** Robust-mutex selection is an **independent axis** from the C11-atomics tier selection described under **Atomic Operations**. Robust mutexes are a POSIX C-library and kernel feature, not a C language feature: they need neither C11 nor `<stdatomic.h>` and work unchanged in a C99 build. `configure` detects them with a separate probe and a separate macro (`ION_HAVE_ROBUST_MUTEX`), unrelated to `ION_HAVE_C11_ATOMICS` or `--with-atomics`. The two features also guard disjoint data: anything serialized *inside* an SDR transaction needs no atomics, because acquiring and releasing the mutex is itself a full memory barrier, while lock-free data continues to use the atomic types. The one point of contact is the robust-mutex shutdown flag (`sdrXnEnded`, which carries the signal that `sm_SemEnded()` provided on the semaphore path): it lives in the shared `SdrState`, so it must be a **Zone 2** `ion_ipc_atomic_t`, never a process-local `ion_atomic_t` — see **Dual-Zone Architecture**.

**Limitations.** The robust mutex detects owner *death*; it does not help if a live process hangs or deadlocks while holding the lock. Full recovery of a dead *writer* requires the SDR to be configured `SDR_REVERSIBLE` (i.e. to keep a transaction log); without the log, an orphaned writer is still detected deterministically but is reported unrecoverable rather than silently wedging.

## BP Service Access Point (SAP) Ownership

A Bundle Protocol Service Access Point (`BpSAP`) opened for reception via `bp_open()` is **owned exclusively by the single thread that opened it**. This is a hard invariant of the ION BP API, not a recommendation, and it has two consequences that callers must respect.

**Only the owning thread may receive on the SAP.** The volatile endpoint object (`VEndpoint`) tracks ownership with a `<appPid, appCookie>` tuple, set on `bp_open()` and cleared on `bp_close()`. `appPid` is `sm_TaskIdSelf()` (the OS PID; on Linux this is the thread-group ID via `getpid()`, so every thread in a process reports the same value). `appCookie` is `sm_ProcessCookie()`, a per-process-instance value that lets ION distinguish two distinct processes that happen to share a recycled PID. Ownership checks in `bp_receive()` and `bp_close()` are at *process* granularity, not thread granularity: ION cannot detect a second thread in the owning process calling `bp_receive()` on a SAP it did not open. Such cross-thread reception is undefined behaviour at the API level and will race against the owner inside the delivery semaphore and SDR transactions. Applications that need delivery from multiple threads must funnel reception through the single owning thread.

**A second `bp_open()` on the same endpoint always fails.** ION does not permit a process or thread to "reopen" an endpoint it already holds. If `vpoint->appPid` is set to a live task, every subsequent `bp_open()` returns `-1` with `putErrmsg("Endpoint is already open.", "<pid>")`, regardless of whether the second caller is:

* the same thread that already opened it,
* a different thread in the same process, or
* a thread in a different process.

The collapse of these three cases into one error path is deliberate: ION has no per-thread ownership token, so it cannot meaningfully distinguish them, and any attempt to be "lenient" for the same-PID case produced an `rc == 0` return with `*bpsapPtr == NULL` that propagated as a delayed null-pointer dereference in `bp_receive()`. Callers should test `bp_open()`'s return with `if (rc != 0)` (or `if (rc < 0 || sap == NULL)` to also handle the `dtn:none` null-EID case) and bail out rather than dereferencing the SAP. If a previous owner died without calling `bp_close()`, `createBpSAP()` self-heals by clearing `appPid` and allowing the new open to proceed; the caller does not need to retry. The same self-heal fires for PID recycling: when a brand-new process happens to receive the same PID as the dead original owner, its `appCookie` mismatches, so `createBpSAP()` treats the stored ownership as stale and reclaims the endpoint instead of misidentifying the new process as the original owner.

**Source-only SAPs are exempt.** `bp_open_source()` (used for send-only SAPs) does not set `appPid` and imposes no exclusivity. Multiple threads or processes may open source SAPs on the same endpoint concurrently without conflict.

**Lifetime invariant — close from the owning thread, on every exit path.** A reception SAP must be closed by its owning thread before that thread exits, *including* abnormal exit paths (cancellation, longjmp out of the receive loop, an error return out of the worker function). If the owning thread terminates without calling `bp_close()`, the endpoint remains locked for the lifetime of the **process**: ION's self-heal logic in `createBpSAP()` only fires when `sm_TaskExists(appPid)` reports the whole process is gone, not the individual thread. Subsequent `bp_open()` calls on that endpoint — from any thread in the same process — will keep returning `-1` with `"Endpoint is already open."` until the process exits.

The recommended idiom is to bracket the receive loop with a thread-cancellation cleanup handler so `bp_close()` runs regardless of how the thread leaves the loop:

```c
static void closeSap(void *arg)
{
    bp_close((BpSAP) arg);
}

void *worker(void *arg)
{
    BpSAP sap;

    if (bp_open(eid, &sap) != 0)
    {
        return NULL;       /* bp_open already logged via putErrmsg */
    }
    pthread_cleanup_push(closeSap, sap);
    while (running)
    {
        if (bp_receive(sap, &dlv, BP_BLOCKING) < 0) break;
        /* ... process delivery ... */
    }
    pthread_cleanup_pop(1);     /* runs closeSap on every exit path */
    return NULL;
}
```

`pthread_cleanup_pop(1)` invokes the handler on normal return, on `pthread_exit()`, and on `pthread_cancel()`. It does **not** run if the thread is killed by an asynchronous signal or if the whole process crashes — for those, only process-exit cleanup applies, and `createBpSAP()`'s self-heal will release the endpoint on the next ION-aware start.

## Static Analysis Notes

Static analysis tools (CodeSonar, Coverity, cppcheck) flag certain patterns in ION that are **not bugs** given ION's architecture and fault model. The following documents known false-positive categories to help developers triage warnings efficiently.

### Null Pointer Dereference — Shared Memory Lookups

ION's `psp()`, `sm_list_data()`, and `sdr_list_data()` functions return pointers into shared memory (PSM) or the SDR heap. Static analyzers flag every dereference of these return values because the functions *could* theoretically return NULL.

**Why these are false positives in ION:** The addresses passed to `psp()` are stored by ION's own data structure management layer. They are written during validated insertion operations (e.g., `sm_list_insert`, `sdr_list_insert`) and are only read within the same transaction context or after the insertion transaction committed successfully. A NULL return from `psp()` would indicate PSM corruption — a fatal system condition handled at a higher level (ION restarts), not a recoverable error within individual functions.

**Guidance:** Do not add NULL checks after `psp()` / `sm_list_data()` / `sdr_list_data()` unless the address itself could be zero (e.g., an optional field). Adding unnecessary checks clutters the code and creates dead branches.

### Null Pointer Dereference — Callback Parameters

Container callbacks (comparison functions for `sm_rbt`, `sm_list`, radix trees) receive data pointers from the container infrastructure. These are guaranteed non-NULL by the container's insertion invariants. CodeSonar cannot see through the callback indirection and flags every parameter dereference.

### File System Race Condition (TOCTOU)

ION utilities (`sendfile`, `recvfile`, `metadata.c`) use standard POSIX file operations. CodeSonar flags the inherent race between filename resolution and file operations (e.g., `fopen`, `stat`, `remove`, `rename`).

**Why these are not exploitable in ION:** ION file utilities operate on locally-managed temporary files with randomized names, or on user-specified files in a single-user context. No untrusted actors share the file namespace in ION's operational environment (embedded spacecraft systems, dedicated ground stations). There is no privilege boundary crossed between the check and the use.

**Exception:** If ION is ever deployed in a multi-tenant environment with shared directories, these patterns should be revisited.

### Double Lock / Double Unlock — Session Teardown

The TCPCL adapter (`tcpcli.c`) uses a deliberate "unlock before destroy" pattern in `endSession()`: mutexes are unconditionally unlocked before `pthread_mutex_destroy()` to ensure they are not held during destruction, regardless of which error path led to teardown.

**Why this is intentional:** Session teardown can be initiated from multiple threads (clock, receiver, sender). The unconditional unlock ensures `pthread_mutex_destroy()` does not encounter a locked mutex, which is undefined behavior on some platforms. The redundant unlock of an already-unlocked `PTHREAD_MUTEX_DEFAULT` is technically UB per POSIX, but is a deliberate safety pattern that is benign on Linux/glibc (returns EPERM silently).

### Unreasonable Size Argument

Functions like `memcpy`, `strncmp`, and `malloc` are flagged when the size parameter could theoretically be negative (when stored as a signed type that gets implicitly converted to `size_t`).

**Common patterns:**

- **Serialization lengths from internal helpers** (e.g., `bpsec_rfc9173utl_outBlkHdrSerialize`): These return -1 on error, but callers only invoke them after verifying preconditions (block exists, bundle is valid). The error path is unreachable in practice.
- **Pointer subtraction for EID parsing** (e.g., `traceEid_dot - traceEid_num`): IPN EIDs are validated by the BP layer (`parseEidString`) before reaching these functions. The format `ipn:X.Y` guarantees the colon precedes the dot.

### Coercion Alters Value

CodeSonar flags narrowing conversions from `uvast` (64-bit) to smaller types (e.g., `int`, `unsigned int`) in admin command parsing and EID handling.

**Guidance:** These are generally acceptable when the value range is constrained by the protocol (e.g., node numbers fit in 32 bits for 2-part IPN, service numbers are small integers). Add an explicit cast with a range check only when the value originates from external/untrusted input and could realistically exceed the target type's range.

### Cast Alters Value

Similar to Coercion Alters Value, but flagged for explicit casts rather than implicit conversions. Common in `ipnadmin.c`, `libipnfw.c`, and `tcpcli.c` where pointer-to-integer or wider-to-narrower casts are used for IPC data (e.g., storing a length in a `saddr` passed through `lyst_data()`).

**Guidance:** Same as Coercion Alters Value. These casts are intentional and the values are range-constrained by protocol definitions.

### Ignored Return Value

CodeSonar flags calls to functions like `fseek()`, `sdr_list_data()`, `psp()`, and `lyst_first()` where the return value is either assigned but not compared to an error sentinel, or entirely discarded.

**Common patterns in ION:**

- **SDR/PSM accessors** (`sdr_list_data`, `psp`, `sm_list_data`): These do not have a traditional error return. NULL indicates fatal PSM corruption handled at a higher level, not a per-call recoverable condition.
- **File I/O in utilities** (`fseek`, `fread` in `metadata.c`): These operate on files just created by the same process. I/O errors indicate hardware failure, not a programmatic condition.
- **Best-effort operations** (CGR routing, multicast forwarding): Individual sub-operations may fail without invalidating the overall operation. The function proceeds with partial results.

**Guidance:** Return values should be checked when failure indicates a recoverable condition that changes control flow. Do not add checks purely to satisfy the analyzer when the only possible response would be to abort (which ION already handles via transaction rollback or process termination).

### Useless Assignment — `sm_TaskVar()` Pattern

ION's semaphore-accessor functions (e.g., `_ipnfwSemaphore`, `ltpcloSemaphore`, `bsspcloSemaphore`) use the `sm_TaskVar()` pattern:

```c
static int *_fooSemaphore(int *newValue)
{
    void *value;
    if (newValue)           /* Set branch */
    {
        value = (void *) (*newValue);
        /* CodeSonar flags this assignment as "useless" */
    }
    return (int *) sm_TaskVar(&value);
}
```

CodeSonar sees only one branch per call and flags the assignment as useless. In reality, the value is consumed by `sm_TaskVar` through the pointer.

**Also flagged:** CBOR serialization macros that update cursor/length variables consumed by subsequent macro expansions, and loop variables initialized before a `for` that the analyzer considers redundant.

### Unreachable Data Flow / Unreachable Computation / Unreachable Call

CodeSonar determines that certain code paths are unreachable based on prior condition checks or compile-time constants.

**Common patterns:**

- **AMP (Application Management Protocol) generated accessors** (`adm_bpsec_impl.c`): Template-generated functions with uniform error handling where specific error branches are unreachable for specific accessor instances but exist for template uniformity.
- **Exhaustive switch/if-else with default case**: Functions that handle all current enum values but include a default/else clause for future values or configuration portability.
- **Build-configuration-dependent paths**: Code guarded by constants (protocol versions, feature flags) that eliminate paths in a specific build but are valid in others.

**Guidance:** Do not remove these "unreachable" paths. They serve as defensive coding for future enum extensions, cross-configuration portability, and template uniformity. Removing them would create maintenance burden when new values are added.

### Redundant Condition

CodeSonar flags conditions that are always true or always false given the preceding control flow. Common in `bpsecadmin.c` JSON parsing where multiple validation layers check overlapping conditions.

**Why these are intentional:** The BPSec admin parser functions are called from multiple contexts with different precondition guarantees. The "redundant" conditions provide safety without relying on caller discipline — a function validates its own inputs regardless of what the caller already checked. This is deliberate defensive programming for security-critical code (key management, policy rules).

### Empty if Statement

ION uses the `oK()` macro pattern:

```c
if (condition)
{
    /* intentionally empty — side effect is in the condition */
}
```

Or more commonly, `oK(someFunction())` where the function is called for its side effects and the return value is explicitly discarded via the `oK` macro. CodeSonar sometimes flags the expanded form as an "empty if" when the macro expands to a conditional.

### Dangerous Function Cast

Signal handler registration (`isignal()`, `signal()`) requires casting between `void(*)(int)` and platform-specific handler signatures. This is standard POSIX practice and unavoidable without platform-specific wrappers (which ION provides via `isignal` in `platform.h`).

### Tainted Buffer Access

Flagged in test utilities (`bpsendtest`, `bpdriver`) that read command-line arguments into fixed buffers. These are test/development tools run in controlled environments, not production daemons exposed to untrusted input. Buffer sizes are adequate for their intended use (EID strings, file paths within OS limits).
