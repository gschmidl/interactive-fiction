@echo off
rem Rebuilds MiddleEarth.exe from the original lesson file.
set WORK=D:\tools\IFBackup\_PLATO_work
python "%WORK%\tools\mkgame.py" --title "Middle-earth (Thomas A. Wooded, PLATO 1980)" --out "%~dp0." --exe MiddleEarth.exe --lesson "%~dp0..\src_original\midearth.words"
