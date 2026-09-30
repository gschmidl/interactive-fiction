@echo off
rem 382-point Adventure, 1979 version.  -u, given here, lifts the cave
rem hours, the short expedition's turn limit and the wait before a
rem suspended game may be resumed; run.bat -c continues a suspended game.
rem advent-1978.exe and advent-1981.exe are the other versions (give them
rem -u too).
"%~dp0advent-1979.exe" -u %*
