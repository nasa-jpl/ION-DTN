# A Short List of Useful Utility Programs

* `ionexit` - A program that shuts down ION with the option to preserve the SDR. 

Normally, when ION was shut down by calling `ionstop`,  issuing the command '.' to the ionadmin programs, or using the `killm` script, the SDR will be modified/destroyed in the process. Calling `ionexit` with an argument 'keep' allows the SDR state, just prior to the execution of `ionexit` to be preserved in the non-volatile storage such as a file.

* bplist
* bpcancel
* ionrestart
* etc...
