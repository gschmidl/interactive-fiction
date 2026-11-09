@echo off
rem Adventure for the HP 9825 (Eric Nelson and Craig Robinson, HP Desktop
rem Computer Division, 1980) - Windows console port.  advent.exe needs no
rem other files; a suspended game goes to saves\ADVSAV next to it.
rem Add --no-fixes to play with the original's restore bug.
"%~dp0advent.exe" %*
