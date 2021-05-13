NOTE: Originally this folder contained some ION threads modified to make Unibo-CGR work.
      Now Unibo-CGR is fully integrated into ION bpv7 and it is no longer necessary to modify external files.

Note on configure:
Launch the configure script with the following sintax (from ION's root directory):  
Only Unibo-CGR: ./configure --enable-unibo-cgr
Unibo-CGR + RGR Ext. Block: ./configure --enable-unibo-cgr CPPFLAGS='-DRGR=1 -DRGREB=1'
Unibo-CGR + CGRR Ext. Block: ./configure --enable-unibo-cgr CPPFLAGS='-DCGRR=1 -DCGRREB=1'
Unibo-CGR + RGR and CGRR Ext. Block: ./configure --enable-unibo-cgr CPPFLAGS='-DRGR=1 -DCGRR=1 -DRGREB=1 -DCGRREB=1'
Unibo-CGR disabled: ./configure