@echo off
rem Adventure 350 (TOPS-10).  -u, given here, lifts the cave hours, the
rem demonstration game's turn limit and the wait before a suspended game
rem may be resumed; run.bat ADVENT.SAV continues a suspended game.
"%~dp0advent350.exe" -u %*
