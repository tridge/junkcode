#!/usr/bin/env python

import sys

if len(sys.argv) < 4:
    print("Usage: P I D tau")
    sys.exit(1)

P = float(sys.argv[1])
I = float(sys.argv[2])
D = float(sys.argv[3])
tau = float(sys.argv[4])

print(P,I,D,tau)

kp_ff = max((P - I * tau) * tau  - D, 0)

print("FF=%f" % kp_ff)
