@echo off
rem CP-V Adventure (David Platt, Honeywell LADC, 1979).  advt.dat and
rem advi.dat - the cave, built by build.sh - must be here; SAVE writes
rem saves\advfreeze.dat.  The cave keeps the hours of 1979: pass -u to
rem play at any time, without the 600-move limit or the restore wait.
"%~dp0adv.exe" %*
