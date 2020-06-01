#!/usr/bin/env python
'''
onboard log analysis script for ArduPilot, focussing on discrepancies between multiple
estimates of the same variable
'''

from pymavlink import mavutil
from pymavlink import mavextra
import math, sys

from argparse import ArgumentParser
parser = ArgumentParser(description=__doc__)

parser.add_argument("--num-cells", default=12, type=int, help="number of LiPo cells in main battery")
parser.add_argument("--min-cell-volt", default=3.0, type=float, help="cell voltage threshold")
parser.add_argument("--max-current", default=70.0, type=float, help="maximum current")
parser.add_argument("--innovation-threshold", default=0.7, type=float, help="EKF innovation threshold")
parser.add_argument("--gyro-bias-threshold", type=float, default=4.0, help="EKF gyro bias threshold")
parser.add_argument("infile", default=None, help="input file")
parser.add_argument("outfile", default=None, nargs='?', help="output file")
args = parser.parse_args()

mlog = mavutil.mavlink_connection(args.infile)

msg_types = set(['NKF1', 'NKF2', 'NKF4', 'NKF6', 'NKF7', 'NKF9', 'GPS', 'GPS2', 'VIBE', 'BAT',
                 'IMU', 'IMU2', 'IMU3', 'BARO', 'BAR2', 'CTUN', 'EV', 'PM', 'DSF',
                 'MAG', 'MAG2', 'NTUN'])

errors = {}

def add_error(label):
    '''add an error'''
    global errors
    if not label in errors:
        errors[label] = 0
    errors[label] += 1

check_errors = {}
max_errors = {}

def check_error(label, value, margin, count, check_low=False):
    '''check if an error if over a margin for count samples'''
    global check_errors, max_errors
    if not label in check_errors:
        check_errors[label] = 0
        max_errors[label] = 0
    if check_low:
        in_error = value < margin
    else:
        in_error = value > margin
    if not in_error:
        check_errors[label] = 0
        return
    check_errors[label] += 1
    if check_errors[label] >= count:
        if check_low:
            max_errors[label] = min(value, max_errors[label])
        else:
            max_errors[label] = max(value, max_errors[label])

earth_field = None
done_takeoff = False

last_CTUN = None
have_PM_LogDrop = False
pending_GPS_GMS = False
previous_msgs = {}

while True:
    m = mlog.recv_match(type=msg_types)
    if m is None:
        break

    mtype = m.get_type()

    if mtype == 'CTUN':
        if last_CTUN is not None and last_CTUN.ThO > 0:
            if 'TimeUS' in m._fieldnames:
                if m.TimeUS*1.0e-6 - last_CTUN.TimeUS*1.0e-6 > 2:
                    # detect gaps in logging of more than 2 seconds by looking at CTUN messages
                    add_error("Log Gap")
        last_CTUN = m;

    if mtype == 'PM' and 'LogDrop' in m._fieldnames:
        have_PM_LogDrop = True

    # skip most tests when not flying
    if last_CTUN is None or last_CTUN.ThO < 0.05:
        previous_msgs[mtype] = m
        continue

    if mtype in previous_msgs:
        mprev = previous_msgs[mtype]
    else:
        mprev = None

    if mtype == 'NKF6' and 'NKF1' in mlog.messages:
        # check for roll/pitch error
        NKF1 = mlog.messages['NKF1']
        check_error('EKF Roll', abs(NKF1.Roll - m.Roll), 5, 3)
        check_error('EKF Pitch', abs(NKF1.Pitch - m.Pitch), 5, 3)
        check_error('EKF Height error', abs(NKF1.PD - m.PD), 3, 3)
        check_error('EKF Position error', math.sqrt((NKF1.PN-m.PN)**2 + (NKF1.PE-m.PE)**2), 5, 3)
        check_error('EKF Velocity error', math.sqrt((NKF1.VN-m.VN)**2 + (NKF1.VE-m.VE)**2), 3, 3)

    if mtype == 'NKF4':
        check_error('Velocity Innovation Lane1', m.SV, args.innovation_threshold, 2)
        check_error('Position Innovation Lane1', m.SP, args.innovation_threshold, 2)
        check_error('Height Innovation Lane1', m.SH, args.innovation_threshold, 2)
        check_error('Compass Innovation Lane1', m.SM, args.innovation_threshold, 2)

    if mtype == 'NKF1':
        check_error('EKF gyro bias X lane1', m.GX, args.gyro_bias_threshold, 2)
        check_error('EKF gyro bias Y lane1', m.GY, args.gyro_bias_threshold, 2)
        check_error('EKF gyro bias Z lane1', m.GZ, args.gyro_bias_threshold, 2)

    if mtype == 'NKF6':
        check_error('EKF gyro bias X lane2', m.GX, args.gyro_bias_threshold, 2)
        check_error('EKF gyro bias Y lane2', m.GY, args.gyro_bias_threshold, 2)
        check_error('EKF gyro bias Z lane2', m.GZ, args.gyro_bias_threshold, 2)
        
    if mtype == 'NKF9':
        check_error('Velocity Innovation Lane2', m.SV, args.innovation_threshold, 2)
        check_error('Position Innovation Lane2', m.SP, args.innovation_threshold, 2)
        check_error('Height Innovation Lane2', m.SH, args.innovation_threshold, 2)
        check_error('Compass Innovation Lane2', m.SM, args.innovation_threshold, 2)

    if done_takeoff and mtype == 'NKF2' and earth_field is not None and (m.MX != 0 or m.MY != 0 or m.MZ != 0):
        check_error('EKF Lane1 EarthField N', abs(m.MN-earth_field.x), 150, 2)
        check_error('EKF Lane1 EarthField E', abs(m.ME-earth_field.y), 150, 2)
        check_error('EKF Lane1 EarthField D', abs(m.MD-earth_field.z), 200, 2)

    if done_takeoff and mtype == 'NKF7' and earth_field is not None and (m.MX != 0 or m.MY != 0 or m.MZ != 0):
        check_error('EKF Lane2 EarthField N', abs(m.MN-earth_field.x), 150, 2)
        check_error('EKF Lane2 EarthField E', abs(m.ME-earth_field.y), 150, 2)
        check_error('EKF Lane2 EarthField D', abs(m.MD-earth_field.z), 200, 2)
        
    if mtype == 'NKF4':
        # check for lane changed
        if m.PI > 0:
            add_error('EKF lane2 while flying')

    if mtype == 'GPS2' and 'GPS' in mlog.messages:
        # check for GPS discrepancies
        GPS = mlog.messages['GPS']
        check_error('GPS Altitude', abs(m.Alt-GPS.Alt), 6, 2)
        check_error('GPS Speed', abs(m.Spd-GPS.Spd), 3, 2)

    if mtype.startswith('GPS'):
        check_error(mtype + ' Status', m.Status, 2, 1, check_low=True)
        if earth_field is None and 'ATT' in mlog.messages:
            ATT = mlog.messages['ATT']
            ef = mavextra.expected_mag(m, ATT)
            earth_field = mavextra.earth_field

        status = m.Status
        if status == 0 and mprev is not None and mprev.Status > 0:
            if mtype == 'GPS2':
                add_error('Lost GPS2')
            else:
                add_error('Lost GPS1')

        if m.GMS != 0 and mprev is not None and mprev.GMS != 0 and m.GWk == mprev.GWk:
            if m.GMS - mprev.GMS > 390:
                # this could be a gap in logging or it could be lost GPS packets, delay this error
                # until we know if we are losing log packets
                pending_GPS_GMS = True

    if mtype.startswith('GPA'):
        # check for high GPS delta numbers
        if 'Delta' in m._fieldnames:
            if m.Delta > 500:
                add_error("GPS High DeltaTime")
            elif m.Delta > 390:
                add_error("GPS Packet Loss")

    if mtype == 'BAR2' and 'BARO' in mlog.messages:
        # check for baro discrepancies
        BARO = mlog.messages['BARO']
        check_error('Baro Altitude', abs(m.Alt-BARO.Alt), 10, 3)
        check_error('Baro Temp', abs(m.Temp-BARO.Temp), 15, 3)

    if mtype in ['BARO', 'BAR2']:
        check_error(mtype + ' Max Temp', m.Temp, 70, 3)
        if m.Alt > 10:
            # simple takeoff check
            done_takeoff = True

    if mtype.startswith('IMU') and 'Temp' in m._fieldnames:
        check_error(mtype + ' Max Temp', m.Temp, 70, 3)

    if mtype == 'VIBE':
        check_error('Accel Clip0', m.Clip0, 100, 1)
        check_error('Accel Clip1', m.Clip1, 100, 1)
        check_error('Accel Clip2', m.Clip2, 100, 1)

    if mtype == 'BAT':
        check_error('Battery Current', m.Curr, args.max_current, 1)
        check_error('Battery Cell Voltage', m.Volt/args.num_cells, args.min_cell_volt, 1, check_low=True)

    if mtype == 'EV':
        if m.Id == 51:
            add_error('Parachute release')

    if mtype == 'PM':
        check_error('Scheduling Long', m.NLon, 1000, 1)
        check_error('Low Memory', m.Mem, 2000, 1, check_low=True)

    if mtype == 'DSF' and mprev is not None:
        dropped = m.Dp - mprev.Dp
        check_error('Log Drop', dropped, 0, 1)
        if pending_GPS_GMS:
            if dropped == 0:
                # we now know the GMS gap was real, as we didn't drop any more log packets
                # in this period
                add_error("GPS GMS Delta")
            pending_GPS_GMS = False

    if mtype.startswith('MAG'):
        check_error(mtype + ' Health', m.Health, 0, 1, check_low=True)

    if mtype == 'NTUN':
        check_error('Nav Velocity error', math.sqrt((m.VelX-m.DVelX)**2 + (m.VelY-m.DVelY)**2)*0.01, 5, 5)
        check_error('Nav Position error', math.sqrt((m.PosX-m.DPosX)**2 + (m.PosY-m.DPosY)**2)*0.01, 150, 2)

    if mtype == 'CTUN':
        check_error('Climb Rate', abs(m.DCRt-m.CRt)*0.01, 2, 3)
        check_error('Target Alt', abs(m.DAlt-m.Alt)*0.01, 3, 3)

    # check IMU and gyro health
    if mtype.startswith('IMU') and 'AH' in m._fieldnames:
        check_error(mtype + ' Accel Health', m.AH, 0, 1, check_low=True)
        check_error(mtype + ' Gyro Health', m.GH, 0, 1, check_low=True)

    previous_msgs[mtype] = m

# if motors were running in last CTUN then we stopped logging while flying
if last_CTUN is not None and last_CTUN.ThO > 0:
    add_error("Logging stopped while flying")

err_count = 0

if args.outfile is None:
    f = sys.stdout
else:
    f = open(args.outfile, 'w')

for k in sorted(max_errors.keys()):
    if max_errors[k] > 0:
        f.write("%s: max %.2f\n" % (k, max_errors[k]))
        err_count += 1

for k in sorted(errors.keys()):
    f.write("%s: %u\n" % (k, errors[k]))
    err_count += 1
        
f.write("ERRCOUNT: %u\n" % err_count)
f.close()
