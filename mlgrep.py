#!/usr/bin/env python

import sys, string

def load_file(filename):
    '''return contents of a file'''
    try:
        f = open(filename, 'r')
        r = f.read()
        f.close()
    except:
        return None
    return r


argv = sys.argv

if len(argv) < 4:
    print('Usage: mlgrep.py PATTERN1 PATTERN2 <FILE..>')
    sys.exit(1)

pattern1 = argv[1]
pattern2 = argv[2]

for i in range(3,len(argv)):
    d = load_file(argv[i])

    min = 0

    while True:
        start = string.find(d, pattern1, min)
        if start == -1:
            break
        end = string.find(d, pattern2, start+1)
        if end == -1:
            break
        end = end + len(pattern2)
        print "%s" % d[start:end]        
        min = end+1
