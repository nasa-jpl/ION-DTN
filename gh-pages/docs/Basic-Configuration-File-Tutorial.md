# Basic Configuration File Tutorial

For an overview of all available configuration tools, see the [Configuration Tools Overview](Configuration-Tools-Overview.md).

## Programs in ION

The following tools are available to you after ION is built:

Daemon and Configuration

* ionadmin is the administration and configuration interface for the local ION node contacts and manages shared memory resources used by ION.
* ltpadmin is the administration and configuration interface for LTP operations on the local ION node.
* bsspadmin is the administrative interface for operations of the Bundle Streaming Service Protocol on the local ION node.
* bpadmin is the administrative interface for bundle protocol operations on the local ION node.
* ipnadmin is the administration and configuration interface for the IPN addressing system and routing on the ION node. (ipn:)
* dtn2admin is the administration and configuration interface for the DTN addressing system and routing on the ION node. (dtn://)
* killm is a script which tears down the daemon and any running ducts on a single machine (use ionstop instead).
* ionstart is a script which completely configures an ION node with the proper configuration file(s).
* ionstop is a script which completely tears down the ION node.
* ionscript is a script which aides in the creation and management of configuration files to be used with ionstart.

Simple Sending and Receiving

- bpsource and bpsink are for testing basic connectivity between endpoints. bpsink listens for and then displays messages sent by bpsource.
- bpsendfile and bprecvfile are used to send files between ION nodes.

Testing and Benchmarking

- bpdriver benchmarks a connection by sending bundles in two modes: request-response and streaming.
- bpecho issues responses to bpdriver in request-response mode.
- bpcounter acts as receiver for streaming mode, outputting markers on receipt of data from bpdriver and computing throughput metrics.

## ION Logging

It is important to note that, by default, the administrative programs will all trigger the creation of a log file called ion.log in the directory where the program is called. This means that write-access in your current working directory is required. The log file itself will contain the expected log information from administrative daemons, but it will also contain error reports from simple applications such as bpsink. This is important to note since the BP applications may not be reporting all error information to stdout or stderr.

## Starting the ION Daemon

A script has been created which allows a more streamlined configuration and startup of an ION node. This script is called ionstart, and it has the following syntax. Don't run it yet; we still have to configure it!

`ionstart -I <filename>`

filename: This is the name for configuration file which the script will attempt to use for the various configuration commands. The script will perform a sanity check on the file, splitting it into command sections appropriate for each of the administration programs.

Configuration information (such as routes, connections, etc) can be specified one of two ways for any of the individual administration programs:

(Recommended) Creating a configuration file and passing it to ionadmin, bpadmin, ipnadmin... either directly or via the ionstart helper script.
Manually typing configuration commands into the terminal for each administration program.

> **Tip**: If ION was built with `--enable-commandline-history`, the admin
> utilities support command-line history and editing (use arrow keys to
> recall previous commands). This is useful for interactive terminal
> sessions but is intended for ground-based use only. See the
> [ION Deployment Guide](ION-Deployment-Guide.md#input_history-configure-option---enable-commandline-history)
> for details.

You can find appropriate commands in the following sections.

## Configuration Files Overview

There are six configuration files about which you should be aware.

**ionadmin** (.ionrc) — assigns an identity (node number) to the node, optionally configures the resources that will be made available to the node, and specifies contact bandwidths and one-way transmission times. Specifying the "contact plan" is important in deep-space scenarios where the bandwidth must be managed and where acknowledgments must be timed according to propagation delays. It is also vital to the function of contact-graph routing.

**ltpadmin** (.ltprc) — specifies spans, transmission speeds, and resources for the Licklider Transfer Protocol convergence layer.

**bpadmin** (.bprc) — specifies the EID scheme, all of the open endpoints for delivery on your local end, the convergence layer protocol(s) and their inducts/outducts, and egress plans that govern how bundles are forwarded to neighboring nodes. Egress plans and their attached outducts are configured using `a plan` and `a planduct` commands (see the [Egress Plans](#egress-plans-bpadmin) section below).

**ipnadmin** (.ipnrc) — defines egress plans for the "ipn" naming scheme using a simplified shortcut syntax. Note that the `ipnadmin` plan commands are a simplified shortcut for the more general and more powerful `bpadmin` plan commands (see bprc(5)). For new configurations, using `a plan` and `a planduct` in the bprc file is recommended.

**ionsecadmin** (.ionsecrc) — configures the security policy for the node, including Bundle Protocol Security (BPSec) rules for block integrity (BIB) and block confidentiality (BCB). Even when no security policy is needed, an ionsecrc file with just the initialization command (`1`) is typically provided.

**dtn2admin** (.dtn2rc) — defines egress plans for the "dtn" naming scheme, analogous to ipnadmin for the "ipn" scheme.

## Updated IPN-URI Format Support (ION 4.1.4-a.2)

Starting with ION 4.1.4-a.2, ION has been updated to support the new IPN URI
scheme defined in [RFC 9758](https://datatracker.ietf.org/doc/html/rfc9758)
as a alpha release feature. The new format is as follows:

```abnf
ipn-uri = "ipn:" [allocator-identifier "."] node-number "." service-number
```

`allocator-identifier`: An unsigned integer identifying the allocation
authority. If the authority is the default (IANA, Allocator ID 0), this
part and the following dot (.) may be omitted for brevity. ION is backward
compatible with IPN URIs that omit the allocator identifier, which is
interpreted as having the default value of 0.

For all examples in this tutorial, the allocator identifier is omitted and
defaults to 0.

New IPN URI support is under alpha testing.

## The ION Configuration File

Given to ionadmin either as a file or from the daemon command line, this file configures contacts for the ION node. We will assume that the local node's identification number is 1.

This file specifies contact times and one-way light times between nodes. This is useful in deep-space scenarios: for instance, Mars may be 20 light-minutes away, or 8. Though only some transport protocols make use of this time (currently, only LTP), it must be specified for all links nonetheless. Times may be relative (prefixed with a + from current time) or absolute. Absolute times, are in the format `yyyy/mm/dd-hh:mm:ss`. By default, the contact-graph routing engine will make bundle routing decisions based on the contact information provided.

The configuration file lines are as follows:

`1 1 ''`

This command will initialize the ion node to be node number 1.

1 refers to this being the initialization or ''first'' command.
1 specifies the node number of this ion node. (IPN node 1).
'' specifies the name of a file of configuration commands for the node's use of shared memory and other resources (suitable defaults are applied if you leave this argument as an empty string).

`s `

This will start the ION node. It mostly functions to officially "start" the node in a specific instant; it causes all of ION's protocol-independent background daemons to start running.

`a contact +1 +3600 1 1 100000`

specifies a transmission opportunity for a given time duration between two connected nodes (or, in this case, a loopback transmission opportunity).

a adds this entry in the configuration table.
contact specifies that this entry defines a transmission opportunity.
+1 is the start time for the contact (relative to when the s command is issued).
+3600 is the end time for the contact (relative to when the s command is issued).
1 is the source node number.
1 is the destination node number.
100000 is the maximum rate at which data is expected to be transmitted from the source node to the destination node during this time period (here, it is 100000 bytes / second).

`a range +1 +3600 1 1 1 `

specifies a distance between nodes, expressed as a number of light seconds, where each element has the following meaning:

a adds this entry in the configuration table.
range declares that what follows is a distance between two nodes.
+1 is the earliest time at which this is expected to be the distance between these two nodes (relative to the time s was issued).
+3600 is the latest time at which this is still expected to be the distance between these two nodes (relative to the time s was issued).
1 is one of the two nodes in question.
1 is the other node.
1 is the distance between the nodes, measured in light seconds, also sometimes called the "one-way light time" (here, one light second is the expected distance).

`m production 1000000 `

specifies the maximum rate at which data will be produced by the node.

m specifies that this is a management command.
production declares that this command declares the maximum rate of data production at this ION node.
1000000 specifies that at most 1000000 bytes/second will be produced by this node.

`m consumption 1000000 `

specifies the maximum rate at which data can be consumed by the node.

m specifies that this is a management command.
consumption declares that this command declares the maximum rate of data consumption at this ION node.
1000000 specifies that at most 1000000 bytes/second will be consumed by this node.

This will make a final configuration file host1.ionrc which looks like this:

```
1 1 ''
s
a contact +1 +3600 1 1 100000
a range +1 +3600 1 1 1
m production 1000000
m consumption 1000000
```

## The Licklider Transfer Protocol Configuration File

Given to ltpadmin as a file or from the command line, this file configures the LTP engine itself. We will assume the local IPN node number is 1; in ION, node numbers are used as the LTP engine numbers.

`1 32`

This command will initialize the LTP engine:

1 refers to this being the initialization or ''first'' command.
32 is an estimate of the maximum total number of LTP ''block'' transmission sessions - for all spans - that will be concurrently active in this LTP engine. It is used to size a hash table for session lookups.

`a span 1 32 32 1400 10000 1 'udplso localhost:1113'`

This command defines an LTP engine 'span':

a indicates that this will add something to the engine.

span indicates that an LTP span will be added.

1 is the engine number for the span, the number of the remote engine to which LTP segments will be transmitted via this span. In this case, because the span is being configured for loopback, it is the number of the local engine, i.e., the local node number. This will have to match an outduct in Section 2.6.

32 specifies the maximum number of LTP ''block'' transmission sessions that may be active on this span. The product of the mean block size and the maximum number of transmission sessions is effectively the LTP flow control ''window'' for this span: if it's less than the bandwidth delay product for traffic between the local LTP engine and this spa's remote LTP engine then you'll be under-utilizing that link. We often try to size each block to be about one second's worth of transmission, so to select a good value for this parameter you can simply divide the span's bandwidth delay product (data rate times distance in light seconds) by your best guess at the mean block size.

The second 32 specifies the maximum number of LTP ''block'' reception sessions that may be active on this span. When data rates in both directions are the same, this is usually the same value as the maximum number of transmission sessions.

1400 is the number of bytes in a single segment. In this case, LTP runs atop UDP/IP on ethernet, so we account for some packet overhead and use 1400.

10000 is the LTP aggregation size limit, in bytes. LTP will aggregate multiple bundles into blocks for transmission. This value indicates that the block currently being aggregated will be transmitted as soon as its aggregate size exceeds 10000 bytes.

1 is the LTP aggregation time limit, in seconds. This value indicates that the block currently being aggregated will be transmitted 1 second after aggregation began, even if its aggregate size is still less than the aggregation size limit.

'udplso localhost:1113' is the command used to implement the link itself. The link is implemented via UDP, sending segments to the localhost Internet interface on port 1113 (the IANA default port for LTP over UDP).

`s 'udplsi localhost:1113'`

Starts the ltp engine itself:

s starts the ltp engine.

'udplsi localhost:1113' is the link service input task. In this case, the input ''duct' is a UDP listener on the local host using port 1113.

This means that the entire configuration file host1.ltprc looks like this:

```
1 32
a span 1 32 32 1400 10000 1 'udplso localhost:1113'
s 'udplsi localhost:1113'
```

## The Bundle Protocol Configuration File

Given to bpadmin either as a file or from the daemon command line, this file configures the endpoints through which this node's Bundle Protocol Agent (BPA) will communicate. We will assume the local BPA's node number is 1; as for LTP, in ION node numbers are used to identify bundle protocol agents.

`1` 

This initializes the bundle protocol:

1 refers to this being the initialization or ''first'' command.

`a scheme ipn 'ipnfw' 'ipnadminep' `

This adds support for a new Endpoint Identifier (EID) scheme:

a means that this command will add something.

scheme means that this command will add a scheme.

ipn is the name of the scheme to be added.

'ipnfw' is the name of the IPN scheme's forwarding engine daemon.

'ipnadminep' is the name of the IPN scheme's administrative endpoint task, which handles administrative bundles (status reports, etc.).

`a endpoint ipn:1.0 q `

This command establishes this BP node's membership in a BP endpoint:

a means that this command will add something.

endpoint means that this command adds an endpoint.

ipn is the scheme name of the endpoint.

1.0 is the scheme-specific part of the endpoint. For the IPN scheme the scheme-specific part always has the form nodenumber.servicenumber. Each node must be a member of the endpoint whose node number is the node's own node number and whose service number is 0, indicating administrative traffic.

q means that the behavior of the engine, upon receipt of a new bundle for this endpoint, is to queue it until an application accepts the bundle. The alternative is to silently discard the bundle if no application is actively listening; this is specified by replacing q with x.

`a endpoint ipn:1.1 q `

`a endpoint ipn:1.2 q `

These specify two more endpoints that will be used for test traffic.

`a protocol ltp `

This command adds support for a convergence-layer protocol:

a means that this command will add something.

protocol means that this command will add a convergence-layer protocol.

ltp is the name of the convergence-layer protocol. Protocol classes for well-known protocols (ltp, tcp, stcp, udp, etc.) are hard-coded in ION.

> **Note**: Earlier versions of ION took two additional arguments
> (`payload_bytes_per_frame` and `overhead_bytes_per_frame`, e.g.,
> `a protocol ltp 1400 100`). These arguments are deprecated and silently
> ignored. The modern syntax is simply `a protocol ltp`.

`a induct ltp 1 ltpcli `

This command adds an induct, through which incoming bundles can be received from other nodes:

a means that this command will add something.

induct means that this command will add an induct.

ltp is the convergence layer protocol of the induct.

1 is the identifier of the induct, in this case the ID of the local LTP engine.

ltpcli is the name of the daemon used to implement the induct.

`a outduct ltp 1 ltpclo `

This command adds an outduct, through which outgoing bundles can be sent to other nodes:

a means that this command will add something.

outduct means that this command will add an outduct.

ltp is the convergence layer protocol of the outduct.

1 is the identifier of the outduct, the ID of the convergence-layer protocol induct of some remote node. See Section 2.5 for remote LTP engine IDs.

ltpclo is the name of the daemon used to implement the outduct.

### Egress Plans (bpadmin)

`a plan ipn:1.0 `

This command establishes an egress plan governing transmission to a neighboring node:

a means that this command will add something.

plan means that this command will add an egress plan.

ipn:1.0 is the endpoint name identifying the neighboring node (here, the local node for loopback). The plan commands in bpadmin supersede and generalize the plan commands in ipnadmin, and support wildcarding of endpoint names (e.g., `ipn:1.*`).

An optional transmission rate (in bytes/second) may follow; when omitted, rate control is determined by applicable contacts in the contact plan.

`a planduct ipn:1.0 ltp 1 `

This command attaches a convergence-layer protocol outduct to an egress plan:

a means that this command will add something.

planduct means that this command attaches an outduct to a plan.

ipn:1.0 is the endpoint name of the egress plan.

ltp is the convergence-layer protocol name.

1 is the duct name (the outduct identifier, here the remote LTP engine ID).

> **Recommended**: Configure egress plans with `a plan` and `a planduct`
> in the bprc file. The equivalent ipnadmin one-liner (`a plan 1 ltp/1`)
> is a simplified shortcut. For the reasons behind this recommendation
> — and for what the ipnrc file *should* be used for instead — see
> [Egress Plans: bprc vs. ipnrc, and What Belongs Where](#egress-plans-bprc-vs-ipnrc-and-what-belongs-where)
> below.

`s`

This command starts the bundle engine including all daemons for the inducts and outducts.

That means that the entire configuration file host1.bprc looks like this:

```
1
a scheme ipn 'ipnfw' 'ipnadminep'
a endpoint ipn:1.0 q
a endpoint ipn:1.1 q
a endpoint ipn:1.2 q
a protocol ltp
a induct ltp 1 ltpcli
a outduct ltp 1 ltpclo
a plan ipn:1.0
a planduct ipn:1.0 ltp 1
s
```

### Egress Plans: bprc vs. ipnrc, and What Belongs Where

An egress plan can be defined in **two** places, using **two different
syntaxes**. This frequently causes confusion, so it is worth understanding
exactly how they relate.

**The two syntaxes:**

| | bprc (bpadmin) | ipnrc (ipnadmin) |
|---|---|---|
| Create plan | `a plan ipn:19.0 [rate]` | *(combined below)* |
| Attach duct | `a planduct ipn:19.0 ltp 19` | *(combined below)* |
| Combined form | *(two commands above)* | `a plan 19 ltp/19 [rate]` |
| Endpoint / node | full EID `ipn:19.0` (any scheme) | bare node number `19` (ipn only) |
| Duct expression | two tokens: `ltp 19` | one token, slash form: `ltp/19` |

> **Watch the duct delimiter.** bprc uses two space-separated tokens
> (`ltp 19`); ipnrc uses a single slash-joined token (`ltp/19`). Copying a
> duct expression from one file to the other without adjusting the delimiter
> is a common configuration mistake.

**They configure the same thing.** Both forms create the *same* underlying
BP egress plan. The ipnrc `a plan` command is simply a shortcut: it
auto-builds the `ipn:<node>.0` endpoint ID for you, attaches the single
duct, and starts the plan — all in one line. It is not a different kind of
plan; it is the bprc plan with less control and less flexibility.

**Why the bprc form is recommended.** The move toward defining egress plans
in bprc is about *full generality*:

- **Multiple ducts and lifecycle control.** In bprc you can attach several
  `a planduct` commands to a single plan and then start, stop, block, or
  unblock ducts independently (`s plan`, `b`/`u`). The ipnrc one-liner is
  limited to a single duct and starts it implicitly, with no separate
  stop/block control.
- **Concepts placed where they belong.** Egress plans and their
  inducts/outducts are *general Bundle Protocol concepts* — they are not
  specific to the ipn naming scheme. Keeping them in bprc, alongside the
  scheme, endpoints, protocols, inducts, and outducts they depend on, means
  the entire convergence-layer wiring for a node reads coherently in one
  file.
- **Scheme-agnostic and wildcard-capable.** bprc plans work for any naming
  scheme (`ipn:`, `dtn:`, `imc:`) and support wildcarded endpoint names
  (e.g. `ipn:19.*`). The ipnrc shortcut hardcodes the ipn scheme and a
  single node number.

**What the ipnrc file is for.** With egress plans in bprc, the ipnrc file
is best reserved for the functions that are genuinely *specific to the ipn
naming scheme* — its routing and forwarding directives, which have no bprc
equivalent:

- `a exit` — a static default route: for a range of destination node
  numbers that cannot otherwise be routed, forward via a designated "via"
  node. (See the ipn routing section below.)
- `a rtovrd` — a routing override that pins traffic (optionally filtered by
  data label, destination, and source) to a specific neighbor/duct.
- `a cosovrd` — a class-of-service override (priority, ordinal, QoS flags).

These are scheme-specific forwarding policy, which is why they live with
ipnadmin rather than bpadmin.

> **An ipnrc file with no `a plan` command is perfectly valid.** In the
> recommended layout, plans live in bprc and the ipnrc file contains only
> ipn-scheme routing directives (exits and overrides) — or may be omitted
> entirely if none are needed. The absence of a plan command in ipnrc does
> not indicate a misconfiguration.

**Recommended split (two-node example).** Node 1 forwards to neighbor node 2
over LTP, and uses an exit as a default route for any node in the range
10–20:

`host1.bprc` — scheme, endpoints, CL protocol, ducts, and the egress plan:

```
1
a scheme ipn 'ipnfw' 'ipnadminep'
a endpoint ipn:1.0 q
a endpoint ipn:1.1 q
a protocol ltp
a induct ltp 1 ltpcli
a outduct ltp 2 ltpclo
a plan ipn:2.0
a planduct ipn:2.0 ltp 2
s
```

`host1.ipnrc` — only ipn-scheme routing directives (no plan):

```
a exit 10 20 ipn:2.0
```

## IPN Routing Configuration (ipnadmin — Legacy Shortcut)

> **Note**: The `ipnadmin` `a plan` command shown below is a simplified
> shortcut for the more general `bpadmin` plan commands. For new
> configurations, define egress plans with `a plan` and `a planduct` in the
> bprc file, and reserve the ipnrc file for ipn-scheme routing directives
> (`a exit`, `a rtovrd`, `a cosovrd`). See
> [Egress Plans: bprc vs. ipnrc, and What Belongs Where](#egress-plans-bprc-vs-ipnrc-and-what-belongs-where)
> above for the rationale. The ipnrc `a plan` shortcut is retained for
> backward compatibility.

For this simple loopback example the ipnrc file defines a single egress plan
— a routing rule specifying which outduct to use for forwarding bundles to a
neighboring node. Since we have only one outduct, forwarding bundles to one
place (the local node), we have only one egress plan.

`a plan 1 ltp/1 `

This command defines an egress plan for bundles to be transmitted to the local node:

a means this command adds something.

plan means this command adds an egress plan.

1 is the node number of the remote node. In this case, that is the local node's own node number; we're configuring for loopback.

ltp/1 is the duct expression identifying the convergence-layer protocol and outduct (format: `protocol/duct_name`) through which to transmit bundles to reach this node.

This means that the entire configuration file host1.ipnrc looks like this:

`a plan 1 ltp/1`

## Testing Your Connection

Assuming no errors occur with the configuration above, we are now ready to test loopback communications. In one terminal, we have to run the start script (the one we said that you would have to have earlier). It's right here, in case you forgot to write it down:

`ionstart -i host1.ionrc -s host1.ionsecrc -l host1.ltprc -b host1.bprc -p host1.ipnrc `

This command will run the appropriate administration programs, in order, with the appropriate configuration files. Don't worry that the command is lengthy and unwieldy; we will show you how to make a more clean single configuration file later.

Once the daemon is started, run:

`bpsink ipn:1.1 `

This will begin listening on the Endpoint ID with the endpoint_number 1 on service_number 1, which is used for testing.

Now open another terminal and run the command:

`bpsource ipn:1.1`

This will begin sending messages you type to the Endpoint ID ipn:1.1, which is currently being listened to by bpsink. Type messages into bpsource, press enter, and see if they are reported by bpsink.

If so, you're ready for bigger and better things. If not, check the following:

Do you have write permissions for your current directory? If not, you will not be able to start the daemon as it has to write out to the ion.log file.
Are your config files exactly as specified, except for IP address changes?
Are you running it on one of our supported platforms? Currently, those are the only supported distributions.

If you are still having problems, you can ask for help on the ION users' list or file an ION bug report.

## Stopping the Daemon

As the daemon launches many ducts and helper applications, it can be complicated to turn it all off. To help this, we provided a script. The script similar to ionstart exists called ionstop, which tears down the ion node in one step. You can call it like so:

`ionstop `

After stopping the daemon, it can be restarted using the same procedures as outlined above. Do remember that the ion.log file is still present, and will just keep growing as you experiment with ION.

IMPORTANT: The user account that runs ionstart must also run ionstop. If that account does not, no other accounts can successfully start the daemon, as the shared memory vital to ION's functionality will already be occupied.

## More Advanced Usage

Detailed documentation of ION and its applications are available via the man pages. It is suggested that you start with man ion , as this is an overview man page listing all available ION packages.

## Ionscript for Simplified Configuration Files

The most difficult and cumbersome method of starting an ION node is to manually run the various administration programs in order, manually typing configuration commands all the way. It is much more efficient and less error-prone to place the configuration commands into a configuration file and using that as input to the administration program, but this is still cumbersome as you must type in each administration program in order. The ionstart program will automatically execute the appropriate administration programs with their respective configuration files in order. Unfortunately, as seen in the previous sections, the command is lengthy. This is why the ionscript script was added to make things even easier.

The ionscript will basically concatenate the configuration files into one large file. The format of this large configuration file is simply to bookend configuration sections with the lines: ## begin PROGRAM and ## end PROGRAM, where PROGRAM is the name of the administration program for which the configuration commands should be sent (such as ionadmin, bpadmin, ipnadmin).

To create a single file host1.rc out of the various configuration files defined in the previous section, run this command:

`ionscript -i host1.ionrc -p host1.ipnrc -l host1.ltprc -b host1.bprc -O host1.rc `

The command can also be used to split the large host1.rc into the individual configuration files (so long as the large file is formatted correctly). Just run this command to revert the process:

`ionscript -i host1.ionrc -p host1.ipnrc -l host1.ltprc -b host1.bprc -I host1.rc `

This isn't very practical in this specific case (as you already have the individual files) but if you start with a single configuration file, this can be helpful.

Once you have a single configuration file, starting the ION node is a single command:

`ionstart -I host1.rc `

Note that ionstart and ionscript require sed and awk, but those are almost universally available on Unix-based systems. The two scripts will always sanity-check the large configuration file to ensure that it interprets the bookend lines correctly- and it will warn you of any errors you have made in the file. Consult the USAGE for each script for further help, by attempting to run the script with no arguments or the -h argument.

## Examples of Network Configurations

For a simple single-node ION configuration - running multiple instances of ION in the same host, see the tutorial [here.](community/dtn-gcp-main/ION-One-Node-on-Cloud-Linux-VM.md)

For a two-node configuration, see the tutorial [here.](community/dtn-gcp-2nodes/ION-Two-Node-on-Cloud-Linux-VMs.md)

For a multi-hop and also multi-network configuration, see this [page.](Configure-Multiple-Network-Interfaces.md)
