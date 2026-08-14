Note: place this in a file named ionstart.sh

````
#!/bin/bash
# shell script to get node running

ionsecadmin     host.ionsecrc
sleep 1
ltpadmin        host.ltprc
sleep 1
bpadmin         host.bprc.1
sleep 1
ipnadmin        host.ipnrc
sleep 2
bpadmin         host.bprc.2
echo "Started Host 1."
````
