# ION Utility Programs

Here is a short list of frequently used utility programs that are distributed with ION.

## bpstats

## bplist

## bpcancel

## ionrestart

## sdrwatch

## psmwatch

## ionstart

The `ionstart` script is typically copied into the `/usr/local/bin` directory using the standard installation procedure and is globally accessible. It provides a convenient and flexible way to launch an ION node using either:

- A consolidated configuration file,
- A set of individual configuration file for each required ION modules, or
- A combination of both

ION needs to be launched through a number of administration programs in the proper order: `.rc`, `ionrc`, `ionsecrc`, `bpsecrc`, `ltpsecrc`, `ltprc`, `bprc`, `ipnrc`, `biberc`, `dtn2rc`, `acsrc`, `imcrc`, `bssprc`, and `cfdprc`. 

To instantiate a bare-bone minimal DTN node, only the `ionrc` and the `bprc` configuration files are needed. The remaining modules are optional, depending on the set of features your DTN node is configured to have. 

The `.rc` file is a consolidated file that includes multiple sections, each corresponding to different modules.

The `ionstart` script automatically parses these files and launches the administrative programs in the right order for you. Here is the usage:

```
IONSTART: Interplanetary Overlay Network startup script
USAGE:
        ionstart [-I config]   [-t tag]   [-a acsrc] [-b bprc]  [-B bssprc]
                 [-d dtn2rc]   [-e biberc][-i ionrc] [-l ltprc] [-m imcrc]
                 [-p ipnrc]    [-s ionsecrc] [-S bpsecrc] [-L ltpsecrc] [-c cfdprc]

        Defined configurations will be run in the following order:
        config, ionrc, ionsecrc, bpsecrc, ltpsecrc ltprc, bprc, ipnrc, biberc, dtn2rc, acsrc, imcrc, bssprc, cfdprc

        -I config       Specifies file containing the configuration for each
                        ion administration program. Each section must be
                        preceded by: ## begin programname tag
                        and proceeded by: ## end programname tag

        -t tag          Optional tag, used to specify which sections are used
                        in config file.  If unspecified, sections with no tag
                        are used.

  -a acsrc        Specifies file acsrc to be used to configure acsadmin.
        -b bprc         Specifies file bprc to be used to configure bpadmin.
        -B bssprc       Specifies file bssprc to be used to configure bsspadmin.
        -d dtn2rc       Specifies file dtn2rc to be used to configure dtn2admin.
        -e biberc       Specifies file biberc to be used to configure bibeadmin.
        -i ionrc        Specifies file ionrc to be used to configure ionadmin.
        -l ltprc        Specifies file ltprc to be used to configure ltpadmin.
        -m imcrc        Specifies file imcrc to be used to configure imcdmin.
        -p ipnrc        Specifies file ipnrc to be used to configure ipnadmin.
        -s ionsecrc     Specifies file ionsecrc to be used to configure ionsecadmin.
        -S bpsecrc      Specifies file bpsecrc to be used to configure bpsecadmin.
        -L ltpsecrc     Specifies file ltpsecrc to be used to configure ltpsecadmin.
        -c cfdprc       Specifies file cfdprc to be used to configure cfdpadmin.
```

Currently, the AMS configuration file `.amsrc` are not supported by `ionstart`.

### Launching combined configuration file

To launch ION using a consolidated configuration file:

`ionstart -I <config-file.rc>`	








* ionstop
* ionscript
* killm

###  `ionexit`

The `ionexit` program shuts down ION with the option to preserve the SDR.

Normally, when ION's various daemons were stopped down by calling `ionstop`,  issuing the command '.' to the administration programs, the SDR will be modified/destroyed in the process. 

Calling `ionexit` with an argument 'keep' allows the SDR state just prior to the execution of `ionexit` to be preserved in the non-volatile storage such as a file if ION was configured to use a file for the SDR.

* runtests
* bptrace
* owltsim
* bpstats2
* cfdptest
* bpcrash

