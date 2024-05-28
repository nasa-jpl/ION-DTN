# Available Configuration File Templates

## Two-node ION configurations:

* Two-Node BP over UDP Configuration [download](config-templates/bench-udp.tar.gz)
* Two-Node BP over TCP Configuration [download](config-templates/bench-tcp.tar.gz)
* Two-Node BP over LTP Configuration [download](config-templates/bench-ltp.tar.gz)
* Two-Node BP over BSSP Configuration [download](config-templates/bench-bssp.tar.gz)
* Two-Node BP over STCP Configuration [download](config-templates/bench-stcp.tar.gz)
* Two-Node CFDP over BP over LTP Configuration [download](config-templates/bench-cfdp.tar.gz) - in this CFDP configuration file, we also included a CFDP command file that can be used with  cfdptest  program to send and receive data. Please use `man cfdptest` to get information on command syntax and functions.
* Two-Node Datagram Retransmission (DGR) [download](config-templates/bench-dgr.tar.gz)
* Two-Node Bundle-In-Bundle Encapsulation with Custody Transfer (BIBE-CT) [download](config-templates/bench-bibect-udp.tar.gz)

## Three-node ION Configuration

* Three-Node 'spacecraft-relay-ground' scenario with BPv7 and LTP [download](config-templates/3-node-GS-Relay-SC-LTP.zip)
  
## Notes

The choice of IP addresses, port IDs, contact delay ('range') and data rate, and memory/storage for 'heap', 'SDR working memory', 'ION working memory', etc, do not indicate a recommended setting. The approproiate setting for ION must be tailored to your specific use case.

To understand the full range of configuration parameters, please consult the appropriate man pages for more details: for memory configuration, see `man ionconfig`, for ION and contact graph configurations, see `man ionrc`, For bundle protocol and LTP protocol configurations options, see `man bprc`, `man ltprc`, etc.

To launch each ION node, cd into the configuration folder and execute the appropriate shell script that will launch ION by running a series of administration programs. This script is   called `./ionstart` or `./start_<name name>` in these examples but they can be called anything.

To terminate each ION node, please utilize a globall installed `killm` or `ionstop` script.

You can write and use your own script to shutdown ION from the local directory. The advantage of using your own script is that you can also clean up, and post-process data generated during the ION execution such as removing unwanted files and storing/copying the `ion.log` to a different location.

These configuration files are provided to give you a basic functional setup; they may not be sufficient to support all features and achieve the throughput performance needed by your applications. Please use them as a template and modify/optimize as necessary.
