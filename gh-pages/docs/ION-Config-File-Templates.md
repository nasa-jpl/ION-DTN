# Available Configuration File Templates

The following configurations can be downloaded (see file attachment)

* Two-Node BP over UDP Configuration [download](config-templates/bench-udp.tar.gz)
* Two-Node BP over TCP Configuration [download](config-templates/bench-tcp.tar.gz)
* Two-Node BP over LTP Configuration [download](config-templates/bench-ltp.tar.gz)
* Two-Node BP over BSSP Configuration [download](config-templates/bench-bssp.tar.gz)
* Two-Node BP over STCP Configuration [download](config-templates/bench-stcp.tar.gz)
* Two-Node CFDP over BP over LTP Configuration [download](config-templates/bench-cfdp.tar.gz)
  * In this CFDP configuration file, we also included a CFDP command file that can be used with  cfdptest  program to send and receive data. Please use  man cfdptest for information on command syntax and functions. - bench-cfdp.tar.gz
* Two-Node 'dgr' [download](config-templates/bench-dgr.tar.gz)
* Two-Node 'bibe-ct-udp' - bench-bibect-udp.tar.gz [download](config-templates/bench-bibect-udp.tar.gz)
  * Check the README.txt file for further instruction on how to use the scenario for simple testing

## Usage Notes

* All ION nodes are placed in a subnetwork 192.168.3.0/24.
* Node number x, will be given IP address of 192.168.3.y where y = 20 + x. So node 2 will have address of 192.168.3.22.
* Some configurations uses specific port ID; there are no consistent patterns in the selection of port IDs. You may need to modify the port ID to work with your firewall settings.
* ION is configured with
* 50MB of ION working memory
* 20M words (1 word is 8 bytes for 64 bit OS, and 4 bytes for 32 bit OS) of heap space
* SDR is implemented in DRAM only
* SDR working memory is not specified, therefore default to 1MB only. This may not be sufficient when you have large quantity of data staged in ION. Please update based on your operational scenario.
* The communications link is configured for a minimal of 1 second delay but with varying data rates. Again, there are no right or wrong values. They should be updated according to your target test environment.
* To launch each ION node, enter the configuration folder and execute ./ionstart
* To terminate each ION node, please utilize the killm  command
* After each run there might be artifact left over in the working directory such as
* Temporary ltp and bp acquisition files (if the shutdown occurs while data acquisition did not complete),
* Temporary file created by bpdriver as bundle data payload
* BSSP database files with extension such as .tbl , .dat , and .lst , etc.
* ion.log
* You may consider clearing (deleting) these files before your next run.
* The number of end-points defined are limited. If you need more, please update .bprc file.

These configuration files are provided to give you a basic functional setup; they may not be sufficient to support all features and throughput performance you want to achieve for your network. So please use them as a template and apply update as necessary.
