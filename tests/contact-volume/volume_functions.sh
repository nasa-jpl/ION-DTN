#!/bin/bash

function applyrelativevolume () {
    echo -e "a contact +1 +$1 1 1 $2\nq" | ionadmin >/dev/null
    echo "contacts:"
    echo "l contact" | ionadmin 
    echo "ranges:"
    echo "l range" | ionadmin
}

function applyabsolutevolume () {
    echo -e "a contact +1 $1 1 1 $2\nq" | ionadmin >/dev/null
    echo "contacts:"
    echo "l contact" | ionadmin 
    echo "ranges:"
    echo "l range" | ionadmin
}

function deleteallvolumes () {
# We don't know the time that "+1" evaluated to when we added it,
# so remove all the contacts.
#    for contact in $(echo -e "l contact" | ionadmin 2>/dev/null | sed -e 's/.*From[ \t]*\([0-9/:-]*\)[ \t]*to.*xmit rate from node[ \t]*\([0-9]*\)[ \t]*to node[ \t]*\([0-9]*\).*/\1_\2_\3/')
#    do
#        if ! echo $contact | grep -e '[0-9]' > /dev/null; then
#            # Didn't match anything, not reading useful ionadmin output.
#            continue
#        fi
#        contactsplit=$(echo $contact | sed -e 's/\(.*\)_\(.*\)_\(.*\)/\1 \2 \3/')
#        echo "d contact $contactsplit"
#        echo "d contact $contactsplit" | ionadmin >/dev/null
#    done
    echo "d contact * 1 1" | ionadmin > /dev/null
    echo "d contact * 1 2" | ionadmin > /dev/null
    echo "d contact * 2 1" | ionadmin > /dev/null
    echo "contacts (should be empty):"
    echo "l contact" | ionadmin
}

function testvolume () {
    echo "Testing contact volume $1 $2"
    deleteallvolumes 
    if echo "$1" | grep ":" >/dev/null; then
        applyabsolutevolume $1 $2
    else
        applyrelativevolume $1 $2
    fi
    echo -e "$IONMESSAGE $1 $2\n!" | bpsource ipn:1.1 &
    BPSOURCEPID=$!

    # sleep and kill process in case it didn't end properly
    sleep 3
    echo "Killing bpsource if it is still running..."
    kill -9 $BPSOURCEPID >/dev/null 2>&1
}
