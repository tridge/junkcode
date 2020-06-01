#!/usr/bin/env python

import csv, os, sys, time, re
import common

from argparse import ArgumentParser
parser = ArgumentParser(description=__doc__)

parser.add_argument("--csv", default=None, help="csv flight descriptions")
args = parser.parse_args()

all_records = []
all_byname = {}
by_error = {}
by_vehicle = {}
by_month = {}
by_route = {}
by_location = {}
by_ecount = {}
by_id = {}
all_errors = []
all_routes = []
all_vehicles = []
all_months = []
all_locations = []
all_ecounts = []

class FlightRecord(object):
    def __init__(self, rec):
        self.vehicle = rec[0]
        self.id = rec[1]
        self.route = rec[2]
        self.location = rec[3]
        self.departure = rec[4]
        self.date = self.departure.split(' ')[0]
        self.month = self.date[:7]
        self.filename = rec[5]
        self.errors = {}
        self.error_count = 0
        self.load_errors()

    def load_errors(self):
        '''load err file for log'''
        errfile = os.path.join('all', self.filename+".err")
        if not os.path.exists(errfile):
            return
        for e in open(errfile, 'r').readlines():
            e = e.strip()
            if not e:
                continue
            a = e.split(':')
            v = a[1].strip()
            ename = a[0]
            if ename != 'ERRCOUNT':
                self.errors[ename] = v
        self.error_count = len(self.errors.keys())

    def __str__(self):
        return "%s/%s/%s/%s" % (self.vehicle, self.location, self.route, self.departure)

def load_csv():
    '''load the csv file'''
    global all_records, all_byname
    f = open(args.csv, 'r')
    lines = f.readlines()
    for r in lines:
        rec = re.split('\t|,',r.strip())
        if rec[0] == 'name':
            continue
        if len(rec) == 6:
            filename = rec[5]
            if not filename:
                continue
            if not os.path.exists(os.path.join("all", filename)):
                continue
            if filename.endswith('html'):
                continue
            all_records.append(FlightRecord(rec))
            rec = all_records[-1]
            all_byname[filename] = rec
    # sort by reverse date, most recent first
    all_records.sort(key=lambda x: x.date, reverse=True)
    print("Loaded %u records" % len(all_records))

def load_unknown():
    '''load files not in csv'''
    global all_records, all_byname
    nrecs = len(all_records)
    for f in os.listdir('all'):
        if not f.endswith(".BIN"):
            continue
        if f in all_byname:
            continue
        dstr = time.strftime('%Y-%m-%d %H:%M:%S', time.localtime(os.path.getmtime(os.path.join('all', f))))
        rec = ['UNKNOWN', 'UNKNOWN', 'UNKNOWN', 'UNKNOWN', dstr, f]
        all_records.append(FlightRecord(rec))
        all_byname[f] = all_records[-1]
    # sort by reverse date, most recent first
    all_records.sort(key=lambda x: x.date, reverse=True)
    print("Loaded %u UNKNOWN records" % (len(all_records)-nrecs))
    
def create_mappings():
    '''create the by_ and all_ dictionaries'''
    global all_records, all_vehicles, all_errors, all_routes, all_locations, all_months, all_ecounts
    for r in all_records:
        if not r.vehicle in all_vehicles:
            all_vehicles.append(r.vehicle)
        if not r.month in all_months:
            all_months.append(r.month)
        if not r.route in all_routes:
            all_routes.append(r.route)
        if not r.location in all_locations:
            all_locations.append(r.location)
        if not r.error_count in all_ecounts:
            all_ecounts.append(r.error_count)
        for e in r.errors.keys():
            if not e in all_errors:
                all_errors.append(e)
    for k in all_vehicles:
        by_vehicle[k] = []
    for k in all_months:
        by_month[k] = []
    for k in all_errors:
        by_error[k] = []
    for k in all_routes:
        by_route[k] = []
    for k in all_ecounts:
        by_ecount[k] = []
    for k in all_locations:
        by_location[k] = []
    for r in all_records:
        by_vehicle[r.vehicle].append(r)
        by_route[r.route].append(r)
        by_location[r.location].append(r)
        by_month[r.month].append(r)
        by_ecount[r.error_count].append(r)
        if not r.id in by_id:
            by_id[r.id] = []
        by_id[r.id].append(r)
        for e in r.errors.keys():
            by_error[e].append(r)

def write_headers(f, label):
    '''write HTML headers'''
    f.write('''<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml">
<head>
<meta http-equiv="Content-Type" content="text/html; charset=UTF-8" />
<style>
table, th, td {
  border: 1px solid black;
}
</style>
<title>Logs %s</title>
</head>
<body>
<a href="/"><img src="/logo.jpg"></a><p>
<h1>%s</h1>
''' % (label, label))

def write_footers(f):
    '''write HTML footers'''
    f.write('''
</body>
</html>
''')

def clean_string(s):
    '''cleanup a string for use as a dirname'''
    cleaned = ''
    s = str(s)
    for c in s:
        if c.isalnum() or c == '-':
            cleaned += c
        elif c.isspace():
            cleaned += '_'
    return cleaned

def create_list(label, html, recs, show_zero_errors):
    '''create a html file of errors for a label'''
    f = open(html, 'w')
    write_headers(f, label)
    f.write('<table>\n')
    for r in recs:
        if not show_zero_errors and len(r.errors.keys()) == 0:
            continue
        if not r.filename.upper().endswith('.BIN'):
            continue
        f.write('<tr>\n')
        f.write('<td>\n')
        f.write(str(r) + '<br>\n')
        if os.path.exists("all/%s.html" % r.filename) or os.path.exists("all/%s.html.gz" % r.filename):
            f.write('Graphs: <a href="../all/%s.html" target="_blank">%s.html</a><br>\n' % (r.filename, r.filename))
        if os.path.exists("all/%s.parm" % r.filename):
            f.write('Parameters: <a href="../all/%s.parm" target="_blank">%s.parm</a><br>\n' % (r.filename, r.filename))
        if os.path.exists("all/%s.msg" % r.filename):
            f.write('Messages: <a href="../all/%s.msg" target="_blank">%s.msg</a><br>\n' % (r.filename, r.filename))
            common.add_version(f, "all/%s" % r.filename)
        for idrec in by_id[r.id]:
            if not idrec.filename:
                continue
            fname, ext = os.path.splitext(idrec.filename)
            if ext == '.log':
                f.write('Mate log: <a href="../all/%s" target="_blank">%s</a><br>\n' % (idrec.filename, idrec.filename))
            elif ext == '.tlog':
                f.write('Telemetry log: <a href="../all/%s" target="_blank">%s</a><br>\n' % (idrec.filename, idrec.filename))

        f.write('Logfile: <a href="../all/%s">%s</a><p>\n' % (r.filename, r.filename))
        if len(r.errors.keys()) == 0:
            f.write("NO ERRORS\n")
        for e in sorted(r.errors.keys()):
            f.write("%s: %s<br>\n" % (e, r.errors[e]))
        f.write('</td>\n')
        f.write('<td>\n')
        mapfile = "all/%s.map.small.jpg" % r.filename
        if os.path.exists(mapfile) and os.path.getsize(mapfile) > 0:
            f.write('<a href="../all/%s.map.jpg"><img src="../all/%s.map.small.jpg"></a>\n' % (r.filename, r.filename))
        f.write('</td>\n')
        f.write('</tr>\n')
    f.write('</table>\n')
    write_footers(f)
    f.close()

all_selections = []

def create_selection(label, by_dict, show_zero_errors):
    '''create a selection for a given label'''
    global all_selections, all_records
    print("Creating %s" % label)
    all_selections.append(label)
    dirname = clean_string(label)
    try:
        os.makedirs(dirname)
    except Exception:
        pass
    f = open(os.path.join(dirname, 'index.html'), 'w')
    write_headers(f, label)
    f.write('<ul>\n')
    for k in sorted(by_dict.keys()):
        ck = clean_string(k)
        if not ck:
            continue
        html = ck + ".html"
        nlogs = len(by_dict[k])
        nlog_err = 0
        for r in by_dict[k]:
            if len(r.errors.keys()) > 0:
                nlog_err += 1
        if label.find('Error') != -1:
            total_logs = len(all_records)
        else:
            total_logs = nlogs
        if ck == "0":
            show_zero = True
            nlog_err = nlogs
        else:
            show_zero = show_zero_errors
        f.write('<li><a href="%s">%s</a> (%u logs with errors out of %u - %.1f%%)</li>\n' % (html, k, nlog_err, total_logs, nlog_err*100.0/total_logs))
        create_list(label + ": " + str(k), os.path.join(dirname, html), by_dict[k], show_zero)
    f.write('</ul>\n')
    write_footers(f)
    f.close()

def create_index(all_selections):
    '''create top level index'''
    f = open('index.html', 'w')
    write_headers(f, "Log Analysis")
    f.write('<ul>\n')
    for k in sorted(all_selections):
        dirname = clean_string(k)
        f.write('<li><a href="%s">%s</a></li>\n' % (dirname, k))
    f.write('</ul>\n')
    write_footers(f)
    f.close()

load_csv()
load_unknown()
create_mappings()
create_selection('By Vehicle', by_vehicle, True)
create_selection('By Location', by_location, True)
create_selection('By Route', by_route, True)
create_selection('By Error', by_error, False)
create_selection('By Month', by_month, True)
create_selection('By ErrorCount', by_ecount, False)
create_index(all_selections)
