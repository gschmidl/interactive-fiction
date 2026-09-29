@echo off
rem Adventures in Pascal (Barry C. Breen, 1980-82) - double-click launcher.
rem -u, given here, ignores the cave hours and the resume wait and answers
rem the wizard test; run adventure.exe directly for the original rules.
rem Other options are passed on: run.bat --help
"%~dp0adventure.exe" -u %*
echo.
pause
