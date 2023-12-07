# ION Launcher
Previous versions of ION required a good understanding of the different ION adminstrative programs, how to write RC files from them, and what the different configuration commands mean.
`ionlauncher.sh` was developed to ease the user into ION configuration by taking a few parameters that mission designer would likely know when planning a network.
Using those parameters, in a simple JSON model, an entire ION network with defined configurations files can be created and started rather quickly.

## Simple Model Syntax
This section will outline the necessary parameters needed to create a simple model for `ionlauncher.sh`.

### Model Parameters
There are seven parameters that are needed to define a simple model. They are as follows:

    - Node Name: serves as the key for the other parameters and naming start scripts
    - IP: Host IP address or domain name the node will be running on
    - NODE: assigned node number, will be used for addressing with neighbor(s)
    - SERVICES: Applications running on the node, currently supports CFDP, AMS, & AMP
    - DEST: node's neighbor(s)
    - PROTOCOL: convergance layer to reach neighbor(s)
      - supported: LTP, TCP, UDP, & STCP
      - untested: BSSP & DCCP untested
    - RATE: Data rate used to communicate with neighbor(s), in bytes/s

### Example Model
There are a few example models included with `ionlauncher.sh` under *example_models/*. This section shows one of them and explains how it works.
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
This is an example of a three node setup where Relay serves as a relay between SC and GS. Order is important in the lists for DEST, PROTOCOL, and RATE. They assume each element in the lists correspond to each other. For example, Relay communicates with SC via LTP at 10,000 bytes/s and Relay communicates with GS via TCP at 2,500,000 bytes/s.

There are other two examples included with `ionlaucher.sh`. The first is a simple two node setup over TCP. The second is a four node scenario where SC can only uplink to Relay1 and downlink from Relay2, while GS has continuous coverage of the two relays.

## Prototype
Ionlauncher is currently a prototype and not install globally. To install it, please copy `ionlauncher` and `net_model_gen` to `/usr/local/bin` or where ION was installed.

Ionlauncher's simple model json file current does not handle multi-network interface configuration - this will be updated in future release.

## Usage
This section will outline how to run ionlauncher and what the different parameters mean. It is assumed `ionlauncher.sh` will be run on each host independently with the same simple model and the only parameter changing is the node name.

`ionlauncher [-h] -n <node name> -m <simple model file> [-d <ionconfig cli directory>]`

    -h: display help
    -n: node name that will be used to start ION on the host
    -m: path to simple model file, ionlauncher assumes the file is in the same directory or a directory below
    -d: optional parameter that defines the path to the ION Config tool CLI scripts, default is /home/$USER/ionconfig-4.8.1/cli/bin

Once the ION configuration files have been generate. ION will be started using the configuration files for the node passed via `-n`.
Stopping the node is done via `ionstop` and if that hangs or errors out, `killm` can be used to force stop ION processes.
To restart ION, `ionlauncher.sh` can be used again, but this will overwrite the configuration files. To bypass this behavior, it is recommended to use the start script in the node's directory, `./start_{node_name}.sh`.

## Directory Structure
The ionlauncher and associated python scripts will be installed in the same install path for ION, therefore making them available for use from any working directory.

For example, say the 3 node simple file is called `3node.json` and it is stored at directory `$WKDIR`. After cd into the working directory and executing the ionlauncher, then you a new directory will be created:

* `$WKDIR/3node-ion` - this folder will contain the following:
    * _ION model_ file in json format. Its name is the simple model filename + `-ion.json`. In this case, it will be called `3node-ion.json`. This file can be opened and edited by the ION Config Tool's browser-based GUI
    * _ION network model_ in json format. Its name is the simple model filename + `-net_model.json`. In this case, it will be called `3node-net_model.json`. This file can be opened and edited by the ION Network Model's browser-based GUI.
    * For details on how to use these files, please download the _ION Config Tool_ and the _ION Network Model_ from GitHub.com. 
* Each node will have its own subfolder: `$WKDIR/3node/SC`, `$WKDIR/3node/Relay`, and `$WKDIR/3node/GS` 
* Within each subfolder there will be a set of ION configutration files and a start script called `start_<node name>.sh`, that you can use to launch ION again.

After the initial ionlauncher run, the ION configuration files are generated for you based on the simple model file's description and a set of default settings. To activate additional features, optimize parameters settings, and refine protocol behavior, you will need to edit the ION config files individually. For those changes to take effect, you need to stop ION and restart ION using the start script.

If you run ionlauncher again, the ION configuration files will regenerate and replaces your custom changes.

## Dependency
The ionlaunch script requires that the installation of the [ION Config Tool](https://github.com/nasa-jpl/ion-config-tool), which is publically accessible (starting January 2024) from GitHub. A companion tool called [ION Network Model](https://github.com/nasa-jpl/ion-network-model) is also available for download although it is not needed for using ionlauncher.

Download the latest release of the `ION Config Tool` and note the directory of the CLI (command line interface executables). For example, if it is `/home/$USER/ionconfig-4.8.1/cli/bin`, then you don't need to provide the `-d` option to ionlauncher. If it is somewhere else, then you should provide the `-d` option.

You also need to install `node.js` and make sure python version 3.6 or higher is available in your system.
