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
import socket
from MAVProxy.modules.lib import mp_image

ap = argparse.ArgumentParser()
ap.add_argument("--dir", type=str, default=None, required=True)
ap.add_argument("--delay", type=float, default=0.02)
ap.add_argument("--scale", type=float, default=2.0)
ap.add_argument("--avi", type=str, default=None)
ap.add_argument("--debug", type=str, default=None)
ap.add_argument("--udp", type=int, default=-1)
ap.add_argument("--saveimgs", action='store_true')
args = ap.parse_args()

viewer = mp_image.MPImage(title='OpenCV', width=200, height=200, auto_size=True)
masked_view = None
thresh_view = None

if args.debug:
    masked_view = mp_image.MPImage(title='Masked', width=200, height=200, auto_size=True)
    thresh_view = mp_image.MPImage(title='Thresh', width=200, height=200, auto_size=True)
avi_out = None

def mask_dark_gray(img):
    '''return a mask for dark gray in image only'''

    hsv = cv2.cvtColor(img, cv2.COLOR_BGR2HSV)
    lower = numpy.array([10, 0, 0])
    upper = numpy.array([250,140,60])

    mask = cv2.inRange(hsv, lower, upper)
    return mask

left_count = 0
right_count = 0

def match_gate(img, cnt):
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
    width1 = vertices[1][0] - vertices[0][0]
    width2 = vertices[1][0] - vertices[2][0]
    if height < 1 or abs(width1) < 1 or abs(width2) < 1:
        return False
    ratio1 = height / float(abs(width1))
    ratio2 = height / float(abs(width2))
    if abs(ratio1 - 3.0) > 1.0 or abs(ratio2 - 3.0) > 1.0:
        return False

    if not args.saveimgs:
        return True

    h,w,_ = img.shape
    border = 10
    global left_count, right_count

    if width1 < 0:
        left_count += 1
        x1 = max(vertices[1][0]-border,0)
        y1 = max(vertices[0][1]-border,0)
        x2 = min(vertices[0][0]+border,w-1)
        y2 = min(vertices[2][1]+border,h-1)
        arrow = img[y1:y2,x1:x2,:]
        cv2.imwrite('train/left%04u.jpg' % left_count, arrow)
    else:
        right_count += 1
        x1 = max(vertices[0][0]-border,0)
        y1 = max(vertices[0][1]-border,0)
        x2 = min(vertices[1][0]+border,w-1)
        y2 = min(vertices[2][1]+border,h-1)
        arrow = img[y1:y2,x1:x2,:]
        cv2.imwrite('train/right%04u.jpg' % right_count, arrow)
    return True

def process_image_file(img):
    mask = mask_dark_gray(img)

    gray = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)

    gray[mask != 255] = (255)
    gray[mask == 255] = (0)

    #kernel = numpy.ones((4, 4), numpy.uint8)
    #dilation = cv2.dilate(gray, kernel, iterations=1)
    #blur = cv2.GaussianBlur(dilation, (1, 1), 0)

    img_masked = img.copy()
    img_masked[mask != 255] = (255,255,255)
    img_masked = cv2.resize(img_masked, (0,0), fx=args.scale, fy=args.scale)
    if masked_view is not None:
        masked_view.set_image(img_masked)

    _, contours, _ = cv2.findContours(gray, cv2.RETR_LIST, cv2.CHAIN_APPROX_SIMPLE)

    gray = cv2.resize(gray, (0,0), fx=args.scale, fy=args.scale)
    if thresh_view is not None:
        thresh_view.set_image(cv2.cvtColor(gray, cv2.COLOR_GRAY2BGR))
    
    coordinates = []
    for cnt in contours:
        approx = cv2.approxPolyDP(cnt, 0.12 * cv2.arcLength(cnt, True), True)
        if not match_gate(img, approx):
            #cv2.drawContours(img, [approx], 0, (255, 0, 0), 2)
            pass
        else:
            cv2.drawContours(img, [approx], 0, (0, 0, 255), 2)
    img = cv2.resize(img, (0,0), fx=args.scale, fy=args.scale)
    viewer.set_image(img,bgr=True)

    global avi_out
    if args.avi is not None and avi_out is None:
        frame_height,frame_width,_ = img.shape
        avi_out = cv2.VideoWriter(args.avi,cv2.VideoWriter_fourcc('M','J','P','G'), 50, (frame_width,frame_height))
    if avi_out is not None:
        avi_out.write(img)

if args.udp > 0:
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.bind(('', args.udp))
    while True:
        p = sock.recv(80000)
        img = cv2.imdecode(numpy.fromstring(p, dtype=numpy.uint8), -1)
        img = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)
        process_image_file(img)
elif os.path.isfile(args.dir):
    print("Processing one file %s" % args.dir)
    img = cv2.imread(args.dir)
    process_image_file(img)
else:
    print("Processing directory %s" % args.dir)
    flist = sorted(glob.glob(os.path.join(args.dir, '*.jpg')))
    for f in flist:
        print(f)
        img = cv2.imread(f)
        process_image_file(img)
        time.sleep(args.delay)

if avi_out is not None:
    avi_out.release()
