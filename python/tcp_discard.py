#!/usr/bin/env python3

import socket

from argparse import ArgumentParser

parser = ArgumentParser('TCP discard service')
parser.add_argument("--port", type=int,
                    help="port number", default=9)
args = parser.parse_args()

listen_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
listen_sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
listen_sock.bind(("0.0.0.0", args.port))
listen_sock.listen(1)

conn_sock = None

while True:
    if conn_sock is None:
        print("Waiting for a TCP connection.")
        conn_sock, addr = listen_sock.accept()
        conn_sock.setblocking(1)  # blocking mode

    buf = conn_sock.recv(1024)
    if len(buf) == 0:
        conn_sock = None
