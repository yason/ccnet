#coding: UTF-8

"""
Packet level protocol of ccnet.

About various types of id:

  - A slave processor's id has its highest bit set; a master processor has its highest bit clear
  - The <id> field of a ccnet packet always has its highest bit clear. The
    <type> field of the packet determines what type of the packet is (a
    request, a response, or an update)
"""

import struct

REQUEST_ID_MASK = 0x7fffffff
SLAVE_BIT_MASK = 0x80000000

CCNET_MSG_OK        = 0
CCNET_MSG_HANDSHAKE = 1
CCNET_MSG_REQUEST   = 2
CCNET_MSG_RESPONSE  = 3
CCNET_MSG_UPDATE    = 4
CCNET_MSG_RELAY     = 5

def to_request_id(pid):
    return pid & REQUEST_ID_MASK

to_response_id = to_request_id
to_update_id = to_request_id
to_master_id = to_request_id

def to_slave_id(pid):
    return pid | SLAVE_BIT_MASK

def to_print_id(pid):
    if pid & SLAVE_BIT_MASK:
        return -to_request_id(pid)
    else:
        return pid

# the byte sequence of ccnet packet header
CCNET_HEADER_FORMAT = '>BBHI'
# Number of bytes for the header 
CCNET_HEADER_LENGTH = struct.calcsize(CCNET_HEADER_FORMAT)

class PacketHeader(object):
    def __init__(self, ver, ptype, length, pid):
        self.ver = ver
        self.ptype = ptype
        self.length = length
        self.pid = pid

    def to_string(self):
        return struct.pack(CCNET_HEADER_FORMAT, self.ver, self.ptype, self.length, self.pid)

    def __str__(self):
        return "<PacketHeader: type = %d, length = %d, id = %u>" % (self.ptype, self.length, self.pid)

class Packet(object):
    version = 1
    def __init__(self, header, body):
        self.header = header
        self.body = body

def parse_header(buf):
    ver, ptype, length, pid = struct.unpack(CCNET_HEADER_FORMAT, buf)
    return PacketHeader(ver, ptype, length, pid)

def format_response(code, code_msg, content):
    body = code
    if code_msg:
        body += " " + code_msg
    body += "\n"

    if content:
        body += content

    return body

format_update = format_response

def request_to_packet(pid, buf):
    hdr = PacketHeader(1, CCNET_MSG_REQUEST, len(buf), to_request_id(pid))
    return Packet(hdr, buf)

def response_to_packet(pid, code, code_msg, content):
    body = format_response(code, code_msg, content)
    hdr = PacketHeader(1, CCNET_MSG_RESPONSE, len(body), to_response_id(pid))
    return Packet(hdr, body)

def update_to_packet(pid, code, code_msg, content):
    body = format_update(code, code_msg, content)
    hdr = PacketHeader(1, CCNET_MSG_UPDATE, len(body), to_update_id(pid))
    return Packet(hdr, body)