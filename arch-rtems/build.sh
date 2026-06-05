#!/usr/bin/env bash
# Local Ubuntu repro of ION's RTEMS port under QEMU.
# Run from the ION-DTN repo root.
#
# Pick a target with ARCH=:
#   ARCH=sparc   (default) - Gaisler LEON3 (SPARC V8, qemu-system-sparc -M leon3_generic)
#                            * CPU      : LEON3 (SPARC V8, IU+FPU, Gaisler extensions)
#                            * UART     : APBUART (Gaisler) wired to stdio
#                            * Ethernet : GRETH (Gaisler 10/100 MAC), RTEMS libbsd
#                            * Timer    : GPTIMER (Gaisler)
#                            * RAM      : 64 MiB (GR-CPCI-LEON3 / GR712RC class)
#   ARCH=arm               - Xilinx Zynq-7000 Cortex-A9 (qemu-system-arm -M xilinx-zynq-a9)
#                            Spacecraft-typical 32-bit ARM SoC: dual Cortex-A9 + FPGA
#                            fabric, flown on numerous cubesats and OBCs (e.g. NASA
#                            SpaceCube, ESA OPS-SAT class). RTEMS BSP
#                            arm/xilinx_zynq_a9_qemu; console is UART1.
#   ARCH=aarch64           - ARMv8-A virt machine, Cortex-A53 (qemu-system-aarch64)
#
# leon3_generic is the closest in-tree QEMU model of a real Gaisler dev board.

set -euo pipefail

# RTEMS 7 (development line) is required because RTEMS-6 / 6-freebsd-12|14
# libbsd both wedge generic IMFS stat()/open() once the BSD stack is up.
# The fix combinations (cpukit/libio !635 + libbsd IOP-refcount fixes) only
# land cleanly on RTEMS main / 7-freebsd-14.
RTEMS_VER=${RTEMS_VER:-7}
RTEMS_SERIES=${RTEMS_VER}
LIBBSD_BRANCH=${LIBBSD_BRANCH:-7-freebsd-14}
RTEMS_KERNEL_BRANCH=${RTEMS_KERNEL_BRANCH:-main}
RSB_BRANCH=${RSB_BRANCH:-$RTEMS_SERIES}
RTEMS_PREFIX=${RTEMS_PREFIX:-$HOME/rtems-$RTEMS_VER}
WORK=${RTEMS_WORK:-$HOME/rtems-work-$RTEMS_VER}
# Repo root, derived from this script's own location (arch-rtems/),
# so it can be invoked from anywhere.
ION_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
ARCH=${ARCH:-sparc}

# RTEMS 7 uses the main RSB branch
if [ "$RSB_BRANCH" = "7" ]; then
    RSB_BRANCH=main
fi

# Per-arch serial wiring. Default is single UART → stdio; Zynq has two UARTs
# and the RTEMS console lives on UART1, so we need to skip UART0 first.
SERIAL_ARGS=(-serial mon:stdio)

case "$ARCH" in
    sparc)
        BSP=sparc/leon3
        ARCH_BSP=sparc-rtems${RTEMS_VER}-leon3
        RSB_TARGET=${RTEMS_SERIES}/rtems-sparc
        TOOL_GCC=sparc-rtems${RTEMS_VER}-gcc
        QEMU=qemu-system-sparc
        QEMU_PKG=qemu-system-sparc
        # leon3_generic: Gaisler LEON3 board model in mainline QEMU.
        # -m 256: enough headroom for the unified workspace + libbsd's
        # UMA + ION's 6 MB of PSM/SDR without provoking
        # INTERNAL_ERROR_TOO_LITTLE_WORKSPACE on libio allocations.
        QEMU_ARGS=(-M leon3_generic -m 256)
        ;;
    arm)
        # Xilinx Zynq-7000 (Cortex-A9 MPCore). RTEMS BSP
        # arm/xilinx_zynq_a9_qemu targets QEMU's xilinx-zynq-a9 model and
        # also runs on real Zedboard / MicroZed / flight Zynq OBCs.
        BSP=arm/xilinx_zynq_a9_qemu
        ARCH_BSP=arm-rtems${RTEMS_VER}-xilinx_zynq_a9_qemu
        RSB_TARGET=${RTEMS_SERIES}/rtems-arm
        TOOL_GCC=arm-rtems${RTEMS_VER}-gcc
        QEMU=qemu-system-arm
        QEMU_PKG=qemu-system-arm
        QEMU_ARGS=(-M xilinx-zynq-a9 -m 256)
        # Zynq has UART0 + UART1; RTEMS console is UART1, so push the
        # stdio binding past a null UART0.
        SERIAL_ARGS=(-serial null -serial mon:stdio)
        ;;
    aarch64)
        BSP=aarch64/a53_lp64_qemu
        ARCH_BSP=aarch64-rtems${RTEMS_VER}-a53_lp64_qemu
        RSB_TARGET=${RTEMS_SERIES}/rtems-aarch64
        TOOL_GCC=aarch64-rtems${RTEMS_VER}-gcc
        QEMU=qemu-system-aarch64
        QEMU_PKG=qemu-system-arm
        QEMU_ARGS=(-machine virt,gic-version=3 -cpu cortex-a53 -m 4096)
        ;;
    riscv)
        BSP=riscv/rv64imafdc           # RV64GC, lp64d ABI (QEMU virt)
        ARCH_BSP=riscv-rtems${RTEMS_VER}-rv64imafdc
        RSB_TARGET=${RTEMS_SERIES}/rtems-riscv
        TOOL_GCC=riscv-rtems${RTEMS_VER}-gcc
        QEMU=qemu-system-riscv64
        QEMU_PKG=qemu-system-misc
        # -bios none: RTEMS runs in M-mode at 0x80000000; without this
        # QEMU's default OpenSBI firmware loads there too and overlaps.
        QEMU_ARGS=(-M virt -cpu rv64 -m 256 -nographic -bios none)
        ;;
    powerpc)
        echo "rtems.sh: ARCH=powerpc (qoriq_e500) is UNSUPPORTED." >&2
        echo "Known issues: rtems-libbsd does not build cleanly for ppc," >&2
        echo "SDA memory-mapping problems, and additional RTEMS 6 ppc bugs." >&2
        echo "No CI coverage; do not use for release qualification." >&2
        exit 1
        BSP=powerpc/qoriq_e500
        ARCH_BSP=powerpc-rtems${RTEMS_VER}-qoriq_e500
        RSB_TARGET=${RTEMS_SERIES}/rtems-powerpc
        TOOL_GCC=powerpc-rtems${RTEMS_VER}-gcc
        QEMU=qemu-system-ppc
        QEMU_PKG=qemu-system-ppc
        QEMU_ARGS=(-M ppce500 -cpu e500mc -m 1024 -nographic)
        ;;
    *)
        echo "rtems.sh: unknown ARCH=$ARCH" \
             "(expected: sparc, arm, aarch64, riscv, powerpc)" >&2
        exit 1
        ;;
esac

echo "[rtems.sh] ARCH=$ARCH  BSP=$BSP  QEMU=$QEMU ${QEMU_ARGS[*]}"

# 1. Host packages (RSB + arch-specific qemu)
#sudo apt-get update
#sudo apt-get install -y \
#    build-essential git python3 python3-dev \
#    bison flex texinfo unzip pkg-config \
#    libssl-dev libgmp-dev libmpfr-dev libmpc-dev \
#    "$QEMU_PKG"

mkdir -p "$WORK"
cd "$WORK"

# 2. RSB: cross toolchain for the selected ARCH (skip if already built)
if [ ! -x "$RTEMS_PREFIX/bin/$TOOL_GCC" ]; then
    [ -d rtems-source-builder ] || git clone --depth 1 -b "$RSB_BRANCH" \
        https://gitlab.rtems.org/rtems/tools/rtems-source-builder.git
    ( cd rtems-source-builder/rtems && \
      PATH=/usr/bin:/bin:/usr/local/bin \
        ../source-builder/sb-set-builder --prefix="$RTEMS_PREFIX" "$RSB_TARGET" )
fi
export PATH="$RTEMS_PREFIX/bin:$PATH"

# 3. RTEMS kernel + BSP for the selected ARCH (RTEMS 7 lives on main)
if [ ! -d "$WORK/rtems" ]; then
    git clone --depth 1 -b "$RTEMS_KERNEL_BRANCH" \
        https://gitlab.rtems.org/rtems/rtos/rtems.git
fi
cd rtems
cat > config.ini <<EOF
[$BSP]
BUILD_TESTS = False
RTEMS_POSIX_API = True
EOF
( ./waf configure --prefix="$RTEMS_PREFIX" --rtems-bsps="$BSP" && \
  ./waf && ./waf install )

cd "$WORK"
# 4. rtems-libbsd — FreeBSD-derived TCP/IP stack for RTEMS.  Provides the
# BSD sockets API (socket()/bind()/sendto()/...), getaddrinfo() and the
# loopback interface that ION's loopback UDP/LTP test relies on.
if [ ! -d "$WORK/rtems-libbsd" ]; then
    git clone -b "$LIBBSD_BRANCH" \
        https://gitlab.rtems.org/rtems/pkg/rtems-libbsd.git
fi
cd "$WORK/rtems-libbsd"
git submodule update --init        # idempotent (pulls rtems_waf)

./waf configure --prefix="$RTEMS_PREFIX" \
    --rtems-bsps="$BSP" \
    --buildset=buildset/default.ini
./waf
./waf install

# 5. Build ION's RTEMS port
cd "$ION_DIR"
git submodule update --init --force contrib/rtems_waf
mkdir -p rtems-test-output
(
    cd arch-rtems
    ./waf clean || true
    ./waf configure --rtems="$RTEMS_PREFIX" --rtems-version="$RTEMS_VER" \
        --rtems-bsp="$BSP"
    ./waf build
) 2>&1 | tee rtems-test-output/build-log.txt

# 6. Run in QEMU (exit 124 or 143 from `timeout` is fine)
set +e

# run interactively
"$QEMU" \
    -no-reboot -nographic "${SERIAL_ARGS[@]}" \
    "${QEMU_ARGS[@]}" \
    -kernel "arch-rtems/build/$ARCH_BSP/ion.exe" \
    </dev/null

exit
