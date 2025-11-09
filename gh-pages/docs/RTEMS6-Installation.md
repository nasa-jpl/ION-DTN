# ION on RTEMS 6.1

## Status

**ION 4.1.4+ RTEMS 6.1 ARM64 Port**: **FULLY OPERATIONAL**

- Successfully tested with UDP networking in QEMU emulation
- Minimal but fully functional BP + LTP implementation
- Template/reference implementation for QEMU ARM64 BSP
- Requires adaptation to specific hardware platforms

## Key Features

- Bundle Protocol v7 (BP) with IPN scheme
- Licklider Transmission Protocol (LTP) over UDP
- RTEMS libbsd (FreeBSD network stack) integration
- ION Admin Public API (no configuration files)
- Optional modules: DGR, BSSP, CFDP
- 64-bit addressing (SPACE_ORDER=3 for AArch64)

## Installation and Documentation

**For complete installation instructions, implementation details, and troubleshooting**, please refer to the detailed documentation in the `arch-rtems/` directory of the ION source distribution:

### Primary Documentation Files

**[arch-rtems/README](https://github.com/nasa-jpl/ION-DTN/blob/current/arch-rtems/README)**

   - Complete installation and build instructions
   - Prerequisites and environment setup
   - BSP configuration and adaptation guide
   - Testing and verification procedures
   - Platform-specific customization

**[arch-rtems/KEY-FIXES-SUMMARY.md](https://github.com/nasa-jpl/ION-DTN/blob/current/arch-rtems/KEY-FIXES-SUMMARY.md)**

   - Critical fixes and workarounds
   - Troubleshooting common issues
   - Platform-specific considerations

**[arch-rtems/UDP-NETWORK-STATUS.md](https://github.com/nasa-jpl/ION-DTN/blob/current/arch-rtems/UDP-NETWORK-STATUS.md)**

   - Network stack integration details
   - UDP configuration and testing
   - Network troubleshooting

### Quick Start Overview

The basic installation workflow is:

```bash
# Navigate to RTEMS port directory
cd ion-open-source-X.Y.Z/arch-rtems

# Create symbolic links to ION source
./srclinks

# Configure build for your BSP
./waf configure --rtems=$RTEMS_ROOT --rtems-bsp=aarch64/a53_lp64_qemu

# Build ION
./waf build

# Test in QEMU (example)
qemu-system-aarch64 -M raspi3b -m 1G -kernel build/ionrtems.exe \
  -serial null -serial mon:stdio -nographic
```

**See [arch-rtems/README](https://github.com/nasa-jpl/ION-DTN/blob/current/arch-rtems/README) for detailed instructions and requirements.**

## Important Notes

- **Template Port**: This is a reference implementation that requires adaptation to your specific hardware and BSP
- **Prerequisites**: RTEMS 6.1 toolchain, BSP with libbsd support, Python 3
- **Default Configuration**: Loopback testing on 127.0.0.1
- **Customization Required**: Network interface, memory sizes, BSP configuration, system clock

## Getting Help

- **Installation Issues**: Consult arch-rtems/README and KEY-FIXES-SUMMARY.md
- **Network Configuration**: See arch-rtems/UDP-NETWORK-STATUS.md
- **Bug Reports**: ION GitHub Issues
- **RTEMS Questions**: RTEMS Users Mailing List

---

**For all installation details, please refer to the documentation files in the `arch-rtems/` directory of your ION distribution.**
