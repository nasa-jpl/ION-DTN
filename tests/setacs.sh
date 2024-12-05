#!/bin/bash

##########################################################################
# SET ACS
#
# bpversion(1) is used before looking for acsadmin(1). While acsadmin(1)
# should not be in BPV7, build errors can build it in BPV7 or PATH errors
# can provide a path to acsadmin(1) within BPV7 on a host with multiple
# BP versions. Because of this, testing for the absence of acsadmin(1) as
# proof that a script is running in BPV7 can incorrectly use acsadmin(1).
#
# This ACS determination block is intended to be universally applicable
# for all tests BUT IT REQUIRES the comment character "#" be removed
# from or added to the exit line and the echo line that precedes it in
# the if-then-else blocks as appropriate for a specific test.
#
# John Veregge - JPL - Jan 25 2023
if type bpversion &>/dev/null
then
    bpversion=$(bpversion)
    echo "This test appears to be running \"$bpversion\"."

    if [ "$bpversion" = "bpv6" ]
    then
        # Comment out or uncomment the next 2 lines as appropriate.
        #echo "This test does not support $bpversion...skipping test."
        #exit 2

        if type acsadmin &>/dev/null
        then
            ACS="found"
        else
            ACS="notfound"
            echo "acsadmin(1) was not found."

            # Comment out or uncomment the next 2 lines as appropriate.
            #echo "This test requires acsadmin(1) ...skipping test."
            #exit 2
        fi
    else
        ACS="notsupported"

        # Comment out or uncomment the next 2 lines as appropriate.
        #echo "This test does not support $bpversion...skipping test."
        #exit 2
    fi
else
    ACS="undefined"
    echo "ERROR: bpversion not found. Please modify your PATH."
    exit 1
fi
# SET ACS END