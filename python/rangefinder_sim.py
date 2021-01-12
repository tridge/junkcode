#!/usr/bin/env python

import serial

from argparse import ArgumentParser
import serial
import time

parser = ArgumentParser(description=__doc__)
parser.add_argument("--baudrate", default=57600, type=int, help="baud rate")
parser.add_argument("--delay", type=float, default=0.05, help="loop delay")
parser.add_argument("port", default=None, type=str, help="serial port")
args = parser.parse_args()

alt = 0.1

port = serial.Serial(args.port, args.baudrate, timeout=0, dsrdtr=False, rtscts=False, xonxoff=False)
while True:
    port.write("%.1f\r\n" % alt)
    alt += 0.5
    if alt > 100:
        alt = 0.1
    if args.delay > 0:
        time.sleep(args.delay)
