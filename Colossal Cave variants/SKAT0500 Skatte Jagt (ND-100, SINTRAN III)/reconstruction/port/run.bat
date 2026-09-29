@echo off
rem Skattejakt (ND-100, SINTRAN III).  -u, given here, lets a suspended game
rem (SPAR) go on at once instead of 90 minutes later; run skattejakt.exe
rem directly for the wait.  run.bat NAME continues the game saved as NAME.
"%~dp0skattejakt.exe" -u %*
