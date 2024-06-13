# ION Utility Programs

Here is a short list of utility programs that comes with ION that are frequently used by users launch, stop, and query ION/BP operation status:

* bpstats
* bplist
* bpcancel
* ionrestart
* sdrwatch
* psmwatch
* 
### `ionstart`

The `ionstart` script is a bash script that provides a convenient and flexible tool to launch ION in a single command based on a consolidated configuration file with an extension `.rc` or a set of configuration command with various file name extension for different ION administration programs, such as `.ionrc`, `.bprc`, `ipnrc`, `ionsecrc`, `ltprc`, etc.

The `ionstart` typically copied into in `/usr/local/bin` directory by the `make install` command during ION build/installation process.

To launch ION requires a number of daemons be launched with the right configuration and in the appropriate order since each daemon's operation will depend on the preivous daemon's presence. A user can launch ION manually from a shell like this:

```bash
ionadmin	bench.ionrc

ionsecadmin	bench.ionsecrc

ltpadmin	bench.ltprc

bpadmin		bench.bprc

ionadmin	../global.ionrc
```

In general, one launches ionrc files first to establish the working memory and heap space in shared memory space or in file, and establishes a basic set of semaphores for inter-process communications. 

Then one will launch the `.ionsecrc` file to 

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

