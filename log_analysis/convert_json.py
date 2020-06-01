#!/usr/bin/env python
'''
rough script to convert MAVNet JSON file to a format that MAVExplorer can read
'''

import json
import sys
from math import *

# map of JSON field names to log msg fields
field_map = {
    "lat": "GPS.Lat",
    "lon": "GPS.Lng",
    "alt": "GPS.Alt",
    "heading": "GPS.GCrs",
    "pitch": "ATT.Pitch",
    "yaw": "ATT.Yaw",
    "roll": "ATT.Roll",
    "batteryCurrent": "BATT.Curr",
    "batteryVoltage": "BATT.Volt",
    "flightMode": "MODE.Mode",
    "timestamp": "JSON.timestamp",
    "systemStatus": "JSON.systemStatus",
    "relAlt": "POS.RelHomeAlt",
    "aglAlt": "POS.AGLAlt",
    "pitchSpeed": "ATT.GyrY",
    "rollSpeed": "ATT.GyrX",
    "yawSpeed": "ATT.GyrZ",
    "vx": "GPS.vx",
    "vy": "GPS.vy",
    "vz": "GPS.vz",
    "navRoll": "ATT.DesRoll",
    "navPitch": "ATT.DesPitch",
    "navBearing": "ATT.NavYaw",
    "targetBearing": "NTUN.targetBearing",
    "pixhawkUptime": "TIME.pixhawkUptime",
    "beagleboneUptime": "TIME.beagleboneUptime",
    "autopilotVoltage": "POWR.VCC",
    "gpsFixType": "GPS.Status",
    "satellitesVisible": "GPS.NSats",
    "gpsHDOP": "GPS.HDOP",
    "gpsVDOP": "GPS.VDOP",
    "xacc": "IMU.AccX",
    "yacc": "IMU.AccY",
    "zacc": "IMU.AccZ",
    "xgyro": "IMU.GyrX",
    "ygyro": "IMU.GyrY",
    "zgyro": "IMU.GyrZ",
    "xmag": "MAG.MagX",
    "ymag": "MAG.MagY",
    "zmag": "MAG.MagZ",
    "rcChan1Raw": "RCIN.C1",
    "rcChan2Raw": "RCIN.C2",
    "rcChan3Raw": "RCIN.C3",
    "rcChan4Raw": "RCIN.C4",
    "rcChan5Raw": "RCIN.C5",
    "rcChan6Raw": "RCIN.C6",
    "rcChan7Raw": "RCIN.C7",
    "rc1": "RCOU.C1",
    "rc2": "RCOU.C2",
    "rc3": "RCOU.C3",
    "rc4": "RCOU.C4",
    "airspeed": "JSON.Airspeed",
    "groundspeed": "GPS.Spd",
    "throttle": "CTUN.ThO",
    "missionCurrentSeq": "NTUN.seq",
    "flightPlan": "JSON.flightPlan",
    "wpDist": "NTUN.wpDist",
    "accessoryState": "JSON.accessoryState",
    "accessoryGoal": "JSON.accessoryGoal",
    "ftsCellVolt1": "FTS.Volt1",
    "ftsCellVolt2": "FTS.Volt2",
    "ftsCellVolt3": "FTS.Volt3",
    "ftsBattSOC": "FTS.BattSOC",
    "ftsFETTemp": "FTS.Temp",
    "smbtFlag": "SMBT.Flag",
    "smbtPackVoltage": "SMBT.Volt",
    "smbtDischargeCurrent": "SMBT.Curr",
    "smbtChargeCurrent": "SMBT.ChargeCurrent",
    "smbtSOC": "SMBT.SOC",
    "smbtTemp1": "SMBT.Temp1",
    "smbtTemp2": "SMBT.Temp2",
    "smbtTemp3": "SMBT.Temp3",
    "smbtTemp4": "SMBT.Temp4",
    "autoPilotLoad": "PM.Load",
    "sensorsPresent": "SENS.Present",
    "sensorsEnabled": "SENS.Enabled",
    "sensorsHealth": "SENS.Health",
    "ekfFlags": "EKF.Flags",
    "velocityVariance": "EKF.velocityVariance",
    "posHorizVariance": "EKF.posHorizVariance",
    "posVertVariance": "EKF.posVertVariance",
    "compassVariance": "EKF.compassVariance",
    "cellModemRssi": "CELL.RSSI",
    "cellModemBitErrorRate": "CELL.ErrorRate",
    "GPSWk" : "GPS.GWk",
    "GPSMS" : "GPS.GMS",
}

# scaling factors to apply to fields
scalings = {
    "IMU.AccX" : 9.81 * 0.001,
    "IMU.AccY" : 9.81 * 0.001,
    "IMU.AccZ" : 9.81 * 0.001,
    "ATT.Roll" : degrees(1),
    "ATT.Pitch" : degrees(1),
    "ATT.Yaw" : degrees(1),
    "GPS.Alt" : 0.001,
    "GPS.GCrs" : 0.01,
    "POS.RelHomeAlt" : 0.001,
    "POS.AGLAlt" : 0.001,
    "BATT.Curr" : 0.01,
    "BATT.Volt" : 0.001,
}

# from field name to msg
field_to_msg = {}

# from field name to msgfield
field_to_msgfield = {}

# from msg name to array of msgfield names
msgfields = {}

# from key to index into msg array
msgindex = {}


# create lookup dictionaries
for k in field_map.keys():
    a = field_map[k].split(".")
    msg = a[0]
    msgfield = a[1]
    field_to_msg[k] = msg
    field_to_msgfield[k] = msgfield
    if not msg in msgfields:
        msgfields[msg] = []
    msgindex[k] = len(msgfields[msg])
    msgfields[msg].append(msgfield)


# open files
fname = sys.argv[1]

f = open(fname, 'r')
a = json.load(f)

of = open(fname + ".log", 'w')

first_timestamp = 0

def add_fmt(msg, of):
    '''add a FMT msg describing a log message'''
    idx = msgfields.keys().index(msg)+129
    nfields = len(msgfields[msg])
    of.write("FMT, %u, %u, %s, %s, TimeUS,%s\n" % (idx, 8+4*nfields, msg, "Q"+"f"*nfields, ','.join(msgfields[msg])))

# write out the FMT lines
of.write('FMT, 128, 89, FMT, BBnNZ, Type,Length,Name,Format,Columns\n')
for msg in msgfields.keys():
    add_fmt(msg, of)

def convert_record(r, of):
    '''convert one JSON record and write as a set of log msgs'''
    global first_timestamp
    timestamp = r['timestamp']
    if first_timestamp == 0:
        first_timestamp = timestamp
    timeUS = (timestamp - first_timestamp) * 1000000
    msgs = {}
    for k in r.keys():
        if not k in field_map:
            continue
        field = field_map[k]
        msg = field_to_msg[k]
        msgfield = field_to_msgfield[k]
        if not msg in msgs:
            msgs[msg] = ['0.0']*len(msgfields[msg])
        v = r[k]
        if field in scalings:
            v *= scalings[field]
        msgs[msg][msgindex[k]] = str(v)
    # fake up GPS time
    msgs['GPS'][msgindex['GPSWk']] = '0'
    msgs['GPS'][msgindex['GPSMS']] = str(timeUS/1000)
    for msg in msgs.keys():
        of.write("%s, %u, %s\n" % (msg, timeUS, ", ".join(msgs[msg])))

# convert all records
for r in a:
    convert_record(r,of)
