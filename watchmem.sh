#!/bin/sh

vmstat 1| awk '{print $0"     "$4 + $5 + $6 - $3}'
