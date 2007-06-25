#!/bin/sh

while :; do 
[ 1 -eq 1 ] && {
    echo "failing"
    /bin/false || exit 3
}
done

echo "oh no"

