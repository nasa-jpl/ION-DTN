# ION Utility Programs

Here is a short list of utility programs that comes with ION that are frequently used by users launch, stop, and query ION/BP operation status:

*  `ionexit` - A program that shuts down ION with the option to preserve the SDR.

Normally, when ION was shut down by calling `ionstop`,  issuing the command '.' to the ionadmin programs, or using the `killm` script, the SDR will be modified/destroyed in the process. Calling `ionexit` with an argument 'keep' allows the SDR state, just prior to the execution of `ionexit` to be preserved in the non-volatile storage such as a file.

* bpstats
* bplist
* bpcancel
* ionrestart
* sdrwatch
* psmwatch
* ionstart
* ionstop
* killm
* runtests
* bptrace
*
