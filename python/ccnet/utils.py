import socket

from ccnet.errors import NetworkError

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