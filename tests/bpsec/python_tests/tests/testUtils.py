import os
import subprocess
import time
import json
import random
import string

'''
    The Johns Hopkins University Applied Physics Laboratory (JHU/APL)

    File Name: testUtils.py

    Description: This file contains utility functions for the BPSec Python
    test suite. These functions can create an ionsecadmin or bpsecadmin command
    and execute them, as well as perform the log checking necessary to determine
    if a test has passed.
'''

################################ Global Data  #################################

# Track the total number of tests passed and failed
global g_tests_passed
global g_tests_failed

# Verbose testing displays all security policy information known at each
# node configuration step
global g_verbose

# Debug mode to display extra information to help a test user find errors
global g_debug

# Use os.devnull for verbose logging. Set this variable to the preferred
# location for stdout
verbose = subprocess.DEVNULL

# Default security context parameters for bib-integrity and bcb-confidentiality
bibScParms = [["key_name", "key_1_32bytes"]]
bcbScParms = [["key_name", "bcb_key_32bytes"]]

# Default security context parameters for bib-integrity services targeting the
# Primary Block, for which scope must be set to 4: include security block header
bibPrimaryScParms = [["key_name", "key_1_32bytes"], ["scope_flags", "4"]]

# Security context parameters to be used when a bcb-confidentiality and
# bib-integrity operation may share a security target: the Primary Block
bcbPrimaryScParms = [["key_name", "bcb_key_32bytes"], ["aad_scope", "4"]]

# Time macros, in seconds, to allow ION utilities time to finish their actions
TIME_TESTTIMEOUT = 10 # Wait before an attempted subprocess call should timeout
TIME_IONSTOP = 10     # Wait for the ionstop script to finish
TIME_CLEANUP = 1      # Wait for test cleanup to complete
TIME_DISPLAYTEXT = 2  # Show test text to the user's screen
TIME_TESTFINISH = 5   # Test wrap-up, typically a delay needed for bptrace

# Status macros for test events
STATUS_SUCCESS = 1   # Test event found if expected, or not found if not expected
STATUS_MISSING = 2   # Test event not found but expected - failure indicator
STATUS_UNEXPECT = 3  # Test event found but not expected - failure indicator
STATUS_UNCHECKED = 4 # Test event has not yet been verified by the test suite

# Security context ID macros
global BIB_SCID
global BCB_SCID
BIB_HMAC_SHA2_SCID = 1
BCB_AES_GCM_SCID = 2
ION_TEST_SCID = 0

################################## UTILITIES ##################################

def print_debug(output):

    if g_debug:
        print("\tSecurity policy command:")

        cmd = output.args.splitlines()
        
        for line in cmd:
            line.strip()
            if line:
                if ((line[0] != ':') and ("END" not in line)):
                    print("\t\t" + line)

        if output.returncode != None:
            print("\tReturn code: ", output.returncode)

        if output.stdout != None:
            print("\tstdout:")
            
            out = output.stdout.splitlines()
            for line in out:
                print("\t\t", line)

        if output.stderr != None:
            print("\tstderr:")

            err = output.stdout.splitlines()
            for line in err:
                print("\t\t", line)
        print()
        

def print_ion_text(ion_text):
    """
    [print_ion_text] displays raw ION output [ion_text] as formatted, line-by-line text.

    """
    for line in ion_text:
            line.strip()

            # Handle ionsecadmin output
            key_idx = line.find("key name")
            if(key_idx != -1):
                print("\t\t" + line[key_idx:])

            # Print lines that do not begin with ':' from bpsecadmin
            elif (line and (line[0] != ':')):
                print("\t\t" + line)

    print()

################################ ION COMMANDS #################################

def get_version(node_num):
    """[get_version] returns the version of ION running at [node_num], either:
        'ION-NASA-BASELINE' for the ION NASA Baseline (INB)
        'ION-OPEN-SOURCE' for ION Open Source (IOS)
    
    Precondition: node_num identifies an ION node that has been initialized.

    Assumption: node_num is in the bundle path for the test checking 
            the ION version.

    ION utility used: bpsecadmin
    """
    os.chdir(str(node_num) + ".ipn.ltp")

    vers = subprocess.run("bpsecadmin<<END\nv\nq\nEND", shell=True, capture_output=True, timeout=TIME_TESTTIMEOUT, text=True)

    print_debug(vers)

    os.chdir("..")

    # Some ION version tags truncate INB to "BASELI"
    if "ION-NASA-BASELI" in str(vers.stdout):
        return "ION-NASA-BASELINE"
    elif "ION-OPEN-SOURCE" in str(vers.stdout):
        return "ION-OPEN-SOURCE"
    else:
        return "ERROR"
    
    

def add_key(node_num, k_name):
    """[add_key] adds a key to a node with a name [k_name] to the node at
    [node_num].

    Precondition: k_name is a string that is the name of a key.

    Assumption: k_name is the name of the .hmk key file to use to create the key.

    ION utility used: ionsecadmin
    """

    os.chdir(str(node_num) + ".ipn.ltp")

    cmd = "ionsecadmin<<END\na key " + k_name + " " + k_name + ".hmk\nq\nEND"

    out = subprocess.run(cmd,shell=True, timeout=TIME_TESTTIMEOUT, capture_output=True, text=True)

    print_debug(out)

    os.chdir("..")


def del_key(node_num, k_name):
    """[del_key] deletes a key with a name [k_name] from the node at
    [node_num].

    Precondition: k_name is a string that is the name of a key.

    Assumption: k_name is the name of the .hmk key file that has been added
    using the ionadmin utility.

    ION utility used: ionsecadmin
    """
    os.chdir(str(node_num) + ".ipn.ltp")

    cmd = "ionsecadmin<<END\nd key " + k_name + "\nq\nEND"

    out = subprocess.run(cmd, shell=True, timeout=TIME_TESTTIMEOUT, capture_output=True, text=True)

    print_debug(out)

    os.chdir("..")


def list_key(node_num):
    """[list_key] lists all keys known at node [node_num].

    ION utility used: ionsecadmin
    """
    os.chdir(str(node_num) + ".ipn.ltp")

    cmd = "ionsecadmin<<END\nl key\nq\nEND"

    out = subprocess.run(cmd, shell=True, timeout=TIME_TESTTIMEOUT, capture_output=True, text=True)

    print_debug(out)

    key_list = out.stdout.splitlines()

    # Display the keys defined at the node
    print("\tKeys:")
    print_ion_text(key_list)

    os.chdir("..")


def add_event_set(node_num, es_name, es_des=""):
    """
    [add_event_set] adds an named [es_name] event set to a node with
    an optional description [es_des].

    Precondition: es_name is a string that is the name of the event set to add
    Precondition: es_des is a string that is the description of the event set.
                  The description for a named event set is optional.

    ION utility used: bpsecadmin
    """
    os.chdir(str(node_num) + ".ipn.ltp")

    json_obj = {"event_set": {"name": es_name, "desc": es_des}}

    # Serialize json_obj to a json formatted string
    cmd = json.dumps(json_obj)

    out = subprocess.run("bpsecadmin<<END\n a " + cmd + "\nq\nEND", shell=True, timeout=TIME_TESTTIMEOUT, capture_output=True, text=True)

    print_debug(out)

    os.chdir("..")


def del_event_set(node_num, es_name):
    """"
    [del_event_set] deletes an event set with a name [es_name] from node
    [node_num].

    Precondition: es_name is a string that is the name for an existing event set
    """

    os.chdir(str(node_num) + ".ipn.ltp")

    # Delete event set by name
    cmd = "bpsecadmin<<END\nd {\"event_set\" : {\"name\" : \"" + es_name + "\"}}\nq\nEND"

    out = subprocess.run(cmd, shell=True, timeout=TIME_TESTTIMEOUT, capture_output=True, text=True)

    print_debug(out)

    os.chdir("..")


def list_event_set(node_num):
    """
    [list_event_set] lists all of the event sets defined at [node_num].

    ION Utility Used: bpsecadmin
    """
    os.chdir(str(node_num) + ".ipn.ltp")

    cmd = "bpsecadmin<<END\nl {\"event_set\"}\nEND"

    result = subprocess.run(cmd, shell=True, timeout=TIME_TESTTIMEOUT, capture_output=True, text=True)

    print_debug(result)

    es_list = result.stdout.splitlines()

    # Display the event sets defined at the node
    print("\tEvent Sets:")
    print_ion_text(es_list)

    os.chdir("..")


def info_event_set(node_num, es_name):
    """
    [info_event_set] Provides the results of the 'info' event set command run at [node_num] 
    for the named event set (identified by [es_name]) provided. 

    ION Utility Used: bpsecadmin
    """
    os.chdir(str(node_num) + ".ipn.ltp")

    cmd = "bpsecadmin<<END\ni {\"event_set\" : {\"name\" : \"" + str(es_name) + "\" }}\nEND"

    result = subprocess.run(cmd, shell=True, timeout=TIME_TESTTIMEOUT, capture_output=True, text=True)
    
    print_debug(result)

    es_info = result.stdout.splitlines()

    # Display the event sets found at the node
    print("\tEvent Sets:")
    print_ion_text(es_info)

    os.chdir("..")



def build_processing_action(action):
    """
    [build_processing_action] creates the command necessary to add an optional
    processing [action] to a named event set.

    Precondition: [action] is an ION-supported processing action

    Note: This function currently supports standalone processing actions as well
          as processing actions associated with a single parameter. To associate a
          parameter with the action, specify the following:
          action[0] - Processing action
          action[1] - Processing action parameter label (optional)
          action[2] - Processing action parameter value (optional)

    Returned Command Format:
        {"id": “<processing action>”, “<(opt.) action parm label>” : <(opt.) parm value>}
    """

    # If action does not have associated parameters
    if len(action) == 1:
        cmd = {"id": action[0]}
    # If processing action is associated with a single parameter
    elif len(action) == 3:
        cmd = {"id": action[0], action[1]: action[2]}
    else:
        raise AssertionError("An invalid processing action was provided.")
    return cmd


def build_event(es_ref, event_id, actions_lst):
    """
    [build_event] builds a JSON command for an event [event_id] to be associated
    with the named event set [es_ref]. Use the [actions_lst] to specify each processing
    action with an optional set of parameters to be associated with the event.
          actions_lst[0] - Processing action
          actions_lst[1] - Processing action parameter label (optional)
          actions_lst[2] - Processing action parameter value (optional)

    TODO: Update the event and action building functions to handle multiple actions and parms.

    Precondition: es_ref is a named event set that has already been defined.

    Returned Command Format:
    {"event" :
         {
         "es_ref" : “<event set name>”,
         "event_id" : “<security operation event ID>”,
         "actions" : [{"id": “<processing action>”,
                    “<(opt.) action parm label>” : <(opt.) parm value>}]
         }
    }
    """

    new_lst = []

    # For each processing action provided, build the processing action command to
    # append to the event command below
    for action in actions_lst:
        new_lst.append(build_processing_action(action))

    cmd = {"event": {"es_ref": es_ref, "event_id": event_id, "actions": new_lst}}
    return json.dumps(cmd)


def add_event(node_num, es_name, event_id, actions_lst):
    """
    [add_event] adds an event [event_id] with corresponding [actions]
    to an existing, named event set [es_name].

    Precondition: es_name is the name for an existing event set

    Precondition: event_id is a string that represents a valid security operation event

    Precondition: actions is a list of actions. An action is a list with values
    [processing action, optional action param label, optional param value]

    ION utility used: bpsecadmin
    """
    os.chdir(str(node_num) + ".ipn.ltp")

    cmd = "bpsecadmin<<END\na " + build_event(es_name, event_id, actions_lst) + "\nq\nEND"

    out = subprocess.run(cmd, shell=True, timeout=TIME_TESTTIMEOUT, capture_output=True, text=True)

    print_debug(out)

    os.chdir("..")


def del_event(es_name, event_id):
    """"
    [del_event] deletes an event [event_id] from a named event set [es_name]

    Precondition: event_id is a string that represents a valid security operation event

    Precondition: es_name is a string that is the name of an existing event set

    ION utility used: bpsecadmin
    """
    cmd = "bpsecadmin<<END\nd {\"event\" : { \"es_ref\": \"" + es_name + "\", \"event_id\" : \"" + event_id + "\"}}\nq\nEND"

    out = subprocess.run(cmd, shell=True, timeout=TIME_TESTTIMEOUT, capture_output=True, text=True)

    print_debug(out)


def build_sc_param(sc_param):
    """
    [build_sc_param] builds the JSON command necessary to add a pair of security
    context parameters [sc_param] to a security policy command.
        sc_param[0] - security context parameter id
        sc_param[1] - security context parameter value

     TODO: make function more robust and fix header
    """
   
    if len(sc_param) == 2:
        cmd = {sc_param[0] : sc_param[1]}

    else:
        raise AssertionError("Not a valid security context parameter. Expected an id and value pair.")
    
    return cmd


def build_spec(svc, sc_id=None, sc_params=None):
    """
    [build_spec] builds the JSON-formatted specification criteria for a
    security policy rule. The specification criteria include a [svc]
    security service, [scid] security context id, and optional [sc_params].
    """

    # Handle optional security context parameters
    sc_lst = []
    if sc_params != None:
        for x in sc_params:
            sc_lst.append(build_sc_param(x))

    cmd = {"svc": svc, "sc_id": sc_id, "sc_parms": sc_lst}

    spec_cmd = dict(filter(lambda x: x[1] is not None, cmd.items()))

    return spec_cmd


def build_filter(rule_id, role, src=None, dest=None, sec_src=None, tgt=None, sc_id=None):
    """
    [build_filter] builds the JSON-formatted filter criteria for a security
    policy rule. The filter criteria must include a unique [rule_id], security
    [role], and at least one of the following: bundle [source], bundle [dest], or
    [sec_src]. [tgt] identifies the target block type for the security operation
    and [scid] is the identifier for the security context to use when applying
    the specified security operation.
    """
    cmd = {"rule_id": rule_id, "role": role, "src": src, "dest": dest,
           "sec_src": sec_src, "tgt": tgt, "sc_id": sc_id}

    filter_cmd = dict(filter(lambda x: x[1] is not None, cmd.items()))

    return filter_cmd


def build_policy_rule(desc, filter, spec, es_ref):
    """
    [build_policy_rule] builds the JSON-formatted security policy rule command from
    the optional [desc] provided, and the [filter] criteria, [spec] criteria, and
    named eventset [es_ref] provided.

    Returned Command Format:
    a {"policyrule" :
             {
             "desc" : “<description>”,
             "filter" :
                     {
                     “rule_id” : <security policy rule id>,
                     "role" : “<security policy role>”,
                     "src" : “<bundle source>”,
                     "dest" : “<bundle destination>”,
                     “sec_src” : “<security source>”,
                     "tgt" : <security target block type>,
                     "sc_id" : <security context ID>
                     },
             "spec" :
                     {
                     "svc" : “<security service>”,
                     “sc_id” : <security context ID>,
                     "sc_parms" : [{"id": <ID>, "value": <SC parm>}, … ,
                                   {"id": <ID>, "value": <SC parm>}]
                     },
             "es_ref" : “<event set name>”
             }
         }
    """

    cmd = {"policyrule": {"desc": desc, "filter": filter, "spec": spec, "es_ref": es_ref}}

    return json.dumps(cmd)


def add_policy_rule(node_num, desc, filter, spec, es_ref):
    """
    [add_policy_rule] adds a security policy rule to node [node_num]
    referencing an existing event set with name [es_ref]. To build the
    policy rule, provide the [filter] criteria, [spec] criteria, and an
    optional description [desc].

    ION Utility Used: bpsecadmin
    """
    os.chdir(str(node_num) + ".ipn.ltp")

    cmd = "bpsecadmin<<END\na " + build_policy_rule(desc, filter, spec, es_ref) + "\nEND"

    out = subprocess.run(cmd, shell=True, timeout=TIME_TESTTIMEOUT, capture_output=True, text=True)

    print_debug(out)    

    os.chdir("..")


def find_policy_rule(node_num, type, role=None, src=None, dest=None, sec_src=None, tgt=None, sc_id=None, svc=None, es_ref=None):
    """
    [find_policy_rule] executes a find command for policy rules, based on [type] 
    which is set to "best" or "all". The find command matches any criteria
    provided to the set of existing policy rules at the provided [node_num].
    
    ION Utility Used: bpsecadmin
    """

    if ((type == "best") or (type == "all")):

        os.chdir(str(node_num) + ".ipn.ltp")

        cmd_body = {"type": type, "role": role, "src": src, "dest": dest,
               "sec_src": sec_src, "tgt": tgt, "sc_id": sc_id, "svc": svc, "es_ref": es_ref}

        # Clean any omitted filter items
        cmd_body = dict(filter(lambda x: x[1] is not None, cmd_body.items()))

        find_cmd = {"policyrule": cmd_body}
        find_cmd = json.dumps(find_cmd)

        find_cmd = "bpsecadmin<<END\nf " + find_cmd + "\nq\nEND"

        rule_results = subprocess.run(find_cmd, shell=True, timeout=TIME_TESTTIMEOUT, capture_output=True, text=True)

        print_debug(rule_results)

        ret_val = rule_results.stdout

        rule_results = rule_results.stdout.splitlines()

        print("\tFind command results:")
        print_ion_text(rule_results)

        os.chdir("..")

        return ret_val

    return None


def list_policy_rule(node_num):
    """
    [list_policy_rule] lists all of the security policy rules defined at [node_num].

    ION Utility Used: bpsecadmin
    """
    os.chdir(str(node_num) + ".ipn.ltp")

    cmd = "bpsecadmin<<END\nl {\"policyrule\"}\nEND"

    result = subprocess.run(cmd, shell=True, timeout=TIME_TESTTIMEOUT, capture_output=True, text=True)
    
    print_debug(result)

    rule_list = result.stdout.splitlines()

    # Display the security policy rules defined at the node
    print("\tSecurity Policy Rules:")
    print_ion_text(rule_list)

    os.chdir("..")


def info_policy_rule(node_num, rule_id):
    """
    [info_policy_rule] Provides the results of the 'info' policy command run at [node_num] for the [rule_id] 
    provided. 

    ION Utility Used: bpsecadmin
    """
    os.chdir(str(node_num) + ".ipn.ltp")

    cmd = "bpsecadmin<<END\ni {\"policyrule\" : {\"rule_id\" : " + str(rule_id) + " }}\nEND"

    result = subprocess.run(cmd, shell=True, timeout=TIME_TESTTIMEOUT, capture_output=True, text=True)
    
    print_debug(result)
    
    rule_info = result.stdout.splitlines()

    # Display the rules found at the node
    print("\tSecurity Policy Rules:")
    print_ion_text(rule_info)

    os.chdir("..")


def del_policy_rule(node_num, rule_id):
    """
    [del_policy] deletes an policy rule with ID [rule_id] from node
    [node_num]

    Precondition: rule_id is an int that represents the ID of an
     existing policy rule

    ION Utility Used: bpsecadmin
    """
    os.chdir(str(node_num) + ".ipn.ltp")

    cmd = "bpsecadmin<<END\nd {\"policyrule\" : {\"rule_id\" : \"" + rule_id + "\"}}\nq\nEND"

    out = subprocess.run(cmd, shell=True, timeout=TIME_TESTTIMEOUT, capture_output=True, text=True)

    print_debug(out)

    os.chdir("..")


def recv_file(node_num):
    """
    [recv_file] sets up a BP node at [node_num] to receive a large file.

    ION Utility Used: bprecvfile
    """
    os.chdir(str(node_num) + ".ipn.ltp")
    cmd = "bprecvfile ipn:" + str(node_num) + ".1 &"

    time.sleep(TIME_DISPLAYTEXT)

    subprocess.run(cmd, shell=True, stdout=verbose, timeout=TIME_TESTTIMEOUT)

    time.sleep(TIME_TESTFINISH)
    print("bprecvfile run at node ipn:" + str(node_num) + ".1\n")
    
    os.chdir("..")


def send_file(src, dest, file_name, transmit_time):
    """
    [send_file] sends a file with [filename] from [src] to [dest] node taking 
    [transmit_time] specified in seconds.

    Precondition: [file_name] is an existing file at [src].

    ION Utility Used: bpsendfile
    """
    os.chdir(str(src) + ".ipn.ltp")
    cmd = "bpsendfile ipn:" + str(src) + ".1 ipn:" + str(dest) + ".1 " + str(file_name)

    time.sleep(TIME_DISPLAYTEXT)

    print("bpsendfile running from node ipn:" + str(src) + ".1 to ipn:" + str(dest) + ".1 sending file " + str(file_name))
    print("Transmit time: " + str(transmit_time))
    print("\nbpsendfile output:\n")

    subprocess.run(cmd, shell=True, stdout=verbose, timeout=TIME_TESTTIMEOUT)

    time.sleep(transmit_time)
    
    print("bpsendfile complete")

    os.chdir("..")


def send_bundle(start, end, rpt, txt):
    """
    [send_bundle] sends a bundle containing [txt] from node [start] to [end]
    with a report to node [rpt]

    Precondition: start, end, and rpt are valid endpoint IDs
    Precondition: txt is a string

    ION Utility Used: bptrace
    """

    node_num = start[0]
    os.chdir(node_num + ".ipn.ltp")
    cmd = "bptrace ipn:" + start + " ipn:" + end + " ipn:" + rpt + " 5 0.1 \"" + txt + "\""

    time.sleep(TIME_DISPLAYTEXT)
    print("bptrace output:")

    subprocess.run(cmd, shell=True, stdout=verbose, timeout=TIME_TESTTIMEOUT)

    time.sleep(TIME_TESTFINISH)
    print()

    os.chdir("..")


def start_bpsink(node_num):
    """
    [start_bpsink] Starts bpsink on the node specified by [node_num] and
    removes the previous results file, redirecting the bpsink output to
    a new results file at:
        <node_num>.ipn.ltp/<node_num>_results.txt

    ION Utility Used: bpsink
    """
    node = node_num + ".ipn.ltp"
    file = node_num + "_results.txt"
    
    try:
        os.remove(node + "/" + file)
    except FileNotFoundError:
        pass
    
    os.chdir(node)
    open(file, 'w')

    subprocess.run("bpsink ipn:" + node_num +".1 >> " + file + " &", shell=True)

    os.chdir("..")

def clean_ion():
    """
    [clean_ion] Cleans the test environment from previous ION runs. This includes running
    'killm' to ensure that any lingering processes are killed, as well as removing
    existing ion.log files from each node directory.
    """
    print("Cleaning environment from previous ION runs\n\n")
   
    # Removes old ION Logs from each node
    dirs = os.scandir()
    for i in dirs:
        if os.DirEntry.is_dir(i) and "ipn.ltp" in i.name:
            try:
                os.remove(i.name + "/ion.log")
            except FileNotFoundError:
                pass

    subprocess.run("killm", shell=True)

    time.sleep(TIME_CLEANUP)


def node_setup():
    """
    [node_setup] This function configures and starts ION on three nodes:
        ipn:2.1
        ipn:3.1
        ipn:4.1
    The function also starts bpsink on nodes ipn:3.1 and ipn:4.1 to gather result
    messages from use of bptrace in the test scripts.
    """
    print("\n\nStarting ION\n\n")

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

            subprocess.run("ionadmin amroc.ionrc", shell=True)
            subprocess.run("ionadmin ../global.ionrc", shell=True)
            subprocess.run("ionsecadmin amroc.ionsecrc", shell=True)
            subprocess.run("ltpadmin amroc.ltprc", shell=True)
            subprocess.run("bpadmin amroc.bprc", shell=True)
            subprocess.run("bpsecadmin amroc.bpsecrc", shell=True)

            os.chdir("..")

    # Setup bpsink on node ipn:3.1. Write results to file 3_results.txt
    start_bpsink("3")

    # Setup bpsink on node ipn:4.1. Write results to file 4_results.txt
    start_bpsink("4")

    time.sleep(TIME_CLEANUP)


def stop_and_clean():
    """
    [stop_and_clean] stops ION and kills all existing ION processes.

    ION Utilities used: ionstop and killm
    """

    print("\n\nStopping ION\n\n")

    d = os.scandir()
    for i in d:
        if os.DirEntry.is_dir(i) and "ipn.ltp" in i.name:
            os.chdir(i.name)
            subprocess.run("./ionstop &", shell=True, stdout=verbose)
            os.chdir("..")
    
    time.sleep(TIME_TESTFINISH)
    
    subprocess.run("killm")


def start_test(test_num, desc):
    """
    [start_test] prints the test number [test_num] a description [desc] of
    the current test case being run
    """
    print("\n\n***** TEST " + test_num + "*****")
    print(desc)


########################## LOG CHECKING FUNCTIONS #############################

def gen_large_file(node_num, filename, size):
    """
    [gen_large_file] creates a file [filename] at [node_num] with [size] 
    to be used as a message payload. 

    This function provided from: 
    https://www.bswen.com/2018/04/python-How-to-generate-random-large-file-using-python.html
    """

    node = str(node_num) + ".ipn.ltp"
    os.chdir(node)

    chars = ''.join([random.choice(string.ascii_letters) for i in range(size)]) 

    with open(filename, 'w') as f:
        f.write(chars)

    os.chdir("..")

def check_policy_rules(rule_results, rule_id_list):
    """
    [check_policy_rules] accepts the output from a find policyrule command as 
    [rule_results] and checks to ensure that only the [rule_id_list] provided is
    present in those results.

    If each rule ID in the [rule_id_list] is present and no additional rule IDs 
    appear in the [rule_results], the check is determined to be successful. 
    
    This means that an unsuccessful check can indicate either that a rule ID is not 
    present in the [rule_results], or there is an additional rule ID in the 
    [rule_results] that was not expected (part of the [rule_id_list]).

    Return:
        1 - Check was successful.
        0 - Check was unsuccessful. Unexpected rule ID found or rule ID missing.
    """

    rules_found = []
    rule_results = rule_results.splitlines()

    for line in rule_results:
        line.strip()
        if (line and (line[0] != ':')):
            
            # Find rule ID in results
            idx = line.find("#")
            if idx != -1:
                rules_found.append(line[idx + 1])

    rule_id_list.sort()
    rules_found.sort()

    if rule_id_list == rules_found:
        return 1
    else:
        return 0


def print_test_event(name, bsrc, bdest, svc, tgt):
    """
    [print_test_event] prints the details from a test event, which maps to
    a security operation event, providing the bundle source, destination,
    security service and target block type.
    """
    print("\t\tTest event: " + name + " from bundle source " + bsrc + " to bundle " 
            "destination " + bdest + " for " + svc + " on target block " + tgt)


def create_bundle_dict(te_lst):
    """
    [create bundle dict] creates a dictionary (by looking at the test event list [te_lst])
    that maps a unique bundle identifier (bsrc, msec, count) to a unique bundle number.
    """

    bundles = {}
    i = 1

    # For each test event provided for each ion.log file (one per node)
    for log in te_lst:
        
        for te in log:

            # If the bundle identifier is not already known
            # Bundle identifier: bundle source, time (msec), count
            if (te['bsrc'], te['msec'], te['count']) not in bundles:
                
                # Add the new bundle identifier
                bundles[(te['bsrc'], te['msec'], te['count'])] = i
                i += 1

    return bundles


def find_bptrace_msg(node_num, msg, deliver_bundle):
    """
    [find_bptrace_msg] This function  determines if a bptrace [msg] 
    is in the _results.txt file of node [node_num]. 

    A test may be designed in which bundle delivery is expected to fail. In this case,
    [deliver_bundle] is set to False. In this case, the check function ensures that 
    the message is not in the results file from bptrace. Otherwise, [deliver_bundle] 
    is set to True and this function returns success if the message is found.

    Precondition: msg is a string
    Precondition: node_num is an int that represents a node in the bundle's path. Typically
                  the bundle destination.
    Precondition: deliver_bundle is a boolean indicating if the bptrace message is expected to 
                  be found or not, as a result of successful or unsuccessful bundle delivery.
                  Expected failures are typically intentional security policy misconfigurations.

    Return:
        1 - Message is present (deliver_bundle=True) or not present (deliver_bundle=False)
            in the results file as expected.
        0 - Message is not present (deliver_bundle=True) or present (deliver_bundle=False)
            in the results file - opposite of expected outcome.

    Sample Usage:
        find_bptrace_msg(3, "test_trace_ex", False)
    """
    
    try:

        node = str(node_num) + ".ipn.ltp"
        result_file = str(node_num) + "_results.txt"
        results = open(node + "/" + result_file, 'r').read()

        # If a result message is expected from bptrace and found
        # Bundle transmission was successful
        if (msg in results) and (deliver_bundle is True):
            return 1

        # If a result message is NOT expected from bptrace and NOT found
        # Bundle transmission was unsuccessful as expected
        elif (msg not in results) and (deliver_bundle is False):
            return 1

        # If a result message is NOT expected from bptrace and found
        # Bundle transmission was unexpectedly successful
        elif (msg in results) and (deliver_bundle is False):
            print("Bundle transmission succeeded unexpectedly.")
            return 0
        
        # If a result message is expected from bptrace and NOT found
        # Bundle transmission was not successful
        # (msg not in results) and (deliver_bundle is True)
        else: 
            print("Bundle transmission was unsuccessful.")
            return 0

    except:

        # When handling the results of bptrace, a non utf-8 char
        # may be encountered indicating the bundle payload is: 
        # encrypted OR corrupted
        if deliver_bundle is True:
            print("Bundle transmission was unsuccessful.")
            return 0
        else:
            return 1


def inc_passed_tests():
    global g_tests_passed
    g_tests_passed += 1


def inc_failed_tests():
    global g_tests_failed
    g_tests_failed += 1


def check_test_results(node_num, msg, te_lst, deliver_bundle=True):
    """
    [check_test_results] This function first determines the bundle each test event
    in [te_lst] represents and prints this information to the screen. [te_lst] provides a 
    list of test events associated with EACH ion.log file examined for the test case. 
    This is typically the same as the number of nodes used for the test case.

    Second, the check function determines if the bptrace [msg] is in the _results.txt 
    file of node [node_num]. If [deliver_bundle] is True, the bundle is expected to 
    be delivered and this message should be present.
    
    Third, the function checks the test events that accompany the bundles received
    during the test. Test events may be identified as missing or unexpected in the case
    of a test failure.

    Precondition: msg is a string
    Precondition: node_num is an int that represents a node in the bundle's path, typically
                  its destination
    Precondition: te_lst is a dictionary that provides all test events that must be
                  present in the ion.log file for the test to pass.
    Precondition: deliver_bundle is a boolean indicating if the bundle involved in the test was
                  anticipated to be delivered successfully.
                  Expected failures are typically intentional security policy misconfigurations.

    Sample Usage:
        check_test_results(3, "test_trace_ex", [test_event_one, test_event_two])
        check_test_results(3, "test_trace_ex", [test_event_one, test_event_two], False)
    """
    passed = 1
    
    #node = str(node_num) + ".ipn.ltp"
    #file = str(node_num) + "_results.txt"
    #results = open(node + "/" + file, 'r').read()

    bundle_dict = create_bundle_dict(te_lst)
    bundle_output_dict = {}

    # Assemble bundle dictionaries based on test events
    for log in te_lst:
        
        for te in log:

            if(te['status'] == STATUS_MISSING):
                passed = 0
                output_line = "Missing test event: " + str(te['name']) + " for " + str(te['svc']) + " on target " + str(te['tgt'])

            elif(te['status'] == STATUS_UNCHECKED):
                te['status'] = STATUS_UNEXPECT
                passed = 0
                output_line = "Unexpected test event: " + str(te['name']) + " for " + str(te['svc']) + " on target " + str(te['tgt'])
            
            else:
                output_line = str(te['name']) + " for " + str(te['svc']) + " on target " + str(te['tgt'])

            # Extract the bundle identifying characteristics
            bundle_id = bundle_dict[(te['bsrc'], te['msec'], te['count'])]

            # If the bundle id has already been encountered, this test event belongs
            # to an existing bundle dictionary
            if bundle_id in bundle_output_dict:
                bundle_output_dict[bundle_id] = bundle_output_dict[bundle_id] + "\n\t" + output_line
            
            # Otherwise, add this bundle to the bundle dictionary
            else:
                bundle_output_dict[bundle_id] = "\t" + output_line

    # Print the test events for each bundle
    for id, num in zip(bundle_dict, bundle_output_dict):
        
        print("\nBundle " + str(num) + ": " + str(id))
        print("Received with the following test event(s) checked: ")
        print(bundle_output_dict[num])

    print()

    # Check the bundle delivery status
    for i in range(len(node_num)):
        
        msg_success = find_bptrace_msg(node_num[i], msg[i], deliver_bundle)

        if msg_success == 0:
            passed = 0

    # Handle test status
    if passed == 1:
        print("TEST PASSED")
        inc_passed_tests()
    
    else:
        print("TEST FAILED")
        inc_failed_tests()


def log_position(node_num):
    """
    [log_position] returns the number of the last line in the ion.log file
     at node [node_num]

    Precondition: [node_num] is an int that represents the directory of a
    node in a bundle's path
    """
    node = str(node_num) + ".ipn.ltp"
    log = open(node + "/ion.log", 'r+')
    log.read()
    log_position = log.tell()
    log.close()
    return log_position


def parse_test_event(line):
    """
    [parse_test_event] parses a test event [line] present in the ion.log file
    and returns that test event formatted as a dictionary for later processing.

    Precondition: [line] is a string from the ion.log file
    """
    keys = ['bsrc', 'bdest', 'svc', 'tgt', 'msec', 'count']
    
    # Strip the test event identifier "[te] <security op event> - "
    new_line = line[line.find(' - ') + 3:]

    test_event_dict = {}

    for te_field in keys:
        new_line = new_line[new_line.find(te_field):]

        st = new_line.find(':') + 1
        end = new_line.find(',')
        end = end if end != -1 else len(new_line) - 1

        test_event_dict[te_field] = new_line[st:end].strip(' ')

    # Extract the name of the test event separately as it is included in the test event
    # identifier "[te - <name>]"
    test_event_dict['name'] = line[line.rfind(']') + 1:line.rfind(' - '):].strip(' ')
    
    return test_event_dict


def te_equal(expected_te, curr_te):
    """
    [te_equal] compares an expected test event [expected_te] and an actual
    test event from the ion.log file [curr_te].

    Precondition: [expected_te] and [curr_te] are dictionaries that represent 
    test events. Parse a test event using the parse_test_event function to 
    create this dictionary.
    """
    
    keys = ['name', 'bsrc', 'bdest', 'svc', 'tgt', 'msec', 'count']

    # If working with a security operation from the security source,
    # do not compare the bundle identifiers 'msec' and 'count' 
    # as this is the first place they are populated.
    if 'src' in expected_te['name']:
        keys.remove('msec')
        keys.remove('count')

    # Compare each item in the test event identified in the keys[] list above
    for item in keys:
        if (item == 'tgt') and (int(expected_te[item]) != int(curr_te[item])):
            return False
        elif (item != 'tgt') and (expected_te[item] != curr_te[item]):
            return False

    return True


def get_test_events(curr_node, log_position):
    """
    [get_test_events] retrieves all test events from the [curr_node] ion.log file
    starting at [log_position], providing every test event that has been generated by running
    the current test case.
    """
    # Open the ion log at the provided position to read all test events added by the 
    # current test case
    node = str(curr_node) + ".ipn.ltp"
    log = open(node + "/ion.log", 'r+')
    log.seek(log_position)

    # A list of test event dictionaries to be returned by the function
    test_event_list = []

    for line in log:

        if "[te]" in line:

            # Create a test event dictionary from the test event found in the log file
            test_event = parse_test_event(line)

            # All test events are noted as 'unchecked' until the test framework
            # verifies if they were expected or not - handled in the find_test_event calls
            test_event['status'] = STATUS_UNCHECKED

            test_event_list.append(test_event)
    
    return test_event_list


def find_test_event(te_list, name, bsrc, bdest, svc, tgt, msec, count, find=True):
    """
    [find_test_event] examines the test events in te_list extracted from the local ion.log file
    to locate the test event identified by the [name], [bsrc], [bdest], [svc], [tgt], [msec], 
    and [count] fields. 

    [find] indicates if the test event is expected to be found or not in the ion.log. 
    [find] defaults to true.

    This function populates the 'status' field to the test event returned, providing one of the 
    following statuses:
        STATUS_SUCCESS = 1  Test event found if expected, or not found if not expected
        STATUS_MISSING = 2  Test event not found but expected - failure indicator
        STATUS_UNEXPECT = 3 Test event found but not expected - failure indicator

    Precondition: [te_list] provides all test events in dictgionary form that have been extracted
    from the ion.log file where policy for this event was configured.

    Return: te_list modified to indicate the new status of a test event (if found).

    Examples:
        name: 'sop_added_at_src', 'sop_verified', 'sop_processed'
        bsrc: 'ipn:2.1', 'ipn:3.1', 'ipn:4.1'
        bdest: 'ipn:3.1', 'ipn:4.1'
        svc:  'bib-integrity', 'bcb-confidentiality'
        tgt: '0','1', '11' blocks that bib/bcb target (ie. payload bock, primary block)
        msec: None
        count: None, '0', '1'
        curr_node: '2', '3', '4'  Number of the node in the bundle path 
        log_position: position to start scanning in the ion.log file
    """
    expected_test_event = {'name': name, 'bsrc': bsrc, 'bdest': bdest, 'svc': svc,
                           'tgt': tgt, 'msec': msec, 'count': count}

    # Check if the provided test event is present in the list of all known test events at the 
    # current node
    for te in te_list:

        if te_equal(expected_test_event, te):
            
            # If this test event was expected to be found
            if find is True:
                te['status'] = STATUS_SUCCESS
                return te, te_list

            # If this test event was found unexpectedly (find is False)
            else:
                te['status'] = STATUS_UNEXPECT
                return te, te_list

    # If we make it to this point, the test event was not found in ion.log

    # If this test event was expected to be found, it is missing
    if find is True:

        expected_test_event['status'] = STATUS_MISSING

        #Add this test event to the list of test events for the node
        te_list.append(expected_test_event)
        return expected_test_event, te_list

    # If this test event was NOT expected to be found, the check was successful
    else:
        #Add this test event to the list of test events for the node
        te_list.append(expected_test_event)

        expected_test_event['status'] = STATUS_SUCCESS
        return expected_test_event, te_list
