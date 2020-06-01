#!/usr/bin/env python

import sys, subprocess, random, os, time, multiprocessing

from argparse import ArgumentParser
parser = ArgumentParser(description=__doc__)

parser.add_argument("--newer", action='store_true')
parser.add_argument("--parallel", default=6, type=int)
parser.add_argument("files", type=str, nargs='+', help="input files")
args = parser.parse_args()

files = list(dict.fromkeys(args.files))
nfiles = len(files)

analysed = set()

def script_path():
    '''path of this script'''
    import inspect
    return os.path.abspath(inspect.getfile(inspect.currentframe()))

def script_dir():
    '''directory of this script'''
    return os.path.dirname(script_path())

log_analyse = os.path.join(script_dir(), 'log_analyse.py')
if not os.path.exists(log_analyse):
    print("Missing log_analyse.py")
    sys.exit(1)

def process_one(fname, err):
    '''process one file'''
    print("Analysing %s %u/%u" % (fname, len(analysed), len(files)))
    tmp = err+str(random.randint(0,100000))
    try:
        os.unlink(tmp)
    except Exception:
        pass

    ret = subprocess.call(['nice', log_analyse, fname, tmp])
    if ret == 0:
        try:
            os.unlink(err)
        except Exception:
            pass
        os.rename(tmp, err)

procs = []

def is_done(err):
    '''check if already done'''
    if os.path.exists(err):
        if args.newer:
            if os.path.getmtime(err) > os.path.getmtime(log_analyse):
                return True
        else:
            return True
    return False

files2 = []
for f in files:
    if os.path.getsize(f) == 0:
        continue
    if not is_done(f + ".err"):
        files2.append(f)
files = files2
nfiles = len(files2)

while len(analysed) < nfiles:
    time.sleep(0.01)
    r = random.randint(0, nfiles-1)
    fname = files[r]
    if fname in analysed:
        continue
    analysed.add(fname)
    err = fname + ".err"
    if is_done(err):
        continue
    p = multiprocessing.Process(target=process_one, args=(fname,err))
    p.start()
    procs.append(p)
    while len(procs) >= args.parallel:
        for p in procs:
            if p.exitcode is not None:
                p.join()
                procs.remove(p)
        time.sleep(0.1)
