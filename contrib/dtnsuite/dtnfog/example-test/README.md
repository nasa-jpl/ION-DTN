# Example Test

This directory contains all file needed to test fog infrastructure.
This test enviroment is in according to the example presented in release guide, please refers to thet for infracstructure configuration and application usege.

## Usage

Once created the network infrastructure and compiled dtnfog on each virtual machines,
move the configuration files in the correct machine (that is indicated in append of the file name, for example dtnfog.config.vm1 will be moved in vm1),
then remove the appendix .vm, 

- dtnfog.conf.vm1 -> dtnfog.conf
- dtnfog.conf.vm2 -> dtnfog.conf
- dtnfog.conf.vm4 -> dtnfog.conf

In directory is also present file test-fog.tar, this is a well formed comunication file ready to be send to infrastructure.
Refer to release guide for usage of test-fog.tar.