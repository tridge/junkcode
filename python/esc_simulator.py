#!/usr/bin/env python

import serial

from argparse import ArgumentParser
import serial
import time
import struct

parser = ArgumentParser(description=__doc__)
parser.add_argument("--baudrate", default=57600, type=int, help="baud rate")
parser.add_argument("--rate", type=float, default=100, help="output rate")
parser.add_argument("port", default=None, type=str, help="serial port")
args = parser.parse_args()

pkt_counter = 0

def calc_crc(buf):
    crc = 0
    for b in buf:
        crc = (crc + b) % 0xFFFF
    return crc

def write_esc_data(port, rpm, current, voltage, temperature):
    '''write fake ESC data to the port'''
    global pkt_counter
    pkt_counter += 1
    s = struct.pack("<BBIHHHHhhBBH",
                    0x9B, # header
                    0x16, # pkt_len
                    pkt_counter,
                    30, # throttle_req;
                    30, # throttle;
                    int(rpm),
                    int(voltage*10),
                    int(current*100),
                    int(current*100), # phase_current;
                    int(temperature), # mos_temperature;
                    int(temperature), # cap_temperature;
                    0, # status;
                    )
    crc = calc_crc(s)
    s += struct.pack("<H", crc)
    port.write(s)

port = serial.Serial(args.port, args.baudrate, timeout=0, dsrdtr=False, rtscts=False, xonxoff=False)
while True:
    write_esc_data(port, 3200, 15, 12.3, 37.2)
    time.sleep(1.0/args.rate)
