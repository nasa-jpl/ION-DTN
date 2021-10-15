import os
import subprocess
import time
import sys
from test_cases import testCases
from test_cases import testUtils

'''
    The Johns Hopkins University Applied Physics Laboratory (JHU/APL)

    File Name: __init__.py

    Description: This file performs the setup necessary to run all of 
    the BPSec tests included in this Python test suite. First, any remaining
    ION logs and/or processes are cleaned up from previous runs. Then, the 
    set of three nodes needed for testing (ipn:2.1, ipn:3.1, and ipn:4.1)
    are configured and started. Finally, bpsink is run on node ipn:3.1 and 
    ipn:4.1 with the results being written to 3_results.txt and 4_results.txt.
'''

if (sys.argv[1] == 'help'):
    testCases.help()
    sys.exit()
else:

    ################################# CLEAN ION ###################################
    print("Cleaning old ION\n\n")
    # Removes old ION Logs from each node
    dirs = os.scandir()
    for i in dirs:
        if os.DirEntry.is_dir(i) and "ipn.ltp" in i.name:
            try:
                os.remove(i.name + "/ion.log")
            except FileNotFoundError:
                pass

    subprocess.run("killm", shell=True)
    time.sleep(1)

    ################################ SETUP NODES ##################################
    print("Starting ION...")

    os.environ['ION_NODE_LIST_DIR'] = os.getcwd()

    try:
        os.remove("ion_nodes")
    except FileNotFoundError:
        pass

    dirs = os.scandir()

    for i in dirs:
        if os.DirEntry.is_dir(i) and "ipn.ltp" in i.name:
            os.chdir(i.name)
            env = "export ION_NODE_LIST_DIR=$PWD\n"
            subprocess.call("ionadmin amroc.ionrc", shell=True)
            subprocess.call("ionadmin ../global.ionrc", shell=True)
            subprocess.call("ionsecadmin amroc.ionsecrc", shell=True)
            subprocess.call("ltpadmin amroc.ltprc", shell=True)
            subprocess.call("bpadmin amroc.bprc", shell=True)
            subprocess.call("bpsecadmin amroc.bpsecrc", shell=True)
            os.chdir("..")

    # Setup bpsink on node ipn:3.1. Write to file 3_results.txt
    testUtils.start_bpsink("3")

    # Setup bpsink on node ipn:4.1. Write to file 4_results.txt
    testUtils.start_bpsink("4")
