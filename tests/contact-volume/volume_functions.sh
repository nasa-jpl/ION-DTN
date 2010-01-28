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
    for contact in $(echo -e "l contact" | ionadmin 2>/dev/null | sed -r 's/.*From  ([0-9/:-]*) to.*/\1/')
    do
        echo "d contact $contact 1 1" | ionadmin >/dev/null
    done
    echo "contacts (should be empty):"
    echo "l contact" | ionadmin 2>/dev/null
}

function testvolume () {
    echo "Testing contact volume $1 $2"
    deleteallvolumes 
    if echo "$2" | grep -q ":"; then
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
