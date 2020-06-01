#!/usr/bin/env python

import sys, subprocess, random, os, time

files = sys.argv[1:]
nfiles = len(files)

while True:
      r = random.randint(0, nfiles-1)
      fname = files[r]
      reduced = os.path.join('reduced', fname)
      if os.path.exists(reduced):
         print("Skipping %s" % fname)
         time.sleep(0.1)
         continue

      print("Generating %s" % reduced)
      tmp = reduced+".tmp"
      try:
          os.unlink(tmp)
      except Exception:
          pass
      ret = subprocess.call(['nice', 'mavlogdump.py', '--reduce=20', '-q', '-o', tmp, fname])
      if ret == 0:
          try:
              os.unlink(reduced)
          except Exception:
              pass
          os.rename(tmp, reduced)
