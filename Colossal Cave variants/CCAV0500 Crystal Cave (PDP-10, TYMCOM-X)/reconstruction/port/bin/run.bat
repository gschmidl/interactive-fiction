@echo off
rem Crystal Cave (1979).  -u, given here, lifts the cave hours, the short
rem expedition's turn limit and the wait before a suspended game may be
rem resumed; run.bat -c continues a suspended game.  cave-1983.exe and
rem cave-1984.exe are the later versions (give them -u too).
"%~dp0cave.exe" -u %*
