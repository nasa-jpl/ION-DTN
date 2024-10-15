# DTN 101 - Running the Interplanetary Internet on Cloud VM 

This project has been developed by Dr Lara Suzuki, a visiting researcher at NASA JPL.

## Introduction

In this project we demonstrate how to run DTN on a cloud VM using NASA's implementation of the bundle protocol - ION. DTN stands for delay-tolerant and disruption-tolerant networks.
> *"It is an evolution of the architecture originally designed for the Interplanetary Internet, a communication system envisioned to provide Internet-like services across interplanetary distances in support of deep space exploration"* [Cerf et al, 2007](https://tools.ietf.org/html/rfc4838). 

The ION (interplanetary overlay network) software is a suite of communication protocol implementations designed to support mission operation communications across an end-to-end interplanetary network, which might include on-board (flight) subnets, in-situ planetary or lunar networks, proximity links, deep space links, and terrestrial internets.

## Getting Started with Cloud Linux VMs

On your preferred Cloud provider dashboard, create a Linux VM (e.g. Debian).

When prompted, select the reagion closest to you. If you are prompted to select the machine type, select the configuration that will suit your needs. I have selected the a machine which has 2 virtual CPUs and 4 GB memory.

For boot disk, in this tutorial we are using Debian GNU/Linux 10 (buster). In firewall configurations, I have selected it Allow HTTP and HTTPS.

Once the VM is started you can `SSH` directly into the VM.

## SSH in the Cloud Linux VM Instance

Mac and Linux support SSH connection natively. You just need to generate an SSH key pair (public key/private key) to connect securely to the virtual machine.

To generate the SSH key pair to connect securely to the virtual machine, follow these steps:

1. Enter the following command in Terminal: `ssh-keygen -t rsa .` 
2. It will start the key generation process. 
3. You will be prompted to choose the location to store the SSH key pair. 
4. Press `ENTER` to accept the default location
5. Now run the following command: `cat ~/.ssh/id_rsa.pub .` 
6. It will display the public key in the terminal. 
7. Highlight and copy this key

Back in the Cloud VM tools, follow your provider's direction on how to SSH into the VM. If you are requested to provide your SSH Keys, locate the SSH key file in your computer and inform it here.

Now you can just open your terminal on your Mac or Linux machine and type `ssh IP.IP.IP.IP` and you will be on the VM (IP.IP.IP.IP is the external IP of the VM).


## Getting Started with  ION

This example uses ION version 4.0.1, which can be downloaded [here](https://sourceforge.net/projects/ion-dtn/files). ION 4.0.1 uses the [version 7 of the Bundle Protocol](https://tools.ietf.org/html/draft-ietf-dtn-bpbis-31).

On your VM execute the following commands

````
$ sudo apt update
$ sudo apt install build-essential -y
$ sudo apt-get install wget -y
$ wget https://sourceforge.net/projects/ion-dtn/files/ion-open-source-4.0.1.tar.gz/download
$ tar xzvf download
````


### Compiling ION using autotools

Follow the standard autoconf method for compiling the project. In the **base ION directory** run:

```
$ ./configure
```
Then compile with:
```
$ make
````
Finally, install (requires root privileges):
```
$ sudo make install
```

For Linux based systems, you may need to run `sudo ldconfig` with no arguments after install.

### Programs in ION

The following tools are a few examples of programs availale to you after ION is built:

**1. Daemon and Configuration:**
- `ionadmin` is the administration and configuration interface for the local ION node contacts and manages shared memory resources used by ION.
- `ltpadmin` is the administration and configuration interface for LTP operations on the local ION node.
- `bsspadmin` is the administrative interface for operations of the Bundle Streaming Service Protocol on the local ion node.
- `bpadmin` is the administrative interface for bundle protocol operations on the local ion node.
- `ipnadmin` is the administration and configuration interface for the IPN addressing system and routing on the ION node. (ipn:)
- `ionstart` is a script which completely configures an ION node with the proper configuration file(s).
- `ionstop` is a script which cleanly shut down all of the daemon processes.
- `killm` is a script which releases all of the shared-memory resources that had been allocated to the state of the node.  This actually destroys the node and enables a subsequent clean new start (the “ionstart” script) to succeed.
- `ionscript` is a script which aides in the creation and management of configuration files to be used with ionstart.

**2. Simple Sending and Receiving:**
- `bpsource` and `bpsink` are for testing basic connectivity between endpoints. bpsink listens for and then displays messages sent by bpsource.
- `bpsendfile` and `bprecvfile` are used to send files between ION nodes.

**3. Testing and Benchmarking:**
- `bpdriver` benchmarks a connection by sending bundles in two modes: request-response and streaming.
- `bpecho` issues responses to bpdriver in request-response mode.
- `bpcounter` acts as receiver for streaming mode, outputting markers on receipt of data from bpdriver and computing throughput metrics.

**4. Logging:**
- By default, the administrative programs will all trigger the creation of a log file called `ion.log` in the directory where the program is called. This means that write-access in your current working directory is required. The log file itself will contain the expected log information from administrative daemons, but it will also contain error reports from simple applications such as bpsink. 

### The Configuration Files

Below we present the configuration files that you should be aware and configure for ION to execute correctly. 

1. `ionadmin's` configuration file, assigns an identity (node number) to the node, optionally configures the resources that will be made available to the node, and specifies contact bandwidths and one-way transmission times. Specifying the "contact plan" is important in deep-space scenarios where the bandwidth must be managed and where acknowledgments must be timed according to propagation delays. It is also vital to the function of contact-graph routing - [How To](ion-config.md)

2. `ltpadmin's` configuration file, specifies spans, transmission speeds, and resources for the Licklider Transfer Protocol convergence layer - [How To](ltp-config.md)

3. `bpadmin's` configuration file, specifies all of the open endpoints for delivery on your local end and specifies which convergence layer protocol(s) you intend to use. With the exception of LTP, most convergence layer adapters are fully configured in this file - [How To](bp-config.md)

4. `ipnadmin's` configuration file, maps endpoints at "neighboring" (topologically adjacent, directly reachable) nodes to convergence-layer addresses. This file populates the ION analogue to an ARP cache for the "ipn" naming scheme - [How To](ipn-config.md)

5. `ionsecadmin's` configuration file, enables bundle security to avoid error messages in ion.log - [How To](ionsec-config.md)

### Testing and Stopping your Connection
Assuming no errors occur with the configuration files above, we are now ready to test loopback communications, and also learn how to properly stop ION nodes. The below items are covered in this [How To](Running-ION.md) page.

1. Testing your connection
2. Stopping the Daemon
3. Creating a single configuration file
