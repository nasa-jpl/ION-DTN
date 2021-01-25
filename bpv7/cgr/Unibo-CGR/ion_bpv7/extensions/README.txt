In this directory you find the source code for the RGR and CGRR Extension Blocks.
You have to insert the "rgr" and "cgrr" directories under ./bpv7/library/ext/ where "." is ION's root directory.
You need also the modified "bpextensions.c" and "bp.h" files but you find them under Unibo-CGR/ion_bpv7/aux_files/ directory.

To disable CGRR Ext. Block you have to go into cgrr/cgrr.h header file and set to 0 the CGRREB macro.
To disable RGR Ext. Block you have to go into rgr/rgr.h header file and set to 0 the RGREB macro.

Just a reminder: CGRR Ext. Block is required to enable Moderate Source Routing and RGR Ext. Block
is required to enable anti-loop mechanism of Unibo-CGR.