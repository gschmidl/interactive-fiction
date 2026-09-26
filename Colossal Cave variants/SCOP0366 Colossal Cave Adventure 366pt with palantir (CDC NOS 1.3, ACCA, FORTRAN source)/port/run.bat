@echo off
rem Adventure 366 as it ran under NOS 1.3 at ACCA (CDC FORTRAN Extended).
rem adventure.txt - TAPE1, the text database - must be here; build.sh puts
rem it there.  -u turns the clock off, so the cave's hours (shut
rem 06.00-11.30 and 13.30-15.30 unless you were the wizard) do not apply;
rem --time HHMM holds the clock still.
"%~dp0advent.exe" -u %*
