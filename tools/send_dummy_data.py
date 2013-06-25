#!/usr/bin/python

import socket
import sys
import time

HOST = 'localhost'    # The remote host
PORT = 9999          # The same port as used by the server
s = None
for res in socket.getaddrinfo(HOST, PORT, socket.AF_UNSPEC, socket.SOCK_STREAM):
    af, socktype, proto, canonname, sa = res
    try:
        s = socket.socket(af, socktype, proto)
    except socket.error, msg:
        s = None
        continue
    try:
        s.connect(sa)
    except socket.error, msg:
        s.close()
        s = None
        continue
    break
if s is None:
    print 'could not open socket'
    sys.exit(1)

filename = sys.argv[1]
if filename == '-':
    for l in sys.stdin:
        s.send(l)
else:
    try:
        with open(filename) as f:
            for l in f:
                s.send(l)
    finally:
        s.close()

