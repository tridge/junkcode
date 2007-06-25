#!/bin/sh

( sleep 3; exit 7 ) || echo $? &

