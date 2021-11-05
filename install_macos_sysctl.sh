#!/bin/sh
#
# Shawn Ostermann - Ohio University
# Version 0.9 - Jan 29, 2021
#
# Starting with Macos Catalina (10.15), MacOS no longer runs the commands in the /etc/sysctl.conf file
# This shell script will create a launchtl plist file that will run the 4 sysctl commands needed for ION every
# time a Mac reboots
#
if [[ -z `uname -a | grep Darwin` ]]; then
    echo "This is only needed on a Mac" 1>&2
    exit 1
fi
#
PLISTNAME="com.ion.launched.ion_shared_memory.plist"
PLISTHOME="/Library/LaunchDaemons"
PLISTFULLDIR="${PLISTHOME}/${PLISTNAME}"
#
# create the plist file in /tmp
#
echo "If this file is run as root, it will actually do the installation."
echo "If that makes you nervous (and it should), just run as yourself and then run the 2 failing lines with sudo"
#
#
echo "========"
echo "Creating Plist file ${PLISTNAME} in /tmp"
#
cat > /tmp/${PLISTNAME} <<EOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple Computer//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
	<key>Label</key>
	<string>com.ion.launched.ion_shared_memory</string>
	<key>ProgramArguments</key>
	<array>
		<string>sh</string>
		<string>-c</string>
		<string>\
/usr/sbin/sysctl kern.sysv.shmseg=32;  \
/usr/sbin/sysctl kern.sysv.shmall=65536; \
/usr/sbin/sysctl net.inet.udp.maxdgram=32000; \
/usr/sbin/sysctl kern.sysv.shmmax=134217728; \
</string>
	</array>
	<key>RunAtLoad</key>
	<true/>
	<key>UserName</key>
	<string>root</string>
</dict>
</plist>
EOF
#
#
#
# install the file
#
echo "========"
echo "Trying to install the file into ${PLISTHOME}"
echo "(this command will fail if not run as root - in which case, just run it yourself with sudo)"
sh -x -c "cp  /tmp/${PLISTNAME} ${PLISTFULLDIR}"
#
# activate the file with launchctl
#
echo "========"
echo "Trying to install the file permanently with Launchctl"
echo "(this command will fail if not run as root - in which case, just run it yourself with sudo)"
sh -x -c "launchctl load -w ${PLISTFULLDIR}"
#
#
echo "========"
echo "Current Settings:"
/usr/sbin/sysctl kern.sysv.shmseg
/usr/sbin/sysctl kern.sysv.shmall
/usr/sbin/sysctl net.inet.udp.maxdgram
/usr/sbin/sysctl kern.sysv.shmmax
#
echo
echo "To change these settings later, you can either"
echo "     1) edit this script and change the values, then run it again, then reboot"
echo "  or 2) edit the file as root: $PLISTFULLDIR   then reboot"
#
exit 0
