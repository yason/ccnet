#coding: UTF-8

import os
import socket
import ConfigParser
import logging

from ccnet.packet import Packet, parse_header
from ccnet.packet import to_request_id, to_update_id
from ccnet.packet import request_to_packet, update_to_packet
from ccnet.packet import CCNET_HEADER_LENGTH, CCNET_MSG_RESPONSE

from ccnet.status_code import SC_PROC_DONE, SS_PROC_DONE

from ccnet.message import message_from_string, gen_inner_message_string

class NetworkError(Exception):
    def __init__(self, msg):
        Exception.__init__(self)
        self.msg = msg

    def __str__(self):
        return self.msg

class Response(object):
    '''The parsed content of a ccnet response packet'''
    def __init__(self, code, code_msg, content):
        self.code = code
        self.code_msg = code_msg
        self.content = content

    def __str__(self):
        return "<Response> %s %s %s" % (self.code, self.code_msg, self.content[:10])

class Update(object):
    '''The parsed content of a ccnet update packet'''
    def __init__(self, code, code_msg, content):
        self.code = code
        self.code_msg = code_msg
        self.content = content

    def __str__(self):
        return "<Update> %s %s %s" % (self.code, self.code_msg, self.content[:10])

def parse_response(body):
    '''Parse the content of the response
    The struct of response data:
    - first 3 bytes is the <status code>
    - from the 4th byte to the first occurrence of '\n' is the <status message>. If the 4th byte is '\n', then there is no <status message>
    - from the first occurrence of '\n' to the end is the <content>
    '''
    code = body[:3]
    if body[3] == '\n':
        code_msg = ''
        content = body[4:]
    else:
        pos = body.index('\n')
        code_msg = body[4:pos]
        content = body[pos + 1:]

    return Response(code, code_msg, content)

def parse_update(body):
    '''The structure of an update is the same with a response'''
    code = body[:3]
    if body[3] == '\n':
        code_msg = ''
        content = body[4:]
    else:
        pos = body.index('\n')
        code_msg = body[4:pos]
        content = body[pos + 1:]

    return Update(code, code_msg, content)

def recvall(fd, total):
    remain = total
    data = ''
    while remain > 0:
        try:
            new = fd.recv(remain)
        except socket.error as e:
            raise NetworkError('Failed to read from socket: %s' % e)

        n = len(new)
        if n <= 0:
            raise NetworkError("Failed to read from socket")
        else:
            data += new
            remain -= n

    return data

def sendall(fd, data):
    total = len(data)
    offset = 0
    while offset < total:
        try:
            n = fd.send(data[offset:])
        except socket.error as e:
            raise NetworkError('Failed to write to socket: %s' % e)

        if n <= 0:
            raise NetworkError('Failed to write to socket')
        else:
            offset += n

class Client(object):
    '''Ccnet client class. It's the python implmentation of the **sync mode**
    client of the C client

    '''
    def __init__(self, config_dir):
        if not isinstance(config_dir, unicode):
            config_dir = config_dir.decode('UTF-8')

        config_dir = os.path.expanduser(config_dir)
        config_file = os.path.join(config_dir, u'ccnet.conf')
        if not os.path.exists(config_file):
            raise RuntimeError(u'%s does not exits' % config_file)

        self.config_dir = config_dir
        self.config_file = config_file
        self.config = None

        self.port = None
        self.peerid = None
        self.peername = None

        self.parse_config()

        self._connfd = None
        self._req_id = 1000
        self.mq_req_id = -1

    def __del__(self):
        '''Destructor of the client class. We close the socket here, if
        connetced to daemon

        '''
        if self.is_connected():
            self._connfd.close()

    def parse_config(self):
        self.config = ConfigParser.ConfigParser()
        self.config.read(self.config_file)
        self.port = self.config.getint('Client', 'PORT')
        self.peerid = self.config.get('General', 'ID')
        self.peername = self.config.get('General', 'NAME')

    def connect_daemon(self):
        self._connfd = socket.socket()
        self._connfd.setsockopt(socket.SOL_TCP, socket.TCP_NODELAY, 1)
        try:
            self._connfd.connect(('127.0.0.1', self.port))
        except:
            raise NetworkError("Can't connect to daemon")

    def is_connected(self):
        return self._connfd != None

    def write_packet(self, packet):
        hdr = packet.header.to_string()
        sendall(self._connfd, hdr)
        sendall(self._connfd, packet.body)

    def read_packet(self):
        hdr = recvall(self._connfd, CCNET_HEADER_LENGTH)
        if len(hdr) == 0:
            logging.warning('connection to daemon is lost')
            raise NetworkError('Connection to daemon is lost')
        elif len(hdr) < CCNET_HEADER_LENGTH:
            raise NetworkError('Only read %d bytes header, expected 8' % len(hdr))

        header = parse_header(hdr)

        if header.length == 0:
            body = ''
        else:
            body = recvall(self._connfd, header.length)
            if len(body) < header.length:
                raise NetworkError('Only read %d bytes body, expected %d' % (len(body), header.length))

        return Packet(header, body)

    def get_request_id(self):
        self._req_id += 1
        return self._req_id

    def send_request(self, id, req):
        id = to_request_id(id)
        pkt = request_to_packet(id, req)
        self.write_packet(pkt)

    def send_update(self, id, code, code_msg, content):
        id = to_update_id(id)
        pkt = update_to_packet(id, code, code_msg, content)
        self.write_packet(pkt)

    def read_response(self):
        packet = self.read_packet()
        if packet.header.ptype != CCNET_MSG_RESPONSE:
            raise RuntimeError('Invalid Response')

        return parse_response(packet.body)

    def send_cmd(self, cmd):
        req_id = self.get_request_id()
        self.send_request(req_id, 'receive-cmd')
        resp = self.read_response()
        if resp.code != '200':
            raise RuntimeError('Failed to send-cmd: %s %s' % (resp.code, resp.code_msg))

        cmd += '\000'
        self.send_update(req_id, '200', '', cmd)

        resp = self.read_response()
        if resp.code != '200':
            raise RuntimeError('Failed to send-cmd: %s %s' % (resp.code, resp.code_msg))

        self.send_update(req_id, SC_PROC_DONE, SS_PROC_DONE, '')

    def prepare_send_message(self):
        request = 'mq-server'
        mq_req_id = self.get_request_id()
        self.send_request(mq_req_id, request)
        resp = self.read_response()
        if resp.code != '200':
            raise RuntimeError('bad response: %s %s' % (resp.code, resp.code_msg))
        self.mq_req_id = mq_req_id
    
    def send_message(self, msg_type, content):
        if self.mq_req_id == -1:
            self.prepare_send_message()

        msg = gen_inner_message_string(self.peerid, msg_type, content)
        self.send_update(self.mq_req_id, "300", '', msg)
        resp = self.read_response()
        if resp.code != '200':
            self.mq_req_id = -1
            raise RuntimeError('bad response: %s %s' % (resp.code, resp.code_msg))

    def prepare_recv_message(self, msg_type):
        request = 'mq-server %s' % msg_type
        req_id = self.get_request_id()
        self.send_request(req_id, request)

        resp = self.read_response()
        if resp.code != '200':
            raise RuntimeError('bad response: %s %s' % (resp.code, resp.code_msg))

    def receive_message(self):
        resp = self.read_response()
        # the message from ccnet daemon has the trailing null byte included
        msg = message_from_string(resp.content[:-1])
        return msg
