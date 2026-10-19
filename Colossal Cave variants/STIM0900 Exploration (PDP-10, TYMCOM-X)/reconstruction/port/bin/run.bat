@echo off
rem Exploration.  -u, given here, lifts the cave hours, the 900-turn limit
rem and the wait before a suspended game may be resumed; run.bat -c
rem continues a suspended game.
"%~dp0explor.exe" -u %*
