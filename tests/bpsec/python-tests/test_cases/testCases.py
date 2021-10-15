import os
import subprocess
import time

from test_cases import testUtils

'''
    The Johns Hopkins University Applied Physics Laboratory (JHU/APL)

    File Name: testCases.py

    Description: This file contains each of the BPSec test cases supported
    by this Python test suite.
'''

TIME_DISPLAYTEXT = 2
TIME_TESTFINISH = 5
g_tests_failed = 0
g_tests_passed = 0

############################### BP BLOCK TYPES ################################
UNKNOWNLK = -1,
PRIMARY = 0,
PAYLOAD = 1,
PREVNODE = 6,
BUNDLEAGE = 7,
METADATA = 8,
HOPCOUNT = 10,
BIB = 11,
BCB = 12,
DATALABEL = 192,
QUALITYOFSERVICE = 193,
SNWPERMITS = 194,
IMCDESTINATION = 195,
RGR = 196,
CGRR = 197

############################## TEST CASES #####################################


def test1():
    print("###############################################################################")

    try:
        testUtils.start_test(
            "1", "Brief: BIB targeting a Payload Block.\n\n"
                 "Description: Add a BIB targeting a Payload Block at \n"
                 "Security Source ipn:2.1 and process that BIB at Security \n"
                 "Acceptor ipn:3.1\n\n"
                 "Bundle Path: ipn:2.1 -> ipn:3.1\n\n")
        time.sleep(TIME_DISPLAYTEXT)

        # Tracking current position in log
        log2 = testUtils.log_position("2")
        log3 = testUtils.log_position("3")

        print("Node ipn:2.1 Configuration\n")
        testUtils.add_key(2, "key1")
        testUtils.add_event_set(2, "d_integrity", "default integrity event set")

        desc = "bib src tgt:payload"
        filter = testUtils.build_filter("1", "s", "ipn:2.1", tgt=1)
        spec = testUtils.build_spec("bib-integrity", 2, [["key_name", "key1"]])
        es_ref = "d_integrity"
        testUtils.add_policy_rule(2, desc, filter, spec, es_ref)

        print("Node ipn:3.1 Configuration \n")
        testUtils.add_key(3, "key1")
        testUtils.add_event_set(3, "d_integrity", "default integrity event set")

        desc = "bib acceptor tgt:payload"
        filter = testUtils.build_filter("2", "a", "ipn:2.1", tgt=1)
        spec = testUtils.build_spec("bib-integrity", 2, [["key_name", "key1"]])
        es_ref = "d_integrity"
        testUtils.add_policy_rule(3, desc, filter, spec, es_ref)

        # Send the bundle from ipn:2.1 -> ipn:3.1
        testUtils.send_bundle("2.1", "3.1", "2.0", "test_trace1")

        print("\n\nClear Node ipn:2.1 Policy \n")
        testUtils.del_policy_rule(2, "1")
        testUtils.del_event_set(2, "d_integrity")
        testUtils.del_key(2, "key1")

        print("Clear Node ipn:3.1 Policy\n")
        testUtils.del_policy_rule(3, "2")
        testUtils.del_event_set(3, "d_integrity")
        testUtils.del_key(3, "key1")

        te_1 = testUtils.check_test_event(
            'sop_added_at_src', 'ipn:2.1', 'ipn:3.1', 'bib-integrity', '1', None, None, '2', log2)
        te_2 = testUtils.check_test_event(
            'sop_processed', 'ipn:2.1', 'ipn:3.1', 'bib-integrity', '1', te_1['msec'], te_1['count'], '3', log3)

        testUtils.check_test_results(3, "test_trace1", False, [te_1, te_2])

    except subprocess.TimeoutExpired as e:
        print(e)
        print("TEST FAILED")
        dotest.g_tests_failed += 1
        os.chdir("..")

    print("###############################################################################")


def test2():
    print("###############################################################################")

    try:
        testUtils.start_test(
            "2", "Brief: BCB targeting a Payload Block.\n\n"
                 "Description: Add a BCB targeting a Payload Block at \n"
                 "Security Source ipn:2.1 and process that BCB at Security \n"
                 "Acceptor ipn:3.1\n\n"
                 "Bundle Path: ipn:2.1 -> ipn:3.1\n\n")
        time.sleep(TIME_DISPLAYTEXT)

        # Tracking current position in log
        log2 = testUtils.log_position("2")
        log3 = testUtils.log_position("3")

        print("Node ipn:2.1 Configuration\n")
        testUtils.add_key(2, "bcbkey")
        testUtils.add_event_set(2, "d_bcb_conf", "default confidentiality event set")

        desc = "bcb src tgt:payload"
        filter = testUtils.build_filter("4", "s", "ipn:2.1", tgt=1)
        spec = testUtils.build_spec("bcb-confidentiality", 8, [["key_name", "bcbkey"]])
        es_ref = "d_bcb_conf"
        testUtils.add_policy_rule(2, desc, filter, spec, es_ref)

        print("Node ipn:3.1 Configuration \n")
        testUtils.add_key(3, "bcbkey")
        testUtils.add_event_set(3, "d_bcb_conf", "default confidentiality event set")

        desc = "bcb acceptor tgt:payload"
        filter = testUtils.build_filter("5", "a", "ipn:2.1", tgt=1)
        spec = testUtils.build_spec("bcb-confidentiality", 8, [["key_name", "bcbkey"]])
        es_ref = "d_bcb_conf"
        testUtils.add_policy_rule(3, desc, filter, spec, es_ref)

        # Send the bundle from ipn:2.1 -> ipn:3.1
        testUtils.send_bundle("2.1", "3.1", "2.0", "test_trace2")

        print("\n\nClear Node ipn:2.1 Policy \n")
        testUtils.del_policy_rule(2, "4")
        testUtils.del_event_set(2, "d_bcb_conf")
        testUtils.del_key(2, "bcbkey")

        print("Clear Node ipn:3.1 Policy\n")
        testUtils.del_policy_rule(3, "5")
        testUtils.del_event_set(3, "d_bcb_conf")
        testUtils.del_key(3, "bcbkey")

        te_1 = testUtils.check_test_event(
            'sop_added_at_src', 'ipn:2.1', 'ipn:3.1', 'bcb-confidentiality', '1', None, None, '2', log2)
        te_2 = testUtils.check_test_event(
            'sop_processed', 'ipn:2.1', 'ipn:3.1', 'bcb-confidentiality', '1', te_1['msec'], te_1['count'], '3', log3)

        testUtils.check_test_results(3, "test_trace2", False, [te_1, te_2])

    except subprocess.TimeoutExpired as e:
        print(e)
        print("TEST FAILED")
        dotest.g_tests_failed += 1
        os.chdir("..")

    print("###############################################################################")


def test3():
    print("###############################################################################")

    try:
        testUtils.start_test(
            "3", "Brief: BIB targeting a Primary Block.\n\n"
                 "Description: Add a BIB targeting a Primary Block at \n"
                 "Security Source ipn:2.1 and process that BIB at Security \n"
                 "Acceptor ipn:3.1\n\n"
                 "Bundle Path: ipn:2.1 -> ipn:3.1\n\n")
        time.sleep(TIME_DISPLAYTEXT)

        # Tracking current position in log
        log2 = testUtils.log_position("2")
        log3 = testUtils.log_position("3")

        print("Node ipn:2.1 Configuration\n")
        testUtils.add_key(2, "key1")
        testUtils.add_event_set(2, "d_integrity", "default integrity event set")

        desc = "bib src tgt:primary"
        filter = testUtils.build_filter("6", "s", "ipn:2.1", tgt=0)
        spec = testUtils.build_spec("bib-integrity", 2, [["key_name", "key1"]])
        es_ref = "d_integrity"
        testUtils.add_policy_rule(2, desc, filter, spec, es_ref)

        print("Node ipn:3.1 Configuration \n")
        testUtils.add_key(3, "key1")
        testUtils.add_event_set(3, "d_integrity", "default integrity event set")

        desc = "bib acceptor tgt:primary"
        filter = testUtils.build_filter("7", "a", "ipn:2.1", tgt=0)
        spec = testUtils.build_spec("bib-integrity", 2, [["key_name", "key1"]])
        es_ref = "d_integrity"
        testUtils.add_policy_rule(3, desc, filter, spec, es_ref)

        testUtils.send_bundle("2.1", "3.1", "2.0", "test_trace3")

        print("\n\nClear Node ipn:2.1 Policy \n")
        testUtils.del_policy_rule(2, "6")
        testUtils.del_event_set(2, "d_integrity")
        testUtils.del_key(2, "key1")

        print("Clear Node ipn:3.1 Policy \n")
        testUtils.del_policy_rule(3, "7")
        testUtils.del_event_set(3, "d_integrity")
        testUtils.del_key(3, "key1")

        te_1 = testUtils.check_test_event(
            'sop_added_at_src', 'ipn:2.1', 'ipn:3.1', 'bib-integrity', '0', None, None, '2', log2)
        te_2 = testUtils.check_test_event(
            'sop_processed', 'ipn:2.1', 'ipn:3.1', 'bib-integrity', '0', te_1['msec'], te_1['count'], '3', log3)

        testUtils.check_test_results(3, "test_trace3", False, [te_1, te_2])

    except subprocess.TimeoutExpired as e:
        print(e)
        print("TEST FAILED")
        dotest.g_tests_failed += 1
        os.chdir("..")

    print("###############################################################################")


def test4():
    print("###############################################################################")

    try:
        testUtils.start_test(
            "4", "Brief: BCB targeting a Primary Block. Note this is an INVALID\n"
                 "configuration\n\n"
                 "Description: Attempt to add a BCB targeting a Primary Block,\n"
                 "which is not permitted, at Security Source ipn:2.1 and \n"
                 "process that BCB at Security Acceptor ipn:3.1. \n"
                 "A BCB should not be added to this bundle.\n\n"
                 "Bundle Path: ipn:2.1 -> ipn:3.1\n\n")
        time.sleep(TIME_DISPLAYTEXT)

        # Tracking current position in log
        log2 = testUtils.log_position("2")
        log3 = testUtils.log_position("3")

        print("Node ipn:2.1 Configuration\n")
        testUtils.add_key(2, "bcbkey")
        testUtils.add_event_set(2, "d_bcb_conf", "default confidentiality event set")

        desc = "bcb src tgt:primary"
        filter = testUtils.build_filter("8", "s", "ipn:2.1", tgt=0)
        spec = testUtils.build_spec("bcb-confidentiality", 8, [["key_name", "bcbkey"]])
        es_ref = "d_bcb_conf"
        testUtils.add_policy_rule(2, desc, filter, spec, es_ref)

        print("Node ipn:3.1 Configuration \n")
        testUtils.add_key(3, "bcbkey")
        testUtils.add_event_set(3, "d_bcb_conf", "default confidentiality event set")

        desc = "bcb acceptor tgt:primary"
        filter = testUtils.build_filter("9", "a", "ipn:2.1", tgt=0)
        spec = testUtils.build_spec("bcb-confidentiality", 8, [["key_name", "bcbkey"]])
        es_ref = "d_bcb_conf"
        testUtils.add_policy_rule(3, desc, filter, spec, es_ref)

        # Send the bundle from ipn:2.1 -> ipn:3.1
        testUtils.send_bundle("2.1", "3.1", "2.0", "test_trace4")

        print("\n\nClear Node ipn:2.1 Policy \n")
        testUtils.del_policy_rule(2, "8")
        testUtils.del_event_set(2, "d_bcb_conf")
        testUtils.del_key(2, "bcbkey")

        print("Clear Node ipn:3.1 Policy \n")
        testUtils.del_policy_rule(3, "9")
        testUtils.del_event_set(3, "d_bcb_conf")
        testUtils.del_key(3, "bcbkey")

        te_1 = testUtils.check_test_event(
            'sop_added_at_src', 'ipn:2.1', 'ipn:3.1', 'bcb-confidentiality', '0', None, None, '2', log2)
        te_2 = testUtils.check_test_event(
            'sop_processed', 'ipn:2.1', 'ipn:3.1', 'bcb-confidentiality', '0', te_1['msec'], te_1['count'], '3', log3)

        # Note that we do not expect to find these test events as
        # security policy is intentionally misconfigured
        testUtils.check_test_results(3, "test_trace4", True, [te_1, te_2])

    except subprocess.TimeoutExpired as e:
        print(e)
        print("TEST FAILED")
        dotest.g_tests_failed += 1
        os.chdir("..")

    print("###############################################################################")


def test5():
    print("###############################################################################")

    try:
        testUtils.start_test(
            "5",
            "Brief: BIB targeting a Payload Block, being processed at a \n"
            "Security Source, Verifier, and Acceptor.\n\n"
            "Description: Add a BIB targeting a Payload Block\n"
            "at Security Source ipn:2.1, transmit to the Security Verifier\n"
            "ipn:3.1 to check integrity and process that BIB at\n"
            "Security Acceptor ipn:4.1.\n\n"
            "Bundle Path: ipn:2.1 -> ipn:3.1 -> ipn:4.1\n\n")
        time.sleep(TIME_DISPLAYTEXT)

        # Tracking current position in log
        log2 = testUtils.log_position("2")
        log3 = testUtils.log_position("3")
        log4 = testUtils.log_position("4")

        print("Node ipn:2.1 Configuration\n")
        testUtils.add_key(2, "key1")
        testUtils.add_event_set(2, "d_integrity", "default integrity event set")

        desc = "bib src tgt:payload"
        filter = testUtils.build_filter("10", "s", "ipn:2.1", tgt=1)
        spec = testUtils.build_spec("bib-integrity", 2, [["key_name", "key1"]])
        es_ref = "d_integrity"
        testUtils.add_policy_rule(2, desc, filter, spec, es_ref)

        print("Node ipn:3.1 Configuration \n")
        testUtils.add_key(3, "key1")
        testUtils.add_event_set(3, "d_integrity", "default integrity event set")

        desc = "bib verifier tgt:payload"
        filter = testUtils.build_filter("11", "v", "ipn:2.1", tgt=1)
        spec = testUtils.build_spec("bib-integrity", 2, [["key_name", "key1"]])
        es_ref = "d_integrity"
        testUtils.add_policy_rule(3, desc, filter, spec, es_ref)

        print("Node ipn:4.1 Configuration\n")
        testUtils.add_key(4, "key1")
        testUtils.add_event_set(4, "d_integrity", "default integrity event set")

        desc = "bib acceptor tgt:payload"
        filter = testUtils.build_filter("12", "a", "ipn:2.1", tgt=1)
        spec = testUtils.build_spec("bib-integrity", 2, [["key_name", "key1"]])
        es_ref = "d_integrity"
        testUtils.add_policy_rule(4, desc, filter, spec, es_ref)

        # Send the bundle from ipn:2.1 -> ipn:3.1 -> ipn:4.1
        testUtils.send_bundle("2.1", "4.1", "2.0", "test_trace5")

        print("\n\nClear Node ipn:2.1 Policy \n")
        testUtils.del_policy_rule(2, "10")
        testUtils.del_event_set(2, "d_integrity")
        testUtils.del_key(2, "key1")

        print("Clear Node ipn:3.1 Policy \n")
        testUtils.del_policy_rule(3, "11")
        testUtils.del_event_set(3, "d_integrity")
        testUtils.del_key(3, "key1")

        print("Clear Node ipn:4.1 Policy \n\n")
        testUtils.del_policy_rule(4, "12")
        testUtils.del_event_set(4, "d_integrity")
        testUtils.del_key(4, "key1")

        te_1 = testUtils.check_test_event(
            'sop_added_at_src', 'ipn:2.1', 'ipn:4.1', 'bib-integrity', '1', None, None, '2', log2)
        te_2 = testUtils.check_test_event(
            'sop_verified', 'ipn:2.1', 'ipn:4.1', 'bib-integrity', '1', te_1['msec'], te_1['count'], '3', log3)
        te_3 = testUtils.check_test_event(
            'sop_processed', 'ipn:2.1', 'ipn:4.1', 'bib-integrity', '1', te_1['msec'], te_1['count'], '4', log4)

        testUtils.check_test_results(4, "test_trace5", False, [te_1, te_2, te_3])

    except subprocess.TimeoutExpired as e:
        print(e)
        print("TEST FAILED")
        dotest.g_tests_failed += 1
        os.chdir("..")

    print("###############################################################################")


def test6():
    print("###############################################################################")

    try:
        testUtils.start_test(
            "6",
            "Brief: BCB targeting a Payload Block, being processed at a \n"
            "Security Source, Verifier, and Acceptor.\n\n"
            "Description: Add a BCB targeting a Payload Block\n"
            "at Security Source ipn:2.1, transmit to the Security Verifier \n"
            "ipn:3.1, retain the BCB, and process that BCB at\n"
            "Security Acceptor ipn:4.1.\n\n"
            "Bundle Path: ipn:2.1 -> ipn:3.1 -> ipn:4.1\n\n")

        time.sleep(TIME_DISPLAYTEXT)

        # Tracking current position in log
        log2 = testUtils.log_position("2")
        log4 = testUtils.log_position("4")

        print("Node ipn:2.1 Configuration\n")
        testUtils.add_key(2, "bcbkey")
        testUtils.add_event_set(2, "d_bcb_conf", "default confidentiality event set")

        desc = "bcb src tgt:payload"
        filter = testUtils.build_filter("13", "s", "ipn:2.1", tgt=1)
        spec = testUtils.build_spec("bcb-confidentiality", 8, [["key_name", "bcbkey"]])
        es_ref = "d_bcb_conf"
        testUtils.add_policy_rule(2, desc, filter, spec, es_ref)

        print("Node ipn:3.1 Configuration \n")
        testUtils.add_key(3, "bcbkey")
        testUtils.add_event_set(3, "d_bcb_conf", "default confidentiality event set")

        desc = "bcb verifier tgt:payload"
        filter = testUtils.build_filter("14", "v", "ipn:2.1", tgt=1)
        spec = testUtils.build_spec("bcb-confidentiality", 8, [["key_name", "bcbkey"]])
        es_ref = "d_bcb_conf"
        testUtils.add_policy_rule(3, desc, filter, spec, es_ref)

        print("Node ipn:4.1 Configuration\n")
        testUtils.add_key(4, "bcbkey")
        testUtils.add_event_set(4, "d_bcb_conf", "default confidentiality event set")

        desc = "bcb acceptor tgt:payload"
        filter = testUtils.build_filter("15", "a", "ipn:2.1", tgt=1)
        spec = testUtils.build_spec("bcb-confidentiality", 8, [["key_name", "bcbkey"]])
        es_ref = "d_bcb_conf"
        testUtils.add_policy_rule(4, desc, filter, spec, es_ref)

        # Send the bundle from ipn:2.1 -> ipn:3.1 -> ipn:4.1
        testUtils.send_bundle("2.1", "4.1", "2.0", "test_trace6")

        print("\n\nClear Node ipn:2.1 Policy \n")
        testUtils.del_policy_rule(2, "13")
        testUtils.del_event_set(2, "d_bcb_conf")
        testUtils.del_key(2, "bcbkey")

        print("Clear Node ipn:3.1 Policy \n")
        testUtils.del_policy_rule(3, "14")
        testUtils.del_event_set(3, "d_bcb_conf")
        testUtils.del_key(3, "bcbkey")

        print("Clear Node ipn:4.1 Policy \n\n")
        testUtils.del_policy_rule(4, "15")
        testUtils.del_event_set(4, "d_bcb_conf")
        testUtils.del_key(4, "bcbkey")

        te_1 = testUtils.check_test_event(
            'sop_added_at_src', 'ipn:2.1', 'ipn:4.1', 'bcb-confidentiality', '1', None, None, '2', log2)
        te_2 = testUtils.check_test_event(
            'sop_processed', 'ipn:2.1', 'ipn:4.1', 'bcb-confidentiality', '1', te_1['msec'], te_1['count'], '4',
            log4)

        testUtils.check_test_results(4, "test_trace6", False, [te_1, te_2])

    except subprocess.TimeoutExpired as e:
        print(e)
        print("TEST FAILED")
        dotest.g_tests_failed += 1
        os.chdir("..")

    print("###############################################################################")


def test7():
    print("###############################################################################")

    try:
        testUtils.start_test(
            "7", "Brief: Duplicate security policy rules at a security source/acceptor for a BIB.\n\n"
                 "Description: Create two security policy rules at a security source\n"
                 "Requiring a BIB targeting the Payload block. \n"
                 "A single BIB should be added to the bundle, indicating that \n"
                 "rule prioritization behaves as expected.\n"
                 "Configure the Security Acceptor with duplicate rules as well,\n"
                 "expecting to see that the single BIB is processed.\n\n"
                 "Bundle Path: ipn:2.1 -> ipn:3.1\n\n")
        time.sleep(TIME_DISPLAYTEXT)

        # Tracking current position in log
        log2 = testUtils.log_position("2")
        log3 = testUtils.log_position("3")

        print("Node ipn:2.1 Configuration\n")
        testUtils.add_key(2, "key1")
        testUtils.add_event_set(2, "d_integrity", "default integrity event set")

        desc = "bib src tgt:payload"
        filter = testUtils.build_filter("16", "s", "ipn:2.1", tgt=1)
        spec = testUtils.build_spec("bib-integrity", 2, [["key_name", "key1"]])
        es_ref = "d_integrity"
        testUtils.add_policy_rule(2, desc, filter, spec, es_ref)

        desc = "dup bib src tgt:payload"
        filter = testUtils.build_filter("17", "s", "ipn:2.1", tgt=1)
        spec = testUtils.build_spec("bib-integrity", 2, [["key_name", "key1"]])
        es_ref = "d_integrity"
        testUtils.add_policy_rule(2, desc, filter, spec, es_ref)

        print("Node ipn:3.1 Configuration \n")
        testUtils.add_key(3, "key1")
        testUtils.add_event_set(3, "d_integrity", "default integrity event set")

        # Arguments for policy to require the BIB targeting the Primary Block
        # Note: requiring the presence of a single BIB
        desc = "bib acceptor tgt:payload"
        filter = testUtils.build_filter("18", "a", "ipn:2.1", tgt=1)
        spec = testUtils.build_spec("bib-integrity", 2, [["key_name", "key1"]])
        es_ref = "d_integrity"
        testUtils.add_policy_rule(3, desc, filter, spec, es_ref)

        desc = "dup bib acceptor tgt:payload"
        filter = testUtils.build_filter("19", "a", "ipn:2.1", tgt=1)
        spec = testUtils.build_spec("bib-integrity", 2, [["key_name", "key1"]])
        es_ref = "d_integrity"
        testUtils.add_policy_rule(3, desc, filter, spec, es_ref)

        # Sending the bundle from ipn:2.1->ipn:3.1
        testUtils.send_bundle("2.1", "3.1", "2.0", "test_trace7")

        print("\n\nClear Node ipn:2.1 Policy \n")
        testUtils.del_policy_rule(2, "16")
        testUtils.del_policy_rule(2, "17")
        testUtils.del_event_set(2, "d_integrity")
        testUtils.del_key(2, "key1")

        print("Clear Node ipn:3.1 Policy\n")
        testUtils.del_policy_rule(3, "18")
        testUtils.del_policy_rule(3, "19")
        testUtils.del_event_set(3, "d_integrity")
        testUtils.del_key(3, "key1")

        te_1 = testUtils.check_test_event(
            'sop_added_at_src', 'ipn:2.1', 'ipn:3.1', 'bib-integrity', '1', None, None, '2', log2)
        te_2 = testUtils.check_test_event(
            'sop_processed', 'ipn:2.1', 'ipn:3.1', 'bib-integrity', '1', te_1['msec'], te_1['count'], '3', log3)

        testUtils.check_test_results(3, "test_trace7", False, [te_1, te_2])

    except subprocess.TimeoutExpired as e:
        print(e)
        print("TEST FAILED")
        dotest.g_tests_failed += 1
        os.chdir("..")

    print("###############################################################################")


def test8():
    print("###############################################################################")

    try:
        testUtils.start_test(
            "8", "Brief: Duplicate security policy rules at a security source/acceptor for a BCB.\n\n"
                 "Description: Create two security policy rules at a security source and acceptor \n"
                 "requiring a BCB targeting the Payload block. \n"
                 "A single BCB should be added to the bundle, indicating that \n"
                 "rule prioritization behaves as expected.\n\n"
                 "Bundle Path: ipn:2.1 -> ipn:3.1\n\n")

        time.sleep(TIME_DISPLAYTEXT)

        # Tracking current position in log
        log2 = testUtils.log_position("2")
        log3 = testUtils.log_position("3")

        print("Node ipn:2.1 Configuration\n")
        testUtils.add_key(2, "bcbkey")
        testUtils.add_event_set(2, "d_bcb_conf", "default confidentiality event set")

        desc = "bcb src tgt:payload"
        filter = testUtils.build_filter("21", "s", "ipn:2.1", tgt=1)
        spec = testUtils.build_spec("bcb-confidentiality", 8, [["key_name", "bcbkey"]])
        es_ref = "d_bcb_conf"
        testUtils.add_policy_rule(2, desc, filter, spec, es_ref)

        desc = "dup bcb src tgt:payload"
        filter = testUtils.build_filter("22", "s", "ipn:2.1", tgt=1)
        spec = testUtils.build_spec("bcb-confidentiality", 8, [["key_name", "bcbkey"]])
        es_ref = "d_bcb_conf"
        testUtils.add_policy_rule(2, desc, filter, spec, es_ref)

        print("Node ipn:3.1 Configuration \n")
        testUtils.add_key(3, "bcbkey")
        testUtils.add_event_set(3, "d_bcb_conf", "default confidentiality event set")

        desc = "bcb acceptor tgt:payload"
        filter = testUtils.build_filter("23", "a", "ipn:2.1", tgt=1)
        spec = testUtils.build_spec("bcb-confidentiality", 8, [["key_name", "bcbkey"]])
        es_ref = "d_bcb_conf"
        testUtils.add_policy_rule(3, desc, filter, spec, es_ref)

        desc = "dup bcb acceptor tgt:payload"
        filter = testUtils.build_filter("24", "a", "ipn:2.1", tgt=1)
        spec = testUtils.build_spec("bcb-confidentiality", 8, [["key_name", "bcbkey"]])
        es_ref = "d_bcb_conf"
        testUtils.add_policy_rule(3, desc, filter, spec, es_ref)

        # Sending the bundle from ipn:2.1 -> ipn:3.1
        testUtils.send_bundle("2.1", "3.1", "2.0", "test_trace8")

        print("\n\nClear Node ipn:2.1 Policy \n")
        testUtils.del_policy_rule(2, "21")
        testUtils.del_policy_rule(2, "22")
        testUtils.del_event_set(2, "d_bcb_conf")
        testUtils.del_key(2, "bcbkey")

        print("Clear Node ipn:3.1 Policy\n")
        testUtils.del_policy_rule(3, "23")
        testUtils.del_policy_rule(3, "24")
        testUtils.del_event_set(3, "d_bcb_conf")
        testUtils.del_key(3, "bcbkey")

        te_1 = testUtils.check_test_event(
            'sop_added_at_src', 'ipn:2.1', 'ipn:3.1', 'bcb-confidentiality', '1', None, None, '2', log2)
        te_2 = testUtils.check_test_event(
            'sop_processed', 'ipn:2.1', 'ipn:3.1', 'bcb-confidentiality', '1', te_1['msec'], te_1['count'], '3', log3)

        testUtils.check_test_results(3, "test_trace8", False, [te_1, te_2])

    except subprocess.TimeoutExpired as e:
        print(e)
        print("TEST FAILED")
        dotest.g_tests_failed += 1
        os.chdir("..")

    print("###############################################################################")


def test9():
    print("###############################################################################")
    print("Test 9 not yet implemented")
    print("###############################################################################")


def test10():
    print("###############################################################################")
    print("Test 10 not yet implemented")
    print("###############################################################################")


def test11():
    print("###############################################################################")

    try:
        testUtils.start_test(
            "11", "Brief: Configure only a security acceptor for a BIB. \n"
                  "Trigger the sop_missing_at_acceptor event.\n"
                  "Intentional misconfiguration of security policy.\n\n"
                  "Description: Create a security acceptor rule at ipn:3.1 requiring\n"
                  "a BIB on the Primary Block. Omit security source rule at ipn:2.1\n"
                  "and expect to see the sop_missing_at_acceptor test event.\n\n"
                  "Bundle Path: ipn:2.1 -> ipn:3.1\n\n"
                  "NOTE: This test currently fails and should be fixed by the ION 4.2 refactor of BPSec\n\n")
        time.sleep(TIME_DISPLAYTEXT)

        # Tracking current position in log
        log3 = testUtils.log_position("3")

        print("Node 3.1 Configuration \n")
        testUtils.add_key(3, "key1")
        testUtils.add_event_set(3, "d_integrity", "default integrity event set")

        desc = "bib acceptor tgt:primary"
        filter = testUtils.build_filter("25", "a", "ipn:2.1", tgt=0)
        spec = testUtils.build_spec("bib-integrity", 2, [["key_name", "key1"]])
        es_ref = "d_integrity"
        testUtils.add_policy_rule(3, desc, filter, spec, es_ref)

        # Sending the bundle from ipn:2.1 to ipn:3.1
        testUtils.send_bundle("2.1", "3.1", "2.0", "test_trace11")

        print("\n\nClear Node 3 Policy\n")
        testUtils.del_policy_rule(3, "25")
        testUtils.del_event_set(3, "d_integrity")

        te_1 = testUtils.check_test_event(
            'sop_missing_at_acceptor', 'ipn:2.1', 'ipn:3.1', 'bib-integrity', '0', None, None, '3', log3)
        testUtils.check_test_results(3, "test_trace11", False, [te_1])

    except subprocess.TimeoutExpired as e:
        print(e)
        print("TEST FAILED")
        dotest.g_tests_failed += 1
        os.chdir("..")

    print("###############################################################################")


def test12():
    print("###############################################################################")
    try:
        testUtils.start_test(
            "12", "Brief: Configure only a security acceptor for a BCB. \n"
                  "Trigger the sop_missing_at_acceptor event.\n"
                  "Intentional misconfiguration of security policy.\n\n"
                  "Description: Create a security acceptor rule at ipn:3.1 requiring\n"
                  "a BCB on the Payload Block. Omit security source rule at ipn:2.1\n"
                  "and expect to see the sop_missing_at_acceptor test event.\n\n"
                  "Bundle Path: ipn:2.1 -> ipn:3.1\n\n"
                  "NOTE: This test currently fails and should be fixed by the ION 4.2 refactor of BPSec\n\n")
        time.sleep(TIME_DISPLAYTEXT)

        # Tracking current position in log
        log3 = testUtils.log_position("3")

        print("Node 3.1 Configuration \n")
        testUtils.add_key(3, "bcbkey")
        testUtils.add_event_set(3, "d_bcb_conf", "default confidentiality event set")

        desc = "bcb acceptor tgt:payload"
        filter = testUtils.build_filter("26", "a", "ipn:2.1", tgt=1)
        spec = testUtils.build_spec("bcb-confidentiality", 8, [["key_name", "bcbkey"]])
        es_ref = "d_bcb_conf"
        testUtils.add_policy_rule(3, desc, filter, spec, es_ref)

        # Send the bundle from ipn:2.1 -> ipn:3.1
        testUtils.send_bundle("2.1", "3.1", "2.0", "test_trace12")

        print("\n\nClear Node 3 Policy\n")
        testUtils.del_policy_rule(3, "26")
        testUtils.del_event_set(3, "d_bcb_conf")

        te_1 = testUtils.check_test_event(
            'sop_missing_at_acceptor', 'ipn:2.1', 'ipn:3.1', 'bcb-confidentiality', '1', None, None, '3', log3)

        testUtils.check_test_results(3, "test_trace12", False, [te_1])

    except subprocess.TimeoutExpired as e:
        print(e)
        print("TEST FAILED")
        dotest.g_tests_failed += 1
        os.chdir("..")

    print("###############################################################################")


def test13():
    print("###############################################################################")
    print("Test 13 not yet implemented")
    print("###############################################################################")


def test14():
    print("###############################################################################")
    print("Test 14 not yet implemented")
    print("###############################################################################")


def test15():
    print("###############################################################################")
    print("Test 15 not yet implemented")
    print("###############################################################################")


def test16():
    print("###############################################################################")
    print("Test 16 not yet implemented")
    print("###############################################################################")


def test17():
    print("###############################################################################")

    try:
        testUtils.start_test(
            "17", "Brief: Check that a Security Acceptor can identify a misconfigured BIB \n"
                  "due to use of mismatched keys. \n"
                  "Trigger the sop_misconf_at_acceptor event.\n"
                  "Intentional misconfiguration of security policy.\n\n"
                  "Description: Create a Security Source rule at ipn: 2.1 using key1 (key1.hmk)\n"
                  "for a BIB on the Payload block and intentionally misconfigure the Security \n"
                  "Acceptor to use key2 (key2.hmk). Check that the Security Acceptor \n"
                  "acknowledges the BIB misconfiguration.\n\n"
                  "Bundle Path: ipn:2.1 -> ipn:3.1\n\n"
                  "NOTE: This test is not needed for ION Open Source testing as the \n"
                  "NULL_SUITE cipher suites do not generate cryptographic material such \n"
                  "as integrity signatures that would be impacted by a key misconfiguration.\n\n")
        time.sleep(TIME_DISPLAYTEXT)

        '''
        This test is not needed for ION Open Source testing as the NULL_SUITE cipher suites
        do not generate cryptographic material such as integrity signatures that would be
        impacted by a key misconfiguration. To test the ION NASA Baseline (INB) release,
        uncomment these test events.
                
        # Tracking current position in log
        log2 = testUtils.log_position("2")
        log3 = testUtils.log_position("3")

        print("Node 2.1 Configuration \n")
        testUtils.add_key(2, "key1")
        testUtils.add_event_set(2, "d_integrity", "default integrity event set")

        desc = "bib src rule tgt:payload"
        filter = testUtils.make_filter("1", "s", "ipn:2.1", tgt=1)
        spec = testUtils.make_spec("bib-integrity", 2, [["key_name", "key1"]])
        es_ref = "d_integrity"
        testUtils.add_policy(2, desc, filter, spec, es_ref)

        print("Node 3.1 Configuration \n")
        testUtils.add_key(3, "key2")
        testUtils.add_event_set(3, "d_integrity", "default integrity event set")

        desc = "bib acceptor rule tgt:payload"
        filter = testUtils.make_filter("2", "a", "ipn:2.1", tgt=1)
        spec = testUtils.make_spec("bib-integrity", 2, [["key_name", "key2"]])
        es_ref = "d_integrity"
        testUtils.add_policy(3, desc, filter, spec, es_ref)

        # Send the bundle from ipn:2.1 -> ipn:3.1
        testUtils.send_bundle("2.1", "3.1", "2.0", "test_trace17")

        print("\n\nClear Node 2 Policy\n")
        testUtils.del_policy(2, "1")
        testUtils.del_event_set(2, "d_integrity")

        print("Clear Node 3 Policy\n")
        testUtils.del_policy(3, "2")
        testUtils.del_event_set(3, "d_integrity")
        
        te_1 = testUtils.check_test_event(
            'sop_added_at_src', 'ipn:2.1', 'ipn:3.1', 'bib-integrity', '1', None, None, '2', log2)
        te_2 = testUtils.check_test_event(
            'sop_misconf_at_acceptor', 'ipn:2.1', 'ipn:3.1', 'bib-integrity', '1', te_1['msec'], te_1['count'], '3', log3)

        testUtils.check(3, "test_trace17", False, [te_1, te_2])
        '''

    except subprocess.TimeoutExpired as e:
        print(e)
        print("TEST FAILED")
        dotest.g_tests_failed += 1
        os.chdir("..")

    print("###############################################################################")


def test18():
    print("###############################################################################")

    try:
        testUtils.start_test(
            "18", "Brief: Check that a Security Acceptor can identify a misconfigured BCB \n"
                  "due to use of mismatched keys. \n"
                  "Trigger the sop_misconf_at_acceptor event.\n"
                  "Intentional misconfiguration of security policy.\n\n"
                  "Description: Create a Security Source rule at ipn: 2.1 using bcbkey (bcbkey.hmk)\n"
                  "for a BCB on the Payload block and intentionally misconfigure the Security \n"
                  "Acceptor to use bcbkey2 (bcbkey2.hmk). Check that the Security Acceptor \n"
                  "acknowledges the BCB misconfiguration.\n\n"
                  "Bundle Path: ipn:2.1 -> ipn:3.1\n\n"
                  "NOTE: This test is not needed for ION Open Source testing as the NULL_SUITE \n"
                  "cipher suites do not generate cryptographic material that would be impacted \n"
                  "by a key misconfiguration. \n\n")
        time.sleep(TIME_DISPLAYTEXT)

        '''
        This test is not needed for ION Open Source testing as the NULL_SUITE cipher suites
        do not generate cryptographic material that would be
        impacted by a key misconfiguration. To test the ION NASA Baseline (INB) release,
        uncomment these test events.
                
        # Tracking current position in log
        log2 = testUtils.log_position("2")
        log3 = testUtils.log_position("3")

        print("Node 2.1 Configuration \n")
        testUtils.add_key(2, "bcbkey")
        testUtils.add_event_set(2, "d_conf", "default confidentiality event set")

        desc = "bcb src rule tgt:payload"
        filter = testUtils.make_filter("1", "s", "ipn:2.1", tgt=1)
        spec = testUtils.make_spec("bcb-confidentiality", 8, [["key_name", "bcbkey"]])
        es_ref = "d_conf"
        testUtils.add_policy(2, desc, filter, spec, es_ref)

        print("Node 3.1 Configuration \n")
        testUtils.add_key(3, "bcbkey2")
        testUtils.add_event_set(3, "d_conf", "default confidentiality event set")

        desc = "bcb acceptor rule tgt:payload"
        filter = testUtils.make_filter("2", "a", "ipn:2.1", tgt=1)
        spec = testUtils.make_spec("bcb-confidentiality", 8, [["key_name", "bcbkey2"]])
        es_ref = "d_conf"
        testUtils.add_policy(3, desc, filter, spec, es_ref)

        # Send the bundle from ipn:2.1 -> ipn:3.1
        testUtils.send_bundle("2.1", "3.1", "2.0", "test_trace18")

        print("\n\nClear Node 2 Policy\n")
        testUtils.del_policy(2, "1")
        testUtils.del_event_set(2, "d_conf")

        print("Clear Node 3 Policy\n")
        testUtils.del_policy(3, "2")
        testUtils.del_event_set(3, "d_conf")
        
        te_1 = testUtils.check_test_event(
            'sop_added_at_src', 'ipn:2.1', 'ipn:3.1', 'bcb-confidentiality', '1', None, None, '2', log2)
        te_2 = testUtils.check_test_event(
            'sop_misconf_at_acceptor', 'ipn:2.1', 'ipn:3.1', 'bcb-confidentiality', '1', te_1['msec'], te_1['count'],
            '3', log3)

        testUtils.check(3, "test_trace18", False, [te_1, te_2])
        '''

    except subprocess.TimeoutExpired as e:
        print(e)
        print("TEST FAILED")
        dotest.g_tests_failed += 1
        os.chdir("..")

    print("###############################################################################")


def test19():
    print("###############################################################################")

    try:
        testUtils.start_test(
            "19", "Brief: Check that a Security Verifier can identify a misconfigured BIB \n"
                  "due to use of mismatched keys. \n"
                  "Trigger the sop_misconf_at_verifier event.\n"
                  "Intentional misconfiguration of security policy.\n\n"
                  "Description: Create a Security Source rule at ipn: 2.1 using key1 (key1.hmk)\n"
                  "for a BIB on the Payload block and intentionally misconfigure the Security \n"
                  "Verifier to use key2 (key2.hmk). Check that the Security Verifier \n"
                  "acknowledges the BIB misconfiguration.\n\n"
                  "Bundle Path: ipn:2.1 -> ipn:3.1 -> ipn:4.1 \n\n"
                  "NOTE: This test is not needed for ION Open Source testing as the \n"
                  "NULL_SUITE cipher suites do not generate cryptographic material such \n"
                  "as integrity signatures that would be impacted by a key misconfiguration.\n\n")
        time.sleep(TIME_DISPLAYTEXT)

        '''
        This test is not needed for ION Open Source testing as the NULL_SUITE cipher suites
        do not generate cryptographic material such as integrity signatures that would be
        impacted by a key misconfiguration. To test the ION NASA Baseline (INB) release,
        uncomment these test events.
        
        # Tracking current position in log
        log2 = testUtils.log_position("2")
        log3 = testUtils.log_position("3")
        log4 = testUtils.log_position("4")

        print("Node 2.1 Configuration \n")
        testUtils.add_key(2, "key1")
        testUtils.add_event_set(2, "d_integrity", "default integrity event set")

        desc = "bib src rule tgt:payload"
        filter = testUtils.make_filter("1", "s", "ipn:2.1", tgt=1)
        spec = testUtils.make_spec("bib-integrity", 2, [["key_name", "key1"]])
        es_ref = "d_integrity"
        testUtils.add_policy(2, desc, filter, spec, es_ref)

        print("Node 3.1 Configuration \n")
        testUtils.add_key(3, "key2")
        testUtils.add_event_set(3, "d_integrity", "default integrity event set")

        desc = "bib verifier rule tgt:payload"
        filter = testUtils.make_filter("2", "v", "ipn:2.1", tgt=1)
        spec = testUtils.make_spec("bib-integrity", 2, [["key_name", "key2"]])
        es_ref = "d_integrity"
        testUtils.add_policy(3, desc, filter, spec, es_ref)

        # Send the bundle from ipn:2.1 -> ipn:3.1 -> ipn:4.1
        testUtils.send_bundle("2.1", "4.1", "2.0", "test_trace19")

        print("\n\nClear Node 2 Policy\n")
        testUtils.del_policy(2, "1")
        testUtils.del_event_set(2, "d_integrity")

        print("Clear Node 3 Policy\n")
        testUtils.del_policy(3, "2")
        testUtils.del_event_set(3, "d_integrity")

        te_1 = testUtils.check_test_event(
            'sop_added_at_src', 'ipn:2.1', 'ipn:4.1', 'bib-integrity', '1', None, None, '2', log2)
        te_2 = testUtils.check_test_event(
            'sop_misconf_at_verifier', 'ipn:2.1', 'ipn:4.1', 'bib-integrity', '1', te_1['msec'], te_1['count'], 
            '3', log3)

        testUtils.check(4, "test_trace19", False, [te_1, te_2])
        '''

    except subprocess.TimeoutExpired as e:
        print(e)
        print("TEST FAILED")
        dotest.g_tests_failed += 1
        os.chdir("..")

    print("###############################################################################")


def test20():
    print("###############################################################################")
    try:
        testUtils.start_test(
            "20", "Brief: Check that a Security Verifier can identify a misconfigured BCB \n"
                  "due to use of mismatched keys. \n"
                  "Trigger the sop_misconf_at_verifier event.\n"
                  "Intentional misconfiguration of security policy.\n\n"
                  "Description: Create a Security Source rule at ipn: 2.1 using bcbkey (bcbkey.hmk)\n"
                  "for a BCB on the Payload block and intentionally misconfigure the Security \n"
                  "Verifier to use bcbkey2 (bcbkey2.hmk). Check that the Security Verifier \n"
                  "acknowledges the BCB misconfiguration.\n\n"
                  "Bundle Path: ipn:2.1 -> ipn:3.1 -> ipn:4.1\n\n"
                  "NOTE: This test is not needed for ION Open Source testing as the NULL_SUITE \n"
                  "cipher suites do not generate cryptographic material that would be impacted \n"
                  "by a key misconfiguration. \n\n")
        time.sleep(TIME_DISPLAYTEXT)

        '''
        This test is not needed for ION Open Source testing as the NULL_SUITE cipher suites
        do not generate cryptographic material that would be
        impacted by a key misconfiguration. To test the ION NASA Baseline (INB) release,
        uncomment these test events.
                
        # Tracking current position in log
        log2 = testUtils.log_position("2")
        log3 = testUtils.log_position("3")
        log4 = testUtils.log_position("4")

        print("Node 2.1 Configuration \n")
        testUtils.add_key(2, "bcbkey")
        testUtils.add_event_set(2, "d_conf", "default confidentiality event set")

        desc = "bcb src rule tgt:payload"
        filter = testUtils.make_filter("1", "s", "ipn:2.1", tgt=1)
        spec = testUtils.make_spec("bcb-confidentiality", 8, [["key_name", "bcbkey"]])
        es_ref = "d_conf"
        testUtils.add_policy(2, desc, filter, spec, es_ref)

        print("Node 3.1 Configuration \n")
        testUtils.add_key(3, "bcbkey2")
        testUtils.add_event_set(3, "d_conf", "default confidentiality event set")

        desc = "bcb verifier rule tgt:payload"
        filter = testUtils.make_filter("2", "v", "ipn:2.1", tgt=1)
        spec = testUtils.make_spec("bcb-confidentiality", 8, [["key_name", "bcbkey2"]])
        es_ref = "d_conf"
        testUtils.add_policy(3, desc, filter, spec, es_ref)

        # Send the bundle from ipn:2.1 -> ipn:3.1 -> ipn:4.1
        testUtils.send_bundle("2.1", "4.1", "2.0", "test_trace20")

        print("\n\nClear Node 2 Policy\n")
        testUtils.del_policy(2, "1")
        testUtils.del_event_set(2, "d_conf")

        print("Clear Node 3 Policy\n")
        testUtils.del_policy(3, "2")
        testUtils.del_event_set(3, "d_conf")

        te_1 = testUtils.check_test_event(
            'sop_added_at_src', 'ipn:2.1', 'ipn:4.1', 'bcb-confidentiality', '1', None, None, '2', log2)
        te_2 = testUtils.check_test_event(
            'sop_misconf_at_verifier', 'ipn:2.1', 'ipn:4.1', 'bcb-confidentiality', '1', te_1['msec'], te_1['count'],
            '3', log3)

        testUtils.check(4, "test_trace20", False, [te_1, te_2])
        '''

    except subprocess.TimeoutExpired as e:
        print(e)
        print("TEST FAILED")
        dotest.g_tests_failed += 1
        os.chdir("..")

    print("###############################################################################")


def test21():
    print("###############################################################################")
    print("Test 21 not yet implemented")
    print("###############################################################################")


def test22():
    print("###############################################################################")
    try:
        testUtils.start_test(
            "22", "Brief: Catch security policy misconfiguration. Attempt to use a \n"
                  "confidentiality security context when requiring a bib-integrity service.\n\n"
                  "Description: Create a security policy rule for a Security Source at ipn:2.1\n"
                  "for a BIB on the Payload block using security context BIB-HMAC-SHA2. Create \n"
                  "a misconfigured security policy rule for the Security Acceptor at ipn:3.1 \n"
                  "using the wrong security context BCB-AES-GCM.\n\n"
                  "Bundle Path: ipn:2.1 -> ipn:3.1\n\n")
        time.sleep(TIME_DISPLAYTEXT)

        # Tracking current position in log
        log2 = testUtils.log_position("2")
        log3 = testUtils.log_position("3")

        print("Node 2.1 Configuration\n")
        testUtils.add_key(2, "key1")
        testUtils.add_event_set(2, "d_integrity", "default integrity event set")

        desc = "bib src tgt:payload"
        filter = testUtils.build_filter("1", "s", "ipn:2.1", tgt=1)
        spec = testUtils.build_spec("bib-integrity", 2, [["key_name", "key1"]])
        es_ref = "d_integrity"
        testUtils.add_policy_rule(2, desc, filter, spec, es_ref)

        print("Node 3.1 Configuration \n")
        testUtils.add_key(3, "key1")
        testUtils.add_event_set(3, "d_integrity", "default integrity event set")

        desc = "bib acceptor tgt:payload"
        filter = testUtils.build_filter("2", "a", "ipn:2.1", tgt=1)
        spec = testUtils.build_spec("bib-integrity", 8, [["key_name", "key1"]])
        es_ref = "d_integrity"
        testUtils.add_policy_rule(3, desc, filter, spec, es_ref)

        # Send the bundle from ipn:2.1 -> ipn:3.1
        testUtils.send_bundle("2.1", "3.1", "2.0", "test_trace22")

        print("\n\nClear Node 2 Policy \n")
        testUtils.del_policy_rule(2, "1")
        testUtils.del_event_set(2, "d_integrity")

        print("Clear Node 3 Policy\n")
        testUtils.del_policy_rule(3, "2")
        testUtils.del_event_set(3, "d_integrity")

        te_1 = testUtils.check_test_event(
            'sop_added_at_src', 'ipn:2.1', 'ipn:3.1', 'bib-integrity', '1', None, None, '2', log2)
        te_2 = testUtils.check_test_event(
            'sop_misconf_at_acceptor', 'ipn:2.1', 'ipn:3.1', 'bib-integrity', '1', te_1['msec'], te_1['count'],
            '3', log3)

        testUtils.check_test_results(3, "test_trace22", False, [te_1, te_2])

    except subprocess.TimeoutExpired as e:
        print(e)
        print("TEST FAILED")
        dotest.g_tests_failed += 1
        os.chdir("..")

    print("###############################################################################")


def test23():
    print("###############################################################################")
    try:
        testUtils.start_test(
            "23", "Brief: Catch security policy misconfiguration. Attempt to use an\n"
                  "integrity security context when requiring a bcb-confidentiality service.\n\n"
                  "Description: Create a security policy rule for a Security Source at ipn:2.1\n"
                  "for a BCB on the Payload block using security context BCB-AES-GCM. Create \n"
                  "a misconfigured security policy rule for the Security Acceptor at ipn:3.1 \n"
                  "using the wrong security context BIB-HMAC-SHA2.\n\n"
                  "Bundle Path: ipn:2.1 -> ipn:3.1\n\n")
        time.sleep(TIME_DISPLAYTEXT)

        # Tracking current position in log
        log2 = testUtils.log_position("2")
        log3 = testUtils.log_position("3")

        print("Node 2.1 Configuration\n")
        testUtils.add_key(2, "bcbkey")
        testUtils.add_event_set(2, "d_bcb_conf", "default confidentiality event set")

        desc = "bcb src tgt:payload"
        filter = testUtils.build_filter("4", "s", "ipn:2.1", tgt=1)
        spec = testUtils.build_spec("bcb-confidentiality", 8, [["key_name", "bcbkey"]])
        es_ref = "d_bcb_conf"
        testUtils.add_policy_rule(2, desc, filter, spec, es_ref)

        print("Node 3.1 Configuration \n")
        testUtils.add_key(3, "bcbkey")
        testUtils.add_event_set(3, "d_bcb_conf", "default confidentiality event set")

        desc = "bcb acceptor tgt:payload"
        filter = testUtils.build_filter("5", "a", "ipn:2.1", tgt=1)
        spec = testUtils.build_spec("bcb-confidentiality", 2, [["key_name", "bcbkey"]])
        es_ref = "d_bcb_conf"
        testUtils.add_policy_rule(3, desc, filter, spec, es_ref)

        # Send the bundle from ipn:2.1 -> ipn:3.1
        testUtils.send_bundle("2.1", "3.1", "2.0", "test_trace23")

        print("\n\nClear Node 2 Policy \n")
        testUtils.del_policy_rule(2, "4")
        testUtils.del_event_set(2, "d_bcb_conf")

        print("Clear Node 3 Policy\n")
        testUtils.del_policy_rule(3, "5")
        testUtils.del_event_set(3, "d_bcb_conf")

        te_1 = testUtils.check_test_event(
            'sop_added_at_src', 'ipn:2.1', 'ipn:3.1', 'bcb-confidentiality', '1', None, None, '2', log2)
        te_2 = testUtils.check_test_event(
            'sop_misconf_at_acceptor', 'ipn:2.1', 'ipn:3.1', 'bcb-confidentiality', '1', te_1['msec'],
            te_1['count'], '3', log3)

        testUtils.check_test_results(3, "test_trace23", False, [te_1, te_2])

    except subprocess.TimeoutExpired as e:
        print(e)
        print("TEST FAILED")
        dotest.g_tests_failed += 1
        os.chdir("..")

    print("###############################################################################")


def test24():
    print("###############################################################################")

    try:
        testUtils.start_test(
            "24", "Brief: Misconfigure security policy to require a BCB targeting another BCB. \n"
                  "Intentional misconfiguration of security policy.\n\n"
                  "Description: Create a security source rule at ipn:2.1 requiring\n"
                  "a BCB on the Payload Block. Add a second security source rule at\n"
                  "the same node requiring a BCB targeting the first one created.\n"
                  "Expect to see a BCB on the Payload Block only, as a BCB targeting\n"
                  "another BCB is prohibited by BPSec.\n\nn"
                  "Bundle Path: ipn:2.1 -> ipn:3.1\n\n")

        time.sleep(TIME_DISPLAYTEXT)

        # Tracking current position in log
        log2 = testUtils.log_position("2")
        log3 = testUtils.log_position("3")

        print("Node ipn:2.1 Configuration\n")
        testUtils.add_key(2, "bcbkey")
        testUtils.add_event_set(2, "d_bcb_conf", "default confidentiality event set")

        desc = "bcb src rule tgt:payload"
        filter = testUtils.build_filter("21", "s", "ipn:2.1", tgt=1)
        spec = testUtils.build_spec("bcb-confidentiality", 8, [["key_name", "bcbkey"]])
        es_ref = "d_bcb_conf"
        testUtils.add_policy_rule(2, desc, filter, spec, es_ref)

        desc = "bcb src rule tgt:bcb"
        filter = testUtils.build_filter("22", "s", "ipn:2.1", tgt=12)
        spec = testUtils.build_spec("bcb-confidentiality", 8, [["key_name", "bcbkey"]])
        es_ref = "d_bcb_conf2"
        testUtils.add_policy_rule(2, desc, filter, spec, es_ref)

        print("Node ipn:3.1 Configuration \n")
        testUtils.add_key(3, "bcbkey")
        testUtils.add_event_set(3, "d_bcb_conf", "default confidentiality event set")

        desc = "bcb acceptor rule tgt:bcb"
        filter = testUtils.build_filter("24", "a", "ipn:2.1", tgt=12)
        spec = testUtils.build_spec("bcb-confidentiality", 8, [["key_name", "bcbkey"]])
        es_ref = "d_bcb_conf"
        testUtils.add_policy_rule(3, desc, filter, spec, es_ref)

        desc = "bcb acceptor rule tgt:payload"
        filter = testUtils.build_filter("23", "a", "ipn:2.1", tgt=1)
        spec = testUtils.build_spec("bcb-confidentiality", 8, [["key_name", "bcbkey"]])
        es_ref = "d_bcb_conf"
        testUtils.add_policy_rule(3, desc, filter, spec, es_ref)

        # Sending the bundle from ipn:2.1 -> ipn:3.1
        testUtils.send_bundle("2.1", "3.1", "2.0", "test_trace24")

        print("\n\nClear Node ipn:2.1 Policy \n")
        testUtils.del_policy_rule(2, "21")
        testUtils.del_policy_rule(2, "22")
        testUtils.del_event_set(2, "d_bcb_conf")
        testUtils.del_key(2, "bcbkey")

        print("Clear Node ipn:3.1 Policy\n")
        testUtils.del_policy_rule(3, "23")
        testUtils.del_policy_rule(3, "24")
        testUtils.del_event_set(3, "d_bcb_conf")
        testUtils.del_key(3, "bcbkey")

        te_1 = testUtils.check_test_event(
            'sop_added_at_src', 'ipn:2.1', 'ipn:3.1', 'bcb-confidentiality', '1', None, None, '2', log2)
        te_2 = testUtils.check_test_event(
            'sop_processed', 'ipn:2.1', 'ipn:3.1', 'bcb-confidentiality', '1', te_1['msec'], te_1['count'], '3',
            log3)

        testUtils.check_test_results(3, "test_trace24", False, [te_1, te_2])

    except subprocess.TimeoutExpired as e:
        print(e)
        print("TEST FAILED")
        dotest.g_tests_failed += 1
        os.chdir("..")

    print("###############################################################################")


def test25():
    print("###############################################################################")

    try:
        testUtils.start_test(
            "25", "Brief: BIB and BCB both targeting the Payload Block. \n\n"
                  "Description: Create a security source rule at ipn:2.1 requiring\n"
                  "a BCB on the Payload Block. Add a second security source rule at\n"
                  "the same node requiring a BIB targeting the Payload Block as well.\n"
                  "Add security acceptor rules at ipn:3.1 for both the BIB and BCB.\n\n"
                  "Bundle Path: ipn:2.1 -> ipn:3.1\n\n"
                  "NOTE: This test currently fails, possibly indicating that the ability of a security block\n"
                  "to target another security block has not yet been implemented in ION.\n\n")

        time.sleep(TIME_DISPLAYTEXT)

        # Tracking current position in log
        log2 = testUtils.log_position("2")
        log3 = testUtils.log_position("3")

        print("Node 2.1 Configuration\n")
        testUtils.add_key(2, "bcbkey")
        testUtils.add_event_set(2, "d_bcb_conf", "default confidentiality event set")

        desc = "bcb src tgt:payload"
        filter = testUtils.build_filter("21", "s", "ipn:2.1", tgt=1)
        spec = testUtils.build_spec("bcb-confidentiality", 8, [["key_name", "bcbkey"]])
        es_ref = "d_bcb_conf"
        testUtils.add_policy_rule(2, desc, filter, spec, es_ref)

        testUtils.add_key(2, "key1")
        testUtils.add_event_set(2, "d_integrity", "default integrity event set")

        desc = "bib src tgt:payload"
        filter = testUtils.build_filter("20", "s", "ipn:2.1", tgt=1)
        spec = testUtils.build_spec("bib-integrity", 2, [["key_name", "key1"]])
        es_ref = "d_integrity"
        testUtils.add_policy_rule(2, desc, filter, spec, es_ref)

        print("Node 3.1 Configuration \n")
        testUtils.add_key(3, "bcbkey")
        testUtils.add_event_set(3, "d_bcb_conf", "default confidentiality event set")

        desc = "bcb acceptor tgt:payload"
        filter = testUtils.build_filter("23", "a", "ipn:2.1", tgt=1)
        spec = testUtils.build_spec("bcb-confidentiality", 8, [["key_name", "bcbkey"]])
        es_ref = "d_bcb_conf"
        testUtils.add_policy_rule(3, desc, filter, spec, es_ref)

        testUtils.add_key(3, "key1")
        testUtils.add_event_set(3, "d_integrity", "default integrity event set")

        desc = "bib acceptor tgt:payload"
        filter = testUtils.build_filter("22", "a", "ipn:2.1", tgt=1)
        spec = testUtils.build_spec("bib-integrity", 2, [["key_name", "key1"]])
        es_ref = "d_integrity"
        testUtils.add_policy_rule(3, desc, filter, spec, es_ref)

        # Sending the bundle from ipn:2.1 -> ipn:3.1
        testUtils.send_bundle("2.1", "3.1", "2.0", "test_trace25")

        print("\nClear Node 2 Policy \n")
        testUtils.del_policy_rule(2, "20")
        testUtils.del_policy_rule(2, "21")
        testUtils.del_event_set(2, "d_bcb_conf")
        testUtils.del_event_set(2, "d_integrity")

        print("Clear Node 3 Policy\n")
        testUtils.del_policy_rule(3, "22")
        testUtils.del_policy_rule(3, "23")
        testUtils.del_event_set(3, "d_bcb_conf")
        testUtils.del_event_set(3, "d_integrity")

        te_1 = testUtils.check_test_event(
            'sop_added_at_src', 'ipn:2.1', 'ipn:3.1', 'bcb-confidentiality', '1', None, None, '2', log2)
        te_2 = testUtils.check_test_event(
            'sop_added_at_src', 'ipn:2.1', 'ipn:3.1', 'bib-integrity', '1', None, None, '2', log2)
        te_3 = testUtils.check_test_event(
            'sop_processed', 'ipn:2.1', 'ipn:3.1', 'bcb-confidentiality', '1', te_1['msec'], te_1['count'], '3', log3)
        te_4 = testUtils.check_test_event(
            'sop_processed', 'ipn:2.1', 'ipn:3.1', 'bib-integrity', '1', te_1['msec'], te_1['count'], '3', log3)

        testUtils.check_test_results(3, "test_trace25", False, [te_1, te_2, te_3, te_4])

    except subprocess.TimeoutExpired as e:
        print(e)
        print("TEST FAILED")
        dotest.g_tests_failed += 1
        os.chdir("..")

    print("###############################################################################")


def test26():
    try:
        testUtils.start_test(
            "26", "Brief: Create a bundle with a BCB targeting the Payload Block.\n"
                  "Attempt to add a BIB at a waypoint node targeting the Payload Block.\n"
                  "The BIB should not be added. Process the BCB at the security acceptor.\n\n"
                  "Description: Create a security source rule at ipn:2.1 requiring\n"
                  "a BCB on the Payload Block. Create a security source rule at\n"
                  "waypoint node ipn:3.1 requiring a BIB targeting the Payload Block.\n"
                  "Note that BPSec prohibits a BIB with a target block that is already\n"
                  "encrypted by a BCB. The BIB should NOT be added to the bundle. \n"
                  "Add a security acceptor rule at ipn:4.1 for the BCB.\n\n"
                  "Bundle Path: ipn:2.1 -> ipn:3.1 -> ipn:4.1\n\n"
                  "NOTE: This test currently fails and causes unexpected behavior.\n\n")
        time.sleep(TIME_DISPLAYTEXT)

        # Tracking current position in log
        log2 = testUtils.log_position("2")
        log3 = testUtils.log_position("3")
        log4 = testUtils.log_position("4")

        print("Node 2.1 Configuration\n")
        testUtils.add_key(2, "bcbkey")
        testUtils.add_event_set(2, "d_bcb_conf", "default confidentiality event set")

        desc = "bcb src rule tgt:payload"
        filter = testUtils.build_filter("1", "s", "ipn:2.1", tgt=1)
        spec = testUtils.build_spec("bcb-confidentiality", 8, [["key_name", "bcbkey"]])
        es_ref = "d_bcb_conf"
        testUtils.add_policy_rule(2, desc, filter, spec, es_ref)

        print("Node 3.1 Configuration \n")
        testUtils.add_key(3, "key1")
        testUtils.add_event_set(3, "d_integrity", "default integrity event set")

        desc = "bib src rule tgt:payload"
        filter = testUtils.build_filter("2", "s", "ipn:2.1", tgt=1)
        spec = testUtils.build_spec("bib-integrity", 2, [["key_name", "key1"]])
        es_ref = "d_integrity"
        testUtils.add_policy_rule(3, desc, filter, spec, es_ref)

        print("Node 4.1 Configuration \n")
        testUtils.add_key(4, "bcbkey")
        testUtils.add_event_set(3, "d_bcb_conf", "default confidentiality event set")

        desc = "bcb acceptor rule tgt:payload"
        filter = testUtils.build_filter("3", "a", "ipn:2.1", tgt=1)
        spec = testUtils.build_spec("bcb-confidentiality", 8, [["key_name", "bcbkey"]])
        es_ref = "d_bcb_conf"
        testUtils.add_policy_rule(4, desc, filter, spec, es_ref)

        testUtils.add_key(4, "key1")
        testUtils.add_event_set(4, "d_integrity", "default integrity event set")

        desc = "bib acceptor rule tgt:payload"
        filter = testUtils.build_filter("4", "s", "ipn:2.1", tgt=1)
        spec = testUtils.build_spec("bib-integrity", 2, [["key_name", "key1"]])
        es_ref = "d_integrity"
        testUtils.add_policy_rule(4, desc, filter, spec, es_ref)

        # Sending the bundle from ipn:2.1 -> ipn:3.1 -> ipn:4.1
        testUtils.send_bundle("2.1", "4.1", "2.0", "test_trace26")

        print("\n\nClear Node 2 Policy \n")
        testUtils.del_policy_rule(2, "1")
        testUtils.del_event_set(2, "d_bcb_conf")

        print("Clear Node 3 Policy\n")
        testUtils.del_policy_rule(3, "2")
        testUtils.del_event_set(3, "d_integrity")

        print("Clear Node 4 Policy \n")
        testUtils.del_policy_rule(4, "3")
        testUtils.del_policy_rule(4, "4")
        testUtils.del_event_set(4, "d_bcb_conf")
        testUtils.del_event_set(4, "d_integrity")

        te_1 = testUtils.check_test_event(
            'sop_added_at_src', 'ipn:2.1', 'ipn:4.1', 'bcb-confidentiality', '1', None, None, '2', log2)
        te_2 = testUtils.check_test_event(
            'sop_processed', 'ipn:2.1', 'ipn:4.1', 'bcb-confidentiality', '1', te_1['msec'], te_1['count'], '4', log4)

        testUtils.check_test_results(4, "test_trace26", False, [te_1, te_2])

    except subprocess.TimeoutExpired as e:
        print(e)
        print("TEST FAILED")
        dotest.g_tests_failed += 1
        os.chdir("..")

    print("##############################################################################")


def test27():
    try:
        testUtils.start_test(
            "27", "Brief: Misconfigure security policy to require a BIB targeting another BIB. \n"
                  "Intentional misconfiguration of security policy.\n\n"
                  "Description: Create a security source rule at ipn:2.1 requiring\n"
                  "a BIB on the Payload Block. Add a second security source rule at\n"
                  "the same node requiring a BIB targeting the first one created.\n"
                  "Expect to see a BIB on the Payload Block only, as a BIB targeting\n"
                  "another BIB is prohibited by BPSec.\n"
                  "Bundle Path: ipn:2.1 -> ipn:3.1\n"
                  "NOTE: This test currently fails and causes unexpected behavior.\n")
        time.sleep(TIME_DISPLAYTEXT)

        # Tracking current position in log
        log2 = testUtils.log_position("2")
        log3 = testUtils.log_position("3")

        print("Node 2.1 Configuration\n")
        testUtils.add_key(2, "key1")
        testUtils.add_event_set(2, "d_integrity", "default integrity event set")

        desc = "bib src rule tgt:payload"
        filter = testUtils.build_filter("50", "s", "ipn:2.1", tgt=1)
        spec = testUtils.build_spec("bib-integrity", 2, [["key_name", "key1"]])
        es_ref = "d_integrity"
        testUtils.add_policy_rule(2, desc, filter, spec, es_ref)

        desc = "bib src rule tgt:bib"
        filter = testUtils.build_filter("51", "s", "ipn:2.1", tgt=11)
        spec = testUtils.build_spec("bib-integrity", 2, [["key_name", "key1"]])
        es_ref = "d_integrity"
        testUtils.add_policy_rule(2, desc, filter, spec, es_ref)

        print("Node 3.1 Configuration \n")
        testUtils.add_key(3, "key1")
        testUtils.add_event_set(3, "d_integrity", "default integrity event set")

        desc = "Integrity acceptor rule"
        filter = testUtils.build_filter("52", "a", "ipn:2.1", tgt=1)
        spec = testUtils.build_spec("bib-integrity", 2, [["key_name", "key1"]])
        es_ref = "d_integrity"
        testUtils.add_policy_rule(3, desc, filter, spec, es_ref)

        # Sending the bundle from ipn:2.1 -> ipn:3.1
        testUtils.send_bundle("2.1", "3.1", "2.0", "test_trace27")

        print("Clear Node 2 Policy \n")
        testUtils.del_policy_rule(2, "50")
        testUtils.del_policy_rule(2, "51")
        testUtils.del_event_set(2, "d_integrity")

        print("Clear Node 3 Policy\n")
        testUtils.del_policy_rule(3, "52")
        testUtils.del_event_set(3, "d_integrity")

        te_1 = testUtils.check_test_event(
            'sop_added_at_src', 'ipn:2.1', 'ipn:3.1', 'bib-integrity', '0', None, None, '2', log2)
        te_2 = testUtils.check_test_event(
            'sop_processed', 'ipn:2.1', 'ipn:3.1', 'bib-integrity', '0', te_1['msec'], te_1['count'], '3', log3)

        testUtils.check_test_results(3, "test_trace7", True, [te_1, te_2])

    except subprocess.TimeoutExpired as e:
        print(e)
        print("TEST FAILED")
        dotest.g_tests_failed += 1
        os.chdir("..")

    print("###############################################################################")


def test28():
    print("###############################################################################")

    try:
        testUtils.start_test(
            "28", "Brief: Check that a BCB is not permitted to target a BIB \n"
                  "it does not share a security target with.\n\n"
                  "Description: Create a bundle with a BIB targeting the Payload Block.\n"
                  "Designate a waypoint node as the security source for a BCB whose target\n"
                  "is that BIB.\n\n"
                  "Bundle Path: ipn:2.1 -> ipn:3.1 -> ipn:4.1\n\n")
        time.sleep(TIME_DISPLAYTEXT)

        # Tracking current position in log
        log2 = testUtils.log_position("2")
        log4 = testUtils.log_position("4")

        print("Node 2.1 Configuration\n")
        testUtils.add_key(2, "key1")
        testUtils.add_event_set(2, "d_integrity", "default integrity event set")

        desc = "bib src rule tgt:payload"
        filter = testUtils.build_filter("20", "s", "ipn:2.1", tgt=1)
        spec = testUtils.build_spec("bib-integrity", 2, [["key_name", "key1"]])
        es_ref = "d_integrity"
        testUtils.add_policy_rule(2, desc, filter, spec, es_ref)

        print("Node 3.1 Configuration \n")
        testUtils.add_key(3, "bcbkey")
        testUtils.add_event_set(3, "d_bcb_conf", "default confidentiality event set")

        desc = "bcb src rule tgt:bib"
        filter = testUtils.build_filter("21", "s", "ipn:2.1", tgt=11)
        spec = testUtils.build_spec("bcb-confidentiality", 8, [["key_name", "bcbkey"]])
        es_ref = "d_bcb_conf"
        testUtils.add_policy_rule(3, desc, filter, spec, es_ref)

        print("Node 4.1 Configuration \n")
        testUtils.add_key(4, "key1")
        testUtils.add_event_set(4, "d_integrity", "default integrity event set")

        desc = "bib acceptor rule tgt:payload"
        filter = testUtils.build_filter("22", "a", "ipn:2.1", tgt=1)
        spec = testUtils.build_spec("bib-integrity", 2, [["key_name", "key1"]])
        es_ref = "d_integrity"
        testUtils.add_policy_rule(3, desc, filter, spec, es_ref)

        # Sending the bundle from ipn:2.1 -> ipn:3.1 -> ipn:4.1
        testUtils.send_bundle("2.1", "4.1", "2.0", "test_trace28")

        print("\n\nClear Node 2 Policy \n")
        testUtils.del_policy_rule(2, "20")
        testUtils.del_event_set(2, "d_integrity")

        print("Clear Node 3 Policy\n")
        testUtils.del_policy_rule(3, "21")
        testUtils.del_event_set(3, "d_bcb_conf")

        print("Clear Node 4 Policy\n")
        testUtils.del_policy_rule(4, "22")
        testUtils.del_event_set(4, "d_integrity")

        te_1 = testUtils.check_test_event(
            'sop_added_at_src', 'ipn:2.1', 'ipn:4.1', 'bib-integrity', '1', None, None, '2', log2)
        te_2 = testUtils.check_test_event(
            'sop_processed', 'ipn:2.1', 'ipn:4.1', 'bib-integrity', '1', te_1['msec'], te_1['count'], '4', log4)

        testUtils.check_test_results(4, "test_trace28", False, [te_1, te_2])

    except subprocess.TimeoutExpired as e:
        print(e)
        print("TEST FAILED")
        dotest.g_tests_failed += 1
        os.chdir("..")

    print("##############################################################################")


def test29():
    print("##############################################################################")

    try:
        testUtils.start_test(
            "29", "Brief: Check that a BCB can encrypt a BIB if the two security blocks \n"
                  "share a security target. \n\n"
                  "Description: Configure node ipn:2.1 to be a security source for both a \n"
                  "BIB and BCB. Both of these security blocks share the same target: the Payload Block.\n"
                  "Node 3.1 serves as the security acceptor for both security operations.\n\n"
                  "Bundle Path: ipn:2.1 -> ipn:3.1 \n\n"
                  "NOTE: This test currently fails.\n\n")
        time.sleep(TIME_DISPLAYTEXT)

        # Tracking current position in log
        log2 = testUtils.log_position("2")
        log3 = testUtils.log_position("3")

        print("Node 2.1 Configuration\n")
        testUtils.add_key(2, "key1")
        testUtils.add_event_set(2, "d_integrity", "default integrity event set")

        desc = "bib src tgt:payload"
        filter = testUtils.build_filter("20", "s", "ipn:2.1", tgt=1)
        spec = testUtils.build_spec("bib-integrity", 2, [["key_name", "key1"]])
        es_ref = "d_integrity"
        testUtils.add_policy_rule(2, desc, filter, spec, es_ref)

        testUtils.add_key(2, "bcbkey")
        testUtils.add_event_set(2, "d_bcb_conf", "default confidentiality event set")

        desc = "bcb src tgt:payload"
        filter = testUtils.build_filter("21", "s", "ipn:2.1", tgt=1)
        spec = testUtils.build_spec("bcb-confidentiality", 8, [["key_name", "bcbkey"]])
        es_ref = "d_bcb_conf"
        testUtils.add_policy_rule(2, desc, filter, spec, es_ref)

        print("Node 3.1 Configuration \n")
        testUtils.add_key(3, "bcbkey")
        testUtils.add_event_set(3, "d_bcb_conf", "default confidentiality event set")

        desc = "bcb acceptor tgt:payload"
        filter = testUtils.build_filter("22", "a", "ipn:2.1", tgt=1)
        spec = testUtils.build_spec("bcb-confidentiality", 8, [["key_name", "bcbkey"]])
        es_ref = "d_bcb_conf"
        testUtils.add_policy_rule(3, desc, filter, spec, es_ref)

        testUtils.add_key(3, "key1")
        testUtils.add_event_set(3, "d_integrity", "default integrity event set")

        desc = "bib acceptor tgt:payload"
        filter = testUtils.build_filter("23", "a", "ipn:2.1", tgt=1)
        spec = testUtils.build_spec("bib-integrity", 2, [["key_name", "key1"]])
        es_ref = "d_integrity"
        testUtils.add_policy_rule(3, desc, filter, spec, es_ref)

        # Sending the bundle from ipn:2.1 -> ipn:3.1
        testUtils.send_bundle("2.1", "3.1", "2.0", "test_trace29")

        print("\n\nClear Node 2 Policy \n")
        testUtils.del_policy_rule(2, "20")
        testUtils.del_policy_rule(2, "21")
        testUtils.del_event_set(2, "d_integrity")
        testUtils.del_event_set(2, "d_bcb_conf")

        print("Clear Node 3 Policy\n")
        testUtils.del_policy_rule(3, "22")
        testUtils.del_policy_rule(3, "23")
        testUtils.del_event_set(3, "d_bcb_conf")
        testUtils.del_event_set(3, "d_integrity")

        te_1 = testUtils.check_test_event(
            'sop_added_at_src', 'ipn:2.1', 'ipn:3.1', 'bib-integrity', '1', None, None, '2', log2)
        te_2 = testUtils.check_test_event(
            'sop_added_at_src', 'ipn:2.1', 'ipn:3.1', 'bcb-confidentiality', '1', te_1['msec'], te_1['count'], '2',
            log2)
        te_3 = testUtils.check_test_event(
            'sop_processed', 'ipn:2.1', 'ipn:3.1', 'bcb-confidentiality', '1', te_1['msec'], te_1['count'], '3', log3)
        te_4 = testUtils.check_test_event(
            'sop_processed', 'ipn:2.1', 'ipn:3.1', 'bib-integrity', '1', te_1['msec'], te_1['count'], '3', log3)

        testUtils.check_test_results(3, "test_trace29", False, [te_1, te_2, te_3, te_4])

    except subprocess.TimeoutExpired as e:
        print(e)
        print("TEST FAILED")
        dotest.g_tests_failed += 1
        os.chdir("..")

    print("##############################################################################")


def test30():
    print("###############################################################################")
    print("Test 30 not yet implemented")
    print("###############################################################################")


def test31():
    print("###############################################################################")
    try:
        testUtils.start_test(
            "31", "Brief: Check that a BCB can be added to the bundle at a waypoint node \n"
                  "if it shares some of an existing BIB’s targets.\n\n"
                  "Description: Configure node ipn:2.1 to be the Security Source for a BIB \n"
                  "targeting the Primary Block and the Payload Block.\n\n"
                  "Configure waypoint node ipn:3.1 to be the Security Source for a BCB \n"
                  "targeting the Payload Block.\n\n"
                  "Node ipn:4.1 is configured to be the Security Acceptor for all three \n"
                  "security operations.\n\n"
                  "Bundle Path: ipn:2.1 -> ipn:3.1 -> ipn:4.1\n\n")
        time.sleep(TIME_DISPLAYTEXT)

        # Tracking current position in log
        log2 = testUtils.log_position("2")
        log3 = testUtils.log_position("3")
        log4 = testUtils.log_position("4")

        print("Node 2.1 Configuration\n")
        testUtils.add_key(2, "key1")
        testUtils.add_event_set(2, "d_integrity", "default integrity event set")

        desc = "bib src tgt:primary"
        filter = testUtils.build_filter("1", "s", "ipn:2.1", tgt=0)
        spec = testUtils.build_spec("bib-integrity", 2, [["key_name", "key1"]])
        es_ref = "d_integrity"
        testUtils.add_policy_rule(2, desc, filter, spec, es_ref)

        desc = "bib src tgt:payload"
        filter = testUtils.build_filter("2", "s", "ipn:2.1", tgt=1)
        spec = testUtils.build_spec("bib-integrity", 2, [["key_name", "key1"]])
        es_ref = "d_integrity"
        testUtils.add_policy_rule(2, desc, filter, spec, es_ref)

        print("Node 3.1 Configuration \n")
        testUtils.add_key(3, "bcbkey")
        testUtils.add_event_set(3, "d_conf", "default confidentiality event set")

        desc = "bcb src tgt:payload"
        filter = testUtils.build_filter("3", "a", "ipn:2.1", tgt=1)
        spec = testUtils.build_spec("bcb-confidentiality", 8, [["key_name", "bcbkey"]])
        es_ref = "d_conf"
        testUtils.add_policy_rule(3, desc, filter, spec, es_ref)

        print("Node 4.1 Configuration \n")
        testUtils.add_key(4, "key1")
        testUtils.add_event_set(4, "d_integrity", "default integrity event set")

        desc = "bib acceptor tgt:primary"
        filter = testUtils.build_filter("4", "a", "ipn:2.1", tgt=0)
        spec = testUtils.build_spec("bib-integrity", 2, [["key_name", "key1"]])
        es_ref = "d_integrity"
        testUtils.add_policy_rule(4, desc, filter, spec, es_ref)

        desc = "bib acceptor tgt:payload"
        filter = testUtils.build_filter("5", "a", "ipn:2.1", tgt=1)
        spec = testUtils.build_spec("bib-integrity", 2, [["key_name", "key1"]])
        es_ref = "d_integrity"
        testUtils.add_policy_rule(4, desc, filter, spec, es_ref)

        testUtils.add_key(4, "bcbkey")
        testUtils.add_event_set(4, "d_conf", "default confidentiality event set")

        desc = "bcb src tgt:payload"
        filter = testUtils.build_filter("6", "a", "ipn:2.1", tgt=1)
        spec = testUtils.build_spec("bcb-confidentiality", 8, [["key_name", "bcbkey"]])
        es_ref = "d_conf"
        testUtils.add_policy_rule(4, desc, filter, spec, es_ref)

        # Sending the bundle from ipn:2.1 -> ipn:3.1 -> ipn:4.1
        testUtils.send_bundle("2.1", "4.1", "2.0", "test_trace31")

        print("\n\nClear Node 2 Policy \n")
        testUtils.del_policy_rule(2, "1")
        testUtils.del_policy_rule(2, "2")
        testUtils.del_event_set(2, "d_integrity")

        print("Clear Node 3 Policy\n")
        testUtils.del_policy_rule(3, "3")
        testUtils.del_event_set(3, "d_conf")

        print("\n\nClear Node 4 Policy \n")
        testUtils.del_policy_rule(4, "4")
        testUtils.del_policy_rule(4, "5")
        testUtils.del_policy_rule(4, "6")
        testUtils.del_event_set(4, "d_integrity")
        testUtils.del_event_set(4, "d_conf")

        te_1 = testUtils.check_test_event(
            'sop_added_at_src', 'ipn:2.1', 'ipn:4.1', 'bib-integrity', '0', None, None, '2', log2)
        te_2 = testUtils.check_test_event(
            'sop_added_at_src', 'ipn:2.1', 'ipn:4.1', 'bib-integrity', '1', te_1['msec'], te_1['count'], '2', log2)
        te_3 = testUtils.check_test_event(
            'sop_added_at_src', 'ipn:2.1', 'ipn:4.1', 'bcb-confidentiality', '1', te_1['msec'], te_1['count'], '3', log3)
        te_4 = testUtils.check_test_event(
            'sop_processed', 'ipn:2.1', 'ipn:4.1', 'bib-integrity', '0', te_1['msec'], te_1['count'], '4', log4)
        te_5 = testUtils.check_test_event(
            'sop_processed', 'ipn:2.1', 'ipn:4.1', 'bib-integrity', '1', te_1['msec'], te_1['count'], '4', log4)
        te_6 = testUtils.check_test_event(
            'sop_processed', 'ipn:2.1', 'ipn:4.1', 'bcb-confidentiality', '1', te_1['msec'], te_1['count'], '4', log4)

        testUtils.check_test_results(4, "test_trace31", False, [te_1, te_2, te_3, te_4, te_5, te_6])

    except subprocess.TimeoutExpired as e:
        print(e)
        print("TEST FAILED")
        dotest.g_tests_failed += 1
        os.chdir("..")

    print("###############################################################################")


def test32():
    print("###############################################################################")
    print("Test 32 not yet implemented")
    print("###############################################################################")


def test33():
    print("###############################################################################")

    try:
        testUtils.start_test(
            "33", "Brief: BIB target multiplicity test. Check that a BIB can have multiple targets of "
                  "different block types.\n\n"
                  "Description: Configure node ipn:2.1 to be a security source for a BIB targeting both \n"
                  "the Payload and Primary blocks. Node ipn:3.1 serves as the security acceptor for both \n"
                  "security operations.\n\n"
                  "Bundle Path: ipn:2.1 -> ipn:3.1\n\n")
        time.sleep(TIME_DISPLAYTEXT)

        # Tracking current position in log
        log2 = testUtils.log_position("2")
        log3 = testUtils.log_position("3")

        print("Node 2.1 Configuration\n")
        testUtils.add_key(2, "key1")
        testUtils.add_event_set(2, "d_integrity", "default integrity event set")

        desc = "bib src tgt:primary"
        filter = testUtils.build_filter("16", "s", "ipn:2.1", tgt=0)
        spec = testUtils.build_spec("bib-integrity", 2, [["key_name", "key1"]])
        es_ref = "d_integrity"
        testUtils.add_policy_rule(2, desc, filter, spec, es_ref)

        desc = "bib src tgt:payload"
        filter = testUtils.build_filter("17", "s", "ipn:2.1", tgt=1)
        spec = testUtils.build_spec("bib-integrity", 2, [["key_name", "key1"]])
        es_ref = "d_integrity"
        testUtils.add_policy_rule(2, desc, filter, spec, es_ref)

        print("Node 3.1 Configuration \n")
        testUtils.add_key(3, "key1")
        testUtils.add_event_set(3, "d_integrity", "default integrity event set")

        desc = "bib acceptor tgt:primary"
        filter = testUtils.build_filter("18", "a", "ipn:2.1", tgt=0)
        spec = testUtils.build_spec("bib-integrity", 2, [["key_name", "key1"]])
        es_ref = "d_integrity"
        testUtils.add_policy_rule(3, desc, filter, spec, es_ref)

        desc = "bib acceptor tgt:payload"
        filter = testUtils.build_filter("19", "a", "ipn:2.1", tgt=1)
        spec = testUtils.build_spec("bib-integrity", 2, [["key_name", "key1"]])
        es_ref = "d_integrity"
        testUtils.add_policy_rule(3, desc, filter, spec, es_ref)

        # Sending the bundle from ipn:2.1 -> ipn:3.1
        testUtils.send_bundle("2.1", "3.1", "2.0", "test_trace33")

        print("\n\nClear Node 2 Policy \n")
        testUtils.del_policy_rule(2, "16")
        testUtils.del_policy_rule(2, "17")
        testUtils.del_event_set(2, "d_integrity")

        print("Clear Node 3 Policy\n")
        testUtils.del_policy_rule(3, "18")
        testUtils.del_policy_rule(3, "19")
        testUtils.del_event_set(3, "d_integrity")

        te_1 = testUtils.check_test_event(
            'sop_added_at_src', 'ipn:2.1', 'ipn:3.1', 'bib-integrity', '0', None, None, '2', log2)
        te_2 = testUtils.check_test_event(
            'sop_added_at_src', 'ipn:2.1', 'ipn:3.1', 'bib-integrity', '1', te_1['msec'], te_1['count'], '2', log2)
        te_3 = testUtils.check_test_event(
            'sop_processed', 'ipn:2.1', 'ipn:3.1', 'bib-integrity', '0', te_1['msec'], te_1['count'], '3', log3)
        te_4 = testUtils.check_test_event(
            'sop_processed', 'ipn:2.1', 'ipn:3.1', 'bib-integrity', '1', te_1['msec'], te_1['count'], '3', log3)

        testUtils.check_test_results(3, "test_trace33", False, [te_1, te_2, te_3, te_4])

    except subprocess.TimeoutExpired as e:
        print(e)
        print("TEST FAILED")
        dotest.g_tests_failed += 1
        os.chdir("..")

    print("###############################################################################")


def test34():
    print("###############################################################################")
    print("Test 34 not yet implemented")
    print("###############################################################################")


def test35():
    print("###############################################################################")
    print("Test 31 not yet implemented")
    print("###############################################################################")


def test36():
    print("###############################################################################")
    print("Test 32 not yet implemented")
    print("###############################################################################")


def test37():
    print("###############################################################################")

    try:
        testUtils.start_test(
            "37", "Brief: Multi bundle test.\n\n"
                  "Description: Configure node ipn:2.1  to serve as a security source for\n"
                  "bib-integrity targeting the payload block regardless of bundle\n"
                  "destination. Configure node ipn:3.1 to serve as the security acceptor for "
                  "Bundle 1 and node ipn:4.1 to serve as the security acceptor for Bundle 2.\n"
                  "Ensure that BIB processing events are handled on a per-bundle basis by \n"
                  "the test framework.\n\n"
                  "Bundle 1 Path: ipn:2.1 -> ipn:4.1\n"
                  "Bundle 2 Path: ipn:2.1 -> ipn:3.1\n\n")
        time.sleep(TIME_DISPLAYTEXT)

        # Tracking current position in log
        log2 = testUtils.log_position("2")
        log3 = testUtils.log_position("3")
        log4 = testUtils.log_position("4")

        print("Node ipn:2.1 Configuration\n")
        testUtils.add_key(2, "key1")
        testUtils.add_event_set(2, "d_integrity", "default integrity event set")

        desc = "bib src tgt:payload"
        filter = testUtils.build_filter("10", "s", "ipn:2.1", tgt=1)
        spec = testUtils.build_spec("bib-integrity", 2, [["key_name", "key1"]])
        es_ref = "d_integrity"
        testUtils.add_policy_rule(2, desc, filter, spec, es_ref)

        print("Node ipn:3.1 Configuration \n")
        testUtils.add_key(3, "key1")
        testUtils.add_event_set(3, "d_integrity", "default integrity event set")

        desc = "bib acceptor tgt:payload"
        filter = testUtils.build_filter("11", "a", dest="ipn:3.1", tgt=1)
        spec = testUtils.build_spec("bib-integrity", 2, [["key_name", "key1"]])
        es_ref = "d_integrity"
        testUtils.add_policy_rule(3, desc, filter, spec, es_ref)

        print("Node ipn:4.1 Configuration\n")
        testUtils.add_key(4, "key1")
        testUtils.add_event_set(4, "d_integrity", "default integrity event set")

        desc = "bib acceptor for dest ipn:4.1"
        filter = testUtils.build_filter("12", "a", dest="ipn:4.1", tgt=1)
        spec = testUtils.build_spec("bib-integrity", 2, [["key_name", "key1"]])
        es_ref = "d_integrity"
        testUtils.add_policy_rule(4, desc, filter, spec, es_ref)

        # Send Bundle 1 from ipn:2.1 -> ipn:4.1
        testUtils.send_bundle("2.1", "4.1", "2.0", "test_trace37_1")

        # Send Bundle 2 from ipn:2.1 -> ipn:3.1
        testUtils.send_bundle("2.1", "3.1", "2.0", "test_trace37_2")

        print("\n\nClear Node ipn:2.1 Policy \n")
        testUtils.del_policy_rule(2, "10")
        testUtils.del_event_set(2, "d_integrity")
        testUtils.del_key(2, "key1")

        print("Clear Node ipn:3.1 Policy \n")
        testUtils.del_policy_rule(3, "11")
        testUtils.del_event_set(3, "d_integrity")
        testUtils.del_key(3, "key1")

        print("Clear Node ipn:4.1 Policy \n\n")
        testUtils.del_policy_rule(4, "12")
        testUtils.del_event_set(4, "d_integrity")
        testUtils.del_key(4, "key1")

        # Bundle 1 test events
        te_1 = testUtils.check_test_event(
            'sop_added_at_src', 'ipn:2.1', 'ipn:4.1', 'bib-integrity', '1', None, None, '2', log2)
        te_2 = testUtils.check_test_event(
            'sop_processed', 'ipn:2.1', 'ipn:4.1', 'bib-integrity', '1', te_1['msec'], te_1['count'], '4', log4)

        # Bundle 2 test events
        te_3 = testUtils.check_test_event(
            'sop_added_at_src', 'ipn:2.1', 'ipn:3.1', 'bib-integrity', '1', None, None, '2', log2)
        te_4 = testUtils.check_test_event(
            'sop_processed', 'ipn:2.1', 'ipn:3.1', 'bib-integrity', '1', te_3['msec'], te_3['count'], '3', log3)

        testUtils.check_test_results(4, "test_trace37_1", False, [te_1, te_2])
        testUtils.check_test_results(3, "test_trace37_2", False, [te_3, te_4])

    except subprocess.TimeoutExpired as e:
        print(e)
        print("TEST FAILED")
        dotest.g_tests_failed += 1
        os.chdir("..")

    print("###############################################################################")


def test38():
    print("###############################################################################")

    try:
        testUtils.start_test(
            "38", "Brief: Check that a Security Verifier can identify a missing BIB. \n"
                  "Trigger the sop_missing_at_verifier event.\n"
                  "Intentional misconfiguration of security policy.\n\n"
                  "Description: Create a Security Verifier rule at ipn:3.1 requiring a \n"
                  "BIB on the Primary Block. Omit security source rule at ipn:2.1 and \n"
                  "expect to see the sop_missing_at_verifier test event.\n\n"
                  "Bundle Path: ipn:2.1 -> ipn:3.1 -> ipn:4.1\n\n"
                  "NOTE: This test currently fails and should be fixed by the ION 4.2 refactor of BPSec\n\n")
        time.sleep(TIME_DISPLAYTEXT)

        # Tracking current position in log
        log3 = testUtils.log_position("3")

        print("Node 3.1 Configuration \n")
        testUtils.add_key(3, "key1")
        testUtils.add_event_set(3, "d_integrity", "default integrity event set")

        desc = "bib verifier tgt:primary"
        filter = testUtils.build_filter("1", "v", "ipn:2.1", tgt=0)
        spec = testUtils.build_spec("bib-integrity", 2, [["key_name", "key1"]])
        es_ref = "d_integrity"
        testUtils.add_policy_rule(3, desc, filter, spec, es_ref)

        # Sending the bundle from ipn:2.1 -> ipn:3.1 -> ipn:4.1
        testUtils.send_bundle("2.1", "4.1", "2.0", "test_trace38")

        print("\n\nClear Node 3 Policy\n")
        testUtils.del_policy_rule(3, "1")
        testUtils.del_event_set(3, "d_integrity")

        te_1 = testUtils.check_test_event(
            'sop_missing_at_acceptor', 'ipn:2.1', 'ipn:4.1', 'bib-integrity', '0', None, None, '3', log3)
        testUtils.check_test_results(4, "test_trace38", False, [te_1])

    except subprocess.TimeoutExpired as e:
        print(e)
        print("TEST FAILED")
        dotest.g_tests_failed += 1
        os.chdir("..")

    print("###############################################################################")


def test39():
    print("###############################################################################")

    try:
        testUtils.start_test(
            "39", "Brief: Check that a Security Verifier can identify a missing BCB. \n"
                  "Trigger the sop_missing_at_verifier event.\n"
                  "Intentional misconfiguration of security policy.\n\n"
                  "Description: Create a Security Verifier rule at ipn:3.1 requiring a \n"
                  "BCB on the Payload Block. Omit security source rule at ipn:2.1 and \n"
                  "expect to see the sop_missing_at_verifier test event.\n\n"
                  "Bundle Path: ipn:2.1 -> ipn:3.1 -> ipn:4.1\n\n"
                  "NOTE: This test currently fails and should be fixed by the ION 4.2 refactor of BPSec\n\n")
        time.sleep(TIME_DISPLAYTEXT)

        # Tracking current position in log
        log3 = testUtils.log_position("3")

        print("Node 3.1 Configuration \n")
        testUtils.add_key(3, "bcbkey")
        testUtils.add_event_set(3, "d_conf", "default confidentiality event set")

        desc = "bcb verifier tgt:payload"
        filter = testUtils.build_filter("1", "v", "ipn:2.1", tgt=1)
        spec = testUtils.build_spec("bib-integrity", 8, [["key_name", "bcbkey"]])
        es_ref = "d_conf"
        testUtils.add_policy_rule(3, desc, filter, spec, es_ref)

        # Sending the bundle from ipn:2.1 -> ipn:3.1 -> ipn:4.1
        testUtils.send_bundle("2.1", "4.1", "2.0", "test_trace39")

        print("\n\nClear Node 3 Policy\n")
        testUtils.del_policy_rule(3, "1")
        testUtils.del_event_set(3, "d_conf")

        te_1 = testUtils.check_test_event(
            'sop_missing_at_acceptor', 'ipn:2.1', 'ipn:4.1', 'bcb-confidentiality', '1', None, None, '3', log3)
        testUtils.check_test_results(4, "test_trace39", False, [te_1])

    except subprocess.TimeoutExpired as e:
        print(e)
        print("TEST FAILED")
        dotest.g_tests_failed += 1
        os.chdir("..")

    print("###############################################################################")


def all_tests():
    test1()
    test2()
    test3()
    test4()
    test5()
    test6()
    test7()
    test8()
    #test9()
    #test10()
    #test11()
    #test12()
    #test13()
    #test14()
    #test15()
    #test16()
    test17()
    test18()
    test19()
    test20()
    #test21()
    #test22()
    #test23()
    test24()
    #test25()
    #test26()
    #test27()
    #test28()
    #test29()
    #test30()
    #test31()
    #test32()
    #test33()
    #test34()
    #test35()
    #test36()
    test37()
    #test38()
    #test39()


def bib_tests():
    test1()
    test3()
    test5()
    test7()
    #test11()
    test17()
    test19()
    #test22()
    #test25()
    #test26()
    #test27()
    #test28()
    #test29()
    #test31()
    #test33()
    test37()
    #test38()


def bcb_tests():
    test2()
    test4()
    test6()
    test8()
    #test10()
    #test12()
    test18()
    test20()
    #test23()
    test24()
    #test25()
    #test26()
    #test28()
    #test29()
    #test31()
    #test39()


def payload_tgt():
    test1()
    test2()
    test5()
    test6()
    test7()
    test8()
    #test12()
    test17()
    test18()
    test19()
    test20()
    #test22()
    #test23()
    test24()
    #test25()
    #test26()
    #test27()
    #test28()
    #test29()
    #test31()
    #test33()
    test37()
    #test39()


def primary_tgt():
    test3()
    test4()
    #test11()
    #test31()
    #test33()
    #test38()


def help():
    print("Test format:\npython3 main.py <test> <test> \n\nTests to run: \n"
          "single tests: 1 - 39 \n"
          "group tests: 'all', 'bib', 'bcb', 'payload', 'primary' \n")


test_dict = {'1': test1, '2': test2, '3': test3, '4': test4, '5': test5, '6': test6, '7': test7, '8': test8,
             '9': test9, '10': test10, '11': test11, '12': test12, '13': test13, '14': test14, '15': test15,
             '16': test16, '17': test17, '18': test18, '19': test19, '20': test20, '21': test21, '22': test22,
             '23': test23, '24': test24, '25': test25, '26': test26, '27': test27, '28': test28, '29': test29,
             '30': test30, '31': test31, '32': test32, '33': test33, '34': test34, '35': test35, '36': test36,
             '37': test37, '38': test38, '39': test39,
             "all": all_tests, "bib_tests": bib_tests,
             "bib": bib_tests, "bcb_tests": bcb_tests, "bcb": bcb_tests, "payload tests": payload_tgt,
             "payload": payload_tgt, "primary tests": primary_tgt, "primary": primary_tgt,
             "help": help, "h": help}
