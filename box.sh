#!/bin/sh

esc="\033["

echo -e "\033[40;38;1mX\033[0m"

exit 0
echo -n " _ _ _ _ _40 _ _ _ 41_ _ _ _42 _ _ _ 43"
  echo "_ _ _ 44_ _ _ _45 _ _ _ 46_ _ _ _47 _"
  for fore in 38 39 40 41 30 31 32 33 34 35 36 37; do
    line1="$fore  "
    line2="    "
    for back in 40 41 42 43 44 45 46 47; do
      line1="${line1}${esc}${back};${fore}m Normal  ${esc}0m"
      line2="${line2}${esc}${back};${fore};1m Bold    ${esc}0m"
    done
    echo -e "$line1\n$line2"
  done


