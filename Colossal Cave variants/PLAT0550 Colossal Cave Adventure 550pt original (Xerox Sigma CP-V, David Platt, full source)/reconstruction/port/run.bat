@echo off
rem CP-V Adventure (David Platt, Honeywell LADC, 1979).  advt.dat and
rem advi.dat - the cave, built by build.sh - must be here; SAVE writes
rem saves\advfreeze.dat.  -u, given here, opens the cave at any time,
rem without the 600-move limit or the restore wait; run adv.exe
rem directly for the hours of 1979.
"%~dp0adv.exe" -u %*
