Note: place this in a file named ionstop.sh

````
#!/bin/bash
#
# ionstop
# David Young
# Aug 20, 2008
#
# will quickly and completely stop an ion node.

ION_OPEN_SOURCE=1

echo "IONSTOP will now stop ion and clean up the node for you..."
bpversion
if [ $? -eq 6 ]; then
echo "acsadmin ."
acsadmin .
sleep 1
fi
echo "bpadmin ."
bpadmin .
sleep 1
if [ "$ION_OPEN_SOURCE" == "1" ]; then
echo "cfdpadmin ."
cfdpadmin .
sleep 1
fi
echo "ltpadmin ."
ltpadmin .
sleep 1
echo "ionadmin ."
ionadmin .
sleep 1
echo "killm"
killm
echo "ION node ended. Log file: ion.log"
````
