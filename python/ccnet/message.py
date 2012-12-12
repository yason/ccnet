#coding: UTF-8

'''Message is the carrier of a simple Pub/Sub system on top of ccnet'''

import datetime
import re

MESSAGE_PATTERN = re.compile(r'(?P<flags>[\d]+) (?P<from>[^ ]+) (?P<to>[^ ]+) (?P<id>[^ ]+) (?P<ctime>[^ ]+) (?P<rtime>[^ ]+) (?P<app>[^ ]+) (?P<body>.*)')

class Message(object):
    def __init__(self, d):
        self.flags = int(d['flags'])
        self.from_ = d['from']
        self.to = d['to']
        self.id = d['id']
        self.ctime = datetime.datetime.fromtimestamp(float(d['ctime']))
        self.rtime = datetime.datetime.fromtimestamp(float(d['rtime']))
        self.app = d['app']
        self.body = d['body']

def message_from_string(s):
    results = MESSAGE_PATTERN.match(s)
    if results is None:
        raise RuntimeError('Bad message: %s' % s)

    d = results.groupdict()
    return Message(d)