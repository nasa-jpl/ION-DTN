# ION Quick Start Guide

## Installing ION on Linux, MacOS, Solaris

To build and install the entire ION system on a Linux, MacOS, or Solaris platform, cd into ion-open-source and enter the following commands:

`./configure`

If configure is not present run: `autoreconf -fi` first

`make`

`sudo make install`

`sudo ldconfig`

For MacOS,  the `ldconfig` command is not present and not necessary to run.

### Compile Time Switches

If you want to set overriding compile-time switches for a build, the place to do this is in the `./configure` command.  For details,

`./configure -h`

By default, Bundle Protocol V7 will be built and installed, but BPv6 source code is still available. The BPv6 implementation is essentially the same as that of ION 3.7.4, with only critical bugs being updated going forward. All users are encouraged to switch to BPV7.

To build BPv6, run

`./configure --enable-bpv6`

To clean up compilation artifacts such as object files and shared libraries stored within the ION open-source directory, cd to the ION open-source directory and run:

`make clean`

To remove executables and shared libraries installed in the system, run:

`sudo make uninstall`

## Windows

To install ION for Windows, please download the Windows installer.

## Build Individual Packages

It's also possible to build the individual packages of ION, using platform-specific Makefiles in the package subdirectories.  Currently  the only actively maintained platform-specific Makefile is for 64-bits Linux under the "i86_48-fedora" folder. If you choose this option, be aware of the dependencies among the packages:

* The "ici" package must be built (run `make` and `make install`) before any other package.
* The "bp" package is dependent on "dgr" and "ltp" and "bssp" as well as "ici"
* The "cfdp", "ams", "bss", and "dtpc" packages are dependent on "bpv7"
* The "restart" package is dependent on "cfdp", "bp", "ltp", and "ici"

For more detailed instruction on building ION, see section 2 of the "ION Design and Operation Guide" document that is distributed with this package.

Also, be aware that these Makefiles install everything into subdirectories of /usr/local.  To override this behavior, change the value of `OPT` in the top-level Makefile of each package.

Additional details are provided in the README.txt files in the root directories of some of the subsystems.

Note that all Makefiles are for gmake; on a FreeBSD platform, be sure to install gmake before trying to build ION.

## Running ION

### Check Installed BP and ION versions

Before running ION, let's confirm which version of Bundle Protocol is installed by running:

`bpversion`

You will see a simple string on the terminal windows indicating either "bpv6" or "bpv7".

Also check the ION version installed by running:

`ionadmin`

 At the  ":" prompt, please enter the single character command 'v' and you should see a response like this:

```
 $ ionadmin
: v
ION-OPEN-SOURCE-4.1.2
```

Then type 'q' to quit ionadmin. While ionadmin quits, it may display certain error messages like this:

```
at line 427 of ici/library/platform_sm.c, Can't get shared memory segment: Invalid argument (0)
at line 312 of ici/library/memmgr.c, Can't open memory region.
at line 367 of ici/sdr/sdrxn.c, Can't open SDR working memory.
at line 513 of ici/sdr/sdrxn.c, Can't open SDR working memory.
at line 963 of ici/library/ion.c, Can't initialize the SDR system.
Stopping ionadmin.
```

This is normal due to the fact that ION has not launched yet.

### Try the 'bping' test

The `tests` directory contains regression tests used by system integrator to check ION before issuing each new release. To make sure ION is operating properly after installation, you can also manually run the bping test:

First enter the test directory: `cd tests`

Enter the command: `./runtests bping/`

This command invokes one of the simplest test whereby two ION instances are created and a ping message is sent from one to the other and an echo is returned to the sender of the ping.

During test, ION will display the configuration files used, clean the system of existing ION instances, relaunch ION according to the test configuration files, execute bping actions, display texts that indicates what the actions are being executed in real-time, and then shutdown ION, and display the final test status message, which looks like this:

```
ION node ended. Log file: ion.log
TEST PASSED!

passed: 1
    bping

failed: 0

skipped: 0

excluded by OS type: 0

excluded by BP version: 0

obsolete tests: 0
```

In this case, the test script confirms that ION is able to execute a bping function properly.

### Try to Setup a UDP Session

Under the `demos` folder of the ION code directory, there are benchmark tests for various ION configurations. These tests also provide a template of how to configure ION.

Take the example of the `bench-udp` demo:

Go into the `demos/bench-udp/` folder, you will see two subfolders: `2.bench.udp` and `3.bench.udp`, these folders configures two ION nodes, one with node numbers 2 and 3.

Looking inside the `2.bench.udp` folder, you will see specific files used to configure ION. These include:

```
bench.bprc 
bench.ionconfig  
bench.ionrc  
bench.ionsecrc  
bench.ipnrc  
ionstart  
ionstop
```

* `bench.bprc` is the configuration file for the bundle protocol. To study the command options contained in this file, run `man bprc`.
* `bench.ionconfig` is the configuration file for the storage configuration of ION. See `man ionconfig` for details.
* `bench.ionrc` is the configuration file for ION. See `man ionrc` for details.
* `bench.ionsecrc` is the configuration file for ION security administration. See `man ionsecrc` for details.
* `bench.ipnrc` is the configuration file for the IPN  scheme. See `man ipnrc` for details.
* `ionstart` and `ionstop` are scripts to launch and shutdown ION.

One must note that ION distribution comes with a separate, global `ionstart` and `ionstop` scripts installed in `/usr/local/bin` that can launch and stop ION. The advantage of using local script is that it allows you customize the way you launch and stop ION, for example add helpful text prompt, perform additional checks and clean up activities, etc.

To run this demo test, first go into the test directory bench-udp, then run the dotest script:

`./dotest`

You can also study the test script to understand better what is happening.

## Running multiple ION instances on a single host

If you study the test script under the "tests" and the "demos" folders, you will realize that these tests often will launch 2 or 3 ION nodes on the same host to conduct the necessary tests. While this is necessary to simplify and better automate regression testing for ION developer and integration, it is not a typical, recommended configuration for new users.

In order to run multiple ION instances in one host, specific, different IPCS keys must be used for each instance, and several  variables must be set properly in the shell environment. Please see the ION Deployment Guide (included with the ION distribution) for more information on how to do that.

We recommend that most users, unless due to specific contrain that they must run multiple ION instance on one host, to run each ION instance on a separate host or (VM).

## Setup UDP Configuration on Two Hosts

Once you have studied these scripts, you can try to run it on two different machines running ION.

First, install ION in host A with an IP address of, for example, 192.168.0.2, and host B with an IP address of 192.168.0.3. Verify your installation based on earlier instructions.

Copy the `2.bench.udp` folder into host A and the `3.bench.udp` folder into host B.

Also copy the file `global.ionrc` from the `bench.udp` folder into the same folder where you placed `2.bench.udp` and `3.bench.udp`

Then you need to modify the IP addresses in the UDP demo configuration files to match the IP addresses of hosts A and B.

For example, the bprc files copied into host A is:

```
1
a scheme ipn 'ipnfw' 'ipnadminep'
a endpoint ipn:2.0 x
a endpoint ipn:2.1 x
a endpoint ipn:2.2 x
a endpoint ipn:2.64 x
a endpoint ipn:2.65 x
a protocol udp 1400 100
a induct udp 127.0.0.1:2113 udpcli
a outduct udp 127.0.0.1:3113 'udpclo 1'
r 'ipnadmin bench.ipnrc'
s
```

To make it work for host A, you need to replace the induct ip address `127.0.0.1:2113` to `192.168.0.2:2113` - this is where host A's ION will receive incoming UDP traffic.

Similarly for outduct, you want to change the ip address from `127.0.0.1:3113` to `192.168.0.3:3113` - this is where UDP traffic will go out to host B.

You can make similar modifications to the ipnrc file as well.

In the ionconfig file, you want to comment out or delete the `wmKey` entry. Since we are running these two nodes on different hosts, we recommend not specifying any IPC key values but let ION use the default value.

Repeat the same updates for host B by appropriately substituting old IP address to that of the new hosts.

## Launch ION on two separate hosts

After updating the configuration files on host A and B to reflect the new IP addresses and using default wmKey (by not specifying any), we are new ready to try launching ION.

Before you try to launch ION, it is recommended that you:

1. Use netcat or iperf to test the connection between host A and B. Make sure it is working properly. That means have a sufficiently high data rate and low loss rate (low single digit percent or fraction of a percent should not be a concern).
2. If iperf tests show that the data rate between the two hosts are at or above 800 megabits per second, in both directions, and the UDP loss rate is no more than a few percent, then you are good to go.
3. If not, then you want to reduce the data rate in the `global.ionrc` file, change the data rates for the `a contact` command down to something similar to your connection speed. Remember, the unit in the `global.ionrc` file is Bytes per second, not bits per second, which is typically what iperf test report uses.
4. If the error rate is high, you may want to check both the physical connection or kernel buffer setting.
5. Check firewall setting and MTU setting may help you narrow down problems.
6. Using wireshark can also be helpful both for initial connection check as well as during ION testing.

Once you are ready to launch ION on both host A and B, open a terminal and go to the directory where the configuration files are stored, and run the local ionstart script:

`./ionstart`

Note: do not run `ionstart` since that will trigger the global script in the execution PATH

You should see some standard output confirming that ION launch has completed. For example you might see something like this:

```
Starting ION...
wmSize:          5000000
wmAddress:       0
sdrName:        'ion2'
sdrWmSize:       0
configFlags:     1
heapWords:       100000000
heapKey:         -1
logSize:         0
logKey:          -1
pathName:       '/tmp'
Stopping ionadmin.
Stopping ionadmin.
Stopping ionsecadmin.
Stopping ltpadmin.
Stopping ipnadmin.
Stopping bpadmin.
```

You can also see additional status information in the `ion.log` file in the same directory.

Launch ION on both host A and B.

## Run a bpdriver-bpcounter test

Now that we have launched ION on both host A and B, it's time to send some data.

We can repeat the bping test at this point. But since you have already seen that before, let's try something different.

Let's use the bpdriver-bpcounter test utilities. This pair of utility programs simply sends a number of data in bundles from one node to another and provides a measurement on the throughput.

On host B, run this command:

`bpcounter ipn:3.2 3`

This command tells ION node number 3 to be ready to receive three bundles on the end-point ID `ipn:3.2` which was specified in the `.bprc` file.

After host B has launched bpcounter, then on host A, run this command:

`bpdriver 3 ipn:2.2 ipn:3.2 -10000`

This command tells ION running in host A to send 3 bundles from EID 2.2 to EID 3.2, which is waiting for data (per bpcounter command.) And each bundle should be 10,000 bytes in size.

Why use the "-" sign in front of the size parameter? It's not a typo. The "-" indicates that bpdriver should keep sending bundles without waiting for any response from the receiver. The feature where bpdriver waits for the receiver is available in BPv6 but no longer part of BPv7.

When the test completed, you should see output indicating that all the data were sent, how many bundles were transmitted/received, and at what rate.

Please note that on the sending side the transmission may appear to be almost instantaneous. That is because bpdriver, as an application, is pushing data into bundle protocol which has the ability to rate buffer the data. So as soon as the bpdriver application pushes all data into the local bundle protocol agent, it considers the transmission completed and it will report a very high throughput value, one that is far above the contact graph's data rate limit. This is not an error; it simple report the throughput as experienced by the sending application, knowing that the data has not yet delivered fully to the destination.

Throughput reported by bpcounter, on the other hand, is quite accurate if a large number of bundles are sent. To accurately measure the time it takes to send the bundles, bpdriver program will send a "pilot" bundle just before sending the test data to signals to the bpcounter program to run its throughput calculation timer. This allows the user to run bpcounter and not haveing to worry about immediately send all the bundles in order to produce an accurate throughput measurement.

If you want to emulate the action of a constant rate source, instead of having bpdriver pushing all data as fast as possible, then you can use the 'i' option to specify a data rate throttle in bits per second.

If you want to know more about how bpdriver and bpcounter work, look up their man pages for details on syntax and command line options. Other useful ION test utility commands include `bpecho`, `bping`, `bpsource`, `bpsink`, `bpsendfile`, `bprecvfile`, etc.

## Check the ion.log

To confirm whether ION is running properly or has experienced an error, the first thing to do is to check the ion.log, which is a file created in the directory from which ION was launched. If an ion.log file exists when ION starts, it will simply append additional log entries into that file. Each entry has a timestamp to help you determine the time and the relative order in which events occurred.

When serious error occurs, ion.log  will have detailed messages that can pinpoint the name  and line number of the source code where the error was reported or triggered.

## bpacq and ltpacq files

Sometimes after operating ION for a while, you will notice a number of files with names such as "bpacq" or "ltpacq" followed by a number. These are temporary files created by ION to stage bundles or LTP blocks during reception and processing.  Once a bundle or LTP block is completely constructed, delivered, or cancelled properly, these temporary files are automatically removed by ION. But if ION experiences an anomalous shutdown, then these files may remain and accumulate in the local directory.

It is generally safe to remove these files between ION runs. Their presence does not automatically imply issues with ION but can indicate that ION operations were interrupted for some reason. By noting their creation time stamp, it can provide clues on when these interruptions occurred. Right now there are no ION utilty program to parse them because these files are essentially bit buckets and do not contain internal markers or structure and allows user to parse them or extract information by processes outside the bundle agents that created them in the first place.

## Forced Shutdown of ION

Sometimes shutting down ION does not go smoothly and you can't seem to relaunch ION properly. In that case, you can use the global `ionstop` script (or the `killm` script) to kill all ION processes that did not terminate using local ionstop script. The global ionstop or killm scripts also clears out the IPC shared memory and semaphores allocations that were locked by ION processes and would not terminate otherwise.

---

# Additional Tutorials

### ION Configuration File Tutorial

To learn about the configuration files and the basic set of command syntax and functions:
[ION Config File Tutorial](https://sourceforge.net/p/ion-dtn/wiki/ION%20Configuration%20File%20Tutorial/)

### ION Configuration File Template

[ION Config File Template](https://sourceforge.net/p/ion-dtn/wiki/ION%20Configuration%20File%20Templates/)

### ION NASA Course

To learn more about the design principle of ION and how to use it, a complete series of tutorials is available here:
[NASA ION Course](https://www.nasa.gov/directorates/heo/scan/engineering/technology/disruption_tolerant_networking_software_options_ion)

If you use the ION Dev Kit mentioned in the NASA ION Course, you can find some additional helpful files here:
[Additional DevKit Files](https://sourceforge.net/p/ion-dtn/wiki/DevKit%20-%20additional%20materials/)

---

# Accessing ION Open-Source Code Repository

## Releases

Use the Summary or the Files tab to download point releases

## Using the code repository

- Track the "stable" branch to match the ION releases
- Track the "current" branch for bug fixes and small updates between releases

---

# Contributing Code to ION

## Expectations

If you plan to contribute to the ION project, please keep these in mind:

- Submitted code should adhere to the ION coding style found in the current code. We plan to add a more formal coding style guide in the future.
- Provide documentation describing the contributed code’s features, its inputs and outputs, dependencies, behavior (provide a high-level state machine or flowchart if possible), and API description. Please provide a draft of a man page.
- Provide canned tests (ION configuration and script) that can be executed to verify and demonstrate the proper functioning of the features. Ideally it should demonstrate nominal operation and off-nominal scenarios.
- The NASA team will review these contributions and determine to either

  1. incorporate the code into the baseline, or
  2. not incorporate the code into the baseline but make it available in the /contrib folder (if possible) as experimental modules, or
  3. not incorporate it at all.
- All baselined features will be supported with at least bug-fixes until removed
- All /contrib folder features are provided ”as is,” and no commitment is made regarding bug-fixes.
- The contributor is expected to help with regression testing.
- Due to resource constraints, we cannot make any commitment as to response time. We will do our best to review them on a best effort basis.

## If you want to contribute code to ION

1. Fork this repository
2. Starting with the "current" branch, create a named feature or bugfix branch and develop/test your code in this branch
3. Generate a pull request (called Merge Request on Source Forge) with

   - Your feature or bugfix branch as the Source branch
   - "current" as the destination branch
