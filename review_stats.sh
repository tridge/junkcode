#!/bin/bash

if [ $# -ge 1 ]; then
    n="$1"
else
    n=1
fi

if [ $# -ge 2 ]; then
    committers="$2"
else
    committers=$(git log --since="$n months ago" |grep ^Author| sort -u | cut -d'<' -f2 | cut -d'>' -f1)
fi

for c in $committers; do
    patch_count=$(git log --since="$n months ago" --author="$c" |grep ^Author|wc -l)
    review_count=$(git log --since="$n months ago" --author="$c" |egrep 'Signed.off|Review|Pair.Programmed' |wc -l)
    review_pct=$(expr 100 \* $review_count / $patch_count)
    echo "$review_pct% $patch_count $review_count $c"
done
