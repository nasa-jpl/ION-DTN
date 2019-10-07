# ALBP library
The ALBP library born to abstract the 3 most common DTN implementation:
- ION
- IBR-DTN
- DTN2
## Download and installation ALBP library
The first step is to download the ALBP library.
```bash
$ git clone https://gitlab.com/dtnsuite/al_bp
$ cd al_bp
```
Then compile the downloaded library.
- To compile only for DTN2 implementation
    ```bash
    $ make DTN2_DIR=<dtn2_dir>
    ```
- To compile only for ION implementation
    ```bash
    $ make ION_DIR=<ion_dir>
    ```
- To compile only for IBR-DTN implementation
    ```bash
    $ make IBRDTN_DIR=<ibrdtn_dir>
    ```
- To compile for DTN2 and ION implementations
    ```bash
    $ make DTN2_DIR=<dtn2_dir> ION_DIR=<ion_dir>
    ```
- To compile for DTN2 and IBR-DTN implementations
    ```bash
    $ make DTN2_DIR=<dtn2_dir> IBRDTN_DIR=<ibrdtn_dir>
    ```
- To compile for ION and IBR-DTN implementations
    ```bash
    $ make ION_DIR=<ion_dir> IBRDTN_DIR=<ibrdtn_dir>
    ```
- To compile for ION, IBR-DTN and DTN2 implementations
    ```bash
    $ make DTN2_DIR=<dtn2_dir> ION_DIR=<ion_dir> IBRDTN_DIR=<ibrdtn_dir>
    ```
Then install the ALBP library
```bash
$ sudo make install
```
