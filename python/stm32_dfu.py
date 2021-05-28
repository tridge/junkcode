#!/usr/bin/env python3
'''
simplified version of stm32loader
https://github.com/florisla/stm32loader
used to help understand the protocol
'''

from argparse import ArgumentParser
import serial
import struct
import operator
from functools import reduce

parser = ArgumentParser(description=__doc__)
parser.add_argument("--baudrate", default=115200, type=int, help="baud rate")
parser.add_argument("port", default=None, type=str, help="serial port")
parser.add_argument("image", default=None, type=str, help="firmware image")
args = parser.parse_args()

port = serial.Serial(args.port, args.baudrate, timeout=5, dsrdtr=False, rtscts=False, xonxoff=False, parity='E', stopbits=1)

CMD_SYNCHRONIZE = 0x7F
CMD_GET = 0x00
CMD_GET_VERSION = 0x01
CMD_GET_ID = 0x02
CMD_READ_MEMORY = 0x11
CMD_GO = 0x21
CMD_WRITE_MEMORY = 0x31
CMD_ERASE = 0x43

CMD_ACK = 0x79
CMD_NACK = 0x1F

MAX_DATA = 256

def wait_for_ack():
    '''wait for an ACK'''
    read_data = bytearray(port.read(1))
    if not read_data:
        return False
    return read_data[0] == CMD_ACK

def write_and_ack(data):
    '''write data and wait for an ACK'''
    port.write(data)
    return wait_for_ack()

def synchronize():
    for i in range(2):
        port.write(bytearray([CMD_SYNCHRONIZE]))
        d = port.read(1)
        if d and d[0] == CMD_ACK:
            print("Sync OK")
            return True
    return False

def run_command(cmd):
    '''run a command, return true if acked'''
    pkt = bytearray([cmd, cmd^0xff])
    port.write(bytearray([cmd]))
    port.write(bytearray([cmd^0xff]))
    return wait_for_ack()

def get_version():
    '''get firmware version'''
    run_command(CMD_GET)
    d = port.read(2)
    if not d or len(d) != 2:
        print("Bad version")
        return
    length = bytearray(d)[0]
    version = bytearray(d)[1]
    print("Bootloader version: %d" % int(version))
    print("Length: ", length)
    data = bytearray(port.read(length))
    print("Available commands: " + ", ".join(hex(b) for b in data))
    wait_for_ack()

def encode_address(address):
    """Return the given address as big-endian bytes with a checksum."""
    address_bytes = bytearray(struct.pack(">I", address))
    checksum_byte = struct.pack("B", reduce(operator.xor, address_bytes))
    return address_bytes + checksum_byte
    
def read_memory(address, length):
    '''read and return data from a given address'''
    ret = bytearray()
    while length > 0:
        n = min(length, MAX_DATA)
        if not run_command(CMD_READ_MEMORY):
            return False
        if not write_and_ack(encode_address(address)):
            return False
        nr_of_bytes = (length - 1) & 0xFF
        checksum = nr_of_bytes ^ 0xFF
        if not write_and_ack(bytearray([nr_of_bytes, checksum])):
            return False
        ret += bytearray(port.read(n))
        address += n
        length -= n
    return ret

def write_memory(address, data):
    '''write data to given address'''
    while len(data) > 0:
        n = min(len(data), MAX_DATA)
        if not run_command(CMD_WRITE_MEMORY):
            return False
        if not write_and_ack(encode_address(address)):
            return False
        b = data[:n]
        if len(b) % 4 != 0:
            pad = 4 - (len(b) % 4)
            b += bytearray([0xff]*pad)

        crc = bytearray([reduce(operator.xor, b, len(b))])
        print(crc)
        b.extend(crc)
        if not write_and_ack(b):
            return False
        address += n
        data = data[n:]
    return True


synchronize()
get_version()
d = read_memory(0x08000000, 1024)
print(len(d))
write_memory(0x08000000, d)

