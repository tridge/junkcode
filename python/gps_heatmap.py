#!/usr/bin/env python

'''
show heatmap of high/low quality GPS data for a dual GPS setup
'''

import sys, time, os

from math import *

from pymavlink import mavutil, mavextra
from MAVProxy.modules.mavproxy_map import mp_slipmap

from optparse import OptionParser
parser = OptionParser("gps_heatmap.py [options]")
parser.add_option("--service", default="MicrosoftSat", help="tile service")
parser.add_option("--threshold-min", type=float, default=3.0, help="pos threshold min")
parser.add_option("--threshold-max", type=float, default=10.0, help="pos threshold max")
parser.add_option("--radius", type=float, default=5.0, help="spot radius")
parser.add_option("--linewidth", type=int, default=3, help="line width")

(opts, args) = parser.parse_args()

if len(args) < 1:
    print("Usage: gps_heatmap.py [options] <LOGFILE...>")
    sys.exit(1)

points = []
min_pos = [None,None]
max_pos = [None,None]

histogram = {}

def add_file(filename):
    '''add data from one file to map'''
    mlog = mavutil.mavlink_connection(filename)
    recv_match_types = ['GPS','GPS2','GPA','GPA2','CTUN']
    GPS = None
    GPS2 = None
    CTUN = None

    max_err = 0.0

    while True:
        try:
            m = mlog.recv_match(type=recv_match_types)
            if m is None:
                break
        except Exception:
            break

        type = m.get_type()
        if type == 'GPS':
            GPS = m
        if type == 'GPS2':
            GPS2 = m
        if type == 'CTUN':
            CTUN = m

        if GPS is None or GPS2 is None or CTUN is None or (GPS.Status<3 and GPS2.Status<3) or CTUN.ThO<0.05:
            continue

        if type == 'GPS2':
            if min_pos[0] is None or GPS2.Lat > min_pos[0]:
                min_pos[0] = GPS2.Lat
            if min_pos[1] is None or GPS2.Lng < min_pos[1]:
                min_pos[1] = GPS2.Lng
            if max_pos[0] is None or GPS2.Lat < min_pos[0]:
                max_pos[0] = GPS2.Lat
            if max_pos[1] is None or GPS2.Lng > max_pos[1]:
                max_pos[1] = GPS2.Lng
            if abs((GPS.TimeUS - GPS2.TimeUS)*1.0e-6) > 0.25:
                continue
            dist = mavextra.distance_two(GPS,GPS2,True)
            if dist > max_err:
                max_err = dist
            if dist < opts.threshold_min:
                dist = opts.threshold_min
            err = dist / opts.threshold_max
            points.append((err,GPS2.Lat, GPS2.Lng))
    hbin = int(max_err+0.5)
    if not hbin in histogram:
        histogram[hbin] = 1
    else:
        histogram[hbin] += 1
    print(filename, hbin)

for i in range(len(args)):
    filename = args[i]
    print("Processing %s %u/%u" % (filename, i, len(args)))
    add_file(filename)


def distance_two_latlon(pos1,pos2):
    '''distance between two points'''
    lat1 = radians(pos1[0])
    lat2 = radians(pos2[0])
    lon1 = radians(pos1[1])
    lon2 = radians(pos2[1])
    dLat = lat2 - lat1
    dLon = lon2 - lon1

    a = sin(0.5*dLat)**2 + sin(0.5*dLon)**2 * cos(lat1) * cos(lat2)
    c = 2.0 * atan2(sqrt(a), sqrt(1.0-a))
    ground_dist = 6371 * 1000 * c
    return ground_dist

def get_color(range):
    if range > 1:
        range = 1.0
    elif range < 0:
        range = 0.0
    return (255*range, 255*(1-range), 0)

# sort by error
points = sorted(points, key=lambda x: x[0])

width = int(1.2*distance_two_latlon(min_pos, [min_pos[0],max_pos[1]]))

map = mp_slipmap.MPSlipMap(title="GPS Heatmap",
                           service=opts.service,
                           elevation=True,
                           width=600,
                           height=600,
                           ground_width=width,
                           lat=min_pos[0], lon=min_pos[1],
                           debug=False)


for (err,lat,lon) in points:
    map.add_object(mp_slipmap.SlipCircle(str(err), 3,
                                         (lat, lon),
                                         opts.radius, get_color(err), linewidth=opts.linewidth))

print(histogram)
keys = sorted(histogram.keys())
maxkey = keys[-1]
for k in range(maxkey+1):
    if not k in histogram:
        print("%2u, 0" % k)
    else:
        print("%2u, %u" % (k, histogram[k]))
