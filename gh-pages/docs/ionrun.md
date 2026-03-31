# ionrun - Interactive ION Setup and Launch Utility

`ionrun` is a command-line utility that simplifies setting up and running ION. It provides an interactive wizard that generates the ION configuration files needed for common network topologies, and then launches ION in a user-specified working directory.

- [ionrun - Interactive ION Setup and Launch Utility](#ionrun---interactive-ion-setup-and-launch-utility)
  - [Overview](#overview)
  - [Usage](#usage)
  - [Interactive Wizard](#interactive-wizard)
    - [Topology Types](#topology-types)
    - [Node Configuration](#node-configuration)
    - [Convergence Layers](#convergence-layers)
    - [Port Numbers](#port-numbers)
  - [Generated Files](#generated-files)
    - [ionrun.rc](#ionrunrc)
    - [ionrun.meta](#ionrunmeta)
  - [Examples](#examples)
    - [Loopback Test](#loopback-test)
    - [Two-Node Network](#two-node-network)
    - [Three-Node Relay with Mixed Convergence Layers](#three-node-relay-with-mixed-convergence-layers)
    - [Custom Port Numbers](#custom-port-numbers)
  - [Multi-Node Workflow](#multi-node-workflow)
  - [How It Works](#how-it-works)

## Overview

`ionrun` eliminates the need to manually write ION configuration files for common scenarios. It:

1. Creates a working directory for ION runtime files
2. Asks a series of questions about the desired network topology
3. Generates a combined configuration file (`ionrun.rc`) compatible with `ionstart -I`
4. Optionally starts ION immediately

On subsequent runs in the same directory, it detects the existing configuration and starts ION directly.

## Usage

```
ionrun [options] <working_directory>

Options:
  -h, --help            Show help message
  -s, --stop            Stop ION in the working directory (runs ionstop)
  -n, --node <name>     Start a specific node (skip interactive selection)
  -g, --generate-only   Generate config files without starting ION
  -f, --force           Regenerate config even if one already exists
```

## Interactive Wizard

When `ionrun` is pointed at an empty or new directory (or when `--force` is used), it runs an interactive setup wizard.

### Topology Types

The wizard supports three topologies:

| Topology | Nodes | Description |
|----------|-------|-------------|
| **Loopback** | 1 | Single node sending to itself. Good for initial testing. |
| **2-node** | 2 | Two nodes on distinct hosts. Basic point-to-point link. |
| **3-node** | 3 | Three nodes in a linear chain (1--2--3). Node 2 acts as a relay. |

### Node Configuration

For each node, the wizard asks:

- **Name**: A label for the node (e.g., `host1`). Used as the ionstart tag for multi-node configs. Must contain only letters, digits, hyphens, and underscores.
- **IPN Node ID**: The node's IPN identifier. Can be a simple integer (e.g., `1`) or an allocator.node pair (e.g., `5.1`) for 3-part IPN addressing.
- **IP Address**: The node's network address. Defaults to `127.0.0.1`. For multi-node topologies, use the actual IP of each host.

### Convergence Layers

Three convergence layers are supported:

| CL | Protocol Name | Transport | Default Port | CLI Programs |
|----|--------------|-----------|--------------|-------------|
| **LTP** | `ltp` | UDP | 1113 | `ltpcli` / `ltpclo` |
| **TCP** | `tcp` | TCP | 4556 | `tcpcli` / `tcpclo` |
| **UDP** | `udp` | UDP | 4556 | `udpcli` / `udpclo` |

- For **loopback** and **2-node** topologies, one convergence layer is selected for the entire network.
- For **3-node** topologies, two convergence layers are selected independently: one for the link between nodes 1-2, and another for the link between nodes 2-3. This allows mixed-CL networks (e.g., LTP on one hop and TCP on the other).

### Port Numbers

After selecting each convergence layer, the wizard prompts for a port number. Press Enter to accept the IANA-registered default:

- **LTP**: 1113 (IANA registered for Licklider Transmission Protocol)
- **TCP/UDP**: 4556 (IANA registered for DTN TCP Convergence Layer)

Custom ports are useful when running alongside other services or when firewall rules require specific ports.

## Generated Files

### ionrun.rc

The main configuration file, compatible with `ionstart -I`. It uses the combined config format with `## begin`/`## end` section markers.

- **Loopback**: Sections have no tags. Run with `ionstart -I ionrun.rc`.
- **Multi-node**: Sections are tagged with node names. Run with `ionstart -I ionrun.rc -t <nodename>`.

The tag mechanism is a built-in feature of `ionstart.awk` that allows multiple nodes' configurations to coexist in a single file.

### ionrun.meta

A key-value metadata file that stores the topology parameters so `ionrun` knows how to re-launch ION on subsequent runs. Example:

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

Topology [1-3]: 1

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
Topology [1-3]: 2
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
Topology [1-3]: 3
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

### Custom Port Numbers

Use non-default ports when needed:

```bash
$ ionrun -g ~/ion-custom-port
Topology [1-3]: 1
  Name [node1]:
  IPN node ID [1]:
Select convergence layer:
  Choice [1-3]: 2
  Port [4556]: 9000
```

This generates TCP configuration using port 9000 instead of the default 4556.

## Multi-Node Workflow

For multi-node topologies, `ionrun` generates a single `ionrun.rc` containing all nodes' configurations, differentiated by tags. The workflow is:

1. **Generate once**: Run `ionrun -g <workdir>` on any machine to create the config.
2. **Distribute**: Copy the working directory to all participating hosts.
3. **Start per-host**: On each host, run `ionrun -n <nodename> <workdir>` to start that host's ION instance.
4. **Stop per-host**: Run `ionrun -s <workdir>` to stop ION on that host.

This approach ensures all nodes share an identical contact plan and consistent routing configuration.

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

Programs are always executed in a fixed order: `ionadmin`, `ionsecadmin`, `ltpadmin`, `bpadmin`, `ipnadmin`. When a tag is specified with `-t`, only sections matching that tag are processed.

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
