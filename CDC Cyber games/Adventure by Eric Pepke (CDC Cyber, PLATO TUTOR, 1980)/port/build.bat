@echo off
rem Rebuilds Adventure.exe from the original lesson file and the rebuilt dataset.
rem The shared PLATO engine tree is not published; point PLATO_WORK at it.
set WORK=%PLATO_WORK%
if "%WORK%"=="" set WORK=%~dp0..\..\_PLATO_work
python "%WORK%\tools\adventure_data.py" "%~dp0..\src_original\adventure.words" "%~dp0data\adventds.dataset"
python "%WORK%\tools\mkgame.py" --title "Adventure (Eric Pepke, PLATO 1980)" --out "%~dp0." --exe Adventure.exe --lesson "%~dp0..\src_original\adventure.words" --dataset "adventds=%~dp0data\adventds.dataset"
