@echo off
rem ARCHON.  -u, given here, lifts the game's hours, the turn limit and
rem the wait before a suspended game may be resumed; -m keeps wandering
rem creatures out of the map's two slips; -b mends the slips in the
rem game's own data (the barrow wight's tomb, the maze, and others);
rem run.bat -c continues a suspended game.
"%~dp0archon.exe" -u -m -b %*
