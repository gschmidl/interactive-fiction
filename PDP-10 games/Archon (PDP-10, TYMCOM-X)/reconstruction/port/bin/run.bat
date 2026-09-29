@echo off
rem ARCHON.  -u, given here, lifts the game's hours, the turn limit and
rem the wait before a suspended game may be resumed; -m keeps wandering
rem creatures out of the map's two slips; run.bat -c continues a
rem suspended game.
"%~dp0archon.exe" -u -m %*
