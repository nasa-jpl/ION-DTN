import os
from re import I
import subprocess
import time

from tests import testUtils

'''
    The Johns Hopkins University Applied Physics Laboratory (JHU/APL)

    File Name: testCases.py

    Description: This file contains each of the BPSec test cases supported
    by this Python test suite.
'''

############################### BP BLOCK TYPES ################################
UNKNOWNLK = -1
PRIMARY = 0
PAYLOAD = 1
PREVNODE = 6
BUNDLEAGE = 7
BIB = 11
BCB = 12
QUALITYOFSERVICE = 193

############################## TEST CASES #####################################

def test1(inputScId=testUtils.BIB_HMAC_SHA2_SCID, inputScParams=testUtils.bibScParms):
    print("###############################################################################")

    try:
        testUtils.start_test(
            "1", "Brief: BIB targeting a Payload Block.\n\n"
                 "Description: Add a BIB targeting a Payload Block at \n"
                 "Security Source ipn:2.1 and process that BIB at Security \n"
                 "Acceptor ipn:3.1\n\n"
                 "Bundle Path: ipn:2.1 -> ipn:3.1\n\n")
        time.sleep(testUtils.TIME_DISPLAYTEXT)

        # Tracking current position in log
        log2 = testUtils.log_position("2")
        log3 = testUtils.log_position("3")

        print("Node ipn:2.1 Configuration\n")
        testUtils.add_key(2, "key_1_32bytes")
        testUtils.add_event_set(2, "d_integrity", "default bib")

        desc = "bib src tgt:payload"
        filter = testUtils.build_filter("1", "s", "ipn:2.1", tgt=PAYLOAD, sc_id=inputScId)
        spec = testUtils.build_spec("bib-integrity", sc_params=inputScParams)
        es_ref = "d_integrity"
        testUtils.add_policy_rule(2, desc, filter, spec, es_ref)

        if testUtils.g_verbose == "detailed":
            testUtils.list_key(2)
            testUtils.list_event_set(2)
            testUtils.list_policy_rule(2)
        if testUtils.g_verbose == "verbose":
            testUtils.list_key(2)
            testUtils.info_event_set(2, "d_integrity")
            testUtils.info_policy_rule(2, 1)

        print("Node ipn:3.1 Configuration \n")
        testUtils.add_key(3, "key_1_32bytes")
        testUtils.add_event_set(3, "d_integrity", "default bib")

        desc = "bib acceptor tgt:payload"
        filter = testUtils.build_filter("2", "a", "ipn:2.1", tgt=PAYLOAD)
        spec = testUtils.build_spec("bib-integrity", sc_id=inputScId, sc_params=inputScParams)
        es_ref = "d_integrity"
        testUtils.add_policy_rule(3, desc, filter, spec, es_ref)

        if testUtils.g_verbose == "detailed":
            testUtils.list_key(3)
            testUtils.list_event_set(3)
            testUtils.list_policy_rule(3)
        if testUtils.g_verbose == "verbose":
            testUtils.list_key(3)
            testUtils.info_event_set(3, "d_integrity")
            testUtils.info_policy_rule(3, 2)

        # Send the bundle from ipn:2.1 -> ipn:3.1
        testUtils.send_bundle("2.1", "3.1", "2.0", "test_trace1")

        print("\n\nClear Node ipn:2.1 Policy \n")
        testUtils.del_policy_rule(2, "1")
        testUtils.del_event_set(2, "d_integrity")
        testUtils.del_key(2, "key_1_32bytes")

        print("Clear Node ipn:3.1 Policy\n")
        testUtils.del_policy_rule(3, "2")
        testUtils.del_event_set(3, "d_integrity")
        testUtils.del_key(3, "key_1_32bytes")

        # Verify results on ipn:2.1
        log2_events = testUtils.get_test_events(2, log2)
        te_1, log2_events = testUtils.find_test_event(
            log2_events, 'sop_added_at_src', 'ipn:2.1', 'ipn:3.1', 'bib-integrity', '1', None, None)

        # Verify results on ipn:3.1
        log3_events = testUtils.get_test_events(3, log3)
        te_2, log3_events = testUtils.find_test_event(
            log3_events, 'sop_processed', 'ipn:2.1', 'ipn:3.1', 'bib-integrity', '1', te_1['msec'], te_1['count'])

        # Process all test results
        testUtils.check_test_results([3], ["test_trace1"], [log2_events, log3_events])

    except subprocess.TimeoutExpired as e:
        print(e)
        print("TEST FAILED")
        testUtils.g_tests_failed += 1
        os.chdir("..")

    print("###############################################################################")


def test2(inputScId=testUtils.BCB_AES_GCM_SCID, inputScParams=testUtils.bcbScParms):
    print("###############################################################################")

    try:
        testUtils.start_test(
            "2", "Brief: BCB targeting a Payload Block.\n\n"
                 "Description: Add a BCB targeting a Payload Block at \n"
                 "Security Source ipn:2.1 and process that BCB at Security \n"
                 "Acceptor ipn:3.1\n\n"
                 "Bundle Path: ipn:2.1 -> ipn:3.1\n\n")
        time.sleep(testUtils.TIME_DISPLAYTEXT)

        # Tracking current position in log
        log2 = testUtils.log_position("2")
        log3 = testUtils.log_position("3")

        print("Node ipn:2.1 Configuration\n")
        testUtils.add_key(2, "bcb_key_32bytes")
        testUtils.add_event_set(2, "d_bcb_conf", "default bcb")

        desc = "bcb src tgt:payload"
        filter = testUtils.build_filter("4", "s", "ipn:2.1", tgt=PAYLOAD, sc_id=inputScId)
        spec = testUtils.build_spec("bcb-confidentiality", sc_params=inputScParams)
        es_ref = "d_bcb_conf"
        testUtils.add_policy_rule(2, desc, filter, spec, es_ref)

        if testUtils.g_verbose == "detailed":
            testUtils.list_key(2)
            testUtils.list_event_set(2)
            testUtils.list_policy_rule(2)
        if testUtils.g_verbose == "verbose":
            testUtils.list_key(2)
            testUtils.info_event_set(2, "d_bcb_conf")
            testUtils.info_policy_rule(2, 4)

        print("Node ipn:3.1 Configuration \n")
        testUtils.add_key(3, "bcb_key_32bytes")
        testUtils.add_event_set(3, "d_bcb_conf", "default bcb")

        desc = "bcb acceptor tgt:payload"
        filter = testUtils.build_filter("5", "a", "ipn:2.1", tgt=PAYLOAD)
        spec = testUtils.build_spec("bcb-confidentiality", sc_id=inputScId, sc_params=inputScParams)
        es_ref = "d_bcb_conf"
        testUtils.add_policy_rule(3, desc, filter, spec, es_ref)

        if testUtils.g_verbose == "detailed":
            testUtils.list_key(3)
            testUtils.list_event_set(3)
            testUtils.list_policy_rule(3)
        if testUtils.g_verbose == "verbose":
            testUtils.list_key(3)
            testUtils.info_event_set(3, "d_bcb_conf")
            testUtils.info_policy_rule(3, 5)

        # Send the bundle from ipn:2.1 -> ipn:3.1
        testUtils.send_bundle("2.1", "3.1", "2.0", "test_trace2")

        print("\n\nClear Node ipn:2.1 Policy \n")
        testUtils.del_policy_rule(2, "4")
        testUtils.del_event_set(2, "d_bcb_conf")
        testUtils.del_key(2, "bcb_key_32bytes")

        print("Clear Node ipn:3.1 Policy\n")
        testUtils.del_policy_rule(3, "5")
        testUtils.del_event_set(3, "d_bcb_conf")
        testUtils.del_key(3, "bcb_key_32bytes")

        # Verify results on ipn:2.1
        log2_events = testUtils.get_test_events(2, log2)
        te_1, log2_events = testUtils.find_test_event(
            log2_events, 'sop_added_at_src', 'ipn:2.1', 'ipn:3.1', 'bcb-confidentiality', '1', None, None)

        # Verify results on ipn:3.1
        log3_events = testUtils.get_test_events(3, log3)
        te_2, log3_events = testUtils.find_test_event(
            log3_events, 'sop_processed', 'ipn:2.1', 'ipn:3.1', 'bcb-confidentiality', '1', te_1['msec'], te_1['count'])

        # Process all test results
        testUtils.check_test_results([3], ["test_trace2"], [log2_events, log3_events])

    except subprocess.TimeoutExpired as e:
        print(e)
        print("TEST FAILED")
        testUtils.g_tests_failed += 1
        os.chdir("..")

    print("###############################################################################")


def test3(inputScId=testUtils.BIB_HMAC_SHA2_SCID, inputScParams=testUtils.bibPrimaryScParms):
    print("###############################################################################")

    try:
        testUtils.start_test(
            "3", "Brief: BIB targeting a Primary Block.\n\n"
                 "Description: Add a BIB targeting a Primary Block at \n"
                 "Security Source ipn:2.1 and process that BIB at Security \n"
                 "Acceptor ipn:3.1\n\n"
                 "Bundle Path: ipn:2.1 -> ipn:3.1\n\n")
        time.sleep(testUtils.TIME_DISPLAYTEXT)

        # Tracking current position in log
        log2 = testUtils.log_position("2")
        log3 = testUtils.log_position("3")

        print("Node ipn:2.1 Configuration\n")
        testUtils.add_key(2, "key_1_32bytes")
        testUtils.add_event_set(2, "d_integrity", "default bib")

        desc = "bib src tgt:primary"
        filter = testUtils.build_filter("6", "s", "ipn:2.1", tgt=PRIMARY, sc_id=inputScId)
        spec = testUtils.build_spec("bib-integrity", sc_params=inputScParams)
        es_ref = "d_integrity"
        testUtils.add_policy_rule(2, desc, filter, spec, es_ref)

        if testUtils.g_verbose == "detailed":
            testUtils.list_key(2)
            testUtils.list_event_set(2)
            testUtils.list_policy_rule(2)
        if testUtils.g_verbose == "verbose":
            testUtils.list_key(2)
            testUtils.info_event_set(2, "d_integrity")
            testUtils.info_policy_rule(2, 6)

        print("Node ipn:3.1 Configuration \n")
        testUtils.add_key(3, "key_1_32bytes")
        testUtils.add_event_set(3, "d_integrity", "default bib")

        desc = "bib acceptor tgt:primary"
        filter = testUtils.build_filter("7", "a", "ipn:2.1", tgt=PRIMARY)
        spec = testUtils.build_spec("bib-integrity", sc_id=inputScId, sc_params=inputScParams)
        es_ref = "d_integrity"
        testUtils.add_policy_rule(3, desc, filter, spec, es_ref)

        if testUtils.g_verbose == "detailed":
            testUtils.list_key(3)
            testUtils.list_event_set(3)
            testUtils.list_policy_rule(3)
        if testUtils.g_verbose == "verbose":
            testUtils.list_key(3)
            testUtils.info_event_set(3, "d_integrity")
            testUtils.info_policy_rule(3, 7)

        testUtils.send_bundle("2.1", "3.1", "2.0", "test_trace3")

        print("\n\nClear Node ipn:2.1 Policy \n")
        testUtils.del_policy_rule(2, "6")
        testUtils.del_event_set(2, "d_integrity")
        testUtils.del_key(2, "key_1_32bytes")

        print("Clear Node ipn:3.1 Policy \n")
        testUtils.del_policy_rule(3, "7")
        testUtils.del_event_set(3, "d_integrity")
        testUtils.del_key(3, "key_1_32bytes")

        # Verify results on ipn:2.1
        log2_events = testUtils.get_test_events(2, log2)
        te_1, log2_events = testUtils.find_test_event(
            log2_events, 'sop_added_at_src', 'ipn:2.1', 'ipn:3.1', 'bib-integrity', '0', None, None)

        # Verify results on ipn:3.1
        log3_events = testUtils.get_test_events(3, log3)
        te_2, log3_events = testUtils.find_test_event(
            log3_events, 'sop_processed', 'ipn:2.1', 'ipn:3.1', 'bib-integrity', '0', te_1['msec'], te_1['count'])

        # Process all test results
        testUtils.check_test_results([3], ["test_trace3"], [log2_events, log3_events])


    except subprocess.TimeoutExpired as e:
        print(e)
        print("TEST FAILED")
        testUtils.g_tests_failed += 1
        os.chdir("..")

    print("###############################################################################")


def test4(inputScId=testUtils.BCB_AES_GCM_SCID, inputScParams=testUtils.bcbScParms):
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
        time.sleep(testUtils.TIME_DISPLAYTEXT)

        # Tracking current position in log
        log2 = testUtils.log_position("2")
        log3 = testUtils.log_position("3")

        print("Node ipn:2.1 Configuration\n")
        testUtils.add_key(2, "bcb_key_32bytes")
        testUtils.add_event_set(2, "d_bcb_conf", "default bcb")

        desc = "bcb src tgt:primary"
        filter = testUtils.build_filter("8", "s", "ipn:2.1", tgt=PRIMARY, sc_id=inputScId)
        spec = testUtils.build_spec("bcb-confidentiality", sc_params=inputScParams)
        es_ref = "d_bcb_conf"
        testUtils.add_policy_rule(2, desc, filter, spec, es_ref)

        if testUtils.g_verbose == "detailed":
            testUtils.list_key(2)
            testUtils.list_event_set(2)
            testUtils.list_policy_rule(2)
        if testUtils.g_verbose == "verbose":
            testUtils.list_key(2)
            testUtils.info_event_set(2, "d_bcb_conf")
            testUtils.info_policy_rule(2, 8)

        print("Node ipn:3.1 Configuration \n")
        testUtils.add_key(3, "bcb_key_32bytes")
        testUtils.add_event_set(3, "d_bcb_conf", "default bcb")

        desc = "bcb acceptor tgt:primary"
        filter = testUtils.build_filter("9", "a", "ipn:2.1", tgt=PRIMARY)
        spec = testUtils.build_spec("bcb-confidentiality", sc_id=inputScId, sc_params=inputScParams)
        es_ref = "d_bcb_conf"
        testUtils.add_policy_rule(3, desc, filter, spec, es_ref)

        if testUtils.g_verbose == "detailed":
            testUtils.list_key(3)
            testUtils.list_event_set(3)
            testUtils.list_policy_rule(3)
        if testUtils.g_verbose == "verbose":
            testUtils.list_key(3)
            testUtils.info_event_set(3, "d_bcb_conf")
            testUtils.info_policy_rule(3, 9)

        # Send the bundle from ipn:2.1 -> ipn:3.1
        testUtils.send_bundle("2.1", "3.1", "2.0", "test_trace4")

        print("\n\nClear Node ipn:2.1 Policy \n")
        testUtils.del_policy_rule(2, "8")
        testUtils.del_event_set(2, "d_bcb_conf")
        testUtils.del_key(2, "bcb_key_32bytes")

        print("Clear Node ipn:3.1 Policy \n")
        testUtils.del_policy_rule(3, "9")
        testUtils.del_event_set(3, "d_bcb_conf")
        testUtils.del_key(3, "bcb_key_32bytes")

        # Note that we do not expect to find these test events as
        # security policy is intentionally misconfigured

        # Verify results on ipn:2.1
        log2_events = testUtils.get_test_events(2, log2)
        te_1, log2_events = testUtils.find_test_event(
            log2_events, 'sop_added_at_src', 'ipn:2.1', 'ipn:3.1', 'bcb-confidentiality', '0', None, None, find=False)

        # Verify results on ipn:3.1
        log3_events = testUtils.get_test_events(3, log3)
        te_2, log3_events = testUtils.find_test_event(
            log3_events, 'sop_processed', 'ipn:2.1', 'ipn:3.1', 'bcb-confidentiality', '0', te_1['msec'], te_1['count'], find=False)

        # Process all test results
        testUtils.check_test_results([3], ["test_trace4"], [log2_events, log3_events])

    except subprocess.TimeoutExpired as e:
        print(e)
        print("TEST FAILED")
        testUtils.g_tests_failed += 1
        os.chdir("..")

    print("###############################################################################")


def test5(inputScId=testUtils.BIB_HMAC_SHA2_SCID, inputScParams=testUtils.bibScParms):
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
        time.sleep(testUtils.TIME_DISPLAYTEXT)

        # Tracking current position in log
        log2 = testUtils.log_position("2")
        log3 = testUtils.log_position("3")
        log4 = testUtils.log_position("4")

        print("Node ipn:2.1 Configuration\n")
        testUtils.add_key(2, "key_1_32bytes")
        testUtils.add_event_set(2, "d_integrity", "default bib")

        desc = "bib src tgt:payload"
        filter = testUtils.build_filter("10", "s", "ipn:2.1", tgt=PAYLOAD, sc_id=inputScId)
        spec = testUtils.build_spec("bib-integrity", sc_params=inputScParams)
        es_ref = "d_integrity"
        testUtils.add_policy_rule(2, desc, filter, spec, es_ref)

        if testUtils.g_verbose == "detailed":
            testUtils.list_key(2)
            testUtils.list_event_set(2)
            testUtils.list_policy_rule(2)
        if testUtils.g_verbose == "verbose":
            testUtils.list_key(2)
            testUtils.info_event_set(2, "d_integrity")
            testUtils.info_policy_rule(2, 10)

        print("Node ipn:3.1 Configuration \n")
        testUtils.add_key(3, "key_1_32bytes")
        testUtils.add_event_set(3, "d_integrity", "default bib")

        desc = "bib verifier tgt:payload"
        filter = testUtils.build_filter("11", "v", "ipn:2.1", tgt=PAYLOAD)
        spec = testUtils.build_spec("bib-integrity", sc_id=inputScId, sc_params=inputScParams)
        es_ref = "d_integrity"
        testUtils.add_policy_rule(3, desc, filter, spec, es_ref)

        if testUtils.g_verbose == "detailed":
            testUtils.list_key(3)
            testUtils.list_event_set(3)
            testUtils.list_policy_rule(3)
        if testUtils.g_verbose == "verbose":
            testUtils.list_key(3)
            testUtils.info_event_set(3, "d_integrity")
            testUtils.info_policy_rule(3, 11)

        print("Node ipn:4.1 Configuration\n")
        testUtils.add_key(4, "key_1_32bytes")
        testUtils.add_event_set(4, "d_integrity", "default bib")

        desc = "bib acceptor tgt:payload"
        filter = testUtils.build_filter("12", "a", "ipn:2.1", tgt=PAYLOAD)
        spec = testUtils.build_spec("bib-integrity", sc_id=inputScId, sc_params=inputScParams)
        es_ref = "d_integrity"
        testUtils.add_policy_rule(4, desc, filter, spec, es_ref)

        if testUtils.g_verbose == "detailed":
            testUtils.list_key(4)
            testUtils.list_event_set(4)
            testUtils.list_policy_rule(4)
        if testUtils.g_verbose == "verbose":
            testUtils.list_key(4)
            testUtils.info_event_set(4, "d_integrity")
            testUtils.info_policy_rule(4, 12)

        # Send the bundle from ipn:2.1 -> ipn:3.1 -> ipn:4.1
        testUtils.send_bundle("2.1", "4.1", "2.0", "test_trace5")

        print("\n\nClear Node ipn:2.1 Policy \n")
        testUtils.del_policy_rule(2, "10")
        testUtils.del_event_set(2, "d_integrity")
        testUtils.del_key(2, "key_1_32bytes")

        print("Clear Node ipn:3.1 Policy \n")
        testUtils.del_policy_rule(3, "11")
        testUtils.del_event_set(3, "d_integrity")
        testUtils.del_key(3, "key_1_32bytes")

        print("Clear Node ipn:4.1 Policy \n\n")
        testUtils.del_policy_rule(4, "12")
        testUtils.del_event_set(4, "d_integrity")
        testUtils.del_key(4, "key_1_32bytes")

        # Verify results on ipn:2.1
        log2_events = testUtils.get_test_events(2, log2)
        te_1, log2_events = testUtils.find_test_event(
            log2_events, 'sop_added_at_src', 'ipn:2.1', 'ipn:4.1', 'bib-integrity', '1', None, None)

        # Verify results on ipn:3.1
        log3_events = testUtils.get_test_events(3, log3)
        te_2, log3_events = testUtils.find_test_event(
            log3_events, 'sop_verified', 'ipn:2.1', 'ipn:4.1', 'bib-integrity', '1', te_1['msec'], te_1['count'])

        # Verify results on ipn:4.1
        log4_events = testUtils.get_test_events(4, log4)
        te_3, log4_events = testUtils.find_test_event(
            log4_events, 'sop_processed', 'ipn:2.1', 'ipn:4.1', 'bib-integrity', '1', te_1['msec'], te_1['count'])

        # Process all test results
        testUtils.check_test_results([4], ["test_trace5"], [log2_events, log3_events, log4_events])


    except subprocess.TimeoutExpired as e:
        print(e)
        print("TEST FAILED")
        testUtils.g_tests_failed += 1
        os.chdir("..")

    print("###############################################################################")


def test6(inputScId=testUtils.BCB_AES_GCM_SCID, inputScParams=testUtils.bcbScParms):
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

        time.sleep(testUtils.TIME_DISPLAYTEXT)

        # Tracking current position in log
        log2 = testUtils.log_position("2")
        log4 = testUtils.log_position("4")

        print("Node ipn:2.1 Configuration\n")
        testUtils.add_key(2, "bcb_key_32bytes")
        testUtils.add_event_set(2, "d_bcb_conf", "default bcb")

        desc = "bcb src tgt:payload"
        filter = testUtils.build_filter("13", "s", "ipn:2.1", tgt=PAYLOAD, sc_id=inputScId)
        spec = testUtils.build_spec("bcb-confidentiality", sc_params=inputScParams)
        es_ref = "d_bcb_conf"
        testUtils.add_policy_rule(2, desc, filter, spec, es_ref)

        if testUtils.g_verbose == "detailed":
            testUtils.list_key(2)
            testUtils.list_event_set(2)
            testUtils.list_policy_rule(2)
        if testUtils.g_verbose == "verbose":
            testUtils.list_key(2)
            testUtils.info_event_set(2, "d_bcb_conf")
            testUtils.info_policy_rule(2, 13)

        print("Node ipn:3.1 Configuration \n")
        testUtils.add_key(3, "bcb_key_32bytes")
        testUtils.add_event_set(3, "d_bcb_conf", "default bcb")

        desc = "bcb verifier tgt:payload"
        filter = testUtils.build_filter("14", "v", "ipn:2.1", tgt=PAYLOAD)
        spec = testUtils.build_spec("bcb-confidentiality", sc_id=inputScId, sc_params=inputScParams)
        es_ref = "d_bcb_conf"
        testUtils.add_policy_rule(3, desc, filter, spec, es_ref)

        if testUtils.g_verbose == "detailed":
            testUtils.list_key(3)
            testUtils.list_event_set(3)
            testUtils.list_policy_rule(3)
        if testUtils.g_verbose == "verbose":
            testUtils.list_key(3)
            testUtils.info_event_set(3, "d_bcb_conf")
            testUtils.info_policy_rule(3, 14)

        print("Node ipn:4.1 Configuration\n")
        testUtils.add_key(4, "bcb_key_32bytes")
        testUtils.add_event_set(4, "d_bcb_conf", "default bcb")

        desc = "bcb acceptor tgt:payload"
        filter = testUtils.build_filter("15", "a", "ipn:2.1", tgt=PAYLOAD)
        spec = testUtils.build_spec("bcb-confidentiality", sc_id=inputScId, sc_params=inputScParams)
        es_ref = "d_bcb_conf"
        testUtils.add_policy_rule(4, desc, filter, spec, es_ref)

        if testUtils.g_verbose == "detailed":
            testUtils.list_key(4)
            testUtils.list_event_set(4)
            testUtils.list_policy_rule(4)
        if testUtils.g_verbose == "verbose":
            testUtils.list_key(4)
            testUtils.info_event_set(4, "d_bcb_conf")
            testUtils.info_policy_rule(4, 15)

        # Send the bundle from ipn:2.1 -> ipn:3.1 -> ipn:4.1
        testUtils.send_bundle("2.1", "4.1", "2.0", "test_trace6")

        print("\n\nClear Node ipn:2.1 Policy \n")
        testUtils.del_policy_rule(2, "13")
        testUtils.del_event_set(2, "d_bcb_conf")
        testUtils.del_key(2, "bcb_key_32bytes")

        print("Clear Node ipn:3.1 Policy \n")
        testUtils.del_policy_rule(3, "14")
        testUtils.del_event_set(3, "d_bcb_conf")
        testUtils.del_key(3, "bcb_key_32bytes")

        print("Clear Node ipn:4.1 Policy \n\n")
        testUtils.del_policy_rule(4, "15")
        testUtils.del_event_set(4, "d_bcb_conf")
        testUtils.del_key(4, "bcb_key_32bytes")

        # Verify results on ipn:2.1
        log2_events = testUtils.get_test_events(2, log2)
        te_1, log2_events = testUtils.find_test_event(
            log2_events, 'sop_added_at_src', 'ipn:2.1', 'ipn:4.1', 'bcb-confidentiality', '1', None, None)

        # Verify results on ipn:3.1
        #log3_events = testUtils.get_test_events(3, log3)
        #te_2, log3_events = testUtils.find_test_event(
        #    log3_events, 'sop_verified', 'ipn:2.1', 'ipn:4.1', 'bcb-confidentiality', '1', te_1['msec'], te_1['count'])

        # Verify results on ipn:4.1
        log4_events = testUtils.get_test_events(4, log4)
        te_3, log4_events = testUtils.find_test_event(
            log4_events, 'sop_processed', 'ipn:2.1', 'ipn:4.1', 'bcb-confidentiality', '1', te_1['msec'], te_1['count'])

        # Process all test results
        testUtils.check_test_results([4], ["test_trace6"], [log2_events, log4_events])


    except subprocess.TimeoutExpired as e:
        print(e)
        print("TEST FAILED")
        testUtils.g_tests_failed += 1
        os.chdir("..")

    print("###############################################################################")


def test7(inputScId=testUtils.BIB_HMAC_SHA2_SCID, inputScParams=testUtils.bibScParms):
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
        time.sleep(testUtils.TIME_DISPLAYTEXT)

        # Tracking current position in log
        log2 = testUtils.log_position("2")
        log3 = testUtils.log_position("3")

        print("Node ipn:2.1 Configuration\n")
        testUtils.add_key(2, "key_1_32bytes")
        testUtils.add_event_set(2, "d_integrity", "default bib")

        desc = "bib src tgt:payload"
        filter = testUtils.build_filter("16", "s", "ipn:2.1", tgt=PAYLOAD, sc_id=inputScId)
        spec = testUtils.build_spec("bib-integrity", sc_params=inputScParams)
        es_ref = "d_integrity"
        testUtils.add_policy_rule(2, desc, filter, spec, es_ref)

        desc = "dup bib src tgt:payload"
        filter = testUtils.build_filter("17", "s", "ipn:2.1", tgt=PAYLOAD, sc_id=inputScId)
        spec = testUtils.build_spec("bib-integrity", sc_params=inputScParams)
        es_ref = "d_integrity"
        testUtils.add_policy_rule(2, desc, filter, spec, es_ref)

        if testUtils.g_verbose == "detailed":
            testUtils.list_key(2)
            testUtils.list_event_set(2)
            testUtils.list_policy_rule(2)
        if testUtils.g_verbose == "verbose":
            testUtils.list_key(2)
            testUtils.info_event_set(2, "d_integrity")
            testUtils.info_policy_rule(2, 16)
            testUtils.info_policy_rule(2, 17)

        print("Node ipn:3.1 Configuration \n")
        testUtils.add_key(3, "key_1_32bytes")
        testUtils.add_event_set(3, "d_integrity", "default bib")

        # Arguments for policy to require the BIB targeting the Primary Block
        # Note: requiring the presence of a single BIB
        desc = "bib acceptor tgt:payload"
        filter = testUtils.build_filter("18", "a", "ipn:2.1", tgt=PAYLOAD)
        spec = testUtils.build_spec("bib-integrity", sc_id=inputScId, sc_params=inputScParams)
        es_ref = "d_integrity"
        testUtils.add_policy_rule(3, desc, filter, spec, es_ref)

        desc = "dup bib acceptor tgt:payload"
        filter = testUtils.build_filter("19", "a", "ipn:2.1", tgt=PAYLOAD)
        spec = testUtils.build_spec("bib-integrity", sc_id=inputScId, sc_params=inputScParams)
        es_ref = "d_integrity"
        testUtils.add_policy_rule(3, desc, filter, spec, es_ref)

        if testUtils.g_verbose == "detailed":
            testUtils.list_key(3)
            testUtils.list_event_set(3)
            testUtils.list_policy_rule(3)
        if testUtils.g_verbose == "verbose":
            testUtils.list_key(3)
            testUtils.info_event_set(3, "d_integrity")
            testUtils.info_policy_rule(3, 18)
            testUtils.info_policy_rule(3, 19)

        # Sending the bundle from ipn:2.1->ipn:3.1
        testUtils.send_bundle("2.1", "3.1", "2.0", "test_trace7")

        print("\n\nClear Node ipn:2.1 Policy \n")
        testUtils.del_policy_rule(2, "16")
        testUtils.del_policy_rule(2, "17")
        testUtils.del_event_set(2, "d_integrity")
        testUtils.del_key(2, "key_1_32bytes")

        print("Clear Node ipn:3.1 Policy\n")
        testUtils.del_policy_rule(3, "18")
        testUtils.del_policy_rule(3, "19")
        testUtils.del_event_set(3, "d_integrity")
        testUtils.del_key(3, "key_1_32bytes")

        # Verify results on ipn:2.1
        log2_events = testUtils.get_test_events(2, log2)
        te_1, log2_events = testUtils.find_test_event(
            log2_events, 'sop_added_at_src', 'ipn:2.1', 'ipn:3.1', 'bib-integrity', '1', None, None)

        # Verify results on ipn:3.1
        log3_events = testUtils.get_test_events(3, log3)
        te_2, log3_events = testUtils.find_test_event(
            log3_events, 'sop_processed', 'ipn:2.1', 'ipn:3.1', 'bib-integrity', '1', te_1['msec'], te_1['count'])

        # Process all test results
        testUtils.check_test_results([3], ["test_trace7"], [log2_events, log3_events])

    except subprocess.TimeoutExpired as e:
        print(e)
        print("TEST FAILED")
        testUtils.g_tests_failed += 1
        os.chdir("..")

    print("###############################################################################")


def test8(inputScId=testUtils.BCB_AES_GCM_SCID, inputScParams=testUtils.bcbScParms):
    print("###############################################################################")

    try:
        testUtils.start_test(
            "8", "Brief: Duplicate security policy rules at a security source/acceptor for a BCB.\n\n"
                 "Description: Create two security policy rules at a security source and acceptor \n"
                 "requiring a BCB targeting the Payload block. \n"
                 "A single BCB should be added to the bundle, indicating that \n"
                 "rule prioritization behaves as expected.\n\n"
                 "Bundle Path: ipn:2.1 -> ipn:3.1\n\n")

        time.sleep(testUtils.TIME_DISPLAYTEXT)

        # Tracking current position in log
        log2 = testUtils.log_position("2")
        log3 = testUtils.log_position("3")

        print("Node ipn:2.1 Configuration\n")
        testUtils.add_key(2, "bcb_key_32bytes")
        testUtils.add_event_set(2, "d_bcb_conf", "default bcb")

        desc = "bcb src tgt:payload"
        filter = testUtils.build_filter("21", "s", "ipn:2.1", tgt=PAYLOAD, sc_id=inputScId)
        spec = testUtils.build_spec("bcb-confidentiality", sc_params=inputScParams)
        es_ref = "d_bcb_conf"
        testUtils.add_policy_rule(2, desc, filter, spec, es_ref)

        desc = "dup bcb src tgt:payload"
        filter = testUtils.build_filter("22", "s", "ipn:2.1", tgt=PAYLOAD, sc_id=inputScId)
        spec = testUtils.build_spec("bcb-confidentiality", sc_params=inputScParams)
        es_ref = "d_bcb_conf"
        testUtils.add_policy_rule(2, desc, filter, spec, es_ref)

        if testUtils.g_verbose == "detailed":
            testUtils.list_key(2)
            testUtils.list_event_set(2)
            testUtils.list_policy_rule(2)
        if testUtils.g_verbose == "verbose":
            testUtils.list_key(2)
            testUtils.info_event_set(2, "d_bcb_conf")
            testUtils.info_policy_rule(2, 21)
            testUtils.info_policy_rule(2, 22)

        print("Node ipn:3.1 Configuration \n")
        testUtils.add_key(3, "bcb_key_32bytes")
        testUtils.add_event_set(3, "d_bcb_conf", "default bcb")

        desc = "bcb acceptor tgt:payload"
        filter = testUtils.build_filter("23", "a", "ipn:2.1", tgt=PAYLOAD)
        spec = testUtils.build_spec("bcb-confidentiality", sc_id=inputScId, sc_params=inputScParams)
        es_ref = "d_bcb_conf"
        testUtils.add_policy_rule(3, desc, filter, spec, es_ref)

        desc = "dup bcb acceptor tgt:payload"
        filter = testUtils.build_filter("24", "a", "ipn:2.1", tgt=PAYLOAD)
        spec = testUtils.build_spec("bcb-confidentiality", sc_id=inputScId, sc_params=inputScParams)
        es_ref = "d_bcb_conf"
        testUtils.add_policy_rule(3, desc, filter, spec, es_ref)

        if testUtils.g_verbose == "detailed":
            testUtils.list_key(3)
            testUtils.list_event_set(3)
            testUtils.list_policy_rule(3)
        if testUtils.g_verbose == "verbose":
            testUtils.list_key(3)
            testUtils.info_event_set(3, "d_bcb_conf")
            testUtils.info_policy_rule(3, 23)
            testUtils.info_policy_rule(3, 24)

        # Sending the bundle from ipn:2.1 -> ipn:3.1
        testUtils.send_bundle("2.1", "3.1", "2.0", "test_trace8")

        print("\n\nClear Node ipn:2.1 Policy \n")
        testUtils.del_policy_rule(2, "21")
        testUtils.del_policy_rule(2, "22")
        testUtils.del_event_set(2, "d_bcb_conf")
        testUtils.del_key(2, "bcb_key_32bytes")

        print("Clear Node ipn:3.1 Policy\n")
        testUtils.del_policy_rule(3, "23")
        testUtils.del_policy_rule(3, "24")
        testUtils.del_event_set(3, "d_bcb_conf")
        testUtils.del_key(3, "bcb_key_32bytes")

        # Verify results on ipn:2.1
        log2_events = testUtils.get_test_events(2, log2)
        te_1, log2_events = testUtils.find_test_event(
            log2_events, 'sop_added_at_src', 'ipn:2.1', 'ipn:3.1', 'bcb-confidentiality', '1', None, None)

        # Verify results on ipn:3.1
        log3_events = testUtils.get_test_events(3, log3)
        te_2, log3_events = testUtils.find_test_event(
            log3_events, 'sop_processed', 'ipn:2.1', 'ipn:3.1', 'bcb-confidentiality', '1', te_1['msec'], te_1['count'])

        # Process all test results
        testUtils.check_test_results([3], ["test_trace8"], [log2_events, log3_events])

    except subprocess.TimeoutExpired as e:
        print(e)
        print("TEST FAILED")
        testUtils.g_tests_failed += 1
        os.chdir("..")

    print("###############################################################################")


def test9(inputScId=testUtils.BIB_HMAC_SHA2_SCID, inputScParams=testUtils.bibScParms):
    print("###############################################################################")

    try:
        testUtils.start_test(
            "9", "Brief: BIB SHA Variant Misconfiguration.\n\n"
                 "Description: Add a BIB targeting the Payload Block at \n"
                 "Security Source ipn:2.1 using SHA variant 5 - HMAC 256/256 \n"
                 "and process that BIB at Security Acceptor ipn:3.1 with misconfigured \n"
                 "SHA variant 6 - HMAC 384/384.\n\n"
                 "Bundle Path: ipn:2.1 -> ipn:3.1\n\n")
        time.sleep(testUtils.TIME_DISPLAYTEXT)

        vers = testUtils.get_version(2)

        if vers == "ION-OPEN-SOURCE":
            print("This test (#9) is not used for ION Open Source testing as the \n"
            "IOS security contexts do NOT generate cryptographic material.\n")
            print("TEST PASSED")
            testUtils.g_tests_passed += 1
            return
        
        elif vers == "ION-NASA-BASELINE":
        
            # Tracking current position in log
            log2 = testUtils.log_position("2")
            log3 = testUtils.log_position("3")

            print("Node ipn:2.1 Configuration\n")
            testUtils.add_key(2, "key_1_32bytes")
            testUtils.add_event_set(2, "d_integrity", "default bib")

            # Create misconfigured parameters for SHA variant
            inputScParams.append(["sha_variant", "5"])

            desc = "bib src tgt:payload"
            filter = testUtils.build_filter("1", "s", "ipn:2.1", tgt=PAYLOAD, sc_id=inputScId)
            spec = testUtils.build_spec("bib-integrity", sc_params=inputScParams)
            es_ref = "d_integrity"
            testUtils.add_policy_rule(2, desc, filter, spec, es_ref)

            if testUtils.g_verbose == "detailed":
                testUtils.list_key(2)
                testUtils.list_event_set(2)
                testUtils.list_policy_rule(2)
            if testUtils.g_verbose == "verbose":
                testUtils.list_key(2)
                testUtils.info_event_set(2, "d_integrity")
                testUtils.info_policy_rule(2, 1)

            print("Node ipn:3.1 Configuration \n")
            testUtils.add_key(3, "key_1_32bytes")
            testUtils.add_event_set(3, "d_integrity", "default bib")

            # Create misconfigured parameters for SHA variant
            inputScParams.remove(["sha_variant", "5"])
            inputScParams.append(["sha_variant", "6"])

            desc = "bib acceptor tgt:payload"
            filter = testUtils.build_filter("2", "a", "ipn:2.1", tgt=PAYLOAD)
            spec = testUtils.build_spec("bib-integrity", sc_id=inputScId, sc_params=inputScParams)
            es_ref = "d_integrity"
            testUtils.add_policy_rule(3, desc, filter, spec, es_ref)

            if testUtils.g_verbose == "detailed":
                testUtils.list_key(3)
                testUtils.list_event_set(3)
                testUtils.list_policy_rule(3)
            if testUtils.g_verbose == "verbose":
                testUtils.list_key(3)
                testUtils.info_event_set(3, "d_integrity")
                testUtils.info_policy_rule(3, 2)

            # Send the bundle from ipn:2.1 -> ipn:3.1
            testUtils.send_bundle("2.1", "3.1", "2.0", "test_trace9")

            print("\n\nClear Node ipn:2.1 Policy \n")
            testUtils.del_policy_rule(2, "1")
            testUtils.del_event_set(2, "d_integrity")
            testUtils.del_key(2, "key_1_32bytes")

            print("Clear Node ipn:3.1 Policy\n")
            testUtils.del_policy_rule(3, "2")
            testUtils.del_event_set(3, "d_integrity")
            testUtils.del_key(3, "key_1_32bytes")

            # Verify results on ipn:2.1
            log2_events = testUtils.get_test_events(2, log2)
            te_1, log2_events = testUtils.find_test_event(
                log2_events, 'sop_added_at_src', 'ipn:2.1', 'ipn:3.1', 'bib-integrity', '1', None, None)

            # Verify results on ipn:3.1
            log3_events = testUtils.get_test_events(3, log3)
            te_2, log3_events = testUtils.find_test_event(
                log3_events, 'sop_misconfigured_at_acceptor', 'ipn:2.1', 'ipn:3.1', 'bib-integrity', '1', te_1['msec'], te_1['count'])

            # Process all test results
            testUtils.check_test_results([3], ["test_trace9"], [log2_events, log3_events])
        
        else:
            print("Invalid ION version detected.")
            print("TEST FAILED")
            testUtils.g_tests_failed += 1
            os.chdir("..")

    except subprocess.TimeoutExpired as e:
        print(e)
        print("TEST FAILED")
        testUtils.g_tests_failed += 1
        os.chdir("..")

    print("###############################################################################")


def test10(inputScId=testUtils.BCB_AES_GCM_SCID, inputScParams=testUtils.bcbScParms):
    print("###############################################################################")

    try:
        testUtils.start_test(
            "10", "Brief: BCB AES Variant Misconfiguration.\n\n"
                 "Description: Add a BCB targeting a Payload Block at \n"
                 "Security Source ipn:2.1 using AES variant 1 - A128GCM - \n"
                 "and process that BCB at Security Acceptor ipn:3.1 with \n"
                 "misconfigured AES variant 3 - A256GCM.\n\n"
                 "Bundle Path: ipn:2.1 -> ipn:3.1\n\n")
        time.sleep(testUtils.TIME_DISPLAYTEXT)

        vers = testUtils.get_version(2)

        if vers == "ION-OPEN-SOURCE":
            print("This test (#10) is not used for ION Open Source testing as the \n"
            "IOS security contexts do NOT generate cryptographic material.\n")
            print("TEST PASSED")
            testUtils.g_tests_passed += 1
            return
        
        elif vers == "ION-NASA-BASELINE":
        
            # Tracking current position in log
            log2 = testUtils.log_position("2")
            log3 = testUtils.log_position("3")

            print("Node ipn:2.1 Configuration\n")
            testUtils.add_key(2, "bcb_key_32bytes")
            testUtils.add_event_set(2, "d_bcb_conf", "default bcb")

            # Create misconfigured parameters for AES variant
            inputScParams.append(["aes_variant", "1"])

            desc = "bcb src tgt:payload"
            filter = testUtils.build_filter("4", "s", "ipn:2.1", tgt=PAYLOAD, sc_id=inputScId)
            spec = testUtils.build_spec("bcb-confidentiality", sc_params=inputScParams)
            es_ref = "d_bcb_conf"
            testUtils.add_policy_rule(2, desc, filter, spec, es_ref)

            if testUtils.g_verbose == "detailed":
                testUtils.list_key(2)
                testUtils.list_event_set(2)
                testUtils.list_policy_rule(2)
            if testUtils.g_verbose == "verbose":
                testUtils.list_key(2)
                testUtils.info_event_set(2, "d_bcb_conf")
                testUtils.info_policy_rule(2, 4)

            print("Node ipn:3.1 Configuration \n")
            testUtils.add_key(3, "bcb_key_32bytes")
            testUtils.add_event_set(3, "d_bcb_conf", "default bcb")

            # Create misconfigured parameters for AES variant
            inputScParams.remove(["aes_variant", "1"])
            inputScParams.append(["aes_variant", "3"])

            desc = "bcb acceptor tgt:payload"
            filter = testUtils.build_filter("5", "a", "ipn:2.1", tgt=PAYLOAD)
            spec = testUtils.build_spec("bcb-confidentiality", sc_id=inputScId, sc_params=inputScParams)
            es_ref = "d_bcb_conf"
            testUtils.add_policy_rule(3, desc, filter, spec, es_ref)

            if testUtils.g_verbose == "detailed":
                testUtils.list_key(3)
                testUtils.list_event_set(3)
                testUtils.list_policy_rule(3)
            if testUtils.g_verbose == "verbose":
                testUtils.list_key(3)
                testUtils.info_event_set(3, "d_bcb_conf")
                testUtils.info_policy_rule(3, 5)

            # Send the bundle from ipn:2.1 -> ipn:3.1
            testUtils.send_bundle("2.1", "3.1", "2.0", "test_trace10")

            print("\n\nClear Node ipn:2.1 Policy \n")
            testUtils.del_policy_rule(2, "4")
            testUtils.del_event_set(2, "d_bcb_conf")
            testUtils.del_key(2, "bcb_key_32bytes")

            print("Clear Node ipn:3.1 Policy\n")
            testUtils.del_policy_rule(3, "5")
            testUtils.del_event_set(3, "d_bcb_conf")
            testUtils.del_key(3, "bcb_key_32bytes")

            # Verify results on ipn:2.1
            log2_events = testUtils.get_test_events(2, log2)
            te_1, log2_events = testUtils.find_test_event(
                log2_events, 'sop_added_at_src', 'ipn:2.1', 'ipn:3.1', 'bcb-confidentiality', '1', None, None)

            # Verify results on ipn:3.1
            log3_events = testUtils.get_test_events(3, log3)
            te_2, log3_events = testUtils.find_test_event(
                log3_events, 'sop_misconfigured_at_acceptor', 'ipn:2.1', 'ipn:3.1', 'bcb-confidentiality', '1', te_1['msec'], te_1['count'])

            # Process all test results
            testUtils.check_test_results([3], ["test_trace10"], [log2_events, log3_events])
        
        else:
            print("Invalid ION version detected.")
            print("TEST FAILED")
            testUtils.g_tests_failed += 1
            os.chdir("..")

    except subprocess.TimeoutExpired as e:
        print(e)
        print("TEST FAILED")
        testUtils.g_tests_failed += 1
        os.chdir("..")

    print("###############################################################################")


def test11(inputScId=testUtils.BIB_HMAC_SHA2_SCID, inputScParams=testUtils.bibPrimaryScParms):
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
                  "NOTE: Identification of missing security operations is not supported in the IOS 4.1.2 release.\n"
                  "Skipping test case.\n\n")
        time.sleep(testUtils.TIME_DISPLAYTEXT)

        '''
        # Tracking current position in log
        log3 = testUtils.log_position("3")

        print("Node 3.1 Configuration \n")
        testUtils.add_key(3, "key_1_32bytes")
        testUtils.add_event_set(3, "d_integrity", "default bib")

        desc = "bib acceptor tgt:primary"
        filter = testUtils.build_filter("25", "a", "ipn:2.1", tgt=PRIMARY)
        spec = testUtils.build_spec("bib-integrity", sc_id=inputScId, sc_params=inputScParams)
        es_ref = "d_integrity"
        testUtils.add_policy_rule(3, desc, filter, spec, es_ref)

        if testUtils.g_verbose == "detailed":
            testUtils.list_key(3)
            testUtils.list_event_set(3)
            testUtils.list_policy_rule(3)
        if testUtils.g_verbose == "verbose":
            testUtils.list_key(3)
            testUtils.info_event_set(3, "d_integrity")
            testUtils.info_policy_rule(3, 25)

        # Sending the bundle from ipn:2.1 to ipn:3.1
        testUtils.send_bundle("2.1", "3.1", "2.0", "test_trace11")

        print("\n\nClear Node 3 Policy\n")
        testUtils.del_policy_rule(3, "25")
        testUtils.del_event_set(3, "d_integrity")

        # Verify results on ipn:3.1
        log3_events = testUtils.get_test_events(3, log3)
        te_1, log3_events = testUtils.find_test_event(
            log3_events, 'sop_missing_at_acceptor', 'ipn:2.1', 'ipn:3.1', 'bib-integrity', '0', None, None)

        # Process all test results
        testUtils.check_test_results([3], ["test_trace11"], [log3_events])
        '''

    except subprocess.TimeoutExpired as e:
        print(e)
        print("TEST FAILED")
        testUtils.g_tests_failed += 1
        os.chdir("..")

    print("###############################################################################")


def test12(inputScId=testUtils.BCB_AES_GCM_SCID, inputScParams=testUtils.bcbScParms):
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
                  "NOTE: Identification of missing security operations is not supported in the IOS 4.1.2 release.\n"
                  "Skipping test case.\n\n")
        time.sleep(testUtils.TIME_DISPLAYTEXT)

        '''
        # Tracking current position in log
        log3 = testUtils.log_position("3")

        print("Node 3.1 Configuration \n")
        testUtils.add_key(3, "bcb_key_32bytes")
        testUtils.add_event_set(3, "d_bcb_conf", "default bcb")

        desc = "bcb acceptor tgt:payload"
        filter = testUtils.build_filter("26", "a", "ipn:2.1", tgt=PAYLOAD)
        spec = testUtils.build_spec("bcb-confidentiality", sc_id=inputScId, sc_params=inputScParams)
        es_ref = "d_bcb_conf"
        testUtils.add_policy_rule(3, desc, filter, spec, es_ref)

        if testUtils.g_verbose == "detailed":
            testUtils.list_key(3)
            testUtils.list_event_set(3)
            testUtils.list_policy_rule(3)
        if testUtils.g_verbose == "verbose":
            testUtils.list_key(3)
            testUtils.info_event_set(3, "d_bcb_conf")
            testUtils.info_policy_rule(3, 26)

        # Send the bundle from ipn:2.1 -> ipn:3.1
        testUtils.send_bundle("2.1", "3.1", "2.0", "test_trace12")

        print("\n\nClear Node 3 Policy\n")
        testUtils.del_policy_rule(3, "26")
        testUtils.del_event_set(3, "d_bcb_conf")

         # Verify results on ipn:3.1
        log3_events = testUtils.get_test_events(3, log3)
        te_1, log3_events = testUtils.find_test_event(
            log3_events, 'sop_missing_at_acceptor', 'ipn:2.1', 'ipn:3.1', 'bcb-confidentiality', '0', None, None)

        # Process all test results
        testUtils.check_test_results([3], ["test_trace12"], [log3_events])
        '''

    except subprocess.TimeoutExpired as e:
        print(e)
        print("TEST FAILED")
        testUtils.g_tests_failed += 1
        os.chdir("..")

    print("###############################################################################")


def test13():
    print("###############################################################################")
    print("Test 13 not implemented. This test will be provided with the IOS 4.2 release.")
    print("###############################################################################")


def test14():
    print("###############################################################################")
    print("Test 14 not implemented. This test will be provided with the IOS 4.2 release.")
    print("###############################################################################")


def test15():
    print("###############################################################################")
    print("Test 15 not implemented. This test will be provided with the IOS 4.2 release.")
    print("###############################################################################")


def test16():
    print("###############################################################################")
    print("Test 16 not implemented. This test will be provided with the IOS 4.2 release.")
    print("###############################################################################")


def test17(inputScId=testUtils.BIB_HMAC_SHA2_SCID, inputSourceScParams=testUtils.bibScParms, inputAcceptorScParams=[["key_name", "key_2_32bytes"]]):
    print("###############################################################################")

    try:
        testUtils.start_test(
            "17", "Brief: Check that a Security Acceptor can identify a misconfigured BIB \n"
                  "due to use of mismatched keys. \n"
                  "Trigger the sop_misconf_at_acceptor event.\n"
                  "Intentional misconfiguration of security policy.\n\n"
                  "Description: Create a Security Source rule at ipn: 2.1 using key_1_32bytes (key_1_32bytes.hmk)\n"
                  "for a BIB on the Payload block and intentionally misconfigure the Security \n"
                  "Acceptor to use key_2_32bytes (key_2_32bytes.hmk). Check that the Security Acceptor \n"
                  "acknowledges the BIB misconfiguration.\n\n"
                  "Bundle Path: ipn:2.1 -> ipn:3.1\n\n")
        time.sleep(testUtils.TIME_DISPLAYTEXT)

        vers = testUtils.get_version(2)

        if vers == "ION-OPEN-SOURCE":
            print("This test (#17) is not used for ION Open Source testing as the \n"
            "ION TEST SUITE cipher suites do not generate cryptographic material such \n"
            "as integrity signatures that would be impacted by a key misconfiguration.\n")
            print("TEST PASSED")
            testUtils.g_tests_passed += 1
            return
        
        elif vers == "ION-NASA-BASELINE":

            # Tracking current position in log
            log2 = testUtils.log_position("2")
            log3 = testUtils.log_position("3")

            print("Node 2.1 Configuration \n")
            testUtils.add_key(2, "key_1_32bytes")
            testUtils.add_event_set(2, "d_integrity", "default bib")

            desc = "bib src rule tgt:payload"
            filter = testUtils.build_filter("1", "s", "ipn:2.1", tgt=PAYLOAD, sc_id=inputScId)
            spec = testUtils.build_spec("bib-integrity", sc_params=inputSourceScParams)
            es_ref = "d_integrity"
            testUtils.add_policy_rule(2, desc, filter, spec, es_ref)

            if testUtils.g_verbose == "detailed":
                testUtils.list_key(2)
                testUtils.list_event_set(2)
                testUtils.list_policy_rule(2)
            if testUtils.g_verbose == "verbose":
                testUtils.list_key(2)
                testUtils.info_event_set(2, "d_integrity")
                testUtils.info_policy_rule(2, 1)

            print("Node 3.1 Configuration \n")
            testUtils.add_key(3, "key_2_32bytes")
            testUtils.add_event_set(3, "d_integrity", "default bib")

            desc = "bib acceptor rule tgt:payload"
            filter = testUtils.build_filter("2", "a", "ipn:2.1", tgt=PAYLOAD)
            spec = testUtils.build_spec("bib-integrity", sc_id=inputScId, sc_params=inputAcceptorScParams)
            es_ref = "d_integrity"
            testUtils.add_policy_rule(3, desc, filter, spec, es_ref)

            if testUtils.g_verbose == "detailed":
                testUtils.list_key(3)
                testUtils.list_event_set(3)
                testUtils.list_policy_rule(3)
            if testUtils.g_verbose == "verbose":
                testUtils.list_key(3)
                testUtils.info_event_set(3, "d_integrity")
                testUtils.info_policy_rule(3, 2)

            # Send the bundle from ipn:2.1 -> ipn:3.1
            testUtils.send_bundle("2.1", "3.1", "2.0", "test_trace17")

            print("\n\nClear Node 2 Policy\n")
            testUtils.del_policy_rule(2, "1")
            testUtils.del_event_set(2, "d_integrity")
            testUtils.del_key(2, "key_1_32bytes")
            
            print("Clear Node 3 Policy\n")
            testUtils.del_policy_rule(3, "2")
            testUtils.del_event_set(3, "d_integrity")
            testUtils.del_key(3, "key_2_32bytes")

            # Verify results on ipn:2.1
            log2_events = testUtils.get_test_events(2, log2)
            te_1, log2_events = testUtils.find_test_event(
                log2_events, 'sop_added_at_src', 'ipn:2.1', 'ipn:3.1', 'bib-integrity', '1', None, None)

            # Verify results on ipn:3.1
            log3_events = testUtils.get_test_events(3, log3)
            te_2, log3_events = testUtils.find_test_event(
                log3_events, 'sop_misconf_at_acceptor', 'ipn:2.1', 'ipn:3.1', 'bib-integrity', '1', te_1['msec'], te_1['count'])

            # Process all test results
            testUtils.check_test_results([3], ["test_trace17"], [log2_events, log3_events])

        else:
            print("Invalid ION version detected.")
            print("TEST FAILED")
            testUtils.g_tests_failed += 1
            os.chdir("..")

    except subprocess.TimeoutExpired as e:
        print(e)
        print("TEST FAILED")
        testUtils.g_tests_failed += 1
        os.chdir("..")

    print("###############################################################################")


def test18(inputScId=testUtils.BCB_AES_GCM_SCID, inputSourceScParams=testUtils.bcbScParms, inputAcceptorScParams=[["key_name", "bcb_key_2_32bytes"]]):
    print("###############################################################################")

    try:
        testUtils.start_test(
            "18", "Brief: Check that a Security Acceptor can identify a misconfigured BCB \n"
                  "due to use of mismatched keys. \n"
                  "Trigger the sop_misconf_at_acceptor event.\n"
                  "Intentional misconfiguration of security policy.\n\n"
                  "Description: Create a Security Source rule at ipn: 2.1 using bcb_key_32bytes (bcb_key_32bytes.hmk)\n"
                  "for a BCB on the Payload block and intentionally misconfigure the Security \n"
                  "Acceptor to use bcb_key_2_32bytes (bcb_key_2_32bytes.hmk). Check that the Security Acceptor \n"
                  "acknowledges the BCB misconfiguration.\n\n"
                  "Bundle Path: ipn:2.1 -> ipn:3.1\n\n")
        time.sleep(testUtils.TIME_DISPLAYTEXT)

        vers = testUtils.get_version(2)

        if vers == "ION-OPEN-SOURCE":
            print("This test (#18) is not used for ION Open Source testing as the \n"
            "ION TEST SUITE cipher suites do not generate cryptographic material such \n"
            "as cipher text that would be impacted by a key misconfiguration.\n")
            print("TEST PASSED")
            testUtils.g_tests_passed += 1
            return
        
        elif vers == "ION-NASA-BASELINE":
        
            # Tracking current position in log
            log2 = testUtils.log_position("2")
            log3 = testUtils.log_position("3")

            print("Node 2.1 Configuration \n")
            testUtils.add_key(2, "bcb_key_32bytes")
            testUtils.add_event_set(2, "d_conf", "default bcb")

            desc = "bcb src rule tgt:payload"
            filter = testUtils.build_filter("1", "s", "ipn:2.1", tgt=PAYLOAD, sc_id=inputScId)
            spec = testUtils.build_spec("bcb-confidentiality", sc_params=inputSourceScParams)
            es_ref = "d_conf"
            testUtils.add_policy_rule(2, desc, filter, spec, es_ref)

            if testUtils.g_verbose == "detailed":
                testUtils.list_key(2)
                testUtils.list_event_set(2)
                testUtils.list_policy_rule(2)
            if testUtils.g_verbose == "verbose":
                testUtils.list_key(2)
                testUtils.info_event_set(2, "d_conf")
                testUtils.info_policy_rule(2, 1)

            print("Node 3.1 Configuration \n")
            testUtils.add_key(3, "bcb_key_2_32bytes")
            testUtils.add_event_set(3, "d_conf", "default bcb")

            desc = "bcb acceptor rule tgt:payload"
            filter = testUtils.build_filter("2", "a", "ipn:2.1", tgt=PAYLOAD)
            spec = testUtils.build_spec("bcb-confidentiality", sc_id=inputScId, sc_params=inputAcceptorScParams)
            es_ref = "d_conf"
            testUtils.add_policy_rule(3, desc, filter, spec, es_ref)

            if testUtils.g_verbose == "detailed":
                testUtils.list_key(3)
                testUtils.list_event_set(3)
                testUtils.list_policy_rule(3)
            if testUtils.g_verbose == "verbose":
                testUtils.list_key(3)
                testUtils.info_event_set(3, "d_conf")
                testUtils.info_policy_rule(3, 2)

            # Send the bundle from ipn:2.1 -> ipn:3.1
            testUtils.send_bundle("2.1", "3.1", "2.0", "test_trace18")

            print("\n\nClear Node 2 Policy\n")
            testUtils.del_policy_rule(2, "1")
            testUtils.del_event_set(2, "d_conf")
            testUtils.del_key(2, "bcb_key_32bytes")

            print("Clear Node 3 Policy\n")
            testUtils.del_policy_rule(3, "2")
            testUtils.del_event_set(3, "d_conf")
            testUtils.del_key(3, "bcb_key_2_32bytes")

            # Verify results on ipn:2.1
            log2_events = testUtils.get_test_events(2, log2)
            te_1, log2_events = testUtils.find_test_event(
                log2_events, 'sop_added_at_src', 'ipn:2.1', 'ipn:3.1', 'bcb-confidentiality', '1', None, None)

            # Verify results on ipn:3.1
            log3_events = testUtils.get_test_events(3, log3)
            te_2, log3_events = testUtils.find_test_event(
                log3_events, 'sop_misconf_at_acceptor', 'ipn:2.1', 'ipn:3.1', 'bcb-confidentiality', '1', te_1['msec'], te_1['count'])

            # Process all test results
            testUtils.check_test_results([3], ["test_trace18"], [log2_events, log3_events])

        else:
            print("Invalid ION version detected.")
            print("TEST FAILED")
            testUtils.g_tests_failed += 1  

    except subprocess.TimeoutExpired as e:
        print(e)
        print("TEST FAILED")
        testUtils.g_tests_failed += 1
        os.chdir("..")

    print("###############################################################################")


def test19(inputScId=testUtils.BIB_HMAC_SHA2_SCID, inputSourceScParams=testUtils.bibScParms, inputVerifierScParams=[["key_name", "key_2_32bytes"]]):
    print("###############################################################################")

    try:
        testUtils.start_test(
            "19", "Brief: Check that a Security Verifier can identify a misconfigured BIB \n"
                  "due to use of mismatched keys. \n"
                  "Trigger the sop_misconf_at_verifier event.\n"
                  "Intentional misconfiguration of security policy.\n\n"
                  "Description: Create a Security Source rule at ipn: 2.1 using key_1_32bytes (key_1_32bytes.hmk)\n"
                  "for a BIB on the Payload block and intentionally misconfigure the Security \n"
                  "Verifier to use key_2_32bytes (key_2_32bytes.hmk). Check that the Security Verifier \n"
                  "acknowledges the BIB misconfiguration.\n\n"
                  "Bundle Path: ipn:2.1 -> ipn:3.1 -> ipn:4.1 \n\n")
        time.sleep(testUtils.TIME_DISPLAYTEXT)

        vers = testUtils.get_version(2)

        if vers == "ION-OPEN-SOURCE":
            print("This test (#19) is not used for ION Open Source testing as the \n"
            "ION TEST SUITE cipher suites do not generate cryptographic material such \n"
            "as integrity signatures that would be impacted by a key misconfiguration.\n")
            print("TEST PASSED")
            testUtils.g_tests_passed += 1
            return
        
        elif vers == "ION-NASA-BASELINE":
        
            # Tracking current position in log
            log2 = testUtils.log_position("2")
            log3 = testUtils.log_position("3")
            log4 = testUtils.log_position("4")

            print("Node 2.1 Configuration \n")
            testUtils.add_key(2, "key_1_32bytes")
            testUtils.add_event_set(2, "d_integrity", "default bib")

            desc = "bib src rule tgt:payload"
            filter = testUtils.build_filter("1", "s", "ipn:2.1", tgt=PAYLOAD, sc_id=inputScId)
            spec = testUtils.build_spec("bib-integrity", sc_params=inputSourceScParams)
            es_ref = "d_integrity"
            testUtils.add_policy_rule(2, desc, filter, spec, es_ref)

            if testUtils.g_verbose == "detailed":
                testUtils.list_key(2)
                testUtils.list_event_set(2)
                testUtils.list_policy_rule(2)
            if testUtils.g_verbose == "verbose":
                testUtils.list_key(2)
                testUtils.info_event_set(2, "d_integrity")
                testUtils.info_policy_rule(2, 1)

            print("Node 3.1 Configuration \n")
            testUtils.add_key(3, "key_2_32bytes")
            testUtils.add_event_set(3, "d_integrity", "default bib")

            desc = "bib verifier rule tgt:payload"
            filter = testUtils.build_filter("2", "v", "ipn:2.1", tgt=PAYLOAD)
            spec = testUtils.build_spec("bib-integrity", sc_id=inputScId, sc_params=inputVerifierScParams)
            es_ref = "d_integrity"
            testUtils.add_policy_rule(3, desc, filter, spec, es_ref)

            if testUtils.g_verbose == "detailed":
                testUtils.list_key(3)
                testUtils.list_event_set(3)
                testUtils.list_policy_rule(3)
            if testUtils.g_verbose == "verbose":
                testUtils.list_key(3)
                testUtils.info_event_set(3, "d_integrity")
                testUtils.info_policy_rule(3, 2)

            # Send the bundle from ipn:2.1 -> ipn:3.1 -> ipn:4.1
            testUtils.send_bundle("2.1", "4.1", "2.0", "test_trace19")

            print("\n\nClear Node 2 Policy\n")
            testUtils.del_policy_rule(2, "1")
            testUtils.del_event_set(2, "d_integrity")
            testUtils.del_key(2, "key_1_32bytes")

            print("Clear Node 3 Policy\n")
            testUtils.del_policy_rule(3, "2")
            testUtils.del_event_set(3, "d_integrity")
            testUtils.del_key(3, "key_2_32bytes")

            # Verify results on ipn:2.1
            log2_events = testUtils.get_test_events(2, log2)
            te_1, log2_events = testUtils.find_test_event(
                log2_events, 'sop_added_at_src', 'ipn:2.1', 'ipn:4.1', 'bib-integrity', '1', None, None)

            # Verify results on ipn:3.1
            log3_events = testUtils.get_test_events(3, log3)
            te_2, log3_events = testUtils.find_test_event(
                log3_events, 'sop_misconf_at_verifier', 'ipn:2.1', 'ipn:4.1', 'bib-integrity', '1', te_1['msec'], te_1['count'])

            # Process all test results
            testUtils.check_test_results([4], ["test_trace19"], [log2_events, log3_events])


        else:
            print("Invalid ION version detected.")
            print("TEST FAILED")
            testUtils.g_tests_failed += 1

    except subprocess.TimeoutExpired as e:
        print(e)
        print("TEST FAILED")
        testUtils.g_tests_failed += 1
        os.chdir("..")

    print("###############################################################################")


def test20(inputScId=testUtils.BCB_AES_GCM_SCID, inputSourceScParams=testUtils.bcbScParms, inputVerifierScParams=[["key_name", "bcb_key_2_32bytes"]]):
    print("###############################################################################")
    try:
        testUtils.start_test(
            "20", "Brief: Check that a Security Verifier can identify a misconfigured BCB \n"
                  "due to use of mismatched keys. \n"
                  "Trigger the sop_misconf_at_verifier event.\n"
                  "Intentional misconfiguration of security policy.\n\n"
                  "Description: Create a Security Source rule at ipn: 2.1 using bcb_key_32bytes (bcb_key_32bytes.hmk)\n"
                  "for a BCB on the Payload block and intentionally misconfigure the Security \n"
                  "Verifier to use bcb_key_2_32bytes (bcb_key_2_32bytes.hmk). Check that the Security Verifier \n"
                  "acknowledges the BCB misconfiguration.\n\n"
                  "Bundle Path: ipn:2.1 -> ipn:3.1 -> ipn:4.1\n\n"
                  "NOTE: The Security Verifier role for bcb-confidentiality is not supported in the IOS 4.1.2 release.\n"
                  "Skipping test case.\n\n")

        time.sleep(testUtils.TIME_DISPLAYTEXT)
           
        '''
        This test is not needed for IOS 4.1.2 testing as the Security Verifier role
        for bcb-confidentiality has not been implemented yet. 
        '''

        '''
        # Tracking current position in log
        log2 = testUtils.log_position("2")
        log3 = testUtils.log_position("3")
        log4 = testUtils.log_position("4")

        print("Node 2.1 Configuration \n")
        testUtils.add_key(2, "bcb_key_32bytes")
        testUtils.add_event_set(2, "d_conf", "default bcb")

        desc = "bcb src rule tgt:payload"
        filter = testUtils.build_filter("1", "s", "ipn:2.1", tgt=PAYLOAD)
        spec = testUtils.build_spec("bcb-confidentiality", testUtils.BCB_AES_GCM_SCID, testUtils.bcbScParms)
        es_ref = "d_conf"
        testUtils.add_policy_rule(2, desc, filter, spec, es_ref)

        print("Node 3.1 Configuration \n")
        testUtils.add_key(3, "bcb_key_2_32bytes")
        testUtils.add_event_set(3, "d_conf", "default bcb")

        desc = "bcb verifier rule tgt:payload"
        filter = testUtils.build_filter("2", "v", "ipn:2.1", tgt=PAYLOAD)
        spec = testUtils.build_spec("bcb-confidentiality", testUtils.BCB_AES_GCM_SCID, [["key_name", "bcb_key_2_32bytes"]])
        es_ref = "d_conf"
        testUtils.add_policy_rule(3, desc, filter, spec, es_ref)

        # Send the bundle from ipn:2.1 -> ipn:3.1 -> ipn:4.1
        testUtils.send_bundle("2.1", "4.1", "2.0", "test_trace20")

        print("\n\nClear Node 2 Policy\n")
        testUtils.del_policy_rule(2, "1")
        testUtils.del_event_set(2, "d_conf")
        testUtils.del_key(2, "bcb_key_32bytes")

        print("Clear Node 3 Policy\n")
        testUtils.del_policy_rule(3, "2")
        testUtils.del_event_set(3, "d_conf")
        testUtils.del_key(3, "bcb_key_2_32bytes")

        te_1 = testUtils.find_test_event(
            'sop_added_at_src', 'ipn:2.1', 'ipn:4.1', 'bcb-confidentiality', '1', None, None, '2', log2)
        te_2 = testUtils.find_test_event(
            'sop_corrupt_at_verifier', 'ipn:2.1', 'ipn:4.1', 'bcb-confidentiality', '1', te_1['msec'], te_1['count'],
            '3', log3)

        testUtils.check_test_results(4, "", False, [te_1, te_2])

        '''
    except subprocess.TimeoutExpired as e:
        print(e)
        print("TEST FAILED")
        testUtils.g_tests_failed += 1
        os.chdir("..")

        
    print("###############################################################################")


def test21(inputScId=testUtils.BIB_HMAC_SHA2_SCID, inputScParams=testUtils.bibScParms):
    print("###############################################################################")

    try:
        testUtils.start_test(
            "21", "Brief: Multi-bundle test.\n\n"
                  "Description: Configure node ipn:2.1 to serve as a security source for\n"
                  "bib-integrity targeting the payload block regardless of bundle\n"
                  "destination. Configure node ipn:3.1 to serve as the security acceptor for\n"
                  "Bundle 1 and node ipn:4.1 to serve as the security acceptor for Bundle 2.\n"
                  "Ensure that BIB processing events are handled on a per-bundle basis by \n"
                  "the test framework.\n\n"
                  "Bundle 1 Path: ipn:2.1 -> ipn:4.1\n"
                  "Bundle 2 Path: ipn:2.1 -> ipn:3.1\n\n")
        time.sleep(testUtils.TIME_DISPLAYTEXT)

        # Tracking current position in log
        log2 = testUtils.log_position("2")
        log3 = testUtils.log_position("3")
        log4 = testUtils.log_position("4")

        print("Node ipn:2.1 Configuration\n")
        testUtils.add_key(2, "key_1_32bytes")
        testUtils.add_event_set(2, "d_integrity", "default bib")

        desc = "bib src tgt:payload"
        filter = testUtils.build_filter("10", "s", "ipn:2.1", tgt=PAYLOAD, sc_id=inputScId)
        spec = testUtils.build_spec("bib-integrity", sc_params=inputScParams)
        es_ref = "d_integrity"
        testUtils.add_policy_rule(2, desc, filter, spec, es_ref)

        if testUtils.g_verbose == "detailed":
            testUtils.list_key(2)
            testUtils.list_event_set(2)
            testUtils.list_policy_rule(2)
        if testUtils.g_verbose == "verbose":
            testUtils.list_key(2)
            testUtils.info_event_set(2, "d_integrity")
            testUtils.info_policy_rule(2, 10)

        print("Node ipn:3.1 Configuration \n")
        testUtils.add_key(3, "key_1_32bytes")
        testUtils.add_event_set(3, "d_integrity", "default bib")

        desc = "bib acceptor tgt:payload"
        filter = testUtils.build_filter("11", "a", dest="ipn:3.1", tgt=PAYLOAD)
        spec = testUtils.build_spec("bib-integrity", sc_id=inputScId, sc_params=inputScParams)
        es_ref = "d_integrity"
        testUtils.add_policy_rule(3, desc, filter, spec, es_ref)

        if testUtils.g_verbose == "detailed":
            testUtils.list_key(3)
            testUtils.list_event_set(3)
            testUtils.list_policy_rule(3)
        if testUtils.g_verbose == "verbose":
            testUtils.list_key(3)
            testUtils.info_event_set(3, "d_integrity")
            testUtils.info_policy_rule(3, 11)

        print("Node ipn:4.1 Configuration\n")
        testUtils.add_key(4, "key_1_32bytes")
        testUtils.add_event_set(4, "d_integrity", "default bib")

        desc = "bib acceptor for dest ipn:4.1"
        filter = testUtils.build_filter("12", "a", dest="ipn:4.1", tgt=PAYLOAD)
        spec = testUtils.build_spec("bib-integrity", sc_id=inputScId, sc_params=inputScParams)
        es_ref = "d_integrity"
        testUtils.add_policy_rule(4, desc, filter, spec, es_ref)

        if testUtils.g_verbose == "detailed":
            testUtils.list_key(4)
            testUtils.list_event_set(4)
            testUtils.list_policy_rule(4)
        if testUtils.g_verbose == "verbose":
            testUtils.list_key(4)
            testUtils.info_event_set(4, "d_integrity")
            testUtils.info_policy_rule(4, 12)

        # Send Bundle 1 from ipn:2.1 -> ipn:4.1
        testUtils.send_bundle("2.1", "4.1", "2.0", "test_trace21_1")

        # Send Bundle 2 from ipn:2.1 -> ipn:3.1
        testUtils.send_bundle("2.1", "3.1", "2.0", "test_trace21_2")

        print("\n\nClear Node ipn:2.1 Policy \n")
        testUtils.del_policy_rule(2, "10")
        testUtils.del_event_set(2, "d_integrity")
        testUtils.del_key(2, "key_1_32bytes")

        print("Clear Node ipn:3.1 Policy \n")
        testUtils.del_policy_rule(3, "11")
        testUtils.del_event_set(3, "d_integrity")
        testUtils.del_key(3, "key_1_32bytes")

        print("Clear Node ipn:4.1 Policy \n\n")
        testUtils.del_policy_rule(4, "12")
        testUtils.del_event_set(4, "d_integrity")
        testUtils.del_key(4, "key_1_32bytes")
        
        # Verify results on ipn:2.1
        log2_events = testUtils.get_test_events(2, log2)
        te_1, log2_events = testUtils.find_test_event(
            log2_events, 'sop_added_at_src', 'ipn:2.1', 'ipn:4.1', 'bib-integrity', '1', None, None)
        te_2, log2_events = testUtils.find_test_event(
            log2_events, 'sop_added_at_src', 'ipn:2.1', 'ipn:3.1', 'bib-integrity', '1', None, None)
        
        # Verify results on ipn:3.1
        log3_events = testUtils.get_test_events(3, log3)
        te_3, log3_events = testUtils.find_test_event(
            log3_events, 'sop_processed', 'ipn:2.1', 'ipn:3.1', 'bib-integrity', '1', te_2['msec'], te_2['count'])

        # Verify results on ipn:4.1
        log4_events = testUtils.get_test_events(4, log4)
        te_4, log4_events = testUtils.find_test_event(
            log4_events, 'sop_processed', 'ipn:2.1', 'ipn:4.1', 'bib-integrity', '1', te_1['msec'], te_1['count'])

        # Process all test results
        testUtils.check_test_results([4, 3], ["test_trace21_1", "test_trace21_2"], [log2_events, log3_events, log4_events])

    except subprocess.TimeoutExpired as e:
        print(e)
        print("TEST FAILED")
        testUtils.g_tests_failed += 1
        os.chdir("..")

    print("###############################################################################")


def test22(inputBibScId=testUtils.BIB_HMAC_SHA2_SCID, inputBcbScId= testUtils.BCB_AES_GCM_SCID, inputScParams=testUtils.bibScParms):
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
        time.sleep(testUtils.TIME_DISPLAYTEXT)

        
        vers = testUtils.get_version(2)

        if vers == "ION-OPEN-SOURCE":
            print("This test (#22) is not used for ION Open Source testing as the \n"
            "ION TEST SUITE supports both confidentiality and integrity operations and \n"
            "will not be impacted by a security context \"misconfiguration\".\n")
            return
        
        elif vers == "ION-NASA-BASELINE":
        
            # Tracking current position in log
            log2 = testUtils.log_position("2")
            log3 = testUtils.log_position("3")

            print("Node 2.1 Configuration\n")
            testUtils.add_key(2, "key_1_32bytes")
            testUtils.add_event_set(2, "d_integrity", "default bib")

            desc = "bib src tgt:payload"
            filter = testUtils.build_filter("1", "s", "ipn:2.1", tgt=PAYLOAD, sc_id=inputBibScId)
            spec = testUtils.build_spec("bib-integrity", sc_params=inputScParams)
            es_ref = "d_integrity"
            testUtils.add_policy_rule(2, desc, filter, spec, es_ref)

            if testUtils.g_verbose == "detailed":
                testUtils.list_key(2)
                testUtils.list_event_set(2)
                testUtils.list_policy_rule(2)
            if testUtils.g_verbose == "verbose":
                testUtils.list_key(2)
                testUtils.info_event_set(2, "d_integrity")
                testUtils.info_policy_rule(2, 1)

            print("Node 3.1 Configuration \n")
            testUtils.add_key(3, "key_1_32bytes")
            testUtils.add_event_set(3, "d_integrity", "default bib")

            desc = "bib acceptor tgt:payload"
            filter = testUtils.build_filter("2", "a", "ipn:2.1", tgt=PAYLOAD)
            spec = testUtils.build_spec("bib-integrity", sc_id=inputBcbScId, sc_params=inputScParams)
            es_ref = "d_integrity"
            testUtils.add_policy_rule(3, desc, filter, spec, es_ref)

            if testUtils.g_verbose == "detailed":
                testUtils.list_key(3)
                testUtils.list_event_set(3)
                testUtils.list_policy_rule(3)
            if testUtils.g_verbose == "verbose":
                testUtils.list_key(3)
                testUtils.info_event_set(3, "d_integrity")
                testUtils.info_policy_rule(3, 2)

            # Send the bundle from ipn:2.1 -> ipn:3.1
            testUtils.send_bundle("2.1", "3.1", "2.0", "test_trace22")

            print("\n\nClear Node 2 Policy \n")
            testUtils.del_policy_rule(2, "1")
            testUtils.del_event_set(2, "d_integrity")

            print("Clear Node 3 Policy\n")
            testUtils.del_policy_rule(3, "2")
            testUtils.del_event_set(3, "d_integrity")

            # Verify results on ipn:2.1
            log2_events = testUtils.get_test_events(2, log2)
            te_1, log2_events = testUtils.find_test_event(
                log2_events, 'sop_added_at_src', 'ipn:2.1', 'ipn:3.1', 'bib-integrity', '1', None, None)

            # Verify results on ipn:3.1
            log3_events = testUtils.get_test_events(3, log3)
            te_2, log3_events = testUtils.find_test_event(
                log3_events, 'sop_misconf_at_acceptor', 'ipn:2.1', 'ipn:3.1', 'bib-integrity', '1', te_1['msec'], te_1['count'])

            # Process all test results
            testUtils.check_test_results([3], ["test_trace22"], [log2_events, log3_events])

        else:
            print("Invalid ION version detected.")
            print("TEST FAILED")
            testUtils.g_tests_failed += 1
        
    except subprocess.TimeoutExpired as e:
        print(e)
        print("TEST FAILED")
        testUtils.g_tests_failed += 1
        os.chdir("..")

    print("###############################################################################")


def test23(inputBibScId=testUtils.BIB_HMAC_SHA2_SCID, inputBcbScId= testUtils.BCB_AES_GCM_SCID, inputScParams=testUtils.bcbScParms):
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
        time.sleep(testUtils.TIME_DISPLAYTEXT)

        vers = testUtils.get_version(2)

        if vers == "ION-OPEN-SOURCE":
            print("This test (#23) is not used for ION Open Source testing as the \n"
            "ION TEST SUITE supports both confidentiality and integrity operations and \n"
            "will not be impacted by a security context \"misconfiguration\".\n")
            return

        elif vers == "ION-NASA-BASELINE":

            # Tracking current position in log
            log2 = testUtils.log_position("2")
            log3 = testUtils.log_position("3")

            print("Node 2.1 Configuration\n")
            testUtils.add_key(2, "bcb_key_32bytes")
            testUtils.add_event_set(2, "d_bcb_conf", "default bcb")

            desc = "bcb src tgt:payload"
            filter = testUtils.build_filter("4", "s", "ipn:2.1", tgt=PAYLOAD, sc_id=inputBcbScId)
            spec = testUtils.build_spec("bcb-confidentiality", sc_params=inputScParams)
            es_ref = "d_bcb_conf"
            testUtils.add_policy_rule(2, desc, filter, spec, es_ref)

            if testUtils.g_verbose == "detailed":
                testUtils.list_key(2)
                testUtils.list_event_set(2)
                testUtils.list_policy_rule(2)
            if testUtils.g_verbose == "verbose":
                testUtils.list_key(2)
                testUtils.info_event_set(2, "d_bcb_conf")
                testUtils.info_policy_rule(2, 4)

            print("Node 3.1 Configuration \n")
            testUtils.add_key(3, "bcb_key_32bytes")
            testUtils.add_event_set(3, "d_bcb_conf", "default bcb")

            desc = "bcb acceptor tgt:payload"
            filter = testUtils.build_filter("5", "a", "ipn:2.1", tgt=PAYLOAD)
            spec = testUtils.build_spec("bcb-confidentiality", sc_id=inputBibScId, sc_params=inputScParams)
            es_ref = "d_bcb_conf"
            testUtils.add_policy_rule(3, desc, filter, spec, es_ref)

            if testUtils.g_verbose == "detailed":
                testUtils.list_key(3)
                testUtils.list_event_set(3)
                testUtils.list_policy_rule(3)
            if testUtils.g_verbose == "verbose":
                testUtils.list_key(3)
                testUtils.info_event_set(3, "d_bcb_conf")
                testUtils.info_policy_rule(3, 5)

            # Send the bundle from ipn:2.1 -> ipn:3.1
            testUtils.send_bundle("2.1", "3.1", "2.0", "test_trace23")

            print("\n\nClear Node 2 Policy \n")
            testUtils.del_policy_rule(2, "4")
            testUtils.del_event_set(2, "d_bcb_conf")

            print("Clear Node 3 Policy\n")
            testUtils.del_policy_rule(3, "5")
            testUtils.del_event_set(3, "d_bcb_conf")

            # Verify results on ipn:2.1
            log2_events = testUtils.get_test_events(2, log2)
            te_1, log2_events = testUtils.find_test_event(
                log2_events, 'sop_added_at_src', 'ipn:2.1', 'ipn:3.1', 'bcb-confidentiality', '1', None, None)

            # Verify results on ipn:3.1
            log3_events = testUtils.get_test_events(3, log3)
            te_2, log3_events = testUtils.find_test_event(
                log3_events, 'sop_misconf_at_acceptor', 'ipn:2.1', 'ipn:3.1', 'bcb-confidentiality', '1', te_1['msec'], te_1['count'])

            # Process all test results
            testUtils.check_test_results([3], ["test_trace23"], [log2_events, log3_events])

        else:
            print("Invalid ION version detected.")
            print("TEST FAILED")
            testUtils.g_tests_failed += 1

    except subprocess.TimeoutExpired as e:
        print(e)
        print("TEST FAILED")
        testUtils.g_tests_failed += 1
        os.chdir("..")

    print("###############################################################################")


def test24(inputScId=testUtils.BCB_AES_GCM_SCID, inputScParams=testUtils.bcbScParms):
    print("###############################################################################")

    try:
        testUtils.start_test(
            "24", "Brief: Misconfigure security policy to require a BCB targeting another BCB. \n"
                  "Intentional misconfiguration of security policy.\n\n"
                  "Description: Create a security source rule at ipn:2.1 requiring\n"
                  "a BCB on the Payload Block. Add a second security source rule at\n"
                  "the same node requiring a BCB targeting the first one created.\n"
                  "Expect to see a BCB on the Payload Block only, as a BCB targeting\n"
                  "another BCB is prohibited by BPSec.\n\n"
                  "Bundle Path: ipn:2.1 -> ipn:3.1\n\n"
                  "NOTE: This security block interaction is not supported in the IOS 4.1.2 release.\n"
                  "Skipping test case.\n\n")

        time.sleep(testUtils.TIME_DISPLAYTEXT)

        '''
        # Tracking current position in log
        log2 = testUtils.log_position("2")
        log3 = testUtils.log_position("3")

        print("Node ipn:2.1 Configuration\n")
        testUtils.add_key(2, "bcb_key_32bytes")
        testUtils.add_event_set(2, "d_bcb_conf", "default bcb")

        desc = "bcb src rule tgt:payload"
        filter = testUtils.build_filter("21", "s", "ipn:2.1", tgt=PAYLOAD, sc_id=inputScId)
        spec = testUtils.build_spec("bcb-confidentiality", sc_params=inputScParams)
        es_ref = "d_bcb_conf"
        testUtils.add_policy_rule(2, desc, filter, spec, es_ref)

        desc = "bcb src rule tgt:bcb"
        filter = testUtils.build_filter("22", "s", "ipn:2.1", tgt=BCB, sc_id=inputScId)
        spec = testUtils.build_spec("bcb-confidentiality", sc_params=inputScParams)
        es_ref = "d_bcb_conf"
        testUtils.add_policy_rule(2, desc, filter, spec, es_ref)

        if testUtils.g_verbose == "detailed":
            testUtils.list_key(2)
            testUtils.list_event_set(2)
            testUtils.list_policy_rule(2)
        if testUtils.g_verbose == "verbose":
            testUtils.list_key(2)
            testUtils.info_event_set(2, "d_bcb_conf")
            testUtils.info_policy_rule(2, 21)
            testUtils.info_policy_rule(2, 22)

        print("Node ipn:3.1 Configuration \n")
        testUtils.add_key(3, "bcb_key_32bytes")
        testUtils.add_event_set(3, "d_bcb_conf", "default bcb")

        desc = "bcb acceptor rule tgt:bcb"
        filter = testUtils.build_filter("24", "a", "ipn:2.1", tgt=BCB)
        spec = testUtils.build_spec("bcb-confidentiality", sc_id=inputScId, sc_params=inputScParams)
        es_ref = "d_bcb_conf"
        testUtils.add_policy_rule(3, desc, filter, spec, es_ref)

        desc = "bcb acceptor rule tgt:payload"
        filter = testUtils.build_filter("23", "a", "ipn:2.1", tgt=PAYLOAD)
        spec = testUtils.build_spec("bcb-confidentiality", sc_id=inputScId, sc_params=inputScParams)
        es_ref = "d_bcb_conf"
        testUtils.add_policy_rule(3, desc, filter, spec, es_ref)

        if testUtils.g_verbose == "detailed":
            testUtils.list_key(3)
            testUtils.list_event_set(3)
            testUtils.list_policy_rule(3)
        if testUtils.g_verbose == "verbose":
            testUtils.list_key(3)
            testUtils.info_event_set(3, "d_bcb_conf")
            testUtils.info_policy_rule(3, 24)
            testUtils.info_policy_rule(3, 23)

        # Sending the bundle from ipn:2.1 -> ipn:3.1
        testUtils.send_bundle("2.1", "3.1", "2.0", "test_trace24")

        print("\n\nClear Node ipn:2.1 Policy \n")
        testUtils.del_policy_rule(2, "21")
        testUtils.del_policy_rule(2, "22")
        testUtils.del_event_set(2, "d_bcb_conf")
        testUtils.del_key(2, "bcb_key_32bytes")

        print("Clear Node ipn:3.1 Policy\n")
        testUtils.del_policy_rule(3, "23")
        testUtils.del_policy_rule(3, "24")
        testUtils.del_event_set(3, "d_bcb_conf")
        testUtils.del_key(3, "bcb_key_32bytes")

        # Verify results on ipn:2.1
        log2_events = testUtils.get_test_events(2, log2)
        te_1, log2_events = testUtils.find_test_event(
            log2_events, 'sop_added_at_src', 'ipn:2.1', 'ipn:3.1', 'bcb-confidentiality', '1', None, None)

        # Verify results on ipn:3.1
        log3_events = testUtils.get_test_events(3, log3)
        te_2, log3_events = testUtils.find_test_event(
            log3_events, 'sop_processed', 'ipn:2.1', 'ipn:3.1', 'bcb-confidentiality', '1', te_1['msec'], te_1['count'])

        # Process all test results
        testUtils.check_test_results([3], ["test_trace24"], [log2_events, log3_events])
        '''

    except subprocess.TimeoutExpired as e:
        print(e)
        print("TEST FAILED")
        testUtils.g_tests_failed += 1
        os.chdir("..")

    print("###############################################################################")


def test25(inputBcbScId=testUtils.BCB_AES_GCM_SCID, inputBibScId=testUtils.BIB_HMAC_SHA2_SCID, 
inputBcbScParams=testUtils.bcbScParms, inputBibScParams=testUtils.bibScParms):
    print("###############################################################################")

    try:
        testUtils.start_test(
            "25", "Brief: BIB and BCB both targeting the Payload Block. \n\n"
                  "Description: Create a security source rule at ipn:2.1 requiring\n"
                  "a BCB on the Payload Block. Add a second security source rule at\n"
                  "the same node requiring a BIB targeting the Payload Block as well.\n"
                  "Add security acceptor rules at ipn:3.1 for both the BIB and BCB.\n\n"
                  "Bundle Path: ipn:2.1 -> ipn:3.1\n\n"
                  "NOTE: This security block interaction is not supported in the IOS 4.1.2 release.\n"
                  "Skipping test case.\n\n")

        time.sleep(testUtils.TIME_DISPLAYTEXT)

        '''
        # Tracking current position in log
        log2 = testUtils.log_position("2")
        log3 = testUtils.log_position("3")

        print("Node 2.1 Configuration\n")
        testUtils.add_key(2, "bcb_key_32bytes")
        testUtils.add_event_set(2, "d_bcb_conf", "default bcb")

        desc = "bcb src tgt:payload"
        filter = testUtils.build_filter("21", "s", "ipn:2.1", tgt=PAYLOAD, sc_id=inputBcbScId)
        spec = testUtils.build_spec("bcb-confidentiality", sc_params=inputBcbScParams)
        es_ref = "d_bcb_conf"
        testUtils.add_policy_rule(2, desc, filter, spec, es_ref)

        testUtils.add_key(2, "key_1_32bytes")
        testUtils.add_event_set(2, "d_integrity", "default bib")

        desc = "bib src tgt:payload"
        filter = testUtils.build_filter("20", "s", "ipn:2.1", tgt=PAYLOAD, sc_id=inputBibScId)
        spec = testUtils.build_spec("bib-integrity", sc_params=inputBibScParams)
        es_ref = "d_integrity"
        testUtils.add_policy_rule(2, desc, filter, spec, es_ref)

        if testUtils.g_verbose == "detailed":
            testUtils.list_key(2)
            testUtils.list_event_set(2)
            testUtils.list_policy_rule(2)
        if testUtils.g_verbose == "verbose":
            testUtils.list_key(2)
            testUtils.info_event_set(2, "d_integrity")
            testUtils.info_policy_rule(2, 21)
            testUtils.info_policy_rule(2, 20)

        print("Node 3.1 Configuration \n")
        testUtils.add_key(3, "bcb_key_32bytes")
        testUtils.add_event_set(3, "d_bcb_conf", "default bcb")

        desc = "bcb acceptor tgt:payload"
        filter = testUtils.build_filter("23", "a", "ipn:2.1", tgt=PAYLOAD)
        spec = testUtils.build_spec("bcb-confidentiality", sc_id=inputBcbScId, sc_params=inputBcbScParams)
        es_ref = "d_bcb_conf"
        testUtils.add_policy_rule(3, desc, filter, spec, es_ref)

        testUtils.add_key(3, "key_1_32bytes")
        testUtils.add_event_set(3, "d_integrity", "default bib")

        desc = "bib acceptor tgt:payload"
        filter = testUtils.build_filter("22", "a", "ipn:2.1", tgt=PAYLOAD)
        spec = testUtils.build_spec("bib-integrity", sc_id=inputBibScId, sc_params=inputBibScParams)
        es_ref = "d_integrity"
        testUtils.add_policy_rule(3, desc, filter, spec, es_ref)

        if testUtils.g_verbose == "detailed":
            testUtils.list_key(3)
            testUtils.list_event_set(3)
            testUtils.list_policy_rule(3)
        if testUtils.g_verbose == "verbose":
            testUtils.list_key(3)
            testUtils.info_event_set(3, "d_integrity")
            testUtils.info_policy_rule(3, 23)
            testUtils.info_policy_rule(3, 22)

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

        # Verify results on ipn:2.1
        log2_events = testUtils.get_test_events(2, log2)
        te_1, log2_events = testUtils.find_test_event(
            log2_events, 'sop_added_at_src', 'ipn:2.1', 'ipn:3.1', 'bcb-confidentiality', '1', None, None)
        te_2, log2_events = testUtils.find_test_event(
            log2_events, 'sop_added_at_src', 'ipn:2.1', 'ipn:3.1', 'bib-integrity', '1', None, None)

        # Verify results on ipn:3.1
        log3_events = testUtils.get_test_events(3, log3)
        te_3, log3_events = testUtils.find_test_event(
            log3_events, 'sop_processed', 'ipn:2.1', 'ipn:3.1', 'bcb-confidentiality', '1', te_1['msec'], te_1['count'])
        te_4, log3_events = testUtils.find_test_event(
            log3_events, 'sop_processed', 'ipn:2.1', 'ipn:3.1', 'bib-integrity', '1', te_1['msec'], te_1['count'])

        # Process all test results
        testUtils.check_test_results([3], ["test_trace25"], [log2_events, log3_events])
        '''

    except subprocess.TimeoutExpired as e:
        print(e)
        print("TEST FAILED")
        testUtils.g_tests_failed += 1
        os.chdir("..")

    print("###############################################################################")


def test26(inputBcbScId=testUtils.BCB_AES_GCM_SCID, inputBibScId=testUtils.BIB_HMAC_SHA2_SCID, 
inputBcbScParams=testUtils.bcbScParms, inputBibScParams=testUtils.bibScParms):
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
                  "NOTE: This security block interaction is not supported in the IOS 4.1.2 release.\n"
                  "Skipping test case.\n\n")
        time.sleep(testUtils.TIME_DISPLAYTEXT)

        '''
        # Tracking current position in log
        log2 = testUtils.log_position("2")
        log3 = testUtils.log_position("3")
        log4 = testUtils.log_position("4")

        print("Node 2.1 Configuration\n")
        testUtils.add_key(2, "bcb_key_32bytes")
        testUtils.add_event_set(2, "d_bcb_conf", "default bcb")

        desc = "bcb src rule tgt:payload"
        filter = testUtils.build_filter("1", "s", "ipn:2.1", tgt=PAYLOAD, sc_id=inputBcbScId)
        spec = testUtils.build_spec("bcb-confidentiality", sc_params=inputBcbScParams)
        es_ref = "d_bcb_conf"
        testUtils.add_policy_rule(2, desc, filter, spec, es_ref)

        if testUtils.g_verbose == "detailed":
            testUtils.list_key(2)
            testUtils.list_event_set(2)
            testUtils.list_policy_rule(2)
        if testUtils.g_verbose == "verbose":
            testUtils.list_key(2)
            testUtils.info_event_set(2, "d_bcb_conf")
            testUtils.info_policy_rule(2, 1)

        print("Node 3.1 Configuration \n")
        testUtils.add_key(3, "key_1_32bytes")
        testUtils.add_event_set(3, "d_integrity", "default bib")

        desc = "bib src rule tgt:payload"
        filter = testUtils.build_filter("2", "s", "ipn:2.1", tgt=PAYLOAD, sc_id=inputBibScId)
        spec = testUtils.build_spec("bib-integrity", sc_params=inputBibScParams)
        es_ref = "d_integrity"
        testUtils.add_policy_rule(3, desc, filter, spec, es_ref)

        if testUtils.g_verbose == "detailed":
            testUtils.list_key(3)
            testUtils.list_event_set(3)
            testUtils.list_policy_rule(3)
        if testUtils.g_verbose == "verbose":
            testUtils.list_key(3)
            testUtils.info_event_set(3, "d_integrity")
            testUtils.info_policy_rule(3, 2)

        print("Node 4.1 Configuration \n")
        testUtils.add_key(4, "bcb_key_32bytes")
        testUtils.add_event_set(3, "d_bcb_conf", "default bcb")

        desc = "bcb acceptor rule tgt:payload"
        filter = testUtils.build_filter("3", "a", "ipn:2.1", tgt=PAYLOAD)
        spec = testUtils.build_spec("bcb-confidentiality", sc_id=inputBcbScId, sc_params=inputBcbScParams)
        es_ref = "d_bcb_conf"
        testUtils.add_policy_rule(4, desc, filter, spec, es_ref)

        testUtils.add_key(4, "key_1_32bytes")
        testUtils.add_event_set(4, "d_integrity", "default bib")

        desc = "bib acceptor rule tgt:payload"
        filter = testUtils.build_filter("4", "a", "ipn:2.1", tgt=PAYLOAD)
        spec = testUtils.build_spec("bib-integrity", sc_id = inputBibScId, sc_params=inputBibScParams)
        es_ref = "d_integrity"
        testUtils.add_policy_rule(4, desc, filter, spec, es_ref)

        if testUtils.g_verbose == "detailed":
            testUtils.list_key(4)
            testUtils.list_event_set(4)
            testUtils.list_policy_rule(4)
        if testUtils.g_verbose == "verbose":
            testUtils.list_key(4)
            testUtils.info_event_set(4, "d_integrity")
            testUtils.info_policy_rule(4, 3)
            testUtils.info_policy_rule(4, 4)

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

        # Verify results on ipn:2.1
        log2_events = testUtils.get_test_events(2, log2)
        te_1, log2_events = testUtils.find_test_event(
            log2_events, 'sop_added_at_src', 'ipn:2.1', 'ipn:4.1', 'bcb-confidentiality', '1', None, None)

        # Verify results on ipn:4.1
        log4_events = testUtils.get_test_events(4, log4)
        te_2, log4_events = testUtils.find_test_event(
            log4_events, 'sop_processed', 'ipn:2.1', 'ipn:4.1', 'bcb-confidentiality', '1', te_1['msec'], te_1['count'])

        # Process all test results
        testUtils.check_test_results([4], ["test_trace26"], [log2_events, log4_events])
        '''

    except subprocess.TimeoutExpired as e:
        print(e)
        print("TEST FAILED")
        testUtils.g_tests_failed += 1
        os.chdir("..")

    print("##############################################################################")


def test27(inputScId=testUtils.BIB_HMAC_SHA2_SCID, inputScParams=testUtils.bibScParms):
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
                  "NOTE: This security block interaction is not supported in the IOS 4.1.2 release.\n"
                  "Skipping test case.\n\n")
        time.sleep(testUtils.TIME_DISPLAYTEXT)

        '''
        # Tracking current position in log
        log2 = testUtils.log_position("2")
        log3 = testUtils.log_position("3")

        print("Node 2.1 Configuration\n")
        testUtils.add_key(2, "key_1_32bytes")
        testUtils.add_event_set(2, "d_integrity", "default bib")

        desc = "bib src rule tgt:payload"
        filter = testUtils.build_filter("50", "s", "ipn:2.1", tgt=PAYLOAD, sc_id=inputScId)
        spec = testUtils.build_spec("bib-integrity", sc_params=inputScParams)
        es_ref = "d_integrity"
        testUtils.add_policy_rule(2, desc, filter, spec, es_ref)

        desc = "bib src rule tgt:bib"
        filter = testUtils.build_filter("51", "s", "ipn:2.1", tgt=BIB, sc_id=inputScId)
        spec = testUtils.build_spec("bib-integrity", sc_params=inputScParams)
        es_ref = "d_integrity"
        testUtils.add_policy_rule(2, desc, filter, spec, es_ref)

        if testUtils.g_verbose == "detailed":
            testUtils.list_key(2)
            testUtils.list_event_set(2)
            testUtils.list_policy_rule(2)
        if testUtils.g_verbose == "verbose":
            testUtils.list_key(2)
            testUtils.info_event_set(2, "d_integrity")
            testUtils.info_policy_rule(2, 50)
            testUtils.info_policy_rule(2, 51)

        print("Node 3.1 Configuration \n")
        testUtils.add_key(3, "key_1_32bytes")
        testUtils.add_event_set(3, "d_integrity", "default bib")

        desc = "Integrity acceptor rule"
        filter = testUtils.build_filter("52", "a", "ipn:2.1", tgt=PAYLOAD)
        spec = testUtils.build_spec("bib-integrity", sc_id=inputScId, sc_params=inputScParams)
        es_ref = "d_integrity"
        testUtils.add_policy_rule(3, desc, filter, spec, es_ref)

        if testUtils.g_verbose == "detailed":
            testUtils.list_key(3)
            testUtils.list_event_set(3)
            testUtils.list_policy_rule(3)
        if testUtils.g_verbose == "verbose":
            testUtils.list_key(3)
            testUtils.info_event_set(3, "d_integrity")
            testUtils.info_policy_rule(3, 52)

        # Sending the bundle from ipn:2.1 -> ipn:3.1
        testUtils.send_bundle("2.1", "3.1", "2.0", "test_trace27")

        print("Clear Node 2 Policy \n")
        testUtils.del_policy_rule(2, "50")
        testUtils.del_policy_rule(2, "51")
        testUtils.del_event_set(2, "d_integrity")

        print("Clear Node 3 Policy\n")
        testUtils.del_policy_rule(3, "52")
        testUtils.del_event_set(3, "d_integrity")

        # Verify results on ipn:2.1
        log2_events = testUtils.get_test_events(2, log2)
        te_1, log2_events = testUtils.find_test_event(
            log2_events, 'sop_added_at_src', 'ipn:2.1', 'ipn:3.1', 'bib-integrity', '1', None, None)

        # Verify results on ipn:3.1
        log3_events = testUtils.get_test_events(3, log3)
        te_2, log3_events = testUtils.find_test_event(
            log3_events, 'sop_processed', 'ipn:2.1', 'ipn:3.1', 'bib-integrity', '1', te_1['msec'], te_1['count'])

        # Process all test results
        testUtils.check_test_results([3], ["test_trace27"], [log2_events, log3_events])
        '''

    except subprocess.TimeoutExpired as e:
        print(e)
        print("TEST FAILED")
        testUtils.g_tests_failed += 1
        os.chdir("..")

    print("###############################################################################")


def test28(inputBcbScId=testUtils.BCB_AES_GCM_SCID, inputBibScId=testUtils.BIB_HMAC_SHA2_SCID, 
inputBcbScParams=testUtils.bcbScParms, inputBibScParams=testUtils.bibScParms):
    print("###############################################################################")

    try:
        testUtils.start_test(
            "28", "Brief: Check that a BCB is not permitted to target a BIB \n"
                  "it does not share a security target with.\n\n"
                  "Description: Create a bundle with a BIB targeting the Payload Block.\n"
                  "Designate a waypoint node as the security source for a BCB whose target\n"
                  "is that BIB.\n\n"
                  "Bundle Path: ipn:2.1 -> ipn:3.1 -> ipn:4.1\n\n"
                  "NOTE: This security block interaction is not supported in the IOS 4.1.2 release.\n"
                  "Skipping test case.\n\n")
        time.sleep(testUtils.TIME_DISPLAYTEXT)

        '''
        # Tracking current position in log
        log2 = testUtils.log_position("2")
        log4 = testUtils.log_position("4")

        print("Node 2.1 Configuration\n")
        testUtils.add_key(2, "key_1_32bytes")
        testUtils.add_event_set(2, "d_integrity", "default bib")

        desc = "bib src rule tgt:payload"
        filter = testUtils.build_filter("20", "s", "ipn:2.1", tgt=PAYLOAD, sc_id=inputBibScId)
        spec = testUtils.build_spec("bib-integrity", sc_params=inputBibScParams)
        es_ref = "d_integrity"
        testUtils.add_policy_rule(2, desc, filter, spec, es_ref)

        if testUtils.g_verbose == "detailed":
            testUtils.list_key(2)
            testUtils.list_event_set(2)
            testUtils.list_policy_rule(2)
        if testUtils.g_verbose == "verbose":
            testUtils.list_key(2)
            testUtils.info_event_set(2, "d_integrity")
            testUtils.info_policy_rule(2, 20)

        print("Node 3.1 Configuration \n")
        testUtils.add_key(3, "bcb_key_32bytes")
        testUtils.add_event_set(3, "d_bcb_conf", "default bcb")

        desc = "bcb src rule tgt:bib"
        filter = testUtils.build_filter("21", "s", "ipn:2.1", tgt=BIB, sc_id=inputBcbScId)
        spec = testUtils.build_spec("bcb-confidentiality", sc_params=inputBcbScParams)
        es_ref = "d_bcb_conf"
        testUtils.add_policy_rule(3, desc, filter, spec, es_ref)

        if testUtils.g_verbose == "detailed":
            testUtils.list_key(3)
            testUtils.list_event_set(3)
            testUtils.list_policy_rule(3)
        if testUtils.g_verbose == "verbose":
            testUtils.list_key(3)
            testUtils.info_event_set(3, "d_bcb_conf")
            testUtils.info_policy_rule(3, 21)

        print("Node 4.1 Configuration \n")
        testUtils.add_key(4, "key_1_32bytes")
        testUtils.add_event_set(4, "d_integrity", "default bib")

        desc = "bib acceptor rule tgt:payload"
        filter = testUtils.build_filter("22", "a", "ipn:2.1", tgt=PAYLOAD)
        spec = testUtils.build_spec("bib-integrity", sc_id=inputBibScId, sc_params=inputBibScParams)
        es_ref = "d_integrity"
        testUtils.add_policy_rule(3, desc, filter, spec, es_ref)

        if testUtils.g_verbose == "detailed":
            testUtils.list_key(4)
            testUtils.list_event_set(4)
            testUtils.list_policy_rule(4)
        if testUtils.g_verbose == "verbose":
            testUtils.list_key(4)
            testUtils.info_event_set(4, "d_integrity")
            testUtils.info_policy_rule(4, 22)

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

        # Verify results on ipn:2.1
        log2_events = testUtils.get_test_events(2, log2)
        te_1, log2_events = testUtils.find_test_event(
            log2_events, 'sop_added_at_src', 'ipn:2.1', 'ipn:4.1', 'bib-integrity', '1', None, None)

        # Verify results on ipn:4.1
        log4_events = testUtils.get_test_events(4, log4)
        te_2, log4_events = testUtils.find_test_event(
            log4_events, 'sop_processed', 'ipn:2.1', 'ipn:4.1', 'bib-integrity', '1', te_1['msec'], te_1['count'])

        # Process all test results
        testUtils.check_test_results([4], ["test_trace28"], [log2_events, log4_events])
        '''

    except subprocess.TimeoutExpired as e:
        print(e)
        print("TEST FAILED")
        testUtils.g_tests_failed += 1
        os.chdir("..")

    print("##############################################################################")


def test29(inputBcbScId=testUtils.BCB_AES_GCM_SCID, inputBibScId=testUtils.BIB_HMAC_SHA2_SCID, 
inputBcbScParams=testUtils.bcbScParms, inputBibScParams=testUtils.bibScParms):
    print("##############################################################################")

    try:
        testUtils.start_test(
            "29", "Brief: Check that a BCB can encrypt a BIB if the two security blocks \n"
                  "share a security target. \n\n"
                  "Description: Configure node ipn:2.1 to be a security source for both a \n"
                  "BIB and BCB. Both of these security blocks share the same target: the Payload Block.\n"
                  "Node 3.1 serves as the security acceptor for both security operations.\n\n"
                  "Bundle Path: ipn:2.1 -> ipn:3.1 \n\n"
                  "NOTE: This security block interaction is not supported in the IOS 4.1.2 release.\n"
                  "Skipping test case.\n\n")
        time.sleep(testUtils.TIME_DISPLAYTEXT)

        '''
        # Tracking current position in log
        log2 = testUtils.log_position("2")
        log3 = testUtils.log_position("3")

        print("Node 2.1 Configuration\n")
        testUtils.add_key(2, "key_1_32bytes")
        testUtils.add_event_set(2, "d_integrity", "default bib")

        desc = "bib src tgt:payload"
        filter = testUtils.build_filter("20", "s", "ipn:2.1", tgt=PAYLOAD, sc_id=inputBibScId)
        spec = testUtils.build_spec("bib-integrity", sc_params=inputBibScParams)
        es_ref = "d_integrity"
        testUtils.add_policy_rule(2, desc, filter, spec, es_ref)

        testUtils.add_key(2, "bcb_key_32bytes")
        testUtils.add_event_set(2, "d_bcb_conf", "default bcb")

        desc = "bcb src tgt:payload"
        filter = testUtils.build_filter("21", "s", "ipn:2.1", tgt=PAYLOAD, sc_id=inputBcbScId)
        spec = testUtils.build_spec("bcb-confidentiality", sc_params=inputBcbScParams)
        es_ref = "d_bcb_conf"
        testUtils.add_policy_rule(2, desc, filter, spec, es_ref)

        if testUtils.g_verbose == "detailed":
            testUtils.list_key(2)
            testUtils.list_event_set(2)
            testUtils.list_policy_rule(2)
        if testUtils.g_verbose == "verbose":
            testUtils.list_key(2)
            testUtils.info_event_set(2, "d_bcb_conf")
            testUtils.info_policy_rule(2, 20)
            testUtils.info_policy_rule(2, 21)

        print("Node 3.1 Configuration \n")
        testUtils.add_key(3, "bcb_key_32bytes")
        testUtils.add_event_set(3, "d_bcb_conf", "default bcb")

        desc = "bcb acceptor tgt:payload"
        filter = testUtils.build_filter("22", "a", "ipn:2.1", tgt=PAYLOAD)
        spec = testUtils.build_spec("bcb-confidentiality", sc_id=inputBcbScId, sc_params=inputBcbScParams)
        es_ref = "d_bcb_conf"
        testUtils.add_policy_rule(3, desc, filter, spec, es_ref)

        testUtils.add_key(3, "key_1_32bytes")
        testUtils.add_event_set(3, "d_integrity", "default bib")

        desc = "bib acceptor tgt:payload"
        filter = testUtils.build_filter("23", "a", "ipn:2.1", tgt=PAYLOAD)
        spec = testUtils.build_spec("bib-integrity", sc_id=inputBibScId, sc_params=inputBibScParams)
        es_ref = "d_integrity"
        testUtils.add_policy_rule(3, desc, filter, spec, es_ref)

        if testUtils.g_verbose == "detailed":
            testUtils.list_key(3)
            testUtils.list_event_set(3)
            testUtils.list_policy_rule(3)
        if testUtils.g_verbose == "verbose":
            testUtils.list_key(3)
            testUtils.info_event_set(3, "d_integrity")
            testUtils.info_policy_rule(3, 22)
            testUtils.info_policy_rule(3, 23)

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

        # Verify results on ipn:2.1
        log2_events = testUtils.get_test_events(2, log2)
        te_1, log2_events = testUtils.find_test_event(
            log2_events, 'sop_added_at_src', 'ipn:2.1', 'ipn:3.1', 'bcb-confidentiality', '1', None, None)
        te_2, log2_events = testUtils.find_test_event(
            log2_events, 'sop_added_at_src', 'ipn:2.1', 'ipn:3.1', 'bib-integrity', '1', te_1['msec'], te_1['count'])

        # Verify results on ipn:3.1
        log3_events = testUtils.get_test_events(3, log3)
        te_3, log3_events = testUtils.find_test_event(
            log3_events, 'sop_processed', 'ipn:2.1', 'ipn:3.1', 'bcb-confidentiality', '1', te_1['msec'], te_1['count'])
        te_4, log3_events = testUtils.find_test_event(
            log3_events, 'sop_processed', 'ipn:2.1', 'ipn:3.1', 'bib-integrity', '1', te_1['msec'], te_1['count'])

        # Process all test results
        testUtils.check_test_results([3], ["test_trace29"], [log2_events, log3_events])
        '''

    except subprocess.TimeoutExpired as e:
        print(e)
        print("TEST FAILED")
        testUtils.g_tests_failed += 1
        os.chdir("..")

    print("##############################################################################")


def test30(inputBcbScId=testUtils.BCB_AES_GCM_SCID, inputBibScId=testUtils.BIB_HMAC_SHA2_SCID, 
inputScParams=testUtils.bibScParms):
    print("###############################################################################")

    try:
        testUtils.start_test(
            "30", "Brief: Target Multiplicity and Security Block Interactions.\n\n"
                "Description: Configure node ipn:2.1 to be the Security Source \n"
                "for a BIB targeting the Bundle Age Block and Payload Block. \n"
                "Waypoint node ipn:3.1 serves as the Security Source for a BCB \n"
                "targeting the same Bundle Age Block and Payload Block. ipn:4.1 \n"
                "serves as the Security Acceptor for all four security operations.\n\n"
                "Bundle Path: ipn:2.1 -> ipn:3.1 -> ipn:4.1\n\n"
                "NOTE: This security block interaction is not supported in the IOS 4.1.2 release.\n"
                "Skipping test case.\n\n")
        time.sleep(testUtils.TIME_DISPLAYTEXT)

        '''
        # Tracking current position in log
        log2 = testUtils.log_position("2")
        log3 = testUtils.log_position("3")
        log4 = testUtils.log_position("4")

        print("Node ipn:2.1 Configuration\n")
        testUtils.add_key(2, "key_1_32bytes")
        testUtils.add_event_set(2, "d_integrity", "default bib")

        desc = "bib src tgt:BAB"
        filter = testUtils.build_filter("1", "s", "ipn:2.1", tgt=BUNDLEAGE, sc_id=inputBibScId)
        spec = testUtils.build_spec("bib-integrity", sc_params=inputScParams)
        es_ref = "d_integrity"
        testUtils.add_policy_rule(2, desc, filter, spec, es_ref)

        desc = "bib src tgt:payload"
        filter = testUtils.build_filter("2", "s", "ipn:2.1", tgt=PAYLOAD, sc_id=inputBibScId)
        spec = testUtils.build_spec("bib-integrity", sc_params=inputScParams)
        es_ref = "d_integrity"
        testUtils.add_policy_rule(2, desc, filter, spec, es_ref)

        if testUtils.g_verbose == "detailed":
            testUtils.list_key(2)
            testUtils.list_event_set(2)
            testUtils.list_policy_rule(2)
        if testUtils.g_verbose == "verbose":
            testUtils.list_key(2)
            testUtils.info_event_set(2, "d_integrity")
            testUtils.info_policy_rule(2, 1)
            testUtils.info_policy_rule(2, 2)

        print("Node ipn:3.1 Configuration \n")
        testUtils.add_key(3, "key_1_32bytes")
        testUtils.add_event_set(3, "d_conf", "default bcb")

        desc = "bcb src tgt:BAB"
        filter = testUtils.build_filter("3", "s", "ipn:2.1", tgt=BUNDLEAGE, sc_id=inputBcbScId)
        spec = testUtils.build_spec("bcb-confidentiality", sc_params=inputScParams)
        es_ref = "d_conf"
        testUtils.add_policy_rule(3, desc, filter, spec, es_ref)

        desc = "bcb src tgt:payload"
        filter = testUtils.build_filter("4", "s", "ipn:2.1", tgt=PAYLOAD, sc_id=inputBcbScId)
        spec = testUtils.build_spec("bcb-confidentiality", sc_params=inputScParams)
        es_ref = "d_conf"
        testUtils.add_policy_rule(3, desc, filter, spec, es_ref)

        if testUtils.g_verbose == "detailed":
            testUtils.list_key(3)
            testUtils.list_event_set(3)
            testUtils.list_policy_rule(3)
        if testUtils.g_verbose == "verbose":
            testUtils.list_key(3)
            testUtils.info_event_set(3, "d_conf")
            testUtils.info_policy_rule(3, 3)
            testUtils.info_policy_rule(3, 4)
        
        print("Node ipn:4.1 Configuration \n")
        testUtils.add_key(4, "key_1_32bytes")
        testUtils.add_event_set(4, "d_integrity", "default bib")
        testUtils.add_event_set(4, "d_conf", "default bcb")

        desc = "bib acceptor tgt:BAB"
        filter = testUtils.build_filter("5", "a", "ipn:2.1", tgt=BUNDLEAGE)
        spec = testUtils.build_spec("bib-integrity", sc_id=inputBibScId, sc_params=inputScParams)
        es_ref = "d_integrity"
        testUtils.add_policy_rule(4, desc, filter, spec, es_ref)

        desc = "bib acceptor tgt:payload"
        filter = testUtils.build_filter("6", "a", "ipn:2.1", tgt=PAYLOAD)
        spec = testUtils.build_spec("bib-integrity", sc_id=inputBibScId, sc_params=inputScParams)
        es_ref = "d_integrity"
        testUtils.add_policy_rule(4, desc, filter, spec, es_ref)

        desc = "bcb acceptor tgt:BAB"
        filter = testUtils.build_filter("7", "a", "ipn:2.1", tgt=BUNDLEAGE, sc_id=inputBcbScId)
        spec = testUtils.build_spec("bcb-confidentiality", sc_params=inputScParams)
        es_ref = "d_conf"
        testUtils.add_policy_rule(4, desc, filter, spec, es_ref)

        desc = "bcb acceptor tgt:payload"
        filter = testUtils.build_filter("8", "a", "ipn:2.1", tgt=PAYLOAD, sc_id=inputBcbScId)
        spec = testUtils.build_spec("bcb-confidentiality", sc_params=inputScParams)
        es_ref = "d_conf"
        testUtils.add_policy_rule(4, desc, filter, spec, es_ref)

        if testUtils.g_verbose == "detailed":
            testUtils.list_key(4)
            testUtils.list_event_set(4)
            testUtils.list_policy_rule(4)
        if testUtils.g_verbose == "verbose":
            testUtils.list_key(4)
            testUtils.info_event_set(4, "d_integrity")
            testUtils.info_event_set(4, "d_conf")
            testUtils.info_policy_rule(4, 5)
            testUtils.info_policy_rule(4, 6)
            testUtils.info_policy_rule(4, 7)
            testUtils.info_policy_rule(4, 8)

        # Send the bundle from ipn:2.1 -> ipn:3.1 -> ipn:4.1
        testUtils.send_bundle("2.1", "4.1", "2.0", "test_trace30")

        print("\n\nClear Node ipn:2.1 Policy \n")
        testUtils.del_policy_rule(2, "1")
        testUtils.del_policy_rule(2, "2")
        testUtils.del_event_set(2, "d_integrity")
        testUtils.del_key(2, "key_1_32bytes")

        print("Clear Node ipn:3.1 Policy\n")
        testUtils.del_policy_rule(3, "3")
        testUtils.del_policy_rule(3, "4")
        testUtils.del_event_set(3, "d_conf")
        testUtils.del_key(3, "key_1_32bytes")

        print("Clear Node ipn:4.1 Policy \n")
        testUtils.del_policy_rule(4, "5")
        testUtils.del_policy_rule(4, "6")
        testUtils.del_policy_rule(4, "7")
        testUtils.del_policy_rule(4, "8")
        testUtils.del_event_set(4, "d_integrity")
        testUtils.del_event_set(4, "d_conf")
        testUtils.del_key(4, "key_1_32bytes")

        # Verify results on ipn:2.1
        log2_events = testUtils.get_test_events(2, log2)
        te_1, log2_events = testUtils.find_test_event(
            log2_events, 'sop_added_at_src', 'ipn:2.1', 'ipn:4.1', 'bib-integrity', BUNDLEAGE, None, None)
        te_2, log2_events = testUtils.find_test_event(
            log2_events, 'sop_added_at_src', 'ipn:2.1', 'ipn:4.1', 'bib-integrity', PAYLOAD, te_1['msec'], te_1['count'])

        # Verify results on ipn:3.1
        log3_events = testUtils.get_test_events(3, log3)
        te_3, log3_events = testUtils.find_test_event(
            log3_events, 'sop_added_at_src', 'ipn:2.1', 'ipn:4.1', 'bcb-confidentiality', BUNDLEAGE, te_1['msec'], te_1['count'])
        te_4, log3_events = testUtils.find_test_event(
            log3_events, 'sop_added_at_src', 'ipn:2.1', 'ipn:4.1', 'bcb-confidentiality', PAYLOAD, te_1['msec'], te_1['count'])

        # Verify results on ipn:4.1
        log4_events = testUtils.get_test_events(4, log4)
        te_5, log4_events = testUtils.find_test_event(
            log4_events, 'sop_processed', 'ipn:2.1', 'ipn:4.1', 'bcb-confidentiality', BUNDLEAGE, te_1['msec'], te_1['count'])
        te_6, log4_events = testUtils.find_test_event(
            log4_events, 'sop_processed', 'ipn:2.1', 'ipn:4.1', 'bcb-confidentiality', PAYLOAD, te_1['msec'], te_1['count'])
        te_7, log4_events = testUtils.find_test_event(
            log4_events, 'sop_processed', 'ipn:2.1', 'ipn:4.1', 'bib-integrity', BUNDLEAGE, te_1['msec'], te_1['count'])
        te_8, log4_events = testUtils.find_test_event(
            log4_events, 'sop_processed', 'ipn:2.1', 'ipn:4.1', 'bib-integrity', PAYLOAD, te_1['msec'], te_1['count'])
   
        # Process all test results
        testUtils.check_test_results([4], ["test_trace30"], [log2_events, log3_events, log4_events])
        '''

    except subprocess.TimeoutExpired as e:
        print(e)
        print("TEST FAILED")
        testUtils.g_tests_failed += 1
        os.chdir("..")

    print("###############################################################################")


def test31(inputBcbScId=testUtils.BCB_AES_GCM_SCID, inputBibScId=testUtils.BIB_HMAC_SHA2_SCID, 
inputBcbScParams=testUtils.bcbPrimaryScParms, inputBibScParams=testUtils.bibPrimaryScParms):
    print("###############################################################################")
    try:
        testUtils.start_test(
            "31", "Brief: Check that a BCB can be added to the bundle at a waypoint node \n"
                  "if it shares some of an existing BIBs targets.\n\n"
                  "Description: Configure node ipn:2.1 to be the Security Source for a BIB \n"
                  "targeting the Primary Block and the Payload Block.\n\n"
                  "Configure waypoint node ipn:3.1 to be the Security Source for a BCB \n"
                  "targeting the Payload Block.\n\n"
                  "Node ipn:4.1 is configured to be the Security Acceptor for all three \n"
                  "security operations.\n\n"
                  "Bundle Path: ipn:2.1 -> ipn:3.1 -> ipn:4.1\n\n"
                  "NOTE: This security block interaction is not supported in the IOS 4.1.2 release.\n"
                  "Skipping test case.\n\n")
        time.sleep(testUtils.TIME_DISPLAYTEXT)

        '''
        # Tracking current position in log
        log2 = testUtils.log_position("2")
        log3 = testUtils.log_position("3")
        log4 = testUtils.log_position("4")

        print("Node 2.1 Configuration\n")
        testUtils.add_key(2, "key_1_32bytes")
        testUtils.add_event_set(2, "d_integrity", "default bib")

        desc = "bib src tgt:primary"
        filter = testUtils.build_filter("1", "s", "ipn:2.1", tgt=PRIMARY, sc_id=inputBibScId)
        spec = testUtils.build_spec("bib-integrity", sc_params=inputBibScParams)
        es_ref = "d_integrity"
        testUtils.add_policy_rule(2, desc, filter, spec, es_ref)

        desc = "bib src tgt:payload"
        filter = testUtils.build_filter("2", "s", "ipn:2.1", tgt=PAYLOAD, sc_id=inputBibScId)
        spec = testUtils.build_spec("bib-integrity", sc_params=inputBibScParams)
        es_ref = "d_integrity"
        testUtils.add_policy_rule(2, desc, filter, spec, es_ref)

        if testUtils.g_verbose == "detailed":
            testUtils.list_key(2)
            testUtils.list_event_set(2)
            testUtils.list_policy_rule(2)
        if testUtils.g_verbose == "verbose":
            testUtils.list_key(2)
            testUtils.info_event_set(2, "d_integrity")
            testUtils.info_policy_rule(2, 1)
            testUtils.info_policy_rule(2, 2)

        print("Node 3.1 Configuration \n")
        testUtils.add_key(3, "bcb_key_32bytes")
        testUtils.add_event_set(3, "d_conf", "default bcb")

        desc = "bcb src tgt:payload"
        filter = testUtils.build_filter("3", "s", "ipn:2.1", tgt=PAYLOAD, sc_id=inputBcbScId)
        spec = testUtils.build_spec("bcb-confidentiality", sc_params=inputBcbScParams)
        es_ref = "d_conf"
        testUtils.add_policy_rule(3, desc, filter, spec, es_ref)

        if testUtils.g_verbose == "detailed":
            testUtils.list_key(3)
            testUtils.list_event_set(3)
            testUtils.list_policy_rule(3)
        if testUtils.g_verbose == "verbose":
            testUtils.list_key(3)
            testUtils.info_event_set(3, "d_conf")
            testUtils.info_policy_rule(3, 3)

        print("Node 4.1 Configuration \n")
        testUtils.add_key(4, "key_1_32bytes")
        testUtils.add_event_set(4, "d_integrity", "default bib")

        desc = "bib acceptor tgt:primary"
        filter = testUtils.build_filter("4", "a", "ipn:2.1", tgt=PRIMARY)
        spec = testUtils.build_spec("bib-integrity", sc_id=inputBibScId, sc_params=inputBibScParams)
        es_ref = "d_integrity"
        testUtils.add_policy_rule(4, desc, filter, spec, es_ref)

        desc = "bib acceptor tgt:payload"
        filter = testUtils.build_filter("5", "a", "ipn:2.1", tgt=PAYLOAD)
        spec = testUtils.build_spec("bib-integrity", sc_id=inputBibScId, sc_params=inputBibScParams)
        es_ref = "d_integrity"
        testUtils.add_policy_rule(4, desc, filter, spec, es_ref)

        testUtils.add_key(4, "bcb_key_32bytes")
        testUtils.add_event_set(4, "d_conf", "default bcb")

        desc = "bcb src tgt:payload"
        filter = testUtils.build_filter("6", "a", "ipn:2.1", tgt=PAYLOAD)
        spec = testUtils.build_spec("bcb-confidentiality", sc_id=inputBcbScId, sc_params=inputBcbScParams)
        es_ref = "d_conf"
        testUtils.add_policy_rule(4, desc, filter, spec, es_ref)

        if testUtils.g_verbose == "detailed":
            testUtils.list_key(4)
            testUtils.list_event_set(4)
            testUtils.list_policy_rule(4)
        if testUtils.g_verbose == "verbose":
            testUtils.list_key(4)
            testUtils.info_event_set(4, "d_conf")
            testUtils.info_policy_rule(4, 4)
            testUtils.info_policy_rule(4, 5)
            testUtils.info_policy_rule(4, 6)

        # Sending the bundle from ipn:2.1 -> ipn:3.1 -> ipn:4.1
        testUtils.send_bundle("2.1", "4.1", "2.0", "test_trace31")

        print("\n\nClear Node 2 Policy \n")
        testUtils.del_policy_rule(2, "1")
        testUtils.del_policy_rule(2, "2")
        testUtils.del_event_set(2, "d_integrity")

        print("Clear Node 3 Policy\n")
        testUtils.del_policy_rule(3, "3")
        testUtils.del_event_set(3, "d_conf")

        print("Clear Node 4 Policy \n")
        testUtils.del_policy_rule(4, "4")
        testUtils.del_policy_rule(4, "5")
        testUtils.del_policy_rule(4, "6")
        testUtils.del_event_set(4, "d_integrity")
        testUtils.del_event_set(4, "d_conf")

        # Verify results on ipn:2.1
        log2_events = testUtils.get_test_events(2, log2)
        te_1, log2_events = testUtils.find_test_event(
            log2_events, 'sop_added_at_src', 'ipn:2.1', 'ipn:4.1', 'bib-integrity', '0', None, None)
        te_2, log2_events = testUtils.find_test_event(
            log2_events, 'sop_added_at_src', 'ipn:2.1', 'ipn:4.1', 'bib-integrity', '1', te_1['msec'], te_1['count'])
        
        # Verify results on ipn:3.1
        log3_events = testUtils.get_test_events(3, log3)
        te_3, log3_events = testUtils.find_test_event(
            log3_events, 'sop_added_at_src', 'ipn:2.1', 'ipn:4.1', 'bcb-confidentiality', '1', te_1['msec'], te_1['count'])

        # Verify results on ipn:4.1
        log4_events = testUtils.get_test_events(4, log4)
        te_4, log4_events = testUtils.find_test_event(
            log4_events, 'sop_processed', 'ipn:2.1', 'ipn:4.1', 'bib-integrity', '0', te_1['msec'], te_1['count'])
        te_5, log4_events = testUtils.find_test_event(
            log4_events, 'sop_processed', 'ipn:2.1', 'ipn:4.1', 'bib-integrity', '1', te_1['msec'], te_1['count'])
        te_6, log4_events = testUtils.find_test_event(
            log4_events, 'sop_processed', 'ipn:2.1', 'ipn:4.1', 'bcb-confidentiality', '1', te_1['msec'], te_1['count'])
        
        # Process all test results
        testUtils.check_test_results([4], ["test_trace31"], [log2_events, log3_events, log4_events])
        '''

    except subprocess.TimeoutExpired as e:
        print(e)
        print("TEST FAILED")
        testUtils.g_tests_failed += 1
        os.chdir("..")

    print("###############################################################################")


def test32(inputBcbScId=testUtils.BCB_AES_GCM_SCID, inputBibScId=testUtils.BIB_HMAC_SHA2_SCID, 
inputScParams=testUtils.bibScParms):
    print("###############################################################################")

    try:
        testUtils.start_test(
            "32", "Brief: Target Multiplicity and Security Block Interactions. \n"
            "(Security blocks sharing some target blocks).\n\n"
                "Description: Configure node ipn:2.1 to be the Security Source \n"
                "for a BIB targeting the Payload Block. Waypoint node ipn:3.1 \n"
                "serves as the Security Source for a BCB targeting the Bundle \n"
                "Age Block and Payload Block. ipn:4.1 serves as the Security \n"
                "Acceptor for all three security operations.\n\n"
                "Bundle Path: ipn:2.1 -> ipn:3.1 -> ipn:4.1\n\n"
                "NOTE: This security block interaction is not supported in the IOS 4.1.2 release.\n"
                "Skipping test case.\n\n")
        time.sleep(testUtils.TIME_DISPLAYTEXT)

        '''
        # Tracking current position in log
        log2 = testUtils.log_position("2")
        log3 = testUtils.log_position("3")
        log4 = testUtils.log_position("4")

        print("Node ipn:2.1 Configuration\n")
        testUtils.add_key(2, "key_1_32bytes")
        testUtils.add_event_set(2, "d_integrity", "default bib")

        desc = "bib src tgt:payload"
        filter = testUtils.build_filter("1", "s", "ipn:2.1", tgt=PAYLOAD, sc_id=inputBibScId)
        spec = testUtils.build_spec("bib-integrity", sc_params=inputScParams)
        es_ref = "d_integrity"
        testUtils.add_policy_rule(2, desc, filter, spec, es_ref)

        if testUtils.g_verbose == "detailed":
            testUtils.list_key(2)
            testUtils.list_event_set(2)
            testUtils.list_policy_rule(2)
        if testUtils.g_verbose == "verbose":
            testUtils.list_key(2)
            testUtils.info_event_set(2, "d_integrity")
            testUtils.info_policy_rule(2, 1)

        print("Node ipn:3.1 Configuration \n")
        testUtils.add_key(3, "key_1_32bytes")
        testUtils.add_event_set(3, "d_conf", "default bcb")

        desc = "bcb src tgt:BAB"
        filter = testUtils.build_filter("2", "s", "ipn:2.1", tgt=BUNDLEAGE, sc_id=inputBcbScId)
        spec = testUtils.build_spec("bcb-confidentiality", sc_params=inputScParams)
        es_ref = "d_conf"
        testUtils.add_policy_rule(3, desc, filter, spec, es_ref)

        desc = "bcb src tgt:payload"
        filter = testUtils.build_filter("3", "s", "ipn:2.1", tgt=PAYLOAD, sc_id=inputBcbScId)
        spec = testUtils.build_spec("bcb-confidentiality", sc_params=inputScParams)
        es_ref = "d_conf"
        testUtils.add_policy_rule(3, desc, filter, spec, es_ref)

        if testUtils.g_verbose == "detailed":
            testUtils.list_key(3)
            testUtils.list_event_set(3)
            testUtils.list_policy_rule(3)
        if testUtils.g_verbose == "verbose":
            testUtils.list_key(3)
            testUtils.info_event_set(3, "d_conf")
            testUtils.info_policy_rule(3, 2)
            testUtils.info_policy_rule(3, 3)
        
        print("Node ipn:4.1 Configuration \n")
        testUtils.add_key(4, "key_1_32bytes")
        testUtils.add_event_set(4, "d_integrity", "default bib")
        testUtils.add_event_set(4, "d_conf", "default bcb")

        desc = "bib acceptor tgt:payload"
        filter = testUtils.build_filter("4", "a", "ipn:2.1", tgt=PAYLOAD)
        spec = testUtils.build_spec("bib-integrity", sc_id=inputBibScId, sc_params=inputScParams)
        es_ref = "d_integrity"
        testUtils.add_policy_rule(4, desc, filter, spec, es_ref)

        desc = "bcb acceptor tgt:BAB"
        filter = testUtils.build_filter("5", "a", "ipn:2.1", tgt=BUNDLEAGE, sc_id=inputBcbScId)
        spec = testUtils.build_spec("bcb-confidentiality", sc_params=inputScParams)
        es_ref = "d_conf"
        testUtils.add_policy_rule(4, desc, filter, spec, es_ref)

        desc = "bcb acceptor tgt:payload"
        filter = testUtils.build_filter("6", "a", "ipn:2.1", tgt=PAYLOAD, sc_id=inputBcbScId)
        spec = testUtils.build_spec("bcb-confidentiality", sc_params=inputScParams)
        es_ref = "d_conf"
        testUtils.add_policy_rule(4, desc, filter, spec, es_ref)

        if testUtils.g_verbose == "detailed":
            testUtils.list_key(4)
            testUtils.list_event_set(4)
            testUtils.list_policy_rule(4)
        if testUtils.g_verbose == "verbose":
            testUtils.list_key(4)
            testUtils.info_event_set(4, "d_integrity")
            testUtils.info_event_set(4, "d_conf")
            testUtils.info_policy_rule(4, 4)
            testUtils.info_policy_rule(4, 5)
            testUtils.info_policy_rule(4, 6)

        # Send the bundle from ipn:2.1 -> ipn:3.1 -> ipn:4.1
        testUtils.send_bundle("2.1", "4.1", "2.0", "test_trace32")

        print("\n\nClear Node ipn:2.1 Policy \n")
        testUtils.del_policy_rule(2, "1")
        testUtils.del_event_set(2, "d_integrity")
        testUtils.del_key(2, "key_1_32bytes")

        print("Clear Node ipn:3.1 Policy \n")
        testUtils.del_policy_rule(3, "2")
        testUtils.del_policy_rule(3, "3")
        testUtils.del_event_set(3, "d_conf")
        testUtils.del_key(3, "key_1_32bytes")

        print("Clear Node ipn:4.1 Policy \n")
        testUtils.del_policy_rule(4, "4")
        testUtils.del_policy_rule(4, "5")
        testUtils.del_policy_rule(4, "6")
        testUtils.del_event_set(4, "d_integrity")
        testUtils.del_event_set(4, "d_conf")
        testUtils.del_key(4, "key_1_32bytes")

        # Verify results on ipn:2.1
        log2_events = testUtils.get_test_events(2, log2)
        te_1, log2_events = testUtils.find_test_event(
            log2_events, 'sop_added_at_src', 'ipn:2.1', 'ipn:4.1', 'bib-integrity', PAYLOAD, None, None)

        # Verify results on ipn:3.1
        log3_events = testUtils.get_test_events(3, log3)
        te_2, log3_events = testUtils.find_test_event(
            log3_events, 'sop_added_at_src', 'ipn:2.1', 'ipn:4.1', 'bcb-confidentiality', BUNDLEAGE, te_1['msec'], te_1['count'])
        te_3, log3_events = testUtils.find_test_event(
            log3_events, 'sop_added_at_src', 'ipn:2.1', 'ipn:4.1', 'bcb-confidentiality', PAYLOAD, te_1['msec'], te_1['count'])

        # Verify results on ipn:4.1
        log4_events = testUtils.get_test_events(4, log4)
        te_4, log4_events = testUtils.find_test_event(
            log4_events, 'sop_processed', 'ipn:2.1', 'ipn:4.1', 'bcb-confidentiality', BUNDLEAGE, te_1['msec'], te_1['count'])
        te_5, log4_events = testUtils.find_test_event(
            log4_events, 'sop_processed', 'ipn:2.1', 'ipn:4.1', 'bcb-confidentiality', PAYLOAD, te_1['msec'], te_1['count'])
        te_6, log4_events = testUtils.find_test_event(
            log4_events, 'sop_processed', 'ipn:2.1', 'ipn:4.1', 'bib-integrity', PAYLOAD, te_1['msec'], te_1['count'])
   
        # Process all test results
        testUtils.check_test_results([4], ["test_trace32"], [log2_events, log3_events, log4_events])
        '''

    except subprocess.TimeoutExpired as e:
        print(e)
        print("TEST FAILED")
        testUtils.g_tests_failed += 1
        os.chdir("..")

    print("###############################################################################")


def test33(inputScId=testUtils.BIB_HMAC_SHA2_SCID, inputScParams=testUtils.bibPrimaryScParms):
    print("###############################################################################")

    try:
        testUtils.start_test(
            "33", "Brief: BIB target multiplicity test. Check that a BIB can have multiple targets of "
                  "different block types.\n\n"
                  "Description: Configure node ipn:2.1 to be a security source for a BIB targeting both \n"
                  "the Payload and Primary blocks. Node ipn:3.1 serves as the security acceptor for both \n"
                  "security operations.\n\n"
                  "Bundle Path: ipn:2.1 -> ipn:3.1\n\n"
                  "NOTE: Target multiplicity is not supported in the IOS 4.1.2 release.\n"
                  "Skipping test case.\n\n")
        time.sleep(testUtils.TIME_DISPLAYTEXT)

        '''
        # Tracking current position in log
        log2 = testUtils.log_position("2")
        log3 = testUtils.log_position("3")

        print("Node 2.1 Configuration\n")
        testUtils.add_key(2, "key_1_32bytes")
        testUtils.add_event_set(2, "d_integrity", "default bib")

        desc = "bib src tgt:primary"
        filter = testUtils.build_filter("16", "s", "ipn:2.1", tgt=PRIMARY, sc_id=inputScId)
        spec = testUtils.build_spec("bib-integrity", sc_params=inputScParams)
        es_ref = "d_integrity"
        testUtils.add_policy_rule(2, desc, filter, spec, es_ref)

        desc = "bib src tgt:payload"
        filter = testUtils.build_filter("17", "s", "ipn:2.1", tgt=PAYLOAD, sc_id=inputScId)
        spec = testUtils.build_spec("bib-integrity", sc_params=inputScParams)
        es_ref = "d_integrity"
        testUtils.add_policy_rule(2, desc, filter, spec, es_ref)

        if testUtils.g_verbose == "detailed":
            testUtils.list_key(2)
            testUtils.list_event_set(2)
            testUtils.list_policy_rule(2)
        if testUtils.g_verbose == "verbose":
            testUtils.list_key(2)
            testUtils.info_event_set(2, "d_integrity")
            testUtils.info_policy_rule(2, 16)
            testUtils.info_policy_rule(2, 17)

        print("Node 3.1 Configuration \n")
        testUtils.add_key(3, "key_1_32bytes")
        testUtils.add_event_set(3, "d_integrity", "default bib")

        desc = "bib acceptor tgt:primary"
        filter = testUtils.build_filter("18", "a", "ipn:2.1", tgt=PRIMARY)
        spec = testUtils.build_spec("bib-integrity", sc_id=inputScId, sc_params=inputScParams)
        es_ref = "d_integrity"
        testUtils.add_policy_rule(3, desc, filter, spec, es_ref)

        desc = "bib acceptor tgt:payload"
        filter = testUtils.build_filter("19", "a", "ipn:2.1", tgt=PAYLOAD)
        spec = testUtils.build_spec("bib-integrity", sc_id=inputScId, sc_params=inputScParams)
        es_ref = "d_integrity"
        testUtils.add_policy_rule(3, desc, filter, spec, es_ref)

        if testUtils.g_verbose == "detailed":
            testUtils.list_key(3)
            testUtils.list_event_set(3)
            testUtils.list_policy_rule(3)
        if testUtils.g_verbose == "verbose":
            testUtils.list_key(3)
            testUtils.info_event_set(3, "d_integrity")
            testUtils.info_policy_rule(3, 18)
            testUtils.info_policy_rule(3, 19)

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

        # Verify results on ipn:2.1
        log2_events = testUtils.get_test_events(2, log2)
        te_1, log2_events = testUtils.find_test_event(
            log2_events, 'sop_added_at_src', 'ipn:2.1', 'ipn:3.1', 'bib-integrity', '0', None, None)
        te_2, log2_events = testUtils.find_test_event(
            log2_events, 'sop_added_at_src', 'ipn:2.1', 'ipn:3.1', 'bib-integrity', '1', te_1['msec'], te_1['count'])

        # Verify results on ipn:3.1
        log3_events = testUtils.get_test_events(3, log3)
        te_3, log3_events = testUtils.find_test_event(
            log3_events, 'sop_processed', 'ipn:2.1', 'ipn:3.1', 'bib-integrity', '0', te_1['msec'], te_1['count'])
        te_4, log3_events = testUtils.find_test_event(
            log3_events, 'sop_processed', 'ipn:2.1', 'ipn:3.1', 'bib-integrity', '1', te_1['msec'], te_1['count'])

        # Process all test results
        testUtils.check_test_results([3], ["test_trace33"], [log2_events, log3_events])
        '''

    except subprocess.TimeoutExpired as e:
        print(e)
        print("TEST FAILED")
        testUtils.g_tests_failed += 1
        os.chdir("..")

    print("###############################################################################")


def test34(inputScId=testUtils.BCB_AES_GCM_SCID, inputScParams=testUtils.bcbScParms):
    print("###############################################################################")

    try:
        testUtils.start_test(
            "34", "Brief: BCB Target Multiplicity.\n\n"
                 "Description: Add a BCB targeting the Payload Block and Bundle \n"
                 "Age Block at Security Source ipn:2.1 and process that BCB at \n"
                 "Security Acceptor ipn:3.1\n\n"
                 "Bundle Path: ipn:2.1 -> ipn:3.1\n\n"
                 "NOTE: Target multiplicity is not supported in the IOS 4.1.2 release.\n"
                 "Skipping test case.\n\n")
        time.sleep(testUtils.TIME_DISPLAYTEXT)

        '''
        # Tracking current position in log
        log2 = testUtils.log_position("2")
        log3 = testUtils.log_position("3")

        print("Node ipn:2.1 Configuration\n")
        testUtils.add_key(2, "bcb_key_32bytes")
        testUtils.add_event_set(2, "d_bcb_conf", "default bcb")

        desc = "bcb src tgt:payload"
        filter = testUtils.build_filter("1", "s", "ipn:2.1", tgt=PAYLOAD, sc_id=inputScId)
        spec = testUtils.build_spec("bcb-confidentiality", sc_params=inputScParams)
        es_ref = "d_bcb_conf"
        testUtils.add_policy_rule(2, desc, filter, spec, es_ref)

        desc = "bcb src tgt:BAB"
        filter = testUtils.build_filter("2", "s", "ipn:2.1", tgt=BUNDLEAGE, sc_id=inputScId)
        spec = testUtils.build_spec("bcb-confidentiality", sc_params=inputScParams)
        es_ref = "d_bcb_conf"
        testUtils.add_policy_rule(2, desc, filter, spec, es_ref)

        if testUtils.g_verbose == "detailed":
            testUtils.list_key(2)
            testUtils.list_event_set(2)
            testUtils.list_policy_rule(2)
        if testUtils.g_verbose == "verbose":
            testUtils.list_key(2)
            testUtils.info_event_set(2, "d_bcb_conf")
            testUtils.info_policy_rule(2, 1)
            testUtils.info_policy_rule(2, 2)

        print("Node ipn:3.1 Configuration \n")
        testUtils.add_key(3, "bcb_key_32bytes")
        testUtils.add_event_set(3, "d_bcb_conf", "default bcb")

        desc = "bcb acceptor tgt:payload"
        filter = testUtils.build_filter("3", "a", "ipn:2.1", tgt=PAYLOAD)
        spec = testUtils.build_spec("bcb-confidentiality", sc_id=inputScId, sc_params=inputScParams)
        es_ref = "d_bcb_conf"
        testUtils.add_policy_rule(3, desc, filter, spec, es_ref)

        desc = "bcb acceptor tgt:BAB"
        filter = testUtils.build_filter("4", "a", "ipn:2.1", tgt=BUNDLEAGE)
        spec = testUtils.build_spec("bcb-confidentiality", sc_id=inputScId, sc_params=inputScParams)
        es_ref = "d_bcb_conf"
        testUtils.add_policy_rule(3, desc, filter, spec, es_ref)

        if testUtils.g_verbose == "detailed":
            testUtils.list_key(3)
            testUtils.list_event_set(3)
            testUtils.list_policy_rule(3)
        if testUtils.g_verbose == "verbose":
            testUtils.list_key(3)
            testUtils.info_event_set(3, "d_bcb_conf")
            testUtils.info_policy_rule(3, 3)
            testUtils.info_policy_rule(3, 4)

        # Send the bundle from ipn:2.1 -> ipn:3.1
        testUtils.send_bundle("2.1", "3.1", "2.0", "test_trace34")

        print("\n\nClear Node ipn:2.1 Policy \n")
        testUtils.del_policy_rule(2, "1")
        testUtils.del_policy_rule(2, "2")
        testUtils.del_event_set(2, "d_bcb_conf")
        testUtils.del_key(2, "bcb_key_32bytes")

        print("Clear Node ipn:3.1 Policy\n")
        testUtils.del_policy_rule(3, "3")
        testUtils.del_policy_rule(3, "4")
        testUtils.del_event_set(3, "d_bcb_conf")
        testUtils.del_key(3, "bcb_key_32bytes")

        # Verify results on ipn:2.1
        log2_events = testUtils.get_test_events(2, log2)
        te_1, log2_events = testUtils.find_test_event(
            log2_events, 'sop_added_at_src', 'ipn:2.1', 'ipn:3.1', 'bcb-confidentiality', PAYLOAD, None, None)
        te_2, log2_events = testUtils.find_test_event(
            log2_events, 'sop_added_at_src', 'ipn:2.1', 'ipn:3.1', 'bcb-confidentiality', BUNDLEAGE, te_1['msec'], te_1['count'])


        # Verify results on ipn:3.1
        log3_events = testUtils.get_test_events(3, log3)
        te_3, log3_events = testUtils.find_test_event(
            log3_events, 'sop_processed', 'ipn:2.1', 'ipn:3.1', 'bcb-confidentiality', PAYLOAD, te_1['msec'], te_1['count'])
        te_4, log3_events = testUtils.find_test_event(
            log3_events, 'sop_processed', 'ipn:2.1', 'ipn:3.1', 'bcb-confidentiality', BUNDLEAGE, te_1['msec'], te_1['count'])


        # Process all test results
        testUtils.check_test_results([3], ["test_trace32"], [log2_events, log3_events])
        '''

    except subprocess.TimeoutExpired as e:
        print(e)
        print("TEST FAILED")
        testUtils.g_tests_failed += 1
        os.chdir("..")

    print("###############################################################################")


def test35():
    print("###############################################################################")
    print("Test 35 not implemented. This test will be provided when target multiplicity")
    print("is implemented for ION Open Source.")
    print("###############################################################################")


def test36():
    print("###############################################################################")
    print("Test 36 not implemented. This test will be provided when target multiplicity")
    print("is implemented for ION Open Source.")
    print("###############################################################################")


def test37():
    print("###############################################################################")
    print("Test 37 not implemented. This test will be provided for the ION Open Source 4.2")
    print("release to test the \'do_not_forward\' security policy processing action behavior.")
    print("###############################################################################")


def test38(inputScId=testUtils.BIB_HMAC_SHA2_SCID, inputScParams=testUtils.bibPrimaryScParms):
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
                  "NOTE: Identification of missing security operations is not supported in the IOS 4.1.2 release.\n"
                  "Skipping test case.\n\n")
        time.sleep(testUtils.TIME_DISPLAYTEXT)

        '''
        # Tracking current position in log
        log3 = testUtils.log_position("3")

        print("Node 3.1 Configuration \n")
        testUtils.add_key(3, "key_1_32bytes")
        testUtils.add_event_set(3, "d_integrity", "default bib")

        desc = "bib verifier tgt:primary"
        filter = testUtils.build_filter("1", "v", "ipn:2.1", tgt=PRIMARY)
        spec = testUtils.build_spec("bib-integrity", sc_id=inputScId, sc_params=inputScParams)
        es_ref = "d_integrity"
        testUtils.add_policy_rule(3, desc, filter, spec, es_ref)

        if testUtils.g_verbose == "detailed":
            testUtils.list_key(3)
            testUtils.list_event_set(3)
            testUtils.list_policy_rule(3)
        if testUtils.g_verbose == "verbose":
            testUtils.list_key(3)
            testUtils.info_event_set(3, "d_integrity")
            testUtils.info_policy_rule(3, 1)

        # Sending the bundle from ipn:2.1 -> ipn:3.1 -> ipn:4.1
        testUtils.send_bundle("2.1", "4.1", "2.0", "test_trace38")

        print("\n\nClear Node 3 Policy\n")
        testUtils.del_policy_rule(3, "1")
        testUtils.del_event_set(3, "d_integrity")

        # Verify results on ipn:3.1
        log3_events = testUtils.get_test_events(3, log3)
        te_1, log3_events = testUtils.find_test_event(
            log3_events, 'sop_missing_at_acceptor', 'ipn:2.1', 'ipn:4.1', 'bib-integrity', '0', None, None)

        # Process all test results
        testUtils.check_test_results([4], ["test_trace38"], [log3_events])
        '''

    except subprocess.TimeoutExpired as e:
        print(e)
        print("TEST FAILED")
        testUtils.g_tests_failed += 1
        os.chdir("..")

    print("###############################################################################")


def test39(inputScId=testUtils.BCB_AES_GCM_SCID, inputScParams=testUtils.bcbScParms):
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
                  "NOTE: Identification of missing security operations is not supported in the IOS 4.1.2 release.\n"
                  "Skipping test case.\n\n")
        time.sleep(testUtils.TIME_DISPLAYTEXT)

        '''
        # Tracking current position in log
        log3 = testUtils.log_position("3")

        print("Node 3.1 Configuration \n")
        testUtils.add_key(3, "bcb_key_32bytes")
        testUtils.add_event_set(3, "d_conf", "default bcb")

        desc = "bcb verifier tgt:payload"
        filter = testUtils.build_filter("1", "v", "ipn:2.1", tgt=PAYLOAD)
        spec = testUtils.build_spec("bib-integrity", sc_id=inputScId, sc_params=inputScParams)
        es_ref = "d_conf"
        testUtils.add_policy_rule(3, desc, filter, spec, es_ref)

        if testUtils.g_verbose == "detailed":
            testUtils.list_key(3)
            testUtils.list_event_set(3)
            testUtils.list_policy_rule(3)
        if testUtils.g_verbose == "verbose":
            testUtils.list_key(3)
            testUtils.info_event_set(3, "d_conf")
            testUtils.info_policy_rule(3, 1)

        # Sending the bundle from ipn:2.1 -> ipn:3.1 -> ipn:4.1
        testUtils.send_bundle("2.1", "4.1", "2.0", "test_trace39")

        print("\n\nClear Node 3 Policy\n")
        testUtils.del_policy_rule(3, "1")
        testUtils.del_event_set(3, "d_conf")

        # Verify results on ipn:3.1
        log3_events = testUtils.get_test_events(3, log3)
        te_1, log3_events = testUtils.find_test_event(
            log3_events, 'sop_missing_at_acceptor', 'ipn:2.1', 'ipn:4.1', 'bcb-confidentiality', '1', None, None)

        # Process all test results
        testUtils.check_test_results([4], ["test_trace39"], [log3_events])
        '''

    except subprocess.TimeoutExpired as e:
        print(e)
        print("TEST FAILED")
        testUtils.g_tests_failed += 1
        os.chdir("..")

    print("###############################################################################")


def test40():

    print("###############################################################################")

    try:
        testUtils.start_test(
            "40", "Brief: Tests to exercise the bpsecadmin find command \n\n"
                  "Description: Testing the bpsecadmin find command, performing\n"
                  "searches with find types set to 'all' or 'best'.\n\n"
                  "Bundle Path: ipn:2.1 (No bundle transmission necessary).\n\n" )
        time.sleep(testUtils.TIME_DISPLAYTEXT)

        # Tracking current position in log
        log2 = testUtils.log_position("2")

        # Track overall success of find command contents
        success = True 

        print("Node 2.1 Configuration \n")
        testUtils.add_key(2, "key_1_32bytes")
        testUtils.add_key(2, "bcb_key_32bytes")

        testUtils.add_event_set(2, "d_integrity", "default bib")
        testUtils.add_event_set(2, "d_bib_int", "bib integrity event set")
        testUtils.add_event_set(2, "d_bcb_conf", "default bcb")

        desc = "bib src tgt:payload"
        filter = testUtils.build_filter("1", "s", src="ipn:2.1", tgt=PAYLOAD, sc_id=testUtils.BIB_HMAC_SHA2_SCID)
        spec = testUtils.build_spec("bib-integrity", sc_params=testUtils.bibScParms)
        es_ref = "d_integrity"
        testUtils.add_policy_rule(2, desc, filter, spec, es_ref)

        desc = "bib src tgt:primary"
        filter = testUtils.build_filter("2", "s", src="ipn:2.1", dest= "ipn:3.1", sec_src="ipn:4.1", tgt=PRIMARY, sc_id=testUtils.ION_TEST_SCID)
        spec = testUtils.build_spec("bib-integrity", sc_params=testUtils.bibScParms)
        es_ref = "d_integrity"
        testUtils.add_policy_rule(2, desc, filter, spec, es_ref)

        desc = "bib src tgt:bundle age blk"
        filter = testUtils.build_filter("3", "s", src="ipn:2.1", tgt=BUNDLEAGE, sc_id=testUtils.BIB_HMAC_SHA2_SCID )
        spec = testUtils.build_spec("bib-integrity", sc_params=testUtils.bibScParms)
        es_ref = "d_integrity"
        testUtils.add_policy_rule(2, desc, filter, spec, es_ref)

        desc = "bcb accpt tgt:payload"
        filter = testUtils.build_filter("4", "a", src="ipn:4.1", tgt=PAYLOAD)
        spec = testUtils.build_spec("bcb-confidentiality", testUtils.BCB_AES_GCM_SCID, testUtils.bcbScParms)
        es_ref = "d_bcb_conf"
        testUtils.add_policy_rule(2, desc, filter, spec, es_ref)

        desc = "bib ver tgt:bundle age blk"
        filter = testUtils.build_filter("5", "v", src="ipn:2.1", tgt= BUNDLEAGE)
        spec = testUtils.build_spec("bib-integrity", testUtils.BIB_HMAC_SHA2_SCID, testUtils.bibScParms)
        es_ref = "d_bib_int"
        testUtils.add_policy_rule(2, desc, filter, spec, es_ref)

        desc = "bib src tgt:primary"
        filter = testUtils.build_filter("6", "s", src="ipn:2.1", dest="ipn:3.1", tgt=PRIMARY, sc_id=testUtils.BIB_HMAC_SHA2_SCID)
        spec = testUtils.build_spec("bib-integrity", sc_params=testUtils.bibScParms)
        es_ref = "d_integrity"
        testUtils.add_policy_rule(2, desc, filter, spec, es_ref)

        if testUtils.g_verbose == "detailed":
            testUtils.list_key(2)
            testUtils.list_event_set(2)
            testUtils.list_policy_rule(2)
        if testUtils.g_verbose == "verbose":
            testUtils.list_key(2)
            testUtils.info_event_set(2, "d_bcb_conf")
            testUtils.info_event_set(2, "d_bib_int")
            testUtils.info_event_set(2, "d_integrity")
            testUtils.info_policy_rule(2, 1)
            testUtils.info_policy_rule(2, 2)
            testUtils.info_policy_rule(2, 3)
            testUtils.info_policy_rule(2, 4)
            testUtils.info_policy_rule(2, 5)
            testUtils.info_policy_rule(2, 6)

        print("1. Perform find with the following criteria:")
        print("\ttype: all\n\trole: source \n\tbsrc: ipn:2.1\n")
        output = testUtils.find_policy_rule(2, "all", role="s", src="ipn:2.1")
        res = testUtils.check_policy_rules(output, ["1", "2", "3", "6"])
        print()

        if(res != 1):
            success = False
            print("Find test (1) unsuccessful")

        print("2. Perform find with the following criteria:")
        print("\ttype: all \n\trole: source")
        output = testUtils.find_policy_rule(2, "all", role="s")
        res = testUtils.check_policy_rules(output, ["1", "2", "3", "6"])
        print()

        if(res != 1):
            success = False
            print("Find test (2) unsuccessful")

        print("3. Perform find with the following criteria:")
        print("\ttype: all \n\tbsrc: ipn:2.1")
        output = testUtils.find_policy_rule(2, "all", src="ipn:2.1")
        res = testUtils.check_policy_rules(output, ["1", "2", "3", "5", "6"])
        print()

        if(res != 1):
            success = False
            print("Find test (3) unsuccessful")

        print("4. Perform find with the following criteria:")
        print("\ttype: all \n\trole: acceptor")
        output = testUtils.find_policy_rule(2, "all", role="a")
        res = testUtils.check_policy_rules(output, ["4"])
        print()

        if(res != 1):
            success = False
            print("Find test (4) unsuccessful")

        print("5. Perform find with the following criteria:")
        print("\ttype: all \n\ttarget type: ", str(BUNDLEAGE))
        output = testUtils.find_policy_rule(2, "all", tgt=BUNDLEAGE)
        res = testUtils.check_policy_rules(output, ["3", "5"])
        print()

        if(res != 1):
            success = False
            print("Find test (5) unsuccessful")

        print("6. Perform find with the following criteria:")
        print("\ttype: best \n\tsc_id: ION Test Suite")
        output = testUtils.find_policy_rule(2, "best", sc_id=testUtils.ION_TEST_SCID)
        res = testUtils.check_policy_rules(output, ["2"])
        print()

        if(res != 1):
            success = False
            print("Find test (6) unsuccessful")

        print("7. Perform find with the following criteria:")
        print("\ttype: all \n\trole: source \n\tbsrc: ipn:2.1 \n\ttarget type: ", str(PRIMARY))
        output = testUtils.find_policy_rule(2, "all", role="s", src="ipn:2.1", tgt=PRIMARY)
        res = testUtils.check_policy_rules(output, ["2", "6"])
        print()

        if(res != 1):
            success = False
            print("Find test (7) unsuccessful")

        print("8. Perform find with the following criteria:")
        print("\ttype: best \n\trole: source \n\tbsrc: ipn:2.1 \n\ttarget type: ", str(PRIMARY))
        output = testUtils.find_policy_rule(2, "best", role="s", src="ipn:2.1", tgt=PRIMARY)
        res = testUtils.check_policy_rules(output, ["2"])
        print()

        if(res != 1):
            success = False
            print("Find test (8) unsuccessful")

        print("9. Perform find with the following criteria:")
        print("\ttype: best \n\ttarget type: ", str(BUNDLEAGE))
        output = testUtils.find_policy_rule(2, "best", tgt=BUNDLEAGE)
        res = testUtils.check_policy_rules(output, ["5"])
        print()

        if(res != 1):
            success = False
            print("Find test (9) unsuccessful")

        print("10. Perform find with the following criteria:")
        print("\ttype: all \n\tsvc: bcb-confidentiality")
        output = testUtils.find_policy_rule(2, "all", svc="bcb-confidentiality")
        res = testUtils.check_policy_rules(output, ["4"])
        print()

        if(res != 1):
            success = False
            print("Find test (10) unsuccessful")

        print("11. Perform find with the following criteria:")
        print("\ttype: all \n\tes_ref: d_integrity")
        output = testUtils.find_policy_rule(2, "all", es_ref="d_integrity")
        res = testUtils.check_policy_rules(output, ["1", "2", "3", "6"])
        print()

        if(res != 1):
            success = False
            print("Find test (11) unsuccessful")

        # Check overall success of the find tests to record
        if (success == True):
            print("TEST PASSED")
            testUtils.inc_passed_tests()
        else:
            print("TEST FAILED")
            testUtils.inc_failed_tests()

        print("\n\nClear Node 2 Policy\n")
        testUtils.del_policy_rule(2, "1")
        testUtils.del_policy_rule(2, "2")
        testUtils.del_policy_rule(2, "3")
        testUtils.del_policy_rule(2, "4")
        testUtils.del_policy_rule(2, "5")
        testUtils.del_policy_rule(2, "6")
        testUtils.del_event_set(2, "d_conf")
        testUtils.del_event_set(2, "d_bib_int")
        testUtils.del_event_set(2, "d_integrity")

    except subprocess.TimeoutExpired as e:
        print(e)
        print("TEST FAILED")
        testUtils.g_tests_failed += 1
        os.chdir("..")

    print("###############################################################################")


def test41():

    print("###############################################################################")

    try:
        testUtils.start_test(
            "41", "Brief: Checking that sc_id field can accept an integer or string, and\n"
                  "that invalid security contexts are identified.\n\n"
                  "Description: This test creates policy rules using both string and integer\n"
                  "values to represent the associated security context. This test checks that:\n"
                  "\t1. Only policy rules with supported security context IDs are created.\n"
                  "\t2. The sc_id field can accept string or integer security context identifiers.\n\n")
        time.sleep(testUtils.TIME_DISPLAYTEXT)

        vers = testUtils.get_version(2)

        if vers == "ION-OPEN-SOURCE":

            invalid_bcb_scid = 4

            print("Node 2.1 Configuration \n")
            testUtils.add_event_set(2, "d_conf", "default conf event set")

            desc = "bcb valid sec ctxt"
            filter = testUtils.build_filter("1", "v", "ipn:2.1", tgt=1)
            spec = testUtils.build_spec("bcb-confidentiality", sc_id=testUtils.ION_TEST_SCID, sc_params=testUtils.bcbScParms)
            es_ref = "d_conf"
            testUtils.add_policy_rule(2, desc, filter, spec, es_ref)

            desc = "bcb invalid num sec ctxt"
            filter = testUtils.build_filter("2", "v", "ipn:2.1", tgt=1)
            spec = testUtils.build_spec("bcb-confidentiality", sc_id=invalid_bcb_scid, sc_params=testUtils.bcbScParms)
            es_ref = "d_conf"
            testUtils.add_policy_rule(2, desc, filter, spec, es_ref)        

            desc = "bcb valid str sec ctxt"
            filter = testUtils.build_filter("3", "v", "ipn:2.1", tgt=1)
            spec = testUtils.build_spec("bcb-confidentiality", sc_id="ION Test Contexts", sc_params=testUtils.bcbScParms)
            es_ref = "d_conf"
            testUtils.add_policy_rule(2, desc, filter, spec, es_ref)
            
            desc = "bcb invalid str sec ctxt"
            filter = testUtils.build_filter("4", "v", "ipn:2.1", tgt=1)
            spec = testUtils.build_spec("bcb-confidentiality", sc_id="NOT ION Test Contexts", sc_params=testUtils.bcbScParms)
            es_ref = "d_conf"
            testUtils.add_policy_rule(2, desc, filter, spec, es_ref)

            if testUtils.g_verbose == "detailed":
                testUtils.list_policy_rule(2)
            if testUtils.g_verbose == "verbose":
                testUtils.info_policy_rule(2, 1)
                testUtils.info_policy_rule(2, 3)

            # Use find to make sure only rules 1 and 3 were added
            output = testUtils.find_policy_rule(2, "all", role="v")
            find_result = testUtils.check_policy_rules(output, ["1", "3"])

            if(find_result == 1):
                print("TEST PASSED")
                testUtils.inc_passed_tests()
            else:
                print("TEST FAILED")
                testUtils.inc_failed_tests()

            print("\n\nClear Node 2 Policy\n")
            testUtils.del_policy_rule(2, "1")
            testUtils.del_policy_rule(2, "3")
            testUtils.del_event_set(2, "d_conf")

        elif vers == "ION-NASA-BASELINE":

            invalid_bcb_scid = -11

            print("Node 2.1 Configuration \n")
            testUtils.add_event_set(2, "d_conf", "default conf event set")

            desc = "bcb valid sec ctxt"
            filter = testUtils.build_filter("1", "v", "ipn:2.1", tgt=1)
            spec = testUtils.build_spec("bcb-confidentiality", sc_id=testUtils.BCB_AES_GCM_SCID, sc_params=testUtils.bcbScParms)
            es_ref = "d_conf"
            testUtils.add_policy_rule(2, desc, filter, spec, es_ref)

            desc = "bcb invalid num sec ctxt"
            filter = testUtils.build_filter("2", "v", "ipn:2.1", tgt=1)
            spec = testUtils.build_spec("bcb-confidentiality", sc_id=invalid_bcb_scid, sc_params=testUtils.bcbScParms)
            es_ref = "d_conf"
            testUtils.add_policy_rule(2, desc, filter, spec, es_ref)        

            desc = "bcb valid str sec ctxt"
            filter = testUtils.build_filter("3", "v", "ipn:2.1", tgt=1)
            spec = testUtils.build_spec("bcb-confidentiality", sc_id="BCB-AES-GCM", sc_params=testUtils.bcbScParms)
            es_ref = "d_conf"
            testUtils.add_policy_rule(2, desc, filter, spec, es_ref)
            
            desc = "bcb invalid str sec ctxt"
            filter = testUtils.build_filter("4", "v", "ipn:2.1", tgt=1)
            spec = testUtils.build_spec("bcb-confidentiality", sc_id="NOT-BCB-AES-GCM", sc_params=testUtils.bcbScParms)
            es_ref = "d_conf"
            testUtils.add_policy_rule(2, desc, filter, spec, es_ref)

            if testUtils.g_verbose == "detailed":
                testUtils.list_policy_rule(2)
            if testUtils.g_verbose == "verbose":
                testUtils.info_policy_rule(2, 1)
                testUtils.info_policy_rule(2, 3)

            # Use find to make sure only rules 1 and 3 were added
            output = testUtils.find_policy_rule(2, "all", role="v")
            find_result = testUtils.check_policy_rules(output, ["1", "3"])

            if(find_result == 1):
                print("TEST PASSED")
                testUtils.inc_passed_tests()
            else:
                print("TEST FAILED")
                testUtils.inc_failed_tests()

            print("\n\nClear Node 2 Policy\n")
            testUtils.del_policy_rule(2, "1")
            testUtils.del_policy_rule(2, "3")
            testUtils.del_event_set(2, "d_conf")

        else:
            print("Invalid ION version detected.")
            print("TEST FAILED")
            testUtils.g_tests_failed += 1
            os.chdir("..")

    except subprocess.TimeoutExpired as e:
        print(e)
        print("TEST FAILED")
        testUtils.g_tests_failed += 1
        os.chdir("..")

    print("###############################################################################")


def test42(inputScId=testUtils.BIB_HMAC_SHA2_SCID, inputScParams=testUtils.bibScParms):
    print("###############################################################################")

    try:
        testUtils.start_test(
            "42", "Brief: BIB targeting an Extension Block.\n\n"
                 "Description: Add a BIB targeting the Bundle Age Block at \n"
                 "Security Source ipn:2.1 and process that BIB at Security \n"
                 "Acceptor ipn:3.1\n\n"
                 "Bundle Path: ipn:2.1 -> ipn:3.1\n\n")
        time.sleep(testUtils.TIME_DISPLAYTEXT)

        # Tracking current position in log
        log2 = testUtils.log_position("2")
        log3 = testUtils.log_position("3")

        print("Node ipn:2.1 Configuration\n")
        testUtils.add_key(2, "key_1_32bytes")
        testUtils.add_event_set(2, "d_integrity", "default bib")

        desc = "bib src tgt:BAB"
        filter = testUtils.build_filter("1", "s", "ipn:2.1", tgt=BUNDLEAGE, sc_id=inputScId)
        spec = testUtils.build_spec("bib-integrity", sc_params=inputScParams)
        es_ref = "d_integrity"
        testUtils.add_policy_rule(2, desc, filter, spec, es_ref)

        if testUtils.g_verbose == "detailed":
            testUtils.list_key(2)
            testUtils.list_event_set(2)
            testUtils.list_policy_rule(2)
        if testUtils.g_verbose == "verbose":
            testUtils.list_key(2)
            testUtils.info_event_set(2, "d_integrity")
            testUtils.info_policy_rule(2, 1)

        print("Node ipn:3.1 Configuration \n")
        testUtils.add_key(3, "key_1_32bytes")
        testUtils.add_event_set(3, "d_integrity", "default bib")

        desc = "bib acceptor tgt:BAB"
        filter = testUtils.build_filter("2", "a", "ipn:2.1", tgt=BUNDLEAGE)
        spec = testUtils.build_spec("bib-integrity", sc_id=inputScId, sc_params=inputScParams)
        es_ref = "d_integrity"
        testUtils.add_policy_rule(3, desc, filter, spec, es_ref)

        if testUtils.g_verbose == "detailed":
            testUtils.list_key(3)
            testUtils.list_event_set(3)
            testUtils.list_policy_rule(3)
        if testUtils.g_verbose == "verbose":
            testUtils.list_key(3)
            testUtils.info_event_set(3, "d_integrity")
            testUtils.info_policy_rule(3, 2)

        # Send the bundle from ipn:2.1 -> ipn:3.1
        testUtils.send_bundle("2.1", "3.1", "2.0", "test_trace42")

        print("\n\nClear Node ipn:2.1 Policy \n")
        testUtils.del_policy_rule(2, "1")
        testUtils.del_event_set(2, "d_integrity")
        testUtils.del_key(2, "key_1_32bytes")

        print("Clear Node ipn:3.1 Policy\n")
        testUtils.del_policy_rule(3, "2")
        testUtils.del_event_set(3, "d_integrity")
        testUtils.del_key(3, "key_1_32bytes")

        # Verify results on ipn:2.1
        log2_events = testUtils.get_test_events(2, log2)
        te_1, log2_events = testUtils.find_test_event(
            log2_events, 'sop_added_at_src', 'ipn:2.1', 'ipn:3.1', 'bib-integrity', BUNDLEAGE, None, None)

        # Verify results on ipn:3.1
        log3_events = testUtils.get_test_events(3, log3)
        te_2, log3_events = testUtils.find_test_event(
            log3_events, 'sop_processed', 'ipn:2.1', 'ipn:3.1', 'bib-integrity', BUNDLEAGE, te_1['msec'], te_1['count'])

        # Process all test results
        testUtils.check_test_results([3], ["test_trace42"], [log2_events, log3_events])

    except subprocess.TimeoutExpired as e:
        print(e)
        print("TEST FAILED")
        testUtils.g_tests_failed += 1
        os.chdir("..")

    print("###############################################################################")


def test43(inputScId=testUtils.BCB_AES_GCM_SCID, inputScParams=testUtils.bcbScParms):
    print("###############################################################################")

    try:
        testUtils.start_test(
            "43", "Brief: BCB targeting an Extension Block.\n\n"
                 "Description: Add a BCB targeting the Bundle Age Block at \n"
                 "Security Source ipn:2.1 and process that BCB at Security \n"
                 "Acceptor ipn:3.1\n\n"
                 "Bundle Path: ipn:2.1 -> ipn:3.1\n\n")
        time.sleep(testUtils.TIME_DISPLAYTEXT)

        # Tracking current position in log
        log2 = testUtils.log_position("2")
        log3 = testUtils.log_position("3")

        print("Node ipn:2.1 Configuration\n")
        testUtils.add_key(2, "bcb_key_32bytes")
        testUtils.add_event_set(2, "d_bcb_conf", "default bcb")

        desc = "bcb src tgt:BAB"
        filter = testUtils.build_filter("4", "s", "ipn:2.1", tgt=BUNDLEAGE, sc_id=inputScId)
        spec = testUtils.build_spec("bcb-confidentiality", sc_params=inputScParams)
        es_ref = "d_bcb_conf"
        testUtils.add_policy_rule(2, desc, filter, spec, es_ref)

        if testUtils.g_verbose == "detailed":
            testUtils.list_key(2)
            testUtils.list_event_set(2)
            testUtils.list_policy_rule(2)
        if testUtils.g_verbose == "verbose":
            testUtils.list_key(2)
            testUtils.info_event_set(2, "d_bcb_conf")
            testUtils.info_policy_rule(2, 4)

        print("Node ipn:3.1 Configuration \n")
        testUtils.add_key(3, "bcb_key_32bytes")
        testUtils.add_event_set(3, "d_bcb_conf", "default bcb")

        desc = "bcb acceptor tgt:BAB"
        filter = testUtils.build_filter("5", "a", "ipn:2.1", tgt=BUNDLEAGE)
        spec = testUtils.build_spec("bcb-confidentiality", sc_id=inputScId, sc_params=inputScParams)
        es_ref = "d_bcb_conf"
        testUtils.add_policy_rule(3, desc, filter, spec, es_ref)

        if testUtils.g_verbose == "detailed":
            testUtils.list_key(3)
            testUtils.list_event_set(3)
            testUtils.list_policy_rule(3)
        if testUtils.g_verbose == "verbose":
            testUtils.list_key(3)
            testUtils.info_event_set(3, "d_bcb_conf")
            testUtils.info_policy_rule(3, 5)

        # Send the bundle from ipn:2.1 -> ipn:3.1
        testUtils.send_bundle("2.1", "3.1", "2.0", "test_trace43")

        print("\n\nClear Node ipn:2.1 Policy \n")
        testUtils.del_policy_rule(2, "4")
        testUtils.del_event_set(2, "d_bcb_conf")
        testUtils.del_key(2, "bcb_key_32bytes")

        print("Clear Node ipn:3.1 Policy\n")
        testUtils.del_policy_rule(3, "5")
        testUtils.del_event_set(3, "d_bcb_conf")
        testUtils.del_key(3, "bcb_key_32bytes")

        # Verify results on ipn:2.1
        log2_events = testUtils.get_test_events(2, log2)
        te_1, log2_events = testUtils.find_test_event(
            log2_events, 'sop_added_at_src', 'ipn:2.1', 'ipn:3.1', 'bcb-confidentiality', BUNDLEAGE, None, None)

        # Verify results on ipn:3.1
        log3_events = testUtils.get_test_events(3, log3)
        te_2, log3_events = testUtils.find_test_event(
            log3_events, 'sop_processed', 'ipn:2.1', 'ipn:3.1', 'bcb-confidentiality', BUNDLEAGE, te_1['msec'], te_1['count'])

        # Process all test results
        testUtils.check_test_results([3], ["test_trace43"], [log2_events, log3_events])

    except subprocess.TimeoutExpired as e:
        print(e)
        print("TEST FAILED")
        testUtils.g_tests_failed += 1
        os.chdir("..")

    print("###############################################################################")


def test44(inputScId=testUtils.BIB_HMAC_SHA2_SCID, inputScParams=testUtils.bibScParms):
    print("###############################################################################")

    try:
        testUtils.start_test(
            "44", "Brief: Intentionally corrupt a BIB targeting an Extension Block.\n\n"
                 "Description: Add a BIB targeting the Bundle Age Block at \n"
                 "Security Source ipn:2.1 using key_1_32bytes and process that BIB at Security \n"
                 "Acceptor ipn:3.1 using key_2_32bytes. This intentional key misconfiguration \n"
                 "causes the Security Acceptor to identify the integrity security operation as \n"
                 "corrupted. A processing action is added to the event set to instruct the BPA to \n"
                 "discard the security target (the BAB) if it is corrupted at the Acceptor.\n\n"
                 "Bundle Path: ipn:2.1 -> ipn:3.1\n\n")
        time.sleep(testUtils.TIME_DISPLAYTEXT)

        # Tracking current position in log
        log2 = testUtils.log_position("2")
        log3 = testUtils.log_position("3")

        print("Node ipn:2.1 Configuration\n")
        testUtils.add_key(2, "key_1_32bytes")
        testUtils.add_event_set(2, "d_integrity", "default bib")

        desc = "bib src tgt:BAB"
        filter = testUtils.build_filter("1", "s", "ipn:2.1", tgt=BUNDLEAGE, sc_id=inputScId)
        spec = testUtils.build_spec("bib-integrity", sc_params=inputScParams)
        es_ref = "d_integrity"
        testUtils.add_policy_rule(2, desc, filter, spec, es_ref)

        if testUtils.g_verbose == "detailed":
            testUtils.list_key(2)
            testUtils.list_event_set(2)
            testUtils.list_policy_rule(2)
        if testUtils.g_verbose == "verbose":
            testUtils.list_key(2)
            testUtils.info_event_set(2, "d_integrity")
            testUtils.info_policy_rule(2, 1)

        print("Node ipn:3.1 Configuration \n")
        testUtils.add_key(3, "key_2_32bytes")
        testUtils.add_event_set(3, "d_integrity", "default bib")
        testUtils.add_event(3, "d_integrity", "sop_corrupt_at_acceptor", [["remove_sop_target"]])        
       
        desc = "bib acceptor tgt:BAB"
        filter = testUtils.build_filter("2", "a", "ipn:2.1", tgt=BUNDLEAGE)
        spec = testUtils.build_spec("bib-integrity", sc_id=inputScId, sc_params=inputScParams)
        es_ref = "d_integrity"
        testUtils.add_policy_rule(3, desc, filter, spec, es_ref)

        if testUtils.g_verbose == "detailed":
            testUtils.list_key(3)
            testUtils.list_event_set(3)
            testUtils.list_policy_rule(3)
        if testUtils.g_verbose == "verbose":
            testUtils.list_key(3)
            testUtils.info_event_set(3, "d_integrity")
            testUtils.info_policy_rule(3, 2)

        # Send the bundle from ipn:2.1 -> ipn:3.1
        testUtils.send_bundle("2.1", "3.1", "2.0", "test_trace44")

        print("\n\nClear Node ipn:2.1 Policy \n")
        testUtils.del_policy_rule(2, "1")
        testUtils.del_event_set(2, "d_integrity")
        testUtils.del_key(2, "key_1_32bytes")

        print("Clear Node ipn:3.1 Policy\n")
        testUtils.del_policy_rule(3, "2")
        testUtils.del_event_set(3, "d_integrity")
        testUtils.del_key(3, "key_2_32bytes")

        # Verify results on ipn:2.1
        log2_events = testUtils.get_test_events(2, log2)
        te_1, log2_events = testUtils.find_test_event(
            log2_events, 'sop_added_at_src', 'ipn:2.1', 'ipn:3.1', 'bib-integrity', BUNDLEAGE, None, None)

        # Verify results on ipn:3.1
        log3_events = testUtils.get_test_events(3, log3)
        te_2, log3_events = testUtils.find_test_event(
            log3_events, 'sop_corrupt_at_acceptor', 'ipn:2.1', 'ipn:3.1', 'bib-integrity', BUNDLEAGE, te_1['msec'], te_1['count'])

        # Process all test results
        testUtils.check_test_results([3], ["test_trace44"], [log2_events, log3_events], deliver_bundle=False)

    except subprocess.TimeoutExpired as e:
        print(e)
        print("TEST FAILED")
        testUtils.g_tests_failed += 1
        os.chdir("..")

    print("###############################################################################")


def test45(inputScId=testUtils.BCB_AES_GCM_SCID, inputScParams=testUtils.bcbScParms):
    print("###############################################################################")

    try:
        testUtils.start_test(
            "45", "Brief: Intentionally corrupt a BCB targeting an Extension Block.\n\n"
                 "Description: Add a BCB targeting the Bundle Age Block at \n"
                 "Security Source ipn:2.1 using bcb_key_32bytes and process that BCB at Security \n"
                 "Acceptor ipn:3.1 using bcb_key_2_32bytes. This intentional key misconfiguration \n"
                 "causes the Security Acceptor to identify the confidentiality security operation as \n"
                 "corrupted. A processing action is added to the event set to instruct the BPA to \n"
                 "discard the security target (the BAB) if it is corrupted at the Acceptor.\n\n"
                 "Bundle Path: ipn:2.1 -> ipn:3.1\n\n")
        time.sleep(testUtils.TIME_DISPLAYTEXT)

        # Tracking current position in log
        log2 = testUtils.log_position("2")
        log3 = testUtils.log_position("3")

        print("Node ipn:2.1 Configuration\n")
        testUtils.add_key(2, "bcb_key_32bytes")
        testUtils.add_event_set(2, "d_bcb_conf", "default bcb")

        desc = "bcb src tgt:BAB"
        filter = testUtils.build_filter("4", "s", "ipn:2.1", tgt=BUNDLEAGE, sc_id=inputScId)
        spec = testUtils.build_spec("bcb-confidentiality", sc_params=inputScParams)
        es_ref = "d_bcb_conf"
        testUtils.add_policy_rule(2, desc, filter, spec, es_ref)

        if testUtils.g_verbose == "detailed":
            testUtils.list_key(2)
            testUtils.list_event_set(2)
            testUtils.list_policy_rule(2)
        if testUtils.g_verbose == "verbose":
            testUtils.list_key(2)
            testUtils.info_event_set(2, "d_bcb_conf")
            testUtils.info_policy_rule(2, 4)

        print("Node ipn:3.1 Configuration \n")
        testUtils.add_key(3, "bcb_key_2_32bytes")
        testUtils.add_event_set(3, "d_bcb_conf", "default bcb")
        testUtils.add_event(3, "d_bcb_conf", "sop_corrupt_at_acceptor", [["remove_sop_target"]])
        
        desc = "bcb acceptor tgt:BAB"
        filter = testUtils.build_filter("5", "a", "ipn:2.1", tgt=BUNDLEAGE)
        spec = testUtils.build_spec("bcb-confidentiality", sc_id=inputScId, sc_params=inputScParams)
        es_ref = "d_bcb_conf"
        testUtils.add_policy_rule(3, desc, filter, spec, es_ref)

        if testUtils.g_verbose == "detailed":
            testUtils.list_key(3)
            testUtils.list_event_set(3)
            testUtils.list_policy_rule(3)
        if testUtils.g_verbose == "verbose":
            testUtils.list_key(3)
            testUtils.info_event_set(3, "d_bcb_conf")
            testUtils.info_policy_rule(3, 5)

        # Send the bundle from ipn:2.1 -> ipn:3.1
        testUtils.send_bundle("2.1", "3.1", "2.0", "test_trace45")

        print("\n\nClear Node ipn:2.1 Policy \n")
        testUtils.del_policy_rule(2, "4")
        testUtils.del_event_set(2, "d_bcb_conf")
        testUtils.del_key(2, "bcb_key_32bytes")

        print("Clear Node ipn:3.1 Policy\n")
        testUtils.del_policy_rule(3, "5")
        testUtils.del_event_set(3, "d_bcb_conf")
        testUtils.del_key(3, "bcb_key_2_32bytes")

        # Verify results on ipn:2.1
        log2_events = testUtils.get_test_events(2, log2)
        te_1, log2_events = testUtils.find_test_event(
            log2_events, 'sop_added_at_src', 'ipn:2.1', 'ipn:3.1', 'bcb-confidentiality', BUNDLEAGE, None, None)

        # Verify results on ipn:3.1
        log3_events = testUtils.get_test_events(3, log3)
        te_2, log3_events = testUtils.find_test_event(
            log3_events, 'sop_corrupt_at_acceptor', 'ipn:2.1', 'ipn:3.1', 'bcb-confidentiality', BUNDLEAGE, te_1['msec'], te_1['count'])

        # Process all test results
        testUtils.check_test_results([3], ["test_trace45"], [log2_events, log3_events], deliver_bundle=False)

    except subprocess.TimeoutExpired as e:
        print(e)
        print("TEST FAILED")
        testUtils.g_tests_failed += 1
        os.chdir("..")

    print("###############################################################################")

def test46():
    print("###############################################################################")
    print("Test 46 not implemented. This test will be provided for the ION Open Source 4.2")
    print("release to test the execution of multiple security policy processing actions")
    print("in response to an integrity corruption security operation event.")
    print("###############################################################################")


def test47():
    print("###############################################################################")
    print("Test 47 not implemented. This test will be provided for the ION Open Source 4.2")
    print("release to test the execution of multiple security policy processing actions")
    print("in response to a confidentiality corruption security operation event.")
    print("###############################################################################")


def test48():
    print("###############################################################################")
    print("Test 48 not implemented. This test will be provided for the ION Open Source 4.2")
    print("release to test the \'report_reason_code\' security policy processing action behavior.")
    print("###############################################################################")


def test49(inputScId=testUtils.BIB_HMAC_SHA2_SCID, inputScParams=testUtils.bibScParms):
    print("###############################################################################")

    try:
        testUtils.start_test(
            "49", "Brief: BIB targeting a large Payload (1MB).\n\n"
                 "Description: Add a BIB targeting a large Payload Block (1MB) at \n"
                 "Security Source ipn:2.1 and process that BIB at Security \n"
                 "Acceptor ipn:3.1\n\n"
                 "Bundle Path: ipn:2.1 -> ipn:3.1\n\n")
        time.sleep(testUtils.TIME_DISPLAYTEXT)

        # Tracking current position in log
        log2 = testUtils.log_position("2")
        log3 = testUtils.log_position("3")

        print("Node ipn:2.1 Configuration\n")
        testUtils.add_key(2, "key_1_32bytes")
        testUtils.add_event_set(2, "d_integrity", "default bib")

        desc = "bib src tgt:payload"
        filter = testUtils.build_filter("1", "s", "ipn:2.1", tgt=PAYLOAD, sc_id=inputScId)
        spec = testUtils.build_spec("bib-integrity", sc_params=inputScParams)
        es_ref = "d_integrity"
        testUtils.add_policy_rule(2, desc, filter, spec, es_ref)

        if testUtils.g_verbose == "detailed":
            testUtils.list_key(2)
            testUtils.list_event_set(2)
            testUtils.list_policy_rule(2)
        if testUtils.g_verbose == "verbose":
            testUtils.list_key(2)
            testUtils.info_event_set(2, "d_integrity")
            testUtils.info_policy_rule(2, 1)

        print("Node ipn:3.1 Configuration \n")
        testUtils.add_key(3, "key_1_32bytes")
        testUtils.add_event_set(3, "d_integrity", "default bib")

        desc = "bib acceptor tgt:payload"
        filter = testUtils.build_filter("2", "a", "ipn:2.1", tgt=PAYLOAD)
        spec = testUtils.build_spec("bib-integrity", sc_id=inputScId, sc_params=inputScParams)
        es_ref = "d_integrity"
        testUtils.add_policy_rule(3, desc, filter, spec, es_ref)

        if testUtils.g_verbose == "detailed":
            testUtils.list_key(3)
            testUtils.list_event_set(3)
            testUtils.list_policy_rule(3)
        if testUtils.g_verbose == "verbose":
            testUtils.list_key(3)
            testUtils.info_event_set(3, "d_integrity")
            testUtils.info_policy_rule(3, 2)

        # Generate the large payload for this bundle
        testUtils.gen_large_file(2, "test49.txt", 1000000)

        time.sleep(2)

        # Set up bprecvfile at destination ipn:3.1
        testUtils.recv_file(3)

        # Send the file from ipn:2.1 -> ipn:3.1
        testUtils.send_file(2, 3, "test49.txt", 120)

        print("\n\nClear Node ipn:2.1 Policy \n")
        testUtils.del_policy_rule(2, "1")
        testUtils.del_event_set(2, "d_integrity")
        testUtils.del_key(2, "key_1_32bytes")

        print("Clear Node ipn:3.1 Policy\n")
        testUtils.del_policy_rule(3, "2")
        testUtils.del_event_set(3, "d_integrity")
        testUtils.del_key(3, "key_1_32bytes")

        # Verify results on ipn:2.1
        log2_events = testUtils.get_test_events(2, log2)
        te_1, log2_events = testUtils.find_test_event(
            log2_events, 'sop_added_at_src', 'ipn:2.1', 'ipn:3.1', 'bib-integrity', '1', None, None)

        # Verify results on ipn:3.1
        log3_events = testUtils.get_test_events(3, log3)
        te_2, log3_events = testUtils.find_test_event(
            log3_events, 'sop_processed', 'ipn:2.1', 'ipn:3.1', 'bib-integrity', '1', te_1['msec'], te_1['count'])

        # Process all test results
        testUtils.check_test_results([3], ["payload length is 1000000"], [log2_events, log3_events])

        if os.path.exists('2.ipn.ltp/test49.txt'):
            os.remove('2.ipn.ltp/test49.txt')

    except subprocess.TimeoutExpired as e:
        print(e)
        print("TEST FAILED")
        testUtils.g_tests_failed += 1
        os.chdir("..")

    print("###############################################################################")


def test50(inputScId=testUtils.BCB_AES_GCM_SCID, inputScParams=testUtils.bcbScParms):
    print("###############################################################################")

    try:
        testUtils.start_test(
            "50", "Brief: BCB targeting a large Payload (1MB).\n\n"
                 "Description: Add a BCB targeting a large Payload Block (1MB) at \n"
                 "Security Source ipn:2.1 and process that BCB at Security \n"
                 "Acceptor ipn:3.1\n\n"
                 "Bundle Path: ipn:2.1 -> ipn:3.1\n\n")
        time.sleep(testUtils.TIME_DISPLAYTEXT)

        # Tracking current position in log
        log2 = testUtils.log_position("2")
        log3 = testUtils.log_position("3")

        print("Node ipn:2.1 Configuration\n")
        testUtils.add_key(2, "bcb_key_32bytes")
        testUtils.add_event_set(2, "d_bcb_conf", "default bcb")

        desc = "bcb src tgt:payload"
        filter = testUtils.build_filter("1", "s", "ipn:2.1", tgt=PAYLOAD, sc_id=inputScId)
        spec = testUtils.build_spec("bcb-confidentiality", sc_params=inputScParams)
        es_ref = "d_bcb_conf"
        testUtils.add_policy_rule(2, desc, filter, spec, es_ref)

        if testUtils.g_verbose == "detailed":
            testUtils.list_key(2)
            testUtils.list_event_set(2)
            testUtils.list_policy_rule(2)
        if testUtils.g_verbose == "verbose":
            testUtils.list_key(2)
            testUtils.info_event_set(2, "d_bcb_conf")
            testUtils.info_policy_rule(2, 1)

        print("Node ipn:3.1 Configuration \n")
        testUtils.add_key(3, "bcb_key_32bytes")
        testUtils.add_event_set(3, "d_bcb_conf", "default bcb")

        desc = "bcb acceptor tgt:payload"
        filter = testUtils.build_filter("2", "a", "ipn:2.1", tgt=PAYLOAD)
        spec = testUtils.build_spec("bcb-confidentiality", sc_id=inputScId, sc_params=inputScParams)
        es_ref = "d_bcb_conf"
        testUtils.add_policy_rule(3, desc, filter, spec, es_ref)

        if testUtils.g_verbose == "detailed":
            testUtils.list_key(3)
            testUtils.list_event_set(3)
            testUtils.list_policy_rule(3)
        if testUtils.g_verbose == "verbose":
            testUtils.list_key(3)
            testUtils.info_event_set(3, "d_bcb_conf")
            testUtils.info_policy_rule(3, 2)

        # Generate the large payload for this bundle
        testUtils.gen_large_file(2, "test50.txt", 1000000)

        time.sleep(2)

        # Set up bprecvfile at destination ipn:3.1
        testUtils.recv_file(3)

        # Send the file from ipn:2.1 -> ipn:3.1
        testUtils.send_file(2, 3, "test50.txt", 120)

        print("\n\nClear Node ipn:2.1 Policy \n")
        testUtils.del_policy_rule(2, "1")
        testUtils.del_event_set(2, "d_bcb_conf")
        testUtils.del_key(2, "bcb_key_32bytes")

        print("Clear Node ipn:3.1 Policy\n")
        testUtils.del_policy_rule(3, "2")
        testUtils.del_event_set(3, "d_bcb_conf")
        testUtils.del_key(3, "bcb_key_32bytes")

        # Verify results on ipn:2.1
        log2_events = testUtils.get_test_events(2, log2)
        te_1, log2_events = testUtils.find_test_event(
            log2_events, 'sop_added_at_src', 'ipn:2.1', 'ipn:3.1', 'bcb-confidentiality', '1', None, None)

        # Verify results on ipn:3.1
        log3_events = testUtils.get_test_events(3, log3)
        te_2, log3_events = testUtils.find_test_event(
            log3_events, 'sop_processed', 'ipn:2.1', 'ipn:3.1', 'bcb-confidentiality', '1', te_1['msec'], te_1['count'])

        # Process all test results
        testUtils.check_test_results([3], ["payload length is 1000000"], [log2_events, log3_events])

        if os.path.exists('2.ipn.ltp/test50.txt'):
            os.remove('2.ipn.ltp/test50.txt')

    except subprocess.TimeoutExpired as e:
        print(e)
        print("TEST FAILED")
        testUtils.g_tests_failed += 1
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
    test9()
    test10()
    test11()
    test12()
    test13()
    test14()
    test15()
    test16()
    test17()
    test18()
    test19()
    test20()
    test21()
    test22()
    test23()
    test24()
    test25()
    test26()
    test27()
    test28()
    test29()
    test30()
    test31()
    test32()
    test33()
    test34()
    test35()
    test36()
    test37()
    test38()
    test39()
    test40()
    test41()
    test42()
    test43()
    test44()
    test45()
    test46()
    test47()
    test48()
    test49()
    test50()

def bib_tests():
    test1()
    test3()
    test5()
    test7()
    test9()
    test11()
    test13()
    test15()
    test17()
    test19()
    test21()
    test22()
    test25()
    test26()
    test27()
    test28()
    test29()
    test30()
    test31()
    test32()
    test33()
    test35()
    test37()
    test38()
    test42()
    test44()
    test46()
    test49()


def bcb_tests():
    test2()
    test4()
    test6()
    test8()
    test10()
    test12()
    test14()
    test16()
    test18()
    test20()
    test23()
    test24()
    test25()
    test26()
    test28()
    test29()
    test30()
    test31()
    test32()
    test34()
    test36()
    test39()
    test43()
    test45()
    test47()
    test50()


def payload_tgt():
    test1()
    test2()
    test5()
    test6()
    test7()
    test8()
    test9()
    test10()
    test12()
    test13()
    test14()
    test15()
    test16()
    test17()
    test18()
    test19()
    test20()
    test21()
    test22()
    test23()
    test24()
    test25()
    test26()
    test27()
    test28()
    test29()
    test30()
    test31()
    test32()
    test33()
    test34()
    test37()
    test39()
    test44()


def primary_tgt():
    test3()
    test4()
    test11()
    test31()
    test33()
    test38()


def help():
    print("Test format:\npython3 main.py <test> <test> \n\nTests to run: \n"
          "single tests: 1 - 50 \n"
          "group tests: 'all', 'bib', 'bcb', 'payload', 'primary' \n")


test_dict = {1: test1, 2: test2, 3: test3, 4: test4, 5: test5, 6: test6, 7: test7, 8: test8,
             9: test9, 10: test10, 11: test11, 12: test12, 13: test13, 14: test14, 15: test15,
             16: test16, 17: test17, 18: test18, 19: test19, 20: test20, 21: test21, 22: test22,
             23: test23, 24: test24, 25: test25, 26: test26, 27: test27, 28: test28, 29: test29,
             30: test30, 31: test31, 32: test32, 33: test33, 34: test34, 35: test35, 36: test36,
             37: test37, 38: test38, 39: test39, 40: test40, 41: test41, 42: test42, 43:test43,
             44: test44, 45: test45, 46: test46, 47: test47, 48: test48, 49: test49, 50: test50,
             "all": all_tests, 
             "bib_tests": bib_tests, "bib": bib_tests, 
             "bcb_tests": bcb_tests, "bcb": bcb_tests, 
             "payload tests": payload_tgt, "payload": payload_tgt, 
             "primary tests": primary_tgt, "primary": primary_tgt }
