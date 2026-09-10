@echo off
REM Build ankh.exe with MinGW gcc (tested with gcc 15.2.0).
gcc -O2 -Wall -Wextra -o ankh.exe ankh.c
