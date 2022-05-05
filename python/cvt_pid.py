#!/usr/bin/env python

import sys

if len(sys.argv) < 4:
    print("Usage: P I D tau")
    sys.exit(1)

P = float(sys.argv[1])
I = float(sys.argv[2])
D = float(sys.argv[3])
tau = float(sys.argv[4])

print('Old PID: P=%.2f I=%.2f D=%.2f TCONST=%.2f' % (P,I,D,tau))

kp_ff = max((P - I * tau) * tau  - D, 0)

print("RATE_FF=%.2f" % kp_ff)
print("RATE_P=%.2f" % D)
print("RATE_I=%.2f" % (I*tau))
print("RATE_D=0.00")
print("TCONST=%.2f" % tau)


