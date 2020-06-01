#!/usr/bin/env python

import sys, random, os, subprocess, multiprocessing, time
import PIL
from PIL import Image

from argparse import ArgumentParser
parser = ArgumentParser(description=__doc__)

parser.add_argument("--newer", action='store_true')
parser.add_argument("--parallel", default=6, type=int)
parser.add_argument("files", type=str, nargs='+', help="input files")
args = parser.parse_args()

files = list(dict.fromkeys(args.files))
nfiles = len(files)

done = set()

def script_path():
    '''path of this script'''
    import inspect
    return os.path.abspath(inspect.getfile(inspect.currentframe()))

def process_one(fname):
    '''process one file'''
    msgfile = fname + ".msg"
    print("Creating messages for %s %u/%u" % (fname, len(done), len(files)))
    tmp = msgfile+str(random.randint(0,100000))
    try:
        os.unlink(tmp)
    except Exception:
        pass

    ret = subprocess.call(['mavlogdump.py', '--type=MSG', fname], stdout=open(tmp, 'w'))
    if os.path.exists(tmp):
        try:
            os.unlink(msgfile)
        except Exception:
            pass
        os.rename(tmp, msgfile)

def is_done(fname):
    '''check if already done'''
    if os.path.exists(fname):
        if args.newer:
            if os.path.getmtime(fname) > os.path.getmtime(script_path()):
                return True
        else:
            return True
    return False

files2 = []
for f in files:
    if os.path.getsize(f) == 0:
        continue
    if not is_done(f + ".msg"):
        files2.append(f)
files = files2
nfiles = len(files2)
        
procs = []

while len(done) < nfiles:
    r = random.randint(0, nfiles-1)
    fname = files[r]
    if fname in done:
        continue
    done.add(fname)
    msgfile = fname + ".msg"
    if is_done(msgfile):
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
