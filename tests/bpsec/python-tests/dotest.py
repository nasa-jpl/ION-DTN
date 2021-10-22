import sys
import time

from test_cases import testUtils
from test_cases import testCases

'''
    The Johns Hopkins University Applied Physics Laboratory (JHU/APL)
    
    File Name: dotest.py
    
    Description: This python test script has been written for BPSec. 
    Tests cover bib-integrity and bcb-confidentiality security services
    and test features such as target multiplicity.
    
    To run the test script, use the following command:
        python3 dotest.py <test>
        Where test is a number of an individual test to run (tests 1-39 are 
        supported). 
        Multiple tests can be specified, separated by a space. 
        To run a group of tests, provide one of the following group names
        rather than a single test number:
            all
            bib
            bcb
            payload
            primary
'''

time.sleep(1)

# Track the total number of tests passed and failed
testUtils.g_tests_passed = 0
testUtils.g_tests_failed = 0

lst = sys.argv[1:]

for x in lst:
    testCases.test_dict[x]()

testUtils.stop_and_clean()

print("\n###############################################################################")
print("Test(s) completed.")
print("\tTests passed: " + str(testUtils.g_tests_passed))
print("\tTests failed: " + str(testUtils.g_tests_failed))
print("###############################################################################")
