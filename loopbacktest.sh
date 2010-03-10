#!/bin/sh

#loopbacktest.sh
# David Young
# July 10, 2008

# script used in "make test" to ensure ion is functioning properly.
# script is expected to be run by automake, meaning:
# 1: Environment variable "srcdir" exists and points to the root directory of
# the ion source code.
# 2: The current working directory contains the built programs of ion.

# message sent over ion
IONMESSAGE="iontestmessage"
IONSENDFILE=./ionsendfile.txt
IONRECEIVEFILE=./ionreceivefile.txt
# adding ./ to path is VERY BAD. Do as I say, not as I do.
PRELOOPBACKPATH=${PATH}
PATH=".:${PATH}"
export PATH
CONFIGDIR="${srcdir}/configs/loopback-udp"

echo "Use config files in ${CONFIGDIR}"
echo "Added . to PATH. This is a very bad thing to do."
echo "PATH = ${PATH}"
echo

echo "Starting ION..."
echo "${srcdir}/ionstart -i ${CONFIGDIR}/loopback-udp.ionrc -b ${CONFIGDIR}/loopback-udp.bprc -p ${CONFIGDIR}/loopback-udp.ipnrc"
"${srcdir}/ionstart" -i "${CONFIGDIR}/loopback-udp.ionrc" -b "${CONFIGDIR}/loopback-udp.bprc" -p "${CONFIGDIR}/loopback-udp.ipnrc"

# create the test message in a sent file
echo "${IONMESSAGE}" > "${IONSENDFILE}"
# the exclamation point signals the bundle sender to quit
echo "!" >> "${IONSENDFILE}"


# receive the message and store it in a file via test bundle sink
echo
echo "Starting Message Listener..."
echo "bpsink ipn:1.1 > ${IONRECEIVEFILE} &"
bpsink ipn:1.1 > "${IONRECEIVEFILE}" &
BPSINKPID=${!}

# send the message in the file via test bundle source
sleep 5
echo
echo "Sending message..."
echo "bpsource ipn:1.1 < ${IONSENDFILE} &"
bpsource ipn:1.1 < "${IONSENDFILE}" &
BPSOURCEPID=${!}
# sleep and kill process in case it didn't end properly
sleep 5
echo
echo "Killing bpsource if it is still running..."
kill -9 ${BPSOURCEPID} >/dev/null 2>&1

# bpsink does not self-terminate, so send it SIGINT
sleep 5
echo
echo "Sending bpsink SIGINT..."
kill -2 ${BPSINKPID} >/dev/null 2>&1
# in case bpsink is unresponsive, send SIGKILL
sleep 5
echo "Killing bpsink..."
kill -9 ${BPSINKPID} >/dev/null 2>&1
echo
echo "Contents sent from ${IONSENDFILE}:"
cat "${IONSENDFILE}"
echo
echo "Contents received to ${IONRECEIVEFILE}:"
cat "${IONRECEIVEFILE}"

# compare the sent message to the received one
echo
echo "Comparing ${IONSENDFILE} to ${IONRECEIVEFILE}..."
grep "${IONMESSAGE}" "${IONRECEIVEFILE}"  >/dev/null 2>/dev/null
RETVAL=${?}

if test "${RETVAL}" -ne 0 ; then
	echo "Files do not match."
else
	echo "Files match."
fi

# shut down ion processes
echo
echo "Stopping ion..."
echo "${srcdir}/ionstop"
"${srcdir}/ionstop"

# clean up
echo
echo "Cleaning up..."
echo
rm "${IONSENDFILE}" "${IONRECEIVEFILE}"
echo "Done"
echo "Any error messages may be found in ion.log"

PATH="${PRELOOPBACKPATH}"
export PATH
echo
echo "Removed . from PATH."
echo "PATH = ${PATH}"
echo

# return the result of the match
if test "${RETVAL}" -ne 0 ; then
	echo "Returning '${RETVAL}' for failure."
else
	echo "Returning '${RETVAL}' for success."
fi

exit ${RETVAL}
