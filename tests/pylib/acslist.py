#!/usr/bin/python
# Gets the data from acslist into a useful python format.

import subprocess
import re
from binascii import unhexlify

cbid_re = re.compile(r"\((?P<sourceEid>[^,]*),(?P<timestamp>\d*),(?P<count>\d*),(?P<fragOffset>\d*),(?P<fragLength>\d*)\)->\((?P<custodyId>\d*)")
mismatch_re = re.compile(r"Mismatch: (?P<mismatch>.*)")

def acslist_line_to_ids(line, cbids, mismatches):
    # Try to match this line to a CBID line; if match, put result in cbids.
    mo = cbid_re.match(line)
    if mo != None:
        key = mo.group("sourceEid", "timestamp", "count", "fragOffset", "fragLength")
        key = (key[0], int(key[1]), int(key[2]), int(key[3]), int(key[4]))
        val = mo.group("custodyId")
        val = int(val)
        cbids[key] = val
        return

    # Try to match this line to a mismatch; if it is a mismatch, put result in mismatches.
    mo = mismatch_re.match(line)
    if mo != None:
        mismatches.append(mo.group("mismatch"))
        return

def acslist_lines_to_cbids(lines):
    cbids = { }
    mismatches = [ ]

    # Iterate through the acslist output.
    for line in lines:
        acslist_line_to_ids(line, cbids, mismatches)
    return (cbids, mismatches)
    

def acslist():
    """
    Executes the "acslist" command, and returns a dictionary of 
    custody/bundle IDs that acslist reports, and any mismatches reported, like:

    ( { ("ipn:1.2", 946684800, 2, 0, 0) : ( 0 ) },
      [ "creation time in database (946684800, 1) != in key (946684800, 2)" ] )
    or, returns None if there is a problem parsing the output of acslist.
    """
    # bufsize = 1 --> line buffered

    import sys
    mswindows = (sys.platform == 'win32')

    if mswindows:
      #close_fds raises a ValueError exception on windows.
      acslist_p = subprocess.Popen("acslist -s", shell = True, stdout=subprocess.PIPE, bufsize = 1)
    else:
      acslist_p = subprocess.Popen("acslist -s", shell = True, close_fds = True, stdout=subprocess.PIPE, bufsize = 1)

    return acslist_lines_to_cbids(acslist_p.stdout)

if __name__ == '__main__':
    sample_acslist_output = """
(ipn:13.42,331231628,1,0,0)->(0)
(ipn:13.42,331231628,16,0,0)->(9)
(ipn:13.42,331231628,5,0,0)->(5)
(ipn:13.42,331231628,2,0,0)->(1)
(ipn:13.42,331231628,17,0,0)->(10)
Mismatch: creation time in database (331231628, 17) != in key (331231628, 27)
(ipn:13.42,331231628,9,0,0)->(6)
(ipn:13.42,331231628,4,0,0)->(2)
(ipn:13.42,331231628,23,0,0)->(11)
(ipn:13.42,331231628,14,0,0)->(7)
(ipn:13.42,331231628,3,0,0)->(3)
(ipn:13.42,331231628,25,0,0)->(12)
(ipn:13.42,331231628,15,0,0)->(8)
(ipn:13.42,331231628,11,0,0)->(4)
(ipn:13.42,331231628,0,0,0)->(13)
14 custody IDs
""".split("\n")

    (cbids, mismatches) = acslist_lines_to_cbids(sample_acslist_output)

    assertedCbids = { ("ipn:13.42", 331231628,  1, 0, 0) : (0),
                      ("ipn:13.42", 331231628, 16, 0, 0) : (9),
                      ("ipn:13.42", 331231628,  5, 0, 0) : (5),
                      ("ipn:13.42", 331231628,  2, 0, 0) : (1),
                      ("ipn:13.42", 331231628, 17, 0, 0) : (10),
                      ("ipn:13.42", 331231628,  9, 0, 0) : (6),
                      ("ipn:13.42", 331231628,  4, 0, 0) : (2),
                      ("ipn:13.42", 331231628, 23, 0, 0) : (11),
                      ("ipn:13.42", 331231628, 14, 0, 0) : (7),
                      ("ipn:13.42", 331231628,  3, 0, 0) : (3),
                      ("ipn:13.42", 331231628, 25, 0, 0) : (12),
                      ("ipn:13.42", 331231628, 15, 0, 0) : (8),
                      ("ipn:13.42", 331231628, 11, 0, 0) : (4),
                      ("ipn:13.42", 331231628,  0, 0, 0) : (13) }
    assertedMismatches = [ "creation time in database (331231628, 17) != in key (331231628, 27)" ] 

    assert (cbids == assertedCbids)
    assert (mismatches == assertedMismatches)
