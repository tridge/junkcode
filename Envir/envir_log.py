#!/usr/bin/env python

import serial, fcntl, sys, time

dev = sys.argv[1]
is_device = dev.startswith("/dev")

date_str = time.strftime("%Y-%m-%d")

if is_device:
	lock = open("envir.lck", mode="w")
	try:
		fcntl.flock(lock.fileno(),fcntl.LOCK_EX | fcntl.LOCK_NB)
	except:
		sys.exit(0)
	s = serial.Serial(dev, 57600, parity='N', rtscts=False,
			  xonxoff=False, timeout=1.0)
else:
	s = open(dev, mode="r")

while True:
    for line in s:
    	date_str = time.strftime("%Y-%m-%d")
    	time_str = time.strftime("%H:%M:%S")
	if is_device:
		f = open("../XML/%s.xml" % date_str, mode="a")
		f.write(line)
		f.close()
    if not is_device:
	    break
	
	
