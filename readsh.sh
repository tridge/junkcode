#!/bin/sh

FILE="$1"

(
  while read v; do
    echo "Got $v";
  done
) < $FILE


