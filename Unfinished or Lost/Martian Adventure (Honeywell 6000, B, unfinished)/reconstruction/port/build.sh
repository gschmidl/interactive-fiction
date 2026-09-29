#!/bin/sh
# Regenerate data.h from the archive, then build the game.
set -e
cd "$(dirname "$0")"
python gendata.py
gcc -O2 -Wall -Wextra -o mars.exe mars.c
echo "built mars.exe"
