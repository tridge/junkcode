#!/usr/bin/env python

"""
send to tcp from a file
"""

from argparse import ArgumentParser
import socket, sys, time, errno


parser = ArgumentParser(description=__doc__)
parser.add_argument("--dest", type=str, default="127.0.0.1:5763", help="destination IP:port")
parser.add_argument("--rate", type=float, default=5, help="lines per second")
parser.add_argument("filename", type=str, default=None, help="source file")
args = parser.parse_args()

if not args.filename:
    print("Must supply filename")
    sys.exit(1)

def open_socket(dest):
    a = dest.split(':')
    ip = a[0]
    port = int(a[1])
    tcp = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    tcp.connect((ip, port))
    return tcp

tcpsock = open_socket(args.dest)
lines = open(args.filename, 'r').readlines()
linenum = 0

while True:
    line = lines[linenum]
    linenum = (linenum + 1) % len(lines)

    print(line)
    tcpsock.send(line)
    time.sleep(1.0 / args.rate)
