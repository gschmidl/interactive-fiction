@echo off
REM Build tower.exe with MinGW gcc (tested with gcc 15.2.0).
gcc -O2 -Wall -Wextra -o tower.exe tower.c
