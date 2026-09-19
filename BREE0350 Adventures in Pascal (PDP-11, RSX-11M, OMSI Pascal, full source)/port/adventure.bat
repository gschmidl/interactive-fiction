@echo off
rem Adventures in Pascal (Barry C. Breen, 1980-82) - double-click launcher.
rem Options are passed on: adventure.bat -u   ignores the cave hours.
rem                        adventure.bat --help
"%~dp0adventure.exe" %*
echo.
pause
