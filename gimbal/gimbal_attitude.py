#!/usr/bin/env python

from pymavlink import rotmat
from math import radians, degrees
import sys

def calculate_attitude(encoders, flight_control_attitude):
    '''calculate attitude from 312 encoders and 321 flight_control_attitude
       returns 321 euler attitude
    '''
    m = rotmat.Matrix3()
    m.from_euler(flight_control_attitude[0],flight_control_attitude[1],flight_control_attitude[2])
    m.rotate_312(encoders[0],encoders[1],encoders[2])
    return m.to_euler()
    

tests = [
    #  FC_Att_Deg      Encoders_Deg      ATT_Out_Deg
    (( 16, -29, -54 ), ( 0,    0,   0 ), (  16,        -29,        -54 ) ),
    (( 0, 0, 0 ),      ( 0,  -29, -54 ), (   0,        -29,        -54 ) ),
    (( 0, 0, 73 ),     ( 0,  -29, -54 ), (   0,        -29,         19 ) ),
    (( 0, 0, 0 ),      ( 16, -29, -54 ), (  18.151808, -27.776832, -62.686943 ) ),
    (( 10, 15,  165 ), ( 15, 24, -123 ), (  -3.243347,  23.949174,  39.364223 ) ),
    ((-37, 62, -175 ), ( 84, 39,   12 ), ( 142.790446,  53.089256, -69.374480 ) ),
]

for i in range(len(tests)):
    t = tests[i]
    fc_att, encoders, att_out = t
    r,p,y = calculate_attitude(
        (radians(encoders[0]),radians(encoders[1]),radians(encoders[2])),
        (radians(fc_att[0]),radians(fc_att[1]),radians(fc_att[2])))
    err = abs(degrees(r)-att_out[0])
    err = max(err, abs(degrees(p)-att_out[1]))
    err = max(err, abs(degrees(y)-att_out[2]))
    if err > 0.001:
        print("Test[%u] err=%.3f (%.6f, %.6f, %.6f)" % (i, err, degrees(r), degrees(p), degrees(y)))
        sys.exit(1)
print("All OK (python)")

