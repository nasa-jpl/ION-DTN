#!/usr/bin/python
# Gets the data from bplist into a useful python format.
#
#	Author: Andrew Jenkins
#				University of Colorado at Boulder
#	Copyright (c) 2008-2011, Regents of the University of Colorado.
#	This work was supported by NASA contracts NNJ05HE10G, NNC06CB40C, and
#	NNC07CB47C.			

import subprocess
import re
from bp import decode_block
from binascii import unhexlify

def readOptionalMatchesThenNext(bundle, line, statename, **kwargs):
    assert kwargs.has_key("nextifmatched")
    assert kwargs.has_key("nextifunmatched")
    assert kwargs.has_key("re")

    mo = kwargs["re"].match(line)
    
    # If we didn't match our expected line, then short circuit.
    if mo == None:
        if kwargs["nextifunmatched"] == None:
            return None
        nextifunmatched_name = kwargs["nextifunmatched"]
        nextifunmatched_value = bplistline_statemachine[nextifunmatched_name]
        return nextifunmatched_value[0](bundle,
                 line,
                 nextifunmatched_name,
                 **nextifunmatched_value[1])

    for k, v in mo.groupdict().items():
        # If we're dealing with an integer field, convert it.
        if k in [ "timestamp", "count", "fragOffset", 
                  "priority", "ordinal", "expiration", "aduLen", "dictLen", "paylen" ]:
            v = int(v)
        
        # If we're dealing with a binary field, convert it.
        if k in [ "frag", "admin", "dontFrag", "custodial", "destSingleton",
                  "appAckRequested", "unreliable", "critical" ]:
            v = bool(int(v))
        
        bundle[k] = v
    
    return kwargs["nextifmatched"]

# Simple state implementation that takes a regular expression with named
# match groups and applies it to line.  Any resulting matches are just added
# to the bundle dictionary with:
#  key:   name of the match group from the regular expression.
#  value: value of the match.
# It finds itself in the set of states and returns the state after itself.
def readMatchesThenNext(bundle, line, statename, **kwargs):
    assert kwargs.has_key("next")
    kwargs["nextifunmatched"] = None
    kwargs["nextifmatched"] = kwargs.pop("next")
    return readOptionalMatchesThenNext(bundle, line, statename, **kwargs)

# State implementation that extracts chunks of bytes and adds them to a
# key in the bundle dictionary.  If there are no more chunks of bytes in the
# input line, it will short-circuit to shortcircuitstate.  Useful for chunks
# that don't define in advance how big they are, so you have to keep walking
# until you fail to match to know that you're finished.
def readUnlimitedChunksThenNext(bundle, line, statename, **kwargs):
    assert kwargs.has_key("chunkName")
    assert kwargs.has_key("next")
    assert kwargs.has_key("re")

    mo = kwargs["re"].match(line)
    
    # If we didn't match our expected line, then short circuit.
    if mo == None:
        if kwargs["next"] == None:
            return None
        next_name = kwargs["next"]
        next_value = bplistline_statemachine[kwargs["next"]]
        return next_value[0](bundle,
                line,
                next_name,
                **next_value[1])

    strippedchunk = mo.groupdict()["chunkbytes"].strip("\n").replace(" ", "")

    if bundle.has_key(kwargs["chunkName"]):
        bundle[kwargs["chunkName"]] += strippedchunk
    else:
        bundle[kwargs["chunkName"]] = strippedchunk
        
    # We did match a chunk, so come back to this state again.
    return statename

# State implementation that needs to be called when an extension block
# has been totally parsed.
#
# bplist prints extension blocks like this:
#
#
# ****** Extension
# (     0)  0a010a01 00076970 6e3a312e 30                  ......ipn:1.0
# ****** Extension
# (     0)  0b010a01 00076970 6e3a312e 30303030 30303030   ......ipn:1.0000000
# (    20)  31313131 31313131 31313131 30303030 30303030   1111111111110000000
# ****** Payload
#
# The challenge is that the length of the extension isn't announced anywhere.
# You don't know you've finished with an extension until you get a new extension
# header or something that's not a bunch of bytes.  Also, it is possible for
# the same extension block type to appear more than once in a bundle.
#
# This "state implementation" is intended to be called whenever you leave the
# extension block state.  It takes the current array of bytes in extensionBytes,
# and extracts the extension block type out of it, and appends it to the list
# of extension blocks of that type in bundle.extensions[ebtype].
#
# Then, it falls through to state fallthroughname.
def parseExtensionBytes(bundle, line, statename, **kwargs):
    assert kwargs.has_key("fallthroughname")

    if bundle.has_key("extensionBytes"):
        eb = decode_block(unhexlify(bundle["extensionBytes"]))
        if not bundle.has_key("extensions"):
            bundle["extensions"] = { }
        if not bundle["extensions"].has_key(eb[0]):
            bundle["extensions"][eb[0]] = [ ]
        bundle["extensions"][eb[0]].append(eb)
        del bundle["extensionBytes"]

    next_name = kwargs["fallthroughname"]
    next_value = bplistline_statemachine[kwargs["fallthroughname"]]
    return next_value[0](bundle,
        line,
        next_name,
        **next_value[1])
    

 
# The definition of the state machine that takes text from bplist and parses
# it into a representation of bundles.  A dictionary:
#  statename : ( stateImplementation, kwargs )
#
# If kwargs has a key "re", the value of that key is assumed to be a regular
# expression and will be compiled on module instantiation for better
# performance.
#
# The regular expression is precompiled by the compileStateMachine call.
# The stateImplementation takes as arguments:
#   bundle:         A dictionary, keyed by bplist params like "sourceEid"
#   line:           The current line passed as input to this state.
#   statename:      From state machine definition.
#   kwargs:         The kwargs (above) associated with the key statename.
#  the stateImplementation should do any matching or extraction necessary to add
#  keys to the bundle dictionary of whatever information is available.  It returns
#  the name of the next state to enter which could be itself, or could be a special
#  state "done" which appends the current bundle to the list of bundles and re-enters
#  the start state.
bplistline_statemachine = {
    "start" :           (readMatchesThenNext,
                            { "re" : r"",
                              "next" : "beginbundle" } ), 
    "beginbundle" :     (readMatchesThenNext,
                            { "re" : r"\*\*\*\* Bundle",
                              "next" : "source" } ),
    "source" :          (readMatchesThenNext,
                            { "re" : r"Source EID\s*'(?P<sourceEid>[^']*)'",
                              "next" : "dest" } ),
    "dest" :            (readMatchesThenNext, 
                            { "re" : r"Destination EID\s*'(?P<destEid>[^']*)'", 
                              "next" : "rptto" } ),
    "rptto" :           (readMatchesThenNext, 
                            { "re" : r"Report-to EID\s*'(?P<rpttoEid>[^']*)'", 
                              "next" : "cust" } ),
    "cust" :            (readMatchesThenNext,
                            { "re" : r"Custodian EID\s*'(?P<custEid>[^']*)'",
                              "next" : "id" } ),
    "id" :              (readMatchesThenNext,
                         { "re" : r"Creation sec\s*(?P<timestamp>[0-9]+)\s*count\s*(?P<count>[0-9]+)\s*frag offset\s*(?P<fragOffset>[0-9]+)",
                           "next" : "frag" } ),
    "frag" :            (readMatchesThenNext,
                            { "re" : r"- is a fragment:\s*(?P<frag>[01])",
                              "next" : "admin" } ),
    "admin" :           (readMatchesThenNext,
                            { "re" : r"- is admin:\s*(?P<admin>[01])",  "next" : "dontfrag" } ),
    "dontfrag" :        (readMatchesThenNext,
                            { "re" : r"- does not fragment:\s*(?P<dontFrag>[01])",
                              "next" : "custodial" } ),
    "custodial" :       (readMatchesThenNext,
                            { "re" : r"- is custodial:\s*(?P<custodial>[01])",
                              "next" : "destsingleton" } ),
    "destsingleton" :   (readMatchesThenNext,
                            { "re" : r"- dest is singleton:\s*(?P<destSingleton>[01])",  "next" : "appackreq" } ),
    "appackreq" :       (readMatchesThenNext,
                            { "re" : r"- app ack requested:\s*(?P<appAckRequested>[01])",
                              "next" : "priority" } ),
    "priority" :        (readMatchesThenNext,
                            { "re" : r"Priority\s*(?P<priority>[0-9]+)",
                              "next" : "ordinal" } ),
    "ordinal" :         (readMatchesThenNext,
                            { "re" : r"Ordinal\s*(?P<ordinal>[0-9]+)",
                              "next" : "unreliable" } ),
    "unreliable" :      (readMatchesThenNext,
                            { "re" : r"Unreliable:\s*(?P<unreliable>[01])",
                              "next" : "critical" } ),
    "critical" :        (readMatchesThenNext, 
                            { "re" : r"Critical:\s*(?P<critical>[01])",
                              "next" : "expires" } ),
    "expires" :         (readMatchesThenNext, 
                            { "re" : r"Expiration sec\s*(?P<expiration>[0-9]+)",
                              "next" : "adulen" } ),
    "adulen" :          (readMatchesThenNext, 
                            { "re" : r"Total ADU len\s*(?P<aduLen>[0-9]+)",
                              "next" : "dictionarylen" } ),
    "dictionarylen" :   (readMatchesThenNext, 
                            { "re" : r"Dictionary len\s*(?P<dictLen>[0-9]+)",
                              "next" : "dictionary" } ),
    "dictionary" :      (readMatchesThenNext, 
                            { "re" : r"Dictionary:\s*(?P<dictionary>.*)",
                              "next" : "extensionheader" } ),
    "extensionheader" : (readOptionalMatchesThenNext, 
                            { "re" : r"\*\*\*\*\*\* Extension", 
                              "nextifmatched" : "extensionbytes", 
                              "nextifunmatched" : "payloadlen" }),
    "extensionbytes" :  (readUnlimitedChunksThenNext,
                         { "re" : r"\(\s*[0-9]+\)  (?P<chunkbytes>[0-9a-f ]{44})",
                           "chunkName" : "extensionBytes",
                           "next" : "parseextbytes" }),
    "parseextbytes" :   (parseExtensionBytes,
                         { "fallthroughname" : "extensionheader" }),
    "payloadlen" :   (readMatchesThenNext, 
                            { "re" : r"Payload len\s*(?P<payLen>[0-9]+)",
                              "next" : "payloadheader" } ),
    "payloadheader" :   (readOptionalMatchesThenNext,
                         { "re": r"\*\*\*\*\*\* Payload",
                           "nextifmatched" : "payloadbytes",
                           "nextifunmatched" : "queue" }),
    "payloadbytes" :    (readUnlimitedChunksThenNext,
                         { "re" : r"\(\s*[0-9]+\)  (?P<chunkbytes>[0-9a-f ]{44})",
                           "chunkName" : "payloadBytes",
                           "next" : "queue" }),
    "queue" :          (readMatchesThenNext,
                            { "re" : r"\*\*\*\*\*\* Awaiting completion of convergence-layer transmission.",
                              "next" : "endofbundle" } ),
    "endofbundle" :     (readMatchesThenNext, 
                         { "re" : r"\*\*\*\* End of bundle",
                           "next" : "done" })
}

# Compile the regular expressions of each state in the state machine.  This
# makes matching faster.
for k, v in bplistline_statemachine.items():
    if v[1] == None or not v[1].has_key("re"):
        # No regexp to compile.
        continue
    
    bplistline_statemachine[k][1]["re"] = re.compile(v[1]["re"])


def bplist_line_to_bundles(bundles, line, state):
    (statename, bundle) = state

    # Find the implementation of this state and execute.
    statestep = bplistline_statemachine[statename]
    nextstatename = statestep[0](bundle, line, statename, **statestep[1])

    if nextstatename == None:
        # Error.
        return None

    if nextstatename == "done":
        bundles.append(bundle)
        bundle = { }
        nextstatename = "start"

    return (nextstatename, bundle)

def bplist_lines_to_bundles(lines):
    bundles = []
    state = ("start", { })

    # Iterate through the bplist output.
    for line in lines:
	if line.strip() == "Reporting detail of all bundles.":
	    continue
        state = bplist_line_to_bundles(bundles, line.strip(), state)
        #if state == None:
            #return None

    return bundles
    

def bplist():
    """
    Executes the "bplist" command, and returns a list of dictionaries
    of the bundles that bplist mentions, like:
    [   { "sourceEid" : "ipn:1.2", "timestamp" : 123456, "count" : 1 ... },
        { "sourceEid" : "ipn:1.2", "timestamp" : 123456, "count" : 2 ... },
        { "sourceEid" : "ipn:1.2", "timestamp" : 123456, "count" : 3 ... } ]
    or, returns None if there is a problem parsing the output of bplist.
    """
    # bufsize = 1 --> line buffered

    import sys
    mswindows = (sys.platform == "win32")

    if mswindows:
      #close_fds raises a ValueError exception on windows.
      bplist_p = subprocess.Popen("bplist", shell = True, stdout=subprocess.PIPE, bufsize = 1)
    else:
      bplist_p = subprocess.Popen("bplist", shell = True, close_fds = True, stdout=subprocess.PIPE, bufsize = 1)

    return bplist_lines_to_bundles(bplist_p.stdout)

if __name__ == '__main__':
    sample_bplist_output = """
**** Bundle
Source EID      'ipn:2.1'
Creation sec    332267883   count          1   frag offset          0
- is a fragment:        0
- is admin:             0
- does not fragment:    0
- is custodial:         1
- dest is singleton:    1
- app ack requested:    0
Priority                0
Ordinal                 0
Unreliable:             0
Critical:               0
Destination EID 'ipn:3.1'
Report-to EID   'ipn:2.0'
Custodian EID   'ipn:1.0'
Expiration sec  490090443
Total ADU len           0
Dictionary len          0
Dictionary:
****** Extension
(     0)  0a010a01 00076970 6e3a312e 30                  ......ipn:1.0
****** Payload
(     0)  68657265 20697320 61207472 61636520 62756e64   here is a trace bund
(    20)  6c6500                                         le.
**** End of bundle

**** Bundle
Source EID      'ipn:2.1'
Creation sec    332267883   count          2   frag offset          0
- is a fragment:        0
- is admin:             0
- does not fragment:    0
- is custodial:         1
- dest is singleton:    1
- app ack requested:    0
Priority                0
Ordinal                 0
Unreliable:             0
Critical:               0
Destination EID 'ipn:3.1'
Report-to EID   'ipn:2.0'
Custodian EID   'ipn:1.0'
Expiration sec  490090443
Total ADU len           0
Dictionary len          0
Dictionary:
****** Extension
(     0)  0a010a01 00076970 6e3a312e 30                  ......ipn:1.0
****** Payload
(     0)  68657265 20697320 74726163 65206275 6e646c65   here is trace bundle
(    20)  203200                                          2.
**** End of bundle

**** Bundle
Source EID      'ipn:2.1'
Creation sec    332267883   count          3   frag offset          0
- is a fragment:        0
- is admin:             0
- does not fragment:    0
- is custodial:         1
- dest is singleton:    1
- app ack requested:    0
Priority                0
Ordinal                 0
Unreliable:             0
Critical:               0
Destination EID 'ipn:3.1'
Report-to EID   'ipn:2.0'
Custodian EID   'ipn:1.0'
Expiration sec  490090443
Total ADU len           0
Dictionary len          0
Dictionary:
****** Extension
(     0)  0a010a01 00076970 6e3a312e 30                  ......ipn:1.0
****** Payload
(     0)  68657265 20697320 74726163 65206275 6e646c65   here is trace bundle
(    20)  203300                                          3.
**** End of bundle

**** Bundle
Source EID      'ipn:2.1'
Creation sec    332267883   count          4   frag offset          0
- is a fragment:        0
- is admin:             0
- does not fragment:    0
- is custodial:         1
- dest is singleton:    1
- app ack requested:    0
Priority                0
Ordinal                 0
Unreliable:             0
Critical:               0
Destination EID 'ipn:3.1'
Report-to EID   'ipn:2.0'
Custodian EID   'ipn:1.0'
Expiration sec  490090443
Total ADU len           0
Dictionary len          0
Dictionary:
****** Extension
(     0)  0a010a01 00076970 6e3a312e 30                  ......ipn:1.0
****** Payload
(     0)  68657265 20697320 74726163 65206275 6e646c65   here is trace bundle
(    20)  203400                                          4.
**** End of bundle

**** Bundle
Source EID      'ipn:2.1'
Creation sec    332267883   count          6   frag offset          0
- is a fragment:        0
- is admin:             0
- does not fragment:    0
- is custodial:         1
- dest is singleton:    1
- app ack requested:    0
Priority                0
Ordinal                 0
Unreliable:             0
Critical:               0
Destination EID 'ipn:3.1'
Report-to EID   'ipn:2.0'
Custodian EID   'ipn:1.0'
Expiration sec  490090443
Total ADU len           0
Dictionary len          0
Dictionary:
****** Extension
(     0)  0a010a01 00076970 6e3a312e 30                  ......ipn:1.0
****** Payload
(     0)  68657265 20697320 74726163 65206275 6e646c65   here is trace bundle
(    20)  203600                                          6.
**** End of bundle

**** Bundle
Source EID      'ipn:2.1'
Creation sec    332267883   count          7   frag offset          0
- is a fragment:        0
- is admin:             0
- does not fragment:    0
- is custodial:         1
- dest is singleton:    1
- app ack requested:    0
Priority                0
Ordinal                 0
Unreliable:             0
Critical:               0
Destination EID 'ipn:3.1'
Report-to EID   'ipn:2.0'
Custodian EID   'ipn:1.0'
Expiration sec  490090443
Total ADU len           0
Dictionary len          0
Dictionary:
****** Extension
(     0)  0a010a01 00076970 6e3a312e 30                  ......ipn:1.0
****** Payload
(     0)  68657265 20697320 74726163 65206275 6e646c65   here is trace bundle
(    20)  203700                                          7.
**** End of bundle

**** Bundle
Source EID      'ipn:2.1'
Creation sec    332267883   count          8   frag offset          0
- is a fragment:        0
- is admin:             0
- does not fragment:    0
- is custodial:         1
- dest is singleton:    1
- app ack requested:    0
Priority                0
Ordinal                 0
Unreliable:             0
Critical:               0
Destination EID 'ipn:3.1'
Report-to EID   'ipn:2.0'
Custodian EID   'ipn:1.0'
Expiration sec  490090443
Total ADU len           0
Dictionary len          0
Dictionary:
****** Extension
(     0)  0a010a01 00076970 6e3a312e 30                  ......ipn:1.0
****** Payload
(     0)  68657265 20697320 74726163 65206275 6e646c65   here is trace bundle
(    20)  203800                                          8.
**** End of bundle

**** Bundle
Source EID      'ipn:2.1'
Creation sec    332267883   count          9   frag offset          0
- is a fragment:        0
- is admin:             0
- does not fragment:    0
- is custodial:         1
- dest is singleton:    1
- app ack requested:    0
Priority                0
Ordinal                 0
Unreliable:             0
Critical:               0
Destination EID 'ipn:3.1'
Report-to EID   'ipn:2.0'
Custodian EID   'ipn:1.0'
Expiration sec  490090443
Total ADU len           0
Dictionary len          0
Dictionary:
****** Extension
(     0)  0a010a01 00076970 6e3a312e 30                  ......ipn:1.0
****** Payload
(     0)  68657265 20697320 74726163 65206275 6e646c65   here is trace bundle
(    20)  203900                                          9.
**** End of bundle
""".split("\n")

    #bundles = bplist_lines_to_bundles(sample_bplist_output)

    assertedBundle = { 
        'sourceEid' : 'ipn:2.1',
        'timestamp' : 332267883,
        'count' : 1,
        'fragOffset' : 0,
        'frag' : False,
        'admin' : False,
        'dontFrag' : False,
        'custodial' : True,
        'destSingleton' : True,
        'appAckRequested' : False,
        'priority' : 0,
        'ordinal' : 0,
        'unreliable' : False,
        'critical' : False,
        'destEid' : 'ipn:3.1',
        'rpttoEid' : 'ipn:2.0',
        'custEid' : 'ipn:1.0',
        'expiration' : 490090443,
        'aduLen' : 0,
        'dictLen' : 0,
        'dictionary' : '',
        'extensions' : { 10 : [(10, 1, '\x01\x00\x07ipn:1.0', 13)] },
        'payloadBytes' : "6865726520697320612074726163652062756e646c6500"
    }

    #assert (bundles[0] == assertedBundle)

    #for i in range(0, 8):
        #if i < 4:
            #assertedBundle["count"] = i + 1
        #else:
            #assertedBundle["count"] = i + 2

        # We don't bother trying to construct the payload for this test, just
        # cheat and copy the answer
        #assertedBundle["payloadBytes"] = bundles[i]["payloadBytes"]
        #try:
            #assert(bundles[i] == assertedBundle)
        #except AssertionError:
            #print "bundles[%d] != assertedBundle with count %d" % (i, assertedBundle["count"])
            #raise


    # If you want to try bplist "for real", enable this.
    # It isn't a default self-test because it requires a running instance of ION.
    bplistresults = bplist()
    for i in bplistresults:
        print "%s, %d, %d" % (i["sourceEid"], i["timestamp"], i["count"])
