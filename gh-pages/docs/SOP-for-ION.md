# Standard Operating Procedure Handbook for the Interplanetary Overlay Network (ION) Bundle Protocol Suite

Date: 2024-07-08

Version: 0

## Revision History

| Version | Date      | Applicable ION Version | Change Description |
| ------- | --------- | ---------------------- | ------------------ |
| 0       | 6/26/2024 | 4.1.3                  | Initial draft      |

## Contributors

* Sky DeBaun (JPL/Caltech)
* Jay Gao (JPL/Caltech)
* Patricia Lindner (Ohio University)
* Shawn Osterman (Ohio University)
* Nate Richard (JPL/Caltech)
* Leigh Torgerson (JPL/Caltech)

## Table of Content

- [Standard Operating Procedure Handbook for the Interplanetary Overlay Network (ION) Bundle Protocol Suite](#standard-operating-procedure-handbook-for-the-interplanetary-overlay-network-ion-bundle-protocol-suite)
  - [Revision History](#revision-history)
  - [Contributors](#contributors)
  - [Table of Content](#table-of-content)
  - [Introduction](#introduction)
  - [Document Scope](#document-scope)
  - [ION Online Documentations and Source Repositories](#ion-online-documentations-and-source-repositories)
  - [Case 1: Single User, Admin \& Developer](#case-1-single-user-admin--developer)
    - [Dependencies \[^1\]](#dependencies-1)
    - [Pre-installation Check](#pre-installation-check)
    - [Installation](#installation)
    - [Pre-Launch](#pre-launch)
    - [Launch ION](#launch-ion)
    - [Establish a shell session to interact with an ION instance](#establish-a-shell-session-to-interact-with-an-ion-instance)
    - [Real-time Commanding/Configuration](#real-time-commandingconfiguration)
    - [Shutdown ION](#shutdown-ion)
    - [Remove ION Installation](#remove-ion-installation)
    - [Check for running ION instance(s)](#check-for-running-ion-instances)
      - [Method 1: Check ION daemons](#method-1-check-ion-daemons)
      - [Method 2: Check IPC status](#method-2-check-ipc-status)
  - [Case 2: Shared Enviroment](#case-2-shared-enviroment)
    - [ION Installation \& Operations](#ion-installation--operations)
  - [SystemD](#systemd)
    - [SystemD Service Files](#systemd-service-files)
    - [SystemD Service File Notes](#systemd-service-file-notes)
    - [SystemD Service Operational Notes](#systemd-service-operational-notes)
    - [Permissions](#permissions)
      - [Configuration Files](#configuration-files)
      - [Log File(s)](#log-files)
  - [Appendix](#appendix)
    - [ION Shared Memory KEY](#ion-shared-memory-key)
      - [Shared Memory Segments](#shared-memory-segments)
      - [SVR4 Semaphores](#svr4-semaphores)
      - [POSIX Named Semaphores](#posix-named-semaphores)
    - [ION-related Daemons](#ion-related-daemons)
    - [Notes](#notes)

## Introduction

In this document, we provide a detailed SOP on how to deploy ION for
system administrators, software developers/testers, and ION network
operators.

## Document Scope

The initial draft of this document will focus on `Linux`, `Solaris`, and
`MacOS` using the ION open-source codebase released through Github.

We will address single-user and multi-user cases.

Future versions of this document will expand the SOP to include the ION
Core package.

During versions of this document, we intend to address specific
considerations for adopting the SOP to other platforms/OS such as:

* Windows Operating System
* Cloud Computing
* Embedded System

## ION Online Documentations and Source Repositories

* ION Open Source Online Documentation:
  [https://nasa-jpl.github.io/ION-DTN/](https://nasa-jpl.github.io/ION-DTN/)
* ION Open Source Git Repository:
  [https://github.com/nasa-jpl/ION-DTN](https://github.com/nasa-jpl/ION-DTN)
* ION Core:
  [https://github.com/nasa-jpl/ion-core](https://github.com/nasa-jpl/ion-core)

## Case 1: Single User, Admin & Developer

We start with the simplest configuration where a single user has full control over a host or a VM within a secured envrionment within which ION will execute. The host and VM have no other purpose other than the development and testing of ION.

### Dependencies [^1]

* `gcc`
* `make`
* `automake`
* `autoconf`
* `libtool`
* `bash` - (optional) Many scripts are written for bash shell; having bash installed in the system will ensure these scripts execute predictably.
* `python3` - (optional) Python is used for some BPSec regression tests but is not strictly required to build and run ION.
* `sudo` - (optional) While sudo privilege is required during the last step of a nominal installation on Linux to execute the shared library update. However, it is not strictly required to use ION if it is installed in a directory owned by the user with a properly updated library and execution paths.
* `net-tools` - (optional) useful for checking network connectivity and performance for ION troubleshooting.

### Pre-installation Check

If the user plans to install only one version of ION, then the user should:

1. [Check for running ION instances](#check-for-running-ion-instances) and [shutdown ION](#shutdown-ion-1).
2. [Check for an existing installation of ION](#check-for-ion-installation) and [remove ION](#remove-ion-installation).

If the user plans to install multiple versions of ION in different directories, then the user should:

1. [Determine the location of any existing installation of ION](#check-for-ion-installation).
2. Select a different prefix (directory) for the installation.

### Installation

1. **Download the ION release from GitHub and verify the checksum of the tarfile**.

2. **Extract ION into the source directory of your choice**.

3. **In a shell environment, change to ION source code's root directory**:
    * Verify the ION version by checking the content of the `release_notes.txt` file in ION's root directory. The release version will be stated in the first line of the file.
    * Check for the presence of the `configure` script in the ION root directory - if not present, run `autoreconf -fi` to generate the `configure` script.
    * Check the execution path in your system, e.g., `echo $PATH` and make a note whether `/usr/local/bin` or your intended installation directory is part of it.

4. **Run `./configure` to build the Makefile**:
    * If the Makefile already reflects the configuration you wanted, say from a previous `./configure` run, you don't need to do it again. Skip to the next step.
    * Run `./configure --help` to see a list of options.
    * For Solaris OS, it is recommended to add the argument `MAKE=gmake`.
    * If you want to install bundle protocol version 6 (BPv6) instead of the most current BPv7 implementation, then include the `--enable-bpv6` option.
    * Check the execution path to make sure where you intend to install ION is in the `PATH` variable or will be added later.
    * To install ION to a directory other than the default of `/usr/local`, use the `--prefix` option.

5. **Build ION**:
    * Run `make` to build ION.
        * If you know ION was built before, either under a different set of configuration options, or just want to ensure everything is rebuilt correctly, run `make clean` first.
        * If using Solaris, you could use `gmake` instead.

6. **Install ION**:
    * Run `sudo make install` or `make install` to install ION. Whether `sudo` is needed will depend on the permissions required to write to the installation directory. Using the default installation location of `/usr/local` requires sudo privilege.
        * If you noticed prior installations of ION were not removed, run `sudo make uninstall` or `make uninstall` (depending on where you are installing the executables and the libraries). This assumes that the prior installation is using essentially the same ION version and the same installation directory. If the situation is more complicated, you should try the [remove ION installation](#remove-ion-installation) procedure.
        * If using Solaris, you could use `gmake` instead.

7. **Run `sudo ldconfig`**:
    * At the end of the installation, you may need to run this command to update the shared library management in the operating system.
        * For Solaris and MacOS, `ldconfig` is not required.
        * If you install ION in a directory owned by the user, `ldconfig` is not useful because it manages only system-wide shared libraries. In that case, you need to update the shared library separately in the shell environment in which you plan to execute ION:

    ```sh
    export LD_LIBRARY_PATH=/path/to/your/libs:$LD_LIBRARY_PATH
    ```

8. **(Optional) Run tests**:
    * Run `make test` to execute the C code-based regression tests in ION.
        * Verify that the `bping` test at the end passed.

9. **Verify ION installation location**:
    * Run `which bpversion`; the path shown should be either `/usr/local/bin` or the bin directory under the prefix you specified.
    * Verify installed Bundle Protocol version by running `bpversion`.
    * Verify ION version by running `ionadmin`, then followed by the `v` command at the prompt. Type `q` to quit.
    * Verify ION operation by running the basic `bping` test. You can skip this if you ran `make test` earlier, which would automatically trigger the `bping` test.

    ```sh
    cd tests
    ./runtests bping
    ```

### Pre-Launch

1. **Check for running instances of ION** by following these [steps](#check-for-running-ion-instances).
2. If you find ION instances running in the system that are not part of a multi-ION configuration you are participating in, or if you intend to launch a single-instance ION configuration in this host, follow the [Shutdown](#shutdown-ion) procedure to stop all ION instances in the system.
3. **Load ION configuration files** into the predetermined working directory and `cd` into that directory.
4. **Verify all required `.rc` files and the contact graph file (if used) are present in the working directory**.
5. **Remove files created by previous ION runs & remove them as necessary**:
    * `ion.log` in the current directory.
    * BSS database files: `*.lst`, `*.dat`, and `*.tbl`.
    * Check for temporary files left over from previous failed BP and LTP acquisition processes, typically found in the working directory: `bpacq.<integer>` and `ltpacq.<integer>`.
    * Check for leftover `ion.sdrlog` file, typically found in `/tmp`. However, it may be located in another directory if a different `pathName` was specified by the previous run's `ionconfig` file. See the man page entry for `ionconfig` for details.
    * If you know the installed ION will utilize the POSIX Named Semaphore (this is default as of ION 4.1.3), check for any POSIX named semaphore files left in the folder (for Ubuntu) `/dev/shm`. Identify the ION-related semaphores named with this pattern: `sem.ion:GLOBAL:<integer>`. If these files are owned by another user, they should be removed so you can launch a new instance of ION.
        * Typically, with a nominal ION shutdown, these semaphore files are removed automatically. However, due to system crashes or other ungraceful shutdowns, i.e., utilizing `kill -9` or ION's kill script alone, they will be left behind.
        * The location of the semaphore files can change for different distributions. Always verify the directory you are looking at is the correct location.
        * For MacOS, the POSIX-named semaphores are not accessible through the file system, so this step can be skipped.
    * TBD.....Check network connection, firewall setting, run tcp/udp or both with the peer's IP address and port. Update firewall settings as necessary.

### Launch ION

* TBD

### Establish a shell session to interact with an ION instance

1. **Start a shell session as needed**.
2. **Check & set environment variables**:
    * If you are interacting with one of multiple ION instances that are running simultaneously on this host, you need to set the following environment variables:
        * `export ION_NODE_LIST_DIR=$PWD`
        * `export ION_NODE_WDNAME=<path to the ION WD>` this is needed if you are not currently in the working directory of the ION instance with which you plan to interact.
3. **Confirm you are now interacting with the correct ION instance by running**:

    ```sh
    bpadmin
    l induct
    ```

    The output will show the configured induct, which will provide you an indirect confirmation of which ION node you are interacting with.

### Real-time Commanding/Configuration

1. **First establish a shell session with the ION instance using these [steps](#establish-a-shell-session-to-interact-with-an-ion-instance)**.
2. TBD...
3. TBD...

### Shutdown ION

1. **Determine if you are in a multi-ION instance configuration**. First, establish a shell session with the ION instance you want to shut down using these [steps](#establish-a-shell-session-to-interact-with-an-ion-instance).
2. **Launch the shutdown script**:
    * If you know the working directory and a shutdown script is provided, then execute that script.
    * If you don't know the working directory but ION appears to be running normally, try to shut it down by executing the admin program associated with the ION daemons. For example, you could run the following commands:

    ```sh
    bpadmin .
    ltpadmin .
    ionadmin .
    ```

    Additional admin programs will need to run to try to shut down the associated services. To determine exactly which services are running, you need to look at the output of the `ps` command to figure that out.

3. **Use the [check ION instance](#check-for-running-ion-instances) procedure**. If the instance you are trying to shut down is still running, then use the installed `ionstop` stop script from the working directory of the ION instance you want to shut down.
4. **Re-verify ION is no longer running using the [check ION instance](#check-for-running-ion-instances) procedure**.

### Remove ION Installation

1. **Determine if you know the source directory of the existing installation**:
    * If you know this, `cd` to that directory and proceed to next step.
    * If you don't know, use `bpversion` and `ionadmin` to get the bundle protocol version and ION release number. This may help you identify the probable ION source on the host. If you can identify the source code directory used to build the existing ION `cd` to that directory.
      * If you can't find the source code or nothing matches the bundle protocol version and ION release:
          * Use `which ionadmin` to determine the installation location.
          * Delete all ION-related daemons (from bin) and libraries (from lib) manually. Remember, other programs and libraries may also be installed under the same prefix. **Be careful.**
          * Execute `sudo ldconfig` to update the shared library cache or update the LD library path. This completes the removal.
          * Verify by trying to run `ionadmin`.

2. **From the installation source directory**:
    * Run `sudo make uninstall` or `make uninstall`. This should remove all ION build artifacts from the system, including all the binaries and libraries installed in the system.
        * You can also run `make clean` to clear the artifacts of the compilation process from the source directory. However, the `Makefile` still retains the options you set via the `./configure` command. If you want to build with different options, you need to run `./configure` with the new compile options.

### Check for running ION instance(s)

There are multiple ways to check whether one or more ION instances are running.

#### Method 1: Check ION daemons

1. Run `ps -fe` and look for various ION-related daemons. Here is a [list](#ion-related-daemons).

#### Method 2: Check IPC status

1. Run `ipcs` to make sure there are no ION-related shared memory:
    * Shared memory area identified by the KEY used to create them.
    * ION-related shared memory KEY may not be the default value; this requires some prior knowledge.
    * If ION was launched using the default keys, you should see the typical set of default KEY listed [here](#ion-shared-memory-key).

## Case 2: Shared Enviroment

Running ION in a shared environment with multiple users can cause unexpected errors, this section will outline recommendations and best practices.

### ION Installation & Operations

* Installation is the same as Case 1, though it is recommended to keep the source files in a central folder so all approved users can access it, like */opt*.
* Start, stop, and restart is done through the `systemctl` commands
  * Start: `# systemtclt start ion.service`
  * Stop: `# systemtclt stop ion.service`
  * Restart: `# systemtclt restart ion.service`
* Interacting with the service should be kept to a single user at a time
* logrotate should be configured to include the ion.log file, see example below */etc/logrotate.d/ion*

```text
/opt/configs/ion.log
{
    # copytruncate is *required* for correct operation.
    # Without copytruncate, ION may write to archival versions of ion.log.
    copytruncate
    su ioa ioa
    rotate 3
    daily
}
```

## SystemD

Using SystemD is the recommended method of running ION in a shared environment. It has several benefits over using a start script and a shared folder.

* One user runs ION as the semaphore system does not support multiple users using the same ION instance.
* ION can be restarted automatically if the host is rebooted.
* Watch characters are logged into `journalctl` automatically.
* All the ION processes can be identifed with a single command, `systemctl status`.
* Simplifies ION restart, whether to apply config changes or clear out errors.
* Debug messages that normally go to stdout are recorded in `journalctl`.
* `systemctl status` will report the memory used by all the ION processes, cached and RAM combined.
* Help identify any failed ION processes.

### SystemD Service Files

SystemD requires service files in order for programs to be run as part of systemd. Before creating the systemd service file, there are a couple of things that need to be setup.

* Define dedicated user to run ION. For the example's purpose the user and their group will be called ioa.
  * Create group for dedicated user (doing this first allows others to join the group): `# groupadd ioa`.
  * Create user: `# useradd -g ioa ioa`.
* Create ION config files.
  * It is recommended creating the config files with a tool, like the ION config tool, to minimize configuration errors.
  * Should be placed in a central location accessible by the dedicated ION user and other users that would need to to edit the files. Using */opt* is recommended.
  * In the *ionrc* file, the full path to *ionconfig* must be used otherwise parameters set there will not be loaded.
* Create service file for ION, saved as */etc/systemd/system/ion.service*
  * Below is an example ION service file for a node called OPS.
  * Relies on ionscript and ionstart to ensure reliable start and stop of ION while allowing editing of separate config files.
  * Service file outline
    * WorkingDirectory is where the *ion.log* file will be created, so ensure the user has access.
    * For simplicity, it is recommended that the *ion.rc* file be created in the same place as *ion.log*.
    * Create empty *ion.rc* file for `ionscript`.
    * Load separate config files plus contact graph into a single *ion.rc* file using `ionscript`.
    * Start ION using `ionstart` and *ion.rc* file.
    * Stop ION with `killm`.
  * **Note** bpecho lines should be removed if not used

```text
[Unit]
Description=ION daemons
After=network-online.target
Wants=network-online.target
Wants=bpecho.service
Before=bpecho.service

[Service]
Type=forking
User=ioa
Group=ioa
WorkingDirectory=/opt/configs
ExecStartPre=rm -f /opt/configs/ion.rc
ExecStartPre=touch /opt/configs/ion.rc
ExecStartPre=/usr/local/bin/ionscript -O /opt/configs/ion.rc -i /opt/configs/OPS/OPS.ionrc -l /opt/configs/OPS/OPS.ltprc -b /opt/configs/OPS/OPS.bpv7rc -s /opt/configs/OPS/OPS.ionsecrc -i /opt/configs/OPS/purpletb-graph.cg
ExecStart=ionstart -I /opt/configs/ion.rc
ExecStartPost=chmod g+w {{ cfgs }}/ion.log
ExecStop=/bin/bash -l -c /usr/local/bin/killm

[Install]
WantedBy=multi-user.target
```

* (Optional) Create serivce file for desired ION services
  * Below is an example service for the bpecho ION service, running on endpoint ID ipn:35.3
  * Reliant on ION service being started first
  * Uses `kill` to stop service, since Ctrl+C is used normally to stop it

```text
[Unit]
Description=Bundle Echo Service
After=ion.service
BindsTo=ion.service

[Service]
Type=simple
User=ioa
Group=ioa
# JPL bpecho standard is on service number 3
ExecStart=/usr/local/bin/bpecho ipn:35.3
ExecStop=/bin/kill $MAINPID

[Install]
WantedBy=multi-user.target

```

### SystemD Service File Notes

* The `ionscript` and `ionstart` method for systemD service files is the recommended way to start ION. There are other ways to do it such as running the start script, which can be found in many ION examples and tests, but they have been found to not always behave correctly within a systemD context. It has the added benefit of separating operational configs from untested config updates or tests, so config changes are only applied when the `systemctl restart` command is used.
* All service files should be saved in */etc/systemd/system/*.
* Running other ION executables as a service like bpecho is untested, but should behave in a similar way.
* If running another service, make sure it starts after ION is started like the bpecho service does.
* See here for more details on creating systemD service files: [systemd.service](https://www.freedesktop.org/software/systemd/man/latest/systemd.service.html).

### SystemD Service Operational Notes

Operating DTN applications with the systemD service is different than running from a script, so few things need to be kept in mind to ensure proper operations.

- Applications that do not modify ION parameters can be started by any user.
- Changes to ION parameters must be made under the user that started ION, so admin programs can only be run by it.
- If ION parameters are changed by another user, it is simpler to reboot than try resolving the semaphore permission issues.

### Permissions

Starting ION is the easy part, correctly managing permissions can be tricky if not done properly. Improper permisions can cause such things ranging from messages not being logged to executables not executing. Permission management for ION configuration files and related log files needs to be managed appropriately, so these do not happen. Even if not using systemD, this section is still relevant on how to best handle permissions in a shared environment.

#### Configuration Files

As mentioned before, ION config files should be in a central location so the files can be easily accessed by users approved to modified the config files. Placing a folder within `/opt` is generally recognized as the best place. The folder should have the necessary access permissions for both the dedicated user and any users of the authorized group. It is recommended that the folder containing the config files have the ordinal permission of `0774` (user & group: read, write, execute | other: read) and the config files have the ordinal permission of `0664` (user & group: read, write | other: read).

#### Log File(s)

The *ion.log* file's permission needs to be modified in a shared environment, otherwise certain executables will show an error. Setting the permission to `0664` will fix any errors. This will add the group write permission, so approved users can have executable messages logged. It is not recommended in a shared environment, but if ION is running with the SDR in file there is another log file that needs to have its permissions updated, `/tmp/ion.sdrlog`. It also needs to be allowed to have group write permission, ordinal `0664`.

## Appendix

### ION Shared Memory KEY

ION uses shared memory for storing and passing data between ION service
daemons. When ION is launched, several shared memory segments are
created according to the `.ionconfig` file. If no specific key values are
provided by the user, ION will launch using the following default values
as seen when executing the `ipcs` command:

```text
------ Shared Memory Segments --------

key         shmid   owner   perms   bytes       nattch      status
0x0000ee02  0       root    666     641024      8
0x0000ff00  1       root    666     1000000     8
0x14770006  2       root    666     4002544     8
0x0000ff01  3       root    666     50000000    8

------ Semaphore Arrays --------

key         semid   owner   perms   nsems
0x0000ee01  0       root    666     1
0x14770001  1       root    666     250
```

#### Shared Memory Segments

* shmid 0 (0xee02 = 60930) is SM_SEMBASEKEY, this is the shared memory used to track the set of semaphores created and used by ION and its daemons.
* shmid 1 (0xff00 = 65280) is the SDR working memory `sdrwm`.
* shmid 2 (0x14770006) is the SDR heap space 500000 * 8 bytes = 4000000 (the extra space is for the SDR catalog at the beginning), and created by process ID hex 1477 (dec 5239).
* shmid 3 (0xff01 = 65281) is the shared memory space allocated to the ION working memory `ionwm`.

#### SVR4 Semaphores

* semid 0 (0xee01 = 60929) is SM_SEMKEY, the initial semaphore set called `ipcSemaphore` that is used to boostrap ION.
* semid 1 (0x14770001) is a set of 250 additional semaphores created by process ID hex 1477 (dec 5239). Key is generated by the function `getUniqueKey()` whereby 1477 is the process ID with addition of a randomized value in the lower 16 bits.

NOTE: you may notice one or multiple shared memory blocks of size 20,000,000 bytes. These are `sptrace` working memory created by either the `sdrwatch` or the `psmwatch` programs.

#### POSIX Named Semaphores

When ION is running or was improperly shut down due to a crash or `kill -9` command, the POSIX named semaphore files will remain in the file system. For Ubuntu, the location of these files is `/dev/shm`; for other Linux distributions, the locations can vary. The semaphore files created by ION adopts the following name pattern:

```text
sem.ion:GLOBAL:\<integer\>
```

Most operating system restrict semaphore files in such ways that only the owner can remove them, so it is important to make sure that ION is properly shutdown or that these semaphore files are removed after a crash or before launching a new instance of ION.

### ION-related Daemons

In the `killm` script provided by ION, there is a list of all ION-related daemons. While ION is running, the set of daemons running depends on the user's configuration and DTN services activated. The presence of any of these daemons could indicate either a nominal state of operation or an imcomplete shutdown, leaving some processes to run in the background.

To determine which state ION is in, you should check the ion.log. If you know the working directory of the ION instance. Any anomaly will likely be reported to the ion.log.

Another indication of anomaly is by examine whether a critical set of daemons are present - for example, `rfxclock`, `bpclock`, `bptransit`, `bpclm`, `ipnfw`, `ipnadminep` are some of the most critical daemons what should be running even for a barebone ION configuration. If any of them are missing while other ION daemons remains in the system, it should indicate an anomaly.

Here is a list of all possible ION daemons:

* acsadmin lt-acsadmin \\
* acslist lt-acslist \\
* aoslsi lt-aoslsi \\
* aoslso lt-aoslso \\
* bibeadmin lt-bibeadmin \\
* bibeclo lt-bibeclo \\
* bpadmin lt-bpadmin \\
* bpcancel lt-bpcancel \\
* bpchat lt-bpchat \\
* bpclm lt-bpclm \\
* bpclock lt-bpclock \\
* bpcounter lt-bpcounter \\
* bpdriver lt-bpdriver \\
* bpecho lt-bpecho \\
* bping lt-bping \\
* bplist lt-bplist \\
* bpnmtest lt-bpnmtest \\
* bprecvfile lt-bprecvfile \\
* bpsecadmin lt-bpsecadmin \\
* bpsendfile lt-bpsendfile \\
* bpsink lt-bpsink \\
* bpsource lt-bpsource \\
* bpstats lt-bpstats \\
* bpstats2 lt-bpstats2 \\
* bptrace lt-bptrace \\
* bptransit lt-bptransit \\
* brsccla lt-brsccla \\
* brsscla lt-brsscla \\
* bsscounter lt-bsscounter \\
* bssdriver lt-bssdriver \\
* bsspadmin lt-bsspadmin \\
* bsspcli lt-bsspcli \\
* bsspclo lt-bsspclo \\
* bsspclock lt-bsspclock \\
* bssrecv lt-bssrecv \\
* bssStreamingApp lt-bssStreamingApp \\
* cgrfetch lt-cgrfetch \\
* cpsd lt-cpsd \\
* dccpcli lt-dccpcli \\
* dccpclo lt-dccpclo
* dccplso lt-dccplso \\
* dgr2file lt-dgr2file \\
* dgrcli lt-dgrcli \\
* dgrclo lt-dgrclo \\
* dtka lt-dtka \\
* dtkaadmin lt-dtkaadmin \\
* dtn2admin lt-dtn2admin \\
* dtn2adminep lt-dtn2adminep \\
* dtn2fw lt-dtn2fw \\
* dtpcadmin lt-dtpcadmin \\
* dtpcclock lt-dtpcclock \\
* dtpcd lt-dtpcd \\
* dtpcreceive lt-dtpcreceive \\
* dtpcsend lt-dtpcsend \\
* file2dgr lt-file2dgr \\
* file2sdr lt-file2sdr \\
* file2sm lt-file2sm \\
* file2tcp lt-file2tcp \\
* file2udp lt-file2udp \\
* hmackeys lt-hmackeys \\
* imcadmin lt-imcadmin \\
* imcadminep lt-imcadminep \\
* imcfw lt-imcfw \\
* ionadmin lt-ionadmin \\
* ionexit lt-ionexit \\
* ionrestart lt-ionrestart \\
* ionsecadmin lt-ionsecadmin \\
* ionunlock lt-ionunlock \\
* ionwarn lt-ionwarn \\
* ipnadmin lt-ipnadmin \\
* ipnadminep lt-ipnadminep \\
* ipnd lt-ipnd \\
* ipnfw lt-ipnfw \\
* lgagent lt-lgagent \\
* lgsend lt-lgsend \\
* ltpadmin lt-ltpadmin \\
* ltpcli lt-ltpcli \\
* ltpclo lt-ltpclo \\
* ltpclock lt-ltpclock \\
* ltpcounter lt-ltpcounter \\
* ltpdeliv lt-ltpdeliv \\
* ltpdriver lt-ltpdriver \\
* ltpmeter lt-ltpmeter \\
* ltpsecadmin lt-ltpsecadmin \\
* nm_agent lt-nm_agent \\
* nm_mgr lt-nm_mgr \\
* owltsim lt-owltsim \\
* owlttb lt-owlttb \\
* psmshell lt-psmshell \\
* psmwatch lt-psmwatch \\
* ramsgate lt-ramsgate \\
* recvfile lt-recvfile \\
* rfxclock lt-rfxclock \\
* sdatest lt-sdatest \\
* sdr2file lt-sdr2file \\
* sdrmend lt-sdrmend \\
* sdrwatch lt-sdrwatch \\
* sendfile lt-sendfile \\
* sm2file lt-sm2file \\
* smlistsh lt-smlistsh \\
* smrbtsh lt-smrbtsh \\
* stcpcli lt-stcpcli \\
* stcpclo lt-stcpclo \\
* tcaadmin lt-tcaadmin\\
* tcaboot lt-tcaboot\\
* tcacompile lt-tcacompile\\
* tcapublish lt-tcapublish\\
* tcarecv lt-tcarecv\\
* tcc lt-tcc\\
* tccadmin lt-tccadmin \\
* tcp2file lt-tcp2file \\
* tcpbsi lt-tcpbsi \\
* tcpbso lt-tcpbso \\
* tcpcli lt-tcpcli \\
* tcpclo lt-tcpclo \\
* udp2file lt-udp2file \\
* udpbsi lt-udpbsi \\
* udpbso lt-udpbso \\
* udpcli lt-udpcli \\
* udpclo lt-udpclo \\
* udplsi lt-udplsi \\
* udplso lt-udplso \\
* amsbenchr lt-amsbenchr \\
* amsbenchs lt-amsbenchs \\
* amsd lt-amsd \\
* amshello lt-amshello \\
* amslog lt-amslog \\
* amslogprt lt-amslogprt \\
* amsshell lt-amsshell \\
* amsstop lt-amsstop \\
* bputa lt-bputa \\
* cfdpadmin lt-cfdpadmin \\
* cfdpclock lt-cfdpclock \\
* cfdptest lt-cfdptest \\
* bpcp lt-bpcp\\
* bpcpd lt-bpcpd\\

### Notes

[^1]: In this document, we refer to software packages by name given for
       the Ubuntu distribution. Equivalent packages for other Linux
       distributions/OS could be different.

[^2]: At this time, ION's admin program does not provide a command to
       identify its own "node ID." Short of that, the best way to check for
       the identity of a running ION node is to either look at the current
       ion.log for output generated during launch, or by asking bpadmin to
       report its induct information.

[^3]: It is assumed that you know the working directory associated with
       the running ION instance you want to interact with.

[^4]: It is assumed that you know the working directory associated with
       the running ION instance you want to shutdown.
