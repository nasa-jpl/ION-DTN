#!/usr/bin/python
# from Kurtis Heimerl's pydtn (http://hg.sourceforge.net/hgweb/dtn/pydtn)
import sdnv
import struct

def array_into_string(array):
    return ''.join(map(chr,array))

__all__ = ( "encode", "decode", "VERSION" )

#
# The currently-implemented version of the bundle protocol
#
VERSION = 6

#
# Time conversion to go from 1/1/1970 to 1/1/2000
#
TIMEVAL_CONVERSION = 946684800

#
# Bit definitions for bundle flags.
#
BUNDLE_IS_FRAGMENT             = 1 << 0
BUNDLE_IS_ADMIN                = 1 << 1
BUNDLE_DO_NOT_FRAGMENT         = 1 << 2
BUNDLE_CUSTODY_XFER_REQUESTED  = 1 << 3
BUNDLE_SINGLETON_DESTINATION   = 1 << 4
BUNDLE_ACK_BY_APP              = 1 << 5
BUNDLE_UNUSED                  = 1 << 6

#
# COS values
#
COS_BULK      = 0 << 7
COS_NORMAL    = 1 << 7
COS_EXPEDITED = 2 << 7

#
# Status report request flags
#
STATUS_RECEIVED         = 1 << 14
STATUS_CUSTODY_ACCEPTED = 1 << 15
STATUS_FORWARDED        = 1 << 16
STATUS_DELIVERED        = 1 << 17
STATUS_DELETED          = 1 << 18
STATUS_ACKED_BY_APP     = 1 << 19
STATUS_UNUSED2          = 1 << 20

#
# Reason codes for status reports
#
REASON_NO_ADDTL_INFO              = 0x00
REASON_LIFETIME_EXPIRED           = 0x01
REASON_FORWARDED_UNIDIR_LINK      = 0x02
REASON_TRANSMISSION_CANCELLED     = 0x03
REASON_DEPLETED_STORAGE           = 0x04
REASON_ENDPOINT_ID_UNINTELLIGIBLE = 0x05
REASON_NO_ROUTE_TO_DEST           = 0x06
REASON_NO_TIMELY_CONTACT          = 0x07
REASON_BLOCK_UNINTELLIGIBLE       = 0x08
REASON_SECURITY_FAILED            = 0x09
    
#
# Custody transfer reason codes
#
CUSTODY_NO_ADDTL_INFO              = 0x00
CUSTODY_REDUNDANT_RECEPTION        = 0x03
CUSTODY_DEPLETED_STORAGE           = 0x04
CUSTODY_ENDPOINT_ID_UNINTELLIGIBLE = 0x05
CUSTODY_NO_ROUTE_TO_DEST           = 0x06
CUSTODY_NO_TIMELY_CONTACT          = 0x07
CUSTODY_BLOCK_UNINTELLIGIBLE       = 0x08

#
# Block type codes
#
PRIMARY_BLOCK               = 0x000
PAYLOAD_BLOCK               = 0x001
BUNDLE_AUTHENTICATION_BLOCK = 0x002
PAYLOAD_SECURITY_BLOCK      = 0x003
CONFIDENTIALITY_BLOCK       = 0x004
PREVIOUS_HOP_BLOCK          = 0x005
METADATA_BLOCK              = 0x008

#
# Block processing flags
#
BLOCK_FLAG_REPLICATE               = 1 << 0
BLOCK_FLAG_REPORT_ONERROR          = 1 << 1
BLOCK_FLAG_DISCARD_BUNDLE_ONERROR  = 1 << 2
BLOCK_FLAG_LAST_BLOCK              = 1 << 3
BLOCK_FLAG_DISCARD_BLOCK_ONERROR   = 1 << 4
BLOCK_FLAG_FORWARDED_UNPROCESSED   = 1 << 5
BLOCK_FLAG_EID_REFS                = 1 << 6

#
# Message type codes
#
DATA_SEGMENT  = 0x1
ACK_SEGMENT   = 0x2
REFUSE_BUNDLE = 0x3
KEEPALIVE     = 0x4
SHUTDOWN      = 0x5

#
# Connection flags for TCP Convergence Layer
#
REQUEST_BUNDLE_ACK    = 1 << 0
REQUEST_REACTIVE_FRAG = 1 << 1
INDICATE_NACK_SUPPORT = 1 << 2

#
# Bundle Data Transmission Flags
#
END = 1 << 0
START = 1 << 1

#depends on the above, so need to be imported after
import bundle 


def encode_noncbheeids(bundle):
    # Put the eid offsets into the dictionary and append their sdnvs
    # to the data buffer
    #this seems like an optimization --kurtis
    eids_buffer = ''
    dict_offsets = {}
    dict_buffer  = ''
    for eid in ( bundle.dest,
                 bundle.source,
                 bundle.replyto,
                 bundle.custodian ) :

        (scheme, ssp) = eid.split(':')
        
        if dict_offsets.has_key(scheme):
            scheme_offset = dict_offsets[scheme]
        else:
            dict_offsets[scheme] = scheme_offset = len(dict_buffer)
            dict_buffer += scheme
            dict_buffer += '\0'

        if dict_offsets.has_key(ssp):
            ssp_offset = dict_offsets[ssp]
        else:
            dict_offsets[ssp] = ssp_offset = len(dict_buffer)
            dict_buffer += ssp
            dict_buffer += '\0'

        eids_buffer += sdnv.sdnv_encode(scheme_offset)
        eids_buffer += sdnv.sdnv_encode(ssp_offset)
    
    return (eids_buffer, dict_buffer)

def encode_cbheeids(bundle):
    eids_buffer = ''
    dict_buffer = ''
    for eid in ( bundle.dest,
                 bundle.source,
                 bundle.replyto,
                 bundle.custodian ) :

        if eid == "dtn:none":
            eids_buffer += sdnv.sdnv_encode(0)
            eids_buffer += sdnv.sdnv_encode(0)
            continue

        (scheme, ssp) = eid.split(':')
        (node, service) = map(int,ssp.split('.'))

        eids_buffer += sdnv.sdnv_encode(node)
        eids_buffer += sdnv.sdnv_encode(service)

    return (eids_buffer, dict_buffer)

def can_cbhe(bundle):
    for eid in ( bundle.dest,
                 bundle.source,
                 bundle.replyto,
                 bundle.custodian ) :
        
        (scheme, ssp) = eid.split(':')
        if scheme != "ipn" and eid != "dtn:none":
            return False
    return True

def encode_eids(bundle):
    if can_cbhe(bundle):
        return encode_cbheeids(bundle)
    else:
        return encode_noncbheeids(bundle)



#----------------------------------------------------------------------
def encode(bundle):
    """Encodes all the bundle blocks, not including the preamble for the
    payload block or the payload data itself, into a binary string."""

    data = ''
    
    #------------------------------
    # Primary block
    #------------------------------
    
    (eids_buffer, dict_buffer) = encode_eids(bundle)
    data += eids_buffer
    
    # Now append the creation time and expiration sdnvs, the
    # dictionary length, and the dictionary itself
    data += sdnv.sdnv_encode(int(bundle.creation_secs))
    data += sdnv.sdnv_encode(bundle.creation_seqno)
    data += sdnv.sdnv_encode(bundle.expiration)
    data += sdnv.sdnv_encode(len(dict_buffer))
    data += dict_buffer

    # Now fill in the preamble portion, including the version,
    # processing flags and whole length of the block
    preamble = struct.pack('B', VERSION)
    preamble += sdnv.sdnv_encode(bundle.bundle_flags |
                            bundle.priority |
                            bundle.srr_flags)
    preamble += sdnv.sdnv_encode(len(data))
    return preamble + data

#----------------------------------------------------------------------
def encode_block_preamble(type, flags, eid_offsets, length):
    """Encode the standard preamble for a block"""
    
    eid_data = ''
    if len(eid_offsets) != 0:
        flags = flags | BLOCK_FLAG_EID_REFS

        eid_data = sdnv.sdnv_encode(len(eid_offsets))
        for o in eid_offsets:
            eid_data += sdnv.sdnv_encode(o)

    return ''.join((struct.pack('B', type),
                    sdnv.sdnv_encode(flags),
                    eid_data,
                    sdnv.sdnv_encode(length)))


def decode_cbhe_eids(offsets):
    return ("ipn:%d.%d" % (offsets[0][0], offsets[0][1]),
            "ipn:%d.%d" % (offsets[1][0], offsets[1][1]),
            "ipn:%d.%d" % (offsets[2][0], offsets[2][1]),
            "ipn:%d.%d" % (offsets[3][0], offsets[3][1]))

def decode_noncbhe_eids(bytes, offsets):
    eids = []
    for i in range(0,4):
        eids.append(__get_decoded_address(bytes, offsets[i][0], offsets[i][1]))
    return tuple(eids)

def decode_eids(bytes, offsets):
    if len(bytes) == 0:
        return decode_cbhe_eids(offsets)
    else:
        return decode_noncbhe_eids(bytes, offsets)


def decode_block(bytes):
    (type, typelen) = sdnv.sdnv_decode(bytes)
    bytes = bytes[typelen:]
    (flags, flagslen) = sdnv.sdnv_decode(bytes)
    bytes = bytes[flagslen:]
    if (flags & BLOCK_FLAG_EID_REFS):
        raise Exception("Block type %d has EID refs" % type)
    (length, lengthlen) = sdnv.sdnv_decode(bytes)
    bytes = bytes[lengthlen:]
    return (type, flags, bytes[:length], typelen + flagslen + lengthlen + length)

def decode_blocks(bytes):
    # Blocks is a dict of lists, keyed by block type.
    blocks = { }
    while len(bytes) > 0:
        (type, flags, blockpayload, totalblocklength) = decode_block(array_into_string(bytes))
        if blocks.has_key(flags):
            blocks[type].append( { "flags" : flags, "payload" : blockpayload } )
        else:
            blocks[type] = [ { "flags" : flags, "payload" : blockpayload } ]
        bytes = bytes[totalblocklength:]
    return blocks
    

#----------------------------------------------------------------------
#This should decode an encoded bundle, presumably including 
#the block preamble but not including the payload
#this is likely to explode from a bad block, i'm not adding
#error checks
#also assumes that the primary and preamble are completely contained
#takes an string containing the message
#
def decode(message):
    bytes = list(message)
    bytes = map(ord, bytes)
    version = bytes[0]
    if (version != VERSION):
        raise Exception("Version mismatch, decoding failed")
    i = 1
    #there's probably a smarter way to do this
    #my python-fu is weak
    (flags, block_length, i) = __decode_assist(i,bytes)
    (dest_sch_offset, dest_ssp_off, i) = __decode_assist(i,bytes)
    (source_sch_offset, source_ssp_off, i) = __decode_assist(
        i,bytes)
    (reply_sch_offset, reply_ssp_off, i) = __decode_assist(
        i,bytes)
    (cust_sch_offset, cust_ssp_off, i) = __decode_assist(
        i,bytes)
    (creation_ts, creation_ts_sq_no, i) = __decode_assist(
        i,bytes)
    (lifetime, dict_len, i) = __decode_assist(i,bytes)
    #should test if fragment, then see a fragment offset. 
    #Skipping as we say no fragments at this point

    #build the bundle
    b = bundle.Bundle()
    b.creation_secs = creation_ts
    b.creation_seqno = creation_ts_sq_no
    b.expiration = lifetime
    (b.dest, b.source, b.replyto, b.custodian) = decode_eids(bytes[i:i+dict_len],
        ((dest_sch_offset, dest_ssp_off),
         (source_sch_offset, source_ssp_off),
         (reply_sch_offset, reply_ssp_off),
         (cust_sch_offset, cust_ssp_off)))
    for flag in (BUNDLE_IS_FRAGMENT,
                 BUNDLE_IS_ADMIN,
                 BUNDLE_DO_NOT_FRAGMENT,
                 BUNDLE_CUSTODY_XFER_REQUESTED,
                 BUNDLE_SINGLETON_DESTINATION,
                 BUNDLE_ACK_BY_APP,
                 BUNDLE_UNUSED):
        b.bundle_flags += (flags & flag)
    for flag in (STATUS_RECEIVED,
                 STATUS_CUSTODY_ACCEPTED,
                 STATUS_FORWARDED,
                 STATUS_DELIVERED,
                 STATUS_DELETED,
                 STATUS_ACKED_BY_APP,
                 STATUS_UNUSED2):
        b.srr_flags += (flags & flag)
    b.priority = flags & (COS_BULK | COS_NORMAL | COS_EXPEDITED)

    # Skip the dictionary
    i += dict_len

    #ok, primary bundle done.  Decode other blocks.
    b.blocks = decode_blocks(bytes[i:])

    # Return
    if b.blocks.has_key(PAYLOAD_BLOCK):
        return (b, 
                len(b.blocks[PAYLOAD_BLOCK][0]["payload"]), 
                b.blocks[PAYLOAD_BLOCK][0]["payload"])
    else:
        return (b, 0, "")

def __decode_assist(len, array): #this is not efficient
    (one, temp_len) = sdnv.sdnv_decode(array_into_string(array[len:]))
    len += temp_len
    (two, temp_len) = sdnv.sdnv_decode(array_into_string(array[len:]))
    len += temp_len
    return (one, two, len)

def __get_decoded_address(array, sch_off, ssp_off):
    tmp = array[sch_off:]
    zero = __find_zero(tmp)
    res = array_into_string(tmp[0:zero]) + ":"
    tmp = array[ssp_off:]
    zero = __find_zero(tmp)
    res += array_into_string(tmp[0:zero])
    return res

def __find_zero(array):
    for i in range(len(array)):
        if (array[i] == 0):
            return i
    return -1

def get_bundle_id(bundle):
    return (bundle.source, bundle.creation_secs, bundle.creation_seqno)

#----------------------------------------------------------------------
if (__name__ == "__main__"):
    testbundlebytes = "\x06\x82\x16\x12\x02\x00\x01\x00\x01\x00\x00\x00\x81\x9f\xb6\xb0\x44\x01\x85\xa3\x00\x00\x13\x01\x02\x00\xff\x01\x09\x18\x30\x80\x00\x00\x09\x69\x70\x6e\x3a\x31\x33\x2e\x34\x32\x01\x81\x9d\xf8\xe3\x0c\x00\x01\x01\x05"
    from binascii import hexlify
    from bundle import *

    (b, b_len, b_rem) = decode(testbundlebytes)
    print b
    assert get_bundle_id(b) == ("ipn:1.0", 334338116, 1)

    b = Bundle()
    b.source = 'dtn:me'
    b.dest   = 'dtn:you'
    b.bundle_flags  |= BUNDLE_SINGLETON_DESTINATION
    b.bundle_flags  |= BUNDLE_CUSTODY_XFER_REQUESTED
    b.srr_flags     |= STATUS_DELETED | STATUS_DELIVERED
    b.payload = StringPayload("test")
    
    d = encode(b)
    preamble = encode_block_preamble(PAYLOAD_BLOCK,
                                     BLOCK_FLAG_LAST_BLOCK,
                                     [], len(b.payload))
    message = d + preamble + b.payload.data()
    print "encoded data: ", hexlify(message)
    (bundle, bundle_len, remainder) = decode(message)
    bundle.payload = StringPayload(remainder)
    
    print(b)
    print(bundle)
    
    assert(bundle == b)

