#!/usr/bin/env python

import time

from pymavlink import mavutil

from argparse import ArgumentParser
parser = ArgumentParser(description=__doc__)

parser.add_argument("--baudrate", type=int,
                    help="port baud rate", default=115200)
parser.add_argument("--device", required=True, help="serial device")
args = parser.parse_args()

master = mavutil.mavlink_connection(args.device, baud=args.baudrate)

lat = -35.1234
lon = 139.47
alt_m = 584.0
climb_rate_fps = 1.2
icao_address = 87654
squawk = 172
trkn = 7

while True:
    master.mav.adsb_vehicle_send(icao_address,
                                 int(lat*1e7),
                                     int(lon*1e7),
                                     mavutil.mavlink.ADSB_ALTITUDE_TYPE_GEOMETRIC,
                                     int(alt_m*1000), # mm
                                     0, # heading
                                     0, # hor vel
                                     int(climb_rate_fps * 0.3048 * 100), # cm/s
                                     "%08x" % icao_address,
                                     100 + (trkn // 10000),
                                     1.0,
                                     (mavutil.mavlink.ADSB_FLAGS_VALID_COORDS |
                                          mavutil.mavlink.ADSB_FLAGS_VALID_ALTITUDE |
                                          mavutil.mavlink.ADSB_FLAGS_VALID_VELOCITY |
                                     mavutil.mavlink.ADSB_FLAGS_VALID_HEADING),
                                     squawk)
    time.sleep(1)
