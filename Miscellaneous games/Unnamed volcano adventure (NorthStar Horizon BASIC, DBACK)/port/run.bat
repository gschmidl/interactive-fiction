@echo off
rem The unnamed volcano adventure (North Star BASIC file DBACK), run by the
rem North Star BASIC on the same disk.  Moves are single letters: U D B R F T,
rem and INVENTORY.  A wrong move on the slopes or at the rim is fatal: see
rem WALKTHROUGH.md (with --seed 5, twelve moves always reach the last ending).
"%~dp0volcano.exe" %*
