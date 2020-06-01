#!/usr/bin/env python

import sys, random, os, subprocess, multiprocessing, time
import PIL
from PIL import Image

from argparse import ArgumentParser
parser = ArgumentParser(description=__doc__)

parser.add_argument("--parallel", default=6, type=int)
parser.add_argument("files", type=str, nargs='+', help="input files")
args = parser.parse_args()

files = list(dict.fromkeys(args.files))
nfiles = len(files)

done = set()

def process_one(fname):
    '''process one file'''
    print("Creating gzip for %s %u/%u" % (fname, len(done), len(files)))
    subprocess.call(['gzip', '-f', fname])

def is_done(fname):
    '''check if already done'''
    if os.path.exists(fname+".gz"):
        return True
    return False

files2 = []
for f in files:
    if not is_done(f):
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
    if is_done(fname):
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
