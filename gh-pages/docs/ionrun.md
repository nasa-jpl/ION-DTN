# ionrun - Interactive ION Setup and Launch Utility

`ionrun` is a command-line utility that simplifies setting up and running ION. It provides an interactive wizard that generates the ION configuration files needed for common network topologies, and then launches ION in a user-specified working directory.

- [ionrun - Interactive ION Setup and Launch Utility](#ionrun---interactive-ion-setup-and-launch-utility)
  - [Overview](#overview)
  - [Usage](#usage)
  - [Interactive Wizard](#interactive-wizard)
    - [Topology Types](#topology-types)
    - [Node Configuration](#node-configuration)
    - [Convergence Layers](#convergence-layers)
    - [CFDP (File Transfer)](#cfdp-file-transfer)
    - [Port Numbers](#port-numbers)
  - [Generated Files](#generated-files)
    - [ionrun.rc](#ionrunrc)
    - [ionrun.meta](#ionrunmeta)
    - [ionrun.ionconfig (same-host only)](#ionrunionconfig-same-host-only)
    - [Per-Node Subdirectories](#per-node-subdirectories)
  - [Examples](#examples)
    - [Loopback Test](#loopback-test)
    - [Two-Node Network](#two-node-network)
    - [Three-Node Relay with Mixed Convergence Layers](#three-node-relay-with-mixed-convergence-layers)
    - [Two Nodes on the Same Host](#two-nodes-on-the-same-host)
    - [Three Nodes on the Same Host](#three-nodes-on-the-same-host)
    - [Custom Port Numbers](#custom-port-numbers)
    - [Regenerate Configuration](#regenerate-configuration)
  - [Multi-Node Workflow](#multi-node-workflow)
    - [Remote (Multi-Host)](#remote-multi-host)
    - [Same-Host](#same-host)
  - [Environment Variables](#environment-variables)
  - [Pre-Existing Configurations](#pre-existing-configurations)
  - [How It Works](#how-it-works)

## Overview

`ionrun` eliminates the need to manually write ION configuration files for common scenarios. It:

1. Creates a working directory for ION runtime files
2. Asks a series of questions about the desired network topology
3. Generates a combined configuration file (`ionrun.rc`) compatible with `ionstart -I`
4. Optionally starts ION immediately

On subsequent runs in the same directory, it detects the existing configuration and starts ION directly. The `--regenerate` option can rewrite config files from saved metadata without re-running the wizard, which is useful after updating `ionrun` itself.

## Usage

```
ionrun [options] <working_directory>

Options:
  -h, --help            Show help message
  -s, --stop            Stop ION in the working directory (runs ionstop)
  -n, --node <name>     Start a specific node (skip interactive selection)
  -g, --generate-only   Generate config files without starting ION
  -f, --force           Regenerate config even if one already exists
  -r, --regenerate      Regenerate config from existing ionrun.meta (no wizard)
```

## Interactive Wizard

When `ionrun` is pointed at an empty or new directory (or when `--force` is used), it runs an interactive setup wizard.

### Topology Types

The wizard supports five topologies:

| Topology | Nodes | Description |
|----------|-------|-------------|
| **Loopback** | 1 | Single node sending to itself. Good for initial testing. |
| **2-node** | 2 | Two nodes on distinct hosts. Basic point-to-point link. |
| **3-node** | 3 | Three nodes in a linear chain (1--2--3) on distinct hosts. Node 2 acts as a relay. |
| **2-node (same host)** | 2 | Two ION instances on the same machine, using per-node subdirectories with unique shared memory keys and ports. |
| **3-node (same host)** | 3 | Three ION instances on the same machine in a linear chain. Node 2 acts as a relay. |

### Node Configuration

For each node, the wizard asks:

- **Name**: A label for the node (e.g., `host1`). Used as the ionstart tag for remote multi-node configs, and as the subdirectory name for same-host configs. Must contain only letters, digits, hyphens, and underscores.
- **IPN Node ID**: The node's IPN identifier. Can be a simple integer (e.g., `1`) or an allocator.node pair (e.g., `5.1`) for 3-part IPN addressing.
- **IP Address**: The node's network address. Defaults to `127.0.0.1`. For remote multi-node topologies, use the actual IP of each host. For loopback and same-host topologies, the IP is automatically set to `127.0.0.1`.

### Convergence Layers

Three convergence layers are supported:

| CL | Protocol Name | Transport | Default Port | CLI Programs |
|----|--------------|-----------|--------------|-------------|
| **LTP** | `ltp` | UDP | 1113 | `ltpcli` / `ltpclo` |
| **TCP** | `tcp` | TCP | 4556 | `tcpcli` / `tcpclo` |
| **UDP** | `udp` | UDP | 4556 | `udpcli` / `udpclo` |

- For **loopback**, **2-node**, and **same-host** topologies, one convergence layer is selected for the entire network.
- For **3-node** (remote) topologies, two convergence layers are selected independently: one for the link between nodes 1-2, and another for the link between nodes 2-3. This allows mixed-CL networks (e.g., LTP on one hop and TCP on the other).

For **same-host** topologies, each node is automatically assigned a unique listening port starting from the base port. For example, with a base port of 1113: node1 gets 1113, node2 gets 1114, node3 gets 1115.

### CFDP (File Transfer)

For non-loopback topologies, the wizard asks whether to enable CFDP (CCSDS File Delivery Protocol). When enabled, `ionrun` generates a `cfdpadmin` configuration section for each node that includes:

- CFDP initialization
- Entity entries for all peer nodes (using BP transport, RTT of 7 seconds)
- Default settings: discard incomplete files on cancellation, 65,000-byte segment size
- `bputa` (BP UT Adapter) startup

The BP endpoints required by CFDP (service numbers 64 and 65) are always registered in the `bpadmin` section for all topologies.

CFDP is not offered for loopback topologies due to complications with single-node CFDP operation.

Once ION is running with CFDP enabled, you can use `cfdptest` interactively to send and receive files. Note that CFDP resolves relative file paths against the ION node's working directory, so use absolute paths when running `cfdptest` from a different directory.

### Port Numbers

After selecting each convergence layer, the wizard prompts for a port number. Press Enter to accept the IANA-registered default:

- **LTP**: 1113 (IANA registered for Licklider Transmission Protocol)
- **TCP/UDP**: 4556 (IANA registered for DTN TCP Convergence Layer)

Custom ports are useful when running alongside other services or when firewall rules require specific ports.

## Generated Files

### ionrun.rc

The main configuration file, compatible with `ionstart -I`. It uses the combined config format with `## begin`/`## end` section markers.

- **Loopback**: Sections have no tags. Run with `ionstart -I ionrun.rc`.
- **Remote multi-node**: Sections are tagged with node names. Run with `ionstart -I ionrun.rc -t <nodename>`.
- **Same-host multi-node**: Each node gets its own `ionrun.rc` in a subdirectory. Sections have no tags (each file is standalone). Run with `ionstart -I ionrun.rc` from the node subdirectory.

The tag mechanism is a built-in feature of `ionstart.awk` that allows multiple nodes' configurations to coexist in a single file.

### ionrun.meta

A key-value metadata file that stores the topology parameters so `ionrun` knows how to re-launch ION on subsequent runs. Example for remote multi-node:

```
topology=2node
node_count=2
node1_name=host1
node1_id=1
node1_ip=10.0.0.1
node2_name=host2
node2_id=2
node2_ip=10.0.0.2
cl1=tcp
port1=4556
```

Example for same-host multi-node:

```
topology=2node-local
node_count=2
node1_name=node1
node1_id=1
node1_ip=127.0.0.1
node2_name=node2
node2_id=2
node2_ip=127.0.0.1
cl1=ltp
port1=1113
node1_port=1113
node2_port=1114
```

### ionrun.ionconfig (same-host only)

For same-host topologies, each node subdirectory contains an `ionrun.ionconfig` file with unique shared memory keys, SDR working memory keys, and SDR names to prevent conflicts between ION instances:

```
wmKey 10001
sdrWmKey 65281
sdrName ion1
wmSize 50000000
configFlags 1
heapWords 10000000
pathName /tmp
```

Each node gets a unique `sdrWmKey` (derived from `SDR_SM_KEY + node_number`) so that multiple ION instances on the same host use separate SDR working memory segments.

The directory layout for a 2-node same-host topology:

```
workdir/
  ionrun.meta
  node1/
    ionrun.rc
    ionrun.ionconfig    # wmKey 10001, sdrWmKey 65281, sdrName ion1
  node2/
    ionrun.rc
    ionrun.ionconfig    # wmKey 10002, sdrWmKey 65282, sdrName ion2
```

### Per-Node Subdirectories

In addition to the combined `ionrun.rc`, ionrun generates per-node subdirectories containing individual config files split by admin program and a `start_<name>.sh` script. This applies to all topologies (including loopback).

Each node subdirectory contains:

| File | Description |
|------|-------------|
| `ionrun.ionrc` | ionadmin commands (init, contacts, ranges) |
| `ionrun.ionsecrc` | ionsecadmin commands (security init) |
| `ionrun.ltprc` | ltpadmin commands (only if LTP is used) |
| `ionrun.bprc` | bpadmin commands (includes `r 'ipnadmin ionrun.ipnrc'` before `s`) |
| `ionrun.ipnrc` | ipnadmin commands (routing groups) |
| `ionrun.cfdprc` | cfdpadmin commands (only if CFDP is enabled) |
| `start_<name>.sh` | Startup script using direct admin calls |

The `start_<name>.sh` script follows the standard ION convention used in tests and demos: it calls each admin program directly in the correct order. Routing (`ipnadmin`) is invoked from within the `bprc` file via the `r` command, ensuring routes are loaded before BP starts. For example:

```bash
#!/usr/bin/env bash
# Start ION node 'node1' - generated by ionrun
cd "$(dirname "$0")"
ionadmin    ionrun.ionrc
sleep 1
ionsecadmin ionrun.ionsecrc
sleep 1
ltpadmin    ionrun.ltprc
sleep 1
bpadmin     ionrun.bprc
sleep 1
```

Directory layout for a 2-node remote topology:

```
workdir/
  ionrun.rc           # combined file with tags
  ionrun.meta
  node1/
    ionrun.ionrc
    ionrun.ionsecrc
    ionrun.ltprc
    ionrun.bprc
    ionrun.ipnrc
    start_node1.sh
  node2/
    ionrun.ionrc
    ionrun.ionsecrc
    ionrun.ltprc
    ionrun.bprc
    ionrun.ipnrc
    start_node2.sh
```

These scripts can be used independently of ionrun — for example, to start a node manually or to integrate with existing test harnesses.

## Examples

### Loopback Test

The simplest way to verify ION is working:

```bash
$ ionrun ~/ion-loopback
=== ION Configuration Wizard ===

Select network topology:
  1) Loopback (single node)
  2) 2-node (two hosts)
  3) 3-node (three hosts, linear chain)
  4) 2-node (same host)
  5) 3-node (same host, linear chain)

Topology [1-5]: 1

--- Node 1 ---
  Name [node1]:
  IPN node ID [1]:
  IP address: 127.0.0.1 (loopback)

Select convergence layer:
  Convergence layer:
    1) ltp
    2) tcp
    3) udp
  Choice [1-3]: 1
  Port [1113]:

Configuration written to:
  /home/user/ion-loopback/ionrun.rc
  /home/user/ion-loopback/ionrun.meta

Starting ION (loopback) in /home/user/ion-loopback ...

To operate on this ION instance from any directory, run:
  export ION_NODE_WDNAME=/home/user/ion-loopback
```

Once ION is running, test with:

```bash
# In one terminal:
bpsink ipn:1.1 &

# In another terminal:
echo "Hello DTN" | bpsource ipn:1.1
```

To stop: `ionrun -s ~/ion-loopback`

### Two-Node Network

Set up a TCP link between two hosts (10.0.0.1 and 10.0.0.2):

```bash
# Generate configs (run on either host)
$ ionrun -g ~/ion-2node
Topology [1-5]: 2
--- Node 1 ---
  Name [node1]: alpha
  IPN node ID [1]: 1
  IP address [127.0.0.1]: 10.0.0.1
--- Node 2 ---
  Name [node2]: beta
  IPN node ID [2]: 2
  IP address [127.0.0.1]: 10.0.0.2
  Choice [1-3]: 2
  Port [4556]:
```

Copy the `~/ion-2node` directory to both hosts, then:

```bash
# On host 10.0.0.1:
ionrun -n alpha ~/ion-2node

# On host 10.0.0.2:
ionrun -n beta ~/ion-2node
```

### Three-Node Relay with Mixed Convergence Layers

Create a 3-node network where LTP connects nodes 1-2 and TCP connects nodes 2-3:

```bash
$ ionrun -g ~/ion-3node
Topology [1-5]: 3
--- Node 1 ---
  Name [node1]: earth
  IPN node ID [1]: 1
  IP address [127.0.0.1]: 10.1.1.1
--- Node 2 ---
  Name [node2]: relay
  IPN node ID [2]: 2
  IP address [127.0.0.1]: 10.1.1.2
--- Node 3 ---
  Name [node3]: mars
  IPN node ID [3]: 3
  IP address [127.0.0.1]: 10.1.1.3
Select convergence layer between earth and relay:
  Choice [1-3]: 1
  Port [1113]:
Select convergence layer between relay and mars:
  Choice [1-3]: 2
  Port [4556]:
```

The relay node (node 2) automatically gets both LTP and TCP convergence layers configured. Routing between non-adjacent nodes (earth-to-mars) is handled via static group routes through the relay.

### Two Nodes on the Same Host

Run two ION instances on one machine for local testing:

```bash
$ ionrun -g ~/ion-2local
Topology [1-5]: 4
--- Node 1 ---
  Name [node1]:
  IPN node ID [1]:
  IP address: 127.0.0.1 (loopback)
--- Node 2 ---
  Name [node2]:
  IPN node ID [2]:
  IP address: 127.0.0.1 (loopback)
Select convergence layer (all links):
  Choice [1-3]: 1
  Port [1113]:
  Port assignments:
    node1: 1113
    node2: 1114
```

This creates per-node subdirectories with unique configurations:

```
~/ion-2local/
  ionrun.meta
  node1/ionrun.rc  node1/ionrun.ionconfig
  node2/ionrun.rc  node2/ionrun.ionconfig
```

Start each node in a separate terminal:

```bash
# Terminal 1:
ionrun -n node1 ~/ion-2local

# Terminal 2:
ionrun -n node2 ~/ion-2local
```

Test connectivity:

```bash
# In the node2 terminal:
export ION_NODE_LIST_DIR=$HOME/ion-2local
export ION_NODE_WDNAME=$HOME/ion-2local/node2
bpsink ipn:2.1 &

# In the node1 terminal:
export ION_NODE_LIST_DIR=$HOME/ion-2local
export ION_NODE_WDNAME=$HOME/ion-2local/node1
echo "Hello from node1" | bpsource ipn:2.1
```

Stop all nodes: `ionrun -s ~/ion-2local`

### Three Nodes on the Same Host

Run a 3-node relay topology on one machine:

```bash
$ ionrun -g ~/ion-3local
Topology [1-5]: 5
--- Node 1 ---
  Name [node1]:
  IPN node ID [1]:
  IP address: 127.0.0.1 (loopback)
--- Node 2 ---
  Name [node2]:
  IPN node ID [2]:
  IP address: 127.0.0.1 (loopback)
--- Node 3 ---
  Name [node3]:
  IPN node ID [3]:
  IP address: 127.0.0.1 (loopback)
Select convergence layer (all links):
  Choice [1-3]: 1
  Port [1113]:
  Port assignments:
    node1: 1113
    node2: 1114
    node3: 1115
```

Start each node in a separate terminal, then test end-to-end delivery from node 1 to node 3 (relayed through node 2):

```bash
# Terminal 3 (receiver):
export ION_NODE_LIST_DIR=$HOME/ion-3local
export ION_NODE_WDNAME=$HOME/ion-3local/node3
bpsink ipn:3.1 &

# Terminal 1 (sender):
export ION_NODE_LIST_DIR=$HOME/ion-3local
export ION_NODE_WDNAME=$HOME/ion-3local/node1
echo "Hello via relay" | bpsource ipn:3.1
```

### Custom Port Numbers

Use non-default ports when needed:

```bash
$ ionrun -g ~/ion-custom-port
Topology [1-5]: 1
  Name [node1]:
  IPN node ID [1]:
Select convergence layer:
  Choice [1-3]: 2
  Port [4556]: 9000
```

This generates TCP configuration using port 9000 instead of the default 4556.

### Regenerate Configuration

If `ionrun` has been updated (e.g., to pick up bug fixes in config generation), you can regenerate the configuration files from the saved metadata without re-running the wizard:

```bash
$ ionrun -r ~/ion-2local
Configuration regenerated from /home/user/ion-2local/ionrun.meta
```

This reads the topology parameters from `ionrun.meta` and regenerates `ionrun.rc` (and `ionrun.ionconfig` for same-host topologies). The original parameters are preserved — only the generated config files are rewritten. Combine with `-g` to regenerate without starting ION:

```bash
$ ionrun -r -g ~/ion-2local
Configuration regenerated from /home/user/ion-2local/ionrun.meta
```

## Multi-Node Workflow

### Remote (Multi-Host)

For remote multi-node topologies (`2-node` and `3-node`), `ionrun` generates a single `ionrun.rc` containing all nodes' configurations, differentiated by tags. The workflow is:

1. **Generate once**: Run `ionrun -g <workdir>` on any machine to create the config.
2. **Distribute**: Copy the working directory to all participating hosts.
3. **Start per-host**: On each host, run `ionrun -n <nodename> <workdir>` to start that host's ION instance.
4. **Stop per-host**: Run `ionrun -s <workdir>` to stop ION on that host.

This approach ensures all nodes share an identical contact plan and consistent routing configuration.

### Same-Host

For same-host topologies (`2-node (same host)` and `3-node (same host)`), `ionrun` creates per-node subdirectories, each containing a standalone `ionrun.rc` and `ionrun.ionconfig` with unique shared memory keys and ports. The workflow is:

1. **Generate once**: Run `ionrun -g <workdir>` to create subdirectories and configs for all nodes.
2. **Start each node**: In separate terminals, run `ionrun -n <nodename> <workdir>` for each node.
3. **Stop all nodes**: Run `ionrun -s <workdir>` to stop all ION instances and clean up shared memory.

Each node runs from its own subdirectory. The `ION_NODE_LIST_DIR` environment variable is set to the parent working directory so ION can track all instances via the `ion_nodes` file. The `ION_NODE_WDNAME` variable is set to the node's subdirectory so ION commands can be run from any directory.

## Environment Variables

`ionrun` sets the following environment variables when starting ION:

| Variable | Set When | Purpose |
|----------|----------|---------|
| `ION_NODE_WDNAME` | All topologies | Points to the ION working directory, allowing ION commands (`bpadmin`, `bpsource`, etc.) to be run from any directory. |
| `ION_NODE_LIST_DIR` | Same-host topologies only | Points to the parent directory containing the `ion_nodes` file, which tracks all ION instances on the host. |

After starting a node, `ionrun` prints the `export` commands you need to run in other terminals to operate on that node.

## Pre-Existing Configurations

`ionrun` can detect and use pre-existing ION configurations that were not generated by its wizard. When pointed at a directory that has no `ionrun.meta` and no top-level `ionrun.rc`, but contains node subdirectories with `start_<name>.sh` scripts, ionrun will detect and offer to start them.

For example, given this directory layout:

```
my-test/
  node1/
    start_node1.sh    # executable
    amroc.ionrc
    amroc.bprc
    ...
  node2/
    start_node2.sh    # executable
    amroc.ionrc
    amroc.bprc
    ...
```

Running `ionrun my-test` will detect both nodes and prompt which one to start:

```bash
$ ionrun my-test
Detected nodes:
  node1
  node2

Which node to start? node1
Starting ION node 'node1' in /home/user/my-test/node1 ...
```

Use `-n` to skip the prompt: `ionrun -n node1 my-test`.

Detection priority: `ionrun.meta` (ionrun-generated config) > top-level `ionrun.rc` (bare combined file) > subdirectories with `start_<name>.sh` (pre-existing config) > wizard (new directory).

When stopping (`ionrun -s`) a directory with no `ionrun.meta`, ionrun uses `killm` to terminate all ION processes.

## How It Works

`ionrun` generates a combined config file in the format understood by `ionstart -I` (parsed by `ionstart.awk`). The file contains sections delimited by markers:

```
## begin ionadmin [tag]
<ionadmin commands>
## end ionadmin [tag]

## begin bpadmin [tag]
<bpadmin commands>
## end bpadmin [tag]
```

Programs are always executed in a fixed order: `ionadmin`, `ionsecadmin`, `ltpadmin`, `bpadmin`, `cfdpadmin` (if enabled), `ipnadmin`. When a tag is specified with `-t`, only sections matching that tag are processed.

For same-host topologies, each node's `ionrun.rc` is a standalone file (no tags) and the `ionadmin` init line references the local `ionrun.ionconfig` file (e.g., `1 1 ionrun.ionconfig`). Each node registers endpoints: `.0` (admin, disposition `x`), `.1`, `.2`, `.3` (user, disposition `q`), and `.64`, `.65` (CFDP, disposition `q`).

After generating the combined `ionrun.rc`, ionrun also splits it into per-node subdirectories with individual config files (`ionrun.ionrc`, `ionrun.bprc`, etc.) and a `start_<name>.sh` script. In the individual `ionrun.bprc`, routing is loaded via `r 'ipnadmin ionrun.ipnrc'` before the `s` command, following the standard ION convention. See [Per-Node Subdirectories](#per-node-subdirectories) for details.

The generated configuration uses these fixed parameters:

| Parameter | Value |
|-----------|-------|
| Contact duration | 24 hours (+1 to +86400 seconds) |
| Contact rate | 100,000 bytes/sec |
| One-Way Light Time | 1 second |
| Production/Consumption rate | 1,000,000 bytes/sec |
| LTP sessions | 32 |
| LTP segment size | 1,400 bytes |
| LTP block size | 10,000 bytes |
| Protocol payload | 1,400 bytes |
| Protocol overhead | 100 bytes |

Same-host ionconfig defaults:

| Parameter | Value |
|-----------|-------|
| Working memory (wmSize) | 50,000,000 bytes (50 MB) |
| Heap words (heapWords) | 10,000,000 |
| Shared memory key (wmKey) | 10001, 10002, ... (unique per node) |
| SDR working memory key (sdrWmKey) | 65281, 65282, ... (SDR_SM_KEY + node, unique per node) |
| SDR name (sdrName) | ion1, ion2, ... (unique per node) |
