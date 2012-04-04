#!/usr/bin/python

# sdnv_decode() takes a string argument s, which is assumed to be an
#   SDNV.  The function returns a pair of the non-negative integer n
#   that is the numeric value encoded in the SDNV, and and integer l
#   that is the distance parsed into the input string.  If the slen
#   argument is not given (or is not a non-zero number) then, s is
#   parsed up to the first byte whose high-order bit is 0 -- the
#   length of the SDNV portion of s does not have to be pre-computed
#   by calling code.  If the slen argument is given as a non-zero
#   value, then slen bytes of s are parsed.  The value for n of -1 is
#   returned for any type of parsing error.
#
# NOTE: In python, integers can be of arbitrary size.  In other
#   languages, such as C, SDNV-parsing routines should take
#   precautions to avoid overflow (e.g. by using the Gnu MP library,
#   or similar).
#
def sdnv_decode(s, slen=0):
    n = long(0)
    for i in range(0, len(s)):
        v = ord(s[i])
        n = n<<7
        n = n + (v & 0x7F)
        if v>>7 == 0:
            slen = i+1
            break
        elif i == len(s)-1 or (slen != 0 and i > slen):
            n = -1 # reached end of input without seeing end of SDNV
    return (n, slen)

# sdnv_encode() returns the SDNV-encoded string that represents n.
#   An empty string is returned if n is not a non-negative integer
def sdnv_encode(n):
    r = ""
    # validate input
    if n >= 0 and (type(n) in [type(int(1)), type(long(1))]):
        flag = 0
        done = False
        while not done:
            # encode lowest 7 bits from n
            newbits = n & 0x7F
            n = n>>7
            r = chr(newbits + flag) + r
            if flag == 0:
                flag = 0x80
            if n == 0:
                done = True
    return r


# test cases from LTP and BP internet-drafts, only print failures
if __name__ == '__main__':
    tests = [(0xABC, chr(0x95) + chr(0x3C)),
             (0x1234, chr(0xA4) + chr (0x34)),
             (0x4234, chr(0x81) + chr(0x84) + chr(0x34)),
             (0x7F, chr(0x7F))]
    
    for tp in tests:
        # test encoding function
        if sdnv_encode(tp[0]) != tp[1]:
            print "sdnv_encode fails on input %s" % hex(tp[0])
        # test decoding function
        if sdnv_decode(tp[1])[0] != tp[0]:
            print "sdnv_decode fails on input %s, giving %s" % \
                    (hex(tp[0]), sdnv_decode(tp[1]))

