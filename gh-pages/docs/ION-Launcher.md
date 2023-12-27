# ION Launcher

*Last Updated: 12/27/2023*

Previous versions of ION required a good understanding of the different ION adminstrative programs, how to write RC files from them, and what the different configuration commands mean.

The  `ionlauncher` was developed to ease the user into ION configuration by taking a few parameters that mission designer would likely know when planning a network. Using those parameters, captured in a simple JSON format, an entire ION network with defined configurations files can be created and started rather quickly.

## Simple Network Model Syntax

This section will outline the necessary parameters needed to create a simple model for `ionlauncher`.

### Model Parameters

There are seven parameters that are needed to define a simple network model. They are as follows:

```
NAME: serves as the key for the other parameters and naming start scripts
IP ADDRESS: Host IP address or domain name the node will be running on
NODE: assigned node number, will be used for addressing with neighbor(s)
SERVICES: Applications running on the node, currently supports CFDP, AMS, & AMP
DEST: node's neighbor(s)
PROTOCOL: convergance layer to reach a neighbor. Currently supported options include LTP, TCP, UDP, and STCP. 
            (untested options: BSSP & DCCP)
RATE: Data rate used to communicate with neighbor(s), in bytes/s
```

### Example Model

There are a few example models included with the `ionlauncher` prototype under *example_models/*. This section shows one of them and explains how it works.

```json
{
    "SC": {
        "IP": "192.168.1.115",
        "NODE": 21,
        "SERVICES": [],
        "DEST": [
            "Relay"
        ],
        "PROTOCOL": [
            "ltp"
        ],
        "RATE": [
            100000
        ]
    },
    "Relay": {
        "IP": "192.168.1.114",
        "NODE": 22,
        "SERVICES": [],
        "DEST": [
            "SC",
            "GS"
        ],
        "PROTOCOL": [
            "ltp",
            "tcp"
        ],
        "RATE": [
            10000,
            2500000
        ]
    },
    "GS": {
        "IP": "192.168.1.113",
        "NODE": 23,
        "SERVICES": [],
        "DEST": [
            "Relay"
        ],
        "PROTOCOL": [
            "tcp"
        ],
        "RATE": [
            2500000
        ]
    }
}
```

This is an example of a three node setup where *Relay* serves as a DTN relay between *SC* and *GS*. Order is important in the lists for *DEST*, *PROTOCOL*, and *RATE*. They assume each element in the lists correspond to each other. For example, *Relay* communicates with *SC* via LTP at 10,000 bytes/s and *Relay* communicates with *GS* via TCP at 2,500,000 bytes/s.

There are other two examples included with `ionlaucher`. The first is a simple two node setup over TCP. The second is a four node scenario where *SC* can only uplink to *Relay1* and downlink from *Relay2*, while *GS* has continuous coverage of the two relays.

## Prototype - ION 4.1.3

Ionlauncher is currently a prototype and may not be installed globally. Please make sure that both `ionlauncher` and `net_model_gen` in the demo folder has been copied to the execution path of ION, which is typically `/usr/local/bin` or something specified as part of the ./configure script during initial ION installation.

Ionlauncher's simple network model file current does not handle multi-network interface configuration - **this will be updated for ION 4.1.4**.

## Usage

This section will outline how to run ionlauncher and what the different parameters mean. It is assumed `ionlauncher` will be run on each host independently with the same simple model and the only parameter changing is the node name.

`ionlauncher [-h] -n <node name> -m <simple model file> [-d <ionconfig cli directory>]`

    -h: display help
    -n: node name that will be used to start ION on the host
    -m: path to simple model file, ionlauncher assumes the file 
        is in the same directory or a directory below
    -d: optional parameter that defines the path to the ION Config 
        tool CLI scripts, default is /home/$USER/ionconfig-4.8.1/cli/bin

Once the ION configuration files have been generate. ION will be started using the configuration files for the node passed via `-n`.
Stopping the node is done via `ionstop` and if that hangs or errors out, `killm` can be used to force stop ION processes.
To restart ION, `ionlauncher` can be used again, but this will overwrite the configuration files and wipe out any customization that has been added to the initial set of configuration files generated from previous run. If you did not add any customization, it is perfectly find to launch ION again in the same way. If you did make changes, then it is recommended that you use the start script in the node's working directory, `./start_{node_name}.sh` to start ION.

## Directory Structure

The `ionlauncher` and associated `net_model_gen` python scripts will be installed in the same install path for ION, therefore making them available for use from any working directory.

For example, say the 3 node simple file is called `3node.json` and it is stored at directory `$WKDIR`. After cd into the working directory and executing the `ionlauncher`, a new directory `$WKDIR/3node-ion` will be created and it contains the following:

* _ION model_ file in json format. Its name is the simple model filename + `-ion.json`. In this case, it will be called `3node-ion.json`. This file can be opened and edited by the ION Config Tool's browser-based GUI
* _ION network model_ in json format. Its name is the simple model filename + `-net_model.json`. In this case, it will be called `3node-net_model.json`. This file can be opened and edited by the ION Network Model's browser-based GUI.
* For details on how to use these files, please download the _ION Config Tool_ and the _ION Network Model_ from GitHub.com.
* Each node will have its own subfolder: `$WKDIR/3node/SC`, `$WKDIR/3node/Relay`, and `$WKDIR/3node/GS`
* Within each subfolder there will be a set of ION configutration files and a start script called `start_<node name>.sh`, that you can use to launch ION again.

After the initial ionlauncher run, the ION configuration files are generated for you based on the simple network model description and a set of default settings. To activate additional features, optimize parameters settings, and refine protocol behaviors, you will need to edit the ION config files individually. For those changes to take effect, you need to stop ION and restart ION using the start script in each node's working folder.

NOTE: If you run ionlauncher again, the ION configuration files will regenerate and over-write your custom changes. So it is recommended that you make a copy or rename the configuration to avoid this situation.

## Dependency

The `ionlaunch` script requires that the installation of the [ION Config Tool](https://github.com/nasa-jpl/ion-config-tool), which is publically accessible (starting January 2024) from GitHub, and the companion tool called [ION Network Model](https://github.com/nasa-jpl/ion-network-model) is also available for download, although it is not needed for using `ionlauncher`.

Download the latest release of the `ION Config Tool` and note the directory of the CLI (command line interface executables). For example, if it is `/home/$USER/ionconfig-4.8.1/cli/bin`, then you don't need to provide the `-d` option to `ionlauncher`. If it is somewhere else, then you should provide the `-d` option.

You also need to install `node.js` and make sure python version 3.6 or higher is available in your system.
