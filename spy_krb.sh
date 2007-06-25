#!/bin/sh

UDP="88"

for p in $UDP; do
    ./udpproxy $p 192.168.114.5 $p &
done
