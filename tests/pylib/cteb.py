#!/usr/bin/python

from struct import unpack
from sdnv import sdnv_decode

CTE_BLOCK = 0x0a

class Cteb:
    
    def __init__(self, bundle = None):
        # See if the bundle parser found one CTEB.
        if bundle == None or not bundle.blocks.has_key(CTE_BLOCK):
            self.custodyId = None
            self.custodian = None
            self.ctebPresent = False
            return
        if len(bundle.blocks[CTE_BLOCK]) != 1:
            raise Exception("Bundle has %d CTEBs, cannot handle." % 
                                (len(bundle.blocks[CTE_BLOCK])) )

        # Parse the CTEB.
        cteb_bytes = bundle.blocks[CTE_BLOCK][0]["payload"]
        i = 0
        self.ctebPresent = True

        # Custody ID
        (custodyId, sdnv_len) = sdnv_decode(cteb_bytes[i:])
        i += sdnv_len
        self.custodyId = custodyId

        # Custodian EID
        self.custodian = cteb_bytes[i:]

    def __repr__(self):
        if self.ctebPresent == False:
            return "No CTEB present"
        return "CTEB %s custodian: %s" % (self.custodyId, self.custodian)

if __name__ == '__main__':
    import bp
    from binascii import unhexlify

    # A bundle with a CTEB, custody ID 3, custodian "ipn:1.0"
    test_bundle_bytes = unhexlify("0618130301020102000100819eb8826b04cba0dc60000a01080369706e3a312e30010917686572652069732074726163652062756e646c65203400")
    (bundle, length, remainder) = bp.decode(test_bundle_bytes)
    cteb = Cteb(bundle)

    assert cteb.ctebPresent == True
    assert cteb.custodyId == 3
    assert cteb.custodian == "ipn:1.0"
    assert repr(cteb) == "CTEB 3 custodian: ipn:1.0"
