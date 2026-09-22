@echo off
rem Adventure 366 as it ran under NOS 1.3 at ACCA (CDC FORTRAN Extended).
rem adventure.txt - TAPE1, the text database - must be here; build.sh puts
rem it there.  The game is shut 06.00-11.30 and 13.30-15.30 unless you say
rem you are a wizard (the password is WORMTONGUE); --time HHMM holds the
rem clock still.
"%~dp0advent.exe" %*
