@echo off
rem ARCHON.  -u, given here, lifts the game's hours, the turn limit and
rem the wait before a suspended game may be resumed; -m keeps wandering
rem creatures out of the map's two slips; -b lets you out of the
rem barrow wight's tomb once the wight is dead; run.bat -c continues
rem a suspended game.
"%~dp0archon.exe" -u -m -b %*
