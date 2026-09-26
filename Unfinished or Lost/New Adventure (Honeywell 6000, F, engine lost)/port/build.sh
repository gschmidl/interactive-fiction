#!/bin/sh
# newadv -- walkable map of the recovered "New Adventure" locations.
set -e
python gendata.py
gcc -O2 -Wall -Wextra -o newadv newadv.c
echo "built: ./newadv"
