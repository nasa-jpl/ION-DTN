#!/bin/bash
# Author: Robert Martin, Ohio University
# Checks sysctl variable vaues on OSX to see if they are large enough
# to handle the shared memory requirements for ION

case $OSTYPE in
    *darwin*)
        VAR_LIST=('kern.sysv.shmmax' 'kern.sysv.shmmin' 'kern.sysv.shmmni'
                  'kern.sysv.shmseg' 'kern.sysv.shmall' 'net.inet.udp.maxdgram'
                  'kern.sysv.semmns' 'kern.sysv.semmni')
    ;;
    *freebsd*)
        VAR_LIST=('kern.ipc.shmmax' 'kern.ipc.shmmin' 'kern.ipc.shmmni'
                  'kern.ipc.shmseg' 'kern.ipc.shmall' 'net.inet.udp.maxdgram'
                  'kern.ipc.semmns' 'kern.ipc.semmni')
    ;;
    *)
        echo "No need to update sysctl variables."
        exit 0
    ;;
esac

VAR_VALUE=(10485760 1 32 32 32768 32000 32000 128)
CUR_VALUE=0
I=0
CHANGE=0

while [[ $I -lt ${#VAR_LIST[@]} ]]; do
        CUR_VALUE=`sysctl ${VAR_LIST[$I]} | awk '{ print $2 }'`
        if [[ $CUR_VALUE -lt ${VAR_VALUE[$I]} ]]; then
		echo "${VAR_LIST[$I]}=${VAR_VALUE[I]}"
                let CHANGE=1
        fi
        let I=$I+1
done

if [[ $CHANGE == 0 ]]; then
	echo "This system is ready to run ION."
else
	echo
	echo "Your system's sysctl configuration needs be updated in order to"
	echo "run ION. This is usually done by copying the above assignments into"
	echo "/etc/sysctl.conf or /boot/loader.conf and rebooting."
	exit 1
fi
