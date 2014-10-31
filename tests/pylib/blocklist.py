#!/usr/bin/python
# blocklist.py
# Turns a list of elements:                 [ 1 2 3 4 7 8 9 17 ]
# into a list of block tuples:              [ (1, 4) (7, 9) (17, 17) ]
#   or a list of block-length tuples:       [ (1, 4) (+3, 2) (+8, 1) ]
#
#	Author: Andrew Jenkins
#				University of Colorado at Boulder
#	Copyright (c) 2008-2011, Regents of the University of Colorado.
#	This work was supported by NASA contracts NNJ05HE10G, NNC06CB40C, and
#	NNC07CB47C.	

def listToBlocks(list):
    list = sorted(list)
    i = 0
    toReturn = [ ]
    while i < len(list):
        j = i
        while j+1 < len(list) and list[j+1] - list[i] == (j - i + 1):
            j += 1
        toReturn.append((list[i], list[j]))
        i = j + 1
    return toReturn

def blocksToLengthBlocks(blocks):
    toReturn = [ ]
    lastEnd = None
    for i in blocks:
        if (lastEnd == None):
            toReturn.append((i[0], i[1] - i[0] + 1))
        else:
            toReturn.append((i[0]-lastEnd, i[1]-i[0] + 1))
        lastEnd = i[1]
    return toReturn

def listToLengthBlocks(list):
    return blocksToLengthBlocks(listToBlocks(list))

def lengthBlocksToBlocks(lengthBlocks):
    toReturn = [ ]
    lastEnd = None
    for i in lengthBlocks:
        if (lastEnd == None):
            toReturn.append((i[0], i[0] + i[1] - 1))
            lastEnd = i[0] + i[1] - 1
        else:
            toReturn.append((lastEnd + i[0], lastEnd + i[0] + i[1] - 1))
            lastEnd = lastEnd + i[0] + i[1] - 1
    return toReturn

def blocksToList(blocks):
    toReturn = [ ]
    for i in blocks:
        toReturn.extend(range(i[0], i[1] + 1))
    return toReturn

def lengthBlocksToList(lengthBlocks):
    return blocksToList(lengthBlocksToBlocks(lengthBlocks))

def mergeBlocks(blocks):
    toReturn = [ ]
    for (curstart, curend) in sorted(blocks):
        try:
            (prevstart, prevend) = toReturn[-1]
            if prevend + 1 >= curstart:
                toReturn[-1] = (prevstart, curend)
            else:
                toReturn.append((curstart, curend))
        except IndexError:
            toReturn.append((curstart, curend))
    return toReturn

def mergeLengthBlocks(lengthBlocks):
    return blocksToLengthBlocks(mergeBlocks(lengthBlocksToBlocks(lengthBlocks)))

if __name__ == '__main__':
    testCase1 = [ [1,2,3,4,7,8,9,17],
                  [(1,4),(7,9),(17,17)],
                  [(1,4),(3,3),(8,1)] ]
    
    testCase2 = [ [1],
                  [ (1,1) ],
                  [ (1,1) ] ]
    
    testCase3 = [ range(1,75) + range(77,86),
                  [ (1,74), (77, 85) ],
                  [ (1,74), (3, 9) ] ]

    for i in [ testCase1, testCase2, testCase3 ]:
        assert listToBlocks(i[0]) == i[1]
        assert listToLengthBlocks(i[0]) == i[2]
        assert blocksToList(i[1]) == i[0]
        assert lengthBlocksToList(i[2]) == i[0]
        assert blocksToLengthBlocks(i[1]) == i[2]
        assert lengthBlocksToBlocks(i[2]) == i[1]

    mergeTestCase1 = [ [ (1, 10), (11, 20) ],
                       [ (1, 20) ],
                       [ (1, 20) ] ]

    mergeTestCase2 = [ [ (1, 15), (11, 20) ],
                       [ (1, 20) ],
                       [ (1, 20) ] ]

    mergeTestCase3 = [ [ (1, 9), (11, 20) ],
                       [ (1, 9), (11, 20) ],
                       [ (1, 9), (2, 10) ] ]

    mergeTestCase4 = [ [ (11, 20), (1, 5), (6, 10) ],
                       [ (1, 20) ],
                       [ (1, 20) ] ]

    for i in [ mergeTestCase1, mergeTestCase2, mergeTestCase3, mergeTestCase4 ]:
        assert mergeBlocks(i[0]) == i[1]
        assert mergeLengthBlocks(blocksToLengthBlocks(i[0])) == i[2]
