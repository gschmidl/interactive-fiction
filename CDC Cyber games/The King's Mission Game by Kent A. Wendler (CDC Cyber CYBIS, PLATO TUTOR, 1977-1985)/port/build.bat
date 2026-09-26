@echo off
rem Rebuilds KingsMission.exe from the original lesson files and dataset.
rem The shared PLATO engine tree is not published; point PLATO_WORK at it.
set WORK=%PLATO_WORK%
if "%WORK%"=="" set WORK=%~dp0..\..\_PLATO_work
python "%WORK%\tools\mkgame.py" --title "The King's Mission Game (Kent A. Wendler, PLATO)" --out "%~dp0." --exe KingsMission.exe --lesson "%~dp0..\src_original\2tkm.words" --lesson "%~dp0..\src_original\2tkmb.words" --dataset "2tkmds=%~dp0..\src_original\2tkmds.dataset"
