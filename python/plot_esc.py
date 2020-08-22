#!/usr/bin/env python

import matplotlib.pyplot as plt
import csv, sys

from optparse import OptionParser
parser = OptionParser("plot_esc.py [options]")
parser.add_option("--poles", type=int, default=16, help="motor poles")
parser.add_option("--plot1", default='Motor_RPM', help="plot1")
parser.add_option("--plot2", default='Drive_Current', help="plot2")
parser.add_option("--last-session", action='store_true', default=False, help="only last session")
parser.add_option("--save", default=None, help="safe name")

(opts, args) = parser.parse_args()

if len(args) < 1:
    print("Usage: plot_esc.py [options] <LOGFILE...>")
    sys.exit(1)

def get_value(row, fields, p):
    if not p in fields:
        print("No column %s" % p)
        sys.exit(1)
    v = float(row[fields.index(p)])
    if p == 'Motor_RPM':
        v /= (0.5 * opts.poles)
    return v

x = []
p1 = []
p2 = []
fields = None
filename = args[0]

last_session = None
session_ofs = 0
lines = open(filename,"r").readlines()
i = 0

for line in lines:
    line = line.strip()
    if line[0] == '#':
        continue
    if fields is None:
        fields = line.split(",")
        continue
    i += 1
    row = line.split(',')
    session = get_value(row, fields, 'LogSession_ID')
    t0 = i / float(row[fields.index('Log_Frequency')])
    if last_session is not None and session != last_session:
        x.append(t0 + session_ofs*30)
        p1.append(0)
        p2.append(0)
        session_ofs += 30
        x.append(t0 + session_ofs*30)
        p1.append(0)
        p2.append(0)
        if opts.last_session:
            x = []
            p1 = []
            p2 = []
    t = t0 + session_ofs * 30
    v1 = get_value(row, fields, opts.plot1)
    v2 = get_value(row, fields, opts.plot2)
    p1.append(v1)
    p2.append(v2)
    x.append(t)
    last_session = session

fig, ax1 = plt.subplots()
ax1.set_xlabel("time(s)")
ax1.set_ylabel(opts.plot1)
ax1.plot(x, p1, label=opts.plot1, color='tab:red')

ax2 = ax1.twinx()
ax2.set_ylabel(opts.plot2)
ax2.plot(x, p2, label=opts.plot2, color='tab:green')

if filename.endswith(".csv"):
    filename = filename[:-4]
plt.title('ESC %s %s\n%s' % (opts.plot1, opts.plot2, filename))
fig.legend()
if opts.save:
    print("Saving %s" % opts.save)
    plt.savefig(opts.save)
plt.show()

