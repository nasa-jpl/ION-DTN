# ION on RTEMS

## Verification status (ION 4.2.0)

ION's RTEMS port now covers a broader set of architectures than in 4.1.x.
JPL CI exercises one target end-to-end; the remaining targets are reachable
through the contributor-supplied `arch-rtems/build.sh` recipe but are not
qualified by JPL.

### Verified mdash; RTEMS 6.1 on aarch64 / `a53_lp64_qemu`

This is the certified target for the ION 4.2.0 release.

- BSP: `aarch64/a53_lp64_qemu` (ARMv8-A virt machine, Cortex-A53, LP64)
- Networking: rtems-libbsd (FreeBSD-derived TCP/IP stack, loopback)
- Modules exercised under QEMU on every PR:
  - Bundle Protocol v7 (BP) with IPN scheme
  - Licklider Transmission Protocol (LTP) over UDP
  - TCP convergence layer (TCPCL)
  - CFDP (file delivery loopback)
  - BSSP and DGR convergence layers
  - AMS (minimal core: `libams` + `amscommon` + `loadmib` + DGR/TCP/UDP
    transport services; pitch/catch loopback under `amsd`)
- Driver: ION Admin Public API (no configuration files)
- 64-bit addressing (`SPACE_ORDER=3`)

CI workflow: `.github/workflows/ci-rtems61-aarch64-libbsd.yml`
(`rtems-build-and-test`).
Verification scans the QEMU console for IPN forwarder + UDP services
+ LTP segment transmit/receive + bundle payload delivery
on every relevant pull request.

### Community-contributed mdash; not certified

The `arch-rtems/build.sh` helper also targets the following BSPs.
They build and pass the contributor's local QEMU smoke runs,
but are not exercised in JPL CI.
Treat them as starting points for your own port qualification,
not as flight-ready configurations.

| `ARCH=`     | BSP                              | RTEMS series | Notes                                              |
|-------------|----------------------------------|--------------|----------------------------------------------------|
| `sparc`     | `sparc/leon3`                    | 7            | Gaisler LEON3 (SPARC V8). 32-bit.                  |
| `arm`       | `arm/xilinx_zynq_a9_qemu`        | 7            | Xilinx Zynq-7000 Cortex-A9. 32-bit.                |
| `aarch64`   | `aarch64/a53_lp64_qemu`          | 7            | Same BSP as the verified target, RTEMS 7 toolchain.|
| `riscv`     | `riscv/rv64imafdc`               | 7            | RV64GC, lp64d ABI (QEMU virt). 64-bit.             |

Why RTEMS 7 for these BSPs?
Per the contributor's notes in `build.sh`,
RTEMS 6 + libbsd deadlocks in generic IMFS `stat()`/`open()`
once the BSD stack is up on these BSPs.
The fix combinations land cleanly only on RTEMS main / 7-freebsd-14,
so non-aarch64-6.1 ports must use RTEMS 7.

### Known broken

- **PowerPC `qoriq_e500`** &mdash; `rtems-libbsd` does not build cleanly for PPC;
  SDA memory-mapping issues and additional RTEMS 6 PPC defects
  prevent a working build.
  `arch-rtems/build.sh` exits with an UNSUPPORTED message for `ARCH=powerpc`.

## Footprint note for 32-bit RTEMS targets

ION 4.2.0 widens the PSM small-block alignment grain to `max(WORD_SIZE, 8)`.
On 64-bit hosts (RTEMS aarch64, Linux, Solaris) this is a no-op
because `WORD_SIZE` is already 8.
On 32-bit RTEMS targets (sparc/leon3, arm/zynq),
small-block overhead and stride double from 4 to 8 bytes &mdash;
re-budget PSM working-memory sizes if your deployment is memory constrained.
The widening is required for correct alignment of 8-byte-aligned types
(`long long`, `time_t`, `ion_ipc_atomic_t`) embedded in PSM-allocated structs.

## Installation and documentation

For complete installation instructions, implementation details, and
troubleshooting, refer to the documentation in `arch-rtems/`:

**[arch-rtems/README](https://github.com/nasa-jpl/ION-DTN/blob/current/arch-rtems/README)**

- Installation and build instructions
- Prerequisites and environment setup
- BSP configuration and adaptation guide
- Testing and verification procedures

**[arch-rtems/build.sh](https://github.com/nasa-jpl/ION-DTN/blob/current/arch-rtems/build.sh)**

- One-shot recipe (RSB toolchain + RTEMS kernel + rtems-libbsd + ION +
  QEMU smoke test) per `ARCH=` selector

**[arch-rtems/KEY-FIXES-SUMMARY.md](https://github.com/nasa-jpl/ION-DTN/blob/current/arch-rtems/KEY-FIXES-SUMMARY.md)**

- Critical fixes and workarounds
- Troubleshooting common issues

**[arch-rtems/UDP-NETWORK-STATUS.md](https://github.com/nasa-jpl/ION-DTN/blob/current/arch-rtems/UDP-NETWORK-STATUS.md)**

- libbsd network-stack integration details

### Quick start (verified target)

```bash
# From the ION-DTN repo root
cd arch-rtems

# Populate symbolic links to ION source
./srclinks

# Configure and build for aarch64 / a53_lp64_qemu (RTEMS 6.1)
./waf configure --rtems=$RTEMS_ROOT --rtems-bsp=aarch64/a53_lp64_qemu
./waf build

# Smoke-test in QEMU
qemu-system-aarch64 \
    -no-reboot -nographic -serial mon:stdio \
    -machine virt,gic-version=3 -cpu cortex-a53 -m 4096 \
    -kernel build/aarch64-rtems6-a53_lp64_qemu/ion.exe
```

### Quick start (community-contributed targets)

```bash
# From the ION-DTN repo root; build.sh handles RSB + kernel + libbsd + ION
ARCH=sparc   arch-rtems/build.sh   # sparc/leon3, RTEMS 7
ARCH=arm     arch-rtems/build.sh   # arm/xilinx_zynq_a9_qemu, RTEMS 7
ARCH=riscv   arch-rtems/build.sh   # riscv/rv64imafdc, RTEMS 7
ARCH=aarch64 arch-rtems/build.sh   # aarch64/a53_lp64_qemu, RTEMS 7
```

See `build.sh` for the full set of environment overrides
(`RTEMS_VER`, `LIBBSD_BRANCH`, `RTEMS_PREFIX`, etc.).

## Important notes

- **Template port.** Outside the verified target, this remains a
  reference implementation that requires adaptation to your specific
  hardware and BSP.
- **Prerequisites.** RTEMS 6.1 toolchain (for the verified target) or
  RTEMS 7 toolchain (for the community-contributed targets), BSP with
  libbsd support, Python 3, QEMU appropriate to the chosen `ARCH=`.
- **Default configuration.** Loopback testing on 127.0.0.1.
- **Customization required for hardware.** Network interface, memory
  sizes, BSP configuration, system clock.

## Getting help

- **Installation issues:** consult `arch-rtems/README` and
  `KEY-FIXES-SUMMARY.md`
- **Network configuration:** see `arch-rtems/UDP-NETWORK-STATUS.md`
- **Bug reports:** ION GitHub Issues
