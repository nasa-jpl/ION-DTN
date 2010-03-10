#!/bin/bash
# Author: Robert Martin, Ohio University
# Checks sysctl variable vaues on OSX to see if they are large enough
# to handle the shared memory requirements for ION

if [[ $OSTYPE != *darwin* ]]; then
       echo "Not an OSX system, exiting script"
       exit 0
fi

VAR_LIST=('shmmax' 'shmmin' 'shmmni' 'shmseg' 'shmall')
VAR_VALUE=(10485760 1 32 32 4096)
CUR_VALUE=0
I=0
CHANGE=0

echo
echo "If any lines appear below, copy and paste them into /etc/sysctl.conf and reboot."
echo "If /etc/sysctl.conf does not yet exist, create it."

while [[ $I -lt 5 ]]; do
        CUR_VALUE=`sysctl kern.sysv.${VAR_LIST[$I]} | awk '{ print $2 }'`
        if [[ $CUR_VALUE -lt ${VAR_VALUE[$I]} ]]; then
		echo "kern.sysv.${VAR_LIST[$I]}=${VAR_VALUE[I]}"
                let CHANGE=1
        fi
        let I=$I+1
done

if [[ $CHANGE == 0 ]]; then
	echo
	echo "This system's shared memory is ready to run ION."
	echo "No changes are needed."
	exit 0
fi

#If there's a problem, we exit with an error
exit 1
