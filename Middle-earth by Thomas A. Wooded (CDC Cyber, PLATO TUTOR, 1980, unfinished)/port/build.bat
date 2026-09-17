@echo off
rem Rebuilds MiddleEarth.exe from the original lesson file.
rem The shared PLATO engine tree is not published; point PLATO_WORK at it.
set WORK=%PLATO_WORK%
if "%WORK%"=="" set WORK=%~dp0..\..\_PLATO_work
python "%WORK%\tools\mkgame.py" --title "Middle-earth (Thomas A. Wooded, PLATO 1980)" --out "%~dp0." --exe MiddleEarth.exe --lesson "%~dp0..\src_original\midearth.words"
