import os
import subprocess
import time
import json

'''
    The Johns Hopkins University Applied Physics Laboratory (JHU/APL)

    File Name: testUtils.py

    Description: This file contains utility functions for the BPSec Python
    test suite. These functions can create an ionsecadmin or bpsecadmin command
    and execute them, as well as perform the log checking necessary to determine
    if a test has passed.
'''

################################ Global Data  #################################

# Used to track the total number of tests passed and failed
global g_tests_passed
global g_tests_failed

# Use os.devnull for verbose logging. Set this variable to the preferred
# location for stdout
verbose = subprocess.DEVNULL

# Number of seconds before an attempted subprocess call should timeout
timeout_sec = 10


################################ ION COMMANDS #################################


def add_key(node_num, k_name):
    """[add_key] adds a key to a node with a name [k_name] to the node at
    [node_num].

    Precondition: k_name is a string that is the name of a key.

    Assumption: k_name is the name of the .hmk key file to use to create the key.

    ION utility used: ionsecadmin
    """

    os.chdir(str(node_num) + ".ipn.ltp")
    cmd = "a key " + k_name + " " + k_name + ".hmk"
    subprocess.run("ionsecadmin<<END\n" + cmd + "\nq\nEND",
                   shell=True, stdout=verbose, timeout=timeout_sec)
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
    cmd = "d key " + k_name
    subprocess.run("ionsecadmin<<END\n" + cmd + "\nq\nEND",
                   shell=True, stdout=verbose, timeout=timeout_sec)
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

    subprocess.call("bpsecadmin<<END\n a " + cmd + "\nq\nEND",
                    shell=True, stdout=verbose, timeout=timeout_sec)
    os.chdir("..")


def del_event_set(node_num, es_name):
    """"
    [del_event_set] deletes an event set with a name [es_name] from node
    [node_num].

    Precondition: es_name is a string that is the name for an existing event set
    """

    os.chdir(str(node_num) + ".ipn.ltp")

    cmd = "d {\"event_set\" : {\"name\" : \"" + es_name + "\"}}"

    subprocess.call("bpsecadmin<<END\n" + cmd + "\nq\nEND",
                    shell=True, stdout=verbose, timeout=timeout_sec)
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
        raise AssertionError("Not a valid action")
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


def add_event(es_name, event_id, actions_lst):
    """
    [add_event] adds an event [event_id] with corresponding [actions]
    to an existing, named event set [es_name].

    Precondition: es_name is the name for an existing event set

    Precondition: event_id is a string that represents a valid security operation event

    Precondition: actions is a list of actions. An action is a list with values
    [processing action, optional action param label, optional param value]

    ION utility used: bpsecadmin
    """

    cmd = "bpsecadmin<<END\na " + build_event(es_name, event_id, actions_lst) + \
          "\nq\nEND"

    subprocess.call(cmd, shell=True, stdout=verbose, timeout=timeout_sec)


def del_event(es_name, event_id):
    """"
    [del_event] deletes an event [event_id] from a named event set [es_name]

    Precondition: event_id is a string that represents a valid security operation event

    Precondition: es_name is a string that is the name of an existing event set

    ION utility used: bpsecadmin
    """
    cmd = "d {\"event\" : { \"es_ref\": \"" + es_name + "\", \"event_id\" : \"" \
          + event_id + "\"}}"

    subprocess.call("bpsecadmin<<END\n" + cmd + "\nq\nEND",
                    shell=True, stdout=verbose, timeout=timeout_sec)


def build_sc_param(sc_param):
    """
    [build_sc_param] builds the JSON command necessary to add a pair of security
    context parameters [sc_param] to a security policy command.
        sc_param[0] - security context parameter id
        sc_param[1] - security context parameter value
    """

    if len(sc_param) == 2:
        cmd = {"id": sc_param[0], "value": sc_param[1]}
    else:
        raise AssertionError("Not a valid sc_param")
    return cmd


def build_spec(svc, scid, sc_params):
    """
    [build_spec] builds the JSON-formatted specification criteria for a
    security policy rule. The specification criteria include a [svc]
    security service, [scid] security context id, and optional [sc_params].
    """

    sc_lst = []
    # Handle optional security context parameters
    for x in sc_params:
        sc_lst.append(build_sc_param(x))

    cmd = {"svc": svc, "sc_id": scid, "sc_parms": sc_lst}
    return cmd


def build_filter(rule_id, role, src=None, dest=None, sec_src=None, tgt=None, scid=None):
    """
    [build_filter] builds the JSON-formatted filter criteria for a security
    policy rule. The filter criteria must include a unique [rule_id], security
    [role], and at least one of the following: bundle [source], bundle [dest], or
    [sec_src]. [tgt] identifies the target block type for the security operation
    and [scid] is the identifier for the security context to use when applying
    the specified security operation.
    """
    cmd = {"rule_id": rule_id, "role": role, "src": src, "dest": dest,
           "sec_src": sec_src, "tgt": tgt, "scid": scid}

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
                     "scid" : <security context ID>
                     },
             "spec" :
                     {
                     "svc" : “<security service>”,
                     “scid” : <security context ID>,
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

    cmd = "bpsecadmin<<END\na " + build_policy_rule(desc, filter, spec, es_ref) + \
          "\nq\nEND"

    subprocess.call(cmd, shell=True, timeout=timeout_sec, stdout=verbose)
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
    cmd = "d {\"policyrule\" : {\"rule_id\" : \"" + rule_id + "\"}}"

    subprocess.call("bpsecadmin<<END\n" + cmd + "\nq\nEND",
                    shell=True, timeout=timeout_sec, stdout=verbose)
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
    cmd = "bptrace ipn:" + start + " ipn:" + end + \
          " ipn:" + rpt + " 5 0.1 \"" + txt + "\""

    time.sleep(2)
    subprocess.run(cmd, shell=True, stdout=verbose)
    time.sleep(5)
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
    os.remove(node + "/" + file)
    os.chdir(node)
    open(file, 'w')
    subprocess.call("bpsink ipn:" + node_num +
                    ".1 >> " + file + " &", shell=True)
    os.chdir("..")


def stop_and_clean():
    """
    [stop_and_clean] stops ION and kills all existing ION processes.

    ION Utilities used: ionstop and killm
    """

    print("Stopping ION")
    d = os.scandir()
    for i in d:
        if os.DirEntry.is_dir(i) and "ipn.ltp" in i.name:
            os.chdir(i.name)
            subprocess.call("./ionstop &", shell=True, stdout=verbose)
            os.chdir("..")
    time.sleep(12)
    subprocess.call("killm")


def start_test(test_num, desc):
    """
    [start_test] prints the test number [test_num] a description [desc] of
    the current test case being run
    """
    print("\n\n***** TEST " + test_num + "*****")
    print(desc)


########################## LOG CHECKING FUNCTIONS #############################


def print_test_event(name, bsrc, bdest, svc, tgt):
    """
    [print_test_event] prints the details from a test event, which maps to
    a security operation event, providing the bundle source, destination,
    security service and target block type.
    """
    print("Test event: " + name + " from bundle source " + bsrc + " to bundle " 
                                                                  "destination " + bdest + " for " + svc + " on target block " + tgt)


def create_bundle_dict(te_lst):
    """
    [create bundle dict] creates a dictionary (by looking at the test event list [te_lst])
    that maps a unique bundle identifier (bsrc, msec, count) to a unique bundle number 
    """

    bundles = {}
    i = 1
    for x in te_lst:
        if (x['bsrc'], x['msec'], x['count']) not in bundles:
            bundles[(x['bsrc'], x['msec'], x['count'])] = i
            i += 1
    return bundles


def inc_passed_tests():
    global g_tests_passed
    g_tests_passed += 1


def inc_failed_tests():
    global g_tests_failed
    g_tests_failed += 1


def check_test_results(node_num, msg, expect_fail, te_lst):
    """
    [check_test_results] This function first determines the bundle each test event
    in [te_lst] represents and prints this information to the screen. Second, the
    check function determines if the expected [msg] is in the _results.txt file of
    node [node_num]. Third, the function looks for an instance of each test event
    from [te_lst] in the node's ion.log file.

    A test may be expected to fail, which can be indicated by setting [expect_fail]
    to True. In this case, the check function notes the test as passing if one
    or more of the listed test events in te_lst are missing.

    Where [expect_fail] is False, the check function ensures that both the [msg] is
    in the results.txt file and all of the test events in [te_lst] are present
    in the ION log.

    Precondition: msg is a string
    Precondition: node_num is an int that represents a node in the bundle's path, typically
                  its destination
    Precondition: expect_fail is a boolean indicating if the test was expected to fail or not.
                  Expected failures are typically intentional security policy misconfigurations.
    Precondition: te_lst is a dictionary that provides all test events that must be
                  present in the ion.log file for the test to pass.

    Sample Usage:
        testFunctions.check("3.ipn.ltp/3_results.txt", False, "test_trace_ex")
    """
    node = str(node_num) + ".ipn.ltp"
    file = str(node_num) + "_results.txt"

    results = open(node + "/" + file, 'r').read()
    bundle_dict = create_bundle_dict(te_lst)
    bundle_output_dict = {}

    for x in te_lst:

        if expect_fail:
            print("Test event(s) NOT anticipated to be found")

        output_line = (x['name']).upper() + " " + \
                      x['svc'] + " on target " + x['tgt']

        bundle_num = bundle_dict[(x['bsrc'], x['msec'], x['count'])]

        if bundle_num in bundle_output_dict:
            bundle_output_dict[bundle_num] = bundle_output_dict[bundle_num] + \
                                             "\n\t" + output_line
        else:
            bundle_output_dict[bundle_num] = "\t" + output_line

    for x, y in zip(bundle_dict, bundle_output_dict):
        print("\nBundle " + str(y) + ": " + str(x))
        print(bundle_output_dict[y])
    print()

    if expect_fail:
        if msg in results and not any(te['is_found'] for te in te_lst):
            print("TEST SUCCESSFUL: Test configuration failed as expected")
            inc_passed_tests()

        elif msg not in results:
            print(
                "TEST FAILED: bptrace message was not in " + str(node_num) + "_results.txt")
            inc_failed_tests()

        else:
            print("TEST FAILED: Test unexpectedly passed. Invalid test event(s):")
            for te in te_lst:
                if te['is_found']:
                    print("Invalid: ")
                    print_test_event(te['name'], te['bsrc'], te['bdest'], te['svc'], te['tgt'])
            inc_failed_tests()
    else:
        if msg in results and all(te['is_found'] for te in te_lst):
            print("TEST SUCCESSFUL")
            inc_passed_tests()

        elif msg not in results:
            print("TEST FAILED: bptrace message was not in " + str(node_num) + "_results.txt")
            inc_failed_tests()

        else:
            print("TEST FAILED: One or more test events missing.")
            for te in te_lst:
                if not te['is_found']:
                    print("Missing: ")
                    print_test_event(te['name'], te['bsrc'], te['bdest'], te['svc'], te['tgt'])
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
    [parse_test_event] parses a test event [line] from the ion log and returns 
    a dictionary

    Precondition: [line] is a string from the ion.log file
    """
    keys = ['bsrc', 'bdest', 'svc', 'tgt', 'msec', 'count']
    new_line = line[line.find(' - ') + 3:]
    test_event_dict = {}

    for x in keys:
        new_line = new_line[new_line.find(x):]

        st = new_line.find(':') + 1
        end = new_line.find(',')
        end = end if end != -1 else len(new_line) - 2

        test_event_dict[x] = new_line[st:end].strip(' ')

    test_event_dict['name'] = line[line.rfind(']') + 1:line.rfind(' - '):].strip(' ')
    return test_event_dict


def te_equal(expected_te, curr_te):
    """
    [te_equal] compares an expected test event [expected_te] and an actual
     test event from the ion.log file [curr_te]

    Precondition: [expected_te] and [curr_te] are dictionaries
    that represent test events. Parse a test event using the
    parse_test_event function.
    """
    keys = ['name', 'bsrc', 'bdest', 'svc', 'tgt', 'msec', 'count']
    if 'src' in expected_te['name']:
        keys.remove('msec')
        keys.remove('count')

    for x in keys:
        if expected_te[x] != curr_te[x]:
            return False

    return True


def check_test_event(name, bsrc, bdest, svc, tgt, msec, count, curr_node, log_position):
    """
    [check_test_event] checks that an expected test event is present in the ion.log file.
    The test event is identified by [name] which is the security operation event expected
    to be found in the ION log.
    If the test event is found, the function returns that test event from the ion log.
    If the test event is not found, the function returns the expected test event
    but changes the event name to to <node security role>'_missing'

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

    node = str(curr_node) + ".ipn.ltp"
    log = open(node + "/ion.log", 'r+')

    log.seek(log_position)

    for line in log:

        if "[te]" in line:

            actual_test_event = parse_test_event(line)

            if te_equal(expected_test_event, actual_test_event):
                actual_test_event['is_found'] = True
                return actual_test_event

    expected_test_event['is_found'] = False
    return expected_test_event
