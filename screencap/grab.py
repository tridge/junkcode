#!/usr/bin/env python
'''
script to capture a region of a screen and send as jpg images over UDP
to another host. This provides very low latency screen forwarding
'''
import d3dshot
import time
import cv2
import socket
import argparse
import sys

ap = argparse.ArgumentParser()
ap.add_argument("--host", type=str, default=None, required=True)
ap.add_argument("--port", type=int, default=19721)
ap.add_argument("--region", type=str, default="0,0,300,300")
ap.add_argument("--rate", type=int, default=50)
ap.add_argument("--quality", type=int, default=80)

args = ap.parse_args()

usock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
usock.connect((args.host, args.port))

region = args.region.split(',')
if len(region) != 4:
    print("Region must be 'x,y,width,height'")
    sys.exit(1)
region = (int(region[0]),int(region[1]),int(region[2]),int(region[3]))

d = d3dshot.create(capture_output="numpy")

last_print_s = time.time()
count = 0
target_dt = 1.0 / args.rate
total_size = 0

while True:
    t1 = time.time()
    img = d.screenshot(region=region)
    encode_param = [int(cv2.IMWRITE_JPEG_QUALITY), args.quality]
    result, encimg = cv2.imencode('.jpg', img, encode_param)
    total_size += len(encimg)
    usock.send(encimg)
    t2 = time.time()
    dt = t2 - t1
    if dt < target_dt:
        time.sleep(target_dt - dt)
    count += 1
    now = time.time()
    if now - last_print_s >= 1.0:
        dt = now - last_print_s
        print("%.1f FPS %.1f kByte/sec" % (count/dt, (total_size/1024.0)/dt))
        last_print_s = now
        count = 0
        total_size = 0
        
        

