import struct

# transport layer segment
class rt_pkt:
    
    hdr_fields = ['master', 'slave', 'pkg_no', 'pkg_type', 'seg_ct', 'seg_no', 'payload', 'checksum']

    def __init__(self, raw=None, parms=None):
    
        self.checksum_good = True
    
        # call appropriate helper
        if raw != None and parms != None:
            raise Exception("Cannot use both raw and parms")
        elif raw == None and parms == None:
            raise Exception("Need to specify either raw or parms")
        elif raw != None:
            self._parse_raw(raw)
        elif parms != None:
            self._parse_parms(parms)
            
    def _parse_raw(self, raw):
    
        # parse packet
        self.raw = raw
        self.parsed = {}
        header_tuple = struct.unpack("<HHHBBBB", raw[0:10])
        for i in range(0, len(header_tuple)-1):
            self.parsed[rt_pkt.hdr_fields[i]] = header_tuple[i]
        self.parsed['payload'] = raw[10:10+header_tuple[len(header_tuple)-1]]
        self.parsed['checksum'] = raw[10+header_tuple[len(header_tuple)-1]:]
        
        # verify checksum
        acc = 0
        for i in range(0, len(raw)):
            acc = (acc + ord(raw[i])) & 0xff
        if acc != 0xff:
            self.checksum_good = False
        
    def _parse_parms(self, parms):
    
        # default format string: no payload
        fmt = ("<HHHBBBB", "<HHHBBBBB")
    
        # create dictionary
        self.parsed = {}
        for i,v in parms.items():
            self.parsed[i] = v
        
        # create field list
        l = [self.parsed[i] for i in rt_pkt.hdr_fields if i != 'checksum' and i != 'payload']
        l.append(len(parms['payload']))
        
        # if there is a payload, add it to the field list and change the format string
        if len(parms['payload']) > 0:
            fmt = ("<HHHBBBBs", "<HHHBBBBsB")
            l.append(parms['payload'])
            
        # compute the checksum
        raw = struct.pack(fmt[0], *l)
        acc = 0
        for i in range(0, len(raw)):
            acc = (acc + ord(raw[i])) & 0xff
        self.parsed['checksum'] = 0xff - (acc & 0xff)
        l.append(self.parsed['checksum'])
        
        # construct the packet
        self.raw = struct.pack(fmt[1], *l)
        
    def __getitem__(self, key):
        return self.parsed[key]
        
    def print_raw(self):
        tmp = ""
        for i in range(0, len(self.raw)):
            tmp += "%02x " % ord(self.raw[i])
        print(tmp)
