# Basic Configuration File Tutorial

## ION Configuration Tools Available for Public Use

The [ION Configuration Tools](https://github.com/nasa-jpl/ion-config-tool) provides both command line and browser interfaces for creating and editing configuration files for the Interplanetary Overlay Network (ION) software, an implementation of Delay/Disruption Tolerant Networking technology.

The [DTN Network Model](https://github.com/nasa-jpl/ion-network-model) is simple graphical frontend to specify a network of computer hosts, DTN nodes, IP address assignments, and link connections in a JSON file, which can be ported into the ION Configuration Tool to generate configuration files the ION software uses.

## Programs in ION

The following tools are available to you after ION is built:

Daemon and Configuration

* ionadmin is the administration and configuration interface for the local ION node contacts and manages shared memory resources used by ION.
* ltpadmin is the administration and configuration interface for LTP operations on the local ION node.
* bsspadmin is the administrative interface for operations of the Bundle Streaming Service Protocol on the local ion node.
* bpadmin is the administrative interface for bundle protocol operations on the local ion node.
* ipnadmin is the administration and configuration interface for the IPN addressing system and routing on the ION node. (ipn:)
* dtn2admin is the administration and configuration interface for the DTN addressing system and routing on the ION node. (dtn://)
* killm is a script which tears down the daemon and any running ducts on a single machine (use ionstop instead).
* ionstart is a script which completely configures an ION node with the proper configuration file(s).
* ionstop is a script which completely tears down the ION node.
* ionscript is a script which aides in the creation and management of configuration files to be used with ionstart.

Simple Sending and Receiving

- bpsource and bpsink are for testing basic connectivity between endpoints. bpsink listens for and then displays messages sent by bpsource.
- bpsendfile and bprecvfile are used to send files between ION nodes.

Testing and Benchmarking

- bpdriver benchmarks a connection by sending bundles in two modes: request-response and streaming.
- bpecho issues responses to bpdriver in request-response mode.
- bpcounter acts as receiver for streaming mode, outputting markers on receipt of data from bpdriver and computing throughput metrics.

## ION Logging

It is important to note that, by default, the administrative programs will all trigger the creation of a log file called ion.log in the directory where the program is called. This means that write-access in your current working directory is required. The log file itself will contain the expected log information from administrative daemons, but it will also contain error reports from simple applications such as bpsink. This is important to note since the BP applications may not be reporting all error information to stdout or stderr.

## Starting the ION Daemon

A script has been created which allows a more streamlined configuration and startup of an ION node. This script is called ionstart, and it has the following syntax. Don't run it yet; we still have to configure it!

`ionstart -I <filename>`

filename: This is the name for configuration file which the script will attempt to use for the various configuration commands. The script will perform a sanity check on the file, splitting it into command sections appropriate for each of the administration programs.

Configuration information (such as routes, connections, etc) can be specified one of two ways for any of the individual administration programs:

(Recommended) Creating a configuration file and passing it to ionadmin, bpadmin, ipnadmin... either directly or via the ionstart helper script.
Manually typing configuration commands into the terminal for each administration program.

You can find appropriate commands in the following sections.

##Configuration Files Overview

There are five configuration files about which you should be aware.

The first, ionadmin's configuration file, assigns an identity (node number) to the node, optionally configures the resources that will be made available to the node, and specifies contact bandwidths and one-way transmission times. Specifying the "contact plan" is important in deep-space scenarios where the bandwidth must be managed and where acknowledgments must be timed according to propagation delays. It is also vital to the function of contact-graph routing.

The second, ltpadmin's configuration file, specifies spans, transmission speeds, and resources for the Licklider Transfer Protocol convergence layer.

The third, ipnadmin's configuration file, maps endpoints at "neighboring" (topologically adjacent, directly reachable) nodes to convergence-layer addresses. Our examples use TCP/IP and LTP (over IP/UDP), so it maps endpoint IDs to IP addresses. This file populates the ION analogue to an ARP cache for the "ipn" naming scheme.

The fourth, bpadmin's configuration file, specifies all of the open endpoints for delivery on your local end and specifies which convergence layer protocol(s) you intend to use. With the exception of LTP, most convergence layer adapters are fully configured in this file.

The fifth, dtn2admin's configuration file, populates the ION analogue to an ARP cache for the "dtn" naming scheme.

## The ION Configuration File

Given to ionadmin either as a file or from the daemon command line, this file configures contacts for the ION node. We will assume that the local node's identification number is 1.

This file specifies contact times and one-way light times between nodes. This is useful in deep-space scenarios: for instance, Mars may be 20 light-minutes away, or 8. Though only some transport protocols make use of this time (currently, only LTP), it must be specified for all links nonetheless. Times may be relative (prefixed with a + from current time) or absolute. Absolute times, are in the format `yyyy/mm/dd-hh:mm:ss`. By default, the contact-graph routing engine will make bundle routing decisions based on the contact information provided.

The configuration file lines are as follows:

`1 1 ''`

This command will initialize the ion node to be node number 1.

1 refers to this being the initialization or ''first'' command.
1 specifies the node number of this ion node. (IPN node 1).
'' specifies the name of a file of configuration commands for the node's use of shared memory and other resources (suitable defaults are applied if you leave this argument as an empty string).

`s `

This will start the ION node. It mostly functions to officially "start" the node in a specific instant; it causes all of ION's protocol-independent background daemons to start running.

`a contact +1 +3600 1 1 100000`

specifies a transmission opportunity for a given time duration between two connected nodes (or, in this case, a loopback transmission opportunity).

a adds this entry in the configuration table.
contact specifies that this entry defines a transmission opportunity.
+1 is the start time for the contact (relative to when the s command is issued).
+3600 is the end time for the contact (relative to when the s command is issued).
1 is the source node number.
1 is the destination node number.
100000 is the maximum rate at which data is expected to be transmitted from the source node to the destination node during this time period (here, it is 100000 bytes / second).

`a range +1 +3600 1 1 1 `

specifies a distance between nodes, expressed as a number of light seconds, where each element has the following meaning:

a adds this entry in the configuration table.
range declares that what follows is a distance between two nodes.
+1 is the earliest time at which this is expected to be the distance between these two nodes (relative to the time s was issued).
+3600 is the latest time at which this is still expected to be the distance between these two nodes (relative to the time s was issued).
1 is one of the two nodes in question.
1 is the other node.
1 is the distance between the nodes, measured in light seconds, also sometimes called the "one-way light time" (here, one light second is the expected distance).

`m production 1000000 `

specifies the maximum rate at which data will be produced by the node.

m specifies that this is a management command.
production declares that this command declares the maximum rate of data production at this ION node.
1000000 specifies that at most 1000000 bytes/second will be produced by this node.

`m consumption 1000000 `

specifies the maximum rate at which data can be consumed by the node.

m specifies that this is a management command.
consumption declares that this command declares the maximum rate of data consumption at this ION node.
1000000 specifies that at most 1000000 bytes/second will be consumed by this node.

This will make a final configuration file host1.ionrc which looks like this:

```
1 1 ''
s
a contact +1 +3600 1 1 100000
a range +1 +3600 1 1 1
m production 1000000
m consumption 1000000
```

## The Licklider Transfer Protocol Configuration File

Given to ltpadmin as a file or from the command line, this file configures the LTP engine itself. We will assume the local IPN node number is 1; in ION, node numbers are used as the LTP engine numbers.

`1 32`

This command will initialize the LTP engine:

1 refers to this being the initialization or ''first'' command.
32 is an estimate of the maximum total number of LTP ''block'' transmission sessions - for all spans - that will be concurrently active in this LTP engine. It is used to size a hash table for session lookups.

`a span 1 32 32 1400 10000 1 'udplso localhost:1113'`

This command defines an LTP engine 'span':

a indicates that this will add something to the engine.

span indicates that an LTP span will be added.

1 is the engine number for the span, the number of the remote engine to which LTP segments will be transmitted via this span. In this case, because the span is being configured for loopback, it is the number of the local engine, i.e., the local node number. This will have to match an outduct in Section 2.6.

32 specifies the maximum number of LTP ''block'' transmission sessions that may be active on this span. The product of the mean block size and the maximum number of transmission sessions is effectively the LTP flow control ''window'' for this span: if it's less than the bandwidth delay product for traffic between the local LTP engine and this spa's remote LTP engine then you'll be under-utilizing that link. We often try to size each block to be about one second's worth of transmission, so to select a good value for this parameter you can simply divide the span's bandwidth delay product (data rate times distance in light seconds) by your best guess at the mean block size.

The second 32specifies the maximum number of LTP ''block'' reception sessions that may be active on this span. When data rates in both directions are the same, this is usually the same value as the maximum number of transmission sessions.

1400 is the number of bytes in a single segment. In this case, LTP runs atop UDP/IP on ethernet, so we account for some packet overhead and use 1400.

1000 is the LTP aggregation size limit, in bytes. LTP will aggregate multiple bundles into blocks for transmission. This value indicates that the block currently being aggregated will be transmitted as soon as its aggregate size exceeds 10000 bytes.

1 is the LTP aggregation time limit, in seconds. This value indicates that the block currently being aggregated will be transmitted 1 second after aggregation began, even if its aggregate size is still less than the aggregation size limit.

'udplso localhost:1113' is the command used to implement the link itself. The link is implemented via UDP, sending segments to the localhost Internet interface on port 1113 (the IANA default port for LTP over UDP).

`s 'udplsi localhost:1113'`

Starts the ltp engine itself:

s starts the ltp engine.

'udplsi localhost:1113' is the link service input task. In this case, the input ''duct' is a UDP listener on the local host using port 1113.

This means that the entire configuration file host1.ltprc looks like this:

```
1 32
a span 1 32 32 1400 10000 1 'udplso localhost:1113'
s 'udplsi localhost:1113'
```

##The Bundle Protocol Configuration File

Given to bpadmin either as a file or from the daemon command line, this file configures the endpoints through which this node's Bundle Protocol Agent (BPA) will communicate. We will assume the local BPA's node number is 1; as for LTP, in ION node numbers are used to identify bundle protocol agents.

`1` 

This initializes the bundle protocol:

1 refers to this being the initialization or ''first'' command.

`a scheme ipn 'ipnfw' 'ipnadminep' `

This adds support for a new Endpoint Identifier (EID) scheme:

a means that this command will add something.

scheme means that this command will add a scheme.

ipn is the name of the scheme to be added.

'ipnfw' is the name of the IPN scheme's forwarding engine daemon.

'ipnadminep' is the name of the IPN scheme's custody transfer management daemon.

`a endpoint ipn:1.0 q `

This command establishes this BP node's membership in a BP endpoint:

a means that this command will add something.

endpoint means that this command adds an endpoint.

ipn is the scheme name of the endpoint.

1.0 is the scheme-specific part of the endpoint. For the IPN scheme the scheme-specific part always has the form nodenumber:servicenumber. Each node must be a member of the endpoint whose node number is the node's own node number and whose service number is 0, indicating administrative traffic.

q means that the behavior of the engine, upon receipt of a new bundle for this endpoint, is to queue it until an application accepts the bundle. The alternative is to silently discard the bundle if no application is actively listening; this is specified by replacing q with x.

`a endpoint ipn:1.1 q `

`a endpoint ipn:1.2 q `

These specify two more endpoints that will be used for test traffic.

`a protocol ltp 1400 100 `

This command adds support for a convergence-layer protocol:

a means that this command will add something.

protocol means that this command will add a convergence-layer protocol.

ltp is the name of the convergence-layer protocol.

1400 is the estimated size of each convergence-layer protocol data unit (in bytes); in this case, the value is based on the size of a UDP/IP packet on Ethernet.

100 is the estimated size of the protocol transmission overhead (in bytes) per convergence-layer procotol data unit sent.

`a induct ltp 1 ltpcli `

This command adds an induct, through which incoming bundles can be received from other nodes:

a means that this command will add something.

induct means that this command will add an induct.

ltp is the convergence layer protocol of the induct.

1 is the identifier of the induct, in this case the ID of the local LTP engine.

ltpcli is the name of the daemon used to implement the induct.

`a outduct ltp 1 ltpclo `

This command adds an outduct, through which outgoing bundles can be sent to other nodes:

a means that this command will add something.

outduct means that this command will add an outduct.

ltp is the convergence layer protocol of the outduct.

1 is the identifier of the outduct, the ID of the convergence-layer protocol induct of some remote node. See Section 2.5 for remote LTP engine IDs.

ltpclo is the name of the daemon used to implement the outduct.

`s`

This command starts the bundle engine including all daemons for the inducts and outducts.

That means that the entire configuration file host1.bprc looks like this:

```
1
a scheme ipn 'ipnfw' 'ipnadminep'
a endpoint ipn:1.0 q
a endpoint ipn:1.1 q
a endpoint ipn:1.2 q
a protocol ltp 1400 100
a induct ltp 1 ltpcli
a outduct ltp 1 ltpclo
s
```

## IPN Routing Configuration

As noted earlier, this file is used to build ION's analogue to an ARP cache, a table of ''egress plans.'' It specifies which outducts to use in order to forward bundles to the local node's neighbors in the network. Since we only have one outduct, for forwarding bundles to one place (the local node), we only have one egress plan.

`a plan 1 ltp/1 `

This command defines an egress plan for bundles to be transmitted to the local node:

a means this command adds something.

plan means this command adds an egress plan.

1 is the node number of the remote node. In this case, that is the local node's own node number; we're configuring for loopback.

ltp/1 is the identifier of the outduct through which to transmit bundles in order to convey them to this ''remote'' node.

This means that the entire configuration file host1.ipnrc looks like this:

`a plan 1 ltp/1`

## Testing Your Connection

Assuming no errors occur with the configuration above, we are now ready to test loopback communications. In one terminal, we have to run the start script (the one we said that you would have to have earlier). It's right here, in case you forgot to write it down:

`ionstart -i host1.ionrc -l host1.ltprc -b host1.bprc -p host1.ipnrc `

This command will run the appropriate administration programs, in order, with the appropriate configuration files. Don't worry that the command is lengthy and unwieldly; we will show you how to make a more clean single configuration file later.

Once the daemon is started, run:

`bpsink ipn:1.1 `

This will begin listening on the Endpoint ID with the endpoint_number 1 on service_number 1, which is used for testing.

Now open another terminal and run the command:

`bpsource ipn:1.1`

This will begin sending messages you type to the Endpoint ID ipn:1.1, which is currently being listened to by bpsink. Type messages into bpsource, press enter, and see if they are reported by bpsink.

If so, you're ready for bigger and better things. If not, check the following:

Do you have write permissions for your current directory? If not, you will not be able to start the daemon as it has to write out to the ion.log file.
Are your config files exactly as specified, except for IP address changes?
Are you running it on one of our supported platforms? Currently, those are the only supported distributions.

If you are still having problems, you can ask for help on the ION users' list or file an ION bug report.

## Stopping the Daemon

As the daemon launches many ducts and helper applications, it can be complicated to turn it all off. To help this, we provided a script. The script similar to ionstart exists called ionstop, which tears down the ion node in one step. You can call it like so:

`ionstop `

After stopping the daemon, it can be restarted using the same procedures as outlined above. Do remember that the ion.log file is still present, and will just keep growing as you experiment with ION.

IMPORTANT: The user account that runs ionstart must also run ionstop. If that account does not, no other accounts can successfully start the daemon, as the shared memory vital to ION's functionality will already be occupied.

## More Advanced Usage

Detailed documentation of ION and its applications are available via the man pages. It is suggested that you start with man ion , as this is an overview man page listing all available ION packages.

## Ionscript for Simplified Configuration Files

The most difficult and cumbersome method of starting an ION node is to manually run the various administration programs in order, manually typing configuration commands all the way. It is much more efficient and less error-prone to place the configuration commands into a configuration file and using that as input to the administration program, but this is still cumbersome as you must type in each administration program in order. The ionstart program will automatically execute the appropriate administration programs with their respective configuration files in order. Unfortunately, as seen in the previous sections, the command is lengthy. This is why the ionscript script was added to make things even easier.

The ionscript will basically concatenate the configuration files into one large file. The format of this large configuration file is simply to bookend configuration sections with the lines: ## begin PROGRAM and ## end PROGRAM, where PROGRAM is the name of the administration program for which the configuration commands should be sent (such as ionadmin, bpadmin, ipnadmin).

To create a single file host1.rc out of the various configuration files defined in the previous section, run this command:

`ionscript -i host1.ionrc -p host1.ipnrc -l host1.ltprc -b host1.bprc -O host1.rc `

The command can also be used to split the large host1.rc into the individual configuration files (so long as the large file is formatted correctly). Just run this command to revert the process:

`ionscript -i host1.ionrc -p host1.ipnrc -l host1.ltprc -b host1.bprc -I host1.rc `

This isn't very practical in this specific case (as you already have the individual files) but if you start with a single configuration file, this can be helpful.

Once you have a single configuration file, starting the ION node is a single command:

`ionstart -I host1.rc `

Note that ionstart and ionscript require sed and awk, but those are almost universally available on Unix-based systems. The two scripts will always sanity-check the large configuration file to ensure that it interprets the bookend lines correctly- and it will warn you of any errors you have made in the file. Consult the USAGE for each script for further help, by attempting to run the script with no arguments or the -h argument.

## Examples of Network Configurations

For a simple single-node ION configuration - running multiple instances of ION in the same host, see the tutorial [here.](community/dtn-gcp-main/ION-One-Node-on-Cloud-Linux-VM.md)

For a two-node configuration, see the tutorial [here.](community/dtn-gcp-2nodes/ION-Two-Node-on-Cloud-Linux-VMs.md)

For a multi-hop and also multi-network configuration, see this [page.](Configure-Multiple-Network-Interfaces.md)
