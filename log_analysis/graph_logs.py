#!/usr/bin/env python

import sys, random, os, time, multiprocessing, gzip
from pymavlink import mavutil
from MAVProxy.modules.lib import grapher
import common

from argparse import ArgumentParser
parser = ArgumentParser(description=__doc__)

parser.add_argument("--newer", action='store_true')
parser.add_argument("--parallel", default=6, type=int)
parser.add_argument("files", type=str, nargs='+', help="input files")
args = parser.parse_args()

files = list(dict.fromkeys(args.files))
nfiles = len(files)

graphed = set()

graphs = [
    ('EKF Innovation Lane1', 'NKF4.SV NKF4.SP NKF4.SH NKF4.SM'),
    ('EKF Innovation Lane2', 'NKF9.SV NKF9.SP NKF9.SH NKF9.SM'),
    ('Yaw Discrepancy', 'wrap_180(NKF1.Yaw-NKF6.Yaw)'),
    ('Attitude Control', 'ATT.Roll ATT.DesRoll ATT.Pitch ATT.DesPitch'),
    ('GPS Altitude', 'GPS.Alt GPS2.Alt'),
    ('GPS Speed', 'GPS.Spd GPS2.Spd'),
    ('GPS Status', 'GPS.Status GPS2.Status'),
    ('GPS Sats', 'GPS.NSats GPS2.NSats'),
    ('GPS1 Accuracy', 'GPA.HAcc GPA.SAcc GPA.VAcc'),
    ('GPS2 Accuracy', 'GPA2.HAcc GPA2.SAcc GPA2.VAcc'),
    ('EKF Roll', 'NKF1.Roll NKF6.Roll'),
    ('EKF Pitch', 'NKF1.Pitch NKF6.Pitch'),
    ('EKF PosDown', 'NKF1.PD NKF6.PD'),
    ('EKF PosError', 'sqrt((NKF1.PN-NKF6.PN)**2+(NKF1.PE-NKF6.PE)**2)'),
    ('EKF VelError', 'sqrt((NKF1.VN-NKF6.VN)**2+(NKF1.VE-NKF6.VE)**2)'),
    ('EKF Active Lane', 'NKF4.PI'),
    ('Baro Altitude', 'BARO.Alt BAR2.Alt'),
    ('Baro Temperature', 'BARO.Temp BAR2.Temp'),
    ('Vibration', 'VIBE.VibeX VIBE.VibeY VIBE.VibeZ'),
    ('Clipping', 'VIBE.Clip0 VIBE.Clip1 VIBE.Clip2'),
    ('Battery Current', 'BAT.Curr'),
    ('Battery Voltage', 'BAT.Volt'),
    ('Events', 'EV.Id'),
    ('Scheduling NLon', 'PM.NLon'),
    ('Scheduling MaxT', 'PM.MaxT'),
    ('Nav Velocity Error', 'sqrt((NTUN.VelX-NTUN.DVelX)**2+(NTUN.VelY-NTUN.DVelY)**2)*0.01'),
    ('Nav Position Error', 'sqrt((NTUN.PosX-NTUN.DPosX)**2+(NTUN.PosY-NTUN.DPosY)**2)*0.01'),
    ('Climb Rate', 'CTUN.CRt CTUN.DCRt'),
    ('Height Control', 'CTUN.Alt CTUN.DAlt'),
    ('IMU LPF X', 'lowpass(IMU.AccX,0,0.9) lowpass(IMU2.AccX,1,0.9) lowpass(IMU3.AccX,2,0.9)'),
    ('IMU LPF Y', 'lowpass(IMU.AccY,0,0.9) lowpass(IMU2.AccY,1,0.9) lowpass(IMU3.AccY,2,0.9)'),
    ('IMU LPF Z', 'lowpass(IMU.AccZ,0,0.9) lowpass(IMU2.AccZ,1,0.9) lowpass(IMU3.AccZ,2,0.9)'),
    ('EKF EarthField', 'NKF2.MN NKF2.ME NKF2.MD NKF7.MN NKF7.ME NKF7.MD'),
    ('EKF Lane1 EarthField Error', 'earth_field_error(GPS,NKF2).x earth_field_error(GPS,NKF2).y earth_field_error(GPS,NKF2).z'),
    ('EKF Lane2 EarthField Error', 'earth_field_error(GPS,NKF7).x earth_field_error(GPS,NKF7).y earth_field_error(GPS,NKF7).z'),
    ('Compass 1', 'MAG.MagX MAG.MagY MAG.MagZ sqrt(MAG.MagX**2+MAG.MagY**2+MAG.MagZ**2)'),
    ('Compass 2', 'MAG2.MagX MAG2.MagY MAG2.MagZ sqrt(MAG2.MagX**2+MAG2.MagY**2+MAG2.MagZ**2)'),
    ('Compass1 vs WMM', 'MAG.MagX expected_mag(GPS,ATT).x MAG.MagY expected_mag(GPS,ATT).y MAG.MagZ expected_mag(GPS,ATT).z'),
    ('Compass2 vs WMM', 'MAG2.MagX expected_mag(GPS,ATT).x MAG2.MagY expected_mag(GPS,ATT).y MAG2.MagZ expected_mag(GPS,ATT).z'),
    ('EKF Gyro Bias Lane1', 'NKF1.GX NKF1.GY NKF1.GZ'),
    ('EKF Gyro Bias Lane2', 'NKF6.GX NKF6.GY NKF6.GZ'),
    ('PL.Heal', 'PL.Heal'),
    ('Precland Pos', 'PL.pX PL.pY PL.mpX PL.mpY'),
    ('Board Voltage', 'POWR.Vcc'),
    ('GPS Jam Indicator', 'UBX1.jamInd'),
    ('Motor Outputs', 'RCOU.C1 RCOU.C2 RCOU.C3 RCOU.C4'),
    ('Flight Mode', 'MODE.Mode'),
    ('FTS State', 'FTSS.State FTSS.Rsn FTS2.Fuse'),
    ('FTS Watchdog Margin', 'FTS2.WDTm'),
    ('FTSPower', 'FTSS.BC1mV FTSS.BC2mV FTSS.BC3mV'),
    ('FTS SOC', 'FTSS.BSoC'),
    ('Rangefinder1', 'RFND.Dist1 RFND.Dist2'),
]

def graph_one(mlog, title, expression, filename):
    '''create one graph'''
    print("Graphing %s" % title)
    mg = grapher.MavGraph()
    mg.add_mav(mlog)
    for e in expression.split():
        mg.add_field(e)
    mg.set_title(title)
    mg.process([],[],0)
    mg.show(1, output=filename)

def script_path():
    '''path of this script'''
    import inspect
    return os.path.abspath(inspect.getfile(inspect.currentframe()))

def process_one(fname):
    '''process one file'''
    html = fname + ".html.gz"
    print("Graphing %s %u/%u" % (fname, len(graphed), len(files)))
    tmp = html+".tmp"+str(random.randint(0,100000))+".gz"
    tmpg = tmp+".html"
    try:
        os.unlink(tmp)
    except Exception:
        pass
    try:
        os.unlink(tmpg)
    except Exception:
        pass

    bname = os.path.basename(fname)
    f = gzip.open(tmp, "wb")
    f.write("<html><head><title>Graphs of %s</title><body>\n" % bname)
    f.write('<a href="/"><img src="/logo.jpg"></a><p>')
    f.write("<h1>Graphs of log %s</h1>\n" % bname)
    if os.path.exists("all/%s.parm" % bname):
        f.write('Parameters: <a href="../all/%s.parm" target="_blank">%s.parm</a><br>\n' % (bname, bname))
    if os.path.exists("all/%s.msg" % bname):
        f.write('Messages: <a href="../all/%s.msg" target="_blank">%s.msg</a><br>\n' % (bname, bname))
        common.add_version(f, "all/%s" % bname)
    f.write('Logfile: <a href="../all/%s" target="_blank">%s</a><br>\n' % (bname, bname))
    if os.path.exists("all/%s.err" % bname):
        f.write("<pre>\n")
        f.write(open("all/%s.err" % bname).read())
        f.write("</pre>\n<hr>\n")
    mlog = mavutil.mavlink_connection(fname)
    for (title, expression) in graphs:
        p = multiprocessing.Process(target=graph_one, args=(mlog, title, expression, tmpg))
        p.start()
        p.join()
        if os.path.exists(tmpg):
            gdata = open(tmpg,"r").read()
            f.write(gdata)
            os.unlink(tmpg)
    f.write("</body>\n")
    f.close()
    if os.path.exists(tmp):
        try:
            os.unlink(html)
        except Exception:
            pass
        os.rename(tmp, html)
    if os.path.exists(fname + ".html"):
        os.unlink(fname + ".html")

def is_done(fname):
    '''check if already done'''
    if os.path.exists(fname):
        if args.newer:
            if os.path.getmtime(fname) > os.path.getmtime(script_path()):
                return True
        else:
            return True
    if os.path.exists(fname+".gz"):
        if args.newer:
            if os.path.getmtime(fname+".gz") > os.path.getmtime(script_path()):
                return True
        else:
            return True
    return False

files2 = []
for f in files:
    if os.path.getsize(f) == 0:
        continue
    if not is_done(f + ".html"):
        files2.append(f)
files = files2
nfiles = len(files2)

procs = []

while len(graphed) < nfiles:
    r = random.randint(0, nfiles-1)
    fname = files[r]
    if fname in graphed:
        continue
    graphed.add(fname)
    html = fname + ".html"
    if is_done(html):
        continue
    p = multiprocessing.Process(target=process_one, args=(fname,))
    p.start()
    procs.append(p)
    while len(procs) >= args.parallel:
        for p in procs:
            if p.exitcode is not None:
                p.join()
                procs.remove(p)
        time.sleep(0.1)
