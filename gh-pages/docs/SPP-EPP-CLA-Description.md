# CCSDS Space Packet Protocol (SPP) and Encapsulation Packet Protocol (EPP) CLAs in ION

**January 2026**

## Overview

ION provides two Convergence Layer Adapters (CLAs) for interfacing with CCSDS space link protocols:

- **SPP CLA**: Space Packet Protocol CLA for transmitting bundles via CCSDS Space Packets (CCSDS 133.0-B-2)
- **EPP CLA**: Encapsulation Packet Protocol CLA for transmitting bundles via CCSDS Encapsulation Packets (CCSDS 734.20-O-1)

Both CLAs use a plugin architecture that allows users to provide their own protocol provider libraries. ION includes loopback stub libraries for testing purposes.

## SPP CLA Description

In the CCSDS Bundle Protocol Version 7 Orange Book, the Bundle Protocol Agent (BPA) interacts with an underlying Space Packet service through the following service primitives:

- **OCTET_STRING.request** (Octet String, APID, Secondary Header Indicator, Packet Type, Packet Sequence Count/Packet Name)
- **OCTET_STRING.indication** (Octet String, APID, Secondary Header Indicator, Data Loss Indicator (optional))

### SPP Constraints

- Maximum bundle size: **65,536 bytes** (minus the size of packet secondary header)
- Octet string shall be a single CBOR serialized bundle
- The specific link destination determines the APID
- Packet Secondary Header Indicator shall be set to absent
- Packet Sequence Count shall always be used instead of a Packet Name

**NOTE**: Additional configuration and service limitations for an SPP provider instance can be found in ANNEX B of the CCSDS BPv7 Orange Book.

## EPP CLA Description

The Encapsulation Packet Protocol provides a simpler interface for transmitting bundles over CCSDS Space Data Link Protocol (SDLP) channels. The service primitives are:

- **ENCAPSULATION.request** (Data, Length, SDLP Channel, EPI)
- **ENCAPSULATION.indication** (Data, SDLP Channel, EPI)

### EPP Characteristics

- Maximum bundle size: **4,294,967,287 bytes** per CCSDS 734.20-O-1 (ION uses a practical 1MB buffer)
- Encapsulation Protocol Identifier (EPI) for Bundle Protocol: **4** (registered in SANA)
- Simpler than SPP with no sequence counting required at the CLA level

## Design Considerations

- In actual deployment, the SPP/EPP provider is external software/hardware available in a user's platform
- The CLAs use dynamic library loading (`dlopen`) to interface with provider libraries at runtime
- The provider library handles the means of transferring packets to a peer entity (TCP, UDP, serial, SpaceWire, etc.)
- The CLA is not responsible for configuring communications below the SPP/EPP layer

### Architecture Overview

**Sender Side:**
- BP &rarr; SPP/EPP CLA &rarr; Provider Library &rarr; Underlying Transport

**Receiver Side:**
- BP &larr; SPP/EPP CLA &larr; Provider Library &larr; Underlying Transport

## CLA Executables

### SPP CLA

| Executable | Description |
|------------|-------------|
| `sppcli`   | SPP Convergence Layer Input daemon (receiver) |
| `sppclo`   | SPP Convergence Layer Output daemon (sender) |

### EPP CLA

| Executable | Description |
|------------|-------------|
| `eppcli`   | EPP Convergence Layer Input daemon (receiver) |
| `eppclo`   | EPP Convergence Layer Output daemon (sender) |

## Loopback Stub Libraries for Testing

ION includes loopback provider libraries that enable testing the CLAs without real space link hardware. These libraries use named pipes (FIFOs) for local loopback communication.

### Available Stub Libraries

| Library | FIFO Path | Purpose |
|---------|-----------|---------|
| `libspp_loopback_provider.so` | `/tmp/spp_loopback_fifo` | SPP CLA loopback testing |
| `libepp_loopback_provider.so` | `/tmp/epp_loopback_fifo` | EPP CLA loopback testing |

These libraries are built when running `make buildcheck` and are located in the build directory under:
- `bpv7/spp/stub/.libs/` (SPP)
- `bpv7/epp/stub/.libs/` (EPP)

### Running the Loopback Tests

```bash
# From the ION build directory
cd tests/loopback-spp
./dotest

cd tests/loopback-epp
./dotest
```

## Configuration in bprc

### Adding the Protocol

```
# For SPP (payload_bytes, overhead_bytes)
a protocol spp 1400 100

# For EPP
a protocol epp 1400 100
```

### Adding Inducts (Receivers)

```
# SPP induct
a induct spp <duct_name> 'sppcli <duct_name> <library_path>'

# EPP induct
a induct epp <duct_name> 'eppcli <duct_name> <library_path>'
```

### Adding Outducts (Senders)

```
# SPP outduct - config format: APID,seq_count,packet_type,sec_header_flag
a outduct spp <duct_name> 'sppclo <duct_name> <library_path> <apid>,<seq_count>,<packet_type>,<sec_header_flag>'

# EPP outduct
a outduct epp <duct_name> 'eppclo <duct_name> <library_path> <sdlp_channel>'
```

### SPP Configuration Parameters

| Parameter | Description | Valid Values |
|-----------|-------------|--------------|
| `apid` | Application Process ID | 0-2047 (11 bits) |
| `seq_count` | Initial sequence count | 0-16383 (14 bits, auto-increments) |
| `packet_type` | Packet type | 0=TM, 1=TC |
| `sec_header_flag` | Secondary header flag | 0=absent, 1=present |

### EPP Configuration Parameters

| Parameter | Description |
|-----------|-------------|
| `sdlp_channel` | SDLP channel identifier (mission-specific integer) |

### Example bprc Configuration (SPP Loopback)

```
# Initialization
1

# Add scheme
a scheme ipn 'ipnfw' 'ipnadminep'

# Add endpoints
a endpoint ipn:1.1 q
a endpoint ipn:1.2 q

# Add SPP protocol
a protocol spp 1400 100

# Add induct (receiver)
a induct spp loopback 'sppcli loopback /path/to/libspp_loopback_provider.so'

# Add outduct (sender) - APID=123, seq_count=0, packet_type=0, sec_header_flag=0
a outduct spp loopback 'sppclo loopback /path/to/libspp_loopback_provider.so 123,0,0,0'
```

### Example bprc Configuration (EPP Loopback)

```
# Initialization
1

# Add scheme
a scheme ipn 'ipnfw' 'ipnadminep'

# Add endpoints
a endpoint ipn:1.1 q
a endpoint ipn:1.2 q

# Add EPP protocol
a protocol epp 1400 100

# Add induct (receiver)
a induct epp loopback 'eppcli loopback /path/to/libepp_loopback_provider.so'

# Add outduct (sender) - SDLP channel = 42
a outduct epp loopback 'eppclo loopback /path/to/libepp_loopback_provider.so 42'
```

## Configuration in ipnrc

Add egress plans to route bundles through the SPP or EPP outducts:

```
# For SPP
a plan <destination_node> spp/<duct_name>

# For EPP
a plan <destination_node> epp/<duct_name>
```

### Example ipnrc Configuration

```
# Route bundles to node 1 via SPP loopback outduct
a plan 1 spp/loopback

# Or for EPP
a plan 1 epp/loopback
```

## Implementing a Custom Provider Library

To use the SPP or EPP CLA with real hardware or custom transport, you must implement a provider library that exports the required functions.

### SPP Provider Library Functions

Your library must export these functions with exact names:

```c
/* Called once at startup to initialize sender resources */
void init_space_packet_sender(void);

/* Called once at shutdown to clean up sender resources */
void finalize_space_packet_sender(void);

/* Send a space packet (OCTET_STRING.request primitive)
 * Returns: total bytes sent including SPP header, or negative on error */
int packet_request(unsigned char *buffer, int apid, int seq_count,
                   int packet_type, int sec_header_flag, size_t total_length);

/* Receive a space packet (OCTET_STRING.indication primitive)
 * Returns: bundle data length, 0 for error, 1 for normal stop */
size_t packet_indication(char *buffer, int *received_apid);
```

### EPP Provider Library Functions

Your library must export these functions with exact names:

```c
/* Called once at startup to initialize sender resources */
void init_epp_sender(void);

/* Called once at shutdown to clean up sender resources */
void finalize_epp_sender(void);

/* Send an encapsulation packet (ENCAPSULATION.request primitive)
 * Returns: number of bytes sent, or negative on error */
int encapsulation_request(unsigned char *data, size_t length,
                          int sdlp_channel, int epi);

/* Receive an encapsulation packet (ENCAPSULATION.indication primitive)
 * Returns: number of bytes received, 0 for error, 1 for normal stop */
size_t encapsulation_indication(char *buffer, int *sdlp_channel, int *epi);
```

### Building Your Provider Library

Compile your provider as a shared library:

```bash
# Linux
gcc -shared -fPIC -o libmyprovider.so myprovider.c

# macOS
gcc -shared -fPIC -o libmyprovider.dylib myprovider.c
```

Ensure functions are exported with `extern` (default for C functions) and use the exact function names specified above.

## Compatibility Notes

- **Embedded RTOS**: Compatibility may be limited. FreeRTOS does not support `dlopen`. Custom embedded Linux systems built using Yocto or Buildroot should be supported.
- **Security**: Security mechanisms may need to be configured to allow `dlopen` (e.g., SELinux denials).
- **Dependencies**: Any libraries used by your provider must be available when running the CLA daemons.

## Test Configurations

Template configurations are available in the ION source:

| Configuration | Location |
|--------------|----------|
| SPP Loopback Test | `tests/loopback-spp/` |
| EPP Loopback Test | `tests/loopback-epp/` |
| SPP Two-Node Demo | `configs/two-node-spp/` |
| SPP Loopback Config | `configs/loopback-spp/` |
| EPP Loopback Config | `configs/loopback-epp/` |
