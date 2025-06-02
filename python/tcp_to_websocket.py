#!/usr/bin/env python3
'''
bridge an incoming TCP connection to a WebSocket server.
'''

import socket
import selectors
import sys
from wsproto import WSConnection
from wsproto.connection import ConnectionType
from wsproto.events import BytesMessage, CloseConnection, AcceptConnection
from wsproto.handshake import Request

class WebSocketBridge:
    def __init__(self, host, port, resource='/'):
        print(f"Connecting to WebSocket server at {host}:{port}{resource}")
        self.sock = socket.create_connection((host, port))
        self.ws = WSConnection(ConnectionType.CLIENT)
        b = self.ws.send(Request(host=host, target=resource))
        self.sock.send(b)
        self.buffer = b''

        # wait for handshake response
        while True:
            data = self.sock.recv(4096)
            if not data:
                raise RuntimeError("WebSocket handshake failed")
            self.ws.receive_data(data)
            for event in self.ws.events():
                if isinstance(event, AcceptConnection):
                    return

    def recv(self):
        if self.buffer:
            out, self.buffer = self.buffer, b''
            return out
        data = self.sock.recv(4096)
        if not data:
            return b''
        self.ws.receive_data(data)
        for event in self.ws.events():
            if isinstance(event, BytesMessage):
                self.buffer += event.data
            elif isinstance(event, CloseConnection):
                return b''
        out, self.buffer = self.buffer, b''
        return out

    def send(self, data):
        b = self.ws.send(BytesMessage(data=data))
        self.sock.send(b)

    def close(self):
        self.sock.close()

def main(listen_port, ws_host, ws_port, ws_path='/'):
    server = socket.socket()
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server.bind(('0.0.0.0', listen_port))
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server.listen(1)
    print(f"Listening on TCP port {listen_port}...")

    conn, addr = server.accept()
    print(f"TCP connection from {addr}")

    ws = None
    try:
        ws = WebSocketBridge(ws_host, ws_port, ws_path)
        sel = selectors.DefaultSelector()
        sel.register(conn, selectors.EVENT_READ)
        sel.register(ws.sock, selectors.EVENT_READ)

        while True:
            events = sel.select()
            for key, _ in events:
                if key.fileobj == conn:
                    data = conn.recv(4096)
                    if not data:
                        raise EOFError("TCP closed")
                    ws.send(data)
                elif key.fileobj == ws.sock:
                    data = ws.recv()
                    if not data:
                        continue
                    conn.sendall(data)
    finally:
        conn.close()
        if ws:
            ws.close()
        server.close()

if __name__ == "__main__":
    if len(sys.argv) < 4:
        print(f"Usage: {sys.argv[0]} <listen_port> <ws_host> <ws_port> [ws_path]")
        sys.exit(1)
    listen_port = int(sys.argv[1])
    ws_host = sys.argv[2]
    ws_port = int(sys.argv[3])
    ws_path = sys.argv[4] if len(sys.argv) > 4 else '/'
    main(listen_port, ws_host, ws_port, ws_path)
