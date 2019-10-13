#!/usr/bin/env python
'''
attempt to recognise drone racing gates from RealFlight9 'FPV Racing Site' map
'''

import cv2
import numpy
import argparse
import os
import glob
import time
from MAVProxy.modules.lib import mp_image

ap = argparse.ArgumentParser()
ap.add_argument("--dir", type=str, default=None, required=True)
ap.add_argument("--delay", type=float, default=0.02)
ap.add_argument("--scale", type=float, default=2.0)
args = ap.parse_args()

viewer = mp_image.MPImage(title='OpenCV', width=200, height=200, auto_size=True)

def match_gate(cnt):
    '''return true if this contour could be a gate'''
    if len(cnt) != 3:
        return False
    area = cv2.contourArea(cnt)
    if area < 20:
        return False
    # get vertices
    vertices = [[cnt[0][0][0], cnt[0][0][1]],
                [cnt[1][0][0], cnt[1][0][1]],
                [cnt[2][0][0], cnt[2][0][1]]]
    # sort by Y coordinate
    vertices.sort(key=lambda x: x[1])
    height = vertices[2][1] - vertices[0][1]
    width1 = abs(vertices[1][0] - vertices[0][0])
    width2 = abs(vertices[1][0] - vertices[2][0])
    if height < 1 or width1 < 1 or width2 < 1:
        return False
    ratio1 = height / float(width1)
    ratio2 = height / float(width2)
    if abs(ratio1 - 3.0) > 1.0 or abs(ratio2 - 3.0) > 1.0:
        return False
    return True

def process_image_file(filename):
    img = cv2.imread(filename)
    gray = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)
    kernel = numpy.ones((4, 4), numpy.uint8)
    dilation = cv2.dilate(gray, kernel, iterations=1)
    blur = cv2.GaussianBlur(dilation, (11, 11), 0)

    thresh = cv2.adaptiveThreshold(blur, 255, 1, 1, 11, 2)
    _, contours, _ = cv2.findContours(thresh, cv2.RETR_LIST, cv2.CHAIN_APPROX_SIMPLE)

    coordinates = []
    for cnt in contours:
        approx = cv2.approxPolyDP(cnt, 0.07 * cv2.arcLength(cnt, True), True)
#        if not match_gate(approx):
#            continue
        cv2.drawContours(img, [approx], 0, (0, 0, 255), 2)
    if args.scale != 1.0:
        img = cv2.resize(img, (0,0), fx=args.scale, fy=args.scale)
    viewer.set_image(img,bgr=True)

if os.path.isfile(args.dir):
    print("Processing one file %s" % args.dir)
    process_image_file(args.dir)
else:
    print("Processing directory %s" % args.dir)
    flist = sorted(glob.glob(os.path.join(args.dir, '*.jpg')))
    for f in flist:
        print(f)
        process_image_file(f)
        time.sleep(args.delay)
