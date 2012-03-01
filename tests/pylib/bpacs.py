#!/usr/bin/python
# Parses and serializes aggregate custody signals.
#
#	Author: Andrew Jenkins
#				University of Colorado at Boulder
#	Copyright (c) 2008-2011, Regents of the University of Colorado.
#	This work was supported by NASA contracts NNJ05HE10G, NNC06CB40C, and
#	NNC07CB47C.			

import pdb
from sdnv import sdnv_encode, sdnv_decode
from blocklist import lengthBlocksToBlocks, blocksToLengthBlocks, mergeBlocks, lengthBlocksToList
from time import time, strptime
from calendar import timegm
import struct

DTNEPOCH = 946684800

#       Class hierarchy:
#
#                           BpAcs
#                           /    \
#                    BpAcsFill  BpAcsFill (not a class, just a list of tuples)
#
# Users of this library should only have to interact with the BpAcs class,
# and the unserialize_acs() function.
#
# You can create a new Aggregate Custody Signal by creating a BpAcs instance
# (yourACS = BpAcs()), and calling yourACS.add() repeatedly.  You can see the
# contents of your ACS with repr(yourACS).  You can serialize your ACS with
# yourACS.serialize().
#
# If you have a string of bytes that represent an ACS (either from
# yourACS.serialize() or elsewhere), you can turn them into a BpAcs instance
# with unserialize_acs().
#
# You can compare two ACS with "=="; two ACS are equal if they:
#   - Have the same "custody transfer successful" flag value.
#   - Have the same reason code.
#   - Have the same signal time.
#   - Cover the same set of custody IDs
# (i.e. each attribute of the two ACS are equal)

class BpAcs:
    """A bundle protocol aggregate custody signal"""
    succeeded = 1
    reason = None
    fills = [ ]
    signalTime = None
    def __init__(self, bundle = None, **kwargs):
        self.fills = [ ]
        if bundle != None:
            self.add(bundle)
        if kwargs.has_key("succeeded"):
            self.succeeded = kwargs["succeeded"]
        if kwargs.has_key("reason"):
            self.reason = kwargs["reason"]
                
    
    def add(self, custodyId):
        """Adds a custodyId to the set of custodyIds that this ACS covers.
        """
        fillsAsBlocks = lengthBlocksToBlocks(self.fills)
        fillsAsBlocks = mergeBlocks(fillsAsBlocks + [(custodyId, custodyId)])
        self.fills = blocksToLengthBlocks(fillsAsBlocks)
    
    def __repr__(self):
        """Prints a friendly representation of the bundles this ACS covers, like:
             SACK: #0 1(3) +9(2)
           (Reporting custody acceptance of bundles with custody IDs 
            1, 2, 3, 12 and 13.)
        """   
        toReturn = ""
        if self.succeeded == 1:
            toReturn += "SACK:"
        else:
            toReturn += "SNACK:"

        printedFirstFill = 0
        for (start, length) in self.fills:
            if not (printedFirstFill):
                toReturn += " %d(%d)" % (start, length)
                printedFirstFill = 1
            else:
                toReturn += " +%d(%d)" % (start, length)
        return toReturn
   
    def serialize(self):
        """Serializes this ACS into a string of bytes that constitute the payload
           of an aggregate custody signal.  This serialization does not include the
           bundle primary block, or the payload block header; it is only the payload
           of the payload block.
        """
        # Encode Administrative Record header byte
        toReturn = "\x40"           # Aggregate Custody Signal, not for a fragment.

        # Encode status byte
        toReturn += struct.pack('!B', 128*self.succeeded)
        
        # Encode the array of fills.
        for (start, length) in self.fills:
            toReturn += sdnv_encode(start)
            toReturn += sdnv_encode(length)
        return toReturn
    
    def __cmp__(self, other):
        try:
            if  self.succeeded == other.succeeded and \
                self.reason == other.reason:
                    return cmp(self.fills, other.fills)
        except AttributeError:
            return -1
        return -1

def unserialize_acs(acs_string):
    """Counterpart to BpAcs.serialize(); takes a string of bytes that are the
       payload of an aggregate custody signal and turns them into an instance of
       the BpAcs class.
       acs_string must be the payload of the payload block of an aggregate
       custody signal bundle (i.e. acs_string must not include a bundle primary
       block, or the payload block header).
    """
    toReturn = BpAcs()
    
    (adminrecordheader, status, ) = struct.unpack("!BB", acs_string[0:2])
    acs_string = acs_string[2:]
    
    # Parse the administrative record header byte.
    if (adminrecordheader & 0xF0) != 0x40:
        # Not an aggregate custody signal.
        return None
    if (adminrecordheader & 0x0F) != 0x00:
        print "Administrative record flags are %x, not 0x00" % (adminrecordheader & 0x0F) 
        raise TypeError
    
    # Parse the status byte
    if (status & 0x80) == 0:
        toReturn.succeeded = 0
    else:
        toReturn.succeeded = 1
    if status & 0x7F:
        toReturn.reason = status & 0x7F
    
    # Parse the fills
    lengthBlocks = []
    while acs_string != "":
        (offset, n) = sdnv_decode(acs_string)
        acs_string = acs_string[n:]
        (length, n) = sdnv_decode(acs_string)
        acs_string = acs_string[n:]
        lengthBlocks += (offset, length),
    for k in lengthBlocksToList(lengthBlocks):
        toReturn.add(k)
    return toReturn


dtntime = lambda x: timegm(strptime(x, "%m/%d/%Y %H:%M:%S")) - DTNEPOCH

if __name__ == '__main__':
    acs = BpAcs()
    
    # Check serialization
    acs_one = BpAcs(0)
    acs_one.add(1)
    acs_one.add(3)
    acs_one.add(7)
    acs_one.add(8)
    acs_one.add(9)
    acs_one.add(15)
    
    assert repr(acs_one) == "SACK: 0(2) +2(1) +4(3) +6(1)"
    assert acs_one.serialize() == "\x40\x80\x00\x02\x02\x01\x04\x03\x06\x01"
    
    acs_two = BpAcs(0, succeeded = 0)
    acs_two.add(3)
    assert repr(acs_two) == "SNACK: 0(1) +3(1)"
    assert acs_two.serialize() == "\x40\x00\x00\x01\x03\x01"
    
    # Check unserialization
    assert acs_two == unserialize_acs(acs_two.serialize())
    assert acs_one == unserialize_acs(acs_one.serialize())
    assert acs_one != unserialize_acs(acs_two.serialize())

    # Fill a hole
    acs_one.add(2)
    assert acs_one.serialize() == "\x40\x80\x00\x04\x04\x03\x06\x01"
    assert acs_one == unserialize_acs(acs_one.serialize())
    
    # Check comparison
    assert acs_one != acs_two
    assert acs_one == acs_one
    acs_two.succeeded = 1
    assert acs_one != acs_two
    acs_two.add(1)
    assert acs_one != acs_two
    acs_two.signalTime = acs_one.signalTime
    assert acs_one != acs_two
    acs_two.add(2)
    acs_two.add(7)
    acs_two.add(8)
    acs_two.add(9)
    acs_two.add(15)

    # Check bundleization.
    import bundle, bp
    bundle_one = bundle.Bundle(bp.BUNDLE_IS_ADMIN, bp.COS_BULK, 0, "ipn:26.40", "ipn:13.42", "dtn:none", "dtn:none", "dtn:none", timegm((2010, 07, 01, 7, 03, 00)) - bp.TIMEVAL_CONVERSION, 0, 86400*7, acs_one.serialize())
#    print bp.encode(bundle_one) + bp.encode_block_preamble(bp.PAYLOAD_BLOCK, bp.BLOCK_FLAG_LAST_BLOCK, [], len(bundle_one.payload)) + bundle_one.payload
