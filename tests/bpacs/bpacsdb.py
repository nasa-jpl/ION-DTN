#!/usr/bin/python
# Mapping from bundle IDs to custody IDs and vice-versa
#
#	Author: Andrew Jenkins
#				University of Colorado at Boulder
#	Copyright (c) 2008-2011, Regents of the University of Colorado.
#	This work was supported by NASA contracts NNJ05HE10G, NNC06CB40C, and
#	NNC07CB47C.		

from bpacs import dtntime

class BpAcsDb:
    """
    The ACS Database lets the user map from bundle IDs to custody IDs and vice-versa.
    
    A bundle ID is (sourceEid, creationTimestamp, creationCount, fragmentOffset, fragmentLength)
    A custody ID is (id)

    So, the AcsDb lets you map:
           (sEid, cT, cC, fO, fL)  ->  id       (with getCustodyId)
    or     id  ->  (sEid, cT, cC, fO, fL)       (with getBundleId)
    """
    
    
    # Mapping from custody ID to bundle ID.
    cid = { }
    
    # Mapping from bundle ID to custody ID.
    bid = { }   

    # Parameters to be used for making up new custody IDs
    nextAvailableId = 0
    
    def __init__(self):
        self.cid = { }
        self.bid = { }
        self.nextAvailableId = 0

    def getNewCustodyId(self):
        toReturn = (self.nextAvailableId)
        self.nextAvailableId += 1
        return toReturn

    def getCustodyId(self, bundleId, dontCreate = False):
        try:
            return self.bid[bundleId]
        except KeyError:
            if (dontCreate == True):
                return None
            newCid = self.getNewCustodyId()
            self.bid[bundleId] = newCid
            self.cid[newCid] = bundleId
            return newCid

    def getBundleId(self, custodyId):
        try:
            return self.cid[custodyId]
        except KeyError:
            return None

    def setCustodyId(self, bundleId, custodyId):
        self.bid[bundleId] = custodyId
        self.cid[custodyId] = bundleId
        if (self.nextAvailableId <= custodyId):
            self.nextAvailableId = custodyId + 1
    
if __name__ == '__main__':
    acsDb = BpAcsDb()

    bundle_zero   = ( "ipn:13.42", dtntime("6/30/2010 16:47:08"), 0 )
    bundle_one    = ( "ipn:13.42", dtntime("6/30/2010 16:47:08"), 1 )
    bundle_two    = ( "ipn:13.42", dtntime("6/30/2010 16:47:09"), 3 )
    bundle_three  = ( "ipn:13.42", dtntime("6/30/2010 16:47:09"), 4, 100, 1024 )
    bundle_four   = ( "ipn:13.42", dtntime("6/30/2010 16:47:09"), 9, 100, 1024 )
    bundle_fourdup = ( "ipn:13.42", dtntime("6/30/2010 16:47:09"), 9, 100, 1024 )
    bundle_five   = ( "ipn:13.42", dtntime("6/30/2010 16:47:11"), 12)
    bundle_six    = ( "ipn:13.42", dtntime("6/30/2010 16:47:11"), 17)

    # Verify the database will give you a new Custody ID.
    assert acsDb.getCustodyId(bundle_zero)  == 0
    assert acsDb.getCustodyId(bundle_one)   == 1

    # Verify the database caches pre-existing Custody IDs.
    assert acsDb.getCustodyId(bundle_zero)  == 0
    assert acsDb.getCustodyId(bundle_two)   == 2
    assert acsDb.getCustodyId(bundle_zero)  == 0
    assert acsDb.getCustodyId(bundle_three) == 3
    assert acsDb.getCustodyId(bundle_two)   == 2

    # Verify the database won't insert a new Custody ID if you ask it not to.
    assert acsDb.getCustodyId(bundle_four, dontCreate = True) == None
    assert acsDb.getCustodyId(bundle_four, dontCreate = False) == 4

    # Verify the database finds things by hashing the tuple rather than
    # referencing the object.
    assert acsDb.getCustodyId(bundle_fourdup) == 4

    # Verify we can look up Bundle IDs by Custody ID.
    assert acsDb.getBundleId(0) == bundle_zero
    assert acsDb.getBundleId(1) == bundle_one
    assert acsDb.getBundleId(2) == bundle_two
    assert acsDb.getBundleId(3) == bundle_three
    assert acsDb.getBundleId(4) == bundle_four
    assert acsDb.getBundleId(4) == bundle_fourdup

    # Set a custody ID.
    acsDb.setCustodyId(bundle_six, 11)
    assert acsDb.getCustodyId(bundle_six) == 11
