@echo off
rem Rebuilds Hobbits.exe from the original lesson file.
rem The shared PLATO engine tree is not published; point PLATO_WORK at it.
set WORK=%PLATO_WORK%
if "%WORK%"=="" set WORK=%~dp0..\..\_PLATO_work
python "%WORK%\tools\mkgame.py" --title "Hobbits (Michael Jackson, PLATO 1980)" --out "%~dp0." --exe Hobbits.exe --lesson "%~dp0..\src_original\ciswork29.words"
