#!/bin/sh

UDP="389"
TCP="135 139 389 445 1024 1025 1026"

killall udpproxy
killall sockspy

for p in $UDP; do
    ./udpproxy $p 192.168.114.5 $p &
done

for p in $TCP; do
    while ./sockspy $p 192.168.114.5 $p; do date; done &
done
