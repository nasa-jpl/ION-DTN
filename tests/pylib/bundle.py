# from Kurtis Heimerl's pydtn (http://hg.sourceforge.net/hgweb/dtn/pydtn)
import bp
import os
import time
import StringIO

class Bundle:
    last_seqno = 0

    def next_seqno(self):
        Bundle.last_seqno += 1
        return Bundle.last_seqno - 1
    
    #lots of defaults
    def __init__(self, bundle_flags = 0, priority = bp.COS_BULK, srr_flags = 0,
                 source = 'dtn:none', dest = 'dtn:none', replyto = 'dtn:none',
                 custodian = 'dtn:none', prevhop = 'dtn:none',
                 creation_secs = int(time.time()) - bp.TIMEVAL_CONVERSION,
                 creation_seqno = None, expiration = 300, payload = None):
        self.bundle_flags   = bundle_flags
        self.priority       = priority
        self.srr_flags      = srr_flags
        self.source         = source
        self.dest           = dest
        self.replyto        = replyto
        self.custodian      = custodian
        self.prevhop        = prevhop
        self.creation_secs  = creation_secs
        if (creation_seqno == None):
            self.creation_seqno = self.next_seqno()
        else:
            self.creation_seqno = creation_seqno
        self.expiration     = expiration
        self.payload        = payload

    def __eq__(self, other):
        try:
            return ((self.bundle_flags == other.bundle_flags) and
                (self.priority == other.priority) and
                (self.srr_flags == other.srr_flags) and
                (self.source == other.source) and
                (self.dest == other.dest) and
                (self.replyto == other.replyto) and
                (self.custodian == other.custodian) and
                (self.prevhop == other.prevhop) and
                (self.creation_secs == other.creation_secs) and
                (self.creation_seqno == other.creation_seqno) and
                (self.expiration == other.expiration) and
                (self.payload.data() == other.payload.data()))
        except AttributeError:
            return False

    def __str__(self):
        x = ''
        x += ("Bundle Flags:    %d" % self.bundle_flags) + "\n"
        x += ("Priority:        %d" % self.priority) + "\n"
        x += ("srr_flags:       %d" % self.srr_flags) + "\n"
        x += ("Source:          %s" % self.source) + "\n"
        x += ("Destination:     %s" % self.dest) + "\n"
        x += ("Reply To:        %s" % self.replyto) + "\n"
        x += ("Custodian:       %s" % self.custodian) + "\n"
        x += ("Previous Hop:    %s" % self.prevhop) + "\n"
        x += ("Creation:        %d seconds" % self.creation_secs) + "\n"
        x += ("Creation Seq No: %d" % self.creation_seqno) + "\n"
        x += ("Expiration:      %d seconds" % self.expiration) + "\n"
        return x
        
class Payload:
    pass

class StringPayload(Payload):
    def __init__(self, data):
        self.payload = data

    def __len__(self):
        return len(self.payload)

    #as string
    def data(self):
        return self.payload

    #file
    def get_file(self):
        return StringIO.StringIO(self.payload)

#rewritten to avoid many open connections to the same file
#symbian disagrees with it
class FilePayload(Payload):
    def __init__(self, filename):
        self.filename = filename

    def __len__(self):
        file = open(self.filename, 'r')
        file.seek(0, os.SEEK_END)
        ret = file.tell()
        file.close()
        return int(ret)

    def data(self):
        file = open(self.filename, 'r')
        file.seek(0, os.SEEK_SET)
        data = file.read()
        file.close()
        return data
        
    #need a new file handle
    def get_file(self, mode='r'):
        file = open(self.filename, mode)
        return file
