#!/usr/bin/env python3

from dronecan.driver import mcast
from dronecan.driver.common import CANFrame
import time
import math
import struct

mc = mcast("mcast:0")
tstart = time.time()
freq = 0.1
max_dist = 10

while True:
    time.sleep(0.02)
    t = time.time() - tstart
    dist = abs(math.sin(math.pi*2*t*freq)*max_dist)
    print(dist)
    data = struct.pack(">H", int(dist*100))
    frame = CANFrame(1, data, False)
    mc.send_frame(frame)
    # show any other 11 bit frames
    f = mc.receive(timeout=0.001)
    if f is not None and not f.extended:
        print(f)

