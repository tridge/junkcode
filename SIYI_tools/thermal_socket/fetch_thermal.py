#!/usr/bin/env python3

# save files with timestamp filename
# download SRTM1 to laptop

import sys
import numpy as np
import cv2
import time
import glob
import os
import socket
import struct
import threading
import datetime
import math
import zlib

from MAVProxy.modules.lib.mp_image import MPImage

TCP_THERMAL=("192.168.144.25", 7345)
#TCP_THERMAL=("127.0.0.1", 7345)

last_data = None
WIN_NAME = "Thermal"
C_TO_KELVIN = 273.15
mouse_temp = -1
tmin = -1
tmax = -1

EXPECTED_DATA_SIZE = 640 * 512 * 2

def update_title():
    global tmin, tmax, mouse_temp
    cv2.setWindowTitle(WIN_NAME, "Thermal: (%.1fC to %.1fC) %.1fC" % (tmin, tmax, mouse_temp))

def click_callback(event, x, y, flags, param):
    global last_data, mouse_temp
    if last_data is None:
        return
    p = last_data[y*640+x]
    mouse_temp = p - C_TO_KELVIN
    update_title()

def display_file(fname, data):
    global last_data, tmin, tmax
    a = np.frombuffer(data, dtype='>u2')
    if len(a) != 640 * 512:
        print("Bad size %u" % len(a))
        return
    # get in Kelvin
    a = (a / 64.0)

    maxv = a.max()
    minv = a.min()

    tmin = minv - C_TO_KELVIN
    tmax = maxv - C_TO_KELVIN
    print("Max=%.3fC Min=%.3fC" % (tmax, tmin))
    if maxv <= minv:
        print("Bad range")
        return

    last_data = a

    # convert to 0 to 255
    a = (a - minv) * 65535.0 / (maxv - minv)

    # convert to uint8 greyscale as 640x512 image
    a = a.astype(np.uint16)
    a = a.reshape(512, 640)

    cv2.imshow(WIN_NAME, a)
    cv2.setMouseCallback(WIN_NAME, click_callback)
    update_title()

    import zlib

def decompress_zlib_buffer(compressed_buffer):
    try:
        # Decompress the buffer using zlib
        uncompressed_buffer = zlib.decompress(compressed_buffer)
        return uncompressed_buffer
    except zlib.error as e:
        # Handle any errors during decompression
        print(f"Decompression error: {e}")
        return None

def fetch_latest():
    tcp = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    tcp.connect(TCP_THERMAL)
    buf = bytearray()

    while True:
        b = tcp.recv(1024)
        if not b:
            break
        buf += b

    print(len(buf))
    header_len = 128 + 12
    if len(buf) < header_len:
        print("Invalid data size %u" % len(buf))
        return None

    print("Got %u bytes" % len(buf))

    fname = buf[:128].decode("utf-8").strip('\x00')
    compressed_size,tstamp = struct.unpack("<Id", buf[128:128+12])

    compressed_data = buf[header_len:]

    if compressed_size != len(compressed_data):
        print("Invalid compressed_size %u expected %u" % (compressed_size, len(compressed_data)))
        return None

    uncompressed_data = decompress_zlib_buffer(compressed_data)

    print("uncompressed_size=%u" % len(uncompressed_data))
    if len(uncompressed_data) != EXPECTED_DATA_SIZE:
        print("Bad uncompressed length %u" % len(uncompressed_data))

    print(fname, tstamp)
    return fname, tstamp, uncompressed_data

def save_file(fname, tstamp, data):
    fname = os.path.basename(fname)[:-4]
    print(fname, tstamp, len(data))
    tstr = datetime.datetime.fromtimestamp(tstamp).strftime("%Y_%m_%d_%H_%M_%S")
    subsec = tstamp - math.floor(tstamp)
    millis = int(subsec * 1000)
    fname = "%s_%s_%03u.bin" % (fname, tstr, millis)
    f = open(fname, 'wb')
    f.write(data)
    f.close()
    os.utime(fname, (tstamp, tstamp))

def fetch_images():
    last_tstamp = None
    while True:
        try:
            fname, tstamp, data = fetch_latest()
        except Exception as ex:
            print(ex)
            time.sleep(0.1)
            continue
        if data is not None and tstamp != last_tstamp:
            last_tstamp = tstamp
            display_file(fname, data)
            save_file(fname, tstamp, data)

t = threading.Thread(target=fetch_images)
t.start()

while True:
    cv2.waitKey(1)
