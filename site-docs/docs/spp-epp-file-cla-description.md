# CCSDS Space Packet Protocol (SPP), Encapsulation Packet Protocol (EPP), and File CLAs in ION

**February 2026**

## Overview

ION provides three plugin-based Convergence Layer Adapters (CLAs) that use dynamic library loading to interface with provider libraries at runtime:

- **SPP CLA**: Space Packet Protocol CLA for transmitting bundles via CCSDS Space Packets (CCSDS 133.0-B-2)
- **EPP CLA**: Encapsulation Packet Protocol CLA for transmitting bundles via CCSDS Encapsulation Packets (CCSDS 734.20-O-1)
- **File CLA**: File-based CLA for transmitting bundles via file storage (store-and-forward / sneakernet scenarios)

All three CLAs use the same plugin architecture that allows users to provide their own provider libraries. ION includes stub/loopback libraries for testing purposes.

---

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

---

## EPP CLA Description

The Encapsulation Packet Protocol provides a simpler interface for transmitting bundles over CCSDS Space Data Link Protocol (SDLP) channels. The service primitives are:

- **ENCAPSULATION.request** (Data, Length, SDLP Channel, EPI)
- **ENCAPSULATION.indication** (Data, SDLP Channel, EPI)

### EPP Characteristics

- Maximum bundle size: **4,294,967,287 bytes** per CCSDS 734.20-O-1 (ION uses a practical 1MB buffer)
- Encapsulation Protocol Identifier (EPI) for Bundle Protocol: **4** (registered in SANA)
- Simpler than SPP with no sequence counting required at the CLA level

---

## File CLA Description

The File CLA enables bundle transmission via file storage, supporting store-and-forward and "sneakernet" scenarios where bundles are physically transported via storage media.

### File CLA Characteristics

- **Use case**: Store-and-forward, air-gapped networks, removable media transport
- **Provider library**: Handles file I/O, framing, and integrity checking
- **Flexible file modes**: Same file (loopback) or separate files (send/receive)
- **Polling-based receiver**: CLI polls for new data at configurable intervals

### Default Frame Format (Simple Provider)

The included `libfile_simple_provider` uses length-prefixed framing with CRC32 integrity checking:

```
+----------------+----------------+------------------+----------------+
| 4-byte length  | 4-byte CRC32   |   Bundle Data    | 4-byte CRC32   |
| (big-endian)   | (of length)    |   (N bytes)      | (of bundle)    |
+----------------+----------------+------------------+----------------+
```

- **12 bytes overhead** per bundle
- **Length CRC**: Detects corrupt length before attempting large reads
- **Data CRC**: Ensures bundle integrity after storage/transport
- **Standard CRC-32**: IEEE 802.3 polynomial (0x04C11DB7)

---

## Design Considerations

- In actual deployment, the provider is external software/hardware available in a user's platform
- The CLAs use dynamic library loading (`dlopen`) to interface with provider libraries at runtime
- The provider library handles the means of transferring data (TCP, UDP, serial, SpaceWire, file I/O, etc.)
- The CLA is not responsible for configuring communications below the protocol layer

### Architecture Overview

**Sender Side:**
- BP &rarr; CLA (sppclo/eppclo/fileclo) &rarr; Provider Library &rarr; Underlying Transport/Storage

**Receiver Side:**
- BP &larr; CLA (sppcli/eppcli/filecli) &larr; Provider Library &larr; Underlying Transport/Storage

---

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

### File CLA

| Executable | Description |
|------------|-------------|
| `filecli`  | File Convergence Layer Input daemon (receiver) |
| `fileclo`  | File Convergence Layer Output daemon (sender) |

---

## Stub/Loopback Libraries for Testing

ION includes provider libraries that enable testing the CLAs without real hardware. These libraries are built when running `make buildcheck`.

### Available Stub Libraries

| Library | Location | Description |
|---------|----------|-------------|
| `libspp_loopback_provider.so` | `bpv7/spp/stub/.libs/` | SPP loopback via named pipe |
| `libepp_loopback_provider.so` | `bpv7/epp/stub/.libs/` | EPP loopback via named pipe |
| `libfile_simple_provider.so` | `bpv7/file/stub/.libs/` | File I/O with CRC32 framing |

### Running the Loopback/Test Scripts

```bash
# From the ION build directory

# SPP Loopback Test
cd tests/loopback-spp
./dotest

# EPP Loopback Test
cd tests/loopback-epp
./dotest

# File CLA Loopback Test (same file for send/receive)
cd tests/loopback-file
./dotest

# File CLA Transfer Test (separate files, simulates sneakernet)
cd tests/file-transfer
./dotest
```

---

## Configuration in bprc

### Adding the Protocol

```
# For SPP (payload_bytes, overhead_bytes)
a protocol spp 1400 100

# For EPP
a protocol epp 1400 100

# For File (payload_bytes, overhead_bytes)
a protocol file 65536 12
```

### Adding Inducts (Receivers)

```
# SPP induct
a induct spp <duct_name> 'sppcli <duct_name> <library_path>'

# EPP induct
a induct epp <duct_name> 'eppcli <duct_name> <library_path>'

# File induct
a induct file <duct_name> 'filecli <duct_name> <library_path> <input_file_path> [poll_interval_ms]'
```

### Adding Outducts (Senders)

```
# SPP outduct - config format: APID,seq_count,packet_type,sec_header_flag
a outduct spp <duct_name> 'sppclo <duct_name> <library_path> <apid>,<seq_count>,<packet_type>,<sec_header_flag>'

# EPP outduct
a outduct epp <duct_name> 'eppclo <duct_name> <library_path> <sdlp_channel>'

# File outduct
a outduct file <duct_name> 'fileclo <duct_name> <library_path> <output_file_path>'
```

---

## Configuration Parameters

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

### File CLA Configuration Parameters

| Parameter | Description | Default |
|-----------|-------------|---------|
| `library_path` | Path to the provider shared library | (required) |
| `input_file_path` | File path for CLI to read bundles from | (required for CLI) |
| `output_file_path` | File path for CLO to write bundles to | (required for CLO) |
| `poll_interval_ms` | How often CLI checks for new data (milliseconds) | 1000 |

---

## Example Configurations

### SPP Loopback Example (bprc)

```
1
a scheme ipn 'ipnfw' 'ipnadminep'
a endpoint ipn:1.1 q
a protocol spp 1400 100
a induct spp loopback 'sppcli loopback /path/to/libspp_loopback_provider.so'
a outduct spp loopback 'sppclo loopback /path/to/libspp_loopback_provider.so 123,0,0,0'
```

### EPP Loopback Example (bprc)

```
1
a scheme ipn 'ipnfw' 'ipnadminep'
a endpoint ipn:1.1 q
a protocol epp 1400 100
a induct epp loopback 'eppcli loopback /path/to/libepp_loopback_provider.so'
a outduct epp loopback 'eppclo loopback /path/to/libepp_loopback_provider.so 42'
```

### File CLA Loopback Example (bprc)

```
1
a scheme ipn 'ipnfw' 'ipnadminep'
a endpoint ipn:1.1 q
a protocol file 65536 12
a induct file loopback 'filecli loopback /path/to/libfile_simple_provider.so /tmp/bundles.dat 500'
a outduct file loopback 'fileclo loopback /path/to/libfile_simple_provider.so /tmp/bundles.dat'
```

### File CLA Store-and-Forward Example (bprc)

For separate send/receive files (sneakernet scenario):

```
1
a scheme ipn 'ipnfw' 'ipnadminep'
a endpoint ipn:1.1 q
a protocol file 65536 12
# CLI reads from inbound file
a induct file transfer 'filecli transfer /path/to/libfile_simple_provider.so /mnt/usb/inbound.dat 500'
# CLO writes to outbound file
a outduct file transfer 'fileclo transfer /path/to/libfile_simple_provider.so /mnt/usb/outbound.dat'
```

---

## Configuration in ipnrc

Add egress plans to route bundles through the outducts:

```
# For SPP
a plan <destination_node> spp/<duct_name>

# For EPP
a plan <destination_node> epp/<duct_name>

# For File
a plan <destination_node> file/<duct_name>
```

### Example ipnrc Configuration

```
# Route bundles to node 1 via file outduct
a plan 1 file/loopback
```

---

## Implementing a Custom Provider Library

To use these CLAs with real hardware or custom transport, implement a provider library that exports the required functions.

### SPP Provider Library Functions

```c
void init_space_packet_sender(void);
void finalize_space_packet_sender(void);
int packet_request(unsigned char *buffer, int apid, int seq_count,
                   int packet_type, int sec_header_flag, size_t total_length);
size_t packet_indication(char *buffer, int *received_apid);
```

### EPP Provider Library Functions

```c
void init_epp_sender(void);
void finalize_epp_sender(void);
int encapsulation_request(unsigned char *data, size_t length,
                          int sdlp_channel, int epi);
size_t encapsulation_indication(char *buffer, int *sdlp_channel, int *epi);
```

### File Provider Library Functions

```c
/* Initialize sender - called once at startup */
int init_file_sender(const char *file_path);

/* Cleanup sender - called at shutdown */
void finalize_file_sender(void);

/* Write a bundle to the file
 * Returns: bytes written, or negative on error */
int file_write_bundle(unsigned char *data, size_t length);

/* Initialize receiver - called once at startup */
int init_file_receiver(const char *file_path, int poll_interval_ms);

/* Cleanup receiver - called at shutdown */
void finalize_file_receiver(void);

/* Read a bundle from the file
 * Returns: >1 = bytes read, 1 = normal stop, 0 = error */
size_t file_read_bundle(char *buffer);
```

### Building Your Provider Library

```bash
# Linux
gcc -shared -fPIC -o libmyprovider.so myprovider.c

# macOS
gcc -shared -fPIC -o libmyprovider.dylib myprovider.c
```

---

## Compatibility Notes

- **Embedded RTOS**: FreeRTOS does not support `dlopen`. Custom embedded Linux systems (Yocto, Buildroot) should work.
- **Security**: Security mechanisms may need configuration to allow `dlopen` (e.g., SELinux).
- **Dependencies**: Any libraries used by your provider must be available when running the CLA daemons.

---

## Test Configurations

| Configuration | Location |
|--------------|----------|
| SPP Loopback Test | `tests/loopback-spp/` |
| EPP Loopback Test | `tests/loopback-epp/` |
| File Loopback Test | `tests/loopback-file/` |
| File Transfer Test | `tests/file-transfer/` |
| SPP Two-Node Demo | `configs/two-node-spp/` |
