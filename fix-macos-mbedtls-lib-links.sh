#!/usr/bin/env bash

# Usage: ./fix_mbedtls_links_all.sh /new/library/path

if [ "$#" -ne 3 ]; then
    echo "Usage: $0 /new/mbedtls-library/path /install/folder /ion/source/folder"
    exit 1
fi

NEW_LIB_PATH="$1"
INSTALL_FOLDER="$2"
ION_FOLDER="$3"

# List of libraries to check and update
LIBRARIES=("libmbedtls.dylib" "libmbedx509.dylib" "libmbedcrypto.dylib")

# From Killm script
KILLPROCESSLIST=(
    acsadmin lt-acsadmin
    acslist lt-acslist
    amsbenchr lt-amsbenchr
    amsbenchs lt-amsbenchs
    amsd lt-amsd
    amshello lt-amshello
    amslog lt-amslog
    amslogprt lt-amslogprt
    amsmib lt-amsmib
    amsshell lt-amsshell
    amsstop lt-amsstop
    aoslsi lt-aoslsi
    aoslso lt-aoslso
    bibeadmin lt-bibeadmin
    bibeclo lt-bibeclo
    bpadmin lt-bpadmin
    bpcancel lt-bpcancel
    bpchat lt-bpchat
    bpclm lt-bpclm
    bpclock lt-bpclock
    bpcounter lt-bpcounter
    bpcp lt-bpcp
    bpcpd lt-bpcpd
    bpdriver lt-bpdriver
    bpecho lt-bpecho
    bping lt-bping
    bplist lt-bplist
    bpnmtest lt-bpnmtest
    bprecvfile lt-bprecvfile
    bprecvtest lt-bprecvtest
    bpsecadmin lt-bpsecadmin
    bpsendfile lt-bpsendfile
    bpsendtest lt-bpsendtest
    bpsink lt-bpsink
    bpsource lt-bpsource
    bpstats lt-bpstats
    bpstats2 lt-bpstats2
    bptrace lt-bptrace
    bptransit lt-bptransit
    bputa lt-bputa
    brsccla lt-brsccla
    brsscla lt-brsscla
    bsscounter lt-bsscounter
    bssdriver lt-bssdriver
    bsspadmin lt-bsspadmin
    bsspcli lt-bsspcli
    bsspclo lt-bsspclo
    bsspclock lt-bsspclock
    bssrecv lt-bssrecv
    bssStreamingApp lt-bssStreamingApp
    cfdpadmin lt-cfdpadmin
    cfdpclock lt-cfdpclock
    cfdptest lt-cfdptest
    cgrfetch lt-cgrfetch
    cpsd lt-cpsd
    dccpcli lt-dccpcli
    dccpclo lt-dccpclo
    dccplsi lt-dccplsi
    dccplso lt-dccplso
    dgr2file lt-dgr2file
    dgrcli lt-dgrcli
    dgrclo lt-dgrclo
    dtka lt-dtka
    dtkaadmin lt-dtkaadmin
    dtn2admin lt-dtn2admin
    dtn2adminep lt-dtn2adminep
    dtn2fw lt-dtn2fw
    dtpcadmin lt-dtpcadmin
    dtpcclock lt-dtpcclock
    dtpcd lt-dtpcd
    dtpcreceive lt-dtpcreceive
    dtpcsend lt-dtpcsend
    file2dgr lt-file2dgr
    file2sdr lt-file2sdr
    file2sm lt-file2sm
    file2tcp lt-file2tcp
    file2udp lt-file2udp
    hmackeys lt-hmackeys
    imcadmin lt-imcadmin
    imcadminep lt-imcadminep
    imcfw lt-imcfw
    ionadmin lt-ionadmin
    ionexit lt-ionexit
    ionrestart lt-ionrestart
    ionsecadmin lt-ionsecadmin
    ionunlock lt-ionunlock
    ionwarn lt-ionwarn
    ipnadmin lt-ipnadmin
    ipnadminep lt-ipnadminep
    ipnd lt-ipnd
    ipnfw lt-ipnfw
    lgagent lt-lgagent
    lgsend lt-lgsend
    ltpadmin lt-ltpadmin
    ltpcli lt-ltpcli
    ltpclo lt-ltpclo
    ltpclock lt-ltpclock
    ltpcounter lt-ltpcounter
    ltpdeliv lt-ltpdeliv
    ltpdriver lt-ltpdriver
    ltpmeter lt-ltpmeter
    ltpsecadmin lt-ltpsecadmin
    nm_agent lt-nm_agent
    nm_mgr lt-nm_mgr
    owltsim lt-owltsim
    owlttb lt-owlttb
    psmshell lt-psmshell
    psmwatch lt-psmwatch
    ramsgate lt-ramsgate
    recvfile lt-recvfile
    rfxclock lt-rfxclock
    sdatest lt-sdatest
    sdr2file lt-sdr2file
    sdrmend lt-sdrmend
    sdrwatch lt-sdrwatch
    sendfile lt-sendfile
    sm2file lt-sm2file
    smlistsh lt-smlistsh
    smrbtsh lt-smrbtsh
    stcpcli lt-stcpcli
    stcpclo lt-stcpclo
    tcaadmin lt-tcaadmin
    tcaboot lt-tcaboot
    tcacompile lt-tcacompile
    tcapublish lt-tcapublish
    tcarecv lt-tcarecv
    tcc lt-tcc
    tccadmin lt-tccadmin
    tcp2file lt-tcp2file
    tcpbsi lt-tcpbsi
    tcpbso lt-tcpbso
    tcpcli lt-tcpcli
    tcpclo lt-tcpclo
    tcputa lt-tcputa
    udp2file lt-udp2file
    udpbsi lt-udpbsi
    udpbso lt-udpbso
    udpcli lt-udpcli
    udpclo lt-udpclo
    udplsi lt-udplsi
    udplso lt-udplso
    bpversion lt-bpversion
)

# From Makefile
check_PROGRAMS=(
    tests/1000.loopback/.libs/dotest
	tests/1500.loopback-brs/.libs/dotest
	tests/issue-260-teach-valgrind-mtake/.libs/domtake
	tests/issue-188-common-cos-syntax/.libs/dotest
	tests/bug-0015-tcpclo-bpcp-sig-handling/.libs/test
	tests/issue-330-cfdpclock-FDU-removal/.libs/cfdplisten
	tests/issue-334-cfdp-transaction-id/.libs/dotest
	tests/nm-unit/.libs/dotest
	tests/nm-unit/utils/vector/.libs/dotest
	tests/nm-unit/utils/rhht/.libs/dotest
	tests/nm-unit/utils/radix_pt/.libs/dotest
	tests/nm-unit/utils/radix_ut/.libs/dotest
	tests/sm_subsystem/.libs/dotest
) 


# Iterate through KILLPROCESSLIST
echo "********** Iterate through installed ION binaries **********"
for BINARY in "${KILLPROCESSLIST[@]}"; do
    BINARY_PATH="$INSTALL_FOLDER/$BINARY"
    if [ ! -f "$BINARY_PATH" ]; then
        echo "Skipping: Binary '$BINARY_PATH' not found."
        continue
    fi

    echo "Checking binary: $BINARY_PATH"

    # Iterate through each library and check its linkage
    for LIB in "${LIBRARIES[@]}"; do
        echo "  Checking linkage for $LIB..."

        CURRENT_PATH=$(otool -L "$BINARY_PATH" | grep "$LIB" | awk '{print $1}')

        if [ -z "$CURRENT_PATH" ]; then
            echo "  Warning: $LIB is not linked in $BINARY"
            continue
        fi

        NEW_PATH="$NEW_LIB_PATH/$LIB"

        if [ "$CURRENT_PATH" != "$NEW_PATH" ]; then
            echo "  Updating $LIB from $CURRENT_PATH to $NEW_PATH..."
            install_name_tool -change "$CURRENT_PATH" "$NEW_PATH" "$BINARY_PATH"

            if [ $? -eq 0 ]; then
                echo "  Successfully updated $LIB."
            else
                echo "  Error updating $LIB."
            fi
        else
            echo "  $LIB is already correctly linked."
        fi
    done
done

# Iterate through check_PROGRAMS
echo "********** Iterate through regression test binaries **********"
for BINARY in "${check_PROGRAMS[@]}"; do
    BINARY_PATH="$ION_FOLDER/$BINARY"
    if [ ! -f "$BINARY_PATH" ]; then
        echo "Skipping: Binary '$BINARY_PATH' not found."
        continue
    fi

    echo "Checking binary: $BINARY_PATH"

    for LIB in "${LIBRARIES[@]}"; do
        echo "  Checking linkage for $LIB..."

        CURRENT_PATH=$(otool -L "$BINARY_PATH" | grep "$LIB" | awk '{print $1}')

        if [ -z "$CURRENT_PATH" ]; then
            echo "  Warning: $LIB is not linked in $BINARY"
            continue
        fi

        NEW_PATH="$NEW_LIB_PATH/$LIB"

        if [ "$CURRENT_PATH" != "$NEW_PATH" ]; then
            echo "  Updating $LIB from $CURRENT_PATH to $NEW_PATH..."
            install_name_tool -change "$CURRENT_PATH" "$NEW_PATH" "$BINARY_PATH"

            if [ $? -eq 0 ]; then
                echo "  Successfully updated $LIB."
            else
                echo "  Error updating $LIB."
            fi
        else
            echo "  $LIB is already correctly linked."
        fi
    done
done

echo "Finished checking and updating library paths."
