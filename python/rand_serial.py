#!/usr/bin/env python

import serial

from argparse import ArgumentParser
import serial
import random
import time

parser = ArgumentParser(description=__doc__)
parser.add_argument("--baudrate", default=57600, type=int, help="baud rate")
parser.add_argument("--delay", type=float, default=0.05, help="loop delay")
parser.add_argument("port", default=None, type=str, help="serial port")
args = parser.parse_args()

port = serial.Serial(args.port, args.baudrate, timeout=0, dsrdtr=False, rtscts=False, xonxoff=False)
while True:
    len = int(random.uniform(1, 300))
    for i in range(len):
        b = chr(int(random.uniform(0, 255)))
        port.write(b)
    if args.delay > 0:
        time.sleep(args.delay)
